#include "Core/ECS/EntityManager.h"
#include "ECS/V4/FauzanECSSystems.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace NeoEngine;
using namespace std::chrono;

int main() {
    printf("============================================\n");
    printf(" STRESS TEST – 100,000 Entities\n");
    printf("============================================\n");

    EntityManager em;
    const int NUM = 100000;
    auto t0 = high_resolution_clock::now();
    for (int i=0; i<NUM; ++i) {
        auto id = em.CreateEntity();
        em.positions.x[id] = rand()%1000;
        em.positions.y[id] = rand()%1000;
        em.positions.z[id] = rand()%1000;
        em.velocities.vx[id] = (rand()%100)/10.0f;
        em.velocities.vy[id] = (rand()%100)/10.0f;
        em.velocities.vz[id] = (rand()%100)/10.0f;
    }
    auto t1 = high_resolution_clock::now();
    printf("  Creation: %.2f ms\n", duration<float,std::milli>(t1-t0).count());

    MovementSystemTurbo sys;
    auto t2 = high_resolution_clock::now();
    for (int f=0; f<100; ++f) sys.Update(0.016f, em);
    auto t3 = high_resolution_clock::now();
    float frameTime = duration<float,std::milli>(t3-t2).count() / 100.0f;
    printf("  Movement (100 frames): %.2f ms total, %.2f ms/frame\n",
           duration<float,std::milli>(t3-t2).count(), frameTime);
    printf("  100K entities @ %.2f ms/frame\n", frameTime);

    // Destruksi massal
    auto t4 = high_resolution_clock::now();
    for (int i=0; i<NUM; ++i) em.DestroyEntity(i);
    auto t5 = high_resolution_clock::now();
    printf("  Destruction: %.2f ms\n", duration<float,std::milli>(t5-t4).count());

    printf("============================================\n");
    if (frameTime < 3.0f) printf(" ✅ STRESS TEST PASSED (< 3 ms)\n");
    else printf(" ⚠️ Frame time di atas target\n");
    printf("============================================\n");
    return 0;
}
