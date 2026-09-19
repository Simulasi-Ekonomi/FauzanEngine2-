#pragma once

#include "HiZBuffer.h"

struct AABB {
    float min[3]{};
    float max[3]{};
};

class OcclusionCulling {
public:
    bool IsOccluded(const AABB& box, const HiZBuffer& hiz);
    bool IsOccludedNDC(const AABB& box, const HiZBuffer& hiz) const;

private:
    float ProjectDepth(const AABB& box) const;
};