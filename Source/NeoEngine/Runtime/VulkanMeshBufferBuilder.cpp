#include "Runtime/VulkanMeshBufferBuilder.h"
#include <utility>

namespace NeoEngine {

VulkanMeshBufferBuilder::~VulkanMeshBufferBuilder() {
    Destroy();
}

VulkanMeshBufferBuilder::VulkanMeshBufferBuilder(VulkanMeshBufferBuilder&& other) noexcept {
    vertexBuffer_ = std::move(other.vertexBuffer_);
    indexBuffer_ = std::move(other.indexBuffer_);
    vertexCount_ = other.vertexCount_;
    indexCount_ = other.indexCount_;

    other.vertexCount_ = 0;
    other.indexCount_ = 0;
}

VulkanMeshBufferBuilder& VulkanMeshBufferBuilder::operator=(VulkanMeshBufferBuilder&& other) noexcept {
    if (this != &other) {
        Destroy();

        vertexBuffer_ = std::move(other.vertexBuffer_);
        indexBuffer_ = std::move(other.indexBuffer_);
        vertexCount_ = other.vertexCount_;
        indexCount_ = other.indexCount_;

        other.vertexCount_ = 0;
        other.indexCount_ = 0;
    }
    return *this;
}

bool VulkanMeshBufferBuilder::BuildMesh(VkDevice device,
                                       VkPhysicalDevice physicalDevice,
                                       const std::vector<MeshVertex3D>& vertices,
                                       const std::vector<uint32_t>& indices) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || vertices.empty() || indices.empty()) {
        return false;
    }

    Destroy();

    VkDeviceSize vertexSize = sizeof(MeshVertex3D) * vertices.size();
    VkDeviceSize indexSize = sizeof(uint32_t) * indices.size();

    // 1. Initialize Vertex Buffer
    if (!vertexBuffer_.Initialize(device, physicalDevice, vertexSize, VulkanBufferType::VertexBuffer,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    if (!vertexBuffer_.UploadData(vertices.data(), vertexSize)) {
        Destroy();
        return false;
    }

    // 2. Initialize Index Buffer
    if (!indexBuffer_.Initialize(device, physicalDevice, indexSize, VulkanBufferType::IndexBuffer,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    if (!indexBuffer_.UploadData(indices.data(), indexSize)) {
        Destroy();
        return false;
    }

    vertexCount_ = static_cast<uint32_t>(vertices.size());
    indexCount_ = static_cast<uint32_t>(indices.size());

    return true;
}

void VulkanMeshBufferBuilder::Destroy() {
    vertexBuffer_.Destroy();
    indexBuffer_.Destroy();
    vertexCount_ = 0;
    indexCount_ = 0;
}

} // namespace NeoEngine
