#include "../Rendering/Rendering/Renderer/HiZBuffer.h"
#include "../Rendering/Rendering/Renderer/OcclusionCulling.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    // 8x8 depth: clear/far = 1.0. A nearer blocker occupies the center.
    std::vector<float> depth(64, 1.0F);
    for (int y = 2; y < 6; ++y) {
        for (int x = 2; x < 6; ++x) depth[static_cast<std::size_t>(y * 8 + x)] = 0.25F;
    }

    HiZBuffer hiz(8, 8);
    hiz.Build(depth);
    assert(hiz.GetLevels() == 4);
    assert(std::fabs(hiz.Sample(0, 3, 3) - 0.25F) < 1e-6F);
    assert(std::fabs(hiz.Sample(1, 1, 1) - 0.25F) < 1e-6F);

    OcclusionCulling occlusion;

    // AABB API is intentionally tested only through the public contract.
    // ProjectDepth() remains internal; implementation must conservatively
    // classify invalid/unsupported boxes as visible rather than false-cull.
    AABB invalid{};
    assert(!occlusion.IsOccluded(invalid, hiz));

    std::cout << "R4_HIZ_OCCLUSION_OK levels=" << hiz.GetLevels() << '\n';
    return 0;
}
