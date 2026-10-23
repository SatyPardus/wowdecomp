#include "WorldMap.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include <cmath>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaPOIRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/DungeonMapChunkRec.h"
#include "WoW/Source/DB/Rec/DungeonMapRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/WorldMapAreaRec.h"
#include "WoW/Source/DB/Rec/WorldMapContinentRec.h"
#include "WoW/Source/DB/Rec/WorldMapOverlayRec.h"
#include "WoW/Source/DB/Rec/WorldMapTransformsRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/Types.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"

// STATIC: 0x00BE8EE0
uint32_t CGWorldMap::m_numLandmarks;

// STATIC: 0x00BE8EE4
uint32_t CGWorldMap::m_numOverlays;

// STATIC: 0x00BE8EE8
CSimpleModel* CGWorldMap::m_playerArrowFrame;

// STATIC: 0x00BE8EEC
CSimpleModel* CGWorldMap::m_playerMiniArrowFrame;

// STATIC: 0x00BE8EF0
CSimpleModel* CGWorldMap::m_playerArrowEffectFrame;

// STATIC: 0x00BE8EF4
CSimpleModel* CGWorldMap::m_playerMiniArrowEffectFrame;

// STATIC: 0x00ACCF04
int32_t CGWorldMap::m_currentContinent = -1;

// STATIC: 0x00ACCF08
int32_t CGWorldMap::m_currentZone = -1;

// STATIC: 0x00ACCF0C
int32_t CGWorldMap::m_currentDungeonMap = -1;

// STATIC: 0x00ACCF10
int32_t CGWorldMap::m_currentOrphanZone = -1;

// STATIC: 0x00BE8F08
TSFixedArray<WorldMapContinentInfo> CGWorldMap::m_continents;

// STATIC: 0x00BE8F14
TSGrowableArray<WorldMapLandmarkInfo> CGWorldMap::m_landmarks;

// STATIC: 0x00BE8F24
TSGrowableArray<int32_t> CGWorldMap::m_overlays;

// STATIC: 0x00BE8F34
TSFixedArray<int32_t> CGWorldMap::m_orphanZones;

// STATIC: 0x00BE8EF8
static int32_t s_chunkCacheMapID;

// STATIC: 0x00BE8EFC
static int32_t s_chunkCacheIndex;

// VERIFY: 3e6cfe40 auto
// OFFSET: 0x00547120
void CGWorldMap::EnterWorld() {
    ClntObjMgrSetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, 0xDF4, 0x200, PlayerExploredZonesUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: ceb82748 auto
// OFFSET: 0x005437B0
static int32_t QSortZones(const void* a, const void* b) {
    WorldMapAreaRec* zoneA = g_worldMapAreaDB.GetRecord(*static_cast<const int32_t*>(a));
    WorldMapAreaRec* zoneB = g_worldMapAreaDB.GetRecord(*static_cast<const int32_t*>(b));

    if (zoneA && zoneB) {
        AreaTableRec* areaA = g_areaTableDB.GetRecord(zoneA->m_areaID);
        AreaTableRec* areaB = g_areaTableDB.GetRecord(zoneB->m_areaID);

        if (areaA && areaB) {
            return SStrSortUTF8I(areaA->m_areaName, areaB->m_areaName, 0x7FFFFFFF);
        }
    }

    return 0;
}

// VERIFY: 0c51c054 manual
// OFFSET: 0x00547EB0
void CGWorldMap::InitializeGame() {
    uint32_t numAreas = g_worldMapAreaDB.m_numRecords;
    uint32_t numContinents = 0;

    for (int32_t i = 0; i < g_worldMapAreaDB.m_numRecords; i++) {
        WorldMapAreaRec* area = i < 0 || i >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[i];

        if (area && !area->m_areaID) {
            numContinents++;
        }
    }

    CGWorldMap::m_continents.SetCount(numContinents);

    uint32_t continentIndex = 0;

    for (int32_t i = 0; i < static_cast<int32_t>(numAreas); i++) {
        WorldMapAreaRec* area = i < 0 || i >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[i];

        if (!area || area->m_areaID) {
            continue;
        }

        WorldMapContinentInfo* continent = &CGWorldMap::m_continents.m_data[continentIndex];
        continent->m_mapID = area->m_mapID;
        continent->m_areaID = area->m_id;

        uint32_t numZones = 0;

        for (int32_t j = 0; j < static_cast<int32_t>(numAreas); j++) {
            WorldMapAreaRec* zone = j < 0 || j >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[j];
            int32_t mapID = zone->m_displayMapID;

            if (mapID < 0) {
                mapID = zone->m_mapID;
            }

            if (zone->m_areaID && mapID == area->m_mapID) {
                numZones++;
            }
        }

        continent->m_zones.SetCount(numZones);

        uint32_t zoneIndex = 0;

        for (int32_t j = 0; j < static_cast<int32_t>(numAreas); j++) {
            WorldMapAreaRec* zone = j < 0 || j >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[j];
            int32_t mapID = zone->m_displayMapID;

            if (mapID < 0) {
                mapID = zone->m_mapID;
            }

            if (zone->m_areaID && mapID == area->m_mapID) {
                continent->m_zones.m_data[zoneIndex++] = zone->m_id;
            }
        }

        qsort(continent->m_zones.m_data, zoneIndex, sizeof(int32_t), QSortZones);

        for (int32_t j = 0; j < g_worldMapContinentDB.m_numRecords; j++) {
            WorldMapContinentRec* rec = j < 0 || j >= g_worldMapContinentDB.m_numRecords ? nullptr : &g_worldMapContinentDB.m_records[j];

            if (rec->m_mapID == area->m_mapID) {
                float offset = rec->m_scale * 32.0f;
                float x = 31.3125f - offset + rec->m_continentOffset[0];
                float y = 20.875f - offset + rec->m_continentOffset[1];

                continent->unk_10018 = rec->m_leftBoundary * rec->m_scale * 0.015968064f + x * 0.015968064f;
                continent->unk_10020 = x * 0.015968064f + 0.015968064f * ((rec->m_rightBoundary + 1) * rec->m_scale);
                continent->unk_10014 = rec->m_topBoundary * rec->m_scale * 0.023952097f + y * 0.023952097f;
                continent->unk_1001C = y * 0.023952097f + 0.023952097f * ((rec->m_bottomBoundary + 1) * rec->m_scale);
                continent->unk_10024 = rec->m_worldMapID;

                break;
            }
        }

        char fileName[260];
        SStrPrintf(fileName, sizeof(fileName), "Interface\\WorldMap\\%s.zmp", area->m_areaName);

        SFileImpl* file;

        if (SFileOpenFile(fileName, &file)) {
            SFileReadFile(file, continent->m_zoneMap, sizeof(continent->m_zoneMap), nullptr, nullptr, 0);
            SFileCloseFile(file);

            for (uint32_t y = 0; y < 128; y++) {
                for (uint32_t x = 0; x < 128; x++) {
                    int32_t areaID = continent->m_zoneMap[y][x];
                    continent->m_zoneMap[y][x] = 0;

                    if (!areaID) {
                        continue;
                    }

                    AreaTableRec* areaRec = g_areaTableDB.GetRecord(areaID);

                    if (!areaRec) {
                        continue;
                    }

                    if (areaRec->m_parentAreaID) {
                        areaID = areaRec->m_parentAreaID;
                    }

                    for (int32_t k = 0; k < static_cast<int32_t>(numAreas); k++) {
                        WorldMapAreaRec* zone = k < 0 || k >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[k];
                        int32_t mapID = zone->m_displayMapID;

                        if (mapID < 0) {
                            mapID = zone->m_mapID;
                        }

                        if (mapID == area->m_mapID && zone->m_areaID == areaID) {
                            continent->m_zoneMap[y][x] = zone->m_id;
                            break;
                        }
                    }
                }
            }
        }

        continentIndex++;
    }

    uint32_t numOrphans = 0;

    for (int32_t i = 0; i < static_cast<int32_t>(numAreas); i++) {
        WorldMapAreaRec* zone = i < 0 || i >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[i];

        if (!zone || !zone->m_areaID) {
            continue;
        }

        if (!(fabsf(zone->m_locLeft) < 0.00000023841858f) || !(fabsf(zone->m_locRight) < 0.00000023841858f) || !(fabsf(zone->m_locTop) < 0.00000023841858f) || !(fabsf(zone->m_locBottom) < 0.00000023841858f)) {
            uint32_t c = 0;

            for (; c < CGWorldMap::m_continents.Count(); c++) {
                if (CGWorldMap::m_continents.m_data[c].m_mapID == zone->m_mapID) {
                    break;
                }
            }

            if (c < CGWorldMap::m_continents.Count()) {
                continue;
            }
        }

        numOrphans++;
    }

    CGWorldMap::m_orphanZones.SetCount(numOrphans);

    if (numOrphans) {
        uint32_t orphanIndex = 0;

        for (int32_t i = 0; i < static_cast<int32_t>(numAreas); i++) {
            WorldMapAreaRec* zone = i < 0 || i >= g_worldMapAreaDB.m_numRecords ? nullptr : &g_worldMapAreaDB.m_records[i];

            if (!zone || !zone->m_areaID) {
                continue;
            }

            if (!(fabsf(zone->m_locLeft) < 0.00000023841858f) || !(fabsf(zone->m_locRight) < 0.00000023841858f) || !(fabsf(zone->m_locTop) < 0.00000023841858f) || !(fabsf(zone->m_locBottom) < 0.00000023841858f)) {
                uint32_t c = 0;

                for (; c < CGWorldMap::m_continents.Count(); c++) {
                    if (CGWorldMap::m_continents.m_data[c].m_mapID == zone->m_mapID) {
                        break;
                    }
                }

                if (c < CGWorldMap::m_continents.Count()) {
                    continue;
                }
            }

            CGWorldMap::m_orphanZones.m_data[orphanIndex++] = zone->m_id;
            DebugLog("Orphan Zone Found:  (%d) (%s)\n", zone->m_id, zone->m_areaName);
        }
    }

    CGWorldMap::m_currentOrphanZone = -1;
    CGWorldMap::m_numLandmarks = 0;
    CGWorldMap::m_numOverlays = 0;
    CGWorldMap::m_landmarks.SetCount(16);
    CGWorldMap::m_overlays.SetCount(32);
}

// VERIFY: fbeb32e6 manual
// OFFSET: 0x00543770
int32_t CGWorldMap::GetCurrentAreaID() {
    if (CGWorldMap::m_currentContinent < 0) {
        return CGWorldMap::m_currentOrphanZone;
    }

    WorldMapContinentInfo& continent = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];

    if (CGWorldMap::m_currentZone < 0) {
        return continent.m_areaID;
    }

    return continent.m_zones.m_data[CGWorldMap::m_currentZone];
}

