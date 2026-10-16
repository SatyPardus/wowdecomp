#include "SpellBookFrame.h"
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Magic/MagicClient/SpellTable.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Magic/Spell.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/DB/Rec/SkillRaceClassInfoRec.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include <cstdio>

// STATIC: 0x00BE8E24
TSGrowableArray<SpellUsableInfo> s_shapeshiftSpells;

// STATIC: 0x00BE8E54
static TSGrowableArray<SpellUsableInfo> s_knownSpells;

// STATIC: 0x00BE5D84
int32_t CGSpellBook::m_autoRangedSpell;

// STATIC: 0x00BE8DA8
uint32_t CGSpellBook::m_interruptSchoolMask;

// STATIC: 0x00BE8DAC
uint32_t CGSpellBook::m_interruptSpecialSchoolMask;

// STATIC: 0x00BE8DB0
uint32_t CGSpellBook::m_silenceSchoolMask;

// STATIC: 0x00BE8DB4
uint32_t CGSpellBook::m_silenceSpecialSchoolMask;

// STATIC: 0x00BE7D98
uint32_t s_petSpellList[1024];

// STATIC: 0x00BE8DA4
uint32_t CGSpellBook::m_numPetSpells;

// STATIC: 0x00BE8DB8
TSBitField<TSFixedArray<uint32_t> > s_knownSpellBits;

// STATIC: 0x00BE8DD8
TSFixedArray<int32_t> CGSpellBook::m_languageSpells;

// STATIC: 0x00ACCCCC
static int32_t s_multiCastSpells[4] = { 66842, 66843, 66844, 36936 };

// STATIC: 0x00BE5D68
static int32_t s_dispelMask;

// STATIC: 0x00BE5D6C
static int32_t s_skinningSpells[4];

// STATIC: 0x00BE5D7C
static int32_t s_skinPlayerCorpseSpell;

// STATIC: 0x00BE5D80
static int32_t s_pendingCompanions;

// STATIC: 0x00BE5D88
uint32_t s_displayedSpells[1024];

// STATIC: 0x00BE6D88
int32_t s_spellList[1024];

// STATIC: 0x00BE7D88
int32_t CGSpellBook::m_summonFriendSpell;

// STATIC: 0x00BE7D8C
int32_t CGSpellBook::m_stuckSpell;

// STATIC: 0x00BE7D90
int32_t CGSpellBook::m_duelSpell;

// STATIC: 0x00BE8D98
uint32_t s_numSpells;

// STATIC: 0x00BE8D9C
uint32_t s_displayedSpellCount;

// STATIC: 0x00BE8DA0
uint32_t s_numSkillInfos;

// STATIC: 0x00BE8DC8
TSGrowableArray<SpellBookSkillInfo*> s_skillInfos;

// STATIC: 0x00BE8DE4
TSGrowableArray<int32_t> s_trackingSpells;

// STATIC: 0x00BE8DF4
TSGrowableArray<int32_t> CGSpellBook::m_companions[2];

// STATIC: 0x00BE8E14
TSGrowableArray<int32_t> s_openLockSpells;

// STATIC: 0x00BE8E34
static TSGrowableArray<int32_t> s_dispelTypes;

// STATIC: 0x00BE8E44
static TSGrowableArray<int32_t> s_summonCreatures;

// STATIC: 0x00BE8EA0
TSGrowableArray<int32_t> CGSpellBook::m_totemSpells[4];

// STATIC: 0x00BE8E64
static TSHashTable<SPELLSBYNAME, HASHKEY_UTF8I> s_spellsByName;

// STATIC: 0x00BE8E8C
TSGrowableArray<int32_t> CGSpellBook::m_unlearnedSpells;

// VERIFY: ee08c995 manual
// OFFSET: 0x0053CF10
void CGSpellBook::UpdateUsable() {
    CGActionBar::UpdateUsable();
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        SpellRec spell;

        for (uint32_t i = 0; i < s_knownSpells.m_count; i++) {
            SpellUsableInfo& entry = s_knownSpells.m_data[i];
            g_spellDB.GetRecord(entry.m_spellId, &spell);

            WOWGUID target = { 0 };
            bool usable = Spell_C_IsUsableAction(player, &spell, target, nullptr, false);
            bool power = Spell_C_HaveSpellPower(player, &spell);
            bool noMana = !power;

            if ((usable && power) != (entry.m_usable && !entry.m_noMana) && usable && !noMana) {
                SendToCombatText(35, spell.m_name);
            }

            entry.m_usable = usable;
            entry.m_noMana = noMana;
        }

        int32_t changed = 0;

        if (s_shapeshiftSpells.m_count) {
            for (uint32_t i = 0; i < s_shapeshiftSpells.m_count; i++) {
                SpellUsableInfo& entry = s_shapeshiftSpells.m_data[i];
                g_spellDB.GetRecord(entry.m_spellId, &spell);

                WOWGUID target = { 0 };
                bool usable = Spell_C_IsUsableAction(player, &spell, target, nullptr, false);
                bool power = Spell_C_HaveSpellPower(player, &spell);

                if ((usable && power) != (entry.m_usable && !entry.m_noMana)) {
                    changed = 1;
                }

                entry.m_usable = usable;
                entry.m_noMana = !power;
            }

            if (changed) {
                FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_USABLE, nullptr);
            }
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_SPELL_UPDATE_USABLE, nullptr);
}

// VERIFY: bcebf4cf manual
// OFFSET: 0x0053F1F0
void CGSpellBook::InitializeGame() {
    s_knownSpellBits.m_count = g_spellDB.m_maxID + 1;
    s_knownSpellBits.m_bits.SetCount(s_knownSpellBits.m_count ? ((s_knownSpellBits.m_count - 1) >> 5) + 1 : 0);
    CGSpellBook::m_languageSpells.SetCount(g_languagesDB.m_maxID + 1);
    CGSpellBook::ClearSpells();
    memset(s_petSpellList, 0, sizeof(s_petSpellList));
    CGSpellBook::m_numPetSpells = 0;
}

// VERIFY: adefcf10 manual
// OFFSET: 0x0053B540
int32_t CGSpellBook::GetSpellProgressionRank(const SpellRec* spell) {
    const char* subtext = spell->m_nameSubtext;
    int32_t rank = 0;
    uint32_t length = SStrLen(subtext);

    for (uint32_t i = 0; i < length; i++) {
        if (IsNumeric(static_cast<uint16_t>(subtext[i]))) {
            rank = rank * 10 + subtext[i] - '0';
        } else if (rank) {
            return rank;
        }
    }

    return rank;
}

// VERIFY: 3e93ab26 manual
// OFFSET: 0x0053EC40
void CGSpellBook::ClearSpells() {
    if (s_knownSpellBits.m_count) {
        memset(s_knownSpellBits.m_bits.m_data, 0, sizeof(s_knownSpellBits.m_bits.m_data[0]) * s_knownSpellBits.m_bits.m_count);
    }

    s_numSpells = 0;
    s_displayedSpellCount = 0;
    memset(s_spellList, 0, sizeof(s_spellList));
    memset(s_displayedSpells, 0, sizeof(s_displayedSpells));

    for (uint32_t i = 0; i < s_skillInfos.Count(); i++) {
        if (s_skillInfos.m_data[i]) {
            delete s_skillInfos.m_data[i];
        }
    }

    for (uint32_t i = 0; i < sizeof(s_skinningSpells) / sizeof(s_skinningSpells[0]); i++) {
        s_skinningSpells[i] = 0;
    }

    s_skinPlayerCorpseSpell = 0;
    s_skillInfos.Clear();
    s_numSkillInfos = 0;
    CGSpellBook::m_duelSpell = 0;
    CGSpellBook::m_stuckSpell = 0;
    CGSpellBook::m_summonFriendSpell = 0;
    CGSpellBook::m_autoRangedSpell = 0;
    s_pendingCompanions = 0;

    for (uint32_t i = CGSpellBook::m_languageSpells.Count(); i;) {
        CGSpellBook::m_languageSpells.m_data[--i] = 0;
    }

    s_trackingSpells.Clear();

    for (uint32_t i = 0; i < sizeof(CGSpellBook::m_companions) / sizeof(CGSpellBook::m_companions[0]); i++) {
        CGSpellBook::m_companions[i].Clear();
    }

    s_openLockSpells.Clear();
    s_shapeshiftSpells.Clear();
    s_dispelTypes.Clear();
    s_summonCreatures.Clear();
    s_dispelMask = 0;
    s_knownSpells.Clear();

    for (uint32_t i = 0; i < sizeof(CGSpellBook::m_totemSpells) / sizeof(CGSpellBook::m_totemSpells[0]); i++) {
        CGSpellBook::m_totemSpells[i].Clear();
    }
}

