#ifndef WOW_SOURCE_UI_TRADESKILLFRAME_H
#define WOW_SOURCE_UI_TRADESKILLFRAME_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

struct lua_State;
class SkillLineRec;

enum TRADESKILL_CATEGORY : int32_t {
    TRADESKILL_OPTIMAL = 0,
    TRADESKILL_MEDIUM = 1,
    TRADESKILL_EASY = 2,
    TRADESKILL_TRIVIAL = 3,
    NUM_TRADESKILL_CATEGORIES = 4,
};

struct TradeSkillInfo {
    /* 0000 */ int32_t m_spellId;
    /* 0004 */ TRADESKILL_CATEGORY m_difficulty;
    /* 0008 */ int32_t m_itemClass;
    /* 000C */ int32_t m_itemSubClass;
    /* 0010 */ uint32_t m_invTypeMask;
    /* 0014 */ int32_t m_itemLevel;
    /* 0018 */ int32_t m_numAvailable;
    /* 001C */ int32_t m_skillLineRank;
    /* 0020 */ int32_t m_visible;
};

struct TradeSkillSubClassInfo {
    /* 0000 */ int32_t m_itemClass;
    /* 0004 */ int32_t m_itemSubClass;
    /* 0008 */ int32_t m_shown;
    /* 000C */ int32_t m_collapsed;
    /* 0010 */ uint32_t m_invTypeMask;
};

class CGTradeSkillInfo {
    public:
    static void Close();
    static void ShutdownGame();
    static WOWGUID m_playerGuid;
    static uint32_t m_itemsPending;
    static uint32_t m_numDisplayEntries;
    static uint32_t m_numSubClasses;
    static int32_t m_showOnlyMakeable;
    static int32_t m_showOnlySkillUps;
    static uint32_t m_invSlots;
    static int32_t m_skillRank;
    static int32_t m_skillMaxRank;
    static int32_t m_lastSkillLineID;
    static TSGrowableArray<TradeSkillInfo*> m_displayList;
    static TSGrowableArray<TradeSkillSubClassInfo*> m_subClassList;
    static uint32_t m_subClassFilter;
    static uint32_t m_invTypeFilter;
    static uint32_t m_collapseFilter;
    static void ClearItemCallbacks();
    static void SetInvTypeFilter(int32_t a1);
    static int32_t GetSubClassIndexFromSkill(uint32_t index);
    static int32_t m_filteredSkills;
    static int32_t m_spellID;
    static void* m_linkData;
    static int32_t m_skillLineID;
    static char* m_itemNameFilter;
    static int32_t m_itemMinLevelFilter;
    static int32_t m_itemMaxLevelFilter;
    static int32_t m_recastSpellID;
    static int32_t m_pendingRecastSpellID;
    static int32_t m_selectedSkill;
    static uint32_t m_recastTimes;
    static uint32_t m_pendingRecastTimes;
    static void RefreshList(int32_t full);
    static void SetLink(const char* link);
    static void CommitPendingRecast(int32_t spellId);
    static void DoRecast(int32_t spellId);
    static void StopRecast();
    static bool GetTradeSkillListLink(int32_t spellId, char* buffer, uint32_t bufferSize);
    static void SetTradeSkill(int32_t spellId, const WOWGUID& guid, const char* link, int32_t skillRank, int32_t skillMaxRank);
    static uint32_t m_subClassFilterIndex;
    static void DoTradeSkill(int32_t spellId, uint32_t times);
    static SkillLineRec* GetSkillLineRec(int32_t spellId, const WOWGUID& guid);
    static int32_t GetSelectionIndex();
    static bool IsCollapsedHeader(uint32_t index);
    static uint32_t GetInvSlotFilter();
    static int32_t GetSkillMaxRank();
    static void SetSubClassFilter(uint32_t filter, uint32_t index);
    static void SetInvSlotFilter(uint32_t filter);
    static void SetItemNameFilter(const char* filter);
    static void SetItemLevelFilter(int32_t minLevel, int32_t maxLevel);
};

int32_t QSortSubClasses(const void* a, const void* b);
int32_t QSortSkills(const void* a, const void* b);
extern const char* s_invSlotTokens[27];

const char* GetInventorySlotToken(int32_t slot);
bool Profession_FilterSpell(int32_t spellId, int32_t minLevel, int32_t maxLevel, const char* name);
void TradeSkillRegisterScriptFunctions();
void TradeSkillUnregisterScriptFunctions();
void UpdateTradeSkillRecast(int32_t spellId);

#endif
