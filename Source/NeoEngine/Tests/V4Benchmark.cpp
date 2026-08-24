#include "FauzanEngine.h"
#include <chrono>
#include <cstdio>
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "V4Benchmark", __VA_ARGS__)

using namespace NeoEngine;
using namespace std::chrono;

int main() {
    LOGI("============================================");
    LOGI(" V4 Benchmark – SoA ECS + AI + Spatial     ");
    LOGI("============================================");

    // 1. Benchmark ECS (FauzanEntityManager SoA)
    FauzanEntityManager em;
    const int NUM_ENTITIES = 50000;

    // Buat 50.000 entitas
    auto t0 = high_resolution_clock::now();
    for (int i = 0; i < NUM_ENTITIES; ++i) {
        EntityID id = em.CreateEntity();
        em.positions.x[id] = rand() % 1000;
        em.positions.y[id] = rand() % 1000;
        em.positions.z[id] = rand() % 1000;
        em.velocities.vx[id] = (rand() % 100) / 10.0f;
        em.velocities.vy[id] = (rand() % 100) / 10.0f;
        em.velocities.vz[id] = (rand() % 100) / 10.0f;
    }
    auto t1 = high_resolution_clock::now();
    LOGI("  Creation + SoA resize: %.2f ms", duration<float, std::milli>(t1 - t0).count());

    // Movement system (simulasi 100 frame)
    MovementSystem sys;
    auto t2 = high_resolution_clock::now();
    for (int f = 0; f < 100; ++f) {
        sys.Update(0.016f, em);
    }
    auto t3 = high_resolution_clock::now();
    float frameTime = duration<float, std::milli>(t3 - t2).count() / 100.0f;
    LOGI("  Movement (100 frames): %.2f ms total, %.2f ms/frame", 
         duration<float, std::milli>(t3 - t2).count(), frameTime);
    LOGI("  Entity throughput: %d entities @ %.2f ms/frame", NUM_ENTITIES, frameTime);

    // 2. Benchmark AI (MoE Reasoning)
    MoEConfig cfg;
    cfg.numExperts = 4;
    FauzanAIAgent ai;
    ai.ConfigureMoE(cfg);
    std::vector<float> obs{0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f};

    auto t4 = high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        ai.Think(obs, 5);
    }
    auto t5 = high_resolution_clock::now();
    LOGI("  AI MoE (100 decisions): %.2f ms", duration<float, std::milli>(t5 - t4).count());

    // 3. Benchmark Spatial (Hybrid query)
    HybridSpatial spatial(1000.0f, 50.0f);
    for (int i = 0; i < 5000; ++i) {
        spatial.Insert(i, rand()%900-450, 0, rand()%900-450, i % 3 == 0);
    }
    auto t6 = high_resolution_clock::now();
    for (int i = 0; i < 100; ++i) {
        spatial.Query(rand()%500, 0, rand()%500, 30.0f);
    }
    auto t7 = high_resolution_clock::now();
    LOGI("  Spatial Query (100 queries): %.2f ms", duration<float, std::milli>(t7 - t6).count());

    LOGI("============================================");
    LOGI(" V4 Benchmark Complete                      ");
    LOGI(" Target: ECS 50K < 3 ms → actual: %.2f ms", frameTime);
    LOGI("============================================");
    return 0;
}
