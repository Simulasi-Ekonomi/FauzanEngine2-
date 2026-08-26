#include "Runtime/EditorSceneSession.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "prefab-session", 1U, {
        {1U, 0U, EditorSceneActorKind::Empty, {1, 0, 0, 0, 0, 0, 1, 1, 1}},
        {2U, 1U, EditorSceneActorKind::Marker, {2, 0, 0, 0, 0, 0, 1, 1, 1}},
        {10U, 0U, EditorSceneActorKind::Empty, {10, 0, 0, 0, 0, 0, 1, 1, 1}},
    }};
    AssetRegistry assets;
    EditorSceneSession session;
    if (!session.Open(document, assets)) return 1;

    EditorScenePrefab prefab{};
    if (!session.CapturePrefab(1U, prefab) || session.LastError() != EditorSceneSessionError::None || prefab.rootSourceId != 1U || prefab.actors.size() != 2U || prefab.actors[0].id != 1U || prefab.actors[1].id != 2U || prefab.actors[1].parentId != 1U) return 1;
    const EditorScenePrefab preservedPrefab = prefab;
    if (session.CapturePrefab(99U, prefab) || session.LastError() != EditorSceneSessionError::InvalidDocument || prefab.rootSourceId != preservedPrefab.rootSourceId || prefab.actors.size() != preservedPrefab.actors.size()) return 1;

    if (!session.InstantiatePrefab(prefab, 10U, {100U, 101U}, assets) || !session.CanUndo()) return 1;
    EditorSceneDocument saved{};
    EditorSceneActor root{};
    EditorSceneActor child{};
    if (!session.Save(saved) || saved.revision != 2U || saved.actors.size() != 5U || !session.InspectActor(100U, root) || !session.InspectActor(101U, child) || root.parentId != 10U || child.parentId != 100U) return 1;
    const uint32_t preservedCount = session.World().AliveCount();
    const uint64_t preservedRevision = saved.revision;

    if (session.InstantiatePrefab(prefab, 10U, {102U, 102U}, assets) || session.LastError() != EditorSceneSessionError::InvalidDocument || !session.Save(saved) || saved.revision != preservedRevision || saved.actors.size() != 5U || session.World().AliveCount() != preservedCount) return 1;
    EditorScenePrefab malformed = prefab;
    malformed.actors[1].parentId = 777U;
    if (session.InstantiatePrefab(malformed, 10U, {102U, 103U}, assets) || session.LastError() != EditorSceneSessionError::InvalidDocument || !session.Save(saved) || saved.revision != preservedRevision || saved.actors.size() != 5U || session.World().AliveCount() != preservedCount) return 1;

    if (!session.Undo(assets) || !session.Save(saved) || saved.revision != 1U || saved.actors.size() != 3U || !session.Redo(assets) || !session.Save(saved) || saved.revision != 2U || saved.actors.size() != 5U) return 1;
    std::printf("EDITOR_SCENE_PREFAB_SMOKE_OK capture=1 remap=1 atomic=1 history=1 actors=%u\n", session.World().AliveCount());
    return 0;
}
