#include "WorldMap.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaPOIRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/DungeonMapRec.h"
#include "WoW/Source/DB/Rec/WorldMapAreaRec.h"
#include "WoW/Source/DB/Rec/WorldMapContinentRec.h"
#include "WoW/Source/DB/Rec/WorldMapOverlayRec.h"
#include "WoW/Source/DB/Rec/WorldSafeLocsRec.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/Frame/CSimpleFrame.h"

// VERIFY: 6b834e58 manual
// OFFSET: 0x00543020
static int32_t Script_GetNumMapLandmarks(lua_State* L) {
    lua_pushnumber(L, CGWorldMap::m_numLandmarks);
    return 1;
}

// VERIFY: 0322bec4 manual
// OFFSET: 0x00543060
static int32_t Script_GetNumMapOverlays(lua_State* L) {
    lua_pushnumber(L, CGWorldMap::m_numOverlays);
    return 1;
}

// VERIFY: 99889007 auto
// OFFSET: 0x005430A0
static int32_t Script_PositionWorldMapArrowFrame(lua_State* L) {
    CSimpleModel* frame = CGWorldMap::m_playerArrowFrame;
    CSimpleModel* effect = CGWorldMap::m_playerArrowEffectFrame;

    if (!frame || !effect) {
        return 0;
    }

    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: PositionWorldMapArrowFrame(\"point\" \"frame\" [, relativePoint] [, offsetX, offsetY])");
        return 0;
    }

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    FRAMEPOINT point;

    if (!StringToFramePoint(lua_tolstring(L, 1, nullptr), point)) {
        luaL_error(L, "Unknown frame point");
        return 0;
    }

    FRAMEPOINT relativePoint = point;
    const char* name = lua_tolstring(L, 2, nullptr);
    CLayoutFrame* relative = frame->GetLayoutFrameByName(name);
    char error[128];

    if (!relative) {
        SStrPrintf(error, sizeof(error), "Couldn't find frame named '%s'", name);
        luaL_error(L, error);
        return 0;
    }

    if (relative == frame) {
        SStrPrintf(error, sizeof(error), "Error: %s is anchored to itself", name);
        luaL_error(L, error);
        return 0;
    }

    if (lua_isstring(L, 3)) {
        if (!StringToFramePoint(lua_tolstring(L, 3, nullptr), relativePoint)) {
            luaL_error(L, "Unknown frame point");
            return 0;
        }

        if (lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
            float value = lua_tonumber(L, 4);
            offsetX = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
            value = lua_tonumber(L, 5);
            offsetY = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
        }
    }

    frame->SetPoint(point, relative, relativePoint, offsetX, offsetY, 1);
    effect->SetPoint(point, relative, relativePoint, offsetX, offsetY, 1);
    return 0;
}

// VERIFY: c4679e14 auto
// OFFSET: 0x005432C0
static int32_t Script_PositionMiniWorldMapArrowFrame(lua_State* L) {
    CSimpleModel* frame = CGWorldMap::m_playerMiniArrowFrame;
    CSimpleModel* effect = CGWorldMap::m_playerMiniArrowEffectFrame;

    if (!frame || !effect) {
        return 0;
    }

    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: PositionMiniWorldMapArrowFrame(\"point\" \"frame\" [, relativePoint] [, offsetX, offsetY])");
        return 0;
    }

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    FRAMEPOINT point;

    if (!StringToFramePoint(lua_tolstring(L, 1, nullptr), point)) {
        luaL_error(L, "Unknown frame point");
        return 0;
    }

    FRAMEPOINT relativePoint = point;
    const char* name = lua_tolstring(L, 2, nullptr);
    CLayoutFrame* relative = frame->GetLayoutFrameByName(name);
    char error[128];

    if (!relative) {
        SStrPrintf(error, sizeof(error), "Couldn't find frame named '%s'", name);
        luaL_error(L, error);
        return 0;
    }

    if (relative == frame) {
        SStrPrintf(error, sizeof(error), "Error: %s is anchored to itself", name);
        luaL_error(L, error);
        return 0;
    }

    if (lua_isstring(L, 3)) {
        if (!StringToFramePoint(lua_tolstring(L, 3, nullptr), relativePoint)) {
            luaL_error(L, "Unknown frame point");
            return 0;
        }

        if (lua_isnumber(L, 4) && lua_isnumber(L, 5)) {
            float value = lua_tonumber(L, 4);
            offsetX = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
            value = lua_tonumber(L, 5);
            offsetY = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
        }
    }

    frame->SetPoint(point, relative, relativePoint, offsetX, offsetY, 1);
    effect->SetPoint(point, relative, relativePoint, offsetX, offsetY, 1);
    return 0;
}

