#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <limits>

namespace NeoEngine {

struct Vec3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 Cross(const Vec3& o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 Normalized() const { float l = Length(); return l > 0.0001f ? Vec3{x/l, y/l, z/l} : Vec3{}; }
};

struct AABB {
    Vec3 min, max;
    Vec3 Center() const { return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f, (min.z + max.z) * 0.5f}; }
    Vec3 Extents() const { return {(max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f, (max.z - min.z) * 0.5f}; }
};

struct CollisionResult {
    bool hit = false;
    Vec3 normal;
    float depth = 0.0f;
    Vec3 contactPoint;
};

class CollisionSAT {
public:
    static bool TestAABBvsAABB(const AABB& a, const AABB& b) {
        return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
               (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
               (a.min.z <= b.max.z && a.max.z >= b.min.z);
    }

    static CollisionResult ResolveAABBvsAABB(const AABB& a, const AABB& b) {
        CollisionResult result;
        Vec3 centerA = a.Center();
        Vec3 centerB = b.Center();
        Vec3 diff = centerB - centerA;
        Vec3 extentsA = a.Extents();
        Vec3 extentsB = b.Extents();

        float overlapX = (extentsA.x + extentsB.x) - std::abs(diff.x);
        float overlapY = (extentsA.y + extentsB.y) - std::abs(diff.y);
        float overlapZ = (extentsA.z + extentsB.z) - std::abs(diff.z);

        if (overlapX <= 0 || overlapY <= 0 || overlapZ <= 0) {
            return result;
        }

        result.hit = true;
        if (overlapX <= overlapY && overlapX <= overlapZ) {
            result.normal = diff.x > 0 ? Vec3{1,0,0} : Vec3{-1,0,0};
            result.depth = overlapX;
        } else if (overlapY <= overlapX && overlapY <= overlapZ) {
            result.normal = diff.y > 0 ? Vec3{0,1,0} : Vec3{0,-1,0};
            result.depth = overlapY;
        } else {
            result.normal = diff.z > 0 ? Vec3{0,0,1} : Vec3{0,0,-1};
            result.depth = overlapZ;
        }
        result.contactPoint = centerA + diff.Normalized() * (extentsA.x - result.depth * 0.5f);
        return result;
    }

    static bool TestSphereVsSphere(const Vec3& c1, float r1, const Vec3& c2, float r2) {
        float distSq = (c2 - c1).Dot(c2 - c1);
        float radiusSum = r1 + r2;
        return distSq <= radiusSum * radiusSum;
    }

    static bool TestRayVsAABB(const Vec3& origin, const Vec3& dir, const AABB& box, float& outT) {
        float tmin = 0.0f, tmax = std::numeric_limits<float>::max();
        for (int i = 0; i < 3; i++) {
            float o = (&origin.x)[i];
            float d = (&dir.x)[i];
            float bmin = (&box.min.x)[i];
            float bmax = (&box.max.x)[i];
            if (std::abs(d) < 0.00001f) {
                if (o < bmin || o > bmax) return false;
            } else {
                float t1 = (bmin - o) / d;
                float t2 = (bmax - o) / d;
                if (t1 > t2) std::swap(t1, t2);
                tmin = std::max(tmin, t1);
                tmax = std::min(tmax, t2);
                if (tmin > tmax) return false;
            }
        }
        outT = tmin;
        return true;
    }
};

} // namespace NeoEngine
