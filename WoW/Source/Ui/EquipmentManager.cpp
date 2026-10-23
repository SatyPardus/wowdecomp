#include "EquipmentManager.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include <new>
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include <cstring>
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/DB/DBClient/DBClient.h"

// STATIC: 0x00C1E95C
uint32_t* CGEquipmentManager::s_equipmentSetHeap;

// STATIC: 0x00C1E954
uint8_t CGEquipmentManager::m_lastCanUseEquipmentSetsValue;

// STATIC: 0x00C1E955
uint8_t CGEquipmentManager::m_loaded;

// STATIC: 0x00C1E956
uint8_t CGEquipmentManager::m_swapPending;

// STATIC: 0x00C1E958
int32_t CGEquipmentManager::m_pendingSetID;

// STATIC: 0x00C1E908
int32_t CGEquipmentManager::m_ignoredSlots[19];

// STATIC: 0x00ACFC00
TSList<EQUIPMENT_SET_NODE> CGEquipmentManager::s_equipmentSets;

// VERIFY: 36e16d6a auto
// OFFSET: 0x005AD690
void CGEquipmentManager::EnterWorld() {
    CGEquipmentManager::m_swapPending = 0;
    CGEquipmentManager::m_lastCanUseEquipmentSetsValue = 0;
    CGEquipmentManager::m_pendingSetID = 0;
}

// VERIFY: 6b01312b auto
// OFFSET: 0x005AD6B0
int32_t CGEquipmentManager::CanUseEquipmentSets(bool a1) {
    uint8_t canUse = 0;

    if (!CGEquipmentManager::m_swapPending) {
        CVar* var = CVar::Lookup("equipmentManager");

        if (var && var->m_intValue) {
            canUse = 1;
        }
    }

    if (CGEquipmentManager::m_lastCanUseEquipmentSetsValue != canUse) {
        CGEquipmentManager::m_lastCanUseEquipmentSetsValue = canUse;

        if (a1) {
            CGActionBar::UpdateUsable();
        }
    }

    return canUse;
}

// VERIFY: 0a86e270 manual
// OFFSET: 0x005AD9B0
static EQUIPMENT_SET* AllocSet(uint32_t* heap) {
    uint32_t handle;
    void* object = nullptr;

    if (!ObjectAlloc(*heap, &handle, &object, false)) {
        return nullptr;
    }

    EQUIPMENT_SET* set = object ? new (object) EQUIPMENT_SET() : nullptr;
    set->m_handle = handle;
    return set;
}

// VERIFY: 53f31e14 manual
// OFFSET: 0x005AF7E0
void CGEquipmentManager::CreateEquipmentSet(const char* name, const char* icon) {
    int32_t count = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        count++;
    }

    if (count >= 10 || CGEquipmentManager::HasSet(name)) {
        return;
    }

    int32_t id = CGEquipmentManager::GetNextFreeSetID();

    if (id < 0) {
        return;
    }

    EQUIPMENT_SET* set = AllocSet(CGEquipmentManager::s_equipmentSetHeap);
    set->m_id = id;
    EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.NewNode(2, 0, 8);
    node->m_set = set;
    CGEquipmentManager::SaveSet(set, name, icon);
    set->m_isNew = 1;
}

// VERIFY: beee0c68 auto
// OFFSET: 0x005AF8E0
void CGEquipmentManager::SaveSetByName(const char* name, const char* icon) {
    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(name);

    if (set) {
        CGEquipmentManager::SaveSet(set, name, icon);
    }
}

// VERIFY: ca2761b6 manual
// OFFSET: 0x005AFBB0
void CGEquipmentManager::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_LOAD_EQUIPMENT_SET, Packet_SMSG_EQUIPMENT_SET_LIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_EQUIPMENT_SET_ID, Packet_SMSG_EQUIPMENT_SET_SAVED, nullptr);
    ClientServices::SetMessageHandler(SMSG_USE_EQUIPMENT_SET_RESULT, Packet_SMSG_EQUIPMENT_SET_USE_RESULT, nullptr);

    if (!CGEquipmentManager::s_equipmentSetHeap) {
        CGEquipmentManager::s_equipmentSetHeap = new (SMemAlloc(sizeof(uint32_t), __FILE__, __LINE__, 0)) uint32_t(ObjectAllocAddHeap(sizeof(EQUIPMENT_SET), 128, "EQUIPMENTSET", true));
    }
}

