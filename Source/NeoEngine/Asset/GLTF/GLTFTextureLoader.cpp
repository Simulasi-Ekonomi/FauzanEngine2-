#include "GLTFTextureLoader.h"

#include "Runtime/BmpTexture.h"
#include "Runtime/PpmTexture.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace NeoEngine {

namespace {

std::string LowerExtension(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) return {};
    std::string extension = path.substr(dot + 1U);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

bool ReadFile(const std::string& path, std::vector<uint8_t>& bytes) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !bytes.empty();
}

} // namespace

TextureData GLTFTextureLoader::Load(const std::string& path) {
    TextureData result{0U, 0U, {}};
    if (path.empty()) return result;

    std::vector<uint8_t> bytes;
    if (!ReadFile(path, bytes)) return result;

    RgbaTexture decoded;
    TextureDecodeError error = TextureDecodeError::None;
    const std::string extension = LowerExtension(path);

    bool ok = false;
    if (extension == "ppm" || extension == "pnm") {
        ok = PpmTextureDecoder::DecodeP6(bytes, decoded, error);
    } else if (extension == "bmp") {
        ok = BmpTextureDecoder::DecodeBiRgb(bytes, decoded, error);
    }

    if (!ok) return result;

    result.width = decoded.width;
    result.height = decoded.height;
    result.pixels = std::move(decoded.rgba);
    return result;
}

} // namespace NeoEngine
