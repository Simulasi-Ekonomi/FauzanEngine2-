#include "Runtime/EditorSceneDocument.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace NeoEngine {
namespace {
bool ValidSceneId(const std::string& value) {
    if (value.empty() || value.size() > 48) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
               (character >= '0' && character <= '9') || character == '_' || character == '-';
    });
}

bool ValidTransform(const Transform3& transform) {
    return std::isfinite(transform.x) && std::isfinite(transform.y) && std::isfinite(transform.z) &&
           std::isfinite(transform.rx) && std::isfinite(transform.ry) && std::isfinite(transform.rz) &&
           std::isfinite(transform.sx) && std::isfinite(transform.sy) && std::isfinite(transform.sz) &&
           transform.sx > 0.0F && transform.sy > 0.0F && transform.sz > 0.0F;
}

bool ValidActorKind(EditorSceneActorKind kind) {
    return kind == EditorSceneActorKind::Empty || kind == EditorSceneActorKind::Mesh ||
           kind == EditorSceneActorKind::Light || kind == EditorSceneActorKind::Camera ||
           kind == EditorSceneActorKind::PlayerStart || kind == EditorSceneActorKind::Marker;
}
} // namespace

bool EditorSceneDocumentAdapter::Fail(EditorSceneDocumentError error) {
    lastError_ = error;
    return false;
}

const SceneEntity* EditorSceneDocumentAdapter::EntityForActor(uint32_t actorId) const {
    const auto found = std::find_if(bindings_.begin(), bindings_.end(), [actorId](const Binding& binding) { return binding.actorId == actorId; });
    return found == bindings_.end() ? nullptr : &found->entity;
}

bool EditorSceneDocumentAdapter::Load(const EditorSceneDocument& document, const AssetRegistry& assets, SceneWorld& target) {
    if (document.version != EditorSceneDocument::kVersion) return Fail(EditorSceneDocumentError::UnsupportedVersion);
    if (!ValidSceneId(document.sceneId)) return Fail(EditorSceneDocumentError::InvalidSceneId);
    if (document.revision == 0) return Fail(EditorSceneDocumentError::InvalidRevision);
    if (document.actors.size() > kMaxActors || document.actors.size() > SceneWorld::kCapacity) return Fail(EditorSceneDocumentError::Capacity);

    std::unordered_set<uint32_t> ids;
    ids.reserve(document.actors.size());
    for (const EditorSceneActor& actor : document.actors) {
        if (actor.id == 0 || !ids.insert(actor.id).second) return Fail(EditorSceneDocumentError::DuplicateActorId);
        if (!ValidActorKind(actor.kind) || !ValidTransform(actor.transform)) return Fail(EditorSceneDocumentError::InvalidActor);
        if (!actor.assetId.empty()) {
            const AssetDefinition* asset = assets.Find(actor.assetId);
            if (asset == nullptr) return Fail(EditorSceneDocumentError::MissingAsset);
            if (asset->state != AssetState::Ready) return Fail(EditorSceneDocumentError::AssetNotReady);
            if (asset->kind != AssetKind::Mesh && asset->kind != AssetKind::Prefab) return Fail(EditorSceneDocumentError::AssetKindMismatch);
        }
    }
    for (const EditorSceneActor& actor : document.actors) {
        if (actor.parentId != 0 && (actor.parentId == actor.id || !ids.contains(actor.parentId))) return Fail(actor.parentId == actor.id ? EditorSceneDocumentError::InvalidHierarchy : EditorSceneDocumentError::MissingParent);
    }

    SceneWorld candidate;
    std::unordered_map<uint32_t, SceneEntity> entities;
    entities.reserve(document.actors.size());
    std::vector<Binding> candidateBindings;
    candidateBindings.reserve(document.actors.size());
    for (const EditorSceneActor& actor : document.actors) {
        SceneEntity entity{};
        if (!candidate.Create(entity) || !candidate.SetTransform(entity, actor.transform)) return Fail(EditorSceneDocumentError::SceneSyncFailed);
        entities.emplace(actor.id, entity);
        candidateBindings.push_back({actor.id, entity});
    }
    for (const EditorSceneActor& actor : document.actors) {
        if (actor.parentId != 0 && !candidate.SetParent(entities.at(actor.id), entities.at(actor.parentId))) return Fail(EditorSceneDocumentError::InvalidHierarchy);
    }
    if (!candidate.UpdateTransforms()) return Fail(EditorSceneDocumentError::SceneSyncFailed);

    target = candidate;
    bindings_ = std::move(candidateBindings);
    lastError_ = EditorSceneDocumentError::None;
    return true;
}

} // namespace NeoEngine
