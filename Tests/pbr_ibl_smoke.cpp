#include "Runtime/PBRIBL.h"

#include <cmath>
#include <iostream>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << "\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting pbr_ibl_smoke...\n";

    const glm::vec3 f0 = NeoEngine::PBRComputeF0(glm::vec3(0.8f, 0.2f, 0.1f), 1.0f);
    TEST_CHECK(glm::length(f0 - glm::vec3(0.8f, 0.2f, 0.1f)) < 1.0e-5f, "Metal F0 must follow base color");

    const glm::vec3 reflection = NeoEngine::PBRReflectionDirection(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    TEST_CHECK(std::isfinite(reflection.x) && std::isfinite(reflection.y) && std::isfinite(reflection.z), "Reflection direction must be finite");
    TEST_CHECK(glm::length(reflection) > 0.99f, "Reflection direction must be normalized");

    TEST_CHECK(NeoEngine::PBRReflectionLod(0.0f, 5.0f) == 0.0f, "Smooth surface must use mip zero");
    TEST_CHECK(std::abs(NeoEngine::PBRReflectionLod(0.5f, 6.0f) - 3.0f) < 1.0e-5f, "Roughness-to-LOD mapping is invalid");

    NeoEngine::PBRIBLSettings settings;
    TEST_CHECK(NeoEngine::ValidatePBRIBLSettings(settings), "Default IBL settings must validate");
    settings.environmentIntensity = -1.0f;
    TEST_CHECK(!NeoEngine::ValidatePBRIBLSettings(settings), "Negative environment intensity must be rejected");

    std::cout << "[Smoke Test] pbr_ibl_smoke passed successfully!\n";
    return 0;
}
