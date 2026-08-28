#pragma once

#include "PpmTexture.h"

#include <cstdint>
#include <span>

namespace NeoEngine {

struct CpuTextureResource;
class SoftwareRenderer;

enum class VulkanPresentStatus : uint8_t { None, InvalidInput, Unavailable, DeviceLost, SurfaceOutOfDate, Timeout, DriverRejected, Presented };

struct VulkanTexturedPresentResult {
    VulkanPresentStatus status = VulkanPresentStatus::None;
    bool sdlInitialized = false;
    bool windowCreated = false;
    bool surfaceCreated = false;
    bool deviceCreated = false;
    bool swapchainCreated = false;
    bool textureUploaded = false;
    bool pipelineCreated = false;
    bool frameSubmitted = false;
    bool framePresented = false;
    bool acquireAttempted = false;
    bool submitAttempted = false;
    bool fenceWaitAttempted = false;
    bool presentAttempted = false;
    int32_t acquireDriverResult = 0;
    int32_t submitDriverResult = 0;
    int32_t fenceWaitDriverResult = 0;
    int32_t presentDriverResult = 0;
    uint32_t imageCount = 0;
    uint64_t textureHash = 0;
    uint64_t stagedSourceHash = 0;
};

class VulkanTexturedPresentProbe {
public:
    static VulkanPresentStatus ClassifyDriverResult(int32_t result);
    static VulkanTexturedPresentResult Present(const RgbaTexture& texture, uint32_t width = 64, uint32_t height = 64);
    static VulkanTexturedPresentResult Present(const CpuTextureResource& texture, uint32_t width = 64, uint32_t height = 64);
    static VulkanTexturedPresentResult Present(std::span<const uint32_t> pixels, uint32_t width, uint32_t height);
};

} // namespace NeoEngine

// NEOENGINE_VULKAN_TEXTURED_PRESENT_H
