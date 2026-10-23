#include "TalentInfo.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/PetInfo.h"
#include <cstdlib>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/TalentRec.h"
#include "WoW/Source/DB/Rec/TalentTabRec.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00C21008
WOWGUID CGTalentInfo_Inspect::m_inspectTarget;

// STATIC: 0x00C20FF4
uint32_t CGTalentInfo::m_activeTalentGroup;

// STATIC: 0x00C21004
uint32_t CGTalentInfo_Inspect::m_activeTalentGroup;

// STATIC: 0x00C20FF8
uint32_t CGTalentInfo::m_numTalentGroups;

// STATIC: 0x00C21000
int32_t CGTalentInfo::m_petTalentTab;

// STATIC: 0x00C21018
TSFixedArray<TalentTabInfo*> CGTalentInfo::m_tabs;

// STATIC: 0x00C21024
TSFixedArray<TalentTabInfo*> CGTalentInfo::m_petTabs;

// STATIC: 0x00C21030
CGTalentGroup_C CGTalentInfo::m_talentGroups[2];

// STATIC: 0x00C210D8
CGTalentGroup_C CGTalentInfo::m_petTalentGroup;

// STATIC: 0x00C20FF0
int32_t CGTalentInfo::m_tabsInitialized;

// STATIC: 0x00C20FFC
int32_t CGTalentInfo::m_unspentTalentPoints;

// STATIC: 0x00C21010
uint32_t CGTalentInfo_Inspect::m_numTalentGroups;

// STATIC: 0x00C21014
int32_t CGTalentInfo_Inspect::m_unspentTalentPoints;

// STATIC: 0x00C2112C
TSFixedArray<TalentTabInfo*> CGTalentInfo_Inspect::m_tabs;

// STATIC: 0x00C21138
CGTalentGroup_C CGTalentInfo_Inspect::m_talentGroups[2];

// VERIFY: 05fa25fb auto
// OFFSET: 0x005C57C0
void CGTalentInfo_Inspect::EnterWorld() {
    CGTalentInfo_Inspect::m_inspectTarget = WOWGUID{ 0 };
}

// VERIFY: 1636395f manual
// OFFSET: 0x005C5BE0
int32_t CGTalentInfo::SumPreviewPointsSpent(int32_t pet) {
    int32_t total = 0;

    if (pet) {
        if (CGTalentInfo::m_petTalentTab && CGTalentInfo::m_petTabs.m_count) {
            for (uint32_t i = 0; i < CGTalentInfo::m_petTalentGroup.m_tabs.m_count; i++) {
                total += CGTalentInfo::m_petTalentGroup.m_tabs.m_data[i].m_previewPointsSpent;
            }
        }
    } else {
        for (uint32_t group = 0; group < CGTalentInfo::m_numTalentGroups; group++) {
            int32_t spent = 0;

            for (uint32_t i = 0; i < CGTalentInfo::m_talentGroups[group].m_tabs.m_count; i++) {
                spent += CGTalentInfo::m_talentGroups[group].m_tabs.m_data[i].m_previewPointsSpent;
            }

            total += spent;
        }
    }

    return total;
}

// VERIFY: b993cffb manual
// OFFSET: 0x005C70D0
void CGTalentInfo::RecalculatePreviewPoints(int32_t pet) {
    if (pet) {
        if (CGTalentInfo::m_petTalentTab && CGTalentInfo::m_petTabs.m_count) {
            CGTalentInfo::m_petTalentGroup.ResetAllPreviewTalentPoints();
        }
    } else {
        for (uint32_t group = 0; group < CGTalentInfo::m_numTalentGroups; group++) {
            CGTalentInfo::m_talentGroups[group].ResetAllPreviewTalentPoints();
        }
    }
}

// VERIFY: 9d9e1595 auto
// OFFSET: 0x005C57A0
uint32_t CGTalentInfo::GetActiveTalentGroup(int32_t pet) {
    return pet == 0 ? CGTalentInfo::m_activeTalentGroup : 0;
}

// VERIFY: 34afe472 auto
// OFFSET: 0x005C5B30
TalentGroupTab_C* CGTalentGroup_C::GetTabFromID(int32_t tabId) {
    for (uint32_t i = 0; i < this->m_tabs.m_count; i++) {
        if (this->m_tabs.m_data[i].m_tabId == tabId) {
            return &this->m_tabs.m_data[i];
        }
    }

    return nullptr;
}

// VERIFY: 624e530c manual
// OFFSET: 0x005C5F70
uint32_t CGTalentInfo::GetUnspentTalentPoints(uint32_t index, int32_t pet) {
    uint32_t count;

    if (pet) {
        if (!CGTalentInfo::m_petTalentTab || !CGTalentInfo::m_petTabs.m_count) {
            return 0;
        }

        count = 1;
    } else {
        count = CGTalentInfo::m_numTalentGroups;
    }

    if (index >= count) {
        return 0;
    }

    if (pet) {
        return CGTalentInfo::m_petTalentGroup.m_unspentPoints;
    }

    CGTalentGroup_C* group = &CGTalentInfo::m_talentGroups[index];
    return group ? group->m_unspentPoints : 0;
}

