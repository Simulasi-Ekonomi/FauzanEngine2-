#include "Rendering/RHI/Vulkan/VulkanRHI.h"
#include <SDL.h>
#include <cstdio>

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;
    SDL_Window* window = SDL_CreateWindow("NeoEngine Vulkan RHI Smoke", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          64, 64, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
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
        rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 4;
    }

    for (int frame = 0; frame < 3; ++frame) {
        rhi.BeginFrame();
        if (!rhi.IsFrameActive()) { rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 5; }
        rhi.EndFrame();
        if (rhi.IsFrameActive()) { rhi.Shutdown(); SDL_DestroyWindow(window); SDL_Quit(); return 6; }
        rhi.Present();
    }

    rhi.Shutdown();
    if (rhi.IsInitialized() || rhi.HasSwapchain() || rhi.GetDevice() != VK_NULL_HANDLE || rhi.IsFrameActive()) {
        SDL_DestroyWindow(window); SDL_Quit(); return 7;
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::puts("VULKAN_RHI_SMOKE_OK real_surface=1 swapchain=1 acquire=1 submit=1 present=1 shutdown=1");
    return 0;
}
