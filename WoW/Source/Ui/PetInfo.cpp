#include "PetInfo.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/ClassTrainerFrame.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Ui/StableInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"

// STATIC: 0x00C234DC
int32_t CGPetInfo::m_attackingTarget;

// STATIC: 0x00C234E0
int32_t CGPetInfo::m_possessSpell;

// STATIC: 0x00C234E4
uint8_t CGPetInfo::m_comboPoints;

// STATIC: 0x00C234E8
WOWGUID CGPetInfo::m_comboTarget;

// STATIC: 0x00C234F8
TSGrowableArray<WOWGUID> s_pets;

// STATIC: 0x00C23508
PetAction CGPetInfo::m_actions[10];

// STATIC: 0x00C234C8
uint32_t CGPetInfo::m_petMode;

// STATIC: 0x00A1A860
static const char s_petModeTokens[3][32] = { "PASSIVE", "DEFENSIVE", "AGGRESSIVE" };

// STATIC: 0x00A1A8C0
static const char s_petOrderTokens[4][32] = { "WAIT", "FOLLOW", "ATTACK", "DISMISS" };

// STATIC: 0x00C234CC
uint32_t CGPetInfo::m_expirationTime;

// STATIC: 0x00C234D0
WOWGUID CGPetInfo::m_pet;

// STATIC: 0x00C234D8
uint32_t CGPetInfo::m_petBarState;

// STATIC: 0x00C23530
static TSGrowableArray<PetAction> s_petSpellActions;

// VERIFY: bce460de auto
// OFFSET: 0x005D32F0
void PetAction::SetAutocast(bool enable) {
    if (enable) {
        this->m_data |= 0x40000000;
    } else {
        this->m_data &= ~0x40000000;
    }
}

// VERIFY: a9652c6a auto
// OFFSET: 0x005D3070
void CGPetInfo::ShowGrid() {
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_SHOWGRID, nullptr);
}

// VERIFY: 0f361a6f auto
// OFFSET: 0x005D3080
void CGPetInfo::HideGrid() {
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_HIDEGRID, nullptr);
}

// VERIFY: b4168bca manual
// OFFSET: 0x005D3390
WOWGUID CGPetInfo::GetPet(uint32_t index) {
    if (index >= s_pets.m_count) {
        return WOWGUID();
    }

    return s_pets.m_data[index];
}

// VERIFY: b990bce8 manual
// OFFSET: 0x005D4650
void CGPetInfo::PetClearTarget() {
    if (!CGPetInfo::m_attackingTarget) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_STOP_ATTACK));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGPetInfo::m_attackingTarget = 0;
}

// VERIFY: 9cd6d950 auto
// OFFSET: 0x005D3520
uint8_t CGPetInfo::GetComboPoints(WOWGUID target) {
    if (!target) {
        target = CGGameUI::m_lockedTarget;
    }

    if (target == CGPetInfo::m_comboTarget) {
        return CGPetInfo::m_comboPoints;
    }

    return 0;
}

// VERIFY: 61b1e561 manual
// OFFSET: 0x005D6E60
void SendRequestPetInfo() {
    CGPetInfo::SetPet(WOWGUID(), 0, 0);
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    const WOWGUID& pet = player->m_unit->m_unitFieldCharm ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

    if (!pet) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PET_INFO));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGPetInfo::FindPossessedSpell();
}

// VERIFY: cf365005 auto
// OFFSET: 0x005D3050
const char* CGPetInfo::GetModeToken(uint32_t mode) {
    return s_petModeTokens[mode];
}

// VERIFY: 5d255e3f auto
// OFFSET: 0x005D3060
const char* CGPetInfo::GetOrdersToken(uint32_t order) {
    return s_petOrderTokens[order];
}

// VERIFY: 804722ba auto
// OFFSET: 0x005D3090
void CGPetInfo::UpdateCooldowns() {
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE_COOLDOWN, nullptr);
}

// VERIFY: 5862d15e manual
// OFFSET: 0x005D33C0
WOWGUID CGPetInfo::GetTarget() {
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

    if (pet) {
        return pet->m_unit->m_unitFieldTarget;
    }

    return WOWGUID();
}

// VERIFY: 41802278 manual
// OFFSET: 0x005D64F0
void CGPetInfo::SetPetCount(uint32_t count) {
    s_pets.SetCount(count);
}

