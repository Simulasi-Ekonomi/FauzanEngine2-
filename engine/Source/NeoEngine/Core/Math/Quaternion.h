#pragma once
#include "Vector3.h"
#include <cmath>

namespace NeoEngine {

struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;

    Quaternion() = default;
    Quaternion(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}

    static Quaternion Identity() { return {0, 0, 0, 1}; }

    static Quaternion FromEuler(float pitch, float yaw, float roll) {
        float cy = std::cos(yaw * 0.5f), sy = std::sin(yaw * 0.5f);
        float cp = std::cos(pitch * 0.5f), sp = std::sin(pitch * 0.5f);
        float cr = std::cos(roll * 0.5f), sr = std::sin(roll * 0.5f);
        return {
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        };
    }

    Quaternion operator*(const Quaternion& q) const {
        return {
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z
        };
    }

    Vector3 Rotate(const Vector3& v) const {
        Vector3 u{x, y, z};
        float s = w;
        return u * (2.0f * u.Dot(v)) + v * (s * s - u.Dot(u)) + u.Cross(v) * (2.0f * s);
    }

    void Normalize() {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len > 0) { x /= len; y /= len; z /= len; w /= len; }
    }
};

} // namespace NeoEngine
