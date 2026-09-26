#include "../Runtime/Vulkan3DRenderer.h"
#include "../Runtime/RuntimeAssetStreamBridge.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>
#include <fstream>
#include <thread>
#include <chrono>

int main() {
    NeoEngine::AssetRegistry registry;
    NeoEngine::AssetResourceManager resources(registry);
    NeoEngine::StreamManager streams(1U, 8U, 16U, 64U);
    NeoEngine::AssetStreamingQueue queue(16U, 8U);
    NeoEngine::RuntimeAssetStreamBridge bridge(registry, resources, streams, queue);
    if (!bridge.Start()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL bridge_start\n");
        return 1;
    }

    const char* texturePath = "vulkan_3d_stream_texture.ppm";
    {
        std::ofstream texture(texturePath, std::ios::binary);
        texture << "P6\n2 2\n255\n";
        const unsigned char pixels[] = {
            255U, 0U, 0U, 255U, 0U, 0U,
            255U, 0U, 0U, 255U, 0U, 0U
        };
        texture.write(reinterpret_cast<const char*>(pixels), sizeof(pixels));
    }
    NeoEngine::StreamRequest textureRequest{};
    textureRequest.id = "vulkan.smoke.texture";
    textureRequest.filepath = texturePath;
    textureRequest.priority = 10.0F;
    textureRequest.estimatedSizeMB = 1U;
    textureRequest.kind = static_cast<uint8_t>(NeoEngine::AssetKind::Texture);
    if (!bridge.Request(textureRequest)) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL bridge_request\n");
        return 1;
    }
    bool pending = false;
    for (uint32_t i = 0U; i < 120U && !pending; ++i) {
        bridge.Pump();
        pending = bridge.PendingGpuUploadCount() != 0U;
        if (!pending) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    if (!pending) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL texture_pending\n");
        return 1;
    }

    NeoEngine::Vulkan3DRenderer renderer;
    if (!renderer.Initialize(640, 480, "NeoEngine Vulkan3D Smoke") ||
        !renderer.BindAssetStreamBridge(bridge)) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL init_or_bind error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 1;
    }

    constexpr std::array<float, 16> identity{{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    }};

    constexpr size_t bulkVertexCount = 9000;
    constexpr size_t bulkIndexCount = 9000;
    std::vector<NeoEngine::Vulkan3DVertex> bulkVertices(bulkVertexCount);
    std::vector<uint32_t> bulkIndices(bulkIndexCount);
    for (size_t i = 0; i < bulkVertexCount; ++i)
        bulkVertices[i] = NeoEngine::Vulkan3DVertex{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
    for (size_t i = 0; i < bulkIndexCount; i += 3) {
        bulkIndices[i + 0] = static_cast<uint32_t>(i + 0);
        bulkIndices[i + 1] = static_cast<uint32_t>(i + 1);
        bulkIndices[i + 2] = static_cast<uint32_t>(i + 2);
    }

    std::array<NeoEngine::Vulkan3DVertex, 3> triangle{{
        NeoEngine::Vulkan3DVertex{-0.05F, -0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.05F, -0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.0F,  0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F},
    }};
    constexpr std::array<uint32_t, 3> triangleIndices{{0, 1, 2}};

    constexpr size_t instanceCount = 2048;
    std::vector<float> instanceTransforms(instanceCount * 16U);
    for (size_t i = 0; i < instanceCount; ++i) {
        std::copy(identity.begin(), identity.end(), instanceTransforms.begin() + i * 16U);
        instanceTransforms[i * 16U + 12U] = -0.95F + static_cast<float>(i % 64U) * 0.03F;
        instanceTransforms[i * 16U + 13U] = -0.95F + static_cast<float>(i / 64U) * 0.03F;
    }

    std::vector<NeoEngine::Mat4> palette(1);
    palette[0].m[0] = palette[0].m[5] = palette[0].m[10] = palette[0].m[15] = 1.0F;
    palette[0].m[12] = 0.02F;
    if (!renderer.BeginFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin1 error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 2;
    }
    if (!renderer.UploadSkinningPalette(palette)) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL skinning upload error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 3;
    }
    for (auto& vertex : triangle) {
        vertex.boneIndices = {0U, 0U, 0U, 0U};
        vertex.boneWeights = {1.0F, 0.0F, 0.0F, 0.0F};
    }
    if (!renderer.DrawIndexed(bulkVertices, bulkIndices, identity.data()) ||
        !renderer.DrawIndexedInstanced(triangle, triangleIndices, instanceTransforms)) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL draw1 error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 3;
    }
    if (!renderer.EndFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL end1 error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 4;
    }

    const auto& firstStats = renderer.LastFrameStats();
    if (firstStats.frameIndex != 1 || firstStats.vertexCount != bulkVertexCount + 3 ||
        firstStats.indexCount != bulkIndexCount + 3 || firstStats.width != 640 || firstStats.height != 480) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL stats1 frame=%llu vertices=%u indices=%u size=%ux%u\n",
                     static_cast<unsigned long long>(firstStats.frameIndex), firstStats.vertexCount,
                     firstStats.indexCount, firstStats.width, firstStats.height);
        return 5;
    }

    for (unsigned frame = 2; frame <= 4; ++frame) {
        if (!renderer.BeginFrame()) {
            std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin%u error=%u\n", frame,
                         static_cast<unsigned>(renderer.LastError()));
            return 6;
        }
        if (!renderer.DrawIndexed(triangle, triangleIndices, identity.data()) || !renderer.EndFrame()) {
            std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL frame%u error=%u\n", frame,
                         static_cast<unsigned>(renderer.LastError()));
            return 7;
        }
        const auto& stats = renderer.LastFrameStats();
        if (stats.frameIndex != frame || stats.vertexCount != 3 || stats.indexCount != 3 ||
            stats.width != 640 || stats.height != 480) {
            std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL stats%u frame=%llu vertices=%u indices=%u size=%ux%u\n",
                         frame, static_cast<unsigned long long>(stats.frameIndex), stats.vertexCount,
                         stats.indexCount, stats.width, stats.height);
            return 8;
        }
    }

    // The first frame records the decoded texture upload outside the render pass.
    if (!renderer.BeginFrame() || !renderer.EndFrame()) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL texture_upload_frame error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 9;
    }

    // Completion is published only after the submitted frame fence is observed.
    if (!renderer.BeginFrame() || !renderer.IsStreamedTextureReady(textureRequest.id) ||
        !renderer.BindStreamedTexture(textureRequest.id) ||
        !renderer.DrawIndexed(triangle, triangleIndices, identity.data()) ||
        !renderer.EndFrame()) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL texture_bind_draw error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 10;
    }

    std::vector<uint8_t> texturedFrame;
    if (!renderer.ReadbackLastFrame(texturedFrame) || texturedFrame.size() != 640U * 480U * 4U) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL texture_readback\n");
        return 11;
    }
    const size_t center = ((480U / 2U) * 640U + (640U / 2U)) * 4U;
    if (texturedFrame[center + 0U] <= texturedFrame[center + 1U] * 2U ||
        texturedFrame[center + 0U] <= texturedFrame[center + 2U] * 2U) {
        std::remove(texturePath);
        bridge.Stop();
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL texture_sampling rgba=%u,%u,%u,%u\n",
                     texturedFrame[center + 0U], texturedFrame[center + 1U],
                     texturedFrame[center + 2U], texturedFrame[center + 3U]);
        return 12;
    }

    bridge.Stop();
    std::remove(texturePath);
    std::printf("VULKAN3D_SMOKE_OK frames=%llu final_vertices=%u final_indices=%u instances=%zu texture_upload=1 texture_fence=1 texture_sampling=1 size=%ux%u\n",
                static_cast<unsigned long long>(renderer.LastFrameStats().frameIndex),
                renderer.LastFrameStats().vertexCount, renderer.LastFrameStats().indexCount,
                instanceCount, renderer.LastFrameStats().width, renderer.LastFrameStats().height);
    return 0;
}
