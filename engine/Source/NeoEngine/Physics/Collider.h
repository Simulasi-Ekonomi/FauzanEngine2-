#pragma once

#include "RigidBody.h"

namespace NeoEngine
{

struct AABB
{
    Vec3 min;
    Vec3 max;
};

class Collider
{
public:

    Collider(RigidBody* body);

    AABB GetAABB() const;

private:

    RigidBody* body;

    Vec3 halfSize;

};

}
