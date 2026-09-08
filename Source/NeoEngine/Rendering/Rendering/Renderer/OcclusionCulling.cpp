#include "OcclusionCulling.h"

#include <algorithm>
#include <cmath>

float OcclusionCulling::ProjectDepth(const AABB& box)
{
    // Reverse-Z: larger depth is nearer, so the nearest point of the box is
    // its maximum Z. This is the conservative depth used for rejection.
    return std::max(box.min[2], box.max[2]);
}

bool OcclusionCulling::IsOccluded(const AABB& box, const HiZBuffer& hiz)
{
    if (hiz.GetLevels() <= 0) return false;
    const float depth = ProjectDepth(box);
    if (!std::isfinite(depth)) return false;

    // Preserve the legacy API, but use the conservative reverse-Z pyramid at
    // its base sample. This can only reject when the sample proves the object
    // is behind a nearer surface.
    const float sceneDepth = hiz.SampleOcclusion(0, 0, 0);
    return std::isfinite(sceneDepth) && depth < sceneDepth;
}

bool OcclusionCulling::IsOccludedNDC(const AABB& box, const HiZBuffer& hiz)
{
    if (hiz.GetLevels() <= 0) return false;
    if (!std::isfinite(box.min[0]) || !std::isfinite(box.min[1]) ||
        !std::isfinite(box.min[2]) || !std::isfinite(box.max[0]) ||
        !std::isfinite(box.max[1]) || !std::isfinite(box.max[2])) return false;
    if (box.min[0] > box.max[0] || box.min[1] > box.max[1] || box.min[2] > box.max[2]) return false;

    const int baseWidth = hiz.GetWidth(0);
    const int baseHeight = hiz.GetHeight(0);
    if (baseWidth <= 0 || baseHeight <= 0) return false;

    const float minX = std::clamp(box.min[0], -1.0F, 1.0F);
    const float maxX = std::clamp(box.max[0], -1.0F, 1.0F);
    const float minY = std::clamp(box.min[1], -1.0F, 1.0F);
    const float maxY = std::clamp(box.max[1], -1.0F, 1.0F);
    if (minX > maxX || minY > maxY) return false;

    const float objectNearestDepth = ProjectDepth(box);
    if (objectNearestDepth <= 0.0F) return false;

    // Choose the coarsest level whose texel footprint is no larger than the
    // projected rectangle. The MIN pyramid is then safe for reverse-Z: every
    // source sample contributing to a texel must be nearer than the object.
    const float rectWidth = std::max(1.0F, (maxX - minX) * 0.5F * baseWidth);
    const float rectHeight = std::max(1.0F, (maxY - minY) * 0.5F * baseHeight);
    int level = 0;
    while (level + 1 < hiz.GetLevels() &&
           static_cast<float>(hiz.GetWidth(level + 1)) >= 1.0F &&
           static_cast<float>(hiz.GetHeight(level + 1)) >= 1.0F &&
           rectWidth >= static_cast<float>(1 << (level + 1)) &&
           rectHeight >= static_cast<float>(1 << (level + 1))) {
        ++level;
    }

    const int w = hiz.GetWidth(level);
    const int h = hiz.GetHeight(level);
    const int x0 = std::clamp(static_cast<int>(std::floor((minX * 0.5F + 0.5F) * w)), 0, w - 1);
    const int x1 = std::clamp(static_cast<int>(std::ceil ((maxX * 0.5F + 0.5F) * w)) - 1, 0, w - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor((minY * 0.5F + 0.5F) * h)), 0, h - 1);
    const int y1 = std::clamp(static_cast<int>(std::ceil ((maxY * 0.5F + 0.5F) * h)) - 1, 0, h - 1);

    if (x0 > x1 || y0 > y1) return false;
    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const float blockerDepth = hiz.SampleOcclusion(level, x, y);
            if (!std::isfinite(blockerDepth) || objectNearestDepth >= blockerDepth) return false;
        }
    }
    return true;
}
