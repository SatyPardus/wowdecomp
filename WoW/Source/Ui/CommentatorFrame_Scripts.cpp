#include "CommentatorFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WorldClient/World.h"

// VERIFY: b9b450f8 manual
// OFFSET: 0x00569230
static int32_t Script_CommentatorGetMode(lua_State* L) {
    int32_t mode = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        uint32_t flags = player->m_player->m_playerFlags;

        if (flags & 0x80000) {
            mode = 1;
        }

        if (flags & 0x400000) {
            mode = 2;
        }
    }

    lua_pushnumber(L, mode);
    return 1;
}

// VERIFY: 414eec99 manual
// OFFSET: 0x00569180
static int32_t Script_CommentatorToggleMode(lua_State* L) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_ENABLE));
    msg.Put(static_cast<uint32_t>(2));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5c4e5f57 manual
// OFFSET: 0x005690A0
static int32_t Script_CommentatorSetMode(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        int32_t mode = static_cast<int32_t>(lua_tonumber(L, 1));
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_ENABLE));

        if (mode) {
            msg.Put(static_cast<uint32_t>(1));
        } else {
            msg.Put(static_cast<uint32_t>(0));
        }

        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: fbcf23af manual
// OFFSET: 0x00568730
static int32_t Script_CommentatorGetNumMaps(lua_State* L) {
    lua_pushnumber(L, CGCommentator::s_Commentator.m_maps.Count());
    return 1;
}

// VERIFY: eaec034b manual
// OFFSET: 0x005693E0
static int32_t Script_CommentatorGetMapInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CommentatorGetMapInfo(mapIndex)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGCommentator::s_Commentator.m_maps.Count()) {
        luaL_error(L, "no map at that index");
        return 0;
    }

    CGCommentator::MapInfo* map = &CGCommentator::s_Commentator.m_maps.m_data[index];
    lua_pushnumber(L, map->unk_0000);
    lua_pushnumber(L, map->unk_0004);
    lua_pushnumber(L, map->unk_0008);
    lua_pushnumber(L, map->m_instances.Count());
    return 4;
}

// VERIFY: 003a522d manual
// OFFSET: 0x0056AFC0
static int32_t Script_CommentatorGetSkirmishQueuePlayerInfo(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(index - 1) >= CGCommentator::s_Commentator.m_skirmishQueue.Count()) {
        return 0;
    }

    CGCommentator::QueueNode* node = &CGCommentator::s_Commentator.m_skirmishQueue.m_data[index - 1];

    if (!node) {
        return 0;
    }

    char buffer[64];
    lua_pushstring(L, GUIDToHexString(node->unk_0000, buffer));
    lua_pushstring(L, GUIDToHexString(node->unk_0008, buffer));
    lua_pushboolean(L, node->unk_0010);
    return 3;
}

// VERIFY: 8ce90f84 manual
// OFFSET: 0x0056AF60
static int32_t Script_CommentatorGetSkirmishQueueCount(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    lua_pushinteger(L, CGCommentator::s_Commentator.m_skirmishQueue.Count());
    return 1;
}

// VERIFY: 9a2b78ca manual
// OFFSET: 0x0056AF00
static int32_t Script_CommentatorGetSkirmishMode(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    lua_pushinteger(L, CGCommentator::s_Commentator.m_skirmishMode);
    return 1;
}

// VERIFY: f387e25a auto
// OFFSET: 0x0056A2A0
static int32_t Script_CommentatorGetCamera(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !player->IsCommentatorUberOrInArena()) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return 0;
    }

    lua_pushnumber(L, CGCommentator::s_Commentator.m_position.x);
    lua_pushnumber(L, CGCommentator::s_Commentator.m_position.y);
    lua_pushnumber(L, CGCommentator::s_Commentator.m_position.z);
    lua_pushnumber(L, camera->m_yaw * 57.29578f);
    lua_pushnumber(L, camera->m_pitch * 57.29578f);
    lua_pushnumber(L, CGCommentator::s_Commentator.m_fov * 57.29578f);
    return 6;
}

