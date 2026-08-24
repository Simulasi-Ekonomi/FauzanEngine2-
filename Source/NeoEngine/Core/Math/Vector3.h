#include "Core/Math/NeoMath.h"
#pragma once
// Tidak ada include math.h untuk menghindari polusi

extern "C" float sqrtf(float);

namespace NeoEngine {

struct Vector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;
    Vector3() = default;
    Vector3(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
    Vector3 operator+(const Vector3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vector3 operator-(const Vector3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vector3 operator*(float s) const { return {x*s, y*s, z*s}; }
    Vector3 operator/(float s) const { return {x/s, y/s, z/s}; }
    float Dot(const Vector3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vector3 Cross(const Vector3& o) const {
        return { y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x };
    }
    float Length() const { return sqrtf(x*x + y*y + z*z); }
    Vector3 Normalized() const {
        float l = Length();
        return (l > 0.0001f) ? Vector3{x/l, y/l, z/l} : Vector3{};
    }
};

using Vec3 = Vector3;   // alias pendek untuk kompatibilitas

} // namespace NeoEngine
