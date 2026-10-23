#include "StableInfo.h"
#include <cstring>
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"

// STATIC: 0x00C0F480
StablePetInfo CGStableInfo::m_pets[6];

// STATIC: 0x00C0F6C0
WOWGUID CGStableInfo::m_stableMaster;

// STATIC: 0x00C0F6C8
uint32_t CGStableInfo::m_numPets;

// STATIC: 0x00C0F6CC
int32_t CGStableInfo::m_numSlots;

// STATIC: 0x00C0F6D0
int32_t CGStableInfo::m_selectedPet;

// VERIFY: ac9cd9fe manual
// OFFSET: 0x005A11A0
int32_t Packet_MSG_LIST_STABLED_PETS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID master;
    msg->Get(master.m_guid);
    WOWGUID npc = master;

    if (CGStableInfo::m_stableMaster != npc) {
        CGGameUI::SetInteractTarget(npc);
        CGStableInfo::m_stableMaster = npc;
    }

    CGStableInfo::m_selectedPet = 0;
    uint8_t numPets;
    uint8_t numSlots;
    msg->Get(numPets);
    msg->Get(numSlots);
    CGStableInfo::m_numPets = numPets;
    CGStableInfo::m_numSlots = numSlots;
    memset(&CGStableInfo::m_pets[0], 0, sizeof(CGStableInfo::m_pets[0]));

    for (uint32_t i = 0; i < CGStableInfo::m_numPets; i++) {
        StablePetInfo& pet = CGStableInfo::m_pets[i + 1];
        msg->Get(pet.m_petNumber);
        msg->Get(reinterpret_cast<uint32_t&>(pet.m_entry));
        msg->Get(pet.m_level);
        msg->GetString(pet.m_name, sizeof(pet.m_name));
        msg->Get(pet.m_state);

        if (!(pet.m_state & 0x2)) {
            CGStableInfo::m_pets[0].m_entry = pet.m_entry;
            CGStableInfo::m_pets[0].m_petNumber = pet.m_petNumber;
            CGStableInfo::m_pets[0].m_level = pet.m_level;
            CGStableInfo::m_pets[0].m_state = pet.m_state;
            SStrCopy(CGStableInfo::m_pets[0].m_name, pet.m_name, sizeof(CGStableInfo::m_pets[0].m_name));
            CGStableInfo::m_numPets--;
            i--;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PET_STABLE_SHOW, nullptr);
    return 1;
}

// VERIFY: 03f68cbf manual
// OFFSET: 0x005A17F0
int32_t Packet_SMSG_STABLE_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t result;
    msg->Get(result);

    switch (result) {
        case 1:
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            return 1;

        case 10:
            CGStableInfo::m_numSlots++;

        case 8:
        case 9: {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (CGStableInfo::m_stableMaster || (player && player->HasAuraEffect(SPELL_AURA_OPEN_STABLE))) {
                CDataStore list;
                list.Put(static_cast<uint32_t>(MSG_LIST_STABLED_PETS));
                list.Put(CGStableInfo::m_stableMaster.m_guid);
                list.Finalize();
                ClientServices::Send2(&list);
            }

            return 1;
        }

        case 11:
            FrameScript_SignalEvent(SCRIPT_EVENT_PET_STABLE_UPDATE, nullptr);
            return 1;

        case 12:
            Spell_C_PetTameFailure(12);
            return 1;
    }

    return 1;
}

// VERIFY: da4c52c5 auto
// OFFSET: 0x005A1ED0
void CGStableInfo::InitializeGame() {
    ClientServices::SetMessageHandler(MSG_LIST_STABLED_PETS, Packet_MSG_LIST_STABLED_PETS, nullptr);
    ClientServices::SetMessageHandler(SMSG_STABLE_RESULT, Packet_SMSG_STABLE_RESULT, nullptr);
}

// VERIFY: 83895cc1 auto
// OFFSET: 0x005A0F10
void CGStableInfo::CloseStables(bool force) {
    if (!CGStableInfo::m_stableMaster && !force) {
        return;
    }

    CGGameUI::ClearInteractTarget(CGStableInfo::m_stableMaster, 0);
    CGStableInfo::m_stableMaster = WOWGUID();
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_STABLE_CLOSED, nullptr);
}

// VERIFY: b8be7ac2 manual
// OFFSET: 0x0050F500
StablePetInfo* CGStableInfo::GetPet(uint32_t index) {
    if (index >= CGStableInfo::m_numPets) {
        return nullptr;
    }

    return &CGStableInfo::m_pets[index + 1];
}

// VERIFY: c546b46a auto
// OFFSET: 0x005A0E00
void CGStableInfo::SelectPet(uint32_t index) {
    if (index == static_cast<uint32_t>(-1)) {
        CGStableInfo::m_selectedPet = -1;
    } else if (index >= CGStableInfo::m_numPets) {
        CGStableInfo::m_selectedPet = 0;
    } else {
        CGStableInfo::m_selectedPet = CGStableInfo::m_pets[index + 1].m_petNumber;
    }
}

// VERIFY: 809d7448 auto
// OFFSET: 0x005A0E40
int32_t CGStableInfo::GetSelectedPetIndex() {
    if (CGStableInfo::m_selectedPet == -1) {
        return -1;
    }

    if (!CGStableInfo::m_selectedPet) {
        return -2;
    }

    for (uint32_t i = 0; i < CGStableInfo::m_numPets; i++) {
        if (CGStableInfo::m_pets[i + 1].m_petNumber == static_cast<uint32_t>(CGStableInfo::m_selectedPet)) {
            return i;
        }
    }

    return -2;
}

// VERIFY: 083d15c1 auto
// OFFSET: 0x005A0EC0
void CGStableInfo::EnterWorld() {
    CGStableInfo::m_stableMaster = WOWGUID{ 0 };
    CGStableInfo::m_numPets = 0;
    CGStableInfo::m_numSlots = 0;
    CGStableInfo::m_selectedPet = 0;
    memset(&CGStableInfo::m_pets[0], 0, sizeof(CGStableInfo::m_pets[0]));
}

// VERIFY: c3ced28b auto
// OFFSET: 0x005A0EF0
void CGStableInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(MSG_LIST_STABLED_PETS);
    ClientServices::ClearMessageHandler(SMSG_STABLE_RESULT);
}

// VERIFY: 37d7284c auto
// OFFSET: 0x005A1160
void CGStableInfo::LeaveWorld() {
    CGStableInfo::CloseStables(false);
}
