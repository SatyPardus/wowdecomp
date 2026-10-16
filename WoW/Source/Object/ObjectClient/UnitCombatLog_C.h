#ifndef WOW_SOURCE_OBJECT_OBJECTCLIENT_UNITCOMBATLOG_C_H
#define WOW_SOURCE_OBJECT_OBJECTCLIENT_UNITCOMBATLOG_C_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Storm/H/stpl.h"

class ATTACKROUNDINFO;
class CGUnit_C;
class CDataStore;
class SpellRec;
struct lua_State;
struct PendingCombatLogData;

struct CombatLogEntry : public TSLinkedNode<CombatLogEntry> {
    /* 0008 */ uint32_t m_timestamp;
    /* 000C */ int32_t m_event;
    /* 0010 */ uint32_t m_pendingMask;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ WOWGUID m_sourceGuid;
    /* 0020 */ const char* m_sourceName;
    /* 0024 */ const char* m_sourceRealm;
    /* 0028 */ uint32_t m_sourceFlags;
    /* 002C */ uint32_t unused_002C;
    /* 0030 */ WOWGUID m_destGuid;
    /* 0038 */ const char* m_destName;
    /* 003C */ const char* m_destRealm;
    /* 0040 */ uint32_t m_destFlags;
    /* 0044 */ int32_t m_spellId;
    /* 0048 */ RCString m_spellName;
    /* 0054 */ uint32_t m_infoFlags;
    /* 0058 */ union {
        char* m_text;
        int32_t m_values[7];
        const char* m_names[7];
    } m_info;
    /* 0074 */ uint8_t m_infoBits;

    static void PendingCombatLogResolved(int32_t id, const WOWGUID& guid, void* param, bool success);

    CombatLogEntry();
    ~CombatLogEntry();
    void Clear();
    void CancelPendingCallbacks();
    const char* ResolveName(PendingCombatLogData* data, const char** realm, bool pending);
    const char* GetObjectName(const WOWGUID& guid, uint32_t mask, const char** realm);
    int32_t PushEvent(lua_State* L);
    bool MatchesFilter();
    void PackEvent(char* buffer, uint32_t size);
    void SetFailReason(const char* reason);
    void SetMissInfo(int32_t missType, int32_t amount);
    void SetExecuteItem(int32_t itemId);
};

struct InvalidatedName : public TSLinkedNode<InvalidatedName> {
    /* 0008 */ char* m_name;

    InvalidatedName();
    ~InvalidatedName();
};

struct PendingCombatLogData : public TSLinkedNode<PendingCombatLogData> {
    /* 0008 */ CombatLogEntry* m_entry;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ WOWGUID m_guid;
    /* 0018 */ int32_t m_id;
    /* 001C */ int32_t m_type;
    /* 0020 */ uint32_t m_mask;
    /* 0024 */ uint32_t unused_0024;
};

struct CombatLogFilter : public TSLinkedNode<CombatLogFilter> {
    /* 0008 */ TSBitField<TSFixedArray<uint32_t>> m_events;
    /* 0018 */ WOWGUID m_sourceGuid;
    /* 0020 */ WOWGUID m_destGuid;
    /* 0028 */ uint32_t m_sourceFlags;
    /* 002C */ uint32_t m_destFlags;
    /* 0030 */ int32_t m_spellId;
    /* 0034 */ RCString m_spellName;

    CombatLogFilter();
    bool SetFilterData(const char* events, const WOWGUID& sourceGuid, uint32_t sourceFlags, const WOWGUID& destGuid, uint32_t destFlags, int32_t spellId, const char* spellName);
    bool MatchesFilter(int32_t event, const WOWGUID& sourceGuid, uint32_t sourceFlags, const WOWGUID& destGuid, uint32_t destFlags, int32_t spellId, const char* spellName);
};

