#include "Runtime/EditorSceneSession.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{'P','6','\n','1',' ','1','\n','2','5','5','\n',90U,230U,140U};
    AssetRegistry assets;
    if (!assets.ImportBytes("revert.sprite", AssetKind::Texture, {}, ppm) || !assets.MarkReady("revert.sprite")) return 1;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "revert-session", 1U, {{1U,0U,EditorSceneActorKind::Empty,{}}, {2U,1U,EditorSceneActorKind::Sprite,{0,0,3,0,0,0,1,1,1},"revert.sprite","","","",1,1,0,0,0xFFFFFFFFU}}};
    EditorSceneSession session;
    if (!session.Open(document, assets) || session.HasUnsavedChanges()) return 1;
    if (!session.UpdateTransform(2U, {2,0,3,0,0,0,1,1,1}, assets) || !session.HasUnsavedChanges() || !session.RevertToSaved(assets) || session.HasUnsavedChanges()) return 1;
    EditorSceneActor actor{};
    if (!session.InspectActor(2U, actor) || actor.transform.x != 0.0F) return 1;
    if (!session.UpdateTransform(2U, {1,0,3,0,0,0,1,1,1}, assets) || !session.HasUnsavedChanges()) return 1;
    EditorSceneDocument saved{};
    if (!session.Save(saved) || saved.revision != 2U || session.HasUnsavedChanges() || !session.AddActor({3U,0U,EditorSceneActorKind::Empty,{}}, assets) || !session.HasUnsavedChanges() || !session.RevertToSaved(assets) || session.HasUnsavedChanges() || session.HierarchySnapshot().size() != 2U) return 1;
    if (!session.UpdateTransform(2U, {4,0,3,0,0,0,1,1,1}, assets) || !session.HasUnsavedChanges() || !assets.ReplaceBytes("revert.sprite", {'x'}) || session.RevertToSaved(assets) || session.LastError() != EditorSceneSessionError::SpriteBindFailed || !session.HasUnsavedChanges() || !session.InspectActor(2U, actor) || actor.transform.x != 4.0F) return 1;
    EditorSceneSession empty;
    if (empty.RevertToSaved(assets) || empty.LastError() != EditorSceneSessionError::InvalidDocument) return 1;
    std::printf("EDITOR_SCENE_SESSION_REVERT_SMOKE_OK restore=1 savedSnapshot=1 failureAtomic=1 revision=%llu\n", static_cast<unsigned long long>(saved.revision));
    return 0;
}
