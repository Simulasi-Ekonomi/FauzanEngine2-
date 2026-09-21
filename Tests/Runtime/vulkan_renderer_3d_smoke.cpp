#include "Runtime/Vulkan3DRenderer.h"

#include <array>
#include <cstdint>
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
        const auto error = renderer.LastError();
        if (error == NeoEngine::Vulkan3DRendererError::SdlFailure ||
            error == NeoEngine::Vulkan3DRendererError::VulkanFailure ||
            error == NeoEngine::Vulkan3DRendererError::ShaderUnavailable) {
            std::cout << "[INFO] Vulkan 3D runtime unavailable in this environment; smoke test skipped.\n";
            return 0;
        }
        std::cerr << "[TEST FAIL] Vulkan3DRenderer initialization failed with error "
                  << static_cast<int>(error) << "\n";
        return 1;
    }

    TEST_CHECK(renderer.Ready(), "Renderer should report Ready after successful initialization");

    const std::array<NeoEngine::Vulkan3DVertex, 3> vertices{{
        {-0.5F, -0.5F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, {}, {}},
        { 0.5F, -0.5F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, {}, {}},
        { 0.0F,  0.5F, 0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F, {}, {}}
    }};
    const std::array<uint32_t, 3> indices{{0U, 1U, 2U}};
    constexpr std::array<float, 16> identity{{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F
    }};

    std::vector<NeoEngine::Mat4> palette(1U);
    palette[0].m[0] = palette[0].m[5] = palette[0].m[10] = palette[0].m[15] = 1.0F;
    palette[0].m[12] = 0.25F;
    std::array<NeoEngine::Vulkan3DVertex, 3> skinnedVertices = vertices;
    for (auto& vertex : skinnedVertices) { vertex.boneIndices = {0U, 0U, 0U, 0U}; vertex.boneWeights = {1.0F, 0.0F, 0.0F, 0.0F}; }

    TEST_CHECK(!renderer.Resize(0, 600), "Zero-width resize must be rejected");
    TEST_CHECK(renderer.LastError() == NeoEngine::Vulkan3DRendererError::InvalidConfiguration,
               "Invalid resize should report InvalidConfiguration");
    TEST_CHECK(renderer.Resize(800, 600), "Renderer should recover after rejected resize");
    TEST_CHECK(renderer.BeginFrame(), "BeginFrame failed");
    TEST_CHECK(!renderer.UploadSkinningPalette({}), "Empty skinning palette must be rejected");
    TEST_CHECK(renderer.UploadSkinningPalette(palette), "UploadSkinningPalette failed");
    TEST_CHECK(renderer.DrawIndexed(skinnedVertices, indices, identity.data()), "DrawIndexed with GPU skinning failed");
    TEST_CHECK(renderer.EndFrame(), "EndFrame failed");

    const auto& stats = renderer.LastFrameStats();
    TEST_CHECK(stats.width == 800U && stats.height == 600U, "Frame dimensions mismatch");
    TEST_CHECK(stats.vertexCount == vertices.size(), "Vertex count mismatch");
    TEST_CHECK(stats.indexCount == indices.size(), "Index count mismatch");
    TEST_CHECK(stats.frameIndex == 1U, "First frame index mismatch");

    TEST_CHECK(renderer.Resize(640, 480), "Renderer resize failed");
    TEST_CHECK(renderer.BeginFrame(), "BeginFrame after resize failed");
    palette[0].m[12] = -0.25F;
    TEST_CHECK(renderer.UploadSkinningPalette(palette), "Second skinning palette upload failed");
    TEST_CHECK(renderer.DrawIndexed(skinnedVertices, indices, identity.data()), "Second GPU skinning draw failed");
    TEST_CHECK(renderer.EndFrame(), "EndFrame after resize failed");

    renderer.Reset();
    TEST_CHECK(!renderer.Ready(), "Renderer should not be ready after Reset");

    std::cout << "[Smoke Test] vulkan_renderer_3d_smoke passed successfully!\n";
    return 0;
}