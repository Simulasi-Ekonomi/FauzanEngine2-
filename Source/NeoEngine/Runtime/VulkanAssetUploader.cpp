#include "VulkanAssetUploader.h"
#include <algorithm>
#include <cstring>
#include <limits>

namespace NeoEngine {

namespace {
uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits,
                        VkMemoryPropertyFlags required) noexcept {
    if (physicalDevice == VK_NULL_HANDLE) return std::numeric_limits<uint32_t>::max();
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((typeBits & (1U << i)) &&
            (properties.memoryTypes[i].propertyFlags & required) == required) return i;
    }
    return std::numeric_limits<uint32_t>::max();
}
}

bool VulkanAssetUploader::UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                        const std::vector<uint8_t>& mipData,
                                        VkImage targetImage, VkImageLayout targetLayout) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        mipData.empty() || targetImage == VK_NULL_HANDLE || targetLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        return false;
    const uint32_t requestedMB = static_cast<uint32_t>((mipData.size() + (1024U * 1024U - 1U)) / (1024U * 1024U));
    if (requestedMB > stagingPoolSizeMB_ || currentStagingUsedMB_ > stagingPoolSizeMB_ - requestedMB) return false;
    lastDevice_ = device;

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = AllocateStagingBuffer(device, mipData.size(), stagingMemory);
    if (stagingBuffer == VK_NULL_HANDLE || stagingMemory == VK_NULL_HANDLE) return false;

    void* mapped = nullptr;
    if (vkMapMemory(device, stagingMemory, 0, mipData.size(), 0, &mapped) != VK_SUCCESS ||
        mapped == nullptr) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    std::memcpy(mapped, mipData.data(), mipData.size());
    vkUnmapMemory(device, stagingMemory);

    if (!CopyBufferToImage(device, cmd, stagingBuffer, targetImage, 2048, 2048)) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }

    UploadTask task{};
    task.stagingBuffer = stagingBuffer;
    task.stagingMemory = stagingMemory;
    task.targetImage = targetImage;
    task.targetLayout = targetLayout;
    task.uploadSizeMB = static_cast<uint32_t>((mipData.size() + (1024U * 1024U - 1U)) / (1024U * 1024U));
    pendingUploads_.push_back(task);
    currentStagingUsedMB_ += task.uploadSizeMB;
    return true;
}

bool VulkanAssetUploader::UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& vertexData,
                                     const std::vector<uint8_t>& indexData,
                                     VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        vertexData.empty() || indexData.empty() ||
        vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) return false;
    const uint32_t requestedMB = static_cast<uint32_t>((vertexData.size() + indexData.size() + (1024U * 1024U - 1U)) / (1024U * 1024U));
    if (requestedMB > stagingPoolSizeMB_ || currentStagingUsedMB_ > stagingPoolSizeMB_ - requestedMB) return false;
    lastDevice_ = device;

    VkDeviceMemory vertexMemory = VK_NULL_HANDLE, indexMemory = VK_NULL_HANDLE;
    VkBuffer vertexStaging = AllocateStagingBuffer(device, vertexData.size(), vertexMemory);
    VkBuffer indexStaging = AllocateStagingBuffer(device, indexData.size(), indexMemory);
    if (vertexStaging == VK_NULL_HANDLE || indexStaging == VK_NULL_HANDLE) {
        if (vertexStaging) vkDestroyBuffer(device, vertexStaging, nullptr);
        if (vertexMemory) vkFreeMemory(device, vertexMemory, nullptr);
        if (indexStaging) vkDestroyBuffer(device, indexStaging, nullptr);
        if (indexMemory) vkFreeMemory(device, indexMemory, nullptr);
        return false;
    }

    void* vertexMapped = nullptr;
    void* indexMapped = nullptr;
    if (vkMapMemory(device, vertexMemory, 0, vertexData.size(), 0, &vertexMapped) != VK_SUCCESS ||
        vkMapMemory(device, indexMemory, 0, indexData.size(), 0, &indexMapped) != VK_SUCCESS ||
        !vertexMapped || !indexMapped) {
        if (vertexMapped) vkUnmapMemory(device, vertexMemory);
        if (indexMapped) vkUnmapMemory(device, indexMemory);
        vkDestroyBuffer(device, vertexStaging, nullptr);
        vkFreeMemory(device, vertexMemory, nullptr);
        vkDestroyBuffer(device, indexStaging, nullptr);
        vkFreeMemory(device, indexMemory, nullptr);
        return false;
    }
    std::memcpy(vertexMapped, vertexData.data(), vertexData.size());
    std::memcpy(indexMapped, indexData.data(), indexData.size());
    vkUnmapMemory(device, vertexMemory);
    vkUnmapMemory(device, indexMemory);

    VkBufferCopy vertexRegion{0, 0, vertexData.size()};
    VkBufferCopy indexRegion{0, 0, indexData.size()};
    vkCmdCopyBuffer(cmd, vertexStaging, vertexBuffer, 1, &vertexRegion);
    vkCmdCopyBuffer(cmd, indexStaging, indexBuffer, 1, &indexRegion);

    const uint32_t usedMB = static_cast<uint32_t>(
        (vertexData.size() + indexData.size() + (1024U * 1024U - 1U)) / (1024U * 1024U));
    pendingUploads_.push_back({vertexStaging, vertexMemory, VK_NULL_HANDLE,
                               VK_IMAGE_LAYOUT_UNDEFINED, usedMB, VK_NULL_HANDLE});
    pendingUploads_.push_back({indexStaging, indexMemory, VK_NULL_HANDLE,
                               VK_IMAGE_LAYOUT_UNDEFINED, usedMB, VK_NULL_HANDLE});
    currentStagingUsedMB_ += usedMB;
    return true;
}