// VERIFY: 0f4be0e6 manual
// OFFSET: 0x0056AB70
static int32_t Script_CommentatorSetCameraCollision(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_player->m_playerFlags & 0x80000) && (player->m_player->m_playerFlags & 0x400000)) {
        if (!lua_isnumber(L, 1)) {
            luaL_error(L, "Usage: CommentatorSetCameraCollision(bool enable)");
            return 0;
        }

        if (lua_tonumber(L, 1) != 0.0) {
            CGCommentator::s_Commentator.m_cameraCollision = 1;
            return 0;
        }

        CGCommentator::s_Commentator.m_cameraCollision = 0;
    }

    return 0;
}

// VERIFY: a53f977c manual
// OFFSET: 0x0056AC10
static int32_t Script_CommentatorSetTargetHeightOffset(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_player->m_playerFlags & 0x80000) && (player->m_player->m_playerFlags & 0x400000)) {
        if (!lua_isnumber(L, 1)) {
            luaL_error(L, "Usage: CommentatorSetTargetHeightOffset(float offset)");
            return 0;
        }

        CGCommentator::s_Commentator.m_targetHeightOffset = lua_tonumber(L, 1);
    }

    return 0;
}

// VERIFY: 49d3efa1 manual
// OFFSET: 0x0056AA10
static int32_t Script_CommentatorSetBattlemaster(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    WOWGUID target = player->m_unit->m_unitFieldTarget;

    if (!target) {
        luaL_error(L, "Battlemaster must be targeted.");
        return 0;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object || !(object->m_obj->m_type & TYPE_UNIT)) {
        luaL_error(L, "Targeted unit is not a Battlemaster.");
        return 0;
    }

    if (!(static_cast<CGUnit_C*>(object)->m_unit->m_unitNpcFlags & 0x100000)) {
        luaL_error(L, "Targeted unit is not a Battlemaster.");
        return 0;
    }

    CGCommentator::s_Commentator.m_battlemaster = object->m_obj->m_guid;
    return 0;
}

// VERIFY: 0accfd26 manual
// OFFSET: 0x00568770
static int32_t Script_CommentatorZoomIn(lua_State* L) {
    float fov = CGCommentator::s_Commentator.m_fov;

    if (fov <= 0.34906584f) {
        fov = 0.34906584f;
    }

    float zoom = CGCommentator::s_Commentator.m_targetFov - fov * 0.1f;
    float limit = CGCommentator::s_Commentator.m_fov * 1.1f;

    if (limit <= zoom) {
        zoom = limit;
    }

    CGCommentator::s_Commentator.m_targetFov = zoom < 2.0943952f ? (zoom > 0.017453292f ? zoom : 0.017453292f) : 2.0943952f;

    if (CGCommentator::s_Commentator.m_targetFov >= 2.0943952f) {
        CGCommentator::s_Commentator.m_targetFov = 2.0943952f;
    } else if (CGCommentator::s_Commentator.m_targetFov <= 0.017453292f) {
        CGCommentator::s_Commentator.m_targetFov = 0.017453292f;
    }

    return 0;
}

// VERIFY: 40759fc4 manual
// OFFSET: 0x00568810
static int32_t Script_CommentatorZoomOut(lua_State* L) {
    float fov = CGCommentator::s_Commentator.m_fov;

    if (fov <= 0.34906584f) {
        fov = 0.34906584f;
    }

    float zoom = (fov + fov * 0.1f) * 0.1f + CGCommentator::s_Commentator.m_targetFov;
    float limit = CGCommentator::s_Commentator.m_fov * 0.9f;

    if (limit >= zoom) {
        zoom = limit;
    }

    CGCommentator::s_Commentator.m_targetFov = zoom < 2.0943952f ? (zoom > 0.017453292f ? zoom : 0.017453292f) : 2.0943952f;

    if (CGCommentator::s_Commentator.m_targetFov >= 2.0943952f) {
        CGCommentator::s_Commentator.m_targetFov = 2.0943952f;
    } else if (CGCommentator::s_Commentator.m_targetFov <= 0.017453292f) {
        CGCommentator::s_Commentator.m_targetFov = 0.017453292f;
    }

    return 0;
}

