#include <cassert>
#include "Collider.h"

namespace NeoEngine
{

Collider::Collider(RigidBody* b)
{
    body = b;
    halfSize = {0.5f,0.5f,0.5f};
}

AABB Collider::GetAABB() const
{
    Vec3 pos = body->GetPosition();

    AABB box;

    box.min =
    {
        pos.x - halfSize.x,
        pos.y - halfSize.y,
        pos.z - halfSize.z
    };

    box.max =
    {
        pos.x + halfSize.x,
        pos.y + halfSize.y,
        pos.z + halfSize.z
    };

    return box;
}

}
