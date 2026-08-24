#include <cassert>
#include "CollisionSAT.h"

namespace NeoEngine
{

bool CollisionSAT::TestAABB(const AABB& a,const AABB& b)
{
    if(a.max.x < b.min.x || a.min.x > b.max.x)
        return false;

    if(a.max.y < b.min.y || a.min.y > b.max.y)
        return false;

    if(a.max.z < b.min.z || a.min.z > b.max.z)
        return false;

    return true;
}

}
