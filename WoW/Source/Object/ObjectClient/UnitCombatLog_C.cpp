#include "UnitCombatLog_C.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/ObjectClient/UnitCombat_C.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <ctime>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/GameObjectStats_C.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Game/GameClient/PetNameCache.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/DB/Rec/SpellMechanicRec.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/SpellDurationRec.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Game/GameClient/WorldText.h"
#include "WoW/Source/Game/ValidateName.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "WoW/Source/Object/CreatureStats.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"

// STATIC: 0x00ADB758
static const char* s_combatLogEventNames[50] = {
    "ENVIRONMENTAL_DAMAGE",
    "SWING_DAMAGE",
    "SWING_MISSED",
    "RANGE_DAMAGE",
    "RANGE_MISSED",
    "SPELL_CAST_START",
    "SPELL_CAST_SUCCESS",
    "SPELL_CAST_FAILED",
    "SPELL_MISSED",
    "SPELL_DAMAGE",
    "SPELL_HEAL",
    "SPELL_ENERGIZE",
    "SPELL_DRAIN",
    "SPELL_LEECH",
    "SPELL_INSTAKILL",
    "SPELL_SUMMON",
    "SPELL_CREATE",
    "SPELL_INTERRUPT",
    "SPELL_EXTRA_ATTACKS",
    "SPELL_DURABILITY_DAMAGE",
    "SPELL_DURABILITY_DAMAGE_ALL",
    "SPELL_AURA_APPLIED",
    "SPELL_AURA_APPLIED_DOSE",
    "SPELL_AURA_REMOVED_DOSE",
    "SPELL_AURA_REMOVED",
    "SPELL_AURA_REFRESH",
    "SPELL_DISPEL",
    "SPELL_STOLEN",
    "SPELL_AURA_BROKEN",
    "SPELL_AURA_BROKEN_SPELL",
    "DAMAGE_AURA_BROKEN",
    "ENCHANT_APPLIED",
    "ENCHANT_REMOVED",
    "SPELL_PERIODIC_MISSED",
    "SPELL_PERIODIC_DAMAGE",
    "SPELL_PERIODIC_HEAL",
    "SPELL_PERIODIC_ENERGIZE",
    "SPELL_PERIODIC_DRAIN",
    "SPELL_PERIODIC_LEECH",
    "SPELL_DISPEL_FAILED",
    "DAMAGE_SHIELD",
    "DAMAGE_SHIELD_MISSED",
    "DAMAGE_SPLIT",
    "PARTY_KILL",
    "UNIT_DIED",
    "UNIT_DESTROYED",
    "SPELL_RESURRECT",
    "SPELL_BUILDING_DAMAGE",
    "SPELL_BUILDING_HEAL",
    "UNIT_DISSIPATES"
};

// STATIC: 0x00ADB820
static const char* s_combatTextTypes[48] = {
    "INTERRUPT",
    "DAMAGE_CRIT",
    "DAMAGE",
    "MISS",
    "DODGE",
    "PARRY",
    "EVADE",
    "IMMUNE",
    "DEFLECT",
    "REFLECT",
    "RESIST",
    "BLOCK",
    "ABSORB",
    "SPELL_DAMAGE_CRIT",
    "SPELL_DAMAGE",
    "SPELL_MISS",
    "SPELL_DODGE",
    "SPELL_PARRY",
    "SPELL_EVADE",
    "SPELL_IMMUNE",
    "SPELL_DEFLECT",
    "SPELL_REFLECT",
    "SPELL_RESIST",
    "SPELL_BLOCK",
    "SPELL_ABSORB",
    "ENCHANTMENT_REMOVED",
    "ENCHANTMENT_ADDED",
    "PERIODIC_HEAL",
    "ENERGIZE",
    "PERIODIC_ENERGIZE",
    "SPELL_CAST",
    "SPELL_AURA_END",
    "SPELL_AURA_END_HARMFUL",
    "SPELL_AURA_START",
    "SPELL_AURA_START_HARMFUL",
    "SPELL_ACTIVE",
    "FACTION",
    "HEAL_CRIT",
    "HEAL",
    "DAMAGE_SHIELD",
    "SPELL_DISPELLED",
    "EXTRA_ATTACKS",
    "SPLIT_DAMAGE",
    "HONOR_GAINED",
    "PERIODIC_HEAL_ABSORB",
    "HEAL_CRIT_ABSORB",
    "HEAL_ABSORB",
    "ARENA_POINTS_GAINED"
};

// STATIC: 0x00ADB93C
static const uint32_t s_mechanicTextColor = 0xFFFFDE00;

// STATIC: 0x00ADB944
static TSList<InvalidatedName, TSGetLink<InvalidatedName> > s_invalidatedNames;

// STATIC: 0x00ADB950
static TSList<PendingCombatLogData, TSGetLink<PendingCombatLogData> > s_pendingData;

// STATIC: 0x00ADB95C
static TSList<PendingCombatLogData, TSGetLink<PendingCombatLogData> > s_freePendingData;

// STATIC: 0x00ADB968
TSList<CombatLogFilter, TSGetLink<CombatLogFilter> > s_filters;

// STATIC: 0x00ADB974
TSList<CombatLogEntry, TSGetLink<CombatLogEntry> > s_entries;

// STATIC: 0x00ADB980
static TSList<CombatLogEntry, TSGetLink<CombatLogEntry> > s_freeEntries;

// STATIC: 0x00CA1380
WOWGUID s_combatTextActiveUnit;

// STATIC: 0x00CA1388
static uint32_t s_startTimeMs;

// STATIC: 0x00CA138C
static uint32_t s_startTime;

// STATIC: 0x00CA1390
CombatLogEntry* s_currentEntry;

// STATIC: 0x00CA1394
CombatLogEntry* s_pendingEntries;

// STATIC: 0x00ADBFAC
const char* g_spellMissStrings[12] = { "NONE", "MISS", "RESIST", "DODGE", "PARRY", "BLOCK", "EVADE", "IMMUNE", "IMMUNE", "DEFLECT", "ABSORB", "REFLECT" };

// STATIC: 0x00AD671C
static const char* s_environmentalDamageTypes[6] = { "FATIGUE", "DROWNING", "FALLING", "LAVA", "SLIME", "FIRE" };

// INLINED
inline CombatLogEntry::CombatLogEntry() {
    this->m_infoFlags = 0;
    this->Clear();
}

// INLINED
inline CombatLogEntry::~CombatLogEntry() {
    if (this->m_infoFlags & 0x2) {
        SMemFree(this->m_info.m_text, __FILE__, __LINE__, 0);
    }

    this->CancelPendingCallbacks();
}

// INLINED
inline InvalidatedName::InvalidatedName() : m_name(nullptr) {
}

// INLINED
inline InvalidatedName::~InvalidatedName() {
    SMemFree(this->m_name, __FILE__, __LINE__, 0);
}

// VERIFY: e107c6e5 manual
// OFFSET: 0x0074D1E0
void CombatLogEntry::SetFailReason(const char* reason) {
    this->m_infoFlags |= 0x2;
    this->m_info.m_text = SStrDupA(reason, __FILE__, __LINE__);
}

// VERIFY: 77d0900b auto
// OFFSET: 0x0074D210
void __stdcall CheckToSanitizeUnitLowGuid(WOWGUID* guid) {
    uint32_t type = static_cast<uint32_t>(guid->m_guid >> 32) & 0xF0F00000;

    if (type == 0xF0300000 || type == 0xF0500000) {
        CreatureStats_C* stats = g_creatureDBCache.GetRecord(static_cast<int32_t>(guid->m_guid >> 24) & 0xFFFFFFF, *guid, nullptr, nullptr, false);

        if (stats && (stats->m_typeFlags & 0x4000)) {
            guid->m_guid &= 0xFFFFFFFFFF000000ull;
        }
    }
}

// VERIFY: 178bd755 auto
// OFFSET: 0x0074D430
int32_t IsCombatTextActiveUnit(WOWGUID guid) {
    return s_combatTextActiveUnit == guid;
}

// VERIFY: 4b539030 auto
// OFFSET: 0x0074D460
void SendToCombatText(int32_t type, const char* text, int32_t amount) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s%d", s_combatTextTypes[type], text, amount);
}

// VERIFY: ec53185e auto
// OFFSET: 0x0074D490
void SendToCombatText(int32_t type, const char* text, int32_t amount, int32_t overflow) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s%d%d", s_combatTextTypes[type], text, amount, overflow);
}

// VERIFY: 41c0dd5e auto
// OFFSET: 0x0074D4C0
void SendToCombatText(int32_t type, int32_t amount, int32_t overflow) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%d", s_combatTextTypes[type], amount, overflow);
}

// VERIFY: 832fb9f0 auto
// OFFSET: 0x0074D4F0
void SendToCombatText(int32_t type, int32_t amount) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d", s_combatTextTypes[type], amount);
}

// VERIFY: 4c54569b auto
// OFFSET: 0x0074D520
void SendToCombatText(int32_t type, const char* text) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s", s_combatTextTypes[type], text);
}

// VERIFY: 8f0453d6 auto
// OFFSET: 0x0074D550
void SendToCombatText(int32_t type) {
    FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s", s_combatTextTypes[type]);
}

// VERIFY: 197967e0 auto
// OFFSET: 0x0074D700
void UnitCombatLogProcGo(const WOWGUID& casterGuid, const SpellRec* spell) {
    if (s_combatTextActiveUnit == casterGuid && (spell->m_attributesExF & 0x40)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s", s_combatTextTypes[30], spell->m_name);
    }
}

// VERIFY: ab3c05fe manual
// OFFSET: 0x0074D920
void CombatLogEntry::Clear() {
    this->m_event = -1;
    this->m_pendingMask = 0;
    this->m_sourceGuid = WOWGUID();
    this->m_sourceName = nullptr;
    this->m_sourceRealm = nullptr;
    this->m_sourceFlags = 0x80000000;
    this->m_destGuid = WOWGUID();
    this->m_destName = nullptr;
    this->m_destRealm = nullptr;
    this->m_destFlags = 0x80000000;
    this->m_spellId = 0;
    this->m_spellName.Copy(static_cast<const char*>(nullptr));

    if (this->m_infoFlags & 0x2) {
        SMemFree(this->m_info.m_text, __FILE__, __LINE__, 0);
    }

    this->m_infoFlags = 0;
}

// VERIFY: 10cdb977 manual
// OFFSET: 0x0074D980
void CombatLogEntry::SetMissInfo(int32_t missType, int32_t amount) {
    this->m_infoFlags |= 0x4;
    this->m_info.m_values[0] = missType;

    switch (missType) {
        case 2:
            this->m_infoFlags |= 0x4000;
            this->m_info.m_values[5] = amount;
            break;
        case 5:
            this->m_infoFlags |= 0x8000;
            this->m_info.m_values[6] = amount;
            break;
        case 10:
            this->m_infoFlags |= 0x2000;
            this->m_info.m_values[4] = amount;
            break;
    }
}

// VERIFY: 4abe76e4 manual
// OFFSET: 0x0074E050
bool CombatLogFilter::SetFilterData(const char* events, const WOWGUID& sourceGuid, uint32_t sourceFlags, const WOWGUID& destGuid, uint32_t destFlags, int32_t spellId, const char* spellName) {
    if (events) {
        if (this->m_events.m_count) {
            memset(this->m_events.m_bits.m_data, 0, sizeof(uint32_t) * this->m_events.m_bits.m_count);
        }

        char token[128];

        do {
            SStrTokenize(&events, token, sizeof(token), " ,", nullptr);

            if (!token[0]) {
                break;
            }

            for (uint32_t i = 0; i < 50; i++) {
                if (!SStrCmpI(token, s_combatLogEventNames[i], STORM_MAX_STR)) {
                    this->m_events.m_bits.m_data[i >> 5] |= 1 << (i & 0x1F);
                    break;
                }
            }
        } while (token[0] && *events);
    } else if (this->m_events.m_count) {
        memset(this->m_events.m_bits.m_data, 0xFF, sizeof(uint32_t) * this->m_events.m_bits.m_count);
    }

    if (sourceGuid) {
        this->m_sourceGuid = sourceGuid;
        this->m_sourceFlags = 0;
    } else {
        this->m_sourceGuid = WOWGUID();
        this->m_sourceFlags = sourceFlags;
    }

    if (destGuid) {
        this->m_destGuid = destGuid;
        this->m_destFlags = 0;
    } else {
        this->m_destGuid = WOWGUID();
        this->m_destFlags = destFlags;
    }

    if (spellId) {
        this->m_spellId = spellId;
        this->m_spellName.Copy(static_cast<const char*>(nullptr));
        return true;
    }

    this->m_spellId = 0;
    this->m_spellName.Copy(spellName);
    return true;
}

