#ifndef WOW_SOURCE_GAME_TYPES_H
#define WOW_SOURCE_GAME_TYPES_H

#include <cstddef>
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#endif

struct LoadedWardenModule {
    /* 0000 */ void* m_imageBase;
    /* 0004 */ uint32_t m_imageSize;
    /* 0008 */ uint32_t m_dllsLoaded;
};

#endif