// VERIFY: f6883164 manual
// OFFSET: 0x00542030
void CGSpellBook::AddKnownSpell(int32_t spellId, int32_t notify, int32_t replaceSpell) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    s_knownSpellBits.m_bits.m_data[static_cast<uint32_t>(spellId) >> 5] |= 1 << (spellId & 0x1F);

    switch (spell.m_effect[0]) {
        case 28:
        case 56: {
            int32_t creature = spell.m_effectMiscValue[0];

            if (creature) {
                uint32_t i;

                for (i = 0; i < s_summonCreatures.Count(); i++) {
                    if (s_summonCreatures.m_data[i] == creature) {
                        break;
                    }
                }

                if (i >= s_summonCreatures.Count()) {
                    *s_summonCreatures.New() = creature;
                }
            }

            break;
        }

        case 33: {
            *s_openLockSpells.New() = spellId;
            break;
        }

        case 39: {
            CGSpellBook::m_languageSpells.m_data[spell.m_effectMiscValue[0]] = spellId;
            CGChat::UpdateLanguages();
            break;
        }

        case 83: {
            CGSpellBook::m_duelSpell = spellId;
            break;
        }

        case 84: {
            CGSpellBook::m_stuckSpell = spellId;
            break;
        }

        case 95: {
            s_skinningSpells[spell.m_effectMiscValue[0]] = spellId;
            break;
        }

        case 116: {
            s_skinPlayerCorpseSpell = spellId;
            break;
        }

        case 152: {
            CGSpellBook::m_summonFriendSpell = spellId;
            break;
        }
    }

    if (SpellIsTrackingAura(&spell)) {
        *s_trackingSpells.New() = spellId;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] != 38 || spell.m_implicitTargetA[i] == 6) {
            continue;
        }

        uint32_t j;

        for (j = 0; j < s_dispelTypes.Count(); j++) {
            if (s_dispelTypes.m_data[j] == spell.m_effectMiscValue[i]) {
                break;
            }
        }

        if (j >= s_dispelTypes.Count()) {
            *s_dispelTypes.New() = spell.m_effectMiscValue[i];
        }
    }

    if (spell.m_effect[0] == 126 && spell.m_implicitTargetA[0] == 6) {
        SpellDispelTypeRec* dispel = g_spellDispelTypeDB.GetRecord(spell.m_effectMiscValue[0]);

        if (dispel && dispel->m_mask) {
            s_dispelMask = dispel->m_mask;
        } else {
            s_dispelMask = 1 << spell.m_effectMiscValue[0];
        }
    }

    if (spell.m_attributesExD & 0x1000000) {
        CGSpellBook::m_autoRangedSpell = spellId;
    }

    if (spell.m_attributesExD & 0x200) {
        uint32_t i;

        for (i = 0; i < s_knownSpells.Count(); i++) {
            SpellRec other;
            g_spellDB.GetRecord(s_knownSpells.m_data[i].m_spellId, &other);

            if (SStrCmp(spell.m_name, other.m_name, STORM_MAX_STR)) {
                continue;
            }

            int32_t rank = CGSpellBook::GetSpellProgressionRank(&spell);

            if (!rank) {
                rank = spell.m_spellLevel;
            }

            int32_t otherRank = CGSpellBook::GetSpellProgressionRank(&other);

            if (!otherRank) {
                otherRank = other.m_spellLevel;
            }

            if (static_cast<uint32_t>(rank) > static_cast<uint32_t>(otherRank)) {
                s_knownSpells.m_data[i].m_spellId = spellId;
            }

            break;
        }

        if (i >= s_knownSpells.Count()) {
            SpellUsableInfo* info = s_knownSpells.New();
            info->m_spellId = spellId;
            info->m_usable = 0;
            info->m_noMana = 0;
        }
    }

    if (spell.m_attributesExB & 0x8100) {
        Spell_C_SetCooldownLeft(spell.m_id, 0, spell.m_category, spell.m_recoveryTime, 0, true, 0);
    }

    if (!(spell.m_attributesExB & 0x2)) {
        bool shapeshift = false;

        for (uint32_t i = 0; i < 3; i++) {
            if (spell.m_effectAura[i] == 36 || (spell.m_attributesExB & 0x10)) {
                shapeshift = true;
                break;
            }
        }

        if (shapeshift) {
            CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
            uint32_t count = s_shapeshiftSpells.Count();

            if (replaceSpell) {
                for (uint32_t i = 0; i < count; i++) {
                    if (s_shapeshiftSpells.m_data[i].m_spellId != replaceSpell) {
                        continue;
                    }

                    s_shapeshiftSpells.m_data[i].m_spellId = spellId;
                    WOWGUID target = WOWGUID();
                    s_shapeshiftSpells.m_data[i].m_usable = Spell_C_IsUsableAction(player, &spell, target, nullptr, false);
                    s_shapeshiftSpells.m_data[i].m_noMana = !Spell_C_HaveSpellPower(player, &spell);
                    Script_SendUniqueSignal(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORMS);
                    break;
                }
            } else {
                uint32_t i;

                for (i = 0; i < count; i++) {
                    if (s_shapeshiftSpells.m_data[i].m_spellId == spellId) {
                        break;
                    }
                }

                if (i >= count) {
                    s_shapeshiftSpells.SetCount(count + 1);
                    s_shapeshiftSpells.m_data[count].m_spellId = spellId;
                    WOWGUID target = WOWGUID();
                    s_shapeshiftSpells.m_data[count].m_usable = Spell_C_IsUsableAction(player, &spell, target, nullptr, false);
                    s_shapeshiftSpells.m_data[count].m_noMana = !Spell_C_HaveSpellPower(player, &spell);
                    qsort(s_shapeshiftSpells.m_data, s_shapeshiftSpells.Count(), sizeof(s_shapeshiftSpells.m_data[0]), SortSpellUsableInfo);
                    Script_SendUniqueSignal(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORMS);
                }
            }
        }
    }

    if (spell.m_attributes & 0x80) {
        return;
    }

    if (spell.m_attributes & 0x20) {
        if (notify) {
            CGGameUI::DisplayError(ERR_LEARN_RECIPE_S, spell.m_name);
        }

        return;
    }

    CGSpellBook::AddSpellToNameCache(spellId, 0);

    if (SpellIsCallCompanion(&spell)) {
        int32_t type = spell.m_effect[0] == 28 ? 0 : (spell.m_effectAura[0] != 78) + 1;
        *CGSpellBook::m_companions[type].New() = spellId;

        if (type == 0) {
            CGTutorial::TriggerTutorial(51);
        } else if (type == 1) {
            CGTutorial::TriggerTutorial(52);
        }

        WOWGUID requester = { 0 };
        requester.m_guid = static_cast<uint32_t>(spellId) | 0x1FE0000000000000ULL;
        CreatureStats_C* creature = g_creatureDBCache.GetRecord(spell.m_effectMiscValue[0], requester, CGSpellBook::CompanionCallback, reinterpret_cast<void*>(static_cast<intptr_t>(notify)), false);

        if (!creature) {
            s_pendingCompanions++;
            return;
        }

        if (notify) {
            CGGameUI::DisplayError(ERR_LEARN_COMPANION_S, creature->m_name[0]);
            CGSpellBook::UpdateCompanions();
            FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_LEARNED, nullptr);
        }

        return;
    }

    if (notify) {
        char name[512];

        if (*spell.m_nameSubtext) {
            SStrPrintf(name, sizeof(name), "%s (%s)", spell.m_name, spell.m_nameSubtext);
        } else {
            SStrCopy(name, spell.m_name, sizeof(name));
        }

        CGGameUI::DisplayError(spell.m_attributes & 0x10 ? ERR_LEARN_ABILITY_S : ERR_LEARN_SPELL_S, name);
    }

    if (spell.m_attributesExD & 0x8000) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    int32_t updateMultiCast = 0;
    bool tutorialShown = CGTutorial::m_tutorialShown.m_count ? !((CGTutorial::m_tutorialShown.m_bits.m_data[1] >> 14) & 1) : false;

    if (spell.m_attributesExG & 0x20) {
        for (uint32_t i = 0; i < 2; i++) {
            int32_t category = spell.m_requiredTotemCategoryID[i];
            uint32_t slotMask = CGActionBar::GetMultiCastSlotMaskForTotemCategory(category);

            for (uint32_t slot = 0; slot < 4; slot++) {
                if (!((1 << slot) & slotMask)) {
                    continue;
                }

                TSGrowableArray<int32_t>& totems = CGSpellBook::m_totemSpells[slot];
                uint32_t j;

                for (j = 0; j < totems.Count(); j++) {
                    if (totems.m_data[j] == spellId) {
                        break;
                    }
                }

                if (j < totems.Count()) {
                    continue;
                }

                bool found = false;
                int32_t replace = -1;
                uint32_t rank = CGSpellBook::GetSpellProgressionRank(&spell);

                if (!rank) {
                    rank = spell.m_spellLevel;
                }

                if (rank) {
                    SpellRec other;

                    for (j = 0; j < totems.Count(); j++) {
                        if (!g_spellDB.GetRecord(totems.m_data[j], &other)) {
                            continue;
                        }

                        if (SStrCmp(spell.m_name, other.m_name, STORM_MAX_STR)) {
                            continue;
                        }

                        found = true;
                        uint32_t otherRank = CGSpellBook::GetSpellProgressionRank(&other);

                        if (!otherRank) {
                            otherRank = other.m_spellLevel;
                        }

                        if (otherRank < rank) {
                            replace = j;
                        }

                        break;
                    }
                }

                if (found) {
                    if (replace >= 0) {
                        totems.m_data[replace] = spellId;
                        updateMultiCast = notify;
                    }
                } else {
                    int32_t* entry = totems.New();

                    if (entry) {
                        *entry = spellId;
                    }

                    updateMultiCast = notify;
                }

                if (!tutorialShown && updateMultiCast && player && player->m_bag.FindTotemCategory(category, 8)) {
                    CGTutorial::TriggerTutorial(46);
                    tutorialShown = true;
                }
            }
        }
    }

    int32_t skillLine = 0;

    if (player) {
        SkillLineAbilityRec* ability = player->LookupAbility(spellId);

        if (ability) {
            const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], ability->m_skillLine);

            if (info && !(info->m_flags & 0x80)) {
                skillLine = info->m_skillID;
            }
        }
    }

    int32_t tab = -1;

    for (uint32_t i = 0; i < s_numSkillInfos; i++) {
        if (s_skillInfos.m_data[i]->m_skillLine == skillLine) {
            s_skillInfos.m_data[i]->m_count++;
            tab = i;
            break;
        }
    }

    if (tab == -1) {
        if (s_numSkillInfos == s_skillInfos.Count()) {
            s_skillInfos.SetCount(s_numSkillInfos + 1);
            s_skillInfos.m_data[s_numSkillInfos] = static_cast<SpellBookSkillInfo*>(SMemAlloc(sizeof(SpellBookSkillInfo), __FILE__, __LINE__, 0));
        }

        s_skillInfos.m_data[s_numSkillInfos]->m_skillLine = skillLine;
        s_skillInfos.m_data[s_numSkillInfos]->m_count = 1;
        s_skillInfos.m_data[s_numSkillInfos++]->m_unk08 = 1;
    }

    s_spellList[s_numSpells++] = spellId;

    if (notify) {
        CGSpellBook::UpdateSpells(1, 0, 1);

        if (tab == -1) {
            for (uint32_t i = 0; i < s_numSkillInfos; i++) {
                if (s_skillInfos.m_data[i]->m_skillLine == skillLine) {
                    tab = i;
                    break;
                }
            }
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_LEARNED_SPELL_IN_TAB, "%d", tab + 1);
        CGTutorial::TriggerTutorial(39);

        for (uint32_t i = 0; i < sizeof(s_multiCastSpells) / sizeof(s_multiCastSpells[0]); i++) {
            if (s_multiCastSpells[i] == spellId) {
                updateMultiCast = 1;
                break;
            }
        }

        if (player && player->m_unit->m_unitFieldLevel <= 10 && CGSpellBook::GetSpellProgressionRank(&spell) <= 1 && !(spell.m_attributes & 0x40) && !(spell.m_attributesExB & 0x10) && !SpellRec::HasAura(&spell, 36)) {
            int32_t offset = GetActionBarPageOffset(player, &spell);

            for (int32_t slot = offset; slot < offset + 12; slot++) {
                if (!CGActionBar::HasAction(slot)) {
                    CGGameUI::SetCursorSpell(spell.m_id);
                    CGActionBar::PutActionInSlot(slot);
                    break;
                }
            }
        }
    }

    if (updateMultiCast) {
        CGActionBar::UpdateMultiCastBar();
    }
}

