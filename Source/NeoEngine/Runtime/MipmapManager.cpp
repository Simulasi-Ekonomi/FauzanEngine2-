#include "MipmapManager.h"
#include <cstring>
#include <algorithm>

namespace NeoEngine {

bool MipmapManager::GenerateGPU(VkDevice device, VkCommandBuffer cmd,
                                const MipmapGenerationRequest& req) noexcept {
    if (req.sourceImage == VK_NULL_HANDLE || req.width == 0 || req.height == 0) {
        lastError_ = true;
        return false;
    }

    uint32_t mipLevels = req.desiredLevels > 0 ? req.desiredLevels : ComputeMipLevels(req.width, req.height);
    
    // Transition to transfer dst for first blit
    VkImageMemoryBarrier barrier{
        VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        nullptr,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
        req.initialLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        req.sourceImage,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1}
    };

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    // Generate mip chain via blit
    for (uint32_t i = 1; i < mipLevels; ++i) {
        if (!BlitMipLevel(device, cmd, req.sourceImage, i, req.width >> i, req.height >> i)) {
            lastError_ = true;
            return false;
        }
    }

    lastError_ = false;
    return true;
}

bool MipmapManager::BlitMipLevel(VkDevice device, VkCommandBuffer cmd,
                                 VkImage image, uint32_t mipLevel,
                                 uint32_t dstWidth, uint32_t dstHeight) noexcept {
    // Simple blit from previous mip level
    uint32_t srcWidth = std::max(1u, dstWidth << 1);
    uint32_t srcHeight = std::max(1u, dstHeight << 1);

    VkImageBlit blit{
        {VK_IMAGE_ASPECT_COLOR_BIT, mipLevel - 1, 0, 1},
        {{0, 0, 0}, {static_cast<int32_t>(srcWidth), static_cast<int32_t>(srcHeight), 1}},
        {VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 0, 1},
        {{0, 0, 0}, {static_cast<int32_t>(dstWidth), static_cast<int32_t>(dstHeight), 1}}
    };

    vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blit, VK_FILTER_LINEAR);

    return true;
}

bool MipmapManager::GenerateCPU(const std::vector<uint8_t>& sourcePixels,
                                uint32_t width, uint32_t height,
                                VkFormat format,
                                std::vector<std::vector<uint8_t>>& outMipLevels) noexcept {
    if (sourcePixels.empty() || width == 0 || height == 0) {
        lastError_ = true;
        return false;
    }

    uint32_t bytesPerPixel = 4;  // Assume RGBA8 for now
    if (format == VK_FORMAT_R8_UNORM) bytesPerPixel = 1;
    else if (format == VK_FORMAT_R8G8_UNORM) bytesPerPixel = 2;

    uint32_t mipLevels = ComputeMipLevels(width, height);
    outMipLevels.clear();
    outMipLevels.reserve(mipLevels);

    std::vector<uint8_t> currentLevel = sourcePixels;
    outMipLevels.push_back(currentLevel);

    uint32_t currentWidth = width;
    uint32_t currentHeight = height;

    for (uint32_t i = 1; i < mipLevels; ++i) {
        uint32_t nextWidth = std::max(1u, currentWidth >> 1);
        uint32_t nextHeight = std::max(1u, currentHeight >> 1);

        currentLevel = DownsampleBoxFilter(currentLevel, currentWidth, currentHeight, bytesPerPixel);
        outMipLevels.push_back(currentLevel);

        currentWidth = nextWidth;
        currentHeight = nextHeight;
    }

    lastError_ = false;
    return true;
}

std::vector<uint8_t> MipmapManager::DownsampleBoxFilter(const std::vector<uint8_t>& source,
                                                         uint32_t srcWidth, uint32_t srcHeight,
                                                         uint32_t bytesPerPixel) noexcept {
    uint32_t dstWidth = std::max(1u, srcWidth >> 1);
    uint32_t dstHeight = std::max(1u, srcHeight >> 1);
    std::vector<uint8_t> dst(dstWidth * dstHeight * bytesPerPixel, 0);

    for (uint32_t y = 0; y < dstHeight; ++y) {
        for (uint32_t x = 0; x < dstWidth; ++x) {
            for (uint32_t c = 0; c < bytesPerPixel; ++c) {
                uint32_t sum = 0;
                uint32_t count = 0;

                for (uint32_t dy = 0; dy < 2; ++dy) {
                    for (uint32_t dx = 0; dx < 2; ++dx) {
                        uint32_t sx = (x << 1) + dx;
                        uint32_t sy = (y << 1) + dy;
                        if (sx < srcWidth && sy < srcHeight) {
                            uint32_t srcIdx = (sy * srcWidth + sx) * bytesPerPixel + c;
                            sum += source[srcIdx];
                            count++;
                        }
                    }
                }

                if (count > 0) {
                    uint32_t dstIdx = (y * dstWidth + x) * bytesPerPixel + c;
                    dst[dstIdx] = static_cast<uint8_t>(sum / count);
                }
            }
        }
    }

    return dst;
}

uint32_t MipmapManager::ComputeMipLevels(uint32_t width, uint32_t height) noexcept {
    uint32_t maxDim = std::max(width, height);
    uint32_t levels = 1;
    while (maxDim > 1) {
        maxDim >>= 1;
        levels++;
    }
    return levels;
}

} // namespace NeoEngine
