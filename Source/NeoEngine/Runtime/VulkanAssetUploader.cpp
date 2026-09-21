#include "VulkanAssetUploader.h"
#include "AssetResourceManager.h"

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
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i)
        if ((typeBits & (1U << i)) && (properties.memoryTypes[i].propertyFlags & required) == required) return i;
    return std::numeric_limits<uint32_t>::max();
}
}

bool VulkanAssetUploader::UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                         const std::vector<uint8_t>& mipData,
                                         VkImage targetImage, VkImageLayout targetLayout) noexcept {
    return UploadTexture(device, cmd, mipData, targetImage, targetLayout, 2048U, 2048U);
}

bool VulkanAssetUploader::UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                         const std::vector<uint8_t>& mipData,
                                         VkImage targetImage, VkImageLayout targetLayout,
                                         uint32_t width, uint32_t height) noexcept {
    if (device == VK_NULL_HANDLE || (lastDevice_ != VK_NULL_HANDLE && device != lastDevice_) || cmd == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        mipData.empty() || targetImage == VK_NULL_HANDLE ||
        targetLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || width == 0U || height == 0U) return false;

    const uint64_t requestedBytes = static_cast<uint64_t>(mipData.size());
    if (requestedBytes > std::numeric_limits<uint64_t>::max() - (1024ULL * 1024ULL - 1ULL)) return false;
    const uint64_t requestedMB64 = (requestedBytes + 1024ULL * 1024ULL - 1ULL) / (1024ULL * 1024ULL);
    if (requestedMB64 == 0ULL || requestedMB64 > UINT32_MAX || requestedMB64 > stagingPoolSizeMB_ ||
        currentStagingUsedMB_ > stagingPoolSizeMB_ - static_cast<uint32_t>(requestedMB64)) return false;
    lastDevice_ = device;

    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = AllocateStagingBuffer(device, mipData.size(), stagingMemory);
    if (stagingBuffer == VK_NULL_HANDLE || stagingMemory == VK_NULL_HANDLE) return false;

    void* mapped = nullptr;
    if (vkMapMemory(device, stagingMemory, 0, mipData.size(), 0, &mapped) != VK_SUCCESS || mapped == nullptr) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    std::memcpy(mapped, mipData.data(), mipData.size());
    vkUnmapMemory(device, stagingMemory);

    if (!CopyBufferToImage(device, cmd, stagingBuffer, targetImage, width, height)) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }

    try {
        pendingUploads_.reserve(pendingUploads_.size() + 1U);
        pendingUploads_.push_back({stagingBuffer, stagingMemory, targetImage, targetLayout,
                                   static_cast<uint32_t>(requestedMB64), VK_NULL_HANDLE});
    } catch (...) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }
    currentStagingUsedMB_ += static_cast<uint32_t>(requestedMB64);
    return true;
}

bool VulkanAssetUploader::UploadTextureResource(AssetResourceManager& resources,
                                                 const AssetResourceHandle& handle,
                                                 VkDevice device, VkCommandBuffer cmd,
                                                 VkImage targetImage, VkImageLayout targetLayout,
                                                 uint32_t width, uint32_t height) noexcept {
    const std::vector<uint8_t>* data = resources.Data(handle);
    if (data == nullptr || data->empty()) return false;
    return UploadTexture(device, cmd, *data, targetImage, targetLayout, width, height);
}

bool VulkanAssetUploader::UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                      const std::vector<uint8_t>& vertexData,
                                      const std::vector<uint8_t>& indexData,
                                      VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE ||
        vertexData.empty() || indexData.empty() || vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE) return false;
    if (vertexData.size() > std::numeric_limits<uint64_t>::max() - indexData.size()) return false;
    const uint64_t requestedBytes = static_cast<uint64_t>(vertexData.size()) + static_cast<uint64_t>(indexData.size());
    if (requestedBytes < vertexData.size() || requestedBytes < indexData.size() || requestedBytes > std::numeric_limits<uint64_t>::max() - (1024ULL * 1024ULL - 1ULL)) return false;
    const uint64_t requestedMB64 = (requestedBytes + 1024ULL * 1024ULL - 1ULL) / (1024ULL * 1024ULL);
    if (requestedMB64 == 0ULL || requestedMB64 > UINT32_MAX || requestedMB64 > stagingPoolSizeMB_ ||
        currentStagingUsedMB_ > stagingPoolSizeMB_ - static_cast<uint32_t>(requestedMB64)) return false;
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

    void* vertexMapped = nullptr; void* indexMapped = nullptr;
    if (vkMapMemory(device, vertexMemory, 0, vertexData.size(), 0, &vertexMapped) != VK_SUCCESS ||
        vkMapMemory(device, indexMemory, 0, indexData.size(), 0, &indexMapped) != VK_SUCCESS ||
        !vertexMapped || !indexMapped) {
        if (vertexMapped) vkUnmapMemory(device, vertexMemory);
        if (indexMapped) vkUnmapMemory(device, indexMemory);
        vkDestroyBuffer(device, vertexStaging, nullptr); vkFreeMemory(device, vertexMemory, nullptr);
        vkDestroyBuffer(device, indexStaging, nullptr); vkFreeMemory(device, indexMemory, nullptr);
        return false;
    }
    std::memcpy(vertexMapped, vertexData.data(), vertexData.size());
    std::memcpy(indexMapped, indexData.data(), indexData.size());
    vkUnmapMemory(device, vertexMemory); vkUnmapMemory(device, indexMemory);

    const uint64_t vertexMB64 = (static_cast<uint64_t>(vertexData.size()) + 1024ULL * 1024ULL - 1ULL) / (1024ULL * 1024ULL);
    const uint64_t indexMB64 = (static_cast<uint64_t>(indexData.size()) + 1024ULL * 1024ULL - 1ULL) / (1024ULL * 1024ULL);
    if (vertexMB64 == 0ULL || indexMB64 == 0ULL ||
        vertexMB64 > UINT32_MAX || indexMB64 > UINT32_MAX ||
        vertexMB64 > stagingPoolSizeMB_ || indexMB64 > stagingPoolSizeMB_ ||
        vertexMB64 > static_cast<uint64_t>(stagingPoolSizeMB_) - indexMB64 ||
        currentStagingUsedMB_ > stagingPoolSizeMB_ - static_cast<uint32_t>(vertexMB64 + indexMB64)) {
        vkDestroyBuffer(device, vertexStaging, nullptr);
        vkFreeMemory(device, vertexMemory, nullptr);
        vkDestroyBuffer(device, indexStaging, nullptr);
        vkFreeMemory(device, indexMemory, nullptr);
        return false;
    }

    const VkBufferCopy vertexRegion{0, 0, vertexData.size()};
    const VkBufferCopy indexRegion{0, 0, indexData.size()};
    vkCmdCopyBuffer(cmd, vertexStaging, vertexBuffer, 1, &vertexRegion);
    vkCmdCopyBuffer(cmd, indexStaging, indexBuffer, 1, &indexRegion);
    try {
        pendingUploads_.reserve(pendingUploads_.size() + 2U);
        pendingUploads_.push_back({vertexStaging, vertexMemory, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
                                   static_cast<uint32_t>(vertexMB64), VK_NULL_HANDLE});
        pendingUploads_.push_back({indexStaging, indexMemory, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
                                   static_cast<uint32_t>(indexMB64), VK_NULL_HANDLE});
    } catch (...) {
        vkDestroyBuffer(device, vertexStaging, nullptr);
        vkFreeMemory(device, vertexMemory, nullptr);
        vkDestroyBuffer(device, indexStaging, nullptr);
        vkFreeMemory(device, indexMemory, nullptr);
        return false;
    }
    currentStagingUsedMB_ += static_cast<uint32_t>(vertexMB64 + indexMB64);
    return true;
}

