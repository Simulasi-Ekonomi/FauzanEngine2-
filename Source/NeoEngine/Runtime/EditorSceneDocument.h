#pragma once

#include "Runtime/AssetRegistry.h"
#include "Runtime/SceneWorld.h"

#include <cstdint>
#include <string>
#include <vector>

namespace NeoEngine {

enum class EditorSceneActorKind : uint8_t { Empty, Mesh, Light, Camera, PlayerStart, Marker, Sprite };
enum class EditorSceneDocumentError : uint8_t { None, UnsupportedVersion, InvalidSceneId, InvalidRevision, Capacity, DuplicateActorId, InvalidActor, MissingParent, InvalidHierarchy, MissingAsset, AssetNotReady, AssetKindMismatch, MissingMaterial, MaterialNotReady, MaterialKindMismatch, MissingTexture, TextureNotReady, TextureKindMismatch, SceneSyncFailed };

struct EditorSceneActor {
    uint32_t id = 0;
    uint32_t parentId = 0;
    EditorSceneActorKind kind = EditorSceneActorKind::Empty;
    Transform3 transform{};
    std::string assetId;
    std::string materialAssetId;
    std::string materialName;
    std::string textureAssetId;
    float spriteWidth = 1.0F;
    float spriteHeight = 1.0F;
    int16_t spriteLayer = 0;
    int16_t spriteOrder = 0;
    uint32_t spriteRgba = 0xFFFFFFFFU;
};

struct EditorSceneDocument {
    static constexpr uint8_t kMinSupportedVersion = 1;
    static constexpr uint8_t kVersion = 3;
    uint8_t version = kVersion;
    std::string sceneId;
    uint64_t revision = 0;
    std::vector<EditorSceneActor> actors;
};

class EditorSceneDocumentAdapter {
public:
    static constexpr size_t kMaxActors = 512;
    bool Load(const EditorSceneDocument& document, const AssetRegistry& assets, SceneWorld& target);
    [[nodiscard]] const SceneEntity* EntityForActor(uint32_t actorId) const;
    [[nodiscard]] EditorSceneDocumentError LastError() const { return lastError_; }
private:
    struct Binding { uint32_t actorId = 0; SceneEntity entity{}; };
    bool Fail(EditorSceneDocumentError error);
    std::vector<Binding> bindings_;
    EditorSceneDocumentError lastError_ = EditorSceneDocumentError::None;
};

} // namespace NeoEngine
