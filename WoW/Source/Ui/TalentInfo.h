#ifndef WOW_SOURCE_UI_TALENTINFO_H
#define WOW_SOURCE_UI_TALENTINFO_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Storm/H/stpl.h"

class CDataStore;

struct lua_State;

class TalentRec;
class CGPlayer_C;
class CGUnit_C;

struct TalentTabInfo {
    /* 0000 */ int32_t m_tabId;
    /* 0004 */ uint32_t m_numTalents;
    /* 0008 */ const TalentRec* m_talents;
};

struct TalentGroupItem_C : public TSHashObject<TalentGroupItem_C, HASHKEY_NONE> {
    /* 0018 */ int32_t m_talentId;
    /* 001C */ int32_t m_rank;
    /* 0020 */ int32_t m_previewRank;

    TalentGroupItem_C();
};

struct TalentGroupTierItem_C : public TSHashObject<TalentGroupTierItem_C, HASHKEY_NONE> {
    /* 0018 */ TalentGroupItem_C* m_item;
};

struct TalentGroupTier_C {
    /* 0000 */ int32_t m_tier;
    /* 0004 */ TSHashTable<TalentGroupTierItem_C, HASHKEY_NONE> m_items;

    TalentGroupTier_C();
    ~TalentGroupTier_C();
};

struct TalentGroupTab_C {
    /* 0000 */ int32_t m_tabId;
    /* 0004 */ uint32_t m_pointsSpent;
    /* 0008 */ int32_t m_previewPointsSpent;
    /* 000C */ TSGrowableArray<TalentGroupTier_C*> m_tiers;

    TalentGroupTab_C();
    ~TalentGroupTab_C();
    TalentGroupTier_C* FindTier(int32_t tier);
    int32_t AddItem(const TalentRec* talent, TalentGroupItem_C* item);
};

class CGTalentGroup_C {
    public:
    /* 0004 */ TSHashTable<TalentGroupItem_C, HASHKEY_NONE> m_items;
    /* 002C */ TSFixedArray<TalentGroupTab_C> m_tabs;
    /* 0038 */ uint32_t m_unspentPoints;
    /* 003C */ uint32_t m_glyphs[6];

    CGTalentGroup_C();
    virtual ~CGTalentGroup_C();
    void Reset();
    uint32_t GetGlyph(uint32_t index);
    TalentGroupTab_C* GetTab(uint32_t index);
    TalentGroupTab_C* GetTabFromID(int32_t tabId);
    TalentGroupTab_C* GetTabFromTalentID(int32_t talentId);
    TalentGroupItem_C* GetTalentItem(int32_t talentId);
    void AddTalentItem(int32_t talentId, int32_t rank);
    void UpdateTabs(const TSFixedArray<TalentTabInfo*>* tabs);
    void ResetPreviewTalentPoints(uint32_t tabIndex);
    void ResetAllPreviewTalentPoints();
    int32_t ValidatePreviewTalentPoints(uint32_t tabIndex, const TalentRec* talent, int32_t points, int32_t* delta, int32_t pet);
    int32_t CanAddPreviewTalentPoints(const TalentRec* talent, int32_t points, int32_t* delta, int32_t pet);
    void AddPreviewTalentPoints(uint32_t tabIndex, const TalentRec* talent, int32_t points, int32_t pet);
};

class CGTalentInfo {
    public:
    static uint32_t m_activeTalentGroup;
    static uint32_t m_numTalentGroups;
    static int32_t m_tabsInitialized;
    static int32_t m_unspentTalentPoints;
    static int32_t m_petTalentTab;
    static TSFixedArray<TalentTabInfo*> m_tabs;
    static TSFixedArray<TalentTabInfo*> m_petTabs;
    static CGTalentGroup_C m_talentGroups[2];
    static CGTalentGroup_C m_petTalentGroup;
    static void EnterWorld();
    static void LeaveWorld();
    static void ShutdownGame();
    static uint32_t GetActiveTalentGroup(int32_t pet);
    static uint32_t GetUnspentTalentPoints(uint32_t index, int32_t pet);
    static CGTalentGroup_C* GetTalentGroup(uint32_t index, int32_t pet);
    static TalentTabInfo* GetTab(uint32_t index, int32_t inspect, int32_t pet);
    static int32_t IsTalentAvailableForPet(CGUnit_C* pet, const TalentRec* talent);
    static TalentGroupItem_C* GetTalentItem(uint32_t group, int32_t talentId, int32_t pet);
    static int32_t GetPreviewPointsSpent(uint32_t group, int32_t pet);
    static void AddPreviewTalentPoints(uint32_t group, uint32_t tab, uint32_t talentIndex, int32_t points, int32_t pet);
    static void ResetPreviewTalentPoints(uint32_t group, uint32_t tab, int32_t pet);
    static void ResetGroupPreviewTalentPoints(uint32_t group, int32_t pet);
    static void RecalculatePreviewPoints(int32_t pet);
    static int32_t SumPreviewPointsSpent(int32_t pet);
    static void ResetTabInfo(int32_t pet);
    static void ResetTalentGroups(int32_t pet);
    static void UpdatePetTabInfo(int32_t family);
    static void UpdateTalentData(CDataStore* msg);
    static void UpdatePetTalentData(CDataStore* msg);
};

void TalentInfoRegisterScriptFunctions();
void TalentInfoUnregisterScriptFunctions();
int32_t TalentInfoIsSpellKnown(int32_t spellId, int32_t isInspect, int32_t isPet);
uint32_t TalentInfoGetScriptGroupIndex(lua_State* L, int32_t index);
CGTalentGroup_C* TalentInfoGetTalentGroup(uint32_t index, int32_t inspect, int32_t pet);
TalentGroupItem_C* TalentInfoGetTalentItem(uint32_t group, int32_t talentId, int32_t inspect, int32_t pet);

class CGTalentInfo_Inspect {
    public:
    static uint32_t m_activeTalentGroup;
    static WOWGUID m_inspectTarget;
    static uint32_t m_numTalentGroups;
    static int32_t m_unspentTalentPoints;
    static TSFixedArray<TalentTabInfo*> m_tabs;
    static CGTalentGroup_C m_talentGroups[2];
    static void EnterWorld();
    static void ShutdownGame();
    static void Reset();
    static void ResetTabInfo();
    static void SetInspectTarget(const WOWGUID& guid);
    static void UpdateTalentData(CDataStore* msg);
};

#endif
