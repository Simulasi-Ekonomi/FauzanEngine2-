#include "Runtime/EditorSceneAgentAPI.h"
#include "Runtime/EditorSceneDocumentCodec.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

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
    assert(codec.Encode(document, bytes));

    EditorSceneDocument decoded;
    assert(codec.Decode(bytes, decoded));
    assert(decoded.sceneId == document.sceneId && decoded.revision == 7 && decoded.actors.size() == 1);
    assert(decoded.actors[0].name == "Hero");
    assert(decoded.actors[0].transform.x == 10.0f && decoded.actors[0].transform.ry == 25.0f && decoded.actors[0].transform.sz == 1.5f);

    auto trailing = bytes;
    trailing.push_back(0xA5U);
    assert(!codec.Decode(trailing, decoded));

    auto truncated = bytes;
    truncated.pop_back();
    assert(!codec.Decode(truncated, decoded));

    AssetRegistry assets;
    EditorSceneSession session;
    assert(session.Open(document, assets));

    EditorSceneAgentAPI agent;
    std::string response;
    assert(agent.Execute(R"({"operation":"select","actorId":42})", session, assets, response));
    assert(session.SelectedActorId() == 42);
    assert(session.SelectedActorIds().size() == 1 && session.SelectedActorIds()[0] == 42);
    assert(agent.Execute(R"({"operation":"duplicate","actorId":42,"newActorId":44})", session, assets, response));
    assert(session.InspectActor(44, inspected));
    assert(inspected.name == "Hero" && inspected.id == 44);
    assert(agent.Execute(R"({"operation":"properties","actorId":44,"name":"HeroCopy","materialAssetId":"mat.copy","textureAssetId":"tex.copy","spriteRgba":4278255360})", session, assets, response));
    assert(session.InspectActor(44, inspected));
    assert(inspected.name == "HeroCopy" && inspected.materialAssetId == "mat.copy" && inspected.textureAssetId == "tex.copy");
    assert(agent.Execute(R"({"operation":"selectMany","actorIds":[42,44]})", session, assets, response));
    assert(session.SelectedActorIds().size() == 2 && session.SelectedActorIds()[0] == 42 && session.SelectedActorIds()[1] == 44);
    assert(agent.Execute(R"({"operation":"query"})", session, assets, response));
    assert(response.find("\"operation\":\"query\"") != std::string::npos);
    assert(response.find("\"returnedCount\":2") != std::string::npos);
    assert(agent.Execute(R"({"operation":"query","offset":1,"limit":1})", session, assets, response));
    assert(response.find("\"offset\":1") != std::string::npos);
    assert(response.find("\"returnedCount\":1") != std::string::npos);
    assert(agent.Execute(R"({"operation":"query","offset":0,"limit":256})", session, assets, response));
    assert(!agent.Execute(R"({"operation":"query","limit":257})", session, assets, response));
    assert(!agent.Execute(R"({"operation":"query","limit":0})", session, assets, response));

    const std::string fullTransform = R"({"operation":"transform","actorId":42,"transform":{"x":3,"y":4,"z":5,"rx":6,"ry":7,"rz":8,"sx":2,"sy":3,"sz":4}})";
    assert(agent.Execute(fullTransform, session, assets, response));
    EditorSceneActor inspected;
    assert(session.InspectActor(42, inspected));
    assert(inspected.transform.x == 3.0f && inspected.transform.ry == 7.0f && inspected.transform.sz == 4.0f);

    assert(!agent.Execute(R"({"operation":"transform","actorId":42,"transform":{"x":9}})", session, assets, response));
    assert(!agent.Execute(R"({"operation":"select","actorId":42,"extra":true})", session, assets, response));
    assert(!agent.Execute(R"({"operation":"unknown"})", session, assets, response));
    assert(!agent.Execute(R"([])", session, assets, response));
    assert(!agent.Execute(R"({"operation":"select","actorId":999})", session, assets, response));

    EditorSceneActor child;
    child.id = 43;
    child.name = "Child";
    child.parentId = 42;
    assert(session.AddActor(child, assets));
    assert(session.InspectActor(43, inspected));
    assert(inspected.parentId == 42);

    EditorSceneActor duplicate = child;
    assert(!session.AddActor(duplicate, assets));
    assert(session.LastError() == EditorSceneSessionError::DuplicateActorId);

    EditorSceneActor selfParent = child;
    selfParent.id = 45;
    selfParent.parentId = 45;
    assert(!session.AddActor(selfParent, assets));
    assert(session.LastError() == EditorSceneSessionError::InvalidHierarchy);

    EditorSceneActor missingParent = child;
    missingParent.id = 46;
    missingParent.parentId = 999;
    assert(!session.AddActor(missingParent, assets));
    assert(session.LastError() == EditorSceneSessionError::UnknownActor);

    assert(!session.ReparentActor(43, 43, assets));
    assert(session.LastError() == EditorSceneSessionError::InvalidHierarchy);

    assert(!session.ReparentActor(43, 999, assets));
    assert(session.LastError() == EditorSceneSessionError::UnknownActor);

    assert(!session.ReparentActor(42, 43, assets));
    assert(session.LastError() == EditorSceneSessionError::InvalidHierarchy);
    assert(session.InspectActor(42, inspected));
    assert(inspected.parentId == 0U);

    assert(!session.DeleteActor(42, assets));
    assert(session.LastError() == EditorSceneSessionError::ActorHasChildren);

    assert(session.DeleteActor(43, assets));
    assert(session.InspectActor(42, inspected));
    assert(inspected.parentId == 0U);

    return 0;
}
