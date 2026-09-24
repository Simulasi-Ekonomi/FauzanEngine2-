#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <cstddef>
#include <cstddef>
#include <vector>

namespace NeoEngine {

struct MeshVertex3D {
    float position[3];
    float normal[3];
    float uv[2];
    std::uint32_t boneIndices[4]{};
    float boneWeights[4]{};
};

static_assert(offsetof(MeshVertex3D, position) == 0U);
static_assert(offsetof(MeshVertex3D, normal) == 12U);
static_assert(offsetof(MeshVertex3D, uv) == 24U);
static_assert(offsetof(MeshVertex3D, boneIndices) == 32U);
static_assert(offsetof(MeshVertex3D, boneWeights) == 48U);
static_assert(sizeof(MeshVertex3D) == 64U);

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
