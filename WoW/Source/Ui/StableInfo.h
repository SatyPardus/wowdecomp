#ifndef WOW_SOURCE_UI_STABLEINFO_H
#define WOW_SOURCE_UI_STABLEINFO_H

#include <cstdint>
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/WGUID.h"

struct lua_State;

struct StablePetInfo {
    /* 0000 */ uint32_t m_petNumber;
    /* 0004 */ uint32_t m_level;
    /* 0008 */ int32_t m_entry;
    /* 000C */ char m_name[80];
    /* 005C */ uint8_t m_state;
    /* 005D */ uint8_t unused_005D[3];
};

class CGStableInfo {
    public:
    static void ShutdownGame();
    static StablePetInfo m_pets[6];
    static WOWGUID m_stableMaster;
    static uint32_t m_numPets;
    static int32_t m_numSlots;
    static int32_t m_selectedPet;

    static void CloseStables(bool force);
    static void EnterWorld();
    static void LeaveWorld();
    static void InitializeGame();
    static StablePetInfo* GetPet(uint32_t index);
    static void SelectPet(uint32_t index);
    static int32_t GetSelectedPetIndex();
};

void StableInfoRegisterScriptFunctions();
void StableInfoUnregisterScriptFunctions();

class CDataStore;

int32_t Packet_MSG_LIST_STABLED_PETS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_STABLE_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

#endif
