#include "GLTFTextureLoader.h"
#include <fstream>
#include <vector>

namespace NeoEngine {
TextureData GLTFTextureLoader::Load(const std::string& path)
{
    TextureData tex{};
    std::ifstream file(path, std::ios::binary);
    if (!file) return tex;
    file.seekg(0, std::ios::end);
    const auto size = file.tellg();
    if (size <= 0) return tex;
    file.seekg(0, std::ios::beg);
    std::vector<char> bytes(static_cast<size_t>(size));
    file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!file) return TextureData{};
    return tex;
}
}
