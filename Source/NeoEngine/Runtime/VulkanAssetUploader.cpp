#include "VulkanAssetUploader.h"

#include <cstring>
#include <limits>

namespace NeoEngine {

namespace {
uint32_t FindHostVisibleMemoryType(VkPhysicalDevice physicalDevice,
                                    uint32_t typeFilter,
                                    VkMemoryPropertyFlags required,
                                    VkMemoryPropertyFlags preferred) noexcept {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);

    uint32_t fallback = UINT32_MAX;
    for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((typeFilter & (1u << i)) == 0u) continue;
        const VkMemoryPropertyFlags flags = properties.memoryTypes[i].propertyFlags;
        if ((flags & required) != required) continue;
        if ((flags & preferred) == preferred) return i;
        if (fallback == UINT32_MAX) fallback = i;
    }
    return fallback;
}

uint32_t BytesToMB(VkDeviceSize bytes) noexcept {
    constexpr VkDeviceSize kMB = 1024u * 1024u;
    const VkDeviceSize rounded = (bytes + kMB - 1u) / kMB;
    return rounded > std::numeric_limits<uint32_t>::max()
        ? std::numeric_limits<uint32_t>::max()
        : static_cast<uint32_t>(rounded);
}
} // namespace

bool VulkanAssetUploader::Initialize(VkDevice device, VkPhysicalDevice physicalDevice) noexcept {
    if (device == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE) return false;
    if (!pendingUploads_.empty()) return false;
    physicalDevice_ = physicalDevice;
    initializedDevice_ = device;
    currentStagingUsedMB_ = 0;
    return true;
}

void VulkanAssetUploader::Shutdown(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE) return;
    for (auto& task : pendingUploads_) DestroyUploadTask(device, task);
    pendingUploads_.clear();
    currentStagingUsedMB_ = 0;
    if (device == initializedDevice_) {
        initializedDevice_ = VK_NULL_HANDLE;
        physicalDevice_ = VK_NULL_HANDLE;
    }
}

bool VulkanAssetUploader::ReserveStaging(VkDeviceSize bytes) noexcept {
    if (bytes == 0 || stagingPoolSizeMB_ == 0) return false;
    const uint64_t used = currentStagingUsedMB_;
    const uint64_t requested = BytesToMB(bytes);
    if (requested > stagingPoolSizeMB_ || used + requested > stagingPoolSizeMB_) return false;
    currentStagingUsedMB_ += static_cast<uint32_t>(requested);
    return true;
}

VkBuffer VulkanAssetUploader::AllocateStagingBuffer(VkDevice device, VkDeviceSize size,
                                                    VkDeviceMemory& outMemory) noexcept {
    outMemory = VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE || physicalDevice_ == VK_NULL_HANDLE || size == 0) return VK_NULL_HANDLE;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer buffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) return VK_NULL_HANDLE;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer, &requirements);

    const VkMemoryPropertyFlags required = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const VkMemoryPropertyFlags preferred = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    const uint32_t memoryType = FindHostVisibleMemoryType(
        physicalDevice_, requirements.memoryTypeBits, required, preferred);
    if (memoryType == UINT32_MAX) {
        vkDestroyBuffer(device, buffer, nullptr);
        return VK_NULL_HANDLE;
    }

    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memoryType;

    if (vkAllocateMemory(device, &allocation, nullptr, &outMemory) != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        return VK_NULL_HANDLE;
    }

    if (vkBindBufferMemory(device, buffer, outMemory, 0) != VK_SUCCESS) {
        vkFreeMemory(device, outMemory, nullptr);
        outMemory = VK_NULL_HANDLE;
        vkDestroyBuffer(device, buffer, nullptr);
        return VK_NULL_HANDLE;
    }
    return buffer;
}

bool VulkanAssetUploader::WriteStagingBuffer(VkDevice device, VkDeviceMemory memory,
                                             const std::vector<uint8_t>& data) noexcept {
    if (device == VK_NULL_HANDLE || memory == VK_NULL_HANDLE || data.empty()) return false;

    void* mapped = nullptr;
    if (vkMapMemory(device, memory, 0, data.size(), 0, &mapped) != VK_SUCCESS) return false;
    std::memcpy(mapped, data.data(), data.size());

    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &properties);

    // The memory type is not stored in UploadStagingAllocation, so flushing is
    // conservatively performed for every staging write. Vulkan permits flushing
    // coherent memory as well; this keeps the uploader correct on non-coherent heaps.
    VkMappedMemoryRange range{};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    const VkResult flushResult = vkFlushMappedMemoryRanges(device, 1, &range);
    vkUnmapMemory(device, memory);
    return flushResult == VK_SUCCESS;
}

bool VulkanAssetUploader::CopyBufferToImage(VkCommandBuffer cmd, VkBuffer stagingBuffer,
                                            VkImage targetImage, VkImageLayout targetLayout,
                                            uint32_t width, uint32_t height) noexcept {
    if (cmd == VK_NULL_HANDLE || stagingBuffer == VK_NULL_HANDLE ||
        targetImage == VK_NULL_HANDLE || width == 0 || height == 0) return false;

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, targetImage, targetLayout, 1, &region);
    return true;
}

