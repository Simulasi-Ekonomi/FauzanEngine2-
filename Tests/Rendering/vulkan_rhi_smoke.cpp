#include "Rendering/RHI/Vulkan/VulkanRHI.h"
#include <cstdio>

int main() {
    using NeoEngine::VulkanRHI;
    VulkanRHI& rhi = VulkanRHI::Get();
    rhi.Shutdown();
    if (rhi.Init(nullptr, 0, 64, "Smoke") || rhi.Init(nullptr, 64, 0, "Smoke") || rhi.Init(nullptr, 64, 64, nullptr)) return 1;
    if (!rhi.Init(nullptr, 64, 64, "FauzanEngineSmoke") || !rhi.IsInitialized() || rhi.GetDevice() == VK_NULL_HANDLE ||
        rhi.GetGPU() == VK_NULL_HANDLE || rhi.GetGraphicsQueue() == VK_NULL_HANDLE || rhi.GetWidth() != 64 || rhi.GetHeight() != 64) return 2;
    if (!rhi.Init(nullptr, 128, 128, "Ignored") || rhi.GetWidth() != 64 || rhi.GetHeight() != 64) return 3;
    rhi.BeginFrame();
    if (!rhi.IsFrameActive()) return 4;
    rhi.BeginFrame();
    rhi.EndFrame();
    if (rhi.IsFrameActive()) return 5;
    rhi.Present();
    rhi.Shutdown();
    if (rhi.IsInitialized() || rhi.GetDevice() != VK_NULL_HANDLE || rhi.IsFrameActive()) return 6;
    std::puts("VULKAN_RHI_SMOKE_OK validation=1 device=1 queue=1 frame_lifecycle=1 shutdown=1");
    return 0;
}
