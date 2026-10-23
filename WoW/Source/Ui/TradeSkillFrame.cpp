#include "TradeSkillFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include <cstdlib>
#include <malloc.h>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include <cstdio>
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/Rec/GemPropertiesRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SpellFocusObjectRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/DB/Rec/TotemCategoryRec.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Ui/QuestTextParser.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// STATIC: 0x00A1B640
static const char s_base64Encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// STATIC: 0x00A1B688
static const uint8_t s_base64Decode[128] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 0, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0,
};

// STATIC: 0x00AD0E00
uint32_t CGTradeSkillInfo::m_subClassFilter = 0xFFFFFFFF;

// STATIC: 0x00AD0E04
uint32_t CGTradeSkillInfo::m_subClassFilterIndex = 0xFFFFFFFF;

// STATIC: 0x00AD0E08
uint32_t CGTradeSkillInfo::m_invTypeFilter = 0xFFFFFFFF;

// STATIC: 0x00AD0E0C
uint32_t CGTradeSkillInfo::m_collapseFilter = 0xFFFFFFFF;

// STATIC: 0x00AD0E10
const char* s_invSlotTokens[27] = { "HEADSLOT", "NECKSLOT", "SHOULDERSLOT", "SHIRTSLOT", "CHESTSLOT", "WAISTSLOT", "LEGSSLOT", "FEETSLOT", "WRISTSLOT", "HANDSSLOT", "FINGER0SLOT", "FINGER1SLOT", "TRINKET0SLOT", "TRINKET1SLOT", "BACKSLOT", "MAINHANDSLOT", "SECONDARYHANDSLOT", "RANGEDSLOT", "TABARDSLOT", "BAGSLOT", "BAGSLOT", "BAGSLOT", "BAGSLOT", "ENCHSLOT_WEAPON", "ENCHSLOT_2HWEAPON", "SHIELDSLOT", "NONEQUIPSLOT" };

// STATIC: 0x00C23598
WOWGUID CGTradeSkillInfo::m_playerGuid;

// STATIC: 0x00C235AC
uint32_t CGTradeSkillInfo::m_itemsPending;

// STATIC: 0x00C235B0
uint32_t CGTradeSkillInfo::m_numDisplayEntries;

// STATIC: 0x00C235B4
uint32_t CGTradeSkillInfo::m_numSubClasses;

// STATIC: 0x00C235C8
int32_t CGTradeSkillInfo::m_showOnlyMakeable;

// STATIC: 0x00C235CC
int32_t CGTradeSkillInfo::m_showOnlySkillUps;

// STATIC: 0x00C235D0
uint32_t CGTradeSkillInfo::m_invSlots;

// STATIC: 0x00C235E8
int32_t CGTradeSkillInfo::m_skillRank;

// STATIC: 0x00C235EC
int32_t CGTradeSkillInfo::m_skillMaxRank;

// STATIC: 0x00C235F0
int32_t CGTradeSkillInfo::m_lastSkillLineID;

// STATIC: 0x00C235F4
TSGrowableArray<TradeSkillInfo*> CGTradeSkillInfo::m_displayList;

// STATIC: 0x00C23604
TSGrowableArray<TradeSkillSubClassInfo*> CGTradeSkillInfo::m_subClassList;

// STATIC: 0x00C235B8
int32_t CGTradeSkillInfo::m_filteredSkills;

// STATIC: 0x00C235BC
char* CGTradeSkillInfo::m_itemNameFilter;

// STATIC: 0x00C235D4
int32_t CGTradeSkillInfo::m_recastSpellID;

// STATIC: 0x00C235DC
int32_t CGTradeSkillInfo::m_pendingRecastSpellID;

// STATIC: 0x00C235C0
int32_t CGTradeSkillInfo::m_itemMinLevelFilter;

// STATIC: 0x00C235C4
int32_t CGTradeSkillInfo::m_itemMaxLevelFilter;

// STATIC: 0x00C235A0
int32_t CGTradeSkillInfo::m_spellID;

// STATIC: 0x00C235A4
int32_t CGTradeSkillInfo::m_skillLineID;
// STATIC: 0x00C235A8
int32_t CGTradeSkillInfo::m_selectedSkill;

// STATIC: 0x00C235D8
uint32_t CGTradeSkillInfo::m_recastTimes;

// STATIC: 0x00C235E0
uint32_t CGTradeSkillInfo::m_pendingRecastTimes;

// STATIC: 0x00C235E4
void* CGTradeSkillInfo::m_linkData;

// VERIFY: c0b1eab0 manual
// OFFSET: 0x005D9FF0
void CGTradeSkillInfo::StopRecast() {
    if (CGTradeSkillInfo::m_recastSpellID || CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }
}

// VERIFY: da469b43 auto
// OFFSET: 0x005DA020
void UpdateTradeSkillRecast(int32_t spellId) {
    if (spellId == CGTradeSkillInfo::m_recastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }

    if (spellId == CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }
}

// VERIFY: e58a5a53 manual
// OFFSET: 0x005DA070
void CGTradeSkillInfo::DoRecast(int32_t spellId) {
    if (!CGTradeSkillInfo::m_recastSpellID) {
        if (!CGTradeSkillInfo::m_pendingRecastSpellID) {
            return;
        }

        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
        return;
    }

    if (!CGTradeSkillInfo::m_recastTimes || CGTradeSkillInfo::m_recastSpellID != spellId) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
        return;
    }

    if (--CGTradeSkillInfo::m_recastTimes) {
        Spell_C::CastSpell(spellId, nullptr, WOWGUID{ 0 }, false);
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    } else {
        CGTradeSkillInfo::m_recastSpellID = 0;
    }
}

// VERIFY: 22405cf4 auto
// OFFSET: 0x005DA0F0
void CGTradeSkillInfo::CommitPendingRecast(int32_t spellId) {
    if (spellId == CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = CGTradeSkillInfo::m_pendingRecastSpellID;
        CGTradeSkillInfo::m_recastTimes = CGTradeSkillInfo::m_pendingRecastTimes;
    }

    CGTradeSkillInfo::m_pendingRecastTimes = 0;
    CGTradeSkillInfo::m_pendingRecastSpellID = 0;
}

