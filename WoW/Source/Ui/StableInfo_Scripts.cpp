#include "StableInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/ItemPetFoodRec.h"
#include "WoW/Source/DB/Rec/TalentTabRec.h"
#include "WoW/Source/DB/Rec/StableSlotPricesRec.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Object/CreatureStats.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CharacterModelBase.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// VERIFY: 79ef9242 auto
// OFFSET: 0x005A0E80
static void CreatureStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_STABLE_UPDATE, nullptr);
    }
}

// VERIFY: 578484fe auto
// OFFSET: 0x005A0EA0
static void CreatureStatsSelectCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_STABLE_UPDATE_PAPERDOLL, nullptr);
    }
}

// VERIFY: e307d8b0 manual
// OFFSET: 0x005A0F60
static int32_t Script_GetNumStablePets(lua_State* L) {
    lua_pushnumber(L, static_cast<int32_t>(CGStableInfo::m_numPets));
    return 1;
}

// VERIFY: 25cd41b2 manual
// OFFSET: 0x005A0FA0
static int32_t Script_GetNumStableSlots(lua_State* L) {
    lua_pushnumber(L, CGStableInfo::m_numSlots);
    return 1;
}

// VERIFY: ef5dfe07 auto
// OFFSET: 0x005A0FE0
static int32_t Script_PickupStablePet(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupStablePet(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    StablePetInfo* pet = index >= 0 ? (static_cast<uint32_t>(index) >= CGStableInfo::m_numPets ? nullptr : &CGStableInfo::m_pets[index + 1]) : &CGStableInfo::m_pets[0];

    if (pet && pet->m_petNumber) {
        CGGameUI::SetCursorPet(index);
    }

    return 0;
}

// VERIFY: 257f7f1f auto
// OFFSET: 0x005A1060
static int32_t Script_GetSelectedStablePet(lua_State* L) {
    lua_pushnumber(L, CGStableInfo::GetSelectedPetIndex() + 1);
    return 1;
}

// VERIFY: 7667c9c1 auto
// OFFSET: 0x005A1090
static int32_t Script_IsAtStableMaster(lua_State* L) {
    if (CGStableInfo::m_stableMaster) {
        lua_pushboolean(L, 1);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

// VERIFY: cea8f24c manual
// OFFSET: 0x005A1330
static int32_t Script_GetStablePetInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetStablePetInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    StablePetInfo* pet = index >= 0 ? (static_cast<uint32_t>(index) >= CGStableInfo::m_numPets ? nullptr : &CGStableInfo::m_pets[index + 1]) : &CGStableInfo::m_pets[0];
    CreatureFamilyRec* family = nullptr;

    if (pet) {
        WOWGUID requester = ClntObjMgrGetActivePlayer();
        CreatureStats_C* stats = g_creatureDBCache.GetRecord(pet->m_entry, requester, &CreatureStatsCallback, nullptr, false);

        if (stats) {
            family = g_creatureFamilyDB.GetRecord(stats->m_family);
        }
    }

    if (!family) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 3;
    }

    lua_pushstring(L, family->m_iconFile);
    lua_pushstring(L, pet->m_name);
    lua_pushnumber(L, pet->m_level);
    lua_pushstring(L, family->m_name);

    if (family->m_petTalentType < 0) {
        return 4;
    }

    for (int32_t i = 0; i < g_talentTabDB.m_numRecords; i++) {
        TalentTabRec* tab = g_talentTabDB.GetRecordByIndex(i);

        if ((1 << family->m_petTalentType) & tab->m_categoryEnumID) {
            lua_pushstring(L, tab->m_name);
            return 5;
        }
    }

    return 4;
}

// VERIFY: c2ffaa4d auto
// OFFSET: 0x005A14D0
static int32_t Script_GetNextStableSlotCost(lua_State* L) {
    StableSlotPricesRec* price = g_stableSlotPricesDB.GetRecord(CGStableInfo::m_numSlots + 1);
    lua_pushnumber(L, price ? price->m_cost : 0);
    return 1;
}

// VERIFY: 75685864 auto
// OFFSET: 0x005A1530
static int32_t Script_SetPetStablePaperdoll(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        luaL_error(L, "Usage: SetPetStablePaperdoll(characterModel)");
        return 0;
    }

    lua_rawgeti(L, 1, 0);
    CGCharacterModelBase* model = static_cast<CGCharacterModelBase*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!model) {
        luaL_error(L, "SetPetStablePaperdoll(): Couldn't find 'this' in frame object");
        return 0;
    }

    if (!model->IsA(CGCharacterModelBase::GetObjectType())) {
        luaL_error(L, "SetPetStablePaperdoll(): Wrong object type, expected frame");
        return 0;
    }

    int32_t selected = CGStableInfo::GetSelectedPetIndex();
    CreatureStats_C* stats;

    if (selected == -1) {
        CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

        if (pet && pet->CanGainXP() && pet->m_creatureCacheEntry) {
            stats = pet->m_creatureCacheEntry;
        } else {
            WOWGUID requester = ClntObjMgrGetActivePlayer();
            stats = g_creatureDBCache.GetRecord(CGStableInfo::m_pets[0].m_entry, requester, &CreatureStatsSelectCallback, nullptr, false);
        }
    } else {
        if (selected < 0) {
            return 0;
        }

        StablePetInfo* info = CGStableInfo::GetPet(selected);

        if (!info) {
            return 0;
        }

        WOWGUID requester = ClntObjMgrGetActivePlayer();
        stats = g_creatureDBCache.GetRecord(info->m_entry, requester, &CreatureStatsSelectCallback, nullptr, false);
    }

    if (stats) {
        model->SetCreature(stats);
    }

    return 0;
}