// VERIFY: 153d331e manual
// OFFSET: 0x005692A0
static int32_t Script_CommentatorUpdatePlayerInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (player && (player->m_player->m_playerFlags & 0x80000) && CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_GET_PLAYER_INFO));
        msg.Put(instance->m_mapID);
        instance->m_key.Write(&msg);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: b827f089 manual
// OFFSET: 0x00569340
static int32_t Script_CommentatorUpdateMapInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && (player->m_player->m_playerFlags & 0x80000)) {
        const char* name = "";

        if (lua_isstring(L, 1)) {
            name = lua_tolstring(L, 1, nullptr);
        }

        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_GET_MAP_INFO));
        msg.PutString(name);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: f6ab0cd5 manual
// OFFSET: 0x00569520
static int32_t Script_CommentatorGetInstanceInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorGetMapInfo(mapIndex,instanceIndex)");
        return 0;
    }

    uint32_t mapIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t instanceIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (mapIndex >= CGCommentator::s_Commentator.m_maps.Count()) {
        luaL_error(L, "no map at that index");
        return 0;
    }

    CGCommentator::MapInfo* map = CGCommentator::s_Commentator.GetMapInfoByIndex(mapIndex);

    if (instanceIndex >= map->m_instances.Count()) {
        luaL_error(L, "no instance at that index");
        return 0;
    }

    CGCommentator::InstanceInfo* instance = &map->m_instances.m_data[instanceIndex];
    lua_pushnumber(L, instance->m_mapID);
    lua_pushnumber(L, instance->unk_0018);
    lua_pushnumber(L, static_cast<uint32_t>(instance->m_guid.m_guid));
    lua_pushnumber(L, static_cast<uint32_t>(instance->m_guid.m_guid >> 32));
    return 4;
}

// VERIFY: 3675279d manual
// OFFSET: 0x005696E0
static int32_t Script_CommentatorEnterInstance(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Unable to enter instance.");
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_ENTER_INSTANCE));
    msg.Put(instance->m_mapID);
    instance->m_key.Write(&msg);
    msg.Put(instance->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 04f8c067 manual
// OFFSET: 0x005697B0
static int32_t Script_CommentatorExitInstance(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && (player->m_player->m_playerFlags & 0x80000)) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_EXIT_INSTANCE));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: f34249a6 manual
// OFFSET: 0x00569820
static int32_t Script_CommentatorGetNumPlayers(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CommentatorGetNumPlayers(faction)");
        return 0;
    }

    uint32_t faction = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (faction - 1 >= 2) {
        luaL_error(L, "Error: faction index too large");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (player && (player->m_player->m_playerFlags & 0x80000) && CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        lua_pushnumber(L, instance->m_numPlayers[faction - 1]);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: 3e3349ce manual
// OFFSET: 0x00569910
static int32_t Script_CommentatorGetPlayerInfo(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorGetPlayerInfo(factionIndex,playerIndex)");
        return 0;
    }

    uint32_t faction = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (faction >= 2) {
        luaL_error(L, "Error: faction index too large");
        return 0;
    }

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Unable to find instance.");
        return 0;
    }

    if (index < instance->m_numPlayers[faction]) {
        CGCommentator::PlayerInfo* info = &instance->m_players[faction][index];
        lua_pushstring(L, info->m_name);
        lua_pushnumber(L, info->m_faction);
        lua_pushnumber(L, info->unk_002C);
        lua_pushnumber(L, info->unk_0030);
        lua_pushnumber(L, info->unk_0034);
        lua_pushnumber(L, info->unk_0038);
        lua_pushnumber(L, info->unk_0008);
    } else {
        lua_pushstring(L, "Unknown");
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 7;
}

// VERIFY: e4c603ed manual
// OFFSET: 0x00569B50
static int32_t Script_CommentatorFollowPlayer(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->IsCommentatorUberOrInArena()) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorFollowPlayer(factionIndex,playerIndex)");
        return 0;
    }

    uint32_t faction = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (faction >= 2) {
        luaL_error(L, "Error: faction index too large");
        return 0;
    }

    if (!index) {
        CGCommentator::s_Commentator.m_followTarget = WOWGUID();
        CGCommentator::s_Commentator.m_lookAtTarget = WOWGUID();
        return 0;
    }

    index--;

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Unable to find instance.");
        return 0;
    }

    if (index < instance->m_numPlayers[faction] && CGWorldFrame::s_currentWorldFrame) {
        CGCommentator::s_Commentator.m_lookAtTarget = WOWGUID();
        CGCommentator::s_Commentator.m_followTarget = instance->m_players[faction][index].m_guid;
    }

    return 0;
}

