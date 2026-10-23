#include "PaperDollInfoFrame.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/DB/Rec/PaperDollItemFrameRec.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"
#include <cstdlib>
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/Object/ArenaTeamCache.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Frame/CSimpleTexture.h"

// VERIFY: c886e670 auto
// OFFSET: 0x005E9BC0
static int32_t Script_GetInventoryItemTexture(lua_State* L) {
    const char* token;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, token, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(token));

    if (CGCharacterInfo::CanInspect(unit)) {
        int32_t displayId = 0;

        if (slot == -1) {
            if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
                int32_t ammoId = CGCharacterInfo::GetActivePlayerAmmoID(unit);

                if (ammoId && unit->GetBag()->GetItemTypeCount(ammoId, 0)) {
                    WOWGUID requester = unit->m_obj->m_guid;
                    ItemStats_C* stats = g_itemDBCache.GetRecord(ammoId, requester, UnitInventoryChangedCallback, nullptr, false);

                    if (stats) {
                        displayId = stats->m_displayId;
                    }
                }
            }
        } else {
            const VisibleItem_C* visible = unit->GetVisibleItem(slot);

            if (visible && visible->m_itemId) {
                displayId = const_cast<VisibleItem_C*>(visible)->GetDisplayID();
            } else {
                const CGItem_C* item = static_cast<const CGBag_C*>(unit->GetBag())->GetItemPointer(slot);

                if (item && !(item->m_modelFlags & 0x10000)) {
                    displayId = item->GetDisplayID();
                }
            }
        }

        if (displayId) {
            char texture[260];
            const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
            SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(displayId));
            lua_pushstring(L, texture);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: cfe5fc3d auto
// OFFSET: 0x005EAC00
static int32_t Script_UpdateInventoryAlertStatus(lua_State* L) {
    CGCharacterInfo::UpdateInventoryAlerts();
    return 0;
}

// VERIFY: 3d87fc58 auto
// OFFSET: 0x005E7780
static int32_t Script_HasInspectHonorData(lua_State* L) {
    if (CGCharacterInfo::m_hasHonorData) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 6245d206 auto
// OFFSET: 0x005E7700
static int32_t Script_KeyRingButtonIDToInvSlotID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: KeyRingButtonIDToInvSlotID(buttonID)");
        return 0;
    }

    lua_pushnumber(L, static_cast<int32_t>(lua_tonumber(L, 1)) + 86);
    return 1;
}

