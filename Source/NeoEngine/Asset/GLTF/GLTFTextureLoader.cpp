#include "GLTFTextureLoader.h"

#include <cctype>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

namespace NeoEngine {
namespace {
bool ReadToken(std::istream& stream, std::string& token)
{
    token.clear();
    char c = 0;
    while (stream.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!token.empty()) return true;
            continue;
        }
        if (c == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (!token.empty()) return true;
            continue;
        }
        token.push_back(c);
    }
    return !token.empty();
}
} // namespace

TextureData GLTFTextureLoader::Load(const std::string& path)
{
    TextureData texture{0U, 0U, {}};
    if (path.empty()) {
        return texture;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return texture;
    }

    std::string magic;
    if (!ReadToken(file, magic) || magic != "P6") {
        return texture;
    }

    std::string widthToken;
    std::string heightToken;
    std::string maxToken;
    if (!ReadToken(file, widthToken) || !ReadToken(file, heightToken) || !ReadToken(file, maxToken)) {
        return texture;
    }

    try {
        const unsigned long width = std::stoul(widthToken);
        const unsigned long height = std::stoul(heightToken);
        const unsigned long maxValue = std::stoul(maxToken);
        if (width == 0UL || height == 0UL || width > std::numeric_limits<unsigned int>::max() ||
            height > std::numeric_limits<unsigned int>::max() || maxValue != 255UL) {
            return texture;
        }

        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        if (pixelCount > std::numeric_limits<size_t>::max() / 4U) {
            return texture;
        }

        const size_t rgbSize = pixelCount * 3U;
        std::vector<unsigned char> rgb(rgbSize);
        if (!file.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgbSize))) {
            return texture;
        }

        texture.width = static_cast<unsigned int>(width);
        texture.height = static_cast<unsigned int>(height);
        texture.pixels.resize(pixelCount * 4U);
        for (size_t i = 0U; i < pixelCount; ++i) {
            texture.pixels[i * 4U + 0U] = rgb[i * 3U + 0U];
            texture.pixels[i * 4U + 1U] = rgb[i * 3U + 1U];
            texture.pixels[i * 4U + 2U] = rgb[i * 3U + 2U];
            texture.pixels[i * 4U + 3U] = 255U;
        }
    } catch (...) {
        texture = TextureData{0U, 0U, {}};
    }
    return texture;
}

} // namespace NeoEngine
