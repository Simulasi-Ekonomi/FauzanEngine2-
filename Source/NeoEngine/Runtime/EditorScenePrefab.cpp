#include "Runtime/EditorScenePrefab.h"

#include <algorithm>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace NeoEngine {
namespace {
bool BuildActorIndex(const std::vector<EditorSceneActor>& actors, std::unordered_map<uint32_t, const EditorSceneActor*>& index) {
    index.clear();
    index.reserve(actors.size());
    for (const EditorSceneActor& actor : actors) {
        if (actor.id == 0U || !index.emplace(actor.id, &actor).second) return false;
    }
    return true;
}

bool ReachesRoot(uint32_t actorId, uint32_t rootActorId, const std::unordered_map<uint32_t, const EditorSceneActor*>& index) {
    uint32_t current = actorId;
    for (size_t steps = 0U; steps <= index.size(); ++steps) {
        if (current == rootActorId) return true;
        const auto found = index.find(current);
        if (found == index.end() || found->second->parentId == 0U) return false;
        current = found->second->parentId;
    }
    return false;
}

bool ValidPrefab(const EditorScenePrefab& prefab) {
    if (prefab.rootSourceId == 0U || prefab.actors.empty() || prefab.actors.size() > EditorScenePrefabAdapter::kMaxActors) return false;
    std::unordered_map<uint32_t, const EditorSceneActor*> index;
    if (!BuildActorIndex(prefab.actors, index)) return false;
    const auto root = index.find(prefab.rootSourceId);
    if (root == index.end() || root->second->parentId != 0U) return false;
    for (const EditorSceneActor& actor : prefab.actors) {
        if (actor.id != prefab.rootSourceId && (actor.parentId == 0U || actor.parentId == actor.id || !index.contains(actor.parentId) || !ReachesRoot(actor.id, prefab.rootSourceId, index))) return false;
    }
    return true;
}
} // namespace

bool EditorScenePrefabAdapter::Fail(EditorScenePrefabError error) { lastError_ = error; return false; }

bool EditorScenePrefabAdapter::Capture(const EditorSceneDocument& document, uint32_t rootActorId, EditorScenePrefab& target) {
    if (document.version < EditorSceneDocument::kMinSupportedVersion || document.version > EditorSceneDocument::kVersion || document.revision == 0U || document.actors.empty() || document.actors.size() > EditorSceneDocumentAdapter::kMaxActors) return Fail(EditorScenePrefabError::InvalidDocument);
    std::unordered_map<uint32_t, const EditorSceneActor*> index;
    if (!BuildActorIndex(document.actors, index)) return Fail(EditorScenePrefabError::InvalidDocument);
    if (!index.contains(rootActorId)) return Fail(EditorScenePrefabError::SourceNotFound);

    EditorScenePrefab candidate{};
    candidate.rootSourceId = rootActorId;
    candidate.actors.reserve(document.actors.size());
    for (const EditorSceneActor& actor : document.actors) {
        if (ReachesRoot(actor.id, rootActorId, index)) {
            EditorSceneActor captured = actor;
            if (captured.id == rootActorId) captured.parentId = 0U;
            candidate.actors.push_back(std::move(captured));
        }
    }
    if (!ValidPrefab(candidate)) return Fail(EditorScenePrefabError::InvalidSubtree);
    target = std::move(candidate);
    lastError_ = EditorScenePrefabError::None;
    return true;
}

bool EditorScenePrefabAdapter::AppendInstance(const EditorSceneDocument& document, const EditorScenePrefab& prefab, uint32_t parentActorId, const std::vector<uint32_t>& instanceActorIds, EditorSceneDocument& target) {
    if (document.version < EditorSceneDocument::kMinSupportedVersion || document.version > EditorSceneDocument::kVersion || document.revision == 0U || document.revision == std::numeric_limits<uint64_t>::max() || document.actors.size() > EditorSceneDocumentAdapter::kMaxActors) return Fail(EditorScenePrefabError::InvalidDocument);
    if (!ValidPrefab(prefab)) return Fail(EditorScenePrefabError::InvalidSubtree);
    if (instanceActorIds.size() != prefab.actors.size() || document.actors.size() + prefab.actors.size() > EditorSceneDocumentAdapter::kMaxActors) return Fail(EditorScenePrefabError::Capacity);

    std::unordered_map<uint32_t, const EditorSceneActor*> documentIndex;
    if (!BuildActorIndex(document.actors, documentIndex)) return Fail(EditorScenePrefabError::InvalidDocument);
    if (parentActorId != 0U && !documentIndex.contains(parentActorId)) return Fail(EditorScenePrefabError::ParentNotFound);

    std::unordered_map<uint32_t, uint32_t> idRemap;
    idRemap.reserve(prefab.actors.size());
    std::unordered_set<uint32_t> instanceIds;
    instanceIds.reserve(prefab.actors.size());
    for (size_t index = 0U; index < prefab.actors.size(); ++index) {
        const uint32_t instanceId = instanceActorIds[index];
        if (instanceId == 0U || documentIndex.contains(instanceId) || !instanceIds.insert(instanceId).second || !idRemap.emplace(prefab.actors[index].id, instanceId).second) return Fail(EditorScenePrefabError::InvalidMapping);
    }

    EditorSceneDocument candidate = document;
    candidate.actors.reserve(document.actors.size() + prefab.actors.size());
    for (const EditorSceneActor& source : prefab.actors) {
        EditorSceneActor instance = source;
        instance.id = idRemap.at(source.id);
        instance.parentId = source.id == prefab.rootSourceId ? parentActorId : idRemap.at(source.parentId);
        candidate.actors.push_back(std::move(instance));
    }
    ++candidate.revision;
    target = std::move(candidate);
    lastError_ = EditorScenePrefabError::None;
    return true;
}

} // namespace NeoEngine
