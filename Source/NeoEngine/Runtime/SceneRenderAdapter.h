#pragma once

#include "SceneMeshAdapter.h"
#include "SceneSpriteAdapter.h"
#include "Vulkan3DRenderer.h"

#include <cstdint>

namespace NeoEngine {
enum class SceneRenderAdapterError : uint8_t { None, MeshDrawFailed, SpriteQueueFailed, SpriteFlushFailed, VulkanFrameFailed, VulkanMeshDrawFailed };

// Bounded runtime composition seam. Meshes are drawn first, then queued sprites;
// a caller framebuffer is replaced only after both passes succeed.
class SceneRenderAdapter {
public:
    bool Draw(const SceneWorld& world, SceneMeshAdapter& meshes, const SceneSpriteAdapter& sprites, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light);

    // Primary 3D scene path. SceneWorld transforms are converted to Vulkan instance
    // transforms and the camera is converted to a real view-projection matrix.
    // Sprite/UI composition remains on the software path until a dedicated GPU UI
    // adapter is introduced; this method never silently falls back to software mesh rendering.
    bool DrawVulkan3D(const SceneWorld& world, const SceneMeshAdapter& meshes, RenderCamera& camera,
                      Vulkan3DRenderer& renderer, float clearR = 0.03F, float clearG = 0.03F,
                      float clearB = 0.05F, float clearA = 1.0F);

    [[nodiscard]] SceneRenderAdapterError LastError() const { return lastError_; }
private:
    SceneRenderAdapterError lastError_ = SceneRenderAdapterError::None;
};
} // namespace NeoEngine
