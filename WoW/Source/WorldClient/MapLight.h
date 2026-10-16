#ifndef WOW_SOURCE_WORLDCLIENT_MAPLIGHT_H
#define WOW_SOURCE_WORLDCLIENT_MAPLIGHT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Model2/M2Light.h"
#include "WoW/Source/WorldClient/MapBaseObj.h"

class CMapLight : public CMapBaseObj {
    public:
    /* 0024 */ C3Vector m_position;
    /* 0030 */ CAaBox m_bbox;
    /* 0048 */ CAaSphere m_sphere;
    /* 0058 */ CM2Light m_light;
    /* 00C4 */ float unused_00C4;
    /* 00C8 */ float m_radius;
    /* 00CC */ float unk_00CC;
    /* 00D0 */ uint8_t unused_00D0;
    /* 00D1 */ uint8_t unk_00D1;
    /* 00D2 */ uint8_t unused_00D2;
    /* 00D3 */ uint8_t unused_00D3;

    CMapLight();
};

class CMapCacheLight {
    public:
    /* 0000 */ uint8_t unused_0000[132];
};

#endif