// VERIFY: fe19e0f0 auto
// OFFSET: 0x0053CEC0
void CGSpellBook::UpdateCompanions() {
    if (s_pendingCompanions) {
        return;
    }

    bool sorted = false;

    for (uint32_t i = 0; i < sizeof(CGSpellBook::m_companions) / sizeof(CGSpellBook::m_companions[0]); i++) {
        if (CGSpellBook::m_companions[i].Count()) {
            qsort(CGSpellBook::m_companions[i].m_data, CGSpellBook::m_companions[i].Count(), sizeof(CGSpellBook::m_companions[i].m_data[0]), SortCompanions);
            sorted = true;
        }
    }

    if (sorted) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_UPDATE, nullptr);
    }
}

// VERIFY: 3365a24e manual
// OFFSET: 0x0053CA70
void CGSpellBook::UpdateSpells(int32_t a1, int32_t a2, int32_t a3) {
    if (a1) {
        qsort(s_skillInfos.m_data, s_numSkillInfos, sizeof(s_skillInfos.m_data[0]), SortSkillInfoByName);
        qsort(s_spellList, s_numSpells, sizeof(s_spellList[0]), SortSpellsForDisplay);

        if (s_trackingSpells.Count()) {
            qsort(s_trackingSpells.m_data, s_trackingSpells.Count(), sizeof(s_trackingSpells.m_data[0]), SortSpellsForDisplay);
        }
    }

    if (a2) {
        qsort(s_petSpellList, CGSpellBook::m_numPetSpells, sizeof(s_petSpellList[0]), SortSpellsForDisplay);
    }

    uint32_t spellIndex = 0;
    s_displayedSpellCount = 0;
    memset(s_displayedSpells, 0, sizeof(s_displayedSpells));

    for (uint32_t tab = 0; tab < s_numSkillInfos; tab++) {
        SpellRec previous;
        SpellRec current;
        int32_t visible = 0;
        bool found = g_spellDB.GetRecord(s_spellList[spellIndex], &previous) != 0;
        spellIndex++;

        if (found && s_skillInfos.m_data[tab]->m_count) {
            int32_t* spell = &s_spellList[spellIndex];
            uint32_t j = 0;

            while (true) {
                if (!g_spellDB.GetRecord(*spell, &current)) {
                    visible++;
                    s_displayedSpells[s_displayedSpellCount++] = s_spellList[spellIndex + j - 1];
                    break;
                }

                if (SStrCmp(current.m_name, previous.m_name, STORM_MAX_STR) || !CGSpellBook::GetSpellProgressionRank(&current)) {
                    visible++;
                    s_displayedSpells[s_displayedSpellCount++] = spell[-1];
                    previous = current;
                }

                spell++;

                if (++j >= static_cast<uint32_t>(s_skillInfos.m_data[tab]->m_count)) {
                    break;
                }
            }
        }

        s_skillInfos.m_data[tab]->m_unk08 = visible;
        spellIndex += s_skillInfos.m_data[tab]->m_count - 1;
    }

    if (a3) {
        CGSpellBook::m_interruptSchoolMask = 0;
        CGSpellBook::m_interruptSpecialSchoolMask = 0;
        CGSpellBook::m_silenceSchoolMask = 0;
        CGSpellBook::m_silenceSpecialSchoolMask = 0;

        for (uint32_t i = 0; i < s_numSpells; i++) {
            SpellRec spell;

            if (!g_spellDB.GetRecord(s_spellList[i], &spell)) {
                continue;
            }

            for (uint32_t j = 0; j < 3; j++) {
                if (spell.m_effect[j] == 6) {
                    if (spell.m_effectAura[j] == 27) {
                        CGSpellBook::m_silenceSchoolMask |= spell.m_schoolMask;
                    }
                } else if (spell.m_effect[j] == 68) {
                    CGSpellBook::m_interruptSchoolMask |= spell.m_schoolMask;
                }
            }

            if (spell.m_attributesExG & 0x800) {
                CGSpellBook::m_interruptSpecialSchoolMask |= spell.m_schoolMask;
            }

            if (spell.m_attributesExG & 0x1000) {
                CGSpellBook::m_silenceSpecialSchoolMask |= spell.m_schoolMask;
            }
        }
    }

    if (a1 || a2) {
        CGUIMacros::UpdateMacroSpells();
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_SPELLS_CHANGED, nullptr);
}

// VERIFY: 5c7e305b manual
// OFFSET: 0x0053B970
int32_t GetActionBarPageOffset(const CGPlayer_C* player, const SpellRec* spell) {
    uint8_t classId = player->m_unit->m_unitFieldBytes0[1];

    if (classId != 1 && classId != 4 && classId != 11) {
        return 0;
    }

    SpellShapeshiftFormRec* form;

    if ((spell->m_shapeshiftMask[0] || spell->m_shapeshiftMask[1]) && !(spell->m_attributesExB & 0x80000)) {
        uint32_t mask = spell->m_shapeshiftMask[0];
        bool high = false;

        if (!mask) {
            mask = spell->m_shapeshiftMask[1];
            high = true;
        }

        int32_t index = 1;

        for (; mask; index++) {
            if (mask & 1) {
                break;
            }

            mask >>= 1;
        }

        if (high) {
            index += 32;
        }

        form = g_spellShapeshiftFormDB.GetRecord(index);
    } else {
        if (classId != 1) {
            return 0;
        }

        form = g_spellShapeshiftFormDB.GetRecord(player->GetShapeshiftForm());
    }

    if (!form) {
        return 0;
    }

    return 12 * (form->m_bonusActionBar + 5);
}

