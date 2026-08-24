#pragma once

#include "HiZBuffer.h"
#include "FrustumCulling.h"

class OcclusionCulling
{
public:

    bool IsOccluded(const AABB& box, const HiZBuffer& hiz);

private:

    float ProjectDepth(const AABB& box);
};
