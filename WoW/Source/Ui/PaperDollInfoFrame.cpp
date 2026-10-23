#include "PaperDollInfoFrame.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/WowServices/WDataStore.h"

// STATIC: 0x00A1CE4C
static const int32_t s_inventoryAlertSlots[12] = { 0, 2, 4, 5, 6, 7, 8, 9, 15, 16, 17, -1 };

// STATIC: 0x00C24230
TSFixedArray<int32_t> CGCharacterInfo::m_invAlertStatus;

// STATIC: 0x00C24220
WOWGUID CGCharacterInfo::m_inspectTarget;

// STATIC: 0x00C24228
int32_t CGCharacterInfo::m_hasHonorData = -1;

// STATIC: 0x00C241C0
InspectArenaTeam CGCharacterInfo::m_inspectArenaTeams[3];

// STATIC: 0x00C24208
uint32_t CGCharacterInfo::m_inspectLifetimeHonorableKills;

// STATIC: 0x00C2420C
uint32_t CGCharacterInfo::m_inspectYesterdayHonor;

// STATIC: 0x00C24210
uint32_t CGCharacterInfo::m_inspectTodayHonor;

// STATIC: 0x00C24214
uint16_t CGCharacterInfo::m_inspectYesterdayHonorableKills;

// STATIC: 0x00C24216
uint8_t CGCharacterInfo::m_inspectLifetimeHighestRank;

// STATIC: 0x00C24218
uint16_t CGCharacterInfo::m_inspectTodayHonorableKills;

// STATIC: 0x00C2422C
int32_t CGCharacterInfo::m_honorRequestPending;

// VERIFY: d0d1018f manual
// OFFSET: 0x005E83E0
int32_t PlayerEquipmentChangedHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateAmmoModel(player->GetEquippedAmmoDisplayId(), 24);
        CGActionBar::UpdateUsable();
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_INVENTORY_CHANGED, "%s", "player");
    }

    return 1;
}

// VERIFY: eebb5365 manual
// OFFSET: 0x005E8330
int32_t PlayerCharacterPointsUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        const int32_t* oldPoints = static_cast<const int32_t*>(oldValue);
        int32_t points2 = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerCharacterPoints[1] : 0;
        int32_t points1 = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerCharacterPoints[0] : 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_CHARACTER_POINTS_CHANGED, "%d%d", points1 - oldPoints[0], points2 - oldPoints[1]);
    }

    return 1;
}

// VERIFY: 37e279b1 auto
// OFFSET: 0x005E8440
void CGCharacterInfo::InitializeGame() {
    ClientServices::SetMessageHandler(MSG_INSPECT_HONOR_STATS, Packet_MSG_INSPECT_HONOR_STATS, nullptr);
    ClientServices::SetMessageHandler(MSG_INSPECT_ARENA_TEAMS, Packet_MSG_INSPECT_ARENA_TEAMS, nullptr);
}

// VERIFY: 174a574b auto
// OFFSET: 0x005E8470
void CGCharacterInfo::RegisterMirrorHandlers(WOWGUID guid) {
    if (!guid) {
        return;
    }

    ClntObjMgrSetObjMirrorHandler(guid, ID_PLAYER, 0xDA0, 8, PlayerCharacterPointsUpdateHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
    ClntObjMgrSetObjMirrorHandler(guid, ID_PLAYER, 0x1068, 4, PlayerEquipmentChangedHandler, nullptr, HANDLER_PRIORITY_NORMAL, 0);
}

// VERIFY: 18a6a029 manual
// OFFSET: 0x005E90D0
void CGCharacterInfo::UpdateInventoryAlerts() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    bool changed = false;

    for (uint32_t i = 0; i < 12; i++) {
        int32_t status = CGCharacterInfo::CalculateInventoryAlertStatus(player, i);

        if (status != CGCharacterInfo::m_invAlertStatus.m_data[i]) {
            CGCharacterInfo::m_invAlertStatus.m_data[i] = status;
            changed = true;
        }
    }

    if (changed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_INVENTORY_ALERTS, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_INVENTORY_DURABILITY, nullptr);
}