// VERIFY: 88b53d9b manual
// OFFSET: 0x005AE260
bool CGEquipmentManager::DeleteEquipmentSet(EQUIPMENT_SET* set) {
    CGActionBar::RemoveEquipmentSet(set->m_id);
    WOWGUID guid = set->m_guid;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (node->m_set == set) {
            CGEquipmentManager::s_equipmentSets.DeleteNode(node);
            break;
        }
    }

    ObjectFree(*CGEquipmentManager::s_equipmentSetHeap, set->m_handle);
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_EQUIPMENT_SET_DELETE));
    SmartGUID setGuid;
    setGuid.m_guid = guid.m_guid;
    msg << setGuid;
    msg.Finalize();
    ClientServices::Send2(&msg);
    FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SETS_CHANGED, nullptr);
    return true;
}

// VERIFY: ecc66233 manual
// OFFSET: 0x005AE380
int32_t CGEquipmentManager::GetSetList(char* buffer, uint32_t size, const WOWGUID* item) {
    uint32_t remaining = size - 1;
    char* out = buffer;
    const char* separator = "";
    const char* delimiter = FrameScript_GetText("PLAYER_LIST_DELIMITER", -1, GENDER_NOT_APPLICABLE);

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        EQUIPMENT_SET* set = node->m_set;

        for (uint32_t i = 0; i < sizeof(set->m_items) / sizeof(set->m_items[0]); i++) {
            if (set->m_items[i] != *item) {
                continue;
            }

            size_t nameLength = SStrLen(set->m_name);
            size_t separatorLength = SStrLen(separator);

            if (separatorLength + nameLength > remaining) {
                break;
            }

            memcpy(out, separator, separatorLength);
            out += separatorLength;
            memcpy(out, set->m_name, nameLength);
            out += nameLength;
            remaining -= nameLength + separatorLength;
            separator = delimiter;
        }
    }

    *out = '\0';
    return buffer != out;
}

// VERIFY: 0f4dee2b manual
// OFFSET: 0x005AE490
bool CGEquipmentManager::HasSet(const char* name) {
    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (!SStrCmp(node->m_set->m_name, name, STORM_MAX_STR)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 2ad55b3a manual
// OFFSET: 0x005AE530
int32_t CGEquipmentManager::GetNextFreeSetID() {
    int32_t id = 0;
    EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head();

    while (node) {
        if (node->m_set->m_id == id) {
            if (++id >= 10) {
                return -1;
            }

            node = CGEquipmentManager::s_equipmentSets.Head();
        } else {
            node = CGEquipmentManager::s_equipmentSets.Next(node);
        }
    }

    return id;
}

// VERIFY: 4e7bfb93 manual
// OFFSET: 0x005AE580
EQUIPMENT_SET* CGEquipmentManager::GetSetByIndex(int32_t index) {
    int32_t i = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (i == index) {
            return node->m_set;
        }

        i++;
    }

    return nullptr;
}

// VERIFY: b88be2b9 manual
// OFFSET: 0x005AE5C0
const EQUIPMENT_SET* CGEquipmentManager::GetSetByID(int32_t id) {
    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (node->m_set->m_id == id) {
            return node->m_set;
        }
    }

    return nullptr;
}

// INLINED
EQUIPMENT_SET::EQUIPMENT_SET() {
    this->m_guid = WOWGUID{ 0 };
    memset(this->m_items, 0, sizeof(this->m_items));
    this->m_isNew = 0;
}

// VERIFY: 10e1d2c8 manual
// OFFSET: 0x005AE600
EQUIPMENT_SET* CGEquipmentManager::GetSetByName(const char* name) {
    if (!name) {
        return nullptr;
    }

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (!SStrCmp(node->m_set->m_name, name, STORM_MAX_STR)) {
            return node->m_set;
        }
    }

    return nullptr;
}

// VERIFY: b9b5fb9b manual
// OFFSET: 0x005AF400
bool CGEquipmentManager::ModifyEquipmentSet(EQUIPMENT_SET* set, const char* name) {
    if (!set || CGEquipmentManager::HasSet(name)) {
        return false;
    }

    SStrCopy(set->m_name, name, STORM_MAX_STR);
    CGEquipmentManager::SaveSet(set, name, set->m_icon);
    FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SETS_CHANGED, nullptr);
    return true;
}

