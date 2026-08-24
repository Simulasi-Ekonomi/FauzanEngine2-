#pragma once

#include "PpmTexture.h"

#include <cstdint>

namespace NeoEngine {

struct VulkanTexturedOffscreenResult {
    bool deviceCreated = false;
    bool textureUploaded = false;
    bool pipelineCreated = false;
    bool commandSubmitted = false;
    bool pixelsReadback = false;
    uint32_t nonBlackPixelCount = 0;
    uint64_t pixelHash = 0;
};

class VulkanTexturedOffscreenRenderer {
public:
    static VulkanTexturedOffscreenResult Render(const RgbaTexture& texture, uint32_t width = 64, uint32_t height = 64);
};

} // namespace NeoEngine
