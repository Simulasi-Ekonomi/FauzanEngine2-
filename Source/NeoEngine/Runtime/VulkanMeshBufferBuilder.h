#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct MeshVertex3D {
    float position[3];
    float normal[3];
    float uv[2];
};

class VulkanMeshBufferBuilder {
public:
    VulkanMeshBufferBuilder() = default;
    ~VulkanMeshBufferBuilder();

    VulkanMeshBufferBuilder(const VulkanMeshBufferBuilder&) = delete;
    VulkanMeshBufferBuilder& operator=(const VulkanMeshBufferBuilder&) = delete;

    VulkanMeshBufferBuilder(VulkanMeshBufferBuilder&& other) noexcept;
    VulkanMeshBufferBuilder& operator=(VulkanMeshBufferBuilder&& other) noexcept;

    bool BuildMesh(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   const std::vector<MeshVertex3D>& vertices,
                   const std::vector<uint32_t>& indices);

    void Destroy();

    [[nodiscard]] const VulkanGPUBuffer& GetVertexBuffer() const { return vertexBuffer_; }
    [[nodiscard]] const VulkanGPUBuffer& GetIndexBuffer() const { return indexBuffer_; }
    [[nodiscard]] uint32_t GetVertexCount() const { return vertexCount_; }
    [[nodiscard]] uint32_t GetIndexCount() const { return indexCount_; }
    [[nodiscard]] bool IsValid() const { return vertexBuffer_.IsValid() && indexBuffer_.IsValid() && indexCount_ > 0; }

private:
    VulkanGPUBuffer vertexBuffer_;
    VulkanGPUBuffer indexBuffer_;
    uint32_t vertexCount_ = 0;
    uint32_t indexCount_ = 0;
};

} // namespace NeoEngine