// VERIFY: b4e8cb94 auto
// OFFSET: 0x005D9F80
static void TradeSkillListItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (CGTradeSkillInfo::m_itemsPending && !--CGTradeSkillInfo::m_itemsPending) {
        CGTradeSkillInfo::RefreshList(1);
    }
}

// VERIFY: 5bd87f16 manual
// OFFSET: 0x005DA7D0
int32_t CGTradeSkillInfo::GetSubClassIndexFromSkill(uint32_t index) {
    if (index >= CGTradeSkillInfo::m_numDisplayEntries) {
        return -1;
    }

    TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

    if (info->m_spellId >= 0) {
        return -1;
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
        TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[i];

        if (subClass->m_itemClass == info->m_itemClass && subClass->m_itemSubClass == info->m_itemSubClass) {
            return i;
        }
    }

    return -1;
}

// VERIFY: f9844be1 manual
// OFFSET: 0x005DAD10
void CGTradeSkillInfo::ClearItemCallbacks() {
    if (!CGTradeSkillInfo::m_itemsPending) {
        return;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numDisplayEntries; i++) {
        if (g_spellDB.GetRecord(CGTradeSkillInfo::m_displayList.m_data[i]->m_spellId, &spell)) {
            g_itemDBCache.CancelCallback(spell.m_effectItemType[0], TradeSkillListItemCallback, nullptr);
        }
    }

    CGTradeSkillInfo::m_itemsPending = 0;
}

// VERIFY: 4ce9d9f6 manual
// OFFSET: 0x005DCC40
void CGTradeSkillInfo::SetInvTypeFilter(int32_t a1) {
    CGTradeSkillInfo::m_filteredSkills = CGTradeSkillInfo::m_numDisplayEntries;

    if (a1) {
        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
            CGTradeSkillInfo::m_subClassList.m_data[i]->m_shown = (CGTradeSkillInfo::m_subClassFilter & (1 << i)) != 0;
            CGTradeSkillInfo::m_subClassList.m_data[i]->m_collapsed = (CGTradeSkillInfo::m_collapseFilter & (1 << i)) == 0;
        }
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numDisplayEntries; i++) {
        TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[i];
        info->m_visible = 1;

        if (info->m_spellId < 0) {
            continue;
        }

        if (CGTradeSkillInfo::m_numSubClasses) {
            uint32_t index = 0;

            for (uint32_t j = 0; j < CGTradeSkillInfo::m_numSubClasses; j++) {
                TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[j];

                if (info->m_itemClass == subClass->m_itemClass && info->m_itemSubClass == subClass->m_itemSubClass) {
                    index = j;
                    break;
                }
            }

            if (!CGTradeSkillInfo::m_subClassList.m_data[index]->m_shown) {
                info->m_visible = 0;
                CGTradeSkillInfo::m_filteredSkills--;
                continue;
            }
        }

        if (!(CGTradeSkillInfo::m_invTypeFilter & info->m_invTypeMask) || (CGTradeSkillInfo::m_showOnlyMakeable && !info->m_numAvailable) || (CGTradeSkillInfo::m_showOnlySkillUps && info->m_difficulty > TRADESKILL_EASY)) {
            info->m_visible = 0;
            CGTradeSkillInfo::m_filteredSkills--;
            continue;
        }

        if ((CGTradeSkillInfo::m_itemNameFilter || CGTradeSkillInfo::m_itemMinLevelFilter || CGTradeSkillInfo::m_itemMaxLevelFilter) && !Profession_FilterSpell(info->m_spellId, CGTradeSkillInfo::m_itemMinLevelFilter, CGTradeSkillInfo::m_itemMaxLevelFilter, CGTradeSkillInfo::m_itemNameFilter)) {
            info->m_visible = 0;
            CGTradeSkillInfo::m_filteredSkills--;
        }
    }

    if (CGTradeSkillInfo::m_numSubClasses) {
        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numDisplayEntries; i++) {
            TradeSkillInfo* header = CGTradeSkillInfo::m_displayList.m_data[i];

            if (header->m_spellId >= 0) {
                continue;
            }

            int32_t subClass = CGTradeSkillInfo::GetSubClassIndexFromSkill(i);
            bool any = false;

            for (uint32_t j = 0; j < CGTradeSkillInfo::m_numDisplayEntries; j++) {
                if (i == j) {
                    continue;
                }

                TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[j];

                if (header->m_itemClass == info->m_itemClass && header->m_itemSubClass == info->m_itemSubClass && info->m_visible) {
                    any = true;

                    if (CGTradeSkillInfo::m_subClassList.m_data[subClass]->m_collapsed) {
                        info->m_visible = 0;
                        CGTradeSkillInfo::m_filteredSkills--;
                    }
                }
            }

            if (!any) {
                header->m_visible = 0;
                CGTradeSkillInfo::m_filteredSkills--;
            }
        }
    }

    qsort(CGTradeSkillInfo::m_subClassList.m_data, CGTradeSkillInfo::m_numSubClasses, sizeof(TradeSkillSubClassInfo*), QSortSubClasses);
    qsort(CGTradeSkillInfo::m_displayList.m_data, CGTradeSkillInfo::m_numDisplayEntries, sizeof(TradeSkillInfo*), QSortSkills);
}