// VERIFY: d7ee5cd7 manual
// OFFSET: 0x005D3FD0
int32_t CGPetInfo::PetActionSlotUsable(const PetAction* action) {
    if (!action) {
        return 1;
    }

    uint32_t type = (action->m_data >> 24) & 0x3F;

    if (type != 1 && (type <= 7 || type > 0x11)) {
        return CGPetInfo::PetActionsUsable(false) ? 1 : 0;
    }

    if (!CGPetInfo::PetActionsUsable(true)) {
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(action->m_data & 0xFFFFFF, &spell)) {
        return 0;
    }

    uint32_t count = s_pets.m_count;
    uint32_t stunned = 0;
    uint32_t pacified = 0;
    uint32_t silenced = 0;
    uint32_t unusable = 0;

    if (!count) {
        return 1;
    }

    for (uint32_t i = 0; i < count; i++) {
        CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(i), TYPE_UNIT, __FILE__, __LINE__));

        if (!pet) {
            return 0;
        }

        if (!(spell.m_attributesExE & 0x8) && (pet->m_unit->m_unitFieldFlags & 0x40000)) {
            stunned++;
        } else if (!(spell.m_attributesExE & 0x20000) && (pet->m_unit->m_unitFieldFlags & 0x800000)) {
            pacified++;
        } else if (!(spell.m_attributesExE & 0x40000) && (pet->m_unit->m_unitFieldFlags & 0x400000)) {
            silenced++;
        }

        WOWGUID target = pet->m_unit->m_unitFieldTarget;

        if (!Spell_C_IsUsableAction(pet, &spell, target, nullptr, true)) {
            unusable++;
        }
    }

    if ((stunned && stunned == count) || (pacified && pacified == count) || (silenced && silenced == count) || (unusable && unusable == count)) {
        return 0;
    }

    return 1;
}

// VERIFY: 9ae639f9 auto
// OFFSET: 0x005D41B0
int32_t CGPetInfo::IsAttackAction(uint32_t index) {
    PetAction* action = CGPetInfo::GetPet(0) && index < sizeof(CGPetInfo::m_actions) / sizeof(CGPetInfo::m_actions[0]) ? &CGPetInfo::m_actions[index] : nullptr;

    if (action && action->m_data && (action->m_data & 0x3F000000) == 0x07000000 && (action->m_data & 0xFFFFFF) == 2) {
        return 1;
    }

    return 0;
}

// VERIFY: c296d650 auto
// OFFSET: 0x005D6D90
void CGPetInfo::InitializeGame() {
    CGPetInfo::m_pet = WOWGUID();
    CGPetInfo::m_expirationTime = 0;
    CGPetInfo::m_petBarState = 0;
    CGPetInfo::m_comboTarget = WOWGUID();
    CGPetInfo::m_comboPoints = 0;
    CGPetInfo::SetPetCount(0);
    ClientServices::SetMessageHandler(SMSG_PET_SPELLS_MESSAGE, Packet_SMSG_PET_SPELLS, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_LEARNED_SPELLS, Packet_Group_7, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_UNLEARNED_SPELLS, Packet_Group_7, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_MODE, Packet_SMSG_PET_MODE, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_ACTION_FEEDBACK, Packet_SMSG_PET_ACTION_FEEDBACK, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_BROKEN, Packet_SMSG_PET_BROKEN, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_RENAMEABLE, Packet_SMSG_PET_RENAMEABLE, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_UPDATE_COMBO_POINTS, Packet_SMSG_PET_UPDATE_COMBO_POINTS, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_GUIDS, Packet_SMSG_PET_GUIDS, nullptr);
}

// VERIFY: b893299b manual
// OFFSET: 0x005D35B0
int32_t CGPetInfo::IsPetBarUsed() {
    if (!s_pets.Count() || !s_pets.m_data[0]) {
        return 0;
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_pets.m_data[0], TYPE_UNIT, __FILE__, __LINE__));
    return !pet || (pet->IsPetBarUsed() && pet->m_unit->m_unitFieldHealth > 0 && (!CGPetInfo::m_petBarState || CGPetInfo::m_possessSpell || pet->CanGainXP()));
}

// VERIFY: b0b8cd1a manual
// OFFSET: 0x005D3560
PetAction* CGPetInfo::FindSpellByID(int32_t spellId) {
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t action = CGPetInfo::m_actions[i].m_data;
        uint32_t type = (action >> 24) & 0x3F;

        if ((type == 1 || (type >= 8 && type <= 0x11)) && static_cast<int32_t>(action & 0xFFFFFF) == spellId) {
            return &CGPetInfo::m_actions[i];
        }
    }

    return nullptr;
}

// VERIFY: 16cfdcd2 auto
// OFFSET: 0x005D30E0
void CGPetInfo::SetPossessedSpell(int32_t spellId) {
    int32_t previous = CGPetInfo::m_possessSpell;
    CGPetInfo::m_possessSpell = spellId;

    if (spellId) {
        CGPetInfo::m_petBarState = 1;
    }

    if (previous != spellId) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BONUS_ACTIONBAR, nullptr);
    }
}

