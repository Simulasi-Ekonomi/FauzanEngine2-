#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

namespace NeoEngine
{

class VulkanGPUBuffer;

class MeshStreamer
{
public:
    MeshStreamer() = default;
    ~MeshStreamer() = default;

    MeshStreamer(const MeshStreamer&) = delete;
    MeshStreamer& operator=(const MeshStreamer&) = delete;
    MeshStreamer(MeshStreamer&&) noexcept = default;
    MeshStreamer& operator=(MeshStreamer&&) noexcept = default;

    [[nodiscard]] bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize capacityBytes);
    [[nodiscard]] bool UploadMesh(const void* vertices, VkDeviceSize byteCount);
    void Destroy();

    [[nodiscard]] bool IsValid() const;
    [[nodiscard]] VkBuffer GetBuffer() const;
    [[nodiscard]] VkDeviceSize GetCapacityBytes() const { return capacityBytes_; }
    [[nodiscard]] VkDeviceSize GetUploadedBytes() const { return uploadedBytes_; }

private:
    VulkanGPUBuffer* Buffer();
    const VulkanGPUBuffer* Buffer() const;

    VulkanGPUBuffer* buffer_ = nullptr;
    VkDeviceSize capacityBytes_ = 0;
    VkDeviceSize uploadedBytes_ = 0;
};

}

