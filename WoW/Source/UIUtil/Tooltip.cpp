#include "Tooltip.h"
#include <cmath>
#include <cstddef>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleStatusBar.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include <typeinfo>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Tempest/CAaRect.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CreatureTypeRec.h"
#include "WoW/Source/DB/Rec/FactionRec.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/GemPropertiesRec.h"
#include "WoW/Source/DB/Rec/ItemSetRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassMaskRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/PowerDisplayRec.h"
#include "WoW/Source/DB/Rec/ScalingStatDistributionRec.h"
#include "WoW/Source/DB/Rec/ScalingStatValuesRec.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentConditionRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellRuneCostRec.h"
#include "WoW/Source/DB/Rec/SpellShapeshiftFormRec.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"
#include "WoW/Source/DB/Rec/TalentRec.h"
#include "WoW/Source/DB/Rec/TalentTabRec.h"
#include "WoW/Source/DB/Rec/TotemCategoryRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Object/CreatureStats.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemNameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/ObjectClient/GuildStats_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/QuestLogLeaderBoardParams.h"
#include "WoW/Source/Ui/QuestTextParser.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/WorldClient/AreaList.h"
#include <cstdio>
#include <cstdlib>
#include "Engine/Source/Frame/CBackdropGenerator.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/DB/Rec/Achievement_CriteriaRec.h"
#include "WoW/Source/DB/Rec/AchievementRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/GlyphPropertiesRec.h"
#include "WoW/Source/DB/Rec/GlyphSlotRec.h"
#include "WoW/Source/DB/Rec/ItemClassRec.h"
#include "WoW/Source/DB/Rec/ItemExtendedCostRec.h"
#include "WoW/Source/DB/Rec/ItemLimitCategoryRec.h"
#include "WoW/Source/DB/Rec/ItemPetFoodRec.h"
#include "WoW/Source/DB/Rec/ItemPurchaseGroupRec.h"
#include "WoW/Source/DB/Rec/ItemRandomPropertiesRec.h"
#include "WoW/Source/DB/Rec/ItemRandomSuffixRec.h"
#include "WoW/Source/DB/Rec/LockRec.h"
#include "WoW/Source/DB/Rec/LockTypeRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/SpellDispelTypeRec.h"
#include "WoW/Source/Object/CGPetition.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/Ui/AchievementInfo.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/ArenaTeamInfo.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/EquipmentManager.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/UIUtil/Cursor.h"

static const float LINE_SPACING = 2.0f;
static const float COLUMN_SPACING = 38.400002f;
static const float XOFFSET = 10.24f;
static const float YOFFSET = 10.24f;
static const float WORDWRAP_MIN_WIDTH = 230.39999f;
static const float TOOLTIP_FADE_TIME = 2.0f;

// STATIC: 0x00AD2AAC
static const char* s_spellLinkColorString = "|cff71d5ff";

// STATIC: 0x00AD2AA4
static const char* s_glyphLinkColorString = "|cff66bbff";

// STATIC: 0x00AD2AB0
static const char* s_talentLinkColorString = "|cff4e96f7";

// STATIC: 0x00C5CF4C
int32_t CGTooltip::s_objectType;

// STATIC: 0x00AD2D08
int32_t CGTooltip::s_metatable = LUA_NOREF;

// STATIC: 0x00AD2A84
static const char* s_itemQualityColorStrings[8] = { "|cff9d9d9d", "|cffffffff", "|cff1eff00", "|cff0070dd", "|cffa335ee", "|cffff8000", "|cffe6cc80", "|cffe6cc80" };

// STATIC: 0x00AD2D2C
uint32_t s_tooltipTitleColor = 0xFFFFD200;

// STATIC: 0x00AD2D30
uint32_t s_tooltipTextColor = 0xFFFFFFFF;

// STATIC: 0x00AD2D34
static uint32_t s_tooltipRedColor = 0xFFFF2020;

// STATIC: 0x00AD2D38
static uint32_t s_tooltipGrayColor = 0xFF808080;

// STATIC: 0x00AD2D40
static uint32_t s_tooltipGreenColor = 0xFF00FF00;

// STATIC: 0x00AD2D70
static uint32_t s_threatStatusColors[5] = { 0xFFFFFFFF, 0xFFB0B0B0, 0xFFFFFF77, 0xFFFF9900, 0xFFFF0000 };

// STATIC: 0x00AD2E6C
static const char* s_classificationTokens[6] = { nullptr, "ELITE", "ELITE", nullptr, nullptr, nullptr };

// STATIC: 0x00AD2D84
static uint32_t s_itemQualityColors[8] = { 0xFF9D9D9D, 0xFFFFFFFF, 0xFF1EFF00, 0xFF0070DD, 0xFFA335EE, 0xFFFF8000, 0xFFE6CC80, 0xFFE6CC80 };

// STATIC: 0x00AD2ADC
const char* CGTooltip::s_objectTypeName = "GameTooltip";

// STATIC: 0x00C5D354
CDataAllocator CGTooltip::s_tooltipHeap(sizeof(CGTooltip), 5);

// STATIC: 0x00AD2A68
static const char* s_questDifficultyColorStrings[5] = {
    "|cffff2020",
    "|cffff8040",
    "|cffffff00",
    "|cff40c040",
    "|cff808080",
};

// STATIC: 0x00AD2D48
static uint32_t s_tooltipBrightRedColor = 0xFFFF0000;

// STATIC: 0x00AD2D50
static uint32_t s_tooltipItemSetColor = 0xFFFFFF97;

// STATIC: 0x00AD2D54
static uint32_t s_skillColors[5] = { 0xFF808080, 0xFF40C040, 0xFFFFFF00, 0xFFFF8040, 0xFFFF2020 };

// STATIC: 0x00AD2DA4
static uint32_t s_tooltipGlyphColor = 0xFF66BBFF;

// STATIC: 0x00AD2DA8
static uint32_t s_tooltipRefundColor = 0xFF00CCFF;

// STATIC: 0x00AD2DAC
static uint32_t s_tooltipTradeColor = 0xFF00CCFF;

// STATIC: 0x00AD2D3C
static uint32_t s_tooltipDisenchantColor = 0xFF88AAFF;

// STATIC: 0x00AD2AC8
static const char* s_colorblindSuffixes[5] = { "", "[+] ", "[++] ", "[+++] ", "[-] " };

// STATIC: 0x00AD2A54
static const char* s_colorStrings[4] = { "|cffffffff", "|cff808080", "|cffff2020", "|cff00ff00" };

// STATIC: 0x00AC7FD8
const char* s_inventoryTypeTokens[29] = { "", "INVTYPE_HEAD", "INVTYPE_NECK", "INVTYPE_SHOULDER", "INVTYPE_BODY", "INVTYPE_CHEST", "INVTYPE_WAIST", "INVTYPE_LEGS", "INVTYPE_FEET", "INVTYPE_WRIST", "INVTYPE_HAND", "INVTYPE_FINGER", "INVTYPE_TRINKET", "INVTYPE_WEAPON", "INVTYPE_SHIELD", "INVTYPE_RANGED", "INVTYPE_CLOAK", "INVTYPE_2HWEAPON", "INVTYPE_BAG", "INVTYPE_TABARD", "INVTYPE_ROBE", "INVTYPE_WEAPONMAINHAND", "INVTYPE_WEAPONOFFHAND", "INVTYPE_HOLDABLE", "INVTYPE_AMMO", "INVTYPE_THROWN", "INVTYPE_RANGEDRIGHT", "INVTYPE_QUIVER", "INVTYPE_RELIC" };

// STATIC: 0x00AD6640
const char* s_itemModTokens[49] = { "ITEM_MOD_MANA", "ITEM_MOD_HEALTH", "", "ITEM_MOD_AGILITY", "ITEM_MOD_STRENGTH", "ITEM_MOD_INTELLECT", "ITEM_MOD_SPIRIT", "ITEM_MOD_STAMINA", "", "", "", "", "ITEM_MOD_DEFENSE_SKILL_RATING", "ITEM_MOD_DODGE_RATING", "ITEM_MOD_PARRY_RATING", "ITEM_MOD_BLOCK_RATING", "ITEM_MOD_HIT_MELEE_RATING", "ITEM_MOD_HIT_RANGED_RATING", "ITEM_MOD_HIT_SPELL_RATING", "ITEM_MOD_CRIT_MELEE_RATING", "ITEM_MOD_CRIT_RANGED_RATING", "ITEM_MOD_CRIT_SPELL_RATING", "ITEM_MOD_HIT_TAKEN_MELEE_RATING", "ITEM_MOD_HIT_TAKEN_RANGED_RATING", "ITEM_MOD_HIT_TAKEN_SPELL_RATING", "ITEM_MOD_CRIT_TAKEN_MELEE_RATING", "ITEM_MOD_CRIT_TAKEN_RANGED_RATING", "ITEM_MOD_CRIT_TAKEN_SPELL_RATING", "ITEM_MOD_HASTE_MELEE_RATING", "ITEM_MOD_HASTE_RANGED_RATING", "ITEM_MOD_HASTE_SPELL_RATING", "ITEM_MOD_HIT_RATING", "ITEM_MOD_CRIT_RATING", "ITEM_MOD_HIT_TAKEN_RATING", "ITEM_MOD_CRIT_TAKEN_RATING", "ITEM_MOD_RESILIENCE_RATING", "ITEM_MOD_HASTE_RATING", "ITEM_MOD_EXPERTISE_RATING", "ITEM_MOD_ATTACK_POWER", "ITEM_MOD_RANGED_ATTACK_POWER", "", "ITEM_MOD_SPELL_HEALING_DONE", "ITEM_MOD_SPELL_DAMAGE_DONE", "ITEM_MOD_MANA_REGENERATION", "ITEM_MOD_ARMOR_PENETRATION_RATING", "ITEM_MOD_SPELL_POWER", "ITEM_MOD_HEALTH_REGEN", "ITEM_MOD_SPELL_PENETRATION", "ITEM_MOD_BLOCK_VALUE" };

// STATIC: 0x00A248F8
static const int32_t s_attackPowerGroup[3] = { 8, 9, 54 };

// STATIC: 0x00A24900
static const int32_t s_manaRegenGroup[3] = { 54, 39, 40 };

// STATIC: 0x00A24904
static const int32_t s_hasteRatingGroup[3] = { 39, 40, 41 };

// STATIC: 0x00A24910
static const int32_t s_critTakenRatingGroup[3] = { 36, 37, 38 };

// STATIC: 0x00A2491C
static const int32_t s_hitTakenRatingGroup[3] = { 33, 34, 35 };

// STATIC: 0x00A24928
static const int32_t s_critRatingGroup[3] = { 30, 31, 32 };

// STATIC: 0x00A24934
static const int32_t s_hitRatingGroup[3] = { 27, 28, 29 };

// STATIC: 0x00A24940
static const int32_t s_spellPowerGroup[2] = { 53, 52 };

// STATIC: 0x00AD2DC8
static const int32_t s_combatRatingStats[25] = { 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, -1, -1, -1, 37, 44 };

// STATIC: 0x00AD2A64
static const char* s_negativeDeltaColor = "|cffff2020";

// STATIC: 0x00AD2EBC
static int32_t s_comparisonIndex = 1;

// STATIC: 0x00AD6704
const char* s_socketColorNames[5] = { "Meta", "Red", "Yellow", "Blue", "Major" };

// STATIC: 0x00A262F0
static const int32_t s_primaryStatOrder[11] = { 4, 3, 7, 5, 6, 1, 0, 8, 9, 2, 10 };

// STATIC: 0x00A25F78
static const int32_t s_spellStatOrder[37] = { 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 41, 43, 44, 45, 46, 47, 48 };

// STATIC: 0x00C5CF48
static const ItemSetRec* s_sortItemSet;

// STATIC: 0x00AD2AB4
static const char* s_gemColorTokens[5] = { nullptr, "META_GEM", "RED_GEM", "YELLOW_GEM", "BLUE_GEM" };

// STATIC: 0x00A246E0
static const int32_t s_gemColorIndices[5] = { -1, 0, 1, 2, 3 };

// STATIC: 0x00AD2A5C
static const char* s_tooltipRedColorString = "|cffff2020";

// STATIC: 0x00AD2EA0
static const char* s_powerCostTokens[7] = { "MANA_COST", "RAGE_COST", "FOCUS_COST", "ENERGY_COST", "HAPPINESS_COST", "RUNE_COST", "RUNIC_POWER_COST" };

// STATIC: 0x00AD2E84
static const char* s_useAllPowerTokens[7] = { "SPELL_USE_ALL_MANA", "SPELL_USE_ALL_RAGE", "SPELL_USE_ALL_FOCUS", "SPELL_USE_ALL_ENERGY", "SPELL_USE_ALL_HAPPINESS", "SPELL_USE_ALL_RUNE", nullptr };

// STATIC: 0x00C5CF50
static char s_itemLink[1024];

// STATIC: 0x00AD2D4C
static uint32_t s_tooltipObjectBackdropColor = 0xFF002E5A;

// STATIC: 0x00AD2DB0
static uint32_t s_frameStackMouseColors[2] = { 0xFFFF9966, 0xFFDDDD88 };

// STATIC: 0x00AD2DB8
static uint32_t s_frameStackColors[2] = { 0xFF6699FF, 0xFF88DDDD };

// STATIC: 0x00AD2DC0
static uint32_t s_frameStackHiddenColors[2] = { 0xFF666666, 0xFF999999 };

// STATIC: 0x00C5D368
static TSGrowableArray<FrameStackInfo> s_frameStack;

// VERIFY: e25b476e auto
// OFFSET: 0x0061B5B0
void CGItemStatsSummary::Create() {
    memset(this->m_stats, 0, sizeof(this->m_stats));
    this->m_dps = 0.0f;
    this->m_flags = 0;
}

// VERIFY: 7ea645b6 manual
// OFFSET: 0x0061B5E0
void CGItemStatsSummary::AddSpellEffects(const SpellRec* spell) {
    if (!(spell->m_attributes & 0x40)) {
        return;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effect[i] != SPELL_EFFECT_APPLY_AURA) {
            continue;
        }

        int32_t value = spell->m_effectBasePoints[i] + 1;
        int32_t misc = spell->m_effectMiscValue[i];

        switch (spell->m_effectAura[i]) {
            case SPELL_AURA_MOD_DAMAGE_DONE:
                this->m_stats[53] += value;
                break;

            case SPELL_AURA_MOD_RESISTANCE:
                for (uint32_t r = 0; r < 7; r++) {
                    if (misc & (1 << r)) {
                        this->m_stats[r] += value;
                    }
                }

                break;

            case SPELL_AURA_MOD_POWER_REGEN:
                this->m_stats[misc + 61] += value;
                break;

            case SPELL_AURA_MOD_ATTACK_POWER:
                this->m_stats[8] += value;
                break;

            case SPELL_AURA_MOD_TARGET_RESISTANCE:
                this->m_stats[58] += -1 - spell->m_effectBasePoints[i];
                break;

            case SPELL_AURA_MOD_RANGED_ATTACK_POWER:
                this->m_stats[9] += value;
                break;

            case SPELL_AURA_MOD_HEALING_DONE:
                this->m_stats[52] += value;
                break;

            case SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT:
                this->m_stats[26] += value;
                break;

            case SPELL_AURA_MOD_SHIELD_BLOCKVALUE:
                this->m_stats[60] += value;
                break;

            case SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT:
                this->m_stats[68] += value;
                break;

            case SPELL_AURA_MOD_RATING:
                for (int32_t r = 0; r < 25; r++) {
                    if ((misc & (1 << r)) && s_combatRatingStats[r] != -1) {
                        this->m_stats[s_combatRatingStats[r] + 11] += value;
                    }
                }

                break;

            default:
                break;
        }
    }
}

// VERIFY: 8aff4a3b auto
// OFFSET: 0x0061B8E0
void CGItemStatsSummary::AddResistances(const ItemStats_C* stats) {
    this->m_flags |= 0x8;

    for (uint32_t i = 0; i < 7; i++) {
        this->m_stats[i] += stats->m_resistances[i];
    }
}

// VERIFY: 1662301b manual
// OFFSET: 0x0061B930
void CGItemStatsSummary::GetSocketFlags(const ItemStats_C* stats) {
    this->m_flags |= 0x10;

    for (uint32_t i = 0; i < 3; i++) {
        uint32_t color = stats->m_socketColor[i];

        if (color & 0x1) {
            this->m_stats[69]++;
        }

        if (color & 0x2) {
            this->m_stats[70]++;
        }

        if (color & 0x4) {
            this->m_stats[71]++;
        }

        if (color & 0x8) {
            this->m_stats[72]++;
        }
    }
}

// VERIFY: 59d9b28c manual
// OFFSET: 0x0061B990
void CGItemStatsSummary::AddStatMods(const ItemStats_C* stats) {
    this->m_flags |= 0x20;

    for (int32_t i = 0; i < stats->m_statsCount; i++) {
        uint32_t type = stats->m_statType[i];
        int32_t value = stats->m_statValue[i];
        this->m_stats[type + 11] += value;

        if (type == 38) {
            this->m_stats[8] += value;
            this->m_stats[9] += value;
        } else if (type == 39) {
            this->m_stats[9] += value;
        }
    }
}

// VERIFY: d44a4a8d manual
// OFFSET: 0x0061B9E0
void CGItemStatsSummary::MakeStatNamesPretty(int32_t stat, const int32_t* group, uint32_t count, int32_t merge) {
    int32_t value = this->m_stats[stat];
    int32_t previous = 0;

    for (uint32_t i = 0; i < count; i++) {
        this->m_stats[group[i]] += value;
        merge = merge && (!i || this->m_stats[group[i]] == previous);
        previous = this->m_stats[group[i]];
    }

    if (merge) {
        this->m_stats[stat] = this->m_stats[group[0]];

        for (uint32_t i = 0; i < count; i++) {
            this->m_stats[group[i]] = 0;
        }
    } else {
        this->m_stats[stat] = 0;
    }
}

// VERIFY: 7fc47552 auto
// OFFSET: 0x0061BA90
void CGItemStatsSummary::CleanStatNames() {
    if (this->m_stats[8] == this->m_stats[10]) {
        this->m_stats[10] = 0;
    }

    this->MakeStatNamesPretty(56, s_spellPowerGroup, 2, 1);
    this->MakeStatNamesPretty(42, s_hitRatingGroup, 3, 1);
    this->MakeStatNamesPretty(43, s_critRatingGroup, 3, 1);
    this->MakeStatNamesPretty(44, s_hitTakenRatingGroup, 3, 1);
    this->MakeStatNamesPretty(45, s_critTakenRatingGroup, 3, 1);
    this->MakeStatNamesPretty(47, s_hasteRatingGroup, 3, 1);
    this->MakeStatNamesPretty(61, s_manaRegenGroup, 1, 1);
    this->MakeStatNamesPretty(7, s_attackPowerGroup, 2, 1);
}

// VERIFY: e5cf160b auto
// OFFSET: 0x0061BB20
char* CGItem_C::GetStatName(uint32_t stat, char* buffer, int32_t size) {
    if (stat - 11 <= 48) {
        SStrPrintf(buffer, size, "%s%s", s_itemModTokens[stat - 11], "_SHORT");
        return buffer;
    }

    if (stat < 7) {
        SStrPrintf(buffer, size, "%s%d%s", "RESISTANCE", stat, "_NAME");
        return buffer;
    }

    if (stat - 69 <= 3) {
        char color[64];
        SStrCopy(color, s_socketColorNames[stat - 69], sizeof(color));
        SStrUpper(color);
        SStrPrintf(buffer, size, "%s%s", "EMPTY_SOCKET_", color);
        return buffer;
    }

    if (stat - 61 <= 6) {
        SStrPrintf(buffer, size, "%s%d%s", "ITEM_MOD_POWER_REGEN", stat - 61, "_SHORT");
        return buffer;
    }

    switch (stat) {
        case 7:
            SStrCopy(buffer, "ITEM_MOD_ATTACK_POWER_SHORT", size);
            break;

        case 8:
            SStrCopy(buffer, "ITEM_MOD_MELEE_ATTACK_POWER_SHORT", size);
            break;

        case 9:
            SStrCopy(buffer, "ITEM_MOD_RANGED_ATTACK_POWER_SHORT", size);
            break;

        case 10:
            SStrCopy(buffer, "ITEM_MOD_FERAL_ATTACK_POWER_SHORT", size);
            break;

        case 60:
            SStrCopy(buffer, "ITEM_MOD_BLOCK_VALUE_SHORT", size);
            break;

        case 68:
            SStrCopy(buffer, "ITEM_MOD_HEALTH_REGEN_SHORT", size);
            break;

        default:
            break;
    }

    return buffer;
}

// VERIFY: 1bfb61f6 manual
// OFFSET: 0x0061BF10
static int32_t HealthUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        CSimpleStatusBar* statusBar = static_cast<CSimpleStatusBar*>(param);
        statusBar->SetMinMaxValues(0.0f, static_cast<float>(static_cast<int32_t>(unit->m_unit->m_unitFieldMaxhealth)));
        statusBar->SetValue(static_cast<float>(unit->m_unit->m_unitFieldHealth));
    }

    return 1;
}

// VERIFY: 30f0882c manual
// OFFSET: 0x0061BF80
static int32_t ObjHealthUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (object) {
        CSimpleStatusBar* statusBar = static_cast<CSimpleStatusBar*>(param);
        statusBar->SetMinMaxValues(0.0f, 1.0f);
        statusBar->SetValue(static_cast<float>(object->m_gameObject->m_animProgress) * 0.0039215689f);
    }

    return 1;
}

// VERIFY: a6e35840 auto
// OFFSET: 0x0061BFF0
const char* CGTooltip::GetAchievementHyperlink(int32_t achievementId, WOWGUID guid, int32_t a3, WowTime time, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
    AchievementRec* achievement = g_achievementDB.GetRecord(achievementId);

    if (!achievement) {
        return "";
    }

    char name[800];
    QuestParserParseText(achievement->m_title, name, sizeof(name), guid, nullptr, 0);
    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Hachievement:%d:%016I64X:%d:%d:%d:%d:%u:%u:%u:%u|h[%s]|h%s", "|cffffff00", achievementId, guid.m_guid, static_cast<uint8_t>(a3), time.m_month + 1, time.m_monthDay + 1, time.m_year, a5, a6, a7, a8, name, "|r");
    return s_itemLink;
}