bool VulkanAssetUploader::UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                        const std::vector<uint8_t>& mipData,
                                        VkImage targetImage, VkImageLayout targetLayout,
                                        uint32_t width, uint32_t height,
                                        VkFence completionFence) noexcept {
    if (device == VK_NULL_HANDLE || device != initializedDevice_ ||
        physicalDevice_ == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE ||
        mipData.empty() || targetImage == VK_NULL_HANDLE ||
        width == 0 || height == 0 || completionFence == VK_NULL_HANDLE) return false;

    if (!ReserveStaging(mipData.size())) return false;

    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkBuffer staging = AllocateStagingBuffer(device, mipData.size(), memory);
    if (staging == VK_NULL_HANDLE || memory == VK_NULL_HANDLE) {
        currentStagingUsedMB_ -= BytesToMB(mipData.size());
        return false;
    }

    UploadTask task{};
    task.completionFence = completionFence;
    task.staging.push_back({staging, memory, mipData.size()});

    if (!WriteStagingBuffer(device, memory, mipData) ||
        !CopyBufferToImage(cmd, staging, targetImage, targetLayout, width, height)) {
        DestroyUploadTask(device, task);
        currentStagingUsedMB_ -= BytesToMB(mipData.size());
        return false;
    }

    try {
        pendingUploads_.push_back(std::move(task));
    } catch (...) {
        DestroyUploadTask(device, task);
        currentStagingUsedMB_ -= BytesToMB(mipData.size());
        return false;
    }
    return true;
}

bool VulkanAssetUploader::UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& vertexData,
                                     const std::vector<uint8_t>& indexData,
                                     VkBuffer vertexBuffer, VkBuffer indexBuffer,
                                     VkFence completionFence) noexcept {
    if (device == VK_NULL_HANDLE || device != initializedDevice_ ||
        physicalDevice_ == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE ||
        vertexData.empty() || indexData.empty() ||
        vertexBuffer == VK_NULL_HANDLE || indexBuffer == VK_NULL_HANDLE ||
        completionFence == VK_NULL_HANDLE) return false;

    const VkDeviceSize totalSize =
        static_cast<VkDeviceSize>(vertexData.size()) +
        static_cast<VkDeviceSize>(indexData.size());
    if (!ReserveStaging(totalSize)) return false;

    VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory = VK_NULL_HANDLE;
    VkBuffer vertexStaging = AllocateStagingBuffer(device, vertexData.size(), vertexMemory);
    VkBuffer indexStaging = AllocateStagingBuffer(device, indexData.size(), indexMemory);

    UploadTask task{};
    task.completionFence = completionFence;
    if (vertexStaging != VK_NULL_HANDLE && vertexMemory != VK_NULL_HANDLE)
        task.staging.push_back({vertexStaging, vertexMemory, vertexData.size()});
    if (indexStaging != VK_NULL_HANDLE && indexMemory != VK_NULL_HANDLE)
        task.staging.push_back({indexStaging, indexMemory, indexData.size()});

    if (task.staging.size() != 2 ||
        !WriteStagingBuffer(device, vertexMemory, vertexData) ||
        !WriteStagingBuffer(device, indexMemory, indexData)) {
        DestroyUploadTask(device, task);
        currentStagingUsedMB_ -= BytesToMB(totalSize);
        return false;
    }

    VkBufferCopy vertexRegion{};
    vertexRegion.size = vertexData.size();
    vkCmdCopyBuffer(cmd, vertexStaging, vertexBuffer, 1, &vertexRegion);

    VkBufferCopy indexRegion{};
    indexRegion.size = indexData.size();
    vkCmdCopyBuffer(cmd, indexStaging, indexBuffer, 1, &indexRegion);

    try {
        pendingUploads_.push_back(std::move(task));
    } catch (...) {
        DestroyUploadTask(device, task);
        currentStagingUsedMB_ -= BytesToMB(totalSize);
        return false;
    }
    return true;
}

bool VulkanAssetUploader::UploadTexture(VkDevice, VkCommandBuffer,
                                        const std::vector<uint8_t>&,
                                        VkImage, VkImageLayout) noexcept {
    return false;
}

bool VulkanAssetUploader::UploadMesh(VkDevice, VkCommandBuffer,
                                     const std::vector<uint8_t>&,
                                     const std::vector<uint8_t>&,
                                     VkBuffer, VkBuffer) noexcept {
    return false;
}

void VulkanAssetUploader::DestroyUploadTask(VkDevice device, UploadTask& task) noexcept {
    for (auto& allocation : task.staging) {
        if (allocation.buffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device, allocation.buffer, nullptr);
        if (allocation.memory != VK_NULL_HANDLE)
            vkFreeMemory(device, allocation.memory, nullptr);
        allocation.buffer = VK_NULL_HANDLE;
        allocation.memory = VK_NULL_HANDLE;
        allocation.size = 0;
    }
    task.staging.clear();
    task.completionFence = VK_NULL_HANDLE;
}

void VulkanAssetUploader::AdvanceFrame(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE || device != initializedDevice_) return;

    for (auto it = pendingUploads_.begin(); it != pendingUploads_.end();) {
        if (it->completionFence == VK_NULL_HANDLE ||
            vkGetFenceStatus(device, it->completionFence) != VK_SUCCESS) {
            ++it;
            continue;
        }

        uint64_t releasedBytes = 0;
        for (const auto& allocation : it->staging) releasedBytes += allocation.size;
        DestroyUploadTask(device, *it);
        const uint32_t releasedMB = BytesToMB(releasedBytes);
        currentStagingUsedMB_ = releasedMB > currentStagingUsedMB_
            ? 0
            : currentStagingUsedMB_ - releasedMB;
        it = pendingUploads_.erase(it);
    }
}

} // namespace NeoEngine