// VERIFY: 5cdb40ad manual
// OFFSET: 0x00543740
uint32_t CGWorldMap::GetNumZones(uint32_t continent) {
    if (continent >= CGWorldMap::m_continents.m_count) {
        return 0;
    }

    return CGWorldMap::m_continents.m_data[continent].m_zones.m_count;
}

// VERIFY: e30bfe4c manual
// OFFSET: 0x00543860
int32_t CGWorldMap::GetZoneID(uint32_t continent, uint32_t zone) {
    if (continent >= CGWorldMap::m_continents.m_count) {
        return -1;
    }

    WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];

    if (zone >= entry.m_zones.m_count) {
        return -1;
    }

    return entry.m_zones.m_data[zone];
}

// VERIFY: 27a3c40a manual
// OFFSET: 0x00542F60
int32_t CGWorldMap::GetCurrentDungeonMap() {
    if (CGWorldMap::m_currentOrphanZone != -1) {
        return CGWorldMap::m_currentDungeonMap;
    }

    return -1;
}

// VERIFY: 6023babc manual
// OFFSET: 0x00543920
bool CGWorldMap::TransformPosition(int32_t mapID, float x, float y, int32_t* newMapID, float* newX, float* newY, int32_t* dungeonMapID) {
    *newX = x;
    *newY = y;
    *newMapID = mapID;
    *dungeonMapID = 0;

    for (int32_t i = 0; i < g_worldMapTransformsDB.m_numRecords; i++) {
        WorldMapTransformsRec* rec = i < 0 || i >= g_worldMapTransformsDB.m_numRecords ? nullptr : &g_worldMapTransformsDB.m_records[i];

        if (!rec || rec->m_mapID != mapID || rec->m_regionMin[0] > x || rec->m_regionMax[0] < x || y < rec->m_regionMin[1] || y > rec->m_regionMax[1]) {
            continue;
        }

        *newMapID = rec->m_newMapID;
        *newX += rec->m_regionOffset[0];
        *newY += rec->m_regionOffset[1];
        *dungeonMapID = rec->m_newDungeonMapID;
        return true;
    }

    return false;
}

