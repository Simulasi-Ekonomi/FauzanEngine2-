#pragma once
#include "Core/Math/Vector3.h"

namespace NeoEngine {

struct AABB {
    Vec3 min, max;
    Vec3 Center() const { return {(min.x+max.x)*0.5f, (min.y+max.y)*0.5f, (min.z+max.z)*0.5f}; }
    Vec3 Extents() const { return {(max.x-min.x)*0.5f, (max.y-min.y)*0.5f, (max.z-min.z)*0.5f}; }
};

class RigidBody {
public:
    Vec3 position{0,0,0};
    Vec3 velocity{0,0,0};
    Vec3 halfSize{0.5f, 0.5f, 0.5f};
    float mass = 1.0f;
    float restitution = 0.3f;

    void SetPosition(const Vec3& p) { position = p; }
    Vec3 GetPosition() const { return position; }
    void SetVelocity(const Vec3& v) { velocity = v; }
    Vec3 GetVelocity() const { return velocity; }
    void SetMass(float m) { mass = m; if(mass<0.1f) mass=0.1f; }
    float GetMass() const { return mass; }
    void SetRestitution(float r) { restitution = (r < 0.0f ? 0.0f : (r > 1.0f ? 1.0f : r)); }
    float GetRestitution() const { return restitution; }
    void SetHalfSize(const Vec3& hs) { halfSize = hs; }
    Vec3 GetHalfSize() const { return halfSize; }
    void ApplyImpulse(const Vec3& impulse) {
        velocity.x += impulse.x / mass;
        velocity.y += impulse.y / mass;
        velocity.z += impulse.z / mass;
    }
    void Integrate(float dt) {
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        position.z += velocity.z * dt;
    }
};

} // namespace NeoEngine
