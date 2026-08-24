#pragma once

#include <cstdint>

namespace NeoEngine {

struct VulkanPresentProbeResult {
    bool windowCreated = false;
    bool surfaceCreated = false;
    bool deviceCreated = false;
    bool swapchainCreated = false;
    bool frameSubmitted = false;
    bool framePresented = false;
    uint32_t imageCount = 0;
};

class VulkanPresentProbe {
public:
    static VulkanPresentProbeResult PresentHiddenFrame(uint32_t width = 64, uint32_t height = 64);
};

} // namespace NeoEngine
