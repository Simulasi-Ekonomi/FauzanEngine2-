#include "MeshStreamer.h"

namespace NeoEngine {

MeshStreamer::~MeshStreamer() {
    Destroy();
}

bool MeshStreamer::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, std::size_t capacityBytes) {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || capacityBytes == 0) {
        return false;
    }

    Destroy();
    device_ = device;
    physicalDevice_ = physicalDevice;

    if (!buffer_.Initialize(device_, physicalDevice_, static_cast<VkDeviceSize>(capacityBytes),
                            VulkanBufferType::VertexBuffer,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        Destroy();
        return false;
    }

    uploadedBytes_ = 0;
    return true;
}

bool MeshStreamer::UploadMesh(const void* vertices, std::size_t bytes) {
    if (!IsValid() || vertices == nullptr || bytes == 0 || bytes > GetCapacityBytes()) {
        return false;
    }

    if (!buffer_.UploadData(vertices, static_cast<VkDeviceSize>(bytes))) {
        return false;
    }

    uploadedBytes_ = bytes;
    return true;
}

void MeshStreamer::Destroy() {
    buffer_.Destroy();
    device_ = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
    uploadedBytes_ = 0;
}

} // namespace NeoEngine