// VERIFY: 315ec719 auto
// OFFSET: 0x005EAF30
void CGCharacterInfo::EnterWorld() {
    CGCharacterInfo::RegisterMirrorHandlers(ClntObjMgrGetActivePlayer());
    CGCharacterInfo::m_invAlertStatus.SetCount(12);
    CGCharacterInfo::UpdateInventoryAlerts();
}

// VERIFY: e09601e2 auto
// OFFSET: 0x005E7660
void UnitInventoryChangedCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_INVENTORY_CHANGED, "%s", "player");
    }
}

// VERIFY: 45ff113c manual
// OFFSET: 0x005E7A00
int32_t CGCharacterInfo::CanInspect(const CGPlayer_C* unit) {
    if (!unit) {
        return 0;
    }

    return unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() || unit->m_obj->m_guid == CGCharacterInfo::m_inspectTarget;
}

// VERIFY: 97f574c5 manual
// OFFSET: 0x005E7C00
int32_t ResolveInventorySlotArg(lua_State* L, int32_t index, int32_t& slot) {
    if (lua_isnumber(L, index)) {
        slot = static_cast<int32_t>(lua_tonumber(L, index)) - 1;
    } else if (lua_isstring(L, index)) {
        const char* name = lua_tolstring(L, index, nullptr);

        for (int32_t i = 0; i < g_paperDollItemFrameDB.m_numRecords; i++) {
            PaperDollItemFrameRec* rec = i >= 0 && i < g_paperDollItemFrameDB.m_numRecords ? &g_paperDollItemFrameDB.m_records[i] : nullptr;

            if (!SStrCmpI(rec->m_itemButtonName, name, STORM_MAX_STR)) {
                slot = rec->m_slotNumber;
                break;
            }
        }
    }

    return (slot >= 0 && slot <= 22) || (slot >= 39 && slot <= 66) || (slot >= 67 && slot <= 73) || slot == -1 || (slot >= 86 && slot <= 117);
}

// VERIFY: f340d760 auto
// OFFSET: 0x005E7CF0
int32_t GetUnitAndSlotFromLua(lua_State* L, const char*& unit, int32_t& slot) {
    if (!lua_isstring(L, 1)) {
        char buffer[1024];
        luaL_error(L, "Usage: %s(unit, slot)", FrameScript_GetCurrentFunction(L, buffer, sizeof(buffer)));
    }

    unit = lua_tolstring(L, 1, nullptr);
    return ResolveInventorySlotArg(L, 2, slot);
}

// VERIFY: d19c5b13 manual
// OFFSET: 0x005E8300
int32_t CGCharacterInfo::GetActivePlayerAmmoID(const CGPlayer_C* player) {
    if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
        return player->m_player->m_playerAmmoId;
    }

    return 0;
}

// VERIFY: b07082fe auto
// OFFSET: 0x005E7A50
int32_t Packet_MSG_INSPECT_HONOR_STATS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if (guid != CGCharacterInfo::m_inspectTarget) {
        msg->m_read = msg->m_size;
        return 1;
    }

    msg->Get(CGCharacterInfo::m_inspectLifetimeHighestRank);
    msg->Get(CGCharacterInfo::m_inspectTodayHonorableKills);
    msg->Get(CGCharacterInfo::m_inspectYesterdayHonorableKills);
    msg->Get(CGCharacterInfo::m_inspectTodayHonor);
    msg->Get(CGCharacterInfo::m_inspectYesterdayHonor);
    msg->Get(CGCharacterInfo::m_inspectLifetimeHonorableKills);
    CGCharacterInfo::m_hasHonorData = 1;
    CGCharacterInfo::m_honorRequestPending = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_INSPECT_HONOR_UPDATE, nullptr);
    return 1;
}