// INLINED
void CGTooltip::operator delete(void* ptr) {
    if (ptr) {
        CGTooltip::s_tooltipHeap.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 7ff12d51 auto
// OFFSET: 0x00514410
int32_t CGTooltip::GetObjectType() {
    if (!CGTooltip::s_objectType) {
        CGTooltip::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGTooltip::s_objectType;
}

// VERIFY: 4591e9ef auto
// OFFSET: 0x0050F590
void ItemLinkInfo::Clear() {
    this->m_itemId = 0;
    memset(this->m_enchants, 0, sizeof(this->m_enchants));
    memset(this->m_enchantDurations, 0, sizeof(this->m_enchantDurations));
    memset(this->m_enchantCharges, 0, sizeof(this->m_enchantCharges));
    this->m_cooldown = 0;
    this->m_proposedEnchant = 0;
    this->m_creator = WOWGUID{ 0 };
    this->m_giftCreator = WOWGUID{ 0 };
    this->m_propertySeed = 0;
    this->m_randomPropertiesId = 0;
    this->m_wrapped = 0;
    this->m_lockId = 0;
    this->m_maxDurability = 0;
    this->m_durability = 0;
    this->m_level = 0;
    this->m_spellCharges = -1;
    this->m_stackCount = 1;
    this->m_maxStackCount = -1;
}

// VERIFY: 280704e6 auto
// OFFSET: 0x0050F630
int32_t ItemLinkInfo::GetItemIdFromLink(const char* link) {
    this->Clear();
    const char* cursor = SStrStr(link, "item:");

    if (!cursor) {
        return 0;
    }

    cursor += 5;
    this->m_itemId = SStrToInt(cursor);

    if (static_cast<int32_t>(this->m_itemId) <= 0) {
        return 0;
    }

    while (*cursor && *cursor != ':') {
        cursor++;
    }

    if (*cursor) {
        this->m_enchants[0] = SStrToInt(++cursor);
    }

    for (uint32_t i = 2; i < 5; i++) {
        while (*cursor && *cursor != ':') {
            cursor++;
        }

        if (*cursor) {
            this->m_enchants[i] = SStrToInt(++cursor);
        }
    }

    while (*cursor && *cursor != ':') {
        cursor++;
    }

    if (!*cursor) {
        return this->m_itemId;
    }

    this->m_enchants[5] = SStrToInt(++cursor);

    while (*cursor && *cursor != ':') {
        cursor++;
    }

    if (!*cursor) {
        return this->m_itemId;
    }

    this->m_randomPropertiesId = SStrToInt(++cursor);

    while (*cursor && *cursor != ':') {
        cursor++;
    }

    if (!*cursor) {
        return this->m_itemId;
    }

    this->m_propertySeed = SStrToInt(++cursor);

    while (*cursor && *cursor != ':') {
        cursor++;
    }

    if (!*cursor) {
        return this->m_itemId;
    }

    this->m_level = SStrToInt(++cursor);

    return this->m_itemId;
}

// VERIFY: bd6b1c11 auto
// OFFSET: 0x005143A0
CSimpleFrame* CGTooltip::Create(CSimpleFrame* parent) {
    if (CGTooltip::s_tooltipHeap.m_bytesPerData < sizeof(CGTooltip)) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    void* data = CGTooltip::s_tooltipHeap.GetData(0, typeid(CGTooltip).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGTooltip(parent) : nullptr;
}

// VERIFY: bd81ee98 auto
// OFFSET: 0x005143F0
void CGTooltip::SetDefaultAnchor() {
    if (this->m_onTooltipSetDefaultAnchor.m_luaRef) {
        this->RunScript(this->m_onTooltipSetDefaultAnchor, 0, nullptr);
    }
}

// VERIFY: b6747482 auto
// OFFSET: 0x0061A280
FrameScript_Object::ScriptIx* CGTooltip::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnTooltipSetDefaultAnchor", STORM_MAX_STR)) {
        return &this->m_onTooltipSetDefaultAnchor;
    }

    if (!SStrCmpI(name, "OnTooltipCleared", STORM_MAX_STR)) {
        return &this->m_onTooltipCleared;
    }

    if (!SStrCmpI(name, "OnTooltipAddMoney", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,cost,maxcost) %s end";
        return &this->m_onTooltipAddMoney;
    }

    if (!SStrCmpI(name, "OnTooltipSetUnit", STORM_MAX_STR)) {
        return &this->m_onTooltipSetUnit;
    }

    if (!SStrCmpI(name, "OnTooltipSetItem", STORM_MAX_STR)) {
        return &this->m_onTooltipSetItem;
    }

    if (!SStrCmpI(name, "OnTooltipSetSpell", STORM_MAX_STR)) {
        return &this->m_onTooltipSetSpell;
    }

    if (!SStrCmpI(name, "OnTooltipSetQuest", STORM_MAX_STR)) {
        return &this->m_onTooltipSetQuest;
    }

    if (!SStrCmpI(name, "OnTooltipSetAchievement", STORM_MAX_STR)) {
        return &this->m_onTooltipSetAchievement;
    }

    if (!SStrCmpI(name, "OnTooltipSetEquipmentSet", STORM_MAX_STR)) {
        return &this->m_onTooltipSetEquipmentSet;
    }

    if (!SStrCmpI(name, "OnTooltipSetFrameStack", STORM_MAX_STR)) {
        return &this->m_onTooltipSetFrameStack;
    }

    return nullptr;
}

// VERIFY: 38ecbe18 auto
// OFFSET: 0x0061A400
void CGTooltip::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetDefaultAnchor, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipCleared, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipAddMoney, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetUnit, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetItem, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetSpell, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetQuest, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetAchievement, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetEquipmentSet, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTooltipSetFrameStack, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: fd8d870d auto
// OFFSET: 0x0061A510
const CImVector* CGTooltip::GetItemQualityColor(uint32_t quality) {
    if (quality >= sizeof(s_itemQualityColors) / sizeof(s_itemQualityColors[0])) {
        quality = 1;
    }

    return reinterpret_cast<const CImVector*>(&s_itemQualityColors[quality]);
}

// VERIFY: 334f5517 auto
// OFFSET: 0x0061A530
const char* CGTooltip::GetItemQualityColorString(uint32_t quality) {
    if (quality >= sizeof(s_itemQualityColorStrings) / sizeof(s_itemQualityColorStrings[0])) {
        quality = 1;
    }

    return s_itemQualityColorStrings[quality];
}

// VERIFY: 217f6dcb manual
// OFFSET: 0x0061B040
void CGTooltip::ResetPosition(int32_t force) {
    if (!this->m_owner || this->m_anchorType == ANCHOR_PRESERVE) {
        return;
    }

    if (force || this->m_anchorType != ANCHOR_NONE) {
        this->MarkFramePointsUnused();
    }

    switch (this->m_anchorType) {
        case ANCHOR_LEFT:
            this->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this->m_owner, FRAMEPOINT_TOPLEFT, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_RIGHT:
            this->SetPoint(FRAMEPOINT_BOTTOMLEFT, this->m_owner, FRAMEPOINT_TOPRIGHT, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_BOTTOMLEFT:
            this->SetPoint(FRAMEPOINT_TOPRIGHT, this->m_owner, FRAMEPOINT_BOTTOMLEFT, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_BOTTOM:
            this->SetPoint(FRAMEPOINT_TOP, this->m_owner, FRAMEPOINT_BOTTOM, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_BOTTOMRIGHT:
            this->SetPoint(FRAMEPOINT_TOPLEFT, this->m_owner, FRAMEPOINT_BOTTOMRIGHT, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_TOPLEFT:
            this->SetPoint(FRAMEPOINT_BOTTOMLEFT, this->m_owner, FRAMEPOINT_TOPLEFT, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_TOP:
            this->SetPoint(FRAMEPOINT_BOTTOM, this->m_owner, FRAMEPOINT_TOP, this->m_offsetX, this->m_offsetY, 1);
            break;

        case ANCHOR_TOPRIGHT:
            this->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this->m_owner, FRAMEPOINT_TOPRIGHT, this->m_offsetX, this->m_offsetY, 1);
            break;

        default:
            break;
    }
}

// VERIFY: 5e00a252 auto
// OFFSET: 0x0061B290
void CGTooltip::FadeOut() {
    if (this->m_anchorType == ANCHOR_CURSOR || this->m_anchorType == ANCHOR_CURSOR_RIGHT) {
        this->HideThis();
        this->m_fading = 0;
    } else {
        this->m_fading = 1;
        this->m_fadeTime = TOOLTIP_FADE_TIME;
    }
}

// VERIFY: 5d77733c manual
// OFFSET: 0x0061B2E0
void CGTooltip::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    if (this->m_anchorType == ANCHOR_CURSOR) {
        float x = 0.0f;
        float y = 0.0f;
        NDCToDDC(this->m_simpleTop->m_mousePosition.m_x, this->m_simpleTop->m_mousePosition.m_y, &x, &y);
        float scale = 1.0f / this->m_layoutScale;
        x *= scale;
        y *= scale;
        this->SetPoint(FRAMEPOINT_BOTTOM, CSimpleTop::s_instance, FRAMEPOINT_BOTTOMLEFT, x, y, 1);
    } else if (this->m_anchorType == ANCHOR_CURSOR_RIGHT) {
        float x = 0.0f;
        float y = 0.0f;
        NDCToDDC(this->m_simpleTop->m_mousePosition.m_x, this->m_simpleTop->m_mousePosition.m_y, &x, &y);
        float scale = 1.0f / this->m_layoutScale;
        x *= scale;
        y *= scale;
        this->SetPoint(FRAMEPOINT_BOTTOMLEFT, CSimpleTop::s_instance, FRAMEPOINT_BOTTOMLEFT, x + this->m_offsetX, y + this->m_offsetY, 1);
    }

    if (this->m_fading) {
        this->m_fadeTime -= elapsedSec;

        if (this->m_fadeTime <= 0.0f) {
            this->SetFrameAlpha(255);
            this->Hide();
            return;
        }

        float alpha = this->m_fadeTime;

        if (alpha >= 1.0f) {
            alpha = 1.0f;
        }

        this->SetFrameAlpha(static_cast<uint8_t>(static_cast<int32_t>(alpha * 255.0f)));
    }
}

// VERIFY: f1de934c manual
// OFFSET: 0x0061C620
void CGTooltip::ClearTooltip() {
    if (this->m_healthGuid != WOWGUID()) {
        if (this->m_unitGuid != WOWGUID()) {
            ClntObjMgrUnsetObjMirrorHandler(this->m_healthGuid, ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), HealthUpdateHandler, this->m_statusBar);
        } else {
            ClntObjMgrUnsetObjMirrorHandler(this->m_healthGuid, ID_GAMEOBJECT, offsetof(CGGameObjectData, m_animProgress), ObjHealthUpdateHandler, this->m_statusBar);
        }

        this->m_statusBar->Hide();
    }

    this->m_unitGuid = WOWGUID();
    this->m_objectGuid = WOWGUID();
    this->unused_0338 = 0;
    this->unused_033C = 0;
    this->m_corpseGuid.m_guid = 0;
    this->m_healthGuid = WOWGUID();
    this->m_itemId = 0;
    this->m_spellId = 0;
    this->m_questId = 0;
    this->m_achievementId = 0;
    this->m_nextRankSpellId = 0;

    for (uint32_t i = 0; i < this->m_numTextures; i++) {
        uint32_t line = this->m_textureLines[i];
        CSimpleFontString** left = &this->m_leftStrings.m_data[line];
        this->m_leftStrings.m_data[line]->SetPoint(FRAMEPOINT_TOPLEFT, left[-1], FRAMEPOINT_BOTTOMLEFT, 0.0f, NDCToDDCWidth(-2.0f / (CoordinateGetAspectCompensation() * 1024.0f)), 1);

        if (line < this->m_maxLines - 1 && this->m_leftStrings.m_data[line + 1]) {
            CSimpleFontString** strings = &this->m_leftStrings.m_data[line];
            strings[1]->SetPoint(FRAMEPOINT_TOPLEFT, strings[0], FRAMEPOINT_BOTTOMLEFT, 0.0f, 0.0f, 1);
        }

        this->m_textures[i]->Hide();
    }

    bool hadLines = this->m_numLines > 0;
    this->m_numTextures = 0;

    if (this->m_numLines) {
        for (uint32_t i = 0; i < this->m_numLines; i++) {
            this->m_leftStrings.m_data[i]->SetWidth(0.0f);
            this->m_leftStrings.m_data[i]->SetText("", 1);
            this->m_leftStrings.m_data[i]->Hide();
            this->m_rightStrings.m_data[i]->SetText("", 1);
            this->m_rightStrings.m_data[i]->Hide();
            this->m_lineWrap.m_data[i] = 0;
        }

        this->m_numLines = 0;
    }

    if (!this->m_minimumWidthForce) {
        this->m_minimumWidth = 0.0f;
    }

    if (hadLines && this->m_onTooltipCleared.m_luaRef) {
        this->RunScript(this->m_onTooltipCleared, 0, nullptr);
    }
}

// VERIFY: 994304f7 manual
// OFFSET: 0x0061CAF0
void CGTooltip::CalculateSize() {
    if (!this->m_owner) {
        return;
    }

    float width = NDCToDDCWidth(this->m_minimumWidth / (CoordinateGetAspectCompensation() * 1024.0f));

    if (!this->m_minimumWidthForce) {
        for (uint32_t i = 0; i < this->m_numLines; i++) {
            if (this->m_lineWrap.m_data[i]) {
                continue;
            }

            float lineWidth = 0.0f;

            if (this->m_leftStrings.m_data[i]->m_shown && this->m_rightStrings.m_data[i]->m_shown) {
                lineWidth = NDCToDDCWidth(COLUMN_SPACING / (CoordinateGetAspectCompensation() * 1024.0f));
            }

            if (this->m_leftStrings.m_data[i]->m_shown) {
                lineWidth = this->m_leftStrings.m_data[i]->GetWidth() + lineWidth;
            }

            if (this->m_rightStrings.m_data[i]->m_shown) {
                lineWidth = this->m_rightStrings.m_data[i]->GetWidth() + lineWidth;
            }

            for (uint32_t j = 0; j < this->m_numTextures; j++) {
                if (this->m_textureLines[j] == i) {
                    lineWidth = this->m_textures[j]->GetWidth() + lineWidth + 0.0049999999f;
                    break;
                }
            }

            if (lineWidth > width) {
                width = lineWidth;
            }
        }
    }

    for (uint32_t i = 0; i < this->m_numLines; i++) {
        if (!this->m_minimumWidthForce && !this->m_lineWrap.m_data[i]) {
            continue;
        }

        float stringWidth = this->m_leftStrings.m_data[i]->GetStringWidth();

        if (stringWidth >= NDCToDDCWidth(WORDWRAP_MIN_WIDTH / (CoordinateGetAspectCompensation() * 1024.0f))) {
            stringWidth = NDCToDDCWidth(WORDWRAP_MIN_WIDTH / (CoordinateGetAspectCompensation() * 1024.0f));
        }

        float lineWidth = stringWidth;

        if (width < lineWidth) {
            CSimpleFontString* left = this->m_leftStrings.m_data[i];
            const char* text = left->m_text && *left->m_text ? left->m_text : nullptr;
            uint32_t lineStarts[30];
            uint32_t count = left->WrapText(text, lineWidth, lineStarts, 30) >= 30 ? 30 : this->m_leftStrings.m_data[i]->WrapText(text, stringWidth, lineStarts, 30);

            lineWidth = 0.0f;
            stringWidth = 0.0f;

            for (uint32_t j = 0; j < count; j++) {
                uint32_t start = lineStarts[j];
                uint32_t end = j >= count - 1 ? SStrLen(text) : lineStarts[j + 1];
                lineWidth = this->m_leftStrings.m_data[i]->GetTextWidth(&text[start], end - start);

                if (stringWidth >= lineWidth) {
                    lineWidth = stringWidth;
                } else {
                    stringWidth = lineWidth;
                }
            }
        }

        if (width > lineWidth) {
            lineWidth = width;
        } else {
            width = lineWidth;
        }

        this->m_leftStrings.m_data[i]->SetWidth(lineWidth);
    }

    for (uint32_t i = 0; i < this->m_numLines; i++) {
        float offset = width;

        if (!this->m_rightStrings.m_data[i]->m_shown) {
            continue;
        }

        for (uint32_t j = 0; j < this->m_numTextures; j++) {
            if (this->m_textureLines[j] == i) {
                offset = width - this->m_textures[j]->GetWidth() - 0.0049999999f;
                break;
            }
        }

        this->m_rightStrings.m_data[i]->SetPoint(FRAMEPOINT_RIGHT, this->m_leftStrings.m_data[i], FRAMEPOINT_LEFT, offset, 0.0f, 1);
    }

    float height = 0.0f;

    for (uint32_t i = 0; i < this->m_numLines; i++) {
        if (!this->m_leftStrings.m_data[i]->m_shown) {
            continue;
        }

        if (height != 0.0f) {
            height = NDCToDDCWidth(LINE_SPACING / (CoordinateGetAspectCompensation() * 1024.0f)) + height;
        }

        height = this->m_leftStrings.m_data[i]->GetHeight() + height;
    }

    float borderX = NDCToDDCWidth(XOFFSET / (CoordinateGetAspectCompensation() * 1024.0f));
    float totalWidth = borderX + borderX + this->m_padding + width;
    float borderY = NDCToDDCWidth(YOFFSET / (CoordinateGetAspectCompensation() * 1024.0f));
    float totalHeight = borderY + borderY + height;
    this->SetWidth(totalWidth);
    this->SetHeight(totalHeight);

    if (this->m_anchorType != ANCHOR_CURSOR && this->m_anchorType != ANCHOR_CURSOR_RIGHT) {
        this->ResetPosition(0);
    }

    this->Resize(1);
}

// VERIFY: 49557c47 auto
// OFFSET: 0x0061CFF0
int32_t CGTooltip::ShowThis() {
    if (!this->m_owner || !this->m_numLines) {
        this->Hide();
        return 0;
    }

    this->SetFrameAlpha(255);
    this->m_fading = 0;
    this->CalculateSize();
    return CSimpleFrame::ShowThis();
}

// VERIFY: 14f9b7d7 manual
// OFFSET: 0x0061DEE0
CGTooltip::CGTooltip(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_itemLinkInfo.Clear();
    this->m_padding = 0.0f;
    this->m_minimumWidth = 0.0f;
    this->m_owner = nullptr;
    this->m_offsetX = 0.0f;
    this->m_numLines = 0;
    this->m_offsetY = 0.0f;
    this->m_maxLines = 0;
    this->m_numTextures = 0;
    this->m_unitGuid = WOWGUID();
    this->m_objectGuid = WOWGUID();
    this->unused_0338 = 0;
    this->unused_033C = 0;
    this->m_itemGuid = WOWGUID();
    this->m_corpseGuid.m_guid = 0;
    this->m_healthGuid = WOWGUID();
    this->m_itemId = 0;
    this->m_spellId = 0;
    this->m_questId = 0;
    this->m_achievementId = 0;
    this->m_nextRankSpellId = 0;
    this->m_itemNameOnly = 0;
    this->m_itemSocketPreview = 0;
    this->m_itemCompare = 0;
    this->m_itemOwner = WOWGUID{ 0 };
    this->m_pendingQueries = 0;
    this->m_fading = 0;
    this->m_minimumWidthForce = 0;
    this->m_itemMinimumLevel = 0;
    this->m_itemDisplayedLevel = 0;
    this->m_itemSkipSellPrice = 0;
    this->m_statusBar = nullptr;
}

// VERIFY: c7c9afc0 auto
// OFFSET: 0x0061E0B0
const char* CGTooltip::GetObjectTypeName() {
    return CGTooltip::s_objectTypeName;
}

// VERIFY: e430a47c auto
// OFFSET: 0x0061E0C0
int32_t CGTooltip::GetScriptMetaTable() {
    return CGTooltip::s_metatable;
}

// VERIFY: f9970628 auto
// OFFSET: [0x0061E0D0, 0x0061E160, 0x0061F910]
CGTooltip::~CGTooltip() {
    if (this->m_healthGuid != WOWGUID() && ClntObjMgrGetCurrent()) {
        if (this->m_unitGuid != WOWGUID()) {
            ClntObjMgrUnsetObjMirrorHandler(this->m_healthGuid, ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), HealthUpdateHandler, this->m_statusBar);
        } else {
            ClntObjMgrUnsetObjMirrorHandler(this->m_healthGuid, ID_GAMEOBJECT, offsetof(CGGameObjectData, m_animProgress), ObjHealthUpdateHandler, this->m_statusBar);
        }
    }
}

// VERIFY: 870e83bb auto
// OFFSET: 0x0061E0E0
bool CGTooltip::IsA(int32_t type) {
    return type == CGTooltip::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 92061154 auto
// OFFSET: 0x0061E120
bool CGTooltip::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGTooltip::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 6c7a9cf3 auto
// OFFSET: 0x0061EA00
void CGTooltip::SetOwner(CSimpleFrame* owner, TOOLTIP_ANCHORPOINT anchorType, float offsetX, float offsetY) {
    this->ClearTooltip();
    this->SetFrameAlpha(255);
    this->m_fading = 0;

    if (owner == this->m_owner && anchorType == this->m_anchorType && fabs(offsetX - this->m_offsetX) < 0.00000023841858f && fabs(offsetY - this->m_offsetY) < 0.00000023841858f) {
        return;
    }

    this->m_offsetX = offsetX;
    this->m_owner = owner;
    this->m_offsetY = offsetY;
    this->m_anchorType = ANCHOR_NONE;

    if (owner) {
        this->m_anchorType = anchorType;
    }

    this->ResetPosition(1);
}

// VERIFY: 499594c2 auto
// OFFSET: 0x0061EB20
int32_t CGTooltip::HideThis() {
    this->SetOwner(nullptr, ANCHOR_LEFT, 0.0f, 0.0f);
    return CSimpleFrame::HideThis();
}

// VERIFY: 82cc22f3 auto
// OFFSET: 0x0061F2B0
void CGItemStatsSummary::AddEnchantEffect(int32_t type, int32_t arg, int32_t amount) {
    switch (type) {
        case 3: {
            SpellRec spell;

            if (g_spellDB.GetRecord(arg, &spell)) {
                this->AddSpellEffects(&spell);
            }

            break;
        }

        case 4:
            this->m_stats[arg] += amount;
            break;

        case 5:
            this->m_stats[arg + 11] += amount;

            if (arg == 38) {
                this->m_stats[8] += amount;
                this->m_stats[9] += amount;
            } else if (arg == 39) {
                this->m_stats[9] += amount;
            }

            break;
    }
}

// VERIFY: a73b50c1 auto
// OFFSET: 0x0061F350
void CGItemStatsSummary::ScanEnchantSpells(const ItemStats_C* stats) {
    this->m_flags |= 0x1;

    for (uint32_t i = 0; i < 5; i++) {
        if (static_cast<int32_t>(stats->m_spellId[i]) <= 0) {
            continue;
        }

        SpellRec spell;

        if (g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
            this->AddSpellEffects(&spell);
        }
    }
}

// VERIFY: 6294a102 manual
// OFFSET: 0x0061F410
void CGItemStatsSummary::AccumulateDpsStat(const ItemStats_C* stats) {
    this->m_flags |= 0x4;
    float mainAverage = (stats->m_damageMax[0] + stats->m_damageMin[0]) * 0.5f;
    this->m_dps = mainAverage / (stats->m_delay * 0.001f) + this->m_dps;
    float offAverage = 0.5f * (stats->m_damageMax[1] + stats->m_damageMin[1]);
    this->m_dps = offAverage / (0.001f * stats->m_delay) + this->m_dps;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_unit->m_unitFieldBytes0[1] == 11) {
        this->m_stats[10] += this->m_stats[8] + GetFeralAttackPower(stats, mainAverage + offAverage);
    }
}

// VERIFY: 4f967d8e auto
// OFFSET: 0x0061F4C0
void CGItemStatsSummary::AccumulateStats(const ItemStats_C* stats) {
    this->AddStatMods(stats);
    this->ScanEnchantSpells(stats);
    this->m_flags |= 0x8;

    for (uint32_t i = 0; i < 7; i++) {
        this->m_stats[i] += stats->m_resistances[i];
    }

    this->GetSocketFlags(stats);
    this->m_flags |= 0x40;
    this->m_stats[60] += stats->m_block;
    this->AccumulateDpsStat(stats);
}

// VERIFY: b9de4bb7 manual
// OFFSET: 0x0061F550
void CGItemStatsSummary::FillOutScalingItem(const ItemStats_C* stats, int32_t level) {
    this->m_flags |= 0x80;

    if (!level) {
        level = CGTooltip::GetDisplayedScalingItemLevel(stats, nullptr, WOWGUID(), 0);
    }

    ScalingStatDistributionRec* distribution = g_scalingStatDistributionDB.GetRecord(stats->m_scalingStatDistribution);
    ScalingStatValuesRec* values = g_scalingStatValuesDB.GetRecordByIndex(level - 1);

    if (!distribution || !values) {
        return;
    }

    this->ScanEnchantSpells(stats);

    if (stats->m_scalingStatValue) {
        int32_t budget = GetScalingStatBudget(values, stats->m_scalingStatValue);

        for (uint32_t i = 0; i < 10; i++) {
            int32_t type = distribution->m_statID[i];

            if (type <= 0) {
                continue;
            }

            int32_t value = budget * distribution->m_bonus[i] / 10000;
            this->m_stats[type + 11] += value;

            if (type == 38) {
                this->m_stats[8] += value;
                this->m_stats[9] += value;
            } else if (type == 39) {
                this->m_stats[9] += value;
            }
        }
    } else {
        this->AddStatMods(stats);
    }

    if (stats->m_scalingStatValue & 0xF801E0) {
        this->m_stats[0] += GetScalingStatArmor(values, stats->m_scalingStatValue);
    } else {
        this->AddResistances(stats);
    }

    if (stats->m_scalingStatValue & 0x7E00) {
        float minDamage;
        float maxDamage;
        GetScalingStatDamage(values, stats->m_scalingStatValue, &minDamage, &maxDamage);
        float total = minDamage + maxDamage;
        this->m_dps = 0.5f * total;
        float damage = total * stats->m_delay * 0.00050000002f;
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && player->m_unit->m_unitFieldBytes0[1] == 11) {
            this->m_stats[10] += this->m_stats[8] + GetFeralAttackPower(stats, damage);
        }
    } else {
        this->AccumulateDpsStat(stats);
    }

    if (stats->m_scalingStatValue & 0x8000) {
        this->m_stats[56] += values->m_spellPower;
    }
}

// VERIFY: e578015f auto
// OFFSET: 0x0061F940
void CGItemStatsSummary::AddEnchantment(const SpellItemEnchantmentRec* enchant) {
    for (uint32_t i = 0; i < 3; i++) {
        this->AddEnchantEffect(enchant->m_effect[i], enchant->m_effectArg[i], (enchant->m_effectPointsMin[i] + enchant->m_effectPointsMax[i]) / 2);
    }
}

// VERIFY: f46f01b2 auto
// OFFSET: 0x0061F980
void CGItemStatsSummary::AddEnchantment(const SpellItemEnchantmentRec* enchant, int32_t amount) {
    for (uint32_t i = 0; i < 3; i++) {
        this->AddEnchantEffect(enchant->m_effect[i], enchant->m_effectArg[i], amount);
    }
}

// VERIFY: b5f42f92 manual
// OFFSET: 0x0061FA20
void CGItemStatsSummary::FillOutRandomProperties(int32_t randomPropertiesId, uint32_t seed) {
    this->m_flags |= 0x2;

    if (randomPropertiesId > 0) {
        ItemRandomPropertiesRec* properties = g_itemRandomPropertiesDB.GetRecord(randomPropertiesId);

        if (!properties) {
            return;
        }

        for (uint32_t i = 0; i < 5; i++) {
            SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(properties->m_enchantment[i]);

            if (enchant) {
                this->AddEnchantment(enchant);
            }
        }

        return;
    }

    ItemRandomSuffixRec* suffix = g_itemRandomSuffixDB.GetRecord(-randomPropertiesId);

    if (!suffix) {
        return;
    }

    for (uint32_t i = 0; i < 5; i++) {
        int32_t amount = CGItem_C::GetRandomPropertyValue(i + 7, randomPropertiesId, static_cast<uint16_t>(seed));
        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(suffix->m_enchantment[i]);

        if (enchant) {
            this->AddEnchantment(enchant, amount);
        }
    }
}

// VERIFY: 37802838 manual
// OFFSET: 0x0061FB30
void CGTooltip::PostLoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::PostLoadXML(node, status);

    for (int32_t pass = 0; pass < 2; pass++) {
        if (pass == 1) {
            this->m_leftStrings.SetCount(this->m_maxLines);
            this->m_rightStrings.SetCount(this->m_maxLines);
            this->m_lineWrap.SetCount(this->m_maxLines);
        }

        for (this->m_maxLines = 0; ; this->m_maxLines++) {
            char leftName[256];
            char rightName[256];

            const char* name = this->GetName();
            SStrPrintf(leftName, sizeof(leftName), "%sTextLeft%d", name ? name : "<unnamed>", this->m_maxLines + 1);

            name = this->GetName();
            SStrPrintf(rightName, sizeof(rightName), "%sTextRight%d", name ? name : "<unnamed>", this->m_maxLines + 1);

            CSimpleFontString* left = static_cast<CSimpleFontString*>(CScriptObject::GetScriptObjectByName(leftName, CSimpleFontString::GetObjectType()));
            CSimpleFontString* right = static_cast<CSimpleFontString*>(CScriptObject::GetScriptObjectByName(rightName, CSimpleFontString::GetObjectType()));

            if (!left || !right) {
                break;
            }

            if (pass == 1) {
                this->m_leftStrings[this->m_maxLines] = left;
                this->m_rightStrings[this->m_maxLines] = right;
                this->m_lineWrap[this->m_maxLines] = 0;
            }
        }
    }

    char buffer[256];

    for (uint32_t i = 0; i < sizeof(this->m_textures) / sizeof(this->m_textures[0]); i++) {
        const char* name = this->GetName();
        SStrPrintf(buffer, sizeof(buffer), "%sTexture%d", name ? name : "<unnamed>", i + 1);
        this->m_textures[i] = static_cast<CSimpleTexture*>(CScriptObject::GetScriptObjectByName(buffer, CSimpleTexture::GetObjectType()));
    }

    const char* name = this->GetName();
    SStrPrintf(buffer, sizeof(buffer), "%sStatusBar", name ? name : "<unnamed>");
    this->m_statusBar = static_cast<CSimpleStatusBar*>(CScriptObject::GetScriptObjectByName(buffer, CSimpleStatusBar::GetObjectType()));
}

// VERIFY: d942bcf0 manual
// OFFSET: 0x0061E830
void CGTooltip::GetSummonedByString(CGUnit_C* unit, char* buffer, uint32_t size) {
    if (!unit || !buffer) {
        return;
    }

    WOWGUID owner = *unit->GetCharmerOrCreatorGuid();

    if (!owner) {
        return;
    }

    CGUnit_C* ownerUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(owner, TYPE_UNIT, __FILE__, __LINE__));

    if (ownerUnit && *ownerUnit->GetCharmerOrCreatorGuid()) {
        owner = *ownerUnit->GetCharmerOrCreatorGuid();
    }

    char token[1024];
    SpellRec spell;

    if (g_spellDB.GetRecord(unit->m_unit->m_unitCreatedBySpell, &spell)) {
        for (uint32_t i = 0; i < sizeof(spell.m_effect) / sizeof(spell.m_effect[0]); i++) {
            if (spell.m_effect[i] != SPELL_EFFECT_SUMMON) {
                continue;
            }

            SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[i]);

            if (properties && properties->m_title != -1) {
                if (properties->m_title) {
                    SStrPrintf(token, sizeof(token), "UNITNAME_SUMMON_TITLE%d", properties->m_title);
                    SStrPrintf(buffer, size, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), GetObjectNameFromGuid(owner));
                }

                return;
            }

            break;
        }
    }

    if (unit->GetCreatureType() == 1) {
        SStrPrintf(token, sizeof(token), "UNITNAME_SUMMON_TITLE1");
    } else {
        SStrPrintf(token, sizeof(token), "UNITNAME_SUMMON_TITLE3");
    }

    SStrPrintf(buffer, size, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), GetObjectNameFromGuid(owner));
}

// VERIFY: 4411ff17 auto
// OFFSET: 0x0061FE30
void CGTooltip::AddFontStrings(CSimpleFontString* left, CSimpleFontString* right) {
    this->m_maxLines++;
    this->m_leftStrings.SetCount(this->m_maxLines);
    this->m_rightStrings.SetCount(this->m_maxLines);
    this->m_lineWrap.SetCount(this->m_maxLines);
    this->m_leftStrings.m_data[this->m_maxLines - 1] = left;
    this->m_rightStrings.m_data[this->m_maxLines - 1] = right;
    this->m_lineWrap.m_data[this->m_maxLines - 1] = 0;
}

// VERIFY: c9a37f57 manual
// OFFSET: 0x0061FEC0
void CGTooltip::AddLine(const char* left, const char* right, const CImVector& leftColor, const CImVector& rightColor, int32_t wrap) {
    if ((!left || !*left) && (!right || !*right)) {
        return;
    }

    if (!this->m_maxLines) {
        return;
    }

    if (this->m_numLines == this->m_maxLines - 1) {
        char name[256];

        const char* tooltipName = this->GetName();
        SStrPrintf(name, sizeof(name), "%sTextLeft%d", tooltipName ? tooltipName : "<unnamed>", this->m_maxLines + 1);
        void* data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
        CSimpleFontString* leftString = data ? new (data) CSimpleFontString(this, 2, true) : nullptr;
        leftString->SetName(name);
        leftString->SetFontObject(this->m_leftStrings.m_data[this->m_maxLines - 1]->m_fontObject);
        CSimpleFontString* previous = this->m_leftStrings.m_data[this->m_maxLines - 1];
        leftString->SetPoint(FRAMEPOINT_TOPLEFT, previous, FRAMEPOINT_BOTTOMLEFT, 0.0f, NDCToDDCWidth(-2.0f / (CoordinateGetAspectCompensation() * 1024.0f)), 0);
        leftString->m_shown = 0;
        leftString->HideThis();

        tooltipName = this->GetName();
        SStrPrintf(name, sizeof(name), "%sTextRight%d", tooltipName ? tooltipName : "<unnamed>", this->m_maxLines + 1);
        data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
        CSimpleFontString* rightString = data ? new (data) CSimpleFontString(this, 2, true) : nullptr;
        rightString->SetName(name);
        rightString->SetFontObject(this->m_rightStrings.m_data[this->m_maxLines - 1]->m_fontObject);
        rightString->SetPoint(FRAMEPOINT_RIGHT, leftString, FRAMEPOINT_LEFT, NDCToDDCWidth(40.0f / (CoordinateGetAspectCompensation() * 1024.0f)), 0.0f, 0);
        rightString->m_shown = 0;
        rightString->HideThis();

        this->AddFontStrings(leftString, rightString);
    }

    if (left && *left) {
        CSimpleFontString* string = this->m_leftStrings.m_data[this->m_numLines];
        string->SetVertexColor(leftColor);
        string->SetText(left, 1);
        this->m_leftStrings.m_data[this->m_numLines]->m_shown = 1;
        this->m_leftStrings.m_data[this->m_numLines]->ShowThis();
    }

    int32_t lineWrap;

    if (right && *right) {
        CSimpleFontString* string = this->m_rightStrings.m_data[this->m_numLines];
        string->SetVertexColor(rightColor);
        string->SetText(right, 1);
        this->m_rightStrings.m_data[this->m_numLines]->m_shown = 1;
        this->m_rightStrings.m_data[this->m_numLines]->ShowThis();
        lineWrap = 0;
    } else {
        lineWrap = wrap;
    }

    this->m_lineWrap.m_data[this->m_numLines++] = lineWrap;
}

// VERIFY: 77aa9693 auto
// OFFSET: 0x006206D0
int32_t CGItemStatsSummary::Create(CGItem_C* item, int32_t level) {
    this->Create();
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (stats->m_scalingStatValue) {
        this->FillOutScalingItem(stats, level);
        this->CleanStatNames();
        return 1;
    }

    this->AccumulateStats(stats);

    if (item->m_item->m_itemFieldRandomPropertiesId) {
        this->FillOutRandomProperties(item->m_item->m_itemFieldRandomPropertiesId, item->m_item->m_itemFieldPropertySeed);
    }

    this->CleanStatNames();
    return 1;
}

// VERIFY: fe542a77 manual
// OFFSET: 0x006207A0
int32_t CGItemStatsSummary::Fill(const char* link) {
    memset(this->m_stats, 0, sizeof(this->m_stats));
    this->m_dps = 0.0f;
    this->m_flags = 0;
    int32_t randomPropertiesId = 0;
    uint32_t seed = 0;
    int32_t level = 0;
    const char* cursor = SStrStr(link, "item:");

    if (!cursor) {
        return 0;
    }

    cursor += 5;
    int32_t itemId = SStrToInt(cursor);

    if (itemId <= 0) {
        return 0;
    }

    if (*cursor) {
        while (*cursor != ':') {
            if (!*++cursor) {
                break;
            }
        }

        if (*cursor) {
            cursor++;
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (*cursor) {
            while (*cursor != ':') {
                if (!*++cursor) {
                    break;
                }
            }

            if (*cursor) {
                cursor++;
            }
        }
    }

    if (*cursor) {
        while (*cursor != ':') {
            if (!*++cursor) {
                goto lookup;
            }
        }

        if (*cursor && *++cursor) {
            while (*cursor != ':') {
                if (!*++cursor) {
                    goto lookup;
                }
            }

            if (*cursor) {
                cursor++;
                randomPropertiesId = SStrToInt(cursor);

                if (*cursor) {
                    while (*cursor != ':') {
                        if (!*++cursor) {
                            goto lookup;
                        }
                    }

                    if (*cursor) {
                        cursor++;
                        seed = SStrToInt(cursor);

                        if (*cursor) {
                            while (*cursor != ':') {
                                if (!*++cursor) {
                                    goto lookup;
                                }
                            }

                            if (*cursor) {
                                level = SStrToInt(cursor + 1);
                            }
                        }
                    }
                }
            }
        }
    }

lookup:
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    if (stats->m_scalingStatValue) {
        this->FillOutScalingItem(stats, level);
    } else {
        this->AccumulateStats(stats);

        if (randomPropertiesId) {
            this->FillOutRandomProperties(randomPropertiesId, seed);
        }
    }

    this->CleanStatNames();
    return 1;
}

// VERIFY: 6f35979f manual
// OFFSET: 0x00621070
int32_t CGTooltip::SetUnit(const WOWGUID& guid, int32_t showHealth, const WOWGUID& healthGuid, int32_t a4) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);

    this->ClearTooltip();

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!healthGuid && unit && unit->HasAuraEffect(SPELL_AURA_INITIALIZE_IMAGES)) {
        SpellRec spell;

        for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
            AuraData* aura = unit->GetAura(i);

            if (!g_spellDB.GetRecord(aura->m_spellId, &spell)) {
                continue;
            }

            for (uint32_t j = 0; j < sizeof(spell.m_effectAura) / sizeof(spell.m_effectAura[0]); j++) {
                if (spell.m_effectAura[j] == SPELL_AURA_INITIALIZE_IMAGES && ((1 << j) & aura->m_flags)) {
                    return this->SetUnit(aura->m_creator, showHealth, guid, 0);
                }
            }
        }
    }

    CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(guid);
    CGPartyPetState* petState = CGPartyMemberStateRepository::FindPetState(guid);
    WOWGUID none = { 0 };
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);
    CreatureStats_C* creature = nullptr;

    if (unit) {
        creature = unit->m_creatureCacheEntry;
    } else if (!state && !petState) {
        uint32_t high = static_cast<uint32_t>(guid.m_guid >> 32);
        WOWGUID requester = { 0 };

        if (((high & 0xF0F00000) != 0xF0300000 && (high & 0xF0F00000) != 0xF0500000) || !(creature = g_creatureDBCache.GetRecord((guid.m_guid >> 24) & 0xFFFFFFF, requester, nullptr, nullptr, false))) {
            if (!name) {
                this->Hide();
                return 0;
            }
        }

        showHealth = 0;
    }

    this->m_unitGuid = guid;

    if (this->m_statusBar && showHealth) {
        this->m_healthGuid = healthGuid ? healthGuid : this->m_unitGuid;
        CGUnit_C* healthUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_healthGuid, TYPE_UNIT, __FILE__, __LINE__));
        ClntObjMgrSetObjMirrorHandler(this->m_healthGuid, ID_UNIT, offsetof(CGUnitData, m_unitFieldHealth), sizeof(int32_t), HealthUpdateHandler, this->m_statusBar, HANDLER_PRIORITY_NORMAL, 0);

        float maxHealth;

        if (healthUnit) {
            maxHealth = static_cast<float>(healthUnit->m_unit->m_unitFieldMaxhealth);
        } else if (state) {
            maxHealth = static_cast<float>(state->m_maxHealth);
        } else {
            maxHealth = static_cast<float>(petState->m_maxHealth);
        }

        this->m_statusBar->SetMinMaxValues(0.0f, maxHealth);

        float health;

        if (healthUnit) {
            health = (healthUnit->m_unit->m_unitDynamicFlags & 0x20) ? 0.0f : static_cast<float>(healthUnit->m_unit->m_unitFieldHealth);
        } else if (state) {
            health = static_cast<float>(state->m_health);
        } else {
            health = static_cast<float>(petState->m_health);
        }

        this->m_statusBar->SetValue(health);
        this->m_statusBar->Show();
    }

    char line[1024];
    char title[1024];

    if (unit) {
        const char* realm = nullptr;
        unit->GetUnitName(&realm, 1);

        if (realm && *realm) {
            SStrPrintf(title, sizeof(title), "%s - %s", unit->GetUnitChosenTitle(line, sizeof(line), 1, nullptr), realm);
        } else {
            SStrPrintf(title, sizeof(title), unit->GetUnitChosenTitle(line, sizeof(line), 1, nullptr));
        }

        this->AddLine(title, nullptr, titleColor, titleColor, 0);

        if (unit->m_obj->m_type & TYPE_PLAYER) {
            uint32_t guildId = static_cast<CGPlayer_C*>(unit)->m_player->m_playerGuildid;

            if (guildId) {
                WOWGUID requester = unit->m_obj->m_guid;
                GuildStats_C* guild = g_guildInfoCache.GetRecord(guildId, requester, TooltipUnitNameCallback, this, false);

                if (guild) {
                    this->AddLine(guild->m_name, nullptr, textColor, textColor, 0);
                }
            }
        }
    } else if (name) {
        if (name->m_realmName[0]) {
            SStrPrintf(line, sizeof(line), "%s - %s", name->m_name, name->m_realmName);
            this->AddLine(line, nullptr, titleColor, titleColor, 0);
        } else {
            this->AddLine(name->m_name, nullptr, titleColor, titleColor, 0);
        }
    } else if (petState) {
        this->AddLine(petState->m_name, nullptr, titleColor, titleColor, 0);
    } else if (creature) {
        this->AddLine(creature->m_name[0], nullptr, titleColor, titleColor, 0);
    }

    CVar* colorblind = CVar::Lookup("colorblindMode");
    CGPlayer_C* player = nullptr;
    const char* subName = nullptr;

    if (colorblind && colorblind->m_intValue && (player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__))) != nullptr) {
        if (unit) {
            line[0] = 0;
            FRAMESCRIPT_GENDER gender = player->m_unit->m_unitFieldBytes0[2] == UNITSEX_FEMALE ? GENDER_FEMALE : GENDER_MALE;
            SStrPrintf(line, sizeof(line), "FACTION_STANDING_LABEL%d", unit->UnitReaction(player) + 1);
            this->AddLine(FrameScript_GetText(line, 0, gender), nullptr, textColor, textColor, 0);
        }
    }

    if (!subName) {
        if (unit && !(unit->m_obj->m_type & TYPE_PLAYER)) {
            subName = unit->GetUnitTitle();
        } else if (creature) {
            subName = creature->m_subName;
        }
    }

    if (subName && *subName) {
        this->AddLine(subName, nullptr, textColor, textColor, 0);
    }

    line[0] = 0;
    CGTooltip::GetSummonedByString(unit, line, sizeof(line));

    if (line[0]) {
        this->AddLine(line, nullptr, textColor, textColor, 0);
    }

    char race[128] = {};
    char className[128] = {};
    char type[64] = {};
    char level[32];

    CGPlayer_C* activePlayer = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t dead = 0;
    CreatureTypeRec* creatureType = nullptr;
    int32_t unitLevel = 0;
    int32_t showLevel = 1;

    if (unit) {
        dead = unit->m_unit->m_unitFieldHealth <= 0 || (unit->m_unit->m_unitDynamicFlags & 0x20);
        unitLevel = unit->m_unit->m_unitFieldLevel;

        if (unit->GetCreatureType() != 10) {
            if (activePlayer) {
                if (activePlayer->UnitReaction(unit) < UNIT_REACTION_FRIENDLY && !activePlayer->IsTreatAsRaidUnit()) {
                    creatureType = g_creatureTypeDB.GetRecord(unit->GetCreatureType());
                }
            }
        }

        if (unit->GetCreatureType() == 10 || activePlayer) {
            if (activePlayer && unit->UnitReaction(activePlayer) <= UNIT_REACTION_HOSTILE && static_cast<int32_t>(activePlayer->m_unit->m_unitFieldLevel) <= unitLevel - 10) {
                showLevel = 0;
            }
        }

        if (showLevel && unit->IsBossMob()) {
            showLevel = 0;
        }
    } else if (state) {
        dead = (state->m_flags & 0x1) && state->m_health <= 0;
        unitLevel = reinterpret_cast<int32_t*>(state)[12];
    } else {
        showLevel = 0;

        if (petState) {
            dead = petState->m_health <= 0;
        } else if (creature && creature->m_type != 10) {
            creatureType = g_creatureTypeDB.GetRecord(creature->m_type);
        }
    }

    if (showLevel && unitLevel > 0) {
        int32_t displayLevel = unitLevel;

        if (activePlayer && ((activePlayer->m_player->m_playerBytes3[1]) || activePlayer->m_player->m_playerFakeInebriation) && unit && activePlayer->CanAttack(unit)) {
            displayLevel = unitLevel - RoundToInt64(activePlayer->GetDrunknessTurnValue() * 5.0f);

            if (displayLevel < 1) {
                displayLevel = 1;
            }
        }

        if (displayLevel > 0) {
            SStrPrintf(level, sizeof(level), "%d", displayLevel);
        } else {
            SStrCopy(level, "??", sizeof(level));
        }
    } else {
        SStrCopy(level, "??", sizeof(level));
    }

    if (dead) {
        SStrCopy(className, FrameScript_GetText("CORPSE", -1, GENDER_NOT_APPLICABLE), sizeof(className));
    } else if ((unit && (unit->m_obj->m_type & TYPE_PLAYER)) || name) {
        const char* raceName = CGUnit_C::GetDisplayRaceName(unit, name);
        const char* classNameText = CGUnit_C::GetDisplayClassName(unit, name);

        if (raceName && classNameText) {
            SStrCopy(race, raceName, sizeof(race));
            SStrCopy(className, classNameText, sizeof(className));
        }
    } else if (creatureType) {
        SStrCopy(className, creatureType->m_name, sizeof(className));
    }

    const char* classification = nullptr;

    if (unit) {
        classification = unit->IsBossMob() ? "BOSS" : s_classificationTokens[unit->GetCreatureClassification()];

        if (unit->m_obj->m_type & TYPE_PLAYER) {
            SStrCopy(type, FrameScript_GetText("PLAYER", -1, GENDER_NOT_APPLICABLE), sizeof(type));
        } else if (name) {
            SStrCopy(type, FrameScript_GetText("PLAYER", -1, GENDER_NOT_APPLICABLE), sizeof(type));
        } else if (classification) {
            SStrCopy(type, FrameScript_GetText(classification, -1, GENDER_NOT_APPLICABLE), sizeof(type));
        }
    } else {
        if (creature) {
            classification = (creature->m_typeFlags & 0x4) ? "BOSS" : s_classificationTokens[creature->m_rank];
        }

        if (name) {
            SStrCopy(type, FrameScript_GetText("PLAYER", -1, GENDER_NOT_APPLICABLE), sizeof(type));
        } else if (classification) {
            SStrCopy(type, FrameScript_GetText(classification, -1, GENDER_NOT_APPLICABLE), sizeof(type));
        }
    }

    if (race[0] && className[0]) {
        if (type[0]) {
            SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL_RACE_CLASS_TYPE", -1, GENDER_NOT_APPLICABLE), level, race, className, type);
        } else {
            SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL_RACE_CLASS", -1, GENDER_NOT_APPLICABLE), level, race, className);
        }
    } else if (className[0]) {
        if (type[0]) {
            SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL_CLASS_TYPE", -1, GENDER_NOT_APPLICABLE), level, className, type);
        } else {
            SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL_CLASS", -1, GENDER_NOT_APPLICABLE), level, className);
        }
    } else if (type[0]) {
        SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL_TYPE", -1, GENDER_NOT_APPLICABLE), level, type);
    } else {
        SStrPrintf(line, sizeof(line), FrameScript_GetText("TOOLTIP_UNIT_LEVEL", -1, GENDER_NOT_APPLICABLE), level);
    }

    this->AddLine(line, nullptr, textColor, textColor, 0);

    if (!unit && state && !(state->m_flags & 0x1) && !a4) {
        this->AddLine(FrameScript_GetText("PLAYER_OFFLINE", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);

        if (this->m_onTooltipSetUnit.m_luaRef) {
            this->RunScript(this->m_onTooltipSetUnit, 0, nullptr);
        }

        this->Show();
        return 0;
    }

    if (activePlayer && unit && unit->ShowFactionTooltip()) {
        FactionTemplateRec* factionTemplate = g_factionTemplateDB.GetRecord(unit->m_unit->m_unitFieldFactiontemplate);
        FactionRec* faction = factionTemplate ? g_factionDB.GetRecord(factionTemplate->m_faction) : nullptr;

        if (faction && faction->m_reputationIndex >= 0) {
            for (uint32_t i = 0; i < 4; i++) {
                uint32_t raceMask = faction->m_reputationRaceMask[i];
                uint32_t classMask = faction->m_reputationClassMask[i];

                if ((!raceMask || ((1 << (activePlayer->m_unit->m_unitFieldBytes0[0] - 1)) & raceMask)) && (raceMask || classMask) && (!classMask || ((1 << (activePlayer->m_unit->m_unitFieldBytes0[1] - 1)) & classMask))) {
                    if (!(faction->m_reputationFlags[i] & 0x4)) {
                        this->AddLine(faction->m_name, nullptr, textColor, textColor, 0);
                    }

                    break;
                }
            }
        }
    }

    if (unit) {
        if (((unit->m_obj->m_type & TYPE_PLAYER) && (static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags & 0x100)) || (unit->m_unit->m_unitFieldBytes2[1] & 0x1)) {
            this->AddLine(FrameScript_GetText("PVP_ENABLED", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        }

        if (unit->m_unit->m_unitFieldHealth <= 0 && (unit->m_obj->m_type & TYPE_PLAYER) && activePlayer && activePlayer->CanAssist(unit, false)) {
            this->AddLine(FrameScript_GetText("RESURRECTABLE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
        }

        if (unit->m_unit->m_unitDynamicFlags & 0x10) {
            this->AddEmpathyText(unit, line, sizeof(line));
        }

        if ((unit->m_unit->m_unitFieldFlags & 0x4000000) && !(unit->m_obj->m_type & TYPE_PLAYER)) {
            this->AddSkinnableText(unit, line, sizeof(line));
        }
    }

    if (CGGameUI::IsPartyMember(guid) && guid != ClntObjMgrGetActivePlayer() && !unit) {
        if (state && AreaListGetName(reinterpret_cast<int32_t*>(state)[13], line, sizeof(line), 0)) {
            this->AddLine(line, nullptr, textColor, textColor, 0);
        }
    } else if (unit && unit->IsPVPLeader()) {
        this->AddLine(FrameScript_GetText("PVP_RANK_LEADER", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
    }

    CVar* questTracking = CVar::Lookup("showQuestTrackingTooltips");

    if (creature && questTracking && questTracking->m_intValue) {
        uint32_t summonedHigh = unit ? static_cast<uint32_t>(unit->m_unit->m_unitFieldSummonedby.m_guid >> 32) : 0;

        if (!unit || (summonedHigh & 0xF0000000) || !((summonedHigh & 0xF07FFFFF) | static_cast<uint32_t>(unit->m_unit->m_unitFieldSummonedby.m_guid))) {
            uint32_t entry = unit ? unit->m_obj->m_entryID : static_cast<uint32_t>((guid.m_guid >> 24) & 0xFFFFFFF);
            QuestLogLeaderBoardParams params;
            params.m_questCallback = TooltipUnitNameCallback;
            params.m_objectCallback = TooltipUnitNameCallback;
            params.m_creatureCallback = TooltipUnitNameCallback;
            params.m_itemCallback = TooltipUnitNameCallback;
            params.m_param = this;
            const char* objective = params.m_text;
            char buffer[4099];

            for (uint32_t i = 0; i < CGQuestLog::m_numQuest; i++) {
                WOWGUID requester = { 0 };
                QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::GetQuestAt(i), requester, TooltipUnitNameCallback, this, false);

                if (!quest) {
                    continue;
                }

                bool header = false;

                if (CGQuestLog::GetQuestLeaderBoardForObject(quest->m_id, entry, &params)) {
                    this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                    header = true;
                    SStrPrintf(buffer, sizeof(buffer), " - %s", objective);
                    this->AddLine(buffer, nullptr, textColor, textColor, 0);
                }

                for (uint32_t j = 0; j < sizeof(creature->m_killCredit) / sizeof(creature->m_killCredit[0]); j++) {
                    if (creature->m_killCredit[j] && CGQuestLog::GetQuestLeaderBoardForObject(quest->m_id, creature->m_killCredit[j], &params)) {
                        if (!header) {
                            this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                            header = true;
                        }

                        SStrPrintf(buffer, sizeof(buffer), " - %s", objective);
                        this->AddLine(buffer, nullptr, textColor, textColor, 0);
                    }
                }

                for (uint32_t j = 0; j < sizeof(creature->m_questItem) / sizeof(creature->m_questItem[0]); j++) {
                    int32_t itemId = creature->m_questItem[j];

                    if (!itemId) {
                        break;
                    }

                    if (CGQuestLog::GetQuestLeaderBoardForItem(quest->m_id, itemId, &params)) {
                        if (!header) {
                            this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                            header = true;
                        }

                        SStrPrintf(buffer, sizeof(buffer), " - %s", objective);
                        this->AddLine(buffer, nullptr, textColor, textColor, 0);
                    }

                    if (!CGQuestLog::IsQuestCompleteEx(i, 1)) {
                        for (uint32_t k = 0; k < sizeof(quest->m_intermediateItemId) / sizeof(quest->m_intermediateItemId[0]); k++) {
                            if (quest->m_intermediateItemId[k] != static_cast<uint32_t>(itemId)) {
                                continue;
                            }

                            if (!header) {
                                this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                                header = true;
                            }

                            WOWGUID itemRequester = { 0 };
                            ItemStats_C* stats = g_itemDBCache.GetRecord(quest->m_intermediateItemId[k], itemRequester, TooltipUnitNameCallback, this, false);

                            if (stats) {
                                SStrPrintf(buffer, sizeof(buffer), " - %s", stats->m_name[0]);
                                this->AddLine(buffer, nullptr, textColor, textColor, 0);
                            }
                        }
                    }
                }
            }
        }
    }

    if (unit && dead) {
        if (unit->GetMasterLooter()) {
            WOWGUID requester = { 0 };
            NameCache* looter = g_nameDBCache.GetRecord(unit->GetMasterLooter(), requester, TooltipUnitNameCallback, this, false);

            if (looter) {
                char buffer[64];
                SStrPrintf(buffer, sizeof(buffer), "%s: %s", FrameScript_GetText("MASTER_LOOTER", -1, GENDER_NOT_APPLICABLE), looter->m_name);
                this->AddLine(buffer, nullptr, textColor, textColor, 0);
            }
        }

        if (unit->GetLooter()) {
            WOWGUID requester = { 0 };
            NameCache* looter = g_nameDBCache.GetRecord(unit->GetLooter(), requester, TooltipUnitNameCallback, this, false);

            if (looter) {
                char buffer[64];
                SStrPrintf(buffer, sizeof(buffer), "%s: %s", FrameScript_GetText("LOOT", -1, GENDER_NOT_APPLICABLE), looter->m_name);
                this->AddLine(buffer, nullptr, textColor, textColor, 0);
            }
        }
    }

    int32_t result = 0;

    if (activePlayer && CGGameUI::m_threatWarningEnabled && unit) {
        WOWGUID playerGuid = activePlayer->m_obj->m_guid;
        uint8_t status;
        float percent;

        if (unit->CalculateThreat(playerGuid, &status, nullptr, &percent, nullptr)) {
            SStrPrintf(line, sizeof(line), FrameScript_GetText("THREAT_TOOLTIP", -1, GENDER_NOT_APPLICABLE), static_cast<int32_t>(percent - 0.5f));
            this->AddLine(line, nullptr, textColor, textColor, 0);

            uint32_t threatStatus = status >= 5 ? 1 : status;
            CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
            this->AddTexture("Interface\\RaidFrame\\UI-RaidFrame-Threat", coords, *reinterpret_cast<const CImVector*>(&s_threatStatusColors[threatStatus]));
            result = 1;
        }
    }

    if (this->m_onTooltipSetUnit.m_luaRef) {
        this->RunScript(this->m_onTooltipSetUnit, 0, nullptr);
    }

    this->Show();
    return result;
}

// VERIFY: 047a2a17 manual
// OFFSET: 0x00622410
void CGTooltip::SetCorpse(const WOWGUID& guid) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);

    this->ClearTooltip();

    CGCorpse_C* corpse = static_cast<CGCorpse_C*>(ClntObjMgrObjectPtr(guid, TYPE_CORPSE, __FILE__, __LINE__));

    if (!corpse) {
        this->Hide();
        return;
    }

    this->m_corpseGuid = guid;
    *reinterpret_cast<WOWGUID*>(&this->m_corpseOwnerGuid) = corpse->m_corpse->m_corpseFieldOwner;
    NameCache* name = g_nameDBCache.GetRecord(reinterpret_cast<WOWGUID*>(&this->m_corpseOwnerGuid)->m_guid, *reinterpret_cast<WOWGUID*>(&this->m_corpseOwnerGuid), TooltipCorpseCallback, this, true);

    if (name) {
        char line[1024];
        SStrPrintf(line, sizeof(line), FrameScript_GetText("CORPSE_TOOLTIP", -1, GENDER_NOT_APPLICABLE), name->m_name);
        this->AddLine(line, nullptr, textColor, textColor, 0);
        this->Show();
    }
}

// VERIFY: cbeee45f manual
// OFFSET: 0x0061E540
const char* CGTooltip::GetQuestDifficultyColorString(int32_t level) {
    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return s_questDifficultyColorStrings[2];
    }

    int32_t delta = level - static_cast<int32_t>(player->m_unit->m_unitFieldLevel);

    if (delta >= 5) {
        return s_questDifficultyColorStrings[0];
    }

    if (delta >= 3) {
        return s_questDifficultyColorStrings[1];
    }

    if (delta >= -2) {
        return s_questDifficultyColorStrings[2];
    }

    if (-delta <= CGQuestLog::GetQuestGreenRange()) {
        return s_questDifficultyColorStrings[3];
    }

    return s_questDifficultyColorStrings[4];
}

// VERIFY: a380ef23 auto
// OFFSET: 0x0061DDD0
void TooltipUnitNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (tooltip->m_unitGuid) {
        WOWGUID unitGuid = tooltip->m_unitGuid;
        WOWGUID healthGuid = {};
        tooltip->SetUnit(unitGuid, 1, healthGuid, 0);
        tooltip->CalculateSize();
    }
}

// VERIFY: 456c6e01 auto
// OFFSET: 0x0061EAB0
void CGTooltip::AppendText(const char* text) {
    if (!this->m_numLines) {
        return;
    }

    const char* current = this->m_leftStrings.m_data[0]->m_text;

    if (!current || !*current) {
        current = nullptr;
    }

    char buffer[256];
    SStrPrintf(buffer, sizeof(buffer), "%s%s", current, text);
    this->m_leftStrings.m_data[0]->SetText(buffer, 1);
    this->CalculateSize();
}

// VERIFY: 8fe663a0 manual
// OFFSET: 0x0061E290
const char* CGTooltip::CreateItemLink(int32_t itemId, uint32_t quality, int32_t enchant, const int32_t* gems, int32_t randomPropertiesId, int32_t seed) {
    uint32_t level = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        level = player->m_unit->m_unitFieldLevel;
    }

    if (quality >= sizeof(s_itemQualityColorStrings) / sizeof(s_itemQualityColorStrings[0])) {
        quality = 1;
    }

    const char* color = s_itemQualityColorStrings[quality];
    const char* reset = *color ? "|r" : "";
    char name[256];
    CGItem_C::BuildItemName(name, sizeof(name), itemId, randomPropertiesId);
    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Hitem:%d:%d:%d:%d:%d:%d:%d:%d:%d|h[%s]|h%s", color, itemId, enchant, gems[0], gems[1], gems[2], 0, randomPropertiesId, seed, level, name, reset);
    return s_itemLink;
}