// VERIFY: 73a4e27f auto
// OFFSET: 0x005C5FC0
CGTalentGroup_C* CGTalentInfo::GetTalentGroup(uint32_t index, int32_t pet) {
    uint32_t count;

    if (pet) {
        if (!CGTalentInfo::m_petTalentTab || !CGTalentInfo::m_petTabs.m_count) {
            return nullptr;
        }

        count = 1;
    } else {
        count = CGTalentInfo::m_numTalentGroups;
    }

    if (index >= count) {
        return nullptr;
    }

    return pet ? &CGTalentInfo::m_petTalentGroup : &CGTalentInfo::m_talentGroups[index];
}

// VERIFY: 02243f68 manual
// OFFSET: 0x005C60C0
int32_t TalentInfoIsSpellKnown(int32_t spellId, int32_t isInspect, int32_t isPet) {
    if (isInspect) {
        return isPet == 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    if (isPet) {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));

        if (!unit) {
            return 0;
        }
    }

    return unit->IsSpellKnown(spellId) || unit->IsSpellSuperceded(spellId);
}

// VERIFY: 7cbaa1c1 auto
// OFFSET: 0x005C5750
static int TierCompare(const void* a, const void* b) {
    return (*static_cast<TalentGroupTier_C* const*>(a))->m_tier - (*static_cast<TalentGroupTier_C* const*>(b))->m_tier;
}

// VERIFY: 2584c2bc auto
// OFFSET: 0x005C5770
uint32_t CGTalentGroup_C::GetGlyph(uint32_t index) {
    return index < 6 ? this->m_glyphs[index] : 6;
}

// VERIFY: 577e032d auto
// OFFSET: 0x005C5790
void CGTalentInfo::LeaveWorld() {
    CGTalentInfo::m_tabsInitialized = 0;
    CGTalentInfo::m_petTalentTab = 0;
}

// VERIFY: 0d3d49d3 auto
// OFFSET: 0x005C57D0
uint32_t TalentInfoGetScriptGroupIndex(lua_State* L, int32_t index) {
    if (lua_isnumber(L, index)) {
        return lua_tointeger(L, index) - 1;
    }

    return CGTalentInfo::m_activeTalentGroup;
}

// VERIFY: 09a769af manual
// OFFSET: 0x005C5A20
int32_t CGTalentInfo::IsTalentAvailableForPet(CGUnit_C* pet, const TalentRec* talent) {
    if (!pet || !talent) {
        return 0;
    }

    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(pet->GetCreatureFamily());

    if (!family) {
        return 0;
    }

    int32_t category = family->m_categoryEnumID;
    uint32_t word = 0;
    uint8_t bit = 0;

    if (category >= 0) {
        word = category / 32;

        if (word > 2) {
            return 0;
        }

        bit = category % 32;
    }

    for (uint32_t i = 0; !talent->m_categoryMask[i]; i++) {
        if (i + 1 >= 2) {
            return 1;
        }
    }

    return category < 0 || (talent->m_categoryMask[word] & (1 << bit)) != 0;
}

// VERIFY: 6d864da3 manual
// OFFSET: 0x005C5AC0
TalentGroupTier_C* TalentGroupTab_C::FindTier(int32_t tier) {
    int32_t low = 0;
    int32_t high = this->m_tiers.m_count - 1;

    while (low <= high) {
        uint32_t mid = static_cast<uint32_t>(low + high) >> 1;
        TalentGroupTier_C* entry = this->m_tiers.m_data[mid];

        if (entry) {
            if (entry->m_tier < tier) {
                low = mid + 1;
            } else if (entry->m_tier > tier) {
                high = mid - 1;
            } else {
                return entry;
            }
        }
    }

    return nullptr;
}

// VERIFY: af668ec5 auto
// OFFSET: 0x005C5B00
TalentGroupTab_C* CGTalentGroup_C::GetTab(uint32_t index) {
    return index < this->m_tabs.m_count ? &this->m_tabs.m_data[index] : nullptr;
}

// VERIFY: 70c6be1a auto
// OFFSET: 0x005C5B70
TalentGroupTab_C* CGTalentGroup_C::GetTabFromTalentID(int32_t talentId) {
    TalentRec* talent = g_talentDB.GetRecord(talentId);

    if (!talent) {
        return nullptr;
    }

    return this->GetTabFromID(talent->m_tabID);
}

// VERIFY: 9f0c24f3 auto
// OFFSET: 0x005C5C60
TalentTabInfo* CGTalentInfo::GetTab(uint32_t index, int32_t inspect, int32_t pet) {
    if (inspect) {
        if (!pet && index < CGTalentInfo_Inspect::m_tabs.m_count) {
            return CGTalentInfo_Inspect::m_tabs.m_data[index];
        }
    } else if (pet) {
        if (index < CGTalentInfo::m_petTabs.m_count) {
            return CGTalentInfo::m_petTabs.m_data[index];
        }
    } else if (index < CGTalentInfo::m_tabs.m_count) {
        return CGTalentInfo::m_tabs.m_data[index];
    }

    return nullptr;
}

