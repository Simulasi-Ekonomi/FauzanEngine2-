#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>

namespace NeoEngine {

enum class VulkanBufferType {
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    StagingBuffer
};

class VulkanGPUBuffer {
public:
    VulkanGPUBuffer() = default;
    ~VulkanGPUBuffer();

    VulkanGPUBuffer(const VulkanGPUBuffer&) = delete;
    VulkanGPUBuffer& operator=(const VulkanGPUBuffer&) = delete;

    VulkanGPUBuffer(VulkanGPUBuffer&& other) noexcept;
    VulkanGPUBuffer& operator=(VulkanGPUBuffer&& other) noexcept;

    bool Initialize(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkDeviceSize size,
                    VulkanBufferType type,
                    VkMemoryPropertyFlags memoryProperties);

    bool UploadData(const void* data, VkDeviceSize dataSize);
    bool ReadData(void* outData, VkDeviceSize dataSize) const;

    void Destroy();

    [[nodiscard]] VkBuffer GetBuffer() const { return buffer_; }
    [[nodiscard]] VkDeviceMemory GetMemory() const { return memory_; }
    [[nodiscard]] VkDeviceSize GetSize() const { return size_; }
    [[nodiscard]] VulkanBufferType GetType() const { return type_; }
    [[nodiscard]] VkMemoryPropertyFlags GetMemoryProperties() const { return memoryProperties_; }
    [[nodiscard]] bool IsValid() const { return buffer_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE; }

private:
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    VkDevice device_ = VK_NULL_HANDLE;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    VulkanBufferType type_ = VulkanBufferType::VertexBuffer;
    VkMemoryPropertyFlags memoryProperties_ = 0;
};

} // namespace NeoEngine