// VERIFY: 8259e939 manual
// OFFSET: 0x005AF460
bool CGEquipmentManager::CanCreateSet() {
    int32_t count = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        count++;
    }

    return count < 10;
}

// VERIFY: 816e8bf2 manual
// OFFSET: 0x005AF490
int32_t Packet_SMSG_EQUIPMENT_SET_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count = 0;
    msg->Get(count);

    uint32_t existing = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); !CGEquipmentManager::s_equipmentSets.IsEnd(node); node = CGEquipmentManager::s_equipmentSets.RawNext(node)) {
        existing++;
    }

    if (CGEquipmentManager::m_loaded && count == existing) {
        msg->m_read = msg->m_size;
        return 1;
    }

    if (existing) {
        for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); !CGEquipmentManager::s_equipmentSets.IsEnd(node); node = CGEquipmentManager::s_equipmentSets.RawNext(node)) {
            ObjectFree(*CGEquipmentManager::s_equipmentSetHeap, node->m_set->m_handle);
        }

        CGEquipmentManager::s_equipmentSets.Clear();
    }

    for (int32_t i = 0; i < static_cast<int32_t>(count) && i < 10; i++) {
        EQUIPMENT_SET* set = nullptr;
        uint32_t handle;
        void* object = nullptr;

        if (ObjectAlloc(*CGEquipmentManager::s_equipmentSetHeap, &handle, &object, false)) {
            set = object ? new (object) EQUIPMENT_SET() : nullptr;
            set->m_handle = handle;
        }

        SmartGUID guid;
        *msg >> guid;
        set->m_guid = guid;
        msg->Get(reinterpret_cast<uint32_t&>(set->m_id));
        msg->GetString(set->m_name, sizeof(set->m_name));
        msg->GetString(set->m_icon, sizeof(set->m_icon));

        for (uint32_t slot = 0; slot < 19; slot++) {
            SmartGUID item;
            *msg >> item;
            set->m_items[slot] = item;
        }

        EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.NewNode(2, 0, 0);
        node->m_set = set;
    }

    CGEquipmentManager::m_loaded = 1;
    return 1;
}

// VERIFY: aa7448be auto
// OFFSET: 0x005AD700
void CGEquipmentManager::ClearIgnoredSlots() {
    for (uint32_t i = 0; i < 19; i++) {
        CGEquipmentManager::m_ignoredSlots[i] = 0;
    }
}

