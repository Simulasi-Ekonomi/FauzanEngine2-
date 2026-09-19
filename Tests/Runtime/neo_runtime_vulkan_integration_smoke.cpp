#include "Runtime/NeoRuntime.h"
#include <cassert>
#include <limits>

int main() {
    using namespace NeoEngine;

    NeoRuntime runtime;
    RuntimeConfig config{};
    config.farmNpcCount = 1;
    config.renderWidth = 256;
    config.renderHeight = 256;
    config.enableVulkan3DRenderer = true;
    config.sceneCamera.mode = RenderCameraMode::Perspective;
    config.sceneCamera.position = {0.0F, 0.0F, 0.0F};
    config.sceneCamera.forward = {0.0F, 0.0F, 1.0F};
    config.sceneCamera.up = {0.0F, 1.0F, 0.0F};
    config.sceneCamera.aspect = 1.0F;
    config.sceneCamera.nearPlane = 0.1F;
    config.sceneCamera.farPlane = 100.0F;

    assert(runtime.Initialize(config));
    const auto entities = runtime.Scene()->AliveEntities();
    assert(!entities.empty());

    CpuMeshResource mesh{};
    mesh.assetId = "canonical.runtime.vulkan.mesh";
    mesh.sourceHash = 0x1122334455667788ULL;
    mesh.vertices = {
        {{-0.8F, -0.6F, 3.0F}, {0.0F, 0.0F, 1.0F}, 0.0F, 0.0F},
        {{ 0.8F, -0.6F, 3.0F}, {0.0F, 0.0F, 1.0F}, 1.0F, 0.0F},
        {{ 0.0F,  0.8F, 3.0F}, {0.0F, 0.0F, 1.0F}, 0.5F, 1.0F}
    };
    mesh.indices = {0U, 1U, 2U};

    CpuMaterialResource material{};
    material.assetId = "canonical.runtime.vulkan.material";
    material.materialName = "canonical";
    material.sourceHash = 0x8877665544332211ULL;

    assert(runtime.SceneMeshes()->AddStaged(entities.front(), mesh, material));

    if (!runtime.Tick()) {
        runtime.Shutdown();
        return 2;
    }

    const NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    assert(receipt != nullptr);
    assert(receipt->frameStage == RuntimeFrameStage::Completed);
    assert(receipt->hasVulkanRenderReceipt);
    assert(receipt->vulkanRender.vertexCount == 3U);
    assert(receipt->vulkanRender.indexCount == 3U);
    assert(receipt->vulkanRender.frameIndex > 0U);
    assert(runtime.VulkanRenderer() != nullptr);
    assert(runtime.VulkanRenderer()->Ready());

    assert(runtime.Shutdown());
    return 0;
}
