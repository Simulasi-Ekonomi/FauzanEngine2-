#include "Physics/PhysicsSystem.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include "Core/ECS/EntityManager.h"
#include <iostream>
#include <chrono>

using namespace NeoEngine;

int main() {
    const int N = 10000;
    const int frames = 50;
    float dt = 1.0f/60.0f;

    srand(42);
    std::vector<float> posX(N), posZ(N), velX(N), velZ(N);
    std::vector<EntityID> ids(N);
    for (int i=0; i<N; ++i) {
        posX[i] = (rand()%8000 - 4000)/1000.0f;
        posZ[i] = (rand()%8000 - 4000)/1000.0f;
        velX[i] = (rand()%200 - 100)/100.0f;
        velZ[i] = (rand()%200 - 100)/100.0f;
    }

    // --- V4 ---
    EntityManager em4;
    for (int i=0; i<N; ++i) {
        EntityID id = em4.CreateEntity();
        ids[i] = id;
        em4.positions.x[id] = posX[i];
        em4.positions.z[id] = posZ[i];
        em4.velocities.vx[id] = velX[i];
        em4.velocities.vz[id] = velZ[i];
        em4.colliders.radius[id] = 0.2f;
        em4.colliders.invMass[id] = 1.0f;
    }
    PhysicsSystem physV4;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int f=0; f<frames; ++f) physV4.Solve(em4, dt);
    auto t1 = std::chrono::high_resolution_clock::now();
    float msV4 = std::chrono::duration<float,std::milli>(t1-t0).count()/frames;
    int contactsV4 = physV4.GetContactCount();

    // --- V5 ---
    EntityManager em5;
    for (int i=0; i<N; ++i) {
        EntityID id = em5.CreateEntity();
        em5.positions.x[id] = posX[i];
        em5.positions.z[id] = posZ[i];
        em5.velocities.vx[id] = velX[i];
        em5.velocities.vz[id] = velZ[i];
        em5.colliders.radius[id] = 0.2f;
        em5.colliders.invMass[id] = 1.0f;
    }
    XPBDPhysicsSystem physV5;
    auto t2 = std::chrono::high_resolution_clock::now();
    for (int f=0; f<frames; ++f) physV5.Step(em5, dt);
    auto t3 = std::chrono::high_resolution_clock::now();
    float msV5 = std::chrono::duration<float,std::milli>(t3-t2).count()/frames;
    size_t manifoldsV5 = physV5.GetManifoldCount();

    std::cout << "============ AUDIT V4 vs V5 ============\n";
    std::cout << "Entities: " << N << " | Frames: " << frames << "\n";
    std::cout << "V4 (old) : " << msV4 << " ms  | contacts: " << contactsV4 << "\n";
    std::cout << "V5 (new) : " << msV5 << " ms  | manifolds: " << manifoldsV5 << "\n";

    return 0;
}