// VERIFY: aaf59810 manual
// OFFSET: 0x005439E0
int32_t CGWorldMap::GetDungeonMapFromChunk(int32_t mapID, int32_t wmoGroupID, float z) {
    if (mapID <= 0 || wmoGroupID <= 0) {
        return -1;
    }

    int32_t start = 0;
    bool foundMap = false;
    bool foundGroup = false;
    int32_t dungeonMap = -1;

    if (s_chunkCacheMapID == mapID) {
        start = s_chunkCacheIndex;
    } else {
        s_chunkCacheMapID = mapID;
        s_chunkCacheIndex = 0;
    }

    for (int32_t i = start; i < g_dungeonMapChunkDB.m_numRecords; i++) {
        DungeonMapChunkRec* rec = i < 0 || i >= g_dungeonMapChunkDB.m_numRecords ? nullptr : &g_dungeonMapChunkDB.m_records[i];

        if (rec->m_mapID != mapID) {
            if (foundMap) {
                return dungeonMap;
            }

            continue;
        }

        if (!foundMap) {
            foundMap = true;
            s_chunkCacheIndex = i;
        }

        if (rec->m_wmogroupID == wmoGroupID) {
            foundGroup = true;

            if (z >= rec->m_minZ) {
                dungeonMap = rec->m_dungeonMapID;
            }
        } else if (foundGroup) {
            return dungeonMap;
        }
    }

    return dungeonMap;
}

// VERIFY: 3875b5c9 manual
// OFFSET: 0x00544090
WorldMapContinentRec* CGWorldMap::GetContinentRec(int32_t mapID) {
    for (int32_t i = g_worldMapContinentDB.m_numRecords - 1; i >= 0; i--) {
        WorldMapContinentRec* rec = i < 0 || i >= g_worldMapContinentDB.m_numRecords ? nullptr : &g_worldMapContinentDB.m_records[i];

        if (rec->m_mapID == mapID) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: a18ad447 auto
// OFFSET: 0x005440E0
bool CGWorldMap::GetContinentPosition(float x, float y, int32_t mapID, float* mapX, float* mapY) {
    WorldMapContinentRec* rec = CGWorldMap::GetContinentRec(mapID);

    if (!rec) {
        return false;
    }

    float top = rec->m_continentOffset[1] * 0.023952097f + 0.5f - x * 0.000044910183f * rec->m_scale;
    *mapX = rec->m_continentOffset[0] * 0.015968064f + 0.5f - y * 0.00002994012f * rec->m_scale;
    *mapY = 1.0f - top;
    return true;
}

// VERIFY: 3cedc975 manual
// OFFSET: 0x00544140
bool CGWorldMap::GetWorldPosition(const C3Vector& position, int32_t mapID, float* x, float* y, bool* outland, bool noClamp, int32_t dungeonMapID) {
    *x = 0.0f;
    *y = 0.0f;
    C2Vector transformed = { 0.0f, 0.0f };

    if (outland) {
        *outland = false;
    }

    int32_t newMapID;
    int32_t transformedDungeonMap;
    CGWorldMap::TransformPosition(mapID, position.x, position.y, &newMapID, &transformed.x, &transformed.y, &transformedDungeonMap);
    WorldMapContinentRec* continentRec = CGWorldMap::GetContinentRec(newMapID);

    if (continentRec && CGWorldMap::m_currentContinent == -1) {
        if (outland && newMapID == 530) {
            *outland = true;
        }

        *x = continentRec->m_continentOffset[0] * 0.015968064 + 0.5 - transformed.y * 0.00002994012 * continentRec->m_scale;
        *y = continentRec->m_continentOffset[1] * 0.023952097 + 0.5 - transformed.x * 0.000044910183 * continentRec->m_scale;
        return false;
    }

    int32_t areaID;

    if (CGWorldMap::m_currentContinent < 0) {
        areaID = CGWorldMap::m_currentOrphanZone;
    } else {
        if (static_cast<uint32_t>(CGWorldMap::m_currentContinent) >= CGWorldMap::m_continents.m_count) {
            return false;
        }

        WorldMapContinentInfo& continent = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];

        if (newMapID != continent.m_mapID) {
            return false;
        }

        areaID = CGWorldMap::m_currentZone < 0 ? continent.m_areaID : continent.m_zones.m_data[CGWorldMap::m_currentZone];
    }

    if (areaID == -1) {
        return false;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaID);

    if (!area || (area->m_mapID != mapID && area->m_mapID != newMapID)) {
        return false;
    }

    C2Vector pos = CGWorldMap::m_currentZone >= 0 ? C2Vector(position) : transformed;
    int32_t dungeonMap = dungeonMapID;

    if (dungeonMap <= 0) {
        dungeonMap = CGWorldMap::GetDungeonMapFromChunk(mapID, CGGameUI::m_wmoGroupID, position.z);
    }

    DungeonMapRec* dungeonRec;

    if (dungeonMap == (CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap)) {
        dungeonRec = g_dungeonMapDB.GetRecord(dungeonMap);
    } else if (transformedDungeonMap == (CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap)) {
        dungeonRec = g_dungeonMapDB.GetRecord(transformedDungeonMap);
    } else {
        if (CGWorldMap::GetCurrentDungeonMap() > 0) {
            return false;
        }

        dungeonRec = nullptr;
    }

    float maxX = dungeonRec ? dungeonRec->m_maxX : area->m_locLeft;
    float minX = dungeonRec ? dungeonRec->m_minX : area->m_locRight;
    double rangeX = maxX - minX;
    double offsetX = pos.y - minX;
    *x = 0.0 == offsetX || 0.0 == rangeX ? 0.0 : 1.0 - offsetX / rangeX;

    float maxY = dungeonRec ? dungeonRec->m_maxY : area->m_locTop;
    float minY = dungeonRec ? dungeonRec->m_minY : area->m_locBottom;
    double rangeY = maxY - minY;
    double offsetY = pos.x - minY;
    *y = 0.0 == offsetY || 0.0 == rangeY ? 0.0 : 1.0 - offsetY / rangeY;

    if (!noClamp && (*x < 0.0f || *x > 1.0f || *y < 0.0f || *y > 1.0f)) {
        *x = 0.0f;
        *y = 0.0f;
    }

    return true;
}

// VERIFY: c1149227 manual
// OFFSET: 0x005445F0
void CGWorldMap::GetPOIPosition(const AreaPOIRec* poi, float* x, float* y) {
    *x = 0.0f;
    *y = 0.0f;

    if (!poi) {
        return;
    }

    if (((CGWorldMap::m_currentContinent >= 0 && CGWorldMap::m_currentZone >= 0) || CGWorldMap::m_currentContinent == -2) && !(poi->m_flags & 0x4)) {
        return;
    }

    if (CGWorldMap::m_currentContinent == -1 && !(poi->m_flags & 0x10)) {
        return;
    }

    if (CGWorldMap::m_currentZone < 0 && CGWorldMap::m_currentOrphanZone < 0 && !(poi->m_flags & 0x8)) {
        return;
    }

    if (poi->m_flags & 0x400) {
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetZoneID(CGWorldMap::m_currentContinent, CGWorldMap::m_currentZone));

        if (!area || area->m_areaID != poi->m_areaID || area->m_mapID != poi->m_continentID) {
            return;
        }
    }

    C3Vector position = { poi->m_pos[0], poi->m_pos[1], 0.0f };
    bool outland;
    CGWorldMap::GetWorldPosition(position, poi->m_continentID, x, y, &outland, false, 0);

    if (outland) {
        *x = 0.0f;
        *y = 0.0f;
    }
}

