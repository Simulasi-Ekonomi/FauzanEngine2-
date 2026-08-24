#include "Core/ECS/EntityManager.h"
#include "ECS/V4/AISystem.h"
#include "ECS/AIComponent.h"
#include "Physics/PhysicsSystem.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

using namespace NeoEngine;
using namespace std::chrono;

int main() {
    printf("FAUZANENGINE V4 - FINAL TUNED\n");

    EntityManager em;
    AISystem ai;
    PhysicsSystem phys;

    const int N = 500;
    std::vector<AIComponent*> comps(N);

    // SPAWN LEBIH MERATA
    for (int i = 0; i < N; ++i) {
        EntityID id = em.CreateEntity();
        em.positions.x[id] = (rand() % 50) - 25.0f;
        em.positions.z[id] = (rand() % 50) - 25.0f;
        em.colliders.radius[id] = 1.0f;
        em.colliders.invMass[id] = 1.0f;

        comps[i] = new AIComponent();
        ai.Attach(id, comps[i]);
    }

    const float DT = 0.016f;
    const float TOTAL_TIME = 5.0f;

    float simTime = 0;
    int frameCount = 0;
    int totalContacts = 0;

    auto start = high_resolution_clock::now();

    while (simTime < TOTAL_TIME) {
        ai.Update(DT, em);

        phys.Solve(em, DT);
        totalContacts += phys.GetContactCount();

        for (int i = 0; i < N; ++i) {
            if (!em.IsAlive(i)) continue;

            em.positions.x[i] += em.velocities.vx[i] * DT;
            em.positions.z[i] += em.velocities.vz[i] * DT;
        }

        simTime += DT;
        frameCount++;
    }

    auto end = high_resolution_clock::now();

    float elapsed = duration<float>(end - start).count();
    float fps = frameCount / elapsed;

    printf("FPS: %.1f\n", fps);
    printf("Contacts(avg): %d\n", totalContacts / frameCount);

    for (auto* c : comps) delete c;
}
