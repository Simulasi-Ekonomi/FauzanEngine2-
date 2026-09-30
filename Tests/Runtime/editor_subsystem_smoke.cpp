#include "Runtime/EditorSceneAgentAPI.h"
#include "Runtime/EditorSceneDocumentCodec.h"

#include <cstdint>
#include <string>
#include <vector>

#define REQUIRE(...) do { if (!(__VA_ARGS__)) return 1; } while (false)

int main() {
    using namespace NeoEngine;

    EditorSceneDocument document;
    document.sceneId = "editor-smoke";
    document.revision = 7;
    EditorSceneActor actor;
    actor.id = 42;
    actor.name = "Hero";
    actor.transform.x = 10.0f;
    actor.transform.ry = 25.0f;
    actor.transform.sz = 1.5f;
    document.actors.push_back(actor);

    EditorSceneDocumentCodec codec;
    std::vector<uint8_t> bytes;
    REQUIRE(codec.Encode(document, bytes));

    EditorSceneDocument decoded;
    REQUIRE(codec.Decode(bytes, decoded));
    REQUIRE(decoded.sceneId == document.sceneId && decoded.revision == 7 && decoded.actors.size() == 1);
    REQUIRE(decoded.actors[0].name == "Hero");
    REQUIRE(decoded.actors[0].transform.x == 10.0f && decoded.actors[0].transform.ry == 25.0f && decoded.actors[0].transform.sz == 1.5f);

    auto trailing = bytes;
    trailing.push_back(0xA5U);
    REQUIRE(!codec.Decode(trailing, decoded));

    auto truncated = bytes;
    truncated.pop_back();
    REQUIRE(!codec.Decode(truncated, decoded));

    AssetRegistry assets;
    EditorSceneSession session;
    REQUIRE(session.Open(document, assets));

    EditorSceneAgentAPI agent;
    std::string response;
    REQUIRE(agent.Execute(R"({"operation":"select","actorId":42})", session, assets, response));
    REQUIRE(session.SelectedActorId() == 42);

    const std::string fullTransform = R"({"operation":"transform","actorId":42,"transform":{"x":3,"y":4,"z":5,"rx":6,"ry":7,"rz":8,"sx":2,"sy":3,"sz":4}})";
    REQUIRE(agent.Execute(fullTransform, session, assets, response));
    EditorSceneActor inspected;
    REQUIRE(session.InspectActor(42, inspected));
    REQUIRE(inspected.transform.x == 3.0f && inspected.transform.ry == 7.0f && inspected.transform.sz == 4.0f);

    REQUIRE(!agent.Execute(R"({"operation":"transform","actorId":42,"transform":{"x":9}})", session, assets, response));
    REQUIRE(!agent.Execute(R"({"operation":"select","actorId":42,"extra":true})", session, assets, response));
    REQUIRE(!agent.Execute(R"({"operation":"unknown"})", session, assets, response));
    REQUIRE(!agent.Execute(R"([])", session, assets, response));
    REQUIRE(!agent.Execute(R"({"operation":"select","actorId":999})", session, assets, response));

    EditorSceneActor child;
    child.id = 43;
    child.name = "Child";
    child.parentId = 42;
    REQUIRE(session.AddActor(child, assets));
    REQUIRE(session.InspectActor(43, inspected));
    REQUIRE(inspected.parentId == 42);

    EditorSceneActor duplicate = child;
    REQUIRE(!session.AddActor(duplicate, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::DuplicateActorId);

    EditorSceneActor selfParent = child;
    selfParent.id = 44;
    selfParent.parentId = 44;
    REQUIRE(!session.AddActor(selfParent, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::InvalidHierarchy);

    EditorSceneActor missingParent = child;
    missingParent.id = 45;
    missingParent.parentId = 999;
    REQUIRE(!session.AddActor(missingParent, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::UnknownActor);

    REQUIRE(!session.ReparentActor(43, 43, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::InvalidHierarchy);

    REQUIRE(!session.ReparentActor(43, 999, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::UnknownActor);

    REQUIRE(!session.ReparentActor(42, 43, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::InvalidHierarchy);
    REQUIRE(session.InspectActor(42, inspected));
    REQUIRE(inspected.parentId == 0U);

    REQUIRE(!session.DeleteActor(42, assets));
    REQUIRE(session.LastError() == EditorSceneSessionError::ActorHasChildren);

    REQUIRE(session.DeleteActor(43, assets));
    REQUIRE(session.InspectActor(42, inspected));
    REQUIRE(inspected.parentId == 0U);

    return 0;
}
