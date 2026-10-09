#include "Runtime/RuntimeAssetStreamBridge.h"
#include "Runtime/Vulkan3DRenderer.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#define TEST_CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[TEST FAIL] " << msg << " (" << #cond << ")\n"; \
            cleanup(); \
            return 1; \
        } \
    } while (0)

int main() {
    using namespace NeoEngine;
    std::cout << "[Smoke Test] Starting vulkan_renderer_3d_smoke...\n";

    constexpr const char* assetPath = "vulkan_renderer_3d_streamed_texture.ppm";
    {
        std::ofstream file(assetPath, std::ios::binary | std::ios::trunc);
        const char header[] = "P6\n1 1\n255\n";
        const unsigned char pixel[] = {24U, 96U, 192U};
        if (!file.is_open()) {
            std::cerr << "[TEST FAIL] could not create streamed texture fixture\n";
            return 1;
        }
        file.write(header, sizeof(header) - 1U);
        file.write(reinterpret_cast<const char*>(pixel), sizeof(pixel));
        if (!file.good()) {
            std::remove(assetPath);
            std::cerr << "[TEST FAIL] could not write streamed texture fixture\n";
            return 1;
        }
    }

    AssetRegistry assets;
    AssetResourceManager resources(assets);
    StreamManager streams;
    AssetStreamingQueue queue;
    RuntimeAssetStreamBridge bridge(assets, resources, streams, queue);
    Vulkan3DRenderer renderer;
    const auto cleanup = [&]() noexcept {
        renderer.FlushAssetUploads();
        (void)bridge.ReleaseAllGpuUploads();
        renderer.Reset();
        bridge.Stop();
        std::remove(assetPath);
    };

    TEST_CHECK(bridge.Start(), "stream manager failed to start");
    if (!renderer.Initialize(800, 600, "NeoEngine Vulkan 3D Smoke")) {
        std::cout << "[INFO] Vulkan3DRenderer initialization unavailable in this environment; streaming integration not exercised.\n";
        cleanup();
        return 0;
    }
    TEST_CHECK(renderer.Ready(), "Vulkan3DRenderer should be ready after Initialize");
    TEST_CHECK(renderer.BindAssetStreamBridge(bridge), "renderer must bind the runtime asset bridge");

    const std::vector<Vulkan3DVertex> vertices = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
        { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f}
    };
    const std::vector<uint32_t> indices = {0U, 1U, 2U};
    constexpr std::array<float, 16> identity = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    StreamRequest request{};
    request.id = "renderer.streamed.texture";
    request.filepath = assetPath;
    request.priority = 10.0F;
    request.estimatedSizeMB = 1U;
    request.kind = static_cast<uint8_t>(AssetKind::Texture);
    TEST_CHECK(bridge.Request(request), "file-to-GPU stream request should be accepted");

    bool streamedTextureReady = false;
    for (uint32_t frame = 0U; frame < 120U && !streamedTextureReady; ++frame) {
        (void)bridge.Pump();
        TEST_CHECK(renderer.BeginFrame(), "BeginFrame failed during streamed upload");
        if (renderer.IsStreamedTextureReady(request.id)) {
            TEST_CHECK(renderer.BindStreamedTexture(request.id), "completed streamed texture descriptor should bind");
            streamedTextureReady = true;
        }
        TEST_CHECK(renderer.DrawIndexed(vertices, indices, identity.data()), "DrawIndexed failed");
        TEST_CHECK(renderer.EndFrame(), "EndFrame failed during streamed upload");
        if (!streamedTextureReady) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    TEST_CHECK(streamedTextureReady, "file load, GPU upload fence, renderer descriptor publication, and texture binding must complete");
    AssetResourceReceipt receipt{};
    TEST_CHECK(resources.Query(request.id, receipt), "streamed resource receipt should remain queryable");
    TEST_CHECK(receipt.refCount == 1U && receipt.gpuUploadsInFlight == 0U && receipt.gpuResident,
               "resource residency must publish only after authoritative GPU completion");
    TEST_CHECK(bridge.PendingGpuUploadCount() == 0U && bridge.ResidentGpuUploadCount() == 1U,
               "bridge must transfer upload from pending to resident ownership");

    const auto& stats = renderer.LastFrameStats();
    TEST_CHECK(stats.width == 800U && stats.height == 600U, "Unexpected framebuffer dimensions");
    TEST_CHECK(stats.vertexCount == vertices.size(), "Unexpected vertex count");
    TEST_CHECK(stats.indexCount == indices.size(), "Unexpected index count");
    TEST_CHECK(stats.frameIndex > 0U, "Frame index did not advance");

    renderer.FlushAssetUploads();
    TEST_CHECK(bridge.ReleaseAllGpuUploads(), "resident GPU resources must release through their canonical owner");
    TEST_CHECK(bridge.ResidentGpuUploadCount() == 0U, "bridge should not retain resident GPU upload ownership");
    renderer.Reset();
    TEST_CHECK(!renderer.Ready(), "Renderer should not remain ready after Reset");
    bridge.Stop();
    std::remove(assetPath);

    std::cout << "VULKAN_RENDERER_3D_SMOKE_OK file_to_registry_to_gpu_to_descriptor=1 fence_authority=1 release=1\n";
    return 0;
}
