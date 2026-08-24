#include "Core/Math/NeoMath.h"
#include <cstdio>
#include <chrono>
#include <vector>
#include <cmath>

#include "../Core/Memory/MemoryManager.h"
#include "../World/NeoWorld.h"
#include "../ECS/EntityManager.h"
#include "../Systems/RPGSystem.h"
#include "../Systems/CombatSystem.h"
#include "../Systems/FarmingSystem.h"
#include "../Systems/CraftingSystem.h"
#include "../Systems/InventorySystem.h"

using namespace NeoEngine;
using Clock = std::chrono::high_resolution_clock;

static void SimulatePhysicsTick(EntityManager& em) {
    em.ForEach([&](EntityID e) {
        volatile float x = (e * 3.14159f) * 0.001f;
        volatile float y = NeoEngine::Math::Sin(x);
        y += x * 0.001f;
        (void)y;
    });
}

int main() {
    printf("========================================\n");
    printf("  FAUZANENGINE PERF BENCHMARK v7 (GRID)\n");
    printf("========================================\n\n");

    auto t0 = Clock::now();

    // --- FASE 1: Memory ---
    printf("[1/6] Memory Stress (10,000 allocs)...\n");
    MemoryManager::Init();
    auto t1 = Clock::now();
    std::vector<void*> all; all.reserve(10000);
    for (int i=0; i<10000; i++) {
        void* p = MemoryManager::Allocate(1024);
        if (p) all.push_back(p);
    }
    for (void* p : all) MemoryManager::Free(p);
    all.clear();
    auto t2 = Clock::now();
    printf("  ✓ %.2f ms (peak %.2f MB)\n\n",
           std::chrono::duration<float,std::milli>(t2-t1).count(),
           MemoryManager::GetPeakAllocated()/(1024.f*1024.f));

    // --- FASE 2: ECS ---
    printf("[2/6] ECS (50,000 entities)...\n");
    EntityManager em;
    std::vector<EntityID> ents; ents.reserve(50000);
    auto t3 = Clock::now();
    for (int i=0; i<50000; i++) {
        EntityID e = em.CreateEntity();
        if (e!=INVALID_ENTITY) ents.push_back(e);
    }
    for (int pass=0; pass<10; pass++) em.ForEach([&](EntityID e) { volatile float x=e*3.14f; (void)x; });
    for (EntityID e : ents) em.DestroyEntity(e);
    ents.clear();
    auto t4 = Clock::now();
    printf("  ✓ %.2f ms\n\n", std::chrono::duration<float,std::milli>(t4-t3).count());

    // --- FASE 3: World Generation ---
    printf("[3/6] World Gen (5,000 actors)...\n");
    NeoWorld world;
    const char* names[5] = {"Oak","Pine","Birch","Maple","Elm"};
    char buf[32];
    auto t5 = Clock::now();
    for (int i=0; i<5000; i++) {
        snprintf(buf, sizeof(buf), "Tree_%d", i);
        world.SpawnActor(buf, names[i%5], (i%100)*10.f, 0.f, (i/100)*10.f);
    }
    auto t6 = Clock::now();
    printf("  ✓ %.2f ms\n\n", std::chrono::duration<float,std::milli>(t6-t5).count());

    // --- FASE 4: RPG ---
    printf("[4/6] RPG / Combat...\n");
    RPGSystem rpg;
    CombatSystem combat;
    InventorySystem inv;
    CraftingSystem craft;
    auto* hero = rpg.CreateCharacter("hero","NeoHero");
    rpg.AddExp(hero, 5000);
    CombatStats atk{100,100,50,20,2.f,2.f,0.2f,2.5f,0};
    CombatStats def{200,200,30,30,1.f,2.f,0.05f,1.5f,0};
    float totalDmg=0;
    for (int i=0; i<1000; i++) totalDmg += combat.CalculateDamage(atk,def).amount;
    inv.AddGold(10000);
    inv.AddItem("Potion","potion",50);
    inv.AddItem("Sword","weapon",1);
    craft.AddRecipe({"Potion","Potion",1,{{"Herb",3},{"Water",1}},2.f});
    printf("  ✓ Hero Lv.%d | Avg dmg:%.1f | Items:%zu | Recipes:%zu\n\n",
           hero->level, totalDmg/1000.f, inv.GetItems().size(), craft.GetAllRecipes().size());

    // --- FASE 5: Game Loop dengan Spatial Grid ---
    printf("[5/6] Game Loop (5s, 5000 NPCs + Spatial Grid)...\n");
    FarmingSystem farm;
    farm.TillPlot(3,3); farm.PlantCrop(3,3, CropType::Wheat); farm.WaterPlot(3,3);

    std::vector<EntityID> npcs; npcs.reserve(5000);
    for (int i=0; i<5000; i++) {
        EntityID e = em.CreateEntity();
        if (e!=INVALID_ENTITY) npcs.push_back(e);
    }

    const float dt = 0.016f;
    float simTime = 0.f;
    int frames = 0;
    auto loopStart = Clock::now();

    while (simTime < 5.0f) {
        SimulatePhysicsTick(em);
        world.Update(dt);    // ← SEKARANG DENGAN SPATIAL GRID
        farm.Update(dt);
        simTime += dt;
        frames++;
    }

    auto loopEnd = Clock::now();
    float loopMs = std::chrono::duration<float,std::milli>(loopEnd-loopStart).count();
    float fps = (loopMs > 0.f) ? frames / (loopMs / 1000.f) : 0.f;
    printf("  ✓ %d frames in %.2f ms → %.1f FPS (%.2f ms/frame)\n\n",
           frames, loopMs, fps, loopMs/frames);

    for (EntityID e : npcs) em.DestroyEntity(e);
    npcs.clear();

    // --- FASE 6: Final Report ---
    auto tEnd = Clock::now();
    float totalMs = std::chrono::duration<float,std::milli>(tEnd-t0).count();
    printf("[6/6] Final Report...\n");
    printf("  Total: %.2f ms\n", totalMs);
    printf("  ECS: %.2f ms\n", std::chrono::duration<float,std::milli>(t4-t3).count());
    printf("  World: %.2f ms\n", std::chrono::duration<float,std::milli>(t6-t5).count());
    printf("  Game Loop: %.1f FPS (%.2f ms/frame)\n", fps, loopMs/frames);
    printf("  Memory peak: %zu bytes\n", MemoryManager::GetPeakAllocated());
    printf("  World actors: %zu\n", world.GetActorCount());
    printf("  ECS alive: %zu\n", em.GetLivingEntityCount());
    printf("========================================\n");

    FILE* log = fopen("/sdcard/Buku saya/benchmark_log.txt","w");
    if (log) {
        fprintf(log, "HIGH-PERF v7 GRID\nTotal: %.2f ms\nECS: %.2f ms\nWorld: %.2f ms\nLoop: %.1f FPS (%.2f ms/frame)\nMemory: %zu bytes\n",
                totalMs,
                std::chrono::duration<float,std::milli>(t4-t3).count(),
                std::chrono::duration<float,std::milli>(t6-t5).count(),
                fps, loopMs/frames,
                MemoryManager::GetPeakAllocated());
        fclose(log);
    }

    return 0;
}
