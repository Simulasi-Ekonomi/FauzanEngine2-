#pragma once
#include "GLTFMesh.h"
#include <string>
#include <vector>
#include "GLTFMeshBuilder.h"

namespace NeoEngine {
class GLTFLoader {
public:
    void ParseMeshes(const std::string& json);
    [[nodiscard]] bool ParseMeshesChecked(const std::string& json);
    [[nodiscard]] bool IsValid() const noexcept { return !meshes.empty(); }
    [[nodiscard]] const std::vector<GLTFMesh>& GetMeshes() const noexcept { return meshes; }
private:
    std::vector<GLTFMesh> meshes;
};
} // namespace NeoEngine