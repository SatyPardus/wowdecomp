#ifndef ENGINE_SOURCE_TEMPEST_CMATH_H
#define ENGINE_SOURCE_TEMPEST_CMATH_H

#include <cmath>
#include <cstddef>
#include <cstdint>

static const float PI = 3.1415927f;
static const float TWO_PI = 6.2831855f;

class CMath {
    public:
    static float cos_(float x);
    static double exp_(float x);
    static double exp2_(double x);
    static bool fequal(float a, float b);
    static int32_t fint_(float f);
    static int32_t fint_n(float f);
    static int32_t ftrunc(float f);
    static int32_t ftol_round_0_255_(float f);
    static double log2_(double x);
    static bool fnotequal(float a, float b);
    static float normalizeAnglePi(float angle);
    static float normalizeAngleNegPiToPi_(float angle);
    static float normalizeangle0to2pi_(float angle);
    static float sin_(float x);
    static bool solvequad_(float a, float b, float c, float& r0, float& r1);
    static void split_(float x, float* frac, int32_t* whole);
    static void sincos_(float x, float* s, float* c);
    static uint32_t fuint_pi(float f);
    static uint32_t sqrt_(uint32_t n);
};

class CSoftFloat {
    public:
    static CSoftFloat Ceil(const CSoftFloat& value);
    static uint32_t CountLeadingZeros(uint32_t value);
    static CSoftFloat Floor(const CSoftFloat& value);
    static CSoftFloat FromInt(int32_t value);
    static CSoftFloat FromRatio(int32_t numerator, int32_t denominator);
    static CSoftFloat Reciprocal(const CSoftFloat& value);
    static CSoftFloat Round(const CSoftFloat& value);

    /* 0000 */ uint32_t m_bits;
};

CSoftFloat operator*(const CSoftFloat& l, const CSoftFloat& r);
CSoftFloat operator+(const CSoftFloat& l, const CSoftFloat& r);

float fclamp(float value, float min, float max);
bool fequal(float a, float b);
int32_t fequalz(float a, float b, float z);
int32_t Clamp(int32_t value, int32_t min, int32_t max);
int32_t fnotequalz(float a, float b, float z);
uint32_t CeilToInt64(float value);
int32_t FloatToInt64(float value);
int32_t RoundToInt64(float value);
void ClampInPlace(float& value, float min, float max);
bool fnotequal(float a, float b);
int32_t IsNaN(double value);

// VERIFY: 03d90c2d auto
// OFFSET: 0x005FFF80
inline float CMath::cos_(float x) {
    float frac;
    int32_t whole;
    CMath::split_(x * 0.31830987f, &frac, &whole);

    float result = 1.0f - frac * ((6.0f - 4.0f * frac) * frac);

    return (whole & 1) ? -result : result;
}

// INLINED
inline int32_t CMath::fint_(float f) {
    return static_cast<int32_t>(lrintf(f));
}

// INLINED
inline int32_t CMath::ftol_round_0_255_(float f) {
    return static_cast<int32_t>(f * 255.0f + 0.5f);
}

float OrganicSmooth(float from, float to, float t);

#endif
