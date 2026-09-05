#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>

namespace NeoEngine {

class VulkanSemaphore {
public:
    VulkanSemaphore() = default;
    ~VulkanSemaphore();

    VulkanSemaphore(const VulkanSemaphore&) = delete;
    VulkanSemaphore& operator=(const VulkanSemaphore&) = delete;

    VulkanSemaphore(VulkanSemaphore&& other) noexcept;
    VulkanSemaphore& operator=(VulkanSemaphore&& other) noexcept;

    bool Initialize(VkDevice device);
    void Destroy();

    [[nodiscard]] VkSemaphore GetSemaphore() const { return semaphore_; }
    [[nodiscard]] bool IsValid() const { return semaphore_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
};

class VulkanFence {
public:
    VulkanFence() = default;
    ~VulkanFence();

    VulkanFence(const VulkanFence&) = delete;
    VulkanFence& operator=(const VulkanFence&) = delete;

    VulkanFence(VulkanFence&& other) noexcept;
    VulkanFence& operator=(VulkanFence&& other) noexcept;

    bool Initialize(VkDevice device, bool signaled = true);
    bool Wait(uint64_t timeout = UINT64_MAX) const;
    bool Reset();
    void Destroy();

    [[nodiscard]] VkFence GetFence() const { return fence_; }
    [[nodiscard]] bool IsValid() const { return fence_ != VK_NULL_HANDLE; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
};

} // namespace NeoEngine