// VERIFY: 8428bb36 manual
// OFFSET: 0x005AD940
int32_t CGEquipmentManager::SetContainsLockedItems(const EQUIPMENT_SET* set) {
    if (!set) {
        return 0;
    }

    for (uint32_t i = 0; i <= 18; i++) {
        if (set->m_items[i].m_guid > 1) {
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(set->m_items[i], TYPE_ITEM, __FILE__, __LINE__));

            if (item && (item->m_itemFlags & 0x1) && !CGMerchantInfo::IsBuybackItem(set->m_items[i])) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 82f5bbb0 manual
// OFFSET: 0x005AE040
int32_t CGEquipmentManager::SetUsesLockedSlots(const EQUIPMENT_SET* set) {
    if (!set) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    for (int32_t slot = 0; slot <= 18; slot++) {
        if (set->m_items[slot].m_guid == 1) {
            continue;
        }

        CGItem_C* item = player->m_bag.GetItemPointer(slot);

        if (item && (item->m_itemFlags & 0x1)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: c926fac3 manual
// OFFSET: 0x005AE660
void CGEquipmentManager::EquipSet(int32_t id) {
    const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(id);

    if (set) {
        FrameScript_SignalEvent(SCRIPT_EVENT_WEAR_EQUIPMENT_SET, "%s", set->m_name);
    }
}

// VERIFY: 5bdd1316 manual
// OFFSET: 0x005AE6B0
void CGEquipmentManager::GetFullIconPath(int32_t id, char* buffer, uint32_t size) {
    const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(id);

    if (!set) {
        SStrPrintf(buffer, size, "");
        return;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char* icon = set->m_icon;

    if (!icon || !*icon) {
        SStrPrintf(buffer, size, "");
        return;
    }

    SStrPrintf(buffer, size, "%s%s%s", path, *path ? "\\" : "", icon);
}

// VERIFY: 7e1e9f40 manual
// OFFSET: 0x005AE760
static void OnEquipmentSetSaved(int32_t id, WOWGUID guid) {
    EQUIPMENT_SET* set = const_cast<EQUIPMENT_SET*>(CGEquipmentManager::GetSetByID(id));

    if (!set) {
        return;
    }

    if (guid.m_guid) {
        set->m_guid = guid;
        set->m_isNew = 0;
        return;
    }

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        if (node->m_set->m_id == id) {
            CGEquipmentManager::s_equipmentSets.DeleteNode(node);
            break;
        }
    }

    ObjectFree(*CGEquipmentManager::s_equipmentSetHeap, set->m_handle);
}

// VERIFY: 591dc89b auto
// OFFSET: 0x005AF6C0
int32_t Packet_SMSG_EQUIPMENT_SET_SAVED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t id = 0;
    msg->Get(reinterpret_cast<uint32_t&>(id));
    SmartGUID guid;
    *msg >> guid;
    OnEquipmentSetSaved(id, guid);
    return 1;
}

// VERIFY: f8a867f0 manual
// OFFSET: 0x005AF710
int32_t Packet_SMSG_EQUIPMENT_SET_USE_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGEquipmentManager::m_swapPending = 0;
    uint8_t result = 0;
    msg->Get(result);

    if (result) {
        const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGEquipmentManager::m_pendingSetID);
        FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SWAP_FINISHED, "%b%s", 0, set ? set->m_name : nullptr);

        if (result == 4) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UI_ERROR_MESSAGE, "%s", FrameScript_GetText("EQUIPMENT_MANAGER_BAGS_FULL", -1, GENDER_NOT_APPLICABLE));
        }
    } else {
        const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGEquipmentManager::m_pendingSetID);
        FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SWAP_FINISHED, "%b%s", 1, set ? set->m_name : nullptr);
    }

    CGActionBar::UpdateUsable();
    CGEquipmentManager::m_pendingSetID = 0;
    return 1;
}