// VERIFY: 23038301 manual
// OFFSET: 0x0074F350
PendingCombatLogData* GetPendingCombatLogData(CombatLogEntry* entry, const WOWGUID& guid, uint32_t mask) {
    PendingCombatLogData* data = s_freePendingData.Head();

    if (!data) {
        data = s_freePendingData.NewNode(2, 0, 0);
    }

    data->m_entry = entry;
    data->m_guid = guid;
    data->m_id = 0;
    data->m_type = 0;
    data->m_mask = mask;
    return data;
}

// VERIFY: 106263c5 manual
// OFFSET: 0x0074F400
void UnitCombatLogInvalidateName(const char* name) {
    InvalidatedName* node = s_invalidatedNames.NewNode(2, 0, 0);
    node->m_name = SStrDupA(name, __FILE__, __LINE__);

    for (CombatLogEntry* entry = s_entries.Head(); entry; entry = s_entries.Next(entry)) {
        if (entry->m_sourceName == name) {
            entry->m_sourceName = node->m_name;
        }

        if (entry->m_destName == name) {
            entry->m_destName = node->m_name;
        }
    }
}

// VERIFY: aa58db07 manual
// OFFSET: 0x0074D0D0
char* GUIDToHexString(WOWGUID guid, char* buffer) {
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[18] = 0;

    uint64_t value = guid.m_guid;
    char* digit = &buffer[17];

    for (int32_t i = 0; i < 16; i++) {
        uint8_t nibble = value & 0xF;
        *digit-- = nibble >= 10 ? nibble + 'A' - 10 : nibble + '0';
        value >>= 4;
    }

    return buffer;
}

// VERIFY: b288c8ef auto
// OFFSET: 0x00756BD0
void UnitCombatClientInitialize() {
    ClientServices::SetMessageHandler(SMSG_ATTACK_START, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACK_STOP, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACKER_STATE_UPDATE, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACKSWING_NOTINRANGE, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACKSWING_BADFACING, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACKSWING_DEADTARGET, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ATTACKSWING_CANT_ATTACK, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_ENVIRONMENTAL_DAMAGE_LOG, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_CLEAR_TARGET, Packet_Group_29, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMBAT_EVENT_FAILED, Packet_Group_29, nullptr);
    s_entries.Clear();
    s_freeEntries.Clear();
    s_pendingEntries = nullptr;
    s_currentEntry = nullptr;
    s_filters.Clear();
    s_pendingData.Clear();
    s_freePendingData.Clear();
    s_invalidatedNames.Clear();
    s_startTime = static_cast<uint32_t>(time(nullptr));
    s_startTimeMs = FrameTime::s_curTimeMs;
    ClientServices::SetMessageHandler(SMSG_SPELL_PERIODIC_AURA_LOG, Packet_SMSG_PERIODICAURALOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_ENCHANTMENT_LOG, Packet_SMSG_ENCHANTMENTLOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_PARTY_KILL_LOG, Packet_SMSG_PARTYKILLLOG, nullptr);
    ClientServices::SetMessageHandler(SMSG_PROC_RESIST, Packet_SMSG_PROCRESIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_DISPEL_FAILED, Packet_SMSG_DISPEL_FAILED, nullptr);
    ClientServices::SetMessageHandler(SMSG_DESTRUCTIBLE_BUILDING_DAMAGE, Packet_SMSG_DESTRUCTIBLE_BUILDING_DAMAGE, nullptr);
    s_combatTextActiveUnit = WOWGUID{ 0 };
}

// VERIFY: 0526cc02 manual
// OFFSET: 0x00750270
void CombatLogEntry::CancelPendingCallbacks() {
    PendingCombatLogData* data = s_pendingData.Head();

    while (data) {
        if (data->m_entry != this) {
            data = s_pendingData.Next(data);
            continue;
        }

        switch (data->m_type) {
            case 1:
                g_nameDBCache.CancelCallback(data->m_guid.m_guid, &CombatLogEntry::PendingCombatLogResolved, data);
                break;
            case 2:
                g_petNameCache.CancelCallback(data->m_id, &CombatLogEntry::PendingCombatLogResolved, data);
                break;
            case 3:
                g_creatureDBCache.CancelCallback(data->m_id, &CombatLogEntry::PendingCombatLogResolved, data);
                break;
            case 4:
                g_gameObjectDBCache.CancelCallback(data->m_id, &CombatLogEntry::PendingCombatLogResolved, data);
                break;
            case 5:
                g_itemDBCache.CancelCallback(data->m_id, &CombatLogEntry::PendingCombatLogResolved, data);
                break;
        }

        data = s_pendingData.DeleteNode(data);
    }
}

// VERIFY: 65a90c44 manual
// OFFSET: 0x00750390
void RecycleCombatLogEntry(CombatLogEntry* entry) {
    if (s_pendingEntries == entry) {
        s_pendingEntries = s_entries.Next(entry);
    }

    if (s_currentEntry == entry) {
        s_currentEntry = s_entries.Next(entry);
    }

    entry->CancelPendingCallbacks();
    s_freeEntries.LinkToTail(entry);
}

// VERIFY: 40633cfb manual
// OFFSET: 0x0074F720
const char* CombatLogEntry::ResolveName(PendingCombatLogData* data, const char** realm, bool pending) {
    void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success) = pending ? &CombatLogEntry::PendingCombatLogResolved : nullptr;
    const char* name = nullptr;

    switch (data->m_type) {
        case 1: {
            NameCache* record = g_nameDBCache.GetRecord(data->m_guid.m_guid, data->m_guid, callback, data, false);

            if (record) {
                if (record->m_realmName[0] && realm) {
                    *realm = record->m_realmName;
                }

                name = record->m_name;
            }

            break;
        }
        case 2: {
            PetNameCache* record = g_petNameCache.GetRecord(data->m_id, data->m_guid, callback, data, false);

            if (!record) {
                break;
            }

            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(data->m_guid, TYPE_UNIT, __FILE__, __LINE__));

            if (unit ? record->m_timestamp == unit->m_unit->m_unitFieldPetNameTimestamp : !pending) {
                name = record->m_name;
                break;
            }

            UnitCombatLogInvalidateName(record->m_name);
            g_petNameCache.Invalidate(data->m_id);
            g_petNameCache.GetRecord(data->m_id, data->m_guid, callback, data, false);
            break;
        }
        case 3: {
            CreatureStats_C* stats = g_creatureDBCache.GetRecord(data->m_id, data->m_guid, callback, data, false);

            if (stats) {
                name = stats->GetName(0);
            }

            break;
        }
        case 4: {
            GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(data->m_id, data->m_guid, callback, data, false);

            if (stats) {
                name = stats->GetName(0);
            }

            break;
        }
        case 5: {
            ItemStats_C* stats = g_itemDBCache.GetRecord(data->m_id, data->m_guid, callback, data, false);

            if (stats) {
                name = stats->GetName(0);
            }

            break;
        }
    }

    if (name) {
        return name;
    }

    const char* text = FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);

    if (pending) {
        this->m_pendingMask |= data->m_mask;
        s_pendingData.LinkToTail(data);
    }

    return text;
}

// VERIFY: d93fe918 manual
// OFFSET: 0x0074FD40
const char* CombatLogEntry::GetObjectName(const WOWGUID& guid, uint32_t mask, const char** realm) {
    PendingCombatLogData* data = GetPendingCombatLogData(this, guid, mask);
    uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);

    if (!(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid))) {
        data->m_type = 1;
    } else {
        uint32_t type = high & 0xF0F00000;

        if (type == 0xF0300000 || type == 0xF0500000) {
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

            if (!unit) {
                data->m_type = 3;
                data->m_id = static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF;
            } else if (unit->m_unit->m_unitFieldPetnumber) {
                data->m_type = 2;
                data->m_id = unit->m_unit->m_unitFieldPetnumber;
            } else {
                data->m_id = unit->m_obj->m_entryID;
                data->m_type = 3;
            }
        } else if (type == 0xF0400000) {
            data->m_id = static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF;
            data->m_type = 2;
        } else if (type == 0xF0100000) {
            data->m_id = static_cast<int32_t>(guid.m_guid >> 24) & 0xFFFFFFF;
            data->m_type = 4;
        } else {
            return FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);
        }
    }

    return this->ResolveName(data, realm, true);
}

// VERIFY: 3635c9ab auto
// OFFSET: 0x0074FE90
void CombatLogEntry::PendingCombatLogResolved(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingCombatLogData* data = static_cast<PendingCombatLogData*>(param);
    CombatLogEntry* entry = data->m_entry;

    if (success) {
        const char* realm = nullptr;
        const char* name = entry->ResolveName(data, &realm, false);

        switch (data->m_mask) {
            case 1:
                entry->m_sourceName = name;
                entry->m_sourceRealm = realm;
                break;
            case 2:
                entry->m_destName = name;
                entry->m_destRealm = realm;
                break;
            case 4:
                entry->m_info.m_names[1] = name;
                break;
        }
    }

    entry->m_pendingMask &= ~data->m_mask;

    if (!entry->m_pendingMask) {
        CheckToSanitizeUnitLowGuid(&entry->m_sourceGuid);
        CheckToSanitizeUnitLowGuid(&entry->m_destGuid);

        if (s_pendingEntries == entry) {
            SendCombatLogEvents();
        }
    }

    s_freePendingData.LinkToTail(data);
}

// VERIFY: 96d6a275 manual
// OFFSET: 0x0074DCB0
uint32_t GetCombatLogFlags(const WOWGUID& guid) {
    if (!guid.m_guid) {
        return 0x80000000;
    }

    bool inRaid = CGRaidInfo::m_numMembers != 0;
    bool inGroup = inRaid || CGPartyInfo::GetNumPartyMembers();
    WOWGUID owner = guid;
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);
    CGUnit_C* unit = nullptr;

    if (object) {
        if (object->m_obj->m_type & TYPE_UNIT) {
            unit = static_cast<CGUnit_C*>(object);

            if (unit->GetCharmerOrCreatorGuid()->m_guid) {
                owner = *unit->GetCharmerOrCreatorGuid();
            }
        } else if (object->m_obj->m_type & TYPE_GAMEOBJECT) {
            owner = static_cast<CGGameObject_C*>(object)->m_gameObject->m_createdBy;
        }

        uint32_t ownerHigh = static_cast<uint32_t>(owner.m_guid >> 32);

        if (owner != guid && ((ownerHigh & 0xF0000000) || !((ownerHigh & 0xF07FFFFF) | static_cast<uint32_t>(owner.m_guid)))) {
            CGUnit_C* ownerUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(owner, TYPE_UNIT, __FILE__, __LINE__));

            if (ownerUnit && ownerUnit->GetCharmerOrCreatorGuid()->m_guid) {
                owner = *ownerUnit->GetCharmerOrCreatorGuid();
            }
        }
    }

    uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);
    bool isPlayer = !(high & 0xF0000000) && ((high & 0xF07FFFFF) | static_cast<uint32_t>(guid.m_guid));
    uint32_t type = high & 0xF0F00000;
    uint32_t flags;

    if (isPlayer || type == 0xF0300000 || type == 0xF0500000 || type == 0xF0400000) {
        if (owner == guid) {
            if (isPlayer) {
                flags = 0x400;
            } else {
                flags = type == 0xF0400000 ? 0x1000 : 0x800;
            }
        } else {
            const WOWGUID& controller = unit->m_unit->m_unitFieldCharmedby.m_guid ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldSummonedby;
            flags = controller.m_guid ? 0x1000 : 0x2000;
        }
    } else {
        flags = 0x4000;
    }

    uint32_t ownerHigh = static_cast<uint32_t>(owner.m_guid >> 32);

    if ((ownerHigh & 0xF0000000) || !((ownerHigh & 0xF07FFFFF) | static_cast<uint32_t>(owner.m_guid))) {
        flags |= 0x200;
    } else {
        flags |= 0x100;
    }

    if (owner == ClntObjMgrGetActivePlayer()) {
        flags |= 0x11;
    } else {
        bool hostile = false;

        if (unit) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                UNIT_REACTION reaction = unit->UnitReaction(player);

                if (reaction >= UNIT_REACTION_FRIENDLY) {
                    flags |= 0x10;
                } else if (reaction <= UNIT_REACTION_HOSTILE) {
                    flags |= 0x40;
                    hostile = true;
                }
            }
        }

        if (!hostile && inGroup) {
            if (CGPartyInfo::IsMember(owner)) {
                flags |= 0x12;
            } else if (inRaid && IsGuidInMyRaid(owner)) {
                flags |= 0x14;
            }
        }

        if (!(flags & 0xF)) {
            flags |= 0x8;
        }

        if (!(flags & 0xF0)) {
            flags |= 0x20;
        }
    }

    if (guid == CGGameUI::m_lockedTarget) {
        flags |= 0x10000;
    }

    if (guid == CGGameUI::m_focusTarget) {
        flags |= 0x20000;
    }

    if ((flags & 0x400) && ((flags & 0x6) || (owner == ClntObjMgrGetActivePlayer() && inGroup))) {
        uint8_t assignments = inRaid ? CGRaidInfo::GetPartyAssignments(guid) : CGPartyInfo::GetPartyAssignments(guid);

        if (assignments & 0x2) {
            flags |= 0x40000;
        } else if (assignments & 0x4) {
            flags |= 0x80000;
        }
    }

    if (inGroup) {
        uint32_t index = GetRaidTargetIndexFromGuid(guid);

        if (index < 8) {
            return (0x100000 << index) | flags;
        }
    }

    return flags;
}