// VERIFY: ffe3f14b manual
// OFFSET: 0x0061E3A0
const char* CGTooltip::GetItemHyperlinkString(CGItem_C* item) {
    if (!item) {
        return nullptr;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, WOWGUID(), nullptr, nullptr, false);
    uint32_t wrapped = (item->m_item->m_itemFieldFlags >> 3) & 1;
    int32_t gems[3];

    for (uint32_t i = 0; i < 3; i++) {
        if (!wrapped && !(item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION)) {
            gems[i] = item->m_item->m_itemFieldEnchantment[2 + i].m_id;
        } else {
            gems[i] = 0;
        }
    }

    int32_t enchant = 0;
    int32_t randomPropertiesId;
    int32_t seed;

    if (wrapped) {
        seed = 0;
        randomPropertiesId = 0;
    } else {
        randomPropertiesId = item->m_item->m_itemFieldRandomPropertiesId;
        seed = item->m_item->m_itemFieldPropertySeed;
        enchant = item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION ? 0 : item->m_item->m_itemFieldEnchantment[0].m_id;
    }

    return CGTooltip::CreateItemLink(item->m_obj->m_entryID, stats ? stats->m_quality : 1, enchant, gems, randomPropertiesId, seed);
}

// VERIFY: 55852bae auto
// OFFSET: 0x0061A190
int32_t FormatString(const char* token, int32_t count, FRAMESCRIPT_GENDER gender, char* buffer, uint32_t size) {
    const char* text = FrameScript_GetText(token, count, gender);
    SStrCopy(buffer, text, size);
    return *text != '\0';
}

// VERIFY: e0b2f2ad auto
// OFFSET: 0x0061A960
int32_t TimeFractionBelow(float time, float fraction) {
    double value;

    if (time >= 86400000.0f) {
        value = time * 1.1574074e-8f;
    } else if (time >= 3600000.0f) {
        value = time * 2.7777779e-7f;
    } else if (time >= 60000.0f) {
        value = time * 1.6666667e-5f;
    } else {
        value = time * 0.001f;
    }

    float scaled = static_cast<float>(value);
    return scaled - floor(value) < fraction;
}

// VERIFY: 49b20620 manual
// OFFSET: 0x0061A9E0
void CGTooltip::GetDurationString(char* buffer, uint32_t size, uint64_t time, const char* token, const char* prefix, int32_t roundUp, int32_t seconds) {
    if (!buffer || !token) {
        return;
    }

    uint32_t minute = seconds ? 60 : 60000;
    uint32_t count = static_cast<uint32_t>(time);
    char format[256];
    *buffer = '\0';

    if (time >= 1440 * minute) {
        count = static_cast<uint32_t>(roundUp ? (time - 1) / (1440 * minute) + 1 : time / (1440 * minute));
        SStrPrintf(format, sizeof(format), "%s_DAYS", token);
    } else if (time >= 60 * minute) {
        uint64_t hours = roundUp ? (time - 1) / (60 * minute) + 1 : time / (60 * minute);
        count = static_cast<uint32_t>(hours);

        if (hours == 24) {
            count = 1;
            SStrPrintf(format, sizeof(format), "%s_DAYS", token);
        } else {
            SStrPrintf(format, sizeof(format), "%s_HOURS", token);
        }
    } else if (time >= minute) {
        uint64_t minutes = roundUp ? (time - 1) / minute + 1 : time / minute;
        count = static_cast<uint32_t>(minutes);

        if (minutes == 60) {
            count = 1;
            SStrPrintf(format, sizeof(format), "%s_HOURS", token);
        } else {
            SStrPrintf(format, sizeof(format), "%s_MIN", token);
        }
    } else {
        if (!seconds) {
            count = static_cast<uint32_t>(time / 1000);
        }

        SStrPrintf(format, sizeof(format), "%s_SEC", token);
    }

    if (prefix) {
        SStrPrintf(buffer, size, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), prefix, count);
    } else {
        SStrPrintf(buffer, size, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), count);
    }
}

// VERIFY: 2e33ff12 manual
// OFFSET: 0x0061AEE0
void CGTooltip::GetFloatDurationString(char* buffer, uint32_t size, float time, const char* token, const char* prefix) {
    static float s_msPerHour = 3600000.0f;
    static float s_msPerDay = 24.0f * s_msPerHour;

    if (!buffer || !token) {
        return;
    }

    char format[256];
    float value;
    *buffer = '\0';

    if (time >= s_msPerDay) {
        value = time / s_msPerDay;
        SStrPrintf(format, sizeof(format), "%s_DAYS", token);
    } else if (time >= s_msPerHour) {
        value = time / s_msPerHour;
        SStrPrintf(format, sizeof(format), "%s_HOURS", token);
    } else if (time >= 60000.0f) {
        value = time * 1.6666667e-5f;
        SStrPrintf(format, sizeof(format), "%s_MIN", token);
    } else {
        value = time * 0.001f;
        SStrPrintf(format, sizeof(format), "%s_SEC", token);
    }

    if (!(value > 0.0f)) {
        value = 0.0f;
    }

    if (prefix) {
        SStrPrintf(buffer, size, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), prefix, value);
    } else {
        SStrPrintf(buffer, size, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), value);
    }
}

// VERIFY: 74ea258e auto
// OFFSET: 0x0061DD60
void TooltipSpellCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (tooltip->m_pendingQueries) {
        tooltip->m_pendingQueries--;
    }

    if (tooltip->m_pendingQueries <= 0 && tooltip->m_spellId) {
        tooltip->SetSpell(tooltip->m_spellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        tooltip->CalculateSize();
    }
}

// VERIFY: 73537865 manual
// OFFSET: 0x006224F0
int32_t CGTooltip::AddTalentPrereqs(const TalentRec* talent, CGTalentGroup_C* group, int32_t isInspect, int32_t isPet, int32_t usePreview) {
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    char text[256];

    if (!talent) {
        return 0;
    }

    int32_t met = 1;

    for (uint32_t i = 0; i < 3; i++) {
        int32_t prereqId = talent->m_prereqTalent[i];

        if (!prereqId) {
            continue;
        }

        TalentRec* prereq = g_talentDB.GetRecord(prereqId);

        if (!prereq) {
            continue;
        }

        int32_t rank = talent->m_prereqRank[i];
        TalentGroupItem_C* item;

        if (group && (item = group->GetTalentItem(prereqId)) && (usePreview ? item->m_previewRank : item->m_rank) >= rank) {
            continue;
        }

        for (uint32_t r = rank; r < 9; r++) {
            int32_t spellId = prereq->m_spellRank[r];

            if (spellId) {
                met = 0;
                SpellRec spell;

                if (g_spellDB.GetRecord(spellId, &spell)) {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("TOOLTIP_TALENT_PREREQ", -1, GENDER_NOT_APPLICABLE), rank + 1, spell.m_name);
                    this->AddLine(text, nullptr, redColor, redColor, 0);
                }

                break;
            }
        }
    }

    if (group && talent->m_tierID > 0) {
        TalentTabRec* tab = g_talentTabDB.GetRecord(talent->m_tabID);

        if (tab) {
            TalentGroupTab_C* groupTab = group->GetTabFromID(talent->m_tabID);
            uint32_t spent = groupTab ? groupTab->m_pointsSpent + groupTab->m_previewPointsSpent : 0;

            if (spent < static_cast<uint32_t>(talent->m_tierID * (2 * (isPet == 0) + 3))) {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("TOOLTIP_TALENT_TIER_POINTS", -1, GENDER_NOT_APPLICABLE), talent->m_tierID * (2 * (isPet == 0) + 3), tab->m_name);
                this->AddLine(text, nullptr, redColor, redColor, 0);
                met = 0;
            }
        }
    }

    if (talent->m_requiredSpellID && !TalentInfoIsSpellKnown(talent->m_requiredSpellID, isInspect, isPet)) {
        SpellRec spell;

        if (g_spellDB.GetRecord(talent->m_requiredSpellID, &spell)) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), spell.m_name);
            this->AddLine(text, nullptr, redColor, redColor, 0);
            met = 0;
        }
    }

    return met;
}

