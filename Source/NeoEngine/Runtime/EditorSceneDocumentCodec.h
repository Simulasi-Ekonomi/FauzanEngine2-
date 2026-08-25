#pragma once

#include "EditorSceneDocument.h"

#include <cstdint>
#include <vector>

namespace NeoEngine {
enum class EditorSceneDocumentCodecError : uint8_t { None, InvalidDocument, UnsupportedVersion, Capacity, InvalidString, InvalidTransform, InvalidActorKind, InvalidFormat, Truncated, TrailingData };

// Bounded in-memory codec only. It serializes no assets and performs no filesystem I/O;
// EditorSceneDocumentAdapter remains the canonical scene/asset/hierarchy validation seam.
class EditorSceneDocumentCodec {
public:
    static constexpr size_t kMaxBytes = 1024U * 1024U;
    static constexpr size_t kMaxStringBytes = 4096U;
    bool Encode(const EditorSceneDocument& document, std::vector<uint8_t>& bytes);
    bool Decode(const std::vector<uint8_t>& bytes, EditorSceneDocument& document);
    [[nodiscard]] EditorSceneDocumentCodecError LastError() const { return lastError_; }
private:
    EditorSceneDocumentCodecError lastError_ = EditorSceneDocumentCodecError::None;
};
} // namespace NeoEngine
