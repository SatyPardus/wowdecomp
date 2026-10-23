#include "UIMacroOptions.h"
#include <cstring>
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MacroConditions.h"
#include "WoW/Source/Ui/MacroSubstring.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/UIMacros.h"

void InitializeMacroOptions() {
}

// VERIFY: a046a90a auto
// OFFSET: 0x005EED70
int32_t EvaluateExists(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGObject_C* object = ClntObjMgrObjectPtr(*unit, TYPE_OBJECT, __FILE__, __LINE__);
    int32_t exists = (object && object->CanBeTargetted()) || CGGameUI::IsRaidMemberOrPet(*unit);

    if (args->m_count) {
        return exists == args->m_data->ToNumber();
    }

    return exists;
}

// VERIFY: 44e9953c auto
// OFFSET: 0x005EEDE0
int32_t EvaluateParty(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    int32_t party = CGPartyInfo::IsPlayerOrPetInParty(*unit);

    if (args->m_count) {
        return party == args->m_data->ToNumber();
    }

    return party;
}

// VERIFY: e207a633 auto
// OFFSET: 0x005EEE20
int32_t EvaluateRaid(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    int32_t raid = CGPartyInfo::IsPlayerOrPetInParty(*unit) || CGRaidInfo::IsPlayerOrPetInRaid(*unit);

    if (args->m_count) {
        return raid == args->m_data->ToNumber();
    }

    return raid;
}

// VERIFY: efb4b366 manual
// OFFSET: 0x005EEE70
int32_t EvaluateFlyable(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(*unit, TYPE_UNIT, __FILE__, __LINE__));
    bool flyable = player ? player->IsInFlyableZone(1) : IsInFlyableArea() != 0;

    if (args->m_count) {
        return flyable == args->m_data->ToNumber();
    }

    return flyable;
}

// VERIFY: 21b64279 manual
// OFFSET: 0x005EEED0
int32_t EvaluateGroup(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    if (!args->m_count) {
        return CGPartyInfo::GetNumPartyMembers() || CGRaidInfo::m_numMembers;
    }

    for (uint32_t i = args->m_count; i; i--) {
        MacroSubstring& arg = args->m_data[i - 1];

        if (arg == "party") {
            return CGPartyInfo::GetNumPartyMembers() != 0;
        }

        if (arg == "raid") {
            return CGRaidInfo::m_numMembers != 0;
        }
    }

    return 0;
}

