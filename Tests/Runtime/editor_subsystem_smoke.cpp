#include "Runtime/EditorSceneDocumentCodec.h"

#include <cassert>
#include <cstdint>
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

    return 0;
}
