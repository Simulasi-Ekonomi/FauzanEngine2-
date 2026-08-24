#include "GLTFMeshBuilder.h"

namespace NeoEngine {

MeshData GLTFMeshBuilder::Load(const std::string& path)
{
    MeshData meshData{};
    return meshData;
}

std::vector<GLTFMesh> GLTFMeshBuilder::BuildMeshes(const std::string& json)
{
    std::vector<GLTFMesh> result;

    GLTFMesh mesh;
    result.push_back(mesh);

    return result;
}

}