// VERIFY: 9aaf1241 manual
// OFFSET: 0x005A16A0
static int32_t Script_GetStablePetFoodTypes(lua_State* L) {
    int32_t count = 0;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetStablePetInfo(index)");
        return 0;
    }

    int32_t number = static_cast<int32_t>(lua_tonumber(L, 1));
    StablePetInfo* pet;

    if (number) {
        uint32_t index = number - 1;

        if (index >= CGStableInfo::m_numPets) {
            return count;
        }

        pet = &CGStableInfo::m_pets[index + 1];
    } else {
        pet = &CGStableInfo::m_pets[0];
    }

    WOWGUID requester = ClntObjMgrGetActivePlayer();
    CreatureStats_C* stats = g_creatureDBCache.GetRecord(pet->m_entry, requester, &CreatureStatsCallback, nullptr, false);

    if (!stats) {
        return count;
    }

    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(stats->m_family);

    if (!family) {
        return count;
    }

    for (int32_t i = 0; i < g_itemPetFoodDB.m_numRecords; i++) {
        ItemPetFoodRec* food = g_itemPetFoodDB.GetRecordByIndex(i);

        if (food && ((1 << (food->m_id - 1)) & family->m_petFoodMask)) {
            lua_checkstack(L, 1);
            lua_pushstring(L, food->m_name);
            count++;
        }
    }

    return count;
}

// VERIFY: ab5803a7 manual
// OFFSET: 0x005A1950
static int32_t Script_ClosePetStables(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->CancelOpenStable();
    }

    CGStableInfo::CloseStables(false);
    return 0;
}

// VERIFY: 7af7212e manual
// OFFSET: 0x005A19C0
static int32_t Script_StablePet(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (CGStableInfo::m_stableMaster || (player && player->HasAuraEffect(SPELL_AURA_OPEN_STABLE))) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_STABLE_PET));
        msg.Put(CGStableInfo::m_stableMaster.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: e3cae124 manual
// OFFSET: 0x005A1AC0
static int32_t Script_UnstablePet(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: UnstablePet(index)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->m_unit->m_unitFieldCharm || player->m_unit->m_unitFieldSummon) {
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= CGStableInfo::m_numPets) {
        return 0;
    }

    StablePetInfo* pet = &CGStableInfo::m_pets[index + 1];
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_UNSTABLE_PET));
    msg.Put(CGStableInfo::m_stableMaster.m_guid);
    msg.Put(pet->m_petNumber);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: b462b5e5 manual
