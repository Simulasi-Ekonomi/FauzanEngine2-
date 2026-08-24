#include "PhysicsEngine.h"

void PhysicsEngine::Step(float dt) {
    const float Gravity = -980.0f; // Unreal Units (cm/s^2)
    
    for (auto& body : physicsBodies) {
        if (body.isStatic) continue;

        // Apply Gravity
        body.velocity.z += Gravity * dt;
        
        // Euler Integration
        body.position += body.velocity * dt;
        
        // Basic Ground Collision
        if (body.position.z < 0.0f) {
            body.position.z = 0.0f;
            body.velocity.z = 0.0f;
        }
    }
}
