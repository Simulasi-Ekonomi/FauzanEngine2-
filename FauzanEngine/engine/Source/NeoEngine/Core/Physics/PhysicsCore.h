#pragma once
#include <vector>
#include <unordered_map>
#include <functional>

namespace NeoEngine {

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3 operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3 operator*(float s) const { return {x*s, y*s, z*s}; }
    float Dot(const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    float LengthSq() const { return x*x + y*y + z*z; }
    float Length() const { return sqrtf(LengthSq()); }
    Vec3 Normalized() const { float l = Length(); return l > 0 ? Vec3{x/l, y/l, z/l} : Vec3{}; }
};

struct AABB {
    Vec3 min, max;
    bool Contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y && p.z >= min.z && p.z <= max.z;
    }
    bool Intersects(const AABB& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
    Vec3 Center() const { return {(min.x+max.x)*0.5f, (min.y+max.y)*0.5f, (min.z+max.z)*0.5f}; }
    Vec3 Extents() const { return {(max.x-min.x)*0.5f, (max.y-min.y)*0.5f, (max.z-min.z)*0.5f}; }
};

struct RaycastHit {
    bool hit = false;
    float distance = 0.0f;
    Vec3 point;
    Vec3 normal;
    void* hitBody = nullptr;
};

enum class BodyType { Static, Dynamic, Kinematic };

struct RigidBody {
    uint32_t id = 0;
    BodyType type = BodyType::Static;

    Vec3 position;
    Vec3 linearVelocity;
    Vec3 acceleration;
    Vec3 force;

    float mass = 1.0f;
    float inverseMass = 1.0f;
    float restitution = 0.3f;  // Bounciness
    float friction = 0.5f;
    float linearDamping = 0.01f;

    AABB bounds;

    bool useGravity = true;
    bool isTrigger = false;
    void* userData = nullptr;
};

struct CollisionInfo {
    uint32_t bodyA;
    uint32_t bodyB;
    Vec3 normal;
    float penetration;
    Vec3 contactPoint;
};

using CollisionCallback = std::function<void(const CollisionInfo&)>;

class PhysicsCore {
public:
    static PhysicsCore& Get();

    void Initialize();
    void Shutdown();

    // Body management
    uint32_t CreateBody(const RigidBody& body);
    void DestroyBody(uint32_t id);
    RigidBody* GetBody(uint32_t id);
    void SetBodyBounds(uint32_t id, const AABB& bounds);

    // Simulation
    void Step(float deltaTime);
    void SetGravity(const Vec3& g) { gravity = g; }
    Vec3 GetGravity() const { return gravity; }

    // Collision queries
    bool Raycast(const Vec3& origin, const Vec3& direction, float maxDist, RaycastHit& outHit);
    bool RaycastAABB(const Vec3& origin, const Vec3& dir, const AABB& box, float& outT);
    bool CheckOverlap(const AABB& box, uint32_t ignoreId = 0);

    // Events
    void SetCollisionCallback(CollisionCallback cb) { collisionCallback = cb; }

    // Stats
    int GetBodyCount() const { return static_cast<int>(bodies.size()); }
    int GetCollisionCount() const { return collisionCount; }

private:
    PhysicsCore() = default;
    Vec3 gravity = { 0.0f, -980.0f, 0.0f };
    std::unordered_map<uint32_t, RigidBody> bodies;
    uint32_t nextId = 1;
    CollisionCallback collisionCallback;
    int collisionCount = 0;

    // Internal
    void IntegrateForces(float dt);
    void DetectCollisions();
    void ResolveCollision(const CollisionInfo& collision);
    void GenerateContactPoints(const RigidBody& a, const RigidBody& b, const Vec3& normal, float pen, CollisionInfo& out);
};

} // namespace NeoEngine
