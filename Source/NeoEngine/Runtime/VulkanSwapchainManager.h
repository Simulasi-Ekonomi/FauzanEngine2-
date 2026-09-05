#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanSwapchainManager {
public:
    VulkanSwapchainManager() = default;
    ~VulkanSwapchainManager();

    VulkanSwapchainManager(const VulkanSwapchainManager&) = delete;
    VulkanSwapchainManager& operator=(const VulkanSwapchainManager&) = delete;

    VulkanSwapchainManager(VulkanSwapchainManager&& other) noexcept;
    VulkanSwapchainManager& operator=(VulkanSwapchainManager&& other) noexcept;

    bool Initialize(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkSurfaceKHR surface,
                    uint32_t width,
                    uint32_t height);

    VkResult AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t* outImageIndex);
    VkResult Present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex);

    void Destroy();

    [[nodiscard]] VkSwapchainKHR GetSwapchain() const { return swapchain_; }
    [[nodiscard]] const std::vector<VkImage>& GetImages() const { return swapchainImages_; }
    [[nodiscard]] const std::vector<VkImageView>& GetImageViews() const { return swapchainImageViews_; }
    [[nodiscard]] VkFormat GetImageFormat() const { return swapchainImageFormat_; }
    [[nodiscard]] VkExtent2D GetExtent() const { return swapchainExtent_; }
    [[nodiscard]] bool IsValid() const { return swapchain_ != VK_NULL_HANDLE && !swapchainImageViews_.empty(); }

    static SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    VkFormat swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent_ = {0, 0};
};

} // namespace NeoEngine