// VERIFY: e1ed1d33 auto
// OFFSET: 0x00750400
CombatLogEntry* GetCombatLogEntry(const WOWGUID& source, const WOWGUID& dest, int32_t event, const SpellRec* spell) {
    CombatLogEntry* entry = s_freeEntries.Head();

    if (!entry) {
        CombatLogEntry* oldest = s_entries.Head();

        if (oldest && static_cast<int32_t>(FrameTime::s_curTimeMs - 1000 * g_combatLogRetentionTimeCVar->m_intValue - oldest->m_timestamp) >= 0) {
            RecycleCombatLogEntry(oldest);
            entry = oldest;
        } else {
            entry = s_freeEntries.NewNode(2, 0, 0);
        }
    }

    entry->Clear();
    entry->m_event = event;

    if (source.m_guid && (!spell || !(spell->m_attributesExD & 0x2000))) {
        entry->m_sourceGuid = source;
        entry->m_sourceName = entry->GetObjectName(source, 1, &entry->m_sourceRealm);
        entry->m_sourceFlags = GetCombatLogFlags(source);
    }

    if (dest.m_guid) {
        entry->m_destGuid = dest;
        entry->m_destName = entry->GetObjectName(dest, 2, &entry->m_destRealm);
        entry->m_destFlags = GetCombatLogFlags(dest);
    }

    if (spell) {
        entry->m_spellId = spell->m_id;
        entry->m_spellName.Copy(spell->m_name);
    }

    return entry;
}

// VERIFY: c707d13e manual
// OFFSET: 0x0074F6C0
bool CombatLogEntry::MatchesFilter() {
    CombatLogFilter* filter = s_filters.Head();

    if (!filter) {
        return true;
    }

    for (; filter; filter = s_filters.Next(filter)) {
        if (filter->MatchesFilter(this->m_event, this->m_sourceGuid, this->m_sourceFlags, this->m_destGuid, this->m_destFlags, this->m_spellId, this->m_spellName.m_str)) {
            return true;
        }
    }

    return false;
}

// VERIFY: c77149c7 manual
// OFFSET: 0x0074F910
void SendCombatLogEvents() {
    char text[4096];
    bool logging = CGChat::IsChatLogging(1) != 0;
    lua_State* L = FrameScript_GetContext();
    lua_pushstring(L, g_scriptEvents[566]);
    CombatLogEntry* entry = s_pendingEntries;

    while (true) {
        int32_t count = entry->PushEvent(L);

        if (s_pendingEntries->MatchesFilter()) {
            FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_LOG_EVENT, L, count + 1);
        }

        if (!FrameScript_GetEventListeners(g_scriptEvents[567])->Head()) {
            lua_settop(L, -1 - count);
        } else {
            lua_pushstring(L, g_scriptEvents[567]);
            lua_insert(L, -count - 1);
            FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_LOG_EVENT_UNFILTERED, L, count + 1);
            lua_settop(L, -count - 2);
        }

        if (logging) {
            s_pendingEntries->PackEvent(text, sizeof(text));
            CGChat::AddCombatMessage(text);
        }

        entry = s_entries.Next(s_pendingEntries);

        if (!entry) {
            break;
        }

        s_pendingEntries = entry;

        if (entry->m_pendingMask) {
            lua_settop(L, -2);
            return;
        }
    }

    s_pendingEntries = nullptr;
    lua_settop(L, -2);
}

// VERIFY: fbd407d2 auto
// OFFSET: 0x0074FF20
void QueueCombatLogEntry(CombatLogEntry* entry, uint32_t timeOffset) {
    entry->m_timestamp = FrameTime::s_curTimeMs - timeOffset;
    s_entries.LinkToTail(entry);

    if (s_pendingEntries) {
        return;
    }

    s_pendingEntries = entry;

    if (!entry->m_pendingMask) {
        CheckToSanitizeUnitLowGuid(&entry->m_sourceGuid);
        CheckToSanitizeUnitLowGuid(&entry->m_destGuid);
        SendCombatLogEvents();
    }
}

// VERIFY: ebb52f1a auto
// OFFSET: 0x00754FD0
bool ShouldShowFeedback(const ATTACKROUNDINFO& info) {
    return !info.m_meleeSpellId || info.m_victimState == VS_WOUND;
}

// VERIFY: 9b0ab217 manual
// OFFSET: 0x007512B0
void UnitCombatLog(const ATTACKROUNDINFO& info) {
    if (info.m_hitInfo & 0x800000) {
        CGUnit_C* attacker = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info.m_attacker, TYPE_UNIT, __FILE__, __LINE__));

        if (attacker) {
            attacker->ModifyPredictedPower(POWER_TYPE_RAGE, info.m_rageGained);
        }
    }

    if (!ShouldShowFeedback(info)) {
        return;
    }

    VICTIMSTATES victimState = info.m_victimState;

    if ((victimState == VS_WOUND || victimState == VS_BLOCK) && info.m_damage) {
        int32_t damage = info.m_damage;
        int32_t resisted = 0;
        int32_t absorbed = 0;

        for (uint32_t i = 0; i < 2 && info.m_schoolMask[i]; i++) {
            resisted += info.m_resisted[i];
            absorbed += info.m_absorbed[i];
        }

        WOWGUID target = info.m_target;
        bool crushing = (info.m_hitInfo & 0x20000) != 0;
        bool glancing = (info.m_hitInfo >> 16) & 1;
        bool critical = (info.m_hitInfo & 0x200) != 0;
        int32_t blocked = info.m_blocked;
        uint32_t school = info.m_schoolMask[0];
        int32_t overkill = info.m_overkill;
        CombatLogEntry* entry = GetCombatLogEntry(info.m_attacker, target, 1, nullptr);
        entry->m_infoFlags |= 0x10;
        entry->m_info.m_values[2] = overkill;
        entry->m_info.m_values[4] = absorbed;
        entry->m_info.m_values[3] = school;
        entry->m_info.m_values[6] = blocked;
        entry->m_info.m_values[1] = damage;
        entry->m_info.m_values[5] = resisted;
        entry->m_infoBits = (entry->m_infoBits & ~0x7) | critical | ((glancing | (crushing << 1)) << 1);
        QueueCombatLogEntry(entry, 0);

        if (s_combatTextActiveUnit == target) {
            if (resisted > 0) {
                FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%d", s_combatTextTypes[10], damage, resisted);
            } else if (blocked) {
                SendToCombatText(11, damage, blocked);
            } else if (absorbed) {
                SendToCombatText(12, damage, absorbed);
            } else if (critical) {
                SendToCombatText(1, damage);
            } else {
                SendToCombatText(2, damage);
            }
        }

        return;
    }

    WOWGUID target = info.m_target;
    CombatLogEntry* entry;

    if (info.m_hitInfo & 0x10) {
        entry = GetCombatLogEntry(info.m_attacker, target, 2, nullptr);
        entry->SetMissInfo(1, 0);

        if (s_combatTextActiveUnit == target) {
            SendToCombatText(3);
        }
    } else if (info.m_hitInfo & 0x20) {
        int32_t absorbed = 0;

        for (uint32_t i = 0; i < 2 && info.m_schoolMask[i]; i++) {
            absorbed += info.m_absorbed[i];
        }

        entry = GetCombatLogEntry(info.m_attacker, target, 2, nullptr);
        entry->SetMissInfo(10, absorbed);

        if (s_combatTextActiveUnit == target) {
            SendToCombatText(12);
        }
    } else if (info.m_hitInfo & 0x80) {
        int32_t resisted = 0;

        for (uint32_t i = 0; i < 2 && info.m_schoolMask[i]; i++) {
            resisted += info.m_resisted[i];
        }

        entry = GetCombatLogEntry(info.m_attacker, target, 2, nullptr);
        entry->SetMissInfo(2, resisted);

        if (s_combatTextActiveUnit == target) {
            SendToCombatText(10);
        }
    } else {
        int32_t missType;
        int32_t textType;

        switch (victimState) {
            case VS_DODGE:
                missType = 3;
                textType = 4;
                break;
            case VS_PARRY:
                missType = 4;
                textType = 5;
                break;
            case VS_BLOCK:
                missType = 5;
                textType = 11;
                break;
            case VS_EVADE:
                missType = 6;
                textType = 6;
                break;
            case VS_IMMUNE:
                missType = 7;
                textType = 7;
                break;
            case VS_DEFLECT:
                missType = 9;
                textType = 8;
                break;
            default:
                return;
        }

        entry = GetCombatLogEntry(info.m_attacker, target, 2, nullptr);
        entry->SetMissInfo(missType, info.m_blocked);

        if (s_combatTextActiveUnit == target) {
            SendToCombatText(textType);
        }
    }

    QueueCombatLogEntry(entry, 0);
}

// VERIFY: a0eac561 manual
// OFFSET: 0x00750C90
int32_t Packet_SMSG_PARTYKILLLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID killer = WOWGUID{ value };
    value = 0;
    msg->Get(value);
    WOWGUID victim = WOWGUID{ value };
    CombatLogEntry* entry = GetCombatLogEntry(killer, victim, 43, nullptr);
    QueueCombatLogEntry(entry, 0);
    return 1;
}

// VERIFY: 7cbd4790 manual
// OFFSET: 0x00750D40
int32_t Packet_SMSG_PROCRESIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID caster = WOWGUID{ value };
    value = 0;
    msg->Get(value);
    WOWGUID target = WOWGUID{ value };
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint8_t debug = 0;
    msg->Get(debug);
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(caster, target, 8, &spell);
        entry->m_infoFlags |= 0x4004;
        entry->m_info.m_values[0] = 2;
        entry->m_info.m_values[5] = 0;

        if (s_combatTextActiveUnit == target) {
            SendToCombatText(22);
        }

        QueueCombatLogEntry(entry, 0);
    }

    return 1;
}

// VERIFY: 46a3d6bd auto
// OFFSET: 0x00750EA0
int32_t Packet_SMSG_DISPEL_FAILED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID caster = WOWGUID{ value };
    value = 0;
    msg->Get(value);
    WOWGUID target = WOWGUID{ value };
    uint32_t spellId = 0;
    msg->Get(spellId);
    SpellRec spell;
    SpellRec failed;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        while (!msg->IsRead()) {
            uint32_t failedId = 0;
            msg->Get(failedId);

            if (g_spellDB.GetRecord(failedId, &failed) && !(failed.m_attributes & 0x180) && *failed.m_name) {
                CombatLogEntry* entry = GetCombatLogEntry(caster, target, 39, &spell);
                entry->m_infoFlags |= 0x100;
                entry->m_info.m_values[0] = failedId;
                QueueCombatLogEntry(entry, 0);
            }
        }
    }

    return 1;
}

