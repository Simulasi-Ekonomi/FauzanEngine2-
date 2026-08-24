#include "Runtime/VulkanOffscreen.h"

#include <cstdio>

int main() {
    const auto zeroExtent = NeoEngine::VulkanOffscreenRenderer::RenderTriangle(0, 64);
    const auto oversizedExtent = NeoEngine::VulkanOffscreenRenderer::RenderTriangle(2049, 64);
    const auto first = NeoEngine::VulkanOffscreenRenderer::RenderTriangle();
    const auto second = NeoEngine::VulkanOffscreenRenderer::RenderTriangle();
    if (zeroExtent.deviceCreated || oversizedExtent.deviceCreated ||
        !first.deviceCreated || !first.pipelineCreated || !first.commandSubmitted || !first.pixelsReadback ||
        first.width != 64 || first.height != 64 || first.nonClearPixelCount == 0 || first.pixelHash == 0 ||
        first.pixelHash != second.pixelHash || first.nonClearPixelCount != second.nonClearPixelCount) {
        return 1;
    }
    std::printf(
        "VULKAN_OFFSCREEN_SMOKE_OK pixels=%u hash=%llu size=%ux%u\n",
        first.nonClearPixelCount,
        static_cast<unsigned long long>(first.pixelHash),
        first.width,
        first.height);
    return 0;
}