// OFFSET: 0x005A1BD0
static int32_t Script_BuyStableSlot(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !CGStableInfo::m_stableMaster || CGStableInfo::m_numSlots == 4) {
        return 0;
    }

    StableSlotPricesRec* price = g_stableSlotPricesDB.GetRecord(CGStableInfo::m_numSlots + 1);

    if (price && player->GetCoinage() >= static_cast<uint32_t>(price->m_cost)) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_BUY_STABLE_SLOT));
        msg.Put(CGStableInfo::m_stableMaster.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 0ca22339 manual
// OFFSET: 0x005A1CA0
static int32_t Script_ClickStablePet(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ClickStablePet(index)");
        return 0;
    }

    double number = lua_tonumber(L, 1);
    uint32_t index = static_cast<int32_t>(number) - 1;

    if (CGGameUI::m_cursorItemType != UICURSOR_PET) {
        CGStableInfo::SelectPet(static_cast<int32_t>(number) - 1);
        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (CGGameUI::m_cursorPet == static_cast<int32_t>(index)) {
        CGGameUI::ClearCursor(1, 1);
        lua_pushnil(L);
        return 1;
    }

    if (CGGameUI::m_cursorPet != -1) {
        if (!number) {
            StablePetInfo* pet = CGStableInfo::GetPet(CGGameUI::m_cursorPet);

            if (pet && pet->m_petNumber) {
                WDataStore msg;
                msg.Put(static_cast<uint32_t>(CGPetInfo::GetPet(0) ? CMSG_STABLE_SWAP_PET : CMSG_UNSTABLE_PET));
                msg.Put(CGStableInfo::m_stableMaster.m_guid);
                msg.Put(pet->m_petNumber);
                msg.Finalize();
                ClientServices::Send2(&msg);
            }
        }
    } else {
        StablePetInfo* pet = CGStableInfo::GetPet(static_cast<int32_t>(number) - 1);

        if (pet && pet->m_petNumber) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_STABLE_SWAP_PET));
            msg.Put(CGStableInfo::m_stableMaster.m_guid);
            msg.Put(pet->m_petNumber);
            msg.Finalize();
            ClientServices::Send2(&msg);
        } else if (index < static_cast<uint32_t>(CGStableInfo::m_numSlots)) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_STABLE_PET));
            msg.Put(CGStableInfo::m_stableMaster.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    CGGameUI::ClearCursor(1, 1);
    lua_pushnil(L);
    return 1;
}

// STATIC: 0x00ACF768
static FrameScript_Method s_ScriptFunctions[] = {
    { "ClosePetStables", &Script_ClosePetStables },
    { "StablePet", &Script_StablePet },
    { "UnstablePet", &Script_UnstablePet },
    { "BuyStableSlot", &Script_BuyStableSlot },
    { "GetNumStablePets", &Script_GetNumStablePets },
    { "GetNumStableSlots", &Script_GetNumStableSlots },
    { "GetStablePetInfo", &Script_GetStablePetInfo },
    { "GetNextStableSlotCost", &Script_GetNextStableSlotCost },
    { "ClickStablePet", &Script_ClickStablePet },
    { "PickupStablePet", &Script_PickupStablePet },
    { "GetSelectedStablePet", &Script_GetSelectedStablePet },
    { "SetPetStablePaperdoll", &Script_SetPetStablePaperdoll },
    { "GetStablePetFoodTypes", &Script_GetStablePetFoodTypes },
    { "IsAtStableMaster", &Script_IsAtStableMaster },
};

// VERIFY: 11c70b44 auto
// OFFSET: 0x005A10D0
void StableInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 27892ca3 auto
// OFFSET: 0x005A1100
void StableInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
