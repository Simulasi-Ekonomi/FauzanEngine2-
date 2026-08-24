#pragma once
#include "../ECS/Systems.hpp"
#include "../ECS/ParallelScheduler.hpp"

namespace Neo {
    struct Position { float x, y, z; };
    struct Velocity { float vx, vy, vz; };

    class MovementSystem : public System<MovementSystem> {
        ParallelScheduler scheduler;
    public:
        void updateImpl(Registry& reg, float dt) {
            auto& posStore = reg.getStore<Position>();
            auto& velStore = reg.getStore<Velocity>();
            
            auto& posData = posStore.raw();
            auto& velData = velStore.raw();
            size_t count = posData.size();

            // Eksekusi paralel: Membagi beban ke semua Core CPU
            scheduler.parallel_for(count, [&](size_t i) {
                posData[i].x += velData[i].vx * dt;
                posData[i].y += velData[i].vy * dt;
                posData[i].z += velData[i].vz * dt;
            });
        }
    };
}
