#include "VulkanBootstrap.h"

#include "VulkanContext.h"

#include <vulkan/vulkan.h>

namespace NeoEngine {

VulkanProbeResult VulkanBootstrap::Probe() {
    VulkanContext context;
    if (!context.Initialize()) {
        return {false, false, false, context.PhysicalDeviceCount()};
    }

    VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = context.GraphicsQueueFamily();
    VkCommandPool commandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(context.Device(), &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        return {true, true, false, context.PhysicalDeviceCount()};
    }

    VkCommandBufferAllocateInfo allocation{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocation.commandPool = commandPool;
    allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocation.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool submitted = false;
    if (vkAllocateCommandBuffers(context.Device(), &allocation, &commandBuffer) == VK_SUCCESS) {
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        if (vkBeginCommandBuffer(commandBuffer, &begin) == VK_SUCCESS && vkEndCommandBuffer(commandBuffer) == VK_SUCCESS) {
            VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
            if (vkCreateFence(context.Device(), &fenceInfo, nullptr, &fence) == VK_SUCCESS) {
                VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
                submit.commandBufferCount = 1;
                submit.pCommandBuffers = &commandBuffer;
                submitted = vkQueueSubmit(context.GraphicsQueue(), 1, &submit, fence) == VK_SUCCESS &&
                            vkWaitForFences(context.Device(), 1, &fence, VK_TRUE, 1'000'000'000ULL) == VK_SUCCESS;
            }
        }
    }
    if (fence != VK_NULL_HANDLE) vkDestroyFence(context.Device(), fence, nullptr);
    vkDestroyCommandPool(context.Device(), commandPool, nullptr);
    return {true, true, submitted, context.PhysicalDeviceCount()};
}

} // namespace NeoEngine
