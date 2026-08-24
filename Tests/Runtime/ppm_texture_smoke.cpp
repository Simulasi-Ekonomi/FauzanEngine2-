#include "Runtime/PpmTexture.h"

#include <cstdio>
#include <vector>

int main() {
    using namespace NeoEngine;
    const std::vector<uint8_t> valid{
        'P', '6', '\n', '#', 't', 'e', 's', 't', '\n', '2', ' ', '1', '\n', '2', '5', '5', '\n',
        0x20, 0x01, 0x02, 0x03, 0x04, 0x05,
    };
    RgbaTexture texture;
    TextureDecodeError error = TextureDecodeError::None;
    if (!PpmTextureDecoder::DecodeP6(valid, texture, error) || texture.width != 2 || texture.height != 1 || texture.rgba.size() != 8 ||
        texture.rgba[0] != 0x20 || texture.rgba[3] != 255 || texture.rgba[4] != 0x03 || texture.rgba[7] != 255) {
        return 1;
    }
    const uint16_t decodedWidth = texture.width;
    const uint16_t decodedHeight = texture.height;
    const size_t decodedRgbaBytes = texture.rgba.size();
    const uint8_t decodedFirst = texture.rgba[0];
    const std::vector<uint8_t> truncated{'P', '6', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n', 1, 2};
    if (PpmTextureDecoder::DecodeP6(truncated, texture, error) || error != TextureDecodeError::ByteCountMismatch) return 1;
    const std::vector<uint8_t> oversized{'P', '6', '\n', '4', '0', '9', '7', ' ', '1', '\n', '2', '5', '5', '\n'};
    if (PpmTextureDecoder::DecodeP6(oversized, texture, error) || error != TextureDecodeError::DimensionLimit) return 1;
    const std::vector<uint8_t> unsupported{'P', '3', '\n', '1', ' ', '1', '\n', '2', '5', '5', '\n'};
    if (PpmTextureDecoder::DecodeP6(unsupported, texture, error) || error != TextureDecodeError::UnsupportedFormat) return 1;
    std::printf("PPM_TEXTURE_SMOKE_OK size=%ux%u rgba=%zu first=%u\n", decodedWidth, decodedHeight, decodedRgbaBytes, decodedFirst);
    return 0;
}