// VERIFY: aefb1bad auto
// OFFSET: 0x0053BA40
int __cdecl SortSkillInfoByName(const void* a, const void* b) {
    int32_t skillA = (*static_cast<SpellBookSkillInfo* const*>(a))->m_skillLine;

    if (!skillA) {
        return -1;
    }

    int32_t skillB = (*static_cast<SpellBookSkillInfo* const*>(b))->m_skillLine;

    if (!skillB) {
        return 1;
    }

    SkillLineRec* recA = g_skillLineDB.GetRecord(skillA);
    SkillLineRec* recB = g_skillLineDB.GetRecord(skillB);

    if (!recA || !recB) {
        return 0;
    }

    return SStrSortUTF8I(recA->m_displayName, recB->m_displayName, STORM_MAX_STR);
}

// VERIFY: 10561fff manual
// OFFSET: 0x0053C680
int __cdecl SortSpellUsableInfo(const void* a, const void* b) {
    SpellRec spellA;
    SpellRec spellB;

    if (!g_spellDB.GetRecord(static_cast<const SpellUsableInfo*>(a)->m_spellId, &spellA) || !g_spellDB.GetRecord(static_cast<const SpellUsableInfo*>(b)->m_spellId, &spellB)) {
        return 0;
    }

    if (spellA.m_stanceBarOrder < 0 && spellB.m_stanceBarOrder < 0) {
        return 2 * (spellA.m_id >= spellB.m_id) - 1;
    }

    if (spellA.m_stanceBarOrder == -1) {
        return 1;
    }

    if (spellB.m_stanceBarOrder == -1) {
        return -1;
    }

    if (spellA.m_stanceBarOrder == spellB.m_stanceBarOrder) {
        return 0;
    }

    return 2 * (spellA.m_stanceBarOrder >= spellB.m_stanceBarOrder) - 1;
}

// VERIFY: 19695381 auto
// OFFSET: 0x0053C7C0
int __cdecl SortCompanions(const void* a, const void* b) {
    CreatureStats_C* creatureA = nullptr;
    CreatureStats_C* creatureB = nullptr;
    SpellRec spell;

    if (g_spellDB.GetRecord(*static_cast<const int32_t*>(a), &spell)) {
        WOWGUID requester = WOWGUID();
        creatureA = g_creatureDBCache.GetRecord(spell.m_effectMiscValue[0], requester, nullptr, nullptr, false);
    }

    if (g_spellDB.GetRecord(*static_cast<const int32_t*>(b), &spell)) {
        WOWGUID requester = WOWGUID();
        creatureB = g_creatureDBCache.GetRecord(spell.m_effectMiscValue[0], requester, nullptr, nullptr, false);
    }

    if (creatureA && creatureB) {
        return SStrSortUTF8I(creatureA->m_name[0], creatureB->m_name[0], STORM_MAX_STR);
    }

    return static_cast<int>(creatureB - creatureA);
}

// VERIFY: d0921763 manual
// OFFSET: 0x0053C430
static void MakeSpellVisualEffectNameLocal(int32_t id) {
    if (!id) {
        return;
    }

    SpellVisualEffectNameRec* effect = g_spellVisualEffectNameDB.GetRecord(id);

    if (!effect) {
        return;
    }

    char path[260];
    strcpy(path, effect->m_fileName);
    SStrLower(path);
    char* extension = strrchr(path, '.');

    if (!extension) {
        return;
    }

    if (!strcmp(extension, ".mdl") || !strcmp(extension, ".mdx")) {
        strcpy(extension, ".m2");
    }

    SFile::MakeFileLocal(path, 1, false);

    for (uint32_t i = 0; i < 4; i++) {
        sprintf(extension, "%02d.skin", i);

        if (!SFile::MakeFileLocal(path, 1, false)) {
            break;
        }
    }
}

// VERIFY: a19bd6f1 auto
// OFFSET: 0x0053C520
static void MakeSpellVisualKitLocal(int32_t id) {
    if (!id) {
        return;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(id);

    if (!kit) {
        return;
    }

    MakeSpellVisualEffectNameLocal(kit->m_headEffect);
    MakeSpellVisualEffectNameLocal(kit->m_chestEffect);
    MakeSpellVisualEffectNameLocal(kit->m_baseEffect);
    MakeSpellVisualEffectNameLocal(kit->m_leftHandEffect);
    MakeSpellVisualEffectNameLocal(kit->m_rightHandEffect);
    MakeSpellVisualEffectNameLocal(kit->m_breathEffect);
    MakeSpellVisualEffectNameLocal(kit->m_leftWeaponEffect);
    MakeSpellVisualEffectNameLocal(kit->m_rightWeaponEffect);

    for (uint32_t i = 0; i < 3; i++) {
        MakeSpellVisualEffectNameLocal(kit->m_specialEffect[i]);
    }

    MakeSpellVisualEffectNameLocal(kit->m_worldEffect);
}

// VERIFY: 4e059ff6 auto
// OFFSET: 0x0053E810
static void MakeKnownSpellVisualKitsLocal(int32_t SpellVisualRec::* kit) {
    SpellRec spell;

    for (uint32_t i = 0; i < s_numSpells; i++) {
        if (!g_spellDB.GetRecord(s_spellList[i], &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 2; j++) {
            if (!spell.m_spellVisualID[j]) {
                continue;
            }

            SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell.m_spellVisualID[j]);

            if (visual) {
                MakeSpellVisualKitLocal(visual->*kit);
            }
        }
    }
}

// VERIFY: 1d27c5ca manual
// OFFSET: 0x0053E930
static void MakeKnownSpellMissilesLocal() {
    SpellRec spell;

    for (uint32_t i = 0; i < s_numSpells; i++) {
        if (!g_spellDB.GetRecord(s_spellList[i], &spell)) {
            continue;
        }

        for (uint32_t j = 0; j < 2; j++) {
            if (!spell.m_spellVisualID[j]) {
                continue;
            }

            SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell.m_spellVisualID[j]);

            if (visual && visual->m_hasMissile) {
                MakeSpellVisualEffectNameLocal(visual->m_missileModel);
            }
        }
    }
}

// OFFSET: 0x0053EA40
void CGSpellBook::MakeKnownSpellModelsLocal() {
    MakeKnownSpellMissilesLocal();
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_castKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_stateKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_channelKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_missileTargetingKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_instantAreaKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_persistentAreaKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_impactKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_casterImpactKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_targetImpactKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_impactAreaKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_precastKit);
    MakeKnownSpellVisualKitsLocal(&SpellVisualRec::m_stateDoneKit);
}

// VERIFY: 356de315 auto
// OFFSET: 0x0053EAA0
void PrefetchSpellVisuals(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    for (uint32_t i = 0; i < 2; i++) {
        if (!spell.m_spellVisualID[i]) {
            continue;
        }

        SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell.m_spellVisualID[i]);

        if (!visual) {
            continue;
        }

        if (visual->m_hasMissile) {
            MakeSpellVisualEffectNameLocal(visual->m_missileModel);
        }

        MakeSpellVisualKitLocal(visual->m_castKit);
        MakeSpellVisualKitLocal(visual->m_stateKit);
        MakeSpellVisualKitLocal(visual->m_channelKit);
        MakeSpellVisualKitLocal(visual->m_missileTargetingKit);
        MakeSpellVisualKitLocal(visual->m_instantAreaKit);
        MakeSpellVisualKitLocal(visual->m_persistentAreaKit);
        MakeSpellVisualKitLocal(visual->m_impactKit);
        MakeSpellVisualKitLocal(visual->m_casterImpactKit);
        MakeSpellVisualKitLocal(visual->m_targetImpactKit);
        MakeSpellVisualKitLocal(visual->m_impactAreaKit);
        MakeSpellVisualKitLocal(visual->m_precastKit);
        MakeSpellVisualKitLocal(visual->m_stateDoneKit);
    }
}

// VERIFY: c8864654 auto
// OFFSET: 0x0053EBC0
void CGSpellBook::CompanionCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (s_pendingCompanions <= 0) {
        return;
    }

    int32_t remaining = --s_pendingCompanions;

    if (param && success) {
        WOWGUID requester = WOWGUID();
        CreatureStats_C* creature = g_creatureDBCache.GetRecord(id, requester, nullptr, nullptr, false);
        CGGameUI::DisplayError(ERR_LEARN_COMPANION_S, creature->m_name[0]);
        CGSpellBook::UpdateCompanions();
        FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_LEARNED, nullptr);
    } else if (!remaining) {
        CGSpellBook::UpdateCompanions();
    }
}

