#ifndef WOWDECOMP_PLATFORM_COMPAT_SSE_H
#define WOWDECOMP_PLATFORM_COMPAT_SSE_H

#if defined(__SSE__)
#include <xmmintrin.h>
#elif defined(__ARM_NEON) || defined(__aarch64__)
#include <arm_neon.h>
using __m128 = float32x4_t;

inline __m128 _mm_set1_ps(float value) {
    return vdupq_n_f32(value);
}

inline __m128 _mm_set_ss(float value) {
    return vsetq_lane_f32(value, vdupq_n_f32(0.0f), 0);
}

inline __m128 _mm_load_ps(const float* value) {
    return vld1q_f32(value);
}

inline __m128 _mm_loadu_ps(const float* value) {
    return vld1q_f32(value);
}

inline __m128 _mm_mul_ps(__m128 left, __m128 right) {
    return vmulq_f32(left, right);
}

inline __m128 _mm_add_ps(__m128 left, __m128 right) {
    return vaddq_f32(left, right);
}

inline void _mm_store_ps(float* destination, __m128 value) {
    vst1q_f32(destination, value);
}

inline void _mm_storeu_ps(float* destination, __m128 value) {
    vst1q_f32(destination, value);
}

inline __m128 _mm_rsqrt_ss(__m128 value) {
    return vcombine_f32(vrsqrte_f32(vget_low_f32(value)), vget_high_f32(value));
}

inline float _mm_cvtss_f32(__m128 value) {
    return vgetq_lane_f32(value, 0);
}
#else
#include <cmath>
struct __m128 {
    float lane[4];
};

inline __m128 _mm_set1_ps(float value) {
    return { { value, value, value, value } };
}

inline __m128 _mm_set_ss(float value) {
    return { { value, 0.0f, 0.0f, 0.0f } };
}

inline __m128 _mm_load_ps(const float* value) {
    return { { value[0], value[1], value[2], value[3] } };
}

inline __m128 _mm_loadu_ps(const float* value) {
    return _mm_load_ps(value);
}

inline __m128 _mm_mul_ps(__m128 left, __m128 right) {
    return { { left.lane[0] * right.lane[0], left.lane[1] * right.lane[1], left.lane[2] * right.lane[2], left.lane[3] * right.lane[3] } };
}

inline __m128 _mm_add_ps(__m128 left, __m128 right) {
    return { { left.lane[0] + right.lane[0], left.lane[1] + right.lane[1], left.lane[2] + right.lane[2], left.lane[3] + right.lane[3] } };
}

inline void _mm_store_ps(float* destination, __m128 value) {
    for (int i = 0; i < 4; i++) {
        destination[i] = value.lane[i];
    }
}

inline void _mm_storeu_ps(float* destination, __m128 value) {
    _mm_store_ps(destination, value);
}

inline __m128 _mm_rsqrt_ss(__m128 value) {
    value.lane[0] = 1.0f / std::sqrt(value.lane[0]);
    return value;
}

inline float _mm_cvtss_f32(__m128 value) {
    return value.lane[0];
}
#endif

#endif
