#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4.h"

namespace NeoEngine {

struct Transform {
    Vector3 position{0.0f, 0.0f, 0.0f};
    Quaternion rotation{0.0f, 0.0f, 0.0f, 1.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};

    Matrix4 matrix() const {
        Matrix4 mat;

        // Calculate rotation matrix from quaternion
        float x2 = rotation.x + rotation.x;
        float y2 = rotation.y + rotation.y;
        float z2 = rotation.z + rotation.z;

        float xx = rotation.x * x2;
        float xy = rotation.x * y2;
        float xz = rotation.x * z2;
        float yy = rotation.y * y2;
        float yz = rotation.y * z2;
        float zz = rotation.z * z2;
        float wx = rotation.w * x2;
        float wy = rotation.w * y2;
        float wz = rotation.w * z2;

        // Row 0 / Column 0
        mat.m[0] = (1.0f - (yy + zz)) * scale.x;
        mat.m[1] = (xy + wz) * scale.x;
        mat.m[2] = (xz - wy) * scale.x;
        mat.m[3] = 0.0f;

        // Row 1 / Column 1
        mat.m[4] = (xy - wz) * scale.y;
        mat.m[5] = (1.0f - (xx + zz)) * scale.y;
        mat.m[6] = (yz + wx) * scale.y;
        mat.m[7] = 0.0f;

        // Row 2 / Column 2
        mat.m[8] = (xz + wy) * scale.z;
        mat.m[9] = (yz - wx) * scale.z;
        mat.m[10] = (1.0f - (xx + yy)) * scale.z;
        mat.m[11] = 0.0f;

        // Row 3 / Column 3 (Translation)
        mat.m[12] = position.x;
        mat.m[13] = position.y;
        mat.m[14] = position.z;
        mat.m[15] = 1.0f;

        return mat;
    }

    Vector3 TransformPosition(const Vector3& p) const {
        Vector3 scaled = { p.x * scale.x, p.y * scale.y, p.z * scale.z };
        Vector3 u = { rotation.x, rotation.y, rotation.z };
        float s = rotation.w;
        Vector3 rotated = u * (2.0f * u.Dot(scaled))
                        + scaled * (s * s - u.Dot(u))
                        + u.Cross(scaled) * (2.0f * s);
        return rotated + position;
    }
};

} // namespace NeoEngine