// VERIFY: dc01d408 auto
// OFFSET: 0x00752210
void UnitCombatLogInstakill(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(casterGuid, targetGuid, 14, &spell);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 80058a0f auto
// OFFSET: 0x00752860
void UnitCombatLogAuraDose(CGUnit_C* unit, const AuraData* aura, int32_t spellId, int32_t oldStack, int32_t newStack, uint8_t a6) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name && spell.m_cumulativeAura > 1) {
        WOWGUID guid = unit->m_obj->m_guid;
        CombatLogEntry* entry = GetCombatLogEntry(aura->m_creator, guid, (newStack <= oldStack) + 22, &spell);
        entry->m_infoFlags |= 0xC00;
        entry->m_info.m_values[2] = newStack;
        reinterpret_cast<uint8_t&>(entry->m_info.m_values[1]) = a6;
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 7035a324 manual
// OFFSET: 0x00752C90
void UnitCombatLogDamageShield(const WOWGUID& victimGuid, const WOWGUID& casterGuid, int32_t spellId, int32_t damage, uint32_t overkill, uint32_t schoolMask) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(casterGuid, TYPE_UNIT, __FILE__, __LINE__));

        if (caster) {
            caster->ModifyPredictedHealth(-damage);
        }

        CombatLogEntry* entry = GetCombatLogEntry(victimGuid, casterGuid, 40, &spell);
        entry->m_infoFlags |= 0x10;
        entry->m_infoBits &= ~0x7;
        entry->m_info.m_values[1] = damage;
        entry->m_info.m_values[2] = overkill;
        entry->m_info.m_values[3] = schoolMask;
        entry->m_info.m_values[5] = 0;
        entry->m_info.m_values[6] = 0;
        entry->m_info.m_values[4] = 0;
        QueueCombatLogEntry(entry, 0);

        if (s_combatTextActiveUnit == casterGuid) {
            SendToCombatText(39, damage);
        }
    }
}

// VERIFY: 40adbf60 auto
// OFFSET: 0x0074D120
WOWGUID StringToGuid(const char* string) {
    uint64_t value = 0;

    if (!SStrCmpI(string, "0x", 2)) {
        string += 2;
    }

    for (int32_t i = 16; i; i--) {
        char c = *string++;
        uint32_t digit;

        if (static_cast<uint8_t>(c - '0') <= 9) {
            digit = c - '0';
        } else if (static_cast<uint32_t>(c - 'a') <= 5) {
            digit = c - 'a' + 10;
        } else if (static_cast<uint32_t>(c - 'A') <= 5) {
            digit = c - 'A' + 10;
        } else {
            break;
        }

        value = (value << 4) | digit;
    }

    return WOWGUID{ value };
}

// VERIFY: 7da56105 manual
// OFFSET: 0x0074E290
int32_t CombatLogEntry::PushEvent(lua_State* L) {
    char name[305];
    char guid[20];
    double timestamp = (this->m_timestamp - s_startTimeMs) * 0.001 + static_cast<int32_t>(s_startTime);
    lua_checkstack(L, 8);
    lua_pushnumber(L, timestamp);
    lua_pushstring(L, s_combatLogEventNames[this->m_event]);
    lua_pushstring(L, GUIDToHexString(this->m_sourceGuid, guid));

    if (!this->m_sourceName) {
        lua_pushnil(L);
    } else if (this->m_sourceRealm) {
        SStrPrintf(name, sizeof(name), "%s-%s", this->m_sourceName, this->m_sourceRealm);
        lua_pushstring(L, name);
    } else if (*this->m_sourceName) {
        lua_pushstring(L, this->m_sourceName);
    } else {
        lua_pushnil(L);
    }

    lua_pushinteger(L, this->m_sourceFlags);
    lua_pushstring(L, GUIDToHexString(this->m_destGuid, guid));

    if (!this->m_destName) {
        lua_pushnil(L);
    } else if (this->m_destRealm) {
        SStrPrintf(name, sizeof(name), "%s-%s", this->m_destName, this->m_destRealm);
        lua_pushstring(L, name);
    } else if (*this->m_destName) {
        lua_pushstring(L, this->m_destName);
    } else {
        lua_pushnil(L);
    }

    lua_pushinteger(L, this->m_destFlags);
    int32_t count = 8;

    if (this->m_spellId) {
        lua_checkstack(L, 3);
        lua_pushinteger(L, this->m_spellId);
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_spellId, &spell)) {
            lua_pushstring(L, spell.m_name);
            lua_pushinteger(L, spell.m_schoolMask);
        } else {
            lua_pushnil(L);
            lua_pushinteger(L, 0);
        }

        count = 11;
    }

    if (this->m_infoFlags & 0x1) {
        lua_checkstack(L, 1);
        lua_pushinteger(L, this->m_info.m_values[0]);
        count++;
    }

    if (this->m_infoFlags & 0x2) {
        lua_checkstack(L, 1);
        lua_pushstring(L, this->m_info.m_text);
        count++;
    }

    if (this->m_infoFlags & 0x4) {
        lua_checkstack(L, 1);
        lua_pushstring(L, g_spellMissStrings[this->m_info.m_values[0]]);
        count++;
    }

    if (this->m_infoFlags & 0x8) {
        lua_checkstack(L, 1);
        lua_pushstring(L, s_environmentalDamageTypes[this->m_info.m_values[0]]);
        count++;
    }

    if (this->m_infoFlags & 0x10) {
        lua_checkstack(L, 9);
        lua_pushnumber(L, this->m_info.m_values[1]);
        lua_pushnumber(L, this->m_info.m_values[2]);
        lua_pushinteger(L, this->m_info.m_values[3]);

        if (this->m_info.m_values[5]) {
            lua_pushnumber(L, this->m_info.m_values[5]);
        } else {
            lua_pushnil(L);
        }

        if (this->m_info.m_values[6]) {
            lua_pushnumber(L, this->m_info.m_values[6]);
        } else {
            lua_pushnil(L);
        }

        if (this->m_info.m_values[4]) {
            lua_pushnumber(L, this->m_info.m_values[4]);
        } else {
            lua_pushnil(L);
        }

        if (this->m_infoBits & 0x1) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        if (this->m_infoBits & 0x2) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        if (this->m_infoBits & 0x4) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        count += 9;
    }

    if (this->m_infoFlags & 0x2000) {
        lua_checkstack(L, 1);
        lua_pushinteger(L, this->m_info.m_values[4]);
        count++;
    }

    if (this->m_infoFlags & 0x4000) {
        lua_checkstack(L, 1);
        lua_pushinteger(L, this->m_info.m_values[5]);
        count++;
    }

    if (this->m_infoFlags & 0x8000) {
        lua_checkstack(L, 1);
        lua_pushinteger(L, this->m_info.m_values[6]);
        count++;
    }

    if (this->m_infoFlags & 0x20) {
        lua_checkstack(L, 4);
        lua_pushnumber(L, this->m_info.m_values[0]);
        lua_pushnumber(L, this->m_info.m_values[1]);
        lua_pushnumber(L, this->m_info.m_values[2]);

        if (static_cast<uint8_t>(this->m_info.m_values[3])) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        count += 4;
    }

    if (this->m_infoFlags & 0x40) {
        lua_checkstack(L, 2);
        lua_pushnumber(L, this->m_info.m_values[0]);
        lua_pushnumber(L, this->m_info.m_values[1]);
        count += 2;
    }

    if (this->m_infoFlags & 0x80) {
        lua_checkstack(L, 3);
        lua_pushnumber(L, this->m_info.m_values[0]);
        lua_pushnumber(L, this->m_info.m_values[1]);

        if (this->m_info.m_values[2]) {
            lua_pushnumber(L, this->m_info.m_values[2]);
        } else {
            lua_pushnil(L);
        }

        count += 3;
    }

    if (this->m_infoFlags & 0x100) {
        lua_checkstack(L, 2);
        lua_pushinteger(L, this->m_info.m_values[0]);
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_info.m_values[0], &spell)) {
            lua_pushstring(L, spell.m_name);
            lua_pushinteger(L, spell.m_schoolMask);
        } else {
            lua_pushnil(L);
            lua_pushnumber(L, 0.0);
        }

        count += 3;
    }

    if (this->m_infoFlags & 0x1000) {
        lua_checkstack(L, 1);
        lua_pushstring(L, this->m_info.m_names[2]);
        count++;
    }

    if (this->m_infoFlags & 0x200) {
        lua_checkstack(L, 2);
        lua_pushinteger(L, this->m_info.m_values[0]);
        lua_pushstring(L, this->m_info.m_names[1]);
        count += 2;
    }

    if (this->m_infoFlags & 0x400) {
        lua_checkstack(L, 1);
        lua_pushstring(L, static_cast<uint8_t>(this->m_info.m_values[1]) ? "DEBUFF" : "BUFF");
        count++;
    }

    if (this->m_infoFlags & 0x800) {
        lua_checkstack(L, 1);
        lua_pushinteger(L, this->m_info.m_values[2]);
        count++;
    }

    return count;
}

// VERIFY: c56cea32 auto
// OFFSET: 0x00750210
void CombatLogEntry::SetExecuteItem(int32_t itemId) {
    this->m_infoFlags |= 0x200;
    this->m_info.m_values[0] = itemId;
    PendingCombatLogData* data = GetPendingCombatLogData(this, WOWGUID{ 0 }, 4);
    data->m_id = itemId;
    data->m_type = 5;
    this->m_info.m_names[1] = this->ResolveName(data, nullptr, true);
}

// VERIFY: ed94944b manual
// OFFSET: 0x00750500
void CombatLog::QueueMissedSpellText(const WOWGUID& source, const WOWGUID& dest, int32_t event, const SpellRec* spell, int32_t missType, int32_t amount, uint32_t timeOffset) {
    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, spell);
    entry->SetMissInfo(missType, amount);

    if (s_combatTextActiveUnit == dest) {
        switch (missType) {
            case 1:
                SendToCombatText(event == 8 ? 15 : 3);
                break;

            case 2:
                SendToCombatText(event == 8 ? 22 : 10);
                break;

            case 3:
                SendToCombatText(event == 8 ? 16 : 4);
                break;

            case 4:
                SendToCombatText(event == 8 ? 17 : 5);
                break;

            case 5:
                SendToCombatText(event == 8 ? 23 : 11);
                break;

            case 6:
                SendToCombatText(event == 8 ? 18 : 6);
                break;

            case 7:
            case 8:
                SendToCombatText(event == 8 ? 19 : 7);
                break;

            case 9:
                SendToCombatText(event == 8 ? 20 : 8);
                break;

            case 10:
                SendToCombatText(event == 8 ? 24 : 12);
                break;

            case 11:
                SendToCombatText(event == 8 ? 21 : 9);
                break;

            default:
                break;
        }
    }

    QueueCombatLogEntry(entry, timeOffset);
}

// VERIFY: 66f75cea auto
// OFFSET: 0x00751050
int32_t Packet_SMSG_ENCHANTMENTLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    uint32_t itemId;
    msg->Get(itemId);
    uint32_t enchantId;
    msg->Get(enchantId);

    SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchantId);

    if (enchant && !(enchant->m_flags & 0x2)) {
        CombatLogEntry* entry = GetCombatLogEntry(caster, target, caster.m_guid ? 31 : 32, nullptr);
        entry->m_infoFlags |= 0x1000;
        entry->m_info.m_names[2] = enchant->m_name;
        entry->SetExecuteItem(itemId);
        QueueCombatLogEntry(entry, 0);
    }

    return 1;
}

// VERIFY: c50eda39 auto
// OFFSET: 0x00751AD0
void UnitCombatLogSpellFail(const WOWGUID& caster, int32_t spellId, const char* reason) {
    if (!reason || !*reason) {
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        WOWGUID dest = { 0 };
        CombatLogEntry* entry = GetCombatLogEntry(caster, dest, 7, &spell);
        entry->SetFailReason(reason);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 94b0219d auto
// OFFSET: 0x00751B80
void UnitCombatLogSpellMissed(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t missType, bool a5, bool a6, int32_t a7) {
    if (!missType) {
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        int32_t event;

        if (a6) {
            event = 33;
        } else if (a5) {
            event = 41;
        } else {
            event = (spell.m_attributesExC & 0x8000) ? 4 : 8;
        }

        CombatLog::QueueMissedSpellText(casterGuid, targetGuid, event, &spell, missType, 0, a7);
    }
}

// VERIFY: d5382c97 manual
// OFFSET: 0x007522A0
void CombatLog::AddSpellCastEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        int32_t event = (static_cast<uint32_t>(dest.m_guid >> 32) & 0xF0F00000) == 0xF0100000 ? 16 : 15;
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, &spell);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 5431f565 auto
// OFFSET: 0x00752340
void CombatLog::AddSpellResurrectEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, 46, &spell);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 397ad275 auto
// OFFSET: 0x007523D0
void UnitCombatLogInterrupt(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t interruptedSpellId) {
    SpellRec spell;
    SpellRec interrupted;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name && g_spellDB.GetRecord(interruptedSpellId, &interrupted) && !(interrupted.m_attributes & 0x180) && *interrupted.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(casterGuid, targetGuid, 17, &spell);
        entry->m_infoFlags |= 0x100;
        entry->m_info.m_values[0] = interruptedSpellId;
        QueueCombatLogEntry(entry, 0);

        if (IsCombatTextActiveUnit(targetGuid)) {
            SendToCombatText(0, interrupted.m_name);
        }

        SpellMechanicRec* mechanic = g_spellMechanicDB.GetRecord(26);

        if (mechanic) {
            DisplayMechanicEffect(&targetGuid, &casterGuid, &spell, mechanic);
        }
    }
}