// VERIFY: 3f9ff382 manual
// OFFSET: 0x005DD5A0
void CGTradeSkillInfo::RefreshList(int32_t full) {
    CGTradeSkillInfo::ClearItemCallbacks();
    CGTradeSkillInfo::m_numDisplayEntries = 0;

    uint32_t savedCount = CGTradeSkillInfo::m_numSubClasses;
    uint32_t saveSize = CGTradeSkillInfo::m_subClassList.Count();

    if (CGTradeSkillInfo::m_numSubClasses < saveSize) {
        saveSize = CGTradeSkillInfo::m_numSubClasses;
    }

    TradeSkillSubClassInfo* saved = static_cast<TradeSkillSubClassInfo*>(alloca(sizeof(TradeSkillSubClassInfo) * saveSize));

    if (full) {
        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
            saved[i].m_itemClass = CGTradeSkillInfo::m_subClassList.m_data[i]->m_itemClass;
            saved[i].m_itemSubClass = CGTradeSkillInfo::m_subClassList.m_data[i]->m_itemSubClass;
            saved[i].m_shown = CGTradeSkillInfo::m_subClassList.m_data[i]->m_shown;
            saved[i].m_collapsed = CGTradeSkillInfo::m_subClassList.m_data[i]->m_collapsed;
        }

        CGTradeSkillInfo::m_numSubClasses = 0;
        CGTradeSkillInfo::m_subClassFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::m_collapseFilter = 0xFFFFFFFF;
    }

    if (!CGTradeSkillInfo::m_skillLineID) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(CGTradeSkillInfo::m_playerGuid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    TSGrowableArray<int32_t>* spells = nullptr;
    TSGrowableArray<const SkillLineAbilityRec*> abilities;
    uint32_t count = 0;

    if (CGTradeSkillInfo::m_linkData) {
        const SkillLineAbilityRec* ability = nullptr;
        uint32_t linkCount = 0;

        if (!SpellTableGetSkillLineLinkInfo(CGTradeSkillInfo::m_skillLineID, ability, linkCount)) {
            return;
        }

        int32_t offset = 0;
        uint8_t bit = 0;

        for (uint32_t i = 0; i < linkCount; i++) {
            if ((1 << bit) & s_base64Decode[static_cast<const uint8_t*>(CGTradeSkillInfo::m_linkData)[offset]]) {
                SpellRec spell;

                if (g_spellDB.GetRecord(ability->m_spell, &spell) && (spell.m_attributes & 0x20)) {
                    if (count == abilities.Count()) {
                        abilities.SetCount(count + 32);
                    }

                    abilities.m_data[count] = ability;
                    count++;
                }
            }

            if (++bit >= 6) {
                bit = 0;
                offset++;
            }

            ability++;
        }
    } else {
        if (!player) {
            return;
        }

        spells = player->GetTradeSkillSpells(CGTradeSkillInfo::m_skillLineID);

        if (!spells) {
            return;
        }

        count = spells->Count();
    }

    for (uint32_t i = CGTradeSkillInfo::m_displayList.Count(); i < count; i++) {
        TradeSkillInfo* info = static_cast<TradeSkillInfo*>(SMemAlloc(sizeof(TradeSkillInfo), __FILE__, __LINE__, 0));
        *CGTradeSkillInfo::m_displayList.New() = info;
    }

    CGTradeSkillInfo::m_numDisplayEntries = count;

    CGPlayer_C* activePlayer = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t skillRank;

    if (activePlayer && CGTradeSkillInfo::m_playerGuid == activePlayer->m_obj->m_guid) {
        skillRank = activePlayer->m_player->m_playerSkillInfo[activePlayer->GetSkillIndex(CGTradeSkillInfo::m_skillLineID)].m_rank;
    } else {
        skillRank = CGTradeSkillInfo::m_skillRank;
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numDisplayEntries; i++) {
        int32_t spellId;
        const SkillLineAbilityRec* ability;

        if (spells) {
            spellId = spells->m_data[i];
            ability = SpellTableLookupAbility(name->m_race, name->m_class, spellId);
        } else {
            ability = abilities.m_data[i];
            spellId = ability->m_spell;
        }

        TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[i];
        info->m_spellId = spellId;
        info->m_difficulty = TRADESKILL_OPTIMAL;
        info->m_skillLineRank = ability->m_trivialSkillLineRankHigh;

        int32_t high = ability->m_trivialSkillLineRankHigh;
        int32_t low = ability->m_trivialSkillLineRankLow;

        if (!low) {
            low = high != 25 ? high - 25 : 0;
        }

        if (skillRank < low) {
            info->m_difficulty = TRADESKILL_OPTIMAL;
        } else if (skillRank < (high + low) / 2) {
            info->m_difficulty = TRADESKILL_MEDIUM;
        } else if (skillRank < high) {
            info->m_difficulty = TRADESKILL_EASY;
        } else {
            info->m_difficulty = TRADESKILL_TRIVIAL;
        }

        int32_t numAvailable = -1;
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            for (uint32_t m = 0; m < 8 && numAvailable; m++) {
                if (!spell.m_reagent[m] || !spell.m_reagentCount[m]) {
                    continue;
                }

                int32_t available = player ? player->m_bag.GetItemTypeCount(spell.m_reagent[m], 0) / spell.m_reagentCount[m] : 0;

                if (numAvailable == -1 || numAvailable >= available) {
                    numAvailable = available;
                }
            }

            if ((spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && spell.m_effectItemType[0]) {
                WOWGUID requester;
                requester.m_guid = static_cast<uint32_t>(spellId) | 0x1FE0000000000000ULL;
                ItemStats_C* stats = g_itemDBCache.GetRecord(spell.m_effectItemType[0], requester, TradeSkillListItemCallback, nullptr, false);

                if (!stats) {
                    CGTradeSkillInfo::m_itemsPending++;
                }

                if (CGTradeSkillInfo::m_itemsPending) {
                    info->m_numAvailable = numAvailable;
                    continue;
                }

                info->m_itemClass = stats->m_class;
                info->m_itemSubClass = stats->m_subClass;
                info->m_itemLevel = stats->m_itemLevel;
                info->m_invTypeMask = s_inventoryTypeSlots[stats->m_inventoryType];

                if (stats->m_inventoryType == 18) {
                    info->m_invTypeMask = 0x80000;
                } else if (stats->m_inventoryType == 11) {
                    info->m_invTypeMask = 0x400;
                } else if (stats->m_inventoryType == 12) {
                    info->m_invTypeMask = 0x1000;
                }

                if (!info->m_invTypeMask) {
                    info->m_invTypeMask = 0x4000000;
                }
            } else {
                info->m_itemClass = -1;
                info->m_itemSubClass = -1;
                info->m_itemLevel = 0;
                info->m_invTypeMask = 0x4000000;

                if (spell.m_equippedItemClass == 2) {
                    info->m_invTypeMask = (spell.m_equippedItemSubclass & 0x1) ? 0x800000 : 0x1000000;
                } else if (spell.m_equippedItemClass == 4) {
                    if (spell.m_equippedItemInvTypes) {
                        info->m_invTypeMask = 0;

                        for (uint32_t j = 0; j < 29; j++) {
                            if ((1 << j) & spell.m_equippedItemInvTypes) {
                                info->m_invTypeMask = s_inventoryTypeSlots[j];

                                if (info->m_invTypeMask & 0x80000) {
                                    info->m_invTypeMask &= 0xFF8FFFFF;
                                } else if (info->m_invTypeMask & 0x400) {
                                    info->m_invTypeMask &= ~0x800;
                                } else if (info->m_invTypeMask & 0x1000) {
                                    info->m_invTypeMask &= ~0x2000;
                                }

                                break;
                            }
                        }

                        if (!info->m_invTypeMask) {
                            info->m_invTypeMask = 0x4000000;
                        }
                    } else if (spell.m_equippedItemSubclass & 0x40) {
                        info->m_invTypeMask = 0x2000000;
                    }
                }
            }

            CGTradeSkillInfo::m_invSlots |= info->m_invTypeMask;

            if (!CGTradeSkillInfo::m_itemsPending && full) {
                uint32_t index;

                for (index = 0; index < CGTradeSkillInfo::m_numSubClasses; index++) {
                    TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[index];

                    if (subClass->m_itemClass == info->m_itemClass && subClass->m_itemSubClass == info->m_itemSubClass) {
                        break;
                    }
                }

                if (index == CGTradeSkillInfo::m_numSubClasses) {
                    if (CGTradeSkillInfo::m_numSubClasses >= CGTradeSkillInfo::m_subClassList.Count()) {
                        TradeSkillSubClassInfo* subClass = static_cast<TradeSkillSubClassInfo*>(SMemAlloc(sizeof(TradeSkillSubClassInfo), __FILE__, __LINE__, 0));
                        *CGTradeSkillInfo::m_subClassList.New() = subClass;
                    }

                    TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[index];
                    subClass->m_itemClass = info->m_itemClass;
                    subClass->m_itemSubClass = info->m_itemSubClass;
                    subClass->m_shown = 1;
                    subClass->m_collapsed = 0;
                    subClass->m_invTypeMask = 0;
                    CGTradeSkillInfo::m_numSubClasses++;
                }

                CGTradeSkillInfo::m_subClassList.m_data[index]->m_invTypeMask |= info->m_invTypeMask;
            }
        }

        info->m_numAvailable = numAvailable;
    }

    if (CGTradeSkillInfo::m_itemsPending) {
        return;
    }

    if (full) {
        for (uint32_t k = 0; k < savedCount; k++) {
            for (uint32_t j = 0; j < CGTradeSkillInfo::m_numSubClasses; j++) {
                TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[j];

                if (subClass->m_itemClass == saved[k].m_itemClass && subClass->m_itemSubClass == saved[k].m_itemSubClass) {
                    subClass->m_shown = saved[k].m_shown;
                    subClass->m_collapsed = saved[k].m_collapsed;
                    break;
                }
            }
        }
    }

    for (uint32_t i = CGTradeSkillInfo::m_displayList.Count(); i < CGTradeSkillInfo::m_numSubClasses + CGTradeSkillInfo::m_numDisplayEntries; i++) {
        TradeSkillInfo* info = static_cast<TradeSkillInfo*>(SMemAlloc(sizeof(TradeSkillInfo), __FILE__, __LINE__, 0));
        *CGTradeSkillInfo::m_displayList.New() = info;
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
        TradeSkillInfo* header = CGTradeSkillInfo::m_displayList.m_data[CGTradeSkillInfo::m_numDisplayEntries + i];
        header->m_spellId = -1;
        header->m_itemClass = CGTradeSkillInfo::m_subClassList.m_data[i]->m_itemClass;
        header->m_itemSubClass = CGTradeSkillInfo::m_subClassList.m_data[i]->m_itemSubClass;
    }

    CGTradeSkillInfo::m_numDisplayEntries += CGTradeSkillInfo::m_numSubClasses;
    CGTradeSkillInfo::SetInvTypeFilter(0);

    if (full) {
        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
            if (!CGTradeSkillInfo::m_subClassList.m_data[i]->m_shown) {
                CGTradeSkillInfo::m_subClassFilter &= ~(1 << i);
            }

            if (CGTradeSkillInfo::m_subClassList.m_data[i]->m_collapsed) {
                CGTradeSkillInfo::m_collapseFilter &= ~(1 << i);
            }
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
}

// VERIFY: 9f22c8a7 manual
// OFFSET: 0x005DCAF0
void CGTradeSkillInfo::ShutdownGame() {
    for (uint32_t i = 0; i < CGTradeSkillInfo::m_displayList.m_count; i++) {
        delete CGTradeSkillInfo::m_displayList.m_data[i];
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_subClassList.m_count; i++) {
        delete CGTradeSkillInfo::m_subClassList.m_data[i];
    }

    CGTradeSkillInfo::m_playerGuid = WOWGUID();
    CGTradeSkillInfo::m_spellID = 0;
    CGTradeSkillInfo::m_skillLineID = 0;
    CGTradeSkillInfo::m_displayList.Clear();
    CGTradeSkillInfo::m_subClassList.Clear();
    CGTradeSkillInfo::m_numDisplayEntries = 0;
    CGTradeSkillInfo::m_filteredSkills = 0;
    CGTradeSkillInfo::m_numSubClasses = 0;

    if (CGTradeSkillInfo::m_itemNameFilter) {
        SMemFree(CGTradeSkillInfo::m_itemNameFilter, __FILE__, __LINE__, 0);
    }

    CGTradeSkillInfo::m_itemNameFilter = nullptr;
    CGTradeSkillInfo::m_lastSkillLineID = 0;
    CGTradeSkillInfo::m_recastSpellID = 0;
    CGTradeSkillInfo::m_pendingRecastSpellID = 0;

    if (CGTradeSkillInfo::m_linkData) {
        SMemFree(CGTradeSkillInfo::m_linkData, __FILE__, __LINE__, 0);
    }

    CGTradeSkillInfo::m_linkData = nullptr;
    CGTradeSkillInfo::m_skillRank = 0;
    CGTradeSkillInfo::m_skillMaxRank = 0;
}

// VERIFY: 9c6c8940 manual
// OFFSET: 0x005DA5A0
void CGTradeSkillInfo::Close() {
    CGTradeSkillInfo::m_spellID = 0;
    CGTradeSkillInfo::m_skillLineID = 0;

    if (CGTradeSkillInfo::m_recastSpellID || CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_CLOSE, nullptr);
}

// VERIFY: 1ba498d5 auto
// OFFSET: 0x005D9F00
const char* GetInventorySlotToken(int32_t slot) {
    switch (slot) {
        case 10:
            return "FINGER0SLOT_UNIQUE";
        case 11:
            return "FINGER1SLOT_UNIQUE";
        case 12:
            return "TRINKET0SLOT_UNIQUE";
        case 13:
            return "TRINKET1SLOT_UNIQUE";
        default:
            return s_invSlotTokens[slot];
    }
}

// VERIFY: 3b6d91ea manual
// OFFSET: 0x005D9F50
static void TradeSkillLinkNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGTradeSkillInfo::SetLink(static_cast<const char*>(param));
    }

    SMemFree(param, __FILE__, __LINE__, 0);
}