// VERIFY: eec95ec5 manual
// OFFSET: 0x005EAC10
static int32_t Script_OffhandHasWeapon(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGItem_C* item = player ? player->m_bag.GetItemPointer(16) : nullptr;

    if (item && item->GetClassID() == 2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a7d955ef manual
// OFFSET: 0x005E7FA0
static int32_t Script_GetInventoryAlertStatus(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInventoryAlertStatus(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t status = index >= CGCharacterInfo::m_invAlertStatus.m_count ? 0 : CGCharacterInfo::m_invAlertStatus.m_data[index];
    lua_pushnumber(L, status);
    return 1;
}

// VERIFY: cd7a1415 manual
// OFFSET: 0x005EAE90
static int32_t Script_HasWandEquipped(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && static_cast<uint32_t>(player->m_bag.m_numSlots) > 17 && player->m_bag.m_slots[17]) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(player->m_bag.m_slots[17], TYPE_ITEM, __FILE__, __LINE__));

        if (item && item->GetSubtypeID() == 19) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: ee365ef7 manual
// OFFSET: 0x005E77C0
static int32_t Script_GetInspectHonorData(lua_State* L) {
    lua_pushnumber(L, CGCharacterInfo::m_inspectTodayHonorableKills);
    lua_pushnumber(L, CGCharacterInfo::m_inspectTodayHonor);
    lua_pushnumber(L, CGCharacterInfo::m_inspectYesterdayHonorableKills);
    lua_pushnumber(L, CGCharacterInfo::m_inspectYesterdayHonor);
    lua_pushnumber(L, CGCharacterInfo::m_inspectLifetimeHonorableKills);
    lua_pushnumber(L, CGCharacterInfo::m_inspectLifetimeHighestRank);
    return 6;
}

// VERIFY: 10a2b29c manual
// OFFSET: 0x005E7D60
static int32_t Script_GetInventorySlotInfo(lua_State* L) {
    if (lua_isstring(L, 1)) {
        const char* name = lua_tolstring(L, 1, nullptr);
        int32_t count = g_paperDollItemFrameDB.m_numRecords;

        for (int32_t i = 0; i < count; i++) {
            PaperDollItemFrameRec* rec = i < 0 || i >= g_paperDollItemFrameDB.m_numRecords ? nullptr : &g_paperDollItemFrameDB.m_records[i];

            if (rec && !SStrCmpI(rec->m_itemButtonName, name, STORM_MAX_STR)) {
                lua_pushnumber(L, rec->m_slotNumber);
                lua_pushstring(L, rec->m_slotIcon);

                if (rec->m_slotNumber == 18) {
                    lua_pushnumber(L, 1.0);
                } else {
                    lua_pushnil(L);
                }

                return 3;
            }
        }
    }

    luaL_error(L, "Invalid inventory slot in GetInventorySlotInfo");
    return 0;
}

// VERIFY: 3c72664e manual
// OFFSET: 0x005EA6E0
static int32_t Script_IsInventoryItemLocked(lua_State* L) {
    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot)) {
        return 0;
    }

    CGPlayer_C* player = slot != -1 ? static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) : nullptr;
    const CGItem_C* item = player ? static_cast<const CGBag_C*>(&player->m_bag)->GetItemPointer(slot) : nullptr;

    if (item && (item->m_itemFlags & 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2fff9517 manual
// OFFSET: 0x005E7E60
static int32_t Script_GetInventoryItemCooldown(lua_State* L) {
    const char* unitName;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, unitName, slot)) {
        return 0;
    }

    CGPlayer_C* unit;
    CGItem_C* item;

    if (slot != -1 && CGCharacterInfo::CanInspect(unit = static_cast<CGPlayer_C*>(GetUnitFromName(unitName))) && (item = unit->m_bag.GetItemPointer(slot)) && item->GetEnchantmentSpell(0) && (!item->GetUsableSpellCharges() || item->GetCurrentSpellCharges())) {
        uint32_t start = 0;
        uint32_t duration = 0;
        uint32_t enable = 0;
        Spell_C_GetItemCooldown(item, &duration, &start, &enable);
        lua_pushnumber(L, start * 0.001);
        lua_pushnumber(L, duration * 0.001);
        lua_pushnumber(L, enable);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 3;
}

// VERIFY: fa692082 manual
// OFFSET: 0x005E9D80
static int32_t Script_GetInventoryItemBroken(lua_State* L) {
    const char* unitName;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, unitName, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(unitName));
    CGItem_C* item;

    if (!CGCharacterInfo::CanInspect(unit) || slot == -1 || !(item = unit->m_bag.GetItemPointer(slot)) || (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) || !item->m_item->m_itemFieldMaxdurability || item->GetDurability()) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 7734206d manual
// OFFSET: 0x005E9E40
static int32_t Script_GetInventoryItemCount(lua_State* L) {
    const char* unitName;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, unitName, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(unitName));

    if (!CGCharacterInfo::CanInspect(unit)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (slot == -1) {
        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            int32_t ammo = CGCharacterInfo::GetActivePlayerAmmoID(unit);

            if (ammo) {
                lua_pushnumber(L, unit->GetBag()->GetItemTypeCount(ammo, 0));
                return 1;
            }
        }

        lua_pushnumber(L, 1.0);
        return 1;
    }

    CGItem_C* item = unit->m_bag.GetItemPointer(slot);

    if (!item) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    if (!(item->m_obj->m_type & TYPE_CONTAINER)) {
        lua_pushnumber(L, item->m_item->m_itemFieldStackCount);
        return 1;
    }

    if (slot > 22) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = g_itemSubClassDB.GetRecordByIndex(i);

        if (rec->m_classID != static_cast<int32_t>(item->GetClassID()) || rec->m_subClassID != static_cast<int32_t>(item->GetSubtypeID())) {
            continue;
        }

        if (!(rec->m_displayFlags & 0x4)) {
            break;
        }

        if (item->GetBag()->GetItemTypeCount(-1, 0) <= 0) {
            lua_pushnumber(L, 0.0);
        } else {
            lua_pushnumber(L, item->GetBag()->GetItemTypeCount(-1, 0));
        }

        return 1;
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: 864b2e9b auto
// OFFSET: 0x005EA5F0
static int32_t Script_PickupInventoryItem(lua_State* L) {
    int32_t slot = 0;

    if (ResolveInventorySlotArg(L, 1, slot)) {
        CGCharacterInfo::PickupInventoryItem(slot);
    }

    return 0;
}

// VERIFY: c662896d auto
// OFFSET: 0x005EA7F0
static int32_t Script_PutItemInBackpack(lua_State* L) {
    if (CGCharacterInfo::PutItemInBackpack(0xFF)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 307bdc3f manual
// OFFSET: 0x005EA880
static int32_t Script_CursorCanGoInSlot(lua_State* L) {
    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot)) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if ((player && player->ValidateSlot(slot, CGGameUI::GetCursorItem())) || (slot == -1 && CGGameUI::m_cursorItemType == UICURSOR_AMMO)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: f4825e25 manual
// OFFSET: 0x005EACA0
static int32_t Script_GetWeaponEnchantInfo(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 6;
    }

    for (int32_t slot = 15; slot <= 16; slot++) {
        CGItem_C* item = player->m_bag.GetItemPointer(slot);

        if (item && item->GetEnchantmentId(1)) {
            lua_pushnumber(L, 1.0);

            if (item->GetEnchantmentDuration(1)) {
                lua_pushnumber(L, item->GetEnchantmentTimeLeft(1));
            } else {
                lua_pushnil(L);
            }

            lua_pushnumber(L, item->GetEnchantmentCharges(1));
        } else {
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushnil(L);
        }
    }

    return 6;
}

// VERIFY: d5cfb323 manual
// OFFSET: 0x005EAAF0
static int32_t Script_GetGuildInfo(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetGuildInfo(\"unit\")");
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 1, nullptr));

    if (unit && (unit->m_obj->m_type & TYPE_PLAYER)) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(unit);
        int32_t guildId = player->m_player->m_playerGuildid;

        if (guildId) {
            WOWGUID guid = unit->m_obj->m_guid;
            GuildStats_C* guild = g_guildInfoCache.GetRecord(guildId, guid, GuildNameCallback, nullptr, false);

            if (guild) {
                char rankName[64];
                SStrCopy(rankName, guild->m_rankNames[player->m_player->m_playerGuildrank], STORM_MAX_STR);
                lua_pushstring(L, guild->m_name);
                lua_pushstring(L, rankName);
                lua_pushnumber(L, player->m_player->m_playerGuildrank);
                return 3;
            }
        }
    }

    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnumber(L, 0.0);
    return 3;
}

