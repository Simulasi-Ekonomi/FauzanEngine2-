#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <cstdio>
#include <memory>

using namespace NeoEngine;

int main() {
    printf("FauzanEngine V5.10 tight benchmark (8 workers)\n");
    printf("Init 8 workers...\n");
    JobSystem::Get().Initialize(8);

    printf("Create ECS & Physics...\n");
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();
    const uint32_t f = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    // Buat 20000 entity yang rapat dan pasti bertabrakan
    printf("Create 20000 entities (tight pack)...\n");
    for (int i = 0; i < 20000; ++i) {
        EntityID id = em.CreateEntity(f);
        auto* p = em.GetPosition(id);
        auto* v = em.GetVelocity(id);
        auto* c = em.GetCollider(id);
        // Atur posisi dalam grid rapat (jarak 0.5 antar entity)
        p->x = (i % 100) * 0.5f;   // 100 kolom, jarak 0.5
        p->z = (i / 100) * 0.5f;    // 200 baris, jarak 0.5
        v->vx = 0.0f; v->vz = 0.0f;
        c->radius = 0.5f;            // radius besar → pasti overlap
        c->invMass = 1.0f;
    }

    printf("Warmup 30 frames...\n");
    for (int i = 0; i < 30; ++i) {
        phys->Step(em, 0.016f);
        if (i == 0) printf("  frame 0 contacts: %zu\n", phys->GetManifoldCount());
    }

    printf("Benchmark 50 frames...\n");
    constexpr int K = 50;
    double sum = 0;
    size_t cnt = 0;
    for (int i = 0; i < K; ++i) {
        auto t1 = std::chrono::steady_clock::now();
        phys->Step(em, 0.016f);
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration<double, std::milli>(t2-t1).count();
        cnt += phys->GetManifoldCount();
    }
    double avg = sum / K;
    printf("20K tight:  avg %.2f ms  contacts %zu  fps %.1f\n",
           avg, cnt/K, 1000.0/avg);

    JobSystem::Get().Shutdown();
    return 0;
}
