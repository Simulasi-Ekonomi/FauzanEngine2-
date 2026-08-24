#pragma once
#include <cstring>
// Tidak ada include math.h

namespace NeoEngine {

struct Matrix4 {
    float m[16];
    Matrix4() { Identity(); }
    void Identity() {
        std::memset(m, 0, sizeof(m));
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    static Matrix4 IdentityMatrix() { Matrix4 mat; return mat; }
};

} // namespace NeoEngine
