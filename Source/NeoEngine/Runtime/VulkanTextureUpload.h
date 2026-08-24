#pragma once

#include "PpmTexture.h"

#include <cstdint>

namespace NeoEngine {

struct VulkanTextureUploadResult {
    bool deviceCreated = false;
    bool textureAllocated = false;
    bool commandSubmitted = false;
    bool pixelsReadback = false;
    uint16_t width = 0;
    uint16_t height = 0;
    uint64_t uploadHash = 0;
    uint64_t readbackHash = 0;
};

class VulkanTextureUploader {
public:
    static VulkanTextureUploadResult UploadAndReadback(const RgbaTexture& texture);
};

} // namespace NeoEngine