// VERIFY: fdc38389 manual
// OFFSET: 0x005D3410
PetAction* CGPetInfo::GetPetSpellAction(int32_t spellId) {
    for (uint32_t i = s_petSpellActions.m_count; i; ) {
        i--;

        if ((s_petSpellActions.m_data[i].m_data & 0xFFFFFF) == static_cast<uint32_t>(spellId)) {
            return &s_petSpellActions.m_data[i];
        }
    }

    return nullptr;
}

// VERIFY: c7df8253 manual
// OFFSET: 0x005D3450
void CGPetInfo::UpdatePetSpellAction(const PetAction* action) {
    uint32_t value = action->m_data;

    if ((value & 0x3F000000) != 0x1000000) {
        return;
    }

    for (uint32_t i = s_petSpellActions.m_count; i; ) {
        i--;

        if ((s_petSpellActions.m_data[i].m_data & 0x3FFFFFFF) == (value & 0x3FFFFFFF)) {
            s_petSpellActions.m_data[i].m_data = value;
            return;
        }
    }
}

// VERIFY: 4970332e manual
// OFFSET: 0x005D4C30
int32_t Packet_Group_7(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[512];
    uint32_t spellId;

    switch (msgId) {
        case SMSG_PET_LEARNED_SPELLS: {
            msg->Get(spellId);
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                if (*spell.m_nameSubtext) {
                    SStrPrintf(name, sizeof(name), "%s (%s)", spell.m_name, spell.m_nameSubtext);
                } else {
                    SStrCopy(name, spell.m_name, sizeof(name));
                }

                CGGameUI::DisplayError(static_cast<UIErrors>(ERR_PET_LEARN_SPELL_S + ((spell.m_attributes & 0x10) != 0)), name);
            }

            break;
        }

        case SMSG_PET_UNLEARNED_SPELLS: {
            msg->Get(spellId);
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                if (*spell.m_nameSubtext) {
                    SStrPrintf(name, sizeof(name), "%s (%s)", spell.m_name, spell.m_nameSubtext);
                } else {
                    SStrCopy(name, spell.m_name, sizeof(name));
                }

                CGGameUI::DisplayError(ERR_PET_SPELL_UNLEARNED_S, name);
            }

            break;
        }

        default: {
            return 0;
        }
    }

    return 1;
}

// VERIFY: f2816ce6 auto
// OFFSET: 0x005D4DA0
int32_t Packet_SMSG_PET_ACTION_FEEDBACK(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t feedback;
    msg->Get(feedback);

    switch (feedback) {
        case 1: {
            CGGameUI::DisplayError(ERR_PET_SPELL_DEAD);
            break;
        }

        case 2: {
            uint32_t spellId = 0;
            msg->Get(spellId);
            SpellRec spell;
            UIErrors error = ERR_NO_ATTACK_TARGET;

            if (g_spellDB.GetRecord(spellId, &spell) && SpellRec::IsHelpfulOrHarmful(&spell) == 1) {
                error = ERR_GENERIC_NO_VALID_TARGETS;
            }

            CGGameUI::DisplayError(error);
            Spell_C_PetSpellFailedUpdateCooldowns(spellId);
            break;
        }

        case 3: {
            CGGameUI::DisplayError(ERR_INVALID_ATTACK_TARGET);
            uint32_t spellId = 0;
            msg->Get(spellId);
            Spell_C_PetSpellFailedUpdateCooldowns(spellId);
            break;
        }

        case 4: {
            CGGameUI::DisplayError(ERR_PET_SPELL_NOPATH);
            break;
        }
    }

    return 1;
}

