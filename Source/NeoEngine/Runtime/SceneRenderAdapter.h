#pragma once

#include "SceneMeshAdapter.h"
#include "SceneSpriteAdapter.h"
#include "Vulkan3DRenderer.h"
#include "Core/ECS/ArchetypeManager.h"
#include "SceneECSBridge.h"

#include <cstdint>

namespace NeoEngine {
enum class SceneRenderAdapterError : uint8_t { None, MeshDrawFailed, SpriteQueueFailed, SpriteFlushFailed, VulkanFrameFailed, VulkanMeshDrawFailed };

class SceneRenderAdapter {
public:
    bool Draw(const SceneWorld& world, SceneMeshAdapter& meshes, const SceneSpriteAdapter& sprites, RenderCamera& camera, SoftwareRenderer& renderer, const DirectionalLight& light);

    // Primary 3D scene path. Vulkan transforms are sourced from the canonical ECS
    // bridge when supplied; no silent SceneWorld transform fallback is allowed.
    bool DrawVulkan3D(const SceneWorld& world, const SceneMeshAdapter& meshes, RenderCamera& camera,
                      Vulkan3DRenderer& renderer, float clearR = 0.03F, float clearG = 0.03F,
                      float clearB = 0.05F, float clearA = 1.0F,
                      const ArchetypeManager* ecs = nullptr, const SceneECSBridge* sceneECS = nullptr);

    [[nodiscard]] SceneRenderAdapterError LastError() const { return lastError_; }
private:
    SceneRenderAdapterError lastError_ = SceneRenderAdapterError::None;
};
} // namespace NeoEngine