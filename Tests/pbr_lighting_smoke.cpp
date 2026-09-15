#include "Runtime/PBRLighting.h"

#include <cmath>
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting pbr_lighting_smoke...\n";

    const float d0 = NeoEngine::PBRDistributionGGX(1.0f, 0.5f);
    const float d45 = NeoEngine::PBRDistributionGGX(0.7071067f, 0.5f);
    TEST_CHECK(std::isfinite(d0) && d0 > 0.0f, "GGX distribution must be finite and positive");
    TEST_CHECK(std::isfinite(d45) && d45 > 0.0f, "GGX distribution must remain finite away from the normal");

    const float g = NeoEngine::PBRGeometrySmith(1.0f, 1.0f, 0.5f);
    TEST_CHECK(std::isfinite(g) && g > 0.0f && g <= 1.0f, "Smith geometry term is invalid");

    const glm::vec3 f0(0.04f);
    const glm::vec3 fNormal = NeoEngine::PBRFresnelSchlick(1.0f, f0);
    const glm::vec3 fGrazing = NeoEngine::PBRFresnelSchlick(0.0f, f0);
    TEST_CHECK(glm::length(fNormal - f0) < 1.0e-5f, "Fresnel at normal incidence must equal F0");
    TEST_CHECK(fGrazing.x > fNormal.x && fGrazing.x <= 1.0f, "Fresnel must increase toward grazing incidence");

    NeoEngine::PBRDirectionalLight directional;
    directional.directionIntensity = glm::vec4(0.0f, -1.0f, 0.0f, 3.0f);
    TEST_CHECK(NeoEngine::ValidatePBRLight(directional), "Directional light validation failed");

    NeoEngine::PBRPointLight point;
    point.positionRadius = glm::vec4(0.0f, 2.0f, 0.0f, 12.0f);
    point.colorIntensity = glm::vec4(1.0f, 0.8f, 0.6f, 25.0f);
    TEST_CHECK(NeoEngine::ValidatePBRLight(point), "Point light validation failed");

    NeoEngine::PBRSpotLight spot;
    spot.positionRadius = glm::vec4(0.0f, 3.0f, 0.0f, 15.0f);
    spot.directionInnerCos = glm::vec4(0.0f, -1.0f, 0.0f, 0.9f);
    spot.colorIntensity = glm::vec4(1.0f, 1.0f, 1.0f, 30.0f);
    spot.outerCosPadding.x = 0.75f;
    TEST_CHECK(NeoEngine::ValidatePBRLight(spot), "Spot light validation failed");

    spot.outerCosPadding.x = 0.95f;
    TEST_CHECK(!NeoEngine::ValidatePBRLight(spot), "Invalid spot cone must be rejected");

    std::cout << "[Smoke Test] pbr_lighting_smoke passed successfully!\n";
    return 0;
}
