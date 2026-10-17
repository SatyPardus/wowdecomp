#ifndef ENGINE_SOURCE_MODEL2_M2VECTOR_H
#define ENGINE_SOURCE_MODEL2_M2VECTOR_H

#include <cstdint>
#if defined(__aarch64__) || defined(_M_ARM64)
#include <arm_neon.h>
#else
#include <xmmintrin.h>
#endif

// Four matrix lanes shared by CPU skinning and hit testing. Keep multiply and
// add separate: the original SSE path does not fuse weighted bone operations.
namespace M2Vector {
#if defined(__aarch64__) || defined(_M_ARM64)
using Float4 = float32x4_t;
inline bool Available(uint32_t) { return true; }
inline Float4 Splat(float x) { return vdupq_n_f32(x); }
inline Float4 Load(const float* p) { return vld1q_f32(p); }
inline void Store(float* p, Float4 x) { vst1q_f32(p, x); }
inline Float4 Multiply(Float4 a, Float4 b) { return vmulq_f32(a, b); }
inline Float4 Add(Float4 a, Float4 b) { return vaddq_f32(a, b); }
#else
using Float4 = __m128;
inline bool Available(uint32_t features) { return (features & 4) != 0; }
inline Float4 Splat(float x) { return _mm_set1_ps(x); }
inline Float4 Load(const float* p) { return _mm_loadu_ps(p); }
inline void Store(float* p, Float4 x) { _mm_storeu_ps(p, x); }
inline Float4 Multiply(Float4 a, Float4 b) { return _mm_mul_ps(a, b); }
inline Float4 Add(Float4 a, Float4 b) { return _mm_add_ps(a, b); }
#endif
}
#endif
