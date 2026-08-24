#include "GLTFTextureLoader.h"
#include <iostream>

namespace NeoEngine {

TextureData GLTFTextureLoader::Load(const std::string& path)
{
    TextureData tex{};

    std::cout << "Loading texture: " << path << std::endl;

    return tex;
}

}
