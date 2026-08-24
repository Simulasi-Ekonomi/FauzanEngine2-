#include "PhysicsCore.h"
#include <cmath>
#include <algorithm>
#include <android/log.h>

#define LOG_TAG "NeoEngine_Physics"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace NeoEngine {

PhysicsCore& PhysicsCore::Get() {
    static PhysicsCore instance;
    return instance;
}

void PhysicsCore::Initialize() {
    LOGI("PhysicsCore initialized - Gravity: (%.1f, %.1f, %.1f)", gravity.x, gravity.y, gravity.z);
    bodies.clear();
    nextId = 1;
    collisionCount = 0;
}

void PhysicsCore::Shutdown() {
    bodies.clear();
    collisionCount = 0;
    LOGI("PhysicsCore shutdown");
}

uint32_t PhysicsCore::CreateBody(const RigidBody& body) {
    uint32_t id = nextId++;
    RigidBody b = body;
    b.id = id;
    if (b.type == BodyType::Static) {
        b.inverseMass = 0.0f;
    }
    bodies[id] = b;
    return id;
}

void PhysicsCore::DestroyBody(uint32_t id) {
    bodies.erase(id);
}

RigidBody* PhysicsCore::GetBody(uint32_t id) {
    auto it = bodies.find(id);
    return it != bodies.end() ? &it->second : nullptr;
}

void PhysicsCore::SetBodyBounds(uint32_t id, const AABB& bounds) {
    auto it = bodies.find(id);
    if (it != bodies.end()) it->second.bounds = bounds;
}

void PhysicsCore::IntegrateForces(float dt) {
    for (auto& [id, body] : bodies) {
        if (body.type == BodyType::Static) continue;

        // Apply gravity
        if (body.useGravity) {
            body.force = body.force + gravity * body.mass;
        }

        // Semi-implicit Euler integration
        // acceleration = force / mass
        Vec3 accel = body.force * body.inverseMass;
        body.linearVelocity = body.linearVelocity + accel * dt;

        // Apply damping
        body.linearVelocity = body.linearVelocity * (1.0f - body.linearDamping);

        // Update position
        body.position = body.position + body.linearVelocity * dt;

        // Update bounds position
        Vec3 center = body.bounds.Center();
        Vec3 extents = body.bounds.Extents();
        Vec3 offset = body.position - center;
        body.bounds.min = body.bounds.min + offset;
        body.bounds.max = body.bounds.max + offset;

        // Reset accumulated force
        body.force = { 0.0f, 0.0f, 0.0f };

        // Simple ground collision (y = 0 plane)
        if (body.position.y < 0.0f) {
            body.position.y = 0.0f;
            if (body.linearVelocity.y < 0.0f) {
                body.linearVelocity.y = -body.linearVelocity.y * body.restitution;
                if (fabsf(body.linearVelocity.y) < 0.5f) {
                    body.linearVelocity.y = 0.0f;
                }
            }
        }
    }
}

void PhysicsCore::DetectCollisions() {
    collisionCount = 0;
    auto it = bodies.begin();

    while (it != bodies.end()) {
        auto jt = std::next(it);
        while (jt != bodies.end()) {
            const RigidBody& a = it->second;
            const RigidBody& b = jt->second;

            // Skip if both are static
            if (a.type == BodyType::Static && b.type == BodyType::Static) {
                ++jt;
                continue;
            }

            // Skip if both are triggers (no physical response)
            if (a.isTrigger && b.isTrigger) {
                ++jt;
                continue;
            }

            // AABB intersection test
            if (a.bounds.Intersects(b.bounds)) {
                CollisionInfo info;
                info.bodyA = a.id;
                info.bodyB = b.id;

                // Calculate collision normal (from A to B)
                Vec3 centerA = a.bounds.Center();
                Vec3 centerB = b.bounds.Center();
                Vec3 diff = centerB - centerA;

                // Calculate overlap on each axis
                float overlapX = (a.bounds.Extents().x + b.bounds.Extents().x) - fabsf(diff.x);
                float overlapY = (a.bounds.Extents().y + b.bounds.Extents().y) - fabsf(diff.y);
                float overlapZ = (a.bounds.Extents().z + b.bounds.Extents().z) - fabsf(diff.z);

                // Find minimum overlap axis
                if (overlapX <= overlapY && overlapX <= overlapZ) {
                    info.normal = diff.x > 0 ? Vec3{1,0,0} : Vec3{-1,0,0};
                    info.penetration = overlapX;
                } else if (overlapY <= overlapX && overlapY <= overlapZ) {
                    info.normal = diff.y > 0 ? Vec3{0,1,0} : Vec3{0,-1,0};
                    info.penetration = overlapY;
                } else {
                    info.normal = diff.z > 0 ? Vec3{0,0,1} : Vec3{0,0,-1};
                    info.penetration = overlapZ;
                }

                info.contactPoint = centerA + info.normal * (a.bounds.Extents().x + info.penetration * 0.5f);

                ResolveCollision(info);
                collisionCount++;

                // Notify callback
                if (collisionCallback) {
                    collisionCallback(info);
                }
            }

            ++jt;
        }
        ++it;
    }
}

