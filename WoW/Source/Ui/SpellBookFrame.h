#ifndef WOW_SOURCE_UI_SPELLBOOKFRAME_H
#define WOW_SOURCE_UI_SPELLBOOKFRAME_H

#include <cstdint>
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

struct lua_State;
class CGUnit_C;
class CGObject_C;
class AuraData;

struct SpellUsableInfo {
    /* 0000 */ int32_t m_spellId;
    /* 0004 */ uint8_t m_usable;
    /* 0005 */ uint8_t m_noMana;
    /* 0006 */ uint8_t unused_0006[2];
};

struct SpellBookSkillInfo {
    /* 0000 */ int32_t m_skillLine;
    /* 0004 */ uint32_t m_count;
    /* 0008 */ uint32_t m_unk08;
};

struct SPELLRANKNODE : public TSLinkedNode<SPELLRANKNODE> {
    /* 0008 */ int32_t m_spellId;
    /* 000C */ int32_t m_petSpell;
    /* 0010 */ RCString m_subtext;
    /* 001C */ uint32_t m_rank;
};

struct SPELLSBYNAME : public TSHashObject<SPELLSBYNAME, HASHKEY_UTF8I> {
    /* 0018 */ TSList<SPELLRANKNODE> m_spells;
};

class CGPlayer_C;
class SpellRec;

class CGSpellBook {
    public:
    static void GetAppropriateSpellRank(uint32_t* spellId, CGUnit_C* caster, CGUnit_C* target);
    static const char* GetSpellTexture(int32_t slot, int32_t petSpell);
    static int32_t IsSelectedSlot(int32_t slot, int32_t petSpell);
    static int32_t FindSpellByName(const char* name, const char* subtext, int32_t* petSpell);
    static int32_t m_autoRangedSpell;
    static void UpdateCooldowns();
    static int32_t IsTrackingSpell(int32_t spellId);
    static TSFixedArray<int32_t> m_languageSpells;
    static TSGrowableArray<int32_t> m_unlearnedSpells;
    static int32_t m_duelSpell;
    static int32_t m_stuckSpell;
    static uint32_t m_numPetSpells;
    static int32_t m_summonFriendSpell;
    static TSGrowableArray<int32_t> m_companions[2];

    static void AddKnownSpell(int32_t spellId, int32_t notify, int32_t replaceSpell);
    static void AddSpellToNameCache(int32_t spellId, int32_t isPet);
    static void CompanionCallback(int32_t id, const WOWGUID& guid, void* param, bool success);
    static int32_t GetSpellProgressionRank(const SpellRec* spell);
    static int32_t GetCompanionType(const SpellRec* spell);
    static int32_t FindCompanionSlotByID(int32_t spellId, int32_t type);
    static TSGrowableArray<int32_t> m_totemSpells[4];
    static uint32_t m_interruptSchoolMask;
    static uint32_t m_interruptSpecialSchoolMask;
    static uint32_t m_silenceSchoolMask;
    static uint32_t m_silenceSpecialSchoolMask;

    static void ClearSpells();
    static void UpdateCompanions();
    static void UpdateSelection();
    static void UpdateUsablePower();
    static void UpdateSpells(int32_t a1, int32_t a2, int32_t a3);
    static void UpdateUsable();
    static void CastSpell(int32_t spellId, int32_t petSpell, WOWGUID target);
    static void InitializeGame();
    static void ShutdownGame();
    static void MakeKnownSpellModelsLocal();
    static int32_t GetSkinningSpell(const CGObject_C* object);
    static void ReplaceSpell(int32_t oldSpell, int32_t newSpell);
    static void DelKnownSpell(int32_t spellId, int32_t notify);
    static void SendCompanionUpdate(int32_t type);
    static int32_t GetSpellRankOrLevel(const SpellRec* spell);
    static void PickupSpell(int32_t slot, int32_t petSpell);
    static void PickupCompanion(int32_t type, uint32_t index);
    static int32_t IsAuraCaster(WOWGUID guid, const WOWGUID* caster);
    static int32_t FindRaidDispel(int32_t dispelType);
    static bool IsUnlearnedSpell(int32_t spellId);
    static void AddPetSpell(int32_t spellId);
    static void DelSpellFromNameCache(int32_t spellId, int32_t petSpell);
    static uint32_t GetShapeshiftForm(int32_t requireForm);
    static int32_t CanShowAura(const SpellRec* spell, const WOWGUID* caster, bool helpful);
    static int32_t CanApplyAura(const AuraData* aura);
    static int32_t FindRaidBuff(int32_t spellId);
    static int32_t IsShapeshiftSpell(int32_t spellId);
    static int32_t CanStealAura(const CGUnit_C* unit, const AuraData* aura, const SpellRec* spell);
};

int32_t IsValidSpell(const char* name, int32_t* petSpell);
int32_t GetSpellSlot(int32_t spellId, int32_t petSpell);
int32_t GetSpellIdBySlot(int32_t slot, int32_t petSpell);
bool HasSpell(int32_t spellId, int32_t petSpell);
bool HasSpell(uint32_t spellId);
void ClearPetSpellInfo();
extern const char* s_companionTypes[2];
extern TSGrowableArray<SpellUsableInfo> s_shapeshiftSpells;
extern uint32_t s_numSkillInfos;
extern uint32_t s_displayedSpells[1024];
extern int32_t s_spellList[1024];
extern uint32_t s_numSpells;
extern uint32_t s_displayedSpellCount;
extern TSGrowableArray<SpellBookSkillInfo*> s_skillInfos;
void PrefetchSpellVisuals(int32_t spellId);

void SpellBookRegisterScriptFunctions();
void SpellBookUnregisterScriptFunctions();

int32_t GetActionBarPageOffset(const CGPlayer_C* player, const SpellRec* spell);
int __cdecl SortSpellUsableInfo(const void* a, const void* b);

int __cdecl SortSkillInfoByName(const void* a, const void* b);
int __cdecl SortSpellsForDisplay(const void* a, const void* b);
int __cdecl SortCompanions(const void* a, const void* b);

extern TSGrowableArray<int32_t> s_trackingSpells;

extern int32_t s_spellList[1024];
extern uint32_t s_numSpells;
extern uint32_t s_petSpellList[1024];
extern TSBitField<TSFixedArray<uint32_t> > s_knownSpellBits;

int32_t FindSpellSlotByName(const char* name, const char* subtext, int32_t* petSpell);
int32_t GetSpellIdByName(const char* name, int32_t* petSpell);
bool GetSlotFromLua(lua_State* L, uint32_t* slot, int32_t* petSpell, int32_t* type, int32_t allowName);
bool HasPetSpell(int32_t spellId);
extern TSGrowableArray<int32_t> s_openLockSpells;

#endif
