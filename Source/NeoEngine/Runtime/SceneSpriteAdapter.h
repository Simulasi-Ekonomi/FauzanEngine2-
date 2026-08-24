#pragma once

#include "SceneWorld.h"
#include "SpriteBatch.h"
#include "TextureStaging.h"

#include <cstdint>
#include <deque>

namespace NeoEngine {

enum class SceneSpriteAdapterError : uint8_t { None, Capacity, InvalidResource, DuplicateEntity, MissingTransform, QueueRejected };

// Stores an owned CPU texture snapshot so queued SpriteBatch pointers remain
// valid throughout a frame. The SceneWorld remains sole owner of entities.
class SceneSpriteAdapter {
public:
    static constexpr size_t kMaxInstances = 512;
    bool AddStaged(SceneEntity entity, const CpuTextureResource& texture, float width, float height, int16_t layer, int16_t order, uint32_t rgba);
    bool Queue(const SceneWorld& world, SpriteBatch& batch) const;
    void Clear();
    [[nodiscard]] size_t InstanceCount() const { return instances_.size(); }
    [[nodiscard]] SceneSpriteAdapterError LastError() const { return lastError_; }
private:
    struct Instance { SceneEntity entity{}; CpuTextureResource texture{}; float width = 1.0F; float height = 1.0F; int16_t layer = 0; int16_t order = 0; uint32_t rgba = 0xFFFFFFFFU; };
    bool Fail(SceneSpriteAdapterError error) const;
    std::deque<Instance> instances_;
    mutable SceneSpriteAdapterError lastError_ = SceneSpriteAdapterError::None;
};

} // namespace NeoEngine