// VERIFY: b1d9078d manual
// OFFSET: 0x005D5AD0
int32_t CGPetInfo::SetAction(uint32_t index, const PetAction& action, int32_t a3) {
    if (!CGGameUI::CanPerformAction(14) || (a3 && !CGPetInfo::PetActionsUsable(false))) {
        return 0;
    }

    if (action.m_data == CGPetInfo::m_actions[index].m_data) {
        return 0;
    }

    if ((action.m_data & 0x3F000000) == 0x1000000) {
        SpellRec spell;

        if (g_spellDB.GetRecord(action.m_data & 0xFFFFFF, &spell) && (spell.m_attributes & 0x40)) {
            return 0;
        }
    }

    int32_t swap = -1;

    if ((action.m_data & 0x3F000000) != 0x1000000 || (action.m_data & 0xFFFFFF)) {
        for (uint32_t i = 0; i < 10; i++) {
            if ((CGPetInfo::m_actions[i].m_data & 0x3FFFFFFF) == (action.m_data & 0x3FFFFFFF) && i != index) {
                CGPetInfo::m_actions[i].m_data = 0;
                swap = i;
                break;
            }
        }
    }

    if (swap < 0) {
        uint32_t type = (CGPetInfo::m_actions[index].m_data >> 24) & 0x3F;

        if (type == 6 || type == 7) {
            for (uint32_t i = 0; i < 10; i++) {
                uint32_t slotType = (CGPetInfo::m_actions[i].m_data >> 24) & 0x3F;

                if (slotType != 6 && slotType != 7 && !(CGPetInfo::m_actions[i].m_data & 0xFFFFFF)) {
                    swap = i;
                    break;
                }
            }

            if (swap < 0) {
                return 0;
            }
        }
    }

    if (swap >= 0) {
        CGPetInfo::m_actions[swap] = CGPetInfo::m_actions[index];
    }

    CGPetInfo::m_actions[index] = action;

    if (a3) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_PET_SET_ACTION));
        msg.Put(CGPetInfo::GetPet(0).m_guid);

        if (swap >= 0) {
            msg.Put(static_cast<uint32_t>(swap));
            msg.Put(CGPetInfo::m_actions[swap].m_data);
        }

        msg.Put(index);
        msg.Put(CGPetInfo::m_actions[index].m_data);
        msg.Finalize();
        ClientServices::Send2(&msg);
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
    }

    return swap < 0;
}

// VERIFY: f67f93cb manual
// OFFSET: 0x005D5D20
void CGPetInfo::SetAutocast(uint32_t index, int32_t enable) {
    if (!CGGameUI::CanPerformAction(2) || !CGPetInfo::PetActionsUsable(false)) {
        return;
    }

    PetAction* action = &CGPetInfo::m_actions[index];

    if (action && (action->m_data & 0x80000000)) {
        action->SetAutocast(enable < 0 ? !(action->m_data & 0x40000000) : enable != 0);
        CGPetInfo::UpdatePetSpellAction(action);
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_PET_SET_ACTION));
        msg.Put(CGPetInfo::GetPet(0).m_guid);
        msg.Put(index);
        msg.Put(CGPetInfo::m_actions[index].m_data);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
}

// VERIFY: ad93780d manual
// OFFSET: 0x005D5E10
void CGPetInfo::SetSpellAutocast(int32_t spellId, int32_t enable) {
    if (!CGGameUI::CanPerformAction(2) || !CGPetInfo::PetActionsUsable(false)) {
        return;
    }

    PetAction* action = CGPetInfo::GetPetSpellAction(spellId);

    if (!action || !(action->m_data & 0x80000000)) {
        return;
    }

    if (enable < 0) {
        enable = !(action->m_data & 0x40000000);
    }

    action->SetAutocast(enable != 0);

    for (uint32_t i = 0; i < 10; i++) {
        if (!((action->m_data ^ CGPetInfo::m_actions[i].m_data) & 0x3FFFFFFF)) {
            if (action->m_data & 0x40000000) {
                CGPetInfo::m_actions[i].m_data |= 0x40000000;
            } else {
                CGPetInfo::m_actions[i].m_data &= ~0x40000000;
            }
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_SPELL_AUTOCAST));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Put(static_cast<uint32_t>(spellId));
    msg.Put(static_cast<uint8_t>((action->m_data & 0x40000000) != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);

    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
}

// VERIFY: c40a54ec auto
// OFFSET: 0x005D2FF0
void CGPetInfo::SetPetMode(uint32_t mode) {
    CGPetInfo::m_petMode = mode | (CGPetInfo::m_petMode & 0xFFFFFF00);
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
}

// VERIFY: a72652ae auto
// OFFSET: 0x005D3020
void CGPetInfo::SetPetOrders(uint32_t orders) {
    CGPetInfo::m_petMode = (orders << 8) | (CGPetInfo::m_petMode & 0x80000FF);
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
}

// VERIFY: d8b1283c manual
// OFFSET: 0x005D3E80
bool CGPetInfo::PetActionsUsable(bool ignoreControl) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->m_unit->m_unitFieldCharmedby.m_guid) {
        return false;
    }

    if (player->GetFarSightGuid() && player->GetFarSightGuid() != CGPetInfo::GetPet(0).m_guid) {
        return false;
    }

    for (uint32_t i = 0; i < s_pets.m_count; i++) {
        CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(i), TYPE_UNIT, __FILE__, __LINE__));

        if (!pet) {
            continue;
        }

        const WOWGUID& owner = pet->m_unit->m_unitFieldCharmedby.m_guid ? pet->m_unit->m_unitFieldCharmedby : pet->m_unit->m_unitFieldSummonedby;

        if (owner == ClntObjMgrGetActivePlayer() && (ignoreControl || pet->IsControllable())) {
            if (!(CGPetInfo::m_petMode & 0x8000000)) {
                return true;
            }

            if (player->GetTransportGUID() == pet->m_obj->m_guid.m_guid) {
                return true;
            }
        }
    }

    return false;
}

