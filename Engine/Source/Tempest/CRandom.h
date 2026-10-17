#ifndef ENGINE_SOURCE_TEMPEST_CRANDOM_H
#define ENGINE_SOURCE_TEMPEST_CRANDOM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Platform/CallingConvention.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"

class CRndSeed {
    public:
    /* 0000 */ uint32_t rndacc;
    /* 0004 */ uint32_t rndvls;

    CRndSeed() = default;
    CRndSeed(uint32_t seed);
    void SetSeed(uint32_t seed);
};

class CRandom {
    public:
    static C2Vector C2Vector_(CRndSeed& seed);
    static C3Vector C3Vector_(CRndSeed& seed);
    static uint32_t dice_(uint32_t sides, CRndSeed& seed);
    static float sfloat_(CRndSeed& seed);
    static float WOW_STDCALL ufloat_(float range, CRndSeed& seed);
    static float ufloat_(CRndSeed& seed);
    static uint32_t uint32_(CRndSeed& seed);
};

extern CRndSeed g_rndSeed;

#endif
