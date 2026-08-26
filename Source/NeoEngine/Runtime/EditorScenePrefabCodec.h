#pragma once

#include "Runtime/EditorSceneDocumentCodec.h"
#include "Runtime/EditorScenePrefab.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {

enum class EditorScenePrefabCodecError : uint8_t { None, InvalidPrefab, InvalidFormat, UnsupportedVersion, Capacity, CodecFailed, TrailingData };

// Bounded in-memory framing around EditorSceneDocumentCodec. It never embeds
// asset bytes and it does not perform filesystem I/O.
class EditorScenePrefabCodec {
public:
    static constexpr size_t kMaxBytes = EditorSceneDocumentCodec::kMaxBytes + 9U;

    bool Encode(const EditorScenePrefab& prefab, std::vector<uint8_t>& bytes);
    bool Decode(const std::vector<uint8_t>& bytes, EditorScenePrefab& prefab);
    [[nodiscard]] EditorScenePrefabCodecError LastError() const { return lastError_; }

private:
    EditorScenePrefabCodecError lastError_ = EditorScenePrefabCodecError::None;
};

} // namespace NeoEngine
