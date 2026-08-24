#include "GLTFMesh.h"
#include <cassert>
#pragma once
#include <string>
#include <vector>
#include "GLTFMeshBuilder.h"

namespace NeoEngine {

class GLTFLoader {
public:
    std::vector<GLTFMesh> meshes;
    void ParseMeshes(const std::string& json);
};

} // namespace NeoEngine
