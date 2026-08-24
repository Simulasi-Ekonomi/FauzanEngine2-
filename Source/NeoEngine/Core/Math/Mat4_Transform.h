#include "Core/Math/NeoMath.h"
#pragma once
#include "Matrix4.h"
#include "Quaternion.h"
#include "Vector3.h"

namespace NeoEngine {

inline Matrix4 Translate(float x, float y, float z) {
    Matrix4 m = Matrix4::IdentityMatrix();
    m.m[12] = x;
    m.m[13] = y;
    m.m[14] = z;
    return m;
}

inline Matrix4 Scale(float sx, float sy, float sz) {
    Matrix4 m = Matrix4::IdentityMatrix();
    m.m[0] = sx;
    m.m[5] = sy;
    m.m[10] = sz;
    return m;
}

inline Matrix4 Multiply(const Matrix4& a, const Matrix4& b) {
    Matrix4 r;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            r.m[i*4+j] = 0;
            for (int k = 0; k < 4; k++)
                r.m[i*4+j] += a.m[i*4+k] * b.m[k*4+j];
        }
    }
    return r;
}

inline Matrix4 FromQuat(const Quaternion& q) {
    Matrix4 m = Matrix4::IdentityMatrix();
    float x = q.x, y = q.y, z = q.z, w = q.w;
    m.m[0] = 1-2*y*y-2*z*z;   m.m[1] = 2*x*y-2*z*w;     m.m[2] = 2*x*z+2*y*w;
    m.m[4] = 2*x*y+2*z*w;     m.m[5] = 1-2*x*x-2*z*z;   m.m[6] = 2*y*z-2*x*w;
    m.m[8] = 2*x*z-2*y*w;     m.m[9] = 2*y*z+2*x*w;     m.m[10]= 1-2*x*x-2*y*y;
    return m;
}

inline Matrix4 Perspective(float fov, float aspect, float nearP, float farP) {
    Matrix4 m;
    float f = 1.0f / NeoEngine::Math::Tan(fov * 3.14159f / 360.0f);
    m.m[0] = f / aspect; m.m[5] = f;
    m.m[10] = (farP + nearP) / (nearP - farP);
    m.m[11] = -1.0f;
    m.m[14] = (2.0f * farP * nearP) / (nearP - farP);
    m.m[15] = 0.0f;
    return m;
}

} // namespace NeoEngine
