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
        (targetLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
         targetLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ||
        width == 0U || height == 0U) return false;

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

    if (!CopyBufferToImage(device, cmd, stagingBuffer, targetImage, targetLayout, width, height)) {
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingMemory, nullptr);
        return false;
    }

    try {
        pendingUploads_.reserve(pendingUploads_.size() + 1U);
        pendingUploads_.push_back({stagingBuffer, stagingMemory, targetImage, targetLayout,
                                   static_cast<uint32_t>(requestedMB64), VK_NULL_HANDLE, nullptr, {}, false});
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
    return UploadTextureResource(resources, handle, *data, device, cmd, targetImage,
                                 targetLayout, width, height);
}

bool VulkanAssetUploader::UploadTextureResource(AssetResourceManager& resources,
                                                 const AssetResourceHandle& handle,
                                                 const std::vector<uint8_t>& pixels,
                                                 VkDevice device, VkCommandBuffer cmd,
                                                 VkImage targetImage, VkImageLayout targetLayout,
                                                 uint32_t width, uint32_t height) noexcept {
    return UploadTextureResource(resources, handle, pixels, device, cmd, targetImage,
                                  targetLayout, width, height, VK_NULL_HANDLE, 0U);
}

bool VulkanAssetUploader::UploadTextureResource(AssetResourceManager& resources,
                                                 const AssetResourceHandle& handle,
                                                 const std::vector<uint8_t>& pixels,
                                                 VkDevice device, VkCommandBuffer cmd,
                                                 VkImage targetImage, VkImageLayout targetLayout,
                                                 uint32_t width, uint32_t height,
                                                 VkDeviceMemory gpuMemory,
                                                 uint32_t gpuAllocationSizeMB) noexcept {
    if (pixels.empty() || width == 0U || height == 0U) return false;
    AssetResourceReceipt preUploadReceipt{};
    if (!resources.Query(handle, preUploadReceipt)) return false;
    const bool alreadyPinned = preUploadReceipt.gpuUploadsInFlight != 0U;
    if (!alreadyPinned && !resources.BeginGpuUpload(handle)) return false;
    const auto cancelIfOwned = [&]() noexcept {
        if (!alreadyPinned) (void)resources.CancelGpuUpload(handle);
    };
    if ((gpuMemory == VK_NULL_HANDLE) != (gpuAllocationSizeMB == 0U)) {
        cancelIfOwned();
        return false;
    }
    if (pixels.size() > std::numeric_limits<uint64_t>::max() / 4ULL ||
        static_cast<uint64_t>(width) * static_cast<uint64_t>(height) >
            std::numeric_limits<uint64_t>::max() / 4ULL ||
        pixels.size() != static_cast<size_t>(static_cast<uint64_t>(width) * height * 4ULL)) {
        cancelIfOwned();
        return false;
    }
    if (!UploadTexture(device, cmd, pixels, targetImage, targetLayout, width, height)) {
        cancelIfOwned();
        return false;
    }
    if (pendingUploads_.empty()) {
        cancelIfOwned();
        return false;
    }
    UploadTask& task = pendingUploads_.back();
    task.resourceManager = &resources;
    task.resourceHandle = handle;
    task.tracksResourceResidency = true;
    if (gpuMemory != VK_NULL_HANDLE) {
        task.gpuMemory = gpuMemory;
        task.gpuAllocationSizeMB = gpuAllocationSizeMB;
    }
    AssetResourceReceipt receipt{};
    if (!resources.Query(handle, receipt) || receipt.assetId.empty()) {
        cancelIfOwned();
        return false;
    }
    task.assetId = receipt.assetId;
    return true;
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
                                   static_cast<uint32_t>(vertexMB64), VK_NULL_HANDLE, nullptr, {}, false});
        pendingUploads_.push_back({indexStaging, indexMemory, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_UNDEFINED,
                                   static_cast<uint32_t>(indexMB64), VK_NULL_HANDLE, nullptr, {}, false});
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

void VulkanAssetUploader::AttachCompletionFence(VkFence fence, bool takeOwnership) noexcept {
    if (fence == VK_NULL_HANDLE || pendingUploads_.empty()) return;
    if (takeOwnership) {
        try {
            if (std::find(ownedCompletionFences_.begin(), ownedCompletionFences_.end(), fence) ==
                ownedCompletionFences_.end()) {
                ownedCompletionFences_.push_back(fence);
            }
        } catch (...) {
            return;
        }
    }
    for (UploadTask& task : pendingUploads_)
        if (task.completionFence == VK_NULL_HANDLE) task.completionFence = fence;
}

