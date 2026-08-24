#include "PhysicsWorld.h"
#include "CollisionSAT.h"

namespace NeoEngine {

void PhysicsWorld::AddBody(RigidBody* body, Collider* collider) {
    bodies.push_back(body);
    colliders.push_back(collider);
}

void PhysicsWorld::Step(float dt) {
    for (size_t i = 0; i < bodies.size(); i++) {
        for (size_t j = i + 1; j < bodies.size(); j++) {
            AABB a = colliders[i]->GetAABB();
            AABB b = colliders[j]->GetAABB();
            if (CollisionSAT::TestAABBvsAABB(a, b)) {
                auto result = CollisionSAT::ResolveAABBvsAABB(a, b);
                if (result.hit) {
                    if (!bodies[i]->isKinematic) {
                        bodies[i]->position.x -= result.normal.x * result.depth * 0.5f;
                        bodies[i]->position.y -= result.normal.y * result.depth * 0.5f;
                        bodies[i]->position.z -= result.normal.z * result.depth * 0.5f;
                    }
                    if (!bodies[j]->isKinematic) {
                        bodies[j]->position.x += result.normal.x * result.depth * 0.5f;
                        bodies[j]->position.y += result.normal.y * result.depth * 0.5f;
                        bodies[j]->position.z += result.normal.z * result.depth * 0.5f;
                    }
                }
            }
        }
    }
}

} // namespace NeoEngine
