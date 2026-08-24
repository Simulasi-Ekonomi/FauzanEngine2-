#pragma once

#include "PpmTexture.h"

#include <span>

namespace NeoEngine {

class BmpTextureDecoder {
public:
    static constexpr uint16_t kMaxDimension = 4096;
    static constexpr uint32_t kMaxPixels = 4U * 1024U * 1024U;

    static bool DecodeBiRgb(std::span<const uint8_t> bytes, RgbaTexture& texture, TextureDecodeError& error);
};

} // namespace NeoEngine
