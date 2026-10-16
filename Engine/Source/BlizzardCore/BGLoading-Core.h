#ifndef ENGINE_SOURCE_BLIZZARDCORE_BGLOADING_CORE_H
#define ENGINE_SOURCE_BLIZZARDCORE_BGLOADING_CORE_H

#include <cstddef>
#include <cstdint>

namespace BackgroundCache {
class CacheManager {
    public:
    /* 0000 */ uint8_t unused_0000;
    /* 0001 */ uint8_t unused_0001[2];
    /* 0003 */ uint8_t unused_0003;
    /* 0004 */ uint8_t unused_0004;
    /* 0005 */ uint8_t unused_0005;
    /* 0006 */ uint8_t unused_0006;
    /* 0007 */ uint8_t unused_0007;
    /* 0008 */ uint8_t unused_0008;
    /* 0009 */ uint8_t unused_0009[2];
};
}

#endif
