#include "../Runtime/Vulkan3DRenderer.h"

#include <array>
#include <cstdio>
#include <vector>

int main() {
    NeoEngine::Vulkan3DRenderer renderer;
    if (!renderer.Initialize(640, 480, "NeoEngine Vulkan3D Smoke")) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL init error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 1;
    }

    constexpr std::array<float, 16> identity{{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    }};

    // Exceed the original 256 KiB geometry arena so the smoke test still covers growth.
    constexpr size_t bulkVertexCount = 9000;
    constexpr size_t bulkIndexCount = 9000;
    std::vector<NeoEngine::Vulkan3DVertex> bulkVertices(bulkVertexCount);
    std::vector<uint32_t> bulkIndices(bulkIndexCount);
    for (size_t i = 0; i < bulkVertexCount; ++i) {
        bulkVertices[i] = NeoEngine::Vulkan3DVertex{0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
    }
    for (size_t i = 0; i < bulkIndexCount; i += 3) {
        bulkIndices[i + 0] = static_cast<uint32_t>(i + 0);
        bulkIndices[i + 1] = static_cast<uint32_t>(i + 1);
        bulkIndices[i + 2] = static_cast<uint32_t>(i + 2);
    }

    constexpr std::array<NeoEngine::Vulkan3DVertex, 3> triangle{{
        NeoEngine::Vulkan3DVertex{-0.05F, -0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.05F, -0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.0F,  0.05F, 0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F},
    }};
    constexpr std::array<uint32_t, 3> triangleIndices{{0, 1, 2}};

    constexpr size_t instanceCount = 2048;
    std::vector<float> instanceTransforms(instanceCount * 16U);
    for (size_t i = 0; i < instanceCount; ++i) {
        std::copy(identity.begin(), identity.end(), instanceTransforms.begin() + i * 16U);
        const float x = -0.95F + static_cast<float>(i % 64U) * 0.03F;
        const float y = -0.95F + static_cast<float>(i / 64U) * 0.03F;
        instanceTransforms[i * 16U + 12U] = x;
        instanceTransforms[i * 16U + 13U] = y;
    }

    // Frame 1: geometry arena growth + GPU instancing. The triangle mesh is uploaded once;
    // all 2048 transforms are consumed by one vkCmdDrawIndexed with instanceCount=2048.
    if (!renderer.BeginFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin1 error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 2;
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

    // Frames 2-4: persistent per-frame geometry and instance arenas must survive frame-slot reuse.
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

    std::printf("VULKAN3D_SMOKE_OK frames=%llu final_vertices=%u final_indices=%u instances=%zu size=%ux%u\n",
                static_cast<unsigned long long>(renderer.LastFrameStats().frameIndex),
                renderer.LastFrameStats().vertexCount, renderer.LastFrameStats().indexCount,
                instanceCount, renderer.LastFrameStats().width, renderer.LastFrameStats().height);
    return 0;
}
