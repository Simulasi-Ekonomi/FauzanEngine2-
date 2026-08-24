#pragma once

#include "Collider.h"

namespace NeoEngine
{

class CollisionSAT
{
public:

    static bool TestAABB(const AABB& a,const AABB& b);

};

}
