#pragma once
#include <cmath>

namespace NeoEngine {

struct Vector3 {
    float x = 0, y = 0, z = 0;

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vector3 operator/(float s) const { return {x / s, y / s, z / s}; }

    float Dot(const Vector3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vector3 Cross(const Vector3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float LengthSquared() const { return x * x + y * y + z * z; }

    Vector3 Normalized() const {
        float l = Length();
        return l > 0.0001f ? Vector3{x / l, y / l, z / l} : Vector3{};
    }

    float Distance(const Vector3& o) const { return (*this - o).Length(); }

    static Vector3 Zero() { return {0, 0, 0}; }
    static Vector3 One() { return {1, 1, 1}; }
    static Vector3 Up() { return {0, 1, 0}; }
    static Vector3 Forward() { return {0, 0, -1}; }
    static Vector3 Right() { return {1, 0, 0}; }

    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t) {
        return a + (b - a) * t;
    }

    float* Data() { return &x; }
    const float* Data() const { return &x; }
};

} // namespace NeoEngine
