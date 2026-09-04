#include "../Runtime/Vulkan3DRenderer.h"

#include <array>
#include <cstdio>

int main() {
    NeoEngine::Vulkan3DRenderer renderer;
    if (!renderer.Initialize(640, 480, "NeoEngine Vulkan3D Smoke")) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL init error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 1;
    }

    constexpr std::array<NeoEngine::Vulkan3DVertex, 3> vertices{{
        {{-0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F}},
        {{ 0.7F, -0.6F, 0.0F}, {0.0F, 0.0F, 1.0F}, {1.0F, 0.0F}},
        {{ 0.0F,  0.7F, 0.0F}, {0.0F, 0.0F, 1.0F}, {0.5F, 1.0F}},
    }};
    constexpr std::array<uint32_t, 3> indices{{0, 1, 2}};
    constexpr std::array<float, 16> identity{{
        1.0F, 0.0F, 0.0F, 0.0F,
        0.0F, 1.0F, 0.0F, 0.0F,
        0.0F, 0.0F, 1.0F, 0.0F,
        0.0F, 0.0F, 0.0F, 1.0F,
    }};

    if (!renderer.BeginFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL begin error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 2;
    }
    if (!renderer.DrawIndexed(vertices, indices, identity.data())) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL draw error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 3;
    }
    if (!renderer.EndFrame()) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL end error=%u\n", static_cast<unsigned>(renderer.LastError()));
        return 4;
    }

    const auto& stats = renderer.LastFrameStats();
    if (stats.frameIndex != 1 || stats.vertexCount != 3 || stats.indexCount != 3) {
        std::fprintf(stderr, "VULKAN3D_SMOKE_FAIL stats frame=%llu vertices=%u indices=%u\n",
                     static_cast<unsigned long long>(stats.frameIndex), stats.vertexCount, stats.indexCount);
        return 5;
    }

    std::printf("VULKAN3D_SMOKE_OK frame=%llu vertices=%u indices=%u size=%ux%u\n",
                static_cast<unsigned long long>(stats.frameIndex), stats.vertexCount, stats.indexCount,
                stats.width, stats.height);
    return 0;
}