// VERIFY: da75f0ab manual
// OFFSET: 0x005D9FA0
void CGTradeSkillInfo::DoTradeSkill(int32_t spellId, uint32_t times) {
    if (spellId == CGTradeSkillInfo::m_recastSpellID) {
        CGTradeSkillInfo::m_recastTimes = times;
    } else {
        CGTradeSkillInfo::m_pendingRecastSpellID = spellId;
        CGTradeSkillInfo::m_pendingRecastTimes = times;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
}

// VERIFY: 4ff75582 auto
// OFFSET: 0x005DA5F0
SkillLineRec* CGTradeSkillInfo::GetSkillLineRec(int32_t spellId, const WOWGUID& guid) {
    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!name) {
        return nullptr;
    }

    SkillLineAbilityRec* ability = SpellTableLookupAbility(name->m_race, name->m_class, spellId);

    if (!ability) {
        return nullptr;
    }

    return g_skillLineDB.GetRecord(ability->m_skillLine);
}

// VERIFY: 51645b21 auto
// OFFSET: 0x005DA670
int32_t CGTradeSkillInfo::GetSelectionIndex() {
    if (!CGTradeSkillInfo::m_selectedSkill) {
        return -1;
    }

    uint32_t i;

    for (i = 0; i < CGTradeSkillInfo::m_numDisplayEntries; i++) {
        if (CGTradeSkillInfo::m_displayList.m_data[i]->m_spellId == CGTradeSkillInfo::m_selectedSkill) {
            break;
        }
    }

    if (i == CGTradeSkillInfo::m_numDisplayEntries) {
        return -1;
    }

    return i;
}

