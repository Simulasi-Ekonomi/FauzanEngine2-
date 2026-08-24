#include "PpmTexture.h"

#include <cctype>
#include <limits>

namespace NeoEngine {
namespace {

bool SkipSpaceAndComments(std::span<const uint8_t> bytes, size_t& offset) {
    while (offset < bytes.size()) {
        if (std::isspace(static_cast<unsigned char>(bytes[offset]))) {
            ++offset;
            continue;
        }
        if (bytes[offset] == '#') {
            while (offset < bytes.size() && bytes[offset] != '\n' && bytes[offset] != '\r') ++offset;
            continue;
        }
        return true;
    }
    return false;
}

bool ReadToken(std::span<const uint8_t> bytes, size_t& offset, uint32_t& value) {
    if (!SkipSpaceAndComments(bytes, offset) || !std::isdigit(static_cast<unsigned char>(bytes[offset]))) return false;
    uint64_t parsed = 0;
    while (offset < bytes.size() && std::isdigit(static_cast<unsigned char>(bytes[offset]))) {
        parsed = parsed * 10U + static_cast<uint32_t>(bytes[offset] - '0');
        if (parsed > std::numeric_limits<uint32_t>::max()) return false;
        ++offset;
    }
    value = static_cast<uint32_t>(parsed);
    return true;
}

} // namespace

bool PpmTextureDecoder::DecodeP6(std::span<const uint8_t> bytes, RgbaTexture& texture, TextureDecodeError& error) {
    texture = {};
    error = TextureDecodeError::None;
    if (bytes.size() < 4 || bytes[0] != 'P' || bytes[1] != '6') {
        error = TextureDecodeError::UnsupportedFormat;
        return false;
    }
    size_t offset = 2;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t maxValue = 0;
    if (!ReadToken(bytes, offset, width) || !ReadToken(bytes, offset, height) || !ReadToken(bytes, offset, maxValue) || maxValue != 255) {
        error = TextureDecodeError::InvalidHeader;
        return false;
    }
    if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension || width > kMaxPixels / height) {
        error = TextureDecodeError::DimensionLimit;
        return false;
    }
    if (offset >= bytes.size() || !std::isspace(static_cast<unsigned char>(bytes[offset]))) {
        error = TextureDecodeError::InvalidHeader;
        return false;
    }
    if (bytes[offset] == '\r' && offset + 1 < bytes.size() && bytes[offset + 1] == '\n') offset += 2;
    else ++offset;
    const size_t pixelCount = static_cast<size_t>(width) * height;
    const size_t rgbByteCount = pixelCount * 3U;
    if (bytes.size() - offset != rgbByteCount) {
        error = TextureDecodeError::ByteCountMismatch;
        return false;
    }
    texture.width = static_cast<uint16_t>(width);
    texture.height = static_cast<uint16_t>(height);
    texture.rgba.resize(pixelCount * 4U);
    for (size_t pixel = 0; pixel < pixelCount; ++pixel) {
        texture.rgba[pixel * 4U] = bytes[offset + pixel * 3U];
        texture.rgba[pixel * 4U + 1U] = bytes[offset + pixel * 3U + 1U];
        texture.rgba[pixel * 4U + 2U] = bytes[offset + pixel * 3U + 2U];
        texture.rgba[pixel * 4U + 3U] = 255;
    }
    return true;
}

} // namespace NeoEngine