// VERIFY: 06236977 manual
// OFFSET: 0x005D4210
void CGPetInfo::SendPetAction(const PetAction& action, const WOWGUID& target, int32_t a3) {
    if (!CGGameUI::CanPerformAction(2)) {
        return;
    }

    WOWGUID actionTarget = (a3 || target.m_guid) ? target : CGGameUI::m_lockedTarget;

    switch ((action.m_data >> 24) & 0x3F) {
        case 1:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17: {
            if (!CGPetInfo::PetActionsUsable(true)) {
                return;
            }

            SpellRec spell;

            if (!g_spellDB.GetRecord(action.m_data & 0xFFFFFF, &spell)) {
                return;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player) {
                return;
            }

            CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

            if (Spell_C_IsActiveSpell(&spell, pet)) {
                if (!Spell_C_WasSpellRecentlyUsed(spell.m_id, 1)) {
                    Spell_C_CancelAura(&spell, pet, -1);
                }

                return;
            }

            uint32_t pacified = 0;
            uint32_t silenced = 0;
            uint32_t disarmed = 0;

            if (s_pets.m_count) {
                for (uint32_t i = 0; i < s_pets.m_count; i++) {
                    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(i), TYPE_UNIT, __FILE__, __LINE__));

                    if (!unit) {
                        return;
                    }

                    if (!(spell.m_attributesExE & 0x8) && (unit->m_unit->m_unitFieldFlags & 0x40000)) {
                        pacified++;
                    } else if (!(spell.m_attributesExE & 0x20000) && (unit->m_unit->m_unitFieldFlags & 0x800000)) {
                        silenced++;
                    } else if (!(spell.m_attributesExE & 0x40000) && (unit->m_unit->m_unitFieldFlags & 0x400000)) {
                        disarmed++;
                    }
                }

                if ((pacified && pacified == s_pets.m_count) || (silenced && silenced == s_pets.m_count) || (disarmed && disarmed == s_pets.m_count)) {
                    return;
                }
            }

            if (!(spell.m_attributesExD & 0x20) && player->GetPossessedUnit() != pet) {
                Spell_C_StartSpellCooldown(&spell, pet);
                break;
            }

            Spell_C_CastSpell(pet, spell.m_id, nullptr, actionTarget, nullptr, false);
            return;
        }

        case 6:
            if (!CGPetInfo::PetActionsUsable(false)) {
                return;
            }

            CGPetInfo::SetPetMode(action.m_data & 0xFFFFFF);
            break;

        case 7: {
            if (!CGPetInfo::PetActionsUsable(false)) {
                return;
            }

            int32_t orders = action.m_data & 0xFFFFFF;

            if (orders <= 1) {
                CGPetInfo::SetPetOrders(orders);
                break;
            }

            if (orders != 2) {
                break;
            }

            for (uint32_t i = 0; i < s_pets.m_count; i++) {
                CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(i), TYPE_UNIT, __FILE__, __LINE__));

                if (unit && unit->VerifyAttackIconPressed(&actionTarget, a3, 1, 1, false)) {
                    CGPetInfo::m_attackingTarget = 1;
                }
            }

            if (!CGPetInfo::m_attackingTarget) {
                return;
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
            break;
        }

        default:
            if (!CGPetInfo::PetActionsUsable(false)) {
                return;
            }

            break;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_ACTION));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Put(action.m_data);
    msg.Put(actionTarget.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 409f5050 manual
// OFFSET: 0x005D6020
void CGPetInfo::PlaceAction(const PetAction& action, uint32_t index) {
    PetAction* current = GetPetAction(index);
    PetAction previous;
    previous.m_data = current ? current->m_data : 0;

    if (CGPetInfo::SetAction(index, action, 1)) {
        CGGameUI::SetCursorPetAction(&previous);
    }
}

// VERIFY: 1c67be1e auto
// OFFSET: 0x005D64A0
void CGPetInfo::PickupAction(uint32_t index, int32_t a2) {
    if (index < 10) {
        PetAction action;
        action.m_data = CGPetInfo::m_actions[index].m_data & 0xFF000000;
        CGPetInfo::SetAction(index, action, a2);
    }
}

// VERIFY: 992c0fb5 auto
// OFFSET: 0x005D64D0
void CGPetInfo::PlaceCursorAction(PetAction action, uint32_t index) {
    CGPetInfo::PlaceAction(action, index);
}

// VERIFY: 4d63ce45 manual
// OFFSET: 0x005D62A0
void CGPetInfo::FindPossessedSpell() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    int32_t onCharmVehicle = 0;

    if (player->GetTransportGUID()) {
        const WOWGUID& charm = player->m_unit->m_unitFieldCharm.m_guid ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;

        if (player->GetTransportGUID() == charm.m_guid) {
            onCharmVehicle = 1;
        }
    }

    SpellRec spell;

    for (uint32_t i = 0; i < player->GetAuraCount(); i++) {
        AuraData* aura = i < player->GetAuraCount() ? &player->m_auras[i] : nullptr;

        if ((aura->m_flags & 0x80) || !g_spellDB.GetRecord(aura->m_spellId, &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 3; j++) {
            int32_t auraType = spell.m_effectAura[j];

            if (auraType == SPELL_AURA_MOD_POSSESS || auraType == SPELL_AURA_MOD_POSSESS_PET || auraType == SPELL_AURA_MOD_CHARM || (auraType == SPELL_AURA_CONTROL_VEHICLE && onCharmVehicle)) {
                CGPetInfo::SetPossessedSpell(aura->m_spellId);
                return;
            }
        }

        for (uint32_t j = 0; j < 3; j++) {
            if (spell.m_effect[j] != SPELL_EFFECT_SUMMON) {
                continue;
            }

            SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[j]);

            if (properties && properties->m_control == 3) {
                CGPetInfo::SetPossessedSpell(aura->m_spellId);
                return;
            }
        }
    }
}

