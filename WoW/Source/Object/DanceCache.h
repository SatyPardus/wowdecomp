#ifndef WOW_SOURCE_OBJECT_DANCECACHE_H
#define WOW_SOURCE_OBJECT_DANCECACHE_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

class CDataStore;

struct DANCEMOVE {
    /* 0000 */ int16_t m_moveId;
    /* 0002 */ uint8_t m_chance;
    /* 0003 */ uint8_t m_type;
};

class DanceCache {
    public:
    static const uint32_t s_cacheVersion = 1;

    /* 0000 */ int32_t m_id;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ char m_name[128];
    /* 0090 */ DANCEMOVE* m_data;
    /* 0094 */ uint32_t m_dataCount;
    /* 0098 */ uint32_t m_version;
    /* 009C */ uint32_t unused_009C;

    DanceCache();
    ~DanceCache();
    DanceCache& operator=(const DanceCache& source);
    void Pack(CDataStore* msg);
    void Unpack(CDataStore* msg);
};

#endif
