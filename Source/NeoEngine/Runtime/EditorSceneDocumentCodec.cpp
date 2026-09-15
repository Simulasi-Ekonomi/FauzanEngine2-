#include "Runtime/EditorSceneDocumentCodec.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <type_traits>

namespace NeoEngine {
namespace {
constexpr uint8_t kMagic[4] = {'F', 'Z', 'S', 'D'};
constexpr uint8_t kCodecVersion = 1;

template <typename T>
    requires std::is_integral_v<T>
void Write(std::vector<uint8_t>& out, T value) {
    using Unsigned = std::make_unsigned_t<T>;
    const Unsigned raw = static_cast<Unsigned>(value);
    for (size_t i = 0; i < sizeof(T); ++i) out.push_back(static_cast<uint8_t>((raw >> (i * 8U)) & 0xFFU));
}

void Write(std::vector<uint8_t>& out, float value) { Write<uint32_t>(out, std::bit_cast<uint32_t>(value)); }

template <typename T>
    requires std::is_integral_v<T>
bool Read(const std::vector<uint8_t>& in, size_t& offset, T& value) {
    if (offset > in.size() || in.size() - offset < sizeof(T)) return false;
    using Unsigned = std::make_unsigned_t<T>;
    Unsigned raw = 0;
    for (size_t i = 0; i < sizeof(T); ++i) raw |= static_cast<Unsigned>(in[offset + i]) << (i * 8U);
    offset += sizeof(T);
    value = static_cast<T>(raw);
    return true;
}

bool Read(const std::vector<uint8_t>& in, size_t& offset, float& value) {
    uint32_t raw = 0;
    if (!Read(in, offset, raw)) return false;
    value = std::bit_cast<float>(raw);
    return true;
}

bool FiniteTransform(const Transform3& transform) {
    return std::isfinite(transform.x) && std::isfinite(transform.y) && std::isfinite(transform.z) &&
           std::isfinite(transform.rx) && std::isfinite(transform.ry) && std::isfinite(transform.rz) &&
           std::isfinite(transform.sx) && std::isfinite(transform.sy) && std::isfinite(transform.sz);
}

bool WriteString(std::vector<uint8_t>& out, const std::string& value) {
    if (value.size() > EditorSceneDocumentCodec::kMaxStringBytes || value.size() > UINT16_MAX) return false;
    Write<uint16_t>(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
    return true;
}

bool ReadString(const std::vector<uint8_t>& in, size_t& offset, std::string& value) {
    uint16_t size = 0;
    if (!Read(in, offset, size) || size > EditorSceneDocumentCodec::kMaxStringBytes || offset > in.size() || in.size() - offset < size) return false;
    value.assign(reinterpret_cast<const char*>(in.data() + offset), size);
    offset += size;
    return true;
}

bool WriteActor(std::vector<uint8_t>& out, const EditorSceneActor& actor) {
    if (!FiniteTransform(actor.transform) || actor.kind > EditorSceneActorKind::Sprite || !std::isfinite(actor.spriteWidth) || !std::isfinite(actor.spriteHeight)) return false;
    Write<uint32_t>(out, actor.id);
    Write<uint32_t>(out, actor.parentId);
    Write<uint8_t>(out, static_cast<uint8_t>(actor.kind));
    for (const float value : {actor.transform.x, actor.transform.y, actor.transform.z, actor.transform.rx, actor.transform.ry, actor.transform.rz, actor.transform.sx, actor.transform.sy, actor.transform.sz}) Write(out, value);
    return WriteString(out, actor.assetId) && WriteString(out, actor.materialAssetId) && WriteString(out, actor.materialName) && WriteString(out, actor.textureAssetId) &&
           (Write(out, actor.spriteWidth), true) && (Write(out, actor.spriteHeight), true) && (Write(out, actor.spriteLayer), true) &&
           (Write(out, actor.spriteOrder), true) && (Write(out, actor.spriteRgba), true);
}

bool ReadActor(const std::vector<uint8_t>& in, size_t& offset, EditorSceneActor& actor) {
    uint8_t kind = 0;
    if (!Read(in, offset, actor.id) || !Read(in, offset, actor.parentId) || !Read(in, offset, kind) || kind > static_cast<uint8_t>(EditorSceneActorKind::Sprite)) return false;
    actor.kind = static_cast<EditorSceneActorKind>(kind);
    float* values[] = {&actor.transform.x, &actor.transform.y, &actor.transform.z, &actor.transform.rx, &actor.transform.ry, &actor.transform.rz, &actor.transform.sx, &actor.transform.sy, &actor.transform.sz};
    for (float* value : values) if (!Read(in, offset, *value)) return false;
    return FiniteTransform(actor.transform) && ReadString(in, offset, actor.assetId) && ReadString(in, offset, actor.materialAssetId) &&
           ReadString(in, offset, actor.materialName) && ReadString(in, offset, actor.textureAssetId) && Read(in, offset, actor.spriteWidth) &&
           Read(in, offset, actor.spriteHeight) && Read(in, offset, actor.spriteLayer) && Read(in, offset, actor.spriteOrder) &&
           Read(in, offset, actor.spriteRgba) && std::isfinite(actor.spriteWidth) && std::isfinite(actor.spriteHeight);
}
} // namespace

bool EditorSceneDocumentCodec::Encode(const EditorSceneDocument& document, std::vector<uint8_t>& bytes) {
    if (document.version < EditorSceneDocument::kMinSupportedVersion || document.version > EditorSceneDocument::kVersion) { lastError_ = EditorSceneDocumentCodecError::UnsupportedVersion; return false; }
    if (document.sceneId.empty() || document.sceneId.size() > kMaxStringBytes || document.actors.size() > EditorSceneDocumentAdapter::kMaxActors) { lastError_ = document.actors.size() > EditorSceneDocumentAdapter::kMaxActors ? EditorSceneDocumentCodecError::Capacity : EditorSceneDocumentCodecError::InvalidDocument; return false; }
    std::vector<uint8_t> candidate;
    candidate.reserve(128U + document.actors.size() * 128U);
    candidate.insert(candidate.end(), std::begin(kMagic), std::end(kMagic));
    Write(candidate, kCodecVersion); Write(candidate, document.version); Write<uint16_t>(candidate, 0U);
    if (!WriteString(candidate, document.sceneId)) { lastError_ = EditorSceneDocumentCodecError::InvalidString; return false; }
    Write(candidate, document.revision); Write<uint16_t>(candidate, static_cast<uint16_t>(document.actors.size()));
    for (const EditorSceneActor& actor : document.actors) if (!WriteActor(candidate, actor)) { lastError_ = !FiniteTransform(actor.transform) || !std::isfinite(actor.spriteWidth) || !std::isfinite(actor.spriteHeight) ? EditorSceneDocumentCodecError::InvalidTransform : EditorSceneDocumentCodecError::InvalidActorKind; return false; }
    if (candidate.size() > kMaxBytes) { lastError_ = EditorSceneDocumentCodecError::Capacity; return false; }
    bytes = std::move(candidate);
    lastError_ = EditorSceneDocumentCodecError::None;
    return true;
}

bool EditorSceneDocumentCodec::Decode(const std::vector<uint8_t>& bytes, EditorSceneDocument& document) {
    if (bytes.size() > kMaxBytes) { lastError_ = EditorSceneDocumentCodecError::Capacity; return false; }
    size_t offset = 0;
    uint8_t magic[4]{};
    for (uint8_t& value : magic) if (!Read(bytes, offset, value)) { lastError_ = EditorSceneDocumentCodecError::Truncated; return false; }
    if (std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) { lastError_ = EditorSceneDocumentCodecError::InvalidFormat; return false; }
    uint8_t codecVersion = 0, documentVersion = 0;
    uint16_t reserved = 0;
    if (!Read(bytes, offset, codecVersion) || !Read(bytes, offset, documentVersion) || !Read(bytes, offset, reserved)) { lastError_ = EditorSceneDocumentCodecError::Truncated; return false; }
    if (codecVersion != kCodecVersion || documentVersion < EditorSceneDocument::kMinSupportedVersion || documentVersion > EditorSceneDocument::kVersion) { lastError_ = EditorSceneDocumentCodecError::UnsupportedVersion; return false; }
    EditorSceneDocument candidate{};
    candidate.version = documentVersion;
    if (!ReadString(bytes, offset, candidate.sceneId) || candidate.sceneId.empty() || !Read(bytes, offset, candidate.revision)) { lastError_ = EditorSceneDocumentCodecError::Truncated; return false; }
    uint16_t actorCount = 0;
    if (!Read(bytes, offset, actorCount)) { lastError_ = EditorSceneDocumentCodecError::Truncated; return false; }
    if (actorCount > EditorSceneDocumentAdapter::kMaxActors) { lastError_ = EditorSceneDocumentCodecError::Capacity; return false; }
    candidate.actors.reserve(actorCount);
    for (uint16_t i = 0; i < actorCount; ++i) { EditorSceneActor actor{}; if (!ReadActor(bytes, offset, actor)) { lastError_ = EditorSceneDocumentCodecError::InvalidFormat; return false; } candidate.actors.push_back(std::move(actor)); }
    if (offset != bytes.size()) { lastError_ = EditorSceneDocumentCodecError::TrailingData; return false; }
    document = std::move(candidate);
    lastError_ = EditorSceneDocumentCodecError::None;
    return true;
}
} // namespace NeoEngine