// VERIFY: 734704f2 manual
// OFFSET: 0x005DA6B0
int32_t QSortSubClasses(const void* a, const void* b) {
    const TradeSkillSubClassInfo* subClassA = *static_cast<TradeSkillSubClassInfo* const*>(a);
    const TradeSkillSubClassInfo* subClassB = *static_cast<TradeSkillSubClassInfo* const*>(b);

    if (subClassA->m_itemClass != subClassB->m_itemClass) {
        return subClassA->m_itemClass > subClassB->m_itemClass ? 1 : -1;
    }

    ItemSubClassRec* recA = nullptr;
    ItemSubClassRec* recB = nullptr;

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = g_itemSubClassDB.GetRecordByIndex(i);

        if (rec->m_classID == subClassA->m_itemClass && rec->m_subClassID == subClassA->m_itemSubClass) {
            recA = rec;

            if (recB) {
                break;
            }
        }

        if (rec->m_classID == subClassB->m_itemClass && rec->m_subClassID == subClassB->m_itemSubClass) {
            recB = rec;

            if (recA) {
                break;
            }
        }
    }

    if (recA && recB) {
        const char* nameA = recA->m_verboseName && *recA->m_verboseName ? recA->m_verboseName : recA->m_displayName;
        const char* nameB = recB->m_verboseName && *recB->m_verboseName ? recB->m_verboseName : recB->m_displayName;

        if (nameA && nameB) {
            return SStrSortUTF8I(nameA, nameB, STORM_MAX_STR);
        }

        return 0;
    }

    if (!recA && !recB) {
        return 0;
    }

    return recA ? 1 : -1;
}

// VERIFY: e2741a4a manual
// OFFSET: 0x005DA830
bool CGTradeSkillInfo::IsCollapsedHeader(uint32_t index) {
    if (index >= CGTradeSkillInfo::m_numDisplayEntries) {
        return false;
    }

    TradeSkillInfo* info = CGTradeSkillInfo::m_displayList.m_data[index];

    if (info->m_spellId >= 0) {
        return false;
    }

    for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
        TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[i];

        if (subClass->m_itemClass == info->m_itemClass && subClass->m_itemSubClass == info->m_itemSubClass) {
            return !(CGTradeSkillInfo::m_collapseFilter & (1 << i));
        }
    }

    return false;
}

// VERIFY: c19145e7 manual
// OFFSET: 0x005DACE0
uint32_t CGTradeSkillInfo::GetInvSlotFilter() {
    TradeSkillSubClassInfo* subClass;

    if (CGTradeSkillInfo::m_subClassFilterIndex < CGTradeSkillInfo::m_numSubClasses && (subClass = CGTradeSkillInfo::m_subClassList.m_data[CGTradeSkillInfo::m_subClassFilterIndex])) {
        return subClass->m_invTypeMask & CGTradeSkillInfo::m_invSlots;
    }

    return CGTradeSkillInfo::m_invSlots;
}