// VERIFY: ceb493fb manual
// OFFSET: 0x00544B20
static int32_t Script_GetMapContinents(lua_State* L) {
    uint32_t count = CGWorldMap::m_continents.Count();

    for (uint32_t i = 0; i < count; i++) {
        MapRec* map = i < CGWorldMap::m_continents.Count() ? g_mapDB.GetRecord(CGWorldMap::m_continents.m_data[i].m_mapID) : nullptr;
        lua_pushstring(L, map ? map->m_mapName : nullptr);
    }

    return count;
}

// VERIFY: 0341ee29 manual
// OFFSET: 0x00544D40
static int32_t Script_GetCurrentMapContinent(lua_State* L) {
    int32_t dungeonMap = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;
    DungeonMapRec* rec = g_dungeonMapDB.GetRecord(dungeonMap);

    if (rec) {
        for (uint32_t i = 0; i < CGWorldMap::m_continents.Count(); i++) {
            int32_t mapID = i < CGWorldMap::m_continents.Count() ? CGWorldMap::m_continents.m_data[i].m_mapID : -1;

            if (mapID == rec->m_mapID) {
                lua_pushnumber(L, i + 1);
                return 1;
            }
        }
    }

    lua_pushnumber(L, CGWorldMap::m_currentContinent + 1);
    return 1;
}

// VERIFY: b8368e00 manual
// OFFSET: 0x00544E10
static int32_t Script_GetCurrentMapAreaID(lua_State* L) {
    int32_t areaID;

    if (CGWorldMap::m_currentContinent >= 0) {
        if (CGWorldMap::m_currentZone >= 0) {
            areaID = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_zones.m_data[CGWorldMap::m_currentZone];
        } else {
            areaID = CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_areaID;
        }
    } else {
        areaID = CGWorldMap::m_currentOrphanZone;
    }

    lua_pushnumber(L, areaID + 1);
    return 1;
}

// VERIFY: 0e572815 manual
// OFFSET: 0x00544E80
static int32_t Script_GetCurrentMapZone(lua_State* L) {
    if (CGWorldMap::m_currentZone < 0) {
        int32_t areaID = CGWorldMap::m_currentContinent < 0 ? CGWorldMap::m_currentOrphanZone : CGWorldMap::m_continents.m_data[CGWorldMap::m_currentContinent].m_areaID;
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(areaID);

        if (area) {
            for (uint32_t continent = 0; continent < CGWorldMap::m_continents.Count(); continent++) {
                int32_t mapID = continent < CGWorldMap::m_continents.Count() ? CGWorldMap::m_continents.m_data[continent].m_mapID : -1;

                if (mapID != area->m_mapID) {
                    continue;
                }

                uint32_t numZones = continent < CGWorldMap::m_continents.Count() ? CGWorldMap::m_continents.m_data[continent].m_zones.Count() : 0;

                for (uint32_t zone = 0; zone < numZones; zone++) {
                    int32_t zoneID = continent < CGWorldMap::m_continents.Count() && zone < CGWorldMap::m_continents.m_data[continent].m_zones.Count() ? CGWorldMap::m_continents.m_data[continent].m_zones.m_data[zone] : -1;

                    if (zoneID == area->m_id) {
                        lua_pushnumber(L, zone + 1);
                        return 1;
                    }
                }
            }
        }
    }

    lua_pushnumber(L, CGWorldMap::m_currentZone + 1);
    return 1;
}

// VERIFY: 8e02e31e auto
// OFFSET: 0x00544FC0
static int32_t Script_GetCurrentMapDungeonLevel(lua_State* L) {
    int32_t dungeonMap = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;
    DungeonMapRec* rec = g_dungeonMapDB.GetRecord(dungeonMap);
    int32_t level = rec ? rec->m_floorIndex : 0;
    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_currentOrphanZone);

    if (area && area->m_defaultDungeonFloor == -1) {
        level++;
    }

    lua_pushnumber(L, level);
    return 1;
}

// VERIFY: cbbb8b2a auto
// OFFSET: 0x00544C40
static int32_t Script_DungeonUsesTerrainMap(lua_State* L) {
    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_currentOrphanZone);

    if (area && area->m_defaultDungeonFloor == -1) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c0808da9 auto
// OFFSET: 0x005434E0
static int32_t Script_ShowWorldMapArrowFrame(lua_State* L) {
    CSimpleModel* frame = CGWorldMap::m_playerArrowFrame;
    CSimpleModel* effect = CGWorldMap::m_playerArrowEffectFrame;

    if (frame && effect) {
        if (StringToBOOL(L, 1, 1)) {
            frame->Show();
            effect->Show();
            return 0;
        }

        frame->Hide();
        effect->Hide();
    }

    return 0;
}

