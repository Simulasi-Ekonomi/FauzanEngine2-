#pragma once

#include "Runtime/VulkanGPUBuffer.h"
#include "Runtime/VulkanMeshBufferBuilder.h"
#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <limits>
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
               const std::vector<const VulkanMeshBufferBuilder*>& meshes) {
        if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || meshes.empty()) return false;
        Destroy();
        std::vector<MeshVertex3D> vertices;
        std::vector<uint32_t> indices;
        ranges_.reserve(meshes.size());

        for (const VulkanMeshBufferBuilder* mesh : meshes) {
            if (mesh == nullptr || !mesh->IsValid()) { Destroy(); return false; }
            const uint64_t vertexOffset = vertices.size();
            const uint64_t firstIndex = indices.size();
            const uint64_t vertexCount = mesh->GetVertexCount();
            const uint64_t indexCount = mesh->GetIndexCount();
            if (vertexOffset + vertexCount > std::numeric_limits<uint32_t>::max() ||
                firstIndex + indexCount > std::numeric_limits<uint32_t>::max()) { Destroy(); return false; }

            std::vector<MeshVertex3D> meshVertices(mesh->GetVertexCount());
            std::vector<uint32_t> meshIndices(mesh->GetIndexCount());
            if (!mesh->GetVertexBuffer().ReadData(meshVertices.data(), sizeof(MeshVertex3D) * meshVertices.size()) ||
                !mesh->GetIndexBuffer().ReadData(meshIndices.data(), sizeof(uint32_t) * meshIndices.size())) { Destroy(); return false; }
            for (const uint32_t index : meshIndices) {
                if (static_cast<uint64_t>(index) >= vertexCount) { Destroy(); return false; }
            }
            vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
            indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());
            ranges_.push_back({static_cast<uint32_t>(vertexOffset), static_cast<uint32_t>(firstIndex), static_cast<uint32_t>(indexCount)});
        }

        if (vertices.empty() || indices.empty()) { Destroy(); return false; }
        const VkDeviceSize vertexBytes = sizeof(MeshVertex3D) * vertices.size();
        const VkDeviceSize indexBytes = sizeof(uint32_t) * indices.size();
        if (!vertexBuffer_.Initialize(device, physicalDevice, vertexBytes, VulkanBufferType::VertexBuffer,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
            !vertexBuffer_.UploadData(vertices.data(), vertexBytes) ||
            !indexBuffer_.Initialize(device, physicalDevice, indexBytes, VulkanBufferType::IndexBuffer,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
            !indexBuffer_.UploadData(indices.data(), indexBytes)) { Destroy(); return false; }
        return true;
    }

    void Destroy() { vertexBuffer_.Destroy(); indexBuffer_.Destroy(); ranges_.clear(); }
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
