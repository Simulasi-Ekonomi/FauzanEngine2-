#include <cassert>
#pragma once
#include <string>
#include <vector>

namespace NeoEngine {

struct TextureData {
    unsigned int width;
    unsigned int height;
    std::vector<unsigned char> pixels;
};

class GLTFTextureLoader {
public:
    static TextureData Load(const std::string& path);
};

} // namespace NeoEngine