// VERIFY: f1172d97 auto
// OFFSET: 0x005E7B00
int32_t Packet_MSG_INSPECT_ARENA_TEAMS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if (guid != CGCharacterInfo::m_inspectTarget) {
        msg->m_read = msg->m_size;
        return 1;
    }

    uint8_t slot;
    msg->Get(slot);

    if (slot > 3) {
        msg->m_read = msg->m_size;
        return 1;
    }

    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_teamId);
    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_rating);
    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_weekGames);
    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_weekWins);
    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_seasonGames);
    msg->Get(CGCharacterInfo::m_inspectArenaTeams[slot].m_personalRating);
    FrameScript_SignalEvent(SCRIPT_EVENT_INSPECT_HONOR_UPDATE, nullptr);
    return 1;
}

// VERIFY: aedc8113 auto
// OFFSET: 0x005E76B0
void CGCharacterInfo::SetInspectTarget(const WOWGUID& guid) {
    if (guid == CGCharacterInfo::m_inspectTarget) {
        return;
    }

    CGCharacterInfo::m_inspectTarget = guid;
    CGCharacterInfo::m_hasHonorData = 0;
    CGCharacterInfo::m_honorRequestPending = 0;

    for (uint32_t i = 0; i < 3; i++) {
        CGCharacterInfo::m_inspectArenaTeams[i].m_teamId = 0;
    }

    CGTalentInfo_Inspect::SetInspectTarget(guid);
}

// VERIFY: dcb1d352 manual
// OFFSET: 0x005E84C0
void CGCharacterInfo::UpdateItem(WOWGUID guid) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));
    int32_t slot = player->m_bag.FindItemSlot(guid);

    if (slot == -1 && item && item->m_item->m_itemFieldContained != ClntObjMgrGetActivePlayer()) {
        slot = player->m_bag.FindItemSlot(item->m_item->m_itemFieldContained);
    }

    if (static_cast<uint32_t>(slot) > 22) {
        if ((slot >= 39 && slot <= 66) || static_cast<uint32_t>(slot - 67) <= 6) {
            FrameScript_SignalEvent(SCRIPT_EVENT_PLAYERBANKSLOTS_CHANGED, "%d", slot - 38);
        }

        if (!item || item->m_obj->m_entryID != CGCharacterInfo::GetActivePlayerAmmoID(player)) {
            return;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_INVENTORY_CHANGED, "%s", "player");
}

// VERIFY: 8ae9eb90 auto
// OFFSET: 0x005E79D0
static void BuyItemInBag(WOWGUID merchant, const MerchantItem* item, uint32_t count, WOWGUID bag) {
    CGPlayer_C::BuyItemInSlot(merchant, item, count, bag, 0xFF);
}

