#include <cassert>
#include "OcclusionCulling.h"

float OcclusionCulling::ProjectDepth(const AABB& box)
{
    float z = box.min[2];
    return z;
}

bool OcclusionCulling::IsOccluded(const AABB& box, const HiZBuffer& hiz)
{
    float depth = ProjectDepth(box);

    float sceneDepth = hiz.Sample(0,0,0);

    if(depth > sceneDepth)
        return true;

    return false;
}
