#pragma once
#include <vector>
#include "../EntityManager.h"
#include "../../Core/Math/Vector3.h"
#include "../../Physics/CollisionSAT.h"

namespace NeoEngine {

struct RigidBody { float mass = 1.0f; bool useGravity = true; bool isKinematic = false; };
struct Collider { AABB bounds; bool isTrigger = false; };

class PhysicsSystem {
public:
    void Update(EntityManager& em, float dt) {
        auto entities = em.GetEntitiesWith<Position, Velocity, RigidBody, Collider>();
        for (auto e : entities) {
            auto& vel = em.GetComponent<Velocity>(e);
            auto& rb = em.GetComponent<RigidBody>(e);
            if (!rb.isKinematic && rb.useGravity) {
                vel.vy -= 9.81f * dt;
            }
        }
        // Collision detection
        auto& allEntities = em.GetAllEntities();
        for (size_t i = 0; i < allEntities.size(); i++) {
            if (!em.HasComponent<Collider>(allEntities[i])) continue;
            for (size_t j = i + 1; j < allEntities.size(); j++) {
                if (!em.HasComponent<Collider>(allEntities[j])) continue;
                auto& colA = em.GetComponent<Collider>(allEntities[i]);
                auto& colB = em.GetComponent<Collider>(allEntities[j]);
                if (colA.isTrigger || colB.isTrigger) {
                    if (CollisionSAT::TestAABBvsAABB(colA.bounds, colB.bounds)) {
                        em.TriggerCollision(allEntities[i], allEntities[j]);
                    }
                }
            }
        }
    }

    void ResolveCollision(EntityManager& em, EntityID a, EntityID b) {
        auto& colA = em.GetComponent<Collider>(a);
        auto& colB = em.GetComponent<Collider>(b);
        auto result = CollisionSAT::ResolveAABBvsAABB(colA.bounds, colB.bounds);
        if (result.hit) {
            if (em.HasComponent<Position>(a)) {
                auto& pos = em.GetComponent<Position>(a);
                pos.x -= result.normal.x * result.depth * 0.5f;
                pos.y -= result.normal.y * result.depth * 0.5f;
                pos.z -= result.normal.z * result.depth * 0.5f;
            }
            if (em.HasComponent<Position>(b)) {
                auto& pos = em.GetComponent<Position>(b);
                pos.x += result.normal.x * result.depth * 0.5f;
                pos.y += result.normal.y * result.depth * 0.5f;
                pos.z += result.normal.z * result.depth * 0.5f;
            }
        }
    }
};

} // namespace NeoEngine
