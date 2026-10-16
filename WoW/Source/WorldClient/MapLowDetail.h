#ifndef WOW_SOURCE_WORLDCLIENT_MAPLOWDETAIL_H
#define WOW_SOURCE_WORLDCLIENT_MAPLOWDETAIL_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2iVector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Source/WorldClient/FVBBList.h"

class CMapObjDef;
struct SMMapObjDef;

class CMapAreaLow {
    public:
    /* 0000 */ uint32_t m_objectIndex;
    /* 0004 */ CAaBox m_bbox;
    /* 001C */ CAaSphere m_sphere;
    /* 002C */ C3Vector m_topLeft;
    /* 0038 */ C2iVector m_index;
    /* 0040 */ uint32_t m_indexCount;
    /* 0044 */ int16_t* m_heights;
    /* 0048 */ uint16_t* m_holes;
    /* 004C */ TSLink<CMapAreaLow> m_horizonLink;
    /* 0054 */ FVBBList::Block* m_block;
    /* 0058 */ uint32_t m_flags;

    CMapAreaLow();
};

class CMapAreaLowHeap {
    public:
    /* 0000 */ uint32_t m_heapId;

    void Delete(CMapAreaLow* areaLow);
};

class CMapLowDetail {
    public:
    /* 0000 */ char* m_data;
    /* 0004 */ char* m_mapObjNames;
    /* 0008 */ uint32_t* m_mapObjNameOffsets;
    /* 000C */ SMMapObjDef* m_mapObjDefs;
    /* 0010 */ uint32_t m_mapObjDefCount;
    /* 0014 */ uint32_t* m_areaOffsets;
    /* 0018 */ CMapAreaLow* m_areas [4096];
    /* 4018 */ TSGrowableArray<CMapObjDef*> m_mapObjDefList;

    CMapLowDetail();
    void CullHorizonAreas(const C3Vector* pos);
    void Load(const char* mapPath, const char* mapName);
    void Purge();
    void Unload();
};

#endif
