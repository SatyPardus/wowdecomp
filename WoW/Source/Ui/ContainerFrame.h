#ifndef WOW_SOURCE_UI_CONTAINERFRAME_H
#define WOW_SOURCE_UI_CONTAINERFRAME_H

#include <cstdint>
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Source/Object/WGUID.h"

struct lua_State;

class CGBag_C;

class CGContainerInfo {
    public:
    static WOWGUID m_containers[TOTAL_BAG_SLOTS];
    static void UpdateContainers();
    static void OpenContainer(WOWGUID guid);
    static void EnterWorld();
    static void LeaveWorld();
    static void UpdateCooldowns();
    static void UpdateItem(WOWGUID guid);
    static void UpdateContents(WOWGUID guid);
    static bool GetLuaBagAndSlot(lua_State* L, CGBag_C** bag, int32_t* slot, int32_t* bank);
};

int32_t Script_PickupContainerItem(lua_State* L);
void ContainerRegisterScriptFunctions();
void ContainerUnregisterScriptFunctions();

int32_t InvUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
int32_t UpdateInvContents(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);
int32_t UpdateContainerContents(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param);

WOWGUID GetBagAtIndex(uint32_t index);

#endif
