#ifndef WOW_SOURCE_UI_SCRIPTEVENTS_H
#define WOW_SOURCE_UI_SCRIPTEVENTS_H

#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/Ui/Types.h"

class CGUnit_C;
class SpellRec;
class VehicleSeatRec;
struct lua_State;

struct UniqueSignal {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ int32_t m_signal;
    /* 000C */ uint32_t unused_000C;
};

struct ScriptAuraInfo {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ int32_t m_index;
    /* 000C */ const char* m_name;
    /* 0010 */ const char* m_rank;
    /* 0014 */ uint8_t m_filter;
};

extern const char* g_scriptEvents[NUM_SCRIPT_EVENTS];

CGUnit_C* GetUnitFromName(const char* token);
WOWGUID GetGuidByKeyword(const char* token);
bool GetUnitByKeyword(const char* token, CGUnit_C** unit, bool defaultToTarget);
int32_t Script_GetUnitAuraInfo(lua_State* L, int32_t index, ScriptAuraInfo* info);
int32_t PushUnitAura(lua_State* L, ScriptAuraInfo* info);
const char* Script_GetAuraTokenFromGUID(const WOWGUID* guid);
const char* Script_GetTokenFromGUID(const WOWGUID* guid);
VehicleSeatRec* GetVehicleSeatRecAsPassengerFromLua(lua_State* L);
void UnitNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
bool Script_GetGUIDFromString(const char** name, WOWGUID& guid);
bool Script_GetGUIDFromToken(const char* token, WOWGUID& guid, bool defaultToTarget);
bool Script_GetNameFromToken(const char* token, const char** name, bool defaultToTarget, bool withRealm);
char** Script_GetTokenIDsFromGUID(const WOWGUID& guid, int32_t* count);
void ScriptEventsInitialize();
void ScriptEventsRegisterFunctions();
void ScriptEventsUnregisterFunctions();
void SystemRegisterFunctions();
void SystemUnregisterFunctions();
void Script_SendUniqueSignal(const WOWGUID& guid, int32_t signal);
void Script_SendUnitSignal(const WOWGUID& guid, int32_t signal);
void Script_SendUniqueSignal(int32_t signal);
void Script_SendUniqueSignalToAllUnits(int32_t signal);
void Script_SendUniqueSignals();

void Script_SendUnitSpellSignal(const WOWGUID& guid, uint8_t castCount, const SpellRec* spell, int32_t signal);
void ScriptEventsRegisterUnit(CGUnit_C* unit);
void ScriptEventsUnregisterUnit(CGUnit_C* unit);

void ClearUniqueSignals();

extern const int32_t s_unitSexValues[3];

extern const char* s_vehicleSkinNames[2];

#endif