// VERIFY: 7adc0202 auto
// OFFSET: 0x0053B4E0
int32_t GetSpellSlot(int32_t spellId, int32_t petSpell) {
    if (!spellId) {
        return -1;
    }

    if (!petSpell) {
        for (uint32_t i = s_numSpells; i;) {
            if (spellId == s_spellList[--i]) {
                return i;
            }
        }

        return -1;
    }

    for (uint32_t i = CGSpellBook::m_numPetSpells; i;) {
        if (spellId == static_cast<int32_t>(s_petSpellList[--i])) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 317d9288 manual
// OFFSET: 0x0053C5F0
static uint32_t GetSpellSkillInfoIndex(int32_t spellId) {
    int32_t skillLine = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        SkillLineAbilityRec* ability = player->LookupAbility(spellId);

        if (ability) {
            const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], ability->m_skillLine);

            if (info && !(info->m_flags & 0x80)) {
                skillLine = info->m_skillID;
            }
        }
    }

    uint32_t i = 0;

    for (; i < s_numSkillInfos; i++) {
        if (s_skillInfos.m_data[i]->m_skillLine == skillLine) {
            break;
        }
    }

    return i;
}

// VERIFY: 1b7ec0ca manual
// OFFSET: 0x0053C8B0
int __cdecl SortSpellsForDisplay(const void* a, const void* b) {
    SpellRec spellA;
    SpellRec spellB;

    if (!g_spellDB.GetRecord(*static_cast<const int32_t*>(a), &spellA) || !g_spellDB.GetRecord(*static_cast<const int32_t*>(b), &spellB)) {
        return 0;
    }

    uint32_t tabA = GetSpellSkillInfoIndex(spellA.m_id);
    uint32_t tabB = GetSpellSkillInfoIndex(spellB.m_id);

    if (tabA != tabB) {
        return tabA < tabB ? -1 : 1;
    }

    int result = SStrSortUTF8I(spellA.m_name, spellB.m_name, STORM_MAX_STR);

    if (result) {
        return result;
    }

    uint32_t rankA = CGSpellBook::GetSpellProgressionRank(&spellA);

    if (!rankA) {
        rankA = spellA.m_spellLevel;
    }

    uint32_t rankB = CGSpellBook::GetSpellProgressionRank(&spellB);

    if (!rankB) {
        rankB = spellB.m_spellLevel;
    }

    if (!rankA || !rankB) {
        return SStrSortUTF8I(spellA.m_nameSubtext, spellB.m_nameSubtext, STORM_MAX_STR);
    }

    if (rankA < rankB) {
        return -1;
    }

    return rankA > rankB ? 1 : 0;
}

// VERIFY: c77f786b manual
// OFFSET: 0x00541DF0
void CGSpellBook::AddSpellToNameCache(int32_t spellId, int32_t isPet) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    SPELLSBYNAME* entry = s_spellsByName.Ptr(spell.m_name);

    if (!entry) {
        entry = s_spellsByName.New(spell.m_name, 0, 0);
    }

    uint32_t rank = CGSpellBook::GetSpellProgressionRank(&spell);

    if (!rank) {
        rank = spell.m_spellLevel;
    }

    SPELLRANKNODE* next = nullptr;

    for (SPELLRANKNODE* node = entry->m_spells.Head(); node; node = node->m_link.Next()) {
        if (node->m_spellId == spellId) {
            return;
        }

        if (rank > node->m_rank && !isPet) {
            next = node;
            break;
        }
    }

    SPELLRANKNODE* node = new (SMemAlloc(sizeof(SPELLRANKNODE), __FILE__, __LINE__, 0)) SPELLRANKNODE;
    node->m_spellId = spellId;
    node->m_petSpell = isPet;

    if (*spell.m_nameSubtext) {
        node->m_subtext.Copy(spell.m_nameSubtext);
    }

    node->m_rank = rank;

    if (next) {
        entry->m_spells.LinkNode(node, 2, next);
    } else {
        entry->m_spells.LinkToTail(node);
    }

    CGItem_C::RemoveNameFromCache(spell.m_name);
}

// VERIFY: 766957c7 auto
// OFFSET: 0x0053B480
void CGSpellBook::UpdateSelection() {
    CGActionBar::UpdateSelection();
    FrameScript_SignalEvent(SCRIPT_EVENT_CURRENT_SPELL_CAST_CHANGED, nullptr);
}

// VERIFY: e14c5005 manual
// OFFSET: 0x0053B410
int32_t CGSpellBook::GetCompanionType(const SpellRec* spell) {
    if (spell->m_effect[0] == 28) {
        return 0;
    }

    return (spell->m_effectAura[0] != 78) + 1;
}

// VERIFY: d8053ee4 manual
// OFFSET: 0x0053BCE0
int32_t CGSpellBook::GetSkinningSpell(const CGObject_C* object) {
    uint32_t type = object->m_obj->m_type;

    if (type & 0x90) {
        return s_skinPlayerCorpseSpell;
    }

    if (type & 0x8) {
        return s_skinningSpells[static_cast<const CGUnit_C*>(object)->GetSkinningType()];
    }

    return 0;
}

// VERIFY: 645ce4bf auto
// OFFSET: 0x0053D990
int32_t CGSpellBook::FindCompanionSlotByID(int32_t spellId, int32_t type) {
    if (!spellId) {
        return -1;
    }

    for (int32_t slot = CGSpellBook::m_companions[type].Count(); slot > 0;) {
        slot--;

        if (spellId == CGSpellBook::m_companions[type].m_data[slot]) {
            return slot;
        }
    }

    return -1;
}

// VERIFY: 67b33918 auto
// OFFSET: 0x0053B900
bool HasPetSpell(int32_t spellId) {
    if (static_cast<uint32_t>(spellId) > static_cast<uint32_t>(g_spellDB.m_maxID)) {
        return false;
    }

    for (uint32_t i = 0; i < 1024; i++) {
        if (s_petSpellList[i] == static_cast<uint32_t>(spellId)) {
            return true;
        }
    }

    return false;
}

// VERIFY: c806ca2e auto
// OFFSET: 0x0053F5E0
int32_t CGSpellBook::FindSpellByName(const char* name, const char* subtext, int32_t* petSpell) {
    if (!name || !*name) {
        return 0;
    }

    SPELLSBYNAME* entry = s_spellsByName.Ptr(name);

    if (!entry) {
        return 0;
    }

    SPELLRANKNODE* node = entry->m_spells.Head();

    if (subtext && *subtext) {
        for (; node; node = node->m_link.Next()) {
            if (node->m_subtext.m_str && !SStrCmpUTF8I(node->m_subtext.m_str, subtext, STORM_MAX_STR)) {
                break;
            }
        }
    }

    if (!node) {
        return 0;
    }

    *petSpell = node->m_petSpell;
    return node->m_spellId;
}

// VERIFY: 586da1f3 auto
// OFFSET: 0x005401D0
int32_t FindSpellSlotByName(const char* name, const char* subtext, int32_t* petSpell) {
    return GetSpellSlot(CGSpellBook::FindSpellByName(name, subtext, petSpell), *petSpell);
}

// VERIFY: efee71cd auto
// OFFSET: 0x00540200
int32_t GetSpellIdByName(const char* name, int32_t* petSpell) {
    if (!name || !*name) {
        return 0;
    }

    char spellName[1024];
    char subtext[512];
    spellName[0] = '\0';
    subtext[0] = '\0';

    if (*name == '!') {
        name++;
    }

    SStrCopy(spellName, name, sizeof(spellName));
    int32_t spellId = CGSpellBook::FindSpellByName(spellName, subtext, petSpell);

    if (spellId) {
        return spellId;
    }

    char* paren = SStrChrR(spellName, '(');

    if (paren) {
        *paren = '\0';
        SStrCopy(subtext, paren + 1, sizeof(subtext));
    } else {
        subtext[0] = '\0';
    }

    char* current = subtext;
    int32_t chars;
    uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars);

    while (c != static_cast<uint32_t>(-1)) {
        if (c == ')') {
            *current = '\0';
            break;
        }

        current += chars;
        c = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars);
    }

    return CGSpellBook::FindSpellByName(spellName, subtext, petSpell);
}