// VERIFY: ca9049e0 manual
// OFFSET: 0x005D6B90
int32_t Packet_SMSG_PET_SPELLS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    for (uint32_t i = 0; i < 10; i++) {
        CGPetInfo::m_actions[i].m_data = 0;
    }

    s_petSpellActions.SetCount(0);
    Spell_C_ClearPetCooldowns();

    uint32_t expiration = 0;
    uint16_t family = 0;
    uint64_t pet;
    msg->Get(pet);

    if (pet) {
        msg->Get(family);
        msg->Get(expiration);
        uint32_t mode;
        msg->Get(mode);
        CGPetInfo::m_petMode = mode;

        for (uint32_t i = 0; i < 10; i++) {
            uint32_t action;
            msg->Get(action);
            PetAction value;
            value.m_data = action;
            CGPetInfo::SetAction(i, value, 0);

            if (((action >> 24) & 0x3F) >= 8 && ((action >> 24) & 0x3F) <= 17) {
                CGSpellBook::AddSpellToNameCache(action & 0xFFFFFF, 1);
            }
        }

        uint8_t count;
        msg->Get(count);
        s_petSpellActions.SetCount(count);
        ClearPetSpellInfo();

        for (uint32_t i = 0; i < count; i++) {
            uint32_t action;
            msg->Get(action);
            s_petSpellActions.m_data[i].m_data = action;
            CGSpellBook::AddPetSpell(action & 0xFFFFFF);
        }

        msg->Get(count);

        for (uint32_t i = 0; i < count; i++) {
            uint32_t spellId;
            msg->Get(spellId);
            uint16_t category;
            msg->Get(category);
            uint32_t cooldown;
            msg->Get(cooldown);
            uint32_t categoryCooldown;
            msg->Get(categoryCooldown);
            bool onHold = static_cast<int32_t>(categoryCooldown) < 0;
            categoryCooldown &= ~0x80000000;
            Spell_C_SetCooldownLeft(spellId, 0, category, cooldown, categoryCooldown, onHold, 1);
        }
    } else {
        ClearPetSpellInfo();
    }

    CGPetInfo::SetPet(WOWGUID{ pet }, expiration, family);
    CGSpellBook::UpdateSpells(0, 1, 0);
    CGClassTrainer::RefreshList();
    return 1;
}

// VERIFY: 920e2dde auto
// OFFSET: 0x005D3120
int32_t Packet_SMSG_PET_BROKEN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_PET_BROKEN);
    return 1;
}

// VERIFY: 859710c8 auto
// OFFSET: 0x005D3140
int32_t Packet_SMSG_PET_RENAMEABLE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_RENAMEABLE, nullptr);
    return 1;
}