// VERIFY: f2ea1f12 auto
// OFFSET: 0x005EA780
static int32_t Script_PutItemInBag(lua_State* L) {
    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot) || slot < 19) {
        return 0;
    }

    if (CGCharacterInfo::PutItemInBackpack(slot)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// STATIC: 0x00AD1270
// VERIFY: 092345f5 auto
// OFFSET: 0x005E7640
static void InspectArenaTeamCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_INSPECT_HONOR_UPDATE, nullptr);
    }
}

// VERIFY: f116b920 manual
// OFFSET: 0x005E7890
static int32_t Script_ClearInspectPlayer(lua_State* L) {
    WOWGUID none = WOWGUID();

    if (CGCharacterInfo::m_inspectTarget) {
        CGCharacterInfo::m_inspectTarget = WOWGUID();
        CGCharacterInfo::m_hasHonorData = 0;
        CGCharacterInfo::m_honorRequestPending = 0;
        CGCharacterInfo::m_inspectArenaTeams[0].m_teamId = 0;
        CGCharacterInfo::m_inspectArenaTeams[1].m_teamId = 0;
        CGCharacterInfo::m_inspectArenaTeams[2].m_teamId = 0;
        CGTalentInfo_Inspect::SetInspectTarget(none);
    }

    return 0;
}

