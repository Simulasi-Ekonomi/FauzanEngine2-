#include <cassert>
#include "FrustumCulling.h"

bool FrustumCulling::IsVisible(const Frustum& frustum, const AABB& box)
{
    for(const Plane& p : frustum.planes)
    {
        float x = p.a > 0 ? box.max[0] : box.min[0];
        float y = p.b > 0 ? box.max[1] : box.min[1];
        float z = p.c > 0 ? box.max[2] : box.min[2];

        if(p.a*x + p.b*y + p.c*z + p.d < 0)
            return false;
    }

    return true;
}
