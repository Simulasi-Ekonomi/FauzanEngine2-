#include "Rendering/Rendering/Renderer/HiZBuffer.h"
#include "Rendering/Rendering/Renderer/OcclusionCulling.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
bool Require(bool condition, const char* message) {
    if (condition) return true;
    std::cerr << "hiz_occlusion_culling_smoke: " << message << '\n';
    return false;
}
}

int main() {
    HiZBuffer hiz(4, 4);
    if (!Require(hiz.GetLevels() == 3, "unexpected mip level count") ||
        !Require(hiz.GetWidth(1) == 2, "unexpected level-1 width") ||
        !Require(hiz.GetHeight(2) == 1, "unexpected level-2 height")) {
        return 1;
    }

    std::vector<float> depth(16, 0.0F);
    depth[2 * 4 + 2] = 0.8F;
    hiz.Build(depth);
    hiz.BuildOcclusion(depth);
    if (!Require(std::fabs(hiz.Sample(0, 2, 2) - 0.8F) < 1e-6F,
                 "depth sample does not preserve source value") ||
        !Require(std::fabs(hiz.SampleOcclusion(1, 1, 1)) < 1e-6F,
                 "occlusion pyramid produced unexpected depth")) {
        return 2;
    }

    OcclusionCulling culling;
    AABB occluded{};
    occluded.min[0] = 0.0F;
    occluded.max[0] = 0.0F;
    occluded.min[1] = 0.0F;
    occluded.max[1] = 0.0F;
    occluded.min[2] = 0.2F;
    occluded.max[2] = 0.5F;
    if (!Require(culling.IsOccludedNDC(occluded, hiz),
                 "fully occluded bounds were not rejected")) {
        return 3;
    }

    std::vector<float> invalidDepth(16, 0.0F);
    invalidDepth[7] = std::numeric_limits<float>::quiet_NaN();
    bool rejectedNonFinite = false;
    try {
        hiz.Build(invalidDepth);
    } catch (const std::invalid_argument&) {
        rejectedNonFinite = true;
    }
    if (!Require(rejectedNonFinite, "non-finite depth was accepted")) return 4;

    bool rejectedWrongSize = false;
    try {
        hiz.Build(std::vector<float>(15, 0.0F));
    } catch (const std::invalid_argument&) {
        rejectedWrongSize = true;
    }
    if (!Require(rejectedWrongSize, "mismatched depth-buffer size was accepted")) return 5;

    hiz.BuildOcclusion(std::vector<float>(16, 0.0F));
    if (!Require(!culling.IsOccludedNDC(occluded, hiz),
                 "empty/far depth incorrectly occluded bounds")) {
        return 6;
    }

    AABB invalid = occluded;
    invalid.min[0] = 2.0F;
    invalid.max[0] = 3.0F;
    if (!Require(!culling.IsOccludedNDC(invalid, hiz),
                 "out-of-frustum bounds were incorrectly occluded")) {
        return 7;
    }

    bool rejectedDimensions = false;
    try {
        HiZBuffer invalidBuffer(0, 4);
    } catch (const std::invalid_argument&) {
        rejectedDimensions = true;
    }
    if (!Require(rejectedDimensions, "non-positive dimensions were accepted")) return 8;

    std::cout << "Hi-Z occlusion smoke passed\n";
    return 0;
}