// VERIFY: 626f663f manual
// OFFSET: 0x005E8030
static int32_t Script_GetInspectArenaTeamData(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInspectArenaTeamData(index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 3) {
        return 0;
    }

    InspectArenaTeam* team = &CGCharacterInfo::m_inspectArenaTeams[index];

    if (!team || !team->m_teamId) {
        return 0;
    }

    WOWGUID requester = CGCharacterInfo::m_inspectTarget;
    ArenaTeamCache* cache = g_arenaTeamCache.GetRecord(team->m_teamId, requester, &InspectArenaTeamCacheCallback, nullptr, false);

    if (!cache) {
        return 0;
    }

    lua_pushstring(L, cache->m_name);
    lua_pushnumber(L, static_cast<int32_t>(cache->m_type));
    lua_pushnumber(L, static_cast<int32_t>(team->m_rating));
    lua_pushnumber(L, static_cast<int32_t>(team->m_weekGames));
    lua_pushnumber(L, static_cast<int32_t>(team->m_weekWins));
    lua_pushnumber(L, static_cast<int32_t>(team->m_seasonGames));
    lua_pushnumber(L, static_cast<int32_t>(team->m_personalRating));
    uint32_t color = cache->m_backgroundColor;
    lua_pushnumber(L, static_cast<uint8_t>(color >> 16) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color >> 8) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color) * 0.0039215689);
    lua_pushnumber(L, static_cast<int32_t>(cache->m_emblemStyle));
    color = cache->m_emblemColor;
    lua_pushnumber(L, static_cast<uint8_t>(color >> 16) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color >> 8) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color) * 0.0039215689);
    lua_pushnumber(L, static_cast<int32_t>(cache->m_borderStyle));
    color = cache->m_borderColor;
    lua_pushnumber(L, static_cast<uint8_t>(color >> 16) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color >> 8) * 0.0039215689);
    lua_pushnumber(L, static_cast<uint8_t>(color) * 0.0039215689);
    return 18;
}

// VERIFY: 795ba989 manual
// OFFSET: 0x005E95C0
static int32_t Script_GetInventoryItemsForSlot(lua_State* L) {
    uint32_t slot = 0;

    if (lua_isnumber(L, 1)) {
        slot = static_cast<int32_t>(lua_tonumber(L, 1) - 1.0);

        if (slot > 18) {
            return 0;
        }
    } else {
        if (lua_isstring(L, 1)) {
            const char* name = lua_tolstring(L, 1, nullptr);

            for (int32_t i = 0; i < g_paperDollItemFrameDB.m_numRecords; i++) {
                PaperDollItemFrameRec* rec = i >= 0 && i < g_paperDollItemFrameDB.m_numRecords ? &g_paperDollItemFrameDB.m_records[i] : nullptr;

                if (rec && !SStrCmpI(rec->m_itemButtonName, name, STORM_MAX_STR)) {
                    slot = rec->m_slotNumber;
                }
            }
        }

        if (!slot) {
            luaL_error(L, "Usage: GetInventoryItemsForSlot(slot [, returnTable])");
            return 0;
        }
    }

    if (lua_type(L, 2) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGBag_C* inventory = player->GetBag();
    ChrClassesRec* chrClass = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]);

    if (!chrClass) {
        return 0;
    }

    int32_t bypass = (chrClass->m_flags & 0x8) != 0;

    for (int32_t i = 0; i <= 18; i++) {
        CGItem_C* item = inventory->GetItemPointer(i);

        if (item && item->CanGoInSlot(slot, bypass) && CGCharacterInfo::CanEquipItem(item, slot)) {
            lua_checkstack(L, 2);
            lua_pushnumber(L, i + 0x100001);
            lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
            lua_settable(L, -3);
        }
    }

    for (int32_t i = 23; i <= 38; i++) {
        CGItem_C* item = inventory->GetItemPointer(i);

        if (item && item->CanGoInSlot(slot, bypass)) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

            if (stats && CGCharacterInfo::CanEquipItemInSlot(stats, slot)) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, i + 0x2FFFEA);
                lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
                lua_settable(L, -3);
            }
        }
    }

    for (uint32_t bagIndex = 0; bagIndex < 4; bagIndex++) {
        CGObject_C* container = ClntObjMgrObjectPtr(GetBagAtIndex(bagIndex), TYPE_CONTAINER, __FILE__, __LINE__);

        if (!container) {
            continue;
        }

        CGBag_C* bag = container->GetBag();
        int32_t location = ((bagIndex + 1) << 8) | 0x300000;

        for (int32_t i = 0; i <= bag->m_numSlots; i++) {
            CGItem_C* item = bag->GetItemPointer(i);

            if (item && item->CanGoInSlot(slot, bypass)) {
                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats && CGCharacterInfo::CanEquipItemInSlot(stats, slot)) {
                    lua_checkstack(L, 2);
                    lua_pushnumber(L, i + location + 1);
                    lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
                    lua_settable(L, -3);
                }
            }
        }
    }

    if (!CGBankInfo::m_banker) {
        return 1;
    }

    for (int32_t i = 39; i <= 66; i++) {
        CGItem_C* item = inventory->GetItemPointer(i);

        if (item && item->CanGoInSlot(slot, bypass)) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

            if (stats && CGCharacterInfo::CanEquipItemInSlot(stats, slot)) {
                lua_checkstack(L, 2);
                lua_pushnumber(L, i + 0x400001);
                lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
                lua_settable(L, -3);
            }
        }
    }

    for (uint32_t bagIndex = 4; bagIndex < 11; bagIndex++) {
        CGObject_C* container = ClntObjMgrObjectPtr(GetBagAtIndex(bagIndex), TYPE_CONTAINER, __FILE__, __LINE__);

        if (!container) {
            continue;
        }

        CGBag_C* bag = container->GetBag();

        for (int32_t i = 0; i <= bag->m_numSlots; i++) {
            CGItem_C* item = bag->GetItemPointer(i);

            if (item && item->CanGoInSlot(slot, bypass)) {
                WOWGUID none = WOWGUID();
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats && CGCharacterInfo::CanEquipItemInSlot(stats, slot)) {
                    lua_checkstack(L, 2);
                    lua_pushnumber(L, static_cast<int32_t>((((bagIndex - 3) | 0x6000) << 8) + i + 1));
                    lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
                    lua_settable(L, -3);
                }
            }
        }
    }

    return 1;
}