// VERIFY: 130013bd auto
// OFFSET: 0x00752520
void CombatLog::AddSpellDamageEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, uint32_t timeOffset) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name && damage) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, 18, &spell);
        entry->m_infoFlags |= 0x1;
        entry->m_info.m_values[0] = damage;
        QueueCombatLogEntry(entry, timeOffset);

        if (s_combatTextActiveUnit == dest) {
            SendToCombatText(41, damage);
        }
    }
}

// VERIFY: 5dd1e7d2 auto
// OFFSET: 0x007525E0
void CombatLog::AddSpellExecuteEntry(const WOWGUID& source, const WOWGUID& dest, int32_t itemId, int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, 19, &spell);
        entry->SetExecuteItem(itemId);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 52db4556 auto
// OFFSET: 0x00752680
void CombatLog::AddSpellEnergizeEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, 20, &spell);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 8c0f6497 manual
// OFFSET: 0x006D3220
int32_t CombatLog::HandleSpellLogExecute(CDataStore* msg, int32_t timeOffset) {
    SmartGUID caster;
    *msg >> caster;
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t effectCount = 0;
    msg->Get(effectCount);

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        msg->m_read = msg->m_size;
        return 1;
    }

    for (int32_t i = effectCount; i > 0; i--) {
        uint32_t effect = 0;
        msg->Get(effect);
        uint32_t targetCount = 0;
        msg->Get(targetCount);

        for (int32_t j = targetCount; j > 0; j--) {
            SmartGUID target;

            switch (effect) {
                case 8:
                case 62: {
                    *msg >> target;
                    uint32_t amount = 0;
                    msg->Get(amount);
                    uint32_t powerType = 0;
                    msg->Get(powerType);
                    float multiplier = 0.0f;
                    msg->Get(multiplier);
                    UnitCombatLogPowerDrain(caster, target, spellId, powerType, amount, multiplier, (spell.m_attributesExC & 0x2000000) != 0, timeOffset);
                    break;
                }

                case 18:
                case 113:
                    *msg >> target;
                    CombatLog::AddSpellResurrectEntry(caster, target, spellId);
                    break;

                case 19: {
                    *msg >> target;
                    uint32_t damage = 0;
                    msg->Get(damage);
                    CombatLog::AddSpellDamageEntry(caster, target, spellId, damage, timeOffset);
                    break;
                }

                case 24:
                case 59:
                case 157: {
                    uint32_t itemId = 0;
                    msg->Get(itemId);
                    CGChat::LogTradeSkillCreate(caster, itemId);
                    break;
                }

                case 28:
                case 50:
                case 56:
                case 76:
                case 81:
                case 83:
                case 104:
                case 105:
                case 106:
                case 107:
                    *msg >> target;
                    CombatLog::AddSpellCastEntry(caster, target, spellId);
                    break;

                case 33:
                    *msg >> target;
                    CombatLog::GetSpellLogLinkInfo(caster, target, spellId);
                    break;

                case 68: {
                    *msg >> target;
                    uint32_t interrupted = 0;
                    msg->Get(interrupted);
                    UnitCombatLogInterrupt(caster, target, spellId, interrupted);
                    break;
                }

                case 101: {
                    uint32_t itemId = 0;
                    msg->Get(itemId);
                    CGChat::LogFeedPet(caster, itemId);
                    break;
                }

                case 102:
                    *msg >> target;
                    CGChat::LogDismissPet(caster, target);
                    break;

                case 111: {
                    *msg >> target;
                    uint32_t itemId = 0;
                    msg->Get(itemId);
                    uint32_t extra = 0;
                    msg->Get(extra);

                    if (itemId == 0xFFFFFFFF && extra == 0xFFFFFFFF) {
                        CombatLog::AddSpellEnergizeEntry(caster, target, spellId);
                    } else {
                        CombatLog::AddSpellExecuteEntry(caster, target, itemId, spellId);
                    }

                    break;
                }

                default:
                    break;
            }
        }
    }

    return 1;
}

// VERIFY: ae480a8e manual
// OFFSET: 0x00751F70
void UnitCombatLogHeal(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t amount, int32_t overheal, int32_t absorbed, bool crit, bool periodic, uint32_t timeOffset) {
    if (!amount && !overheal && !absorbed) {
        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (target) {
        target->ModifyPredictedHealth(amount);
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        SendHealEvent(periodic ? 35 : 10, casterGuid, targetGuid, &spell, amount, overheal, absorbed, crit, timeOffset);
    }
}

// VERIFY: 9de2c0e0 manual
// OFFSET: 0x00752040
void UnitCombatLogEnergize(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t powerType, int32_t amount, bool periodic, uint32_t timeOffset) {
    if (!amount) {
        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (target) {
        target->ModifyPredictedPower(static_cast<POWER_TYPE>(powerType), amount);
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        SendEnergizeEvent(periodic ? 36 : 11, casterGuid, targetGuid, &spell, powerType, amount, timeOffset);
    }
}

// VERIFY: 013ab8d4 manual
// OFFSET: 0x00752710
void UnitCombatLogAuraAddedOrRemoved(CGUnit_C* unit, AuraData* aura, const SpellRec* spell, uint8_t harmful, int32_t added) {
    if (unit->m_unit->m_unitFieldHealth <= 0 && !added) {
        return;
    }

    if (!spell || !spell->m_id || (spell->m_attributes & 0x180) || !*spell->m_name || (spell->m_attributes & 0x40) || (spell->m_attributesExF & 0x400)) {
        return;
    }

    WOWGUID guid = unit->m_obj->m_guid;
    CombatLogEntry* entry = GetCombatLogEntry(aura->m_creator, guid, added ? 21 : 24, spell);
    entry->m_infoFlags |= 0x400;
    reinterpret_cast<uint8_t&>(entry->m_info.m_values[1]) = harmful;
    QueueCombatLogEntry(entry, 0);

    if (added) {
        WOWGUID unitGuid = unit->m_obj->m_guid;
        UnitCombatLogSpellMechanic(unitGuid, aura, spell);
    }

    if (!IsCombatTextActiveUnit(unit->m_obj->m_guid)) {
        return;
    }

    if (added) {
        if (harmful) {
            SendToCombatText(34, spell->m_name);
        } else if (!(spell->m_attributesExF & 0x40)) {
            SendToCombatText(33, spell->m_name);
        }
    } else {
        SendToCombatText(harmful ? 32 : 31, spell->m_name);
    }
}

// VERIFY: db86f760 manual
// OFFSET: 0x006D3090
int32_t CombatLog::HandleSpellLogMiss(CDataStore* msg, int32_t timeOffset) {
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint64_t value = 0;
    msg->Get(value);
    WOWGUID caster = WOWGUID{ value };
    uint8_t debug = 0;
    msg->Get(debug);
    bool hasDebug = debug != 0;
    uint32_t count = 0;
    msg->Get(count);

    SpellRec spell;
    int32_t found = g_spellDB.GetRecord(spellId, &spell);

    for (; count; count--) {
        uint64_t targetValue = 0;
        msg->Get(targetValue);
        WOWGUID target = WOWGUID{ targetValue };
        uint8_t missType = 0;
        msg->Get(missType);

        if (hasDebug) {
            float a = 0.0f;
            msg->Get(a);
            float b = 0.0f;
            msg->Get(b);
        } else if (found && !(spell.m_attributes & 0x100)) {
            CGGameUI::ShowSpellMissFeedback(target, missType);
            UnitCombatLogSpellMissed(caster, target, spellId, missType, true, false, timeOffset);
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

            if (unit) {
                unit->AddWorldText(caster, static_cast<MISS_REASON>(missType), &spell, false);
            }
        }
    }

    return 1;
}

// VERIFY: b28f2a2e manual
// OFFSET: 0x006D3C30
int32_t CombatLog::HandleSpellHealLog(CDataStore* msg, int32_t timeOffset) {
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t amount = 0;
    msg->Get(amount);
    uint32_t overheal = 0;
    msg->Get(overheal);
    uint32_t absorbed = 0;
    msg->Get(absorbed);
    uint8_t critByte = 0;
    msg->Get(critByte);
    bool crit = critByte != 0;
    uint8_t unused = 0;
    msg->Get(unused);

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 1;
    }

    if (!(spell.m_attributes & 0x100)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

        if (unit && !amount && absorbed) {
            WOWGUID source = caster;
            unit->AddWorldText(source, MISS_ABSORBED, &spell, true);
        }

        WOWGUID dest = target;
        CGGameUI::ShowHealingFeedback(dest, amount, crit);
    }

    UnitCombatLogHeal(caster, target, spellId, amount, overheal, absorbed, crit, (spell.m_attributesExC & 0x2000000) != 0, timeOffset);
    return 1;
}

// VERIFY: c6dd3152 manual
// OFFSET: 0x006D3DF0
int32_t CombatLog::HandleSpellLogEnergizeLog(CDataStore* msg, int32_t timeOffset) {
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t powerType = 0;
    msg->Get(powerType);
    uint32_t amount = 0;
    msg->Get(amount);

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell)) {
        if (!(spell.m_attributes & 0x100)) {
            CGGameUI::ShowEnergizeFeedback(target, powerType, amount);
        }

        UnitCombatLogEnergize(caster, target, spellId, powerType, amount, (spell.m_attributesExC & 0x2000000) != 0, timeOffset);
    }

    return 1;
}

// VERIFY: 4b332cb3 manual
// OFFSET: 0x00750860
void SendHealEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t amount, int32_t overheal, int32_t absorbed, bool crit, uint32_t timeOffset) {
    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, spell);
    entry->m_infoFlags |= 0x20;
    entry->m_info.m_values[1] = overheal;
    entry->m_info.m_values[0] = amount;
    entry->m_info.m_values[2] = absorbed;
    reinterpret_cast<uint8_t&>(entry->m_info.m_values[3]) = crit;
    QueueCombatLogEntry(entry, timeOffset);

    if (event == 35) {
        SpellDurationRec* duration = g_spellDurationDB.GetRecord(spell->m_durationIndex);

        if (duration) {
            int32_t value = duration->m_duration + duration->m_durationPerLevel * static_cast<int32_t>(Spell_C_GetSpellLevel(spell, 0, 0)) >= duration->m_maxDuration ? duration->m_maxDuration : duration->m_duration + duration->m_durationPerLevel * static_cast<int32_t>(Spell_C_GetSpellLevel(spell, 0, 0));

            if (value > 30000) {
                return;
            }
        }
    }

    if (s_combatTextActiveUnit == dest) {
        if (absorbed) {
            if (event == 35) {
                FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s%d%d", s_combatTextTypes[44], GetObjectNameFromGuid(source), amount, absorbed);
            } else {
                SendToCombatText(crit ? 45 : 46, GetObjectNameFromGuid(source), amount, absorbed);
            }
        } else if (event == 35) {
            FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%s%d", s_combatTextTypes[27], GetObjectNameFromGuid(source), amount);
        } else {
            SendToCombatText(crit ? 37 : 38, GetObjectNameFromGuid(source), amount);
        }
    } else {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(dest, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->AddWorldHealingText(source, amount, spell, crit, 0);
        }
    }
}

// VERIFY: f15c546c manual
// OFFSET: 0x00750A90
void SendEnergizeEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t powerType, int32_t amount, uint32_t timeOffset) {
    int32_t value = amount / Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(powerType));
    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, spell);
    entry->m_infoFlags |= 0x40;
    entry->m_info.m_values[0] = value;
    entry->m_info.m_values[1] = powerType;
    QueueCombatLogEntry(entry, timeOffset);

    if (s_combatTextActiveUnit == dest) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%s", s_combatTextTypes[(event == 36) + 28], value, GetPowerString(static_cast<POWER_TYPE>(powerType)));
    }
}