// VERIFY: 00d9ccc8 auto
// OFFSET: 0x00543540
static int32_t Script_ShowMiniWorldMapArrowFrame(lua_State* L) {
    CSimpleModel* frame = CGWorldMap::m_playerMiniArrowFrame;
    CSimpleModel* effect = CGWorldMap::m_playerMiniArrowEffectFrame;

    if (frame && effect) {
        if (StringToBOOL(L, 1, 1)) {
            frame->Show();
            effect->Show();
            return 0;
        }

        frame->Hide();
        effect->Hide();
    }

    return 0;
}

// VERIFY: 2f43a41e auto
// OFFSET: 0x00545050
static int32_t Script_IsZoomOutAvailable(lua_State* L) {
    bool available = true;
    int32_t dungeonMap = CGWorldMap::m_currentOrphanZone == -1 ? -1 : CGWorldMap::m_currentDungeonMap;

    if (!g_dungeonMapDB.GetRecord(dungeonMap) && CGWorldMap::m_currentContinent == -2) {
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

        if (!area || area->m_defaultDungeonFloor != -1) {
            WorldMapAreaRec* parent = area ? g_worldMapAreaDB.GetRecord(area->m_parentWorldMapID) : nullptr;

            if (!area || !parent) {
                available = false;
            }
        }
    }

    lua_pushboolean(L, available);
    return 1;
}

