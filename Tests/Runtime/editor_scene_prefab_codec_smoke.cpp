#include "Runtime/EditorScenePrefabCodec.h"
#include "Runtime/EditorSceneSession.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "prefab-codec", 1U, {
        {1U, 0U, EditorSceneActorKind::Empty, {1, 0, 0, 0, 0, 0, 1, 1, 1}},
        {2U, 1U, EditorSceneActorKind::Marker, {2, 0, 0, 0, 0, 0, 1, 1, 1}},
        {10U, 0U, EditorSceneActorKind::Empty, {10, 0, 0, 0, 0, 0, 1, 1, 1}},
    }};
    AssetRegistry assets;
    EditorSceneSession session;
    EditorScenePrefab source{};
    if (!session.Open(document, assets) || !session.CapturePrefab(1U, source)) return 1;

    EditorScenePrefabCodec codec;
    std::vector<uint8_t> bytes{7U};
    if (!codec.Encode(source, bytes) || codec.LastError() != EditorScenePrefabCodecError::None || bytes.size() <= 9U) return 1;
    EditorScenePrefab decoded{};
    if (!codec.Decode(bytes, decoded) || codec.LastError() != EditorScenePrefabCodecError::None || decoded.rootSourceId != 1U || decoded.actors.size() != 2U || decoded.actors[0].id != 1U || decoded.actors[0].parentId != 0U || decoded.actors[1].id != 2U || decoded.actors[1].parentId != 1U) return 1;
    const EditorScenePrefab preserved = decoded;
    const std::vector<uint8_t> preservedBytes = bytes;

    std::vector<uint8_t> malformed = bytes;
    malformed.push_back(0U);
    if (codec.Decode(malformed, decoded) || codec.LastError() != EditorScenePrefabCodecError::TrailingData || decoded.rootSourceId != preserved.rootSourceId || decoded.actors.size() != preserved.actors.size()) return 1;
    malformed = bytes;
    malformed[0] = 'X';
    if (codec.Decode(malformed, decoded) || codec.LastError() != EditorScenePrefabCodecError::InvalidFormat || decoded.rootSourceId != preserved.rootSourceId || decoded.actors.size() != preserved.actors.size()) return 1;
    EditorScenePrefab invalid = source;
    invalid.actors[1].parentId = 99U;
    if (codec.Encode(invalid, bytes) || codec.LastError() != EditorScenePrefabCodecError::InvalidPrefab || bytes != preservedBytes) return 1;

    if (!session.InstantiatePrefab(decoded, 10U, {100U, 101U}, assets)) return 1;
    EditorSceneDocument saved{};
    EditorSceneActor root{};
    EditorSceneActor child{};
    if (!session.Save(saved) || saved.revision != 2U || saved.actors.size() != 5U || !session.InspectActor(100U, root) || !session.InspectActor(101U, child) || root.parentId != 10U || child.parentId != 100U) return 1;
    std::printf("EDITOR_SCENE_PREFAB_CODEC_SMOKE_OK roundTrip=1 atomic=1 instance=1 bytes=%zu\n", preservedBytes.size());
    return 0;
}
