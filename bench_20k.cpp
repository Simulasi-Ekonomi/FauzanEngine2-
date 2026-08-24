#include "Core/ECS/ArchetypeManager.h"
#include "Physics/V5/XPBDPhysicsSystem.h"
#include <chrono>
#include <iostream>
#include <iomanip>

using namespace NeoEngine;

// PAKSA EXPORT SYMBOL AGAR DILIHAT OLEH .SO SAAT RUNTIME
extern "C" {
    void* GPlatform = nullptr;
}

namespace NeoEngine {
    namespace Log {
        // Dummy log agar tidak undefined
        void Write(int level, int channel, const std::string& msg) {}
    }
}

int main() {
    ArchetypeManager em;
    XPBDPhysicsSystem physics;
    float dt = 1.0f / 60.0f;

    std::cout << "\033[1;36m=== FAUZANENGINE V5.10 STRESS TEST (20K) ===\033[0m" << std::endl;

    // Setup 20k Entities
    for (int i = 0; i < 20000; ++i) {
        EntityID id = em.CreateEntity(7); // Mask: Pos|Vel|Col
        auto* pos = em.GetPosition(id);
        auto* col = em.GetCollider(id);
        if(pos && col) {
            pos->x = (i % 141) * 0.45f;
            pos->z = (i / 141) * 0.45f;
            col->radius = 0.5f;
            col->invMass = 1.0f;
        }
    }

    std::cout << "Warming up..." << std::endl;
    for(int i=0; i<5; ++i) physics.Step(em, dt);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 50; ++i) {
        physics.Step(em, dt);
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<float, std::milli> duration = end - start;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Avg Frame Time: \033[1;32m" << duration.count() / 50.0f << " ms\033[0m" << std::endl;
    std::cout << "------------------------------------" << std::endl;

    return 0;
}