// VERIFY: 9f151dec manual
// OFFSET: 0x005C6010
int32_t CGTalentInfo::GetPreviewPointsSpent(uint32_t group, int32_t pet) {
    CGTalentGroup_C* talentGroup = CGTalentInfo::GetTalentGroup(group, pet);

    if (!talentGroup) {
        return 0;
    }

    int32_t total = 0;

    for (uint32_t i = 0; i < talentGroup->m_tabs.m_count; i++) {
        total += talentGroup->m_tabs.m_data[i].m_previewPointsSpent;
    }

    return total;
}

// VERIFY: ccdc02e4 manual
// OFFSET: 0x005C6080
CGTalentGroup_C* TalentInfoGetTalentGroup(uint32_t index, int32_t inspect, int32_t pet) {
    if (!inspect) {
        return CGTalentInfo::GetTalentGroup(index, pet);
    }

    if (pet || index >= CGTalentInfo_Inspect::m_numTalentGroups) {
        return nullptr;
    }

    return &CGTalentInfo_Inspect::m_talentGroups[index];
}

// VERIFY: f000d43c manual
// OFFSET: 0x005C64D0
static void BuildTabInfo(CGPlayer_C* player, TSFixedArray<TalentTabInfo*>* tabs, int32_t family) {
    if (!tabs) {
        return;
    }

    int32_t petTalentType = -1;

    if (!player) {
        CreatureFamilyRec* familyRec = g_creatureFamilyDB.GetRecord(family);

        if (!familyRec) {
            return;
        }

        petTalentType = familyRec->m_petTalentType;

        if (petTalentType < 0) {
            return;
        }
    }

    uint32_t numTabs = g_talentTabDB.m_numRecords;
    uint32_t count = 0;

    for (uint32_t i = 0; i < numTabs; i++) {
        TalentTabRec* tab = g_talentTabDB.GetRecordByIndex(i);

        if (player) {
            if (tab->m_raceMask) {
                if (!(tab->m_raceMask & (1 << (player->m_unit->m_unitFieldBytes0[0] - 1)))) {
                    continue;
                }

                if (tab->m_classMask && !(tab->m_classMask & (1 << (player->m_unit->m_unitFieldBytes0[1] - 1)))) {
                    continue;
                }
            } else if (!tab->m_classMask || !(tab->m_classMask & (1 << (player->m_unit->m_unitFieldBytes0[1] - 1)))) {
                continue;
            }
        } else if (!tab->m_categoryEnumID || petTalentType < 0 || !(tab->m_categoryEnumID & (1 << petTalentType))) {
            continue;
        }

        count++;
    }

    for (uint32_t i = 0; i < tabs->m_count; i++) {
        if (tabs->m_data[i]) {
            delete tabs->m_data[i];
        }

        tabs->m_data[i] = nullptr;
    }

    tabs->SetCount(count);

    uint32_t index = 0;

    for (uint32_t i = 0; i < numTabs; i++) {
        TalentTabRec* tab = g_talentTabDB.GetRecordByIndex(i);

        if (player) {
            if (tab->m_raceMask) {
                if (!(tab->m_raceMask & (1 << (player->m_unit->m_unitFieldBytes0[0] - 1)))) {
                    continue;
                }

                if (tab->m_classMask && !(tab->m_classMask & (1 << (player->m_unit->m_unitFieldBytes0[1] - 1)))) {
                    continue;
                }
            } else if (!tab->m_classMask || !(tab->m_classMask & (1 << (player->m_unit->m_unitFieldBytes0[1] - 1)))) {
                continue;
            }
        } else if (!tab->m_categoryEnumID || petTalentType < 0 || !(tab->m_categoryEnumID & (1 << petTalentType))) {
            continue;
        }

        tabs->m_data[index] = static_cast<TalentTabInfo*>(SMemAlloc(sizeof(TalentTabInfo), __FILE__, __LINE__, 0));
        tabs->m_data[index]->m_tabId = tab->m_id;
        tabs->m_data[index]->m_numTalents = 0;
        tabs->m_data[index]->m_talents = nullptr;
        index++;

        if (index == count) {
            break;
        }
    }

    int32_t lastTabId = -1;
    TalentTabInfo* info = nullptr;
    int32_t numTalents = g_talentDB.m_numRecords;

    for (int32_t i = 0; i < numTalents; i++) {
        TalentRec* talent = g_talentDB.GetRecordByIndex(i);
        int32_t tabId = talent->m_tabID;

        if (tabId == lastTabId) {
            if (info) {
                info->m_numTalents++;
            }

            continue;
        }

        info = nullptr;
        lastTabId = tabId;

        for (uint32_t j = 0; j < count; j++) {
            if (tabs->m_data[j]->m_tabId == tabId) {
                info = tabs->m_data[j];
                info->m_talents = talent;
                info->m_numTalents = 1;
                break;
            }
        }
    }
}

