#include "Core/Math/Vector3.h"
#include "ConstraintSolver.h"

namespace NeoEngine {

void ConstraintSolver::Resolve(RigidBody& a, RigidBody& b) {
    Vec3 posA = a.GetPosition();
    Vec3 posB = b.GetPosition();
    Vec3 diff = {posB.x - posA.x, posB.y - posA.y, posB.z - posA.z};
    float dist = diff.Length();
    if (dist < 0.001f) return; // hindari pembagian nol

    float penetration = 0.5f - dist; // asumsikan radius 0.5
    if (penetration <= 0) return;

    float correction = penetration / dist;
    float dx = diff.x * correction;
    float dy = diff.y * correction;
    float dz = diff.z * correction;

    a.SetPosition(Vec3(posA.x - dx, posA.y - dy, posA.z - dz));
    b.SetPosition(Vec3(posB.x + dx, posB.y + dy, posB.z + dz));
}

} // namespace NeoEngine
