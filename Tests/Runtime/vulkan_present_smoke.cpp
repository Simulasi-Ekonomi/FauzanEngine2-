#include "Runtime/VulkanPresentProbe.h"

#include <cstdio>

int main() {
    using namespace NeoEngine;
    const VulkanPresentProbeResult invalid = VulkanPresentProbe::PresentHiddenFrame(0, 64);
    const VulkanPresentProbeResult result = VulkanPresentProbe::PresentHiddenFrame(64, 64);
    if (invalid.windowCreated || !result.windowCreated || !result.surfaceCreated || !result.deviceCreated || !result.swapchainCreated || !result.frameSubmitted || !result.framePresented || result.imageCount == 0) return 1;
    std::printf("VULKAN_PRESENT_SMOKE_OK surface=1 swapchain=1 submit=1 present=1 images=%u\n", result.imageCount);
    return 0;
}