// VERIFY: ed9c0233 manual
// OFFSET: 0x005DAE00
int32_t CGTradeSkillInfo::GetSkillMaxRank() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || CGTradeSkillInfo::m_playerGuid != player->m_obj->m_guid) {
        return CGTradeSkillInfo::m_skillMaxRank;
    }

    return player->GetSkillMaxRank(player->GetSkillIndex(CGTradeSkillInfo::m_skillLineID));
}

// VERIFY: adda32be manual
// OFFSET: 0x005DAE60
int32_t QSortSkills(const void* a, const void* b) {
    const TradeSkillInfo* infoA = *static_cast<TradeSkillInfo* const*>(a);
    const TradeSkillInfo* infoB = *static_cast<TradeSkillInfo* const*>(b);

    if (CGTradeSkillInfo::m_numSubClasses) {
        int32_t shownA = 1;
        int32_t shownB = 1;
        uint32_t indexA = 0;
        uint32_t indexB = 0;

        for (uint32_t i = 0; i < CGTradeSkillInfo::m_numSubClasses; i++) {
            TradeSkillSubClassInfo* subClass = CGTradeSkillInfo::m_subClassList.m_data[i];

            if (subClass->m_itemClass == infoA->m_itemClass && subClass->m_itemSubClass == infoA->m_itemSubClass) {
                indexA = i;
                shownA = infoA->m_visible && subClass->m_shown;
            }

            if (subClass->m_itemClass == infoB->m_itemClass && subClass->m_itemSubClass == infoB->m_itemSubClass) {
                indexB = i;
                shownB = infoB->m_visible && subClass->m_shown;
            }
        }

        if (!shownA) {
            return shownB ? 1 : 0;
        }

        if (!shownB) {
            return -1;
        }

        if (indexB != indexA) {
            return indexB < indexA ? 1 : -1;
        }
    }

    if (infoA->m_spellId == -1 || infoB->m_spellId == -1) {
        return infoB->m_spellId == -1 ? 1 : -1;
    }

    SpellRec spellA;
    SpellRec spellB;

    if (!g_spellDB.GetRecord(infoA->m_spellId, &spellA) || !g_spellDB.GetRecord(infoB->m_spellId, &spellB)) {
        return 0;
    }

    if (infoA->m_difficulty != infoB->m_difficulty) {
        return infoA->m_difficulty > infoB->m_difficulty ? 1 : -1;
    }

    if (infoA->m_skillLineRank != infoB->m_skillLineRank) {
        return infoA->m_skillLineRank < infoB->m_skillLineRank ? 1 : -1;
    }

    if (infoA->m_itemLevel != infoB->m_itemLevel) {
        return infoA->m_itemLevel < infoB->m_itemLevel ? 1 : -1;
    }

    return SStrSortUTF8I(spellA.m_name, spellB.m_name, STORM_MAX_STR);
}

// VERIFY: 414f239b manual
// OFFSET: 0x005DB060
bool CGTradeSkillInfo::GetTradeSkillListLink(int32_t spellId, char* buffer, uint32_t bufferSize) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || spell.m_effect[0] != SPELL_EFFECT_TRADE_SKILL) {
        return false;
    }

    WOWGUID guid = player->m_obj->m_guid;
    SkillLineRec* skillLine = CGTradeSkillInfo::GetSkillLineRec(spellId, guid);

    if (!skillLine || !skillLine->m_canLink) {
        return false;
    }

    const SkillLineAbilityRec* ability = nullptr;
    uint32_t count = 0;

    if (!SpellTableGetSkillLineLinkInfo(skillLine->m_id, ability, count)) {
        return false;
    }

    int32_t index = player->GetSkillIndex(skillLine->m_id);
    uint32_t value = player->GetSkillValue(index);
    uint32_t maxRank = player->GetSkillMaxRank(index);
    SStrPrintf(buffer, bufferSize, "|cffffd000|Htrade:%d:%d:%d:%I64X:", spellId, value, maxRank, player->m_obj->m_guid.m_guid);

    uint32_t chars = count / 6;

    if (6 * chars != count) {
        chars++;
    }

    if (SStrLen(buffer) + chars + 1 >= bufferSize) {
        return false;
    }

    int32_t race = player->m_unit->m_unitFieldBytes0[0];
    int32_t classId = player->m_unit->m_unitFieldBytes0[1];
    uint8_t bits = 0;
    uint8_t bit = 0;
    char* out = &buffer[SStrLen(buffer)];

    for (uint32_t i = 0; i < count; i++) {
        if (player->IsSpellKnown(ability->m_spell) && SpellTableMatchRaceClass(race, classId, ability->m_raceMask, ability->m_classMask, 0, 0)) {
            bits |= 1 << bit;
        }

        if (++bit >= 6) {
            *out++ = s_base64Encode[bits];
            bit = 0;
            bits = 0;
        }

        ability++;
    }

    if (bit) {
        *out++ = s_base64Encode[bits];
    }

    *out = 0;
    SStrPrintf(out, buffer + bufferSize - out, "|h[%s]|h|r", spell.m_name);
    return true;
}

