#pragma once
#include "Vector3.h"
#include "Quaternion.h"
#include "Matrix4.h"
// Tidak ada include math.h

namespace NeoEngine {

struct Transform {
    Vector3 position;
    Quaternion rotation;
    Vector3 scale{1,1,1};
    Matrix4 matrix() const {
        // Kode penyusunan matrix tanpa fungsi matematika
        Matrix4 m;
        // Placeholder: implementasikan sesuai kebutuhan
        return m;
    }
};

} // namespace NeoEngine