// VERIFY: 5a3fa27a auto
// OFFSET: 0x00608560
static int32_t Script_GetNumMapDebugObjects(lua_State* L) {
    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: 550ba9da auto
// OFFSET: 0x008E5250
static int32_t Script_HasDebugZoneMap(lua_State* L) {
    return 0;
}

// VERIFY: a5a6186e auto
// OFFSET: 0x00547C10
static int32_t Script_SetMapToCurrentZone(lua_State* L) {
    CGWorldMap::SetMapToCurrentZone(0);
    return 0;
}

// VERIFY: e8555396 auto
// OFFSET: 0x00546290
static int32_t Script_GetNumDungeonMapLevels(lua_State* L) {
    lua_pushnumber(L, CGWorldMap::GetNumDungeonLevels());
    return 1;
}

// VERIFY: 8ff57dd4 manual
// OFFSET: 0x00544CA0
static int32_t Script_GetMapInfo(lua_State* L) {
    lua_pushstring(L, CGWorldMap::GetCurrentMapName());
    int32_t height = CGWorldMap::GetCurrentMapHeight();
    lua_pushnumber(L, height);
    int32_t size = height;

    if ((height - 1) & height) {
        for (int32_t bit = 7; bit >= 0; bit--) {
            if ((1 << bit) & height) {
                size = 1 << (bit + 1);
                break;
            }
        }
    }

    lua_pushnumber(L, size);
    return 3;
}

// VERIFY: b153db79 auto
// OFFSET: 0x00545880
static int32_t Script_GetPlayerMapPosition(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetPlayerMapPosition(\"player\")");
        return 0;
    }

    float x = 0.0f;
    float y = 0.0f;
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (guid == ClntObjMgrGetActivePlayer() || CGPartyInfo::IsMember(guid) || IsGuidInMyRaid(guid)) {
        CGWorldMap::GetPlayerPosition(guid, &x, &y);
    }

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

// VERIFY: 8bcfd2ff manual
// OFFSET: 0x005435A0
static int32_t Script_GetWintergraspWaitTime(lua_State* L) {
    if (!GetWorldState(3801)) {
        return 0;
    }

    int32_t wait = GetWorldState(4354) - CGWorldStateInfo::GetUITime();

    if (wait < 0) {
        wait = 0;
    }

    lua_pushnumber(L, wait);
    return 1;
}

// VERIFY: 95089986 manual
// OFFSET: 0x00545A30
static int32_t Script_GetMapLandmarkInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMapLandmarkInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    WorldMapLandmarkInfo* landmark = index < CGWorldMap::m_numLandmarks ? &CGWorldMap::m_landmarks.m_data[index] : nullptr;

    if (landmark) {
        if (landmark->m_isGraveyard) {
            WorldSafeLocsRec* location = g_worldSafeLocsDB.GetRecord(landmark->m_id);

            if (location) {
                lua_pushstring(L, location->m_areaName);
                lua_pushnil(L);
                lua_pushnumber(L, 6.0);
                lua_pushnumber(L, landmark->m_x);
                lua_pushnumber(L, landmark->m_y);
                lua_pushnil(L);
                return 6;
            }
        } else {
            const AreaPOIRec* poi = landmark->m_isGossip ? MinimapGetGossipPoi() : g_areaPOIDB.GetRecord(landmark->m_id);

            if (poi) {
                lua_pushstring(L, poi->m_name);
                lua_pushstring(L, poi->m_description);
                int32_t icon = 0;

                if (landmark->m_isGossip || (poi->m_flags & 0x80) || (CGWorldMap::m_currentZone == -1 && CGWorldMap::m_currentContinent != -2)) {
                    uint32_t state = 0;

                    if (poi->m_worldStateID) {
                        state = GetWorldState(poi->m_worldStateID) - 1;

                        if (state >= 9) {
                            state = 0;
                        }
                    }

                    icon = poi->m_icon[state];
                }

                lua_pushnumber(L, icon);
                lua_pushnumber(L, landmark->m_x);
                lua_pushnumber(L, landmark->m_y);

                if (poi->m_worldMapLink) {
                    lua_pushnumber(L, poi->m_worldMapLink);
                } else {
                    lua_pushnil(L);
                }

                lua_pushboolean(L, poi->m_flags & 0x200);
                return 7;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushnil(L);
    lua_pushnil(L);
    return 7;
}

// VERIFY: 7570dd98 manual
// OFFSET: 0x00545110
static int32_t Script_UpdateMapHighlight(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: UpdateMapHighlight(x, y)");
        return 0;
    }

    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);

    if (CGWorldMap::m_currentZone >= 0) {
        for (uint32_t i = 0; i < CGWorldMap::m_numOverlays; i++) {
            WorldMapOverlayRec* overlay = g_worldMapOverlayDB.GetRecord(CGWorldMap::m_overlays.m_data[i]);

            if (!overlay) {
                continue;
            }

            CRect rect;
            rect.minX = overlay->m_hitRectLeft * 0.00099800399;
            rect.minY = overlay->m_hitRectTop * 0.001497006;
            rect.maxX = overlay->m_hitRectRight * 0.00099800399;
            rect.maxY = overlay->m_hitRectBottom * 0.001497006;

            if (rect.maxX - rect.minX == 0.0 || rect.maxY - rect.minY == 0.0) {
                continue;
            }

            if (!rect.IsPointInside(C2Vector(x, y))) {
                continue;
            }

            AreaTableRec* areaRec = g_areaTableDB.GetRecord(overlay->m_areaID[0]);

            if (areaRec) {
                lua_pushstring(L, areaRec->m_areaName);
                lua_pushnil(L);

                for (int32_t j = 0; j < 6; j++) {
                    lua_pushnumber(L, 0.0);
                }

                return 8;
            }
        }
    }

    int32_t highlight = CGWorldMap::GetMapHighlight(x, y);
    WorldMapAreaRec* area = highlight ? g_worldMapAreaDB.GetRecord(highlight) : nullptr;

    if (area && CGWorldMap::m_currentZone >= 0) {
        AreaTableRec* areaRec = g_areaTableDB.GetRecord(area->m_areaID);

        if (areaRec) {
            lua_pushstring(L, areaRec->m_areaName);
        } else {
            lua_pushnil(L);
        }

        lua_pushnil(L);

        for (int32_t j = 0; j < 6; j++) {
            lua_pushnumber(L, 0.0);
        }

        return 8;
    }

    if (!area || (!CMath::fnotequal(area->m_locLeft, 0.0f) && !CMath::fnotequal(area->m_locRight, 0.0f) && !CMath::fnotequal(area->m_locTop, 0.0f) && !CMath::fnotequal(area->m_locBottom, 0.0f))) {
        lua_pushnil(L);
        lua_pushnil(L);

        for (int32_t j = 0; j < 6; j++) {
            lua_pushnumber(L, 0.0);
        }

        return 8;
    }

    if (area->m_areaID) {
        AreaTableRec* areaRec = g_areaTableDB.GetRecord(area->m_areaID);

        if (areaRec) {
            lua_pushstring(L, areaRec->m_areaName);
        } else {
            lua_pushnil(L);
        }
    } else {
        MapRec* map = g_mapDB.GetRecord(area->m_mapID);

        if (map) {
            lua_pushstring(L, map->m_mapName);
        } else {
            lua_pushnil(L);
        }
    }

    lua_pushstring(L, area->m_areaName);
    float width = 64.0f;
    float height = 64.0f;
    WorldMapContinentRec* continentRec = nullptr;
    int32_t size = 128;

    if (area->m_areaID) {
        width = area->m_locLeft - area->m_locRight;
        height = area->m_locTop - area->m_locBottom;
        size = static_cast<int32_t>(128.0 * height / width);
    } else {
        for (int32_t i = 0; i < g_worldMapContinentDB.m_numRecords; i++) {
            WorldMapContinentRec* rec = &g_worldMapContinentDB.m_records[i];

            if (!rec) {
                continue;
            }

            if (rec->m_mapID == area->m_mapID) {
                continentRec = rec;
                break;
            }
        }

        if (continentRec) {
            width = continentRec->m_rightBoundary - continentRec->m_leftBoundary + 1;
            height = continentRec->m_bottomBoundary - continentRec->m_topBoundary + 1;
            size = static_cast<int32_t>(512.0 * height / width);
        }
    }

    int32_t textureSize = size;

    if (size) {
        if ((size - 1) & size) {
            for (int32_t bit = 7; bit >= 0; bit--) {
                if ((1 << bit) & size) {
                    textureSize = 1 << (bit + 1);
                    break;
                }
            }
        }

        while (8 * textureSize < size) {
            textureSize *= 2;
        }

        if (static_cast<uint32_t>(size) > 256) {
            textureSize += (size >> 8 << 8) + 256;
        }
    }

    if (!area->m_areaID) {
        if (height >= width) {
            lua_pushnumber(L, width / height);
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnumber(L, 1.0);
            lua_pushnumber(L, height / width);
        }

        if (continentRec) {
            double half = continentRec->m_scale * 32.0;
            double left = continentRec->m_leftBoundary * continentRec->m_scale + 31.3125 - half + continentRec->m_continentOffset[0];
            double top = 20.875 - half + continentRec->m_topBoundary * continentRec->m_scale + continentRec->m_continentOffset[1];
            lua_pushnumber(L, continentRec->m_scale * width * 0.015968064);
            lua_pushnumber(L, continentRec->m_scale * height * 0.023952097);
            lua_pushnumber(L, left * 0.015968064);
            lua_pushnumber(L, top * 0.023952097);
            return 8;
        }
    } else {
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, static_cast<double>(size) / textureSize);
        int32_t displayMapID = area->m_displayMapID;

        for (int32_t i = 0; i < g_worldMapAreaDB.m_numRecords; i++) {
            WorldMapAreaRec* rec = &g_worldMapAreaDB.m_records[i];

            if (!rec) {
                continue;
            }

            int32_t recMapID = rec->m_displayMapID < 0 ? rec->m_mapID : rec->m_displayMapID;

            if (rec->m_areaID || recMapID != (displayMapID >= 0 ? displayMapID : area->m_mapID)) {
                continue;
            }

            float parentWidth = rec->m_locLeft - rec->m_locRight;
            float parentHeight = rec->m_locTop - rec->m_locBottom;
            float offsetX;
            float offsetY;

            if (displayMapID < 0) {
                offsetX = rec->m_locLeft - area->m_locLeft;
                offsetY = rec->m_locTop - area->m_locTop;
            } else {
                int32_t mapID;
                int32_t dungeonMapID;
                float transformedX;
                float transformedY;
                CGWorldMap::TransformPosition(area->m_mapID, area->m_locTop, area->m_locLeft, &mapID, &transformedX, &transformedY, &dungeonMapID);
                offsetX = rec->m_locLeft - transformedY;
                offsetY = rec->m_locTop - transformedX;
            }

            float invWidth = 1.0 / parentWidth;
            float invHeight = 1.0 / parentHeight;
            lua_pushnumber(L, invWidth * width);
            lua_pushnumber(L, invHeight * height);
            lua_pushnumber(L, invWidth * offsetX);
            lua_pushnumber(L, invHeight * offsetY);
            return 8;
        }
    }

    for (int32_t j = 0; j < 6; j++) {
        lua_pushnumber(L, 0.0);
    }

    return 8;
}

