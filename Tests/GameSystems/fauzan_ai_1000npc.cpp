#include "Core/ECS/EntityManager.h"
#include "ECS/AIComponent.h"
#include "ECS/V4/AISystem.h"
#include <chrono>
#include <cstdio>
#include <vector>

using namespace NeoEngine;
using namespace std::chrono;

int main() {
    printf("============================================\n");
    printf(" FAUZANAI – Spatial Grid + 1000 NPC\n");
    printf("============================================\n\n");

    EntityManager em;
    AISystem aiSystem;

    aiSystem.playerX = 0; aiSystem.playerZ = 0;
    aiSystem.playerHealth = 1.0f;

    const int NUM_NPC = 1000;
    std::vector<EntityID> npc(NUM_NPC);
    std::vector<AIComponent*> aiComps(NUM_NPC);

    // Spawn 1000 NPC
    for (int i = 0; i < NUM_NPC; ++i) {
        npc[i] = em.CreateEntity();
        em.positions.x[npc[i]] = (i % 50 - 25) * 5.0f;
        em.positions.y[npc[i]] = 0;
        em.positions.z[npc[i]] = (i / 50 - 10) * 5.0f;

        aiComps[i] = new AIComponent();
        aiSystem.Attach(npc[i], aiComps[i]);
    }

    printf("Spawned %d NPCs.\n", NUM_NPC);
    printf("Running 60 frames...\n\n");

    float dt = 0.05f;
    auto t0 = high_resolution_clock::now();
    for (int f = 0; f < 60; ++f) aiSystem.Update(dt, em);
    auto t1 = high_resolution_clock::now();

    float totalMs = duration<float, std::milli>(t1 - t0).count();
    float frameTime = totalMs / 60.0f;

    printf("Total time  : %.2f ms\n", totalMs);
    printf("Frame time  : %.2f ms\n", frameTime);
    printf("FPS (AI)    : %.1f\n", 1000.0f / frameTime);

    for (int i = 0; i < NUM_NPC; ++i) delete aiComps[i];

    printf("\n============================================\n");
    if (frameTime < 3.0f) printf(" ✅ 1000 NPC AI – SCALABLE\n");
    else printf(" ⚠️ Perlu optimasi lebih lanjut\n");
    printf("============================================\n");
    return 0;
}
