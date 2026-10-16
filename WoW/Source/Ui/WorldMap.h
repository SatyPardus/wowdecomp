#ifndef WOW_SOURCE_UI_WORLDMAP_H
#define WOW_SOURCE_UI_WORLDMAP_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"

struct lua_State;
class AreaPOIRec;
class C3Vector;
class CSimpleFrame;
class CSimpleModel;
class WorldMapContinentRec;

struct DebugElement {
    /* 0000 */ int32_t unused_0000;
    /* 0004 */ int32_t unused_0004;
    /* 0008 */ int32_t m_array1 [6];
    /* 0020 */ int32_t m_array2 [4];
    /* 0030 */ int32_t m_computed48;
    /* 0034 */ int32_t m_computed52;
    /* 0038 */ int32_t m_computed56;
    /* 003C */ int32_t m_extra60;
    /* 0040 */ int32_t m_extra64;
    /* 0044 */ int32_t m_extra68;
    /* 0048 */ float m_extra76;
    /* 004C */ void* unused_004C;
    /* 0050 */ int32_t unused_0050;
    /* 0054 */ int32_t unused_0054;
    /* 0058 */ int32_t unused_0058;
    /* 005C */ int32_t unused_005C;
    /* 0060 */ int32_t unused_0060;
    /* 0064 */ int32_t unused_0064;
    /* 0068 */ int32_t unused_0068;
};

struct WorldMapContinentInfo {
    /* 00000 */ int32_t m_mapID;
    /* 00004 */ int32_t m_areaID;
    /* 00008 */ TSFixedArray<int32_t> m_zones;
    /* 00014 */ uint32_t m_zoneMap[128][128];
    /* 10014 */ float unk_10014;
    /* 10018 */ float unk_10018;
    /* 1001C */ float unk_1001C;
    /* 10020 */ float unk_10020;
    /* 10024 */ int32_t unk_10024;

    WorldMapContinentInfo();
};

struct WorldMapLandmarkInfo {
    /* 0000 */ int32_t m_id;
    /* 0004 */ float m_x;
    /* 0008 */ float m_y;
    /* 000C */ int32_t m_isGraveyard;
    /* 0010 */ int32_t m_isGossip;
};

bool WorldMapTransformPosition(int32_t mapID, float x, float y, int32_t* outMapID, float* outX, float* outY, int32_t* outDungeonMapID);

class CGWorldMap {
    public:
    static uint32_t m_numLandmarks;
    static uint32_t m_numOverlays;
    static int32_t m_currentContinent;
    static int32_t m_currentZone;
    static int32_t m_currentDungeonMap;
    static int32_t m_currentOrphanZone;
    static TSFixedArray<WorldMapContinentInfo> m_continents;
    static TSGrowableArray<WorldMapLandmarkInfo> m_landmarks;
    static TSGrowableArray<int32_t> m_overlays;
    static TSFixedArray<int32_t> m_orphanZones;
    static CSimpleModel* m_playerArrowFrame;
    static CSimpleModel* m_playerMiniArrowFrame;
    static CSimpleModel* m_playerArrowEffectFrame;
    static CSimpleModel* m_playerMiniArrowEffectFrame;
    static void CreateArrowFrame(CSimpleFrame* parent);
    static void CreateMiniArrowFrame(CSimpleFrame* parent);
    static void UpdateArrowFrames();
    static void ProcessClick(float x, float y);
    static void SetDungeonLevel(int32_t level);
    static void EnterWorld();
    static void LeaveWorld();
    static bool GetContinentPosition(float x, float y, int32_t mapID, float* mapX, float* mapY);
    static void InitializeGame();
    static void ShutdownGame();
    static void ClearArrowFrames();
    static void SetMapToCurrentZone(int32_t debug);
    static void RefreshCurrentMap();
    static void SetMap(uint32_t continent, uint32_t zone, int32_t dungeonMap);
    static int32_t GetZoneID(uint32_t continent, uint32_t zone);
    static uint32_t GetNumZones(uint32_t continent);
    static int32_t GetCurrentDungeonMap();
    static bool TransformPosition(int32_t mapID, float x, float y, int32_t* newMapID, float* newX, float* newY, int32_t* dungeonMapID);
    static int32_t GetDungeonMapFromChunk(int32_t mapID, int32_t wmoGroupID, float z);
    static WorldMapContinentRec* GetContinentRec(int32_t mapID);
    static bool GetWorldPosition(const C3Vector& position, int32_t mapID, float* x, float* y, bool* outland, bool noClamp, int32_t dungeonMapID);
    static void GetPOIPosition(const AreaPOIRec* poi, float* x, float* y);
    static int32_t GetNumDungeonLevels();
    static const char* GetZoneName(uint32_t continent, uint32_t zone);
    static int32_t GetMapAreaFromPos(float x, float y);
    static int32_t GetMapHighlight(float x, float y);
    static const char* GetCurrentMapName();
    static int32_t GetCurrentMapHeight();
    static bool GetPlayerPosition(WOWGUID guid, float* x, float* y);
    static int32_t GetCurrentAreaID();
};

void WorldMapRegisterScriptFunctions();
void WorldMapUnregisterScriptFunctions();

// INLINED
inline WorldMapContinentInfo::WorldMapContinentInfo() {
    this->unk_10014 = 0.0f;
    this->unk_10018 = 0.0f;
    this->unk_1001C = 0.0f;
    this->unk_10020 = 0.0f;
}

int32_t PlayerExploredZonesUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);

#endif