// VERIFY: 0f75a387 manual
// OFFSET: 0x005E85D0
void CGCharacterInfo::PickupInventoryItem(int32_t slot) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || (!CGGameUI::m_hasControl && !(player->m_unit->m_unitFieldFlags & 0x100000))) {
        return;
    }

    if (((slot >= 39 && slot <= 66) || static_cast<uint32_t>(slot - 67) <= 6) && !CGBankInfo::m_banker) {
        return;
    }

    WOWGUID cursorItem;
    WOWGUID cursorContainer;
    uint32_t cursorSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
    int32_t virtualId;
    int32_t virtualSlot;
    CGGameUI::GetCursorVirtualItem(virtualId, virtualSlot);

    if (slot == -1) {
        if (cursorItem) {
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cursorItem, TYPE_ITEM, __FILE__, __LINE__));

            if (item && item->GetClassID() == 6) {
                CDataStore msg;
                msg.Put(static_cast<uint32_t>(CMSG_SET_AMMO));
                msg.Put(static_cast<uint32_t>(item->m_obj->m_entryID));
                msg.Finalize();
                ClientServices::Send2(&msg);
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            player->AutoEquipCursorItem(0);
            CGGameUI::ClearCursor(1, 1);
            return;
        }

        if (virtualId && CGGameUI::m_cursorItemType == UICURSOR_AMMO) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_SET_AMMO));
            msg.Put(static_cast<uint32_t>(virtualId));
            msg.Finalize();
            ClientServices::Send2(&msg);
            CGGameUI::ClearCursor(1, 1);
            return;
        }

        int32_t ammo = CGCharacterInfo::GetActivePlayerAmmoID(player);

        if (!ammo || !player->GetBag()->GetItemTypeCount(ammo, 0)) {
            CGGameUI::ClearCursor(1, 1);
            return;
        }

        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(ammo, none, nullptr, nullptr, false);

        if (stats) {
            CGGameUI::SetCursorVirtualItem(ammo, stats->m_displayId, 0, UICURSOR_AMMO, 0);
        }

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_AMMO));
        msg.Put(static_cast<uint32_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
        return;
    }

    WOWGUID itemGuid;
    itemGuid.m_guid = player->m_bag.GetItemGuid(slot);

    if (cursorItem) {
        if (itemGuid == cursorItem) {
            CGGameUI::ClearCursor(1, 1);
            return;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPE_ITEM, __FILE__, __LINE__));

        if (item && (item->m_itemFlags & 1)) {
            return;
        }

        if (!player->ValidateSlot(slot, cursorItem)) {
            if (cursorContainer == player->m_obj->m_guid && cursorSlot < 19) {
                CGGameUI::ClearCursor(1, 1);
                return;
            }

            player->AutoEquipCursorItem(0);
        } else {
            player->SwapItems(cursorItem, cursorContainer, cursorSlot, player->m_obj->m_guid, slot, 0);
            CGGameUI::LockItem(itemGuid);
        }

        return;
    }

    if (virtualId) {
        if (CGGameUI::m_cursorItemType == UICURSOR_MERCHANT && CGMerchantInfo::m_merchant) {
            MerchantItem* merchantItem = CGMerchantInfo::GetItem(virtualSlot);

            if (merchantItem) {
                CGPlayer_C::BuyItemInSlot(CGMerchantInfo::m_merchant, merchantItem, 1, player->m_obj->m_guid, slot);
                CGGameUI::ClearCursor(1, 1);
            }
        }

        return;
    }

    if (!itemGuid) {
        return;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPE_ITEM, __FILE__, __LINE__));

    if (item && (item->m_itemFlags & 1)) {
        return;
    }

    if (Spell_C_IsTargeting() && Spell_C_CanTargetItems()) {
        CGGameUI::CastItemSpell(itemGuid, 0);
        return;
    }

    if (item && CursorGetResetMode() == REPAIR_CURSOR && CGMerchantInfo::CanRepair()) {
        uint32_t cost = CGMerchantInfo::GetRepairCost(item);

        if (cost > player->GetCoinage()) {
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            return;
        }

        SI2::PlaySoundKit("ITEM_REPAIR", nullptr, nullptr, nullptr);
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REPAIR_ITEM));
        msg.Put(CGMerchantInfo::m_merchant.m_guid);
        msg.Put(itemGuid.m_guid);
        msg.Put(static_cast<uint8_t>(0));
        msg.Finalize();
        ClientServices::Send2(&msg);
        return;
    }

    CGGameUI::SetCursorItem(itemGuid, player->m_obj->m_guid, slot, 1, 0, 1);
    CGGameUI::LockItem(itemGuid);
}

// VERIFY: f443ce5b manual
// OFFSET: 0x005E8A60
void CGCharacterInfo::UseItem(int32_t slot, WOWGUID* target) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CGGameUI::ClearCursor(1, 1);
    CGItem_C* item = player->m_bag.GetItemPointer(slot);

    if (!item) {
        return;
    }

    if (Spell_C_IsTargeting() && !Spell_C_CanTargetItems()) {
        Spell_C_StopTargeting();
    }

    if (Spell_C_IsTargeting()) {
        CGGameUI::CastItemSpell(item->m_obj->m_guid, 0);
    } else if (CursorGetResetMode() == REPAIR_CURSOR && CGMerchantInfo::CanRepair()) {
        uint32_t cost = CGMerchantInfo::GetRepairCost(item);

        if (cost <= player->GetCoinage()) {
            SI2::PlaySoundKit("ITEM_REPAIR", nullptr, nullptr, nullptr);
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_REPAIR_ITEM));
            msg.Put(CGMerchantInfo::m_merchant.m_guid);
            msg.Put(item->m_obj->m_guid.m_guid);
            msg.Put(static_cast<uint8_t>(0));
            msg.Finalize();
            ClientServices::Send2(&msg);
        } else {
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        }
    } else {
        item->UseItem(target, 0);
    }
}

