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
    doc.sceneId = "UnrealAgentAuthoringScene";
    doc.revision = 1;

    EditorSceneActor rootActor;
    rootActor.id = 1;
    rootActor.name = "SceneRoot";
    rootActor.kind = EditorSceneActorKind::Marker;
    doc.actors.push_back(rootActor);

    EditorSceneSession session;
    if (!session.Open(doc, assets) || session.Document().sceneId != "UnrealAgentAuthoringScene") {
        std::printf("ERR: Session Open failed\n");
        return 1;
    }

    EditorSceneActor lightActor;
    lightActor.id = 2;
    lightActor.parentId = 1;
    lightActor.name = "MainLight";
    lightActor.kind = EditorSceneActorKind::Light;
    if (!session.AddActor(lightActor, assets)) {
        std::printf("ERR: AddActor Light failed\n");
        return 1;
    }

    EditorSceneActor meshActor;
    meshActor.id = 3;
    meshActor.parentId = 1;
    meshActor.name = "HeroCube";
    meshActor.kind = EditorSceneActorKind::Mesh;
    meshActor.transform = {10.0f, 20.0f, 0.0f, 0, 0, 0, 1, 1, 1};
    if (!session.AddActor(meshActor, assets)) {
        std::printf("ERR: AddActor Mesh failed\n");
        return 1;
    }

    if (!session.UpdateTransform(3, {15.0f, 25.0f, 5.0f, 0, 0, 0, 1, 1, 1}, assets)) {
        std::printf("ERR: UpdateTransform failed\n");
        return 1;
    }

    if (!session.DuplicateActor(3, 4, assets)) {
        std::printf("ERR: DuplicateActor failed\n");
        return 1;
    }

    if (!session.MultiSelectActors({3, 4}) || session.SelectedActorIds().size() != 2) {
        std::printf("ERR: MultiSelectActors failed\n");
        return 1;
    }

    if (!session.CanUndo()) {
        std::printf("ERR: CanUndo should be true\n");
        return 1;
    }

    if (!session.Undo(assets) || session.HierarchySnapshot().size() != 3) {
        std::printf("ERR: Undo failed\n");
        return 1;
    }

    if (!session.Redo(assets) || session.HierarchySnapshot().size() != 4) {
        std::printf("ERR: Redo failed\n");
        return 1;
    }

    std::string spawnJson = R"({"cmd":"SPAWN","actorId":5,"parentId":1,"kind":3,"name":"AgentCamera","x":0.0,"y":5.0,"z":10.0})";
    auto agentResp = EditorAgentAPI::ProcessJsonCommand(session, assets, spawnJson);
    if (!agentResp.success || agentResp.actorCount != 5) {
        std::printf("ERR: Agent SPAWN failed\n");
        return 1;
    }

    std::string queryJson = R"({"cmd":"QUERY"})";
    auto queryResp = EditorAgentAPI::ProcessJsonCommand(session, assets, queryJson);
    if (!queryResp.success || queryResp.jsonPayload.empty()) {
        std::printf("ERR: Agent QUERY failed\n");
        return 1;
    }

    std::vector<uint8_t> sceneBytes;
    if (!session.SaveBytes(sceneBytes) || sceneBytes.empty()) {
        std::printf("ERR: SaveBytes failed\n");
        return 1;
    }

    EditorSceneSession reloadedSession;
    if (!reloadedSession.OpenBytes(sceneBytes, assets) || reloadedSession.HierarchySnapshot().size() != 5) {
        std::printf("ERR: OpenBytes reloaded session failed\n");
        return 1;
    }

    std::printf("EDITOR_SUBSYSTEM_SMOKE_OK actors=%zu revision=%llu json_len=%zu\n",
                reloadedSession.HierarchySnapshot().size(),
                static_cast<unsigned long long>(reloadedSession.Document().revision),
                queryResp.jsonPayload.size());
    return 0;
}
