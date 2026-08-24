#include "Source/NeoEngine/Physics/V5/XPBDPhysicsSystem.h"
#include "Source/NeoEngine/Core/ECS/ArchetypeManager.h"
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <functional>

using namespace NeoEngine;
using Clock = std::chrono::steady_clock;

void runTest(const char* name, std::function<void(ArchetypeManager&, XPBDPhysicsSystem&)> setup) {
    ArchetypeManager em;
    XPBDPhysicsSystem phys;
    
    setup(em, phys);

    phys.Step(em, 0.016f); // Warmup

    auto t0 = Clock::now();
    phys.Step(em, 0.016f); // Ukur
    auto t1 = Clock::now();

    float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
    size_t contacts = phys.GetManifoldCount();
    printf("[%s] %.3f ms | %zu contacts | %.1f us/contact\n", name, ms, contacts, contacts > 0? ms*1000.0f/contacts : 0);
}

int main() {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(-50, 50);

    printf("\n=== BENCHMARK V5.10 BITMASK ECS ===\n");

    constexpr uint32_t MASK = COMP_POSITION | COMP_VELOCITY | COMP_COLLIDER;

    runTest("20k_Sparse", [&](auto& em, auto& phys) {
        for(int i = 0; i < 20000; i++) {
            EntityID e = em.CreateEntity(MASK);
            auto* pos = em.GetPosition(e);
            auto* vel = em.GetVelocity(e);
            auto* col = em.GetCollider(e);
            if(pos) { pos->x = dist(rng); pos->y = 0; pos->z = dist(rng); }
            if(vel) { vel->vx = 0; vel->vy = 0; vel->vz = 0; }
            if(col) { col->radius = 0.5f; col->invMass = 1.0f; }
        }
    });

    runTest("200_Stack", [&](auto& em, auto& phys) {
        for(int i = 0; i < 200; i++) {
            EntityID e = em.CreateEntity(MASK);
            auto* pos = em.GetPosition(e);
            auto* vel = em.GetVelocity(e);
            auto* col = em.GetCollider(e);
            if(pos) { pos->x = 0; pos->y = i*1.0f; pos->z = 0; }
            if(vel) { vel->vx = 0; vel->vy = 0; vel->vz = 0; }
            if(col) { col->radius = 0.5f; col->invMass = 1.0f; }
        }
    });

    runTest("70pct_Tombstone", [&](auto& em, auto& phys) {
        std::vector<EntityID> temp;
        for(int i = 0; i < 23000; i++) {
            EntityID e = em.CreateEntity(MASK);
            float x = (i % 152) * 0.6f;
            float z = (i / 152) * 0.6f;
            auto* pos = em.GetPosition(e);
            auto* vel = em.GetVelocity(e);
            auto* col = em.GetCollider(e);
            if(pos) { pos->x = x; pos->y = 0; pos->z = z; }
            if(vel) { vel->vx = 0; vel->vy = 0; vel->vz = 0; }
            if(col) { col->radius = 0.5f; col->invMass = 1.0f; }
            if(i % 2 == 0) temp.push_back(e);
        }
        for(auto e : temp) em.DestroyEntity(e);
        for(int frame = 0; frame < 100; frame++) phys.Step(em, 0.016f);
    });

    runTest("Degenerate_Hash", [&](auto& em, auto& phys) {
        for(int i = 0; i < 5000; i++) {
            EntityID e = em.CreateEntity(MASK);
            auto* pos = em.GetPosition(e);
            auto* vel = em.GetVelocity(e);
            auto* col = em.GetCollider(e);
            if(pos) { pos->x = i*1.01f; pos->y = 0; pos->z = 0; }
            if(vel) { vel->vx = 0; vel->vy = 0; vel->vz = 0; }
            if(col) { col->radius = 0.5f; col->invMass = 1.0f; }
        }
    });

    printf("=== SELESAI ===\n");
    return 0;
}
