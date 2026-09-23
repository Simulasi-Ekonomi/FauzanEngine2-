#pragma once

#include <vulkan/vulkan.h>
#include <cstddef>
#include "Runtime/VulkanGPUBuffer.h"

namespace NeoEngine {

class MeshStreamer {
public:
    MeshStreamer() = default;
    ~MeshStreamer();

    MeshStreamer(const MeshStreamer&) = delete;
    MeshStreamer& operator=(const MeshStreamer&) = delete;
    MeshStreamer(MeshStreamer&&) noexcept = default;
    MeshStreamer& operator=(MeshStreamer&&) noexcept = default;

    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, std::size_t capacityBytes);
    bool UploadMesh(const void* vertices, std::size_t bytes);
    void Destroy();

    [[nodiscard]] bool IsValid() const noexcept { return buffer_.IsValid(); }
    [[nodiscard]] VkBuffer GetBuffer() const noexcept { return buffer_.GetBuffer(); }
    [[nodiscard]] std::size_t GetCapacityBytes() const noexcept {
        return static_cast<std::size_t>(buffer_.GetSize());
    }
    [[nodiscard]] std::size_t GetUploadedBytes() const noexcept { return uploadedBytes_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VulkanGPUBuffer buffer_;
    std::size_t uploadedBytes_ = 0;
};

} // namespace NeoEngine
