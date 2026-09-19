#include "Runtime/NeoRuntime.h"
#include "Runtime/MeshStaging.h"
#include "Runtime/MaterialStaging.h"
#include <cstdio>

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

    std::fprintf(stderr, "NEO_VULKAN_SMOKE: initialize\n");
    if (!runtime.Initialize(config)) {
        std::fprintf(stderr, "NEO_VULKAN_SMOKE: initialize failed error=%u\n",
                     static_cast<unsigned>(runtime.LastError()));
        return 1;
    }
    std::fprintf(stderr, "NEO_VULKAN_SMOKE: initialized\n");

    const auto fail = [&runtime](const char* reason) {
        std::fprintf(stderr, "NEO_VULKAN_SMOKE: validation failed: %s\n", reason);
        runtime.Shutdown();
        return 2;
    };

    std::fprintf(stderr, "NEO_VULKAN_SMOKE: scene query\n");
    if (runtime.Scene() == nullptr) return fail("scene is null");
    const auto entities = runtime.Scene()->AliveEntities();
    std::fprintf(stderr, "NEO_VULKAN_SMOKE: scene query returned\n");
    if (entities.empty()) return fail("scene has no entities");

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

    if (runtime.SceneMeshes() == nullptr ||
        !runtime.SceneMeshes()->AddStaged(entities.front(), mesh, material)) {
        return fail("scene mesh staging failed");
    }
    std::fprintf(stderr, "NEO_VULKAN_SMOKE: staged\n");

    std::fprintf(stderr, "NEO_VULKAN_SMOKE: tick\n");
    if (!runtime.Tick()) {
        std::fprintf(stderr, "NEO_VULKAN_SMOKE: tick failed error=%u\n",
                     static_cast<unsigned>(runtime.LastError()));
        runtime.Shutdown();
        return 2;
    }

    std::fprintf(stderr, "NEO_VULKAN_SMOKE: tick returned\n");
    const NeoRuntimeFrameReceipt* receipt = runtime.LastFrameReceipt();
    if (receipt == nullptr) return fail("frame receipt is null");
    if (receipt->frameStage != RuntimeFrameStage::Completed) return fail("frame did not complete");
    if (!receipt->hasVulkanRenderReceipt) return fail("Vulkan render receipt missing");
    if (receipt->vulkanRender.vertexCount != 3U) return fail("unexpected Vulkan vertex count");
    if (receipt->vulkanRender.indexCount != 3U) return fail("unexpected Vulkan index count");
    if (receipt->vulkanRender.frameIndex == 0U) return fail("Vulkan frame index did not advance");
    if (runtime.VulkanRenderer() == nullptr) return fail("Vulkan renderer is null");
    if (!runtime.VulkanRenderer()->Ready()) return fail("Vulkan renderer is not ready");

    std::fprintf(stderr, "NEO_VULKAN_SMOKE: shutdown\n");
    if (!runtime.Shutdown()) {
        std::fprintf(stderr, "NEO_VULKAN_SMOKE: shutdown failed error=%u\n",
                     static_cast<unsigned>(runtime.LastError()));
        return 3;
    }
    std::fprintf(stderr, "NEO_VULKAN_SMOKE: shutdown returned\n");
    return 0;
}
