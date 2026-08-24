#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix4.h"

namespace NeoEngine {

// Alias untuk kompatibilitas singkat
using Vec3 = Vector3;
using Mat4 = Matrix4;

struct Transform {
    Vec3 position{0,0,0};
    Quaternion rotation;
    Vec3 scale{1,1,1};
    Mat4 model;

    void UpdateModelMatrix() {
        // Membuat model matrix dari posisi, rotasi, skala
        model = Matrix4::IdentityMatrix();
        // Skala
        model.m[0] = scale.x;
        model.m[5] = scale.y;
        model.m[10] = scale.z;
        // Rotasi (dari quaternion) – versi sederhana
        float x = rotation.x, y = rotation.y, z = rotation.z, w = rotation.w;
        model.m[0] = 1-2*y*y-2*z*z;   model.m[1] = 2*x*y-2*z*w;     model.m[2] = 2*x*z+2*y*w;
        model.m[4] = 2*x*y+2*z*w;     model.m[5] = 1-2*x*x-2*z*z;   model.m[6] = 2*y*z-2*x*w;
        model.m[8] = 2*x*z-2*y*w;     model.m[9] = 2*y*z+2*x*w;     model.m[10]= 1-2*x*x-2*y*y;
        // Posisi
        model.m[12] = position.x;
        model.m[13] = position.y;
        model.m[14] = position.z;
    }
};

} // namespace NeoEngine
