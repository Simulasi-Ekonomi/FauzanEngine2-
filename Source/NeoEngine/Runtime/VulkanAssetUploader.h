#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

struct UploadTask {
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkImage targetImage = VK_NULL_HANDLE;
    VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    uint32_t uploadSizeMB = 0;
    VkFence completionFence = VK_NULL_HANDLE;
};

class VulkanAssetUploader {
public:
    explicit VulkanAssetUploader(uint32_t stagingPoolSizeMB = 512) noexcept 
        : stagingPoolSizeMB_(stagingPoolSizeMB) {}
    
    ~VulkanAssetUploader() noexcept = default;
    
    VulkanAssetUploader(const VulkanAssetUploader&) = delete;
    VulkanAssetUploader& operator=(const VulkanAssetUploader&) = delete;

    // Upload texture via staging buffer (returns staging buffer for later cleanup)
    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout) noexcept;
    
    // Upload mesh geometry (vertex + index buffers)
    [[nodiscard]] bool UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                  const std::vector<uint8_t>& vertexData,
                                  const std::vector<uint8_t>& indexData,
                                  VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept;
    
    // Advance frame (cleanup completed uploads, fence waits)
    void AdvanceFrame(VkDevice device) noexcept;
    
    [[nodiscard]] uint32_t GetStagingPoolSizeMB() const noexcept { return stagingPoolSizeMB_; }
    [[nodiscard]] uint32_t GetCurrentStagingUsedMB() const noexcept { return currentStagingUsedMB_; }

private:
    [[nodiscard]] VkBuffer AllocateStagingBuffer(VkDevice device, size_t size, 
                                                 VkDeviceMemory& outMemory) noexcept;
    
    [[nodiscard]] bool CopyBufferToImage(VkDevice device, VkCommandBuffer cmd,
                                        VkBuffer stagingBuffer, VkImage targetImage,
                                        uint32_t width, uint32_t height) noexcept;

    uint32_t stagingPoolSizeMB_;
    uint32_t currentStagingUsedMB_ = 0;
    std::vector<UploadTask> pendingUploads_;
};

} // namespace NeoEngine