// VERIFY: 1a9c1775 manual
// OFFSET: 0x00569CD0
static int32_t Script_CommentatorLookatPlayer(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->IsCommentatorUberOrInArena()) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorLookatPlayer(factionIndex,playerIndex)");
        return 0;
    }

    uint32_t faction = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (faction >= 2) {
        luaL_error(L, "Error: faction index too large");
        return 0;
    }

    if (!index) {
        CGCommentator::s_Commentator.m_lookAtTarget = WOWGUID();
        CGCommentator::s_Commentator.m_followTarget = WOWGUID();
        return 0;
    }

    index--;

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Unable to find instance.");
        return 0;
    }

    if (index < instance->m_numPlayers[faction]) {
        CGCommentator::s_Commentator.m_followTarget = WOWGUID();
        CGCommentator::s_Commentator.m_lookAtTarget = instance->m_players[faction][index].m_guid;
    }

    return 0;
}

// VERIFY: a4873970 manual
// OFFSET: 0x00569E50
static int32_t Script_CommentatorSetMapAndInstanceIndex(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorSetMapAndInstanceIndex(mapIndex,instanceIndex)");
        return 0;
    }

    uint32_t mapIndex = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (mapIndex >= CGCommentator::s_Commentator.m_maps.Count()) {
        luaL_error(L, "no map at that index");
        return 0;
    }

    CGCommentator::MapInfo* map = CGCommentator::s_Commentator.GetMapInfoByIndex(mapIndex);
    uint32_t instanceIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (instanceIndex >= map->m_instances.Count()) {
        luaL_error(L, "no instance at that index");
        return 0;
    }

    CGCommentator::InstanceInfo* instance = &map->m_instances.m_data[instanceIndex];
    CGCommentator::s_Commentator.m_mapID = instance->m_mapID;
    CGCommentator::s_Commentator.m_key.unk_0000 = instance->m_key.unk_0000;
    CGCommentator::s_Commentator.m_key.unk_0008 = instance->m_key.unk_0008;
    CGCommentator::s_Commentator.m_key.unk_0004 = instance->m_key.unk_0004;
    return 0;
}

// VERIFY: 58696e6b manual
// OFFSET: 0x00569FA0
static int32_t Script_CommentatorSetPlayerIndex(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000)) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CommentatorSetPlayerIndex(factionIndex,playerIndex)");
        return 0;
    }

    uint32_t faction = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (faction >= 2) {
        luaL_error(L, "Error: faction index too large");
        return 0;
    }

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Unable to find instance.");
        return 0;
    }

    if (index >= instance->m_numPlayers[faction]) {
        luaL_error(L, "Error: player index too large");
        return 0;
    }

    CGCommentator::s_Commentator.m_playerIndex = index;
    CGCommentator::s_Commentator.m_playerFaction = faction;
    return 0;
}

