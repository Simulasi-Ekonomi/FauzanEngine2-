#include "Core/ECS/EntityManager.h"
#include "ECS/AIComponent.h"
#include "ECS/V4/AISystem.h"
#include <cstdio>
#include <vector>

using namespace NeoEngine;

int main() {
    printf("============================================\n");
    printf(" FAUZANAI – Utility + Memory + Flocking\n");
    printf("============================================\n\n");

    EntityManager em;
    AISystem aiSystem;

    // --- Player setup ---
    aiSystem.playerX = 0;
    aiSystem.playerZ = 0;
    aiSystem.playerHealth = 1.0f;

    // --- Spawn NPC ---
    const int NUM_NPC = 6;
    std::vector<EntityID> npc(NUM_NPC);
    std::vector<AIComponent*> aiComps(NUM_NPC);

    for (int i = 0; i < NUM_NPC; ++i) {
        npc[i] = em.CreateEntity();
        em.positions.x[npc[i]] = (i - 2) * 6.0f;
        em.positions.y[npc[i]] = 0;
        em.positions.z[npc[i]] = (i % 3 - 1) * 6.0f;
        em.velocities.vx[npc[i]] = 0;
        em.velocities.vz[npc[i]] = 0;

        aiComps[i] = new AIComponent();
        aiSystem.Attach(npc[i], aiComps[i]);
    }

    float dt = 0.05f;

    // === Phase 1 ===
    printf("=== Phase 1: Player at (0,0) ===\n");
    aiSystem.playerX = 0; aiSystem.playerZ = 0;
    aiSystem.playerHealth = 1.0f;
    for (int f = 0; f < 40; ++f) aiSystem.Update(dt, em);

    // === Phase 2 ===
    printf("\n=== Phase 2: Player at (10,10), injured ===\n");
    aiSystem.playerX = 10; aiSystem.playerZ = 10;
    aiSystem.playerHealth = 0.2f;
    for (int f = 0; f < 40; ++f) aiSystem.Update(dt, em);

    // === Phase 3 ===
    printf("\n=== Phase 3: Player far (50,50) ===\n");
    aiSystem.playerX = 50; aiSystem.playerZ = 50;
    aiSystem.playerHealth = 1.0f;
    for (int f = 0; f < 40; ++f) aiSystem.Update(dt, em);

    printf("\nFinal positions:\n");
    for (int i = 0; i < NUM_NPC; ++i) {
        printf("NPC%d: (%.2f, %.2f)\n", i,
            em.positions.x[npc[i]], em.positions.z[npc[i]]);
        delete aiComps[i];
    }

    printf("\n============================================\n");
    printf(" ✅ Utility + Memory + Flocking ACTIVE\n");
    printf("============================================\n");
    return 0;
}