// VERIFY: 2c74fc11 manual
// OFFSET: 0x005D3240
static int32_t PetTempActionBarCallback(uint32_t index, const void* data) {
    return index < 10 ? index | 0x10000000 : 0;
}

// VERIFY: adc6dd8d auto
// OFFSET: 0x005D34B0
void CGPetInfo::SetComboPoints(WOWGUID target, uint8_t points) {
    CGPetInfo::m_comboTarget = target;
    CGPetInfo::m_comboPoints = points;
    CGSpellBook::UpdateUsable();
    Script_SendUnitSignal(CGPetInfo::GetPet(0), 149);
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE_USABLE, nullptr);
}

// VERIFY: 0f67ae3d manual
// OFFSET: 0x005D46F0
void CGPetInfo::PetDismiss() {
    if (!CGPetInfo::PetActionsUsable(false)) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (CGPetInfo::GetPet(0) == CGUnit_C::s_activeMover && player && CGPetInfo::GetPet(0).m_guid == player->GetTransportGUID()) {
        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (mover) {
            uint8_t seat = static_cast<uint8_t>(player->m_movementData.m_transportSeat);

            if (mover->m_vehicle) {
                VehicleSeatRec* seatRec = mover->m_vehicle->GetVehicleSeatRec(seat);

                if (seatRec && (seatRec->m_flags & 0x2000000)) {
                    if (player->m_vehiclePassenger) {
                        player->m_vehiclePassenger->StopMovement();
                    }

                    mover->OnDismissControlledVehicleLocal(FrameTime::s_curTimeMs);
                }
            }

            return;
        }
    }

    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    SummonPropertiesRec* properties;

    if (pet && (properties = pet->GetSummonProperties()) && (properties->m_flags & 0x20)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_ACTION));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Put(static_cast<uint32_t>(0x07000003));
    msg.Put(static_cast<uint64_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_DISMISS_START, "%d", 10000);
}

// VERIFY: 344c914b manual
// OFFSET: 0x005D4900
void CGPetInfo::PetAbandon() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_ABANDON));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (CGStableInfo::m_stableMaster) {
        CDataStore list;
        list.Put(static_cast<uint32_t>(MSG_LIST_STABLED_PETS));
        list.Put(CGStableInfo::m_stableMaster.m_guid);
        list.Finalize();
        ClientServices::Send2(&list);
    }
}

// VERIFY: 3feade6b manual
// OFFSET: 0x005D4A00
void CGPetInfo::PetRename(const char* name, const char* declinedNames) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_PET_RENAME));
    msg.Put(CGPetInfo::GetPet(0).m_guid);
    msg.PutString(name);

    if (declinedNames) {
        msg.Put(static_cast<uint8_t>(1));

        for (uint32_t i = 0; i < 5; i++) {
            msg.PutString(declinedNames);
            declinedNames += 96;
        }
    } else {
        msg.Put(static_cast<uint8_t>(0));
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1003d217 manual
// OFFSET: 0x005D4AD0
void CGPetInfo::UpdatePetTempBar() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t active = 0;

    if (player && player->m_obj->m_guid == ClntObjMgrGetActivePlayer() && player->m_player->m_playerFarsight.m_guid && player->GetFarSightGuid() == CGPetInfo::GetPet(0).m_guid && CGPetInfo::IsPetBarUsed()) {
        active = 1;
        CVehicle_C* vehicle;
        VehicleSeatRec* seat;

        if (!s_pets.m_count || !s_pets.m_data[0].m_guid || player->GetTransportGUID() != s_pets.m_data[0].m_guid || !(vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__))->m_vehicle) || !(seat = vehicle->GetVehicleSeatRec(static_cast<uint8_t>(player->m_movementData.m_transportSeat))) || (active = seat->m_vehicleAbilityDisplay == 1) != 0) {
            CGActionBar::UpdateTemporaryActionBar(PetTempActionBarCallback, nullptr);
        }
    }

    CGActionBar::SetTemporaryActionBarFlags(1, active);
    CGActionBar::UpdateBonusBar();
    FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
}

// VERIFY: 5d3e1ddc auto
// OFFSET: 0x005D6550
int32_t Packet_SMSG_PET_GUIDS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count;
    msg->Get(count);
    CGPetInfo::SetPetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        WOWGUID guid;
        msg->Get(guid.m_guid);

        if (i < s_pets.m_count) {
            s_pets.m_data[i] = guid;
        }
    }

    return 1;
}

