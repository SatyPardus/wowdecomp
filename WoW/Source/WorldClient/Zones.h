#ifndef WOW_SOURCE_WORLDCLIENT_ZONES_H
#define WOW_SOURCE_WORLDCLIENT_ZONES_H

#include <cstdint>

class C2Vector;

struct ZonePath {
    /* 0000 */ C2Vector* m_points;
    /* 0004 */ int32_t m_pointCount;
    /* 0008 */ float m_offsetX;
    /* 000C */ float m_offsetY;
    /* 0010 */ const char* m_path;
    /* 0014 */ float m_minX;
    /* 0018 */ float m_minY;
    /* 001C */ float m_maxX;
    /* 0020 */ float m_maxY;

    ZonePath(const char* path);
    ~ZonePath();
};

struct Zone {
    /* 0000 */ int32_t m_mapID;
    /* 0004 */ int32_t unused_0004;
    /* 0008 */ int32_t m_lightID;
    /* 000C */ ZonePath m_zonePath;
};

#endif
