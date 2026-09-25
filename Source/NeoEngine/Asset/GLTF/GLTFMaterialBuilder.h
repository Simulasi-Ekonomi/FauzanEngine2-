#pragma once
#include <string>

namespace NeoEngine {

struct GLTFMaterial {
    std::string name;
    float baseColor[4] = {1.0F, 1.0F, 1.0F, 1.0F};
    float metallic = 0.0F;
    float roughness = 0.5F;
    float emissive[3] = {0.0F, 0.0F, 0.0F};
    std::string baseColorTexture;
    std::string normalTexture;
    std::string metallicRoughnessTexture;
    bool doubleSided = false;
};

class GLTFMaterialBuilder {
public:
    GLTFMaterialBuilder() = default;
    [[nodiscard]] GLTFMaterial BuildDefaultMaterial() const;
    [[nodiscard]] GLTFMaterial BuildFromJSON(const std::string& json) const;
    [[nodiscard]] bool TryBuildFromJSON(const std::string& json, GLTFMaterial& out) const;
};

} // namespace NeoEngine