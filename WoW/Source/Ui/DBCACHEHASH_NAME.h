#ifndef WOW_SOURCE_UI_DBCACHEHASH_NAME_H
#define WOW_SOURCE_UI_DBCACHEHASH_NAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

struct DBCACHETRAILER_GUID {
    /* 0000 */ uint64_t m_key;
    /* 0008 */ uint8_t m_loaded;
    /* 0009 */ uint8_t unused_0009 [3];
    /* 000C */ TSList<void> m_callbacks;
    /* 0018 */ uint8_t m_noSave;
    /* 0019 */ uint8_t m_dispatching;
    /* 001A */ uint8_t m_invalidatePending;
    /* 001B */ uint8_t unused_001B;
    /* 001C */ TSLink<void> m_pendingLink;
    /* 0024 */ uint32_t m_pad24;
};

#endif