// VERIFY: 29e7c946 auto
// OFFSET: 0x00544B90
static int32_t Script_GetMapZones(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMapZones(continentIndex)");
        return 0;
    }

    uint32_t continent = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t count = continent < CGWorldMap::m_continents.m_count ? CGWorldMap::m_continents.m_data[continent].m_zones.m_count : 0;
    lua_checkstack(L, count);

    for (uint32_t i = 0; i < count; i++) {
        lua_pushstring(L, CGWorldMap::GetZoneName(continent, i));
    }

    return count;
}

// VERIFY: a9ab4466 manual
// OFFSET: 0x00545C80
static int32_t Script_GetMapOverlayInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMapOverlayInfo(index)");
        return 0;
    }

    int64_t index = static_cast<int64_t>(lua_tonumber(L, 1)) - 1;
    int32_t overlayID = index < CGWorldMap::m_numOverlays ? CGWorldMap::m_overlays.m_data[index] : 0;
    WorldMapOverlayRec* overlay = g_worldMapOverlayDB.GetRecord(overlayID);

    if (!overlay) {
        lua_pushnil(L);

        for (int32_t i = 0; i < 6; i++) {
            lua_pushnumber(L, 0.0);
        }

        return 7;
    }

    char texture[260];
    const char* name = overlay->m_textureName;

    if (name && *name) {
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(overlay->m_mapAreaID);

        if (area) {
            SStrPrintf(texture, sizeof(texture), "Interface\\WorldMap\\%s\\%s", area->m_areaName, name);
        } else {
            SStrPrintf(texture, sizeof(texture), "Interface\\WorldMap\\World\\%s", name);
        }
    } else {
        texture[0] = '\0';
    }

    lua_pushstring(L, texture);
    lua_pushnumber(L, overlay->m_textureWidth);
    lua_pushnumber(L, overlay->m_textureHeight);
    lua_pushnumber(L, overlay->m_offsetX);
    lua_pushnumber(L, overlay->m_offsetY);
    lua_pushnumber(L, overlay->m_mapPointX);
    lua_pushnumber(L, overlay->m_mapPointY);
    return 7;
}

