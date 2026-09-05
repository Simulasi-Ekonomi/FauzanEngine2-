#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace NeoEngine {

enum class MipmapFilterType : uint8_t {
    BoxFilter,    // Fast, GPU-friendly
    Lanczos,      // Higher quality (CPU only)
};

struct MipmapGenerationRequest {
    VkImage sourceImage = VK_NULL_HANDLE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    uint32_t width = 0;
    uint32_t height = 0;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    uint32_t desiredLevels = 0;  // 0 = auto-compute
    MipmapFilterType filter = MipmapFilterType::BoxFilter;
};

class MipmapManager {
public:
    MipmapManager() = default;
    ~MipmapManager() = default;

    MipmapManager(const MipmapManager&) = delete;
    MipmapManager& operator=(const MipmapManager&) = delete;

    // GPU-accelerated mipmap generation via Vulkan compute/blit
    [[nodiscard]] bool GenerateGPU(VkDevice device, VkCommandBuffer cmd, 
                                   const MipmapGenerationRequest& req) noexcept;
    
    // CPU-based mipmap generation (fallback)
    [[nodiscard]] bool GenerateCPU(const std::vector<uint8_t>& sourcePixels,
                                   uint32_t width, uint32_t height,
                                   VkFormat format,
                                   std::vector<std::vector<uint8_t>>& outMipLevels) noexcept;
    
    // Compute required mipmap levels for given dimensions
    [[nodiscard]] static uint32_t ComputeMipLevels(uint32_t width, uint32_t height) noexcept;
    
    [[nodiscard]] bool LastError() const noexcept { return lastError_; }

private:
    [[nodiscard]] bool BlitMipLevel(VkDevice device, VkCommandBuffer cmd,
                                    VkImage image, uint32_t mipLevel,
                                    uint32_t width, uint32_t height) noexcept;
    
    [[nodiscard]] std::vector<uint8_t> DownsampleBoxFilter(const std::vector<uint8_t>& source,
                                                            uint32_t srcWidth, uint32_t srcHeight,
                                                            uint32_t bytesPerPixel) noexcept;
    
    mutable bool lastError_ = false;
};

} // namespace NeoEngine