// VERIFY: 4dbe7b4a manual
// OFFSET: 0x005EF0D0
int32_t EvaluateButton(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    const char* button = CSimpleTop::s_instance->m_mouseButtonName;
    const char* macroButton = CGUIMacros::m_macroRunning;

    if (!button && !macroButton) {
        button = "LeftButton";
    }

    if (!args->m_count) {
        return 0;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        char name[1024];

        if (*arg.m_start >= '0' && *arg.m_start <= '9') {
            uint32_t index = 0;

            for (const char* c = arg.m_start; c < arg.m_end && *c >= '0' && *c <= '9'; c++) {
                index = *c + 10 * index - '0';
            }

            SStrCopy(name, GetMouseButtonName(IndexToMouseButton(index)), sizeof(name));
        } else {
            uint32_t length = arg.m_end - arg.m_start;

            if (length > 1023) {
                length = 1023;
            }

            memcpy(name, arg.m_start, length);
            name[length] = '\0';
        }

        if ((button && !SStrCmpUTF8I(name, button, 0x7FFFFFFF)) || (macroButton && !SStrCmpUTF8I(name, macroButton, 0x7FFFFFFF))) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: ecd36a2e manual
// OFFSET: 0x005EF200
int32_t EvaluateActionBar(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    if (!args->m_count) {
        return 0;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        uint32_t page = 0;

        for (const char* c = arg.m_start; c < arg.m_end && *c >= '0' && *c <= '9'; c++) {
            page = *c + 10 * page - '0';
        }

        if (page == CGActionBar::m_currentPage + 1) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: a88db050 manual
// OFFSET: 0x005EF270
int32_t EvaluateBonusBar(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    if (!args->m_count) {
        return 0;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        int32_t offset = 0;

        for (const char* c = arg.m_start; c < arg.m_end && *c >= '0' && *c <= '9'; c++) {
            offset = *c + 10 * offset - '0';
        }

        if (offset == CGActionBar::m_bonusBarOffset) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: fd102a98 manual
// OFFSET: 0x005EF2E0
int32_t EvaluateSpec(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    uint32_t group = CGTalentInfo::GetActiveTalentGroup(0) + 1;

    if (!args->m_count) {
        return 0;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        uint32_t spec = 0;

        for (const char* c = arg.m_start; c < arg.m_end && *c >= '0' && *c <= '9'; c++) {
            spec = *c + 10 * spec - '0';
        }

        if (spec == group) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: c6220e35 auto
// OFFSET: 0x005EF360
static VehicleSeatRec* GetVehicleSeatRec(const WOWGUID* guid) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if (!unit->HasVehicleTransport()) {
            return nullptr;
        }

        WOWGUID transportGuid;
        transportGuid.m_guid = unit->GetTransportGUID();

        if (!transportGuid) {
            return nullptr;
        }

        CGUnit_C* transport = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transportGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (!transport || !transport->m_vehicle) {
            return nullptr;
        }

        return transport->m_vehicle->GetVehicleSeatRec(unit->m_movementData.m_transportSeat);
    }

    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(*guid);

    if (state && state->m_vehicleSeatId > 0) {
        return g_vehicleSeatDB.GetRecord(state->m_vehicleSeatId);
    }

    return nullptr;
}

// VERIFY: 21442e4c auto
// OFFSET: 0x005EF430
int32_t EvaluateVehicleUI(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    WOWGUID guid = ClntObjMgrGetActivePlayer();
    VehicleSeatRec* seat = GetVehicleSeatRec(&guid);

    if (seat) {
        return seat->m_flags & 0x20000000;
    }

    return 0;
}

// VERIFY: d471b39f auto
// OFFSET: 0x005EF470
int32_t EvaluateUnitHasVehicleUI(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    VehicleSeatRec* seat = GetVehicleSeatRec(unit);

    if (seat) {
        return seat->m_flags & 0x20000000;
    }

    return 0;
}

// VERIFY: 4bc45a8e manual
// OFFSET: 0x005EF490
int32_t EvaluateCursor(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    if (!CSimpleTop::s_instance->m_allowProtectedActions) {
        return 0;
    }

    const char* type;

    switch (CGGameUI::m_cursorItemType) {
        case UICURSOR_ITEM:
        case UICURSOR_ACTIONBAR:
        case UICURSOR_AMMO:
        case UICURSOR_GUILDBANK_ITEM:
            type = "item";
            break;

        case UICURSOR_MONEY:
            type = "money";
            break;

        case UICURSOR_SPELL:
            type = "spell";
            break;

        case UICURSOR_MERCHANT:
            type = "merchant";
            break;

        case UICURSOR_MACRO:
            type = "macro";
            break;

        case UICURSOR_GUILDBANK_MONEY:
            type = "guildbankmoney";
            break;

        case UICURSOR_EQUIPMENT_SET:
            type = "equipmentset";
            break;

        default:
            return 0;
    }

    if (!args->m_count) {
        return 1;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        char text[1024];
        uint32_t length = arg.m_end - arg.m_start;

        if (length > 1023) {
            length = 1023;
        }

        memcpy(text, arg.m_start, length);
        text[length] = '\0';

        if (!SStrCmpI(text, type, 0x7FFFFFFF)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 70e118bd manual
// OFFSET: 0x005EF710
int32_t EvaluateCombat(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t combat;

    if (player && (player->m_unit->m_unitFieldFlags & 0x80000)) {
        combat = 1;
    } else {
        CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
        combat = pet && (pet->m_unit->m_unitFieldFlags & 0x80000);
    }

    if (args->m_count) {
        return combat == args->m_data->ToNumber();
    }

    return combat;
}

// VERIFY: 606170a1 auto
// OFFSET: 0x005EF7B0
int32_t EvaluateHelp(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*unit, TYPE_UNIT, __FILE__, __LINE__));
    int32_t help = player && target && player->CanAssist(target, false);

    if (args->m_count) {
        return help == args->m_data->ToNumber();
    }

    return help;
}

// VERIFY: 2f3abe7a auto
// OFFSET: 0x005EF830
int32_t EvaluateHarm(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*unit, TYPE_UNIT, __FILE__, __LINE__));
    int32_t harm = player && target && player->CanAttack(target);

    if (args->m_count) {
        return harm == args->m_data->ToNumber();
    }

    return harm;
}

// VERIFY: 6e6f1e87 manual
// OFFSET: 0x005EF8B0
int32_t EvaluateDead(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGUnit_C* object = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*unit, TYPE_UNIT, __FILE__, __LINE__));
    int32_t dead;

    if (object) {
        if (object->m_unit->m_unitFieldHealth <= 0) {
            dead = 1;
        } else if ((object->m_unit->m_unitDynamicFlags & 0x20) && !CGGameUI::IsRaidMemberOrPet(object->m_obj->m_guid)) {
            dead = 1;
        } else if ((object->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(object)->m_player->m_playerFlags & 0x10)) {
            dead = 1;
        } else {
            dead = 0;
        }
    } else {
        CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(*unit);
        dead = state ? (state->m_flags & 0xC) : 0;
    }

    if (args->m_count) {
        return dead == args->m_data->ToNumber();
    }

    return dead;
}

// VERIFY: 06e51fae manual
// OFFSET: 0x005EF980
int32_t EvaluateIndoors(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t indoors = player && !player->IsOutdoors();

    if (args->m_count) {
        return indoors == args->m_data->ToNumber();
    }

    return indoors;
}

// VERIFY: 1feef5e1 manual
// OFFSET: 0x005EF9E0
int32_t EvaluateOutdoors(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t outdoors = player && player->IsOutdoors();

    if (args->m_count) {
        return outdoors == args->m_data->ToNumber();
    }

    return outdoors;
}

// VERIFY: 96f9b74e manual
// OFFSET: 0x005EFA40
int32_t EvaluateSwimming(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t swimming = player && (player->m_clientFlags & 0x200000);

    if (args->m_count) {
        return swimming == args->m_data->ToNumber();
    }

    return swimming;
}

// VERIFY: b5e8db05 manual
// OFFSET: 0x005EFAA0
int32_t EvaluateFlying(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t flying = player && (player->m_movement->m_flags & 0x2000000);

    if (args->m_count) {
        return flying == args->m_data->ToNumber();
    }

    return flying;
}

// VERIFY: 8a66275e manual
// OFFSET: 0x005EFB00
int32_t EvaluateMounted(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t mounted = player && player->m_mountDisplayId > 0 && !(player->m_clientFlags & 0x10000000);

    if (args->m_count) {
        return mounted == args->m_data->ToNumber();
    }

    return mounted;
}

// VERIFY: 53625612 manual
// OFFSET: 0x005EFB70
int32_t EvaluateStealth(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t stealth = player && (player->m_unit->m_unitFieldBytes1[2] & 0x2);

    if (args->m_count) {
        return stealth == args->m_data->ToNumber();
    }

    return stealth;
}

// VERIFY: bf238021 manual
// OFFSET: 0x005EFBD0
int32_t EvaluatePet(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    const WOWGUID* petGuid = player->m_unit->m_unitFieldCharm.m_guid ? &player->m_unit->m_unitFieldCharm : &player->m_unit->m_unitFieldSummon;
    CGUnit_C* pet = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*petGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!pet) {
        return 0;
    }

    if (!args->m_count) {
        return 1;
    }

    char name[1024] = "";
    char family[1024] = "";
    SStrCopy(name, pet->GetUnitName(nullptr, 1), sizeof(name));
    uint32_t familyId = pet->GetCreatureFamily();

    if (familyId) {
        CreatureFamilyRec* rec = g_creatureFamilyDB.GetRecord(familyId);

        if (rec) {
            SStrCopy(family, rec->m_name, sizeof(family));
        }
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        char text[1024];
        uint32_t length = arg.m_end - arg.m_start;

        if (length > 1023) {
            length = 1023;
        }

        memcpy(text, arg.m_start, length);
        text[length] = '\0';

        if (!SStrCmpUTF8I(text, name, 0x7FFFFFFF) || !SStrCmpUTF8I(text, family, 0x7FFFFFFF)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 0657ba53 manual
// OFFSET: 0x005EFD70
int32_t EvaluateEquipped(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        char text[1024];
        uint32_t length = arg.m_end - arg.m_start;

        if (length > 1023) {
            length = 1023;
        }

        memcpy(text, arg.m_start, length);
        text[length] = '\0';

        int32_t inventoryType = -1;

        for (int32_t j = 0; j < 29; j++) {
            if (!SStrCmpI(text, s_inventoryTypeTokens[j], 0x7FFFFFFF) || !SStrCmpUTF8I(text, FrameScript_GetText(s_inventoryTypeTokens[j], -1, GENDER_NOT_APPLICABLE), 0x7FFFFFFF)) {
                inventoryType = j;
                break;
            }
        }

        WOWGUID container;
        uint32_t slot;
        CGItem_C* item;

        if (inventoryType > 0) {
            item = player->m_bag.FindItem(&CanEquipInSlotCallback, &inventoryType, container, slot, 1);
        } else {
            item = player->m_bag.FindItem(&FindItemClassTypeCallback, text, container, slot, 1);
        }

        if (item) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 84436a9f manual
// OFFSET: 0x005EFEB0
int32_t EvaluateChanneling(const WOWGUID* unit, const TSGrowableArray<MacroSubstring>* args) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    uint32_t now = OsGetAsyncTimeMs();
    SpellRec spell;

    if (!g_spellDB.GetRecord(player->m_channelSpellId, &spell) || static_cast<int32_t>(now - player->m_channelEndTime) >= 0) {
        return 0;
    }

    if (!args->m_count) {
        return 1;
    }

    for (uint32_t i = args->m_count; i; i--) {
        const MacroSubstring& arg = args->m_data[i - 1];
        char text[1024];
        uint32_t length = arg.m_end - arg.m_start;

        if (length > 1023) {
            length = 1023;
        }

        memcpy(text, arg.m_start, length);
        text[length] = '\0';

        if (!SStrCmpUTF8I(text, spell.m_name, 0x7FFFFFFF)) {
            return 1;
        }
    }

    return 0;
}