// VERIFY: 0da742b3 manual
// OFFSET: 0x00546310
void CGWorldMap::SetMap(uint32_t continent, uint32_t zone, int32_t dungeonMap) {
    if (continent < CGWorldMap::m_continents.m_count) {
        CGWorldMap::m_currentContinent = continent;
        CGWorldMap::m_currentZone = zone < CGWorldMap::m_continents.m_data[continent].m_zones.m_count ? zone : -1;
        CGWorldMap::m_currentOrphanZone = -1;
        CGWorldMap::m_currentDungeonMap = -1;
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetZoneID(continent, zone));

        if (area && area->m_defaultDungeonFloor > 0) {
            CGWorldMap::SetMap(-2, area->m_id, area->m_defaultDungeonFloor);
            return;
        }
    } else {
        CGWorldMap::m_currentContinent = continent;
        CGWorldMap::m_currentZone = -1;
        CGWorldMap::m_currentOrphanZone = continent == static_cast<uint32_t>(-2) ? zone : -1;
        CGWorldMap::m_currentDungeonMap = dungeonMap;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGWorldMap::m_numLandmarks = 0;

    for (int32_t i = 0; i < g_areaPOIDB.m_numRecords; i++) {
        AreaPOIRec* poi = i < 0 || i >= g_areaPOIDB.m_numRecords ? nullptr : &g_areaPOIDB.m_records[i];
        float x = 0.0f;
        float y = 0.0f;
        CGWorldMap::GetPOIPosition(poi, &x, &y);

        if (fabs(x) < 0.00000023841858f && fabs(y) < 0.00000023841858f) {
            continue;
        }

        if (poi->m_areaID > 0) {
            AreaTableRec* area = g_areaTableDB.GetRecord(poi->m_areaID);

            if (!area || !player || (area->m_explorationLevel >= 0 && !player->HasZoneExplored(area->m_areaBit))) {
                continue;
            }
        }

        if (poi->m_worldStateID && !GetWorldState(poi->m_worldStateID)) {
            continue;
        }

        if (CGWorldMap::m_landmarks.Count() <= CGWorldMap::m_numLandmarks) {
            CGWorldMap::m_landmarks.SetCount(2 * CGWorldMap::m_landmarks.Count());
        }

        WorldMapLandmarkInfo& landmark = CGWorldMap::m_landmarks.m_data[CGWorldMap::m_numLandmarks++];
        landmark.m_id = poi->m_id;
        landmark.m_x = x;
        landmark.m_y = y;
        landmark.m_isGraveyard = 0;
        landmark.m_isGossip = 0;
    }

    AreaPOIRec* gossip = MinimapGetGossipPoi();

    if (gossip && (fabs(gossip->m_pos[0]) >= 0.00000023841858f || fabs(gossip->m_pos[1]) >= 0.00000023841858f)) {
        float x = 0.0f;
        float y = 0.0f;
        C3Vector position = { gossip->m_pos[0], gossip->m_pos[1], 0.0f };
        bool outland = false;
        CGWorldMap::GetWorldPosition(position, gossip->m_continentID, &x, &y, &outland, false, 0);

        if (!outland && (fabs(x) >= 0.00000023841858f || fabs(y) >= 0.00000023841858f)) {
            if (CGWorldMap::m_landmarks.Count() <= CGWorldMap::m_numLandmarks) {
                CGWorldMap::m_landmarks.SetCount(2 * CGWorldMap::m_landmarks.Count());
            }

            WorldMapLandmarkInfo& landmark = CGWorldMap::m_landmarks.m_data[CGWorldMap::m_numLandmarks++];
            landmark.m_x = x;
            landmark.m_y = y;
            landmark.m_isGraveyard = 0;
            landmark.m_isGossip = 1;
        }
    }

    CGWorldMap::m_numOverlays = 0;

    if (player && continent != static_cast<uint32_t>(-2) && (CGWorldMap::m_currentZone >= 0 || CGWorldMap::m_currentOrphanZone > 0)) {
        int32_t mapAreaID;

        if (CGWorldMap::m_currentContinent < 0) {
            mapAreaID = CGWorldMap::m_currentOrphanZone;
        } else {
            WorldMapContinentInfo& current = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];
            mapAreaID = CGWorldMap::m_currentZone < 0 ? current.m_areaID : current.m_zones.m_data[CGWorldMap::m_currentZone];
        }

        for (int32_t i = 0; i < g_worldMapOverlayDB.m_numRecords; i++) {
            WorldMapOverlayRec* overlay = i < 0 || i >= g_worldMapOverlayDB.m_numRecords ? nullptr : &g_worldMapOverlayDB.m_records[i];

            if (mapAreaID != overlay->m_mapAreaID) {
                continue;
            }

            for (uint32_t j = 0; j < 4; j++) {
                AreaTableRec* area = overlay->m_areaID[j] ? g_areaTableDB.GetRecord(overlay->m_areaID[j]) : nullptr;

                if (!area || (area->m_explorationLevel >= 0 && !player->HasZoneExplored(area->m_areaBit))) {
                    continue;
                }

                if (CGWorldMap::m_overlays.Count() <= CGWorldMap::m_numOverlays) {
                    CGWorldMap::m_overlays.SetCount(2 * CGWorldMap::m_overlays.Count());
                }

                CGWorldMap::m_overlays.m_data[CGWorldMap::m_numOverlays++] = overlay->m_id;
                break;
            }
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_WORLD_MAP_UPDATE, nullptr);
}

