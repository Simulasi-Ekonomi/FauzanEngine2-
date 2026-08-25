#pragma once

#include "SceneMeshAdapter.h"
#include "SceneSpriteAdapter.h"

#include <cstdint>

namespace NeoEngine {
enum class SceneRenderAdapterError : uint8_t { None, MeshDrawFailed, SpriteQueueFailed, SpriteFlushFailed };

// Bounded runtime composition seam. Meshes are drawn first, then queued sprites;
// a caller framebuffer is replaced only after both passes succeed.
class SceneRenderAdapter {
public:
    bool Draw(const SceneWorld& world, SceneMeshAdapter& meshes, const SceneSpriteAdapter& sprites, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light);
    [[nodiscard]] SceneRenderAdapterError LastError() const { return lastError_; }
private:
    SceneRenderAdapterError lastError_ = SceneRenderAdapterError::None;
};
} // namespace NeoEngine
