#include "DestructionPhysicsSolver.h"

void DestructionPhysicsSolver::AddBody(const RigidBody& body)
{
    bodies.push_back(body);
}

void DestructionPhysicsSolver::Step(float dt)
{
    for(auto& b : bodies)
    {
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.z += b.vz * dt;
    }
}
