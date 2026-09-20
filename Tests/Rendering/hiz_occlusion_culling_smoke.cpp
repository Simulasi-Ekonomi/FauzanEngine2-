#include "Rendering/Rendering/Renderer/HiZBuffer.h"
#include "Rendering/Rendering/Renderer/OcclusionCulling.h"

#include <cassert>
#include <cmath>
#include <vector>

int main() {
    HiZBuffer hiz(4, 4);
    assert(hiz.GetLevels() == 3);
    assert(hiz.GetWidth(1) == 2);
    assert(hiz.GetHeight(2) == 1);

    std::vector<float> depth(16, 0.0F);
    depth[2 * 4 + 2] = 0.8F;
    hiz.Build(depth);
    hiz.BuildOcclusion(depth);

    assert(std::fabs(hiz.Sample(0, 2, 2) - 0.8F) < 1e-6F);
    assert(std::fabs(hiz.SampleOcclusion(1, 1, 1) - 0.0F) < 1e-6F);

    OcclusionCulling culling;
    AABB occluded{};
    occluded.min[0] = 0.0F; occluded.max[0] = 0.0F;
    occluded.min[1] = 0.0F; occluded.max[1] = 0.0F;
    occluded.min[2] = 0.2F; occluded.max[2] = 0.5F;
    assert(culling.IsOccludedNDC(occluded, hiz));

    std::vector<float> emptyDepth(16, 0.0F);
    hiz.BuildOcclusion(emptyDepth);
    assert(!culling.IsOccludedNDC(occluded, hiz));

    AABB invalid = occluded;
    invalid.min[0] = 2.0F;
    invalid.max[0] = 3.0F;
    assert(!culling.IsOccludedNDC(invalid, hiz));

    return 0;
}

// Input validation contract: HiZ must reject non-finite depth samples.