// VERIFY: 5d06b108 manual
// OFFSET: 0x00622800
void CGTooltip::AddTalentLearnText(const TalentRec* talent, CGTalentGroup_C* group, int32_t prereqsMet, int32_t rank, int32_t maxRank, int32_t a7, int32_t a8, int32_t isPet, int32_t usePreview) {
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);

    if (!talent || !ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    uint32_t activeGroup = CGTalentInfo::GetActiveTalentGroup(isPet);

    if (group != CGTalentInfo::GetTalentGroup(activeGroup, isPet)) {
        return;
    }

    if (usePreview) {
        if (prereqsMet && !a7 && !a8) {
            int32_t count;

            if (group->CanAddPreviewTalentPoints(talent, 1, &count, isPet)) {
                this->AddLine(FrameScript_GetText("TALENT_TOOLTIP_ADDPREVIEWPOINT", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
            }

            if (group->CanAddPreviewTalentPoints(talent, -1, &count, isPet)) {
                this->AddLine(FrameScript_GetText("TALENT_TOOLTIP_REMOVEPREVIEWPOINT", -1, GENDER_NOT_APPLICABLE), nullptr, redColor, redColor, 0);
            }
        }
    } else {
        uint32_t unspent = CGTalentInfo::GetUnspentTalentPoints(activeGroup, isPet);

        if (prereqsMet && !a7 && !a8 && unspent && rank != maxRank) {
            this->AddLine(FrameScript_GetText("TOOLTIP_TALENT_LEARN", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
        }
    }
}

// INLINED
static int32_t SetSpellPowerCost(char* cost, const SpellRec* spell, const CGUnit_C* caster, int32_t powerCost, const PowerDisplayRec* display) {
    char format[128];
    int32_t costPerSecond = Spell_C_GetPowerCostPerSecond(spell, caster);
    powerCost = static_cast<uint32_t>(powerCost) / Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(spell->m_powerType));
    costPerSecond = static_cast<uint32_t>(costPerSecond) / Spell_C_GetPowerDisplayMod(static_cast<POWER_TYPE>(spell->m_powerType));
    const char* token = static_cast<uint32_t>(spell->m_powerType) > 6 ? "HEALTH_COST" : s_powerCostTokens[spell->m_powerType];

    if (spell->m_powerType == POWER_TYPE_RUNES) {
        SpellRuneCostRec* runes = g_spellRuneCostDB.GetRecord(spell->m_runeCostID);

        if (!runes) {
            return powerCost;
        }

        if (runes->m_blood) {
            SStrPrintf(format, sizeof(format), FrameScript_GetText("RUNE_COST_BLOOD", -1, GENDER_NOT_APPLICABLE), runes->m_blood);
            SStrPack(cost, format, STORM_MAX_STR);

            if (runes->m_unholy || runes->m_frost) {
                SStrPack(cost, " ", STORM_MAX_STR);
            }
        }

        if (runes->m_unholy) {
            SStrPrintf(format, sizeof(format), FrameScript_GetText("RUNE_COST_UNHOLY", -1, GENDER_NOT_APPLICABLE), runes->m_unholy);
            SStrPack(cost, format, STORM_MAX_STR);

            if (runes->m_frost) {
                SStrPack(cost, " ", STORM_MAX_STR);
            }
        }

        if (runes->m_frost) {
            SStrPrintf(format, sizeof(format), FrameScript_GetText("RUNE_COST_FROST", -1, GENDER_NOT_APPLICABLE), runes->m_frost);
            SStrPack(cost, format, STORM_MAX_STR);
        }

        return powerCost;
    }

    if (powerCost > 0 && costPerSecond <= 0) {
        if (display) {
            SStrCopy(format, FrameScript_GetText(display->m_globalStringBaseTag, -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(cost, 128, FrameScript_GetText("POWER_DISPLAY_COST", -1, GENDER_NOT_APPLICABLE), powerCost, format);
        } else {
            SStrCopy(format, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(cost, 128, format, powerCost);
        }
    } else if (costPerSecond > 0) {
        if (display) {
            SStrCopy(format, FrameScript_GetText(display->m_globalStringBaseTag, -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(cost, 128, FrameScript_GetText("POWER_DISPLAY_COST_PER_TIME", -1, GENDER_NOT_APPLICABLE), powerCost, format, costPerSecond);
        } else {
            SStrPrintf(format, sizeof(format), "%s_PER_TIME", token);
            SStrPrintf(cost, 128, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), powerCost, costPerSecond);
        }
    }

    return powerCost;
}

// INLINED
static void FormatSpellRange(char* range, float minRange, float maxRange, float enemyMinRange, const char* side) {
    char format[128];
    char distance[32];

    if (maxRange >= 50000.0f) {
        SStrPrintf(range, 128, FrameScript_GetText("SPELL_RANGE_UNLIMITED", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    if (enemyMinRange > 0.0f) {
        SStrPrintf(distance, sizeof(distance), "%d-%d", CMath::fint_(minRange), CMath::fint_(maxRange));
    } else {
        SStrPrintf(distance, sizeof(distance), "%d", CMath::fint_(maxRange));
    }

    if (side) {
        FormatString("SPELL_RANGE_DUAL", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
        SStrPrintf(range, 128, format, FrameScript_GetText(side, -1, GENDER_NOT_APPLICABLE), distance);
    } else {
        FormatString("SPELL_RANGE", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
        SStrPrintf(range, 128, format, distance);
    }
}

// INLINED
static void AddSpellRangeLines(CGTooltip* tooltip, const SpellRec* spell, CGUnit_C* caster, int32_t brief, char* cost, char* range) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    float enemyMin = 0.0f;
    float enemyMax = 0.0f;
    float friendlyMin = 0.0f;
    float friendlyMax = 0.0f;

    if (!brief && !(spell->m_attributes & 0x404) && !(spell->m_attributesExC & 0x40000000)) {
        if (SpellRec::IsMeleeRange(spell) && !SpellRec::HasModifier(spell, 5)) {
            char format[128];
            FormatString("MELEE_RANGE", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
            SStrPrintf(range, 128, format);
        } else {
            Spell_C_GetMinMaxRange(caster, spell, &enemyMin, &enemyMax, 0, nullptr);
            Spell_C_GetMinMaxRange(caster, spell, &friendlyMin, &friendlyMax, 1, nullptr);

            if (SpellRec::IsMinimumRange(spell)) {
                SpellRec::GetMinRange(spell, &enemyMin);
                SpellRec::GetMinRange(spell, &friendlyMin);
            }

            if (fnotequal(enemyMin, friendlyMin) || fnotequal(enemyMax, friendlyMax)) {
                if (enemyMax > 0.0f) {
                    FormatSpellRange(range, enemyMin, enemyMax, enemyMin, "ENEMY");
                }

                if (cost[0]) {
                    tooltip->AddLine(" ", range, textColor, textColor, 0);
                } else {
                    SStrCopy(cost, range, 128);
                    range[0] = '\0';
                    tooltip->AddLine(cost, range, textColor, textColor, 0);
                }

                if (friendlyMax > 0.0f) {
                    FormatSpellRange(range, friendlyMin, friendlyMax, enemyMin, "FRIENDLY");
                }

                if (!cost[0]) {
                    SStrCopy(cost, range, 128);
                    range[0] = '\0';
                }

                tooltip->AddLine(cost, range, textColor, textColor, 0);
                return;
            }

            if (enemyMax > 0.0f) {
                FormatSpellRange(range, enemyMin, enemyMax, enemyMin, nullptr);
            }
        }
    }

    if (!cost[0]) {
        SStrCopy(cost, range, 128);
        range[0] = '\0';
    }

    tooltip->AddLine(cost, range, textColor, textColor, 0);
}

// INLINED
static int32_t AddSpellCastTimeLine(CGTooltip* tooltip, const SpellRec* spell, int32_t isPet, int32_t isInspect, int32_t powerCost, int32_t cooldown) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char castTime[128];
    char recastTime[128];
    char format[128];

    if (spell->m_effect[0] == SPELL_EFFECT_TRADE_SKILL || (spell->m_attributes & 0x40)) {
        return 1;
    }

    if (spell->m_effect[0] == SPELL_EFFECT_ATTACK) {
        return 0;
    }

    int32_t time = SpellRec::GetCastTime(spell, isPet, isInspect, 1);
    int32_t minutes = time >= 60000;

    if (time > 0) {
        FormatString(minutes ? "SPELL_CAST_TIME_MIN" : "SPELL_CAST_TIME_SEC", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
        SStrPrintf(castTime, sizeof(castTime), format, time / (minutes ? 60000.0f : 1000.0f));
    } else if (spell->m_attributesEx & 0x44) {
        FormatString("SPELL_CAST_CHANNELED", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    } else if (time < 0) {
        FormatString("SPELL_CAST_TIME_INSTANT", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    } else if (spell->m_attributes & 0x404) {
        FormatString("SPELL_ON_NEXT_SWING", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    } else if (spell->m_attributes & 2) {
        FormatString("SPELL_ON_NEXT_RANGED", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    } else if (!spell->m_powerType && powerCost > 0) {
        FormatString("SPELL_CAST_TIME_INSTANT", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    } else {
        FormatString("SPELL_CAST_TIME_INSTANT_NO_MANA", -1, GENDER_NOT_APPLICABLE, castTime, sizeof(castTime));
    }

    if (cooldown <= 0 || (spell->m_attributesExF & 1)) {
        recastTime[0] = '\0';
    } else {
        int32_t recastMinutes = cooldown >= 60000;
        FormatString(recastMinutes ? "SPELL_RECAST_TIME_MIN" : "SPELL_RECAST_TIME_SEC", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
        SStrPrintf(recastTime, sizeof(recastTime), format, cooldown / (recastMinutes ? 60000.0f : 1000.0f));
    }

    tooltip->AddLine(castTime, recastTime, textColor, textColor, 0);
    return 0;
}

// INLINED
static void AddSpellToolLine(CGTooltip* tooltip, const SpellRec* spell, CGPlayer_C* player, int32_t brief) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[4096];
    int32_t first = 1;
    int32_t allFound = 1;

    for (uint32_t i = 0; i < 2; i++) {
        int32_t itemId = spell->m_totem[i];

        if (itemId <= 0) {
            continue;
        }

        WOWGUID guid = { (static_cast<uint64_t>(0x1FE00000) << 32) | static_cast<uint32_t>(spell->m_id) };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, &TooltipSpellCallback, tooltip, true);

        if (!stats) {
            tooltip->m_pendingQueries++;
            continue;
        }

        if (first) {
            first = 0;
            SStrCopy(text, FrameScript_GetText("SPELL_TOTEMS", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        } else {
            SStrPack(text, ", ", sizeof(text));
        }

        CGItem_C* item = player->m_bag.FindItemOfType(spell->m_totem[i], 0);

        if (!item) {
            SStrPack(text, s_tooltipRedColorString, sizeof(text));
            allFound = 0;
        }

        SStrPack(text, stats->m_name[0], sizeof(text));

        if (!item) {
            SStrPack(text, "|r", sizeof(text));
        }
    }

    for (uint32_t i = 0; i < 2; i++) {
        TotemCategoryRec* category = g_totemCategoryDB.GetRecord(spell->m_requiredTotemCategoryID[i]);

        if (!category) {
            continue;
        }

        if (first) {
            first = 0;
            SStrCopy(text, FrameScript_GetText("SPELL_TOTEMS", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        } else {
            SStrPack(text, ", ", sizeof(text));
        }

        if (player->m_bag.FindTotemCategory(spell->m_requiredTotemCategoryID[i], 0)) {
            SStrPack(text, category->m_name, sizeof(text));
        } else {
            SStrPack(text, s_tooltipRedColorString, sizeof(text));
            SStrPack(text, category->m_name, sizeof(text));
            SStrPack(text, "|r", sizeof(text));
            allFound = 0;
        }
    }

    if (!first && (!allFound || !brief)) {
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }
}

// INLINED
static void AddSpellEquippedItemLine(CGTooltip* tooltip, const SpellRec* spell, CGPlayer_C* player, int32_t brief, int32_t isPet) {
    char items[512];
    char format[128];
    char text[512];

    if ((spell->m_targets & 0x10) || spell->m_equippedItemClass < 0 || !spell->m_equippedItemSubclass) {
        return;
    }

    items[0] = '\0';
    int32_t found = 0;

    for (int32_t i = 0; i < g_itemSubClassMaskDB.m_numRecords; i++) {
        ItemSubClassMaskRec* mask = &g_itemSubClassMaskDB.m_records[i];

        if (mask->m_classID == spell->m_equippedItemClass && mask->m_mask == spell->m_equippedItemSubclass) {
            SStrCopy(items, mask->m_name, sizeof(items));
            found = 1;
            break;
        }
    }

    int32_t first = 1;
    int32_t equipped = 1;
    uint32_t slotMask = (spell->m_attributesExC & 0x400) ? 0x8000 : (spell->m_attributesExC & 0x1000000) ? 0x10000 : 0xFFFFFFFF;

    if (!player->EquippedItemMeetSpellRequirements(spell, slotMask)) {
        equipped = 0;
    }

    if ((spell->m_attributesExC & 0x1000400) == 0x1000400 && !player->EquippedItemMeetSpellRequirements(spell, 0x10000)) {
        equipped = 0;
    }

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* subClass = &g_itemSubClassDB.m_records[i];

        if (subClass->m_classID != spell->m_equippedItemClass || !((1 << subClass->m_subClassID) & spell->m_equippedItemSubclass) || found) {
            continue;
        }

        if (first) {
            first = 0;
        } else {
            SStrPack(items, ", ", sizeof(items));
        }

        SStrPack(items, subClass->m_verboseName && *subClass->m_verboseName ? subClass->m_verboseName : subClass->m_displayName, sizeof(items));
    }

    if (isPet) {
        equipped = 1;
    }

    if (!items[0] || (equipped && brief)) {
        return;
    }

    SStrCopy(format, FrameScript_GetText(!equipped && brief ? "SPELL_EQUIPPED_ITEM_NOSPACE" : "SPELL_EQUIPPED_ITEM", -1, GENDER_NOT_APPLICABLE), sizeof(format));
    SStrPrintf(text, sizeof(text), format, items);
    const CImVector& color = *reinterpret_cast<const CImVector*>(equipped ? &s_tooltipTextColor : &s_tooltipRedColor);
    tooltip->AddLine(text, nullptr, color, color, 1);
}

// INLINED
static void AddSpellFormLine(CGTooltip* tooltip, const SpellRec* spell, CGUnit_C* caster, int32_t brief) {
    char forms[512];
    char format[128];
    char text[128];

    if (!spell->m_shapeshiftMask[0] && !spell->m_shapeshiftMask[1]) {
        return;
    }

    int32_t first = 1;
    int32_t needsForm = spell->m_attributesExB & 0x80000;

    if (!needsForm) {
        uint32_t count = g_spellShapeshiftFormDB.m_numRecords;

        if (count > 64) {
            count = 64;
        }

        for (uint32_t i = 0; i < count; i++) {
            if (!SpellRec::UsableInShapeshift(spell, i) || static_cast<int32_t>(i) >= g_spellShapeshiftFormDB.m_numRecords) {
                continue;
            }

            SpellShapeshiftFormRec* form = &g_spellShapeshiftFormDB.m_records[i];

            if (!form->m_name || !*form->m_name) {
                continue;
            }

            if (first) {
                SStrCopy(forms, form->m_name, sizeof(forms));
                first = 0;
            } else {
                SStrPack(forms, ", ", sizeof(forms));
                SStrPack(forms, form->m_name, sizeof(forms));
            }
        }
    }

    int32_t usable = !caster || SpellRec::UsableInShapeshift(spell, caster->GetShapeshiftForm() - 1) || (needsForm && !caster->IsShapeShifted()) || caster->AffectedByAura(SPELL_AURA_MOD_IGNORE_SHAPESHIFT, spell);

    if (first || (usable && brief)) {
        return;
    }

    SStrCopy(format, FrameScript_GetText(!usable && brief ? "SPELL_REQUIRED_FORM_NOSPACE" : "SPELL_REQUIRED_FORM", -1, GENDER_NOT_APPLICABLE), sizeof(format));
    SStrPrintf(text, sizeof(text), format, forms);
    const CImVector& color = *reinterpret_cast<const CImVector*>(usable ? &s_tooltipTextColor : &s_tooltipRedColor);
    tooltip->AddLine(text, nullptr, color, color, 0);
}

// INLINED
static void AddSpellReputationLine(CGTooltip* tooltip, const SpellRec* spell, CGPlayer_C* player, int32_t brief) {
    char token[128];
    char text[128];

    if (!spell->m_minFactionID) {
        return;
    }

    int32_t reached = GetRepListRepValue(spell->m_minFactionID) >= CGReputationInfo::m_reactionThresholds[spell->m_minReputation];

    if (reached && brief) {
        return;
    }

    FactionRec* faction = g_factionDB.GetRecord(spell->m_minFactionID);
    SStrPrintf(token, sizeof(token), "FACTION_STANDING_LABEL%d", spell->m_minReputation + 1);
    const char* name = faction ? faction->m_name : "UNKNOWN";
    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_REPUTATION", -1, GENDER_NOT_APPLICABLE), name, player->GetFrameScriptText(token, -1));
    const CImVector& color = *reinterpret_cast<const CImVector*>(reached ? &s_tooltipTextColor : &s_tooltipRedColor);
    tooltip->AddLine(text, nullptr, color, color, 0);
}

// INLINED
static void AddSpellReagentLine(CGTooltip* tooltip, const SpellRec* spell, CGPlayer_C* player, int32_t brief) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[4096];
    char reagent[128];
    int32_t first = 1;
    int32_t allFound = 1;

    for (uint32_t i = 0; i < 8; i++) {
        int32_t itemId = spell->m_reagent[i];

        if (itemId <= 0) {
            continue;
        }

        WOWGUID guid = { (static_cast<uint64_t>(0x1FE00000) << 32) | static_cast<uint32_t>(spell->m_id) };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, &TooltipSpellCallback, tooltip, true);

        if (!stats) {
            tooltip->m_pendingQueries++;
            continue;
        }

        if (first) {
            first = 0;
            SStrCopy(text, FrameScript_GetText("SPELL_REAGENTS", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        } else {
            SStrPack(text, ", ", sizeof(text));
        }

        const char* name = stats->m_name[0];

        if (spell->m_reagentCount[i] <= 1) {
            SStrCopy(reagent, name, sizeof(reagent));
        } else {
            SStrPrintf(reagent, sizeof(reagent), "%s (%d)", name, spell->m_reagentCount[i]);
        }

        if (player->m_bag.GetItemTypeCount(spell->m_reagent[i], 0) >= spell->m_reagentCount[i]) {
            SStrPack(text, reagent, sizeof(text));
        } else {
            SStrPack(text, s_tooltipRedColorString, sizeof(text));
            SStrPack(text, reagent, sizeof(text));
            SStrPack(text, "|r", sizeof(text));
            allFound = 0;
        }
    }

    if (!first && (!allFound || !brief)) {
        tooltip->AddLine(text, nullptr, textColor, textColor, 1);
    }
}

// INLINED
static int32_t AddSpellEffectChanceLine(CGTooltip* tooltip, const SpellRec* spell, CGPlayer_C* player, int32_t isTradeSkill) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[128];
    int32_t effect = spell->m_effect[0];

    if (effect != SPELL_EFFECT_ATTACK && !(isTradeSkill && (effect == SPELL_EFFECT_DODGE || effect == SPELL_EFFECT_BLOCK || effect == SPELL_EFFECT_PARRY))) {
        return 0;
    }

    float chance;
    const char* token;

    switch (effect) {
        case SPELL_EFFECT_DODGE:
            chance = player->m_player->m_playerDodgePercentage;
            token = "CHANCE_TO_DODGE";
            break;

        case SPELL_EFFECT_PARRY:
            chance = player->m_player->m_playerParryPercentage;
            token = "CHANCE_TO_PARRY";
            break;

        case SPELL_EFFECT_BLOCK:
            chance = player->m_player->m_playerBlockPercentage;
            token = "CHANCE_TO_BLOCK";
            break;

        case SPELL_EFFECT_ATTACK:
            chance = player->m_player->m_playerCritPercentage;
            token = "CHANCE_TO_CRIT";
            break;

        default:
            return 0;
    }

    SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), chance);
    tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    return 1;
}

// INLINED
static void AddSpellUseAllPowerLine(CGTooltip* tooltip, const SpellRec* spell, const PowerDisplayRec* display) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char power[256];
    char text[128];

    if (display) {
        SStrPrintf(power, sizeof(power), FrameScript_GetText(display->m_globalStringBaseTag, -1, GENDER_NOT_APPLICABLE));
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SPELL_USE_ALL_POWER_DISPLAY", -1, GENDER_NOT_APPLICABLE), power);
    } else {
        const char* token = static_cast<uint32_t>(spell->m_powerType) > 6 ? "SPELL_USE_ALL_HEALTH" : s_useAllPowerTokens[spell->m_powerType];
        SStrCopy(text, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(text));
    }

    tooltip->AddLine(text, nullptr, textColor, textColor, 0);
}

// VERIFY: 9ae7315a manual
// OFFSET: 0x006237C0
void CGTooltip::AddDeltaHeader(int32_t* first) {
    if (!*first) {
        return;
    }

    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    this->AddLine(" ", nullptr, titleColor, titleColor, 0);
    this->AddLine(FrameScript_GetText("ITEM_DELTA_DESCRIPTION", -1, GENDER_NOT_APPLICABLE), nullptr, titleColor, titleColor, 1);
    *first = 0;
}

// VERIFY: 2d962f56 auto
// OFFSET: 0x00623810
void CGTooltip::AddColoredItemStat(int32_t value, const char* name, int32_t* first) {
    if (!value) {
        return;
    }

    this->AddDeltaHeader(first);
    const char* color = value > 0 ? s_colorStrings[3] : s_negativeDeltaColor;
    char text[512];
    SStrPrintf(text, sizeof(text), "%s%c%d%s %s", color, value > 0 ? '+' : '-', abs(value), "|r", name);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    this->AddLine(text, nullptr, textColor, textColor, 0);
}

// VERIFY: 8edeac88 manual
// OFFSET: 0x006238A0
int32_t CGTooltip::SetSpell(int32_t spellId, int32_t brief, uint32_t cooldown, int32_t isPet, int32_t showRank, int32_t isInspect, int32_t a8, const TalentRec* talent, CGTalentGroup_C* talentGroup, int32_t append, int32_t usePreview, int32_t a13, int32_t rank, int32_t maxRank, int32_t showLearn) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    char text[128];
    char format[128];
    char cost[128];
    char range[128];
    char description[2048];

    if (!append) {
        this->ClearTooltip();
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        this->Hide();
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        if (!append) {
            this->Hide();
        }

        return 0;
    }

    if (!append) {
        this->m_spellId = spellId;
        this->m_pendingQueries = 0;
    }

    CGUnit_C* caster;

    if (isInspect) {
        caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGCharacterInfo::m_inspectTarget, TYPE_UNIT, __FILE__, __LINE__));
    } else if (isPet) {
        caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    } else {
        caster = player;
    }

    int32_t tradeSkill = 0;
    int32_t createsItem = 0;

    if (spell.m_attributes & 0x20) {
        for (uint32_t i = 0; i < 3; i++) {
            int32_t effect = spell.m_effect[i];

            if (effect == SPELL_EFFECT_ENCHANT_ITEM) {
                tradeSkill = 1;
                break;
            }

            if (effect == SPELL_EFFECT_CREATE_ITEM || effect == SPELL_EFFECT_CREATE_ITEM_2 || effect == SPELL_EFFECT_CREATE_RANDOM_ITEM) {
                tradeSkill = 1;
                createsItem = 1;
            }
        }
    }

    if (talent && append) {
        SStrPrintf(text, sizeof(text), "\n%s", FrameScript_GetText("TOOLTIP_TALENT_NEXT_RANK", -1, GENDER_NOT_APPLICABLE));
        this->AddLine(text, nullptr, textColor, textColor, 0);
    } else if (tradeSkill) {
        SkillLineAbilityRec* ability = SpellTableLookupAbility(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], spellId);

        if (ability) {
            SkillLineRec* skillLine = g_skillLineDB.GetRecord(ability->m_skillLine);

            if (skillLine) {
                SStrPrintf(text, sizeof(text), "%s: %s", skillLine->m_displayName, spell.m_name);
                this->AddLine(text, nullptr, titleColor, titleColor, 0);
            }
        }
    } else {
        this->AddLine(spell.m_name, brief || showRank ? spell.m_nameSubtext : nullptr, textColor, grayColor, 0);
    }

    int32_t prereqsMet = 1;

    if (talent && !append) {
        if (maxRank >= 0) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("TOOLTIP_TALENT_RANK", -1, GENDER_NOT_APPLICABLE), rank + 1, maxRank + 1);
            this->AddLine(text, nullptr, textColor, textColor, 0);
        }

        if (rank < 0) {
            prereqsMet = this->AddTalentPrereqs(talent, talentGroup, isInspect, isPet, usePreview);
        }
    }

    cost[0] = '\0';
    range[0] = '\0';
    PowerDisplayRec* display = g_powerDisplayDB.GetRecord(spell.m_powerDisplayID);
    int32_t powerCost = Spell_C_GetPowerCost(&spell, caster);

    if (!brief) {
        powerCost = SetSpellPowerCost(cost, &spell, caster, powerCost, display);
    }

    AddSpellRangeLines(this, &spell, caster, brief, cost, range);

    int32_t recovery = spell.m_recoveryTime;
    int32_t categoryRecovery = spell.m_categoryRecoveryTime;
    Spell_C_ModifySpellValue(&spell, recovery, 11);
    Spell_C_ModifyCategoryRecovery(categoryRecovery, &spell, 0);
    int32_t recast = recovery > categoryRecovery ? recovery : categoryRecovery;
    int32_t isTradeSkill = 0;

    if (!brief && !tradeSkill) {
        isTradeSkill = AddSpellCastTimeLine(this, &spell, isPet, isInspect, powerCost, recast);
    }

    if (!isPet) {
        AddSpellToolLine(this, &spell, player, brief);
    }

    AddSpellEquippedItemLine(this, &spell, player, brief, isPet);
    AddSpellFormLine(this, &spell, caster, brief);
    AddSpellReputationLine(this, &spell, player, brief);

    if (isTradeSkill && caster && static_cast<int32_t>(caster->m_unit->m_unitFieldLevel) < spell.m_baseLevel) {
        SStrCopy(format, FrameScript_GetText("ITEM_MIN_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, spell.m_baseLevel);
        this->AddLine(text, nullptr, redColor, redColor, 0);
    }

    if (!isPet && !Spell_C_HasNoReagentCost(player, &spell)) {
        AddSpellReagentLine(this, &spell, player, brief);
    }

    int32_t result = 0;

    if (cooldown) {
        CGTooltip::GetDurationString(text, sizeof(text), cooldown, "ITEM_COOLDOWN_TIME", nullptr, 1, 0);
        this->AddLine(text, nullptr, textColor, textColor, 0);
        result = 1;
    }

    if (!brief) {
        if (AddSpellEffectChanceLine(this, &spell, player, isTradeSkill)) {
            result = 1;
        }

        if (spell.m_attributesEx & 2) {
            AddSpellUseAllPowerLine(this, &spell, display);
        }

        if (spell.m_description && *spell.m_description) {
            SpellParserParseText(&spell, description, sizeof(description), isPet, isInspect, 0, 0, 1, 0);
            this->AddLine(description, nullptr, titleColor, titleColor, 1);
        }

        if (talent && !isInspect && showLearn) {
            this->AddTalentLearnText(talent, talentGroup, prereqsMet, rank, maxRank, 0, a8, isPet, usePreview);
        }

        if (createsItem && spell.m_effectItemType[0]) {
            this->m_itemLinkInfo.Clear();
            WOWGUID requester = { 0 };
            WOWGUID itemGuid = { 0 };
            this->SetItem(spell.m_effectItemType[0], &requester, &itemGuid, 0, 0, 0, 1, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 0, 0, 1);
        }
    }

    if (this->m_onTooltipSetSpell.m_luaRef) {
        this->RunScript(this->m_onTooltipSetSpell, 0, nullptr);
    }

    this->Show();
    this->CalculateSize();
    return result;
}

// VERIFY: dc2ceb4a auto
// OFFSET: 0x006081D0
CGItemStatsSummary::CGItemStatsSummary() {
    this->Create();
}

// VERIFY: 16a2d627 auto
// OFFSET: 0x0061A0D0
void CGTooltip::AddMoney(int32_t money, int32_t maxMoney) {
    if (!this->m_onTooltipAddMoney.m_luaRef) {
        return;
    }

    lua_State* L = FrameScript_GetContext();
    lua_pushnumber(L, money);

    if (maxMoney <= 0) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, maxMoney);
    }

    this->RunScript(this->m_onTooltipAddMoney, 2, nullptr);
}

// VERIFY: 37e7fe43 auto
// OFFSET: 0x0061A1C0
static const char* GetColorblindSuffix(int32_t index) {
    CVar* colorblindMode = CVar::Lookup("colorblindMode");

    if (colorblindMode && colorblindMode->m_intValue) {
        return s_colorblindSuffixes[index];
    }

    return "";
}

// VERIFY: 3cef9206 auto
// OFFSET: 0x0061A1F0
static const CImVector* GetSkillColor(int32_t required, int32_t current, const char** suffix) {
    int32_t index = 4;

    if (current >= required + 100) {
        index = 0;
    } else if (current >= required + 50) {
        index = 1;
    } else if (current >= required + 25) {
        index = 2;
    } else if (current >= required) {
        index = 3;
    }

    CVar* colorblindMode = CVar::Lookup("colorblindMode");

    if (colorblindMode && colorblindMode->m_intValue) {
        *suffix = s_colorblindSuffixes[index];
    } else {
        *suffix = "";
    }

    return reinterpret_cast<const CImVector*>(&s_skillColors[index]);
}

// VERIFY: c9abb8d7 auto
// OFFSET: 0x0061A600
static int32_t SortItemSetBonuses(const void* a, const void* b) {
    uint8_t first = *static_cast<const uint8_t*>(a);
    uint8_t second = *static_cast<const uint8_t*>(b);
    int32_t firstThreshold = s_sortItemSet->m_setThreshold[first];
    int32_t secondThreshold = s_sortItemSet->m_setThreshold[second];

    if (firstThreshold > secondThreshold) {
        return 1;
    }

    if (firstThreshold < secondThreshold) {
        return -1;
    }

    return first < second ? -1 : 1;
}

// VERIFY: 3d9b5b4c auto
// OFFSET: 0x0061A650
int32_t GetScalingStatBudget(const ScalingStatValuesRec* values, uint32_t flags) {
    if (!values) {
        return 0;
    }

    switch (flags & 0x4001F) {
        case 0x1:
            return values->m_shoulderBudget;

        case 0x2:
            return values->m_trinketBudget;

        case 0x4:
            return values->m_weaponBudget1h;

        case 0x8:
            return values->m_primaryBudget;

        case 0x10:
            return values->m_rangedBudget;

        case 0x40000:
            return values->m_tertiaryBudget;
    }

    return 0;
}

// VERIFY: 68b9f580 manual
// OFFSET: 0x0061A6B0
int32_t GetScalingStatArmor(const ScalingStatValuesRec* values, uint32_t flags) {
    if (!values) {
        return 0;
    }

    switch (flags & 0xF801E0) {
        case 0x20:
            return values->m_clothShoulderArmor;

        case 0x40:
            return values->m_leatherShoulderArmor;

        case 0x80:
            return values->m_mailShoulderArmor;

        case 0x100:
            return values->m_plateShoulderArmor;

        case 0x80000:
            return values->m_clothCloakArmor;

        case 0x100000:
            return values->m_clothChestArmor;

        case 0x200000:
            return values->m_leatherChestArmor;

        case 0x400000:
            return values->m_mailChestArmor;

        case 0x800000:
            return values->m_plateChestArmor;
    }

    return 0;
}

// VERIFY: 716dc312 auto
// OFFSET: 0x0061A830
void GetScalingStatDamage(const ScalingStatValuesRec* values, uint32_t flags, float* minDamage, float* maxDamage) {
    float dps = 0.0f;

    if (minDamage) {
        *minDamage = 0.0f;
    }

    if (maxDamage) {
        *maxDamage = 0.0f;
    }

    if (!minDamage || !maxDamage || !values) {
        return;
    }

    float variance = 0.3f;

    switch (flags & 0x7E00) {
        case 0x200:
            dps = static_cast<float>(values->m_weaponDps1h);
            break;

        case 0x400:
            dps = static_cast<float>(values->m_weaponDps2h);
            variance = 0.2f;
            break;

        case 0x800:
            dps = static_cast<float>(values->m_spellcasterDps1h);
            break;

        case 0x1000:
            dps = static_cast<float>(values->m_spellcasterDps2h);
            variance = 0.2f;
            break;

        case 0x2000:
            dps = static_cast<float>(values->m_rangedDps);
            break;

        case 0x4000:
            dps = static_cast<float>(values->m_wandDps);
            break;
    }

    *minDamage = dps - dps * variance;
    *maxDamage = dps * (variance + 1.0f);
}

// VERIFY: 64b947f2 manual
// OFFSET: 0x0061A8F0
int32_t GetFeralAttackPower(const ItemStats_C* stats, float damage) {
    if (stats->m_class == 2 && (s_inventoryTypeSlots[stats->m_inventoryType] & 0x8000)) {
        double dps = damage / (stats->m_delay * 0.001f);

        if (dps > 54.81f) {
            return static_cast<int32_t>(floor((dps - 54.81f) * 14.0f + 0.5f));
        }
    }

    return 0;
}

// VERIFY: 7d08691d manual
// OFFSET: 0x0061ABF0
void CGTooltip::GetItemDurationString(char* buffer, uint32_t size, uint64_t time, const char* token) {
    if (!buffer || !token) {
        return;
    }

    int32_t units = 0;
    *buffer = '\0';
    char part[256] = { 0 };
    char text[1024] = { 0 };

    if (time >= 86400000) {
        SStrPrintf(part, sizeof(part), FrameScript_GetText("DAYS_ABBR", -1, GENDER_NOT_APPLICABLE), static_cast<uint32_t>(time) / 86400000);
        SStrPack(text, part, sizeof(text));
        time -= time / 86400000 * 86400000;
        units = 1;
    }

    if (time >= 3600000) {
        if (text[0]) {
            SStrPack(text, FrameScript_GetText("TIME_UNIT_DELIMITER", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        }

        SStrPrintf(part, sizeof(part), FrameScript_GetText("HOURS_ABBR", -1, GENDER_NOT_APPLICABLE), static_cast<uint32_t>(time) / 3600000);
        SStrPack(text, part, sizeof(text));
        time -= time / 3600000 * 3600000;
        units++;
    }

    if (units < 2) {
        if (time >= 60000) {
            if (text[0]) {
                SStrPack(text, FrameScript_GetText("TIME_UNIT_DELIMITER", -1, GENDER_NOT_APPLICABLE), sizeof(text));
            }

            SStrPrintf(part, sizeof(part), FrameScript_GetText("MINUTES_ABBR", -1, GENDER_NOT_APPLICABLE), static_cast<uint32_t>(time) / 60000);
            SStrPack(text, part, sizeof(text));
            time -= time / 60000 * 60000;
            units++;
        }

        if (units < 2 && time) {
            if (text[0]) {
                SStrPack(text, FrameScript_GetText("TIME_UNIT_DELIMITER", -1, GENDER_NOT_APPLICABLE), sizeof(text));
            }

            SStrPrintf(part, sizeof(part), FrameScript_GetText("SECONDS_ABBR", -1, GENDER_NOT_APPLICABLE), static_cast<uint32_t>(time / 1000));
            SStrPack(text, part, sizeof(text));
        }
    }

    SStrPrintf(buffer, size, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), text);
}

// VERIFY: 5b4cf4a8 manual
// OFFSET: 0x0061B4F0
void __stdcall CGTooltip::FormatStringForXML(const char* text, char* buffer) {
    int32_t length = strlen(text);
    const char* processed = LanguageProcess(text);
    int32_t out = 0;

    for (int32_t i = 0; i < length; i++) {
        char c = processed[i];

        if (c == '<') {
            buffer[out++] = '&';
            buffer[out++] = 'l';
            buffer[out++] = 't';
            buffer[out++] = ';';
            i++;
        } else if (c == '>') {
            buffer[out++] = '&';
            buffer[out++] = 'g';
            buffer[out++] = 't';
            buffer[out++] = ';';
            i++;
        } else if (c == '&') {
            buffer[out++] = '&';
            buffer[out++] = 'a';
            buffer[out++] = 'm';
            buffer[out++] = 'p';
            buffer[out++] = ';';
            i++;
        }

        buffer[out++] = processed[i];
    }

    buffer[out] = '\0';
}

// VERIFY: 2fdbd3e2 manual
// OFFSET: 0x0061C0B0
int32_t GetEnchantmentConditionText(CGPlayer_C* player, const SpellItemEnchantmentRec* enchant, char* buffer, uint32_t size, int32_t newline) {
    *buffer = '\0';

    if (!enchant || !enchant->m_srcItemID || !enchant->m_conditionID || !player) {
        return 1;
    }

    SpellItemEnchantmentConditionRec* condition = g_spellItemEnchantmentConditionDB.GetRecord(enchant->m_conditionID);

    if (!condition) {
        return 1;
    }

    int32_t allMet = 1;

    for (uint32_t i = 0; i < 5; i++) {
        char line[1024];
        char rightName[1024];
        char text[1024];
        char leftName[1024];
        int32_t met = 1;
        const char* color;

        if (newline || *buffer) {
            SStrPack(buffer, "\n", size);
        }

        uint8_t leftType = condition->m_ltOperandType[i];
        uint8_t rightType = condition->m_rtOperandType[i];
        uint32_t leftIndex = s_gemColorIndices[leftType];
        int32_t left = leftIndex >= 4 ? 0 : player->m_gemCounts[leftIndex];

        if (leftType >= 5) {
            SStrCopy(leftName, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE), sizeof(leftName));
        } else {
            FormatString(s_gemColorTokens[leftType], -1, GENDER_NOT_APPLICABLE, leftName, sizeof(leftName));
        }

        if (rightType) {
            uint32_t rightIndex = s_gemColorIndices[rightType];
            int32_t right = rightIndex >= 4 ? 0 : player->m_gemCounts[rightIndex];

            if (rightType >= 5) {
                SStrCopy(rightName, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE), sizeof(rightName));
            } else {
                FormatString(s_gemColorTokens[rightType], -1, GENDER_NOT_APPLICABLE, rightName, sizeof(rightName));
            }

            switch (condition->m_operator[i]) {
                case 0:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_EQUAL_COMPARE", -1, GENDER_NOT_APPLICABLE), leftName, rightName);
                    met = left == right;
                    break;

                case 1:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_NOT_EQUAL_COMPARE", -1, GENDER_NOT_APPLICABLE), leftName, rightName);
                    met = left != right;
                    break;

                case 2:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_COMPARE", -1, GENDER_NOT_APPLICABLE), rightName, leftName);
                    met = right > left;
                    break;

                case 3:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_COMPARE", -1, GENDER_NOT_APPLICABLE), leftName, rightName);
                    met = left > right;
                    break;

                case 4:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_EQUAL_COMPARE", -1, GENDER_NOT_APPLICABLE), rightName, leftName);
                    met = right >= left;
                    break;

                case 5:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_EQUAL_COMPARE", -1, GENDER_NOT_APPLICABLE), leftName, rightName);
                    met = left >= right;
                    break;
            }
        } else {
            int32_t value = condition->m_rtOperand[i];

            switch (condition->m_operator[i]) {
                case 0:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_EQUAL_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left == value;
                    break;

                case 1:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_NOT_EQUAL_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left != value;
                    break;

                case 2:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_LESS_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left < value;
                    break;

                case 3:
                    value++;
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left >= value;
                    break;

                case 4:
                    value++;
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_LESS_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left < value;
                    break;

                case 5:
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ENCHANT_CONDITION_MORE_VALUE", -1, GENDER_NOT_APPLICABLE), value, leftName);
                    met = left >= value;
                    break;
            }
        }

        color = met ? s_colorStrings[0] : s_colorStrings[1];
        SStrPrintf(line, sizeof(line), "  %s%s%s%s", color, FrameScript_GetText("ENCHANT_CONDITION_REQUIRES", -1, GENDER_NOT_APPLICABLE), text, "|r");
        SStrPack(buffer, line, size);

        if (!met || !allMet) {
            allMet = 0;
        }

        if (!condition->m_logic[i]) {
            break;
        }
    }

    return allMet;
}

// VERIFY: 05aca7fe manual
// OFFSET: 0x0061C8B0
void CGTooltip::AddTexture(const char* texture, const CAaRect& coords, const CImVector& color) {
    uint32_t index = this->m_numTextures;

    if (index >= 10 || !this->m_textures[index]) {
        return;
    }

    uint32_t lines = this->m_numLines;

    if (lines < 2) {
        return;
    }

    uint32_t line = lines - 1;
    float offset;

    if (index && this->m_textureLines[index - 1] == lines - 2) {
        offset = 0.0f;
    } else {
        offset = this->m_textures[index]->GetWidth() + 0.0049999999f;
    }

    CSimpleFontString** strings = &this->m_leftStrings.m_data[line];
    strings[0]->SetPoint(FRAMEPOINT_TOPLEFT, strings[-1], FRAMEPOINT_BOTTOMLEFT, offset, NDCToDDCWidth(-2.0f / (CoordinateGetAspectCompensation() * 1024.0f)), 1);
    this->m_textures[this->m_numTextures]->MarkFramePointsUnused();
    this->m_textures[this->m_numTextures]->SetPoint(FRAMEPOINT_TOPRIGHT, this->m_leftStrings.m_data[line], FRAMEPOINT_TOPLEFT, -0.0049999999f, 0.0f, 1);

    if (line < this->m_maxLines - 1) {
        CSimpleFontString** next = &this->m_leftStrings.m_data[line];

        if (next[1]) {
            CSimpleTexture* current = this->m_textures[this->m_numTextures];
            float y = NDCToDDCWidth(-2.0f / (CoordinateGetAspectCompensation() * 1024.0f));
            next[1]->SetPoint(FRAMEPOINT_TOPLEFT, next[0], FRAMEPOINT_BOTTOMLEFT, -(current->GetWidth() + 0.0049999999f), y, 1);
        }
    }

    this->m_textures[this->m_numTextures]->SetTexture(texture, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
    this->m_textures[this->m_numTextures]->SetTexCoord(reinterpret_cast<const CRect&>(coords));
    this->m_textures[this->m_numTextures]->SetVertexColor(color);
    CSimpleTexture* shown = this->m_textures[this->m_numTextures];
    shown->m_shown = 1;
    shown->ShowThis();
    this->m_textureLines[this->m_numTextures++] = line;
}

// VERIFY: c9ca1286 manual
// OFFSET: 0x0061E740
int32_t CGTooltip::GetDisplayedScalingItemLevel(const ItemStats_C* stats, const ItemLinkInfo* link, WOWGUID target, int32_t minimumLevel) {
    ScalingStatDistributionRec* distribution = g_scalingStatDistributionDB.GetRecord(stats->m_scalingStatDistribution);
    int32_t level = 0;

    if (minimumLevel) {
        level = 1;
    } else if ((!link || !(level = link->m_level)) && target != CGMerchantInfo::m_merchant) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            level = unit->m_unit->m_unitFieldLevel;
        }
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && !level) {
        level = player->m_unit->m_unitFieldLevel;
    }

    if (distribution) {
        int32_t required = stats->m_requiredLevel;
        int32_t highest = required <= level ? level : required;

        if (highest >= distribution->m_maxlevel) {
            level = distribution->m_maxlevel;
        } else if (required > level) {
            level = stats->m_requiredLevel;
        }
    }

    return level > 1 ? level : 1;
}

// VERIFY: 12a957b9 manual
// OFFSET: 0x00623590
void CGTooltip::AddSpellReagents(const SpellRec* spell, const char* token, void (*callback)(int32_t id, const WOWGUID& guid, void* param, bool success)) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char line[4096];
    char text[4096];
    char reagent[1024];
    int32_t first = 1;

    for (uint32_t i = 0; i < 8; i++) {
        int32_t itemId = spell->m_reagent[i];

        if (itemId <= 0) {
            continue;
        }

        WOWGUID guid = { (static_cast<uint64_t>(0x1FE00000) << 32) | static_cast<uint32_t>(spell->m_id) };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, callback, this, true);

        if (!stats) {
            continue;
        }

        if (first) {
            first = 0;
            text[0] = '\0';
        } else {
            SStrPack(text, ", ", sizeof(text));
        }

        const char* name = stats->m_name[0];

        if (spell->m_reagentCount[i] <= 1) {
            SStrCopy(reagent, name, sizeof(reagent));
        } else {
            SStrPrintf(reagent, sizeof(reagent), "%s (%d)", name, spell->m_reagentCount[i]);
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player && player->m_bag.GetItemTypeCount(spell->m_reagent[i], 0) >= spell->m_reagentCount[i]) {
            SStrPack(text, reagent, sizeof(text));
        } else {
            SStrPack(text, s_tooltipRedColorString, sizeof(text));
            SStrPack(text, reagent, sizeof(text));
            SStrPack(text, "|r", sizeof(text));
        }
    }

    if (!first) {
        SStrPrintf(line, sizeof(line), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), text);
        this->AddLine(line, nullptr, textColor, textColor, 1);
    }
}

// VERIFY: c31b22a0 auto
// OFFSET: 0x00623760
int32_t CGTooltip::SetItemNoStats() {
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    this->AddLine(FrameScript_GetText("RETRIEVING_ITEM_INFO", -1, GENDER_NOT_APPLICABLE), nullptr, redColor, redColor, 0);

    if (this->m_onTooltipSetItem.m_luaRef) {
        this->RunScript(this->m_onTooltipSetItem, 0, nullptr);
    }

    this->Show();
    this->CalculateSize();
    return 0;
}

// VERIFY: 8eac5de9 manual
// OFFSET: 0x00625F00
int32_t SetTooltipUnitAura(CGTooltip* tooltip, ScriptAuraInfo* info) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(info->m_guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        if ((unit->m_auraEffectMask[36] & 0x80) && ClntObjMgrGetActivePlayer() && !unit->IsFriendly(ClntObjMgrGetActivePlayerObj())) {
            return 0;
        }

        if (unit->m_obj->m_guid != CGCharacterInfo::m_inspectTarget) {
            CGPetInfo::GetPet(0);
        }

        if (!(unit->m_modelFlags & 0x10000)) {
            uint32_t slot = unit->GetSortedAuraSlot(info->m_index, info->m_name, info->m_rank, info->m_filter);

            if (static_cast<int32_t>(slot) < 0) {
                return 0;
            }

            uint8_t flags = unit->GetAuraFlags(slot);
            uint8_t stackCount = unit->GetAuraStackCount(slot);
            uint8_t level = unit->GetAuraLevel(slot);
            tooltip->SetBuff(unit->GetAuraSpellId(slot), level, stackCount, flags, 0, 0);
            AuraData* aura = unit->GetAura(slot);

            if (aura && aura->m_duration > 0) {
                int32_t remaining = aura->m_endTime - OsGetAsyncTimeMs();

                if (remaining < 0) {
                    remaining = 0;
                }

                char buffer[1024];
                CGTooltip::GetDurationString(buffer, sizeof(buffer), remaining, "SPELL_TIME_REMAINING", nullptr, 1, 0);
                tooltip->AddLine(buffer, nullptr, 0);
            }

            tooltip->Show();
            return 0;
        }
    }

    int32_t* auras;
    uint8_t* auraFlags;

    if (CGPartyMemberState* state = CGPartyMemberStateRepository::GetState(info->m_guid)) {
        if (!(state->m_flags & 0x1)) {
            return 0;
        }

        auras = state->m_auras;
        auraFlags = state->m_auraFlags;
    } else if (CGPartyPetState* petState = CGPartyMemberStateRepository::FindPetState(info->m_guid)) {
        auras = petState->m_auras;
        auraFlags = petState->m_auraFlags;
    } else {
        return 0;
    }

    if (!auras) {
        return 0;
    }

    bool helpful = info->m_filter & BUFF_FILTER_HELPFUL;
    bool raid = (info->m_filter & BUFF_FILTER_RAID) != 0;
    int32_t found = 0;

    for (uint32_t i = 0; i < 64 && auras[i]; i++) {
        if (helpful != !(auraFlags[i] & 0x80)) {
            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(auras[i], &spell)) {
            continue;
        }

        WOWGUID caster = { 0 };

        if (!CGSpellBook::CanShowAura(&spell, &caster, (auraFlags[i] & 0x8) != 0)) {
            continue;
        }

        if (raid && !(helpful ? CGSpellBook::FindRaidBuff(auras[i]) : CGSpellBook::FindRaidDispel(spell.m_dispelType))) {
            continue;
        }

        if ((auraFlags[i] & 0x10) ? (info->m_filter & BUFF_FILTER_NOT_CANCELABLE) : (info->m_filter & BUFF_FILTER_CANCELABLE)) {
            continue;
        }

        if ((info->m_index >= 0 && info->m_index == found) || (info->m_name && !SStrCmpUTF8I(info->m_name, spell.m_name, STORM_MAX_STR) && (!info->m_rank || !SStrCmpUTF8I(info->m_rank, spell.m_nameSubtext, STORM_MAX_STR)))) {
            tooltip->SetBuff(auras[i], 0, 1, 0, 0, 0);
            tooltip->Show();
            return 0;
        }

        found++;
    }

    return 0;
}

// VERIFY: fd306634 auto
// OFFSET: 0x006265C0
int32_t CGItemStatsSummary::Create(const ItemStats_C* stats, const ItemLinkInfo* link, int32_t level) {
    this->Create();

    if (stats->m_scalingStatValue) {
        this->FillOutScalingItem(stats, level);
        this->CleanStatNames();
        return 1;
    }

    this->AccumulateStats(stats);

    if (link) {
        this->FillOutRandomProperties(link->m_randomPropertiesId, link->m_propertySeed);
    }

    this->CleanStatNames();
    return 1;
}

// VERIFY: 38b9e6db auto
// OFFSET: 0x00626650
void TooltipItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (tooltip->m_spellId) {
        tooltip->SetSpell(tooltip->m_spellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        tooltip->CalculateSize();
    } else if (tooltip->m_itemId) {
        WOWGUID itemGuid = tooltip->m_itemGuid;
        WOWGUID requester = { 0 };
        tooltip->SetItem(tooltip->m_itemId, &requester, &itemGuid, tooltip->m_itemNameOnly, tooltip->m_itemCompare, tooltip->m_itemUseLinkInfo, 0, tooltip->m_itemOwner, tooltip->m_itemSocketPreview, nullptr, tooltip->m_itemHideCharges, nullptr, tooltip->m_itemMinimumLevel, tooltip->m_itemSkipSellPrice, 0);
        tooltip->CalculateSize();
    }
}

// VERIFY: b16ac8ce auto
// OFFSET: 0x006276E0
bool ResolveItemSetItem(ItemSetTooltipData* data, int32_t pass, int32_t itemId, int32_t slot) {
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, WOWGUID{ 0 }, &TooltipItemCallback, data->m_tooltip, true);

    if (!stats || static_cast<int32_t>(stats->m_itemSet) != data->m_set->m_id) {
        return false;
    }

    if (!pass) {
        for (uint32_t i = 0; i < 17; i++) {
            if (!data->m_equipped[i] && itemId == data->m_set->m_itemID[i]) {
                data->m_equipped[i] = itemId;
                data->m_slotUsed[slot] = 1;
                return true;
            }
        }

        return false;
    }

    if (data->m_slotUsed[slot]) {
        return false;
    }

    for (uint32_t i = 0; i < 17; i++) {
        if (data->m_equipped[i] || !data->m_items[i]) {
            continue;
        }

        uint32_t type = stats->m_inventoryType;
        uint32_t setType = data->m_items[i]->m_inventoryType;

        if (type == setType || (type == 5 && setType == 20) || (type == 20 && setType == 5)) {
            data->m_equipped[i] = itemId;
            return true;
        }
    }

    return true;
}

// INLINED
static void AddItemNameLines(CGTooltip* tooltip, const ItemStats_C* stats, int32_t itemId, int32_t randomPropertiesId, int32_t nameOnly, int32_t append, int32_t destroyGem) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char name[1024];
    char format[1024];
    CGItem_C::BuildItemName(name, sizeof(name), itemId, randomPropertiesId);

    if (append) {
        SStrCopy(format, name, sizeof(format));
        SStrPrintf(name, sizeof(name), "\n%s", format);
    }

    if (nameOnly) {
        tooltip->AddLine(name, nullptr, textColor, textColor, 0);
        return;
    }

    const CImVector* nameColor = destroyGem ? &grayColor : reinterpret_cast<const CImVector*>(&s_itemQualityColors[stats->m_quality]);
    tooltip->AddLine(name, nullptr, *nameColor, *nameColor, 0);
    CVar* colorblindMode = CVar::Lookup("colorblindMode");

    if (colorblindMode && colorblindMode->m_intValue) {
        if (stats->m_flags[0] & ITEM_FLAG_HEROIC_TOOLTIP) {
            tooltip->AddLine(FrameScript_GetText("ITEM_HEROIC_EPIC", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        } else {
            SStrPrintf(format, sizeof(format), "ITEM_QUALITY%d_DESC", destroyGem ? 0 : stats->m_quality);
            tooltip->AddLine(FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        }
    } else if (stats->m_flags[0] & ITEM_FLAG_HEROIC_TOOLTIP) {
        tooltip->AddLine(FrameScript_GetText("ITEM_HEROIC", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }
}

// INLINED
static bool AddItemPetitionLines(CGTooltip* tooltip, CGItem_C* item, const WOWGUID* itemGuid) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[4096];

    if (!(item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION)) {
        return true;
    }

    int32_t petitionId = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) ? item->m_item->m_itemFieldEnchantment[0].m_id : 0;
    CGPetition* petition = g_petitionCache.GetRecord(petitionId, *itemGuid, TooltipItemCallback, tooltip, true);

    if (!petition) {
        return false;
    }

    if (petition->m_staticType) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("PETITION_TITLE", -1, GENDER_NOT_APPLICABLE), petition->m_title);
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("GUILD_CHARTER_TITLE", -1, GENDER_NOT_APPLICABLE), petition->m_title);
    }

    tooltip->AddLine(text, nullptr, textColor, textColor, 1);
    NameCache* owner = g_nameDBCache.GetRecord(petition->m_ownerGuid.m_guid, petition->m_ownerGuid, TooltipItemCallback, tooltip, true);

    if (owner) {
        const char* token = petition->m_staticType ? FrameScript_GetText("PETITION_CREATOR", -1, GENDER_NOT_APPLICABLE) : FrameScript_GetText("GUILD_CHARTER_CREATOR", -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(text, sizeof(text), token, owner->m_name);
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    int32_t signatures = item->GetPetitionSignatures();

    if (signatures) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("PETITION_NUM_SIGNATURES", signatures, GENDER_NOT_APPLICABLE), signatures);
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    return true;
}

// INLINED
static void AddItemLockLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, const LockRec* lock) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[4096];
    int32_t spellId = 0;
    int32_t skillValue = 0;
    int32_t requiredSkill = 0;
    int32_t lockIndex = 0;
    const char* suffix = "";
    CImVector lockColor;
    CGItem_C::IsLocked(lock, stats->m_itemLevel, &spellId, &skillValue, &requiredSkill, &lockIndex, nullptr, nullptr);

    if (spellId) {
        lockColor = *GetSkillColor(requiredSkill, skillValue, &suffix);
    } else {
        lockColor = *reinterpret_cast<const CImVector*>(&s_skillColors[1]);
        suffix = GetColorblindSuffix(1);
    }

    bool encrypted = false;

    if (lockIndex) {
        encrypted = lockIndex == 20;
    } else {
        for (uint32_t i = 0; i < 8; i++) {
            if (lock->m_index[i] == 20) {
                encrypted = true;
                break;
            }
        }
    }

    if (encrypted) {
        SStrPrintf(text, sizeof(text), "%s%s", suffix, FrameScript_GetText("ENCRYPTED", -1, GENDER_NOT_APPLICABLE));
    } else {
        SStrPrintf(text, sizeof(text), "%s%s", suffix, FrameScript_GetText("LOCKED", -1, GENDER_NOT_APPLICABLE));
    }

    tooltip->AddLine(text, nullptr, lockColor, lockColor, 0);

    for (uint32_t i = 0; i < 8; i++) {
        int32_t type = lock->m_type[i];

        if (type == 2) {
            int32_t skillSpell = 0;
            int32_t skillCurrent = 0;
            int32_t skillRequired = 0;
            const char* skillSuffix;
            CImVector skillColor = textColor;
            CGItem_C::IsLocked(lock, stats->m_itemLevel, &skillSpell, &skillCurrent, &skillRequired, nullptr, nullptr, nullptr);

            if (skillSpell) {
                char format[128];
                skillColor = *GetSkillColor(skillRequired, skillCurrent, &skillSuffix);
                SStrCopy(format, FrameScript_GetText("ITEM_MIN_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                LockTypeRec* lockType = g_lockTypeDB.GetRecord(lock->m_index[i]);
                SStrPrintf(text, sizeof(text), format, lockType ? lockType->m_name : "UNKNOWN", lock->m_skill[i]);
                tooltip->AddLine(text, nullptr, skillColor, skillColor, 0);
            }

            return;
        }

        if (type == 3) {
            SpellRec spell;

            if (g_spellDB.GetRecord(lock->m_index[i], &spell)) {
                tooltip->AddSpellReagents(&spell, "LOCKED_WITH_SPELL", TooltipItemCallback);
            }

            return;
        }

        if (type == 1) {
            WOWGUID key = item ? item->m_obj->m_guid : WOWGUID{ 0 };
            ItemStats_C* keyStats = g_itemDBCache.GetRecord(lock->m_index[i], key, TooltipItemCallback, tooltip, true);

            if (keyStats) {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("LOCKED_WITH_ITEM", -1, GENDER_NOT_APPLICABLE), keyStats->GetName(0));
                tooltip->AddLine(text, nullptr, textColor, textColor, 0);
            }

            return;
        }
    }
}

// INLINED
static const LockRec* AddItemBindLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, const WOWGUID* requester, int32_t skipLock) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[4096];
    char format[1024];

    if (stats->m_area) {
        AreaTableRec* area = g_areaTableDB.GetRecord(stats->m_area);

        if (area) {
            tooltip->AddLine(area->m_areaName, nullptr, textColor, textColor, 0);
        }
    }

    if (stats->m_map) {
        MapRec* map = g_mapDB.GetRecord(stats->m_map);

        if (map) {
            tooltip->AddLine(map->m_mapName, nullptr, textColor, textColor, 0);
        }
    }

    if (stats->m_flags[0] & ITEM_FLAG_CONJURED) {
        FormatString("ITEM_CONJURED", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
        tooltip->AddLine(format, nullptr, textColor, textColor, 0);
    }

    text[0] = '\0';

    if (item && item->IsBound()) {
        if (stats->m_bonding == 4) {
            FormatString("ITEM_BIND_QUEST", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
        } else if (stats->m_class != 10) {
            if (stats->m_flags[0] & 0x8000000) {
                FormatString("ITEM_ACCOUNTBOUND", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            } else {
                FormatString("ITEM_SOULBOUND", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            }
        }
    }

    if (stats->m_bonding && !text[0] && (stats->m_class != 10 || *requester == CGLootInfo::m_lootObj)) {
        if (stats->m_flags[0] & 0x8000000) {
            FormatString("ITEM_BIND_TO_ACCOUNT", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
        } else {
            switch (stats->m_bonding) {
                case 1:
                    FormatString("ITEM_BIND_ON_PICKUP", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
                    break;

                case 2:
                    FormatString("ITEM_BIND_ON_EQUIP", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
                    break;

                case 3:
                    FormatString("ITEM_BIND_ON_USE", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
                    break;

                case 4:
                    FormatString("ITEM_BIND_QUEST", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
                    break;
            }
        }
    }

    if (text[0]) {
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    if (stats->m_class != 10) {
        if (stats->m_flags[0] & 0x80000) {
            FormatString("ITEM_UNIQUE_EQUIPPABLE", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        } else if (static_cast<int32_t>(stats->m_maxCount) > 0) {
            if (stats->m_maxCount == 1) {
                FormatString("ITEM_UNIQUE", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            } else {
                FormatString("ITEM_UNIQUE_MULTIPLE", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
                SStrPrintf(text, sizeof(text), format, stats->m_maxCount);
            }

            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        } else if (stats->m_itemLimitCategory) {
            ItemLimitCategoryRec* category = g_itemLimitCategoryDB.GetRecord(stats->m_itemLimitCategory);

            if (category->m_flags & 1) {
                FormatString("ITEM_LIMIT_CATEGORY_MULTIPLE", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
            } else {
                FormatString("ITEM_LIMIT_CATEGORY", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
            }

            SStrPrintf(text, sizeof(text), format, category->m_name, category->m_quantity);
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        }
    }

    if (stats->m_startQuest) {
        FormatString("ITEM_STARTS_QUEST", -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    if (skipLock) {
        return nullptr;
    }

    LockRec* lock = g_lockDB.GetRecord(stats->m_lockId);

    if (!lock) {
        if (!tooltip->m_itemUseLinkInfo || !tooltip->m_itemLinkInfo.m_lockId) {
            return nullptr;
        }

        lock = g_lockDB.GetRecord(tooltip->m_itemLinkInfo.m_lockId);

        if (!lock) {
            return nullptr;
        }
    }

    if (item && (item->m_item->m_itemFieldFlags & ITEM_DFLAG_UNLOCKED)) {
        return nullptr;
    }

    return lock;
}

// INLINED
static void AddItemTypeLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player, const ScalingStatValuesRec* values, const ScalingStatDistributionRec* distribution) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& glyphColor = *reinterpret_cast<const CImVector*>(&s_tooltipGlyphColor);
    char slot[1024];
    char format[1024];
    char subClassName[1024];
    ItemSubClassRec* subClass = nullptr;
    subClassName[0] = '\0';

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = g_itemSubClassDB.GetRecordByIndex(i);

        if (rec && rec->m_classID == static_cast<int32_t>(stats->m_class) && rec->m_subClassID == static_cast<int32_t>(stats->m_subClass)) {
            subClass = rec;
            break;
        }
    }

    if (stats->m_inventoryType == 18) {
        if (subClass && subClass->m_displayName && *subClass->m_displayName) {
            SStrCopy(format, FrameScript_GetText("CONTAINER_SLOTS", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(slot, sizeof(slot), format, stats->m_containerSlots, subClass->m_displayName);
            tooltip->AddLine(slot, nullptr, textColor, textColor, 0);
        }

        return;
    }

    if (stats->m_class == 16) {
        for (uint32_t i = 0; i < 5; i++) {
            SpellRec spell;

            if (g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
                for (uint32_t j = 0; j < 3; j++) {
                    if (spell.m_effect[j] != SPELL_EFFECT_APPLY_GLYPH) {
                        continue;
                    }

                    GlyphPropertiesRec* glyph = g_glyphPropertiesDB.GetRecord(spell.m_effectMiscValue[j]);

                    if (glyph) {
                        const char* text = glyph->m_glyphSlotFlags ? FrameScript_GetText("MINOR_GLYPH", -1, GENDER_NOT_APPLICABLE) : FrameScript_GetText("MAJOR_GLYPH", -1, GENDER_NOT_APPLICABLE);
                        tooltip->AddLine(text, nullptr, glyphColor, glyphColor, 0);
                    }
                }
            }
        }

        return;
    }

    if (subClass && stats->m_inventoryType != 16 && !(subClass->m_displayFlags & 1) && subClass->m_displayName && *subClass->m_displayName) {
        SStrCopy(subClassName, subClass->m_displayName, sizeof(subClassName));
    }

    if (stats->m_class != 6) {
        const char* text = FrameScript_GetText(s_inventoryTypeTokens[stats->m_inventoryType], -1, GENDER_NOT_APPLICABLE);
        SStrCopy(slot, text, sizeof(slot));

        if (!*text) {
            slot[0] = '\0';
        }
    } else {
        ItemClassRec* itemClass = g_itemClassDB.GetRecord(6);

        if (itemClass && itemClass->m_className && *itemClass->m_className) {
            SStrCopy(slot, itemClass->m_className, sizeof(slot));
        } else {
            slot[0] = '\0';
        }
    }

    int32_t subClassRed = 0;
    int32_t slotRed = 0;
    uint32_t proficiency = CGPlayer_C::GetProficiency(stats->m_class);

    if (proficiency && !((1 << stats->m_subClass) & proficiency)) {
        if (stats->m_class == 2) {
            int32_t prerequisite = subClass->m_prerequisiteProficiency;

            if (prerequisite != -1) {
                if ((1 << prerequisite) & proficiency) {
                    slotRed = 1;
                } else {
                    subClassRed = 1;
                }
            } else {
                int32_t postrequisite = subClass->m_postrequisiteProficiency;

                if (postrequisite == -1 || !((1 << postrequisite) & proficiency)) {
                    subClassRed = 1;
                } else {
                    slotRed = 1;
                }
            }
        } else {
            int32_t alternate = -1;

            if (values && distribution && stats->m_class == 4 && (stats->m_subClass == 4 || stats->m_subClass == 3)) {
                ChrClassesRec* playerClass = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]);

                if (playerClass) {
                    if (stats->m_subClass == 4 && (playerClass->m_flags & 0x20)) {
                        alternate = ((proficiency & 8) | 0x10) >> 3;
                    } else if (stats->m_subClass == 3 && (playerClass->m_flags & 0x10) && (proficiency & 4)) {
                        alternate = stats->m_subClass - 1;
                    }
                }
            }

            if (alternate == -1) {
                subClassRed = 1;
            } else {
                for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
                    ItemSubClassRec* rec = g_itemSubClassDB.GetRecordByIndex(i);

                    if (rec && rec->m_classID == 4 && rec->m_subClassID == alternate) {
                        if (rec->m_displayName && *rec->m_displayName) {
                            SStrCopy(subClassName, rec->m_displayName, sizeof(subClassName));
                        }

                        break;
                    }
                }
            }
        }
    }

    if (stats->m_inventoryType == 22 && !CGPlayer_C::CanDualWield()) {
        slotRed = 1;
    }

    if (slot[0]) {
        const CImVector& rightColor = subClassRed ? redColor : textColor;
        const CImVector& leftColor = slotRed ? redColor : textColor;
        tooltip->AddLine(slot, subClassName[0] ? subClassName : nullptr, leftColor, rightColor, 0);
    } else if (subClassName[0]) {
        const CImVector& color = subClassRed ? redColor : textColor;
        tooltip->AddLine(subClassName, nullptr, color, color, 0);
    }
}

// INLINED
static float AddItemDamageLines(CGTooltip* tooltip, const ItemStats_C* stats, const ScalingStatValuesRec* values) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    char text[1024];
    char format[1024];
    char speed[1024];
    char school[4096];
    float average = 0.0f;
    int32_t first = 1;

    if ((stats->m_scalingStatValue & 0x7E00) && values) {
        float minDamage;
        float maxDamage;
        GetScalingStatDamage(values, stats->m_scalingStatValue, &minDamage, &maxDamage);

        if (stats->m_delay) {
            float delay = static_cast<float>(static_cast<int32_t>(stats->m_delay));
            minDamage = minDamage * delay * 0.001f;
            maxDamage = 0.001f * (delay * maxDamage);
        }

        average = (minDamage + maxDamage) * 0.5f;
        SStrCopy(format, FrameScript_GetText("DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, static_cast<int32_t>(minDamage), static_cast<int32_t>(maxDamage));
        SStrCopy(format, FrameScript_GetText("SPEED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(speed, sizeof(speed), "%s %.2f", format, static_cast<double>(static_cast<float>(static_cast<int32_t>(stats->m_delay)) * 0.001f));
        tooltip->AddLine(text, speed, textColor, textColor, 0);
        first = 0;
    } else {
        for (uint32_t i = 0; i < 2; i++) {
            float minValue = stats->m_damageMin[i];
            float maxValue = stats->m_damageMax[i];
            int32_t minDamage = static_cast<int32_t>(minValue <= 0.0f ? minValue - 0.99995f : minValue);
            int32_t maxDamage = static_cast<int32_t>(maxValue > 0.0f ? maxValue + 0.99995f : maxValue);

            if (!minDamage && !maxDamage) {
                continue;
            }

            if (stats->m_damageType[i]) {
                SStrPrintf(format, sizeof(format), "SPELL_SCHOOL%d_CAP", stats->m_damageType[i]);
                SStrCopy(school, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), sizeof(school));

                if (stats->m_class == 6) {
                    float ammo = (minDamage + maxDamage) * 0.5f;
                    SStrCopy(format, FrameScript_GetText(first ? "AMMO_SCHOOL_DAMAGE_TEMPLATE" : "PLUS_AMMO_SCHOOL_DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, static_cast<double>(ammo), school);
                } else if (minDamage == maxDamage) {
                    SStrCopy(format, FrameScript_GetText(first ? "SINGLE_DAMAGE_TEMPLATE_WITH_SCHOOL" : "PLUS_SINGLE_DAMAGE_TEMPLATE_WITH_SCHOOL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, minDamage, school);
                } else {
                    SStrCopy(format, FrameScript_GetText(first ? "DAMAGE_TEMPLATE_WITH_SCHOOL" : "PLUS_DAMAGE_TEMPLATE_WITH_SCHOOL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, minDamage, maxDamage, school);
                }
            } else {
                if (stats->m_class == 6) {
                    float ammo = 0.5f * (minDamage + maxDamage);
                    SStrCopy(format, FrameScript_GetText(first ? "AMMO_DAMAGE_TEMPLATE" : "PLUS_AMMO_DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, static_cast<double>(ammo));
                } else if (minDamage == maxDamage) {
                    SStrCopy(format, FrameScript_GetText(first ? "SINGLE_DAMAGE_TEMPLATE" : "PLUS_SINGLE_DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, minDamage);
                } else {
                    SStrCopy(format, FrameScript_GetText(first ? "DAMAGE_TEMPLATE" : "PLUS_DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, minDamage, maxDamage);
                }
            }

            if (!first || stats->m_class != 2) {
                speed[0] = '\0';
            } else {
                SStrCopy(format, FrameScript_GetText("SPEED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                SStrPrintf(speed, sizeof(speed), "%s %.2f", format, static_cast<double>(static_cast<float>(static_cast<int32_t>(stats->m_delay)) * 0.001f));
            }

            tooltip->AddLine(text, speed, textColor, textColor, 0);
            first = 0;
            average = (stats->m_damageMin[i] + stats->m_damageMax[i]) * 0.5f + average;
        }
    }

    if (!first && stats->m_class == 2) {
        SStrCopy(format, FrameScript_GetText("DPS_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, static_cast<double>(average / (static_cast<float>(static_cast<int32_t>(stats->m_delay)) * 0.001f)));
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    return average;
}

// INLINED
static void AddItemArmorAndStatLines(CGTooltip* tooltip, const ItemStats_C* stats, const ScalingStatValuesRec* values, const ScalingStatDistributionRec* distribution) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char format[1024];
    char text[4096];
    char school[32];

    if (stats->m_scalingStatValue & 0xF801E0) {
        int32_t armor = GetScalingStatArmor(values, stats->m_scalingStatValue);
        SStrCopy(format, FrameScript_GetText("ARMOR_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, armor);
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    } else if (static_cast<int32_t>(stats->m_resistances[0]) > 0) {
        SStrCopy(format, FrameScript_GetText("ARMOR_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, stats->m_resistances[0]);

        if (stats->m_armorDamageModifier <= 0.0f) {
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        } else {
            tooltip->AddLine(text, nullptr, greenColor, greenColor, 0);
        }
    }

    if (static_cast<int32_t>(stats->m_block) > 0) {
        SStrCopy(format, FrameScript_GetText("SHIELD_BLOCK_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, stats->m_block);
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    if (!stats->m_scalingStatValue) {
        for (uint32_t i = 0; i < 11; i++) {
            uint32_t j = 0;

            for (; j < 10; j++) {
                if (stats->m_statValue[j] && stats->m_statType[j] != static_cast<uint32_t>(-1) && stats->m_statType[j] == s_primaryStatOrder[i]) {
                    break;
                }
            }

            if (j == 10) {
                continue;
            }

            format[0] = '\0';
            SStrCopy(format, FrameScript_GetText(s_itemModTokens[s_primaryStatOrder[i]], -1, GENDER_NOT_APPLICABLE), sizeof(format));

            if (format[0]) {
                int32_t value = stats->m_statValue[j];
                SStrPrintf(text, sizeof(text), format, 2 * (value <= 0) + '+', abs(value));
                tooltip->AddLine(text, nullptr, textColor, textColor, 1);
            }
        }
    } else if (distribution) {
        int32_t budget = values ? GetScalingStatBudget(values, stats->m_scalingStatValue) : 0;

        for (uint32_t i = 0; i < 11; i++) {
            int32_t value = 0;
            uint32_t j = 0;

            for (; j < 10; j++) {
                if (distribution->m_bonus[j] && (value = budget * distribution->m_bonus[j] / 10000) != 0 && distribution->m_statID[j] == s_primaryStatOrder[i]) {
                    break;
                }
            }

            if (j == 10) {
                continue;
            }

            format[0] = '\0';
            SStrCopy(format, FrameScript_GetText(s_itemModTokens[s_primaryStatOrder[i]], -1, GENDER_NOT_APPLICABLE), sizeof(format));

            if (format[0]) {
                SStrPrintf(text, sizeof(text), format, 2 * (value <= 0) + '+', abs(value));
                tooltip->AddLine(text, nullptr, textColor, textColor, 1);
            }
        }
    }

    int32_t resistance = stats->m_resistances[1];
    bool allEqual = true;

    for (uint32_t i = 2; i < 7; i++) {
        if (static_cast<int32_t>(stats->m_resistances[i]) != resistance) {
            allEqual = false;
            break;
        }
    }

    if (allEqual) {
        if (resistance) {
            SStrCopy(format, FrameScript_GetText("ITEM_RESIST_ALL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, 2 * (static_cast<int32_t>(stats->m_resistances[1]) <= 0) + '+', abs(static_cast<int32_t>(stats->m_resistances[1])));
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        }

        return;
    }

    for (int32_t i = 2; i <= 6; i++) {
        int32_t value = stats->m_resistances[i];

        if (value) {
            SStrPrintf(format, sizeof(format), "SPELL_SCHOOL%d_CAP", i);
            SStrCopy(school, FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE), sizeof(school));
            SStrCopy(format, FrameScript_GetText("ITEM_RESIST_SINGLE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, 2 * (value <= 0) + '+', abs(value), school);
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        }
    }
}

// INLINED
static int32_t AddItemSocketLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, CGPlayer_C* player, CGPlayer_C* ownerPlayer, WOWGUID owner, int32_t socketPreview, int32_t numSockets, int32_t* useSpell, int32_t* useCharges, const SpellItemEnchantmentRec** useEnchant) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    const CImVector& brightRedColor = *reinterpret_cast<const CImVector*>(&s_tooltipBrightRedColor);
    char format[1024];
    char text[4096];
    char conditions[1024];
    char charges[64];
    char name[32];
    int32_t needsUpdate = 0;
    int32_t allGemsMatch = 1;

    for (int32_t slot = 0; slot < 12; slot++) {
        int32_t enchantId;

        if (item) {
            enchantId = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) ? 0 : item->m_item->m_itemFieldEnchantment[slot].m_id;
        } else {
            enchantId = tooltip->m_itemLinkInfo.m_enchants[slot];
        }

        uint32_t socket = slot - 2;
        int32_t isSocket = socket <= 2 && static_cast<int32_t>(socket) < numSockets;

        if (socket <= 2 && socketPreview) {
            WOWGUID gemGuid = socket < 3 ? CGItemSocketInfo::m_gemItems[socket] : WOWGUID{ 0 };
            CGItem_C* gemItem = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(gemGuid, TYPE_ITEM, __FILE__, __LINE__));
            SpellItemEnchantmentRec* current = g_spellItemEnchantmentDB.GetRecord(abs(enchantId));

            if (gemItem) {
                WOWGUID requester = { 0 };
                ItemStats_C* gemStats = g_itemDBCache.GetRecord(gemItem->m_obj->m_entryID, requester, TooltipItemCallback, tooltip, true);

                if (gemStats) {
                    GemPropertiesRec* properties = g_gemPropertiesDB.GetRecord(gemStats->m_gemProperties);

                    if (properties) {
                        enchantId = properties->m_enchantID;
                    }

                    if (!CGItemSocketInfo::GemMatchesSocket(stats, gemStats, socket)) {
                        allGemsMatch = 0;
                    }
                }
            } else if (!current && stats->m_socketColor[socket]) {
                allGemsMatch = 0;
            }
        }

        if (!enchantId) {
            if (socket > 2 || !isSocket || socketPreview) {
                continue;
            }

            uint32_t color = stats->m_socketColor[socket];

            if (color) {
                allGemsMatch = 0;

                for (uint32_t i = 0; i < 4; i++) {
                    if (!((1 << i) & color)) {
                        continue;
                    }

                    SStrCopy(name, s_socketColorNames[i], STORM_MAX_STR);
                    SStrUpper(name);
                    SStrPrintf(format, sizeof(format), "%s%s", "EMPTY_SOCKET_", name);
                    SStrPrintf(text, sizeof(text), FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE));
                    tooltip->AddLine(text, nullptr, grayColor, grayColor, 0);
                    SStrPrintf(text, sizeof(text), "Interface\\ItemSocketingFrame\\UI-EmptySocket-%s.blp", s_socketColorNames[i]);
                    CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
                    uint32_t textureColor = 0xFFFFFFFF;
                    tooltip->AddTexture(text, coords, *reinterpret_cast<const CImVector*>(&textureColor));
                    break;
                }

                continue;
            }

            int32_t prismaticId = 0;

            if (item) {
                prismaticId = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) ? 0 : item->m_item->m_itemFieldEnchantment[6].m_id;
            } else if (tooltip->m_itemUseLinkInfo) {
                prismaticId = tooltip->m_itemLinkInfo.m_enchants[6];
            }

            SpellItemEnchantmentRec* prismatic = g_spellItemEnchantmentDB.GetRecord(prismaticId);

            if (prismaticId && prismatic && prismatic->m_requiredSkillID) {
                int32_t index = player->GetSkillIndex(prismatic->m_requiredSkillID);
                bool skillOk = index >= 0 && static_cast<int32_t>(player->GetSkillValue(index)) >= prismatic->m_requiredSkillRank;
                bool levelOk = prismatic->m_minLevel <= static_cast<int32_t>(player->m_unit->m_unitFieldLevel);
                const CImVector& color = skillOk && levelOk ? grayColor : redColor;
                tooltip->AddLine(FrameScript_GetText("EMPTY_SOCKET_NO_COLOR", -1, GENDER_NOT_APPLICABLE), nullptr, color, color, 0);
                CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
                uint32_t textureColor = 0xFFFFFFFF;
                tooltip->AddTexture("Interface\\ItemSocketingFrame\\UI-EmptySocket.blp", coords, *reinterpret_cast<const CImVector*>(&textureColor));

                if (!skillOk) {
                    SkillLineRec* skill = g_skillLineDB.GetRecord(prismatic->m_requiredSkillID);
                    SStrCopy(format, FrameScript_GetText(prismatic->m_requiredSkillRank ? "SOCKET_ITEM_MIN_SKILL" : "SOCKET_ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));

                    if (prismatic->m_requiredSkillRank) {
                        SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", prismatic->m_requiredSkillRank);
                    } else {
                        SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN");
                    }

                    tooltip->AddLine(text, nullptr, redColor, redColor, 0);
                }

                if (!levelOk) {
                    SStrCopy(format, FrameScript_GetText("SOCKET_ITEM_REQ_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                    SStrPrintf(text, sizeof(text), format, prismatic->m_minLevel);
                }

                continue;
            }

            tooltip->AddLine(FrameScript_GetText("EMPTY_SOCKET_NO_COLOR", -1, GENDER_NOT_APPLICABLE), nullptr, grayColor, grayColor, 0);
            CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
            uint32_t textureColor = 0xFFFFFFFF;
            tooltip->AddTexture("Interface\\ItemSocketingFrame\\UI-EmptySocket.blp", coords, *reinterpret_cast<const CImVector*>(&textureColor));
            continue;
        }

        if (slot == 6 || (stats->m_socketBonus && static_cast<int32_t>(stats->m_socketBonus) == enchantId)) {
            continue;
        }

        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(abs(enchantId));

        if (!enchant) {
            continue;
        }

        if (enchant->m_effect[0] == 7) {
            *useSpell = enchant->m_effectArg[0];

            if (item) {
                *useCharges = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) ? 0 : item->m_item->m_itemFieldEnchantment[slot].m_charges;
            } else {
                *useCharges = tooltip->m_itemLinkInfo.m_enchantCharges[slot];
            }

            *useEnchant = enchant;
            continue;
        }

        if (enchant->m_effect[0] == 3 && owner == ClntObjMgrGetActivePlayer()) {
            SpellRec spell;

            if (g_spellDB.GetRecord(enchant->m_effectArg[0], &spell) && ownerPlayer && !CGUnit_C::IsSpellAllowedInArena(&spell)) {
                continue;
            }
        }

        CImVector color = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);

        if (socket > 2 && slot < 7) {
            color = *reinterpret_cast<const CImVector*>(enchantId > 0 ? &s_tooltipGreenColor : &s_tooltipBrightRedColor);
        }

        int32_t value = item ? item->GetRandomPropertyValue(slot) : CGItem_C::GetRandomPropertyValue(slot, tooltip->m_itemLinkInfo.m_randomPropertiesId, static_cast<uint16_t>(tooltip->m_itemLinkInfo.m_propertySeed));
        SpellItemEnchantmentParseText(enchant, format, sizeof(format), value);
        uint32_t timeLeft;

        if (item && (timeLeft = item->GetEnchantmentTimeLeft(slot)) != 0) {
            CGTooltip::GetDurationString(text, sizeof(text), static_cast<int32_t>(timeLeft), "ITEM_ENCHANT_TIME_LEFT", format, 1, 0);
            needsUpdate = 1;
        } else {
            SStrCopy(text, format, sizeof(text));
        }

        if (!item || !(item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION)) {
            int32_t hasCharges = item ? item->m_item->m_itemFieldEnchantment[slot].m_charges : tooltip->m_itemLinkInfo.m_enchantCharges[slot];

            if (hasCharges) {
                int32_t count;

                if (item) {
                    count = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) ? 0 : item->m_item->m_itemFieldEnchantment[slot].m_charges;
                } else {
                    count = tooltip->m_itemLinkInfo.m_enchantCharges[slot];
                }

                if (item && (item->m_item->m_itemFieldEnchantment[slot].unk_000A & 0xF)) {
                    color = grayColor;
                } else {
                    SStrPrintf(charges, sizeof(charges), FrameScript_GetText("ITEM_SPELL_CHARGES", -1, GENDER_NOT_APPLICABLE), count);
                    SStrPrintf(format, sizeof(format), " (%s)", charges);
                    SStrPack(text, format, sizeof(text));
                }
            }
        }

        conditions[0] = '\0';
        bool skillOk = true;
        bool levelOk = true;

        if (enchant->m_requiredSkillID) {
            int32_t index = player->GetSkillIndex(enchant->m_requiredSkillID);

            if (index < 0 || static_cast<int32_t>(player->GetSkillValue(index)) < enchant->m_requiredSkillRank) {
                skillOk = false;
            }
        }

        if (enchant->m_minLevel > static_cast<int32_t>(player->m_unit->m_unitFieldLevel)) {
            levelOk = false;
        }

        int32_t socketEnchantId = 0;
        bool socketOk = true;
        bool prismaticEmpty;

        if (item) {
            prismaticEmpty = (item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION) || !item->m_item->m_itemFieldEnchantment[6].m_id;
        } else {
            prismaticEmpty = !tooltip->m_itemLinkInfo.m_enchants[6];
        }

        bool conditionLine = true;

        if (slot < 2 || slot > 4) {
            if (slot < 7) {
                const CImVector& lineColor = skillOk && levelOk ? color : redColor;
                tooltip->AddLine(text, nullptr, lineColor, lineColor, 0);
                conditionLine = false;
            } else if (slot > 11) {
                conditionLine = false;
            }
        } else if (!stats->m_socketColor[socket] || !prismaticEmpty) {
            if (item) {
                if (!(item->GetItemStaticFlags(0) & ITEM_FLAG_PETITION)) {
                    socketEnchantId = item->m_item->m_itemFieldEnchantment[slot].m_id;
                }
            } else if (tooltip->m_itemUseLinkInfo) {
                socketEnchantId = tooltip->m_itemLinkInfo.m_enchants[slot];
            }

            if (socketEnchantId) {
                SpellItemEnchantmentRec* socketEnchant = g_spellItemEnchantmentDB.GetRecord(socketEnchantId);

                if (socketEnchant && socketEnchant->m_requiredSkillID) {
                    int32_t index = player->GetSkillIndex(socketEnchant->m_requiredSkillID);

                    if (index < 0 || static_cast<int32_t>(player->GetSkillValue(index)) < socketEnchant->m_requiredSkillRank || enchant->m_minLevel > static_cast<int32_t>(player->m_unit->m_unitFieldLevel)) {
                        socketOk = false;
                    }
                }
            }
        }

        if (conditionLine) {
            CGPlayer_C* gemOwner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(owner, TYPE_PLAYER, __FILE__, __LINE__));
            int32_t conditionsMet;

            if (gemOwner) {
                gemOwner->CountEquippedGems();
                conditionsMet = GetEnchantmentConditionText(gemOwner, enchant, conditions, sizeof(conditions), 1);
            } else {
                conditionsMet = GetEnchantmentConditionText(player, enchant, conditions, sizeof(conditions), 1);
            }

            const char* colorString;

            if (socketOk && skillOk && levelOk) {
                colorString = conditionsMet ? s_colorStrings[0] : s_colorStrings[1];
            } else {
                colorString = s_colorStrings[2];
            }

            SStrPrintf(format, sizeof(format), "%s%s%s", colorString, text, "|r");
            SStrPack(format, conditions, sizeof(text));
            tooltip->AddLine(format, nullptr, titleColor, titleColor, 0);
        }

        if (enchant->m_srcItemID) {
            WOWGUID requester = { 0 };
            ItemStats_C* gem = g_itemDBCache.GetRecord(enchant->m_srcItemID, requester, TooltipItemCallback, tooltip, true);

            if (gem) {
                if (!CGItemSocketInfo::GemMatchesSocket(stats, gem, socket)) {
                    allGemsMatch = 0;
                }

                char texture[260];
                const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
                SStrPrintf(texture, sizeof(texture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(gem->m_displayId));
                CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
                uint32_t textureColor = 0xFFFFFFFF;
                tooltip->AddTexture(texture, coords, *reinterpret_cast<const CImVector*>(&textureColor));
            }
        }

        if (!skillOk && enchant->m_requiredSkillID) {
            SkillLineRec* skill = g_skillLineDB.GetRecord(enchant->m_requiredSkillID);
            SStrCopy(format, FrameScript_GetText(enchant->m_requiredSkillRank ? "ENCHANT_ITEM_MIN_SKILL" : "ENCHANT_ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));

            if (enchant->m_requiredSkillRank) {
                SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", enchant->m_requiredSkillRank);
            } else {
                SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN");
            }

            tooltip->AddLine(text, nullptr, redColor, redColor, 0);
        }

        if (!levelOk) {
            SStrCopy(format, FrameScript_GetText("ENCHANT_ITEM_REQ_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, enchant->m_minLevel);
            tooltip->AddLine(text, nullptr, redColor, redColor, 0);
        }

        if (!socketOk && socketEnchantId) {
            SpellItemEnchantmentRec* socketEnchant = g_spellItemEnchantmentDB.GetRecord(socketEnchantId);

            if (socketEnchant && socketEnchant->m_requiredSkillID) {
                SkillLineRec* skill = g_skillLineDB.GetRecord(socketEnchant->m_requiredSkillID);
                SStrCopy(format, FrameScript_GetText(socketEnchant->m_requiredSkillRank ? "SOCKET_ITEM_MIN_SKILL" : "SOCKET_ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));

                if (socketEnchant->m_requiredSkillRank) {
                    SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", socketEnchant->m_requiredSkillRank);
                } else {
                    SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN");
                }

                tooltip->AddLine(text, nullptr, redColor, redColor, 0);
            }
        }
    }

    if (tooltip->m_itemUseLinkInfo && tooltip->m_itemLinkInfo.m_proposedEnchant) {
        SpellRec spell;

        if (g_spellDB.GetRecord(tooltip->m_itemLinkInfo.m_proposedEnchant, &spell)) {
            SStrCopy(format, FrameScript_GetText("ITEM_PROPOSED_ENCHANT", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, spell.m_name);
            tooltip->AddLine(text, nullptr, greenColor, greenColor, 0);
            tooltip->AddLine(FrameScript_GetText("ITEM_ENCHANT_DISCLAIMER", -1, GENDER_NOT_APPLICABLE), nullptr, brightRedColor, brightRedColor, 0);
        }
    }

    if (stats->m_socketBonus) {
        SpellItemEnchantmentRec* bonus = g_spellItemEnchantmentDB.GetRecord(abs(static_cast<int32_t>(stats->m_socketBonus)));

        if (bonus) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SOCKET_BONUS", -1, GENDER_NOT_APPLICABLE), bonus->m_name);
            CImVector color = allGemsMatch ? greenColor : grayColor;
            tooltip->AddLine(text, nullptr, color, color, 0);
        }
    }

    return needsUpdate;
}

// INLINED
static void AddItemStaticSocketLines(CGTooltip* tooltip, const ItemStats_C* stats) {
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char name[32];
    char format[1024];
    char text[4096];

    for (uint32_t slot = 0; slot < 12; slot++) {
        uint32_t socket = slot - 2;

        if (socket > 2 || !stats->m_socketColor[socket]) {
            continue;
        }

        uint32_t color = stats->m_socketColor[socket];

        for (uint32_t i = 0; i < 4; i++) {
            if (!((1 << i) & color)) {
                continue;
            }

            SStrCopy(name, s_socketColorNames[i], STORM_MAX_STR);
            SStrUpper(name);
            SStrPrintf(format, sizeof(format), "%s%s", "EMPTY_SOCKET_", name);
            SStrPrintf(text, sizeof(text), FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE));
            tooltip->AddLine(text, nullptr, grayColor, grayColor, 0);
            SStrPrintf(text, sizeof(text), "Interface\\ItemSocketingFrame\\UI-EmptySocket-%s.blp", s_socketColorNames[i]);
            CAaRect coords = { 0.0f, 0.0f, 1.0f, 1.0f };
            uint32_t textureColor = 0xFFFFFFFF;
            tooltip->AddTexture(text, coords, *reinterpret_cast<const CImVector*>(&textureColor));
            break;
        }
    }

    if (stats->m_socketBonus) {
        SpellItemEnchantmentRec* bonus = g_spellItemEnchantmentDB.GetRecord(abs(static_cast<int32_t>(stats->m_socketBonus)));

        if (bonus) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SOCKET_BONUS", -1, GENDER_NOT_APPLICABLE), bonus->m_name);
            CImVector color = grayColor;
            tooltip->AddLine(text, nullptr, color, color, 0);
        }
    }

    if (stats->m_randomProperty || stats->m_randomSuffix) {
        tooltip->AddLine(FrameScript_GetText("ITEM_RANDOM_ENCHANT", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }
}

// INLINED
static void AddItemGemLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player, int32_t destroyGem) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    char text[4096];
    GemPropertiesRec* properties = g_gemPropertiesDB.GetRecord(stats->m_gemProperties);

    if (!properties) {
        return;
    }

    SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(abs(properties->m_enchantID));

    if (!enchant) {
        return;
    }

    int32_t skillOk = 1;

    if (stats->m_requiredSkill) {
        int32_t index = player->GetSkillIndex(stats->m_requiredSkill);
        skillOk = index >= 0 && player->GetSkillValue(index) >= stats->m_requiredSkillRank;
    }

    const CImVector* color;

    if (destroyGem) {
        color = &grayColor;
    } else {
        color = skillOk ? &textColor : &redColor;
    }

    tooltip->AddLine(enchant->m_name, nullptr, *color, *color, 0);
    GetEnchantmentConditionText(player, enchant, text, sizeof(text), 0);

    if (text[0]) {
        tooltip->AddLine(text, nullptr, textColor, textColor, 1);
    }
}

// INLINED
static int32_t AddItemDurabilityLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, int32_t wrapped, int32_t socketPreview) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    char format[1024];
    char text[4096];
    int32_t needsUpdate = 0;
    ItemLinkInfo& link = tooltip->m_itemLinkInfo;

    if (!wrapped && !socketPreview) {
        int32_t useLinkInfo = tooltip->m_itemUseLinkInfo;

        if (useLinkInfo && link.m_maxDurability == static_cast<uint32_t>(-1)) {
            link.m_maxDurability = stats->m_maxDurability;
        }

        if ((item && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) || (useLinkInfo && link.m_maxDurability)) {
            SStrCopy(format, FrameScript_GetText("DURABILITY_TEMPLATE", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            uint32_t durability;
            uint32_t maxDurability;

            if (item && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
                durability = (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) ? 0 : item->m_item->m_itemFieldDurability;
            } else if (tooltip->m_itemUseLinkInfo && link.m_maxDurability) {
                durability = link.m_durability;
            } else {
                durability = stats->m_maxDurability;
            }

            if (item && !(item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) && item->m_item->m_itemFieldMaxdurability) {
                maxDurability = (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED) ? 0 : item->m_item->m_itemFieldMaxdurability;
            } else if (!tooltip->m_itemUseLinkInfo || (maxDurability = link.m_maxDurability) == 0) {
                maxDurability = stats->m_maxDurability;
            }

            if (durability >= maxDurability) {
                durability = maxDurability;
            }

            SStrPrintf(text, sizeof(text), format, durability, maxDurability);
            const CImVector& color = durability ? textColor : redColor;
            tooltip->AddLine(text, nullptr, color, color, 0);
        }
    }

    int32_t timeLeft = stats->m_duration;

    if (timeLeft > 0) {
        if (item) {
            timeLeft = item->GetExpirationTimeLeft();
        }

        if (timeLeft >= 0 && !socketPreview) {
            CGTooltip::GetDurationString(text, sizeof(text), timeLeft, "ITEM_DURATION", nullptr, 1, 1);
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
            needsUpdate = 1;
        }
    }

    if (stats->m_holidayId) {
        const char* holiday = GetHolidayName(stats->m_holidayId);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), holiday);
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
    }

    return needsUpdate;
}

// INLINED
static void AddItemRaceClassLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    char format[1024];
    char text[4096];
    char line[512];
    char list[512];
    int32_t allowed = 1;
    text[0] = '\0';

    if (((stats->m_allowableRace - 1) & stats->m_allowableRace) || ((stats->m_allowableClass - 1) & stats->m_allowableClass)) {
        int32_t allRaces = 1;
        int32_t allClasses = 1;

        for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
            ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

            if (race && !(race->m_flags & 1) && !((1 << (race->m_id - 1)) & stats->m_allowableRace)) {
                allRaces = 0;
                break;
            }
        }

        for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
            ChrClassesRec* playerClass = g_chrClassesDB.GetRecordByIndex(i);

            if (playerClass && !((1 << (playerClass->m_id - 1)) & stats->m_allowableClass)) {
                allClasses = 0;
                break;
            }
        }

        if (!allRaces) {
            list[0] = '\0';
            int32_t first = 1;
            allowed = 0;

            for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
                ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

                if (!(race->m_flags & 1) && race->m_name && ((1 << (race->m_id - 1)) & stats->m_allowableRace)) {
                    if (!first) {
                        SStrPack(list, ", ", sizeof(list));
                    }

                    SStrPack(list, race->m_name, sizeof(list));
                    first = 0;

                    if (player->m_unit->m_unitFieldBytes0[0] == race->m_id) {
                        allowed = 1;
                    }
                }
            }

            if (list[0]) {
                SStrCopy(format, FrameScript_GetText("ITEM_RACES_ALLOWED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                SStrPrintf(line, sizeof(line), format, list);
                const CImVector& color = allowed ? textColor : redColor;
                tooltip->AddLine(line, nullptr, color, color, 0);
            }
        }

        if (!allClasses) {
            list[0] = '\0';
            int32_t first = 1;
            allowed = 0;

            for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
                ChrClassesRec* playerClass = g_chrClassesDB.GetRecordByIndex(i);

                if (playerClass && playerClass->m_name && ((1 << (playerClass->m_id - 1)) & stats->m_allowableClass)) {
                    if (!first) {
                        SStrPack(list, ", ", sizeof(list));
                    }

                    SStrPack(list, playerClass->m_name, sizeof(list));
                    first = 0;

                    if (player->m_unit->m_unitFieldBytes0[1] == playerClass->m_id) {
                        allowed = 1;
                    }
                }
            }

            if (list[0]) {
                SStrCopy(format, FrameScript_GetText("ITEM_CLASSES_ALLOWED", -1, GENDER_NOT_APPLICABLE), sizeof(format));
                SStrPrintf(line, sizeof(line), format, list);
                const CImVector& color = allowed ? textColor : redColor;
                tooltip->AddLine(line, nullptr, color, color, 0);
            }
        }

        return;
    }

    for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
        ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

        if (!(race->m_flags & 1) && race->m_name && ((1 << (race->m_id - 1)) & stats->m_allowableRace)) {
            SStrPrintf(text, sizeof(text), "%s ", race->m_name);

            if (player->m_unit->m_unitFieldBytes0[0] != race->m_id) {
                allowed = 0;
            }

            break;
        }
    }

    for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
        ChrClassesRec* playerClass = g_chrClassesDB.GetRecordByIndex(i);

        if (playerClass && playerClass->m_name && ((1 << (playerClass->m_id - 1)) & stats->m_allowableClass)) {
            SStrPack(text, playerClass->m_name, sizeof(text));

            if (player->m_unit->m_unitFieldBytes0[1] != playerClass->m_id) {
                allowed = 0;
            }

            break;
        }
    }

    if (text[0]) {
        char classOnly[128];
        SStrCopy(format, FrameScript_GetText("RACE_CLASS_ONLY", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(classOnly, sizeof(classOnly), format, text);
        const CImVector& color = allowed ? textColor : redColor;
        tooltip->AddLine(classOnly, nullptr, color, color, 0);
    }
}

// INLINED
static void AddItemLevelLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player, const ScalingStatDistributionRec* distribution, int32_t displayedLevel) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    char format[1024];
    char text[4096];
    int32_t requiredLevel = stats->m_requiredLevel;

    if (distribution) {
        SStrCopy(format, FrameScript_GetText("ITEM_LEVEL_RANGE_CURRENT", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, requiredLevel <= 0 ? 1 : requiredLevel, distribution->m_maxlevel, displayedLevel);
        int32_t level = player->m_unit->m_unitFieldLevel;
        const CImVector& color = level < requiredLevel || level > distribution->m_maxlevel ? redColor : textColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    } else if (requiredLevel > 1) {
        SStrCopy(format, FrameScript_GetText("ITEM_MIN_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SStrPrintf(text, sizeof(text), format, requiredLevel);
        const CImVector& color = requiredLevel <= static_cast<int32_t>(player->m_unit->m_unitFieldLevel) ? textColor : redColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }

    CVar* showItemLevel = CVar::Lookup("showItemLevel");

    if (showItemLevel && showItemLevel->m_intValue) {
        uint32_t itemClass = stats->m_class;

        if (itemClass == 2 || itemClass == 4 || itemClass == 5 || itemClass == 6) {
            SStrCopy(format, FrameScript_GetText("ITEM_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(text, sizeof(text), format, stats->m_itemLevel);
            tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        }
    }
}

// INLINED
static void AddItemRequirementLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player, int32_t itemId, const WOWGUID* requester) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    char format[1024];
    char text[4096];

    if (stats->m_requiredSkill) {
        SkillLineRec* skill = g_skillLineDB.GetRecord(stats->m_requiredSkill);
        int32_t index = player->GetSkillIndex(stats->m_requiredSkill);
        bool skillOk = index >= 0 && player->GetSkillValue(index) >= stats->m_requiredSkillRank;

        if (stats->m_flags[0] & 0x40000) {
            if (index >= 0 && static_cast<int32_t>(player->GetSkillValue(index)) > 0) {
                const CImVector& color = skillOk ? textColor : redColor;
                tooltip->AddLine(FrameScript_GetText("ITEM_PROSPECTABLE", -1, GENDER_NOT_APPLICABLE), nullptr, color, color, 0);
            }
        } else if (stats->m_flags[0] & 0x20000000) {
            if (index >= 0 && static_cast<int32_t>(player->GetSkillValue(index)) > 0) {
                const CImVector& color = skillOk ? textColor : redColor;
                tooltip->AddLine(FrameScript_GetText("ITEM_MILLABLE", -1, GENDER_NOT_APPLICABLE), nullptr, color, color, 0);
            }
        } else {
            SStrCopy(format, FrameScript_GetText(stats->m_requiredSkillRank ? "ITEM_MIN_SKILL" : "ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));

            if (stats->m_requiredSkillRank) {
                SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", stats->m_requiredSkillRank);
            } else {
                SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN");
            }

            const CImVector& color = skillOk ? textColor : redColor;
            tooltip->AddLine(text, nullptr, color, color, 0);
        }
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(stats->m_spellId[0], &spell) && spell.m_effect[0] == SPELL_EFFECT_LEARN_SPELL) {
        int32_t taughtSpell = spell.m_effectTriggerSpell[0];

        if (!taughtSpell) {
            for (uint32_t i = 0; i < 5; i++) {
                if (stats->m_spellTrigger[i] == 6) {
                    taughtSpell = stats->m_spellId[i];
                    break;
                }
            }
        }

        CGUnit_C* learner;

        if (spell.m_implicitTargetA[0] == 5 || spell.m_implicitTargetB[0] == 5) {
            CGUnitData* unit = player->m_unit;
            learner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(unit->m_unitFieldCharm ? unit->m_unitFieldCharm : unit->m_unitFieldSummon, TYPE_UNIT, __FILE__, __LINE__));
        } else {
            learner = player;
        }

        if (learner && (learner->IsSpellKnown(taughtSpell) || learner->IsSpellSuperceded(taughtSpell))) {
            tooltip->AddLine(FrameScript_GetText("ITEM_SPELL_KNOWN", -1, GENDER_NOT_APPLICABLE), nullptr, redColor, redColor, 0);
        }
    }

    if (stats->m_requiredSpell && g_spellDB.GetRecord(stats->m_requiredSpell, &spell)) {
        bool known = player->IsSpellKnown(stats->m_requiredSpell) || player->IsSpellSuperceded(stats->m_requiredSpell);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), spell.m_name);
        const CImVector& color = known ? textColor : redColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }

    if (stats->m_requiredHonorRank) {
        bool ranked = player->m_player->m_playerFieldBytes[3] >= stats->m_requiredHonorRank;
        const char* rank = player->GetPVPRankString(stats->m_requiredHonorRank);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), rank);
        const CImVector& color = ranked ? textColor : redColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }

    if (*requester == CGMerchantInfo::m_merchant) {
        MerchantItem* merchantItem = CGMerchantInfo::FindItemById(itemId);

        if (merchantItem) {
            ItemExtendedCostRec* cost = g_itemExtendedCostDB.GetRecord(merchantItem->m_extendedCostID);

            if (cost) {
                if (cost->m_requiredArenaRating > 0) {
                    int32_t bracket = cost->m_arenaBracket;

                    if (bracket > 3) {
                        bracket = 3;
                    } else if (bracket < 0) {
                        bracket = 0;
                    }

                    bool rated = CGArenaTeamInfo::CheckPersonalAndTeamRating(cost->m_requiredArenaRating, bracket);

                    if (bracket == 2) {
                        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_ARENA_RATING_5V5", -1, GENDER_NOT_APPLICABLE), cost->m_requiredArenaRating);
                    } else if (bracket == 1) {
                        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_ARENA_RATING_3V3", -1, GENDER_NOT_APPLICABLE), cost->m_requiredArenaRating);
                    } else {
                        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_ARENA_RATING", -1, GENDER_NOT_APPLICABLE), cost->m_requiredArenaRating);
                    }

                    const CImVector& color = rated ? textColor : redColor;
                    tooltip->AddLine(text, nullptr, color, color, 0);
                }

                if (cost->m_itemPurchaseGroup) {
                    ItemPurchaseGroupRec* group = g_itemPurchaseGroupDB.GetRecord(cost->m_itemPurchaseGroup);

                    if (group) {
                        bool owned = false;

                        for (uint32_t i = 0; i < 8; i++) {
                            if (group->m_itemID[i] && player->m_bag.FindItemOfType(group->m_itemID[i], 8)) {
                                owned = true;
                                break;
                            }
                        }

                        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_PURCHASE_GROUP", -1, GENDER_NOT_APPLICABLE), group->m_name);
                        const CImVector& color = owned ? textColor : redColor;
                        tooltip->AddLine(text, nullptr, color, color, 0);
                    }
                }
            }
        }
    }

    if (stats->m_requiredCityRank) {
        bool hasMedal = ((1 << (stats->m_requiredCityRank - 1)) & player->m_player->m_playerFieldPvpMedals) != 0;
        SStrPrintf(format, sizeof(format), "PVP_MEDAL%d", stats->m_requiredCityRank);
        const char* medal = FrameScript_GetText(format, -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), medal);
        const CImVector& color = hasMedal ? textColor : redColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }

    if (stats->m_requiredReputationFaction) {
        int32_t rank = stats->m_requiredReputationRank;
        bool reached = GetRepListRepValue(stats->m_requiredReputationFaction) >= CGReputationInfo::m_reactionThresholds[rank];
        FactionRec* faction = g_factionDB.GetRecord(stats->m_requiredReputationFaction);
        SStrPrintf(format, sizeof(format), "FACTION_STANDING_LABEL%d", rank + 1);
        const char* name = faction ? faction->m_name : "UNKNOWN";
        const char* standing = player->GetFrameScriptText(format, -1);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_REQ_REPUTATION", -1, GENDER_NOT_APPLICABLE), name, standing);
        const CImVector& color = reached ? textColor : redColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }
}

// INLINED
static int32_t CollectItemSetData(CGTooltip* tooltip, const ItemSetRec* set, CGPlayer_C* player, CGPlayer_C* ownerPlayer, int32_t* equipped, int32_t* numItems, int32_t* numEquipped) {
    ItemSetTooltipData data;
    data.m_tooltip = tooltip;
    data.m_set = set;
    data.m_equipped = equipped;
    memset(data.m_slotUsed, 0, sizeof(data.m_slotUsed));

    for (uint32_t i = 0; i < 17; i++) {
        if (!set->m_itemID[i]) {
            data.m_items[i] = nullptr;
            continue;
        }

        WOWGUID requester = { 0 };
        ItemNameCache_C* item = g_itemNameDBCache.GetRecord(set->m_itemID[i], requester, TooltipItemCallback, tooltip, true);
        data.m_items[i] = item;

        if (!item) {
            continue;
        }

        uint32_t j = 0;

        for (; j < i; j++) {
            ItemNameCache_C* previous = data.m_items[j];

            if (!previous) {
                break;
            }

            uint32_t type = item->m_inventoryType;
            uint32_t previousType = previous->m_inventoryType;

            if (type == previousType && type != 11 && type != 12 && type != 13) {
                break;
            }

            if (type == 5 ? previousType == 20 : type == 20 && previousType == 5) {
                break;
            }
        }

        if (j == i) {
            (*numItems)++;
        }
    }

    for (int32_t pass = 0; pass < 2; pass++) {
        if (ownerPlayer) {
            for (int32_t slot = 0; slot < 19; slot++) {
                const VisibleItem_C* visible = ownerPlayer->GetVisibleItem(slot);

                if (visible) {
                    int32_t itemId = abs(visible->m_itemId);

                    if (itemId && ResolveItemSetItem(&data, pass, itemId, slot)) {
                        (*numEquipped)++;
                    }
                }
            }
        } else {
            for (int32_t slot = 0; slot < 19; slot++) {
                const CGItem_C* item = player->m_bag.GetItemPointer(slot);

                if (item && ResolveItemSetItem(&data, pass, item->m_obj->m_entryID, slot)) {
                    (*numEquipped)++;
                }
            }
        }
    }

    return player->MeetsItemSetSkill(set);
}

// INLINED
static void AddItemSpellStatLines(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player, const ScalingStatValuesRec* values, const ScalingStatDistributionRec* distribution, float averageDamage) {
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char format[1024];
    char bonus[4096];
    char text[4096];

    if (player->m_unit->m_unitFieldBytes0[1] == 11) {
        int32_t attackPower = GetFeralAttackPower(stats, averageDamage);

        if (attackPower > 0) {
            SStrCopy(format, FrameScript_GetText("FERAL_DRUID_ITEM_AP", -1, GENDER_NOT_APPLICABLE), sizeof(format));
            SStrPrintf(bonus, 1024, format, attackPower);
            tooltip->AddLine(bonus, nullptr, greenColor, greenColor, 1);
        }
    }

    if (!stats->m_scalingStatValue) {
        for (uint32_t i = 0; i < 10; i++) {
            int32_t value = stats->m_statValue[i];

            if (!value || stats->m_statType[i] == static_cast<uint32_t>(-1)) {
                continue;
            }

            uint32_t j = 0;

            for (; j < sizeof(s_spellStatOrder) / sizeof(s_spellStatOrder[0]); j++) {
                if (static_cast<int32_t>(stats->m_statType[i]) == s_spellStatOrder[j]) {
                    break;
                }
            }

            if (j == sizeof(s_spellStatOrder) / sizeof(s_spellStatOrder[0])) {
                continue;
            }

            format[0] = '\0';
            SStrCopy(format, FrameScript_GetText(s_itemModTokens[s_spellStatOrder[j]], -1, GENDER_NOT_APPLICABLE), sizeof(format));

            if (format[0]) {
                if (SStrStr(format, "%c")) {
                    SStrPrintf(bonus, sizeof(bonus), format, 2 * (value <= 0) + '+', abs(value));
                } else {
                    SStrPrintf(bonus, sizeof(bonus), format, value);
                }

                SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), bonus);
                tooltip->AddLine(text, nullptr, greenColor, greenColor, 0);
            }
        }

        return;
    }

    if (distribution) {
        int32_t budget = GetScalingStatBudget(values, stats->m_scalingStatValue);

        if (budget) {
            for (uint32_t i = 0; i < 10; i++) {
                int32_t value;

                if (!distribution->m_bonus[i] || (value = distribution->m_bonus[i] * budget / 10000) == 0) {
                    continue;
                }

                uint32_t j = 0;

                for (; j < sizeof(s_spellStatOrder) / sizeof(s_spellStatOrder[0]); j++) {
                    if (distribution->m_statID[i] == s_spellStatOrder[j]) {
                        break;
                    }
                }

                if (j == sizeof(s_spellStatOrder) / sizeof(s_spellStatOrder[0])) {
                    continue;
                }

                format[0] = '\0';
                SStrCopy(format, FrameScript_GetText(s_itemModTokens[s_spellStatOrder[j]], -1, GENDER_NOT_APPLICABLE), sizeof(format));

                if (format[0]) {
                    if (SStrStr(format, "%c")) {
                        SStrPrintf(bonus, sizeof(bonus), format, 2 * (value <= 0) + '+', abs(value));
                    } else {
                        SStrPrintf(bonus, sizeof(bonus), format, value);
                    }

                    SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), bonus);
                    tooltip->AddLine(text, nullptr, greenColor, greenColor, 1);
                }
            }
        }
    }

    if ((stats->m_scalingStatValue & 0x8000) && values && values->m_spellPower) {
        SStrCopy(format, FrameScript_GetText(s_itemModTokens[45], -1, GENDER_NOT_APPLICABLE), sizeof(format));

        if (format[0]) {
            SStrPrintf(bonus, sizeof(bonus), format, values->m_spellPower);
            SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), bonus);
            tooltip->AddLine(text, nullptr, greenColor, greenColor, 1);
        }
    }
}

// INLINED
static int32_t AddItemSpellTriggerLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, int32_t hideCharges, FILE* stream) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char format[1024];
    char cooldown[1024];
    char text[4096];
    char line[1024];
    char buffer[4096];
    int32_t isRecipe = 0;

    for (int32_t i = 0; i < 5; i++) {
        if (stream) {
            sprintf(buffer, "\t\t\t<spelldesc_%d>", i);
            fputs(buffer, stream);
        }

        if (static_cast<int32_t>(stats->m_spellId[i]) <= 0) {
            if (stream) {
                sprintf(buffer, "</spelldesc_%d>\n", i);
                fputs(buffer, stream);
            }

            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
            if (stream) {
                sprintf(buffer, "</spelldesc_%d>\n", i);
                fputs(buffer, stream);
            }

            continue;
        }

        SpellParserParseText(&spell, format, sizeof(format), 0, 0, 0, 0, 1, 0);

        if (!format[0] && stats->m_spellTrigger[i] != 6) {
            if (stream) {
                sprintf(buffer, "</spelldesc_%d>\n", i);
                fputs(buffer, stream);
            }

            continue;
        }

        switch (stats->m_spellTrigger[i]) {
            case 0: {
                int32_t spellCooldown = stats->m_spellCooldown[i];

                if (spellCooldown < 0) {
                    if (static_cast<int32_t>(stats->m_spellCategoryCooldown[i]) >= 0) {
                        spellCooldown = 0;
                    } else {
                        int32_t recoveryTime = spell.m_recoveryTime;
                        int32_t categoryRecoveryTime = spell.m_categoryRecoveryTime;
                        Spell_C_ModifySpellValue(&spell, recoveryTime, 11);

                        if (spell.m_attributesExF >= 0) {
                            Spell_C_ModifySpellValue(&spell, categoryRecoveryTime, 11);
                        }

                        spellCooldown = recoveryTime <= categoryRecoveryTime ? categoryRecoveryTime : recoveryTime;
                    }
                }

                if (spellCooldown <= 0) {
                    SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), format);
                } else {
                    CGTooltip::GetItemDurationString(cooldown, sizeof(cooldown), spellCooldown, "ITEM_COOLDOWN_TOTAL");
                    SStrPrintf(text, sizeof(text), "%s %s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), format, cooldown);
                }

                break;
            }

            case 1:
                SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONEQUIP", -1, GENDER_NOT_APPLICABLE), format);
                break;

            case 2:
                SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONPROC", -1, GENDER_NOT_APPLICABLE), format);
                break;

            case 6:
                SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), stats->m_description);
                isRecipe = 1;
                break;

            default:
                SStrCopy(text, format, sizeof(text));
                break;
        }

        SpellRec created;
        int32_t hasCreated = 0;
        int32_t createsItem = 0;

        if (stats->m_spellTrigger[i] == 6) {
            hasCreated = 1;
            created = spell;
        } else if (spell.m_effect[0] == SPELL_EFFECT_LEARN_SPELL) {
            hasCreated = g_spellDB.GetRecord(spell.m_effectTriggerSpell[0], &created);
        }

        if (hasCreated && (created.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || created.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || created.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && created.m_effectItemType[0]) {
            createsItem = 1;
        }

        if (stream) {
            CGTooltip::FormatStringForXML(format, buffer);
            fputs(buffer, stream);
        }

        const CImVector& color = createsItem ? textColor : greenColor;
        tooltip->AddLine(text, nullptr, color, color, 1);

        if (!hideCharges) {
            int32_t charges;

            if (tooltip->m_itemUseLinkInfo) {
                charges = tooltip->m_itemLinkInfo.m_spellCharges;
            } else {
                charges = item ? item->m_item->m_itemFieldSpellCharges[i] : stats->m_spellCharges[i];
            }

            if (stats->m_spellCharges[i] && stats->m_spellCharges[i] != -1) {
                int32_t count = abs(charges);

                if (count) {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SPELL_CHARGES", -1, GENDER_NOT_APPLICABLE), count);
                } else {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SPELL_CHARGES_NONE", -1, GENDER_NOT_APPLICABLE));
                }

                if (stream) {
                    CGTooltip::FormatStringForXML(text, buffer);
                    fputs(buffer, stream);
                }

                tooltip->AddLine(text, nullptr, textColor, textColor, 0);
            }
        }

        if (hasCreated && createsItem) {
            WOWGUID requester = { 0 };
            WOWGUID itemGuid = { 0 };
            tooltip->SetItem(created.m_effectItemType[0], &requester, &itemGuid, 0, 0, 0, 1, WOWGUID{ 0 }, 0, nullptr, hideCharges, nullptr, 0, 0, 1);
            int32_t first = 1;
            int32_t missing = 0;
            text[0] = '\0';

            for (uint32_t j = 0; j < 8; j++) {
                int32_t reagent = created.m_reagent[j];

                if (reagent <= 0) {
                    continue;
                }

                WOWGUID query;
                query.m_guid = static_cast<uint64_t>(static_cast<uint32_t>(created.m_id)) | 0x1FE0000000000000ULL;
                ItemStats_C* reagentStats = g_itemDBCache.GetRecord(reagent, query, TooltipItemCallback, tooltip, true);

                if (!reagentStats) {
                    missing = 1;
                    continue;
                }

                if (first) {
                    first = 0;
                } else {
                    SStrPack(text, ", ", sizeof(text));
                }

                if (created.m_reagentCount[j] <= 1) {
                    SStrCopy(format, reagentStats->m_name[0], sizeof(format));
                } else {
                    SStrPrintf(format, sizeof(format), "%s (%d)", reagentStats->m_name[0], created.m_reagentCount[j]);
                }

                SStrPack(text, format, sizeof(text));
            }

            if (!first && !missing) {
                SStrPrintf(format, sizeof(format), FrameScript_GetText("ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), text);
                SStrPrintf(line, sizeof(line), "\n%s", format);

                if (stream) {
                    CGTooltip::FormatStringForXML(line, buffer);
                    fputs(buffer, stream);
                }

                tooltip->AddLine(line, nullptr, textColor, textColor, 1);
            }
        }

        if (stream) {
            sprintf(buffer, "</spelldesc_%d>\n", i);
            fputs(buffer, stream);
        }
    }

    return isRecipe;
}

// INLINED
static void AddItemSetLines(CGTooltip* tooltip, const ItemSetRec* set, CGPlayer_C* player, const int32_t* equipped, int32_t numItems, int32_t numEquipped, int32_t skillOk, FILE* stream) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    const CImVector& itemSetColor = *reinterpret_cast<const CImVector*>(&s_tooltipItemSetColor);
    char format[1024];
    char text[4096];
    char buffer[4096];
    tooltip->AddLine(" ", nullptr, titleColor, titleColor, 1);
    SStrCopy(format, FrameScript_GetText("ITEM_SET_NAME", -1, GENDER_NOT_APPLICABLE), sizeof(format));
    SStrPrintf(text, sizeof(text), format, set->m_name, numEquipped, numItems);
    tooltip->AddLine(text, nullptr, titleColor, titleColor, 0);

    if (set->m_requiredSkill) {
        SStrCopy(format, FrameScript_GetText(set->m_requiredSkillRank ? "ITEM_MIN_SKILL" : "ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(format));
        SkillLineRec* skill = g_skillLineDB.GetRecord(set->m_requiredSkill);

        if (set->m_requiredSkillRank) {
            SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", set->m_requiredSkillRank);
        } else {
            SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN");
        }

        int32_t index = player->GetSkillIndex(set->m_requiredSkill);
        const CImVector& color = index < 0 || static_cast<int32_t>(player->GetSkillValue(index)) < set->m_requiredSkillRank ? redColor : textColor;
        tooltip->AddLine(text, nullptr, color, color, 0);
    }

    for (uint32_t i = 0; i < 17; i++) {
        if (!set->m_itemID[i]) {
            continue;
        }

        const char* name;

        if (equipped[i]) {
            WOWGUID requester = { 0 };
            ItemStats_C* stats = g_itemDBCache.GetRecord(equipped[i], requester, nullptr, nullptr, false);

            if (!stats) {
                continue;
            }

            name = stats->m_name[0];
        } else {
            WOWGUID requester = { 0 };
            ItemNameCache_C* item = g_itemNameDBCache.GetRecord(set->m_itemID[i], requester, nullptr, nullptr, false);

            if (!item) {
                continue;
            }

            name = item->m_name;
        }

        if (name) {
            SStrPrintf(text, sizeof(text), "  %s", name);
            const CImVector& color = equipped[i] ? itemSetColor : grayColor;
            tooltip->AddLine(text, nullptr, color, color, 0);
        }
    }

    tooltip->AddLine(" ", nullptr, titleColor, titleColor, 1);
    uint8_t order[8];

    for (uint32_t i = 0; i < 8; i++) {
        order[i] = i;
    }

    s_sortItemSet = set;
    qsort(order, 8, sizeof(order[0]), SortItemSetBonuses);

    for (int32_t i = 0; i < 8; i++) {
        uint8_t index = order[i];

        if (stream) {
            sprintf(buffer, "\t\t\t<setspelldesc_%d>", i);
            fputs(buffer, stream);
        }

        if (set->m_setSpellID[index]) {
            SpellRec spell;

            if (g_spellDB.GetRecord(set->m_setSpellID[index], &spell)) {
                SpellParserParseText(&spell, format, sizeof(format), 0, 0, 0, 0, 1, 0);

                if (skillOk && numEquipped >= set->m_setThreshold[index]) {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SET_BONUS", -1, GENDER_NOT_APPLICABLE), format);
                    tooltip->AddLine(text, nullptr, greenColor, greenColor, 1);
                } else {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SET_BONUS_GRAY", -1, GENDER_NOT_APPLICABLE), set->m_setThreshold[index], format);
                    tooltip->AddLine(text, nullptr, grayColor, grayColor, 1);
                }

                if (stream) {
                    CGTooltip::FormatStringForXML(format, buffer);
                    fputs(buffer, stream);
                }
            }
        }

        if (stream) {
            sprintf(buffer, "</setspelldesc_%d>\n", i);
            fputs(buffer, stream);
        }
    }
}

// INLINED
static void AddItemUseEnchantLine(CGTooltip* tooltip, CGPlayer_C* player, int32_t useSpell, int32_t useCharges, const SpellItemEnchantmentRec* enchant) {
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char format[1024];
    char cooldown[1024];
    char text[4096];
    char charges[64];
    char token[256];
    SpellRec spell;

    if (!g_spellDB.GetRecord(useSpell, &spell)) {
        return;
    }

    SpellParserParseText(&spell, format, sizeof(format), 0, 0, 0, 0, 1, 0);
    int32_t skillOk = 1;
    bool levelOk = true;

    if (enchant->m_requiredSkillID) {
        int32_t index = player->GetSkillIndex(enchant->m_requiredSkillID);

        if (index < 0 || static_cast<int32_t>(player->GetSkillValue(index)) < enchant->m_requiredSkillRank) {
            skillOk = 0;
        }
    }

    if (enchant->m_minLevel > static_cast<int32_t>(player->m_unit->m_unitFieldLevel)) {
        levelOk = false;
    }

    int32_t recoveryTime = spell.m_recoveryTime;
    int32_t categoryRecoveryTime = spell.m_categoryRecoveryTime;
    Spell_C_ModifySpellValue(&spell, recoveryTime, 11);

    if (spell.m_attributesExF >= 0) {
        Spell_C_ModifySpellValue(&spell, categoryRecoveryTime, 11);
    }

    int32_t spellCooldown = recoveryTime <= categoryRecoveryTime ? categoryRecoveryTime : recoveryTime;

    if (spellCooldown <= 0) {
        SStrPrintf(text, sizeof(text), "%s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), format);
    } else {
        CGTooltip::GetItemDurationString(cooldown, sizeof(cooldown), spellCooldown, "ITEM_COOLDOWN_TOTAL");
        SStrPrintf(text, sizeof(text), "%s %s %s", FrameScript_GetText("ITEM_SPELL_TRIGGER_ONUSE", -1, GENDER_NOT_APPLICABLE), format, cooldown);
    }

    if (useCharges && useCharges != -1) {
        SStrPrintf(charges, sizeof(charges), FrameScript_GetText("ITEM_SPELL_CHARGES", abs(useCharges), GENDER_NOT_APPLICABLE));
        SStrPrintf(format, sizeof(format), " (%s)", charges);
        SStrPack(text, format, sizeof(text));
    }

    const CImVector& color = skillOk && levelOk ? greenColor : redColor;
    tooltip->AddLine(text, nullptr, color, color, 1);

    if (!skillOk && enchant->m_requiredSkillID) {
        SkillLineRec* skill = g_skillLineDB.GetRecord(enchant->m_requiredSkillID);
        SStrCopy(token, FrameScript_GetText(enchant->m_requiredSkillRank ? "ENCHANT_ITEM_MIN_SKILL" : "ENCHANT_ITEM_REQ_SKILL", -1, GENDER_NOT_APPLICABLE), sizeof(token));

        if (enchant->m_requiredSkillRank) {
            SStrPrintf(text, sizeof(text), token, skill ? skill->m_displayName : "UNKNOWN", enchant->m_requiredSkillRank);
        } else {
            SStrPrintf(text, sizeof(text), token, skill ? skill->m_displayName : "UNKNOWN");
        }

        tooltip->AddLine(text, nullptr, redColor, redColor, 0);
    }

    if (!levelOk) {
        SStrCopy(token, FrameScript_GetText("ENCHANT_ITEM_REQ_LEVEL", -1, GENDER_NOT_APPLICABLE), sizeof(token));
        SStrPrintf(text, sizeof(text), token, enchant->m_minLevel);
        tooltip->AddLine(text, nullptr, redColor, redColor, 0);
    }
}

// INLINED
static void AddItemCreatorLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, int32_t wrapped) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
    char text[4096];
    WOWGUID guid;

    if (wrapped) {
        if (item) {
            guid = item->m_item->m_itemFieldGiftcreator;
        } else {
            if (!tooltip->m_itemUseLinkInfo) {
                return;
            }

            guid = tooltip->m_itemLinkInfo.m_giftCreator;
        }

        if (guid) {
            NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, guid, TooltipItemCallback, tooltip, true);

            if (name) {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_WRAPPED_BY", -1, GENDER_NOT_APPLICABLE), name->m_name);
                tooltip->AddLine(text, nullptr, textColor, textColor, 0);
            }
        }
    } else {
        if (item) {
            guid = item->m_item->m_itemFieldCreator;
        } else {
            if (!tooltip->m_itemUseLinkInfo) {
                return;
            }

            guid = tooltip->m_itemLinkInfo.m_creator;
        }

        if (guid) {
            NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, guid, TooltipItemCallback, tooltip, true);

            if (name) {
                if (item && (item->m_item->m_itemFieldFlags & 0x200)) {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_WRITTEN_BY", -1, GENDER_NOT_APPLICABLE), name->m_name);
                } else {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_CREATED_BY", -1, GENDER_NOT_APPLICABLE), name->m_name);
                }

                tooltip->AddLine(text, nullptr, textColor, textColor, 0);
            }
        }
    }

    if (!item) {
        return;
    }

    uint32_t flags = stats->m_flags[0];

    if (!tooltip->m_itemUseLinkInfo && (((flags & ITEM_FLAG_HAS_LOOT) && (!stats->m_lockId || (item->m_item->m_itemFieldFlags & ITEM_DFLAG_UNLOCKED))) || ((flags & ITEM_FLAG_IS_WRAPPER) && (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED)))) {
        tooltip->AddLine(FrameScript_GetText("ITEM_OPENABLE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    } else if (item->GetPageTextID(nullptr) || (item->m_item->m_itemFieldFlags & 0x200)) {
        tooltip->AddLine(FrameScript_GetText("ITEM_READABLE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }

    if (item->GetNumSockets()) {
        tooltip->AddLine(FrameScript_GetText("ITEM_SOCKETABLE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }
}

// INLINED
static void AddItemDisenchantLine(CGTooltip* tooltip, const ItemStats_C* stats, CGPlayer_C* player) {
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& disenchantColor = *reinterpret_cast<const CImVector*>(&s_tooltipDisenchantColor);
    char text[4096];
    SkillLineAbilityRec* ability = SpellTableLookupAbility(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], Spell_C_GetTargetingSpell());
    SkillLineRec* skill = g_skillLineDB.GetRecord(ability ? ability->m_skillLine : -1);
    SpellRec spell;

    if (!skill || !ability || !g_spellDB.GetRecord(Spell_C_GetTargetingSpell(), &spell) || spell.m_effect[0] != SPELL_EFFECT_DISENCHANT) {
        return;
    }

    int32_t rank = player->GetSkillRank(ability->m_skillLine);
    int32_t required = stats->m_requiredDisenchantSkill;
    const CImVector* color;

    if (required > 0) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_DISENCHANT_MIN_SKILL", -1, GENDER_NOT_APPLICABLE), skill->m_displayName, required);
        color = rank >= static_cast<int32_t>(stats->m_requiredDisenchantSkill) ? &disenchantColor : &redColor;
    } else if (!required) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_DISENCHANT_ANY_SKILL", -1, GENDER_NOT_APPLICABLE));
        color = &disenchantColor;
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_DISENCHANT_NOT_DISENCHANTABLE", -1, GENDER_NOT_APPLICABLE));
        color = &redColor;
    }

    tooltip->AddLine(text, nullptr, *color, *color, 0);
}

// INLINED
static int32_t AddItemTradeLines(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, CGPlayer_C* player) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& refundColor = *reinterpret_cast<const CImVector*>(&s_tooltipRefundColor);
    const CImVector& tradeColor = *reinterpret_cast<const CImVector*>(&s_tooltipTradeColor);
    char format[1024];
    char text[4096];
    char minutes[256];
    int32_t refundable = 0;
    CGItem_C::ItemPurchaseData* purchase = item->m_itemPurchaseData;

    if (purchase) {
        int32_t timeLeft = purchase->m_refundTimeLeft - player->GetPlayedTime() + 7200;

        if (timeLeft > 0 && !item->HasEnchantments()) {
            if (static_cast<uint32_t>(timeLeft - 3660) > 3539) {
                CGTooltip::GetDurationString(format, sizeof(format), timeLeft, "INT_SPELL_DURATION", nullptr, 0, 1);
            } else {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("INT_SPELL_DURATION_HOURS", 1, GENDER_NOT_APPLICABLE), 1);
                int32_t count = (timeLeft - 3600) / 60;
                SStrPrintf(minutes, sizeof(minutes), FrameScript_GetText("INT_SPELL_DURATION_MIN", count, GENDER_NOT_APPLICABLE), count);
                SStrPrintf(format, sizeof(format), "%s%s%s", text, FrameScript_GetText("TIME_UNIT_DELIMITER", -1, GENDER_NOT_APPLICABLE), minutes);
            }
            SStrPrintf(text, sizeof(text), FrameScript_GetText("REFUND_TIME_REMAINING", -1, GENDER_NOT_APPLICABLE), format);
            tooltip->AddLine(" ", nullptr, titleColor, titleColor, 0);
            tooltip->AddLine(text, nullptr, refundColor, refundColor, 1);
            refundable = 1;
        }
    } else if ((stats->m_flags[0] & ITEM_FLAG_ITEM_PURCHASE_RECORD) && !(item->m_itemFlags & 4)) {
        item->RequestItemPurchaseData();
        refundable = 1;
    }

    if (item->IsBound() && !item->IsPermanentlyBoundForTrade()) {
        int32_t timeLeft = item->m_item->m_itemFieldCreatePlayedTime - player->GetPlayedTime() + 7200;

        if (timeLeft > 0) {
            if (static_cast<uint32_t>(timeLeft - 3660) > 3539) {
                CGTooltip::GetDurationString(format, sizeof(format), timeLeft, "INT_SPELL_DURATION", nullptr, 0, 1);
            } else {
                SStrPrintf(text, sizeof(text), FrameScript_GetText("INT_SPELL_DURATION_HOURS", 1, GENDER_NOT_APPLICABLE), 1);
                int32_t count = (timeLeft - 3600) / 60;
                SStrPrintf(minutes, sizeof(minutes), FrameScript_GetText("INT_SPELL_DURATION_MIN", count, GENDER_NOT_APPLICABLE), count);
                SStrPrintf(format, sizeof(format), "%s%s%s", text, FrameScript_GetText("TIME_UNIT_DELIMITER", -1, GENDER_NOT_APPLICABLE), minutes);
            }
            SStrPrintf(text, sizeof(text), FrameScript_GetText("BIND_TRADE_TIME_REMAINING", -1, GENDER_NOT_APPLICABLE), format);
            tooltip->AddLine(" ", nullptr, titleColor, titleColor, 0);
            tooltip->AddLine(text, nullptr, tradeColor, tradeColor, 1);
        }
    }

    return refundable;
}

// INLINED
static void AddItemSellPrice(CGTooltip* tooltip, const ItemStats_C* stats, CGItem_C* item, CGPlayer_C* player) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    int32_t price = stats->m_sellPrice;
    int32_t maxPrice = -1;

    if (!price) {
        if (!CGMerchantInfo::m_merchant || !item) {
            return;
        }

        if (item->m_item->m_itemFieldContained == player->m_obj->m_guid) {
            WOWGUID guid = item->m_obj->m_guid;
            uint32_t slot = 0;

            for (; slot < player->m_bag.m_numSlots; slot++) {
                if (player->m_bag.m_slots[slot] == guid) {
                    break;
                }
            }

            if (slot >= player->m_bag.m_numSlots || slot < 23) {
                return;
            }
        }

        tooltip->AddLine(FrameScript_GetText("ITEM_UNSELLABLE", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        return;
    }

    if (!item) {
        if (tooltip->m_itemUseLinkInfo) {
            uint32_t stackCount = tooltip->m_itemLinkInfo.m_stackCount;
            int32_t maxStackCount = tooltip->m_itemLinkInfo.m_maxStackCount;
            int32_t sellPrice = price;
            price = stackCount * sellPrice;

            if (maxStackCount > static_cast<int32_t>(stackCount)) {
                maxPrice = sellPrice * maxStackCount;
            }
        }

        tooltip->AddMoney(price, maxPrice);
        return;
    }

    if (item->GetEnchantmentSpell(0) && !item->HasOnUseEnchant() && item->GetUsableSpellCharges() < -1) {
        price = price * item->GetCurrentSpellCharges() / item->GetUsableSpellCharges();
    }

    int32_t repairCost = item->GetRepairCost();

    if (repairCost < price) {
        if (repairCost) {
            price -= repairCost;
        }
    } else {
        price = 1;
    }

    int32_t stackCount = item->m_item->m_itemFieldStackCount;

    if (stackCount <= 1) {
        tooltip->AddMoney(price, maxPrice);
    } else {
        tooltip->AddMoney(stackCount * price, maxPrice);
    }
}

// VERIFY: 3c6e8093 manual
// OFFSET: 0x006277F0
int32_t CGTooltip::SetItem(int32_t itemId, const WOWGUID* requester, const WOWGUID* itemGuid, int32_t nameOnly, int32_t compare, int32_t useLinkInfo, int32_t append, WOWGUID owner, int32_t socketPreview, FILE* stream, int32_t hideCharges, const ItemLinkInfo* link, int32_t minimumLevel, int32_t skipLock, int32_t skipSellPrice) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);
    char text[4096];

    if (!append) {
        this->ClearTooltip();
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        this->Hide();
        return 0;
    }

    CGPlayer_C* ownerPlayer = nullptr;

    if (owner) {
        ownerPlayer = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(owner, TYPE_PLAYER, __FILE__, __LINE__));
    }

    if (!append) {
        this->m_itemGuid = *itemGuid;
        this->m_itemId = itemId;
        this->m_itemUseLinkInfo = useLinkInfo;
        this->m_itemNameOnly = nameOnly;
        this->m_itemSocketPreview = socketPreview;
        this->m_itemCompare = compare;
        this->m_itemOwner = owner;
        this->m_itemHideCharges = hideCharges;
        this->m_itemMinimumLevel = minimumLevel;
        this->m_itemSkipSellPrice = skipSellPrice;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, *requester, TooltipItemCallback, this, true);

    if (!stats) {
        return this->SetItemNoStats();
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(*itemGuid, TYPE_ITEM, __FILE__, __LINE__));
    int32_t needsUpdate = 0;
    int32_t wrapped = (item && (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED)) || (this->m_itemUseLinkInfo && this->m_itemLinkInfo.m_wrapped);
    int32_t randomPropertiesId = 0;
    int32_t useSpell = 0;
    int32_t useCharges = 0;
    const SpellItemEnchantmentRec* useEnchant = nullptr;

    if (!wrapped) {
        if (this->m_itemUseLinkInfo && this->m_itemLinkInfo.m_randomPropertiesId) {
            randomPropertiesId = this->m_itemLinkInfo.m_randomPropertiesId;

            if (randomPropertiesId > 0) {
                ItemRandomPropertiesRec* properties = g_itemRandomPropertiesDB.GetRecord(randomPropertiesId);

                if (properties) {
                    for (uint32_t i = 0; i < 5; i++) {
                        this->m_itemLinkInfo.m_enchants[i + 7] = properties->m_enchantment[i];
                    }
                }
            } else {
                ItemRandomSuffixRec* suffix = g_itemRandomSuffixDB.GetRecord(-randomPropertiesId);

                if (suffix) {
                    for (uint32_t i = 0; i < 5; i++) {
                        this->m_itemLinkInfo.m_enchants[i + 7] = i < 5 ? suffix->m_enchantment[i] : 0;
                    }
                }
            }
        } else if (item) {
            randomPropertiesId = item->m_item->m_itemFieldRandomPropertiesId;
        }
    }

    int32_t destroyGem = 0;

    if (compare) {
        if (stats->m_gemProperties) {
            this->AddLine(FrameScript_GetText("DESTROY_GEM", -1, GENDER_NOT_APPLICABLE), nullptr, redColor, redColor, 0);
            destroyGem = 1;
        } else {
            this->AddLine(FrameScript_GetText("CURRENTLY_EQUIPPED", -1, GENDER_NOT_APPLICABLE), nullptr, grayColor, grayColor, 0);
        }
    }

    AddItemNameLines(this, stats, itemId, randomPropertiesId, nameOnly, append, destroyGem);

    if (item && !AddItemPetitionLines(this, item, itemGuid)) {
        return 0;
    }

    if (stats->m_flags[0] & ITEM_FLAG_PETITION) {
        const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);
        this->AddLine(FrameScript_GetText("ITEM_SIGNABLE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }

    if (!nameOnly && !socketPreview) {
        const LockRec* lock = AddItemBindLines(this, stats, item, requester, skipLock);

        if (lock) {
            AddItemLockLines(this, stats, item, lock);
            needsUpdate = 1;
        }
    }

    const ScalingStatDistributionRec* distribution = nullptr;
    const ScalingStatValuesRec* values = nullptr;
    int32_t displayedLevel = 0;

    if (stats->m_scalingStatValue) {
        distribution = g_scalingStatDistributionDB.GetRecord(stats->m_scalingStatDistribution);
        displayedLevel = CGTooltip::GetDisplayedScalingItemLevel(stats, link, *requester, minimumLevel);
        this->m_itemDisplayedLevel = displayedLevel;
        values = g_scalingStatValuesDB.GetRecordByIndex(displayedLevel - 1);
    }

    AddItemTypeLines(this, stats, player, values, distribution);
    float averageDamage = 0.0f;

    if (!nameOnly) {
        averageDamage = AddItemDamageLines(this, stats, values);
        AddItemArmorAndStatLines(this, stats, values, distribution);
        int32_t numSockets = 0;

        if (item) {
            numSockets = item->GetNumSockets();
        } else {
            for (uint32_t i = 0; i < 3; i++) {
                if (stats->m_socketColor[i]) {
                    numSockets++;
                }
            }

            if (this->m_itemUseLinkInfo && this->m_itemLinkInfo.m_enchants[6]) {
                SpellItemEnchantmentRec* prismatic = g_spellItemEnchantmentDB.GetRecord(this->m_itemLinkInfo.m_enchants[6]);

                if (prismatic) {
                    for (uint32_t i = 0; i < 3; i++) {
                        if (prismatic->m_effect[i] == 8) {
                            numSockets += prismatic->m_effectPointsMin[i];
                            break;
                        }
                    }
                }
            }
        }

        if (!wrapped) {
            if (item || this->m_itemUseLinkInfo) {
                if (AddItemSocketLines(this, stats, item, player, ownerPlayer, owner, socketPreview, numSockets, &useSpell, &useCharges, &useEnchant)) {
                    needsUpdate = 1;
                }
            } else {
                AddItemStaticSocketLines(this, stats);
            }
        }

        if (stats->m_gemProperties && !socketPreview) {
            AddItemGemLines(this, stats, player, destroyGem);
        }
    }

    if (AddItemDurabilityLines(this, stats, item, wrapped, socketPreview)) {
        needsUpdate = 1;
    }

    if (!socketPreview) {
        AddItemRaceClassLines(this, stats, player);
    }

    AddItemLevelLines(this, stats, player, distribution, displayedLevel);

    if (!socketPreview) {
        AddItemRequirementLines(this, stats, player, itemId, requester);
    }

    ItemSetRec* set = nullptr;
    int32_t skillOk = 0;
    int32_t numItems = 0;
    int32_t numEquipped = 0;
    int32_t equipped[17];
    memset(equipped, 0, sizeof(equipped));

    if (stats->m_itemSet) {
        set = g_itemSetDB.GetRecord(stats->m_itemSet);

        if (set && !socketPreview) {
            skillOk = CollectItemSetData(this, set, player, ownerPlayer, equipped, &numItems, &numEquipped);
        }
    }

    AddItemSpellStatLines(this, stats, player, values, distribution, averageDamage);
    int32_t isRecipe = AddItemSpellTriggerLines(this, stats, item, hideCharges, stream);

    if (useSpell) {
        AddItemUseEnchantLine(this, player, useSpell, useCharges, useEnchant);
    }

    if (set && !socketPreview) {
        AddItemSetLines(this, set, player, equipped, numItems, numEquipped, skillOk, stream);
    } else if (stream) {
        for (int32_t i = 0; i < 8; i++) {
            char buffer[260];
            sprintf(buffer, "\t\t\t<setspelldesc_%d></setspelldesc_%d>\n", i, i);
            fputs(buffer, stream);
        }
    }

    if (this->m_itemUseLinkInfo && this->m_itemLinkInfo.m_cooldown && !socketPreview) {
        CGTooltip::GetItemDurationString(text, sizeof(text), this->m_itemLinkInfo.m_cooldown, "ITEM_COOLDOWN_TIME");
        this->AddLine(text, nullptr, textColor, textColor, 0);
        needsUpdate = 1;
    } else {
        uint32_t duration = 0;
        uint32_t enable = 0;
        Spell_C_GetItemCooldown(item, &duration, nullptr, &enable);

        if (duration && !enable) {
            this->AddLine(FrameScript_GetText("COOLDOWN_ON_LEAVE_COMBAT", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
            needsUpdate = 1;
        }
    }

    if (!nameOnly) {
        if (!isRecipe && stats->m_description && *stats->m_description) {
            SStrPrintf(text, sizeof(text), "\"%s\"", stats->m_description);
            this->AddLine(text, nullptr, titleColor, titleColor, 1);
        }

        if (!socketPreview) {
            AddItemCreatorLines(this, stats, item, wrapped);

            if (!append && Spell_C_IsTargeting()) {
                AddItemDisenchantLine(this, stats, player);
            }
        }

        if (*itemGuid) {
            CVar* equipmentManager = CVar::Lookup("equipmentManager");
            CVar* dontShowEquipmentSets = CVar::Lookup("dontShowEquipmentSetsOnItems");

            if (equipmentManager && equipmentManager->m_intValue && (!dontShowEquipmentSets || !dontShowEquipmentSets->m_intValue)) {
                const char* token = FrameScript_GetText("EQUIPMENT_SETS", -1, GENDER_NOT_APPLICABLE);
                char sets[1024];

                if (token && CGEquipmentManager::GetSetList(sets, sizeof(sets), itemGuid)) {
                    SStrPrintf(text, sizeof(text), token, sets);
                    this->AddLine(text, nullptr, titleColor, titleColor, 1);
                }
            }
        }

        int32_t refundable = item ? AddItemTradeLines(this, stats, item, player) : 0;

        if (CursorGetResetMode() != REPAIR_CURSOR && !refundable && !this->m_itemSkipSellPrice && !skipSellPrice) {
            AddItemSellPrice(this, stats, item, player);
        }
    }

    if (this->m_onTooltipSetItem.m_luaRef) {
        this->RunScript(this->m_onTooltipSetItem, 0, nullptr);
    }

    this->Show();
    this->CalculateSize();
    return needsUpdate;
}

// VERIFY: 223b2ae6 auto
// OFFSET: 0x0061E360
const char* CGTooltip::GetItemHyperlinkString(int32_t itemId, uint32_t quality, int32_t enchant, int32_t randomPropertiesId, int32_t seed) {
    int32_t gems[3] = { 0 };
    return CGTooltip::CreateItemLink(itemId, quality, enchant, gems, randomPropertiesId, seed);
}

// VERIFY: 2118de6f auto
// OFFSET: 0x0061E5C0
const char* CGTooltip::GetQuestLink(int32_t questId, int32_t level) {
    WOWGUID none = { 0 };
    QuestCache* quest = g_questDBCache.GetRecord(questId, none, nullptr, nullptr, false);

    if (!quest) {
        return "";
    }

    const char* color = CGTooltip::GetQuestDifficultyColorString(level);
    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Hquest:%d:%d|h[%s]|h%s", color, questId, quest->m_level, quest->m_name, *color ? "|r" : "");
    return s_itemLink;
}

// VERIFY: 0e99f273 auto
// OFFSET: 0x0061E4B0
const char* CGTooltip::GetSpellLink(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return "";
    }

    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Hspell:%d|h[%s]|h%s", s_spellLinkColorString, spellId, spell.m_name, *s_spellLinkColorString ? "|r" : "");
    return s_itemLink;
}

// VERIFY: 83eb3194 auto
// OFFSET: 0x0061A550
const char* CGTooltip::GetTalentHyperlinkString(const TalentRec* talent, const char* name, int32_t rank) {
    if (!talent || !name) {
        return "";
    }

    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Htalent:%d:%d|h[%s]|h%s", s_talentLinkColorString, talent->m_id, rank, name, *s_talentLinkColorString ? "|r" : "");
    return s_itemLink;
}

// VERIFY: 884f5bd4 auto
// OFFSET: 0x0061E650
const char* CGTooltip::GetGlyphLink(int32_t slotId, int32_t glyphId) {
    GlyphSlotRec* slot = g_glyphSlotDB.GetRecord(slotId);
    GlyphPropertiesRec* glyph = g_glyphPropertiesDB.GetRecord(glyphId);

    if (!slot || !glyph) {
        return "";
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(glyph->m_spellID, &spell)) {
        return "";
    }

    SStrPrintf(s_itemLink, sizeof(s_itemLink), "%s|Hglyph:%d:%d|h[%s]|h%s", s_glyphLinkColorString, slotId, glyphId, spell.m_name, *s_glyphLinkColorString ? "|r" : "");
    return s_itemLink;
}

// VERIFY: ed6fe309 manual
// OFFSET: 0x00622BA0
void CGTooltip::SetGlyph(int32_t slotId, int32_t glyphId, int32_t enabled, int32_t hideRemove, int32_t parseUnit) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_skillColors[0]);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipBrightRedColor);
    const CImVector& glyphColor = *reinterpret_cast<const CImVector*>(&s_tooltipGlyphColor);
    char text[1024];

    this->ClearTooltip();
    GlyphSlotRec* slot = g_glyphSlotDB.GetRecord(slotId);
    GlyphPropertiesRec* glyph = g_glyphPropertiesDB.GetRecord(glyphId);
    SpellRec spell;
    int32_t found = 0;

    if (glyph) {
        found = g_spellDB.GetRecord(glyph->m_spellID, &spell);

        if (found) {
            this->AddLine(spell.m_name, nullptr, textColor, textColor, 0);
        }
    } else if (enabled) {
        this->AddLine(FrameScript_GetText("GLYPH_INACTIVE", -1, GENDER_NOT_APPLICABLE), nullptr, grayColor, grayColor, 0);
    } else {
        this->AddLine(FrameScript_GetText("GLYPH_LOCKED", -1, GENDER_NOT_APPLICABLE), nullptr, redColor, redColor, 0);
    }

    if (slot && slot->m_type) {
        this->AddLine(FrameScript_GetText("MINOR_GLYPH", -1, GENDER_NOT_APPLICABLE), nullptr, glyphColor, glyphColor, 0);
    } else {
        this->AddLine(FrameScript_GetText("MAJOR_GLYPH", -1, GENDER_NOT_APPLICABLE), nullptr, glyphColor, glyphColor, 0);
    }

    if (!found) {
        if (enabled) {
            this->AddLine(FrameScript_GetText("GLYPH_EMPTY_DESC", -1, GENDER_NOT_APPLICABLE), nullptr, titleColor, titleColor, 1);
        } else if (slot) {
            SStrPrintf(text, sizeof(text), "GLYPH_SLOT_TOOLTIP%d", slot->m_tooltip);
            this->AddLine(FrameScript_GetText(text, -1, GENDER_NOT_APPLICABLE), nullptr, titleColor, titleColor, 1);
        }
    } else if (spell.m_description && *spell.m_description) {
        SpellParserParseText(&spell, text, sizeof(text), 0, parseUnit, 0, 0, 1, 0);
        this->AddLine(text, nullptr, titleColor, titleColor, 1);

        if (!hideRemove) {
            this->AddLine(FrameScript_GetText("GLYPH_SLOT_REMOVE_TOOLTIP", -1, GENDER_NOT_APPLICABLE), nullptr, grayColor, grayColor, 0);
        }
    }

    this->Show();
}

// VERIFY: a6d7c47a manual
// OFFSET: 0x00626E20
void CGTooltip::SetTalent(const TalentRec* talent, CGTalentGroup_C* group, int32_t isInspect, int32_t useRank, int32_t rank, int32_t isPet, int32_t usePreview) {
    if (!talent) {
        this->Hide();
        return;
    }

    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    int32_t displayRank = -1;
    int32_t currentRank = -1;
    int32_t maxRank = -1;

    for (int32_t i = 8; i >= 0; i--) {
        if (talent->m_spellRank[i]) {
            maxRank = i;
            break;
        }
    }

    if (useRank && rank >= 0 && rank <= maxRank) {
        currentRank = rank;
    } else if (group) {
        TalentGroupItem_C* item = group->GetTalentItem(talent->m_id);

        if (item) {
            displayRank = item->m_previewRank;
            currentRank = item->m_rank;
        }
    }

    if (!usePreview) {
        displayRank = currentRank;
    }

    if (talent->m_flags & 1) {
        int32_t showNext = maxRank > 0 && displayRank >= 0 && displayRank < maxRank;
        this->SetSpell(talent->m_spellRank[displayRank < 0 ? 0 : displayRank], 0, 0, isPet, 0, isInspect, useRank, talent, group, 0, usePreview, displayRank < 0 ? 0 : displayRank, displayRank, maxRank, !showNext);

        if (showNext) {
            this->SetSpell(talent->m_spellRank[displayRank + 1], 0, 0, isPet, 0, isInspect, useRank, talent, group, 1, usePreview, displayRank + 1, displayRank, maxRank, 1);
        }

        return;
    }

    this->ClearTooltip();
    int32_t spellId = talent->m_spellRank[displayRank];
    int32_t nextSpellId = talent->m_spellRank[displayRank + 1];
    SpellRec spell;
    SpellRec nextSpell;
    int32_t hasSpell = displayRank < 0 ? 0 : g_spellDB.GetRecord(spellId, &spell);
    int32_t hasNext = displayRank >= maxRank ? 0 : g_spellDB.GetRecord(nextSpellId, &nextSpell);

    if (!hasSpell && !hasNext) {
        return;
    }

    this->m_spellId = hasSpell ? spellId : 0;
    this->m_nextRankSpellId = hasNext ? nextSpellId : 0;

    if (hasSpell) {
        this->AddLine(spell.m_name, nullptr, textColor, textColor, 0);
    } else if (hasNext) {
        this->AddLine(nextSpell.m_name, nullptr, textColor, textColor, 0);
    }

    char text[2048];

    if (maxRank >= 0) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("TOOLTIP_TALENT_RANK", -1, GENDER_NOT_APPLICABLE), displayRank + 1, maxRank + 1);
        this->AddLine(text, nullptr, textColor, textColor, 0);
    }

    int32_t prereqsMet = hasSpell ? 1 : this->AddTalentPrereqs(talent, group, isInspect, isPet, usePreview);
    int32_t described = 0;

    if (hasSpell && spell.m_description && *spell.m_description) {
        SpellParserParseText(&spell, text, sizeof(text), isPet, isInspect, 0, 0, 1, 0);
        this->AddLine(text, nullptr, titleColor, titleColor, 1);
        described = 1;
    }

    if (hasNext && nextSpell.m_description && *nextSpell.m_description) {
        if (described) {
            this->AddLine(" ", nullptr, titleColor, titleColor, 0);
            this->AddLine(FrameScript_GetText("TOOLTIP_TALENT_NEXT_RANK", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        }

        SpellParserParseText(&nextSpell, text, sizeof(text), isPet, isInspect, 0, 0, 1, 0);
        this->AddLine(text, nullptr, titleColor, titleColor, 1);
    }

    this->AddTalentLearnText(talent, group, prereqsMet, currentRank, maxRank, isInspect, useRank, isPet, usePreview);

    if (this->m_onTooltipSetSpell.m_luaRef) {
        this->RunScript(this->m_onTooltipSetSpell, 0, nullptr);
    }

    this->Show();
}

// VERIFY: 17b021d1 auto
// OFFSET: 0x006201C0
void CGTooltip::AddLine(const char* left, const char* right, int32_t wrap) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    this->AddLine(left, right, titleColor, titleColor, wrap);
}

// VERIFY: 1678db84 auto
// OFFSET: 0x0062D930
int32_t CGTooltip::SetItemDelta(const CGItemStatsSummary* summary, int32_t append) {
    if (!append) {
        this->ClearTooltip();
    }

    int32_t first = 1;
    char text[1024];

    if (fabs(summary->m_dps) > 0.00000011920929f) {
        this->AddDeltaHeader(&first);
        char sign = summary->m_dps <= 0.0f ? '-' : '+';
        const char* color = summary->m_dps <= 0.0f ? s_negativeDeltaColor : s_colorStrings[3];
        float dps = fabs(summary->m_dps);
        SStrPrintf(text, sizeof(text), "%s%c%.1f%s %s", color, sign, dps, "|r", FrameScript_GetText("ITEM_MOD_DAMAGE_PER_SECOND_SHORT", -1, GENDER_NOT_APPLICABLE));
        const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
        this->AddLine(text, nullptr, textColor, textColor, 0);
    }

    for (uint32_t i = 0; i < 73; i++) {
        if (i < 69 && i != 49 && i != 50) {
            this->AddColoredItemStat(summary->m_stats[i], FrameScript_GetText(CGItem_C::GetStatName(i, text, sizeof(text)), -1, GENDER_NOT_APPLICABLE), &first);
        }
    }

    char name[1024];

    for (uint32_t i = 69; i < 73; i++) {
        if (!summary->m_stats[i]) {
            continue;
        }

        const char* label = FrameScript_GetText(CGItem_C::GetStatName(i, name, sizeof(name)), -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(text, sizeof(text), "|TInterface\\ItemSocketingFrame\\UI-EmptySocket-%s.blp:12|t  %s", s_socketColorNames[i - 69], label);
        this->AddColoredItemStat(summary->m_stats[i], text, &first);
    }

    return 1;
}

// VERIFY: 39a89fbc manual
// OFFSET: 0x00630C70
int32_t CGItemStatsSummary::CreateFromTooltip(CGTooltip* tooltip) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(tooltip->m_itemGuid, TYPE_ITEM, __FILE__, __LINE__));

    if (item) {
        this->Create(item, tooltip->m_itemDisplayedLevel);
        return 1;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(tooltip->m_itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    this->Create(stats, &tooltip->m_itemLinkInfo, tooltip->m_itemDisplayedLevel);
    return 1;
}

// VERIFY: 153be78f manual
// OFFSET: 0x00630D20
int32_t CGTooltip::SetMacro(uint32_t macroId) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    this->ClearTooltip();
    MACRONODE* macro = CGUIMacros::GetMacroByID(macroId);

    if (!macro) {
        this->Hide();
        return 0;
    }

    uint32_t dynamic = macro->m_dynamic;

    if (macro->m_showTooltip) {
        if (macro->m_itemID) {
            uint32_t duration = 0;
            uint32_t start = 0;
            uint32_t enable = 0;
            CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(macro->m_itemID);

            if (enchantItem) {
                Spell_C_GetItemCooldown(enchantItem, &duration, &start, &enable);
            } else {
                Spell_C_GetItemCooldown(macro->m_itemID, &duration, &start, &enable);
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
            CGItem_C* item;

            if (player && (item = player->m_bag.FindItemOfType(macro->m_itemID, 0))) {
                this->m_itemLinkInfo.Clear();
                this->m_itemLinkInfo.m_cooldown = enable && start && duration ? duration + start - OsGetAsyncTimeMs() : 0;
                WOWGUID none = { 0 };
                ItemStats_C* stats = g_itemDBCache.GetRecord(macro->m_itemID, none, nullptr, nullptr, false);

                if (stats && stats->m_spellCharges[0] != -1 && stats->m_spellCharges[0]) {
                    this->m_itemLinkInfo.m_spellCharges = player->m_bag.GetItemTypeCharges(macro->m_itemID, 0);
                }

                WOWGUID guid = item->m_obj->m_guid;
                return this->SetItem(item->m_obj->m_entryID, &guid, &guid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0) | dynamic;
            }

            WOWGUID none = { 0 };
            return this->SetItem(macro->m_itemID, &none, &none, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0) | dynamic;
        }

        if (macro->m_spellID) {
            uint32_t duration = 0;
            uint32_t start = 0;
            uint32_t enable = 0;
            Spell_C_GetSpellCooldown(macro->m_spellID, macro->m_spellFlags, &duration, &start, &enable);
            int32_t brief = 1;
            CVar* uber = CVar::Lookup("UberTooltips");

            if (uber && uber->m_intValue) {
                brief = 0;
            }

            if (!enable || !start || !duration) {
                return this->SetSpell(macro->m_spellID, brief, 0, 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0) | dynamic;
            }

            return this->SetSpell(macro->m_spellID, brief, start + duration - OsGetAsyncTimeMs(), 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0) | dynamic;
        }
    }

    this->AddLine(macro->m_name, nullptr, textColor, textColor, 0);
    this->Show();
    return 0;
}

// VERIFY: a5127ade auto
// OFFSET: 0x0061A4F0
const uint32_t* CGTooltip::GetThreatStateColor(uint32_t status) {
    if (status >= 5) {
        status = 1;
    }

    return &s_threatStatusColors[status];
}

// VERIFY: a3a22009 auto
// OFFSET: 0x0061DBC0
CGItemStatsSummary CGItemStatsSummary::operator-(const CGItemStatsSummary& other) const {
    CGItemStatsSummary result;
    memset(result.m_stats, 0, sizeof(result.m_stats));
    result.m_dps = 0.0f;
    result.m_flags = 0;

    for (uint32_t i = 0; i < 73; i++) {
        result.m_stats[i] = this->m_stats[i] - other.m_stats[i];
    }

    result.m_dps = this->m_dps - other.m_dps;
    result.CleanStatNames();
    return result;
}

// VERIFY: e2f9e6ae auto
// OFFSET: 0x0061DD20
void TooltipCorpseCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (static_cast<uint32_t>(tooltip->m_corpseGuid.m_guid)) {
        WOWGUID corpse = tooltip->m_corpseGuid;
        tooltip->SetCorpse(corpse);
        tooltip->CalculateSize();
    }
}

// VERIFY: 746c613d manual
// OFFSET: 0x00631590
int32_t CGItemStatsSummary::GetItemComparison(CGTooltip* first, CGTooltip* second, CGItemStatsSummary* result) {
    struct ComparisonEntry {
        CGItemStatsSummary m_summary;
        int32_t m_itemIds[2];
        int32_t m_randomProperties[2];
        uint32_t m_seeds[2];
        int32_t m_levels[2];

        ComparisonEntry() {
            this->m_summary.Create();
            memset(this->m_itemIds, 0, sizeof(this->m_itemIds));
            memset(this->m_randomProperties, 0, sizeof(this->m_randomProperties));
            memset(this->m_seeds, 0, sizeof(this->m_seeds));
            memset(this->m_levels, 0, sizeof(this->m_levels));
        }
    };

    static ComparisonEntry s_cache[2];
    int32_t itemIds[2];
    uint32_t seeds[2];
    int32_t randomProperties[2];
    int32_t levels[2];

    for (int32_t i = 0; i < 2; i++) {
        CGTooltip* tooltip = i ? second : first;
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(tooltip->m_itemGuid, TYPE_ITEM, __FILE__, __LINE__));

        if (item) {
            itemIds[i] = item->m_obj->m_entryID;
            randomProperties[i] = item->m_item->m_itemFieldRandomPropertiesId;
            seeds[i] = item->m_item->m_itemFieldPropertySeed;
        } else {
            randomProperties[i] = tooltip->m_itemLinkInfo.m_randomPropertiesId;
            seeds[i] = tooltip->m_itemLinkInfo.m_propertySeed;
            itemIds[i] = tooltip->m_itemId;
        }

        levels[i] = tooltip->m_itemDisplayedLevel;
    }

    for (uint32_t i = 0; i < 2; i++) {
        ComparisonEntry& entry = s_cache[i];

        if (itemIds[0] == entry.m_itemIds[0] && randomProperties[0] == entry.m_randomProperties[0] && (!randomProperties[0] || seeds[0] == entry.m_seeds[0]) && levels[0] == entry.m_levels[0] && itemIds[1] == entry.m_itemIds[1] && randomProperties[1] == entry.m_randomProperties[1] && (!randomProperties[1] || seeds[1] == entry.m_seeds[1]) && levels[1] == entry.m_levels[1]) {
            memcpy(result, &entry.m_summary, sizeof(CGItemStatsSummary));
            return 1;
        }
    }

    CGItemStatsSummary firstSummary;
    firstSummary.Create();
    CGItemStatsSummary secondSummary;
    secondSummary.Create();

    if (!firstSummary.CreateFromTooltip(first) || !secondSummary.CreateFromTooltip(second)) {
        return 0;
    }

    int32_t index = 1 - s_comparisonIndex;
    ComparisonEntry& entry = s_cache[index];
    entry.m_itemIds[0] = itemIds[0];
    entry.m_randomProperties[0] = randomProperties[0];
    entry.m_seeds[0] = seeds[0];
    entry.m_levels[0] = levels[0];
    entry.m_itemIds[1] = itemIds[1];
    entry.m_randomProperties[1] = randomProperties[1];
    entry.m_seeds[1] = seeds[1];
    entry.m_levels[1] = levels[1];
    s_comparisonIndex = index;
    CGItemStatsSummary delta = firstSummary - secondSummary;
    memcpy(&entry.m_summary, &delta, sizeof(CGItemStatsSummary));
    memcpy(result, &entry.m_summary, sizeof(CGItemStatsSummary));
    return 1;
}

// VERIFY: 09f37021 manual
// OFFSET: 0x00631850
int32_t SetCompareItem(CGTooltip* tooltip, uint32_t itemId, int32_t offset, int32_t shift, CGTooltip* compare) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, &TooltipItemCallback, tooltip, false);

    if (!player || !stats) {
        return 0;
    }

    uint32_t slots = s_inventoryTypeSlots[stats->m_inventoryType];

    if (offset == 2) {
        if (stats->m_scalingStatValue && CGMerchantInfo::m_merchant && !shift) {
            WOWGUID itemGuid = WOWGUID();
            WOWGUID merchant = CGMerchantInfo::m_merchant;
            tooltip->SetItem(itemId, &merchant, &itemGuid, 0, 0, 0, 0, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 1, 0, 0);
            return 1;
        }

        return 0;
    }

    CGItem_C* item = nullptr;

    if ((slots & 0x8000) || (slots & 0x10000)) {
        CGItem_C* mainHand = nullptr;

        if (((slots & 0x8000) && (slots & 0x10000)) || stats->m_inventoryType == 17) {
            mainHand = player->m_bag.GetItemPointer(15);
            CGItem_C* offHand = player->m_bag.GetItemPointer(16);

            if (mainHand != offHand && mainHand && offHand) {
                item = offset ? mainHand : offHand;
            } else if (offset) {
                return 0;
            } else {
                item = mainHand ? mainHand : offHand;
            }
        } else {
            if (offset) {
                return 0;
            }

            if (slots & 0x8000) {
                item = player->m_bag.GetItemPointer(15);
            } else if (!(slots & 0x10000)) {
                return 0;
            } else {
                item = player->m_bag.GetItemPointer(16);

                if (!item) {
                    mainHand = player->m_bag.GetItemPointer(15);
                    item = mainHand && mainHand->GetInventoryType() == 17 ? mainHand : nullptr;
                }
            }
        }

        if (!item) {
            return 0;
        }
    } else {
        for (int32_t slot = 0; slot <= 18; slot++) {
            if (!((1 << slot) & slots)) {
                continue;
            }

            CGItem_C* candidate = player->m_bag.GetItemPointer(slot);

            if (!candidate) {
                continue;
            }

            if (!offset) {
                item = candidate;
                break;
            }

            offset--;
        }

        if (!item) {
            return 0;
        }
    }

    WOWGUID itemGuid = item->m_obj->m_guid;
    WOWGUID requester = player->m_obj->m_guid;
    tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 1, 0, 0, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 0, 0, 0);

    if (compare) {
        CGItemStatsSummary summary;

        if (CGItemStatsSummary::GetItemComparison(compare, tooltip, &summary)) {
            tooltip->SetItemDelta(&summary, 1);
        }
    }

    return 1;
}

// VERIFY: 2c134cf7 manual
// OFFSET: 0x0061A5B0
static int32_t QSortFrameStack(const void* a, const void* b) {
    const FrameStackInfo* infoA = static_cast<const FrameStackInfo*>(a);
    const FrameStackInfo* infoB = static_cast<const FrameStackInfo*>(b);

    if (infoA->m_strata < infoB->m_strata) {
        return 1;
    }

    if (infoA->m_strata > infoB->m_strata) {
        return -1;
    }

    if (infoA->m_level < infoB->m_level) {
        return 1;
    }

    if (infoA->m_level > infoB->m_level) {
        return -1;
    }

    return SStrCmpI(infoA->m_name, infoB->m_name, STORM_MAX_STR);
}

// VERIFY: eedaf448 auto
// OFFSET: 0x0061DCE0
void TooltipObjectCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (static_cast<uint32_t>(tooltip->m_objectGuid.m_guid)) {
        WOWGUID object = tooltip->m_objectGuid;
        tooltip->SetObject(object);
        tooltip->CalculateSize();
    }
}

// VERIFY: 18827091 auto
// OFFSET: 0x0061DE20
void TooltipQuestCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (tooltip->m_questId) {
        tooltip->SetQuest(tooltip->m_questId);
        tooltip->CalculateSize();
    }
}

// VERIFY: b4a0294d manual
// OFFSET: 0x0061DE50
void TooltipAchievementCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    CGTooltip* tooltip = static_cast<CGTooltip*>(param);

    if (tooltip->m_achievementId) {
        tooltip->SetAchievement(tooltip->m_achievementId, tooltip->m_achievementGuid, tooltip->m_achievementCompleted, tooltip->m_achievementTime, tooltip->m_achievementCriteria[0], tooltip->m_achievementCriteria[1], tooltip->m_achievementCriteria[2], tooltip->m_achievementCriteria[3], 1);
        tooltip->CalculateSize();
    }
}

// VERIFY: 32b57856 manual
// OFFSET: 0x00620950
void CGTooltip::AddEmpathyText(CGUnit_C* unit, char* buffer, uint32_t size) {
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    int32_t minDamage = static_cast<int32_t>(unit->m_unit->m_unitFieldMindamage);
    int32_t maxDamage = static_cast<int32_t>(ceil(static_cast<double>(unit->m_unit->m_unitFieldMaxdamage)));

    if (minDamage == maxDamage) {
        SStrPrintf(buffer, size, FrameScript_GetText("SINGLE_DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), minDamage);
    } else {
        SStrPrintf(buffer, size, FrameScript_GetText("DAMAGE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), minDamage, maxDamage);
    }

    this->AddLine(buffer, nullptr, textColor, textColor, 0);

    int32_t armor = unit->m_unit->m_unitFieldResistances[GetPhysicalDamageClassID()] > 0 ? unit->m_unit->m_unitFieldResistances[GetPhysicalDamageClassID()] : 0;
    SStrPrintf(buffer, size, FrameScript_GetText("ARMOR_TEMPLATE", -1, GENDER_NOT_APPLICABLE), armor);
    this->AddLine(buffer, nullptr, textColor, textColor, 0);

    SStrPrintf(buffer, size, FrameScript_GetText("HP_TEMPLATE", -1, GENDER_NOT_APPLICABLE), unit->m_unit->m_unitFieldHealth);
    this->AddLine(buffer, nullptr, textColor, textColor, 0);

    for (int32_t i = 0; i < 7; i++) {
        if (i == GetPhysicalDamageClassID() || unit->m_unit->m_unitFieldResistances[i] <= 0) {
            continue;
        }

        char token[32];
        SStrPrintf(token, sizeof(token), "RESISTANCE%d_NAME", i);
        const char* name = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);

        if (name && *name) {
            SStrPrintf(buffer, size, FrameScript_GetText("RESISTANCE_TEMPLATE", -1, GENDER_NOT_APPLICABLE), unit->m_unit->m_unitFieldResistances[i], name);
            this->AddLine(buffer, nullptr, textColor, textColor, 0);
        }
    }

    CreatureFamilyRec* family = g_creatureFamilyDB.GetRecord(unit->GetCreatureFamily());

    if (family && family->m_petFoodMask) {
        char diet[256];
        diet[0] = '\0';
        int32_t first = 1;
        uint32_t count = g_itemPetFoodDB.m_numRecords;

        for (uint32_t i = 0; i < count; i++) {
            ItemPetFoodRec* food = g_itemPetFoodDB.GetRecordByIndex(i);

            if (food && ((1 << (food->m_id - 1)) & family->m_petFoodMask)) {
                if (!first) {
                    SStrPack(diet, ", ", sizeof(diet));
                }

                SStrPack(diet, food->m_name, sizeof(diet));
                first = 0;
            }
        }

        if (!first) {
            SStrPrintf(buffer, size, FrameScript_GetText("PET_DIET_TEMPLATE", -1, GENDER_NOT_APPLICABLE), diet);
            this->AddLine(buffer, nullptr, textColor, textColor, 0);
        }
    }

    CreatureStats_C* stats = unit->m_creatureCacheEntry;

    if (!stats || !(stats->m_typeFlags & 0x1) || !stats->m_family) {
        this->AddLine(FrameScript_GetText("NOT_TAMEABLE", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);
        return;
    }

    this->AddLine(FrameScript_GetText(stats->m_typeFlags & 0x10000 ? "TAMEABLE_EXOTIC" : "TAMEABLE", -1, GENDER_NOT_APPLICABLE), nullptr, textColor, textColor, 0);

    family = g_creatureFamilyDB.GetRecord(unit->GetCreatureFamily());

    if (!family) {
        return;
    }

    char spells[512];
    spells[0] = '\0';
    int32_t first = 1;

    for (uint32_t j = 0; j < 2; j++) {
        for (int32_t i = 0; i < g_skillLineAbilityDB.m_numRecords; i++) {
            SkillLineAbilityRec* ability = g_skillLineAbilityDB.GetRecordByIndex(i);

            if (!ability || ability->m_skillLine != family->m_skillLine[j]) {
                continue;
            }

            SpellRec spell;

            if (g_spellDB.GetRecord(ability->m_spell, &spell) && !(spell.m_attributes & 0xC0) && !ability->m_supercededBySpell && ability->m_acquireMethod == 2) {
                if (!first) {
                    SStrPack(spells, ", ", sizeof(spells));
                }

                SStrPack(spells, spell.m_name, sizeof(spells));
                first = 0;
            }
        }
    }

    if (!first) {
        SStrPrintf(buffer, size, FrameScript_GetText("PET_SPELLS_TEMPLATE", -1, GENDER_NOT_APPLICABLE), spells);
        this->AddLine(buffer, nullptr, textColor, textColor, 0);
    }
}

// VERIFY: fa5a7b6d auto
// OFFSET: 0x00620EE0
void CGTooltip::AddSkinnableText(CGUnit_C* unit, char* buffer, uint32_t size) {
    *buffer = '\0';
    CImVector color = *reinterpret_cast<const CImVector*>(&s_skillColors[4]);
    const char* suffix = "";
    int32_t skinningSpell = CGSpellBook::GetSkinningSpell(unit);

    if (skinningSpell) {
        SpellRec spell;
        g_spellDB.GetRecord(skinningSpell, &spell);

        for (uint32_t i = 0; i < 3; i++) {
            if (spell.m_effect[i] != SPELL_EFFECT_SKINNING) {
                continue;
            }

            int32_t skill;
            int32_t maxSkill;
            Spell_C_GetMinMaxPoints(&spell, i, &skill, &maxSkill, 0, 0, 0, 0);

            if (skill > 0) {
                int32_t level = unit->m_unit->m_unitFieldLevel;
                int32_t required;

                if (level <= 10) {
                    required = 1;
                } else if (level < 20) {
                    required = (5 * level - 50) * 2;
                } else {
                    required = 5 * level;
                }

                color = *GetSkillColor(required, skill, &suffix);
            }

            break;
        }
    }

    const char* token;

    switch (unit->GetSkinningType()) {
        case 0:
            token = "UNIT_SKINNABLE_LEATHER";
            break;
        case 1:
            token = "UNIT_SKINNABLE_HERB";
            break;
        case 2:
            token = "UNIT_SKINNABLE_ROCK";
            break;
        case 3:
            token = "UNIT_SKINNABLE_BOLTS";
            break;
        default:
            return;
    }

    SStrPrintf(buffer, size, "%s%s", suffix, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE));

    if (*buffer) {
        this->AddLine(buffer, nullptr, color, color, 0);
    }
}

// VERIFY: 214e3d9a auto
// OFFSET: 0x00622960
int32_t CGTooltip::SetQuest(int32_t questId) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);

    this->ClearTooltip();

    WOWGUID requester = { 0 };
    this->m_questId = questId;
    QuestCache* quest = g_questDBCache.GetRecord(questId, requester, TooltipQuestCallback, this, false);

    if (!quest) {
        return 0;
    }

    this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);

    if (CGQuestLog::GetQuestIndex(questId) > 0) {
        this->AddLine(FrameScript_GetText("QUEST_TOOLTIP_ACTIVE", -1, GENDER_NOT_APPLICABLE), nullptr, greenColor, greenColor, 0);
    }

    this->AddLine(" ", nullptr, titleColor, titleColor, 0);

    char text[1024];
    WOWGUID player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(quest->m_objText, text, sizeof(text), player, nullptr, 0);
    this->AddLine(text, nullptr, textColor, textColor, 1);

    int32_t count = GetNumQuestLeaderBoards(questId, TooltipQuestCallback, this);

    if (count > 0) {
        this->AddLine(" ", nullptr, titleColor, titleColor, 0);
        this->AddLine(FrameScript_GetText("QUEST_TOOLTIP_REQUIREMENTS", -1, GENDER_NOT_APPLICABLE), nullptr, titleColor, titleColor, 0);

        QuestLogLeaderBoardParams params;
        params.m_showProgress = 0;
        params.m_questCallback = TooltipQuestCallback;
        params.m_objectCallback = TooltipQuestCallback;
        params.m_creatureCallback = TooltipQuestCallback;
        params.m_itemCallback = TooltipQuestCallback;
        params.m_param = this;

        for (int32_t i = 0; i < count; i++) {
            params.m_index = i;

            if (CGQuestLog::GetQuestLeaderBoardByID(questId, &params)) {
                char line[4099] = " - ";
                SStrPack(line, params.m_text, STORM_MAX_STR);
                this->AddLine(line, nullptr, textColor, textColor, 0);
            }
        }
    }

    if (this->m_onTooltipSetQuest.m_luaRef) {
        this->RunScript(this->m_onTooltipSetQuest, 0, nullptr);
    }

    this->Show();
    return 1;
}

// VERIFY: b202b707 manual
// OFFSET: 0x00622DD0
int32_t CGTooltip::SetEquipmentSet(uint32_t setId) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& redColor = *reinterpret_cast<const CImVector*>(&s_tooltipRedColor);
    const CImVector& grayColor = *reinterpret_cast<const CImVector*>(&s_tooltipGrayColor);

    this->ClearTooltip();

    const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(setId);

    if (!set) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    int32_t equipped = 0;
    int32_t inInventory = 0;
    int32_t numMissing = 0;
    int32_t ignored = 0;
    uint32_t flags = CGBankInfo::m_banker.m_guid ? 14 : 6;
    int32_t missing[19];

    for (int32_t slot = 0; slot <= 18; slot++) {
        WOWGUID guid = set->m_items[slot];

        if (!guid.m_guid) {
            continue;
        }

        if (guid.m_guid == 1) {
            ignored++;
        } else if (static_cast<uint32_t>(player->m_bag.FindItemSlot(guid)) > 18) {
            if (player->m_bag.ContainsItem(guid, true, flags)) {
                inInventory++;
            } else {
                missing[numMissing++] = slot;
            }
        } else {
            equipped++;
        }
    }

    char text[1024];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEMS_VARIABLE_QUANTITY", -1, GENDER_NOT_APPLICABLE), equipped + inInventory + numMissing);

    if (text[0]) {
        this->AddLine(set->m_name, text, textColor, titleColor, 0);
    } else {
        this->AddLine(set->m_name, nullptr, textColor, textColor, 0);
    }

    if (equipped) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEMS_EQUIPPED", -1, GENDER_NOT_APPLICABLE), equipped);
        this->AddLine(text, nullptr, titleColor, titleColor, 0);
    }

    if (inInventory) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEMS_IN_INVENTORY", -1, GENDER_NOT_APPLICABLE), inInventory);
        this->AddLine(text, nullptr, textColor, textColor, 0);
    }

    if (ignored) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_SLOTS_IGNORED", -1, GENDER_NOT_APPLICABLE), ignored);
        this->AddLine(text, nullptr, grayColor, grayColor, 0);
    }

    for (int32_t i = numMissing; i > 0; i--) {
        const char* slotName = FrameScript_GetText(GetInventorySlotToken(missing[i - 1]), -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ITEM_MISSING", -1, GENDER_NOT_APPLICABLE), slotName);
        this->AddLine(text, nullptr, redColor, redColor, 0);
    }

    if (this->m_onTooltipSetEquipmentSet.m_luaRef) {
        this->RunScript(this->m_onTooltipSetEquipmentSet, 0, nullptr);
    }

    this->Show();
    return 0;
}

// VERIFY: ccbe79da manual
// OFFSET: 0x006230D0
void CGTooltip::SetFrameStack(bool showHidden) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);

    this->ClearTooltip();

    CSimpleTop* top = CSimpleTop::s_instance;
    float x = 0.0f;
    float y = 0.0f;
    NDCToDDC(top->m_mousePosition.m_x, top->m_mousePosition.m_y, &x, &y);

    CSimpleFrame* frame = top->m_frames.Head();

    if (!frame) {
        return;
    }

    char text[1024] = { 0 };
    CRect rect;
    rect.minY = 0.0f;
    rect.minX = 0.0f;
    rect.maxY = 0.0f;
    rect.maxX = 0.0f;
    lua_State* L = FrameScript_GetContext();

    for (; frame; frame = frame->m_framesLink.Next()) {
        if (!frame->GetRect(&rect)) {
            continue;
        }

        uint32_t visible = frame->m_isVisible;

        if ((!visible && !showHidden) || x < rect.minX || x > rect.maxX || y > rect.maxY || y < rect.minY) {
            continue;
        }

        FrameStackInfo info;
        const char* name = frame->GetName();

        if (!name || !*name) {
            if (!frame->m_luaRegistered) {
                frame->RegisterScriptObject(nullptr);
            }

            lua_rawgeti(L, LUA_REGISTRYINDEX, frame->m_luaObjectRef);
            SStrPrintf(info.m_name, sizeof(info.m_name), "%s: %p", lua_typename(L, lua_type(L, -1)), lua_topointer(L, -1));
            lua_settop(L, -2);
        } else {
            lua_pushstring(L, name);
            lua_rawget(L, LUA_GLOBALSINDEX);

            if (lua_type(L, -1) == LUA_TTABLE) {
                lua_rawgeti(L, -1, 0);
                void* object = lua_touserdata(L, -1);
                lua_settop(L, -3);

                if (object && object == frame) {
                    SStrCopy(info.m_name, name, STORM_MAX_STR);
                } else {
                    lua_rawgeti(L, LUA_REGISTRYINDEX, frame->GetLuaObjectRef());
                    SStrPrintf(info.m_name, sizeof(info.m_name), "%s: %p (%s)", lua_typename(L, lua_type(L, -1)), lua_topointer(L, -1), name);
                    lua_settop(L, -2);
                }
            } else {
                SStrPrintf(info.m_name, sizeof(info.m_name), "%s: %p (%s)", lua_typename(L, lua_type(L, -1)), lua_topointer(L, -1), name);
                lua_settop(L, -2);
            }
        }

        info.m_strata = frame->m_strata;
        info.m_level = frame->m_level;
        info.m_mouseEnabled = (frame->m_enabledEventsBitmask & 0x4) != 0;
        info.m_visible = visible;
        s_frameStack.Add(1, &info);
    }

    qsort(s_frameStack.m_data, s_frameStack.m_count, sizeof(FrameStackInfo), QSortFrameStack);

    SStrPrintf(text, sizeof(text), "(%.2f, %.2f)", DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * x), DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * y));
    this->AddLine(FrameScript_GetText("DEBUG_FRAMESTACK", -1, GENDER_NOT_APPLICABLE), text, textColor, titleColor, 0);

    int32_t toggle = 1;
    int32_t lastStrata = 9;
    int32_t lastLevel = 0;

    for (uint32_t i = 0; i < s_frameStack.m_count; i++) {
        FrameStackInfo* info = &s_frameStack.m_data[i];
        int32_t strata = info->m_strata;

        if (strata != lastStrata) {
            this->AddLine(FrameStrataToString(static_cast<FRAME_STRATA>(strata)), nullptr, textColor, textColor, 0);
            lastLevel = info->m_level;
            lastStrata = strata;
            toggle = 0;
        }

        if (info->m_level != lastLevel) {
            toggle = (toggle + 1) % 2;
            lastLevel = info->m_level;
        }

        SStrPrintf(text, sizeof(text), " <%d> %s", info->m_level, info->m_name);

        if (info->m_visible) {
            const CImVector* color = info->m_mouseEnabled ? reinterpret_cast<const CImVector*>(&s_frameStackMouseColors[toggle]) : reinterpret_cast<const CImVector*>(&s_frameStackColors[toggle]);
            this->AddLine(text, nullptr, *color, *color, 0);
        } else {
            this->AddLine(text, nullptr, *reinterpret_cast<const CImVector*>(&s_frameStackHiddenColors[toggle]), *reinterpret_cast<const CImVector*>(&s_frameStackHiddenColors[toggle]), 0);
        }
    }

    if (this->m_onTooltipSetFrameStack.m_luaRef) {
        this->RunScript(this->m_onTooltipSetFrameStack, 0, nullptr);
    }

    this->Show();
    s_frameStack.m_count = 0;
}

// VERIFY: 21654104 auto
// OFFSET: 0x00625350
void CGTooltip::SetBuff(int32_t spellId, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);

    this->ClearTooltip();

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        this->Hide();
        return;
    }

    if (spell.m_cumulativeAura <= 1) {
        a3 = 1;
    }

    const char* dispelName = nullptr;

    if (spell.m_dispelType) {
        SpellDispelTypeRec* dispel = g_spellDispelTypeDB.GetRecord(spell.m_dispelType);

        if (dispel && dispel->m_immunityPossible) {
            dispelName = dispel->m_name;
        }
    }

    this->AddLine(spell.m_name, dispelName, titleColor, titleColor, 0);

    if (spell.m_auraDescription && *spell.m_auraDescription) {
        char text[1024];
        SpellParserParseText(&spell, text, sizeof(text), a6, a5, a2, 1, a3, 1);
        this->AddLine(text, nullptr, textColor, textColor, 1);
    }
}

// VERIFY: 96f1d53f manual
// OFFSET: 0x00626720
void CGTooltip::SetObject(const WOWGUID& guid) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);

    this->ClearTooltip();

    CGGameObject_C* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (!object) {
        this->Hide();
        return;
    }

    this->m_objectGuid = guid;

    if (this->m_backdropGenerator) {
        this->m_backdropGenerator->SetVertexColor(*reinterpret_cast<const CImVector*>(&s_tooltipObjectBackdropColor));
    }

    this->AddLine(object->CGGameObject_C::GetObjectName(), nullptr, titleColor, titleColor, 0);
    object->AddToolTipInfo(this);

    char text[4096];

    if (object->m_gameObject->m_flags & 0x2) {
        CImVector color = *reinterpret_cast<const CImVector*>(&s_skillColors[4]);
        int32_t spellId = 0;
        int32_t skillValue = 0;
        int32_t requiredSkill = 0;
        CGItem_C* key = nullptr;
        text[0] = '\0';

        if (!object->IsLocked(&spellId, &skillValue, &requiredSkill, &key, nullptr)) {
            if (spellId && !key) {
                const char* suffix;
                color = *GetSkillColor(requiredSkill, skillValue, &suffix);
                SStrCopy(text, suffix, sizeof(text));
            } else {
                color = *reinterpret_cast<const CImVector*>(&s_skillColors[1]);
            }
        }

        SStrPack(text, FrameScript_GetText("LOCKED", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        this->AddLine(FrameScript_GetText("LOCKED", -1, GENDER_NOT_APPLICABLE), nullptr, color, color, 0);
    }

    LockRec* lock = object->GetLockRec();

    if (lock && object->IsValidOpenAction(lock->m_action[0])) {
        switch (lock->m_type[0]) {
            case 1: {
                WOWGUID requester = object->m_obj->m_guid;
                ItemStats_C* stats = g_itemDBCache.GetRecord(lock->m_index[0], requester, TooltipObjectCallback, this, true);

                if (stats) {
                    SStrPrintf(text, sizeof(text), FrameScript_GetText("LOCKED_WITH_ITEM", -1, GENDER_NOT_APPLICABLE), stats->GetName(0));
                    this->AddLine(text, nullptr, textColor, textColor, 0);
                }

                break;
            }
            case 2: {
                int32_t spellId = 0;
                int32_t skillValue = 0;
                int32_t requiredSkill = 0;
                CImVector color = textColor;
                const char* suffix = "";
                char format[128];
                object->IsLocked(&spellId, &skillValue, &requiredSkill, nullptr, nullptr);

                if (spellId) {
                    color = *GetSkillColor(requiredSkill, skillValue, &suffix);
                    FormatString("LOCKED_WITH_SPELL_KNOWN", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
                } else if (!(object->m_gameObject->m_flags & 0x2)) {
                    color.Set(1.0f, 1.0f, 0.0f, 0.0f);
                    FormatString("LOCKED_WITH_SPELL", -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
                } else {
                    break;
                }

                char skill[256];
                LockTypeRec* lockType = g_lockTypeDB.GetRecord(lock->m_index[0]);
                SStrPrintf(skill, sizeof(skill), format, lockType ? lockType->m_name : "UNKNOWN");
                SStrPrintf(text, sizeof(text), "%s%s", suffix, skill);
                this->AddLine(text, nullptr, color, color, 0);
                break;
            }
            case 3: {
                SpellRec spell;

                if (g_spellDB.GetRecord(lock->m_index[0], &spell)) {
                    this->AddSpellReagents(&spell, "LOCKED_WITH_SPELL", TooltipObjectCallback);
                }

                break;
            }
        }
    }
    CVar* questTracking = CVar::Lookup("showQuestTrackingTooltips");

    if (questTracking && questTracking->m_intValue) {
        QuestLogLeaderBoardParams params;
        params.m_questCallback = TooltipObjectCallback;
        params.m_objectCallback = TooltipObjectCallback;
        params.m_creatureCallback = TooltipObjectCallback;
        params.m_itemCallback = TooltipObjectCallback;
        params.m_param = this;
        const char* objective = params.m_text;
        char buffer[4099];

        for (uint32_t i = 0; i < static_cast<uint32_t>(CGQuestLog::m_numQuest); i++) {
            WOWGUID requester = { 0 };
            QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::GetQuestAt(i), requester, TooltipObjectCallback, this, false);

            if (!quest) {
                continue;
            }

            bool header = false;

            if (CGQuestLog::GetQuestLeaderBoardForObject(quest->m_id, object->m_obj->m_entryID | 0x80000000, &params)) {
                this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                SStrPrintf(buffer, sizeof(buffer), " - %s", objective);
                this->AddLine(buffer, nullptr, textColor, textColor, 0);
                header = true;
            }

            GameObjectStats* stats = object->m_stats;

            if (!stats) {
                continue;
            }

            for (uint32_t j = 0; j < sizeof(stats->m_questItems) / sizeof(stats->m_questItems[0]); j++) {
                uint32_t itemId = stats->m_questItems[j];

                if (!itemId) {
                    break;
                }

                if (CGQuestLog::GetQuestLeaderBoardForItem(quest->m_id, itemId, &params)) {
                    if (!header) {
                        this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                        header = true;
                    }

                    SStrPrintf(buffer, sizeof(buffer), " - %s", objective);
                    this->AddLine(buffer, nullptr, textColor, textColor, 0);
                }

                if (CGQuestLog::IsQuestCompleteEx(i, 1)) {
                    continue;
                }

                for (uint32_t k = 0; k < sizeof(quest->m_intermediateItemId) / sizeof(quest->m_intermediateItemId[0]); k++) {
                    if (quest->m_intermediateItemId[k] != itemId) {
                        continue;
                    }

                    if (!header) {
                        this->AddLine(quest->m_name, nullptr, titleColor, titleColor, 0);
                        header = true;
                    }

                    WOWGUID itemRequester = { 0 };
                    ItemStats_C* itemStats = g_itemDBCache.GetRecord(quest->m_intermediateItemId[k], itemRequester, TooltipUnitNameCallback, this, false);

                    if (itemStats) {
                        SStrPrintf(buffer, sizeof(buffer), " - %s", itemStats->m_name[0]);
                        this->AddLine(buffer, nullptr, textColor, textColor, 0);
                    }
                }
            }
        }
    }

    if (this->m_statusBar && object->m_gameObject->m_typeID == 33) {
        this->m_healthGuid = this->m_objectGuid;
        ClntObjMgrSetObjMirrorHandler(this->m_healthGuid, ID_GAMEOBJECT, offsetof(CGGameObjectData, m_animProgress), sizeof(uint8_t), ObjHealthUpdateHandler, this->m_statusBar, HANDLER_PRIORITY_NORMAL, 0);
        this->m_statusBar->SetMinMaxValues(0.0f, 1.0f);
        this->m_statusBar->SetValue(static_cast<float>(object->m_gameObject->m_animProgress) * 0.0039215689f);
        this->m_statusBar->Show();
        this->Show();
    } else {
        this->m_healthGuid = WOWGUID();
        this->Show();
    }
}

// VERIFY: b08d75d5 manual
// OFFSET: 0x00627220
int32_t CGTooltip::SetAchievement(int32_t achievementId, WOWGUID guid, bool completed, WowTime time, uint32_t criteria1, uint32_t criteria2, uint32_t criteria3, uint32_t criteria4, int32_t a10) {
    const CImVector& titleColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
    const CImVector& greenColor = *reinterpret_cast<const CImVector*>(&s_tooltipGreenColor);

    AchievementRec* rec = g_achievementDB.GetRecord(achievementId);

    if (!rec) {
        return 0;
    }

    WOWGUID requester = { 0 };
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, requester, TooltipAchievementCallback, this, true);

    if (!name && !a10) {
        return 1;
    }

    this->ClearTooltip();
    this->m_achievementGuid = guid;
    this->m_achievementId = achievementId;
    this->m_achievementCriteria[1] = criteria2;
    this->m_achievementTime = time;
    this->m_achievementCompleted = completed;
    this->m_achievementCriteria[2] = criteria3;
    this->m_achievementCriteria[0] = criteria1;
    this->m_achievementCriteria[3] = criteria4;

    char title[800];
    QuestParserParseText(rec->m_title, title, sizeof(title), guid, nullptr, 0);
    this->AddLine(title, nullptr, textColor, textColor, 0);
    this->AddLine(" ", nullptr, titleColor, titleColor, 0);

    char status[1024];

    if (completed) {
        SStrPrintf(status, sizeof(status), FrameScript_GetText("ACHIEVEMENT_TOOLTIP_COMPLETE", -1, GENDER_NOT_APPLICABLE), name ? name->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE), time.m_month, time.m_monthDay, time.m_year);
    } else {
        SStrPrintf(status, sizeof(status), FrameScript_GetText("ACHIEVEMENT_TOOLTIP_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE), name ? name->m_name : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    }

    this->AddLine(status, nullptr, greenColor, greenColor, 0);
    this->AddLine(" ", nullptr, titleColor, titleColor, 0);

    char description[3000];
    QuestParserParseText(rec->m_description, description, sizeof(description), guid, nullptr, 0);
    this->AddLine(description, nullptr, textColor, textColor, 1);

    char left[1024];
    char right[1024];
    left[0] = '\0';
    right[0] = '\0';
    this->AddLine(" ", nullptr, titleColor, titleColor, 0);

    CImVector leftColor(0, 0, 0, 0);
    CImVector rightColor(0, 0, 0, 0);
    int32_t index = 0;

    ACHIEVEMENT* achievement = s_achievements.Ptr(achievementId, HASHKEY_NONE());

    if (achievement->m_rec->m_sharesCriteria) {
        achievement = s_achievements.Ptr(achievement->m_rec->m_sharesCriteria, HASHKEY_NONE());
    }

    if (achievement && (!achievement->m_rec->m_minimumCriteria || criteria1 != 0xFFFFFFFF || criteria2 != 0xFFFFFFFF || criteria3 != 0xFFFFFFFF || criteria4 != 0xFFFFFFFF)) {
        for (SPECIFIC_CRITERIA* node = achievement->m_criteria.Head(); node; node = achievement->m_criteria.Next(node)) {
            Achievement_CriteriaRec* criteria = node->m_criteria;

            if (!criteria || (criteria->m_flags & 0x2)) {
                continue;
            }

            bool met;

            if (index < 32) {
                met = ((1 << index) & criteria1) != 0;
            } else if (index < 64) {
                met = ((1 << (index - 32)) & criteria2) != 0;
            } else if (index < 96) {
                met = ((1 << (index - 64)) & criteria3) != 0;
            } else if (index < 128) {
                met = ((1 << (index - 96)) & criteria4) != 0;
            } else {
                met = false;
            }

            index++;

            if (left[0]) {
                SStrPrintf(right, sizeof(right), criteria->m_description);
                rightColor = *reinterpret_cast<const CImVector*>(met ? &s_skillColors[1] : &s_skillColors[0]);
            } else {
                SStrPrintf(left, sizeof(left), criteria->m_description);
                leftColor = *reinterpret_cast<const CImVector*>(met ? &s_skillColors[1] : &s_skillColors[0]);
            }

            if (right[0]) {
                this->AddLine(left, right, leftColor, rightColor, 0);
                left[0] = '\0';
                right[0] = '\0';
            }
        }
    }

    if (!right[0] && left[0]) {
        this->AddLine(left, nullptr, leftColor, leftColor, 0);
    }

    if (this->m_onTooltipSetAchievement.m_luaRef) {
        this->RunScript(this->m_onTooltipSetAchievement, 0, nullptr);
    }

    this->Show();
    return 1;
}