// VERIFY: 824e9e46 auto
// OFFSET: 0x0056A0F0
static int32_t Script_CommentatorSetCamera(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->IsCommentatorUberOrInArena()) {
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
        luaL_error(L, "Usage: CommentatorSetCamera(xPos,yPos,zPos,yaw,pitch,fov)");
        return 0;
    }

    C3Vector position;
    position.x = lua_tonumber(L, 1);
    position.y = lua_tonumber(L, 2);
    position.z = lua_tonumber(L, 3);

    if (!World::IsValidPosition(position, 0.0f)) {
        return 0;
    }

    float yaw = lua_tonumber(L, 4);
    float pitch = lua_tonumber(L, 5) * 0.017453292;
    CGCommentator::s_Commentator.SetPosYawPitch(position, 0.017453292f * yaw, pitch);

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        float fov = lua_tonumber(L, 6) * 0.017453292;

        if (fov > 0.017453292f) {
            if (fov >= 2.0943952f) {
                fov = 2.0943952f;
            }
        } else {
            fov = 0.017453292f;
        }

        CGCommentator::s_Commentator.m_fov = fov;
        CGCommentator::s_Commentator.SetFov(fov);
        camera->m_fov = fov;
    }

    return 0;
}

// VERIFY: 6c0f38d2 manual
// OFFSET: 0x0056A380
static int32_t Script_CommentatorGetCurrentMapID(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!player || !(player->m_player->m_playerFlags & 0x80000) || !CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance) || !instance) {
        return 0;
    }

    lua_pushnumber(L, instance->m_mapID);
    return 1;
}

// VERIFY: 321e094a manual
// OFFSET: 0x0056A410
static int32_t Script_CommentatorStartInstance(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x400000)) {
        return 0;
    }

    if (!CGCommentator::s_Commentator.m_battlemaster) {
        luaL_error(L, "Battlemaster must be set first.");
        return 0;
    }

    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
        luaL_error(L, "Usage: CommentatorStartInstance(mapID,teamSize,minLevel,maxLevel)");
        return 0;
    }

    uint32_t mapID = static_cast<uint32_t>(lua_tonumber(L, 1));
    uint32_t teamSize = static_cast<uint32_t>(lua_tonumber(L, 2));
    uint32_t minLevel = static_cast<uint32_t>(lua_tonumber(L, 3));
    uint32_t maxLevel = static_cast<uint32_t>(lua_tonumber(L, 4));

    if (teamSize != 2 && teamSize != 3 && teamSize != 5) {
        luaL_error(L, "Unknown team size. Use 2, 3, or 5.");
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_INSTANCE_COMMAND));
    msg.Put(static_cast<uint64_t>(0));
    msg.Put(static_cast<uint64_t>(0));
    msg.Put(CGCommentator::s_Commentator.m_battlemaster.m_guid);
    msg.Put(static_cast<uint32_t>(0));
    msg.Put(mapID);
    msg.Put(teamSize);
    msg.Put(minLevel);
    msg.Put(maxLevel);
    msg.Put(static_cast<uint32_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: f8c2e044 manual
// OFFSET: 0x0056A640
static int32_t Script_CommentatorAddPlayer(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x400000)) {
        return 0;
    }

    if (!CGGameUI::m_lockedTarget) {
        luaL_error(L, "A player must be targeted.");
        return 0;
    }

    CGObject_C* target = ClntObjMgrObjectPtr(player->m_unit->m_unitFieldTarget, TYPE_OBJECT, __FILE__, __LINE__);

    if (!target || !(target->m_obj->m_type & TYPE_PLAYER)) {
        luaL_error(L, "Must target a player.");
        return 0;
    }

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Must select an instance first.");
        return 0;
    }

    if (!CGCommentator::s_Commentator.m_battlemaster) {
        luaL_error(L, "Must set Battlemaster first.");
        return 0;
    }

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CommentatorAddPlayer(teamNumber)");
        return 0;
    }

    uint32_t team = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (team > 1) {
        team = 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_INSTANCE_COMMAND));
    msg.Put(instance->m_guid.m_guid);
    msg.Put(target->m_obj->m_guid.m_guid);
    msg.Put(CGCommentator::s_Commentator.m_battlemaster.m_guid);
    msg.Put(static_cast<uint32_t>(1));
    msg.Put(instance->m_mapID);
    msg.Put(map->unk_0000);
    msg.Put(map->unk_0004);
    msg.Put(map->unk_0008);
    msg.Put(team);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: d891d496 manual
