#include "Runtime/AssetRegistry.h"
#include "Runtime/TextureStaging.h"
#include "Runtime/VulkanTexturedOffscreen.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> ppm{
        'P', '6', '\n', '2', ' ', '2', '\n', '2', '5', '5', '\n',
        255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 0,
    };
    AssetRegistry registry;
    TextureStagingStore staging;
    if (!registry.ImportBytes("sampled.ppm", AssetKind::Texture, {}, ppm) || !registry.MarkReady("sampled.ppm") || !staging.StagePpm(registry, "sampled.ppm")) return 1;
    const CpuTextureResource* staged = staging.Find("sampled.ppm");
    if (staged == nullptr) return 1;
    RgbaTexture sampled{staged->width, staged->height, staged->rgba};
    RgbaTexture black{2, 2, std::vector<uint8_t>(16, 0)};
    const VulkanTexturedOffscreenResult invalid = VulkanTexturedOffscreenRenderer::Render({});
    const VulkanTexturedOffscreenResult colorFirst = VulkanTexturedOffscreenRenderer::Render(sampled);
    const VulkanTexturedOffscreenResult colorSecond = VulkanTexturedOffscreenRenderer::Render(sampled);
    const VulkanTexturedOffscreenResult blackResult = VulkanTexturedOffscreenRenderer::Render(black);
    if (invalid.deviceCreated || !colorFirst.deviceCreated || !colorFirst.textureUploaded || !colorFirst.pipelineCreated || !colorFirst.commandSubmitted || !colorFirst.pixelsReadback ||
        colorFirst.nonBlackPixelCount == 0 || colorFirst.pixelHash == 0 || !colorSecond.pixelsReadback || colorSecond.pixelHash != colorFirst.pixelHash ||
        !blackResult.pixelsReadback || blackResult.nonBlackPixelCount != 0 || blackResult.pixelHash == colorFirst.pixelHash) return 1;
    std::printf("VULKAN_TEXTURED_OFFSCREEN_SMOKE_OK nonBlack=%u hash=%llu staged=%zu\n", colorFirst.nonBlackPixelCount, static_cast<unsigned long long>(colorFirst.pixelHash), staging.DecodedBytes());
    return 0;
}
