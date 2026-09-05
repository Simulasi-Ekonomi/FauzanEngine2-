#include "Runtime/VulkanSwapchainManager.h"
#include <algorithm>
#include <limits>
#include <utility>

namespace NeoEngine {

VulkanSwapchainManager::~VulkanSwapchainManager() {
    Destroy();
}

VulkanSwapchainManager::VulkanSwapchainManager(VulkanSwapchainManager&& other) noexcept {
    device_ = other.device_;
    swapchain_ = other.swapchain_;
    swapchainImages_ = std::move(other.swapchainImages_);
    swapchainImageViews_ = std::move(other.swapchainImageViews_);
    swapchainImageFormat_ = other.swapchainImageFormat_;
    swapchainExtent_ = other.swapchainExtent_;

    other.device_ = VK_NULL_HANDLE;
    other.swapchain_ = VK_NULL_HANDLE;
    other.swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    other.swapchainExtent_ = {0, 0};
}

VulkanSwapchainManager& VulkanSwapchainManager::operator=(VulkanSwapchainManager&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        swapchain_ = other.swapchain_;
        swapchainImages_ = std::move(other.swapchainImages_);
        swapchainImageViews_ = std::move(other.swapchainImageViews_);
        swapchainImageFormat_ = other.swapchainImageFormat_;
        swapchainExtent_ = other.swapchainExtent_;

        other.device_ = VK_NULL_HANDLE;
        other.swapchain_ = VK_NULL_HANDLE;
        other.swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
        other.swapchainExtent_ = {0, 0};
    }
    return *this;
}

SwapchainSupportDetails VulkanSwapchainManager::QuerySwapchainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    SwapchainSupportDetails details;
    if (physicalDevice == VK_NULL_HANDLE || surface == VK_NULL_HANDLE) {
        return details;
    }

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanSwapchainManager::Initialize(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       VkSurfaceKHR surface,
                                       uint32_t width,
                                       uint32_t height) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || surface == VK_NULL_HANDLE || width == 0 || height == 0) {
        return false;
    }

    Destroy();
    device_ = device;

    SwapchainSupportDetails swapchainSupport = QuerySwapchainSupport(physicalDevice, surface);
    if (swapchainSupport.formats.empty() || swapchainSupport.presentModes.empty()) {
        Destroy();
        return false;
    }

    // 1. Choose Surface Format
    VkSurfaceFormatKHR surfaceFormat = swapchainSupport.formats[0];
    for (const auto& availableFormat : swapchainSupport.formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    // 2. Choose Present Mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : swapchainSupport.presentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = availablePresentMode;
            break;
        }
    }

    // 3. Choose Swap Extent
    VkExtent2D extent;
    if (swapchainSupport.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        extent = swapchainSupport.capabilities.currentExtent;
    } else {
        extent.width = std::clamp(width, swapchainSupport.capabilities.minImageExtent.width, swapchainSupport.capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, swapchainSupport.capabilities.minImageExtent.height, swapchainSupport.capabilities.maxImageExtent.height);
    }

    // Image Count
    uint32_t imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0 && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    // Retrieve Images
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    swapchainImageFormat_ = surfaceFormat.format;
    swapchainExtent_ = extent;

    // Create Image Views
    swapchainImageViews_.resize(swapchainImages_.size());
    for (size_t i = 0; i < swapchainImages_.size(); i++) {
        VkImageViewCreateInfo createViewInfo{};
        createViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createViewInfo.image = swapchainImages_[i];
        createViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createViewInfo.format = swapchainImageFormat_;
        createViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createViewInfo.subresourceRange.baseMipLevel = 0;
        createViewInfo.subresourceRange.levelCount = 1;
        createViewInfo.subresourceRange.baseArrayLayer = 0;
        createViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device_, &createViewInfo, nullptr, &swapchainImageViews_[i]) != VK_SUCCESS) {
            Destroy();
            return false;
        }
    }

    return true;
}

VkResult VulkanSwapchainManager::AcquireNextImage(VkSemaphore imageAvailableSemaphore, uint32_t* outImageIndex) {
    if (!IsValid() || outImageIndex == nullptr) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    return vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, outImageIndex);
}

VkResult VulkanSwapchainManager::Present(VkQueue presentQueue, VkSemaphore renderFinishedSemaphore, uint32_t imageIndex) {
    if (!IsValid() || presentQueue == VK_NULL_HANDLE) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    if (renderFinishedSemaphore != VK_NULL_HANDLE) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
    }
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    return vkQueuePresentKHR(presentQueue, &presentInfo);
}

void VulkanSwapchainManager::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        for (auto imageView : swapchainImageViews_) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(device_, imageView, nullptr);
            }
        }
        swapchainImageViews_.clear();
        swapchainImages_.clear();

        if (swapchain_ != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device_, swapchain_, nullptr);
            swapchain_ = VK_NULL_HANDLE;
        }
        device_ = VK_NULL_HANDLE;
    }
    swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    swapchainExtent_ = {0, 0};
}

} // namespace NeoEngine
