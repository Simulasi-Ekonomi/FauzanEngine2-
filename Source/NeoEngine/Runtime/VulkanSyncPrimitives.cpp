#include "Runtime/VulkanSyncPrimitives.h"
#include <utility>

namespace NeoEngine {

// --- VulkanSemaphore ---

VulkanSemaphore::~VulkanSemaphore() {
    Destroy();
}

VulkanSemaphore::VulkanSemaphore(VulkanSemaphore&& other) noexcept {
    device_ = other.device_;
    semaphore_ = other.semaphore_;

    other.device_ = VK_NULL_HANDLE;
    other.semaphore_ = VK_NULL_HANDLE;
}

VulkanSemaphore& VulkanSemaphore::operator=(VulkanSemaphore&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        semaphore_ = other.semaphore_;

        other.device_ = VK_NULL_HANDLE;
        other.semaphore_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool VulkanSemaphore::Initialize(VkDevice device) {
    if (device == VK_NULL_HANDLE) {
        return false;
    }

    Destroy();
    device_ = device;

    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device_, &createInfo, nullptr, &semaphore_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

void VulkanSemaphore::Destroy() {
    if (device_ != VK_NULL_HANDLE && semaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device_, semaphore_, nullptr);
        semaphore_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
}

// --- VulkanFence ---

VulkanFence::~VulkanFence() {
    Destroy();
}

VulkanFence::VulkanFence(VulkanFence&& other) noexcept {
    device_ = other.device_;
    fence_ = other.fence_;

    other.device_ = VK_NULL_HANDLE;
    other.fence_ = VK_NULL_HANDLE;
}

VulkanFence& VulkanFence::operator=(VulkanFence&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        fence_ = other.fence_;

        other.device_ = VK_NULL_HANDLE;
        other.fence_ = VK_NULL_HANDLE;
    }
    return *this;
}

bool VulkanFence::Initialize(VkDevice device, bool signaled) {
    if (device == VK_NULL_HANDLE) {
        return false;
    }

    Destroy();
    device_ = device;

    VkFenceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled) {
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    }

    if (vkCreateFence(device_, &createInfo, nullptr, &fence_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

bool VulkanFence::Wait(uint64_t timeout) const {
    if (!IsValid()) {
        return false;
    }
    return vkWaitForFences(device_, 1, &fence_, VK_TRUE, timeout) == VK_SUCCESS;
}

bool VulkanFence::Reset() {
    if (!IsValid()) {
        return false;
    }
    return vkResetFences(device_, 1, &fence_) == VK_SUCCESS;
}

void VulkanFence::Destroy() {
    if (device_ != VK_NULL_HANDLE && fence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device_, fence_, nullptr);
        fence_ = VK_NULL_HANDLE;
    }
    device_ = VK_NULL_HANDLE;
}

} // namespace NeoEngine
