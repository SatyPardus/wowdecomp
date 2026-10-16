#ifndef ENGINE_SOURCE_GX_CGXDEVICE_EMERGENCYMEM_H
#define ENGINE_SOURCE_GX_CGXDEVICE_EMERGENCYMEM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class EmergencyMem {
    public:
    /* 0000 */ TSGrowableArray<uint8_t> m_data;
    /* 0010 */ bool m_lock;
    /* 0011 */ uint8_t unused_0011[3];

    EmergencyMem();
    void* Lock(uint32_t size);
    void Unlock();
};

#endif