// VERIFY: 4fffb962 manual
// OFFSET: 0x005E8BC0
void CGCharacterInfo::SocketInventoryItem(int32_t slot) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CGItem_C* item = player->m_bag.GetItemPointer(slot);

    if (item && !(item->m_itemFlags & 0x1) && item->GetNumSockets()) {
        CGItemSocketInfo::SetItem(item->m_obj->m_guid);
    }
}

// VERIFY: e40f790d manual
// OFFSET: 0x005E8C30
void CGCharacterInfo::PickupBag(uint32_t slot) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !CGGameUI::m_hasControl || (slot - 67 <= 6 && !CGBankInfo::m_banker)) {
        return;
    }

    WOWGUID cursorItem;
    WOWGUID cursorContainer;
    uint32_t cursorSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);

    if (cursorItem) {
        CGGameUI::ClearCursor(1, 1);
    }

    WOWGUID bag = static_cast<int32_t>(slot) < player->m_bag.m_numSlots ? player->m_bag.m_slots[slot] : WOWGUID();

    if (!bag) {
        CGCharacterInfo::PickupInventoryItem(slot);
        return;
    }

    CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(bag, TYPE_CONTAINER, __FILE__, __LINE__));

    if (container && !(container->m_itemFlags & 0x1)) {
        CGGameUI::SetCursorItem(bag, player->m_obj->m_guid, slot, 1, 0, 1);
        CGGameUI::LockItem(bag);
    }
}

// VERIFY: 5e4c7a87 manual
// OFFSET: 0x005E9140
void CGCharacterInfo::RequestInspectHonorData() {
    if (CGCharacterInfo::m_hasHonorData || CGCharacterInfo::m_honorRequestPending || !CGCharacterInfo::m_inspectTarget) {
        return;
    }

    CDataStore honor;
    honor.Put(static_cast<uint32_t>(MSG_INSPECT_HONOR_STATS));
    honor.Put(CGCharacterInfo::m_inspectTarget.m_guid);
    honor.Finalize();
    ClientServices::Send2(&honor);
    CDataStore arena;
    arena.Put(static_cast<uint32_t>(MSG_INSPECT_ARENA_TEAMS));
    arena.Put(CGCharacterInfo::m_inspectTarget.m_guid);
    arena.Finalize();
    ClientServices::Send2(&arena);
    CGCharacterInfo::m_honorRequestPending = 1;
}

