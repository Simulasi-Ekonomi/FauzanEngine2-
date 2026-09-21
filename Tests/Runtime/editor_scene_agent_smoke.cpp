#include "Runtime/EditorSceneAgentAPI.h"

#include <iostream>
#include <string>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    using namespace NeoEngine;

    AssetRegistry assets;
    EditorSceneDocument document;
    document.sceneId = "agent-smoke";
    document.revision = 1;
    document.actors.push_back(EditorSceneActor{42, 0, EditorSceneActorKind::Empty, {}, "", "", "", "", 1.0F, 1.0F, 0, 0, 0xFFFFFFFFU, "Root"});

    EditorSceneSession session;
    TEST_CHECK(session.Open(document, assets), "session open failed");

    EditorSceneAgentAPI agent;
    std::string response;

    TEST_CHECK(agent.Execute(R"({"operation":"duplicate","actorId":42,"newActorId":44})", session, assets, response),
               "duplicate command failed");
    TEST_CHECK(response.find("\"ok\":true") != std::string::npos, "duplicate response was not successful");
    TEST_CHECK(session.HierarchySnapshot().size() == 2U, "duplicate did not add actor");
    const auto beforeInvalidMutation = session.HierarchySnapshot();
    EditorSceneActor beforeInvalidActor{};
    TEST_CHECK(session.InspectActor(44, beforeInvalidActor), "failed to snapshot actor before invalid mutation");
    TEST_CHECK(!agent.Execute(R"({"operation":"transform","actorId":9999,"transform":{"x":1,"y":0,"z":0,"rx":0,"ry":0,"rz":0,"sx":1,"sy":1,"sz":1}})", session, assets, response), "unknown actor mutation was accepted");
    const auto afterInvalidMutation = session.HierarchySnapshot();
    EditorSceneActor afterInvalidActor{};
    TEST_CHECK(session.InspectActor(44, afterInvalidActor), "failed to inspect actor after invalid mutation");
    TEST_CHECK(afterInvalidMutation.size() == beforeInvalidMutation.size() && afterInvalidActor.name == beforeInvalidActor.name && afterInvalidActor.transform.x == beforeInvalidActor.transform.x, "failed mutation changed document state");
    TEST_CHECK(!agent.Execute(R"({"operation":"duplicate","actorId":44,"newActorId":42})", session, assets, response), "duplicate actor id mutation was accepted");
    const auto afterDuplicateFailure = session.HierarchySnapshot();
    TEST_CHECK(afterDuplicateFailure.size() == beforeInvalidMutation.size(), "duplicate failure changed document state");

    TEST_CHECK(agent.Execute(R"({"operation":"properties","actorId":44,"name":"Edited","materialAssetId":"","textureAssetId":"","spriteRgba":4294967295})",
                             session, assets, response),
               "properties command failed");
    EditorSceneActor inspected{};
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.name == "Edited", "properties mutation missing");
    EditorSceneDocument saved{};
    TEST_CHECK(session.Save(saved) && saved.revision == 3U, "save after properties failed");
    TEST_CHECK(agent.Execute(R"({"operation":"transform","actorId":44,"transform":{"x":2,"y":0,"z":0,"rx":0,"ry":0,"rz":0,"sx":1,"sy":1,"sz":1}})", session, assets, response), "transform command failed");
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.transform.x == 2.0F, "transform mutation missing");
    TEST_CHECK(agent.Execute(R"({"operation":"undo"})", session, assets, response), "undo command failed");
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.transform.x == 0.0F, "undo did not restore prior transform");
    TEST_CHECK(agent.Execute(R"({"operation":"redo"})", session, assets, response), "redo command failed");
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.transform.x == 2.0F, "redo did not restore transformed state");
    TEST_CHECK(agent.Execute(R"({"operation":"save"})", session, assets, response), "agent save command failed");
    TEST_CHECK(agent.Execute(R"({"operation":"transform","actorId":44,"transform":{"x":3,"y":0,"z":0,"rx":0,"ry":0,"rz":0,"sx":1,"sy":1,"sz":1}})", session, assets, response), "second transform command failed");
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.transform.x == 3.0F, "second transform mutation missing");
    TEST_CHECK(session.RevertToSaved(assets), "revert to saved failed");
    TEST_CHECK(session.InspectActor(44, inspected) && inspected.transform.x == 2.0F, "revert did not restore saved document");

    TEST_CHECK(agent.Execute(R"({"operation":"selectMany","actorIds":[42,44]})", session, assets, response),
               "multi-select command failed");
    TEST_CHECK(session.SelectedActorIds().size() == 2U, "multi-select state mismatch");

    TEST_CHECK(agent.Execute(R"({"operation":"query"})", session, assets, response),
               "query command failed");
    TEST_CHECK(response.find("\"actorCount\":2") != std::string::npos, "query did not report actor count");

    TEST_CHECK(!agent.Execute(R"({"operation":"query","unexpected":1})", session, assets, response),
               "unknown query field was accepted");
    TEST_CHECK(agent.LastError() == EditorAgentError::UnknownField, "wrong error for unknown field");

    TEST_CHECK(!agent.Execute(R"({"operation":"transform","actorId":44,"transform":{"x":1,"y":0,"z":0,"rx":0,"ry":0,"rz":0,"sx":1,"sy":1,"sz":1,"extra":0}})",
                              session, assets, response),
               "transform with unknown field was accepted");
    TEST_CHECK(agent.LastError() == EditorAgentError::InvalidArgument, "wrong transform validation error");

    TEST_CHECK(!agent.Execute(R"({"operation":"not-a-real-operation"})", session, assets, response),
               "unknown operation was accepted");
    TEST_CHECK(agent.LastError() == EditorAgentError::InvalidOperation, "wrong invalid-operation error");

    std::cout << "[Smoke Test] editor_scene_agent_smoke passed successfully!\\n";
    return 0;
}
