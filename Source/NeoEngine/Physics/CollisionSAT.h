#pragma once
#include "RigidBody.h"
#include <cmath>
#include <algorithm>

namespace NeoEngine {

struct CollisionResult {
    bool hit = false;
    Vec3 normal;
    float depth = 0.0f;
};

class CollisionSAT {
public:
    // SAT antara dua AABB
    static CollisionResult ResolveAABB(const RigidBody& a, const RigidBody& b) {
        CollisionResult result;
        Vec3 centerA = a.GetPosition();
        Vec3 centerB = b.GetPosition();
        Vec3 diff = centerB - centerA;
        Vec3 halfA = a.GetHalfSize();
        Vec3 halfB = b.GetHalfSize();
        
        float overlapX = (halfA.x + halfB.x) - std::abs(diff.x);
        float overlapY = (halfA.y + halfB.y) - std::abs(diff.y);
        float overlapZ = (halfA.z + halfB.z) - std::abs(diff.z);
        
        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0) return result;
        
        result.hit = true;
        if (overlapX <= overlapY && overlapX <= overlapZ) {
            result.depth = overlapX;
            result.normal = {diff.x > 0 ? 1.0f : -1.0f, 0, 0};
        } else if (overlapY <= overlapX && overlapY <= overlapZ) {
            result.depth = overlapY;
            result.normal = {0, diff.y > 0 ? 1.0f : -1.0f, 0};
        } else {
            result.depth = overlapZ;
            result.normal = {0, 0, diff.z > 0 ? 1.0f : -1.0f};
        }
        return result;
    }
    
    // Deteksi + Respon tumbukan (Impulse-based)
    static void ResolveCollision(RigidBody& a, RigidBody& b) {
        CollisionResult cr = ResolveAABB(a, b);
        if (!cr.hit) return;
        
        Vec3 normal = cr.normal;
        float depth = cr.depth;
        
        // Pisahkan posisi (position correction)
        float invMassA = 1.0f / a.GetMass();
        float invMassB = 1.0f / b.GetMass();
        float totalInvMass = invMassA + invMassB;
        a.SetPosition(a.GetPosition() - normal * (depth * invMassA / totalInvMass));
        b.SetPosition(b.GetPosition() + normal * (depth * invMassB / totalInvMass));
        
        // Impuls
        Vec3 relVel = b.GetVelocity() - a.GetVelocity();
        float velAlongNormal = relVel.Dot(normal);
        
        // Tidak perlu respon jika sudah menjauh
        if (velAlongNormal > 0) return;
        
        float e = std::min(a.GetRestitution(), b.GetRestitution());
        float j = -(1.0f + e) * velAlongNormal / totalInvMass;
        Vec3 impulse = normal * j;
        a.ApplyImpulse(impulse * -1.0f);
        b.ApplyImpulse(impulse);
    }
    
    static bool CheckCollision(const RigidBody& a, const RigidBody& b) {
        return ResolveAABB(a, b).hit;
    }
};

}
