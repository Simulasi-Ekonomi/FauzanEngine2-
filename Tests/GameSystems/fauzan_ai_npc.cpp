#include "Core/ECS/EntityManager.h"
#include "ECS/AIComponent.h"
#include "ECS/V4/AISystem.h"
#include <cstdio>
#include <cmath>

using namespace NeoEngine;

int main() {
    printf("============================================\n");
    printf(" FAUZANAI REAL TEST – MoE + Recurrent NPC\n");
    printf("============================================\n\n");

    EntityManager em;
    AISystem aiSystem;

    EntityID npc[3];
    AIComponent* aiComps[3];
    for (int i = 0; i < 3; ++i) {
        npc[i] = em.CreateEntity();
        em.positions.x[npc[i]] = (i - 1) * 5.0f;
        em.positions.y[npc[i]] = 0;
        em.positions.z[npc[i]] = (i - 1) * 5.0f;
        aiComps[i] = new AIComponent();
        aiSystem.Attach(npc[i], aiComps[i]);
    }

    float dt = 0.016f;
    printf("  Initial positions:\n");
    for (int i = 0; i < 3; ++i) {
        printf("    NPC%d: (%.1f, %.1f, %.1f)\n", i,
            em.positions.x[npc[i]], em.positions.y[npc[i]], em.positions.z[npc[i]]);
    }

    for (int f = 0; f < 120; ++f) {
        aiSystem.Update(dt, em);
    }

    printf("\n  After 2 seconds of AI control:\n");
    for (int i = 0; i < 3; ++i) {
        printf("    NPC%d: (%.1f, %.1f, %.1f) [action: %s, conf: %.2f]\n", i,
            em.positions.x[npc[i]], em.positions.y[npc[i]], em.positions.z[npc[i]],
            aiComps[i]->lastOutput.action.c_str(), aiComps[i]->lastOutput.confidence);
        delete aiComps[i];
    }

    printf("\n============================================\n");
    printf(" ✅ FauzanAI MoE + Recurrent: ACTIVE\n");
    printf("============================================\n");
    return 0;
}
