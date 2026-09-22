#pragma once

#include "Runtime/VulkanMeshBufferBuilder.h"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

class GLTFGPUUploader {
public:
    GLTFGPUUploader() = default;
    ~GLTFGPUUploader() = default;
    GLTFGPUUploader(const GLTFGPUUploader&) = delete;
    GLTFGPUUploader& operator=(const GLTFGPUUploader&) = delete;

    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice) noexcept;
    void Destroy() noexcept;
    bool IsValid() const noexcept { return device_ != VK_NULL_HANDLE && physicalDevice_ != VK_NULL_HANDLE; }

    // Uploads a validated glTF mesh into the canonical Vulkan mesh-buffer path.
    // The returned handle remains owned by this uploader until the next UploadMesh
    // or Destroy call; callers must not retain it beyond that lifetime.
    bool UploadMesh(const std::vector<Vertex>& vertices,
                    const std::vector<std::uint32_t>& indices);
    [[nodiscard]] const VulkanMeshBufferBuilder& GetMeshBuffer() const noexcept { return meshBuffer_; }

private:
    VkDevice device_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VulkanMeshBufferBuilder meshBuffer_;
};

} // namespace NeoEngine