// VERIFY: 138ffa1d manual
// OFFSET: 0x005D6900
void CGPetInfo::SetPet(WOWGUID pet, uint32_t expiration, int32_t family) {
    int32_t changed = CGPetInfo::m_pet != pet;
    CGPetInfo::m_pet = pet;

    if (pet) {
        uint32_t index = 0;

        if (s_pets.m_count) {
            for (index = 0; index < s_pets.m_count; index++) {
                if (s_pets.m_data[index] == pet) {
                    break;
                }
            }

            if (index) {
                WOWGUID first = s_pets.m_data[0];
                s_pets.m_data[0] = pet;

                if (index < s_pets.m_count) {
                    s_pets.m_data[index] = first;
                }
            }
        }

        if (index == s_pets.m_count) {
            CGPetInfo::SetPetCount(1);

            if (s_pets.m_count) {
                s_pets.m_data[0] = pet;
            }
        }
    } else {
        CGPetInfo::SetPetCount(0);
    }

    CGPetInfo::m_expirationTime = expiration ? expiration + OsGetAsyncTimeMs() : 0;

    if (changed) {
        CGPetInfo::m_petBarState = 0;
        CGPetInfo::m_attackingTarget = 0;

        if (s_pets.m_count && s_pets.m_data[0]) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_pets.m_data[0], TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                CGPetInfo::m_attackingTarget = unit->IsInCombat() != 0;

                if (unit->CanGainXP()) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_PET_UI_CLOSE, nullptr);
                }
            }

            CGTalentInfo::UpdatePetTabInfo(family);
        } else {
            CGPetInfo::SetComboPoints(WOWGUID(), 0);
            CGTalentInfo::UpdatePetTabInfo(0);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_PET_2, "%s", "player");
    }

    CGPetInfo::UpdatePetTempBar();

    if (Spell_C_IsModal(CGPetInfo::GetPet(0)) && (changed || !s_pets.m_count || !s_pets.m_data[0])) {
        Spell_C_CancelSpellByCaster(pet);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && changed && player->m_unit->m_unitFieldHealth <= 0) {
        if (s_pets.m_count && s_pets.m_data[0]) {
            FrameScript_SignalEvent(SCRIPT_EVENT_RAISED_AS_GHOUL, nullptr);
        } else {
            CGGameUI::UpdateActivePlayer();
        }
    }
}

// VERIFY: 833963cb auto
// OFFSET: 0x005D4620
void CGPetInfo::PetAttackTarget(const WOWGUID& target, int32_t a2) {
    PetAction action;
    action.m_data = 0x7000002;
    CGPetInfo::SendPetAction(action, target, a2);
}

// VERIFY: 317c171a manual
// OFFSET: 0x005D3630
int32_t Packet_SMSG_PET_MODE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID pet;
    uint32_t mode;
    msg->Get(pet.m_guid);
    msg->Get(mode);

    if (pet == CGPetInfo::GetPet(0)) {
        CGPetInfo::m_petMode = mode;
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_BAR_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: a7d77109 manual
// OFFSET: 0x005D36A0
int32_t Packet_SMSG_PET_UPDATE_COMBO_POINTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID pet;
    SmartGUID target;
    uint8_t points;
    *msg >> pet;
    *msg >> target;
    msg->Get(points);

    if (pet == CGPetInfo::GetPet(0)) {
        CGPetInfo::SetComboPoints(target, points);
    }

    return 1;
}

// VERIFY: 8d48f08a manual
// OFFSET: 0x005D30A0
void CGPetInfo::SetIsAttackingTarget(int32_t attacking) {
    if (attacking) {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_ATTACK_START, nullptr);
        CGPetInfo::m_attackingTarget = attacking;
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_PET_ATTACK_STOP, nullptr);
        CGPetInfo::m_attackingTarget = 0;
    }
}

// VERIFY: 62a49fb5 auto
// OFFSET: 0x005D2F90
void CGPetInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_PET_SPELLS_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_PET_LEARNED_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_PET_UNLEARNED_SPELLS);
    ClientServices::ClearMessageHandler(SMSG_PET_MODE);
    ClientServices::ClearMessageHandler(SMSG_PET_ACTION_FEEDBACK);
    ClientServices::ClearMessageHandler(SMSG_PET_BROKEN);
    ClientServices::ClearMessageHandler(SMSG_PET_RENAMEABLE);
    ClientServices::ClearMessageHandler(SMSG_PET_UPDATE_COMBO_POINTS);
    ClientServices::ClearMessageHandler(SMSG_PET_GUIDS);
}

// VERIFY: 92931bd9 auto
// OFFSET: 0x005D3360
void CGPetInfo::LeaveWorld() {
    int32_t possessSpell = CGPetInfo::m_possessSpell;
    CGPetInfo::m_possessSpell = 0;

    if (possessSpell) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BONUS_ACTIONBAR, nullptr);
    }
}
