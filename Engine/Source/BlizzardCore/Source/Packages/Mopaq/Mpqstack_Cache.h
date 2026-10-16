#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_MPQSTACK_CACHE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_MPQSTACK_CACHE_H

#include <cstddef>
#include <cstdint>

namespace System_Mopaq {
struct CacheData {
    /* 0000 */ uint8_t unused_0000[4];
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint8_t unused_0008[16];
    /* 0018 */ uint64_t unused_0018;
    /* 0020 */ uint32_t unused_0020;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ uint32_t unused_0028;
    /* 002C */ uint8_t unused_002C[1];
    /* 002D */ uint8_t unused_002D;
    /* 002E */ uint8_t unused_002E[10];
    /* 0038 */ uint32_t unused_0038;
    /* 003C */ uint32_t unused_003C;
    /* 0040 */ uint8_t unused_0040[20];
    /* 0054 */ uint32_t unused_0054;
};
}

#endif
