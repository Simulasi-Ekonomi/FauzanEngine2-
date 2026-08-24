#include <cassert>
#include "ConstraintSolver.h"

namespace NeoEngine
{

void ConstraintSolver::Resolve(RigidBody& a, RigidBody& b)
{
    auto posA = a.GetPosition();
    auto posB = b.GetPosition();

    float dx = posB.x - posA.x;
    float dy = posB.y - posA.y;
    float dz = posB.z - posA.z;

    constexpr float correction = 0.01f;

    a.SetPosition(posA.x - dx*correction,
                  posA.y - dy*correction,
                  posA.z - dz*correction);

    b.SetPosition(posB.x + dx*correction,
                  posB.y + dy*correction,
                  posB.z + dz*correction);
}

} // namespace NeoEngine
