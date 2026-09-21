#include "GLTFMeshBuilder.h"

namespace NeoEngine {

MeshData GLTFMeshBuilder::Load(const std::string& path)
{
    MeshData meshData{};
    if (path.empty()) return meshData;
    return meshData;
}

std::vector<GLTFMesh> GLTFMeshBuilder::BuildMeshes(const std::string& json)
{
    std::vector<GLTFMesh> result;
    if (json.empty()) return result;
    return result;
}

}