void PhysicsCore::ResolveCollision(const CollisionInfo& collision) {
    RigidBody* a = GetBody(collision.bodyA);
    RigidBody* b = GetBody(collision.bodyB);
    if (!a || !b) return;

    // Triggers don't generate physical response
    if (a->isTrigger || b->isTrigger) return;

    // Separate objects (positional correction)
    float totalInverseMass = a->inverseMass + b->inverseMass;
    if (totalInverseMass <= 0.0001f) return;

    float correctionFactor = collision.penetration / totalInverseMass * 0.8f;
    Vec3 correction = collision.normal * correctionFactor;

    a->position = a->position - correction * a->inverseMass;
    b->position = b->position + correction * b->inverseMass;

    // Impulse resolution
    Vec3 relativeVelocity = b->linearVelocity - a->linearVelocity;
    float velAlongNormal = relativeVelocity.Dot(collision.normal);

    // Don't resolve if separating
    if (velAlongNormal > 0) return;

    // Restitution (bounciness)
    float e = std::min(a->restitution, b->restitution);

    // Impulse magnitude
    float j = -(1.0f + e) * velAlongNormal;
    j /= totalInverseMass;

    // Apply impulse
    Vec3 impulse = collision.normal * j;
    a->linearVelocity = a->linearVelocity - impulse * a->inverseMass;
    b->linearVelocity = b->linearVelocity + impulse * b->inverseMass;

    // Friction impulse
    Vec3 tangent = relativeVelocity - collision.normal * velAlongNormal;
    float tangentLen = tangent.Length();
    if (tangentLen > 0.0001f) {
        tangent = tangent.Normalized();
        float jt = -relativeVelocity.Dot(tangent);
        jt /= totalInverseMass;

        // Coulomb's law
        float mu = (a->friction + b->friction) * 0.5f;
        Vec3 frictionImpulse;
        if (fabsf(jt) < j * mu) {
            frictionImpulse = tangent * jt;
        } else {
            frictionImpulse = tangent * (-j * mu);
        }

        a->linearVelocity = a->linearVelocity - frictionImpulse * a->inverseMass;
        b->linearVelocity = b->linearVelocity + frictionImpulse * b->inverseMass;
    }
}

bool PhysicsCore::Raycast(const Vec3& origin, const Vec3& direction, float maxDist, RaycastHit& outHit) {
    Vec3 dir = direction.Normalized();
    float closestT = maxDist;
    outHit.hit = false;

    for (auto& [id, body] : bodies) {
        float t;
        if (RaycastAABB(origin, dir, body.bounds, t) && t < closestT) {
            closestT = t;
            outHit.hit = true;
            outHit.distance = t;
            outHit.point = origin + dir * t;
            outHit.hitBody = body.userData;
            // Simple normal calculation
            Vec3 toPoint = (outHit.point - body.bounds.Center()).Normalized();
            // Determine which face was hit
            Vec3 half = body.bounds.Extents();
            if (fabsf(toPoint.x) / half.x > fabsf(toPoint.y) / half.y &&
                fabsf(toPoint.x) / half.x > fabsf(toPoint.z) / half.z) {
                outHit.normal = toPoint.x > 0 ? Vec3{1,0,0} : Vec3{-1,0,0};
            } else if (fabsf(toPoint.y) / half.y > fabsf(toPoint.z) / half.z) {
                outHit.normal = toPoint.y > 0 ? Vec3{0,1,0} : Vec3{0,-1,0};
            } else {
                outHit.normal = toPoint.z > 0 ? Vec3{0,0,1} : Vec3{0,0,-1};
            }
        }
    }

    return outHit.hit;
}

bool PhysicsCore::RaycastAABB(const Vec3& origin, const Vec3& dir, const AABB& box, float& outT) {
    float tmin = 0.0f;
    float tmax = 999999.0f;

    // X axis
    if (fabsf(dir.x) < 0.0001f) {
        if (origin.x < box.min.x || origin.x > box.max.x) return false;
    } else {
        float t1 = (box.min.x - origin.x) / dir.x;
        float t2 = (box.max.x - origin.x) / dir.x;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Y axis
    if (fabsf(dir.y) < 0.0001f) {
        if (origin.y < box.min.y || origin.y > box.max.y) return false;
    } else {
        float t1 = (box.min.y - origin.y) / dir.y;
        float t2 = (box.max.y - origin.y) / dir.y;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    // Z axis
    if (fabsf(dir.z) < 0.0001f) {
        if (origin.z < box.min.z || origin.z > box.max.z) return false;
    } else {
        float t1 = (box.min.z - origin.z) / dir.z;
        float t2 = (box.max.z - origin.z) / dir.z;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }

    outT = tmin;
    return true;
}

bool PhysicsCore::CheckOverlap(const AABB& box, uint32_t ignoreId) {
    for (auto& [id, body] : bodies) {
        if (id == ignoreId) continue;
        if (box.Intersects(body.bounds)) return true;
    }
    return false;
}

void PhysicsCore::Step(float deltaTime) {
    // Cap delta time to prevent explosion
    if (deltaTime > 0.05f) deltaTime = 0.05f;

    // Sub-stepping for stability
    const int subSteps = 2;
    float subDt = deltaTime / subSteps;

    for (int i = 0; i < subSteps; i++) {
        IntegrateForces(subDt);
        DetectCollisions();
    }
}

} // namespace NeoEngine
