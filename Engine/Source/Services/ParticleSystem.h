#ifndef ENGINE_SOURCE_SERVICES_PARTICLESYSTEM_H
#define ENGINE_SOURCE_SERVICES_PARTICLESYSTEM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class CParticle {
    public:
    /* 0000 */ float m_age;
    /* 0004 */ uint8_t unused_0004[0x24];

    ~CParticle();
};

class CParticleEmitter {
    public:
    /* 0000 */ int32_t m_refCount;
    /* 0004 */ uint8_t unused_0004[0x2C];
    /* 0030 */ TSGrowableArray<CParticle> m_particles;
    /* 0040 */ TSGrowableArray<uint32_t> m_liveList;
    /* 0050 */ TSGrowableArray<uint32_t> m_freeList;

    ~CParticleEmitter();
    void DecRef();
};

#endif
