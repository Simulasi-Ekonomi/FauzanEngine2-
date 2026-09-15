#pragma once

#include <cstddef>
#include <cstdint>

#include <glm/glm.hpp>

namespace NeoEngine {

struct alignas(16) PBRDirectionalLight {
    glm::vec4 directionIntensity{0.0f, -1.0f, 0.0f, 1.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct alignas(16) PBRPointLight {
    glm::vec4 positionRadius{0.0f, 0.0f, 0.0f, 10.0f};
    glm::vec4 colorIntensity{1.0f, 1.0f, 1.0f, 1.0f};
};

struct alignas(16) PBRSpotLight {
    glm::vec4 positionRadius{0.0f, 0.0f, 0.0f, 10.0f};
    glm::vec4 directionInnerCos{0.0f, -1.0f, 0.0f, 0.9f};
    glm::vec4 colorIntensity{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 outerCosPadding{0.75f, 0.0f, 0.0f, 0.0f};
};

static_assert(sizeof(PBRDirectionalLight) == 32);
static_assert(sizeof(PBRPointLight) == 32);
static_assert(sizeof(PBRSpotLight) == 64);

// CPU-side reference implementation of the same Cook-Torrance terms used by the PBR shader.
[[nodiscard]] float PBRDistributionGGX(float nDotH, float roughness) noexcept;
[[nodiscard]] float PBRGeometrySchlickGGX(float nDotV, float roughness) noexcept;
[[nodiscard]] float PBRGeometrySmith(float nDotV, float nDotL, float roughness) noexcept;
[[nodiscard]] glm::vec3 PBRFresnelSchlick(float cosTheta, const glm::vec3& f0) noexcept;
[[nodiscard]] glm::vec3 PBRFresnelSchlickRoughness(float cosTheta, const glm::vec3& f0, float roughness) noexcept;

// Validates a light payload before it is copied into a GPU light buffer.
[[nodiscard]] bool ValidatePBRLight(const PBRDirectionalLight& light) noexcept;
[[nodiscard]] bool ValidatePBRLight(const PBRPointLight& light) noexcept;
[[nodiscard]] bool ValidatePBRLight(const PBRSpotLight& light) noexcept;

} // namespace NeoEngine