void VulkanAssetUploader::Flush(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) return;
    if (vkDeviceWaitIdle(device) != VK_SUCCESS) return;
    for (const UploadTask& task : pendingUploads_) {
        if (task.completionFence) vkDestroyFence(device, task.completionFence, nullptr);
        if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
        if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
    }
    pendingUploads_.clear();
    currentStagingUsedMB_ = 0U;
    lastDevice_ = device;
}

void VulkanAssetUploader::AdvanceFrame(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) return;
    size_t write = 0;
    uint32_t residentMB = 0;
    for (size_t i = 0; i < pendingUploads_.size(); ++i) {
        UploadTask& task = pendingUploads_[i];
        const bool complete = task.completionFence != VK_NULL_HANDLE &&
                              vkGetFenceStatus(device, task.completionFence) == VK_SUCCESS;
        if (complete) {
            vkDestroyFence(device, task.completionFence, nullptr);
            task.completionFence = VK_NULL_HANDLE;
        }
        // UploadTask instances without a completion fence remain owned by the uploader.
        // The caller must attach the submission fence before AdvanceFrame.
        if (task.completionFence == VK_NULL_HANDLE && task.targetImage == VK_NULL_HANDLE) {
            // Buffer-only task cannot be safely reclaimed until its submission fence is attached.
            pendingUploads_[write++] = task;
            residentMB += task.uploadSizeMB;
            continue;
        }
        pendingUploads_[write++] = task;
        residentMB += task.uploadSizeMB;
    }
    pendingUploads_.resize(write);
    currentStagingUsedMB_ = residentMB;
}

VkBuffer VulkanAssetUploader::AllocateStagingBuffer(VkDevice device, size_t size,
                                                    VkDeviceMemory& outMemory) noexcept {
    outMemory = VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || size == 0) return VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return VK_NULL_HANDLE;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t memoryType = FindMemoryType(
        physicalDevice_, requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == std::numeric_limits<uint32_t>::max()) {
        vkDestroyBuffer(device, buffer, nullptr);
        return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;
    if (vkAllocateMemory(device, &allocation, nullptr, &outMemory) != VK_SUCCESS ||
        vkBindBufferMemory(device, buffer, outMemory, 0) != VK_SUCCESS) {
        if (outMemory) vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(device, buffer, nullptr);
        return VK_NULL_HANDLE;
    }
    return buffer;
}

bool VulkanAssetUploader::CopyBufferToImage(VkDevice device, VkCommandBuffer cmd,
                                           VkBuffer stagingBuffer, VkImage targetImage,
                                           uint32_t width, uint32_t height) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE ||
        stagingBuffer == VK_NULL_HANDLE || targetImage == VK_NULL_HANDLE ||
        width == 0 || height == 0) return false;

    VkBufferImageCopy region{
        0, 0, 0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {width, height, 1}
    };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, targetImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    return true;
}

} // namespace NeoEngine
