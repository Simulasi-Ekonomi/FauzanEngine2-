#include "Runtime/EditorSceneSession.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "dirty-session", 1U, {{1U,0U,EditorSceneActorKind::Empty,{}}, {2U,1U,EditorSceneActorKind::Empty,{0,0,1,0,0,0,1,1,1}}}};
    AssetRegistry assets;
    EditorSceneSession session;
    if (!session.Open(document, assets) || session.HasUnsavedChanges()) return 1;
    if (!session.UpdateTransform(2U, {2,0,1,0,0,0,1,1,1}, assets) || !session.HasUnsavedChanges()) return 1;
    if (session.UpdateTransform(2U, {2,0,1,0,0,0,0,1,1}, assets) || !session.HasUnsavedChanges()) return 1;
    EditorSceneDocument saved{};
    if (!session.Save(saved) || session.HasUnsavedChanges() || saved.revision != 2U) return 1;
    if (!session.AddActor({3U,0U,EditorSceneActorKind::Empty,{}}, assets) || !session.HasUnsavedChanges()) return 1;
    std::vector<uint8_t> bytes;
    if (!session.SaveBytes(bytes) || bytes.empty() || session.HasUnsavedChanges()) return 1;
    std::vector<uint8_t> malformed = bytes; malformed[0] = 'X';
    if (session.OpenBytes(malformed, assets) || session.LastError() != EditorSceneSessionError::CodecDecodeFailed || session.HasUnsavedChanges()) return 1;
    if (!session.DeleteActor(3U, assets) || !session.HasUnsavedChanges()) return 1;
    EditorSceneDocument duplicate = document; duplicate.revision = 3U; duplicate.actors.push_back({1U,0U,EditorSceneActorKind::Empty,{}});
    if (session.Open(duplicate, assets) || session.LastError() != EditorSceneSessionError::DocumentLoadFailed || !session.HasUnsavedChanges()) return 1;
    if (!session.OpenBytes(bytes, assets) || session.HasUnsavedChanges() || !session.Save(saved) || saved.revision != 3U) return 1;
    std::printf("EDITOR_SCENE_SESSION_DIRTY_SMOKE_OK cleanOpen=1 dirtyMutation=1 saveClean=1 rollback=1 revision=%llu\n", static_cast<unsigned long long>(saved.revision));
    return 0;
}