// VERIFY: 6dbee0a0 manual
// OFFSET: 0x00540670
bool GetSlotFromLua(lua_State* L, uint32_t* slot, int32_t* petSpell, int32_t* type, int32_t allowName) {
    *petSpell = 0;

    if (lua_isnumber(L, 1) && lua_isstring(L, 2)) {
        *slot = static_cast<uint32_t>(lua_tonumber(L, 1) - 1.0);

        if (*slot <= 1023) {
            if (!SStrCmpI("pet", lua_tolstring(L, 2, nullptr), STORM_MAX_STR)) {
                *petSpell = 1;
            }

            if (type) {
                *type = 3;
            }

            return true;
        }
    } else if (allowName && lua_isstring(L, 1)) {
        WOWGUID guid;

        if (!lua_isstring(L, 2) || Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), guid, false)) {
            *slot = GetSpellSlot(GetSpellIdByName(lua_tolstring(L, 1, nullptr), petSpell), *petSpell);

            if (type) {
                *type = 2;
            }
        } else {
            *slot = FindSpellSlotByName(lua_tolstring(L, 1, nullptr), lua_tolstring(L, 2, nullptr), petSpell);

            if (type) {
                *type = 3;
            }
        }

        return !(*slot & 0x80000000);
    }

    char function[1024];
    luaL_error(L, "%s(): Invalid spell slot", FrameScript_GetCurrentFunction(L, function, sizeof(function)));
    return false;
}

// VERIFY: 50e56750 manual
// OFFSET: 0x0053DC20
const char* CGSpellBook::GetSpellTexture(int32_t slot, int32_t petSpell) {
    SpellRec spell;
    int32_t spellId = slot < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (static_cast<uint32_t>(slot) < s_numSpells ? s_spellList[slot] : 0);

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return nullptr;
    }

    if (spell.m_effect[0] == 78) {
        return CGActionBar::GetAttackTexture();
    }

    if ((spell.m_attributes & 0x2) && (spell.m_attributesExB & 0x20)) {
        const char* texture = CGActionBar::GetRangedTexture();

        if (texture) {
            return texture;
        }
    }

    SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_spellIconID);

    if (!icon) {
        return nullptr;
    }

    return icon->m_textureFilename;
}

// VERIFY: aa6df244 manual
// OFFSET: 0x0053D350
int32_t CGSpellBook::IsSelectedSlot(int32_t slot, int32_t petSpell) {
    SpellRec spell;
    int32_t spellId = slot < 0 ? 0 : petSpell ? (slot < 1024 ? s_petSpellList[slot] : 0) : (static_cast<uint32_t>(slot) < s_numSpells ? s_spellList[slot] : 0);

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player && spell.m_effect[0] == 47) {
        SkillLineAbilityRec* ability = player->LookupAbility(spell.m_id);

        if (ability && !CGTradeSkillInfo::m_linkData && CGTradeSkillInfo::m_skillLineID == ability->m_skillLine) {
            return 1;
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effectAura[i] != 36) {
            continue;
        }

        int32_t form = spell.m_effectMiscValue[i];

        if (form && player && static_cast<int32_t>(player->GetShapeshiftForm()) == form) {
            return 1;
        }

        break;
    }

    return 0;
}

// VERIFY: 6fab80de manual
// OFFSET: 0x0053B4A0
int32_t GetSpellIdBySlot(int32_t slot, int32_t petSpell) {
    if (slot < 0) {
        return 0;
    }

    if (petSpell) {
        return slot < 1024 ? s_petSpellList[slot] : 0;
    }

    return static_cast<uint32_t>(slot) < s_numSpells ? s_spellList[slot] : 0;
}

// VERIFY: 008f2f37 manual
// OFFSET: 0x0053B930
bool HasSpell(int32_t spellId, int32_t petSpell) {
    if (GetSpellSlot(spellId, petSpell) >= 0) {
        return true;
    }

    if (petSpell) {
        return CGPetInfo::FindSpellByID(spellId) != nullptr;
    }

    return false;
}

// VERIFY: 346d7716 auto
// OFFSET: 0x0053BAC0
void CGSpellBook::UpdateCooldowns() {
    CGActionBar::SignalUpdateCooldown();

    if (s_shapeshiftSpells.Count()) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_COOLDOWN, nullptr);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_SPELL_UPDATE_COOLDOWN, nullptr);
}

// VERIFY: 47f87173 auto
// OFFSET: 0x0053BD10
int32_t CGSpellBook::IsTrackingSpell(int32_t spellId) {
    for (uint32_t i = s_trackingSpells.Count(); i; ) {
        i--;

        if (spellId == s_trackingSpells.m_data[i]) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 5e582599 auto
// OFFSET: 0x0053B3F0
void ClearPetSpellInfo() {
    memset(s_petSpellList, 0, sizeof(s_petSpellList));
    CGSpellBook::m_numPetSpells = 0;
}

// VERIFY: 0870e37a auto
// OFFSET: 0x0053B440
void CGSpellBook::SendCompanionUpdate(int32_t type) {
    if (type == 2) {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_UPDATE, nullptr);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_UPDATE, "%s", s_companionTypes[type]);
    }
}

// VERIFY: 99fbc6c2 auto
// OFFSET: 0x0053B5A0
int32_t CGSpellBook::GetSpellRankOrLevel(const SpellRec* spell) {
    int32_t rank = CGSpellBook::GetSpellProgressionRank(spell);

    if (!rank) {
        return spell->m_spellLevel;
    }

    return rank;
}

// VERIFY: 97097f8a manual
// OFFSET: 0x0053BAF0
void CGSpellBook::PickupSpell(int32_t slot, int32_t petSpell) {
    if (!CGGameUI::CanPerformAction(14)) {
        return;
    }

    int32_t spellId;

    if (slot < 0) {
        spellId = 0;
    } else if (petSpell) {
        spellId = slot < 1024 ? s_petSpellList[slot] : 0;
    } else {
        spellId = static_cast<uint32_t>(slot) < s_numSpells ? s_spellList[slot] : 0;
    }

    int32_t cursorSpell = CGGameUI::GetCursorSpell();
    CGGameUI::ClearCursor(1, 1);

    if (spellId == cursorSpell) {
        if (spellId) {
            SI2::PlaySoundKit("igSpellBookSpellIconDrop", nullptr, nullptr, nullptr);
        }
    } else if (cursorSpell <= 0 && spellId) {
        if (petSpell) {
            CGGameUI::SetCursorPetAction(CGPetInfo::GetPetSpellAction(spellId));
        } else {
            CGGameUI::SetCursorSpell(spellId);
        }

        SI2::PlaySoundKit("igSpellBookSpellIconPickup", nullptr, nullptr, nullptr);
    }
}

// VERIFY: e86b1382 auto
// OFFSET: 0x0053BBC0
void CGSpellBook::PickupCompanion(int32_t type, uint32_t index) {
    if (!CGGameUI::CanPerformAction(14) || index >= CGSpellBook::m_companions[type].m_count) {
        return;
    }

    int32_t spellId = CGSpellBook::m_companions[type].m_data[index];
    int32_t cursorSpell = CGGameUI::GetCursorSpell();
    CGGameUI::ClearCursor(1, 1);

    if (spellId == cursorSpell) {
        SI2::PlaySoundKit("igSpellBookSpellIconDrop", nullptr, nullptr, nullptr);
    } else if (cursorSpell <= 0) {
        CGGameUI::SetCursorSpell(spellId);
        SI2::PlaySoundKit("igSpellBookSpellIconPickup", nullptr, nullptr, nullptr);
    }
}

// VERIFY: 95549e84 manual
// OFFSET: 0x0053BC40
void CGSpellBook::CastSpell(int32_t slot, int32_t petSpell, WOWGUID target) {
    if (CGGameUI::GetCursorSpell() > 0) {
        CGSpellBook::PickupSpell(slot, petSpell);
        return;
    }

    CGGameUI::ClearCursor(1, 1);

    int32_t spellId;

    if (slot < 0) {
        spellId = 0;
    } else if (petSpell) {
        spellId = slot < 1024 ? s_petSpellList[slot] : 0;
    } else {
        spellId = static_cast<uint32_t>(slot) < s_numSpells ? s_spellList[slot] : 0;
    }

    if (!petSpell) {
        Spell_C_ClickSpell(spellId, nullptr, target);
        return;
    }

    PetAction action;
    action.m_data = spellId | 0x1000000;
    CGPetInfo::SendPetAction(action, target, 0);
}

