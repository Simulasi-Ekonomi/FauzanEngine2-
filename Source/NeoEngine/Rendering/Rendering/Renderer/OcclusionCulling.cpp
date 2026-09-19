#include "OcclusionCulling.h"

#include <algorithm>
#include <cmath>

float OcclusionCulling::ProjectDepth(const AABB& box) const {
    // Reversed-Z: larger NDC Z is closer to the camera. The closest point
    // of the box is therefore its maximum Z.
    return box.max[2];
}

bool OcclusionCulling::IsOccluded(const AABB& box, const HiZBuffer& hiz) {
    return IsOccludedNDC(box, hiz);
}

bool OcclusionCulling::IsOccludedNDC(const AABB& box, const HiZBuffer& hiz) const {
    if (!std::isfinite(box.min[0]) || !std::isfinite(box.max[0]) ||
        !std::isfinite(box.min[1]) || !std::isfinite(box.max[1]) ||
        !std::isfinite(box.min[2]) || !std::isfinite(box.max[2]) ||
        box.min[0] > box.max[0] || box.min[1] > box.max[1] ||
        box.min[2] > box.max[2]) {
        return false;
    }

    if (box.max[0] < -1.0F || box.min[0] > 1.0F ||
        box.max[1] < -1.0F || box.min[1] > 1.0F) {
        return false;
    }

    const int width = hiz.GetWidth(0);
    const int height = hiz.GetHeight(0);
    if (width <= 0 || height <= 0) return false;

    const float minX = std::max(-1.0F, box.min[0]);
    const float maxX = std::min(1.0F, box.max[0]);
    const float minY = std::max(-1.0F, box.min[1]);
    const float maxY = std::min(1.0F, box.max[1]);

    // NDC edge-to-texel mapping. The maximum edge is inclusive and clamped.
    const int x0 = std::clamp(
        static_cast<int>(std::floor((minX + 1.0F) * 0.5F * width)), 0, width - 1);
    const int x1 = std::clamp(
        static_cast<int>(std::floor((maxX + 1.0F) * 0.5F * width)), 0, width - 1);
    const int y0 = std::clamp(
        static_cast<int>(std::floor((minY + 1.0F) * 0.5F * height)), 0, height - 1);
    const int y1 = std::clamp(
        static_cast<int>(std::floor((maxY + 1.0F) * 0.5F * height)), 0, height - 1);

    if (x0 > x1 || y0 > y1) return false;

    // A level-0 scan is intentionally conservative and exact. Higher-level
    // mip selection can be layered on without changing the visibility contract.
    const float occluderDepth = [&]() {
        float value = 1.0F;
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                value = std::min(value, hiz.SampleOcclusion(0, x, y));
            }
        }
        return value;
    }();

    // If the closest point is not behind every covered occluder, visibility
    // must be retained. Empty/far depth (0 in reversed-Z) also prevents culling.
    return ProjectDepth(box) < occluderDepth;
}