// VERIFY: 6566cbff manual
// OFFSET: 0x00547170
void CGWorldMap::SetMapToCurrentZone(int32_t debug) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        CGWorldMap::SetMap(-1, -1, -1);
        return;
    }

    C3Vector position = player->GetPosition();
    int32_t mapID;
    float mapX;
    float mapY;
    int32_t dungeonMapID;
    bool transformed = CGWorldMap::TransformPosition(CGPlayer_C::GetNewContinentID(), position.x, position.y, &mapID, &mapX, &mapY, &dungeonMapID);
    uint32_t continent = -1;

    for (uint32_t i = 0; i < CGWorldMap::m_continents.m_count; i++) {
        if (CGWorldMap::m_continents.m_data[i].m_mapID == mapID) {
            continent = i;
            break;
        }
    }

    char text[256];

    if (debug) {
        if (!transformed || dungeonMapID || (CGPlayer_C::GetNewContinentID() == mapID && mapX == position.x && mapY == position.y)) {
            SStrPrintf(text, sizeof(text) - 1, "Player at (%03d, %5.2f, %5.2f, %5.2f)", CGPlayer_C::GetNewContinentID(), position.x, position.y, position.z);
        } else {
            SStrPrintf(text, sizeof(text) - 1, "Player at (%03d, %5.2f, %5.2f, %5.2f), alternate position is (%03d, %5.2f, %5.2f, %5.2f)", CGPlayer_C::GetNewContinentID(), position.x, position.y, position.z, mapID, mapX, mapY, position.z);
        }
    }

    if (continent == static_cast<uint32_t>(-1)) {
        for (uint32_t i = 0; i < CGWorldMap::m_orphanZones.m_count; i++) {
            WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_orphanZones.m_data[i]);

            if (!area || area->m_mapID != CGPlayer_C::GetNewContinentID()) {
                continue;
            }

            int32_t dungeonMap = CGWorldMap::GetDungeonMapFromChunk(area->m_mapID, CGGameUI::m_wmoGroupID, position.z);

            if (dungeonMap > 0) {
                if (!debug) {
                    CGWorldMap::SetMap(-2, area->m_id, dungeonMap);
                    return;
                }

                SStrPrintf(text, sizeof(text) - 1, "%s, found", text);
            } else if (dungeonMapID) {
                dungeonMap = dungeonMapID;

                if (!debug) {
                    CGWorldMap::SetMap(-2, area->m_id, dungeonMap);
                    return;
                }

                SStrPrintf(text, sizeof(text) - 1, "%s, using override dungeon map", text);
            } else {
                if (!debug) {
                    CGWorldMap::SetMap(-2, area->m_id, dungeonMap);
                    return;
                }

                SStrPrintf(text, sizeof(text) - 1, "%s, NO MAP FOUND (will use terrain)", text);
            }

            SStrPrintf(text, sizeof(text) - 1, "%s, Dungeon Map Info:  WMOGroupID = %d  DMapID = %d  WorldMapAreaRecID = %d  MapID = %d", text, CGGameUI::m_wmoGroupID, dungeonMap, area->m_id, area->m_mapID);
            CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            ConsolePrintf("%s\n", text);
            return;
        }

        MapRec* map = g_mapDB.GetRecord(mapID);

        if (map && (fabs(map->m_corpse[0]) >= 0.00000023841858f || fabs(map->m_corpse[1]) >= 0.00000023841858f)) {
            CGWorldMap::TransformPosition(map->m_corpseMapID, map->m_corpse[0], map->m_corpse[1], &mapID, &mapX, &mapY, &dungeonMapID);

            for (uint32_t i = 0; i < CGWorldMap::m_continents.m_count; i++) {
                if (CGWorldMap::m_continents.m_data[i].m_mapID == mapID) {
                    continent = i;
                    break;
                }
            }
        }

        if (continent == static_cast<uint32_t>(-1)) {
            if (!debug) {
                CGWorldMap::SetMap(-2, -1, -1);
                return;
            }

            SStrPrintf(text, sizeof(text) - 1, "%s, True Orphan:  Continent = -1", text);
            CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            ConsolePrintf("%s\n", text);
            return;
        }

        double gridX = 0.5 - mapX * 0.000029296876;
        double gridY = 0.5 - mapY * 0.000029296876;

        if (gridX >= 0.0 && gridX < 1.0 && gridY >= 0.0 && gridY < 1.0) {
            WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];
            uint32_t zoneArea = entry.m_zoneMap[static_cast<int32_t>(gridX * 128.0)][static_cast<int32_t>(gridY * 128.0)];
            uint32_t numZones = CGWorldMap::GetNumZones(continent);

            for (uint32_t i = 0; i < numZones; i++) {
                if (static_cast<uint32_t>(entry.m_zones.m_data[i]) != zoneArea) {
                    continue;
                }

                if (!debug) {
                    CGWorldMap::SetMap(continent, i, -1);
                    return;
                }

                SStrPrintf(text, sizeof(text) - 1, "%s Inside Instance:  Continent = %02d  MapID = %03d", text, continent, i);
                CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
                ConsolePrintf("%s\n", text);
                return;
            }
        }
    }

    WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];
    int32_t zone = -1;

    for (uint32_t i = 0; i < entry.m_zones.m_count; i++) {
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(entry.m_zones.m_data[i]);

        if (!area || area->m_areaID != CGGameUI::m_areaID) {
            continue;
        }

        if (fabs(area->m_locLeft) >= 0.00000023841858f || fabs(area->m_locRight) >= 0.00000023841858f || fabs(area->m_locTop) >= 0.00000023841858f || fabs(area->m_locBottom) >= 0.00000023841858f) {
            zone = i;
            break;
        }

        int32_t dungeonMap = CGWorldMap::GetDungeonMapFromChunk(area->m_mapID, CGGameUI::m_wmoGroupID, player->GetPosition().z);

        if (dungeonMap >= 0) {
            if (!debug) {
                CGWorldMap::SetMap(-2, area->m_id, dungeonMap);
                return;
            }

            SStrPrintf(text, sizeof(text) - 1, "%s, Dungeon Map Info (continent):  WMOID = %d  DMapID = %d  DMapRec = %d  z = %5.2f", text, CGGameUI::m_wmoGroupID, dungeonMap, area->m_id, player->GetPosition().z);
            CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            ConsolePrintf("%s\n", text);
            return;
        }

        if (area->m_parentWorldMapID) {
            for (uint32_t j = 0; j < entry.m_zones.m_count; j++) {
                if (entry.m_zones.m_data[j] == area->m_parentWorldMapID) {
                    zone = j;
                    break;
                }
            }
        }
    }

    if (!debug) {
        CGWorldMap::SetMap(continent, zone, -1);
        return;
    }

    SStrPrintf(text, sizeof(text) - 1, "%s, Normal Map:  Continent = %02d  MapID = %03d", text, continent, zone);
    CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    ConsolePrintf("%s\n", text);
}

// VERIFY: 7155245b manual
// OFFSET: 0x005446F0
static void CenterArrowModel(CSimpleModel* model) {
    float x = model->GetWidth() * 0.5f;
    float height = model->GetHeight();
    model->m_position.x = x;
    model->m_position.y = height * 0.5f;
    model->m_position.z = 0.0f;
}

