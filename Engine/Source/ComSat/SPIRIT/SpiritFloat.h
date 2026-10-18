#ifndef ENGINE_SOURCE_COMSAT_SPIRIT_SPIRITFLOAT_H
#define ENGINE_SOURCE_COMSAT_SPIRIT_SPIRITFLOAT_H

#include <cmath>
#include <cstdint>
#include <limits>
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace Spirit {
inline float PitchInverseRoot(float energy) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(energy)));
#else
    // Preserve the normalized-correlation operation. SSE's approximate rsqrt
    // is not an arm64 instruction; use native scalar sqrt/division rather than
    // the lower-precision ARM estimate. Pitch choices near a decision boundary
    // can differ from x86 and require codec-level comparison.
    if (std::fpclassify(energy) == FP_SUBNORMAL) energy = std::copysign(0.0f, energy);
    return 1.0f / std::sqrt(energy);
#endif
}

inline int32_t RoundedPCM(float sample) {
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    return _mm_cvtss_si32(_mm_set_ss(sample));
#else
    // CVTSS2SI uses the active rounding mode (normally nearest/even), and
    // produces INT32_MIN for NaN or an out-of-range integer conversion.
    const float rounded = std::nearbyint(sample);
    if (!std::isfinite(rounded) || rounded >= 2147483648.0f || rounded < -2147483648.0f)
        return std::numeric_limits<int32_t>::min();
    return static_cast<int32_t>(rounded);
#endif
}
}
#endif
