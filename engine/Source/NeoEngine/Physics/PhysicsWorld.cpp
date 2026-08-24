#include <cassert>
#include "PhysicsWorld.h"
#include "CollisionSAT.h"

namespace NeoEngine
{

void PhysicsWorld::AddBody(RigidBody* body,Collider* collider)
{
    bodies.push_back(body);
    colliders.push_back(collider);
}

void PhysicsWorld::Step(float dt)
{
    for(auto body : bodies)
        body->Integrate(dt);

    broadphase.Build(colliders);

    for(auto& pair : broadphase.GetPairs())
    {
        AABB a = pair.first->GetAABB();
        AABB b = pair.second->GetAABB();

        if(CollisionSAT::TestAABB(a,b))
        {
            // collision detected
        }
    }
}

}
