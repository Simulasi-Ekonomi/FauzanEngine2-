#include "../Rendering/Rendering/Renderer/HiZBuffer.h"
#include "../Rendering/Rendering/Renderer/OcclusionCulling.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

int main() {
    // Reverse-Z convention: clear/far = 0, nearer surfaces have larger depth.
    std::vector<float> depth(64, 0.0F);
    for (int y = 2; y < 6; ++y) {
        for (int x = 2; x < 6; ++x) depth[static_cast<std::size_t>(y * 8 + x)] = 0.8F;
    }

    HiZBuffer hiz(8, 8);
    hiz.Build(depth);
    hiz.BuildOcclusion(depth);
    assert(hiz.GetLevels() == 4);
    assert(std::fabs(hiz.Sample(0, 3, 3) - 0.8F) < 1e-6F);
    assert(std::fabs(hiz.Sample(1, 1, 1) - 0.8F) < 1e-6F);
    assert(std::fabs(hiz.SampleOcclusion(0, 3, 3) - 0.8F) < 1e-6F);
    // Level 1 cell (1,1) covers source pixels [2..3] x [2..3], all occupied
    // by the 0.8 reverse-Z blocker, so MIN reduction is also 0.8.
    assert(std::fabs(hiz.SampleOcclusion(1, 1, 1) - 0.8F) < 1e-6F);

    OcclusionCulling occlusion;

    // Align the test occludee to the covered 2x2 Hi-Z cell. This verifies a
    // definite occlusion case without relying on mip-cell boundary overlap.
    AABB behind{};
    behind.min[0] = -0.5F; behind.max[0] = 0.0F;
    behind.min[1] = -0.5F; behind.max[1] = 0.0F;
    behind.min[2] = 0.2F;  behind.max[2] = 0.3F;
    assert(occlusion.IsOccludedNDC(behind, hiz));

    AABB inFront = behind;
    inFront.min[2] = 0.9F; inFront.max[2] = 0.95F;
    assert(!occlusion.IsOccludedNDC(inFront, hiz));

    AABB partial = behind;
    partial.min[0] = -1.0F; partial.max[0] = 0.0F;
    assert(!occlusion.IsOccludedNDC(partial, hiz));

    AABB invalid{};
    invalid.min[0] = 1.0F; invalid.max[0] = -1.0F;
    assert(!occlusion.IsOccludedNDC(invalid, hiz));

    std::cout << "R4_HIZ_OCCLUSION_OK levels=" << hiz.GetLevels() << '\n';
    return 0;
}
