#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Threading/JobSystem.h"
#include <chrono>
#include <random>
#include <cstdio>
#include <memory>

using namespace NeoEngine;

int main() {
    printf("FauzanEngine V5.10 parallel benchmark (8 workers)\n");
    
    printf("Initialize 8 workers...\n");
    JobSystem::Get().Initialize(8);

    printf("Create ECS & Physics...\n");
    ArchetypeManager em;
    auto phys = std::make_unique<XPBDPhysicsSystem>();

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> pd(-50,50);
    std::uniform_real_distribution<float> vd(-2,2);
    std::uniform_real_distribution<float> rd(0.3f, 1.5f);
    const uint32_t f = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    printf("Create 20000 entities...\n");
    for (int i=0; i<20000; ++i) {
        EntityID id = em.CreateEntity(f);
        auto* p = em.GetPosition(id);
        auto* v = em.GetVelocity(id);
        auto* c = em.GetCollider(id);
        p->x = pd(rng); p->z = pd(rng);
        v->vx = vd(rng); v->vz = vd(rng);
        c->radius = rd(rng);
        c->invMass = 1.0f / (c->radius * 10.0f);
    }

    printf("Warmup 30 frames...\n");
    for (int i=0; i<30; ++i) phys->Step(em, 0.016f);

    printf("Benchmark 50 frames...\n");
    constexpr int K = 50;
    double sum = 0;
    size_t cnt = 0;
    for (int i=0; i<K; ++i) {
        auto t1 = std::chrono::steady_clock::now();
        phys->Step(em, 0.016f);
        auto t2 = std::chrono::steady_clock::now();
        sum += std::chrono::duration<double, std::milli>(t2-t1).count();
        cnt += phys->GetManifoldCount();
    }
    double avg = sum / K;
    printf("20K (%d entities):  avg %5.2f ms  contacts %zu  fps %.1f\n",
           20000, avg, cnt/K, 1000.0/avg);

    JobSystem::Get().Shutdown();
    return 0;
}
