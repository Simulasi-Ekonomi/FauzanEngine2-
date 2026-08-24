#include "Core/Math/NeoMath.h"
#include "PhysicsEngine.h"
#include <cmath>

namespace NeoEngine {

void PhysicsEngine::Step(float dt) {
    for (auto& body : m_Bodies) {
        if (body.isStatic || body.isKinematic) continue;
        
        // Gravity
        body.velY -= 9.81f * dt;
        
        // Update posisi
        body.posX += body.velX * dt;
        body.posY += body.velY * dt;
        body.posZ += body.velZ * dt;
        
        // Gesekan
        body.velX *= (1.0f - body.friction * dt);
        body.velZ *= (1.0f - body.friction * dt);
        
        // Batasi kecepatan terminal
        float speed = NeoEngine::Math::Sqrt(body.velX*body.velX + body.velY*body.velY + body.velZ*body.velZ);
        const float maxSpeed = 100.0f;
        if (speed > maxSpeed) {
            float scale = maxSpeed / speed;
            body.velX *= scale;
            body.velY *= scale;
            body.velZ *= scale;
        }
    }
}

} // namespace NeoEngine
