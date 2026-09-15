#include "Runtime/EditorSceneSession.h"
#include "Editor/EditorAgentAPI.h"
#include "Editor/EditorProvider.h"
#include "Runtime/AssetRegistry.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;

    AssetRegistry assets;
    EditorSceneDocument doc{};
    doc.sceneId = "UnrealRemediationScene";
    doc.revision = 1;

    EditorSceneActor rootActor;
    rootActor.id = 1;
    rootActor.name = "SceneRoot";
    rootActor.kind = EditorSceneActorKind::Marker;
    doc.actors.push_back(rootActor);

    EditorSceneSession session;
    if (!session.Open(doc, assets)) {
        std::printf("ERR: Session Open failed\n");
        return 1;
    }

    EditorSceneActor duplicateActor;
    duplicateActor.id = 1;
    duplicateActor.name = "DuplicateRoot";
    if (session.AddActor(duplicateActor, assets)) {
        std::printf("ERR: AddActor with duplicate ID should be rejected\n");
        return 1;
    }

    EditorSceneActor childA;
    childA.id = 10;
    childA.parentId = 1;
    childA.name = "Node_A";
    childA.kind = EditorSceneActorKind::Mesh;
    if (!session.AddActor(childA, assets)) {
        std::printf("ERR: AddActor Node_A failed\n");
        return 1;
    }

    EditorSceneActor childB;
    childB.id = 20;
    childB.parentId = 10;
    childB.name = "Node_B";
    childB.kind = EditorSceneActorKind::Mesh;
    if (!session.AddActor(childB, assets)) {
        std::printf("ERR: AddActor Node_B failed\n");
        return 1;
    }

    if (session.ReparentActor(10, 20, assets)) {
        std::printf("ERR: Reparenting forming hierarchy cycle should be rejected\n");
        return 1;
    }

    if (session.DuplicateActor(20, 10, assets)) {
        std::printf("ERR: DuplicateActor with colliding new ID should be rejected\n");
        return 1;
    }

    if (!session.DuplicateActor(20, 21, assets)) {
        std::printf("ERR: DuplicateActor failed\n");
        return 1;
    }

    if (!session.MultiSelectActors({10, 20, 21}) || session.SelectedActorIds().size() != 3) {
        std::printf("ERR: MultiSelectActors failed\n");
        return 1;
    }

    if (!session.DeleteActor(21, assets)) {
        std::printf("ERR: DeleteActor 21 failed\n");
        return 1;
    }

    if (session.SelectedActorIds().size() != 2) {
        std::printf("ERR: Selection pruning failed after deletion\n");
        return 1;
    }

    size_t actorCountBeforeUndo = session.HierarchySnapshot().size();
    if (!session.Undo(assets) || session.HierarchySnapshot().size() >= actorCountBeforeUndo) {
        std::printf("ERR: Undo failed\n");
        return 1;
    }

    if (!session.Redo(assets) || session.HierarchySnapshot().size() != actorCountBeforeUndo) {
        std::printf("ERR: Redo failed\n");
        return 1;
    }

    std::string escapedNameJson = R"({"cmd":"SPAWN","actorId":99,"parentId":1,"kind":1,"name":"Robot \"Boss\" \\ Special","x":12.5,"y":0.0,"z":-5.0})";
    auto agentResp = EditorAgentAPI::ProcessJsonCommand(session, assets, escapedNameJson);
    if (!agentResp.success || agentResp.jsonPayload.empty()) {
        std::printf("ERR: Agent SPAWN with escaped JSON string failed: %s\n", agentResp.error.c_str());
        return 1;
    }

    std::string malformedJson = R"({"cmd":"SPAWN", actorId: invalid_json})";
    auto errResp = EditorAgentAPI::ProcessJsonCommand(session, assets, malformedJson);
    if (errResp.success || errResp.error.empty()) {
        std::printf("ERR: Malformed JSON should return error response\n");
        return 1;
    }

    std::vector<uint8_t> bytes;
    if (!session.SaveBytes(bytes) || bytes.empty()) {
        std::printf("ERR: SaveBytes failed\n");
        return 1;
    }

    EditorSceneSession reloaded;
    if (!reloaded.OpenBytes(bytes, assets) || reloaded.HierarchySnapshot().size() != session.HierarchySnapshot().size()) {
        std::printf("ERR: OpenBytes roundtrip failed\n");
        return 1;
    }

    std::printf("EDITOR_SUBSYSTEM_SMOKE_OK actors=%zu revision=%llu json_len=%zu\n",
                reloaded.HierarchySnapshot().size(),
                static_cast<unsigned long long>(reloaded.Document().revision),
                agentResp.jsonPayload.size());
    return 0;
}
