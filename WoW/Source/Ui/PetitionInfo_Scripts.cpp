#include "PetitionInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GuildRegistrar.h"
#include "WoW/Source/Ui/Types.h"

// VERIFY: b24a9953 manual
// OFFSET: 0x005CEAC0
static int32_t Script_GetNumPetitionNames(lua_State* L) {
    lua_pushnumber(L, CGPetitionInfo::m_numPetitionNames);
    return 1;
}

// VERIFY: 7299a8b6 manual
// OFFSET: 0x005CF140
static int32_t Script_SignPetition(lua_State* L) {
    uint8_t slot = 1;

    if (lua_isnumber(L, 1)) {
        slot = static_cast<uint8_t>(lua_tonumber(L, 1));
    }

    WOWGUID petition = CGPetitionInfo::m_petitionGUID;

    if (petition) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_PETITION_SIGN));
        msg.Put(petition.m_guid);
        msg.Put(slot);
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGPetitionInfo::m_signed = 1;
    }

    return 0;
}

// VERIFY: 5cff31d5 manual
// OFFSET: 0x005CF040
static int32_t Script_CanSignPetition(lua_State* L) {
    CGPetition* petition = CGPetitionInfo::m_petition;
    int32_t canSign = 1;

    if (petition) {
        if (petition->m_staticType == 0 || petition->m_staticType == 1) {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

            if ((!petition->m_staticType && player && player->m_player->m_playerGuildid) || static_cast<int32_t>(CGPetitionInfo::m_numPetitionNames) >= static_cast<int32_t>(petition->m_maxSignatures)) {
                canSign = 0;
            }
        }

        if (petition->m_ownerGuid == ClntObjMgrGetActivePlayer() || !canSign) {
            lua_pushnil(L);
            return 1;
        }
    }

    for (uint32_t i = 0; i < CGPetitionInfo::m_numPetitionNames; i++) {
        PetitionSignerInfo* signer = i < CGPetitionInfo::m_numPetitionNames ? &CGPetitionInfo::m_signers.m_data[i] : nullptr;

        if (signer && signer->m_guid == ClntObjMgrGetActivePlayer()) {
            lua_pushnil(L);
            return 1;
        }
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 4b99a298 auto
// OFFSET: 0x005CF020
static int32_t Script_ClosePetition(lua_State* L) {
    CGPetitionInfo::SetPetition(WOWGUID(), 0, WOWGUID());
    return 0;
}

// VERIFY: e8056899 manual
// OFFSET: 0x005CECE0
static int32_t Script_GetPetitionInfo(lua_State* L) {
    CGPetition* petition = CGPetitionInfo::m_petition;

    if (!petition) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 7;
    }

    if (petition->m_staticType == 0) {
        lua_pushstring(L, "guild");
    } else if (petition->m_staticType == 1) {
        lua_pushstring(L, "arena");
    } else {
        lua_pushstring(L, "other");
    }

    lua_pushstring(L, petition->m_title);
    lua_pushstring(L, petition->m_bodyText);
    lua_pushnumber(L, petition->m_maxSignatures);
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(petition->m_ownerGuid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        lua_pushstring(L, name->m_name);
    } else {
        lua_pushnil(L);
    }

    if (petition->m_ownerGuid == ClntObjMgrGetActivePlayer()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, petition->m_minSignatures);
    return 7;
}

// VERIFY: c92110d9 auto
// OFFSET: 0x005CEE30
static int32_t Script_GetPetitionNameInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetPetitionNameInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    PetitionSignerInfo* signer;
    NameCache* name;
    WOWGUID none;

    if (index < CGPetitionInfo::m_numPetitionNames && (signer = &CGPetitionInfo::m_signers.m_data[index]) && (none = WOWGUID(), name = g_nameDBCache.GetRecord(signer->m_guid.m_guid, none, nullptr, nullptr, false))) {
        lua_pushstring(L, name->m_name);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 9fd1ab6e manual
// OFFSET: 0x005CF220
static int32_t Script_OfferPetition(lua_State* L) {
    WOWGUID petitionGuid = CGPetitionInfo::m_petitionGUID;

    if (!petitionGuid || !CGGameUI::m_lockedTarget) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGPetition* petition = CGPetitionInfo::m_petition;

    if (!petition) {
        return 0;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lockedTarget, TYPE_UNIT, __FILE__, __LINE__));

    if (!target) {
        CGGameUI::DisplayError(ERR_OUT_OF_RANGE);
        return 0;
    }

    if (!(target->m_obj->m_type & TYPE_PLAYER)) {
        return 0;
    }

    if (player == target) {
        CGGameUI::DisplayError(ERR_PETITION_CREATOR);
        return 0;
    }

    if (!player->CanCooperate(target)) {
        CGGameUI::DisplayError(ERR_GUILD_NOT_ALLIED);
        return 0;
    }

    if (!petition->m_staticType && static_cast<CGPlayer_C*>(target)->m_player->m_playerGuildid) {
        CGGameUI::DisplayError(ERR_ALREADY_IN_GUILD_S, target->GetUnitName(nullptr, 1));
        return 0;
    }

    int32_t minLevel = petition->m_allowedMinLevel;

    if (minLevel > 0 && target->m_unit->m_unitFieldLevel < minLevel) {
        CGGameUI::DisplayError(ERR_ARENA_TEAM_TARGET_TOO_LOW_S, target->GetUnitName(nullptr, 1));
        return 0;
    }

    int32_t maxLevel = petition->m_allowedMaxLevel;

    if (maxLevel > 0 && target->m_unit->m_unitFieldLevel > maxLevel) {
        CGGameUI::DisplayError(ERR_ARENA_TEAM_TARGET_TOO_HIGH_S, target->GetUnitName(nullptr, 1));
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_OFFER_PETITION));

    if (petition->m_staticType == 1) {
        msg.Put(static_cast<uint32_t>(petition->m_minSignatures + 1));
    } else {
        msg.Put(static_cast<uint32_t>(0));
    }

    msg.Put(petitionGuid.m_guid);
    msg.Put(target->m_obj->m_guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::DisplayError(ERR_PETITION_OFFERED_S, target->GetUnitName(nullptr, 1));
    return 0;
}

// VERIFY: d967f9f8 auto
// OFFSET: 0x005CF450
static int32_t Script_RenamePetition(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage(RenamePetition(\"name\")");
        return 0;
    }

    WOWGUID petition = CGPetitionInfo::m_petitionGUID;

    if (!petition) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (CGGuildRegistrar::ValidateAndReportGuildName(name)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_PETITION_RENAME));
        msg.Put(petition.m_guid);
        msg.PutString(name);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// STATIC: 0x00AD0A60
static FrameScript_Method s_ScriptFunctions[] = {
    { "ClosePetition", &Script_ClosePetition },
    { "GetPetitionInfo", &Script_GetPetitionInfo },
    { "GetNumPetitionNames", &Script_GetNumPetitionNames },
    { "GetPetitionNameInfo", &Script_GetPetitionNameInfo },
    { "CanSignPetition", &Script_CanSignPetition },
    { "SignPetition", &Script_SignPetition },
    { "OfferPetition", &Script_OfferPetition },
    { "RenamePetition", &Script_RenamePetition },
};

// VERIFY: 6bda2f35 auto
// OFFSET: 0x005CEB00
void PetitionInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 92e8a70d auto
// OFFSET: 0x005CEB30
void PetitionInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
