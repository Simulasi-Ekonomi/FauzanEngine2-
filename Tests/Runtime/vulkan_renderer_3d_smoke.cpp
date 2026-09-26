#include "Runtime/Vulkan3DRenderer.h"
#include "Runtime/RuntimeAssetStreamBridge.h"
#include <SDL3/SDL.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>
#include <fstream>
#include <thread>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            return 1; \
        } \
    } while (0)

int main() {
    std::cout << "[Smoke Test] Starting vulkan_renderer_3d_smoke...\n";

    using namespace NeoEngine;
    AssetRegistry assets;
    AssetResourceManager resources(assets);
    StreamManager streams(1, 8, 1024U, 1024U);
    AssetStreamingQueue queue(16U, 8U);
    RuntimeAssetStreamBridge bridge(assets, resources, streams, queue);
    TEST_CHECK(bridge.Start(), "Runtime asset stream bridge should start");

    const char* texturePath = "vulkan_renderer_3d_stream_refresh.ppm";
    {
        std::ofstream file(texturePath, std::ios::binary);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {255U, 0U, 0U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    Vulkan3DRenderer renderer;
    if (!renderer.Initialize(800, 600, "NeoEngine Vulkan 3D Smoke")) {
        std::cerr << "[TEST FAIL] Vulkan3DRenderer initialization failed with error "
                  << static_cast<int>(renderer.LastError()) << " SDL=" << SDL_GetError() << "\n";
        return 1;
    }

    TEST_CHECK(renderer.Ready(), "Renderer should report Ready after successful initialization");
    TEST_CHECK(renderer.BindAssetStreamBridge(bridge), "Renderer should bind canonical streamed asset bridge");

    StreamRequest streamedTexture{};
    streamedTexture.id = "renderer.refresh.texture";
    streamedTexture.filepath = texturePath;
    streamedTexture.priority = 10.0F;
    streamedTexture.estimatedSizeMB = 1U;
    streamedTexture.kind = static_cast<uint8_t>(AssetKind::Texture);
    TEST_CHECK(bridge.Request(streamedTexture), "Initial streamed texture request failed");

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
    if (!renderer.DrawIndexed(skinnedVertices, indices, identity.data())) {
        std::cerr << "[TEST FAIL] DrawIndexed with GPU skinning failed; renderer error="
                  << static_cast<int>(renderer.LastError()) << "\n";
        return 1;
    }
    TEST_CHECK(renderer.EndFrame(), "EndFrame failed");
    std::vector<uint8_t> firstFrame;
    TEST_CHECK(renderer.ReadbackLastFrame(firstFrame), "First presented-frame readback failed");
    TEST_CHECK(firstFrame.size() == static_cast<size_t>(800U * 600U * 4U), "First readback size mismatch");
    std::array<bool, 256> firstByteValues{};
    for (uint8_t byte : firstFrame) firstByteValues[byte] = true;
    const size_t firstUniqueBytes = static_cast<size_t>(std::count(firstByteValues.begin(), firstByteValues.end(), true));
    TEST_CHECK(firstUniqueBytes > 4U, "First readback contains no rendered geometry signal");
    const uint64_t firstChecksum = std::accumulate(firstFrame.begin(), firstFrame.end(), uint64_t{0});

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
    std::vector<uint8_t> secondFrame;
    TEST_CHECK(renderer.ReadbackLastFrame(secondFrame), "Second presented-frame readback failed");
    TEST_CHECK(secondFrame.size() == static_cast<size_t>(640U * 480U * 4U), "Second readback size mismatch");
    const uint64_t secondChecksum = std::accumulate(secondFrame.begin(), secondFrame.end(), uint64_t{0});
    TEST_CHECK(secondChecksum != firstChecksum, "GPU skinning palette change did not alter presented pixels");

    bool initialStreamReady = false;
    for (uint32_t frame = 0U; frame < 120U && !initialStreamReady; ++frame) {
        bridge.Pump();
        TEST_CHECK(renderer.BeginFrame(), "BeginFrame during initial streamed texture lifecycle failed");
        if (renderer.IsStreamedTextureReady(streamedTexture.id)) {
            TEST_CHECK(renderer.BindStreamedTexture(streamedTexture.id), "Initial streamed texture descriptor bind failed");
            initialStreamReady = true;
        }
        TEST_CHECK(renderer.DrawIndexed(vertices, indices, identity.data()), "Initial streamed texture frame draw failed");
        TEST_CHECK(renderer.EndFrame(), "EndFrame during initial streamed texture lifecycle failed");
        if (!initialStreamReady) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    TEST_CHECK(initialStreamReady, "Initial streamed texture did not reach authoritative GPU readiness");

    {
        std::ofstream file(texturePath, std::ios::binary | std::ios::trunc);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {0U, 0U, 255U};
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
    }

    TEST_CHECK(bridge.Refresh(streamedTexture), "Resident streamed texture refresh request failed");
    TEST_CHECK(bridge.ResidentGpuUploadCount() == 1U, "Old streamed GPU residency must remain authoritative during refresh");
    TEST_CHECK(renderer.IsStreamedTextureReady(streamedTexture.id), "Old renderer texture must remain usable while refresh is pending");

    bool refreshedStreamReady = false;
    for (uint32_t frame = 0U; frame < 120U && !refreshedStreamReady; ++frame) {
        bridge.Pump();
        TEST_CHECK(renderer.BeginFrame(), "BeginFrame during refreshed streamed texture lifecycle failed");
        const bool replacementCompleted = bridge.PendingGpuUploadCount() == 0U;
        if (replacementCompleted) {
            TEST_CHECK(renderer.IsStreamedTextureReady(streamedTexture.id),
                       "Refreshed streamed texture descriptor must be resident after authoritative completion");
            TEST_CHECK(renderer.BindStreamedTexture(streamedTexture.id),
                       "Refreshed streamed texture descriptor bind failed");
            refreshedStreamReady = true;
        }
        TEST_CHECK(renderer.DrawIndexed(vertices, indices, identity.data()),
                   "Refreshed streamed texture frame draw failed");
        TEST_CHECK(renderer.EndFrame(), "EndFrame during refreshed streamed texture lifecycle failed");
        if (!refreshedStreamReady) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    TEST_CHECK(refreshedStreamReady, "Refreshed streamed texture did not reach authoritative GPU readiness");

    TEST_CHECK(bridge.ReleaseAllGpuUploads(), "Resident refreshed GPU upload release failed");
    renderer.Reset();
    TEST_CHECK(!renderer.Ready(), "Renderer should not be ready after Reset");
    bridge.Stop();
    std::remove(texturePath);

    std::cout << "[Smoke Test] vulkan_renderer_3d_smoke passed successfully!\n";
    return 0;
}