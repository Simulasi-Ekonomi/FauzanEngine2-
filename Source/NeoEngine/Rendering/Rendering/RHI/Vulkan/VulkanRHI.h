#pragma once
#include "../RHI.h"
#include <vulkan/vulkan.h>

class VulkanRHI : public RHI {
public:
    void Init() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Shutdown() override;
    [[nodiscard]] bool IsInitialized() const { return initialized_; }
    [[nodiscard]] bool IsFrameActive() const { return frameActive_; }
    [[nodiscard]] VkDevice Device() const { return device_; }
    [[nodiscard]] VkPhysicalDevice PhysicalDevice() const { return physicalDevice_; }
private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    bool initialized_ = false;
    bool frameActive_ = false;
};