void VulkanAssetUploader::AttachCompletionFence(VkFence fence) noexcept {
    if (fence == VK_NULL_HANDLE || pendingUploads_.empty()) return;
    for (UploadTask& task : pendingUploads_)
        if (task.completionFence == VK_NULL_HANDLE) task.completionFence = fence;
}

void VulkanAssetUploader::Flush(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE || (lastDevice_ != VK_NULL_HANDLE && device != lastDevice_) || vkDeviceWaitIdle(device) != VK_SUCCESS) return;
    std::vector<VkFence> destroyedFences;
    for (const UploadTask& task : pendingUploads_) {
        if (task.completionFence != VK_NULL_HANDLE &&
            std::find(destroyedFences.begin(), destroyedFences.end(), task.completionFence) == destroyedFences.end()) {
            vkDestroyFence(device, task.completionFence, nullptr);
            destroyedFences.push_back(task.completionFence);
        }
        if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
        if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
    }
    pendingUploads_.clear();
    currentStagingUsedMB_ = 0U;
    lastDevice_ = device;
}

void VulkanAssetUploader::AdvanceFrame(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE || (lastDevice_ != VK_NULL_HANDLE && device != lastDevice_)) return;
    size_t write = 0; uint32_t residentMB = 0;
    std::vector<VkFence> completedFences;
    for (size_t i = 0; i < pendingUploads_.size(); ++i) {
        UploadTask& task = pendingUploads_[i];
        const VkFence fence = task.completionFence;
        const bool complete = fence != VK_NULL_HANDLE && vkGetFenceStatus(device, fence) == VK_SUCCESS;
        if (complete) {
            if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
            if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
            if (std::find(completedFences.begin(), completedFences.end(), fence) == completedFences.end()) completedFences.push_back(fence);
            continue;
        }
        pendingUploads_[write++] = task; residentMB += task.uploadSizeMB;
    }
    for (VkFence fence : completedFences) vkDestroyFence(device, fence, nullptr);
    pendingUploads_.resize(write); currentStagingUsedMB_ = residentMB;
}

VkBuffer VulkanAssetUploader::AllocateStagingBuffer(VkDevice device, size_t size, VkDeviceMemory& outMemory) noexcept {
    outMemory = VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || size == 0) return VK_NULL_HANDLE;
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size; bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return VK_NULL_HANDLE;
    VkMemoryRequirements requirements{}; vkGetBufferMemoryRequirements(device, buffer, &requirements);
    const uint32_t memoryType = FindMemoryType(physicalDevice_, requirements.memoryTypeBits,
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memoryType == std::numeric_limits<uint32_t>::max()) { vkDestroyBuffer(device, buffer, nullptr); return VK_NULL_HANDLE; }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size; allocation.memoryTypeIndex = memoryType;
    if (vkAllocateMemory(device, &allocation, nullptr, &outMemory) != VK_SUCCESS ||
        vkBindBufferMemory(device, buffer, outMemory, 0) != VK_SUCCESS) {
        if (outMemory) vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE; vkDestroyBuffer(device, buffer, nullptr); return VK_NULL_HANDLE;
    }
    return buffer;
}

bool VulkanAssetUploader::CopyBufferToImage(VkDevice device, VkCommandBuffer cmd,
                                            VkBuffer stagingBuffer, VkImage targetImage,
                                            uint32_t width, uint32_t height) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE || stagingBuffer == VK_NULL_HANDLE ||
        targetImage == VK_NULL_HANDLE || width == 0U || height == 0U) return false;
    VkBufferImageCopy region{0, 0, 0, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0}, {width, height, 1}};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    return true;
}

} // namespace NeoEngine
