#include "Runtime/VulkanGPUBuffer.h"
#include <cstring>
#include <utility>

namespace NeoEngine {

VulkanGPUBuffer::~VulkanGPUBuffer() {
    Destroy();
}

VulkanGPUBuffer::VulkanGPUBuffer(VulkanGPUBuffer&& other) noexcept {
    device_ = other.device_;
    buffer_ = other.buffer_;
    memory_ = other.memory_;
    size_ = other.size_;
    type_ = other.type_;
    memoryProperties_ = other.memoryProperties_;

    other.device_ = VK_NULL_HANDLE;
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.size_ = 0;
    other.memoryProperties_ = 0;
}

VulkanGPUBuffer& VulkanGPUBuffer::operator=(VulkanGPUBuffer&& other) noexcept {
    if (this != &other) {
        Destroy();

        device_ = other.device_;
        buffer_ = other.buffer_;
        memory_ = other.memory_;
        size_ = other.size_;
        type_ = other.type_;
        memoryProperties_ = other.memoryProperties_;

        other.device_ = VK_NULL_HANDLE;
        other.buffer_ = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
        other.size_ = 0;
        other.memoryProperties_ = 0;
    }
    return *this;
}

uint32_t VulkanGPUBuffer::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool VulkanGPUBuffer::Initialize(VkDevice device,
                                VkPhysicalDevice physicalDevice,
                                VkDeviceSize size,
                                VulkanBufferType type,
                                VkMemoryPropertyFlags memoryProperties) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || size == 0) {
        return false;
    }

    Destroy();

    device_ = device;
    size_ = size;
    type_ = type;
    memoryProperties_ = memoryProperties;

    VkBufferUsageFlags usageFlags = 0;
    switch (type) {
        case VulkanBufferType::VertexBuffer:
            usageFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case VulkanBufferType::IndexBuffer:
            usageFlags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case VulkanBufferType::UniformBuffer:
            usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case VulkanBufferType::StagingBuffer:
            usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
    }

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    VkMemoryRequirements memRequirements{};
    vkGetBufferMemoryRequirements(device_, buffer_, &memRequirements);

    uint32_t memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, memoryProperties);
    if (memoryTypeIndex == UINT32_MAX) {
        Destroy();
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device_, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    if (vkBindBufferMemory(device_, buffer_, memory_, 0) != VK_SUCCESS) {
        Destroy();
        return false;
    }

    return true;
}

bool VulkanGPUBuffer::UploadData(const void* data, VkDeviceSize dataSize) {
    if (!IsValid() || data == nullptr || dataSize == 0 || dataSize > size_) {
        return false;
    }

    void* mappedMemory = nullptr;
    if (vkMapMemory(device_, memory_, 0, dataSize, 0, &mappedMemory) != VK_SUCCESS) {
        return false;
    }

    std::memcpy(mappedMemory, data, static_cast<size_t>(dataSize));

    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        VkMappedMemoryRange flushRange{};
        flushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        flushRange.memory = memory_;
        flushRange.offset = 0;
        flushRange.size = dataSize;
        vkFlushMappedMemoryRanges(device_, 1, &flushRange);
    }

    vkUnmapMemory(device_, memory_);

    return true;
}

bool VulkanGPUBuffer::ReadData(void* outData, VkDeviceSize dataSize) const {
    if (!IsValid() || outData == nullptr || dataSize == 0 || dataSize > size_) {
        return false;
    }

    void* mappedMemory = nullptr;
    if (vkMapMemory(device_, memory_, 0, dataSize, 0, &mappedMemory) != VK_SUCCESS) {
        return false;
    }

    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
        VkMappedMemoryRange invalidateRange{};
        invalidateRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        invalidateRange.memory = memory_;
        invalidateRange.offset = 0;
        invalidateRange.size = dataSize;
        vkInvalidateMappedMemoryRanges(device_, 1, &invalidateRange);
    }

    std::memcpy(outData, mappedMemory, static_cast<size_t>(dataSize));
    vkUnmapMemory(device_, memory_);

    return true;
}

void VulkanGPUBuffer::Destroy() {
    if (device_ != VK_NULL_HANDLE) {
        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer_, nullptr);
            buffer_ = VK_NULL_HANDLE;
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
        device_ = VK_NULL_HANDLE;
    }
    size_ = 0;
    memoryProperties_ = 0;
}

} // namespace NeoEngine
