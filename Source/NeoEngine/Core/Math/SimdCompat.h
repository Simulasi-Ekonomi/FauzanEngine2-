#pragma once

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>
inline bool simdAnyMask(uint32x4_t mask) {
    const uint32x2_t combined = vorr_u32(vget_low_u32(mask), vget_high_u32(mask));
    return (vget_lane_u32(combined, 0) | vget_lane_u32(combined, 1)) != 0;
}
#elif defined(__SSE2__)
#include <emmintrin.h>
#include <xmmintrin.h>

using float32x4_t = __m128;
using uint32x4_t = __m128i;

inline float32x4_t vdupq_n_f32(float value) { return _mm_set1_ps(value); }
inline float32x4_t vld1q_f32(const float* values) { return _mm_loadu_ps(values); }
inline void vst1q_f32(float* values, float32x4_t value) { _mm_storeu_ps(values, value); }
inline uint32x4_t vld1q_u32(const uint32_t* values) {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(values));
}
inline void vst1q_u32(uint32_t* values, uint32x4_t value) {
    _mm_storeu_si128(reinterpret_cast<__m128i*>(values), value);
}
inline float32x4_t vaddq_f32(float32x4_t a, float32x4_t b) { return _mm_add_ps(a, b); }
inline float32x4_t vsubq_f32(float32x4_t a, float32x4_t b) { return _mm_sub_ps(a, b); }
inline float32x4_t vmulq_f32(float32x4_t a, float32x4_t b) { return _mm_mul_ps(a, b); }
inline float32x4_t vnegq_f32(float32x4_t value) { return _mm_sub_ps(_mm_setzero_ps(), value); }
inline float32x4_t vmaxq_f32(float32x4_t a, float32x4_t b) { return _mm_max_ps(a, b); }
inline float32x4_t vminq_f32(float32x4_t a, float32x4_t b) { return _mm_min_ps(a, b); }
inline float32x4_t vrecpeq_f32(float32x4_t value) { return _mm_rcp_ps(value); }
inline float32x4_t vrsqrteq_f32(float32x4_t value) { return _mm_rsqrt_ps(value); }
inline float32x4_t vrecpsq_f32(float32x4_t value, float32x4_t estimate) {
    return _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(value, estimate));
}
inline uint32x4_t vcltq_f32(float32x4_t a, float32x4_t b) {
    return _mm_castps_si128(_mm_cmplt_ps(a, b));
}
inline uint32x4_t vcgtq_f32(float32x4_t a, float32x4_t b) {
    return _mm_castps_si128(_mm_cmpgt_ps(a, b));
}
inline uint32x4_t vandq_u32(uint32x4_t a, uint32x4_t b) { return _mm_and_si128(a, b); }
inline uint32x4_t vorr_u32(uint32x4_t a, uint32x4_t b) { return _mm_or_si128(a, b); }
inline float32x4_t vbslq_f32(uint32x4_t mask, float32x4_t onTrue, float32x4_t onFalse) {
    const __m128 selectedTrue = _mm_and_ps(_mm_castsi128_ps(mask), onTrue);
    const __m128 selectedFalse = _mm_andnot_ps(_mm_castsi128_ps(mask), onFalse);
    return _mm_or_ps(selectedTrue, selectedFalse);
}
inline bool simdAnyMask(uint32x4_t mask) { return _mm_movemask_ps(_mm_castsi128_ps(mask)) != 0; }
#else
struct float32x4_t { float lane[4]; };
struct uint32x4_t { uint32_t lane[4]; };
inline float32x4_t vdupq_n_f32(float value) { return {{value, value, value, value}}; }
inline float32x4_t vld1q_f32(const float* values) { return {{values[0], values[1], values[2], values[3]}}; }
inline void vst1q_f32(float* values, float32x4_t value) { for (int i = 0; i < 4; ++i) values[i] = value.lane[i]; }
inline uint32x4_t vld1q_u32(const uint32_t* values) { return {{values[0], values[1], values[2], values[3]}}; }
inline void vst1q_u32(uint32_t* values, uint32x4_t value) { for (int i = 0; i < 4; ++i) values[i] = value.lane[i]; }
inline float32x4_t vaddq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] + b.lane[i]; return r; }
inline float32x4_t vsubq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] - b.lane[i]; return r; }
inline float32x4_t vmulq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] * b.lane[i]; return r; }
inline float32x4_t vnegq_f32(float32x4_t a) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = -a.lane[i]; return r; }
inline float32x4_t vmaxq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] > b.lane[i] ? a.lane[i] : b.lane[i]; return r; }
inline float32x4_t vminq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] < b.lane[i] ? a.lane[i] : b.lane[i]; return r; }
inline float32x4_t vrecpeq_f32(float32x4_t a) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = 1.0f / a.lane[i]; return r; }
inline float32x4_t vrsqrteq_f32(float32x4_t a) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = 1.0f / __builtin_sqrtf(a.lane[i]); return r; }
inline float32x4_t vrecpsq_f32(float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = 2.0f - a.lane[i] * b.lane[i]; return r; }
inline uint32x4_t vcltq_f32(float32x4_t a, float32x4_t b) { uint32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] < b.lane[i] ? UINT32_MAX : 0; return r; }
inline uint32x4_t vcgtq_f32(float32x4_t a, float32x4_t b) { uint32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] > b.lane[i] ? UINT32_MAX : 0; return r; }
inline uint32x4_t vandq_u32(uint32x4_t a, uint32x4_t b) { uint32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] & b.lane[i]; return r; }
inline uint32x4_t vorr_u32(uint32x4_t a, uint32x4_t b) { uint32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = a.lane[i] | b.lane[i]; return r; }
inline float32x4_t vbslq_f32(uint32x4_t mask, float32x4_t a, float32x4_t b) { float32x4_t r{}; for (int i = 0; i < 4; ++i) r.lane[i] = mask.lane[i] ? a.lane[i] : b.lane[i]; return r; }
inline bool simdAnyMask(uint32x4_t mask) { return (mask.lane[0] | mask.lane[1] | mask.lane[2] | mask.lane[3]) != 0; }
#endif
