#include <cassert>
#include "GLTFLoader.h"

namespace NeoEngine {

void GLTFLoader::ParseMeshes(const std::string& json) {
    GLTFMeshBuilder builder;
    meshes = builder.BuildMeshes(json);
}

} // namespace NeoEngine