// VERIFY: 2f36de9c auto
// OFFSET: 0x005469E0
static int32_t Script_SetMapZoom(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetMapZoom(continentIndex [,zoneIndex])");
        return 0;
    }

    int32_t continent = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if ((continent > 0 && static_cast<uint32_t>(continent) >= CGWorldMap::m_continents.m_count) || continent < -2) {
        luaL_error(L, "Usage: SetMapZoom(continentIndex [,zoneIndex]) .. requested continent out of bounds");
        return 0;
    }

    uint32_t zone = -1;

    if (lua_isnumber(L, 2)) {
        zone = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    }

    CGWorldMap::SetMap(continent, zone, -1);
    return 0;
}

// VERIFY: a3213393 manual
// OFFSET: 0x00545E60
static int32_t Script_CreateWorldMapArrowFrame(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        luaL_error(L, "Usage: CreateWorldMapArrowFrame(parent)");
        return 0;
    }

    lua_rawgeti(L, 1, 0);
    CSimpleFrame* parent = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!parent) {
        luaL_error(L, "CreateWorldMapArrowFrame(): Couldn't find 'this' in parent object");
        return 0;
    }

    if (!parent->IsA(CSimpleFrame::GetObjectType())) {
        luaL_error(L, "CreateWorldMapArrowFrame(): Wrong object type, expected frame");
        return 0;
    }

    CGWorldMap::CreateArrowFrame(parent);
    return 0;
}

// VERIFY: 04ce3302 auto
// OFFSET: 0x00545FF0
static int32_t Script_InitWorldMapPing(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        luaL_error(L, "Usage: InitWorldMapPing(parent)");
        return 0;
    }

    lua_rawgeti(L, 1, 0);
    CSimpleFrame* parent = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!parent) {
        luaL_error(L, "InitWorldMapPing(): Couldn't find 'this' in parent object");
        return 0;
    }

    if (!parent->IsA(CSimpleFrame::GetObjectType())) {
        luaL_error(L, "InitWorldMapPing(): Wrong object type, expected frame");
        return 0;
    }

    CLayoutFrame* layout = parent->GetLayoutFrameByName("WorldMapPing");

    if (!layout) {
        return 0;
    }

    CSimpleModel* ping = static_cast<CSimpleModel*>(layout);

    if (!ping) {
        return 0;
    }

    C3Vector position(layout->GetWidth() * 0.5f, ping->GetHeight() * 0.5f, 0.0f);
    ping->SetPosition(position);
    return 0;
}

// VERIFY: 17164b20 auto
// OFFSET: 0x00543600
static int32_t Script_CanQueueForWintergrasp(lua_State* L) {
    if (GetWorldState(4375)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ef6f67cf auto
// OFFSET: 0x00545950
static int32_t Script_GetCorpseMapPosition(lua_State* L) {
    C3Vector position(0.0f, 0.0f, 0.0f);
    int32_t mapID;
    int32_t actualMapID;
    CGGameUI::GetCorpseLocation(mapID, position, actualMapID);
    float x;
    float y;
    CGWorldMap::GetWorldPosition(position, mapID, &x, &y, nullptr, false, 0);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

// VERIFY: 5b5ef994 auto
// OFFSET: 0x005459C0
static int32_t Script_GetDeathReleasePosition(lua_State* L) {
    C3Vector position(0.0f, 0.0f, 0.0f);
    int32_t mapID;
    CGGameUI::GetDeathReleaseLocation(mapID, position);
    float x;
    float y;
    CGWorldMap::GetWorldPosition(position, mapID, &x, &y, nullptr, false, 0);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    return 2;
}

// VERIFY: f00de7d9 manual
// OFFSET: 0x00545F20
static int32_t Script_CreateMiniWorldMapArrowFrame(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        luaL_error(L, "Usage: CreateMiniWorldMapArrowFrame(parent)");
        return 0;
    }

    lua_rawgeti(L, 1, 0);
    CSimpleFrame* parent = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!parent) {
        luaL_error(L, "CreateMiniWorldMapArrowFrame(): Couldn't find 'this' in parent object");
        return 0;
    }

    if (!parent->IsA(CSimpleFrame::GetObjectType())) {
        luaL_error(L, "CreateMiniWorldMapArrowFrame(): Wrong object type, expected frame");
        return 0;
    }

    CGWorldMap::CreateMiniArrowFrame(parent);
    return 0;
}

// VERIFY: 34d0d0e9 auto
// OFFSET: 0x00545FE0
static int32_t Script_UpdateWorldMapArrowFrames(lua_State* L) {
    CGWorldMap::UpdateArrowFrames();
    return 0;
}

// VERIFY: ab8c4937 manual
// OFFSET: 0x00546A90
static int32_t Script_ZoomOut(lua_State* L) {
    int32_t parent;

    if (CGWorldMap::m_currentOrphanZone == -1 || CGWorldMap::m_currentDungeonMap <= 0) {
        if (CGWorldMap::m_currentContinent != -2) {
            return 0;
        }

        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::GetCurrentAreaID());

        if (!area) {
            return 0;
        }

        parent = area->m_parentWorldMapID;

        if (!parent) {
            for (int32_t i = 0; i < g_dungeonMapDB.m_numRecords; i++) {
                DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecordByIndex(i);

                if (dungeonMap && dungeonMap->m_mapID == area->m_mapID && dungeonMap->m_floorIndex == 1) {
                    parent = dungeonMap->m_parentWorldMapID;
                }
            }

            if (!parent) {
                return 0;
            }
        }
    } else {
        DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(CGWorldMap::m_currentDungeonMap);

        if (!dungeonMap || !dungeonMap->m_parentWorldMapID) {
            return 0;
        }

        parent = dungeonMap->m_parentWorldMapID;
    }

    WorldMapAreaRec* parentArea = g_worldMapAreaDB.GetRecord(parent);

    if (!parentArea) {
        return 0;
    }

    for (uint32_t continent = 0; continent < CGWorldMap::m_continents.Count(); continent++) {
        WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];

        if (entry.m_mapID != parentArea->m_mapID) {
            continue;
        }

        for (uint32_t zone = 0; zone < entry.m_zones.Count(); zone++) {
            if (entry.m_zones.m_data[zone] == parentArea->m_id) {
                CGWorldMap::SetMap(continent, zone, -1);
                return 0;
            }
        }
    }

    return 0;
}

