#ifndef WOW_SOURCE_MAGIC_SPELLCAST_H
#define WOW_SOURCE_MAGIC_SPELLCAST_H

#include <cstddef>
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#endif

class SpellCast {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint8_t unused_0008 [304];
    /* 0138 */ uint32_t unused_0138;
    /* 013C */ uint32_t m_dwordC;
    /* 0140 */ uint32_t unused_0140;
    /* 0144 */ uint32_t unused_0144;
};

#endif
