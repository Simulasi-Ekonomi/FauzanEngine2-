#include "Runtime/PBRIBL.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {

glm::vec3 PBRComputeF0(const glm::vec3& albedo, float metallic) noexcept {
    const float m = std::clamp(metallic, 0.0f, 1.0f);
    return glm::mix(glm::vec3(0.04f), glm::max(albedo, glm::vec3(0.0f)), m);
}

glm::vec3 PBRReflectionDirection(const glm::vec3& normal,
                                  const glm::vec3& viewDirection) noexcept {
    const glm::vec3 n = glm::normalize(normal);
    const glm::vec3 v = glm::normalize(viewDirection);
    if (!std::isfinite(n.x) || !std::isfinite(n.y) || !std::isfinite(n.z) ||
        !std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
        return glm::vec3(0.0f);
    }
    return glm::normalize(glm::reflect(-v, n));
}

float PBRReflectionLod(float roughness, float maxLod) noexcept {
    return std::clamp(roughness, 0.0f, 1.0f) * std::max(maxLod, 0.0f);
}

bool ValidatePBRIBLSettings(const PBRIBLSettings& settings) noexcept {
    return std::isfinite(settings.environmentIntensity) &&
           std::isfinite(settings.maxReflectionLod) &&
           std::isfinite(settings.irradianceStrength) &&
           settings.environmentIntensity >= 0.0f &&
           settings.maxReflectionLod >= 0.0f &&
           settings.irradianceStrength >= 0.0f;
}

} // namespace NeoEngine