// VERIFY: f724a42d manual
// OFFSET: 0x00546C50
static int32_t Script_SetMapByID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetMapByID(mapID)");
        return 0;
    }

    int32_t mapID = static_cast<int32_t>(lua_tonumber(L, 1));
    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(mapID);

    if (!area) {
        return 0;
    }

    for (uint32_t continent = 0; continent < CGWorldMap::m_continents.Count(); continent++) {
        int32_t displayMapID = area->m_displayMapID;

        if (displayMapID < 0) {
            displayMapID = area->m_mapID;
        }

        WorldMapContinentInfo& entry = CGWorldMap::m_continents.m_data[continent];

        if (entry.m_mapID != displayMapID) {
            continue;
        }

        if (area->m_defaultDungeonFloor > 0) {
            CGWorldMap::SetMap(-2, area->m_id, area->m_defaultDungeonFloor);
            lua_pushboolean(L, 1);
            return 1;
        }

        if (!area->m_areaID) {
            CGWorldMap::SetMap(continent, -1, -1);
            lua_pushboolean(L, 1);
            return 1;
        }

        for (uint32_t zone = 0; zone < entry.m_zones.Count(); zone++) {
            if (entry.m_zones.m_data[zone] == area->m_id) {
                CGWorldMap::SetMap(continent, zone, -1);
                lua_pushboolean(L, 1);
                return 1;
            }
        }
    }

    if (area->m_defaultDungeonFloor == -1) {
        CGWorldMap::SetMap(-2, mapID, -1);
        lua_pushboolean(L, 1);
        return 1;
    }

    for (int32_t i = 0; i < g_dungeonMapDB.m_numRecords; i++) {
        DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecordByIndex(i);

        if (dungeonMap && dungeonMap->m_mapID == area->m_mapID && dungeonMap->m_floorIndex == 1) {
            CGWorldMap::SetMap(-2, mapID, dungeonMap->m_id);
            lua_pushboolean(L, 1);
            return 1;
        }
    }

    CGWorldMap::SetMap(-2, mapID, -1);
    lua_pushboolean(L, 1);
    return 1;
}

// VERIFY: 8c4a2255 auto
// OFFSET: 0x00546E80
static int32_t Script_ProcessMapClick(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: ProcessMapClick(x, y)");
        return 0;
    }

    float x = lua_tonumber(L, 1);
    float y = lua_tonumber(L, 2);
    CGWorldMap::ProcessClick(x, y);
    return 0;
}

// VERIFY: 2a928080 auto
// OFFSET: 0x00546EF0
static int32_t Script_ClickLandmark(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ClickLandmark(mapLinkID)");
        return 0;
    }

    int32_t id = static_cast<int32_t>(lua_tonumber(L, 1));
    DungeonMapRec* dungeonMap = g_dungeonMapDB.GetRecord(id);

    if (!dungeonMap) {
        return 0;
    }

    for (uint32_t i = 0; i < CGWorldMap::m_orphanZones.Count(); i++) {
        WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_orphanZones.m_data[i]);

        if (area && area->m_mapID == dungeonMap->m_mapID) {
            CGWorldMap::SetMap(-2, area->m_id, id);
            return 0;
        }
    }

    return 0;
}

