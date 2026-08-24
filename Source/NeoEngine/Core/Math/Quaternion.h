#pragma once
// Tidak ada include math.h untuk menghindari polusi namespace

// Deklarasi fungsi matematika dari C runtime
extern "C" float sqrtf(float);
extern "C" float sinf(float);
extern "C" float cosf(float);
extern "C" float tanf(float);
extern "C" float atan2f(float, float);

namespace NeoEngine {

struct Vector3; // forward declaration
struct Quaternion {
    float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f;
    Quaternion() = default;
    Quaternion(float ix, float iy, float iz, float iw) : x(ix), y(iy), z(iz), w(iw) {}
    Quaternion(const Vector3& v, float iw);

    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w*q.x + x*q.w + y*q.z - z*q.y,
            w*q.y - x*q.z + y*q.w + z*q.x,
            w*q.z + x*q.y - y*q.x + z*q.w,
            w*q.w - x*q.x - y*q.y - z*q.z
        );
    }

    static Quaternion FromEuler(float pitch, float yaw, float roll) {
        float cy = cosf(yaw * 0.5f), sy = sinf(yaw * 0.5f);
        float cp = cosf(pitch * 0.5f), sp = sinf(pitch * 0.5f);
        float cr = cosf(roll * 0.5f), sr = sinf(roll * 0.5f);
        return Quaternion(
            sr * cp * cy - cr * sp * sy,
            cr * sp * cy + sr * cp * sy,
            cr * cp * sy - sr * sp * cy,
            cr * cp * cy + sr * sp * sy
        );
    }
};

} // namespace NeoEngine
