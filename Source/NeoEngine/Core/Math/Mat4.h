#pragma once
#include <cstddef>
#if defined(__aarch64__) || defined(__arm__)
#include <arm_neon.h>
#endif

namespace NeoEngine {

struct alignas(16) Mat4 {
    float m[16];

    Mat4() {
        for (int i = 0; i < 16; ++i) m[i] = 0.0f;
    }
    static Mat4 Identity();
};

Mat4 MulMat4(const Mat4& a, const Mat4& b);
void TransformPoint3(const Mat4& mat, const float* in, float* out);

// Alias
using Matrix4 = Mat4;

} // namespace NeoEngine
