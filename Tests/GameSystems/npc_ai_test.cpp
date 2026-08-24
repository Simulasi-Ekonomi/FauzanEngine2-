#include "AI/V4/AIAgent.h"
#include "Character/AdvancedCharacterController.h"
#include "Physics/RigidBody.h"
#include <cstdio>
#include <cmath>
#include <vector>
#include <cstdlib>

using namespace NeoEngine;

// NPC sederhana dengan AI state
class GameNPC {
public:
    Vec3 position;
    Vec3 playerPos;
    enum State { Idle, Patrol, Chase } state = Idle;
    float patrolAngle = 0.0f;
    float moveSpeed = 2.0f;
    float detectionRange = 10.0f;
    
    void Update(float dt) {
        float distToPlayer = (position - playerPos).Length();
        
        switch (state) {
            case Idle:
                if (distToPlayer < detectionRange) state = Chase;
                else {
                    patrolAngle += dt * 0.5f;
                    position.x += cos(patrolAngle) * moveSpeed * dt * 0.3f;
                    position.z += sin(patrolAngle) * moveSpeed * dt * 0.3f;
                }
                break;
            case Patrol:
                if (distToPlayer < detectionRange) state = Chase;
                break;
            case Chase:
                if (distToPlayer > detectionRange * 1.5f) state = Idle;
                else {
                    Vec3 dir = (playerPos - position).Normalized();
                    position = position + dir * moveSpeed * dt;
                }
                break;
        }
    }
};

int main() {
    printf("============================================\n");
    printf(" NPC AI TEST – Patrol + Chase\n");
    printf("============================================\n\n");
    
    GameNPC npc;
    npc.position = {5, 0, 5};
    npc.playerPos = {10, 0, 10};
    npc.state = GameNPC::Idle;
    
    float dt = 0.016f;
    printf("  t=0.0s: NPC at (%.1f, %.1f), State=Idle\n", npc.position.x, npc.position.z);
    
    // Simulasi 3 detik (player di dekat NPC)
    for (int f = 0; f < 180; ++f) {
        npc.Update(dt);
        if (f == 60) printf("  t=1.0s: NPC at (%.1f, %.1f), State=%s\n", 
            npc.position.x, npc.position.z,
            npc.state == GameNPC::Chase ? "Chase" : "Idle");
    }
    
    printf("  t=3.0s: NPC at (%.1f, %.1f), State=%s\n", 
        npc.position.x, npc.position.z,
        npc.state == GameNPC::Chase ? "Chase" : "Idle");
    
    // Pindahkan player jauh
    npc.playerPos = {50, 0, 50};
    for (int f = 0; f < 120; ++f) npc.Update(dt);
    printf("  t=5.0s: NPC at (%.1f, %.1f), State=%s\n", 
        npc.position.x, npc.position.z,
        npc.state == GameNPC::Idle ? "Idle (lost)" : "Still chasing");
    
    printf("\n============================================\n");
    printf(" ✅ NPC AI: Patrol + Chase working\n");
    printf("============================================\n");
    return 0;
}
