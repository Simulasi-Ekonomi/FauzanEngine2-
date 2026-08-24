#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>

namespace NeoEngine {

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext();

    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    bool Initialize();
    void Reset();

    [[nodiscard]] bool Ready() const { return device_ != VK_NULL_HANDLE; }
    [[nodiscard]] VkInstance Instance() const { return instance_; }
    [[nodiscard]] VkPhysicalDevice PhysicalDevice() const { return physicalDevice_; }
    [[nodiscard]] VkDevice Device() const { return device_; }
    [[nodiscard]] VkQueue GraphicsQueue() const { return graphicsQueue_; }
    [[nodiscard]] uint32_t GraphicsQueueFamily() const { return graphicsQueueFamily_; }
    [[nodiscard]] uint32_t PhysicalDeviceCount() const { return physicalDeviceCount_; }

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = UINT32_MAX;
    uint32_t physicalDeviceCount_ = 0;
};

} // namespace NeoEngine