// VERIFY: d4b2fbc3 auto
// OFFSET: 0x005448A0
void CGWorldMap::CreateMiniArrowFrame(CSimpleFrame* parent) {
    if (!parent) {
        return;
    }

    if (!CGWorldMap::m_playerMiniArrowFrame) {
        void* data = CSimpleModel::s_allocator.GetData(0, typeid(CSimpleModel).raw_name(), SLOG_OBJECT);
        CGWorldMap::m_playerMiniArrowFrame = data ? new (data) CSimpleModel(parent) : nullptr;
        CGWorldMap::m_playerMiniArrowFrame->SetModel("Interface\\Minimap\\MinimapArrow.mdx");
        CGWorldMap::m_playerMiniArrowFrame->m_modelScale = NDCToDDCHeight(1.0f) * 1.1111112f;
        CGWorldMap::m_playerMiniArrowFrame->SetName("PlayerMiniArrowFrame");

        if (CGWorldMap::m_playerMiniArrowFrame->m_model && CGWorldMap::m_playerMiniArrowFrame->m_model->IsLoaded(0, 0)) {
            CenterArrowModel(CGWorldMap::m_playerMiniArrowFrame);
        }
    }

    if (!CGWorldMap::m_playerMiniArrowEffectFrame) {
        void* data = CSimpleModel::s_allocator.GetData(0, typeid(CSimpleModel).raw_name(), SLOG_OBJECT);
        CGWorldMap::m_playerMiniArrowEffectFrame = data ? new (data) CSimpleModel(parent) : nullptr;
        CGWorldMap::m_playerMiniArrowEffectFrame->SetModel("Interface\\Minimap\\MinimapArrow.mdx");
        CGWorldMap::m_playerMiniArrowEffectFrame->m_modelScale = NDCToDDCHeight(1.0f) * 1.1111112f;
        CGWorldMap::m_playerMiniArrowEffectFrame->SetName("PlayerMiniArrowEffectFrame");

        if (CGWorldMap::m_playerMiniArrowEffectFrame->m_model && CGWorldMap::m_playerMiniArrowEffectFrame->m_model->IsLoaded(0, 0)) {
            CenterArrowModel(CGWorldMap::m_playerMiniArrowEffectFrame);
        }
    }
}

// VERIFY: c2a167a7 manual
// OFFSET: 0x005449F0
void CGWorldMap::UpdateArrowFrames() {
    if (CGWorldMap::m_playerArrowFrame && CGWorldMap::m_playerArrowFrame->m_model && CGWorldMap::m_playerArrowFrame->m_model->IsLoaded(0, 0)) {
        CenterArrowModel(CGWorldMap::m_playerArrowFrame);
    }

    if (CGWorldMap::m_playerArrowEffectFrame && CGWorldMap::m_playerArrowEffectFrame->m_model && CGWorldMap::m_playerArrowEffectFrame->m_model->IsLoaded(0, 0)) {
        CenterArrowModel(CGWorldMap::m_playerArrowEffectFrame);
    }

    if (CGWorldMap::m_playerMiniArrowFrame && CGWorldMap::m_playerMiniArrowFrame->m_model && CGWorldMap::m_playerMiniArrowFrame->m_model->IsLoaded(0, 0)) {
        CenterArrowModel(CGWorldMap::m_playerMiniArrowFrame);
    }

    if (CGWorldMap::m_playerMiniArrowEffectFrame && CGWorldMap::m_playerMiniArrowEffectFrame->m_model && CGWorldMap::m_playerMiniArrowEffectFrame->m_model->IsLoaded(0, 0)) {
        CenterArrowModel(CGWorldMap::m_playerMiniArrowEffectFrame);
    }

    CGObject_C* target = ClntObjMgrObjectPtr(CGWorldFrame::GetActiveCamera()->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target) {
        return;
    }

    float facing = target->GetFacing();

    if (CGWorldMap::m_playerArrowFrame) {
        CGWorldMap::m_playerArrowFrame->m_facing = facing;
    }

    if (CGWorldMap::m_playerArrowEffectFrame) {
        CGWorldMap::m_playerArrowEffectFrame->m_facing = facing;
    }

    if (CGWorldMap::m_playerMiniArrowFrame) {
        CGWorldMap::m_playerMiniArrowFrame->m_facing = facing;
    }

    if (CGWorldMap::m_playerMiniArrowEffectFrame) {
        CGWorldMap::m_playerMiniArrowEffectFrame->m_facing = facing;
    }
}

// VERIFY: 5c3545ab auto
// OFFSET: 0x00546190
int32_t CGWorldMap::GetNumDungeonLevels() {
    int32_t count = 0;
    uint32_t floors = 0;

    if (CGWorldMap::m_currentOrphanZone < 0) {
        return 0;
    }

    if (CGWorldMap::m_currentContinent >= 0) {
        WorldMapContinentInfo& continent = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];
        int32_t zone = CGWorldMap::m_currentZone < 0 ? continent.m_areaID : continent.m_zones.m_data[CGWorldMap::m_currentZone];

        if (zone != CGWorldMap::m_currentOrphanZone) {
            return 0;
        }
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_currentOrphanZone);

    if (!area) {
        return 0;
    }

    for (int32_t i = 0; i < g_dungeonMapDB.m_numRecords; i++) {
        DungeonMapRec* rec = i < 0 || i >= g_dungeonMapDB.m_numRecords ? nullptr : &g_dungeonMapDB.m_records[i];

        if (rec->m_mapID != area->m_mapID) {
            continue;
        }

        uint32_t bit = 1 << rec->m_floorIndex;

        if (!(bit & floors)) {
            floors |= bit;
            count++;
        }
    }

    if (area->m_defaultDungeonFloor == -1) {
        count++;
    }

    return count;
}

// VERIFY: c5c6039c manual
// OFFSET: 0x00543C60
const char* CGWorldMap::GetCurrentMapName() {
    int32_t areaID = CGWorldMap::GetCurrentAreaID();

    if (areaID == -1) {
        return nullptr;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaID);

    if (!area) {
        return nullptr;
    }

    return area->m_areaName;
}

// VERIFY: 0ccd3fbd manual
// OFFSET: 0x00543CD0
int32_t CGWorldMap::GetCurrentMapHeight() {
    int32_t areaID = CGWorldMap::GetCurrentAreaID();

    if (areaID == -1) {
        return 0;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaID);

    if (!area) {
        return 0;
    }

    DungeonMapRec* dungeonRec = g_dungeonMapDB.GetRecord(CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap);
    double width;
    double height;

    if (dungeonRec) {
        width = dungeonRec->m_maxX - dungeonRec->m_minX;
        height = dungeonRec->m_maxY - dungeonRec->m_minY;
    } else if (CGWorldMap::m_currentZone == -1) {
        WorldMapContinentRec* continentRec = nullptr;

        for (int32_t i = 0; i < g_worldMapContinentDB.m_numRecords; i++) {
            WorldMapContinentRec* rec = i < 0 || i >= g_worldMapContinentDB.m_numRecords ? nullptr : &g_worldMapContinentDB.m_records[i];

            if (rec->m_mapID == area->m_mapID) {
                continentRec = rec;
                break;
            }
        }

        if (!continentRec) {
            return 0;
        }

        width = continentRec->m_rightBoundary - continentRec->m_leftBoundary + 1;
        height = continentRec->m_bottomBoundary - continentRec->m_topBoundary + 1;
    } else {
        width = area->m_locLeft - area->m_locRight;
        height = area->m_locTop - area->m_locBottom;
    }

    if (0.0 == width || 0.0 == height) {
        return 0;
    }

    return static_cast<int32_t>(height * 1002.0 / width);
}

