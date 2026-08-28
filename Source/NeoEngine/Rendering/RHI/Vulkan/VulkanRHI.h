#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace NeoEngine {
class VulkanRHI {
    VkInstance m_Instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_GPU = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainViews;
    std::vector<VkFramebuffer> m_Framebuffers;
    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    VkCommandPool m_CommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    VkSemaphore m_ImageAvailable = VK_NULL_HANDLE;
    VkSemaphore m_RenderFinished = VK_NULL_HANDLE;
    VkFence m_InFlight = VK_NULL_HANDLE;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    uint32_t m_QueueFamily = UINT32_MAX;
    uint32_t m_ImageIndex = UINT32_MAX;
    int m_Width = 0;
    int m_Height = 0;
    bool m_Initialized = false;
    bool m_FrameActive = false;
    bool m_ImageAcquired = false;
    bool m_FrameSubmitted = false;

    bool CreateSwapchainResources(uint32_t width, uint32_t height);
    void DestroySwapchainResources();

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
    [[nodiscard]] VkSurfaceKHR GetSurface() const { return m_Surface; }
    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
    [[nodiscard]] int GetWidth() const { return m_Width; }
    [[nodiscard]] int GetHeight() const { return m_Height; }
    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }
    [[nodiscard]] bool IsFrameActive() const { return m_FrameActive; }
    [[nodiscard]] bool HasSwapchain() const { return m_Swapchain != VK_NULL_HANDLE; }
};
}
