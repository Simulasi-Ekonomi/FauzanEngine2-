#include "GLTFLoader.h"

namespace NeoEngine {
void GLTFLoader::ParseMeshes(const std::string& json) {
    (void)ParseMeshesChecked(json);
}
bool GLTFLoader::ParseMeshesChecked(const std::string& json) {
    meshes.clear();
    if (json.empty()) return false;
    auto parsed = GLTFMeshBuilder{}.BuildMeshes(json);
    if (parsed.empty()) return false;
    meshes = std::move(parsed);
    return true;
}
} // namespace NeoEngine