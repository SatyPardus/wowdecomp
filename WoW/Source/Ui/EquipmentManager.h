#ifndef WOW_SOURCE_UI_EQUIPMENTMANAGER_H
#define WOW_SOURCE_UI_EQUIPMENTMANAGER_H

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

class CDataStore;
struct lua_State;

struct EQUIPMENT_SET {
    /* 0000 */ uint32_t m_handle;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ WOWGUID m_guid;
    /* 0010 */ int32_t m_id;
    /* 0014 */ char m_icon[256];
    /* 0114 */ char m_name[128];
    /* 0194 */ uint32_t unused_0194;
    /* 0198 */ WOWGUID m_items[19];
    /* 0230 */ uint32_t m_isNew;

    EQUIPMENT_SET();
};

struct EQUIPMENT_SET_NODE : public TSLinkedNode<EQUIPMENT_SET_NODE> {
    /* 0008 */ EQUIPMENT_SET* m_set;
};

int32_t Packet_SMSG_EQUIPMENT_SET_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_EQUIPMENT_SET_SAVED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_EQUIPMENT_SET_USE_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

class CGEquipmentManager {
    public:
    static uint32_t* s_equipmentSetHeap;
    static TSList<EQUIPMENT_SET_NODE> s_equipmentSets;
    static void EnterWorld();
    static uint8_t m_lastCanUseEquipmentSetsValue;
    static uint8_t m_loaded;
    static uint8_t m_swapPending;
    static int32_t m_ignoredSlots[19];
    static int32_t m_pendingSetID;
    static int32_t CanUseEquipmentSets(bool a1);
    static const EQUIPMENT_SET* GetSetByID(int32_t id);
    static int32_t SetContainsLockedItems(const EQUIPMENT_SET* set);
    static int32_t SetUsesLockedSlots(const EQUIPMENT_SET* set);
    static void InitializeGame();
    static void ShutdownGame();
    static void EquipSet(int32_t id);
    static void GetFullIconPath(int32_t id, char* buffer, uint32_t size);
    static int32_t GetSetList(char* buffer, uint32_t size, const WOWGUID* item);
    static void ClearIgnoredSlots();
    static bool UseEquipmentSet(const EQUIPMENT_SET* set);
    static void SaveSet(EQUIPMENT_SET* set, const char* name, const char* icon);
    static bool DeleteEquipmentSet(EQUIPMENT_SET* set);
    static bool HasSet(const char* name);
    static int32_t GetNextFreeSetID();
    static EQUIPMENT_SET* GetSetByIndex(int32_t index);
    static EQUIPMENT_SET* GetSetByName(const char* name);
    static bool ModifyEquipmentSet(EQUIPMENT_SET* set, const char* name);
    static bool CanCreateSet();
    static void CreateEquipmentSet(const char* name, const char* icon);
    static void SaveSetByName(const char* name, const char* icon);
};

void EquipmentManagerRegisterScriptFunctions();
void EquipmentManagerUnregisterScriptFunctions();

#endif