class CombatLog {
    public:
    static int32_t HandleSpellRemoveLog(NETMESSAGE msgId, CDataStore* msg, int32_t timeOffset);
    static int32_t HandleSpellLogMiss(CDataStore* msg, int32_t timeOffset);
    static int32_t HandleSpellLogExecute(CDataStore* msg, int32_t timeOffset);
    static int32_t HandleSpellNonMeleeDamageLog(CDataStore* msg, int32_t timeOffset);
    static int32_t HandleSpellHealLog(CDataStore* msg, int32_t timeOffset);
    static int32_t HandleSpellLogEnergizeLog(CDataStore* msg, int32_t timeOffset);
    static int32_t HandlePeriodicAuraLog(CDataStore* msg, int32_t timeOffset);
    static void AddSpellCastEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId);
    static void AddSpellResurrectEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId);
    static void AddSpellDamageEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, uint32_t timeOffset);
    static void AddSpellExecuteEntry(const WOWGUID& source, const WOWGUID& dest, int32_t itemId, int32_t spellId);
    static void AddSpellEnergizeEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId);
    static void AddAuraReplacedEntry(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t replacedSpellId, uint8_t harmful, bool stolen, uint32_t timeOffset);
    static void AddSpellRemoveEntry(const WOWGUID& source, const WOWGUID& dest, int32_t dispelSpellId, int32_t spellId, uint8_t harmful);
    static void GetSpellLogLinkInfo(const WOWGUID& source, const WOWGUID& dest, int32_t spellId);
    static void QueueMissedSpellText(const WOWGUID& source, const WOWGUID& dest, int32_t event, const SpellRec* spell, int32_t missType, int32_t amount, uint32_t timeOffset);
};