// VERIFY: 07e57135 manual
// OFFSET: 0x005EA040
static int32_t Script_GetInventoryItemQuality(lua_State* L) {
    const char* token;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, token, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(token));

    if (!CGCharacterInfo::CanInspect(unit)) {
        lua_pushnil(L);
        return 1;
    }

    ItemStats_C* stats;

    if (slot != -1) {
        CGItem_C* item = unit->m_bag.GetItemPointer(slot);

        if (!item) {
            lua_pushnil(L);
            return 1;
        }

        WOWGUID none = WOWGUID();
        stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);
    } else {
        if (unit->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            lua_pushnil(L);
            return 1;
        }

        int32_t ammo = CGCharacterInfo::GetActivePlayerAmmoID(unit);

        if (!ammo) {
            lua_pushnil(L);
            return 1;
        }

        WOWGUID requester = unit->m_obj->m_guid;
        stats = g_itemDBCache.GetRecord(ammo, requester, &UnitInventoryChangedCallback, nullptr, false);
    }

    if (!stats) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, stats->m_inventoryType ? static_cast<int32_t>(stats->m_quality) : -1);
    return 1;
}

// VERIFY: 57bc036f manual
// OFFSET: 0x005EA170
static int32_t Script_GetInventoryItemDurability(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInventoryItemDurability(slot)");
        return 0;
    }

    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot) || slot == -1) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGItem_C* item = player->m_bag.GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    int32_t maxDurability = item->GetMaxDurability();

    if (!maxDurability) {
        return 0;
    }

    lua_pushnumber(L, static_cast<int32_t>(item->GetDurability()));
    lua_pushnumber(L, maxDurability);
    return 2;
}

