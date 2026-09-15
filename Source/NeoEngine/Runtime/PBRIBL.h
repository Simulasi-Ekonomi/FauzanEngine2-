#pragma once

#include <cstdint>

#include <glm/glm.hpp>

namespace NeoEngine {

struct PBRIBLSettings {
    float environmentIntensity = 1.0f;
    float maxReflectionLod = 5.0f;
    float irradianceStrength = 1.0f;
};

[[nodiscard]] glm::vec3 PBRComputeF0(const glm::vec3& albedo, float metallic) noexcept;
[[nodiscard]] glm::vec3 PBRReflectionDirection(const glm::vec3& normal,
                                                const glm::vec3& viewDirection) noexcept;
[[nodiscard]] float PBRReflectionLod(float roughness, float maxLod) noexcept;
[[nodiscard]] bool ValidatePBRIBLSettings(const PBRIBLSettings& settings) noexcept;

} // namespace NeoEngine