void UnitCombatLogInstakill(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId);
void UnitCombatLogSpellMissed(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t missType, bool a5, bool a6, int32_t a7);
void UnitCombatLogDamageShield(const WOWGUID& victimGuid, const WOWGUID& casterGuid, int32_t spellId, int32_t damage, uint32_t overkill, uint32_t schoolMask);
PendingCombatLogData* GetPendingCombatLogData(CombatLogEntry* entry, const WOWGUID& guid, uint32_t mask);
CombatLogEntry* GetCombatLogEntry(const WOWGUID& source, const WOWGUID& dest, int32_t event, const SpellRec* spell);
void RecycleCombatLogEntry(CombatLogEntry* entry);
uint32_t GetCombatLogFlags(const WOWGUID& guid);
void SendCombatLogEvents();
void QueueCombatLogEntry(CombatLogEntry* entry, uint32_t timeOffset);
void UnitCombatLogBuildingDamage(const WOWGUID& caster, const WOWGUID& owner, const WOWGUID& building, int32_t damage, int32_t spellId);
void UnitCombatLogBuildingHeal(const WOWGUID& caster, const WOWGUID& owner, const WOWGUID& building, int32_t amount, int32_t spellId);
extern const uint32_t s_environmentalDamageSchools[6];
bool ShouldShowFeedback(const ATTACKROUNDINFO& info);
void UnitCombatLogInvalidateName(const char* name);
char* GUIDToHexString(WOWGUID guid, char* buffer);
WOWGUID StringToGuid(const char* string);
void UnitCombatLog(const WOWGUID& guid, uint32_t type, int32_t damage, int32_t absorbed, int32_t resisted);
void UnitCombatLog(const ATTACKROUNDINFO& info);
void UnitCombatLogUnitDead(CGUnit_C* unit);
void UnitCombatLogCastGo(const WOWGUID& caster, const WOWGUID& target, int32_t spellId);
void UnitCombatLogSpellFail(const WOWGUID& caster, int32_t spellId, const char* reason);
class AuraData;
void UnitCombatLogAuraDose(CGUnit_C* unit, const AuraData* aura, int32_t spellId, int32_t oldStack, int32_t newStack, uint8_t a6);
void UnitCombatLogAuraRefreshed(CGUnit_C* unit, const AuraData* aura, int32_t spellId, uint8_t a4);
int32_t Packet_SMSG_PERIODICAURALOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_ENCHANTMENTLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_PARTYKILLLOG(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_PROCRESIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_DISPEL_FAILED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_DESTRUCTIBLE_BUILDING_DAMAGE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
void __stdcall CheckToSanitizeUnitLowGuid(WOWGUID* guid);
int32_t IsCombatTextActiveUnit(WOWGUID guid);
void SendToCombatText(int32_t type, const char* text, int32_t amount);
void SendToCombatText(int32_t type, const char* text, int32_t amount, int32_t overflow);
void SendToCombatText(int32_t type, int32_t amount, int32_t overflow);
void SendToCombatText(int32_t type, int32_t amount);
void SendToCombatText(int32_t type, const char* text);
void SendToCombatText(int32_t type);
void UnitCombatLogAuraAddedOrRemoved(CGUnit_C* unit, AuraData* aura, const SpellRec* spell, uint8_t harmful, int32_t added);
void UnitCombatLogProcGo(const WOWGUID& casterGuid, const SpellRec* spell);
void UnitCombatLogPowerDrain(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, uint32_t powerType, uint32_t amount, float multiplier, bool a7, uint32_t timeOffset);
void UnitCombatLogInterrupt(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t interruptedSpellId);
void UnitCombatLogHeal(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t amount, int32_t overheal, int32_t absorbed, bool crit, bool periodic, uint32_t timeOffset);
void UnitCombatLogEnergize(const WOWGUID& casterGuid, const WOWGUID& targetGuid, int32_t spellId, int32_t powerType, int32_t amount, bool periodic, uint32_t timeOffset);
void SendHealEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t amount, int32_t overheal, int32_t absorbed, bool crit, uint32_t timeOffset);
void SendEnergizeEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t powerType, int32_t amount, uint32_t timeOffset);
void AddPowerDrainEntry(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t amount, int32_t powerType, int32_t extra, uint32_t timeOffset);
void SendDamageEvent(int32_t event, const WOWGUID& source, const WOWGUID& dest, const SpellRec* spell, int32_t damage, int32_t overkill, uint32_t school, int32_t resisted, int32_t blocked, int32_t absorbed, bool crit, bool a11, bool a12, uint32_t timeOffset);
void UnitCombatLogSpellDamage(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, int32_t overkill, uint32_t school, int32_t absorbed, int32_t resisted, int32_t blocked, uint32_t hitInfo, bool periodic, uint32_t timeOffset);
void UnitCombatLogSplitDamage(const WOWGUID& source, const WOWGUID& dest, int32_t spellId, int32_t damage, int32_t overkill, uint32_t school, int32_t absorbed, int32_t resisted, int32_t blocked, uint32_t hitInfo, uint32_t timeOffset);
void UnitCombatLogSpellMechanic(const WOWGUID& guid, const AuraData* aura, const SpellRec* spell);
bool SpellIsHiddenInCombatLog(const SpellRec* spell);
bool SpellHasOpenLockEffect(const SpellRec* spell);
class SpellMechanicRec;
void DisplayMechanicEffect(const WOWGUID* unitGuid, const WOWGUID* sourceGuid, const SpellRec* spell, const SpellMechanicRec* mechanic);

void UnitCombatLogRegisterScriptFunctions();
void UnitCombatLogUnregisterScriptFunctions();

// INLINED
inline CombatLogFilter::CombatLogFilter() {
    this->m_events.m_count = 50;
    this->m_events.m_bits.SetCount(((50 - 1) >> 5) + 1);
}

extern TSList<CombatLogEntry, TSGetLink<CombatLogEntry> > s_entries;
extern WOWGUID s_combatTextActiveUnit;
extern CombatLogEntry* s_currentEntry;
extern CombatLogEntry* s_pendingEntries;
extern const char* g_spellMissStrings[12];

#endif
