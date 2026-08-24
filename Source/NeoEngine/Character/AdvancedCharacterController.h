#pragma once
#include "Core/Math/Vector3.h"
#include "Physics/RigidBody.h"
#include <vector>
#include <cmath>

namespace NeoEngine {
class AdvancedCharacterController {
public:
    void SetCapsule(float r, float hh) { m_Radius=r; m_HalfHeight=hh; }
    void SetPosition(const Vec3& p) { m_Position=p; }
    Vec3 GetPosition() const { return m_Position; }
    void SetGroundLevel(float l) { m_GroundLevel=l; }
    bool IsGrounded() const { return m_Grounded; }

    void Move(const Vec3& displacement, float dt, const std::vector<RigidBody*>& obstacles) {
        m_Velocity.y -= m_Gravity * dt;
        Vec3 delta = displacement + m_Velocity * dt;
        Vec3 target = m_Position + delta;

        // Collision resolution (tanpa Y)
        for (auto* obs : obstacles) {
            if (!obs) continue;
            Vec3 diff = target - obs->GetPosition();
            float minDist = m_Radius + (obs->GetHalfSize().x + obs->GetHalfSize().y + obs->GetHalfSize().z) / 3.0f;
            float dist = diff.Length();
            if (dist < minDist && dist > 0.001f) {
                Vec3 normal = diff.Normalized();
                normal.y = 0; if (normal.Length() < 0.001f) normal = {1,0,0};
                else normal = normal.Normalized();
                target = target + normal * (minDist - dist);
                float velDot = m_Velocity.Dot(normal);
                if (velDot < 0) m_Velocity = m_Velocity - normal * velDot;
            }
        }

        // Ground clamp
        float footY = target.y - m_HalfHeight;
        if (footY < m_GroundLevel) {
            target.y = m_GroundLevel + m_HalfHeight;
            m_Velocity.y = 0;
            m_Grounded = true;
        } else m_Grounded = false;

        // Obstacle top check
        for (auto* obs : obstacles) {
            if (!obs) continue;
            float obsTop = obs->GetPosition().y + obs->GetHalfSize().y;
            if (footY <= obsTop && footY >= obsTop - 0.3f &&
                target.x >= obs->GetPosition().x - obs->GetHalfSize().x &&
                target.x <= obs->GetPosition().x + obs->GetHalfSize().x &&
                target.z >= obs->GetPosition().z - obs->GetHalfSize().z &&
                target.z <= obs->GetPosition().z + obs->GetHalfSize().z) {
                target.y = obsTop + m_HalfHeight;
                m_Velocity.y = 0; m_Grounded = true;
            }
        }

        m_Position = target;
    }

    void Jump() { if (m_Grounded) { m_Velocity.y = m_JumpForce; m_Grounded = false; } }

private:
    Vec3 m_Position{0,2,0}, m_Velocity{0,0,0};
    float m_Radius=0.4f, m_HalfHeight=1.0f;
    float m_Gravity=15.0f, m_JumpForce=8.0f;
    float m_GroundLevel=0.0f;
    bool m_Grounded=false;
};
}
