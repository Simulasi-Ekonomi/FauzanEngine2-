#include "Runtime/Vulkan3DRenderer.h"
#include <array>
#include <iostream>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_renderer_3d_smoke...\n";

    NeoEngine::Vulkan3DRenderer renderer;
    if (!renderer.Initialize(800, 600, "NeoEngine Vulkan 3D Smoke")) {
        std::cout << "[INFO] Vulkan3DRenderer failed to initialize (likely headless or unavailable Vulkan/SDL environment).\n";
        return 0;
    }

    TEST_CHECK(renderer.Ready(), "Vulkan3DRenderer should be ready after Initialize");

    const std::vector<NeoEngine::Vulkan3DVertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{ 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}}
    };
    const std::vector<uint32_t> indices = {0, 1, 2};
    constexpr std::array<float, 16> identity = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    TEST_CHECK(renderer.BeginFrame(), "BeginFrame failed");
    TEST_CHECK(renderer.DrawIndexed(vertices, indices, identity.data()), "DrawIndexed failed");
    TEST_CHECK(renderer.EndFrame(), "EndFrame failed");

    const auto& stats = renderer.LastFrameStats();
    TEST_CHECK(stats.width == 800 && stats.height == 600, "Unexpected framebuffer dimensions");
    TEST_CHECK(stats.vertexCount == vertices.size(), "Unexpected vertex count");
    TEST_CHECK(stats.indexCount == indices.size(), "Unexpected index count");
    TEST_CHECK(stats.frameIndex > 0, "Frame index did not advance");

    renderer.Reset();
    TEST_CHECK(!renderer.Ready(), "Renderer should not remain ready after Reset");

    std::cout << "[Smoke Test] vulkan_renderer_3d_smoke passed successfully!\n";
    return 0;
}
