#include "Runtime/EditorSceneDocumentCodec.h"

#include <cmath>
#include <cstdio>
#include <limits>

int main() {
    using namespace NeoEngine;
    EditorSceneDocument source{};
    source.sceneId = "codec-scene";
    source.revision = 7U;
    source.actors = {{1U, 0U, EditorSceneActorKind::Mesh, {1, 2, 3, 0.1F, 0.2F, 0.3F, 1, 1, 1}, "mesh-a", "material-a", "main", "texture-a"}, {2U, 1U, EditorSceneActorKind::Sprite, {4, 5, 6, 0, 0, 0, 1, 1, 1}, "", "", "", "sprite-a", 2.5F, 3.5F, 2, 4, 0xFF112233U}};
    EditorSceneDocumentCodec codec;
    std::vector<uint8_t> bytes;
    if (!codec.Encode(source, bytes) || bytes.empty()) return 1;
    EditorSceneDocument decoded{};
    if (!codec.Decode(bytes, decoded) || decoded.version != source.version || decoded.sceneId != source.sceneId || decoded.revision != source.revision || decoded.actors.size() != 2U || decoded.actors[0].assetId != "mesh-a" || decoded.actors[1].textureAssetId != "sprite-a" || decoded.actors[1].spriteWidth != 2.5F || decoded.actors[1].spriteLayer != 2) return 1;
    std::vector<uint8_t> repeated;
    if (!codec.Encode(decoded, repeated) || repeated != bytes) return 1;
    const EditorSceneDocument preserved = decoded;
    const auto preservedOutput = [&]() { return decoded.sceneId == preserved.sceneId && decoded.revision == preserved.revision && decoded.actors.size() == preserved.actors.size() && decoded.actors[1].textureAssetId == preserved.actors[1].textureAssetId && decoded.actors[1].spriteRgba == preserved.actors[1].spriteRgba; };
    const auto rejected = [&](const std::vector<uint8_t>& malformed) { return !codec.Decode(malformed, decoded) && preservedOutput(); };
    std::vector<uint8_t> truncated = bytes; truncated.pop_back(); if (!rejected(truncated)) return 1;
    std::vector<uint8_t> magic = bytes; magic[0] = 'X'; if (!rejected(magic) || codec.LastError() != EditorSceneDocumentCodecError::InvalidFormat) return 1;
    std::vector<uint8_t> version = bytes; version[4] = 2U; if (!rejected(version) || codec.LastError() != EditorSceneDocumentCodecError::UnsupportedVersion) return 1;
    std::vector<uint8_t> oversizedLength = bytes; oversizedLength[8] = 0xFFU; oversizedLength[9] = 0xFFU; if (!rejected(oversizedLength)) return 1;
    const size_t firstActor = 8U + 2U + source.sceneId.size() + 8U + 2U;
    std::vector<uint8_t> unknownKind = bytes; unknownKind[firstActor + 8U] = 0xFFU; if (!rejected(unknownKind)) return 1;
    std::vector<uint8_t> nonFinite = bytes; nonFinite[firstActor + 9U] = 0U; nonFinite[firstActor + 10U] = 0U; nonFinite[firstActor + 11U] = 0x80U; nonFinite[firstActor + 12U] = 0x7FU; if (!rejected(nonFinite)) return 1;
    const size_t countOffset = 8U + 2U + source.sceneId.size() + 8U;
    std::vector<uint8_t> excessiveCount = bytes; excessiveCount[countOffset] = 1U; excessiveCount[countOffset + 1U] = 2U; if (!rejected(excessiveCount) || codec.LastError() != EditorSceneDocumentCodecError::Capacity) return 1;
    std::vector<uint8_t> trailing = bytes; trailing.push_back(0U); if (!rejected(trailing) || codec.LastError() != EditorSceneDocumentCodecError::TrailingData) return 1;
    EditorSceneDocument tooMany{}; tooMany.sceneId = "too-many"; tooMany.actors.resize(EditorSceneDocumentAdapter::kMaxActors + 1U); if (codec.Encode(tooMany, repeated) || codec.LastError() != EditorSceneDocumentCodecError::Capacity || repeated != bytes) return 1;
    EditorSceneDocument nonFiniteSource = source; nonFiniteSource.actors[0].transform.x = std::numeric_limits<float>::infinity(); if (codec.Encode(nonFiniteSource, repeated) || codec.LastError() != EditorSceneDocumentCodecError::InvalidTransform || repeated != bytes) return 1;
    std::printf("EDITOR_SCENE_DOCUMENT_CODEC_SMOKE_OK bytes=%zu actors=%zu deterministic=1 atomic=1 malformed=7\n", bytes.size(), source.actors.size());
    return 0;
}