// VERIFY: 611d8509 manual
// OFFSET: 0x005444F0
bool CGWorldMap::GetPlayerPosition(WOWGUID guid, float* x, float* y) {
    *x = 0.0f;
    *y = 0.0f;
    bool outland = false;
    bool result;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        C3Vector position = player->GetPosition();
        result = CGWorldMap::GetWorldPosition(position, CGPlayer_C::GetNewContinentID(), x, y, &outland, false, 0);
    } else {
        CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);

        if (!state || (state->m_flags & 0x20)) {
            return false;
        }

        AreaTableRec* area = g_areaTableDB.GetRecord(state->m_zoneID);

        if (!area) {
            return false;
        }

        C3Vector position = { static_cast<float>(state->m_posX), static_cast<float>(state->m_posY), 0.0f };
        result = CGWorldMap::GetWorldPosition(position, area->m_continentID, x, y, &outland, false, 0);
    }

    if (outland) {
        *x = 0.0f;
        *y = 0.0f;
    }

    return result;
}

// VERIFY: 560d4ae3 manual
// OFFSET: 0x00543E50
int32_t CGWorldMap::GetMapAreaFromPos(float x, float y) {
    int32_t areaID = CGWorldMap::GetCurrentAreaID();

    if (areaID == -1 || CGWorldMap::m_currentContinent == -2) {
        return 0;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaID);

    if (!area) {
        return 0;
    }

    int32_t mapID;
    int32_t dungeonMapID;
    float topX;
    float topY;
    float bottomX;
    float bottomY;
    CGWorldMap::TransformPosition(area->m_mapID, area->m_locTop, area->m_locLeft, &mapID, &topX, &topY, &dungeonMapID);
    CGWorldMap::TransformPosition(area->m_mapID, area->m_locBottom, area->m_locRight, &mapID, &bottomX, &bottomY, &dungeonMapID);
    double clampedX = x >= 1.0f ? 1.0 : x < 0.0f ? 0.0 : x;
    double clampedY = y >= 1.0f ? 1.0 : y < 0.0f ? 0.0 : y;
    double rangeY = topY - bottomY;
    double rangeX = topX - bottomX;

    if (0.0 == rangeY || 0.0 == rangeX) {
        return 0;
    }

    double worldX = rangeX * (1.0 - clampedY) + bottomX;
    double gridX = 0.5 - (rangeY * (1.0 - clampedX) + bottomY) * 0.000029296876;
    double gridY = 0.5 - 0.000029296876 * worldX;

    if (gridX < 0.0 || gridX > 1.0 || gridY < 0.0 || gridY > 1.0) {
        return 0;
    }

    return CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_zoneMap[static_cast<int32_t>(gridY * 128.0)][static_cast<int32_t>(gridX * 128.0)];
}

// VERIFY: adfb1fcc manual
// OFFSET: 0x00544420
int32_t CGWorldMap::GetMapHighlight(float x, float y) {
    int32_t continent = CGWorldMap::m_currentContinent;

    if (continent == -2) {
        return 0;
    }

    if (continent != -1) {
        int32_t area = CGWorldMap::GetMapAreaFromPos(x, y);

        if (CGWorldMap::m_currentZone >= 0 && CGWorldMap::m_continents.m_data[continent].m_zones.m_data[CGWorldMap::m_currentZone] == area) {
            return 0;
        }

        return area;
    }

    for (uint32_t i = 0; i < CGWorldMap::m_continents.m_count; i++) {
        WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[i];

        if (entry.unk_10024 && x >= entry.unk_10018 && x <= entry.unk_10020 && entry.unk_10014 <= y && entry.unk_1001C >= y) {
            return entry.m_areaID;
        }
    }

    return 0;
}

// VERIFY: 92e61c2c auto
// OFFSET: 0x005438A0
const char* CGWorldMap::GetZoneName(uint32_t continent, uint32_t zone) {
    if (continent >= CGWorldMap::m_continents.m_count) {
        return nullptr;
    }

    WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];

    if (zone >= entry.m_zones.m_count) {
        return nullptr;
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(entry.m_zones.m_data[zone]);
    AreaTableRec* areaRec = area ? g_areaTableDB.GetRecord(area->m_areaID) : nullptr;

    if (!areaRec) {
        return nullptr;
    }

    return areaRec->m_areaName;
}

// VERIFY: b5a7a84c auto
// OFFSET: 0x00544750
void CGWorldMap::CreateArrowFrame(CSimpleFrame* parent) {
    if (!parent) {
        return;
    }

    if (!CGWorldMap::m_playerArrowFrame) {
        void* data = CSimpleModel::s_allocator.GetData(0, typeid(CSimpleModel).raw_name(), SLOG_OBJECT);
        CGWorldMap::m_playerArrowFrame = data ? new (data) CSimpleModel(parent) : nullptr;
        CGWorldMap::m_playerArrowFrame->SetModel("Interface\\Minimap\\MinimapArrow.mdx");
        CGWorldMap::m_playerArrowFrame->m_modelScale = NDCToDDCHeight(1.0f) * 1.6666666f;
        CGWorldMap::m_playerArrowFrame->SetName("PlayerArrowFrame");

        if (CGWorldMap::m_playerArrowFrame->m_model && CGWorldMap::m_playerArrowFrame->m_model->IsLoaded(0, 0)) {
            CenterArrowModel(CGWorldMap::m_playerArrowFrame);
        }
    }

    if (!CGWorldMap::m_playerArrowEffectFrame) {
        void* data = CSimpleModel::s_allocator.GetData(0, typeid(CSimpleModel).raw_name(), SLOG_OBJECT);
        CGWorldMap::m_playerArrowEffectFrame = data ? new (data) CSimpleModel(parent) : nullptr;
        CGWorldMap::m_playerArrowEffectFrame->SetModel("Interface\\Minimap\\MinimapArrow.mdx");
        CGWorldMap::m_playerArrowEffectFrame->m_modelScale = NDCToDDCHeight(1.0f) * 1.6666666f;
        CGWorldMap::m_playerArrowEffectFrame->SetName("PlayerArrowEffectFrame");

        if (CGWorldMap::m_playerArrowEffectFrame->m_model && CGWorldMap::m_playerArrowEffectFrame->m_model->IsLoaded(0, 0)) {
            CenterArrowModel(CGWorldMap::m_playerArrowEffectFrame);
        }
    }
}

