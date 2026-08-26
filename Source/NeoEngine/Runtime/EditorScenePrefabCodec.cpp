#include "Runtime/EditorScenePrefabCodec.h"

#include "Runtime/EditorSceneDocumentCodec.h"

#include <algorithm>

namespace NeoEngine {
namespace {
constexpr uint8_t kMagic[4] = {'F', 'Z', 'P', 'F'};
constexpr uint8_t kCodecVersion = 1U;

void WriteUint32(std::vector<uint8_t>& bytes, uint32_t value) {
    for (size_t offset = 0U; offset < sizeof(value); ++offset) bytes.push_back(static_cast<uint8_t>((value >> (offset * 8U)) & 0xFFU));
}

bool ReadUint32(const std::vector<uint8_t>& bytes, size_t& offset, uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < sizeof(value)) return false;
    value = 0U;
    for (size_t byte = 0U; byte < sizeof(value); ++byte) value |= static_cast<uint32_t>(bytes[offset + byte]) << (byte * 8U);
    offset += sizeof(value);
    return true;
}

bool IsWholePrefabDocument(const EditorSceneDocument& document, uint32_t rootId, EditorScenePrefab& prefab) {
    if (document.version != EditorSceneDocument::kVersion || document.sceneId != "editor-prefab" || document.revision != 1U || document.actors.empty()) return false;
    const auto root = std::find_if(document.actors.begin(), document.actors.end(), [rootId](const EditorSceneActor& actor) { return actor.id == rootId; });
    if (root == document.actors.end() || root->parentId != 0U) return false;
    EditorScenePrefabAdapter adapter;
    EditorScenePrefab candidate{};
    if (!adapter.Capture(document, rootId, candidate) || candidate.actors.size() != document.actors.size()) return false;
    prefab = std::move(candidate);
    return true;
}
} // namespace

bool EditorScenePrefabCodec::Encode(const EditorScenePrefab& prefab, std::vector<uint8_t>& bytes) {
    const EditorSceneDocument document{EditorSceneDocument::kVersion, "editor-prefab", 1U, prefab.actors};
    EditorScenePrefab normalized{};
    if (!IsWholePrefabDocument(document, prefab.rootSourceId, normalized)) { lastError_ = EditorScenePrefabCodecError::InvalidPrefab; return false; }

    EditorSceneDocumentCodec documentCodec;
    std::vector<uint8_t> documentBytes;
    if (!documentCodec.Encode(document, documentBytes)) { lastError_ = EditorScenePrefabCodecError::CodecFailed; return false; }
    if (documentBytes.size() > EditorSceneDocumentCodec::kMaxBytes || documentBytes.size() + 9U > kMaxBytes) { lastError_ = EditorScenePrefabCodecError::Capacity; return false; }

    std::vector<uint8_t> candidate;
    candidate.reserve(documentBytes.size() + 9U);
    candidate.insert(candidate.end(), std::begin(kMagic), std::end(kMagic));
    candidate.push_back(kCodecVersion);
    WriteUint32(candidate, prefab.rootSourceId);
    candidate.insert(candidate.end(), documentBytes.begin(), documentBytes.end());
    bytes = std::move(candidate);
    lastError_ = EditorScenePrefabCodecError::None;
    return true;
}

bool EditorScenePrefabCodec::Decode(const std::vector<uint8_t>& bytes, EditorScenePrefab& prefab) {
    if (bytes.size() > kMaxBytes) { lastError_ = EditorScenePrefabCodecError::Capacity; return false; }
    size_t offset = 0U;
    if (bytes.size() < 9U || !std::equal(std::begin(kMagic), std::end(kMagic), bytes.begin())) { lastError_ = EditorScenePrefabCodecError::InvalidFormat; return false; }
    offset += sizeof(kMagic);
    if (bytes[offset++] != kCodecVersion) { lastError_ = EditorScenePrefabCodecError::UnsupportedVersion; return false; }
    uint32_t rootSourceId = 0U;
    if (!ReadUint32(bytes, offset, rootSourceId) || rootSourceId == 0U || offset == bytes.size()) { lastError_ = EditorScenePrefabCodecError::InvalidFormat; return false; }

    EditorSceneDocumentCodec documentCodec;
    EditorSceneDocument document{};
    const std::vector<uint8_t> documentBytes(bytes.begin() + static_cast<std::ptrdiff_t>(offset), bytes.end());
    if (!documentCodec.Decode(documentBytes, document)) { lastError_ = documentCodec.LastError() == EditorSceneDocumentCodecError::TrailingData ? EditorScenePrefabCodecError::TrailingData : EditorScenePrefabCodecError::CodecFailed; return false; }
    EditorScenePrefab candidate{};
    if (!IsWholePrefabDocument(document, rootSourceId, candidate)) { lastError_ = EditorScenePrefabCodecError::InvalidPrefab; return false; }
    prefab = std::move(candidate);
    lastError_ = EditorScenePrefabCodecError::None;
    return true;
}

} // namespace NeoEngine
