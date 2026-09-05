#include "Runtime/VulkanMeshBatchBuffer.h"
#include <limits>

namespace NeoEngine {

bool VulkanMeshBatchBuffer::Build(VkDevice device, VkPhysicalDevice physicalDevice,
                                  const std::vector<const VulkanMeshBufferBuilder*>& meshes) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || meshes.empty()) {
        return false;
    }

    Destroy();

    std::vector<MeshVertex3D> vertices;
    std::vector<uint32_t> indices;
    ranges_.reserve(meshes.size());

    for (const VulkanMeshBufferBuilder* mesh : meshes) {
        if (mesh == nullptr || !mesh->IsValid()) {
            Destroy();
            return false;
        }

        const uint64_t vertexOffset = vertices.size();
        const uint64_t firstIndex = indices.size();
        const uint64_t vertexCount = mesh->GetVertexCount();
        const uint64_t indexCount = mesh->GetIndexCount();
        if (vertexOffset + vertexCount > std::numeric_limits<uint32_t>::max() ||
            firstIndex + indexCount > std::numeric_limits<uint32_t>::max()) {
            Destroy();
            return false;
        }

        std::vector<MeshVertex3D> meshVertices(mesh->GetVertexCount());
        std::vector<uint32_t> meshIndices(mesh->GetIndexCount());
        if (!mesh->GetVertexBuffer().ReadData(meshVertices.data(), sizeof(MeshVertex3D) * meshVertices.size()) ||
            !mesh->GetIndexBuffer().ReadData(meshIndices.data(), sizeof(uint32_t) * meshIndices.size())) {
            Destroy();
            return false;
        }

        // Keep each mesh's original local indices. vkCmdDrawIndexedIndirect
        // supplies vertexOffset separately, avoiding double application.
        for (uint32_t index : meshIndices) {
            if (static_cast<uint64_t>(index) >= vertexCount) {
                Destroy();
                return false;
            }
        }

        vertices.insert(vertices.end(), meshVertices.begin(), meshVertices.end());
        indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());
        ranges_.push_back({static_cast<uint32_t>(vertexOffset),
                           static_cast<uint32_t>(firstIndex),
                           static_cast<uint32_t>(indexCount)});
    }

    if (vertices.empty() || indices.empty()) {
        Destroy();
        return false;
    }

    if (!vertexBuffer_.Initialize(device, physicalDevice,
                                  sizeof(MeshVertex3D) * vertices.size(),
                                  VulkanBufferType::VertexBuffer,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !vertexBuffer_.UploadData(vertices.data(), sizeof(MeshVertex3D) * vertices.size())) {
        Destroy();
        return false;
    }

    if (!indexBuffer_.Initialize(device, physicalDevice,
                                 sizeof(uint32_t) * indices.size(),
                                 VulkanBufferType::IndexBuffer,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !indexBuffer_.UploadData(indices.data(), sizeof(uint32_t) * indices.size())) {
        Destroy();
        return false;
    }

    return true;
}

void VulkanMeshBatchBuffer::Destroy() {
    vertexBuffer_.Destroy();
    indexBuffer_.Destroy();
    ranges_.clear();
}

} // namespace NeoEngine
