#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include <vulkan/vulkan.h>

namespace NeoEngine
{

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

    [[nodiscard]] bool IsValid() const { return buffer_.IsValid(); }
    [[nodiscard]] VkBuffer GetBuffer() const { return buffer_.GetBuffer(); }
    [[nodiscard]] VkDeviceSize GetCapacityBytes() const { return capacityBytes_; }
    [[nodiscard]] VkDeviceSize GetUploadedBytes() const { return uploadedBytes_; }

private:
    VulkanGPUBuffer buffer_;
    VkDeviceSize capacityBytes_ = 0;
    VkDeviceSize uploadedBytes_ = 0;
};

}
