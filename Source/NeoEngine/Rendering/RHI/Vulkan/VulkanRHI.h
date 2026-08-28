#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace NeoEngine {
class VulkanRHI {
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_GPU = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    int m_Width = 0;
    int m_Height = 0;
    bool m_Initialized = false;
    bool m_FrameActive = false;
public:
    static VulkanRHI& Get();
    bool Init(void* nativeWindow, int w, int h, const char* appName = "FauzanEngine");
    void Shutdown();
    void BeginFrame();
    void EndFrame();
    void Present();
    [[nodiscard]] VkDevice GetDevice() const { return m_Device; }
    [[nodiscard]] VkPhysicalDevice GetGPU() const { return m_GPU; }
    [[nodiscard]] VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
    [[nodiscard]] int GetWidth() const { return m_Width; }
    [[nodiscard]] int GetHeight() const { return m_Height; }
    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }
    [[nodiscard]] bool IsFrameActive() const { return m_FrameActive; }
};
}
