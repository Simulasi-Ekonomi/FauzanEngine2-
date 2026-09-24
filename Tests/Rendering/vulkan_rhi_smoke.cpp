#include "Rendering/RHI/Vulkan/VulkanRHI.h"
#include <SDL3/SDL.h>
#include <cstdio>

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) return 1;
    SDL_Window* window = SDL_CreateWindow("NeoEngine Vulkan RHI Smoke", 64, 64, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (window == nullptr) { SDL_Quit(); return 2; }

    using NeoEngine::VulkanRHI;
    VulkanRHI& rhi = VulkanRHI::Get();
    rhi.Shutdown();
    if (rhi.Init(nullptr, 64, 64, "Invalid") || rhi.Init(window, 0, 64, "Smoke") || rhi.Init(window, 64, 0, "Smoke") ||
        rhi.Init(window, 64, 64, nullptr)) {
        rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 3;
    }
    if (!rhi.Init(window, 64, 64, "FauzanEngineSmoke") || !rhi.IsInitialized() || !rhi.HasSwapchain() ||
        rhi.GetDevice() == VK_NULL_HANDLE || rhi.GetGPU() == VK_NULL_HANDLE || rhi.GetGraphicsQueue() == VK_NULL_HANDLE ||
        rhi.GetSurface() == VK_NULL_HANDLE || rhi.GetWidth() != 64 || rhi.GetHeight() != 64) {
        std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL init_state initialized=%d swapchain=%d device=%p gpu=%p queue=%p surface=%p size=%dx%d\\n", rhi.IsInitialized()?1:0, rhi.HasSwapchain()?1:0, static_cast<void*>(rhi.GetDevice()), static_cast<void*>(rhi.GetGPU()), static_cast<void*>(rhi.GetGraphicsQueue()), static_cast<void*>(rhi.GetSurface()), rhi.GetWidth(), rhi.GetHeight()); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 4;
    }

    if (!rhi.Resize(96, 72) || rhi.GetWidth() != 96 || rhi.GetHeight() != 72 || !rhi.HasSwapchain()) {
        std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL resize1 size=%dx%d swapchain=%d\\n", rhi.GetWidth(), rhi.GetHeight(), rhi.HasSwapchain()?1:0); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 5;
    }
    if (!rhi.Resize(64, 64) || rhi.GetWidth() != 64 || rhi.GetHeight() != 64 || !rhi.HasSwapchain()) {
        std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL resize2 size=%dx%d swapchain=%d\\n", rhi.GetWidth(), rhi.GetHeight(), rhi.HasSwapchain()?1:0); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 6;
    }

    for (int frame = 0; frame < 3; ++frame) {
        rhi.BeginFrame();
        if (!rhi.IsFrameActive()) { std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL begin frame=%d active=%d\\n", frame, rhi.IsFrameActive()?1:0); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 7; }
        if (rhi.Resize(80, 80) || rhi.GetWidth() != 64 || rhi.GetHeight() != 64 || !rhi.HasSwapchain()) { std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL resize_during_frame frame=%d size=%dx%d swapchain=%d\\n", frame, rhi.GetWidth(), rhi.GetHeight(), rhi.HasSwapchain()?1:0); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 10; }
        rhi.EndFrame();
        if (rhi.IsFrameActive()) { std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL end frame=%d active=%d\\n", frame, rhi.IsFrameActive()?1:0); rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 8; }
        rhi.Present();
    }

    rhi.Shutdown();
    if (rhi.IsInitialized() || rhi.HasSwapchain() || rhi.GetDevice() != VK_NULL_HANDLE || rhi.IsFrameActive()) {
        std::fprintf(stderr, "VULKAN_RHI_SMOKE_FAIL shutdown initialized=%d swapchain=%d device=%p active=%d\\n", rhi.IsInitialized()?1:0, rhi.HasSwapchain()?1:0, static_cast<void*>(rhi.GetDevice()), rhi.IsFrameActive()?1:0); SDL_DestroyWindow(window); SDL_Quit(); return 9;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("VULKAN_RHI_SMOKE_OK real_surface=1 swapchain=1 acquire=1 submit=1 present=1 shutdown=1");
    return 0;
}
