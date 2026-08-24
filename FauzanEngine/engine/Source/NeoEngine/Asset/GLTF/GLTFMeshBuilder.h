#pragma once

#include <vector>
#include <string>

#include "GLTFMesh.h"
#include "Core/Geometry/MeshData.h"

namespace NeoEngine {

class GLTFMeshBuilder
{
public:

    static MeshData Load(const std::string& path);

    std::vector<GLTFMesh> BuildMeshes(const std::string& json);

};

}
