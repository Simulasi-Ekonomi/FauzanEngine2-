#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

class AssetResourceManager;
struct AssetResourceHandle;

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

    void SetPhysicalDevice(VkPhysicalDevice physicalDevice) noexcept { physicalDevice_ = physicalDevice; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const noexcept { return physicalDevice_; }

    ~VulkanAssetUploader() noexcept = default;
    VulkanAssetUploader(const VulkanAssetUploader&) = delete;
    VulkanAssetUploader& operator=(const VulkanAssetUploader&) = delete;

    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout) noexcept;
    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout,
                                     uint32_t width, uint32_t height) noexcept;

    // Integrated resource-manager path: validates the live resource lease and
    // uploads its current registry payload without bypassing ownership validation.
    [[nodiscard]] bool UploadTextureResource(AssetResourceManager& resources,
                                              const AssetResourceHandle& handle,
                                              VkDevice device, VkCommandBuffer cmd,
                                              VkImage targetImage, VkImageLayout targetLayout,
                                              uint32_t width, uint32_t height) noexcept;

    [[nodiscard]] bool UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                  const std::vector<uint8_t>& vertexData,
                                  const std::vector<uint8_t>& indexData,
                                  VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept;

    // The fence is borrowed by default; callers retain Vulkan fence ownership.
    // Set takeOwnership only when this uploader created the fence specifically for
    // these pending uploads. A borrowed fence is never destroyed by this class.
    void AttachCompletionFence(VkFence fence, bool takeOwnership = false) noexcept;
    void AdvanceFrame(VkDevice device) noexcept;
    void Flush(VkDevice device) noexcept;

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
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice lastDevice_ = VK_NULL_HANDLE;
    std::vector<VkFence> ownedCompletionFences_;
};

} // namespace NeoEngine
