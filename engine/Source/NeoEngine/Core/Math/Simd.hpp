#pragma once

// EOF: Auto-detect ARM NEON untuk Android atau SSE untuk Desktop
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
    typedef float32x4_t simd4;
    #define SIMD_LOAD(p) vld1q_f32(p)
    #define SIMD_STORE(p, v) vst1q_f32(p, v)
    #define SIMD_ADD(a, b) vaddq_f32(a, b)
    #define SIMD_MUL(a, b) vmulq_f32(a, b)
#elif defined(__SSE__)
    #include <xmmintrin.h>
    typedef __m128 simd4;
    #define SIMD_LOAD(p) _mm_loadu_ps(p)
    #define SIMD_STORE(p, v) _mm_storeu_ps(p, v)
    #define SIMD_ADD(a, b) _mm_add_ps(a, b)
    #define SIMD_MUL(a, b) _mm_mul_ps(a, b)
#else
    // Fallback untuk CPU jadul
    struct simd4 { float v[4]; };
    inline simd4 SIMD_LOAD(const float* p) { return {p[0], p[1], p[2], p[3]}; }
    inline void SIMD_STORE(float* p, simd4 v) { for(int i=0; i<4; i++) p[i]=v.v[i]; }
    inline simd4 SIMD_ADD(simd4 a, simd4 b) { simd4 r; for(int i=0; i<4; i++) r.v[i]=a.v[i]+b.v[i]; return r; }
    inline simd4 SIMD_MUL(simd4 a, simd4 b) { simd4 r; for(int i=0; i<4; i++) r.v[i]=a.v[i]*b.v[i]; return r; }
#endif
