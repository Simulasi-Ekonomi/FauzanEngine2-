#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

struct UploadStagingAllocation {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
};

struct UploadTask {
    std::vector<UploadStagingAllocation> staging;
    VkFence completionFence = VK_NULL_HANDLE;
};

class VulkanAssetUploader {
public:
    explicit VulkanAssetUploader(uint32_t stagingPoolSizeMB = 512) noexcept
        : stagingPoolSizeMB_(stagingPoolSizeMB) {}

    ~VulkanAssetUploader() noexcept = default;

    VulkanAssetUploader(const VulkanAssetUploader&) = delete;
    VulkanAssetUploader& operator=(const VulkanAssetUploader&) = delete;

    // The uploader does not own the Vulkan device. Initialize once after device creation.
    [[nodiscard]] bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice) noexcept;
    void Shutdown(VkDevice device) noexcept;

    // Record an upload and associate its staging lifetime with the fence that will
    // be signalled by the submission containing cmd. The caller owns queue submission.
    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout,
                                     uint32_t width, uint32_t height,
                                     VkFence completionFence) noexcept;

    [[nodiscard]] bool UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                   const std::vector<uint8_t>& vertexData,
                                   const std::vector<uint8_t>& indexData,
                                   VkBuffer vertexBuffer, VkBuffer indexBuffer,
                                   VkFence completionFence) noexcept;

    // Legacy entry points are retained. They now fail closed because the old API
    // provided neither image extent nor a completion fence, so safe GPU lifetime
    // could not be guaranteed.
    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout) noexcept;

    [[nodiscard]] bool UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                  const std::vector<uint8_t>& vertexData,
                                  const std::vector<uint8_t>& indexData,
                                  VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept;

    // Poll completion fences and release only staging allocations whose GPU work
    // has completed. No vkDeviceWaitIdle is performed here.
    void AdvanceFrame(VkDevice device) noexcept;

    [[nodiscard]] uint32_t GetStagingPoolSizeMB() const noexcept { return stagingPoolSizeMB_; }
    [[nodiscard]] uint32_t GetCurrentStagingUsedMB() const noexcept { return currentStagingUsedMB_; }
    [[nodiscard]] uint32_t GetPendingUploadCount() const noexcept {
        return static_cast<uint32_t>(pendingUploads_.size());
    }

private:
    [[nodiscard]] VkBuffer AllocateStagingBuffer(VkDevice device, VkDeviceSize size,
                                                  VkDeviceMemory& outMemory) noexcept;
    [[nodiscard]] bool WriteStagingBuffer(VkDevice device, VkDeviceMemory memory,
                                          const std::vector<uint8_t>& data) noexcept;
    [[nodiscard]] bool CopyBufferToImage(VkCommandBuffer cmd, VkBuffer stagingBuffer,
                                         VkImage targetImage, VkImageLayout targetLayout,
                                         uint32_t width, uint32_t height) noexcept;
    void DestroyUploadTask(VkDevice device, UploadTask& task) noexcept;
    [[nodiscard]] bool ReserveStaging(VkDeviceSize bytes) noexcept;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice initializedDevice_ = VK_NULL_HANDLE;
    uint32_t stagingPoolSizeMB_;
    uint32_t currentStagingUsedMB_ = 0;
    std::vector<UploadTask> pendingUploads_;
};

} // namespace NeoEngine
