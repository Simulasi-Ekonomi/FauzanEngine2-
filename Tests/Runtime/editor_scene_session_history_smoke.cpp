#include "Runtime/EditorSceneSession.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U};
    AssetRegistry assets;
    if (!assets.ImportBytes("history.sprite", AssetKind::Texture, {}, ppm) || !assets.MarkReady("history.sprite")) return 1;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "history-session", 1U, {{1U,0U,EditorSceneActorKind::Empty,{}}, {2U,1U,EditorSceneActorKind::Sprite,{0,0,3,0,0,0,1,1,1},"history.sprite","","","",1,1,0,0,0xFFFFFFFFU}}};
    EditorSceneSession session;
    if (!session.Open(document, assets) || session.CanUndo() || session.CanRedo()) return 1;
    if (!session.UpdateTransform(2U, {1,0,3,0,0,0,1,1,1}, assets) || !session.ReparentActor(2U, 0U, assets) || !session.CanUndo() || session.CanRedo()) return 1;
    EditorSceneActor actor{};
    if (!session.Undo(assets) || !session.CanUndo() || !session.CanRedo() || !session.InspectActor(2U, actor) || actor.parentId != 1U || actor.transform.x != 1.0F) return 1;
    if (!session.Redo(assets) || !session.InspectActor(2U, actor) || actor.parentId != 0U || !session.Undo(assets) || !session.AddActor({3U,0U,EditorSceneActorKind::Empty,{}}, assets) || session.CanRedo() || session.HierarchySnapshot().size() != 3U) return 1;
    if (!assets.ReplaceBytes("history.sprite", {'x'}) || session.Undo(assets) || session.LastError() != EditorSceneSessionError::SpriteBindFailed || !session.CanUndo() || session.CanRedo() || session.HierarchySnapshot().size() != 3U) return 1;
    EditorSceneSession empty;
    if (empty.Undo(assets) || empty.LastError() != EditorSceneSessionError::HistoryUnavailable || empty.Redo(assets) || empty.LastError() != EditorSceneSessionError::HistoryUnavailable) return 1;
    std::printf("EDITOR_SCENE_SESSION_HISTORY_SMOKE_OK undo=1 redo=1 branchClear=1 failureAtomic=1\n");
    return 0;
}