// OFFSET: 0x0056A860
static int32_t Script_CommentatorRemovePlayer(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x400000)) {
        return 0;
    }

    CGCommentator::MapInfo* map;
    CGCommentator::InstanceInfo* instance;

    if (!CGCommentator::s_Commentator.GetCommentatorInfo(&map, &instance)) {
        luaL_error(L, "Must select an instance first.");
        return 0;
    }

    if (!CGCommentator::s_Commentator.m_battlemaster) {
        luaL_error(L, "Must set Battlemaster first.");
        return 0;
    }

    uint32_t faction = CGCommentator::s_Commentator.m_playerFaction;

    if (faction > 2) {
        luaL_error(L, "Error: Faction of current player is invalid.");
        return 0;
    }

    uint32_t index = CGCommentator::s_Commentator.m_playerIndex;

    if (index >= instance->m_numPlayers[faction]) {
        luaL_error(L, "Error: Index of current player is invalid.");
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_INSTANCE_COMMAND));
    msg.Put(instance->m_guid.m_guid);
    msg.Put(instance->m_players[faction][index].m_guid.m_guid);
    msg.Put(CGCommentator::s_Commentator.m_battlemaster.m_guid);
    msg.Put(static_cast<uint32_t>(2));
    msg.Put(instance->m_mapID);
    msg.Put(map->unk_0000);
    msg.Put(map->unk_0004);
    msg.Put(map->unk_0008);
    msg.Put(faction);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5a4b445c auto
// OFFSET: 0x0056AAF0
static int32_t Script_CommentatorSetMoveSpeed(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->IsCommentatorUberOrInArena()) {
        if (!lua_isnumber(L, 1)) {
            luaL_error(L, "Usage: CommentatorSetMoveSpeed(speed)");
            return 0;
        }

        CGCommentator::s_Commentator.SetMoveSpeed(lua_tonumber(L, 1));
    }

    return 0;
}

// VERIFY: ed7ce6f7 manual
// OFFSET: 0x0056AC90
static int32_t Script_CommentatorSetSkirmishMatchmakingMode(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_player->m_playerFlags & 0x80000) || !(player->m_player->m_playerFlags & 0x400000)) {
        return 0;
    }

    if (lua_isnumber(L, 1)) {
        uint8_t mode = static_cast<int32_t>(lua_tonumber(L, 1));
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_SKIRMISH_QUEUE_COMMAND));
        msg.Put(static_cast<uint32_t>(0));
        msg.Put(static_cast<uint32_t>(mode));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint32_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 42dd2161 manual
// OFFSET: 0x0056ADA0
static int32_t Script_CommentatorRequestSkirmishQueueData(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && (player->m_player->m_playerFlags & 0x80000)) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_SKIRMISH_QUEUE_COMMAND));
        msg.Put(static_cast<uint32_t>(1));
        msg.Put(static_cast<uint32_t>(1));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint32_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: cb620e1e manual
// OFFSET: 0x0056AE50
static int32_t Script_CommentatorRequestSkirmishMode(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && (player->m_player->m_playerFlags & 0x80000)) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_SKIRMISH_QUEUE_COMMAND));
        msg.Put(static_cast<uint32_t>(3));
        msg.Put(static_cast<uint32_t>(1));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint64_t>(0));
        msg.Put(static_cast<uint32_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 1af6c281 manual
// OFFSET: 0x0056B0B0
static int32_t Script_CommentatorStartSkirmishMatch(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && (player->m_player->m_playerFlags & 0x80000) && lua_isstring(L, 1) && lua_isstring(L, 2)) {
        WOWGUID first = StringToGuid(lua_tolstring(L, 1, nullptr));
        WOWGUID second = StringToGuid(lua_tolstring(L, 2, nullptr));
        int32_t team = -1;

        if (lua_isnumber(L, 3)) {
            team = static_cast<int32_t>(lua_tonumber(L, 3));
        }

        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_COMMENTATOR_SKIRMISH_QUEUE_COMMAND));
        msg.Put(static_cast<uint32_t>(2));
        msg.Put(static_cast<uint32_t>(1));
        msg.Put(first.m_guid);
        msg.Put(second.m_guid);
        msg.Put(static_cast<uint32_t>(team));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// STATIC: 0x00ACE370