// VERIFY: f361bf64 auto
// OFFSET: 0x00547B80
static int32_t Script_SetDungeonMapLevel(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetDungeonMapLevel(level)");
        return 0;
    }

    int32_t level = static_cast<int32_t>(lua_tonumber(L, 1));
    WorldMapAreaRec* area = g_worldMapAreaDB.GetRecord(CGWorldMap::m_currentOrphanZone);

    if (area && area->m_defaultDungeonFloor == -1) {
        if (--level < 0) {
            return 0;
        }

        CGWorldMap::SetDungeonLevel(level);
    } else if (level > 0) {
        CGWorldMap::SetDungeonLevel(level);
    }

    return 0;
}

// VERIFY: 5c8fe2ff auto
// OFFSET: 0x008E5250
static int32_t Script_GetMapDebugObjectInfo(lua_State* L) {
    return 0;
}

// VERIFY: 254a479d auto
// OFFSET: 0x008E5250
static int32_t Script_TeleportToDebugObject(lua_State* L) {
    return 0;
}

// VERIFY: 0d559bc9 auto
// OFFSET: 0x008E5250
static int32_t Script_GetDebugZoneMap(lua_State* L) {
    return 0;
}

// STATIC: 0x00ACCF18
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetMapContinents", &Script_GetMapContinents },
    { "GetMapZones", &Script_GetMapZones },
    { "SetMapZoom", &Script_SetMapZoom },
    { "ZoomOut", &Script_ZoomOut },
    { "SetDungeonMapLevel", &Script_SetDungeonMapLevel },
    { "GetNumDungeonMapLevels", &Script_GetNumDungeonMapLevels },
    { "DungeonUsesTerrainMap", &Script_DungeonUsesTerrainMap },
    { "SetMapToCurrentZone", &Script_SetMapToCurrentZone },
    { "GetMapInfo", &Script_GetMapInfo },
    { "GetCurrentMapContinent", &Script_GetCurrentMapContinent },
    { "GetCurrentMapAreaID", &Script_GetCurrentMapAreaID },
    { "GetCurrentMapZone", &Script_GetCurrentMapZone },
    { "GetCurrentMapDungeonLevel", &Script_GetCurrentMapDungeonLevel },
    { "SetMapByID", &Script_SetMapByID },
    { "IsZoomOutAvailable", &Script_IsZoomOutAvailable },
    { "ProcessMapClick", &Script_ProcessMapClick },
    { "UpdateMapHighlight", &Script_UpdateMapHighlight },
    { "GetPlayerMapPosition", &Script_GetPlayerMapPosition },
    { "GetCorpseMapPosition", &Script_GetCorpseMapPosition },
    { "GetDeathReleasePosition", &Script_GetDeathReleasePosition },
    { "GetNumMapLandmarks", &Script_GetNumMapLandmarks },
    { "GetMapLandmarkInfo", &Script_GetMapLandmarkInfo },
    { "GetNumMapOverlays", &Script_GetNumMapOverlays },
    { "GetMapOverlayInfo", &Script_GetMapOverlayInfo },
    { "CreateWorldMapArrowFrame", &Script_CreateWorldMapArrowFrame },
    { "InitWorldMapPing", &Script_InitWorldMapPing },
    { "CreateMiniWorldMapArrowFrame", &Script_CreateMiniWorldMapArrowFrame },
    { "UpdateWorldMapArrowFrames", &Script_UpdateWorldMapArrowFrames },
    { "PositionWorldMapArrowFrame", &Script_PositionWorldMapArrowFrame },
    { "PositionMiniWorldMapArrowFrame", &Script_PositionMiniWorldMapArrowFrame },
    { "ShowWorldMapArrowFrame", &Script_ShowWorldMapArrowFrame },
    { "ShowMiniWorldMapArrowFrame", &Script_ShowMiniWorldMapArrowFrame },
    { "ClickLandmark", &Script_ClickLandmark },
    { "GetNumMapDebugObjects", &Script_GetNumMapDebugObjects },
    { "GetMapDebugObjectInfo", &Script_GetMapDebugObjectInfo },
    { "TeleportToDebugObject", &Script_TeleportToDebugObject },
    { "HasDebugZoneMap", &Script_HasDebugZoneMap },
    { "GetDebugZoneMap", &Script_GetDebugZoneMap },
    { "GetWintergraspWaitTime", &Script_GetWintergraspWaitTime },
    { "CanQueueForWintergrasp", &Script_CanQueueForWintergrasp },
};


// VERIFY: 9f60c73b auto
// OFFSET: 0x00543650
void WorldMapRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 554803ad auto
// OFFSET: 0x00543680
void WorldMapUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
