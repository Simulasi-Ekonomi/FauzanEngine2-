#include "Rendering/Rendering/RHI/Vulkan/VulkanRHI.h"
#include <cstdio>

int main() {
    VulkanRHI rhi;
    rhi.Init();
    if (!rhi.IsInitialized() || rhi.Device() == VK_NULL_HANDLE || rhi.PhysicalDevice() == VK_NULL_HANDLE) return 1;
    rhi.BeginFrame();
    if (!rhi.IsFrameActive()) return 2;
    rhi.BeginFrame();
    rhi.EndFrame();
    if (rhi.IsFrameActive()) return 3;
    rhi.Shutdown();
    if (rhi.IsInitialized() || rhi.Device() != VK_NULL_HANDLE) return 4;
    std::puts("LEGACY_VULKAN_RHI_SMOKE_OK device=1 frame_lifecycle=1 shutdown=1");
    return 0;
}
