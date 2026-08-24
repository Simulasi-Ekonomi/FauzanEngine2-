#pragma once

#include <vector>
#include "RigidBody.h"
#include "Collider.h"
#include "BVH.h"

namespace NeoEngine
{

class PhysicsWorld
{
public:

    void AddBody(RigidBody* body,Collider* collider);

    void Step(float dt);

private:

    [[maybe_unused]] std::vector<RigidBody*> bodies;
    [[maybe_unused]] std::vector<Collider*> colliders;

    BVH broadphase;

};

}
