#include "Core/Math/NeoMath.h"
#pragma once
#include <cstdint>
#include <arm_neon.h>  // Untuk NEON di Android

namespace NeoEngine {

// Wrapper SIMD untuk operasi vektor (ARM NEON)
struct SIMDVec4 {
    float32x4_t data;

    SIMDVec4() : data(vdupq_n_f32(0.0f)) {}
    SIMDVec4(float x, float y, float z, float w) {
        float values[4] = {x, y, z, w};
        data = vld1q_f32(values);
    }
    SIMDVec4(float32x4_t v) : data(v) {}

    SIMDVec4 operator+(const SIMDVec4& o) const { return vaddq_f32(data, o.data); }
    SIMDVec4 operator-(const SIMDVec4& o) const { return vsubq_f32(data, o.data); }
    SIMDVec4 operator*(float s) const { return vmulq_n_f32(data, s); }
    SIMDVec4 operator*(const SIMDVec4& o) const { return vmulq_f32(data, o.data); }
    float Dot(const SIMDVec4& o) const {
        float32x4_t prod = vmulq_f32(data, o.data);
        float32x2_t sum = vadd_f32(vget_low_f32(prod), vget_high_f32(prod));
        sum = vpadd_f32(sum, sum);
        return vget_lane_f32(sum, 0);
    }
    float Length() const { return NeoEngine::Math::Sqrt(Dot(*this)); }
    SIMDVec4 Normalized() const {
        float len = Length();
        return (len > 0.0001f) ? *this * (1.0f / len) : SIMDVec4();
    }
};

// Fungsi akselerasi batch untuk ECS SoA
void MoveEntitiesSIMD(float* posX, float* posY, float* posZ,
                      const float* velX, const float* velY, const float* velZ,
                      float dt, int count) {
    for (int i = 0; i + 3 < count; i += 4) {
        float32x4_t px = vld1q_f32(posX + i);
        float32x4_t py = vld1q_f32(posY + i);
        float32x4_t pz = vld1q_f32(posZ + i);
        float32x4_t vx = vld1q_f32(velX + i);
        float32x4_t vy = vld1q_f32(velY + i);
        float32x4_t vz = vld1q_f32(velZ + i);
        float32x4_t dtv = vdupq_n_f32(dt);
        vst1q_f32(posX + i, vmlaq_f32(px, vx, dtv));
        vst1q_f32(posY + i, vmlaq_f32(py, vy, dtv));
        vst1q_f32(posZ + i, vmlaq_f32(pz, vz, dtv));
    }
    // Sisa entitas (kurang dari 4)
    for (int i = (count / 4) * 4; i < count; ++i) {
        posX[i] += velX[i] * dt;
        posY[i] += velY[i] * dt;
        posZ[i] += velZ[i] * dt;
    }
}

} // namespace NeoEngine
