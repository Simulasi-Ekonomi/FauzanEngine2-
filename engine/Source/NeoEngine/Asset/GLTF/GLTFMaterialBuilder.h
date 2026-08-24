#pragma once
#include <string>
#include <vector>

namespace NeoEngine {

struct GLTFMaterial {
    std::string name;
    float baseColor[4] = {1,1,1,1};
    float metallic = 0;
    float roughness = 0.5f;
    float emissive[3] = {0,0,0};
    std::string baseColorTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    bool doubleSided = false;
};

class GLTFMaterialBuilder {
public:
    GLTFMaterialBuilder() = default;
    GLTFMaterial BuildDefaultMaterial() {
        GLTFMaterial mat;
        mat.name = "Default";
        return mat;
    }
    GLTFMaterial BuildFromJSON(const std::string& json);
};

} // namespace NeoEngine
