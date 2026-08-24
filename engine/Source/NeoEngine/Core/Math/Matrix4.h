#pragma once
#include "Vector3.h"
#include <cstring>
#include <cmath>

namespace NeoEngine {

struct Matrix4 {
    float m[16];

    Matrix4() { Identity(); }

    void Identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }

    static Matrix4 IdentityMatrix() { Matrix4 mat; return mat; }

    static Matrix4 Translation(const Vector3& pos) {
        Matrix4 mat;
        mat.m[12] = pos.x; mat.m[13] = pos.y; mat.m[14] = pos.z;
        return mat;
    }

    static Matrix4 Scale(const Vector3& scale) {
        Matrix4 mat;
        mat.m[0] = scale.x; mat.m[5] = scale.y; mat.m[10] = scale.z;
        return mat;
    }

    static Matrix4 Perspective(float fov, float aspect, float nearP, float farP) {
        Matrix4 mat;
        float f = 1.0f / std::tan(fov * 3.14159f / 360.0f);
        mat.m[0] = f / aspect; mat.m[5] = f;
        mat.m[10] = (farP + nearP) / (nearP - farP);
        mat.m[11] = -1.0f;
        mat.m[14] = (2.0f * farP * nearP) / (nearP - farP);
        mat.m[15] = 0.0f;
        return mat;
    }

    Matrix4 operator*(const Matrix4& other) const {
        Matrix4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i * 4 + j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i * 4 + j] += m[i * 4 + k] * other.m[k * 4 + j];
                }
            }
        }
        return result;
    }

    Vector3 TransformPoint(const Vector3& v) const {
        float w = m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15];
        return {
            (m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12]) / w,
            (m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13]) / w,
            (m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14]) / w
        };
    }

    float* Data() { return m; }
    const float* Data() const { return m; }
};

} // namespace NeoEngine
