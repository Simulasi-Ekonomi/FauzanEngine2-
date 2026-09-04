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

    // Deliberately exceed the old 256 KiB initial arena to exercise capacity growth,
    // then issue a second draw in the same frame to exercise persistent append.
    constexpr size_t bulkVertexCount = 9000;
    constexpr size_t bulkIndexCount = 9000;
    std::vector<NeoEngine::Vulkan3DVertex> bulkVertices(bulkVertexCount);
    std::vector<uint32_t> bulkIndices(bulkIndexCount);
    for (size_t i = 0; i < bulkVertexCount; ++i) {
        bulkVertices[i] = NeoEngine::Vulkan3DVertex{
            0.0F, 0.0F, 0.0F,
            0.0F, 0.0F, 1.0F,
            0.0F, 0.0F
        };
    }
    for (size_t i = 0; i < bulkIndexCount; i += 3) {
        bulkIndices[i + 0] = static_cast<uint32_t>(i + 0);
        bulkIndices[i + 1] = static_cast<uint32_t>(i + 1);
        bulkIndices[i + 2] = static_cast<uint32_t>(i + 2);
    }

    constexpr std::array<NeoEngine::Vulkan3DVertex, 3> triangle{{
        NeoEngine::Vulkan3DVertex{-0.7F, -0.6F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.7F, -0.6F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F},
        NeoEngine::Vulkan3DVertex{ 0.0F,  0.7F, 0.0F, 0.0F, 0.0F, 1.0F, 0.5F, 1.0F},
    }};
    constexpr std::array<uint32_t, 3> triangleIndices{{0, 1, 2}};

    // Frame 1: growth + append path.
    if (!renderer.BeginFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin1 error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 2;
    }
    if (!renderer.DrawIndexed(bulkVertices, bulkIndices, identity.data()) ||
        !renderer.DrawIndexed(triangle, triangleIndices, identity.data())) {
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

    // Frames 2-4: prove the two per-frame persistent arenas can be reused after their
    // fences signal, including crossing the frame-slot boundary without reallocating
    // every draw. Keep the workload intentionally small here so this remains a smoke test.
    for (unsigned frame = 2; frame <= 4; ++frame) {
        if (!renderer.BeginFrame()) {
            std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin%u error=%u\n", frame,
                         static_cast<unsigned>(renderer.LastError()));
            return 6;
        }
        if (!renderer.DrawIndexed(triangle, triangleIndices, identity.data()) ||
            !renderer.EndFrame()) {
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

    std::printf("VULKAN3D_SMOKE_OK frames=%llu final_vertices=%u final_indices=%u size=%ux%u\n",
                static_cast<unsigned long long>(renderer.LastFrameStats().frameIndex),
                renderer.LastFrameStats().vertexCount, renderer.LastFrameStats().indexCount,
                renderer.LastFrameStats().width, renderer.LastFrameStats().height);
    return 0;
}
