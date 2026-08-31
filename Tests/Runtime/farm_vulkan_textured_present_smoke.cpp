#include "Runtime/NeoRuntime.h"
#include "Runtime/VulkanTexturedPresent.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    RuntimeConfig config{};
    config.farmWidth = 4U;
    config.farmHeight = 4U;
    config.farmNpcCount = 1U;
    config.renderWidth = 64U;
    config.renderHeight = 64U;
    config.enableFarmRuntimeHud = true;
    config.enableFarmCurriculum = true;

    NeoRuntime runtime;
    if (!runtime.Initialize(config) || runtime.Farm() == nullptr || runtime.Renderer() == nullptr || !runtime.Tick() ||
        !runtime.RenderFarm() || runtime.Renderer()->Pixels().empty()) {
        std::fprintf(stderr, "FARM_VULKAN_TEXTURED_PRESENT_SMOKE_FAIL stage=runtime\n");
        return 1;
    }

    const uint64_t farmFrameHash = runtime.Renderer()->FrameHash();
    const uint16_t completedLessons = runtime.Curriculum() == nullptr ? 0U : runtime.Curriculum()->LastReceipt().completedLessons;
    const auto pixels = runtime.Renderer()->Pixels();
    const VulkanTexturedPresentResult invalid = VulkanTexturedPresentProbe::Present(pixels.subspan(0U, pixels.size() - 1U), runtime.Renderer()->Width(), runtime.Renderer()->Height());
    if (invalid.status != VulkanPresentStatus::InvalidInput || invalid.framePresented) {
        std::fprintf(stderr, "FARM_VULKAN_TEXTURED_PRESENT_SMOKE_FAIL stage=invalid_input\n");
        return 2;
    }

    const VulkanTexturedPresentResult presented = VulkanTexturedPresentProbe::Present(pixels, runtime.Renderer()->Width(), runtime.Renderer()->Height());
    if (presented.status != VulkanPresentStatus::Presented || !presented.windowCreated || !presented.surfaceCreated ||
        !presented.deviceCreated || !presented.swapchainCreated || !presented.textureUploaded || !presented.pipelineCreated ||
        !presented.frameSubmitted || !presented.framePresented || presented.imageCount == 0U || presented.textureHash == 0U ||
        farmFrameHash == 0U || !runtime.Shutdown()) {
        std::fprintf(stderr, "FARM_VULKAN_TEXTURED_PRESENT_SMOKE_FAIL stage=present status=%u sdl=%u window=%u surface=%u device=%u swapchain=%u texture=%u pipeline=%u acquire=%u submit=%u fence=%u present=%u driver=%d/%d/%d/%d\n",
            static_cast<unsigned>(presented.status), presented.sdlInitialized, presented.windowCreated, presented.surfaceCreated, presented.deviceCreated,
            presented.swapchainCreated, presented.textureUploaded, presented.pipelineCreated, presented.acquireAttempted,
            presented.submitAttempted, presented.fenceWaitAttempted, presented.presentAttempted, presented.acquireDriverResult,
            presented.submitDriverResult, presented.fenceWaitDriverResult, presented.presentDriverResult);
        std::fprintf(stderr, "SDL_ERROR=%s\n", SDL_GetError());
        return 3;
    }

    std::printf("FARM_VULKAN_TEXTURED_PRESENT_SMOKE_OK farm_hash=%llu texture_hash=%llu swapchain=%u upload=1 submit=1 present=1 lessons=%u\n",
        static_cast<unsigned long long>(farmFrameHash), static_cast<unsigned long long>(presented.textureHash), presented.imageCount,
        completedLessons);
    return 0;
}