// VERIFY: cb3b2a4b manual
// OFFSET: 0x0053BD70
int32_t CGSpellBook::IsAuraCaster(WOWGUID guid, const WOWGUID* caster) {
    if (!guid.m_guid || !caster || !caster->m_guid) {
        return 0;
    }

    if (guid == *caster) {
        return 1;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*caster, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    const WOWGUID& pet = unit->m_unit->m_unitFieldCharm.m_guid ? unit->m_unit->m_unitFieldCharm : unit->m_unit->m_unitFieldSummon;

    return guid == pet || guid == *unit->GetCharmerOrCreatorGuid();
}

// VERIFY: 586419fd auto
// OFFSET: 0x0053BDF0
int32_t CGSpellBook::FindRaidDispel(int32_t dispelType) {
    for (uint32_t i = 0; i < s_dispelTypes.m_count; i++) {
        if (s_dispelTypes.m_data[i] == dispelType) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 53ad8165 auto
// OFFSET: 0x0053BE30
bool CGSpellBook::IsUnlearnedSpell(int32_t spellId) {
    for (uint32_t i = 0; i < CGSpellBook::m_unlearnedSpells.m_count; i++) {
        if (CGSpellBook::m_unlearnedSpells.m_data[i] == spellId) {
            return true;
        }
    }

    return false;
}

// VERIFY: 64002cc0 manual
// OFFSET: 0x0053C5B0
bool HasSpell(uint32_t spellId) {
    return spellId <= static_cast<uint32_t>(g_spellDB.m_maxID) && (s_knownSpellBits.m_bits.m_data[spellId >> 5] & (1 << (spellId & 0x1F)));
}

// VERIFY: 230462f8 manual
// OFFSET: 0x0053F550
void CGSpellBook::DelSpellFromNameCache(int32_t spellId, int32_t petSpell) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    SPELLSBYNAME* entry = s_spellsByName.Ptr(spell.m_name);

    if (!entry) {
        return;
    }

    for (SPELLRANKNODE* node = entry->m_spells.Head(); node; node = node->m_link.Next()) {
        if (node->m_spellId == spellId && node->m_petSpell == petSpell) {
            entry->m_spells.DeleteNode(node);
            break;
        }
    }
}

// VERIFY: e855f5ff auto
// OFFSET: 0x00540650
int32_t IsValidSpell(const char* name, int32_t* petSpell) {
    return GetSpellSlot(GetSpellIdByName(name, petSpell), *petSpell);
}

// VERIFY: 5b1aa4c6 auto
// OFFSET: 0x00542EB0
void CGSpellBook::ReplaceSpell(int32_t oldSpell, int32_t newSpell) {
    CGSpellBook::DelKnownSpell(oldSpell, 1);
    CGSpellBook::AddKnownSpell(newSpell, 1, oldSpell);
}

// VERIFY: bd1e6229 auto
// OFFSET: 0x00542EE0
void CGSpellBook::AddPetSpell(int32_t spellId) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && !(spell.m_attributes & 0x80) && !(spell.m_attributesExD & 0x8000)) {
        CGSpellBook::AddSpellToNameCache(spellId, 1);
        s_petSpellList[CGSpellBook::m_numPetSpells++] = spellId;
    }
}

// VERIFY: 415a64e8 manual
// OFFSET: 0x0053D1B0
void CGSpellBook::UpdateUsablePower() {
    CGActionBar::UpdateUsablePower();

    CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    SpellRec spell;

    for (uint32_t i = 0; i < s_knownSpells.m_count; i++) {
        SpellUsableInfo* info = &s_knownSpells.m_data[i];

        if (!info->m_usable) {
            continue;
        }

        g_spellDB.GetRecord(info->m_spellId, &spell);
        bool havePower = Spell_C_HaveSpellPower(player, &spell);

        if (!havePower != info->m_noMana) {
            if (havePower) {
                SendToCombatText(35, spell.m_name);
            }

            info->m_noMana = !havePower;
        }
    }

    int32_t changed = 0;

    for (uint32_t i = 0; i < s_shapeshiftSpells.m_count; i++) {
        SpellUsableInfo* info = &s_shapeshiftSpells.m_data[i];

        if (!info->m_usable) {
            continue;
        }

        g_spellDB.GetRecord(info->m_spellId, &spell);
        uint8_t noMana = !Spell_C_HaveSpellPower(player, &spell);

        if (noMana != info->m_noMana) {
            changed = 1;
            info->m_noMana = noMana;
        }
    }

    if (s_shapeshiftSpells.m_count && changed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_SHAPESHIFT_USABLE, nullptr);
    }
}

// VERIFY: 29941820 manual
// OFFSET: 0x0053D4B0
uint32_t CGSpellBook::GetShapeshiftForm(int32_t requireForm) {
    CGUnit_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || (requireForm && !player->GetShapeshiftForm())) {
        return 0;
    }

    SpellRec spell;
    uint32_t form = s_shapeshiftSpells.m_count;

    for (; form; form--) {
        if (g_spellDB.GetRecord(s_shapeshiftSpells.m_data[form - 1].m_spellId, &spell) && Spell_C_IsActiveSpell(&spell, player)) {
            break;
        }
    }

    if (!form && player->GetShapeshiftForm()) {
        form = s_shapeshiftSpells.m_count + 1;
    }

    return form;
}

// VERIFY: cfe23c3b manual
// OFFSET: 0x0053D580
int32_t CGSpellBook::CanShowAura(const SpellRec* spell, const WOWGUID* caster, bool helpful) {
    if ((spell->m_attributes & 0x80) || (spell->m_attributesEx & 0x10000000)) {
        return 0;
    }

    if (spell->m_attributes & 0x40) {
        uint32_t i = 0;

        while (!Spell_IsAreaAuraEffect(spell->m_effect[i])) {
            if (++i >= 3) {
                return 0;
            }
        }
    }

    bool hidden = helpful ? (spell->m_attributesExE & 0x8000000) != 0 : (spell->m_attributesExE & 0x10000000) != 0;

    if (hidden) {
        return 0;
    }

    if ((spell->m_attributesExF & 0x100000) && !(caster->m_guid && (*caster == ClntObjMgrGetActivePlayer() || *caster == CGPetInfo::GetPet(0)))) {
        return 0;
    }

    if (!spell->m_requiredAuraVision) {
        return 1;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->HasAuraVision(spell->m_requiredAuraVision)) {
        return 1;
    }

    return 0;
}

