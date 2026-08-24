#include "Runtime/LocalAuthoringBridge.h"

#include <bit>
#include <cstddef>
#include <string>

namespace NeoEngine {
namespace {
class Cursor {
public:
    explicit Cursor(std::span<const uint8_t> bytes) : bytes_(bytes) {}
    bool ReadU8(uint8_t& out) { if (remaining() < 1) return false; out = bytes_[offset_++]; return true; }
    bool ReadU16(uint16_t& out) { uint8_t low = 0, high = 0; if (!ReadU8(low) || !ReadU8(high)) return false; out = static_cast<uint16_t>(low) | (static_cast<uint16_t>(high) << 8U); return true; }
    bool ReadU32(uint32_t& out) { uint16_t low = 0, high = 0; if (!ReadU16(low) || !ReadU16(high)) return false; out = static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 16U); return true; }
    bool ReadU64(uint64_t& out) { uint32_t low = 0, high = 0; if (!ReadU32(low) || !ReadU32(high)) return false; out = static_cast<uint64_t>(low) | (static_cast<uint64_t>(high) << 32U); return true; }
    bool ReadFloat(float& out) { uint32_t bits = 0; if (!ReadU32(bits)) return false; out = std::bit_cast<float>(bits); return true; }
    bool ReadString(std::string& out, uint8_t maximum) {
        uint8_t length = 0;
        if (!ReadU8(length) || length > maximum || remaining() < length) return false;
        out.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), length);
        offset_ += length;
        for (const unsigned char character : out) if (character < 0x21U || character > 0x7EU) return false;
        return true;
    }
    [[nodiscard]] size_t remaining() const { return bytes_.size() - offset_; }
private:
    std::span<const uint8_t> bytes_;
    size_t offset_ = 0;
};

bool ReadTransform(Cursor& cursor, Transform3& transform) {
    return cursor.ReadFloat(transform.x) && cursor.ReadFloat(transform.y) && cursor.ReadFloat(transform.z) &&
           cursor.ReadFloat(transform.rx) && cursor.ReadFloat(transform.ry) && cursor.ReadFloat(transform.rz) &&
           cursor.ReadFloat(transform.sx) && cursor.ReadFloat(transform.sy) && cursor.ReadFloat(transform.sz);
}

bool ReadActor(Cursor& cursor, uint8_t version, EditorSceneActor& actor) {
    uint8_t kind = 0;
    if (!cursor.ReadU32(actor.id) || !cursor.ReadU32(actor.parentId) || !cursor.ReadU8(kind) || !ReadTransform(cursor, actor.transform) || !cursor.ReadString(actor.assetId, 128)) return false;
    if (version >= 2 && (!cursor.ReadString(actor.materialAssetId, 128) || !cursor.ReadString(actor.materialName, 96) || !cursor.ReadString(actor.textureAssetId, 128))) return false;
    if (version >= 3) { uint16_t layer = 0, order = 0; if (!cursor.ReadFloat(actor.spriteWidth) || !cursor.ReadFloat(actor.spriteHeight) || !cursor.ReadU16(layer) || !cursor.ReadU16(order) || !cursor.ReadU32(actor.spriteRgba)) return false; actor.spriteLayer = std::bit_cast<int16_t>(layer); actor.spriteOrder = std::bit_cast<int16_t>(order); }
    if (kind > static_cast<uint8_t>(EditorSceneActorKind::Sprite)) return false;
    actor.kind = static_cast<EditorSceneActorKind>(kind);
    return true;
}

uint64_t Digest(std::span<const uint8_t> bytes) {
    uint64_t value = 14695981039346656037ULL;
    for (const uint8_t byte : bytes) { value ^= byte; value *= 1099511628211ULL; }
    return value;
}
} // namespace

bool LocalAuthoringBridge::Fail(LocalAuthoringBridgeError error) {
    lastError_ = error;
    return false;
}

bool LocalAuthoringBridge::Load(std::span<const uint8_t> payload, bool approved, const AssetRegistry& assets, SceneWorld& target, LocalAuthoringBridgeReceipt& receipt) {
    if (!approved) return Fail(LocalAuthoringBridgeError::ApprovalRequired);
    Cursor cursor(payload);
    uint8_t magic[4]{};
    for (uint8_t& byte : magic) if (!cursor.ReadU8(byte)) return Fail(LocalAuthoringBridgeError::CorruptPayload);
    if (magic[0] != 'N' || magic[1] != 'A' || magic[2] != 'B' || (magic[3] != '1' && magic[3] != '2' && magic[3] != '3')) return Fail(LocalAuthoringBridgeError::CorruptPayload);
    EditorSceneDocument document;
    uint16_t actorCount = 0;
    if (!cursor.ReadU8(document.version) || !cursor.ReadString(document.sceneId, 48) || !cursor.ReadU64(document.revision) || !cursor.ReadU16(actorCount)) return Fail(LocalAuthoringBridgeError::CorruptPayload);
    const uint8_t envelopeVersion = static_cast<uint8_t>(magic[3] - '0');
    if (document.version != envelopeVersion || document.version < EditorSceneDocument::kMinSupportedVersion || document.version > EditorSceneDocument::kVersion) return Fail(LocalAuthoringBridgeError::UnsupportedVersion);
    if (actorCount > EditorSceneDocumentAdapter::kMaxActors) return Fail(LocalAuthoringBridgeError::CorruptPayload);
    document.actors.reserve(actorCount);
    for (uint16_t index = 0; index < actorCount; ++index) { EditorSceneActor actor{}; if (!ReadActor(cursor, document.version, actor)) return Fail(LocalAuthoringBridgeError::CorruptPayload); document.actors.push_back(std::move(actor)); }
    if (cursor.remaining() != 0) return Fail(LocalAuthoringBridgeError::TrailingBytes);
    if (!adapter_.Load(document, assets, target)) return Fail(LocalAuthoringBridgeError::SceneRejected);
    receipt = {document.sceneId, document.revision, actorCount, Digest(payload)};
    lastError_ = LocalAuthoringBridgeError::None;
    return true;
}

} // namespace NeoEngine