// VERIFY: a94001c7 manual
// OFFSET: 0x00750B60
void AddPowerDrainEntry(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t amount, int32_t powerType, int32_t extra, uint32_t timeOffset) {
    uint32_t mod = Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(powerType));
    int32_t value = amount / mod;
    int32_t extraValue = extra / mod;
    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, spell);
    entry->m_infoFlags |= 0x80;
    entry->m_info.m_values[2] = extraValue;
    entry->m_info.m_values[0] = value;
    entry->m_info.m_values[1] = powerType;
    QueueCombatLogEntry(entry, timeOffset);

    if (s_combatTextActiveUnit == dest && value) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%s", s_combatTextTypes[28], -value, GetPowerString(static_cast<POWER_TYPE>(powerType)));
    }

    if (s_combatTextActiveUnit == source && extraValue) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%s", s_combatTextTypes[(event == 38) + 28], extraValue, GetPowerString(static_cast<POWER_TYPE>(powerType)));
    }
}

// VERIFY: d355aaa8 manual
// OFFSET: 0x00752100
void UnitCombatLogPowerDrain(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, uint32_t powerType, uint32_t amount, float multiplier, bool a7, uint32_t timeOffset) {
    if (!amount) {
        return;
    }

    if (powerType == POWER_TYPE_HAPPINESS) {
        CGChat::LogPetHappinessDecrease(casterGuid, targetGuid, amount / Spell_C_GetPowerDisplayMod(POWER_TYPE_HAPPINESS));
        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (target) {
        target->ModifyPredictedPower(static_cast<POWER_TYPE>(powerType), -static_cast<int32_t>(amount));
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x180) && *spell.m_name) {
        int32_t extra = CMath::ftrunc(amount * multiplier);
        int32_t event = extra ? (a7 ? 38 : 13) : (a7 ? 37 : 12);
        AddPowerDrainEntry(event, casterGuid, targetGuid, &spell, amount, powerType, extra, timeOffset);
    }
}

// VERIFY: 048b1785 auto
// OFFSET: 0x0074D3D0
bool SpellHasOpenLockEffect(const SpellRec* spell) {
    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effect[i] == SPELL_EFFECT_OPEN_LOCK) {
            return true;
        }
    }

    return false;
}

// VERIFY: f32a38df auto
// OFFSET: 0x0074D3F0
bool SpellIsHiddenInCombatLog(const SpellRec* spell) {
    if ((spell->m_attributes & 0x180) || !*spell->m_name) {
        return true;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effect[i] == SPELL_EFFECT_LEARN_SPELL) {
            return true;
        }
    }

    return (spell->m_attributesExD & 0x1) != 0;
}

// VERIFY: 6e0bb93c auto
// OFFSET: 0x0074DA00
static bool ShouldShowMechanicText(WOWGUID guid, const SpellRec* spell) {
    if (!(spell->m_attributesExF & 0x200000)) {
        return false;
    }

    CVar* all = CVar::Lookup("fctAllSpellMechanics");

    if (all && all->m_intValue > 0) {
        return true;
    }

    CVar* mechanics = CVar::Lookup("fctSpellMechanics");

    if (!mechanics || !mechanics->m_intValue) {
        return false;
    }

    CVar* other = CVar::Lookup("fctSpellMechanicsOther");

    if (IsCombatTextActiveUnit(guid)) {
        return true;
    }

    return other && other->m_intValue;
}

// VERIFY: 9c33925c manual
// OFFSET: 0x0074DA70
void DisplayMechanicEffect(const WOWGUID* unitGuid, const WOWGUID* sourceGuid, const SpellRec* spell, const SpellMechanicRec* mechanic) {
    if (!ShouldShowMechanicText(*sourceGuid, spell)) {
        return;
    }

    char text[148];
    uint16_t wide[64];
    char state[64];
    SUniConvertUTF8to16(wide, 64, mechanic->m_stateName, STORM_MAX_STR, nullptr, nullptr);
    wide[0] = ToUpper(wide[0]);
    SUniConvertUTF16to8(state, 64, wide, STORM_MAX_STR, nullptr, nullptr);

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*unitGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (s_combatTextActiveUnit == *sourceGuid) {
        SStrCopy(text, state, STORM_MAX_STR);
    } else {
        SStrPrintf(text, sizeof(text), "%s (%s)", state, GetObjectNameFromGuid(*sourceGuid));
    }

    if (unit) {
        PlayerNameCreateText(unit->m_nameDesc, WT_DAMAGE, text, reinterpret_cast<const CImVector*>(&s_mechanicTextColor), nullptr);
    }
}

// VERIFY: 0a6910f9 manual
// OFFSET: 0x0074DB80
void UnitCombatLogSpellMechanic(const WOWGUID& guid, const AuraData* aura, const SpellRec* spell) {
    if (SpellIsHiddenInCombatLog(spell)) {
        return;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effect[i] == SPELL_EFFECT_OPEN_LOCK) {
            return;
        }
    }

    for (int32_t i = 0; i < 3; i++) {
        int32_t mechanicId = spell->m_effectMechanic[i];

        if (!mechanicId) {
            continue;
        }

        bool duplicate = false;

        for (int32_t j = i - 1; j >= 0; j--) {
            if (mechanicId == spell->m_effectMechanic[j]) {
                duplicate = true;
            }
        }

        if (duplicate) {
            continue;
        }

        SpellMechanicRec* mechanic = g_spellMechanicDB.GetRecord(mechanicId);

        if (mechanic && mechanic->m_id != 26) {
            DisplayMechanicEffect(&guid, &aura->m_creator, spell, mechanic);
        }
    }

    int32_t mechanicId = spell->m_mechanic;

    if (!mechanicId) {
        return;
    }

    bool listed = spell->m_effectMechanic[0] == mechanicId;

    if (spell->m_effectMechanic[1] == mechanicId) {
        listed = true;
    }

    if (spell->m_effectMechanic[2] == mechanicId || listed) {
        return;
    }

    SpellMechanicRec* mechanic = g_spellMechanicDB.GetRecord(mechanicId);

    if (mechanic && mechanic->m_id != 26) {
        DisplayMechanicEffect(&guid, &aura->m_creator, spell, mechanic);
    }
}

// VERIFY: ecea9cd0 manual
// OFFSET: 0x00751920
void UnitCombatLogCastStart(const WOWGUID& caster, const WOWGUID& target, int32_t spellId) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !SpellIsHiddenInCombatLog(&spell) && unit->GetSpellCastingTime(&spell)) {
        WOWGUID dest = { 0 };
        CombatLogEntry* entry = GetCombatLogEntry(caster, dest, 5, &spell);
        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: d9765cf5 manual
// OFFSET: 0x007519E0
void UnitCombatLogCastGo(const WOWGUID& caster, const WOWGUID& target, int32_t spellId) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || SpellIsHiddenInCombatLog(&spell) || SpellHasOpenLockEffect(&spell)) {
        return;
    }

    if (!unit->GetSpellCastingTime(&spell)) {
        CombatLogEntry* entry = GetCombatLogEntry(caster, target, 6, &spell);
        QueueCombatLogEntry(entry, 0);
    }

    if (IsCombatTextActiveUnit(caster) && (spell.m_attributesExF & 0x40)) {
        SendToCombatText(30, spell.m_name);
    }
}

// VERIFY: 5bc10808 auto
// OFFSET: 0x00752930
void CombatLog::AddAuraReplacedEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t replacedSpellId, uint8_t harmful, bool stolen, uint32_t timeOffset) {
    SpellRec spell;
    SpellRec replaced;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    if (!g_spellDB.GetRecord(replacedSpellId, &replaced) || (replaced.m_attributes & 0x180) || !*replaced.m_name) {
        return;
    }

    CombatLogEntry* entry = GetCombatLogEntry(source, dest, stolen + 26, &spell);
    entry->m_infoFlags |= 0x500;
    entry->m_info.m_values[0] = replacedSpellId;
    reinterpret_cast<uint8_t&>(entry->m_info.m_values[1]) = harmful;
    QueueCombatLogEntry(entry, timeOffset);

    if (IsCombatTextActiveUnit(dest)) {
        SendToCombatText(40, replaced.m_name);
    }
}

// VERIFY: c935fa27 auto
// OFFSET: 0x00752A40
void CombatLog::AddSpellRemoveEntry(const WOWGUID& source, const WOWGUID& dest, int32_t dispelSpellId, int32_t spellId, uint8_t harmful) {
    SpellRec dispel;
    SpellRec spell;
    memset(&dispel, 0, sizeof(dispel));

    if (dispelSpellId && (!g_spellDB.GetRecord(dispelSpellId, &dispel) || (dispel.m_attributes & 0x180) || !*dispel.m_name)) {
        return;
    }

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    WOWGUID remover = (dispel.m_attributesExD & 0x2000) ? WOWGUID{ 0 } : source;
    CombatLogEntry* entry = GetCombatLogEntry(remover, dest, (dispelSpellId != 0) + 28, &spell);

    if (dispelSpellId) {
        entry->m_infoFlags |= 0x100;
        entry->m_info.m_values[0] = dispelSpellId;
    }

    entry->m_infoFlags |= 0x400;
    reinterpret_cast<uint8_t&>(entry->m_info.m_values[1]) = harmful;
    QueueCombatLogEntry(entry, 0);

    if (s_combatTextActiveUnit == dest) {
        SendToCombatText(40, spell.m_name);
    }
}

// VERIFY: 6403f90e auto
// OFFSET: 0x00752BA0
void UnitCombatLogAuraRefreshed(CGUnit_C* unit, const AuraData* aura, int32_t spellId, uint8_t a4) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name || (spell.m_attributes & 0x40) || (spell.m_attributesExF & 0x400)) {
        return;
    }

    WOWGUID guid = unit->m_obj->m_guid;
    CombatLogEntry* entry = GetCombatLogEntry(aura->m_creator, guid, 25, &spell);
    entry->m_infoFlags |= 0x400;
    reinterpret_cast<uint8_t&>(entry->m_info.m_values[1]) = a4;
    QueueCombatLogEntry(entry, 0);

    WOWGUID unitGuid = unit->m_obj->m_guid;
    UnitCombatLogSpellMechanic(unitGuid, aura, &spell);
}

// VERIFY: 9d679be0 manual
// OFFSET: 0x00750660
void SendDamageEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t damage, int32_t overkill, uint32_t school, int32_t resisted, int32_t blocked, int32_t absorbed, bool crit, bool a11, bool a12, uint32_t timeOffset) {
    if (event != 1) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(dest, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->ModifyPredictedHealth(-damage);
        }
    }

    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, spell);
    entry->m_infoFlags |= 0x10;
    entry->m_info.m_values[3] = school;
    entry->m_info.m_values[4] = absorbed;
    entry->m_info.m_values[1] = damage;
    entry->m_info.m_values[5] = resisted;
    entry->m_info.m_values[2] = overkill;
    entry->m_info.m_values[6] = blocked;
    entry->m_infoBits = (entry->m_infoBits & ~0x7) | (crit & 1) | (((a11 & 1) | ((a12 & 1) << 1)) << 1);
    QueueCombatLogEntry(entry, timeOffset);

    if (s_combatTextActiveUnit != dest) {
        return;
    }

    if (resisted > 0) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d%d", s_combatTextTypes[event == 9 ? 22 : 10], damage, resisted);
    } else if (blocked) {
        SendToCombatText(event == 9 ? 23 : 11, damage, blocked);
    } else if (absorbed) {
        SendToCombatText(event == 9 ? 24 : 12, damage, absorbed);
    } else if (crit) {
        SendToCombatText(event == 9 ? 13 : 1, damage);
    } else if (event == 40) {
        SendToCombatText(39, damage);
    } else {
        SendToCombatText(event == 9 ? 14 : 2, damage);
    }
}

// VERIFY: d14d7c4f manual
// OFFSET: 0x00751C40
void UnitCombatLogSpellDamage(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, int32_t overkill, uint32_t school, int32_t absorbed, int32_t resisted, int32_t blocked, uint32_t hitInfo, bool periodic, uint32_t timeOffset) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    if (damage) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(dest, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->TriggerDamageFeedback();
        }

        int32_t event = periodic ? 34 : ((spell.m_attributesExC & 0x8000) ? 3 : 9);
        SendDamageEvent(event, source, dest, &spell, damage, overkill, school, resisted, blocked, absorbed, (hitInfo >> 1) & 1, false, false, timeOffset);
        return;
    }

    int32_t event = periodic ? 33 : ((spell.m_attributesExC & 0x8000) ? 4 : 8);

    if (absorbed) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, &spell);
        entry->m_infoFlags |= 0x2004;
        entry->m_info.m_values[0] = 10;
        entry->m_info.m_values[4] = absorbed;

        if (s_combatTextActiveUnit == dest) {
            SendToCombatText(event == 8 ? 24 : 12);
        }

        QueueCombatLogEntry(entry, 0);
    } else if (blocked) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, &spell);
        entry->m_infoFlags |= 0x8004;
        entry->m_info.m_values[0] = 5;
        entry->m_info.m_values[6] = blocked;

        if (s_combatTextActiveUnit == dest) {
            SendToCombatText(event == 8 ? 23 : 11);
        }

        QueueCombatLogEntry(entry, 0);
    } else if (resisted) {
        CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, &spell);
        entry->m_infoFlags |= 0x4004;
        entry->m_info.m_values[0] = 2;
        entry->m_info.m_values[5] = resisted;

        if (s_combatTextActiveUnit == dest) {
            SendToCombatText(event == 8 ? 22 : 10);
        }

        QueueCombatLogEntry(entry, 0);
    }
}

