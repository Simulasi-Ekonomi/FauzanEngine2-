#pragma once

#include "PpmTexture.h"

#include <cstdint>

namespace NeoEngine {

struct VulkanTexturedPresentResult {
    bool windowCreated = false;
    bool surfaceCreated = false;
    bool deviceCreated = false;
    bool swapchainCreated = false;
    bool textureUploaded = false;
    bool pipelineCreated = false;
    bool frameSubmitted = false;
    bool framePresented = false;
    uint32_t imageCount = 0;
    uint64_t textureHash = 0;
};

class VulkanTexturedPresentProbe {
public:
    static VulkanTexturedPresentResult Present(const RgbaTexture& texture, uint32_t width = 64, uint32_t height = 64);
};

} // namespace NeoEngine

// NEOENGINE_VULKAN_TEXTURED_PRESENT_H
