#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Core/ECS/EntityManager.h"
#include <iostream>
#include <chrono>

using namespace NeoEngine;

int main() {
    EntityManager em;   // tanpa parameter (konstruktor default)
    const int N = 10000;
    std::vector<EntityID> ids(N);
    for (int i = 0; i < N; ++i) {
        ids[i] = em.CreateEntity();   // gunakan CreateEntity yang ada
    }
    // Isi posisi & velocity dengan area sempit (-4..4) dan kecepatan random
    for (int i = 0; i < N; ++i) {
        EntityID id = ids[i];
        em.positions.x[id] = (float)((rand() % 8000) - 4000) / 1000.0f;
        em.positions.z[id] = (float)((rand() % 8000) - 4000) / 1000.0f;
        em.velocities.vx[id] = (float)((rand() % 200) - 100) / 100.0f;
        em.velocities.vz[id] = (float)((rand() % 200) - 100) / 100.0f;
        em.colliders.radius[id] = 0.2f;
        em.colliders.invMass[id] = 1.0f;
    }

    XPBDPhysicsSystem phys;
    auto t0 = std::chrono::high_resolution_clock::now();
    const int frames = 50;
    for (int f = 0; f < frames; ++f) {
        phys.Step(em, 1.0f / 60.0f);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    float ms = std::chrono::duration<float, std::milli>(t1 - t0).count() / frames;
    std::cout << "V5 XPBD 2-Phase " << N << " entities: " << ms << " ms/frame\n";
    std::cout << "Manifolds: " << phys.GetManifoldCount() << "\n";
    return 0;
}
