#include "Runtime/VulkanTexturedPresent.h"

#include <cassert>
#include <cstdint>
#include <cstdio>

int main() {
    NeoEngine::RgbaTexture texture{};
    texture.width = 2;
    texture.height = 2;
    texture.rgba = {
        255, 32, 32, 255, 32, 255, 32, 255,
        32, 32, 255, 255, 255, 255, 32, 255,
    };
    const auto invalid = NeoEngine::VulkanTexturedPresentProbe::Present(texture, 0, 64);
    assert(!invalid.windowCreated);
    const auto first = NeoEngine::VulkanTexturedPresentProbe::Present(texture, 64, 64);
    assert(first.windowCreated);
    assert(first.surfaceCreated);
    assert(first.deviceCreated);
    assert(first.swapchainCreated);
    assert(first.textureUploaded);
    assert(first.pipelineCreated);
    assert(first.frameSubmitted);
    assert(first.framePresented);
    assert(first.imageCount >= 2);
    assert(first.textureHash != 0);
    const auto second = NeoEngine::VulkanTexturedPresentProbe::Present(texture, 64, 64);
    assert(second.textureHash == first.textureHash);
    assert(second.framePresented);
    std::printf("VULKAN_TEXTURED_PRESENT_SMOKE_OK surface=%d swapchain=%d upload=%d pipeline=%d submit=%d present=%d images=%u hash=%llu\n",
                first.surfaceCreated, first.swapchainCreated, first.textureUploaded, first.pipelineCreated,
                first.frameSubmitted, first.framePresented, first.imageCount,
                static_cast<unsigned long long>(first.textureHash));
    return 0;
}
