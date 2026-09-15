#include "Runtime/PBRLighting.h"

#include <algorithm>
#include <cmath>

namespace NeoEngine {

namespace {
constexpr float kMinRoughness = 0.045f;
constexpr float kEpsilon = 1.0e-5f;

[[nodiscard]] bool Finite(const glm::vec4& value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
}

[[nodiscard]] float ClampCos(float value) noexcept {
    return std::clamp(value, 0.0f, 1.0f);
}
} // namespace

float PBRDistributionGGX(float nDotH, float roughness) noexcept {
    const float nH = ClampCos(nDotH);
    const float alpha = std::max(roughness, kMinRoughness);
    const float a2 = alpha * alpha;
    const float denom = nH * nH * (a2 - 1.0f) + 1.0f;
    const float normalized = a2 / std::max(glm::pi<float>() * denom * denom, kEpsilon);
    return std::max(normalized, 0.0f);
}

float PBRGeometrySchlickGGX(float nDotV, float roughness) noexcept {
    const float nV = ClampCos(nDotV);
    const float alpha = std::max(roughness, kMinRoughness);
    const float k = ((alpha + 1.0f) * (alpha + 1.0f)) / 8.0f;
    return nV / std::max(nV * (1.0f - k) + k, kEpsilon);
}

float PBRGeometrySmith(float nDotV, float nDotL, float roughness) noexcept {
    return PBRGeometrySchlickGGX(nDotV, roughness) * PBRGeometrySchlickGGX(nDotL, roughness);
}

glm::vec3 PBRFresnelSchlick(float cosTheta, const glm::vec3& f0) noexcept {
    const float c = ClampCos(cosTheta);
    return f0 + (glm::vec3(1.0f) - f0) * std::pow(1.0f - c, 5.0f);
}

glm::vec3 PBRFresnelSchlickRoughness(float cosTheta, const glm::vec3& f0, float roughness) noexcept {
    const float c = ClampCos(cosTheta);
    const glm::vec3 oneMinusRoughness = glm::vec3(1.0f - std::clamp(roughness, 0.0f, 1.0f));
    const glm::vec3 maxReflectance = glm::max(oneMinusRoughness, f0);
    return f0 + (maxReflectance - f0) * std::pow(1.0f - c, 5.0f);
}

bool ValidatePBRLight(const PBRDirectionalLight& light) noexcept {
    const glm::vec3 direction(light.directionIntensity);
    return Finite(light.directionIntensity) && Finite(light.color) &&
           glm::length2(direction) > kEpsilon && light.directionIntensity.w >= 0.0f &&
           light.color.w >= 0.0f;
}

bool ValidatePBRLight(const PBRPointLight& light) noexcept {
    return Finite(light.positionRadius) && Finite(light.colorIntensity) &&
           light.positionRadius.w > 0.0f && light.colorIntensity.w >= 0.0f;
}

bool ValidatePBRLight(const PBRSpotLight& light) noexcept {
    const glm::vec3 direction(light.directionInnerCos);
    return Finite(light.positionRadius) && Finite(light.directionInnerCos) &&
           Finite(light.colorIntensity) && Finite(light.outerCosPadding) &&
           light.positionRadius.w > 0.0f && glm::length2(direction) > kEpsilon &&
           light.directionInnerCos.w >= 0.0f && light.outerCosPadding.x >= 0.0f &&
           light.outerCosPadding.x < light.directionInnerCos.w && light.colorIntensity.w >= 0.0f;
}

} // namespace NeoEngine