// VERIFY: ce8605b3 manual
// OFFSET: 0x0053D750
int32_t CGSpellBook::CanApplyAura(const AuraData* aura) {
    if (!aura) {
        return 0;
    }

    int32_t spellId = aura->m_spellId;

    if (HasSpell(static_cast<uint32_t>(spellId)) || HasPetSpell(spellId)) {
        SpellRec spell;

        if (!g_spellDB.GetRecord(spellId, &spell)) {
            return 0;
        }

        for (uint32_t i = 0; i < 3; i++) {
            if (Spell_IsAuraEffect(spell.m_effect[i])) {
                return 1;
            }
        }
    }

    if (!aura->m_creator.m_guid) {
        return 0;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(aura->m_creator, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit || !unit->m_creatureCacheEntry) {
        return 0;
    }

    const WOWGUID& owner = unit->m_unit->m_unitFieldCharmedby.m_guid ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldCreatedby;

    if (!owner.m_guid) {
        return 0;
    }

    uint32_t entry = unit->m_obj->m_entryID;

    for (uint32_t i = 0; i < s_summonCreatures.m_count; i++) {
        if (entry == static_cast<uint32_t>(s_summonCreatures.m_data[i])) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 51d10eb7 manual
// OFFSET: 0x0053D8A0
int32_t CGSpellBook::FindRaidBuff(int32_t spellId) {
    if (!HasSpell(static_cast<uint32_t>(spellId)) && !HasPetSpell(spellId)) {
        return 0;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributesExC & 0x4000) || spell.m_implicitTargetA[0] == 1) {
        return 0;
    }

    for (uint32_t i = 0; i < 3; i++) {
        int32_t effect = spell.m_effect[i];

        if (effect == 35 || effect == 65 || effect == 119 || effect == 143 || effect == 128 || effect == 129 || effect == 27) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 66b38b26 manual
// OFFSET: 0x0053D9D0
void CGSpellBook::GetAppropriateSpellRank(uint32_t* spellId, CGUnit_C* caster, CGUnit_C* target) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(*spellId, &spell) || (spell.m_attributes & 0x4000000) || (spell.m_attributesExB & 0x8)) {
        return;
    }

    uint32_t i = 0;

    while (!spell.m_effectAura[i]) {
        if (++i >= 3) {
            return;
        }
    }

    if (!caster) {
        return;
    }

    if (!caster->IsActivePlayer() && caster->m_obj->m_guid != CGPetInfo::GetPet(0)) {
        return;
    }

    if (!target || !caster->CanAssist(target, false)) {
        return;
    }

    uint32_t level = target->m_unit->m_unitFieldLevel;

    if (target->m_unit->m_unitFieldSummonedby.m_guid) {
        CGUnit_C* summoner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target->m_unit->m_unitFieldSummonedby, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (summoner) {
            level = summoner->m_unit->m_unitFieldLevel;
        }
    }

    int32_t maxLevel = level + 10;

    if (spell.m_spellLevel <= maxLevel) {
        return;
    }

    int32_t bestLevel = 0;
    SpellRec other;

    if (!caster->IsActivePlayer()) {
        for (uint32_t slot = CGSpellBook::m_numPetSpells; slot; ) {
            slot--;

            if (g_spellDB.GetRecord(s_petSpellList[slot], &other) && !SStrCmp(spell.m_name, other.m_name, STORM_MAX_STR)) {
                if (other.m_spellLevel <= maxLevel && other.m_spellLevel > bestLevel) {
                    *spellId = other.m_id;
                    bestLevel = other.m_spellLevel;
                }
            }
        }

        return;
    }

    for (uint32_t slot = s_numSpells; slot; ) {
        slot--;

        if (g_spellDB.GetRecord(s_spellList[slot], &other) && !SStrCmp(spell.m_name, other.m_name, STORM_MAX_STR)) {
            if (other.m_spellLevel <= maxLevel && other.m_spellLevel > bestLevel) {
                *spellId = other.m_id;
                bestLevel = other.m_spellLevel;
            }
        }
    }
}

// VERIFY: a1eaf271 manual
// OFFSET: 0x0053D680
int32_t CGSpellBook::CanStealAura(const CGUnit_C* unit, const AuraData* aura, const SpellRec* spell) {
    if (!s_dispelMask) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->CanAttack(unit) || (aura->m_flags & 0x80)) {
        return 0;
    }

    if (spell->m_attributes & 0x40) {
        return 0;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (((1 << i) & aura->m_flags) && Spell_IsAreaAuraEffect(spell->m_effect[i])) {
            return 0;
        }
    }

    if ((spell->m_attributesEx & 0x44) || (spell->m_attributesExD & 0x40)) {
        return 0;
    }

    return (s_dispelMask & (1 << spell->m_dispelType)) != 0;
}

// VERIFY: 88e8003e auto
// OFFSET: 0x0053BD40
int32_t CGSpellBook::IsShapeshiftSpell(int32_t spellId) {
    for (uint32_t i = s_shapeshiftSpells.m_count; i; ) {
        i--;

        if (spellId == s_shapeshiftSpells.m_data[i].m_spellId) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: f3047ba9 manual
// OFFSET: 0x0053FAD0
void CGSpellBook::DelKnownSpell(int32_t spellId, int32_t notify) {
    s_knownSpellBits.m_bits.m_data[spellId >> 5] &= ~(1 << (spellId & 0x1F));

    if (spellId == CGSpellBook::m_duelSpell) {
        CGSpellBook::m_duelSpell = 0;
    }

    if (spellId == CGSpellBook::m_stuckSpell) {
        CGSpellBook::m_stuckSpell = 0;
    }

    if (spellId == CGSpellBook::m_summonFriendSpell) {
        CGSpellBook::m_summonFriendSpell = 0;
    }

    if (spellId == CGSpellBook::m_autoRangedSpell) {
        CGSpellBook::m_autoRangedSpell = 0;
    }

    for (uint32_t i = 0; i < 4; i++) {
        if (spellId == s_skinningSpells[i]) {
            s_skinningSpells[i] = 0;
        }
    }

    if (spellId == s_skinPlayerCorpseSpell) {
        s_skinPlayerCorpseSpell = 0;
    }

    CGSpellBook::DelSpellFromNameCache(spellId, 0);

    int32_t removed = 0;

    for (uint32_t i = 0; i < 1024; i++) {
        if (s_spellList[i] != spellId) {
            continue;
        }

        uint32_t last = s_numSpells - 1;

        if (i < last) {
            memcpy(&s_spellList[i], &s_spellList[i + 1], sizeof(s_spellList[0]) * (s_numSpells - i) - sizeof(s_spellList[0]));
        }

        s_numSpells = last;
        s_spellList[last] = 0;
        removed = 1;
        break;
    }

    for (uint32_t i = CGSpellBook::m_languageSpells.m_count; i; ) {
        i--;

        if (CGSpellBook::m_languageSpells.m_data[i] == spellId) {
            CGSpellBook::m_languageSpells.m_data[i] = 0;
        }
    }

    for (uint32_t i = s_trackingSpells.m_count; i; ) {
        i--;

        if (s_trackingSpells.m_data[i] == spellId) {
            uint32_t last = s_trackingSpells.m_count - 1;

            if (i < last) {
                memcpy(&s_trackingSpells.m_data[i], &s_trackingSpells.m_data[i + 1], sizeof(int32_t) * (last - i));
            }

            s_trackingSpells.SetCount(last);
            break;
        }
    }

    for (uint32_t i = s_openLockSpells.m_count; i; ) {
        i--;

        if (s_openLockSpells.m_data[i] == spellId) {
            uint32_t last = s_openLockSpells.m_count - 1;

            if (i < last) {
                memcpy(&s_openLockSpells.m_data[i], &s_openLockSpells.m_data[i + 1], sizeof(int32_t) * (last - i));
            }

            s_openLockSpells.SetCount(last);
            break;
        }
    }

    if (!notify) {
        for (uint32_t i = s_shapeshiftSpells.m_count; i; ) {
            i--;

            if (s_shapeshiftSpells.m_data[i].m_spellId == spellId) {
                uint32_t last = s_shapeshiftSpells.m_count - 1;

                if (i < last) {
                    memcpy(&s_shapeshiftSpells.m_data[i], &s_shapeshiftSpells.m_data[i + 1], sizeof(SpellUsableInfo) * (last - i));
                }

                s_shapeshiftSpells.SetCount(last);
                Script_SendUniqueSignal(SCRIPT_EVENT_UPDATE_SHAPESHIFT_FORMS);
                break;
            }
        }
    }

    bool totemsEmptied = false;

    for (uint32_t t = 0; t < 4; t++) {
        TSGrowableArray<int32_t>& totems = CGSpellBook::m_totemSpells[t];

        for (uint32_t i = totems.m_count; i; ) {
            i--;

            if (totems.m_data[i] == spellId) {
                uint32_t last = totems.m_count - 1;

                if (i < last) {
                    memcpy(&totems.m_data[i], &totems.m_data[i + 1], sizeof(int32_t) * (last - i));
                }

                totems.SetCount(last);

                if (!last) {
                    totemsEmptied = true;
                }

                break;
            }
        }
    }

    uint32_t multiCast;

    for (multiCast = 0; multiCast < 4; multiCast++) {
        if (spellId == s_multiCastSpells[multiCast]) {
            break;
        }
    }

    if (multiCast < 4 || totemsEmptied) {
        CGActionBar::UpdateMultiCastBar();
    }

    if (removed) {
        int32_t skillLine = 0;
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player) {
            SkillLineAbilityRec* ability = player->LookupAbility(spellId);

            if (ability) {
                const SkillRaceClassInfoRec* info = SpellTableLookupInfo(player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], ability->m_skillLine);

                if (info && info->m_flags >= 0) {
                    skillLine = info->m_skillID;
                }
            }
        }

        for (uint32_t i = 0; i < s_numSkillInfos; i++) {
            SpellBookSkillInfo* skill = s_skillInfos.m_data[i];

            if (skill->m_skillLine != skillLine) {
                continue;
            }

            skill->m_count--;

            if (!s_skillInfos.m_data[i]->m_count && i < --s_numSkillInfos) {
                for (; i < s_numSkillInfos; i++) {
                    *s_skillInfos.m_data[i] = *s_skillInfos.m_data[i + 1];
                }
            }

            break;
        }
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && SpellRec::WillSummonCritter(&spell)) {
        uint32_t type = 0;

        if (spell.m_effect[0] != 28) {
            type = (spell.m_effectAura[0] != 78) + 1;
        }

        TSGrowableArray<int32_t>& companions = (&CGSpellBook::m_companions[0])[type];
        int32_t count = companions.m_count;
        bool found = false;

        for (int32_t i = 0; i < count; i++) {
            if (companions.m_data[i] == spellId) {
                found = true;
            } else if (!found) {
                continue;
            }

            companions.m_data[i] = i == count - 1 ? 0 : companions.m_data[i + 1];
        }

        if (found) {
            companions.SetCount(count - 1);
            CGSpellBook::UpdateCompanions();
            FrameScript_SignalEvent(SCRIPT_EVENT_COMPANION_UNLEARNED, nullptr);
        }
    }

    CGSpellBook::UpdateSpells(removed, 0, 1);
}

// VERIFY: 1789a24e manual
// OFFSET: 0x0053F290
void CGSpellBook::ShutdownGame() {
    for (uint32_t i = 0; i < s_skillInfos.m_count; i++) {
        delete s_skillInfos.m_data[i];
    }

    s_skillInfos.Clear();
    s_numSkillInfos = 0;
    s_knownSpellBits.m_count = 0;

    if (s_knownSpellBits.m_bits.m_count) {
        s_knownSpellBits.m_bits.Clear();
    }

    CGSpellBook::m_languageSpells.Clear();
    s_spellsByName.Clear();
}
