#include "Runtime/EditorSceneSession.h"

#include <algorithm>
#include <limits>

namespace NeoEngine {
bool EditorSceneSession::Open(const EditorSceneDocument& document, const AssetRegistry& assets) {
    EditorSceneDocumentAdapter documentAdapter; SceneWorld world;
    if (!documentAdapter.Load(document, assets, world)) { lastError_ = EditorSceneSessionError::DocumentLoadFailed; return false; }
    MeshStagingStore meshStaging; MaterialStagingStore materialStaging; TextureStagingStore textureStaging; SceneMeshAdapter meshes; SceneSpriteAdapter sprites;
    const auto hasKind = [&document](EditorSceneActorKind kind) { return std::any_of(document.actors.begin(), document.actors.end(), [kind](const EditorSceneActor& actor) { return actor.kind == kind; }); };
    if (hasKind(EditorSceneActorKind::Mesh)) { EditorSceneMeshBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, meshStaging, materialStaging, textureStaging, meshes)) { lastError_ = EditorSceneSessionError::MeshBindFailed; return false; } }
    if (hasKind(EditorSceneActorKind::Sprite)) { EditorSceneSpriteBinder binder; if (!binder.BindDocumentAssets(document, documentAdapter, assets, textureStaging, sprites)) { lastError_ = EditorSceneSessionError::SpriteBindFailed; return false; } }
    document_ = document; world_ = std::move(world); documentAdapter_ = std::move(documentAdapter); meshes_ = std::move(meshes); sprites_ = std::move(sprites); renderer_ = {}; lastError_ = EditorSceneSessionError::None; return true;
}
bool EditorSceneSession::OpenBytes(const std::vector<uint8_t>& bytes, const AssetRegistry& assets) {
    EditorSceneDocument candidate{}; EditorSceneDocumentCodec codec;
    if (!codec.Decode(bytes, candidate)) { lastError_ = EditorSceneSessionError::CodecDecodeFailed; return false; }
    return Open(candidate, assets);
}
bool EditorSceneSession::UpdateTransform(uint32_t actorId, const Transform3& transform, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    found->transform = transform; ++candidate.revision;
    return Open(candidate, assets);
}
bool EditorSceneSession::ReparentActor(uint32_t actorId, uint32_t parentId, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    found->parentId = parentId; ++candidate.revision;
    return Open(candidate, assets);
}
bool EditorSceneSession::AddActor(const EditorSceneActor& actor, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; candidate.actors.push_back(actor); ++candidate.revision;
    return Open(candidate, assets);
}
bool EditorSceneSession::DeleteActor(uint32_t actorId, const AssetRegistry& assets) {
    if (document_.revision == 0U || document_.revision == std::numeric_limits<uint64_t>::max()) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocument candidate = document_; const auto found = std::find_if(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.id == actorId; });
    if (found == candidate.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; }
    if (std::any_of(candidate.actors.begin(), candidate.actors.end(), [actorId](const EditorSceneActor& actor) { return actor.parentId == actorId; })) { lastError_ = EditorSceneSessionError::ActorHasChildren; return false; }
    candidate.actors.erase(found); ++candidate.revision;
    return Open(candidate, assets);
}
bool EditorSceneSession::Save(EditorSceneDocument& document) const { if (document_.revision == 0U) return false; document = document_; return true; }
bool EditorSceneSession::SaveBytes(std::vector<uint8_t>& bytes) const {
    if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; }
    EditorSceneDocumentCodec codec;
    if (!codec.Encode(document_, bytes)) { lastError_ = EditorSceneSessionError::CodecEncodeFailed; return false; }
    lastError_ = EditorSceneSessionError::None; return true;
}
std::vector<EditorSceneActor> EditorSceneSession::HierarchySnapshot() const { std::vector<EditorSceneActor> snapshot = document_.actors; std::sort(snapshot.begin(), snapshot.end(), [](const EditorSceneActor& left, const EditorSceneActor& right) { return left.id < right.id; }); return snapshot; }
bool EditorSceneSession::InspectActor(uint32_t actorId, EditorSceneActor& actor) const { const auto found = std::find_if(document_.actors.begin(), document_.actors.end(), [actorId](const EditorSceneActor& candidate) { return candidate.id == actorId; }); if (found == document_.actors.end()) { lastError_ = EditorSceneSessionError::UnknownActor; return false; } actor = *found; lastError_ = EditorSceneSessionError::None; return true; }
bool EditorSceneSession::RenderViewport(RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light) { if (document_.revision == 0U) { lastError_ = EditorSceneSessionError::InvalidDocument; return false; } if (!renderer_.Draw(world_, meshes_, sprites_, camera, renderer, light)) { lastError_ = EditorSceneSessionError::ViewportRenderFailed; return false; } lastError_ = EditorSceneSessionError::None; return true; }
} // namespace NeoEngine
