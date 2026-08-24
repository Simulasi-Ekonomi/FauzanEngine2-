#include "Physics/RigidBody.h"
#include "Physics/CollisionSAT.h"
#include <cstdio>

int main() {
    printf("============================================\n");
    printf(" PHYSICS TEST – SAT + Impulse Response\n");
    printf("============================================\n\n");

    NeoEngine::RigidBody bodyA, bodyB, bodyC;
    
    // Setup body
    bodyA.SetPosition({0, 0, 0});
    bodyA.SetVelocity({2, 0, 0});
    bodyA.SetMass(2.0f);
    bodyA.SetRestitution(0.8f);
    
    bodyB.SetPosition({3, 0, 0});
    bodyB.SetVelocity({-1, 0, 0});
    bodyB.SetMass(1.0f);
    bodyB.SetRestitution(0.5f);
    
    bodyC.SetPosition({6, 0, 0});
    bodyC.SetVelocity({0, 0, 0});
    bodyC.SetMass(5.0f);
    bodyC.SetRestitution(0.1f);
    
    // Simulasi 30 langkah
    float dt = 0.05f;
    int collisionCount = 0;
    
    for (int step = 0; step < 30; ++step) {
        // Cek dan respon semua pasangan
        if (NeoEngine::CollisionSAT::CheckCollision(bodyA, bodyB)) {
            NeoEngine::CollisionSAT::ResolveCollision(bodyA, bodyB);
            collisionCount++;
        }
        if (NeoEngine::CollisionSAT::CheckCollision(bodyA, bodyC)) {
            NeoEngine::CollisionSAT::ResolveCollision(bodyA, bodyC);
            collisionCount++;
        }
        if (NeoEngine::CollisionSAT::CheckCollision(bodyB, bodyC)) {
            NeoEngine::CollisionSAT::ResolveCollision(bodyB, bodyC);
            collisionCount++;
        }
        
        // Integrasi
        bodyA.Integrate(dt);
        bodyB.Integrate(dt);
        bodyC.Integrate(dt);
    }
    
    printf("✅ Collisions detected & resolved: %d\n\n", collisionCount);
    printf("Final positions:\n");
    printf("  Body A: (%.2f, %.2f, %.2f)\n", bodyA.GetPosition().x, bodyA.GetPosition().y, bodyA.GetPosition().z);
    printf("  Body B: (%.2f, %.2f, %.2f)\n", bodyB.GetPosition().x, bodyB.GetPosition().y, bodyB.GetPosition().z);
    printf("  Body C: (%.2f, %.2f, %.2f)\n", bodyC.GetPosition().x, bodyC.GetPosition().y, bodyC.GetPosition().z);
    printf("\nFinal velocities:\n");
    printf("  Body A: (%.2f, %.2f, %.2f)\n", bodyA.GetVelocity().x, bodyA.GetVelocity().y, bodyA.GetVelocity().z);
    printf("  Body B: (%.2f, %.2f, %.2f)\n", bodyB.GetVelocity().x, bodyB.GetVelocity().y, bodyB.GetVelocity().z);
    printf("  Body C: (%.2f, %.2f, %.2f)\n", bodyC.GetVelocity().x, bodyC.GetVelocity().y, bodyC.GetVelocity().z);
    
    printf("\n============================================\n");
    return 0;
}