// VERIFY: 9129ea05 manual
// OFFSET: 0x00752DE0
void UnitCombatLogSplitDamage(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, int32_t overkill, uint32_t school, int32_t absorbed, int32_t resisted, int32_t blocked, uint32_t hitInfo, uint32_t timeOffset) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    SendDamageEvent(42, source, dest, &spell, damage, overkill, school, resisted, blocked, absorbed, (hitInfo >> 1) & 1, false, false, timeOffset);

    if (s_combatTextActiveUnit == dest) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMBAT_TEXT_UPDATE, "%s%d", s_combatTextTypes[42], damage);
    }
}

// VERIFY: 06a9b4e3 auto
// OFFSET: 0x006CE2B0
int32_t CombatLog::HandleSpellRemoveLog(NETMESSAGE msgId, CDataStore* msg, int32_t timeOffset) {
    uint32_t dispelSpellId = 0;
    uint8_t unused = 0;
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    msg->Get(dispelSpellId);
    msg->Get(unused);
    uint32_t count = 0;
    msg->Get(count);

    while (count) {
        count--;
        uint32_t spellId;
        msg->Get(spellId);
        uint8_t harmful;
        msg->Get(harmful);

        if (msgId == SMSG_SPELL_BREAK_LOG) {
            CombatLog::AddSpellRemoveEntry(caster, target, dispelSpellId, spellId, harmful != 0);
        } else {
            CombatLog::AddAuraReplacedEntry(caster, target, dispelSpellId, spellId, harmful != 0, msgId == SMSG_SPELL_STEAL_LOG, timeOffset);
        }
    }

    return 1;
}

// VERIFY: f5db08bc manual
// OFFSET: 0x006D3840
int32_t CombatLog::HandleSpellNonMeleeDamageLog(CDataStore* msg, int32_t timeOffset) {
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t damage = 0;
    msg->Get(damage);
    uint32_t overkill = 0;
    msg->Get(overkill);
    uint8_t schoolByte = 0;
    msg->Get(schoolByte);
    uint32_t school = schoolByte;
    uint32_t absorbed = 0;
    msg->Get(absorbed);
    uint32_t resisted = 0;
    msg->Get(resisted);
    uint8_t periodic = 0;
    msg->Get(periodic);
    uint8_t unused = 0;
    msg->Get(unused);
    uint32_t blocked = 0;
    msg->Get(blocked);
    uint32_t hitInfo = 0;
    msg->Get(hitInfo);
    uint8_t debug = 0;
    msg->Get(debug);

    bool crit = (hitInfo & 0x2) != 0;
    uint32_t flags = crit ? 0x200 : 0;

    if (absorbed) {
        flags |= 0x20;
    }

    if (blocked) {
        flags |= 0x2000;
    }

    if (resisted) {
        flags |= 0x80;
    }

    SpellRec spell;
    int32_t found = g_spellDB.GetRecord(spellId, &spell);

    if (hitInfo & 0x8) {
        UnitCombatLogSplitDamage(caster, target, spellId, damage, overkill, school, absorbed, resisted, blocked, hitInfo, timeOffset);

        if (found && !(spell.m_attributes & 0x100)) {
            CGGameUI::ShowCombatFeedback(target, damage, school, flags);
        }

        return 1;
    }

    WOWGUID source = (hitInfo & 0x10) ? target : caster;
    UnitCombatLogSpellDamage(source, target, spellId, damage, overkill, school, absorbed, resisted, blocked, hitInfo, periodic, timeOffset);

    if (found && !(spell.m_attributes & 0x100)) {
        CGGameUI::ShowCombatFeedback(target, damage, school, flags);
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            if (damage) {
                unit->AddWorldDamageText(caster, damage, &spell, crit, false, false);
            } else if (absorbed) {
                unit->AddWorldText(caster, MISS_ABSORBED, &spell, false);
            } else if (blocked) {
                unit->AddWorldText(caster, MISS_BLOCKED, &spell, false);
            } else if (resisted) {
                unit->AddWorldText(caster, MISS_RESIST, &spell, false);
            }
        }
    }

    if (debug) {
        float value;

        if (hitInfo & 0x1) {
            value = 0.0f;
            msg->Get(value);
            value = 0.0f;
            msg->Get(value);
        }

        if (hitInfo & 0x4) {
            value = 0.0f;
            msg->Get(value);
            value = 0.0f;
            msg->Get(value);
        }

        if (hitInfo & 0x20) {
            for (uint32_t i = 0; i < 6; i++) {
                value = 0.0f;
                msg->Get(value);
            }
        }
    }

    return 1;
}

// VERIFY: f255adaa manual
// OFFSET: 0x007531F0
int32_t CombatLog::HandlePeriodicAuraLog(CDataStore* msg, int32_t timeOffset) {
    SmartGUID target;
    SmartGUID caster;
    *msg >> target;
    *msg >> caster;
    uint32_t spellId = 0;
    msg->Get(spellId);
    uint32_t count = 0;
    msg->Get(count);

    SpellRec spell;
    CGUnit_C* unit;

    if (!g_spellDB.GetRecord(spellId, &spell) || !(unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__)))) {
        msg->m_read = msg->m_size;
        return 1;
    }

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        uint32_t auraType = 0;
        msg->Get(auraType);

        switch (auraType) {
            case 3:
            case 89: {
                uint32_t damage = 0;
                msg->Get(damage);
                uint32_t overkill = 0;
                msg->Get(overkill);
                uint32_t school = 0;
                msg->Get(school);
                uint32_t absorbed = 0;
                msg->Get(absorbed);
                uint32_t resisted = 0;
                msg->Get(resisted);
                uint8_t crit = 0;
                msg->Get(crit);
                uint8_t hitInfo = 0;

                if (!(spell.m_attributes & 0x100)) {
                    if (damage) {
                        unit->AddWorldDamageText(caster, damage, &spell, crit != 0, true, false);
                    } else if (absorbed) {
                        unit->AddWorldText(caster, MISS_ABSORBED, &spell, true);
                    } else if (resisted) {
                        unit->AddWorldText(caster, MISS_RESIST, &spell, true);
                    }

                    uint32_t flags = 0;

                    if (absorbed) {
                        flags = 0x20;
                    }

                    if (resisted) {
                        flags |= 0x80;
                    }

                    if (crit) {
                        flags |= 0x200;
                        hitInfo = 2;
                    }

                    CGGameUI::ShowCombatFeedback(target, damage, school, flags);
                }

                UnitCombatLogSpellDamage(caster, target, spellId, damage, overkill, school, absorbed, resisted, 0, hitInfo, true, timeOffset);
                break;
            }

            case 8:
            case 20: {
                uint32_t amount = 0;
                msg->Get(amount);
                uint32_t overheal = 0;
                msg->Get(overheal);
                uint32_t absorbed = 0;
                msg->Get(absorbed);
                uint8_t crit = 0;
                msg->Get(crit);

                if (!(spell.m_attributes & 0x100)) {
                    if (!amount && absorbed) {
                        unit->AddWorldText(caster, MISS_ABSORBED, &spell, true);
                    }

                    CGGameUI::ShowHealingFeedback(target, amount, crit != 0);
                }

                UnitCombatLogHeal(caster, target, spellId, amount, overheal, absorbed, crit != 0, true, timeOffset);
                break;
            }

            case 21:
            case 24: {
                uint32_t powerType = 0;
                msg->Get(powerType);
                uint32_t amount = 0;
                msg->Get(amount);

                if (!(spell.m_attributes & 0x100)) {
                    CGGameUI::ShowEnergizeFeedback(target, powerType, amount);
                }

                UnitCombatLogEnergize(caster, target, spellId, powerType, amount, true, timeOffset);
                break;
            }

            case 64: {
                uint32_t powerType = 0;
                msg->Get(powerType);
                uint32_t amount = 0;
                msg->Get(amount);
                float multiplier = 0.0f;
                msg->Get(multiplier);
                UnitCombatLogPowerDrain(caster, target, spellId, powerType, amount, multiplier, true, timeOffset);
                break;
            }

            default:
                msg->m_read = msg->m_size;
                return 1;
        }
    }

    return 1;
}

// VERIFY: 150dd800 auto
// OFFSET: 0x00753710
int32_t Packet_SMSG_PERIODICAURALOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CombatLog::HandlePeriodicAuraLog(msg, 0);
}

// VERIFY: c5d9ed0a manual
// OFFSET: 0x00752ED0
void UnitCombatLogUnitDead(CGUnit_C* unit) {
    int32_t event = 44;
    WOWGUID none = WOWGUID();
    CreatureStats_C* creature = g_creatureDBCache.GetRecord(unit->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (creature && creature->m_type == 13) {
        event = 49;
    } else {
        SpellRec spell;

        if (!g_spellDB.GetRecord(unit->m_unit->m_unitCreatedBySpell, &spell) && spell.m_effect[0] == SPELL_EFFECT_SUMMON) {
            SummonPropertiesRec* summon = g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[0]);

            if (summon && static_cast<uint32_t>(summon->m_slot - 1) <= 3) {
                event = 45;
            }
        }
    }

    WOWGUID dest = unit->m_obj->m_guid;
    WOWGUID source = WOWGUID();
    CombatLogEntry* entry = GetCombatLogEntry(source, dest, event, nullptr);
    QueueCombatLogEntry(entry, 0);
}

// VERIFY: a10e0f3b manual
// OFFSET: 0x0074D1A0
static bool CombatLogFlagsMatch(uint32_t filterFlags, uint32_t flags) {
    uint32_t matched = filterFlags & flags;

    if (matched & 0xFFFF0000) {
        return true;
    }

    if (!(matched & 0xF) || !(matched & 0xF0) || !(matched & 0x300)) {
        return false;
    }

    return (matched & 0xFC00) != 0;
}

// VERIFY: e3b2fb68 manual
// OFFSET: 0x0074E1A0
bool CombatLogFilter::MatchesFilter(int32_t event, const WOWGUID& sourceGuid, uint32_t sourceFlags, const WOWGUID& destGuid, uint32_t destFlags, int32_t spellId, const char* spellName) {
    uint32_t index = static_cast<uint32_t>(event);

    if (!(this->m_events.m_bits.m_data[index >> 5] & (1 << (index & 0x1F)))) {
        return false;
    }

    if (this->m_sourceGuid) {
        if (sourceGuid != this->m_sourceGuid) {
            return false;
        }
    } else {
        uint32_t matched = this->m_sourceFlags & sourceFlags;

        if (!(matched & 0xFFFF0000) && (!(matched & 0xF) || !(matched & 0xF0) || !(matched & 0x300) || !(matched & 0xFC00))) {
            return false;
        }
    }

    if (this->m_destGuid) {
        if (destGuid != this->m_destGuid) {
            return false;
        }
    } else if (!CombatLogFlagsMatch(this->m_destFlags, destFlags)) {
        return false;
    }

    if (this->m_spellId) {
        return spellId == this->m_spellId;
    }

    const char* name = this->m_spellName.m_str;
    return !name || (spellName && !SStrCmpUTF8I(spellName, name, 0x7FFFFFFF));
}

// OFFSET: 0x00753980
static void UnitCombatLogShutdown() {
    s_entries.Clear();
    s_freeEntries.Clear();
    s_pendingEntries = nullptr;
    s_currentEntry = nullptr;
    s_filters.Clear();
    s_pendingData.Clear();
    s_freePendingData.Clear();
    s_invalidatedNames.Clear();
    ClientServices::ClearMessageHandler(SMSG_SPELL_PERIODIC_AURA_LOG);
    ClientServices::ClearMessageHandler(SMSG_ENCHANTMENT_LOG);
    ClientServices::ClearMessageHandler(SMSG_PARTY_KILL_LOG);
    ClientServices::ClearMessageHandler(SMSG_PROC_RESIST);
    ClientServices::ClearMessageHandler(SMSG_DISPEL_FAILED);
    ClientServices::ClearMessageHandler(SMSG_DESTRUCTIBLE_BUILDING_DAMAGE);
}

