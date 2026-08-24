#pragma once

#include "Frustum.h"

struct AABB
{
    float min[3];
    float max[3];
};

class FrustumCulling
{
public:

    bool IsVisible(const Frustum& frustum, const AABB& box);
};
