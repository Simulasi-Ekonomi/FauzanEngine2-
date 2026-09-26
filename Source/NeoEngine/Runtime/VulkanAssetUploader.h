#pragma once
#include "AssetResourceManager.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {


struct UploadTask {
    std::string assetId{};
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    VkImage targetImage = VK_NULL_HANDLE;
    VkImageLayout targetLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    uint32_t uploadSizeMB = 0;
    VkFence completionFence = VK_NULL_HANDLE;
    AssetResourceManager* resourceManager = nullptr;
    AssetResourceHandle resourceHandle{};
    bool tracksResourceResidency = false;
    // True when an already resident resource is being refreshed in-place; cancellation
    // must preserve the old residency rather than clearing it.
    bool preservesResidentOnCompletion = false;
    VkDeviceMemory gpuMemory = VK_NULL_HANDLE;
    uint32_t gpuAllocationSizeMB = 0U;
};

class VulkanAssetUploader {
public:
    using UploadCompletionCallback = std::function<void(const UploadTask&, VkResult)>;
    explicit VulkanAssetUploader(uint32_t stagingPoolSizeMB = 512) noexcept
        : stagingPoolSizeMB_(stagingPoolSizeMB) {}

    void SetPhysicalDevice(VkPhysicalDevice physicalDevice) noexcept { physicalDevice_ = physicalDevice; }
    [[nodiscard]] VkPhysicalDevice GetPhysicalDevice() const noexcept { return physicalDevice_; }

    // The Vulkan device must outlive this uploader while uploads are pending.
    ~VulkanAssetUploader() noexcept { if (lastDevice_ != VK_NULL_HANDLE) Flush(lastDevice_); }
    VulkanAssetUploader(const VulkanAssetUploader&) = delete;
    VulkanAssetUploader& operator=(const VulkanAssetUploader&) = delete;

    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout) noexcept;
    [[nodiscard]] bool UploadTexture(VkDevice device, VkCommandBuffer cmd,
                                     const std::vector<uint8_t>& mipData,
                                     VkImage targetImage, VkImageLayout targetLayout,
                                     uint32_t width, uint32_t height) noexcept;

    // Integrated resource-manager path: pins the live resource against eviction/reload,
    // records the exact lease handle on the GPU task, and marks GPU residency only after
    // the authoritative completion fence is observed. resources must outlive pending uploads.
    [[nodiscard]] bool UploadTextureResource(AssetResourceManager& resources,
                                              const AssetResourceHandle& handle,
                                              VkDevice device, VkCommandBuffer cmd,
                                              VkImage targetImage, VkImageLayout targetLayout,
                                              uint32_t width, uint32_t height) noexcept;
    [[nodiscard]] bool UploadTextureResource(AssetResourceManager& resources,
                                              const AssetResourceHandle& handle,
                                              const std::vector<uint8_t>& pixels,
                                              VkDevice device, VkCommandBuffer cmd,
                                              VkImage targetImage, VkImageLayout targetLayout,
                                              uint32_t width, uint32_t height) noexcept;
    [[nodiscard]] bool UploadTextureResource(AssetResourceManager& resources,
                                              const AssetResourceHandle& handle,
                                              const std::vector<uint8_t>& pixels,
                                              VkDevice device, VkCommandBuffer cmd,
                                              VkImage targetImage, VkImageLayout targetLayout,
                                              uint32_t width, uint32_t height,
                                              VkDeviceMemory gpuMemory,
                                              uint32_t gpuAllocationSizeMB) noexcept;

    [[nodiscard]] bool UploadMesh(VkDevice device, VkCommandBuffer cmd,
                                  const std::vector<uint8_t>& vertexData,
                                  const std::vector<uint8_t>& indexData,
                                  VkBuffer vertexBuffer, VkBuffer indexBuffer) noexcept;

    // The fence is borrowed by default; callers retain Vulkan fence ownership.
    // Set takeOwnership only when this uploader created the fence specifically for
    // these pending uploads. A borrowed fence is never destroyed by this class.
    void AttachCompletionFence(VkFence fence, bool takeOwnership = false) noexcept;
    // Optional completion owner. When installed, the callback owns final resource/queue
    // publication for tracked uploads; this prevents the uploader from publishing
    // residency before the runtime stream bridge has an authoritative GPU result.
    void SetUploadCompletionCallback(UploadCompletionCallback callback) noexcept;
    void AdvanceFrame(VkDevice device) noexcept;
    void Flush(VkDevice device) noexcept;

    [[nodiscard]] uint32_t GetStagingPoolSizeMB() const noexcept { return stagingPoolSizeMB_; }
    [[nodiscard]] uint32_t GetCurrentStagingUsedMB() const noexcept { return currentStagingUsedMB_; }

private:
    [[nodiscard]] VkBuffer AllocateStagingBuffer(VkDevice device, size_t size,
                                                 VkDeviceMemory& outMemory) noexcept;
    [[nodiscard]] bool CopyBufferToImage(VkDevice device, VkCommandBuffer cmd,
                                          VkBuffer stagingBuffer, VkImage targetImage, VkImageLayout targetLayout,
                                          uint32_t width, uint32_t height) noexcept;

    uint32_t stagingPoolSizeMB_;
    uint32_t currentStagingUsedMB_ = 0;
    std::vector<UploadTask> pendingUploads_;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice lastDevice_ = VK_NULL_HANDLE;
    std::vector<VkFence> ownedCompletionFences_;
    UploadCompletionCallback completionCallback_{};
};

} // namespace NeoEngine
