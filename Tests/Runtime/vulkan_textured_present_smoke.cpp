#include "Runtime/AssetRegistry.h"
#include "Runtime/TextureStaging.h"
#include "Runtime/VulkanTexturedPresent.h"

#include <cstdint>
#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{
        'P', '6', '\n', '2', ' ', '2', '\n', '2', '5', '5', '\n',
        255, 32, 32, 32, 255, 32, 32, 32, 255, 255, 255, 32,
    };
    AssetRegistry registry;
    TextureStagingStore staging;
    if (!registry.ImportBytes("present.ppm", AssetKind::Texture, {}, ppm) || !registry.MarkReady("present.ppm") ||
        !staging.StagePpm(registry, "present.ppm")) return 1;
    const CpuTextureResource* staged = staging.Find("present.ppm");
    if (staged == nullptr || staged->sourceHash == 0 || staged->rgba.empty()) return 2;
    const auto invalid = VulkanTexturedPresentProbe::Present(*staged, 0, 64);
    if (invalid.windowCreated) return 3;
    const auto first = VulkanTexturedPresentProbe::Present(*staged, 64, 64);
    if (!first.windowCreated || !first.surfaceCreated || !first.deviceCreated || !first.swapchainCreated ||
        !first.textureUploaded || !first.pipelineCreated || !first.frameSubmitted || !first.framePresented ||
        first.imageCount < 2 || first.textureHash == 0 || first.stagedSourceHash != staged->sourceHash) return 4;
    const auto second = VulkanTexturedPresentProbe::Present(*staged, 64, 64);
    if (!second.framePresented || second.textureHash != first.textureHash || second.stagedSourceHash != first.stagedSourceHash) return 5;
    std::printf("VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=%d swapchain=%d upload=%d pipeline=%d submit=%d present=%d images=%u staged=%d hash=%llu\n",
                first.surfaceCreated, first.swapchainCreated, first.textureUploaded, first.pipelineCreated,
                first.frameSubmitted, first.framePresented, first.imageCount, first.stagedSourceHash != 0,
                static_cast<unsigned long long>(first.textureHash));
    return 0;
}