// VERIFY: 957733e3 manual
// OFFSET: 0x005E9250
bool CGCharacterInfo::CanEquipItemInSlot(const ItemStats_C* stats, uint32_t slot) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    if (stats->m_requiredLevel > 1 && static_cast<int32_t>(stats->m_requiredLevel) > player->m_unit->m_unitFieldLevel) {
        return false;
    }

    uint32_t proficiency = CGPlayer_C::GetProficiency(static_cast<uint8_t>(stats->m_class));

    if (proficiency && !((1 << stats->m_subClass) & proficiency)) {
        return false;
    }

    if (slot == 16 && (stats->m_inventoryType == 22 || stats->m_inventoryType == 13) && !CGPlayer_C::CanDualWield()) {
        return false;
    }

    int32_t allowed = 1;

    if (!((stats->m_allowableRace - 1) & stats->m_allowableRace) && !((stats->m_allowableClass - 1) & stats->m_allowableClass)) {
        for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
            ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

            if (race && !(race->m_flags & 0x1) && ((1 << (race->m_id - 1)) & stats->m_allowableRace) && player->m_unit->m_unitFieldBytes0[0] != race->m_id) {
                return false;
            }
        }

        for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
            ChrClassesRec* chrClass = g_chrClassesDB.GetRecordByIndex(i);

            if (chrClass && ((1 << (chrClass->m_id - 1)) & stats->m_allowableClass) && player->m_unit->m_unitFieldBytes0[1] != chrClass->m_id) {
                return false;
            }
        }

        return !stats->m_requiredReputationFaction || GetRepListRepValue(stats->m_requiredReputationFaction) >= CGReputationInfo::m_reactionThresholds[stats->m_requiredReputationRank];
    }

    int32_t allRaces = 1;
    int32_t allClasses = 1;

    for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
        ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

        if (race && !(race->m_flags & 0x1) && !((1 << (race->m_id - 1)) & stats->m_allowableRace)) {
            allRaces = 0;
            break;
        }
    }

    for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
        ChrClassesRec* chrClass = g_chrClassesDB.GetRecordByIndex(i);

        if (chrClass && !((1 << (chrClass->m_id - 1)) & stats->m_allowableClass)) {
            allClasses = 0;
            break;
        }
    }

    if (!allRaces) {
        allowed = 0;

        for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
            ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

            if (race && !(race->m_flags & 0x1) && ((1 << (race->m_id - 1)) & stats->m_allowableRace) && player->m_unit->m_unitFieldBytes0[0] == race->m_id) {
                allowed = 1;
            }
        }

        if (!allowed) {
            return false;
        }
    }

    if (!allClasses) {
        allowed = 0;

        for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
            ChrClassesRec* chrClass = g_chrClassesDB.GetRecordByIndex(i);

            if (chrClass && ((1 << (chrClass->m_id - 1)) & stats->m_allowableClass) && player->m_unit->m_unitFieldBytes0[1] == chrClass->m_id) {
                allowed = 1;
            }
        }
    }

    if (!allowed) {
        return false;
    }

    return !stats->m_requiredReputationFaction || GetRepListRepValue(stats->m_requiredReputationFaction) >= CGReputationInfo::m_reactionThresholds[stats->m_requiredReputationRank];
}

// VERIFY: a84d2b4c manual
// OFFSET: 0x005E9570
bool CGCharacterInfo::CanEquipItem(CGItem_C* item, uint32_t slot) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (stats) {
        return CGCharacterInfo::CanEquipItemInSlot(stats, slot);
    }

    return false;
}

// VERIFY: a708b631 manual
// OFFSET: 0x005E8D30
int32_t CGCharacterInfo::PutItemInBackpack(uint32_t slot) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !CGGameUI::m_hasControl) {
        return 0;
    }

    if (slot - 67 <= 6 && !CGBankInfo::m_banker) {
        return 0;
    }

    WOWGUID cursorItem;
    WOWGUID cursorContainer;
    uint32_t cursorSlot;
    CGGameUI::GetCursorItem(cursorItem, cursorContainer, cursorSlot);
    int32_t virtualId;
    int32_t virtualSlot;
    CGGameUI::GetCursorVirtualItem(virtualId, virtualSlot);
    WOWGUID bag;

    if (slot == 0xFF) {
        bag = player->m_obj->m_guid;
    } else {
        if (slot >= static_cast<uint32_t>(player->m_bag.m_numSlots)) {
            CGCharacterInfo::PickupInventoryItem(slot);
            return 0;
        }

        bag = player->m_bag.m_slots[slot];
    }

    if (!bag) {
        CGCharacterInfo::PickupInventoryItem(slot);
        return 0;
    }

    CGItem_C* bagItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(bag, TYPE_ITEM, __FILE__, __LINE__));

    if ((bagItem && (bagItem->m_itemFlags & 1)) || cursorItem == bag) {
        CGGameUI::ClearCursor(1, 1);
        return 1;
    }

    if (!cursorItem && !virtualId) {
        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    if (cursorContainer == bag && (cursorContainer != player->m_obj->m_guid || cursorSlot - 23 <= 15)) {
        CGGameUI::ClearCursor(1, 1);
        return 1;
    }

    if (!cursorItem) {
        if (CGGameUI::m_cursorItemType == UICURSOR_MERCHANT && CGMerchantInfo::m_merchant) {
            BuyItemInBag(CGMerchantInfo::m_merchant, CGMerchantInfo::GetItem(virtualSlot), 1, bag);
        }

        CGGameUI::ClearCursor(1, 1);
        return 0;
    }

    if (slot != 0xFF) {
        CGItem_C* cursorObject;

        if ((cursorContainer == player->m_obj->m_guid && cursorSlot - 19 <= 3) || ((cursorObject = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cursorItem, TYPE_ITEM, __FILE__, __LINE__))) && (cursorObject->m_obj->m_type & TYPE_CONTAINER))) {
            CGGameUI::LockItem(bag);
            player->SwapItems(cursorItem, cursorContainer, cursorSlot, player->m_obj->m_guid, slot, 0);
            return 1;
        }
    }

    if (CGGameUI::m_stackSplit) {
        player->SplitItem(cursorItem, cursorContainer, cursorSlot, bag, 0xFF, CGGameUI::m_stackSplit);
    } else {
        player->AutoStoreItemInBag(cursorItem, cursorContainer, cursorSlot, bag, 0);
    }

    CGGameUI::ClearCursor(0, 1);
    return 1;
}

