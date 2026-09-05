#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanMeshBufferBuilder.h"
#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace NeoEngine {

struct VulkanMeshBatchRange {
    uint32_t vertexOffset = 0;
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
};

// R3 shared geometry arena. It packs multiple existing mesh buffers into one
// vertex buffer and one index buffer so a single indirect draw stream can
// address all meshes through firstIndex/vertexOffset.
class VulkanMeshBatchBuffer {
public:
    VulkanMeshBatchBuffer() = default;
    ~VulkanMeshBatchBuffer() { Destroy(); }

    VulkanMeshBatchBuffer(const VulkanMeshBatchBuffer&) = delete;
    VulkanMeshBatchBuffer& operator=(const VulkanMeshBatchBuffer&) = delete;

    bool Build(VkDevice device, VkPhysicalDevice physicalDevice,
               const std::vector<const VulkanMeshBufferBuilder*>& meshes);
    void Destroy();

    [[nodiscard]] bool IsValid() const { return vertexBuffer_.IsValid() && indexBuffer_.IsValid() && !ranges_.empty(); }
    [[nodiscard]] const VulkanGPUBuffer& GetVertexBuffer() const { return vertexBuffer_; }
    [[nodiscard]] const VulkanGPUBuffer& GetIndexBuffer() const { return indexBuffer_; }
    [[nodiscard]] const VulkanMeshBatchRange& GetRange(std::size_t i) const { return ranges_.at(i); }
    [[nodiscard]] std::size_t MeshCount() const { return ranges_.size(); }

private:
    VulkanGPUBuffer vertexBuffer_;
    VulkanGPUBuffer indexBuffer_;
    std::vector<VulkanMeshBatchRange> ranges_;
};

} // namespace NeoEngine
