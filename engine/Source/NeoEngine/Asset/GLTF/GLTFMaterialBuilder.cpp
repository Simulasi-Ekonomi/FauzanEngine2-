#include "GLTFMaterialBuilder.h"
namespace NeoEngine {
GLTFMaterial GLTFMaterialBuilder::BuildFromJSON(const std::string& json) {
    return BuildDefaultMaterial();
}
}