// VERIFY: 5d930252 auto
// OFFSET: 0x00754F60
void UnitCombatClientShutdown() {
    ClientServices::ClearMessageHandler(SMSG_ATTACK_START);
    ClientServices::ClearMessageHandler(SMSG_ATTACK_STOP);
    ClientServices::ClearMessageHandler(SMSG_ATTACKER_STATE_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_ATTACKSWING_NOTINRANGE);
    ClientServices::ClearMessageHandler(SMSG_ATTACKSWING_BADFACING);
    ClientServices::ClearMessageHandler(SMSG_ATTACKSWING_DEADTARGET);
    ClientServices::ClearMessageHandler(SMSG_ATTACKSWING_CANT_ATTACK);
    ClientServices::ClearMessageHandler(SMSG_ENVIRONMENTAL_DAMAGE_LOG);
    ClientServices::ClearMessageHandler(SMSG_CLEAR_TARGET);
    ClientServices::ClearMessageHandler(SMSG_COMBAT_EVENT_FAILED);
    UnitCombatLogShutdown();
}

// VERIFY: 5e3c090e manual
// OFFSET: 0x0074D280
static void PackHex(char*& cursor, uint32_t& remaining, uint32_t value) {
    if (remaining > 1) {
        *cursor++ = ',';
        remaining--;
    }

    if (remaining < 10) {
        return;
    }

    *cursor++ = '0';
    *cursor++ = 'x';
    remaining -= 2;

    if (!value) {
        *cursor++ = '0';
        remaining--;
        return;
    }

    uint32_t mask = 0xF0000000;
    int32_t digits = 8;

    while (!(mask & value)) {
        mask >>= 4;
        digits--;
    }

    cursor += digits;

    do {
        cursor--;
        uint8_t digit = value & 0xF;
        *cursor = digit >= 10 ? digit + 'a' - 10 : digit + '0';
        value >>= 4;
    } while (value);

    cursor += digits;
    remaining -= digits;
}

// VERIFY: 9e0977cb manual
// OFFSET: 0x0074D310
static void PackString(char*& cursor, uint32_t& remaining, const char* string) {
    if (remaining > 1) {
        *cursor++ = ',';
        remaining--;
    }

    for (char c = *string; c && remaining; c = *++string) {
        *cursor++ = c;
        remaining--;
    }
}

// VERIFY: aa86aa1a manual
// OFFSET: 0x0074D350
static void PackQuotedString(char*& cursor, uint32_t& remaining, const char* string) {
    if (remaining > 1) {
        *cursor++ = ',';

        if (--remaining > 1) {
            *cursor++ = '"';
            remaining--;
        }
    }

    for (char c = *string; c && remaining; c = *++string) {
        if ((c == '"' || c == '\\') && remaining > 1) {
            *cursor++ = '\\';
            remaining--;
        }

        *cursor++ = *string;
        remaining--;
    }

    if (remaining > 1) {
        *cursor++ = '"';
        remaining--;
    }
}

// INLINED
static inline void PackInt(char*& cursor, uint32_t& remaining, int32_t value) {
    if (remaining > 1) {
        *cursor++ = ',';
        remaining--;
    }

    uint32_t length = SStrPrintf(cursor, remaining, "%d", value);

    if (length && length < remaining) {
        cursor += length;
        remaining -= length;
    }
}

// INLINED
static inline void PackUnit(char*& cursor, uint32_t& remaining, const WOWGUID& guid, const char* name, const char* realm, uint32_t flags) {
    if (remaining > 1) {
        *cursor++ = ',';
        remaining--;
    }

    if (remaining > 18) {
        GUIDToHexString(guid, cursor);
        cursor += 18;
        remaining -= 18;
    }

    if (!name) {
        PackString(cursor, remaining, "nil");
    } else if (realm) {
        char fullName[305];
        SStrPrintf(fullName, sizeof(fullName), "%s-%s", name, realm);
        PackQuotedString(cursor, remaining, fullName);
    } else {
        PackQuotedString(cursor, remaining, name);
    }

    PackHex(cursor, remaining, flags);
}

// VERIFY: 18bf57c5 manual
// OFFSET: 0x0074E880
void CombatLogEntry::PackEvent(char* buffer, uint32_t size) {
    char* cursor = buffer;
    uint32_t remaining = size;
    size_t length = SStrCopy(buffer, s_combatLogEventNames[this->m_event], size);
    remaining += static_cast<uint32_t>(length);
    cursor += length;
    PackUnit(cursor, remaining, this->m_sourceGuid, this->m_sourceName, this->m_sourceRealm, this->m_sourceFlags);
    PackUnit(cursor, remaining, this->m_destGuid, this->m_destName, this->m_destRealm, this->m_destFlags);

    if (this->m_spellId) {
        PackInt(cursor, remaining, this->m_spellId);
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_spellId, &spell)) {
            PackQuotedString(cursor, remaining, spell.m_name);
            PackHex(cursor, remaining, spell.m_schoolMask);
        } else {
            PackString(cursor, remaining, "nil");

            if (remaining > 1) {
                *cursor++ = ',';
                remaining--;
            }

            if (remaining >= 10) {
                *cursor++ = '0';
                *cursor++ = 'x';
                *cursor++ = '0';
                remaining -= 3;
            }
        }
    }

    uint32_t flags = this->m_infoFlags;

    if (flags & 0x1) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
    }

    if (flags & 0x2) {
        PackQuotedString(cursor, remaining, this->m_info.m_text);
    }

    if (flags & 0x4) {
        PackString(cursor, remaining, g_spellMissStrings[this->m_info.m_values[0]]);
    }

    if (flags & 0x8) {
        PackString(cursor, remaining, s_environmentalDamageTypes[this->m_info.m_values[0]]);
    }

    if (flags & 0x10) {
        PackInt(cursor, remaining, this->m_info.m_values[1]);
        PackInt(cursor, remaining, this->m_info.m_values[2]);
        PackInt(cursor, remaining, this->m_info.m_values[3]);
        PackInt(cursor, remaining, this->m_info.m_values[5]);
        PackInt(cursor, remaining, this->m_info.m_values[6]);
        PackInt(cursor, remaining, this->m_info.m_values[4]);

        if (this->m_infoBits & 0x1) {
            PackInt(cursor, remaining, 1);
        } else {
            PackString(cursor, remaining, "nil");
        }

        if (this->m_infoBits & 0x2) {
            PackInt(cursor, remaining, 1);
        } else {
            PackString(cursor, remaining, "nil");
        }

        if (this->m_infoBits & 0x4) {
            PackInt(cursor, remaining, 1);
        } else {
            PackString(cursor, remaining, "nil");
        }
    }

    if (flags & 0x2000) {
        PackInt(cursor, remaining, this->m_info.m_values[4]);
    }

    if (flags & 0x4000) {
        PackInt(cursor, remaining, this->m_info.m_values[5]);
    }

    if (flags & 0x8000) {
        PackInt(cursor, remaining, this->m_info.m_values[6]);
    }

    if (flags & 0x20) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
        PackInt(cursor, remaining, this->m_info.m_values[1]);
        PackInt(cursor, remaining, this->m_info.m_values[2]);

        if (this->m_info.m_values[3]) {
            PackInt(cursor, remaining, 1);
        } else {
            PackString(cursor, remaining, "nil");
        }
    }

    if (flags & 0x40) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
        PackInt(cursor, remaining, this->m_info.m_values[1]);
    }

    if (flags & 0x80000000) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
        PackInt(cursor, remaining, this->m_info.m_values[1]);
        PackInt(cursor, remaining, this->m_info.m_values[2]);
    }

    if (flags & 0x100) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
        SpellRec spell;

        if (g_spellDB.GetRecord(this->m_info.m_values[0], &spell)) {
            PackQuotedString(cursor, remaining, spell.m_name);
            PackInt(cursor, remaining, spell.m_schoolMask);
        } else {
            PackString(cursor, remaining, "nil");
            PackInt(cursor, remaining, 0);
        }
    }

    if (flags & 0x1000) {
        PackQuotedString(cursor, remaining, this->m_info.m_names[2]);
    }

    if (flags & 0x200) {
        PackInt(cursor, remaining, this->m_info.m_values[0]);
        PackQuotedString(cursor, remaining, this->m_info.m_names[1]);
    }

    if (flags & 0x400) {
        PackString(cursor, remaining, this->m_info.m_values[1] ? "DEBUFF" : "BUFF");
    }

    if (flags & 0x800) {
        PackInt(cursor, remaining, this->m_info.m_values[2]);
    }

    *cursor = '\0';
}

// VERIFY: 8242155b manual
// OFFSET: 0x00751150
void UnitCombatLog(const WOWGUID& guid, uint32_t type, int32_t damage, int32_t absorbed, int32_t resisted) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->ModifyPredictedHealth(-damage);
        unit->TriggerDamageFeedback();
    }

    CombatLogEntry* entry = GetCombatLogEntry(WOWGUID(), guid, 0, nullptr);
    entry->m_infoFlags |= 0x8;
    entry->m_info.m_values[0] = type;
    entry->m_infoBits &= ~0x7;
    entry->m_infoFlags |= 0x10;
    entry->m_info.m_values[3] = 1 << s_environmentalDamageSchools[type];
    entry->m_info.m_values[5] = resisted;
    entry->m_info.m_values[1] = damage;
    entry->m_info.m_values[2] = 0;
    entry->m_info.m_values[6] = 0;
    entry->m_info.m_values[4] = absorbed;
    QueueCombatLogEntry(entry, 0);

    if (s_combatTextActiveUnit != guid) {
        return;
    }

    if (resisted) {
        SendToCombatText(10, damage, resisted);
    } else if (absorbed) {
        SendToCombatText(12, damage, absorbed);
    } else {
        SendToCombatText(2, damage);
    }
}

// VERIFY: b02c1457 manual
// OFFSET: 0x00753000
void UnitCombatLogBuildingDamage(const WOWGUID& caster, const WOWGUID& owner, const WOWGUID& building, int32_t damage, int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    CombatLogEntry* entry = GetCombatLogEntry(caster, building, 47, &spell);
    entry->m_infoFlags |= 0x10;
    entry->m_infoBits &= ~0x7;
    entry->m_info.m_values[1] = damage;
    entry->m_info.m_values[2] = 0;
    entry->m_info.m_values[3] = spell.m_schoolMask;
    entry->m_info.m_values[5] = 0;
    entry->m_info.m_values[6] = 0;
    entry->m_info.m_values[4] = 0;
    QueueCombatLogEntry(entry, 0);

    if (owner == ClntObjMgrGetActivePlayer()) {
        CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(building, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (object) {
            object->AddWorldDamageText(damage);
        }
    }
}

// VERIFY: 50749369 auto
// OFFSET: 0x00753100
void UnitCombatLogBuildingHeal(const WOWGUID& caster, const WOWGUID& owner, const WOWGUID& building, int32_t amount, int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    CombatLogEntry* entry = GetCombatLogEntry(caster, building, 48, &spell);
    entry->m_infoFlags |= 0x20;
    entry->m_info.m_values[0] = amount;
    entry->m_info.m_values[1] = 0;
    entry->m_info.m_values[2] = 0;
    entry->m_info.m_values[3] = 0;
    QueueCombatLogEntry(entry, 0);

    if (owner == ClntObjMgrGetActivePlayer()) {
        CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(building, TYPE_GAMEOBJECT, __FILE__, __LINE__));

        if (object) {
            object->AddWorldHealingText(amount, &spell);
        }
    }
}

// VERIFY: e38662ad auto
// OFFSET: 0x00753730
int32_t Packet_SMSG_DESTRUCTIBLE_BUILDING_DAMAGE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID building;
    SmartGUID caster;
    SmartGUID owner;
    int32_t damage;
    int32_t spellId;
    *msg >> building;
    *msg >> caster;
    *msg >> owner;
    msg->Get(reinterpret_cast<uint32_t&>(damage));
    msg->Get(reinterpret_cast<uint32_t&>(spellId));

    if (damage <= 0) {
        WOWGUID a = caster;
        WOWGUID b = owner;
        WOWGUID c = building;
        UnitCombatLogBuildingHeal(a, b, c, -damage, spellId);
    } else {
        WOWGUID c = building;
        WOWGUID b = owner;
        WOWGUID a = caster;
        UnitCombatLogBuildingDamage(a, b, c, damage, spellId);
    }

    return 1;
}
