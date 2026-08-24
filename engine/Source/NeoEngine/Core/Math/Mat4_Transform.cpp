#include "Mat4.h"
#include <cstring>
#ifdef __aarch64__
#include <arm_neon.h>
#endif

namespace NeoEngine {

Mat4 Mat4::Identity() {
    Mat4 res;
    res.m[0] = 1.0f; res.m[5] = 1.0f; res.m[10] = 1.0f; res.m[15] = 1.0f;
    return res;
}

Mat4 MulMat4(const Mat4& a, const Mat4& b) {
    Mat4 result;
#ifdef __aarch64__
    for (int col = 0; col < 4; ++col) {
        float32x4_t col_b = vld1q_f32(b.m + col * 4);
        float32x4_t res_col = vmulq_n_f32(vld1q_f32(a.m), vgetq_lane_f32(col_b, 0));
        res_col = vmlaq_n_f32(res_col, vld1q_f32(a.m + 4), vgetq_lane_f32(col_b, 1));
        res_col = vmlaq_n_f32(res_col, vld1q_f32(a.m + 8), vgetq_lane_f32(col_b, 2));
        res_col = vmlaq_n_f32(res_col, vld1q_f32(a.m + 12), vgetq_lane_f32(col_b, 3));
        vst1q_f32(result.m + col * 4, res_col);
    }
#else
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) sum += a.m[row + k * 4] * b.m[k + col * 4];
            result.m[row + col * 4] = sum;
        }
    }
#endif
    return result;
}

void TransformPoint3(const Mat4& mat, const float* in, float* out) {
#ifdef __aarch64__
    float32x4_t point = vld1q_f32(in);
    float32x4_t row0 = vld1q_f32(mat.m);
    float32x4_t row1 = vld1q_f32(mat.m + 4);
    float32x4_t row2 = vld1q_f32(mat.m + 8);
    float32x4_t row3 = vld1q_f32(mat.m + 12);
    float32x4_t res = vmulq_laneq_f32(row0, point, 0);
    res = vmlaq_laneq_f32(res, row1, point, 1);
    res = vmlaq_laneq_f32(res, row2, point, 2);
    res = vmlaq_laneq_f32(res, row3, point, 3);
    vst1q_f32(out, res);
#else
    float x = mat.m[0]*in[0] + mat.m[4]*in[1] + mat.m[8]*in[2]  + mat.m[12];
    float y = mat.m[1]*in[0] + mat.m[5]*in[1] + mat.m[9]*in[2]  + mat.m[13];
    float z = mat.m[2]*in[0] + mat.m[6]*in[1] + mat.m[10]*in[2] + mat.m[14];
    float w = mat.m[3]*in[0] + mat.m[7]*in[1] + mat.m[11]*in[2] + mat.m[15];
    out[0] = x/w; out[1] = y/w; out[2] = z/w;
#endif
}

} // namespace
