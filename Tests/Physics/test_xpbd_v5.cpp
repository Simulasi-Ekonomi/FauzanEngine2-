#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Core/ECS/ArchetypeManager.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace NeoEngine;

int main() {
    ArchetypeManager em;
    constexpr int N = 10000;
    constexpr uint32_t flags = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    std::mt19937 rng(42U);
    std::uniform_real_distribution<float> position(-4.0F, 4.0F);
    std::uniform_real_distribution<float> velocity(-1.0F, 1.0F);

    for (int i = 0; i < N; ++i) {
        const EntityID id = em.CreateEntity(flags);
        em.SetPosX(id, position(rng));
        em.SetPosZ(id, position(rng));
        em.SetVelX(id, velocity(rng));
        em.SetVelZ(id, velocity(rng));
        em.SetRadius(id, 0.2F);
        em.SetInvMass(id, 1.0F);
    }

    XPBDPhysicsSystem phys;
    const auto t0 = std::chrono::high_resolution_clock::now();
    constexpr int frames = 50;
    for (int f = 0; f < frames; ++f) {
        phys.Step(em, 1.0F / 60.0F);
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const float ms = std::chrono::duration<float, std::milli>(t1 - t0).count() / frames;

    std::cout << "V5 XPBD 2-Phase " << N << " entities: " << ms << " ms/frame\n";
    std::cout << "Manifolds: " << phys.GetManifoldCount() << "\n";
    return 0;
}
