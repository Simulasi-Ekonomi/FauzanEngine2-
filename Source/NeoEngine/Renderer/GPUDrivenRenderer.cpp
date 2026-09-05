#include "GPUDrivenRenderer.h"

#include <algorithm>
#include <cstring>
#include <limits>

uint32_t GPUDrivenRenderer::FindMemoryType(VkPhysicalDevice physicalDevice,
                                            uint32_t typeFilter,
                                            VkMemoryPropertyFlags properties) {
    if (physicalDevice == VK_NULL_HANDLE) {
        return UINT32_MAX;
    }

    VkPhysicalDeviceMemoryProperties memoryProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) != 0u &&
            (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

void GPUDrivenRenderer::Initialize(VkDevice device) {
    Destroy();
    device_ = device;
    initialized_ = device != VK_NULL_HANDLE;
    maxCommands_ = initialized_ ? MaxCommands : 0;
    commands.clear();
    commands.reserve(MaxCommands);
}

bool GPUDrivenRenderer::Initialize(VkDevice device,
                                   VkPhysicalDevice physicalDevice,
                                   std::size_t maxCommands) {
    Destroy();

    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || maxCommands == 0) {
        return false;
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    const std::size_t deviceLimit =
        static_cast<std::size_t>(properties.limits.maxDrawIndirectCount);
    const std::size_t requested = std::min(maxCommands, MaxCommands);
    maxCommands_ = std::min(requested, deviceLimit);
    if (maxCommands_ == 0 || maxCommands_ >
        (std::numeric_limits<VkDeviceSize>::max() / sizeof(GPUIndirectCommand))) {
        Destroy();
        return false;
    }

    device_ = device;
    physicalDevice_ = physicalDevice;
    commands.clear();
    commands.reserve(maxCommands_);

    const VkDeviceSize bufferSize =
        static_cast<VkDeviceSize>(maxCommands_) * sizeof(GPUIndirectCommand);

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &indirectBuffer) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, indirectBuffer, &requirements);

    const uint32_t memoryType = FindMemoryType(
        physicalDevice_, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == UINT32_MAX) {
        Destroy();
        return false;
    }

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;

    if (vkAllocateMemory(device_, &allocation, nullptr, &indirectMemory) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    if (vkBindBufferMemory(device_, indirectBuffer, indirectMemory, 0) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    initialized_ = true;
    return true;
}

void GPUDrivenRenderer::Destroy() {
    commands.clear();

    if (device_ != VK_NULL_HANDLE) {
        if (indirectBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, indirectBuffer, nullptr);
            indirectBuffer = VK_NULL_HANDLE;
        }
        if (indirectMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, indirectMemory, nullptr);
            indirectMemory = VK_NULL_HANDLE;
        }
    }

    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    maxCommands_ = 0;
    initialized_ = false;
}

void GPUDrivenRenderer::SubmitDraw(const GPUIndirectCommand& cmd) {
    (void)TrySubmitDraw(cmd);
}

bool GPUDrivenRenderer::TrySubmitDraw(const GPUIndirectCommand& cmd) {
    if (!initialized_ || cmd.indexCount == 0U || cmd.instanceCount == 0U ||
        commands.size() >= maxCommands_) {
        return false;
    }

    commands.push_back(cmd);
    return true;
}

bool GPUDrivenRenderer::Execute(VkCommandBuffer cmdBuffer) {
    if (!initialized_ || indirectBuffer == VK_NULL_HANDLE ||
        cmdBuffer == VK_NULL_HANDLE || commands.empty()) {
        return false;
    }

    const VkDeviceSize byteCount =
        static_cast<VkDeviceSize>(commands.size() * sizeof(GPUIndirectCommand));

    void* mapped = nullptr;
    if (vkMapMemory(device_, indirectMemory, 0, byteCount, 0, &mapped) != VK_SUCCESS) {
        return false;
    }

    std::memcpy(mapped, commands.data(), static_cast<std::size_t>(byteCount));
    vkUnmapMemory(device_, indirectMemory);

    vkCmdBindIndexBuffer(cmdBuffer, VK_NULL_HANDLE, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexedIndirect(cmdBuffer, indirectBuffer, 0,
                             static_cast<uint32_t>(commands.size()),
                             sizeof(GPUIndirectCommand));
    commands.clear();
    return true;
}