// VERIFY: 12a136d2 manual
// OFFSET: 0x005ADA20
bool CGEquipmentManager::UseEquipmentSet(const EQUIPMENT_SET* set) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    if (!CGEquipmentManager::CanUseEquipmentSets(true)) {
        CVar* var = CVar::Lookup("equipmentManager");

        if (var && var->m_intValue) {
            return false;
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_UI_ERROR_MESSAGE, "%s", FrameScript_GetText("EQUIPMENT_MANAGER_IS_DISABLED", -1, GENDER_NOT_APPLICABLE));
        return false;
    }

    if (CGGameUI::GetCursorItem()) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UI_ERROR_MESSAGE, "%s", FrameScript_GetText("SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW", -1, GENDER_NOT_APPLICABLE));
        return false;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_EQUIPMENT_SET_USE));
    bool inCombat = (player->m_unit->m_unitFieldFlags & 0x80000) != 0;
    bool combatWarned = false;
    bool missingWarned = false;
    char buffer[3000];

    for (uint32_t slot = 0; slot <= 18; slot++) {
        WOWGUID guid = set->m_items[slot];
        SmartGUID itemGuid;
        itemGuid.m_guid = 1;
        uint8_t bag = 0;
        uint8_t bagSlot = 0;

        if ((inCombat && slot != 15 && slot != 16 && slot != 17) || guid.m_guid == 1) {
            if (inCombat && !combatWarned && guid.m_guid != 1 && guid.m_guid != player->m_bag.GetItemGuid(slot)) {
                SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("EQUIPMENT_MANAGER_COMBAT_SWAP", -1, GENDER_NOT_APPLICABLE), set->m_name);
                FrameScript_SignalEvent(SCRIPT_EVENT_UI_ERROR_MESSAGE, "%s", buffer);
                combatWarned = true;
            }
        } else if (!guid) {
            itemGuid.m_guid = 0;
        } else {
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

            if (item) {
                bagSlot = CGPlayer_C::FindItemSlot(ClntObjMgrGetActivePlayer(), item);

                if (static_cast<uint8_t>(bagSlot - 74) <= 11) {
                    bagSlot = 0;
                } else if (static_cast<uint8_t>(bagSlot - 39) <= 27 && !CGBankInfo::m_banker) {
                    bagSlot = 0;
                } else if (bagSlot != 0xFF) {
                    bag = 0xFF;
                    itemGuid.m_guid = guid.m_guid;
                } else {
                    for (uint32_t i = 19; i <= 22; i++) {
                        WOWGUID container = GetBagAtIndex(i - 19);

                        if (container) {
                            bagSlot = CGPlayer_C::FindItemSlot(container, item);

                            if (bagSlot != 0xFF) {
                                bag = static_cast<uint8_t>(i);
                                itemGuid.m_guid = guid.m_guid;
                                break;
                            }
                        }
                    }

                    if (itemGuid.m_guid == 1 && CGBankInfo::m_banker) {
                        for (uint32_t i = 67; i <= 73; i++) {
                            WOWGUID container = GetBagAtIndex(i - 63);

                            if (container) {
                                bagSlot = CGPlayer_C::FindItemSlot(container, item);

                                if (bagSlot != 0xFF) {
                                    bag = static_cast<uint8_t>(i);
                                    itemGuid.m_guid = guid.m_guid;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (itemGuid.m_guid == 1 && !missingWarned) {
                SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("EQUIPMENT_MANAGER_MISSING_ITEM", -1, GENDER_NOT_APPLICABLE), set->m_name);
                FrameScript_SignalEvent(SCRIPT_EVENT_UI_ERROR_MESSAGE, "%s", buffer);
                missingWarned = true;
            }
        }

        msg << itemGuid;
        msg.Put(bag);
        msg.Put(bagSlot);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    CGEquipmentManager::m_swapPending = 1;
    CGEquipmentManager::m_pendingSetID = set->m_id;
    FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SWAP_PENDING, nullptr);
    CGActionBar::UpdateUsable();
    return true;
}

// VERIFY: 2e59ab5d manual
// OFFSET: 0x005ADE50
void CGEquipmentManager::SaveSet(EQUIPMENT_SET* set, const char* name, const char* icon) {
    if (set->m_isNew) {
        return;
    }

    SStrCopy(set->m_name, name, STORM_MAX_STR);
    SStrCopy(set->m_icon, icon, STORM_MAX_STR);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    for (uint32_t slot = 0; slot <= 18; slot++) {
        if (CGEquipmentManager::m_ignoredSlots[slot]) {
            set->m_items[slot].m_guid = 1;
        } else {
            CGItem_C* item = player->m_bag.GetItemPointer(slot);
            set->m_items[slot] = item ? item->m_obj->m_guid : WOWGUID();
        }
    }

    if (set->m_guid) {
        CGActionBar::UpdateEquipmentSet(set->m_id);
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SAVE_EQUIPMENT_SET));
    SmartGUID setGuid;
    setGuid.m_guid = set->m_guid.m_guid;
    msg << setGuid;
    msg.Put(static_cast<uint32_t>(set->m_id));
    msg.PutString(set->m_name);
    msg.PutString(set->m_icon);

    for (uint32_t slot = 0; slot < 19; slot++) {
        SmartGUID itemGuid;
        itemGuid.m_guid = set->m_items[slot].m_guid;
        msg << itemGuid;
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    FrameScript_SignalEvent(SCRIPT_EVENT_EQUIPMENT_SETS_CHANGED, nullptr);
}

// VERIFY: 04c7f51f manual
// OFFSET: 0x005AE1B0
void CGEquipmentManager::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_LOAD_EQUIPMENT_SET);
    ClientServices::ClearMessageHandler(SMSG_EQUIPMENT_SET_ID);
    ClientServices::ClearMessageHandler(SMSG_USE_EQUIPMENT_SET_RESULT);
    CGEquipmentManager::m_swapPending = 0;
    CGEquipmentManager::m_pendingSetID = 0;
    CGEquipmentManager::m_loaded = 0;

    for (EQUIPMENT_SET_NODE* node = CGEquipmentManager::s_equipmentSets.Head(); node; node = CGEquipmentManager::s_equipmentSets.Next(node)) {
        ObjectFree(*CGEquipmentManager::s_equipmentSetHeap, node->m_set->m_handle);
    }

    CGEquipmentManager::s_equipmentSets.Clear();
    delete CGEquipmentManager::s_equipmentSetHeap;
    CGEquipmentManager::s_equipmentSetHeap = nullptr;
}
