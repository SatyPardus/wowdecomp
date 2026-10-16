#ifndef WOW_SOURCE_WORLDCLIENT_MAPENTITY_H
#define WOW_SOURCE_WORLDCLIENT_MAPENTITY_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CMemBlock.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/MapLoad.h"

struct MINIMAPQUADINFO;
class CMapObjDef;
namespace World { struct LiquidQueryResult; }

struct FogQ {
    /* 0000 */ float m_distance;
    /* 0004 */ uint32_t m_fogIndex;
};

class FogQueue : public CMemBlock {
    public:
    /* 0014 */ uint32_t m_chunk;
    /* 0018 */ uint32_t m_capacity;
    /* 001C */ uint32_t m_count;

    FogQueue(const uint32_t* params);
    void Erase(uint32_t index, uint32_t count);
    FogQ Pop();
    void Push(float distance, uint32_t fogIndex);
};

class WMOAreaTableRec;

class CMapEntity : public CMapStaticEntity {
    public:
    ~CMapEntity() override;
    void Tick();
    /* 0090 */ int32_t (*m_func)(void*, uint32_t, uint64_t, uintptr_t);
    /* 0094 */ void* m_funcParam;
    /* 0098 */ uint64_t m_funcParam64;
    /* 00A0 */ uintptr_t m_funcParam32;
    /* 00A4 */ uint32_t unk_00A4;
    /* 00A8 */ C3Vector unused_00A8;
    /* 00B4 */ uint32_t unused_00B4;
    /* 00B8 */ int32_t m_groundType;
    /* 00BC */ uint16_t m_liquidType;
    /* 00BE */ uint16_t unused_00BE;
    /* 00C0 */ CImVector m_targetAmbientColor;
    /* 00C4 */ float m_targetDiffuseLightScale;
    /* 00C8 */ TSLink<CMapEntity> m_sortEntryLink;

    static int32_t QueryCameraFog(SMOFog* fog, CMapObjDef** mapObjDef, uint8_t* found, TSGrowableArray<uint32_t>** groups, float* distance);

    virtual bool QueryInteriorLighting(CMapObjDef* mapObjDef, int32_t groupNum, uint16_t* faceIndex, C3Vector* pos);

    CMapEntity();
    int32_t QueryInterior();
    int32_t QueryMapObjZoneName(const char** zoneName);
    int32_t QueryMapObjSubzoneName(const char** subzoneName, int32_t* areaId);
    bool QueryMapObjAreaTable(WMOAreaTableRec** groupRec, WMOAreaTableRec** rootRec, int32_t* wmoGroupId);
    bool QueryMapObjIDs(int32_t* wmoId, int32_t* wmoHash, int32_t* groupNum);
    int32_t QueryMapObjFileName(const char** fileName);
    bool QueryMapObjMinimap(CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint8_t indoorArea);
    bool QueryMapObjMatrix(C44Matrix* matrix, C44Matrix* invMatrix);
    void QueryLiquidSounds(TSGrowableArray<World::LiquidQueryResult>& results, uint32_t* distance);
    bool UpdateMapObjLiquid();
};

#endif