// VERIFY: ab73b077 manual
// OFFSET: 0x005EA270
static int32_t Script_GetInventoryItemLink(lua_State* L) {
    const char* token;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, token, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(token));

    if (!CGCharacterInfo::CanInspect(unit)) {
        return 0;
    }

    WOWGUID guid = unit->m_obj->m_guid;
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (slot == -1) {
        int32_t ammo = guid == activePlayer ? CGCharacterInfo::GetActivePlayerAmmoID(unit) : 0;
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(ammo, none, nullptr, nullptr, false);

        if (stats) {
            lua_pushstring(L, CGTooltip::GetItemHyperlinkString(ammo, stats->m_quality, 0, 0, 0));
            return 1;
        }
    } else if (guid == activePlayer) {
        CGItem_C* item = unit->m_bag.GetItemPointer(slot);

        if (item) {
            lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
            return 1;
        }
    } else {
        InspectItem* inspected = unit->GetInspectedItem(slot);

        if (inspected && inspected->m_itemId) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(inspected->m_itemId, none, nullptr, nullptr, false);

            if (stats) {
                lua_pushstring(L, CGTooltip::CreateItemLink(inspected->m_itemId, stats->m_quality, inspected->m_enchants[0], &inspected->m_enchants[2], static_cast<uint16_t>(inspected->m_randomPropertiesId), inspected->m_propertySeed));
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 69e24afc manual
// OFFSET: 0x005EA3E0
static int32_t Script_GetInventoryItemID(lua_State* L) {
    const char* token;
    int32_t slot = 0;

    if (!GetUnitAndSlotFromLua(L, token, slot)) {
        return 0;
    }

    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(token));

    if (!CGCharacterInfo::CanInspect(unit)) {
        return 0;
    }

    WOWGUID guid = unit->m_obj->m_guid;
    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

    if (slot == -1) {
        if (guid == activePlayer) {
            int32_t ammo = CGCharacterInfo::GetActivePlayerAmmoID(unit);

            if (ammo) {
                lua_pushnumber(L, ammo);
                return 1;
            }
        }
    } else if (guid == activePlayer) {
        CGItem_C* item = unit->m_bag.GetItemPointer(slot);

        if (item) {
            lua_pushnumber(L, static_cast<int32_t>(item->m_obj->m_entryID));
            return 1;
        }
    } else {
        const VisibleItem_C* visible = unit->GetVisibleItem(slot);

        if (visible) {
            int32_t itemId = abs(visible->m_itemId);

            if (itemId) {
                lua_pushnumber(L, itemId);
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: b474e23d manual
// OFFSET: 0x005EA4F0
static int32_t Script_GetInventoryItemGems(lua_State* L) {
    int32_t slot;

    if (!lua_isnumber(L, 1) || (slot = 0, !ResolveInventorySlotArg(L, 1, slot))) {
        luaL_error(L, "Usage: GetInventoryItemGems(slot)");
        return 0;
    }

    if (slot == -1) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 3;
    }

    CGItem_C* item = player->m_bag.GetItemPointer(slot);

    if (!item) {
        return 0;
    }

    for (int32_t i = 0; i < 3; i++) {
        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(item->GetEnchantmentId(i + 2));

        if (enchant && enchant->m_srcItemID) {
            lua_pushnumber(L, enchant->m_srcItemID);
        } else {
            lua_pushnil(L);
        }
    }

    return 3;
}

// VERIFY: 3c11a723 auto
// OFFSET: 0x005EA630
static int32_t Script_UseInventoryItem(lua_State* L) {
    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot) || slot == -1) {
        return 0;
    }

    WOWGUID target;

    if (Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
        CGCharacterInfo::UseItem(slot, &target);
    }

    return 0;
}

// VERIFY: 8abca478 auto
// OFFSET: 0x005EA6A0
static int32_t Script_SocketInventoryItem(lua_State* L) {
    int32_t slot = 0;

    if (ResolveInventorySlotArg(L, 1, slot) && slot != -1) {
        CGCharacterInfo::SocketInventoryItem(slot);
    }

    return 0;
}

// VERIFY: d7ebadef auto
// OFFSET: 0x005EA840
static int32_t Script_PickupBagFromSlot(lua_State* L) {
    int32_t slot = 0;

    if (ResolveInventorySlotArg(L, 1, slot) && slot >= 19) {
        CGCharacterInfo::PickupBag(slot);
    }

    return 0;
}

// VERIFY: 6371d69b auto
// OFFSET: 0x005EA930
static int32_t Script_ShowInventorySellCursor(lua_State* L) {
    if (Spell_C_IsTargeting() || CursorGetResetMode() != POINT_CURSOR) {
        return 0;
    }

    int32_t slot = 0;

    if (!ResolveInventorySlotArg(L, 1, slot) || slot == -1) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    CGItem_C* item = player->GetBag()->GetItemPointer(slot);

    if (item && !(item->m_itemFlags & 0x1)) {
        CursorSetMode(BUY_CURSOR);
    }

    return 0;
}

// VERIFY: 8c2943fb manual
// OFFSET: 0x005EA9B0
static int32_t Script_SetInventoryPortraitTexture(lua_State* L) {
    CSimpleTexture* texture = static_cast<CSimpleTexture*>(FrameScript_GetObjectThis(L, CSimpleTexture::GetObjectType()));
    texture->SetTexture(static_cast<CTexture*>(nullptr));
    int32_t slot = 0;

    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetInventoryPortraitTexture(texture, unit, slot)");
        return 0;
    }

    if (!ResolveInventorySlotArg(L, 3, slot)) {
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 2, nullptr));

    if (!unit || !(unit->m_obj->m_type & TYPE_PLAYER)) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(unit);
    CGItem_C* item = slot == -1 ? player->m_bag.FindItemOfType(CGCharacterInfo::GetActivePlayerAmmoID(player), 0) : player->m_bag.GetItemPointer(slot);

    if (item) {
        const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
        char buffer[260];
        SStrPrintf(buffer, sizeof(buffer), "%s%s%s", path, *path ? "\\" : "", item->GetInventoryArt());
        SetPortraitTexture(texture, buffer);
    }

    return 0;
}