void VulkanAssetUploader::Flush(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE || (lastDevice_ != VK_NULL_HANDLE && device != lastDevice_) || vkDeviceWaitIdle(device) != VK_SUCCESS) return;
    for (const UploadTask& task : pendingUploads_) {
        if (task.tracksResourceResidency && task.resourceManager != nullptr) {
            const VkResult status = task.completionFence != VK_NULL_HANDLE
                ? vkGetFenceStatus(device, task.completionFence)
                : VK_NOT_READY;
            if (status == VK_SUCCESS) {
                (void)task.resourceManager->CompleteGpuUpload(task.resourceHandle);
            } else {
                (void)task.resourceManager->CancelGpuUpload(task.resourceHandle);
            }
        }
        if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
        if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
    }
    pendingUploads_.clear();
    for (VkFence fence : ownedCompletionFences_) {
        if (fence != VK_NULL_HANDLE) vkDestroyFence(device, fence, nullptr);
    }
    ownedCompletionFences_.clear();
    currentStagingUsedMB_ = 0U;
    lastDevice_ = device;
}

void VulkanAssetUploader::AdvanceFrame(VkDevice device) noexcept {
    if (device == VK_NULL_HANDLE || (lastDevice_ != VK_NULL_HANDLE && device != lastDevice_)) return;
    size_t write = 0; uint32_t residentMB = 0;
    for (size_t i = 0; i < pendingUploads_.size(); ++i) {
        UploadTask& task = pendingUploads_[i];
        const VkFence fence = task.completionFence;
        const VkResult status = fence != VK_NULL_HANDLE ? vkGetFenceStatus(device, fence) : VK_NOT_READY;
        if (status == VK_SUCCESS) {
            if (completionCallback_) {
                try { completionCallback_(task, VK_SUCCESS); } catch (...) {
                    if (task.tracksResourceResidency && task.resourceManager != nullptr)
                        (void)task.resourceManager->CancelGpuUpload(task.resourceHandle);
                }
            } else if (task.tracksResourceResidency && task.resourceManager != nullptr) {
                (void)task.resourceManager->CompleteGpuUpload(task.resourceHandle);
            }
            if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
            if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
            continue;
        }
        if (status != VK_NOT_READY) {
            if (completionCallback_) {
                try { completionCallback_(task, status); } catch (...) {
                    if (task.tracksResourceResidency && task.resourceManager != nullptr)
                        (void)task.resourceManager->CancelGpuUpload(task.resourceHandle);
                }
            } else if (task.tracksResourceResidency && task.resourceManager != nullptr) {
                (void)task.resourceManager->CancelGpuUpload(task.resourceHandle);
            }
            if (task.stagingBuffer) vkDestroyBuffer(device, task.stagingBuffer, nullptr);
            if (task.stagingMemory) vkFreeMemory(device, task.stagingMemory, nullptr);
            continue;
        }
        pendingUploads_[write++] = task; residentMB += task.uploadSizeMB;
    }
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
                                            VkImageLayout targetLayout,
                                            uint32_t width, uint32_t height) noexcept {
    if (device == VK_NULL_HANDLE || cmd == VK_NULL_HANDLE || stagingBuffer == VK_NULL_HANDLE ||
        targetImage == VK_NULL_HANDLE || width == 0U || height == 0U) return false;
    if (targetLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        targetLayout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) return false;

    VkImageMemoryBarrier toTransfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    toTransfer.srcAccessMask = 0U;
    toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toTransfer.image = targetImage;
    toTransfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toTransfer.subresourceRange.baseMipLevel = 0U;
    toTransfer.subresourceRange.levelCount = 1U;
    toTransfer.subresourceRange.baseArrayLayer = 0U;
    toTransfer.subresourceRange.layerCount = 1U;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0U, 0U, nullptr,
                         0U, nullptr, 1U, &toTransfer);

    const VkBufferImageCopy region{0, 0, 0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {0, 0, 0}, {width, height, 1}};
    vkCmdCopyBufferToImage(cmd, stagingBuffer, targetImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    if (targetLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = targetImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0U;
        barrier.subresourceRange.levelCount = 1U;
        barrier.subresourceRange.baseArrayLayer = 0U;
        barrier.subresourceRange.layerCount = 1U;
        vkCmdPipelineBarrier(cmd,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0U, 0U, nullptr, 0U, nullptr, 1U, &barrier);
    }
    return true;
}

} // namespace NeoEngine

void NeoEngine::VulkanAssetUploader::SetUploadCompletionCallback(UploadCompletionCallback callback) noexcept {
    completionCallback_ = std::move(callback);
}
