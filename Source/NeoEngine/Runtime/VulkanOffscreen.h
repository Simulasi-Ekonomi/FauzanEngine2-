#pragma once

#include <cstdint>

namespace NeoEngine {

struct VulkanOffscreenResult {
    bool deviceCreated = false;
    bool pipelineCreated = false;
    bool commandSubmitted = false;
    bool pixelsReadback = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t nonClearPixelCount = 0;
    uint64_t pixelHash = 0;
};

class VulkanOffscreenRenderer {
public:
    static VulkanOffscreenResult RenderTriangle(uint32_t width = 64, uint32_t height = 64);
};

} // namespace NeoEngine