// VERIFY: 6fd1c7d5 auto
// OFFSET: 0x0050F490
void CGWorldMap::RefreshCurrentMap() {
    if (CGWorldMap::m_currentOrphanZone == -1) {
        CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentZone, CGWorldMap::m_currentOrphanZone);
    } else {
        CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentOrphanZone, CGWorldMap::m_currentDungeonMap);
    }
}

// VERIFY: 3d6efa14 manual
// OFFSET: 0x00542FB0
void CGWorldMap::ClearArrowFrames() {
    if (CGWorldMap::m_playerArrowFrame) {
        delete CGWorldMap::m_playerArrowFrame;
        CGWorldMap::m_playerArrowFrame = nullptr;
    }

    if (CGWorldMap::m_playerArrowEffectFrame) {
        delete CGWorldMap::m_playerArrowEffectFrame;
        CGWorldMap::m_playerArrowEffectFrame = nullptr;
    }

    if (CGWorldMap::m_playerMiniArrowFrame) {
        delete CGWorldMap::m_playerMiniArrowFrame;
        CGWorldMap::m_playerMiniArrowFrame = nullptr;
    }

    if (CGWorldMap::m_playerMiniArrowEffectFrame) {
        delete CGWorldMap::m_playerMiniArrowEffectFrame;
        CGWorldMap::m_playerMiniArrowEffectFrame = nullptr;
    }
}

// VERIFY: 1acf1385 manual
// OFFSET: 0x00547A70
void CGWorldMap::SetDungeonLevel(int32_t level) {
    if (CGWorldMap::m_currentOrphanZone < 0) {
        return;
    }

    if (CGWorldMap::m_currentContinent >= 0) {
        WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent];
        int32_t zoneArea = CGWorldMap::m_currentZone < 0 ? entry.m_areaID : entry.m_zones.m_data[CGWorldMap::m_currentZone];

        if (zoneArea != CGWorldMap::m_currentOrphanZone) {
            return;
        }
    }

    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_currentOrphanZone);

    if (!area) {
        return;
    }

    int32_t mapID = area->m_mapID;

    if (!level) {
        CGWorldMap::m_currentDungeonMap = -1;
        CGWorldMap::SetMap(-2, area->m_id, -1);
        return;
    }

    bool inMap = false;

    for (int32_t i = 0; i < g_dungeonMapDB.m_numRecords; i++) {
        DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecordByIndex(i);

        if (dungeonMap->m_mapID == mapID) {
            if (dungeonMap->m_floorIndex == level) {
                if (dungeonMap->m_id > 0) {
                    CGWorldMap::m_currentDungeonMap = dungeonMap->m_id;
                    CGWorldMap::RefreshCurrentMap();
                }

                return;
            }

            if (!inMap) {
                inMap = true;
            }
        } else if (inMap) {
            return;
        }
    }
}

// VERIFY: f429796e manual
// OFFSET: 0x00547D20
void CGWorldMap::ShutdownGame() {
    CGWorldMap::ClearArrowFrames();

    for (uint32_t i = 0; i < CGWorldMap::m_continents.m_count; i++) {
        CGWorldMap::m_continents.m_data[i].m_zones.Clear();
    }

    CGWorldMap::m_continents.Clear();
    CGWorldMap::m_landmarks.Clear();
    CGWorldMap::m_overlays.Clear();
    CGWorldMap::m_orphanZones.Clear();
}

// VERIFY: 51645fdc manual
// OFFSET: 0x005468F0
void CGWorldMap::ProcessClick(float x, float y) {
    uint32_t continent = CGWorldMap::m_currentContinent;

    if (continent == static_cast<uint32_t>(-2)) {
        return;
    }

    if (continent == static_cast<uint32_t>(-1)) {
        for (uint32_t i = 0; i < CGWorldMap::m_continents.Count(); i++) {
            WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[i];

            if (entry.unk_10024 && x >= entry.unk_10018 && x <= entry.unk_10020 && entry.unk_10014 <= y && entry.unk_1001C >= y) {
                CGWorldMap::SetMap(i, -1, -1);
                return;
            }
        }

        return;
    }

    int32_t area = CGWorldMap::GetMapAreaFromPos(x, y);

    if (!area) {
        return;
    }

    for (uint32_t zone = 0; continent < CGWorldMap::m_continents.Count(); zone++) {
        WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];

        if (zone >= entry.m_zones.Count()) {
            break;
        }

        if (entry.m_zones.m_data[zone] == area) {
            CGWorldMap::SetMap(continent, zone, -1);
            return;
        }
    }
}

// VERIFY: a5a7a1db auto
// OFFSET: 0x005470D0
int32_t PlayerExploredZonesUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    if (CGWorldMap::m_currentOrphanZone == -1) {
        CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentZone, CGWorldMap::m_currentOrphanZone);
    } else {
        CGWorldMap::SetMap(CGWorldMap::m_currentContinent, CGWorldMap::m_currentOrphanZone, CGWorldMap::m_currentDungeonMap);
    }

    return 1;
}

// VERIFY: c0334dbd manual
// OFFSET: 0x00543920
bool WorldMapTransformPosition(int32_t mapID, float x, float y, int32_t* outMapID, float* outX, float* outY, int32_t* outDungeonMapID) {
    *outX = x;
    *outY = y;
    *outMapID = mapID;
    *outDungeonMapID = 0;

    for (int32_t i = 0; i < g_worldMapTransformsDB.m_numRecords; i++) {
        WorldMapTransformsRec* transform = g_worldMapTransformsDB.GetRecordByIndex(i);

        if (!transform || mapID != transform->m_mapID || transform->m_regionMin[0] > x || transform->m_regionMax[0] < x || y < transform->m_regionMin[1] || y > transform->m_regionMax[1]) {
            continue;
        }

        *outMapID = transform->m_newMapID;
        *outX = transform->m_regionOffset[0] + *outX;
        *outY = transform->m_regionOffset[1] + *outY;
        *outDungeonMapID = transform->m_newDungeonMapID;
        return true;
    }

    return false;
}

// VERIFY: 1e793a0b auto
// OFFSET: 0x00547150
void CGWorldMap::LeaveWorld() {
    ClntObjMgrUnsetObjMirrorHandler(ClntObjMgrGetActivePlayer(), ID_PLAYER, 0xDF4, PlayerExploredZonesUpdateHandler, nullptr);
}
