#pragma once

#include "SceneWorld.h"
#include "SpriteBatch.h"
#include "FlipbookFrameSelector.h"
#include "TextureStaging.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace NeoEngine {

enum class SceneSpriteAdapterError : uint8_t { None, Capacity, InvalidResource, DuplicateEntity, MissingEntity, MissingTransform, QueueRejected };

// A value-owned, read-only view of a staged sprite binding. It intentionally
// excludes texture storage and display metadata so diagnostics cannot mutate
// adapter-owned render state.
struct SceneSpriteBindingSnapshot {
    SceneEntity entity{};
    std::string sourceAssetId;
    uint64_t sourceHash = 0U;
};

// Stores an owned CPU texture snapshot so queued SpriteBatch pointers remain
// valid throughout a frame. The SceneWorld remains sole owner of entities.
class SceneSpriteAdapter {
public:
    static constexpr size_t kMaxInstances = 512;
    bool AddStaged(SceneEntity entity, const CpuTextureResource& texture, float width, float height, int16_t layer, int16_t order, uint32_t rgba, float rotationRadians = 0.0F, bool faceCamera = false, bool depthWrite = true, uint16_t sourceX = 0U, uint16_t sourceY = 0U, uint16_t sourceWidth = 0U, uint16_t sourceHeight = 0U);
    bool RefreshStaged(SceneEntity entity, const CpuTextureResource& texture);
    [[nodiscard]] bool InspectStagedTexture(SceneEntity entity, std::string& assetId, uint64_t& sourceHash) const;
    [[nodiscard]] std::vector<SceneSpriteBindingSnapshot> BindingSnapshots() const;
    bool Queue(const SceneWorld& world, SpriteBatch& batch) const;
    bool QueueTinted(const SceneWorld& world, SpriteBatch& batch, uint32_t frameRgba) const;
    bool QueueFrame(const SceneWorld& world, SpriteBatch& batch, SpriteSourceRect sourceRect, uint32_t frameRgba = 0xFFFFFFFFU) const;
    void Clear();
    [[nodiscard]] size_t InstanceCount() const { return instances_.size(); }
    [[nodiscard]] SceneSpriteAdapterError LastError() const { return lastError_; }
private:
    struct Instance { SceneEntity entity{}; CpuTextureResource texture{}; float width = 1.0F; float height = 1.0F; int16_t layer = 0; int16_t order = 0; uint32_t rgba = 0xFFFFFFFFU; float rotationRadians = 0.0F; bool faceCamera = false; bool depthWrite = true; uint16_t sourceX = 0U, sourceY = 0U, sourceWidth = 0U, sourceHeight = 0U; };
    bool Fail(SceneSpriteAdapterError error) const;
    std::deque<Instance> instances_;
    mutable SceneSpriteAdapterError lastError_ = SceneSpriteAdapterError::None;
};

} // namespace NeoEngine
