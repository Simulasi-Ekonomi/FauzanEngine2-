#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace NeoEngine {

enum class TextureDecodeError : uint8_t { None, InvalidHeader, UnsupportedFormat, DimensionLimit, ByteCountMismatch };

struct RgbaTexture {
    uint16_t width = 0;
    uint16_t height = 0;
    std::vector<uint8_t> rgba;
};

class PpmTextureDecoder {
public:
    static constexpr uint16_t kMaxDimension = 4096;
    static constexpr uint32_t kMaxPixels = 4U * 1024U * 1024U;

    static bool DecodeP6(std::span<const uint8_t> bytes, RgbaTexture& texture, TextureDecodeError& error);
};

} // namespace NeoEngine