// VERIFY: fd75f39d manual
// OFFSET: 0x005DC110
bool Profession_FilterSpell(int32_t spellId, int32_t minLevel, int32_t maxLevel, const char* name) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return false;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(spell.m_effectItemType[0], none, nullptr, nullptr, false);

    if (stats && ((minLevel && static_cast<int32_t>(stats->m_requiredLevel) < minLevel) || (maxLevel && static_cast<int32_t>(stats->m_requiredLevel) > maxLevel))) {
        return false;
    }

    if (!name) {
        return true;
    }

    char text[1024];
    char format[1024];

    if (stats) {
        for (uint32_t i = 0; i < 10; i++) {
            int32_t value = stats->m_statValue[i];

            if (value <= 0 || static_cast<int32_t>(stats->m_statType[i]) < 0) {
                continue;
            }

            SStrCopy(format, FrameScript_GetText(s_itemModTokens[stats->m_statType[i]], -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, value > 0 ? '+' : '-', abs(value));
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }

        int32_t all = stats->m_resistances[1];
        uint32_t school;

        for (school = 2; school <= 6; school++) {
            if (static_cast<int32_t>(stats->m_resistances[school]) != all) {
                break;
            }
        }

        if (school <= 6) {
            for (school = 2; school <= 6; school++) {
                int32_t value = stats->m_resistances[school];

                if (!value) {
                    continue;
                }

                char token[64];
                char schoolName[1024];
                SStrPrintf(token, sizeof(token), "SPELL_SCHOOL%d_CAP", school);
                SStrCopy(schoolName, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(schoolName));
                SStrCopy(format, FrameScript_GetText("ITEM_RESIST_SINGLE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                SStrPrintf(text, sizeof(text), format, value > 0 ? '+' : '-', abs(value), schoolName);
                CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

                if (SStrStr(text, name)) {
                    return true;
                }
            }
        } else if (all) {
            SStrCopy(format, FrameScript_GetText("ITEM_RESIST_ALL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, all > 0 ? '+' : '-', abs(all));
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }

        SpellRec itemSpell;

        for (uint32_t i = 0; i < 5; i++) {
            if (!g_spellDB.GetRecord(stats->m_spellId[i], &itemSpell)) {
                continue;
            }

            SpellParserParseText(&itemSpell, format, sizeof(format), 0, 0, 0, 0, 1, 0);

            switch (stats->m_spellTrigger[i]) {
                case 0:
                    SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), format);
                    break;
                case 1:
                    SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), format);
                    break;
                case 2:
                    SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONPROC", -1, GENDER_NOT_APPLICABLE), format);
                    break;
                default:
                    SStrCopy(text, format, sizeof(text));
                    break;
            }

            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }

        for (uint32_t i = 0; i < 3; i++) {
            uint32_t color = stats->m_socketColor[i];

            if (!color) {
                continue;
            }

            for (uint32_t j = 0; j < 4; j++) {
                if (!((1 << j) & color)) {
                    continue;
                }

                char colorName[32];
                SStrCopy(colorName, s_socketColorNames[j], STORM_MAX_STR);
                SStrUpper(colorName);
                SStrPrintf(format, sizeof(format), "%s%s", "EMPTY_SOCKET_", colorName);
                SStrPrintf(text, sizeof(text), FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE));
                CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

                if (SStrStr(text, name)) {
                    return true;
                }
            }
        }

        GemPropertiesRec* gem;
        SpellItemEnchantmentRec* enchant;

        if (stats->m_gemProperties && (gem = g_gemPropertiesDB.GetRecord(stats->m_gemProperties)) && (enchant = g_spellItemEnchantmentDB.GetRecord(abs(gem->m_enchantID)))) {
            SStrCopy(text, enchant->m_name, sizeof(text));
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }

        if (stats->m_description && *stats->m_description) {
            SStrCopy(text, stats->m_description, STORM_MAX_STR);
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }
    }

    SStrCopy(text, spell.m_name, STORM_MAX_STR);
    CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

    if (SStrStr(text, name)) {
        return true;
    }

    for (uint32_t i = 0; i < 8; i++) {
        if (!spell.m_reagent[i] || !spell.m_reagentCount[i]) {
            continue;
        }

        ItemStats_C* reagent = g_itemDBCache.GetRecord(spell.m_reagent[i], none, nullptr, nullptr, false);

        if (reagent) {
            SStrCopy(text, reagent->m_name[0], STORM_MAX_STR);
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }
    }

    for (uint32_t i = 0; i < 2; i++) {
        if (!spell.m_totem[i]) {
            continue;
        }

        ItemStats_C* totem = g_itemDBCache.GetRecord(spell.m_totem[i], none, nullptr, nullptr, false);

        if (totem) {
            SStrCopy(text, totem->m_name[0], STORM_MAX_STR);
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }
    }

    for (uint32_t i = 0; i < 2; i++) {
        TotemCategoryRec* category;

        if (spell.m_requiredTotemCategoryID[i] && (category = g_totemCategoryDB.GetRecord(spell.m_requiredTotemCategoryID[i]))) {
            SStrCopy(text, category->m_name, STORM_MAX_STR);
            CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

            if (SStrStr(text, name)) {
                return true;
            }
        }
    }

    SpellFocusObjectRec* focus;

    if (spell.m_requiresSpellFocus && (focus = g_spellFocusObjectDB.GetRecord(spell.m_requiresSpellFocus))) {
        SStrCopy(text, focus->m_name, STORM_MAX_STR);
        CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

        if (SStrStr(text, name)) {
            return true;
        }
    }

    if (spell.m_description && *spell.m_description) {
        SpellParserParseText(&spell, text, sizeof(text), 0, 0, 0, 0, 1, 0);
        CaseAccentInsensitiveParse(text, sizeof(text), s_locale);

        if (SStrStr(text, name)) {
            return true;
        }
    }

    return false;
}

// VERIFY: fa78f097 manual
// OFFSET: 0x005DCF00
void CGTradeSkillInfo::SetSubClassFilter(uint32_t filter, uint32_t index) {
    CGTradeSkillInfo::m_subClassFilter = filter;
    CGTradeSkillInfo::m_subClassFilterIndex = index;
    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
}

// VERIFY: 232e3cb9 auto
// OFFSET: 0x005DCF30
void CGTradeSkillInfo::SetInvSlotFilter(uint32_t filter) {
    CGTradeSkillInfo::m_invTypeFilter = filter;
    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
}

