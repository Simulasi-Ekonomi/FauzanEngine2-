#include "MeshStreamer.h"

namespace NeoEngine
{

bool MeshStreamer::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize capacityBytes)
{
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || capacityBytes == 0) {
        return false;
    }

    Destroy();

    if (!buffer_.Initialize(
            device,
            physicalDevice,
            capacityBytes,
            VulkanBufferType::VertexBuffer,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    capacityBytes_ = capacityBytes;
    uploadedBytes_ = 0;
    return true;
}

bool MeshStreamer::UploadMesh(const void* vertices, VkDeviceSize byteCount)
{
    if (!IsValid() || vertices == nullptr || byteCount == 0 || byteCount > capacityBytes_) {
        return false;
    }

    if (!buffer_.UploadData(vertices, byteCount)) {
        return false;
    }

    uploadedBytes_ = byteCount;
    return true;
}

void MeshStreamer::Destroy()
{
    buffer_.Destroy();
    capacityBytes_ = 0;
    uploadedBytes_ = 0;
}

}
