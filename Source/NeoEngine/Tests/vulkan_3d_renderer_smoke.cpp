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

    // Deliberately exceed the old 256 KiB initial arena to exercise one-time capacity
    // growth, then issue a second draw in the same frame to exercise persistent append.
    constexpr size_t bulkVertexCount = 9000;
    constexpr size_t bulkIndexCount = 9000;
    std::vector<NeoEngine::Vulkan3DVertex> bulkVertices(bulkVertexCount);
    std::vector<uint32_t> bulkIndices(bulkIndexCount);
    for (size_t i = 0; i < bulkVertexCount; ++i) {
        bulkVertices[i] = {{0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}};
    }
    for (size_t i = 0; i < bulkIndexCount; i += 3) {
        bulkIndices[i + 0] = static_cast<uint32_t>(i + 0);
        bulkIndices[i + 1] = static_cast<uint32_t>(i + 1);
        bulkIndices[i + 2] = static_cast<uint32_t>(i + 2);
    }

    constexpr std::array<NeoEngine::Vulkan3DVertex, 3> triangle{{
        {{-0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{ 0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{ 0.0F,  0.7F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 1.0F}},
    }};
    constexpr std::array<uint32_t, 3> triangleIndices{{0, 1, 2}};

    if (!renderer.BeginFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 2;
    }
    if (!renderer.DrawIndexed(bulkVertices, bulkIndices, identity.data())) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL bulk_draw error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 3;
    }
    if (!renderer.DrawIndexed(triangle, triangleIndices, identity.data())) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL append_draw error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 4;
    }
    if (!renderer.EndFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL end error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 5;
    }

    const auto& stats = renderer.LastFrameStats();
    if (stats.frameIndex != 1 || stats.vertexCount != bulkVertexCount + 3 ||
        stats.indexCount != bulkIndexCount + 3) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL stats frame=%llu vertices=%u indices=%u\n",
                     static_cast<unsigned long long>(stats.frameIndex), stats.vertexCount, stats.indexCount);
        return 6;
    }

    std::printf("VULKAN3D_SMOKE_OK frame=%llu vertices=%u indices=%u size=%ux%u\n",
                static_cast<unsigned long long>(stats.frameIndex), stats.vertexCount, stats.indexCount,
                stats.width, stats.height);
    return 0;
}