// VERIFY: 3325159b auto
// OFFSET: 0x005EAC90
static int32_t Script_RequestInspectHonorData(lua_State* L) {
    CGCharacterInfo::RequestInspectHonorData();
    return 0;
}

static FrameScript_Method s_ScriptFunctions[] = {
    { "GetInventorySlotInfo", &Script_GetInventorySlotInfo },
    { "GetInventoryItemsForSlot", &Script_GetInventoryItemsForSlot },
    { "GetInventoryItemTexture", &Script_GetInventoryItemTexture },
    { "GetInventoryItemBroken", &Script_GetInventoryItemBroken },
    { "GetInventoryItemCount", &Script_GetInventoryItemCount },
    { "GetInventoryItemQuality", &Script_GetInventoryItemQuality },
    { "GetInventoryItemCooldown", &Script_GetInventoryItemCooldown },
    { "GetInventoryItemDurability", &Script_GetInventoryItemDurability },
    { "GetInventoryItemLink", &Script_GetInventoryItemLink },
    { "GetInventoryItemID", &Script_GetInventoryItemID },
    { "GetInventoryItemGems", &Script_GetInventoryItemGems },
    { "KeyRingButtonIDToInvSlotID", &Script_KeyRingButtonIDToInvSlotID },
    { "PickupInventoryItem", &Script_PickupInventoryItem },
    { "UseInventoryItem", &Script_UseInventoryItem },
    { "SocketInventoryItem", &Script_SocketInventoryItem },
    { "IsInventoryItemLocked", &Script_IsInventoryItemLocked },
    { "PutItemInBag", &Script_PutItemInBag },
    { "PutItemInBackpack", &Script_PutItemInBackpack },
    { "PickupBagFromSlot", &Script_PickupBagFromSlot },
    { "CursorCanGoInSlot", &Script_CursorCanGoInSlot },
    { "ShowInventorySellCursor", &Script_ShowInventorySellCursor },
    { "SetInventoryPortraitTexture", &Script_SetInventoryPortraitTexture },
    { "GetGuildInfo", &Script_GetGuildInfo },
    { "GetInventoryAlertStatus", &Script_GetInventoryAlertStatus },
    { "UpdateInventoryAlertStatus", &Script_UpdateInventoryAlertStatus },
    { "OffhandHasWeapon", &Script_OffhandHasWeapon },
    { "HasInspectHonorData", &Script_HasInspectHonorData },
    { "RequestInspectHonorData", &Script_RequestInspectHonorData },
    { "GetInspectHonorData", &Script_GetInspectHonorData },
    { "GetInspectArenaTeamData", &Script_GetInspectArenaTeamData },
    { "ClearInspectPlayer", &Script_ClearInspectPlayer },
    { "GetWeaponEnchantInfo", &Script_GetWeaponEnchantInfo },
    { "HasWandEquipped", &Script_HasWandEquipped },
};

// VERIFY: ee30d944 auto
// OFFSET: 0x005E78F0
void CharacterInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 0cab0d3c auto
// OFFSET: 0x005E7920
void CharacterInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