// VERIFY: 7e295b15 manual
// OFFSET: 0x005C6880
void CGTalentInfo::ResetTabInfo(int32_t pet) {
    if (pet) {
        for (uint32_t i = 0; i < CGTalentInfo::m_petTabs.m_count; i++) {
            if (CGTalentInfo::m_petTabs.m_data[i]) {
                delete CGTalentInfo::m_petTabs.m_data[i];
            }

            CGTalentInfo::m_petTabs.m_data[i] = nullptr;
        }

        CGTalentInfo::m_petTabs.Clear();
    } else {
        for (uint32_t i = 0; i < CGTalentInfo::m_tabs.m_count; i++) {
            if (CGTalentInfo::m_tabs.m_data[i]) {
                delete CGTalentInfo::m_tabs.m_data[i];
            }

            CGTalentInfo::m_tabs.m_data[i] = nullptr;
        }

        CGTalentInfo::m_tabs.Clear();
    }
}

// VERIFY: 4ade9896 manual
// OFFSET: 0x005C6980
void CGTalentInfo_Inspect::ResetTabInfo() {
    for (uint32_t i = 0; i < CGTalentInfo_Inspect::m_tabs.m_count; i++) {
        if (CGTalentInfo_Inspect::m_tabs.m_data[i]) {
            delete CGTalentInfo_Inspect::m_tabs.m_data[i];
        }

        CGTalentInfo_Inspect::m_tabs.m_data[i] = nullptr;
    }

    CGTalentInfo_Inspect::m_tabs.Clear();
}

// VERIFY: 8281146a manual
// OFFSET: 0x005C6F20
void CGTalentGroup_C::ResetPreviewTalentPoints(uint32_t tabIndex) {
    if (tabIndex >= this->m_tabs.m_count) {
        return;
    }

    for (uint32_t i = 0; i < this->m_tabs.m_data[tabIndex].m_tiers.m_count; i++) {
        TalentGroupTier_C* tier = this->m_tabs.m_data[tabIndex].m_tiers.m_data[i];

        for (TalentGroupTierItem_C* node = tier->m_items.m_fulllist.Head(); node; node = tier->m_items.Next(node)) {
            node->m_item->m_previewRank = node->m_item->m_rank;
        }
    }

    this->m_tabs.m_data[tabIndex].m_previewPointsSpent = 0;
}

// VERIFY: a371eed5 manual
// OFFSET: 0x005C6FB0
void CGTalentGroup_C::ResetAllPreviewTalentPoints() {
    for (TalentGroupItem_C* item = this->m_items.m_fulllist.Head(); item; item = this->m_items.Next(item)) {
        item->m_previewRank = item->m_rank;
    }

    for (uint32_t i = 0; i < this->m_tabs.m_count; i++) {
        this->m_tabs.m_data[i].m_previewPointsSpent = 0;
    }
}

// VERIFY: 7ec3d912 manual
// OFFSET: 0x005C7010
void CGTalentInfo::ResetPreviewTalentPoints(uint32_t group, uint32_t tab, int32_t pet) {
    CGTalentGroup_C* talentGroup = CGTalentInfo::GetTalentGroup(group, pet);

    if (talentGroup) {
        talentGroup->ResetPreviewTalentPoints(tab);
    }
}

// VERIFY: 4a3205f0 auto
// OFFSET: 0x005C7070
void CGTalentInfo::ResetGroupPreviewTalentPoints(uint32_t group, int32_t pet) {
    CGTalentGroup_C* talentGroup = CGTalentInfo::GetTalentGroup(group, pet);

    if (talentGroup) {
        talentGroup->ResetAllPreviewTalentPoints();
    }
}

// VERIFY: 45900bce auto
// OFFSET: 0x005C73B0
TalentGroupItem_C* CGTalentGroup_C::GetTalentItem(int32_t talentId) {
    HASHKEY_NONE key;
    return this->m_items.Ptr(talentId, key);
}

