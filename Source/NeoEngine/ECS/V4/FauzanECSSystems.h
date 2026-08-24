#pragma once
#include "FauzanECS.h"
#include <cmath>

namespace NeoEngine {

// Pola Processor: setiap system mewarisi IFauzanSystem
class IFauzanSystem {
public:
    virtual ~IFauzanSystem() = default;
    virtual void Update(float dt, FauzanEntityManager& em) = 0;
};

// System pergerakan: update posisi berdasarkan kecepatan
class MovementSystem : public IFauzanSystem {
public:
    void Update(float dt, FauzanEntityManager& em) override {
        auto entities = em.Query<PositionComponent, VelocityComponent>();
        for (EntityID id : entities) {
            if (id >= em.positions.x.size()) continue;
            em.positions.x[id] += em.velocities.vx[id] * dt;
            em.positions.y[id] += em.velocities.vy[id] * dt;
            em.positions.z[id] += em.velocities.vz[id] * dt;
        }
    }
};

// System collision sederhana (grid-based, untuk demonstrasi)
class CollisionSystem : public IFauzanSystem {
public:
    void Update(float dt, FauzanEntityManager& em) override {
        auto entities = em.Query<PositionComponent, CollisionComponent>();
        // Implementasi collision nanti saat diperlukan
    }
};

// System rendering (placeholder)
class RenderSystem : public IFauzanSystem {
public:
    void Update(float dt, FauzanEntityManager& em) override {
        auto entities = em.Query<PositionComponent, ModelComponent>();
        // Implementasi rendering nanti
    }
};

} // namespace NeoEngine
