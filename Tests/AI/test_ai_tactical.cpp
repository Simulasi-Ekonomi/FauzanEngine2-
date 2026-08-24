#include "AI/V4/AITacticalSystem.h"
#include "Core/ECS/EntityManager.h"
#include <iostream>
#include <chrono>
#include <cstdlib>

using namespace NeoEngine;

int main() {
    EntityManager em;
    const int N = 5000;

    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity();
        em.positions.x[id] = (rand() % 8000 - 4000) / 1000.0f;
        em.positions.z[id] = (rand() % 8000 - 4000) / 1000.0f;
        em.velocities.vx[id] = (rand() % 200 - 100) / 100.0f;
        em.velocities.vz[id] = (rand() % 200 - 100) / 100.0f;
        em.colliders.radius[id] = 0.2f;
        em.colliders.invMass[id] = 1.0f;
    }

    AITacticalSystem aiSystem;
    aiSystem.Init(N + 5000);

    const int frames = 50;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int f = 0; f < frames; ++f) {
        aiSystem.Execute(em, 1.0f / 60.0f);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(t1 - t0).count() / frames;

    std::cout << "AI Tactical " << N << " NPC: " << ms << " ms/frame\n";
    return 0;
}