static FrameScript_Method s_ScriptFunctions[] = {
    { "CommentatorSetMode", &Script_CommentatorSetMode },
    { "CommentatorToggleMode", &Script_CommentatorToggleMode },
    { "CommentatorGetMode", &Script_CommentatorGetMode },
    { "CommentatorSetMapAndInstanceIndex", &Script_CommentatorSetMapAndInstanceIndex },
    { "CommentatorSetPlayerIndex", &Script_CommentatorSetPlayerIndex },
    { "CommentatorUpdatePlayerInfo", &Script_CommentatorUpdatePlayerInfo },
    { "CommentatorUpdateMapInfo", &Script_CommentatorUpdateMapInfo },
    { "CommentatorGetNumMaps", &Script_CommentatorGetNumMaps },
    { "CommentatorGetMapInfo", &Script_CommentatorGetMapInfo },
    { "CommentatorGetInstanceInfo", &Script_CommentatorGetInstanceInfo },
    { "CommentatorEnterInstance", &Script_CommentatorEnterInstance },
    { "CommentatorExitInstance", &Script_CommentatorExitInstance },
    { "CommentatorGetNumPlayers", &Script_CommentatorGetNumPlayers },
    { "CommentatorGetPlayerInfo", &Script_CommentatorGetPlayerInfo },
    { "CommentatorFollowPlayer", &Script_CommentatorFollowPlayer },
    { "CommentatorLookatPlayer", &Script_CommentatorLookatPlayer },
    { "CommentatorZoomIn", &Script_CommentatorZoomIn },
    { "CommentatorZoomOut", &Script_CommentatorZoomOut },
    { "CommentatorSetCamera", &Script_CommentatorSetCamera },
    { "CommentatorGetCamera", &Script_CommentatorGetCamera },
    { "CommentatorGetCurrentMapID", &Script_CommentatorGetCurrentMapID },
    { "CommentatorStartInstance", &Script_CommentatorStartInstance },
    { "CommentatorAddPlayer", &Script_CommentatorAddPlayer },
    { "CommentatorRemovePlayer", &Script_CommentatorRemovePlayer },
    { "CommentatorSetBattlemaster", &Script_CommentatorSetBattlemaster },
    { "CommentatorSetMoveSpeed", &Script_CommentatorSetMoveSpeed },
    { "CommentatorSetCameraCollision", &Script_CommentatorSetCameraCollision },
    { "CommentatorSetTargetHeightOffset", &Script_CommentatorSetTargetHeightOffset },
    { "CommentatorSetSkirmishMatchmakingMode", &Script_CommentatorSetSkirmishMatchmakingMode },
    { "CommentatorRequestSkirmishQueueData", &Script_CommentatorRequestSkirmishQueueData },
    { "CommentatorGetSkirmishQueueCount", &Script_CommentatorGetSkirmishQueueCount },
    { "CommentatorGetSkirmishQueuePlayerInfo", &Script_CommentatorGetSkirmishQueuePlayerInfo },
    { "CommentatorStartSkirmishMatch", &Script_CommentatorStartSkirmishMatch },
    { "CommentatorRequestSkirmishMode", &Script_CommentatorRequestSkirmishMode },
    { "CommentatorGetSkirmishMode", &Script_CommentatorGetSkirmishMode },
};

// VERIFY: 7ac1891d auto
// OFFSET: 0x00568460
void CGCommentator::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 01b4e0a3 auto
// OFFSET: 0x00568490
void CGCommentator::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
