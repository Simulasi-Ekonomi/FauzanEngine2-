#pragma once

#include "HiZBuffer.h"
#include "FrustumCulling.h"

class OcclusionCulling
{
public:
    // Existing API preserved. Without a screen-space projection, this remains
    // a conservative single-sample query.
    bool IsOccluded(const AABB& box, const HiZBuffer& hiz);

    // R4 conservative reverse-Z Hi-Z query. The AABB is expected in NDC:
    // x/y in [-1, 1], z in [0, 1], with larger z meaning nearer.
    bool IsOccludedNDC(const AABB& box, const HiZBuffer& hiz);

private:
    float ProjectDepth(const AABB& box);
};