// VERIFY: af7343c8 manual
// OFFSET: 0x005DCF60
void CGTradeSkillInfo::SetItemNameFilter(const char* filter) {
    if (filter && !*filter) {
        filter = nullptr;
    }

    if (filter) {
        if (CGTradeSkillInfo::m_itemNameFilter) {
            if (!SStrCmpUTF8I(filter, CGTradeSkillInfo::m_itemNameFilter, STORM_MAX_STR)) {
                return;
            }

            SMemFree(CGTradeSkillInfo::m_itemNameFilter, __FILE__, __LINE__, 0);
        }
    } else {
        if (!CGTradeSkillInfo::m_itemNameFilter) {
            return;
        }

        SMemFree(CGTradeSkillInfo::m_itemNameFilter, __FILE__, __LINE__, 0);
    }

    if (filter) {
        char buffer[1024];
        SStrCopy(buffer, filter, sizeof(buffer));
        CaseAccentInsensitiveParse(buffer, sizeof(buffer), s_locale);
        CGTradeSkillInfo::m_itemNameFilter = SStrDupA(buffer, __FILE__, __LINE__);
    } else {
        CGTradeSkillInfo::m_itemNameFilter = nullptr;
    }

    CGTradeSkillInfo::SetInvTypeFilter(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_FILTER_UPDATE, nullptr);
}

// VERIFY: 7971dd57 auto
// OFFSET: 0x005DD030
void CGTradeSkillInfo::SetItemLevelFilter(int32_t minLevel, int32_t maxLevel) {
    if (minLevel != CGTradeSkillInfo::m_itemMinLevelFilter || maxLevel != CGTradeSkillInfo::m_itemMaxLevelFilter) {
        CGTradeSkillInfo::m_itemMinLevelFilter = minLevel;
        CGTradeSkillInfo::m_itemMaxLevelFilter = maxLevel;
        CGTradeSkillInfo::SetInvTypeFilter(1);
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_UPDATE, nullptr);
    }
}

// VERIFY: 51532fda manual
// OFFSET: 0x005DE120
void CGTradeSkillInfo::SetTradeSkill(int32_t spellId, const WOWGUID& guid, const char* link, int32_t skillRank, int32_t skillMaxRank) {
    SkillLineRec* skillLine = CGTradeSkillInfo::GetSkillLineRec(spellId, guid);

    if (!skillLine) {
        return;
    }

    if (link) {
        if (!skillLine->m_canLink) {
            return;
        }

        const SkillLineAbilityRec* ability = nullptr;
        uint32_t count = 0;

        if (!SpellTableGetSkillLineLinkInfo(skillLine->m_id, ability, count)) {
            return;
        }

        uint32_t chars = count / 6;

        if (6 * chars != count) {
            chars++;
        }

        if (chars != SStrLen(link)) {
            return;
        }
    }

    if (CGTradeSkillInfo::m_recastSpellID || CGTradeSkillInfo::m_pendingRecastSpellID) {
        CGTradeSkillInfo::m_recastSpellID = 0;
        CGTradeSkillInfo::m_pendingRecastSpellID = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_TRADESKILL_RECAST, nullptr);
    }

    CGTradeSkillInfo::ClearItemCallbacks();

    if (!link && !CGTradeSkillInfo::m_linkData && skillLine->m_id == CGTradeSkillInfo::m_skillLineID) {
        CGTradeSkillInfo::Close();
        return;
    }

    CGTradeSkillInfo::m_playerGuid = guid;
    CGTradeSkillInfo::m_spellID = spellId;
    CGTradeSkillInfo::m_skillLineID = skillLine->m_id;
    CGTradeSkillInfo::m_skillRank = skillRank;
    CGTradeSkillInfo::m_skillMaxRank = skillMaxRank;

    if (CGTradeSkillInfo::m_linkData) {
        SMemFree(CGTradeSkillInfo::m_linkData, __FILE__, __LINE__, 0);
    }

    CGTradeSkillInfo::m_linkData = link ? SStrDupA(link, __FILE__, __LINE__) : nullptr;

    if (CGTradeSkillInfo::m_lastSkillLineID != CGTradeSkillInfo::m_skillLineID) {
        if (CGTradeSkillInfo::m_itemNameFilter) {
            SMemFree(CGTradeSkillInfo::m_itemNameFilter, __FILE__, __LINE__, 0);
        }

        CGTradeSkillInfo::m_itemNameFilter = nullptr;
        CGTradeSkillInfo::m_itemMinLevelFilter = 0;
        CGTradeSkillInfo::m_itemMaxLevelFilter = 0;
        CGTradeSkillInfo::m_showOnlyMakeable = 0;
        CGTradeSkillInfo::m_subClassFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::m_invTypeFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::m_collapseFilter = 0xFFFFFFFF;
        CGTradeSkillInfo::m_invSlots = 0;
        CGTradeSkillInfo::m_selectedSkill = 0;
    }

    CGTradeSkillInfo::RefreshList(1);
    CGTradeSkillInfo::m_lastSkillLineID = CGTradeSkillInfo::m_skillLineID;

    if (CGTradeSkillInfo::m_numDisplayEntries) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRADE_SKILL_SHOW, nullptr);
    }
}

// VERIFY: 9ca14fae manual
// OFFSET: 0x005DE300
void CGTradeSkillInfo::SetLink(const char* link) {
    if (!link || !*link) {
        return;
    }

    int32_t spellId = SStrToInt(link);

    if (!spellId) {
        return;
    }

    const char* colon = SStrChr(link, ':');

    if (!colon) {
        return;
    }

    const char* data = colon + 1;
    int32_t skillRank = 0;
    int32_t skillMaxRank = 0;
    WOWGUID guid = WOWGUID();
    sscanf(data, "%d:%d:%I64X:", &skillRank, &skillMaxRank, &guid.m_guid);

    if (!guid.m_guid) {
        return;
    }

    char* copy = SStrDupA(link, __FILE__, __LINE__);
    WOWGUID none = WOWGUID();

    if (g_nameDBCache.GetRecord(guid.m_guid, none, TradeSkillLinkNameCallback, copy, true)) {
        SMemFree(copy, __FILE__, __LINE__, 0);
        const char* bits = SStrChrR(data, ':');

        if (bits) {
            CGTradeSkillInfo::SetTradeSkill(spellId, guid, bits + 1, skillRank, skillMaxRank);
        }
    }
}
