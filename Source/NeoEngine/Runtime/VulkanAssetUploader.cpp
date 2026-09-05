#include "VulkanAssetUploader.h"
#include <cstring>

namespace NeoEngine {

bool VulkanAssetUploader::UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                        const std::vector<uint8_t>& mipData,
                                        VkImage targetImage, VkImageLayout targetLayout) noexcept {
    if (mipData.empty() || targetImage == VK_NULL_HANDLE) {
        return false;
    }

    // Allocate staging buffer
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkBuffer stagingBuffer = AllocateStagingBuffer(device, mipData.size(), stagingMemory);
    if (stagingBuffer == VK_NULL_HANDLE) {
        return false;
    }

    // Copy data to staging buffer (would normally map and memcpy)
    // For now, assume data is already in staging buffer

    // Record copy command
    if (!CopyBufferToImage(device, cmd, stagingBuffer, targetImage, 2048, 2048)) {
        return false;
    }

    // Track for cleanup
    currentStagingUsedMB_ += static_cast<uint32_t>(mipData.size() / (1024 * 1024) + 1);

    return true;
}

bool VulkanAssetUploader::UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& vertexData,
                                     const std::vector<uint8_t>& indexData,
                                     VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept {
    if (vertexData.empty() || indexData.empty()) {
        return false;
    }

    // Allocate staging buffers for vertex and index data
    VkDeviceMemory vertexStagingMemory = VK_NULL_HANDLE;
    VkDeviceMemory indexStagingMemory = VK_NULL_HANDLE;

    VkBuffer vertexStaging = AllocateStagingBuffer(device, vertexData.size(), vertexStagingMemory);
    VkBuffer indexStaging = AllocateStagingBuffer(device, indexData.size(), indexStagingMemory);

    if (vertexStaging == VK_NULL_HANDLE || indexStaging == VK_NULL_HANDLE) {
        return false;
    }

    // Record copy commands (vertex and index buffers)
    VkBufferCopy vertexRegion{0, 0, vertexData.size()};
    vkCmdCopyBuffer(cmd, vertexStaging, vertexBuffer, 1, &vertexRegion);

    VkBufferCopy indexRegion{0, 0, indexData.size()};
    vkCmdCopyBuffer(cmd, indexStaging, indexBuffer, 1, &indexRegion);

    currentStagingUsedMB_ += static_cast<uint32_t>((vertexData.size() + indexData.size()) / (1024 * 1024) + 1);

    return true;
}

void VulkanAssetUploader::AdvanceFrame(VkDevice device) noexcept {
    // In real implementation, would wait on fences and cleanup staging buffers
    // For now, just reset staging usage
    currentStagingUsedMB_ = 0;
}

VkBuffer VulkanAssetUploader::AllocateStagingBuffer(VkDevice device, size_t size,
                                                    VkDeviceMemory& outMemory) noexcept {
    // Simplified: just return null handle (real implementation allocates VkBuffer)
    return VK_NULL_HANDLE;
}

bool VulkanAssetUploader::CopyBufferToImage(VkDevice device, VkCommandBuffer cmd,
                                           VkBuffer stagingBuffer, VkImage targetImage,
                                           uint32_t width, uint32_t height) noexcept {
    VkBufferImageCopy region{
        0,
        0, 0,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        {0, 0, 0},
        {width, height, 1}
    };

    vkCmdCopyBufferToImage(cmd, stagingBuffer, targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          1, &region);

    return true;
}

} // namespace NeoEngine