// VERIFY: 19ee11f0 manual
// OFFSET: 0x005E8FE0
int32_t CGCharacterInfo::CalculateInventoryAlertStatus(CGPlayer_C* player, uint32_t slot) {
    if (s_inventoryAlertSlots[slot] == -1) {
        if (player->m_obj->m_guid != ClntObjMgrGetActivePlayer()) {
            return 0;
        }

        uint32_t ammo = player->m_player->m_playerAmmoId;

        if (!ammo) {
            return 0;
        }

        return static_cast<uint32_t>(player->m_bag.GetItemTypeCount(ammo, 0)) <= 20;
    }

    CGItem_C* item = player->m_bag.GetItemPointer(s_inventoryAlertSlots[slot]);

    if (!item) {
        return 0;
    }

    if (item->m_item->m_itemFieldFlags & 0x10) {
        return 2;
    }

    if ((item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) || !item->m_item->m_itemFieldMaxdurability) {
        return 0;
    }

    if (!(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldDurability) {
        if (item->GetDurability() > 5) {
            return 0;
        }

        CGTutorial::TriggerTutorial(35);
        return 1;
    }

    CGTutorial::TriggerTutorial(36);
    return 2;
}

// VERIFY: d40125b4 auto
// OFFSET: 0x005E7690
void CGCharacterInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(MSG_INSPECT_HONOR_STATS);
    ClientServices::ClearMessageHandler(MSG_INSPECT_ARENA_TEAMS);
}

// VERIFY: fbee3a82 auto
// OFFSET: 0x005E7760
void GuildNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        Script_SendUnitSignal(guid, SCRIPT_EVENT_PLAYER_GUILD_UPDATE);
    }
}

// VERIFY: 610b1ef3 manual
// OFFSET: 0x005EAF70
void CGCharacterInfo::LeaveWorld() {
    WOWGUID guid = ClntObjMgrGetActivePlayer();

    if (guid) {
        ClntObjMgrUnsetObjMirrorHandler(guid, ID_PLAYER, 0xDA0, PlayerCharacterPointsUpdateHandler, nullptr);
        ClntObjMgrUnsetObjMirrorHandler(guid, ID_PLAYER, 0x1068, PlayerEquipmentChangedHandler, nullptr);
    }

    CGCharacterInfo::m_invAlertStatus.Clear();
    WOWGUID none = { 0 };

    if (CGCharacterInfo::m_inspectTarget) {
        CGCharacterInfo::m_inspectTarget = WOWGUID();
        CGCharacterInfo::m_hasHonorData = 0;
        CGCharacterInfo::m_honorRequestPending = 0;

        for (uint32_t i = 0; i < 3; i++) {
            CGCharacterInfo::m_inspectArenaTeams[i].m_teamId = 0;
        }

        CGTalentInfo_Inspect::SetInspectTarget(none);
    }
}