// VERIFY: be725aca manual
// OFFSET: 0x005C73D0
int32_t CGTalentGroup_C::ValidatePreviewTalentPoints(uint32_t tabIndex, const TalentRec* talent, int32_t points, int32_t* delta, int32_t pet) {
    *delta = 0;

    if (!points) {
        return 1;
    }

    if (!talent || tabIndex >= this->m_tabs.m_count) {
        return 0;
    }

    int32_t previewSpent = 0;

    for (uint32_t i = 0; i < this->m_tabs.m_count; i++) {
        previewSpent += this->m_tabs.m_data[i].m_previewPointsSpent;
    }

    if (static_cast<int32_t>(this->m_unspentPoints - previewSpent) <= 0 && points > 0) {
        return 0;
    }

    TalentGroupTab_C* tab = &this->m_tabs.m_data[tabIndex];

    if (!tab) {
        return 0;
    }

    int32_t talentId = talent->m_id;
    int32_t tierId = talent->m_tierID;
    int32_t pointsPerTier = 2 * (pet == 0) + 3;

    if (tab->m_pointsSpent + tab->m_previewPointsSpent < static_cast<uint32_t>(tierId * pointsPerTier)) {
        return 0;
    }

    TalentGroupItem_C* item = this->GetTalentItem(talent->m_id);
    int32_t rank = item ? item->m_rank : -1;
    int32_t previewRank = item ? item->m_previewRank : -1;

    for (uint32_t i = 0; i < 3; i++) {
        int32_t prereqId = talent->m_prereqTalent[i];

        if (prereqId && g_talentDB.GetRecord(prereqId)) {
            TalentGroupItem_C* prereq = this->GetTalentItem(prereqId);

            if (!prereq || prereq->m_previewRank < talent->m_prereqRank[i]) {
                return 0;
            }
        }
    }

    int32_t maxRank = rank;

    for (int32_t i = 8; i >= 0; i--) {
        if (talent->m_spellRank[i]) {
            maxRank = i;
            break;
        }
    }

    int32_t room = maxRank - rank;
    int32_t wanted = previewRank + points - rank;
    int32_t newRank = rank + (wanted < 0 ? 0 : wanted > room ? room : wanted);
    int32_t change = newRank - previewRank;
    *delta = change;

    if (!change) {
        return 0;
    }

    if (change >= 0) {
        return 1;
    }

    for (TalentGroupItem_C* other = this->m_items.m_fulllist.Head(); other; other = this->m_items.Next(other)) {
        if (other->m_previewRank < 0) {
            continue;
        }

        TalentRec* otherTalent = g_talentDB.GetRecord(other->m_talentId);

        if (!otherTalent) {
            continue;
        }

        for (uint32_t i = 0; i < 3; i++) {
            int32_t prereqId = otherTalent->m_prereqTalent[i];

            if (prereqId && g_talentDB.GetRecord(prereqId) == talent && item && change + item->m_previewRank < otherTalent->m_prereqRank[i]) {
                return 0;
            }
        }
    }

    uint32_t numTiers = tab->m_tiers.m_count;

    if (numTiers <= 1) {
        return 1;
    }

    int32_t total = 0;

    for (uint32_t i = 0; i < numTiers; i++) {
        TalentGroupTier_C* tier = tab->m_tiers.m_data[i];
        int32_t tierSpent = 0;

        for (TalentGroupTierItem_C* node = tier->m_items.m_fulllist.Head(); node; node = tier->m_items.Next(node)) {
            int32_t spent = node->m_item->m_previewRank + 1;
            tierSpent += spent;
            total += spent + (node->m_item->m_talentId != talentId ? 0 : change);
        }

        if (tierSpent > 0 && tier->m_tier > tierId && total - tierSpent < pointsPerTier * tier->m_tier) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: ed5f15fd auto
// OFFSET: 0x005C76E0
int32_t CGTalentGroup_C::CanAddPreviewTalentPoints(const TalentRec* talent, int32_t points, int32_t* delta, int32_t pet) {
    *delta = 0;

    if (!points) {
        return 1;
    }

    if (!talent) {
        return 0;
    }

    for (uint32_t i = 0; i < this->m_tabs.m_count; i++) {
        if (this->m_tabs.m_data[i].m_tabId == talent->m_tabID) {
            return this->ValidatePreviewTalentPoints(i, talent, points, delta, pet);
        }
    }

    return 0;
}

// VERIFY: b579a45c manual
// OFFSET: 0x005C7750
TalentGroupItem_C* CGTalentInfo::GetTalentItem(uint32_t group, int32_t talentId, int32_t pet) {
    CGTalentGroup_C* talentGroup = CGTalentInfo::GetTalentGroup(group, pet);

    if (!talentGroup) {
        return nullptr;
    }

    return talentGroup->GetTalentItem(talentId);
}

// VERIFY: bd3e004d manual
// OFFSET: 0x005C77B0
TalentGroupItem_C* TalentInfoGetTalentItem(uint32_t group, int32_t talentId, int32_t inspect, int32_t pet) {
    if (!inspect) {
        return CGTalentInfo::GetTalentItem(group, talentId, pet);
    }

    CGTalentGroup_C* talentGroup = TalentInfoGetTalentGroup(group, inspect, pet);

    if (!talentGroup) {
        return nullptr;
    }

    return talentGroup->GetTalentItem(talentId);
}

// INLINED
TalentGroupItem_C::TalentGroupItem_C() : m_talentId(0), m_rank(-1), m_previewRank(-1) {
}

// VERIFY: ebac1df8 auto
// OFFSET: 0x005C8DE0
TalentGroupTier_C::TalentGroupTier_C() : m_tier(-1) {
    this->m_items.Clear();
}

// VERIFY: e3c846a1 manual
// OFFSET: 0x005C8E10
TalentGroupTier_C::~TalentGroupTier_C() {
    this->m_items.Clear();
}

// INLINED
TalentGroupTab_C::TalentGroupTab_C() : m_tabId(0), m_pointsSpent(0), m_previewPointsSpent(0) {
}

// VERIFY: 6fcfa30a manual
// OFFSET: 0x005C8F90
TalentGroupTab_C::~TalentGroupTab_C() {
    for (uint32_t i = 0; i < this->m_tiers.m_count; i++) {
        if (this->m_tiers.m_data[i]) {
            delete this->m_tiers.m_data[i];
        }
    }

    this->m_tiers.Clear();
}

// VERIFY: ecdccebe manual
// OFFSET: 0x005C9230
int32_t TalentGroupTab_C::AddItem(const TalentRec* talent, TalentGroupItem_C* item) {
    if (!talent || !item) {
        return 0;
    }

    HASHKEY_NONE key;
    TalentGroupTier_C* tier = this->FindTier(talent->m_tierID);

    if (tier) {
        tier->m_items.New(talent->m_columnIndex, key, 0, 0)->m_item = item;
        return 1;
    }

    tier = new (SMemAlloc(sizeof(TalentGroupTier_C), __FILE__, __LINE__, 0)) TalentGroupTier_C();

    if (tier) {
        TalentGroupTier_C** slot = this->m_tiers.New();

        if (slot) {
            *slot = tier;
            tier->m_tier = talent->m_tierID;
            tier->m_items.New(talent->m_columnIndex, key, 0, 0)->m_item = item;
            qsort(this->m_tiers.m_data, this->m_tiers.m_count, sizeof(TalentGroupTier_C*), TierCompare);
            return 1;
        }

        delete tier;
    }

    return 1;
}

// VERIFY: 7b121db1 manual
// OFFSET: 0x005C9310
void CGTalentGroup_C::AddTalentItem(int32_t talentId, int32_t rank) {
    if (rank < 0) {
        return;
    }

    TalentRec* talent = g_talentDB.GetRecord(talentId);

    if (!talent || static_cast<uint32_t>(rank) >= 9 || !talent->m_spellRank[rank]) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(talent->m_spellRank[rank], &spell)) {
        return;
    }

    HASHKEY_NONE key;
    TalentGroupItem_C* item = this->m_items.Ptr(talentId, key);

    if (item) {
        TalentGroupTab_C* tab = this->GetTabFromTalentID(item->m_talentId);

        if (tab) {
            tab->m_previewPointsSpent += item->m_rank - item->m_previewRank;
        }

        item->m_rank = rank;
        item->m_previewRank = rank;
    } else {
        item = this->m_items.New(talentId, key, 0, 0);
        item->m_talentId = talentId;
        item->m_rank = rank;
        item->m_previewRank = rank;
    }
}

// VERIFY: 45a90b6c manual
// OFFSET: 0x005C9450
void CGTalentGroup_C::AddPreviewTalentPoints(uint32_t tabIndex, const TalentRec* talent, int32_t points, int32_t pet) {
    int32_t delta;

    if (!this->ValidatePreviewTalentPoints(tabIndex, talent, points, &delta, pet)) {
        return;
    }

    int32_t talentId = talent->m_id;
    HASHKEY_NONE key;
    TalentGroupItem_C* item = this->m_items.Ptr(talentId, key);

    if (!item) {
        item = this->m_items.New(talentId, key, 0, 0);
        item->m_talentId = talentId;
        this->m_tabs.m_data[tabIndex].AddItem(talent, item);
    }

    item->m_previewRank += delta;
    this->m_tabs.m_data[tabIndex].m_previewPointsSpent += delta;
}

// VERIFY: 709c5981 manual
// OFFSET: 0x005C94F0
void CGTalentInfo::AddPreviewTalentPoints(uint32_t group, uint32_t tab, uint32_t talentIndex, int32_t points, int32_t pet) {
    CGTalentGroup_C* talentGroup = CGTalentInfo::GetTalentGroup(group, pet);

    if (!talentGroup) {
        return;
    }

    TalentTabInfo* info = CGTalentInfo::GetTab(tab, 0, pet);

    if (info && talentIndex < info->m_numTalents) {
        const TalentRec* talent = &info->m_talents[talentIndex];

        if (talent) {
            talentGroup->AddPreviewTalentPoints(tab, talent, points, pet);
        }
    }
}

// VERIFY: cf0844b0 auto
// OFFSET: 0x005C9740
static void ReadTalentGroups(CDataStore* msg, uint32_t* activeGroup, int32_t* unspent, CGTalentGroup_C* groups, uint32_t count, uint32_t* numGroups) {
    if (activeGroup) {
        *activeGroup = 0;
    }

    if (numGroups) {
        *numGroups = 0;
    }

    if (unspent) {
        *unspent = 0;
    }

    if (!msg || !groups) {
        return;
    }

    uint32_t points;
    msg->Get(points);

    if (unspent) {
        *unspent = static_cast<int32_t>(points) < 0 ? 0 : points;
    }

    uint8_t num;
    msg->Get(num);

    if (numGroups) {
        *numGroups = num;
    }

    uint8_t active;
    msg->Get(active);

    if (activeGroup) {
        *activeGroup = active;
    }

    for (uint8_t i = 0; i < num; i++) {
        CGTalentGroup_C* group = i < count ? &groups[i] : nullptr;
        uint8_t numTalents;
        msg->Get(numTalents);

        for (uint8_t j = 0; j < numTalents; j++) {
            uint32_t talentId;
            msg->Get(talentId);
            uint8_t rank;
            msg->Get(rank);

            if (group) {
                group->AddTalentItem(talentId, rank);
            }
        }

        uint8_t numGlyphs;
        msg->Get(numGlyphs);

        for (uint8_t j = 0; j < numGlyphs; j++) {
            uint16_t glyph;
            msg->Get(glyph);

            if (group && j < 6) {
                group->m_glyphs[j] = glyph;
            }
        }
    }
}

// VERIFY: 8694975f auto
// OFFSET: 0x005C98A0
static void ReadPetTalents(CDataStore* msg, CGTalentGroup_C* group) {
    if (!msg || !group) {
        return;
    }

    uint32_t points;
    msg->Get(points);
    group->m_unspentPoints = points;
    uint8_t count;
    msg->Get(count);

    for (uint16_t i = 0; i < count; i++) {
        uint32_t talentId;
        msg->Get(talentId);
        uint8_t rank;
        msg->Get(rank);
        group->AddTalentItem(talentId, rank);
    }
}

// VERIFY: 4b149495 manual
// OFFSET: 0x005C9910
void CGTalentGroup_C::Reset() {
    this->m_items.Clear();
    this->m_tabs.Clear();
    this->m_unspentPoints = 0;

    for (uint32_t i = 0; i < 6; i++) {
        this->m_glyphs[i] = 0;
    }
}

// VERIFY: 26ee2dd9 manual
// OFFSET: 0x005C9960
void CGTalentGroup_C::UpdateTabs(const TSFixedArray<TalentTabInfo*>* tabs) {
    uint32_t count = tabs->m_count;
    this->m_tabs.SetCount(count);

    for (uint32_t i = 0; i < count; i++) {
        this->m_tabs.m_data[i].m_tabId = tabs->m_data[i]->m_tabId;
        this->m_tabs.m_data[i].m_pointsSpent = 0;

        for (uint32_t j = 0; j < tabs->m_data[i]->m_numTalents; j++) {
            const TalentRec* talent = &tabs->m_data[i]->m_talents[j];

            if (!talent) {
                continue;
            }

            HASHKEY_NONE key;
            TalentGroupItem_C* item = this->m_items.Ptr(talent->m_id, key);

            if (item) {
                this->m_tabs.m_data[i].m_pointsSpent += item->m_rank + 1;
                this->m_tabs.m_data[i].AddItem(talent, item);
            }
        }
    }
}

// VERIFY: 0108b1fc manual
// OFFSET: 0x005C9A30
void CGTalentInfo::ResetTalentGroups(int32_t pet) {
    if (pet) {
        CGTalentInfo::m_petTalentGroup.Reset();
        return;
    }

    for (uint32_t i = 0; i < 2; i++) {
        CGTalentInfo::m_talentGroups[i].Reset();
    }

    CGTalentInfo::m_activeTalentGroup = 0;
    CGTalentInfo::m_numTalentGroups = 0;
    CGTalentInfo::m_unspentTalentPoints = 0;
}

// VERIFY: b1d0ceef manual
// OFFSET: 0x005C9AD0
void CGTalentInfo_Inspect::Reset() {
    for (uint32_t i = 0; i < 2; i++) {
        CGTalentInfo_Inspect::m_talentGroups[i].Reset();
    }

    CGTalentInfo_Inspect::m_activeTalentGroup = 0;
    CGTalentInfo_Inspect::m_numTalentGroups = 0;
    CGTalentInfo_Inspect::m_unspentTalentPoints = 0;
}

// VERIFY: e22bac27 manual
// OFFSET: 0x005C9BB0
CGTalentGroup_C::CGTalentGroup_C() {
    this->Reset();
}

// VERIFY: 4ce05bcd manual
// OFFSET: 0x005C9C10
CGTalentGroup_C::~CGTalentGroup_C() {
    this->Reset();
}

// VERIFY: fbf3b704 manual
// OFFSET: 0x005C9CA0
static void UpdateTalentGroups(const TSFixedArray<TalentTabInfo*>* tabs, CGTalentGroup_C* groups, uint32_t count, uint32_t activeGroup, uint32_t numGroups, int32_t unspent) {
    if (!numGroups) {
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        groups[i].UpdateTabs(tabs);
    }

    int32_t total = unspent;

    if (activeGroup < numGroups) {
        CGTalentGroup_C* group = &groups[activeGroup];
        int32_t spent = 0;

        for (uint32_t i = 0; i < group->m_tabs.m_count; i++) {
            spent += group->m_tabs.m_data[i].m_pointsSpent;
        }

        total = spent + unspent;
        group->m_unspentPoints = unspent;
    }

    for (uint32_t i = 0; i < count; i++) {
        if (i == activeGroup) {
            continue;
        }

        int32_t spent = 0;

        for (uint32_t j = 0; j < groups[i].m_tabs.m_count; j++) {
            spent += groups[i].m_tabs.m_data[j].m_pointsSpent;
        }

        int32_t left = total - spent;

        if (left < 0) {
            left = 0;
        }

        groups[i].m_unspentPoints = left;
    }
}

// VERIFY: 0219faf4 manual
// OFFSET: 0x005C9D50
void CGTalentInfo::EnterWorld() {
    if (CGTalentInfo::m_tabsInitialized) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    BuildTabInfo(player, &CGTalentInfo::m_tabs, 0);
    CGTalentInfo::m_tabsInitialized = 1;
    UpdateTalentGroups(&CGTalentInfo::m_tabs, CGTalentInfo::m_talentGroups, 2, CGTalentInfo::m_activeTalentGroup, CGTalentInfo::m_numTalentGroups, CGTalentInfo::m_unspentTalentPoints);
}

// VERIFY: 62894563 auto
// OFFSET: 0x005C9DC0
void CGTalentInfo::ShutdownGame() {
    CGTalentInfo::ResetTabInfo(0);
    CGTalentInfo::ResetTabInfo(1);
    CGTalentInfo::ResetTalentGroups(0);
    CGTalentInfo::m_petTalentGroup.Reset();
}

// VERIFY: 3c64c384 auto
// OFFSET: 0x005C9DF0
void CGTalentInfo::UpdatePetTabInfo(int32_t family) {
    if (family) {
        if (CGTalentInfo::m_petTalentTab != family) {
            BuildTabInfo(nullptr, &CGTalentInfo::m_petTabs, family);
            CGTalentInfo::m_petTalentGroup.UpdateTabs(&CGTalentInfo::m_petTabs);
        }
    } else {
        CGTalentInfo::ResetTabInfo(1);
        CGTalentInfo::m_petTalentGroup.Reset();
    }

    CGTalentInfo::m_petTalentTab = family;
}

// VERIFY: fd8c0d16 manual
// OFFSET: 0x005C9E50
void CGTalentInfo::UpdateTalentData(CDataStore* msg) {
    if (!msg) {
        return;
    }

    uint32_t oldNumGroups = CGTalentInfo::m_numTalentGroups;
    uint32_t oldActiveGroup = CGTalentInfo::m_activeTalentGroup;
    CGTalentInfo::ResetTalentGroups(0);
    ReadTalentGroups(msg, &CGTalentInfo::m_activeTalentGroup, &CGTalentInfo::m_unspentTalentPoints, CGTalentInfo::m_talentGroups, 2, &CGTalentInfo::m_numTalentGroups);

    if (CGTalentInfo::m_tabsInitialized) {
        UpdateTalentGroups(&CGTalentInfo::m_tabs, CGTalentInfo::m_talentGroups, 2, CGTalentInfo::m_activeTalentGroup, CGTalentInfo::m_numTalentGroups, CGTalentInfo::m_unspentTalentPoints);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PLAYER_TALENT_UPDATE, nullptr);

    if (oldActiveGroup != CGTalentInfo::m_activeTalentGroup) {
        uint32_t newActive = CGTalentInfo::m_numTalentGroups ? CGTalentInfo::m_activeTalentGroup : -1;
        uint32_t oldActive = oldNumGroups ? oldActiveGroup : -1;
        FrameScript_SignalEvent(SCRIPT_EVENT_ACTIVE_TALENT_GROUP_CHANGED, "%d%d", newActive + 1, oldActive + 1);
    }
}

// VERIFY: f51325dc auto
// OFFSET: 0x005C9F10
void CGTalentInfo::UpdatePetTalentData(CDataStore* msg) {
    if (!msg) {
        return;
    }

    CGTalentInfo::m_petTalentGroup.Reset();
    ReadPetTalents(msg, &CGTalentInfo::m_petTalentGroup);

    if (CGTalentInfo::m_petTalentTab) {
        CGTalentInfo::m_petTalentGroup.UpdateTabs(&CGTalentInfo::m_petTabs);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_PET_TALENT_UPDATE, nullptr);
}

// VERIFY: 8688bf1f auto
// OFFSET: 0x005C9F60
void CGTalentInfo_Inspect::ShutdownGame() {
    CGTalentInfo_Inspect::ResetTabInfo();
    CGTalentInfo_Inspect::Reset();
}

// VERIFY: a8bdf404 manual
// OFFSET: 0x005C9F70
void CGTalentInfo_Inspect::SetInspectTarget(const WOWGUID& guid) {
    CGTalentInfo_Inspect::ResetTabInfo();
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        BuildTabInfo(player, &CGTalentInfo_Inspect::m_tabs, 0);
        CGTalentInfo_Inspect::m_inspectTarget = guid;
        UpdateTalentGroups(&CGTalentInfo_Inspect::m_tabs, CGTalentInfo_Inspect::m_talentGroups, 2, CGTalentInfo_Inspect::m_activeTalentGroup, CGTalentInfo_Inspect::m_numTalentGroups, CGTalentInfo_Inspect::m_unspentTalentPoints);
    }
}

// VERIFY: 7870c364 auto
// OFFSET: 0x005C9FF0
void CGTalentInfo_Inspect::UpdateTalentData(CDataStore* msg) {
    if (!msg) {
        return;
    }

    CGTalentInfo_Inspect::Reset();
    ReadTalentGroups(msg, &CGTalentInfo_Inspect::m_activeTalentGroup, &CGTalentInfo_Inspect::m_unspentTalentPoints, CGTalentInfo_Inspect::m_talentGroups, 2, &CGTalentInfo_Inspect::m_numTalentGroups);

    if (CGTalentInfo_Inspect::m_inspectTarget.m_guid) {
        UpdateTalentGroups(&CGTalentInfo_Inspect::m_tabs, CGTalentInfo_Inspect::m_talentGroups, 2, CGTalentInfo_Inspect::m_activeTalentGroup, CGTalentInfo_Inspect::m_numTalentGroups, CGTalentInfo_Inspect::m_unspentTalentPoints);
    }
}
