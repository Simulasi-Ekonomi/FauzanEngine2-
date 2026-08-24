#pragma once
#include "../RHI.h"
#include <vulkan/vulkan.h>

class VulkanRHI : public RHI {
public:
    void Init() override;
    void BeginFrame() override;
    void EndFrame() override;
    void Shutdown() override;

private:
    VkInstance Instance = VK_NULL_HANDLE;
    VkDevice Device = VK_NULL_HANDLE;
    VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
};
