#include "Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/OverrideSpellDataRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellDifficultyRec.h"
#include "WoW/Source/DB/Rec/SpellShapeshiftFormRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/RuneInfo.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Os/TimeManager.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Game/ViolenceLevels.h"
#include "WoW/Source/DB/Rec/SpellMissileRec.h"
#include "WoW/Source/Object/ObjectClient/UnitCombat_C.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/Missile_C.h"
#include "WoW/Source/DB/Rec/SpellCategoryRec.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include <cmath>
#include "WoW/Source/DB/Rec/LockRec.h"
#include "WoW/Source/DB/Rec/SpellCastTimesRec.h"
#include "WoW/Source/DB/Rec/SpellDurationRec.h"
#include "WoW/Source/DB/Rec/SpellRangeRec.h"
#include "WoW/Source/DB/Rec/SpellRuneCostRec.h"
#include "WoW/Source/DB/Rec/SummonPropertiesRec.h"
#include "WoW/Source/Object/GameObject.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/Missile_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/Corpse.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/DB/Rec/SpellRadiusRec.h"
#include "WoW/Source/Magic/Spell.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/DB/Rec/SkillLineAbilityRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassMaskRec.h"
#include "WoW/Source/DB/Rec/PowerDisplayRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/SpellFocusObjectRec.h"
#include "WoW/Source/DB/Rec/SpellMechanicRec.h"
#include "WoW/Source/DB/Rec/ItemLimitCategoryRec.h"
#include "WoW/Source/DB/Rec/TotemCategoryRec.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/DB/Rec/GameObjectDisplayInfoRec.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/Object/GameObjectStats.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "WoW/Source/DB/Rec/GlyphPropertiesRec.h"
#include "WoW/Source/DB/Rec/GlyphSlotRec.h"
#if defined(_MSC_VER)
#include <malloc.h>
#else
#include <alloca.h>
#endif

// STATIC: 0x00D3F5AC
static SpellHistory s_spellHistory[2];

// STATIC: 0x00D397C8
static int32_t s_lastAutoRangedCombatError;

// STATIC: 0x00D397CC
static int32_t s_autoRangedCombatSpell;

// STATIC: 0x00D397D0
static int32_t s_autoRepeatingSpell;

// STATIC: 0x00D3F4E0
uint32_t s_needTargets;

// STATIC: 0x00D3F4E4
PendingSpellCast* s_targetingSpell;

// STATIC: 0x00D3F4E8
static PendingSpellCast* s_delayedTrajectoryCast;

// STATIC: 0x00D397B0
static CVar* s_enableErrorSpeechCVar;

// STATIC: 0x00D397B4
static int32_t s_spellClassSet;

// STATIC: 0x00DCE234
static const CSoftFloat s_softPercent = { 0x3C23D70A };

// STATIC: 0x00D397B8
static uint32_t s_feedPetSpell;

// STATIC: 0x00D397C4
static uint32_t s_lastFailedSpellId;

// STATIC: 0x00D397C0
static int32_t s_lastFailedReason;

// STATIC: 0x00D397BC
static uint32_t s_lastFailedTime;

// STATIC: 0x00A439C8
static const UIErrors s_powerErrors[] = { ERR_OUT_OF_MANA, ERR_OUT_OF_RAGE, ERR_OUT_OF_FOCUS, ERR_OUT_OF_ENERGY, ERR_NOT_HAPPY_ENOUGH, ERR_OUT_OF_RUNES, ERR_OUT_OF_RUNIC_POWER };

// STATIC: 0x00D397D8
static int32_t s_spellPctModifiers[96][31];

// STATIC: 0x00D3C658
static int32_t s_spellFlatModifiers[96][31];

// STATIC: 0x00AF524C
static TSList<PendingSpellCast, TSGetLink<PendingSpellCast> > s_pendingSpellCasts;

// STATIC: 0x00AF5258
static TSList<PendingSpellCast, TSGetLink<PendingSpellCast> > s_freePendingSpellCasts;

// STATIC: 0x00D397D5
static uint8_t s_nextCastCount;

// STATIC: 0x00D3F570
static CMapEntity* s_spellWorldObject;

// STATIC: 0x00D3F574
static CM2Model* s_spellWorldModel;

// STATIC: 0x00AF5270
static TSList<SpellInTransit, TSGetLink<SpellInTransit>> s_spellsInTransit;

// STATIC: 0x00D3F608
static uint32_t s_nextHistoryCleanup;

// STATIC: 0x00AF5220
static uint32_t s_powerDisplayMod[NUM_POWER_TYPES] = { 1, 10, 1, 1, 1000, 1, 10 };

// STATIC: 0x00D397D4
static uint8_t s_cursorWorldObjectHousing;

// STATIC: 0x00D3F4DC
static float s_cursorWorldObjectFacing;

// STATIC: 0x00D3F578
static C3Vector s_cursorWorldObjectPosition;

// STATIC: 0x00D3F4D8
static float s_cursorWorldObjectScale;

// STATIC: 0x00D3F4F0
static char s_targetString[128];

// STATIC: 0x00AF53C4
static uint8_t s_meleeTutorialActive;

// STATIC: 0x00D3F604
static uint32_t s_meleeTutorialCount;

// STATIC: 0x00D3F584
static TSHashTable<ITEMCOOLDOWNHASHNODE, HASHKEY_NONE> s_itemCooldowns;

// STATIC: 0x00AF5264
static TSList<PeriodicClientTrigger, TSGetLink<PeriodicClientTrigger>> s_periodicClientTriggers;

// STATIC: 0x00D3F5DC
static TSHashTable<DESTLOCSPELLCASTINDEXHASHNODE, CHashKeyGUID> s_destLocSpellCasts;

// VERIFY: f852b51c manual
// OFFSET: 0x008008D0
void Spell_C_CancelChannelSpell(int32_t spellId) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CANCEL_CHANNELLING));
    msg.Put(static_cast<uint32_t>(spellId));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: fe82041e manual
// OFFSET: 0x007FD620
bool Spell_C_IsTargeting() {
    return s_targetingSpell != nullptr;
}

// VERIFY: e3716b07 auto
// OFFSET: 0x007FE130
int32_t Spell_C_GetAutoRepeatingSpell() {
    return s_autoRepeatingSpell;
}

// VERIFY: e95a2641 auto
// OFFSET: 0x007FE140
void Spell_C_SetAutoRangedCombatSpell(int32_t spellId) {
    if (s_autoRangedCombatSpell != spellId) {
        s_autoRangedCombatSpell = spellId;

        if (!s_autoRepeatingSpell) {
            if (spellId) {
                FrameScript_SignalEvent(SCRIPT_EVENT_START_AUTOREPEAT_SPELL, nullptr);
            } else {
                FrameScript_SignalEvent(SCRIPT_EVENT_STOP_AUTOREPEAT_SPELL, nullptr);
            }
        }
    }
}

// VERIFY: 1930d71d auto
// OFFSET: 0x007FE180
int32_t Spell_C_GetAutoRangedCombatSpell() {
    return s_autoRangedCombatSpell;
}

// VERIFY: 94e1f1cb auto
// OFFSET: 0x007FE190
void Spell_C_ClearLastAutoRangedCombatError() {
    s_lastAutoRangedCombatError = 187;
}

// VERIFY: 7a4c1c3e manual
// OFFSET: 0x0080C340
bool Spell_C_HandleTerrainClick(const CTerrainClickEvent& evt) {
    PendingSpellCast* spell = s_targetingSpell;

    if (!spell) {
        return false;
    }

    uint32_t needTargets = s_needTargets;

    if (needTargets & 0x20) {
        spell->m_data.m_sourceTarget = evt.m_guid;
        spell->m_data.m_sourcePos = evt.m_point;
        spell->m_data.m_targetFlags |= 0x20;
        needTargets &= ~0x20u;
    } else if (needTargets & 0x40) {
        spell->m_data.m_destTarget = evt.m_guid;
        spell->m_data.m_destPos = evt.m_point;
        spell->m_data.m_targetFlags |= 0x40;
        needTargets &= ~0x40u;
    } else {
        return false;
    }

    s_needTargets = needTargets;

    if (!needTargets) {
        Spell_C::SendPendingCast(spell, false);
    }

    return true;
}

// VERIFY: bfa71bbe auto
// OFFSET: 0x007FCC70
bool Spell_C_IsRestrictedAbility(const SpellRec* spell) {
    if (!spell) {
        return false;
    }

    return (spell->m_attributes & 0x404) || (spell->m_attributesEx & 0x200) || (spell->m_attributesExB & 0x100000);
}

// VERIFY: e2735a7d auto
// OFFSET: 0x007FDE20
bool Spell_C_HasEffect(const SpellRec* spell, int32_t effect) {
    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (spell->m_effect[i] == effect) {
            return true;
        }
    }

    return false;
}

// VERIFY: 4b3db043 auto
// OFFSET: 0x007FE820
int32_t Spell_C_IsOverride(int32_t spellId, const OverrideSpellDataRec* rec) {
    for (uint32_t i = 0; i < sizeof(rec->m_spells) / sizeof(rec->m_spells[0]); i++) {
        if (rec->m_spells[i] == spellId) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: da3d51b4 manual
// OFFSET: 0x008012F0
int32_t Spell_C_GetPowerCost(const SpellRec* spell, const CGUnit_C* unit) {
    if (!unit) {
        return -1;
    }

    uint32_t rank = unit->GetSpellRank(spell);
    int32_t cost = spell->m_manaCost + spell->m_manaCostPerLevel * (static_cast<int32_t>(rank / 5) - spell->m_baseLevel);

    if (spell->m_manaCostPct) {
        cost += spell->m_manaCostPct * static_cast<int32_t>(unit->GetBasePowerForType(static_cast<POWER_TYPE>(spell->m_powerType))) / 100;
    }

    if ((spell->m_attributesExD & 0x400) && const_cast<CGUnit_C*>(unit)->IsActivePlayer()) {
        uint32_t speed = 0;
        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(unit->GetShapeshiftForm());

        if (form && form->m_combatRoundTime) {
            speed = form->m_combatRoundTime;
        } else {
            const CGItem_C* item = const_cast<CGUnit_C*>(unit)->GetBag()->GetItemPointer(((spell->m_attributesExC & 0x1000000) != 0) + 15);

            if (item && item->IsUsable() && !unit->IsDisarmed((spell->m_attributesExC >> 24) & 0x1)) {
                WOWGUID none = { 0 };
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats) {
                    speed = stats->m_delay;
                }
            } else {
                speed = 2000;
            }
        }

        cost += speed / 100;
    }

    int32_t school = spell->m_schoolMask;
    float modified = static_cast<float>(cost + unit->GetSpellCostModifier(school));
    int32_t base = CMath::fint_(unit->GetSpellCostMultiplier(school) * modified);
    cost = base;

    int32_t flat;
    int32_t pct;

    if (Spell_C_GetSpellModifiers(spell, 14, flat, pct)) {
        cost = (flat + base) * pct / 100;
    }

    if (!(unit->m_obj->m_type & TYPE_PLAYER) && (spell->m_attributes & 0x80000) && spell->m_spellLevel) {
        float spellLevelValue = GetGameTableValue(6, spell->m_spellLevel - 1, 0);
        float unitLevelValue = GetGameTableValue(6, unit->m_unit->m_unitFieldLevel - 1, 0);

        if (spellLevelValue > 0.0f && unitLevelValue > 0.0f) {
            cost = CMath::fint_(unitLevelValue / spellLevelValue * cost);
        }
    }

    if ((unit->m_obj->m_type & TYPE_PLAYER) && Spell_C_HasEffect(spell, SPELL_EFFECT_CAST_BUTTON)) {
        for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
            if (spell->m_effect[i] != SPELL_EFFECT_CAST_BUTTON) {
                continue;
            }

            uint32_t first = spell->m_effectMiscValue[i];
            int32_t count = spell->m_effectMiscValueB[i];

            if (first >= 12 || first + count > 12) {
                continue;
            }

            for (int32_t j = 0; j < count; j++) {
                int32_t petSpell;
                int32_t spellId = CGActionBar::GetSpell(first + j + 132, petSpell);

                if (!spellId || (spellId & 0xF0000000)) {
                    continue;
                }

                SpellRec totemSpell;

                if (g_spellDB.GetRecord(spellId, &totemSpell) && (totemSpell.m_attributesExG & 0x20)) {
                    cost += Spell_C_GetPowerCost(&totemSpell, unit);
                }
            }
        }
    }

    return cost <= 0 ? 0 : cost;
}

// VERIFY: fc34e07a manual
// OFFSET: 0x008017E0
bool Spell_C_HaveSpellPower(CGUnit_C* unit, const SpellRec* spell) {
    if (!unit) {
        return false;
    }

    POWER_TYPE powerType = static_cast<POWER_TYPE>(spell->m_powerType);

    if (powerType == POWER_TYPE_HEALTH) {
        int32_t cost = Spell_C_GetPowerCost(spell, unit);
        return cost < static_cast<int32_t>(unit->GetPredictedHealth());
    }

    if (powerType == POWER_TYPE_RUNES) {
        return unit->IsActivePlayer() && CGRuneInfo::HasRunesForSpell(spell->m_id, nullptr);
    }

    int32_t power = unit->GetPredictedPower(powerType);
    return Spell_C_GetPowerCost(spell, unit) <= power;
}

// VERIFY: 7772af94 manual
// OFFSET: 0x00802850
int32_t QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || !spell.m_difficulty) {
        return spellId;
    }

    SpellDifficultyRec* difficulty = g_spellDifficultyDB.GetRecord(spell.m_difficulty);

    if (!difficulty) {
        return spellId;
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (!map) {
        return spellId;
    }

    uint32_t index;

    if (map->m_instanceType == 2) {
        index = CGGameUI::GetCurrentRaidDifficulty();
    } else if (map->m_instanceType == 1) {
        index = CGGameUI::GetCurrentDungeonDifficulty();
    } else {
        return spellId;
    }

    int32_t result = difficulty->m_difficultySpellID[index];

    if (result || (index == 3 && (result = difficulty->m_difficultySpellID[1]) != 0)) {
        return result;
    }

    result = difficulty->m_difficultySpellID[0];
    return result ? result : spellId;
}

// VERIFY: 36d7f927 auto
// OFFSET: 0x008091B0
int32_t Spell_C_IsActionOnHold(const int32_t* spellId, int32_t index) {
    return s_spellHistory[index].IsOnHold(*spellId, 0);
}

// VERIFY: 668afa49 manual
// OFFSET: 0x0080AC10
int32_t Spell_C_NeedsCooldownEvent(int32_t itemId) {
    WOWGUID player = ClntObjMgrGetActivePlayer();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, player, ItemCheckCooldownCallback, nullptr, true);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < sizeof(stats->m_spellId) / sizeof(stats->m_spellId[0]); i++) {
        if (static_cast<int32_t>(stats->m_spellId[i]) > 0 && !stats->m_spellTrigger[i]) {
            return s_spellHistory[0].IsOnHold(stats->m_spellId[i], itemId);
        }
    }

    return 0;
}

// VERIFY: a6fcb014 manual
// OFFSET: 0x0080B5D0
bool Spell_C_IsUsableAction(CGUnit_C* unit, const SpellRec* spell, const WOWGUID& target, const ItemStats_C* item, bool ignoreControl) {
    if (!unit) {
        return false;
    }

    if (spell->m_effect[0] == SPELL_EFFECT_TRADE_SKILL) {
        return true;
    }

    if (unit->IsActivePlayer()) {
        bool attack = spell->m_effect[0] == SPELL_EFFECT_ATTACK || (spell->m_attributes & 0x2);
        uint32_t flags = static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags;

        if (flags & 0x1000000) {
            if (attack || Spell_C_IsRestrictedAbility(spell)) {
                return false;
            }
        } else if (!attack && (flags & 0x800000)) {
            return false;
        }
    }

    if (unit->IsDeadOrGhost() && !(spell->m_attributes & 0x800000)) {
        return false;
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (map) {
        if (map->m_instanceType == 4) {
            if (!(spell->m_attributesExD & 0x20000) && (spell->m_recoveryTime > 900000 || spell->m_categoryRecoveryTime > 900000 || (spell->m_attributesExD & 0x10000))) {
                return false;
            }
        } else if (spell->m_attributesExF & 0x2) {
            return false;
        }
    }

    if (!CGGameUI::m_hasControl && !ignoreControl) {
        int32_t allowed = 0;

        if ((unit->m_unit->m_unitFieldFlags & 0x800000) && ((spell->m_attributesExE & 0x20000) || Spell_C_CancelsAuraEffect(unit, spell, 7, nullptr))) {
            allowed = 1;
        }

        if ((unit->m_unit->m_unitFieldFlags & 0x400000) && ((spell->m_attributesExE & 0x40000) || Spell_C_CancelsAuraEffect(unit, spell, 5, nullptr))) {
            allowed = 1;
        }

        if (unit->IsCharmed() && Spell_C_CancelsCharm(unit, spell, nullptr)) {
            allowed = 1;
        }

        if ((!(unit->m_unit->m_unitFieldFlags & 0x100000) || !(spell->m_attributes & 0x1000000)) && !allowed) {
            return false;
        }
    }

    if ((!item || !(item->m_flags[0] & 0x10000000)) && !Spell_C_HaveSpellTokens(unit, spell, false, nullptr)) {
        return false;
    }

    if (!Spell_C_HaveEquippedSpellItems(unit, spell, true, false, nullptr)) {
        return false;
    }

    if (spell->m_attributesEx & 0x500000) {
        if (unit->IsActivePlayer() && !CGGameUI::GetComboPoints(target) && !unit->IgnoresAuraState(spell)) {
            return false;
        }

        if (unit->m_obj->m_guid == CGPetInfo::GetPet(0) && !CGPetInfo::GetComboPoints(target)) {
            return false;
        }
    }

    int32_t reason;

    if (!unit->CheckShapeshiftRules(spell, &reason) && (reason == 94 || !unit->CanAutoUnshift())) {
        return false;
    }

    if (!unit->CheckStealthRules(spell, &reason) || ((spell->m_attributes & 0x10000000) && (unit->m_unit->m_unitFieldFlags & 0x80000) && !unit->IgnoresAuraState(spell))) {
        return false;
    }

    if (spell->m_casterAuraState && !unit->HasAuraState(spell->m_casterAuraState) && !unit->IgnoresAuraState(spell)) {
        return false;
    }

    if (spell->m_excludeCasterAuraState && unit->HasAuraState(spell->m_excludeCasterAuraState) && !unit->IgnoresAuraState(spell)) {
        return false;
    }

    int32_t casterAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_casterAuraSpell);

    if (casterAura && !unit->HasAuraBySpellId(casterAura) && !unit->IgnoresAuraState(spell)) {
        return false;
    }

    int32_t excludeCasterAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_excludeCasterAuraSpell);

    if (excludeCasterAura && unit->HasAuraBySpellId(excludeCasterAura) && !unit->IgnoresAuraState(spell)) {
        return false;
    }

    int32_t targetAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_targetAuraSpell);
    int32_t excludeTargetAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_excludeTargetAuraSpell);

    if (spell->m_targetAuraState || spell->m_excludeTargetAuraState || targetAura || excludeTargetAura) {
        WOWGUID guid = target;

        if (!guid) {
            guid = CGGameUI::m_lockedTarget;
        }

        CGUnit_C* targetUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (g_autoSelfCastCVar->m_intValue) {
            int32_t implicit = spell->m_implicitTargetA[0];

            if (!targetUnit || ((implicit == 21 || implicit == 35 || implicit == 57 || implicit == 61) && !unit->CanAssist(targetUnit, false))) {
                targetUnit = ClntObjMgrGetActivePlayerObj();
            }
        }

        if (targetUnit) {
            if (spell->m_targetAuraState && !targetUnit->HasAuraState(spell->m_targetAuraState) && !unit->IgnoresAuraState(spell)) {
                return false;
            }

            if (spell->m_excludeTargetAuraState && targetUnit->HasAuraState(spell->m_excludeTargetAuraState) && !unit->IgnoresAuraState(spell)) {
                return false;
            }

            if (targetAura && !targetUnit->HasAuraBySpellId(targetAura) && !unit->IgnoresAuraState(spell)) {
                return false;
            }

            if (excludeTargetAura && targetUnit->HasAuraBySpellId(excludeTargetAura) && !unit->IgnoresAuraState(spell)) {
                return false;
            }

            if (spell->m_implicitTargetA[0] == 6 && !unit->CanAttack(targetUnit)) {
                return false;
            }

            if (spell->m_implicitTargetA[0] == 21 && !unit->CanAssist(targetUnit, false)) {
                return false;
            }

            if (spell->m_implicitTargetA[0] == 35 && (!unit->IsInMyPartyOrRaid(targetUnit) || !unit->CanAssist(targetUnit, false))) {
                return false;
            }

            int32_t implicit = spell->m_implicitTargetA[0];

            if ((implicit == 57 || implicit == 61) && (!unit->IsInMyParty(targetUnit) || !unit->CanAssist(targetUnit, false))) {
                return false;
            }
        } else if (spell->m_targetAuraState || targetAura) {
            return false;
        }
    }

    if ((spell->m_attributes & 0x2000000) && Spell_C_IsActionOnHold(&spell->m_id, 0)) {
        return false;
    }

    if (unit->m_obj->m_guid == CGPetInfo::GetPet(0) && !CGPetInfo::PetActionsUsable(false)) {
        return false;
    }

    int32_t interruptFlags = spell->m_auraInterruptFlags;

    if (((interruptFlags & 0x100) || ((spell->m_attributesEx & 0x44) && (spell->m_channelInterruptFlags & 0x100))) && !(unit->m_clientFlags & 0x200000)) {
        return false;
    }

    if (((interruptFlags & 0x80) || ((spell->m_attributesEx & 0x44) && (spell->m_channelInterruptFlags & 0x80))) && (unit->m_clientFlags & 0x200000)) {
        return false;
    }

    if (spell->m_attributes & 0xC000) {
        if (unit->IsOutdoors()) {
            if (spell->m_attributes & 0x4000) {
                return false;
            }
        } else if (spell->m_attributes & 0x8000) {
            return false;
        }
    }

    return (!(spell->m_attributesExD & 0x4000000) || static_cast<CGPlayer_C*>(unit)->IsInFlyableZone(1)) && (!(spell->m_attributesExC & 0x800) || !map || map->m_instanceType == 3) && (!(spell->m_attributesExF & 0x800) || !map || map->m_instanceType != 2);
}

// VERIFY: 59e5790d auto
// OFFSET: 0x008100E0
void Spell_C_Initialize() {
    ClientServices::SetMessageHandler(SMSG_SPELL_START, Packet_Group_30, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_GO, Packet_Group_30, nullptr);
    ClientServices::SetMessageHandler(SMSG_CAST_FAILED, Packet_SMSG_CAST_FAILED, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_FAILURE, Packet_SMSG_SPELL_FAILURE, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_FAILED_OTHER, Packet_SMSG_SPELL_FAILED_OTHER, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_CAST_FAILED, Packet_SMSG_PET_CAST_FAILED, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_COOLDOWN, Packet_SMSG_SPELL_COOLDOWN, nullptr);
    ClientServices::SetMessageHandler(SMSG_ITEM_COOLDOWN, Packet_SMSG_ITEM_COOLDOWN, nullptr);
    ClientServices::SetMessageHandler(SMSG_COOLDOWN_EVENT, Packet_Group_31, nullptr);
    ClientServices::SetMessageHandler(SMSG_CLEAR_COOLDOWN, Packet_Group_31, nullptr);
    ClientServices::SetMessageHandler(SMSG_MODIFY_COOLDOWN, Packet_Group_31, nullptr);
    ClientServices::SetMessageHandler(SMSG_COOLDOWN_CHEAT, Packet_SMSG_COOLDOWN_CHEAT, nullptr);
    ClientServices::SetMessageHandler(SMSG_PET_TAME_FAILURE, Packet_SMSG_PET_TAME_FAILURE, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_DELAYED, Packet_SMSG_SPELL_DELAYED, nullptr);
    ClientServices::SetMessageHandler(MSG_CHANNEL_START, Packet_MSG_CHANNEL_START, nullptr);
    ClientServices::SetMessageHandler(MSG_CHANNEL_UPDATE, Packet_MSG_CHANNEL_UPDATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAY_SPELL_VISUAL, Packet_SMSG_PLAY_SPELL_VISUAL, nullptr);
    ClientServices::SetMessageHandler(SMSG_PLAY_SPELL_IMPACT, Packet_SMSG_PLAY_SPELL_IMPACT, nullptr);
    ClientServices::SetMessageHandler(SMSG_SET_FLAT_SPELL_MODIFIER, Packet_Group_32, nullptr);
    ClientServices::SetMessageHandler(SMSG_SET_PCT_SPELL_MODIFIER, Packet_Group_32, nullptr);
    ClientServices::SetMessageHandler(SMSG_GAME_OBJECT_RESET_STATE, Packet_SMSG_GAMEOBJECT_RESET_STATE, nullptr);
    ClientServices::SetMessageHandler(SMSG_FEIGN_DEATH_RESISTED, Packet_SMSG_FEIGN_DEATH_RESISTED, nullptr);
    ClientServices::SetMessageHandler(SMSG_SPELL_UPDATE_CHAIN_TARGETS, Packet_SMSG_SPELL_UPDATE_CHAIN_TARGETS, nullptr);
    ClientServices::SetMessageHandler(SMSG_NOTIFY_DEST_LOC_SPELL_CAST, Packet_SMSG_NOTIFY_DEST_LOC_SPELL_CAST, nullptr);
    ClientServices::SetMessageHandler(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, Packet_SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA, nullptr);
    ClientServices::SetMessageHandler(SMSG_SET_PROJECTILE_POSITION, Packet_SMSG_SET_PROJECTILE_POSITION, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESET_RANGED_COMBAT_TIMER, Packet_SMSG_MEETINGSTONE_IN_PROGRESS, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESUME_CAST_BAR, Packet_SMSG_UNIT_SPELLCAST_START, nullptr);
    s_needTargets = 0;
    s_targetingSpell = nullptr;
    memset(s_spellFlatModifiers, 0, sizeof(s_spellFlatModifiers));
    memset(s_spellPctModifiers, 0, sizeof(s_spellPctModifiers));
    s_autoRepeatingSpell = 0;
    s_autoRangedCombatSpell = 0;
    s_lastAutoRangedCombatError = 187;
    s_lastFailedSpellId = 0;
    s_feedPetSpell = 0;
    s_spellClassSet = 0;
    s_delayedTrajectoryCast = nullptr;
    s_enableErrorSpeechCVar = CVar::Lookup("Sound_EnableErrorSpeech");
}

// INLINED
inline PendingSpellCast::PendingSpellCast() : m_isTrajectoryCast(0) {
}

// VERIFY: 672a78e9 manual
// OFFSET: 0x00805010
PendingSpellCast* CreatePendingSpellCast(bool assignCastCount) {
    PendingSpellCast* cast = s_freePendingSpellCasts.Head();

    if (!cast) {
        cast = s_freePendingSpellCasts.NewNode(2, 0, 8);
    }

    s_pendingSpellCasts.LinkToHead(cast);
    memset(&cast->m_data, 0, sizeof(PendingSpellCastData));
    cast->m_hidden = 0;
    cast->m_isTrajectoryCast = 0;

    if (assignCastCount) {
        uint8_t castCount = s_nextCastCount;

        if (!castCount) {
            castCount = 1;
        }

        cast->m_data.m_castCount = castCount;
        s_nextCastCount = castCount + 1;
    }

    return cast;
}

// VERIFY: e1f3a7c3 manual
// OFFSET: 0x00805100
void ReleasePendingSpellCast(PendingSpellCast* cast) {
    for (PendingSpellCast* node = s_pendingSpellCasts.Head(); node; node = s_pendingSpellCasts.Next(node)) {
        if (node == cast) {
            s_freePendingSpellCasts.LinkToTail(cast);
            return;
        }
    }
}

// VERIFY: 4917a9ff manual
// OFFSET: 0x00805180
PendingSpellCast* GetPendingSpellCast(uint8_t castCount, int32_t spellId, const WOWGUID& caster) {
    if (!spellId) {
        return nullptr;
    }

    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast->m_data.m_castCount == castCount && cast->m_data.m_spellId == spellId && cast->m_data.m_caster == caster) {
            return cast;
        }
    }

    return nullptr;
}

// VERIFY: 01cc0239 auto
// OFFSET: 0x008051D0
PendingSpellCast* FindPendingSpellCast(const WOWGUID& castItem, PendingSpellCast* fallback, uint8_t castCount, int32_t spellId, const WOWGUID& caster) {
    PendingSpellCast* cast = GetPendingSpellCast(castCount, spellId, caster);

    if (cast) {
        return cast;
    }

    if (caster.m_guid && caster == ClntObjMgrGetActivePlayer()) {
        cast = CreatePendingSpellCast(false);
    } else {
        cast = fallback;
    }

    cast->m_data.m_castCount = castCount;
    cast->m_data.m_spellId = spellId;
    cast->m_data.m_castItem = castItem;
    cast->m_data.m_caster = caster;
    return cast;
}

// VERIFY: 1cc8cccc manual
// OFFSET: 0x007FE8D0
bool PendingSpellCast::IsCharmedUnitCast() const {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return false;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    const WOWGUID& controlled = player->m_unit->m_unitFieldCharm.m_guid ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;
    return controlled == this->m_data.m_caster;
}

// VERIFY: 03fbbea4 auto
// OFFSET: 0x007FEC00
void ClearSpellCastState(CGUnit_C* unit, PendingSpellCast* cast, const SpellRec* spell) {
    unit->ClearCastingSpell(unit->m_castSpellId, unit->m_unit->m_unitChannelSpell == 0, true);
    unit->ClearEffectKit(cast->m_data.m_spellId, 4, 0, 0, 0);
    const SpellVisualRec* visual = GetSpellVisual(spell);

    if (visual) {
        SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_precastKit);

        if (kit && (kit->m_startAnimID || kit->m_animID)) {
            unit->UpdateBaseAnimation(0, -1);
        }
    }

    if (unit->m_mountTransitionSpellId == static_cast<uint32_t>(cast->m_data.m_spellId)) {
        if (unit->m_mountDisplayId != static_cast<int32_t>(unit->m_unit->m_unitFieldMountdisplayid)) {
            unit->OnMountDisplayChanged(unit->m_unit->m_unitFieldMountdisplayid);
        }

        unit->m_mountTransitionSpellId = 0;
    }

    unit->StopUpdatingMissileTrajectory(cast->m_data.m_spellId);
}

// VERIFY: 4345f843 manual
// OFFSET: 0x007FA2E0
void SpellVisualsHandleCastStart(CGUnit_C* unit, PendingSpellCastData* data, const SpellRec* spell) {
    if (!unit) {
        SErrSetLastError(87);
        return;
    }

    if (spell->m_attributes & 0x2) {
        unit->SetSheatheState(WEAPONMODE_RANGEDMODE, 1, 0);
    }

    if (spell->m_attributesExB & 0x20) {
        if (unit->m_animationState & 0x200) {
            return;
        }

        unit->SetCastingAutoRepeat(true);
    }

    SpellVisualRec visualRec;
    const SpellVisualRec* visual = unit->GetAppropriateSpellVisual(spell, visualRec, 0, 0);

    if (visual) {
        if (visual->m_hasMissile && visual->m_missileModel && (visual->m_missileModel == -1 || visual->m_missileModel == -2)) {
            unit->SetSheatheState(WEAPONMODE_NORMALMODE, 1, 0);
        }

        SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_precastKit);

        if (kit) {
            if (kit->m_startAnimID > 0 || kit->m_animID > 0) {
                unit->SetCastingAnim(true);
            }

            PlaySpellVisuaKitData kitData(const_cast<SpellRec*>(spell), kit, 4);
            unit->PlaySpellVisualKit(kitData);
        }

        if (unit->IsActivePlayer() && (spell->m_attributes & 0x400000) && (data->m_targetFlags & 0x2) && data->m_objectTarget.m_guid) {
            if (!(spell->m_facingCasterFlags & 0x1) || unit->IsInFrontOfTargetWithGuid(data->m_objectTarget)) {
                unit->SetTrackingTarget(data->m_objectTarget, TRACKTYPE_SPELLPRECAST);
            }
        }

        if (visual->m_flags & 0x20) {
            unit->AttachPrecastMissileModel(visual, data->m_castCount);
        }
    }
}

// VERIFY: 98e97d79 manual
// OFFSET: 0x00805330
void StartSpellCastVisuals(CGUnit_C* unit, PendingSpellCast* cast, const SpellRec* spell, int32_t castTime, bool hidden) {
    if (!unit) {
        return;
    }

    cast->m_hidden = hidden;
    PendingSpellCast* current = GetPendingSpellCast(unit->m_castCount, unit->m_castSpellId, unit->m_obj->m_guid);

    if (current && current->m_hidden) {
        if (!hidden) {
            ClearSpellCastState(unit, current, spell);

            if (unit == ClntObjMgrGetActivePlayerObj()) {
                CGTradeSkillInfo::CommitPendingRecast(cast->m_data.m_spellId);
            }
        }
    } else if (!hidden && unit == ClntObjMgrGetActivePlayerObj()) {
        CGTradeSkillInfo::CommitPendingRecast(cast->m_data.m_spellId);
    }

    if (!unit->m_castSpellId && !cast->m_isTrajectoryCast) {
        unit->SetCastingSpell(cast->m_data.m_castCount, cast->m_data.m_spellId);
        SpellVisualsHandleCastStart(unit, &cast->m_data, spell);
    }

    if (castTime > 0) {
        bool showCastBar = true;

        if (!(spell->m_attributesExD & 0x8000000)) {
            if (spell->m_attributes & 0x2) {
                showCastBar = false;
            }

            if (cast->m_data.m_caster != ClntObjMgrGetActivePlayer() && ((spell->m_attributes & 0x180) || (spell->m_attributesExD & 0x1))) {
                showCastBar = false;
            }
        }

        if (showCastBar) {
            if (cast->m_data.m_spellId != unit->m_castSpellId) {
                ClearSpellCastState(unit, cast, spell);
                unit->SetCastingSpell(cast->m_data.m_castCount, cast->m_data.m_spellId);
                SpellVisualsHandleCastStart(unit, &cast->m_data, spell);
            }

            unit->SetUICastingSpell(cast->m_data.m_spellId, cast->m_data.m_objectTarget, castTime);
            Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 324);
        }
    }

    if ((cast->m_data.m_caster == ClntObjMgrGetActivePlayer() || cast->IsCharmedUnitCast()) && cast->m_data.m_castCount) {
        CGSpellBook::UpdateSelection();
    }
}

// VERIFY: 0e9e554a manual
// OFFSET: 0x00806700
void PendingSpellCast::OnSpellStartPacket(CDataStore* msg) {
    WOWGUID caster = this->m_data.m_caster;
    uint32_t castFlags = this->m_data.m_castFlags;
    uint32_t castTime;
    msg->Get(castTime);
    SpellGetCastTargets(&this->m_data, msg);
    uint32_t power = 0;
    uint32_t hasPower = castFlags & 0x800;

    if (hasPower) {
        msg->Get(power);
    }

    uint8_t runeCooldowns[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t runesBefore = 0;
    uint8_t runesAfter = 0;
    uint32_t hasRunes = castFlags & 0x200000;

    if (hasRunes) {
        msg->Get(runesBefore);
        msg->Get(runesAfter);

        for (uint32_t i = 0; i < 6; i++) {
            if (((1 << i) & runesBefore) && !((1 << i) & runesAfter)) {
                msg->Get(runeCooldowns[i]);
            }
        }
    }

    uint32_t ammoDisplayId = 0;
    uint32_t ammoInventoryType = 0;

    if (castFlags & 0x20) {
        msg->Get(ammoDisplayId);
        msg->Get(ammoInventoryType);
    }

    uint32_t schoolImmunities = 0;
    uint32_t immunities = 0;
    uint32_t hasImmunities = castFlags & 0x4000000;

    if (hasImmunities) {
        msg->Get(schoolImmunities);
        msg->Get(immunities);
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_data.m_spellId, &spell)) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    if (hasPower && !(unit->m_unit->m_unitFieldFlags2 & 0x100)) {
        if (spell.m_powerType == POWER_TYPE_HEALTH) {
            unit->SetPredictedHealth(power);
        } else if (spell.m_powerType == POWER_TYPE_RUNES) {
            if (caster == ClntObjMgrGetActivePlayer() && hasRunes) {
                CGRuneInfo::SetPredictedRunes(runesBefore, runesAfter, runeCooldowns);
            }
        } else {
            unit->SetPredictedPower(static_cast<POWER_TYPE>(spell.m_powerType), power);
        }

        if (spell.m_powerType == POWER_TYPE_HEALTH) {
            unit->SetPredictedHealth(power);
        } else {
            unit->SetPredictedPower(static_cast<POWER_TYPE>(spell.m_powerType), power);
        }
    }

    if (hasImmunities) {
        unit->SetCastSchoolImmunities(schoolImmunities);
        unit->SetCastImmunities(immunities);
    }

    uint32_t hidden = castFlags & 0x1;

    if (!hidden) {
        StartSpellCastVisuals(unit, this, &spell, castTime, false);
    }

    if (caster == CGUnit_C::s_activeMover && castTime) {
        SpellMissileRec* missile = g_spellMissileDB.GetRecord(spell.m_spellMissileID);

        if (missile && (missile->m_flags & 0x1)) {
            unit->StartUpdatingMissileTrajectory(this->m_data.m_spellId, OsGetAsyncTimeMs(), castTime);
        }
    }

    if (!hidden) {
        WOWGUID guid = unit->m_obj->m_guid;
        UnitCombatLogCastStart(guid, this->m_data.m_objectTarget, this->m_data.m_spellId);
    }

    if (spell.m_attributes & 0x2) {
        unit->UpdateAmmoModel(ammoDisplayId, 30);
    }

    if (unit->IsActivePlayer()) {
        unit->ReleaseMissiles();
        WOWGUID target;

        if ((spell.m_attributes & 0x400000) && (this->m_data.m_targetFlags & 0x2) && this->m_data.m_objectTarget.m_guid) {
            target = this->m_data.m_objectTarget;
        } else {
            target = CGGameUI::m_lockedTarget;
        }

        if (spell.m_attributes & 0x400000) {
            unit->SetTrackingTarget(target, TRACKTYPE_SPELLPRECAST);
        }
    } else if ((this->m_data.m_targetFlags & 0x2) && this->m_data.m_objectTarget.m_guid && this->m_data.m_objectTarget == ClntObjMgrGetActivePlayer()) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && !unit->CanAssist(player, false)) {
            WOWGUID guid = unit->m_obj->m_guid;
            TargetAttacker(guid);
        }
    }
}

// VERIFY: ccbce7b4 manual
// OFFSET: 0x0080FEE0
int32_t Packet_Group_30(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID castItem;
    SmartGUID caster;
    *msg >> castItem;
    *msg >> caster;
    uint8_t castCount;
    msg->Get(castCount);
    uint32_t spellId;
    msg->Get(spellId);
    uint32_t castFlags;
    msg->Get(castFlags);
    PendingSpellCast local;
    PendingSpellCast* cast = GetPendingSpellCast(castCount, spellId, caster);

    if (!cast) {
        if (caster.m_guid && caster == ClntObjMgrGetActivePlayer()) {
            cast = CreatePendingSpellCast(false);
        } else {
            cast = &local;
        }

        cast->m_data.m_castCount = castCount;
        cast->m_data.m_spellId = spellId;
        cast->m_data.m_castItem = castItem;
        cast->m_data.m_caster = caster;
    }

    cast->m_data.m_castFlags = castFlags;

    if (msgId == SMSG_SPELL_START) {
        cast->OnSpellStartPacket(msg);
    } else if (msgId == SMSG_SPELL_GO) {
        cast->OnSpellGo(msg);
    }

    return 1;
}

// VERIFY: 2ad579bf manual
// OFFSET: 0x00722C50
void CGUnit_C::SetPredictedPower(POWER_TYPE powerType, int32_t value) {
    uint32_t maximum = powerType == POWER_TYPE_HEALTH ? this->m_unit->m_unitFieldMaxhealth : this->m_unit->m_unitFieldMaxpower[powerType];

    if (static_cast<int32_t>(maximum) < value) {
        value = maximum;
    }

    if (value == this->m_predictedPower[powerType]) {
        return;
    }

    this->m_predictedPower[powerType] = value;

    if (static_cast<uint32_t>(value) == maximum) {
        Script_SendUnitSignal(this->m_obj->m_guid, powerType + 19);
    }

    if (this->m_obj->m_guid == ClntObjMgrGetActivePlayer() || this->m_obj->m_guid == CGPetInfo::GetPet(0)) {
        CGSpellBook::UpdateUsablePower();
    }
}

// VERIFY: 93438464 manual
// OFFSET: 0x0072E240
void CGUnit_C::ReleaseMissiles() {
    C3Vector position;
    position = this->GetMissileFirePosition();
    this->ReleaseMissiles(position);
}

// VERIFY: 936f6df8 manual
// OFFSET: 0x007FD970
bool Spell_C_GetSpellModifiers(const SpellRec* spell, int32_t modifier, int32_t& flat, int32_t& pct) {
    if (!spell->m_spellClassSet || spell->m_spellClassSet != s_spellClassSet) {
        flat = 0;
        pct = 100;
        return false;
    }

    if (spell->m_attributesExC & 0x20000000) {
        flat = 0;
        pct = 100;
        return false;
    }

    pct = 0;
    flat = 0;

    for (uint32_t i = 0; i < sizeof(spell->m_spellClassMask) * 8; i++) {
        if ((1 << (i & 31)) & spell->m_spellClassMask[i >> 5]) {
            flat += s_spellFlatModifiers[i][modifier];
            pct += s_spellPctModifiers[i][modifier];
        }
    }

    if (!flat && !pct) {
        return false;
    }

    pct += 100;

    if (pct < 0) {
        pct = 0;
    }

    return true;
}

// VERIFY: 8fe30e90 manual
// OFFSET: 0x007FDB50
bool Spell_C_ModifySpellValue(const SpellRec* spell, int32_t& value, int32_t modifier) {
    int32_t flat;
    int32_t pct;

    if (!Spell_C_GetSpellModifiers(spell, modifier, flat, pct)) {
        return false;
    }

    value = pct * (flat + value) / 100;
    return true;
}

// VERIFY: a04d6139 auto
// OFFSET: 0x007FD790
CMapEntity* Spell_C_GetSpellWorldObject() {
    return s_spellWorldObject;
}

// VERIFY: 91de1c39 auto
// OFFSET: 0x007FCC30
void Spell_C_DestroySpellWorldObject() {
    if (s_spellWorldObject) {
        World::ObjectDestroy(s_spellWorldObject);
        s_spellWorldObject = nullptr;
    }

    if (s_spellWorldModel) {
        s_spellWorldModel->Release();
        s_spellWorldModel = nullptr;
    }
}

// VERIFY: decfd300 manual
// OFFSET: 0x007FECC0
void HandleSpellFailedOrGo(CGUnit_C* unit, PendingSpellCast* cast, const SpellRec* spell, int32_t reason) {
    if (!unit) {
        return;
    }

    if (spell && spell->m_spellVisualID[0] > 0 && unit->m_worldModel && unit->m_worldModel->IsLoaded(0, 0) && unit->m_torsoKeyBone != -1) {
        SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell->m_spellVisualID[0]);

        if (visual && visual->m_precastKit > 0) {
            uint32_t sequenceId = unit->m_worldModel->GetBoneSequenceId(unit->m_torsoKeyBone);
            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_precastKit);

            if (kit && kit->m_animID > 0 && sequenceId != 0xFFFFFFFF && static_cast<uint32_t>(kit->m_animID) == sequenceId) {
                unit->OnKeyBoneAnimEnd(unit->m_worldModel, unit->m_torsoKeyBone, kit->m_animID);
            }
        }
    }

    uint8_t castCount = unit->m_castCount;

    if (cast->m_data.m_castCount == castCount && cast->m_data.m_spellId == unit->m_castSpellId) {
        ClearSpellCastState(unit, cast, spell);
    }

    switch (reason) {
        case 187:
            Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 330);
            break;
        case 24:
        case 27:
        case 105:
            Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 327);
            break;
        case 40:
        case 41:
            Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 328);
            break;
        default:
            Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 326);
            break;
    }

    if (cast->m_data.m_castCount == castCount && cast->m_data.m_spellId == static_cast<int32_t>(unit->m_castingSpellId)) {
        unit->ResetUICastingSpell();
        Script_SendUnitSpellSignal(unit->m_obj->m_guid, cast->m_data.m_castCount, spell, 325);
    }

    if (cast == s_targetingSpell) {
        s_needTargets = 0;
        s_targetingSpell = nullptr;
        CursorSetResetMode(POINT_CURSOR);
        CursorSetMode(POINT_CURSOR);
        Spell_C_DestroySpellWorldObject();
        CGInputControl* input = CGInputControl::GetActive();
        input->m_mouseModeFlags &= ~0x10;
        input->UpdateMouseMode(0);
    }

    if (reason != 187 && cast->m_data.m_castCount && spell->m_spellMissileID > 0) {
        SpellMissileRec* missile = g_spellMissileDB.GetRecord(spell->m_spellMissileID);

        if (missile && (missile->m_flags & 0x1)) {
            unit->ClearPrecastMissileModel(cast->m_data.m_castCount);
            CMissile::StartFizzling(cast->m_data.m_caster, cast->m_data.m_castCount, cast->m_data.m_spellId);
        }
    }
}

// VERIFY: 107a9a37 manual
// OFFSET: 0x008054F0
void FinishPendingSpellCast(PendingSpellCast* cast, const SpellRec* spell, CGUnit_C* unit) {
    bool playerCast = false;

    if (cast->m_data.m_caster == ClntObjMgrGetActivePlayer() || cast->IsCharmedUnitCast()) {
        playerCast = true;

        if (spell->m_modalNextSpell) {
            int32_t nextSpell = spell->m_modalNextSpell;

            if (nextSpell == GetModalNextSpell()) {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                if (player && !player->IsInMelee() && !player->m_unit->m_unitFieldCharmedby.m_guid) {
                    player->OnAttackIconPressed(WOWGUID{ 0 }, 0);
                }
            } else {
                Spell_C_CastSpell(unit, nextSpell, nullptr, cast->m_data.m_objectTarget, nullptr, false);
            }
        }
    }

    ReleasePendingSpellCast(cast);

    if (playerCast && cast->m_data.m_castCount) {
        CGSpellBook::UpdateSelection();
    }
}

// VERIFY: 840e25a5 manual
// OFFSET: 0x00801220
bool Spell_C_HasCooldownOnEvent(const SpellRec* spell, WOWGUID guid) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__));

    if (item && (item->m_obj->m_type & TYPE_ITEM)) {
        CGUnit_C* owner = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(item->m_item->m_itemFieldOwner, TYPE_UNIT, __FILE__, __LINE__));
        ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, WOWGUID{ 0 }, nullptr, nullptr, false);
        int32_t slot = item->GetOnUseSlot();

        if (stats && owner && slot >= 0) {
            SpellCategoryRec* category = g_spellCategoryDB.GetRecord(stats->m_spellCategory[slot]);

            if (category && (category->m_flags & 0x4)) {
                return true;
            }
        }
    }

    return (spell->m_attributes & 0x2000000) != 0;
}

// VERIFY: 13e43b71 manual
// OFFSET: 0x007FEF60
void Spell_C_ModifyCategoryRecovery(int32_t& recovery, const SpellRec* spell, int32_t pet) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!pet) {
        SpellCategoryRec* category = g_spellCategoryDB.GetRecord(spell->m_category);

        if (category && (category->m_flags & 0x1)) {
            CGItem_C* item = player->GetBag()->GetItemPointer(15);

            if (item) {
                ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, WOWGUID{ 0 }, nullptr, nullptr, false);

                if (stats) {
                    recovery = recovery * static_cast<int32_t>(stats->m_delay) / 1000;
                }
            }
        }

        if ((spell->m_attributes & 0x2) && !(spell->m_attributesExB & 0x20000)) {
            recovery += player->m_unit->m_unitFieldRangedattacktime;
        }
    }

    if (!(spell->m_attributesExF & 0x80000000)) {
        Spell_C_ModifySpellValue(spell, recovery, 11);
    }
}

// VERIFY: fb43827f manual
// OFFSET: 0x00805230
void SpellHistory::AddHistory(int32_t spellId, int32_t itemId, uint32_t time, uint32_t recoveryTime, int32_t category, uint32_t categoryTime, uint32_t categoryRecoveryTime, bool onHold, int32_t startRecoveryCategory, uint32_t startRecoveryTime) {
    if (!recoveryTime && !categoryRecoveryTime && !onHold && !startRecoveryTime) {
        return;
    }

    SPELLHISTORY* history = this->m_free.Head();

    if (history) {
        this->m_free.UnlinkNode(history);
        this->m_history.LinkToTail(history);
    } else {
        history = this->m_history.NewNode(2, 0, 8);
    }

    history->m_spellId = spellId;
    history->m_itemId = itemId;
    history->m_time = time;
    history->m_recoveryTime = recoveryTime;
    history->m_category = category;
    history->m_categoryTime = categoryTime;
    history->m_categoryRecoveryTime = categoryRecoveryTime;
    history->m_onHold = onHold;
    history->m_startRecoveryCategory = startRecoveryCategory;
    history->m_startRecoveryTime = startRecoveryTime;
}

// VERIFY: ab11e157 manual
// OFFSET: 0x00802970
void SpellHistory::ClearOnHold(int32_t spellId, uint32_t time, bool remove) {
    SPELLHISTORY* next;

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = next) {
        next = this->m_history.Next(history);

        if (history->m_spellId != spellId) {
            continue;
        }

        if (history->m_onHold && !remove) {
            history->m_startRecoveryTime = 0;
            history->m_time = time;
            history->m_categoryTime = time;
            history->m_onHold = 0;
        } else if (remove) {
            history->m_link.Unlink();
            this->m_free.LinkToTail(history);
        }
    }
}

// VERIFY: b0e39839 manual
// OFFSET: 0x00809C70
int32_t Packet_SMSG_SPELL_FAILURE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID caster;
    *msg >> caster;
    uint8_t castCount;
    msg->Get(castCount);
    uint32_t spellId;
    msg->Get(spellId);
    uint8_t reason;
    msg->Get(reason);
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 1;
    }

    PendingSpellCast local;
    PendingSpellCast* cast = FindPendingSpellCast(caster, &local, castCount, spellId, caster);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        SI2::PlaySpellFizzleSound(spellId, unit);

        if (unit->IsActivePlayer() && (reason == 52 || reason == 53 || reason == 54)) {
            Spell_C_CancelAutoRepeat(true);
        }

        unit->SpellInterrupted(cast->m_data.m_castCount, cast->m_data.m_spellId);
    }

    CGUnit_C* casterUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (casterUnit) {
        HandleSpellFailedOrGo(casterUnit, cast, &spell, reason);
        FinishPendingSpellCast(cast, &spell, casterUnit);
    }

    return 1;
}

// VERIFY: a8100be0 manual
// OFFSET: 0x00806C30
int32_t Packet_SMSG_PET_CAST_FAILED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID pet;

    if (player) {
        pet = player->m_unit->m_unitFieldCharm.m_guid ? player->m_unit->m_unitFieldCharm : player->m_unit->m_unitFieldSummon;
    } else {
        pet = WOWGUID();
    }

    uint8_t castCount;
    uint32_t spellId;
    uint8_t reason;
    msg->Get(castCount);
    msg->Get(spellId);
    msg->Get(reason);
    uint32_t misc1 = -1;
    uint32_t misc2 = -1;

    if (msg->m_read < msg->m_size) {
        msg->Get(misc1);
    }

    if (msg->m_read < msg->m_size) {
        msg->Get(misc2);
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell)) {
        PendingSpellCast local;
        WOWGUID caster = pet;
        WOWGUID castItem = pet;
        local.m_isTrajectoryCast = 0;
        PendingSpellCast* cast = FindPendingSpellCast(castItem, &local, castCount, spellId, caster);
        s_spellHistory[1].RemoveStartCooldown(spellId);
        CGPetInfo::UpdateCooldowns();
        ShowPetSpellFailed(cast, &spell, reason, misc1, misc2);
    }

    return 1;
}

// VERIFY: b5828213 manual
// OFFSET: 0x00806DD0
int32_t Packet_SMSG_SPELL_COOLDOWN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    bool startRecovery = false;
    bool ignoreEvent = false;
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint8_t flags;
    msg->Get(flags);

    if (flags & 0x1) {
        startRecovery = true;
    }

    if (flags & 0x2) {
        ignoreEvent = true;
    }

    int32_t pet;

    if (guid == ClntObjMgrGetActivePlayer()) {
        pet = 0;
    } else {
        if (guid != CGPetInfo::GetPet(0)) {
            return 1;
        }

        pet = 1;
    }

    while (!msg->IsRead()) {
        uint32_t spellId;
        msg->Get(spellId);
        uint32_t cooldown;
        msg->Get(cooldown);
        SpellRec spell;

        if (!g_spellDB.GetRecord(spellId, &spell)) {
            continue;
        }

        bool onHold = ignoreEvent ? false : Spell_C_HasCooldownOnEvent(&spell, guid);
        int32_t flat;
        int32_t pct;
        int32_t recoveryTime;

        if (cooldown) {
            recoveryTime = cooldown;
        } else {
            recoveryTime = spell.m_recoveryTime;

            if (Spell_C_GetSpellModifiers(&spell, 11, flat, pct)) {
                recoveryTime = pct * (recoveryTime + flat) / 100;
            }
        }

        int32_t category = spell.m_category;
        int32_t categoryRecoveryTime;

        if (cooldown) {
            categoryRecoveryTime = 0;
        } else {
            categoryRecoveryTime = spell.m_categoryRecoveryTime;
            Spell_C_ModifyCategoryRecovery(categoryRecoveryTime, &spell, pet);
        }

        int32_t startRecoveryCategory;
        int32_t startRecoveryTime;

        if (!startRecovery || onHold) {
            startRecoveryCategory = 0;
            startRecoveryTime = 0;
        } else {
            startRecoveryCategory = spell.m_startRecoveryCategory;
            startRecoveryTime = spell.m_startRecoveryTime;

            if (Spell_C_GetSpellModifiers(&spell, 21, flat, pct)) {
                startRecoveryTime = pct * (startRecoveryTime + flat) / 100;
            }
        }

        s_spellHistory[pet].AddHistory(spellId, 0, time, recoveryTime, category, time, categoryRecoveryTime, onHold, startRecoveryCategory, startRecoveryTime);
    }

    CGSpellBook::UpdateCooldowns();

    if (pet) {
        CGPetInfo::UpdateCooldowns();
    } else {
        CGContainerInfo::UpdateCooldowns();
    }

    return 1;
}

// VERIFY: e66e3d66 manual
// OFFSET: 0x00809AF0
int32_t Packet_SMSG_CAST_FAILED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID player = ClntObjMgrGetActivePlayer();
    uint8_t castCount;
    msg->Get(castCount);
    uint32_t spellId;
    msg->Get(spellId);
    uint8_t reason;
    msg->Get(reason);
    int32_t customError = -1;
    int32_t customArg = -1;

    if (msg->m_read < msg->m_size) {
        msg->Get(reinterpret_cast<uint32_t&>(customError));
    }

    if (msg->m_read < msg->m_size) {
        msg->Get(reinterpret_cast<uint32_t&>(customArg));
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 1;
    }

    PendingSpellCast local;
    PendingSpellCast* cast = FindPendingSpellCast(player, &local, castCount, spellId, player);
    Spell_C_SpellFailed(cast, &spell, reason, customError, customArg, true);

    if (reason != SPELL_FAILED_NOT_READY && (spell.m_attributes & 0x2000000)) {
        s_spellHistory[0].ClearOnHold(spellId, 0, true);
        CGSpellBook::UpdateCooldowns();
        CGSpellBook::UpdateUsable();
        CGContainerInfo::UpdateCooldowns();
    }

    return 1;
}

// VERIFY: ca363f59 manual
// OFFSET: 0x007FD830
bool PendingSpellCast::HasValidTransports() const {
    C44Matrix matrix;

    if (this->m_data.m_destTarget.m_guid && !MovementGetTransportMtxSafe(this->m_data.m_destTarget, &matrix)) {
        return false;
    }

    if (this->m_data.m_sourceTarget.m_guid && !MovementGetTransportMtxSafe(this->m_data.m_sourceTarget, &matrix)) {
        return false;
    }

    return true;
}

// VERIFY: a5f8984e manual
// OFFSET: 0x00804B20
void Spell_C_AddToInTransitList(WOWGUID caster, const SpellRec* spell, uint32_t serverTime, uint32_t arrivalTime) {
    if (!spell || spell->m_speed <= 0.0f) {
        return;
    }

    void* memory = SMemAlloc(sizeof(SpellInTransit), __FILE__, __LINE__, 0);
    SpellInTransit* node = memory ? new (memory) SpellInTransit() : nullptr;
    node->m_caster = caster;
    node->m_spellId = spell->m_id;
    node->m_serverTime = serverTime;
    node->m_arrivalTime = arrivalTime;
    SpellInTransit* before = nullptr;

    for (SpellInTransit* entry = s_spellsInTransit.Head(); entry; entry = s_spellsInTransit.Next(entry)) {
        if (static_cast<int32_t>(entry->m_arrivalTime - arrivalTime) >= 0) {
            before = entry;
            break;
        }
    }

    s_spellsInTransit.LinkNode(node, 2, before);
}

// VERIFY: 7d01b8cd manual
// OFFSET: 0x00802B00
void SpellHistory::GarbageCollect(uint32_t time) {
    SPELLHISTORY* next;

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = next) {
        next = this->m_history.Next(history);

        if (history->m_onHold) {
            continue;
        }

        if (history->m_recoveryTime && static_cast<int32_t>(time - history->m_time - history->m_recoveryTime) < 0) {
            continue;
        }

        if (history->m_categoryRecoveryTime && static_cast<int32_t>(time - history->m_categoryTime - history->m_categoryRecoveryTime) < 0) {
            continue;
        }

        history->m_link.Unlink();
        this->m_free.LinkToTail(history);
    }
}

// VERIFY: 67dd3e6d manual
// OFFSET: 0x0080E1B0
void PendingSpellCast::OnSpellGo(CDataStore* msg) {
    WOWGUID castItem = this->m_data.m_castItem;
    WOWGUID caster = this->m_data.m_caster;
    int32_t spellId = this->m_data.m_spellId;
    uint32_t castFlags = this->m_data.m_castFlags;
    uint32_t serverTime = 0;
    msg->Get(serverTime);

    uint8_t hitCount = 0;
    msg->Get(hitCount);
    TSStackArray<WOWGUID> hits(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * hitCount)), hitCount, 1);

    for (uint32_t i = 0; i < hitCount; i++) {
        msg->Get(hits.m_data[i].m_guid);
    }

    uint8_t missCount = 0;
    msg->Get(missCount);
    TSStackArray<WOWGUID> misses(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * missCount)), missCount, 1);
    TSStackArray<uint32_t> missReasons(static_cast<uint32_t*>(alloca(sizeof(uint32_t) * missCount)), missCount, 1);
    TSStackArray<uint32_t> reflectResults(static_cast<uint32_t*>(alloca(sizeof(uint32_t) * missCount)), missCount, 1);

    for (uint32_t i = 0; i < missCount; i++) {
        msg->Get(misses.m_data[i].m_guid);
        uint8_t value = 0;
        msg->Get(value);
        missReasons.m_data[i] = value;

        if (missReasons.m_data[i] == 11) {
            value = 0;
            msg->Get(value);
            reflectResults.m_data[i] = value;
        }
    }

    SpellGetCastTargets(&this->m_data, msg);
    uint32_t power = 0;
    uint32_t hasPower = castFlags & 0x800;

    if (hasPower) {
        msg->Get(power);
    }

    uint8_t runeCooldowns[6] = { 0, 0, 0, 0, 0, 0 };
    uint8_t runesBefore = 0;
    uint8_t runesAfter = 0;
    uint32_t hasRunes = castFlags & 0x200000;

    if (hasRunes) {
        msg->Get(runesBefore);
        msg->Get(runesAfter);

        for (uint32_t i = 0; i < 6; i++) {
            if (((1 << i) & runesBefore) && !((1 << i) & runesAfter)) {
                msg->Get(runeCooldowns[i]);
            }
        }
    }

    uint32_t delay = 0;

    if (castFlags & 0x20000) {
        msg->Get(this->m_data.m_elevation);
        msg->Get(delay);
    }

    uint32_t ammoDisplayId;
    uint32_t ammoInventoryType;

    if (castFlags & 0x20) {
        msg->Get(ammoDisplayId);
        msg->Get(ammoInventoryType);
    } else {
        ammoDisplayId = 0;
        ammoInventoryType = 0;
    }

    uint32_t visualA = 0;
    uint32_t visualB = 0;

    if (castFlags & 0x80000) {
        msg->Get(visualA);
        msg->Get(visualB);
    }

    uint32_t now = OsGetAsyncTimeMs();

    if (this->m_data.m_targetFlags & 0x40) {
        uint8_t count = 0;
        msg->Get(count);

        if (count) {
            Spell_C_AddProcessedDestLocSpellCast(caster, count, now);
        }
    }

    uint32_t pointCount = 0;

    if (this->m_data.m_targetFlags & 0x80000) {
        msg->Get(pointCount);
    }

    TSStackArray<SpellCastTargetPoint> points(static_cast<SpellCastTargetPoint*>(alloca(sizeof(SpellCastTargetPoint) * pointCount)), pointCount, 1);

    if (this->m_data.m_targetFlags & 0x80000) {
        uint32_t valid = 0;

        for (uint32_t i = 0; i < pointCount; i++) {
            SpellCastTargetPoint& point = points.m_data[valid];
            msg->GetArray(reinterpret_cast<uint32_t*>(&point.m_position), 3);
            msg->Get(point.m_transport.m_guid);

            if (point.m_transport.m_guid) {
                C44Matrix matrix;

                if (!MovementGetTransportMtxX(point.m_transport, &matrix, WOWGUID{ 0 }, __FILE__, __LINE__)) {
                    continue;
                }

                point.m_worldPosition = point.m_position * matrix;
            } else {
                point.m_worldPosition = point.m_position;
            }

            valid++;
        }

        points.SetCount(valid);
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_data.m_spellId, &spell)) {
        ReleasePendingSpellCast(this);
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!this->HasValidTransports()) {
        if (!(this->m_data.m_castFlags & 0x1) && unit) {
            FinishPendingSpellCast(this, &spell, unit);
        } else {
            ReleasePendingSpellCast(this);
        }

        return;
    }

    if (unit && hasPower) {
        if (spell.m_powerType == POWER_TYPE_HEALTH) {
            if (!(unit->m_unit->m_unitFieldFlags2 & 0x100)) {
                unit->SetPredictedHealth(power);
            }
        } else if (spell.m_powerType == POWER_TYPE_RUNES) {
            if (caster == ClntObjMgrGetActivePlayer() && hasRunes) {
                CGRuneInfo::SetPredictedRunes(runesBefore, runesAfter, runeCooldowns);
            }
        } else {
            if (!(unit->m_unit->m_unitFieldFlags2 & 0x100)) {
                unit->SetPredictedPower(static_cast<POWER_TYPE>(spell.m_powerType), power);
            }

            if (spell.m_powerType == POWER_TYPE_MANA) {
                unit->SetManaRegenInterrupt();
            }
        }
    }

    uint32_t hidden = castFlags & 0x1;

    if (!hidden) {
        s_lastFailedSpellId = 0;
        HandleSpellFailedOrGo(unit, this, &spell, 187);
    }

    uint32_t arrival = now;
    C3Vector destLocation = this->m_data.GetDestLocation();

    if (unit && spell.m_speed != 0.0f) {
        if (this->m_data.m_targetFlags & 0x40) {
            C3Vector position = unit->GetPosition();
            float dx = position.x - destLocation.x;
            float dy = position.y - destLocation.y;
            float dz = position.z - destLocation.z;
            arrival = now - static_cast<int32_t>(sqrtf(dx * dx + dy * dy + dz * dz) / spell.m_speed * -1000.0f);
            Spell_C_AddToInTransitList(caster, &spell, serverTime, arrival);
        } else if (hitCount) {
            CGObject_C* target = ClntObjMgrObjectPtr(hits.m_data[0], TYPE_OBJECT, __FILE__, __LINE__);

            if (target) {
                C3Vector targetPosition = target->GetPosition();
                C3Vector position = unit->GetPosition();
                float dx = position.x - targetPosition.x;
                float dy = position.y - targetPosition.y;
                float dz = position.z - targetPosition.z;
                arrival = now - static_cast<int32_t>(sqrtf(dx * dx + dy * dy + dz * dz) / spell.m_speed * -1000.0f);
                Spell_C_AddToInTransitList(caster, &spell, serverTime, arrival);
            }
        }

        if (this->m_data.m_targetFlags & 0x80000) {
            for (uint32_t i = 0; i < points.m_count; i++) {
                const SpellCastTargetPoint& point = points.m_data[i];
                C3Vector position = unit->GetPosition();
                float dx = position.x - point.m_worldPosition.x;
                float dy = position.y - point.m_worldPosition.y;
                float dz = position.z - point.m_worldPosition.z;
                arrival = now - static_cast<int32_t>(sqrtf(dx * dx + dy * dy + dz * dz) / spell.m_speed * -1000.0f);
                Spell_C_AddToInTransitList(caster, &spell, serverTime, arrival);
            }
        }
    }

    CGObject_C* castObject = ClntObjMgrObjectPtr(castItem, TYPE_OBJECT, __FILE__, __LINE__);
    WOWGUID source = castObject && (castObject->m_obj->m_type & (TYPE_UNIT | TYPE_GAMEOBJECT)) ? castItem : caster;

    if (!hidden || (spell.m_attributesExG & 0x80000000)) {
        UnitCombatLogCastGo(caster, this->m_data.m_objectTarget, spellId);
    } else if (spell.m_attributesExF & 0x40) {
        UnitCombatLogProcGo(castItem, &spell);
    }

    if (spell.m_speed == 0.0f && !(spell.m_attributesExB & 0x80)) {
        for (uint32_t i = 0; i < missCount; i++) {
            UnitCombatLogSpellMissed(source, misses.m_data[i], spellId, missReasons.m_data[i], false, false, 0);

            if (!(spell.m_attributes & 0x100)) {
                CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(misses.m_data[i], TYPE_UNIT, __FILE__, __LINE__));

                if (target) {
                    target->AddWorldText(source, static_cast<MISS_REASON>(missReasons.m_data[i]), &spell, false);
                }

                CGGameUI::ShowSpellMissFeedback(misses.m_data[i], missReasons.m_data[i]);
            }

            if (missReasons.m_data[i] == 11 && reflectResults.m_data[i]) {
                UnitCombatLogSpellMissed(source, caster, spellId, reflectResults.m_data[i], false, false, 0);

                if (!(spell.m_attributes & 0x100)) {
                    CGUnit_C* reflector = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

                    if (reflector) {
                        reflector->AddWorldText(source, static_cast<MISS_REASON>(reflectResults.m_data[i]), &spell, false);
                    }

                    CGGameUI::ShowSpellMissFeedback(caster, reflectResults.m_data[i]);
                }
            }

            CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(misses.m_data[i], TYPE_UNIT, __FILE__, __LINE__));

            if (target) {
                target->PlayCombatSpellMissEffects(missReasons.m_data[i], source);
            }
        }
    }

    bool onHold = Spell_C_HasCooldownOnEvent(&spell, castItem);
    TSStackArray<WOWGUID> targets(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * (missCount + hitCount))), missCount + hitCount, 1);

    if (castFlags & 0x400) {
        for (uint32_t i = 0; i < missCount; i++) {
            targets.m_data[i] = misses.m_data[i];
        }

        for (uint32_t i = 0; i < hitCount; i++) {
            targets.m_data[missCount + i] = hits.m_data[i];
        }
    } else {
        for (uint32_t i = 0; i < hitCount; i++) {
            targets.m_data[i] = hits.m_data[i];
        }

        for (uint32_t i = 0; i < missCount; i++) {
            targets.m_data[hitCount + i] = misses.m_data[i];
        }
    }

    SpellVisualRec visualRec;
    const SpellVisualRec* visual;

    if (unit) {
        visual = unit->GetAppropriateSpellVisual(&spell, visualRec, visualA, visualB);

        if (visual && Spell_C_SpellVisualHasChainEffect(visual)) {
            unit->SaveChainSpellTargets(targets);
            bool chainLocation = (visual->m_flags & 0x1) && targets.m_count;

            if ((this->m_data.m_targetFlags & 0x40) && !chainLocation) {
                unit->SaveChainSpellLocation(&destLocation);
            } else {
                unit->SaveChainSpellLocation(nullptr);
            }
        }
    } else {
        visual = GetSpellVisual(&spell);
    }

    if (visual) {
        SpellVisualKitRec* castKit = g_spellVisualKitDB.GetRecord(visual->m_castKit);

        if (castKit && unit && !this->m_isTrajectoryCast) {
            PlaySpellVisuaKitData kitData(&spell, castKit, 0);
            unit->PlaySpellVisualKit(kitData);
        }

        if (this->m_data.m_targetFlags & 0x40) {
            SpellVisualKitRec* instantKit = g_spellVisualKitDB.GetRecord(visual->m_instantAreaKit);

            if (instantKit && unit) {
                PlaySpellVisuaKitData kitData(&spell, instantKit, 3);
                kitData.m_position = &this->m_data.m_destPos;
                kitData.m_transportGUID = this->m_data.m_destTarget;
                unit->PlaySpellVisualKit(kitData);
            }

            int32_t impactKitId = visual->m_impactAreaKit;
            SpellVisualKitRec* impactKit = g_spellVisualKitDB.GetRecord(impactKitId);

            if (impactKit && (!visual->m_hasMissile || spell.m_speed <= 0.0f) && !Spell_C_SpellVisualHasChainEffect(visual)) {
                if (unit) {
                    if (spell.m_speed > 0.0f) {
                        unit->DelaySpellVisualKit(spellId, impactKitId, 3, &this->m_data.m_destPos, this->m_data.m_destTarget, 0, 0, -1, 1, arrival);
                    } else {
                        PlaySpellVisuaKitData kitData(&spell, impactKit, 3);
                        kitData.m_position = &this->m_data.m_destPos;
                        kitData.m_transportGUID = this->m_data.m_destTarget;
                        unit->PlaySpellVisualKit(kitData);
                    }
                } else if (ClntObjMgrObjectPtr(castItem, TYPE_GAMEOBJECT, __FILE__, __LINE__)) {
                    NotifyDestLocSpellCastData data;
                    data.m_caster = castItem;
                    data.m_destTarget = this->m_data.m_destTarget;
                    data.m_spellId = spellId;
                    data.m_destLocation = destLocation;
                    data.m_destPosition = this->m_data.m_destPos;
                    data.m_elevation = this->m_data.m_elevation;
                    data.m_missileSpeed = this->m_data.m_missileSpeed;
                    data.m_delay = 0;
                    data.unk_0038 = 0;
                    data.m_castCount = this->m_data.m_castCount;
                    Spell_C_PlayDestLocVisualsWithoutCaster(&data);
                }
            }
        }

        if (this->m_data.m_targetFlags & 0x80000) {
            for (uint32_t i = 0; i < points.m_count; i++) {
                SpellCastTargetPoint& point = points.m_data[i];
                SpellVisualKitRec* instantKit = g_spellVisualKitDB.GetRecord(visual->m_instantAreaKit);

                if (instantKit && unit) {
                    PlaySpellVisuaKitData kitData(&spell, instantKit, 3);
                    kitData.m_position = &point.m_position;
                    kitData.m_transportGUID = point.m_transport;
                    unit->PlaySpellVisualKit(kitData);
                }

                int32_t impactKitId = visual->m_impactAreaKit;
                SpellVisualKitRec* impactKit = g_spellVisualKitDB.GetRecord(impactKitId);

                if (!impactKit || (visual->m_hasMissile && spell.m_speed > 0.0f) || Spell_C_SpellVisualHasChainEffect(visual)) {
                    continue;
                }

                if (unit) {
                    if (spell.m_speed > 0.0f) {
                        unit->DelaySpellVisualKit(spellId, impactKitId, 3, &point.m_position, point.m_transport, 0, 0, -1, 1, arrival);
                    } else {
                        PlaySpellVisuaKitData kitData(&spell, impactKit, 3);
                        kitData.m_position = &point.m_position;
                        kitData.m_transportGUID = point.m_transport;
                        unit->PlaySpellVisualKit(kitData);
                    }
                } else if (ClntObjMgrObjectPtr(castItem, TYPE_GAMEOBJECT, __FILE__, __LINE__)) {
                    NotifyDestLocSpellCastData data;
                    data.m_caster = castItem;
                    data.m_destTarget = WOWGUID{ 0 };
                    data.m_spellId = spellId;
                    data.m_destLocation = point.m_worldPosition;
                    data.m_destPosition = point.m_worldPosition;
                    data.m_elevation = this->m_data.m_elevation;
                    data.m_missileSpeed = this->m_data.m_missileSpeed;
                    data.m_delay = 0;
                    data.unk_0038 = 0;
                    data.m_castCount = this->m_data.m_castCount;
                    Spell_C_PlayDestLocVisualsWithoutCaster(&data);
                }
            }
        }

        if (spell.m_speed <= 0.0f) {
            PlayImpactVisual(&spell, hits, unit, visual);

            if (unit) {
                for (uint32_t i = 0; i < missCount; i++) {
                    if (missReasons.m_data[i] == 11 && !reflectResults.m_data[i] && !(spell.m_attributesExG & 0x20000000)) {
                        PlayImpactVisualOnTarget(&spell, visual, unit, unit);
                    }
                }
            }
        } else if (unit) {
            if (this->m_isTrajectoryCast) {
                if (this->m_data.m_castFlags & 0x20000) {
                    CMissile::AdjustAllMissileTrajectoryDurations(this->m_data.m_castCount, delay);
                }
            } else {
                if (spell.m_attributesExD & 0x40000) {
                    LaunchMissileVisual(hits, misses, unit, &spell, visual, ammoDisplayId, ammoInventoryType, &this->m_data, missReasons, reflectResults, delay);

                    if ((visual->m_hasMissile || ammoDisplayId) && points.m_count) {
                        unit->CreateMissile(&points, &this->m_data, ammoDisplayId, ammoInventoryType, visual, spell.m_id, delay);
                    }
                } else {
                    CreateMissilesForTargets(hits, unit, &spell, visual, ammoDisplayId, ammoInventoryType, &this->m_data, nullptr, nullptr, delay);

                    if (misses.m_count) {
                        CreateMissilesForTargets(misses, unit, &spell, visual, ammoDisplayId, ammoInventoryType, &this->m_data, &missReasons, &reflectResults, delay);
                    }

                    CreateMissilesForTargetPoints(points, unit, &spell, visual, ammoDisplayId, ammoInventoryType, &this->m_data, delay);
                }

                if (visual->m_hasMissile && (!castKit || castKit->m_animID <= 0)) {
                    unit->ReleaseMissiles();
                }
            }
        }
    }

    if (caster == ClntObjMgrGetActivePlayer()) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(unit);

        if (castItem == ClntObjMgrGetActivePlayer() && !(castFlags & 0x41)) {
            CGTradeSkillInfo::DoRecast(spellId);
        }

        for (uint32_t effect = 0; effect < 3; effect++) {
            if (spell.m_effect[effect] != SPELL_EFFECT_OPEN_LOCK) {
                continue;
            }

            if (hits.m_count != 1) {
                break;
            }

            CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(hits.m_data[0], TYPE_GAMEOBJECT, __FILE__, __LINE__));

            if (!gameObject) {
                break;
            }

            int8_t type = gameObject->m_gameObject->m_typeID;

            if (type < 0 || (type > 1 && type != 3)) {
                break;
            }

            if (type == 3) {
                player->OnLootGameObject(hits.m_data[0]);
            }

            int32_t action = -1;
            LockRec* lock = gameObject->GetLockRec();

            if (!lock) {
                break;
            }

            for (uint32_t i = 0; i < 3; i++) {
                if (spell.m_effect[i] != SPELL_EFFECT_OPEN_LOCK) {
                    continue;
                }

                for (uint32_t j = 0; j < 8 && action < 0; j++) {
                    switch (lock->m_type[j]) {
                        case 1:
                            if ((castObject->m_obj->m_type & TYPE_ITEM) && castObject->m_obj->m_entryID == lock->m_index[j]) {
                                action = lock->m_action[j];
                            }

                            break;
                        case 2:
                            if (spell.m_effectMiscValue[i] == lock->m_index[j]) {
                                action = lock->m_action[j];
                            }

                            break;
                        case 3:
                            if (spell.m_id == lock->m_index[j]) {
                                action = lock->m_action[j];
                            }

                            break;
                    }
                }
            }

            if (action >= 0) {
                if (action <= 1) {
                    gameObject->SetDisplayState(0);
                } else if (action == 2) {
                    gameObject->SetDisplayState(1);
                }
            }

            break;
        }

        for (uint32_t i = 0; i < 3; i++) {
            int32_t effect = spell.m_effect[i];

            if (effect == SPELL_EFFECT_PICKPOCKET || effect == SPELL_EFFECT_SKINNING || effect == SPELL_EFFECT_SKIN_PLAYER_CORPSE || ((effect == SPELL_EFFECT_DISENCHANT || effect == SPELL_EFFECT_PROSPECTING || effect == SPELL_EFFECT_MILLING) && CGGameUI::IsAutoLooting())) {
                if (!player->m_autoLoot) {
                    player->SetAutoLoot(g_autoLootDefaultCVar->m_intValue);
                }

                break;
            }
        }

        if (((spell.m_attributesExB & 0x100000) || (((spell.m_attributes & 0x404) || (spell.m_attributesEx & 0x200) || (spell.m_attributesExB & 0x100000)) && (spell.m_interruptFlags & 0x8))) && !player->m_combat.m_attacking.m_guid) {
            player->OnAttackIconPressed(hits.m_count ? hits.m_data[0] : WOWGUID{ 0 }, 0);
        }

        int32_t itemId = 0;
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(castItem, TYPE_ITEM, __FILE__, __LINE__));

        if (item) {
            itemId = item->m_obj->m_entryID;
        }

        if (castItem == caster) {
            int32_t categoryRecovery = spell.m_categoryRecoveryTime;
            Spell_C_ModifyCategoryRecovery(categoryRecovery, &spell, 0);
            int32_t recovery = spell.m_recoveryTime;
            int32_t flat;
            int32_t pct;

            if (Spell_C_GetSpellModifiers(&spell, 11, flat, pct)) {
                recovery = pct * (flat + recovery) / 100;
            }

            s_spellHistory[0].AddHistory(spellId, 0, now, recovery, spell.m_category, now, categoryRecovery, onHold, 0, 0);
            CGSpellBook::UpdateCooldowns();

            if (onHold) {
                CGSpellBook::UpdateUsable();
            }

            CGContainerInfo::UpdateCooldowns();
        } else if (itemId) {
            if (!g_itemDBCache.GetRecord(itemId, castItem, &Spell_C_PendingItemCooldownCallback, reinterpret_cast<void*>(1), false)) {
                Spell_C_AddPendingItemCooldown(itemId, spellId, now, onHold);
                return;
            }

            if (item->GetEnchantmentSpell(0) == spellId) {
                uint32_t cooldown;
                uint32_t categoryCooldown;
                uint32_t category;
                item->GetSpellCooldownInfo(cooldown, categoryCooldown, category);

                if (static_cast<int32_t>(cooldown) < 0) {
                    cooldown = spell.m_recoveryTime;
                }

                if (static_cast<int32_t>(category) <= 0) {
                    category = spell.m_category;
                }

                if (static_cast<int32_t>(categoryCooldown) < 0) {
                    categoryCooldown = spell.m_categoryRecoveryTime;
                }

                s_spellHistory[0].AddHistory(spellId, itemId, now, cooldown, category, now, categoryCooldown, onHold, 0, 0);
            }

            CGSpellBook::UpdateCooldowns();

            if (onHold) {
                CGSpellBook::UpdateUsable();
            }

            CGContainerInfo::UpdateCooldowns();
        }
    }

    if (unit) {
        const WOWGUID& controller = unit->m_unit->m_unitFieldCharmedby.m_guid ? unit->m_unit->m_unitFieldCharmedby : unit->m_unit->m_unitFieldSummonedby;

        if (controller == ClntObjMgrGetActivePlayer()) {
            int32_t recovery = spell.m_recoveryTime;
            int32_t categoryRecovery = spell.m_categoryRecoveryTime;
            int32_t flat;
            int32_t pct;

            if (Spell_C_GetSpellModifiers(&spell, 11, flat, pct)) {
                recovery = pct * (flat + recovery) / 100;
            }

            if (!(spell.m_attributesExF & 0x80000000) && Spell_C_GetSpellModifiers(&spell, 11, flat, pct)) {
                categoryRecovery = pct * (flat + spell.m_categoryRecoveryTime) / 100;
            }

            int32_t startRecoveryTime = 0;
            int32_t startRecoveryCategory = 0;

            if (!(this->m_data.m_castFlags & 0x40000)) {
                startRecoveryTime = spell.m_startRecoveryTime;
                startRecoveryCategory = spell.m_startRecoveryCategory;

                if (startRecoveryTime || startRecoveryCategory) {
                    if (startRecoveryCategory == 133 && startRecoveryTime == 1500 && spell.m_defenseType != 2 && spell.m_defenseType != 3 && !(spell.m_attributes & 0x12)) {
                        float scaled = unit->m_unit->m_unitModCastSpeed * 1500.0f;
                        startRecoveryTime = CMath::fint_(scaled - 0.5f);
                    }

                    if (Spell_C_GetSpellModifiers(&spell, 21, flat, pct)) {
                        startRecoveryTime = pct * (startRecoveryTime + flat) / 100;
                    }

                    if (static_cast<uint32_t>(startRecoveryTime - 1000) > 500) {
                        startRecoveryTime = startRecoveryTime - 1000 < 0 ? 1000 : 1500;
                    }
                }
            }

            s_spellHistory[1].AddHistory(spellId, 0, now, recovery, spell.m_category, now, categoryRecovery, onHold, startRecoveryCategory, startRecoveryTime);
            CGSpellBook::UpdateCooldowns();

            if (onHold) {
                CGSpellBook::UpdateUsable();
            }

            CGPetInfo::UpdateCooldowns();
        }
    }

    if (static_cast<int32_t>(now - s_nextHistoryCleanup) >= 0) {
        s_nextHistoryCleanup = now + 120000;

        for (uint32_t i = 0; i < sizeof(s_spellHistory) / sizeof(s_spellHistory[0]); i++) {
            s_spellHistory[i].GarbageCollect(now);
        }
    }

    if (!(this->m_data.m_castFlags & 0x1) && unit) {
        FinishPendingSpellCast(this, &spell, unit);
    } else {
        ReleasePendingSpellCast(this);
    }
}

// VERIFY: 2af0e265 manual
// OFFSET: 0x007FDE00
uint32_t Spell_C_GetPowerDisplayMod(POWER_TYPE powerType) {
    if (static_cast<int32_t>(powerType) < 0) {
        return 1;
    }

    return s_powerDisplayMod[powerType];
}

// VERIFY: f2862d5e auto
// OFFSET: 0x007FE4B0
const char* GetPowerString(POWER_TYPE powerType) {
    const char* tokens[NUM_POWER_TYPES] = { "MANA", "RAGE", "FOCUS", "ENERGY", "HAPPINESS", "RUNES", "RUNIC_POWER" };

    if (static_cast<uint32_t>(powerType) < sizeof(tokens) / sizeof(tokens[0])) {
        return tokens[powerType];
    }

    return "";
}

// VERIFY: a2eb54f0 auto
// OFFSET: 0x007FA290
const SpellVisualRec* GetSpellVisual(const SpellRec* spell) {
    int32_t visualId = spell->m_spellVisualID[0];

    if (ViolenceGetLevel() < VIOLENCELEVEL_HIGH && spell->m_spellVisualID[1]) {
        visualId = spell->m_spellVisualID[1];
    }

    return g_spellVisualDB.GetRecord(visualId);
}

// VERIFY: 8f243d8d manual
// OFFSET: 0x007FD750
bool Spell_C_CanTargetTerrain() {
    return (s_needTargets & 0x60) != 0;
}

// VERIFY: 4bd9ca55 auto
// OFFSET: 0x007FD7F0
int32_t Spell_C_IsCursorWorldObjectHousing() {
    return s_cursorWorldObjectHousing;
}

// VERIFY: 53c745f7 auto
// OFFSET: 0x007FE1A0
void SpellSetFeedPetSpell(int32_t spellId) {
    s_feedPetSpell = spellId;
}

// VERIFY: 54519a8f manual
// OFFSET: 0x007FD650
bool Spell_C_CanTargetUnits() {
    return (s_needTargets & 0x1878E) != 0;
}

// VERIFY: 80b9e977 manual
// OFFSET: 0x007FD670
bool Spell_C_CanTargetParty() {
    return (s_needTargets & 0x408) != 0;
}

// VERIFY: d32e5cfd manual
// OFFSET: 0x007FD690
bool Spell_C_CanTargetRaid() {
    return (s_needTargets & 0x404) != 0;
}

// VERIFY: d2ed91ed manual
// OFFSET: 0x007FD6B0
bool Spell_C_CanTargetFriends() {
    return (s_needTargets & 0x500) != 0;
}

// VERIFY: f50aa77f manual
// OFFSET: 0x007FD6D0
bool Spell_C_CanTargetEnemies() {
    return (s_needTargets & 0x480) != 0;
}

// VERIFY: 3a1d8303 manual
// OFFSET: 0x007FD6F0
bool Spell_C_CanTargetAlive() {
    return (s_needTargets & 0x8600) == 0;
}

// VERIFY: f7300290 auto
// OFFSET: 0x007FD710
bool Spell_C_CanTargetFriendCorpses() {
    return (s_needTargets >> 15) & 1;
}

// VERIFY: 9cd298b5 auto
// OFFSET: 0x007FD720
bool Spell_C_CanTargetEnemyCorpses() {
    return (s_needTargets >> 9) & 1;
}

// VERIFY: 18b19838 manual
// OFFSET: 0x007FD730
bool Spell_C_CanTargetObjects() {
    return (s_needTargets & 0x4800) != 0;
}

// VERIFY: f1612f08 auto
// OFFSET: 0x007FD780
bool Spell_C_CanTargetNonCombatPet() {
    return (s_needTargets >> 16) & 1;
}

// VERIFY: 238bd596 manual
// OFFSET: 0x007FFEA0
bool Spell_C_CanTargetCorpse(const CGCorpse_C* corpse) {
    if (!s_targetingSpell) {
        return false;
    }

    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_targetingSpell->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return false;
    }

    if (s_needTargets & 0x8000) {
        return !(corpse->m_corpse->m_corpseFieldFlags & 1) && caster->CanAssist(corpse);
    }

    if (s_needTargets & 0x200) {
        return !(corpse->m_corpse->m_corpseFieldFlags & 1) && !caster->CanAssist(corpse);
    }

    return false;
}

// VERIFY: f486f02f manual
// OFFSET: 0x007FFF20
bool Spell_C_CanTargetObject(const CGGameObject_C* object) {
    return (s_needTargets & 0x4800) && (object->m_obj->m_type & TYPE_GAMEOBJECT) && object->IsValidTargetForSpell(s_targetingSpell->m_data.m_castItem, s_targetingSpell->m_data.m_spellId);
}

// VERIFY: 6491424d auto
// OFFSET: 0x00801880
bool Spell_C_CanTargetMe() {
    if (s_needTargets & 0x850E) {
        SpellRec spell;
        g_spellDB.GetRecord(s_targetingSpell->m_data.m_spellId, &spell);

        if (!(spell.m_attributesEx & 0x80000)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 665b5e31 manual
// OFFSET: 0x008018F0
bool Spell_C_CanTargetDead() {
    if (!s_targetingSpell) {
        return false;
    }

    if (s_needTargets & 0x8600) {
        return true;
    }

    SpellRec spell;
    g_spellDB.GetRecord(s_targetingSpell->m_data.m_spellId, &spell);
    return (spell.m_attributesExB & 1) != 0;
}

// VERIFY: 3e9a6e5e manual
// OFFSET: 0x00801960
bool Spell_C_CanTargetPossessedFriends() {
    if (!s_targetingSpell) {
        return false;
    }

    SpellRec spell;
    g_spellDB.GetRecord(s_targetingSpell->m_data.m_spellId, &spell);
    return (spell.m_attributesExF & 0x200) != 0;
}

// VERIFY: 1509504f manual
// OFFSET: 0x00801B10
bool Spell_C_TargetingSpellAllowsTerrain() {
    PendingSpellCast* targeting = s_targetingSpell;

    if (!targeting) {
        return false;
    }

    SpellRec spell;
    return g_spellDB.GetRecord(targeting->m_data.m_spellId, &spell) && (spell.m_attributesExF & 0x1000000);
}

// VERIFY: 50b87c34 manual
// OFFSET: 0x007FFC40
CGUnit_C* Spell_C_TargetImpliedTarget(CGUnit_C* target, const SpellRec* spell, uint32_t needTargets, CGUnit_C* caster) {
    if (needTargets & 0x10C) {
        if (!caster->CanAttack(target)) {
            return target;
        }

        CGUnit_C* implied = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target->m_unit->m_unitFieldTarget, TYPE_UNIT, __FILE__, __LINE__));
        bool ignoreFlags = spell ? (spell->m_attributesExF >> 3) & 1 : false;

        if (!implied || !caster->CanAssist(implied, ignoreFlags)) {
            return nullptr;
        }

        if ((needTargets & 0x8) && caster->IsInMyPartyOrRaid(implied)) {
            return implied;
        }

        if ((needTargets & 0x4) && caster->IsInMyParty(implied)) {
            return implied;
        }

        if (needTargets & 0x100) {
            return implied;
        }

        return nullptr;
    }

    if (needTargets & 0x80) {
        bool ignoreFlags = spell ? (spell->m_attributesExF >> 3) & 1 : false;

        if (caster->CanAssist(target, ignoreFlags)) {
            CGUnit_C* implied = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target->m_unit->m_unitFieldTarget, TYPE_UNIT, __FILE__, __LINE__));

            if (!implied || !caster->CanAttack(implied)) {
                return nullptr;
            }

            return implied;
        }
    }

    return target;
}

// VERIFY: 30711268 manual
// OFFSET: 0x00801650
void Spell_C_GetMinMaxRange(CGUnit_C* caster, const SpellRec* spell, float* minRange, float* maxRange, CGObject_C* target) {
    *minRange = 0.0f;
    *maxRange = 0.0f;

    if (!caster) {
        return;
    }

    int32_t friendly;

    if (target && (target->m_obj->m_type & TYPE_UNIT)) {
        friendly = caster->CanAssist(static_cast<CGUnit_C*>(target), false);
    } else {
        friendly = SpellRec::IsHelpfulOrHarmful(spell) == 1;
    }

    Spell_C_GetMinMaxRange(caster, spell, minRange, maxRange, friendly, target);
}

// VERIFY: 97e96f86 manual
// OFFSET: 0x00802C30
void Spell_C::GetSpellRange(CGUnit_C* caster, int32_t spellId, float* minRange, float* maxRange, CGObject_C* target) {
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell)) {
        Spell_C_GetMinMaxRange(caster, &spell, minRange, maxRange, target);
    } else {
        *minRange = 0.0f;
        *maxRange = 0.0f;
    }
}

// VERIFY: e951a560 manual
// OFFSET: 0x00803A50
bool Spell_C::HandleSpriteRay(const WOWGUID& guid, bool checkRange) {
    PendingSpellCast* targeting = s_targetingSpell;

    if (!targeting) {
        return false;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return false;
    }

    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(targeting->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(targeting->m_data.m_spellId, &spell)) {
        return false;
    }

    uint32_t type = object->m_obj->m_type;
    bool valid = false;

    if (type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(object);

        if (spell.m_attributesExE & 0x800) {
            unit = Spell_C_TargetImpliedTarget(unit, &spell, s_needTargets, caster);

            if (!unit) {
                return false;
            }

            object = unit;
        }

        if (unit->m_obj->m_guid == targeting->m_data.m_caster && (spell.m_attributesEx & 0x80000)) {
            return false;
        }

        CGUnitData* data = unit->m_unit;

        if ((data->m_unitFieldFlags & 0x10000) && !(spell.m_attributesExF & 0x1000000)) {
            return false;
        }

        if (spell.m_targetCreatureType) {
            if (!unit->GetCreatureType() || !((1 << (unit->GetCreatureType() - 1)) & spell.m_targetCreatureType)) {
                return false;
            }
        }

        int32_t health = data->m_unitFieldHealth;
        uint32_t needTargets = s_needTargets;

        if (health <= 0) {
            if (!(data->m_unitDynamicFlags & 0x20) && !(needTargets & 0x8600) && !(spell.m_attributesExB & 1)) {
                return false;
            }
        }

        if (health > 0 && (needTargets & 0x400)) {
            return false;
        }

        if ((needTargets & 0x8) && caster->IsInMyPartyOrRaid(unit) && caster->CanAssist(unit, false)) {
            valid = true;
        } else if ((s_needTargets & 0x4) && caster->IsInMyParty(unit) && caster->CanAssist(unit, false)) {
            valid = true;
        } else if ((s_needTargets & 0x100) && caster->CanAssist(unit, (spell.m_attributesExF >> 3) & 1)) {
            valid = true;
        } else if ((s_needTargets & 0x80) && caster->CanAttack(unit)) {
            valid = true;
        } else {
            needTargets = s_needTargets;

            if ((needTargets & 0x2) && (!(data->m_unitFieldFlags & 0x100) || (spell.m_attributesExC & 0x10000000) || (spell.m_attributesExE & 0x200000))) {
                valid = true;
            } else if ((needTargets & 0x8000) && data->m_unitFieldHealth <= 0 && ((unit->m_obj->m_type & TYPE_PLAYER) || data->m_unitFieldPetnumber)) {
                if (!caster->CanAssist(unit, false)) {
                    return false;
                }

                valid = true;
            } else if ((needTargets & 0x200) && data->m_unitFieldHealth <= 0 && (unit->m_obj->m_type & TYPE_PLAYER)) {
                if (caster->CanAssist(unit, false)) {
                    return false;
                }

                valid = true;
            } else if ((needTargets & 0x10000) && unit->GetCreatureType() == 12) {
                valid = true;
            } else if ((s_needTargets & 0x100) && (spell.m_attributesExF & 0x200) && unit->IsCharmed() && caster->CanAttack(unit)) {
                valid = true;
            }
        }

        if (!valid) {
            return false;
        }
    } else if (type & TYPE_ITEM) {
        if (!(s_needTargets & 0x4010)) {
            return false;
        }
    } else if (type & TYPE_GAMEOBJECT) {
        if (!(s_needTargets & 0x4800) || !static_cast<CGGameObject_C*>(object)->IsValidTargetForSpell(targeting->m_data.m_castItem, targeting->m_data.m_spellId)) {
            return false;
        }
    } else if (type & TYPE_CORPSE) {
        const CGCorpse_C* corpse = static_cast<CGCorpse_C*>(object);
        bool friendly = false;

        if ((s_needTargets & 0x8000) && !(corpse->m_corpse->m_corpseFieldFlags & 1) && caster->CanAssist(corpse)) {
            friendly = true;
        }

        if (!((s_needTargets & 0x200) && !(corpse->m_corpse->m_corpseFieldFlags & 1) && !caster->CanAssist(corpse)) && !friendly) {
            return false;
        }
    } else {
        return false;
    }

    if (!checkRange) {
        return true;
    }

    float distance = caster->GetDistanceToPos(object->GetPosition());
    float minRange;
    float maxRange;
    Spell_C::GetSpellRange(caster, targeting->m_data.m_spellId, &minRange, &maxRange, object);

    if (distance < minRange * minRange || maxRange * maxRange < distance) {
        return false;
    }

    return true;
}

// VERIFY: 402f8659 auto
// OFFSET: 0x007FD950
int32_t Packet_SMSG_FEIGN_DEATH_RESISTED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_FEIGN_DEATH_RESISTED);
    return 1;
}

// VERIFY: ff87ae53 auto
// OFFSET: 0x007FD800
void Spell_C_CursorWorldObjectRotate() {
    s_cursorWorldObjectFacing = s_cursorWorldObjectFacing + 1.5707964f;

    if (s_cursorWorldObjectFacing >= 6.2831855f) {
        s_cursorWorldObjectFacing = s_cursorWorldObjectFacing - 6.2831855f;
    }
}

// VERIFY: 5da1ec93 auto
// OFFSET: 0x007FDF60
bool SpellIsTrackingAura(const SpellRec* spell) {
    for (int32_t i = 0; i < 3; i++) {
        int32_t aura = spell->m_effectAura[i];

        if (aura == SPELL_AURA_TRACK_CREATURES || aura == SPELL_AURA_TRACK_RESOURCES || aura == SPELL_AURA_TRACK_STEALTHED) {
            return true;
        }
    }

    return false;
}

// VERIFY: a97f4372 manual
// OFFSET: 0x00800510
int32_t Packet_SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_vehiclePassenger) {
        player->m_vehiclePassenger->ClearFlag8();
    }

    return 1;
}

// VERIFY: 9a6eeb12 auto
// OFFSET: 0x008007A0
void Spell_C_SetPlayerClass(int32_t classId) {
    ChrClassesRec* rec = g_chrClassesDB.GetRecord(classId);

    if (rec) {
        s_spellClassSet = rec->m_spellClassSet;
    }
}

// VERIFY: 28235946 manual
// OFFSET: 0x007FD900
int32_t Packet_SMSG_GAMEOBJECT_RESET_STATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

    if (gameObject) {
        gameObject->ResetDisplayState();
    }

    return 1;
}

// VERIFY: e42c1b1d auto
// OFFSET: 0x007FDC60
int32_t Packet_Group_32(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t group;
    uint8_t op;
    uint32_t value;
    msg->Get(group);
    msg->Get(op);
    msg->Get(value);

    if (msgId == SMSG_SET_PCT_SPELL_MODIFIER) {
        s_spellPctModifiers[group][op] = value;
    } else {
        s_spellFlatModifiers[group][op] = value;
    }

    return 1;
}

// VERIFY: 2541c8b3 manual
// OFFSET: 0x00805FC0
bool Spell_C_IsCurrentItem(int32_t itemId) {
    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast->m_data.m_castItem == cast->m_data.m_caster) {
            continue;
        }

        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cast->m_data.m_castItem, TYPE_ITEM, __FILE__, __LINE__));

        if (item && item->m_obj->m_entryID == itemId) {
            return true;
        }
    }

    return false;
}

// VERIFY: 1ec86af2 manual
// OFFSET: 0x00807560
void Spell_C_CancelAutoRepeat(bool a1) {
    if (!s_autoRepeatingSpell) {
        return;
    }

    if (a1) {
        WDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CANCEL_AUTO_REPEAT_SPELL));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (unit) {
        unit->SetCastingAutoRepeat(false);
        unit->CancelRangedMode();
    }

    Spell_C_CancelSpellsWithId(s_autoRepeatingSpell);

    if (!s_autoRepeatingSpell) {
        return;
    }

    s_autoRepeatingSpell = 0;

    if (s_autoRangedCombatSpell) {
        return;
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_STOP_AUTOREPEAT_SPELL, nullptr);

    if (s_autoRepeatingSpell) {
        static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__))->MeleeModeLeave();
    }
}

// VERIFY: 2524719c auto
// OFFSET: 0x007FDE50
bool SpellRec::HasAura(const SpellRec* spell, int32_t aura) {
    for (int32_t i = 0; i < 3; i++) {
        if (spell->m_effectAura[i] == aura) {
            return true;
        }
    }

    return false;
}

// VERIFY: 86c68872 auto
// OFFSET: 0x007FE1B0
int32_t SpellRec::IsHelpfulOrHarmful(const SpellRec* spell) {
    if (spell->m_targets & 0x100) {
        return 1;
    }

    if (spell->m_targets & 0x80) {
        return 2;
    }

    for (uint32_t i = 0; i < 3; i++) {
        switch (spell->m_implicitTargetA[i]) {
            case 2:
            case 6:
            case 15:
            case 16:
            case 24:
            case 28:
            case 53:
            case 54:
            case 93:
                return 2;
        }

        switch (spell->m_implicitTargetB[i]) {
            case 2:
            case 6:
            case 15:
            case 16:
            case 24:
            case 28:
            case 53:
            case 54:
            case 93:
                return 2;
        }
    }

    for (uint32_t i = 0; i < 3; i++) {
        switch (spell->m_implicitTargetA[i]) {
            case 1:
                if (spell->m_effectAura[i] != SPELL_AURA_DUMMY) {
                    return 1;
                }

                break;

            case 3:
            case 4:
            case 5:
            case 20:
            case 21:
            case 27:
            case 29:
            case 30:
            case 31:
            case 33:
            case 34:
            case 35:
            case 45:
            case 56:
            case 57:
            case 58:
            case 59:
            case 61:
            case 62:
                return 1;
        }

        switch (spell->m_implicitTargetB[i]) {
            case 1:
                if (spell->m_effectAura[i] != SPELL_AURA_DUMMY) {
                    return 1;
                }

                break;

            case 3:
            case 4:
            case 5:
            case 20:
            case 21:
            case 27:
            case 29:
            case 30:
            case 31:
            case 33:
            case 34:
            case 35:
            case 45:
            case 56:
            case 57:
            case 58:
            case 59:
            case 61:
            case 62:
                return 1;
        }
    }

    return 0;
}

// VERIFY: 69f73b82 auto
// OFFSET: 0x008009B0
bool SpellRec::WillSummonCritter(const SpellRec* spell) {
    if (spell->m_effect[0] == SPELL_EFFECT_SUMMON) {
        SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell->m_effectMiscValueB[0]);

        if (properties && properties->m_slot == 5) {
            return true;
        }
    }

    return spell->m_effectAura[0] == SPELL_AURA_MOUNTED;
}

// VERIFY: 54ab5610 manual
// OFFSET: 0x007FFA30
int32_t Spell_C_GetSpellReagentUses(const SpellRec* spell) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t uses = 0x7FFFFFFF;

    for (uint32_t i = 0; i < 8; i++) {
        int32_t reagent = spell->m_reagent[i];

        if (reagent != -2 && reagent && spell->m_reagentCount[i] > 0) {
            int32_t count = player->GetBag()->GetItemTypeCount(reagent, 0) / spell->m_reagentCount[i];

            if (count < uses) {
                uses = count;
            }
        }
    }

    return uses == 0x7FFFFFFF ? 0 : uses;
}

// VERIFY: 1bbeb20f auto
// OFFSET: 0x00801770
int32_t Spell_C_GetSpellReagentUses(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    return Spell_C_GetSpellReagentUses(&spell);
}

// VERIFY: db856fff manual
// OFFSET: 0x00807980
bool SpellHistory::GetSpellCooldown(int32_t spellId, int32_t itemId, uint32_t* duration, uint32_t* start, uint32_t* enable) {
    if (enable) {
        *enable = 1;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || spell.m_effect[0] == SPELL_EFFECT_ATTACK || spell.m_effect[0] == SPELL_EFFECT_TRADE_SKILL) {
        return false;
    }

    int32_t category = spell.m_category;
    int32_t startRecoveryCategory = spell.m_startRecoveryCategory;

    if (itemId) {
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, WOWGUID(), nullptr, nullptr, false);

        if (stats) {
            for (uint32_t i = 0; i < 5; i++) {
                if (stats->m_spellId[i] == static_cast<uint32_t>(spellId) && static_cast<int32_t>(stats->m_spellCategory[i]) > 0) {
                    category = stats->m_spellCategory[i];
                }
            }
        }
    }

    uint32_t now = OsGetAsyncTimeMs();
    uint32_t latest = now;

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = this->m_history.Next(history)) {
        if (history->m_spellId == spellId && history->m_itemId == itemId && (history->m_recoveryTime || history->m_onHold)) {
            uint32_t end = history->m_recoveryTime + (history->m_onHold ? now + 1 : history->m_time);

            if (static_cast<int32_t>(end - latest) >= 0) {
                if (duration) {
                    *duration = history->m_onHold ? 1 : history->m_recoveryTime;
                }

                if (start) {
                    *start = history->m_onHold ? now : history->m_time;
                }

                if (enable && *enable) {
                    *enable = history->m_onHold == 0;
                }

                latest = end;
            }
        }

        SpellCategoryRec* categoryRec = g_spellCategoryDB.GetRecord(history->m_category);

        if (((history->m_category && history->m_category == category) || (categoryRec && (categoryRec->m_flags & 0x2))) && history->m_categoryRecoveryTime) {
            uint32_t end = history->m_categoryRecoveryTime + (history->m_onHold ? now : history->m_categoryTime);

            if (static_cast<int32_t>(end - latest) >= 0) {
                if (duration) {
                    *duration = history->m_categoryRecoveryTime;
                }

                if (start) {
                    *start = history->m_onHold ? now : history->m_categoryTime;
                }

                if (enable && *enable) {
                    *enable = history->m_onHold == 0;
                }

                latest = end;
            }
        }

        if (history->m_startRecoveryCategory == startRecoveryCategory && history->m_startRecoveryTime) {
            uint32_t end = history->m_startRecoveryTime + history->m_time;

            if (static_cast<int32_t>(end - latest) >= 0) {
                if (duration) {
                    *duration = history->m_startRecoveryTime;
                }

                if (start) {
                    *start = history->m_time;
                }

                latest = end;
            }
        }

        if (!spell.m_runeCostID) {
            continue;
        }

        bool sharesRunes = spellId == history->m_spellId;

        if (!sharesRunes) {
            SpellRec other;

            if (g_spellDB.GetRecord(history->m_spellId, &other) && other.m_runeCostID) {
                SpellRuneCostRec* cost = g_spellRuneCostDB.GetRecord(spell.m_runeCostID);
                SpellRuneCostRec* otherCost = g_spellRuneCostDB.GetRecord(other.m_runeCostID);

                if (cost && otherCost && ((cost->m_blood && otherCost->m_blood) || (cost->m_unholy && otherCost->m_unholy) || (cost->m_frost && otherCost->m_frost))) {
                    sharesRunes = true;
                }
            }
        }

        uint32_t runeTime;

        if (!sharesRunes || !CGRuneInfo::ComputeRuneSpellCooldown(&spell, &runeTime) || static_cast<int32_t>(runeTime - latest) < 0) {
            continue;
        }

        latest = runeTime;

        if (start) {
            *start = history->m_time;
        }

        if (!duration || !start) {
            continue;
        }

        if (!CGRuneInfo::HasRunesForSpell(spellId, nullptr)) {
            *start = latest - 10000;
            *duration = 10000;
        } else {
            *duration = history->m_startRecoveryTime;
        }
    }

    return latest != now;
}

// VERIFY: a5bd03ff auto
// OFFSET: 0x00809000
bool Spell_C_GetSpellCooldown(int32_t spellId, int32_t petSpell, uint32_t* duration, uint32_t* start, uint32_t* enable) {
    return s_spellHistory[petSpell].GetSpellCooldown(spellId, 0, duration, start, enable);
}

// VERIFY: f18946a2 auto
// OFFSET: 0x007FD440
static bool GetEffectModifierType(int32_t effect, bool* periodic, int32_t aura, bool* rounded) {
    *rounded = false;
    *periodic = false;

    switch (effect) {
        case 2:
        case 9:
        case 10:
        case 17:
        case 31:
        case 58:
        case 67:
        case 75:
        case 121:
            *rounded = true;
            return true;

        case 6:
        case 27:
        case 35:
        case 65:
        case 119:
        case 128:
        case 129:
        case 143:
            switch (aura) {
                case 3:
                case 8:
                case 20:
                case 53:
                case 62:
                case 89:
                    *rounded = true;
                    *periodic = true;
                    return true;

                case 15:
                case 43:
                    *rounded = true;
                    return true;

                case 21:
                case 24:
                case 63:
                case 64:
                case 162:
                    *rounded = true;
                    return false;
            }

            return false;

        case 8:
        case 30:
        case 62:
        case 146:
            *rounded = true;
            return false;
    }

    return false;
}

// VERIFY: 207d6b5b manual
// OFFSET: 0x007FDBA0
bool Spell_C_ModifySpellValue(const SpellRec* spell, float& value, int32_t modifier) {
    int32_t flat;
    int32_t pct;

    if (!Spell_C_GetSpellModifiers(spell, modifier, flat, pct)) {
        return false;
    }

    value = (flat + value) * pct * 0.01f;
    return true;
}

// VERIFY: f7b1dbf5 manual
// OFFSET: 0x007FDBE0
static bool Spell_C_ModifySpellValue(const SpellRec* spell, CSoftFloat& value, int32_t modifier) {
    int32_t flat;
    int32_t pct;

    if (!Spell_C_GetSpellModifiers(spell, modifier, flat, pct)) {
        return false;
    }

    value = (value + CSoftFloat::FromInt(flat)) * CSoftFloat::FromInt(pct) * s_softPercent;
    return true;
}

// VERIFY: 49227fd2 auto
// OFFSET: 0x007FDE80
const int32_t* SpellRec::GetClassMask(const SpellRec* spell, int32_t effect) {
    switch (effect) {
        case 0:
            return spell->m_effectSpellClassMaskA;

        case 1:
            return spell->m_effectSpellClassMaskB;

        case 2:
            return spell->m_effectSpellClassMaskC;
    }

    return nullptr;
}

// VERIFY: 9c2128c3 auto
// OFFSET: 0x007FDEC0
bool SpellRec::IsAffectedBy(const SpellRec* spell, const SpellRec* modifier, int32_t effect) {
    if (!spell || !modifier || spell->m_spellClassSet != modifier->m_spellClassSet) {
        return false;
    }

    const int32_t* mask = SpellRec::GetClassMask(modifier, effect);

    for (uint32_t i = 0; i < 3; i++) {
        if (mask[i] & spell->m_spellClassMask[i]) {
            return true;
        }
    }

    return false;
}

// VERIFY: b26cb571 manual
// OFFSET: 0x007FE850
bool SpellRec::UsableInShapeshift(const SpellRec* spell, int32_t form) {
    return form >= 0 && (spell->m_shapeshiftMask[form / 32] & (1 << (form & 0x1F)));
}

// VERIFY: 18bb777f manual
// OFFSET: 0x007FF070
uint32_t Spell_C_GetSpellLevel(const SpellRec* spell, int32_t isPet, int32_t isUnit) {
    CGUnit_C* unit;

    if (isUnit) {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGCharacterInfo::m_inspectTarget, TYPE_UNIT, __FILE__, __LINE__));
    } else if (isPet) {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_UNIT, __FILE__, __LINE__));
    } else {
        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    }

    if (!unit) {
        return 0;
    }

    return static_cast<uint32_t>(unit->GetSpellRank(spell)) / 5;
}

// VERIFY: afcd1587 auto
// OFFSET: 0x007FF100
int32_t Spell_C_GetPowerCostPerSecond(const SpellRec* spell, const CGUnit_C* unit) {
    if (!unit) {
        return 0;
    }

    uint32_t rank = unit->GetSpellRank(spell);
    int32_t cost = spell->m_manaPerSecond + spell->m_manaPerSecondPerLevel * (static_cast<int32_t>(rank / 5) - spell->m_baseLevel);
    int32_t flat;
    int32_t pct;

    if (cost > 0 && Spell_C_GetSpellModifiers(spell, 14, flat, pct)) {
        return cost * pct / 100;
    }

    return cost;
}

// VERIFY: 9130a9d8 manual
// OFFSET: 0x007FF180
int32_t SpellRec::GetCastTime(const SpellRec* spell, int32_t isPet, int32_t isUnit, int32_t allowNegative) {
    if (!spell) {
        return 0;
    }

    SpellCastTimesRec* rec = g_spellCastTimesDB.GetRecord(spell->m_castingTimeIndex);

    if (!rec) {
        return 0;
    }

    int32_t castTime = rec->m_base + rec->m_perLevel * (static_cast<int32_t>(Spell_C_GetSpellLevel(spell, isPet, isUnit)) - spell->m_baseLevel);

    if (castTime < rec->m_minimum) {
        castTime = rec->m_minimum;
    }

    Spell_C_ModifySpellValue(spell, castTime, 10);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_UNIT, __FILE__, __LINE__));

    if (!(spell->m_attributes & 0x30) && !(spell->m_attributesExC & 0x20000000)) {
        if (isPet && unit) {
            const WOWGUID* guid = &unit->m_unit->m_unitFieldCharm;

            if (!*guid) {
                guid = &unit->m_unit->m_unitFieldSummon;
            }

            unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*guid, TYPE_UNIT, __FILE__, __LINE__));
        }

        if (unit && castTime > 0 && fnotequal(unit->m_unit->m_unitModCastSpeed, 1.0f)) {
            castTime = CMath::fint_(castTime * unit->m_unit->m_unitModCastSpeed);
        }
    }

    if (spell->m_attributes & 2) {
        castTime += 500;

        if (!isPet) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                CGBag_C* bag = &player->m_bag;

                if (bag) {
                    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(WOWGUID{ bag->GetItemGuid(17) }, TYPE_ITEM, __FILE__, __LINE__));

                    if (item) {
                        ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, player->m_obj->m_guid, nullptr, nullptr, false);

                        if (stats && stats->m_delay) {
                            uint32_t speed = player->m_unit->m_unitFieldRangedattacktime;

                            if (speed) {
                                castTime = static_cast<int32_t>(static_cast<float>(speed) / static_cast<int32_t>(stats->m_delay) * castTime);
                            }
                        }
                    }
                }
            }
        }
    }

    if (!allowNegative) {
        return castTime > 0 ? castTime : 0;
    }

    return castTime;
}

// VERIFY: 5d32a52a auto
// OFFSET: 0x007FF380
bool SpellRec::IsMeleeRange(const SpellRec* spell) {
    SpellRangeRec* rec = g_spellRangeDB.GetRecord(spell->m_rangeIndex);
    return rec && (rec->m_flags & 1);
}

// VERIFY: 62ade261 auto
// OFFSET: 0x007FF3C0
bool SpellRec::IsMinimumRange(const SpellRec* spell) {
    SpellRangeRec* rec = g_spellRangeDB.GetRecord(spell->m_rangeIndex);
    return rec && (rec->m_flags & 2);
}

// VERIFY: accadc29 manual
// OFFSET: 0x007FF400
void SpellRec::GetMinRange(const SpellRec* spell, float* range) {
    SpellRangeRec* rec = g_spellRangeDB.GetRecord(spell->m_rangeIndex);

    if (!rec) {
        return;
    }

    int32_t friendly = SpellRec::IsHelpfulOrHarmful(spell) == 1;

    if (rec->m_flags & 1) {
        *range = 0.0f;
    } else if (rec->m_flags << 1) {
        *range = 5.0f;
        *range += rec->m_rangeMin[friendly];
    } else {
        *range = rec->m_rangeMin[friendly];
    }
}

// VERIFY: 823383e2 manual
// OFFSET: 0x007FF480
void Spell_C_GetMinMaxRange(CGUnit_C* caster, const SpellRec* spell, float* minRange, float* maxRange, int32_t friendly, CGObject_C* target) {
    CGUnit_C* targetUnit = target && (target->m_obj->m_type & TYPE_UNIT) ? static_cast<CGUnit_C*>(target) : nullptr;

    if (spell->m_attributes & 0x404) {
        *maxRange = 100.0f;
        return;
    }

    float bonus = 0.0f;
    SpellRangeRec* rec = g_spellRangeDB.GetRecord(spell->m_rangeIndex);

    if (rec) {
        if (rec->m_flags & 1) {
            *minRange = 0.0f;

            if (!targetUnit) {
                targetUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster->m_combat.m_attacking, TYPE_UNIT, __FILE__, __LINE__));
            }

            float reach = targetUnit ? targetUnit->m_unit->m_unitFieldCombatreach : caster->m_unit->m_unitFieldCombatreach;
            bonus = reach + caster->m_unit->m_unitFieldCombatreach + 1.3333334f;

            if (bonus <= 5.0f) {
                bonus = 5.0f;
            }
        } else {
            if (rec->m_flags & 2) {
                float reach = targetUnit ? targetUnit->m_unit->m_unitFieldCombatreach : caster->m_unit->m_unitFieldCombatreach;
                *minRange = reach + caster->m_unit->m_unitFieldCombatreach + 1.3333334f;

                if (*minRange <= 5.0f) {
                    *minRange = 5.0f;
                }

                *minRange += rec->m_rangeMin[friendly];
            } else {
                *minRange = rec->m_rangeMin[friendly];
            }

            *maxRange = rec->m_rangeMax[friendly];

            if (target && (target->m_obj->m_type & 0x88)) {
                bonus = caster->m_unit->m_unitFieldCombatreach + (targetUnit ? targetUnit : caster)->m_unit->m_unitFieldCombatreach;

                if (*minRange != 0.0f && !(rec->m_flags & 2)) {
                    *minRange = bonus + *minRange;
                }
            }
        }

        if (targetUnit && (caster->m_movement->m_flags & 0x100D) && (targetUnit->m_movement->m_flags & 0x100D) && !caster->IsRunning() && !targetUnit->IsRunning() && ((rec->m_flags & 1) || (targetUnit->m_obj->m_type & TYPE_PLAYER))) {
            bonus += 2.6666667f;
        }
    }

    if ((spell->m_attributes & 2) && (caster->m_obj->m_type & TYPE_PLAYER)) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(caster);
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(WOWGUID{ player->m_bag.GetItemGuid(17) }, TYPE_ITEM, __FILE__, __LINE__));

        if (item) {
            ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, caster->m_obj->m_guid, nullptr, nullptr, false);

            if (stats) {
                *maxRange = stats->m_rangedModRange * 0.01f * *maxRange;
            }
        }
    }

    int32_t flat;
    int32_t pct;

    if (Spell_C_GetSpellModifiers(spell, 5, flat, pct)) {
        *maxRange = (flat + *maxRange) * pct * 0.01f;
    }

    *maxRange = *maxRange + bonus;
}

// VERIFY: ac5e661b manual
// OFFSET: 0x007FF770
void Spell_C_GetMinMaxPoints(const SpellRec* spell, uint32_t effectIndex, float* minPoints, float* maxPoints, int32_t spellLevel, int32_t isPet, int32_t isUnit, int32_t noModifiers) {
    *minPoints = 0.0f;
    *maxPoints = 0.0f;

    if (!spell) {
        return;
    }

    bool periodic;
    bool rounded;
    bool damage = GetEffectModifierType(spell->m_effect[effectIndex], &periodic, spell->m_effectAura[effectIndex], &rounded);
    int32_t level = spellLevel;

    if (!level) {
        level = Spell_C_GetSpellLevel(spell, isPet, isUnit);
    }

    if (spell->m_baseLevel > 0) {
        level -= spell->m_baseLevel;
    }

    if (level < 0) {
        level = 0;
    }

    CSoftFloat perLevel = *reinterpret_cast<const CSoftFloat*>(&spell->m_effectRealPointsPerLevel[effectIndex]);
    CSoftFloat minValue = CSoftFloat::FromInt(spell->m_effectBasePoints[effectIndex] + 1) + perLevel * CSoftFloat::FromInt(level);
    CSoftFloat maxValue = CSoftFloat::FromInt(spell->m_effectBasePoints[effectIndex] + spell->m_effectDieSides[effectIndex]) + perLevel * CSoftFloat::FromInt(level);

    if (!noModifiers) {
        Spell_C_ModifySpellValue(spell, minValue, 8);
        Spell_C_ModifySpellValue(spell, maxValue, 8);

        if (effectIndex == 0) {
            Spell_C_ModifySpellValue(spell, minValue, 3);
            Spell_C_ModifySpellValue(spell, maxValue, 3);
        } else if (effectIndex == 1) {
            Spell_C_ModifySpellValue(spell, minValue, 12);
            Spell_C_ModifySpellValue(spell, maxValue, 12);
        } else if (effectIndex == 2) {
            Spell_C_ModifySpellValue(spell, minValue, 23);
            Spell_C_ModifySpellValue(spell, maxValue, 23);
        }

        if (damage) {
            if (periodic) {
                Spell_C_ModifySpellValue(spell, minValue, 22);
                Spell_C_ModifySpellValue(spell, maxValue, 22);
            } else {
                Spell_C_ModifySpellValue(spell, minValue, 0);
                Spell_C_ModifySpellValue(spell, maxValue, 0);
            }
        }

        int32_t aura = spell->m_effectAura[effectIndex];

        if (aura == SPELL_AURA_MOD_THREAT || aura == SPELL_AURA_MOD_TOTAL_THREAT || aura == SPELL_AURA_MOD_CRITICAL_THREAT) {
            Spell_C_ModifySpellValue(spell, minValue, 2);
            Spell_C_ModifySpellValue(spell, maxValue, 2);
        }
    }

    minValue.m_bits += (minValue.m_bits & 0x7F800000) ? 0x3800000 : 0;
    minValue = CSoftFloat::Round(minValue);
    minValue.m_bits = (minValue.m_bits - 0x3800000) & ~static_cast<uint32_t>(static_cast<int32_t>((minValue.m_bits - 0x4000000) ^ minValue.m_bits) >> 31);
    maxValue.m_bits += (maxValue.m_bits & 0x7F800000) ? 0x3800000 : 0;
    maxValue = CSoftFloat::Round(maxValue);
    maxValue.m_bits = (maxValue.m_bits - 0x3800000) & ~static_cast<uint32_t>(static_cast<int32_t>((maxValue.m_bits - 0x4000000) ^ maxValue.m_bits) >> 31);

    if (rounded) {
        CSoftFloat low = CSoftFloat::Floor(minValue);
        *minPoints = *reinterpret_cast<float*>(&low);
        CSoftFloat high = CSoftFloat::Ceil(maxValue);
        *maxPoints = *reinterpret_cast<float*>(&high);
    } else {
        *minPoints = *reinterpret_cast<float*>(&minValue);
        *maxPoints = *reinterpret_cast<float*>(&maxValue);
    }
}

// VERIFY: 63ede24e manual
// OFFSET: 0x00800770
bool SpellRec::HasModifier(const SpellRec* spell, int32_t modifier) {
    int32_t flat;
    int32_t pct;
    return Spell_C_GetSpellModifiers(spell, modifier, flat, pct);
}

// VERIFY: 03db76ed manual
// OFFSET: 0x00800A70
int32_t Spell_C_GetDuration(const SpellRec* spell, int32_t isPet, int32_t isUnit, int32_t noModifiers, int32_t applyHaste) {
    if (!spell) {
        return 0;
    }

    SpellDurationRec* rec = g_spellDurationDB.GetRecord(spell->m_durationIndex);

    if (!rec) {
        return 0;
    }

    int32_t duration = rec->m_duration + rec->m_durationPerLevel * (static_cast<int32_t>(Spell_C_GetSpellLevel(spell, isPet, isUnit)) - spell->m_baseLevel);

    if (duration >= rec->m_maxDuration) {
        duration = rec->m_maxDuration;
    }

    if (!noModifiers) {
        Spell_C_ModifySpellValue(spell, duration, 1);
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !applyHaste) {
        return duration;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_effectAuraPeriod[i]) {
            if (((spell->m_attributesExE & 0x2000) || player->AffectedByAura(SPELL_AURA_PERIODIC_HASTE, spell)) && !(spell->m_attributesExC & 0x20000000) && player->m_unit->m_unitModCastSpeed >= 0.001f) {
                duration = CMath::fint_(duration * player->m_unit->m_unitModCastSpeed);
            }

            return duration;
        }
    }

    return duration;
}

// VERIFY: 885593c4 manual
// OFFSET: 0x00800D60
bool Spell_C_HasNoReagentCost(CGPlayer_C* player, const SpellRec* spell) {
    if ((player->m_unit->m_unitFieldFlags & 0x20) && (spell->m_attributesExE & 2)) {
        return true;
    }

    if (!spell->m_spellClassSet || spell->m_spellClassSet != s_spellClassSet) {
        return false;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell->m_spellClassMask[i] & player->m_player->m_playerNoReagentCost1[i]) {
            return true;
        }
    }

    return false;
}

// VERIFY: bfe60446 manual
// OFFSET: 0x008016C0
int32_t Spell_C_GetMinMaxPoints(const SpellRec* spell, uint32_t effectIndex, int32_t* minPoints, int32_t* maxPoints, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
    float low;
    float high;
    Spell_C_GetMinMaxPoints(spell, effectIndex, &low, &high, a5, a6, a7, a8);
    *minPoints = CMath::ftrunc(low);
    *maxPoints = CMath::ftrunc(high);
    return 0;
}

// VERIFY: 3cc995d6 auto
// OFFSET: 0x007FD630
int32_t Spell_C_GetTargetingSpell() {
    return s_targetingSpell ? s_targetingSpell->m_data.m_spellId : 0;
}

// VERIFY: 8d996461 auto
// OFFSET: 0x008090C0
bool Spell_C_GetItemCooldown(CGItem_C* item, uint32_t* duration, uint32_t* start, uint32_t* enable) {
    if (!item) {
        return false;
    }

    return s_spellHistory[0].GetSpellCooldown(item->GetEnchantmentSpell(0), item->m_obj->m_entryID, duration, start, enable);
}

// VERIFY: dac09432 auto
// OFFSET: 0x007FCCA0
const char* GetSpellFailedResult(int32_t reason) {
    switch (reason) {
        case SPELL_FAILED_SUCCESS:
            return "SPELL_FAILED_SUCCESS";
        case SPELL_FAILED_AFFECTING_COMBAT:
            return "SPELL_FAILED_AFFECTING_COMBAT";
        case SPELL_FAILED_ALREADY_AT_FULL_HEALTH:
            return "SPELL_FAILED_ALREADY_AT_FULL_HEALTH";
        case SPELL_FAILED_ALREADY_AT_FULL_MANA:
            return "SPELL_FAILED_ALREADY_AT_FULL_MANA";
        case SPELL_FAILED_ALREADY_AT_FULL_POWER:
            return "SPELL_FAILED_ALREADY_AT_FULL_POWER";
        case SPELL_FAILED_ALREADY_BEING_TAMED:
            return "SPELL_FAILED_ALREADY_BEING_TAMED";
        case SPELL_FAILED_ALREADY_HAVE_CHARM:
            return "SPELL_FAILED_ALREADY_HAVE_CHARM";
        case SPELL_FAILED_ALREADY_HAVE_SUMMON:
            return "SPELL_FAILED_ALREADY_HAVE_SUMMON";
        case SPELL_FAILED_ALREADY_OPEN:
            return "SPELL_FAILED_ALREADY_OPEN";
        case SPELL_FAILED_AURA_BOUNCED:
            return "SPELL_FAILED_AURA_BOUNCED";
        case SPELL_FAILED_AUTOTRACK_INTERRUPTED:
            return "SPELL_FAILED_AUTOTRACK_INTERRUPTED";
        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
            return "SPELL_FAILED_BAD_IMPLICIT_TARGETS";
        case SPELL_FAILED_BAD_TARGETS:
            return "SPELL_FAILED_BAD_TARGETS";
        case SPELL_FAILED_CANT_BE_CHARMED:
            return "SPELL_FAILED_CANT_BE_CHARMED";
        case SPELL_FAILED_CANT_BE_DISENCHANTED:
            return "SPELL_FAILED_CANT_BE_DISENCHANTED";
        case SPELL_FAILED_CANT_BE_DISENCHANTED_SKILL:
            return "SPELL_FAILED_CANT_BE_DISENCHANTED_SKILL";
        case SPELL_FAILED_CANT_BE_MILLED:
            return "SPELL_FAILED_CANT_BE_MILLED";
        case SPELL_FAILED_CANT_BE_PROSPECTED:
            return "SPELL_FAILED_CANT_BE_PROSPECTED";
        case SPELL_FAILED_CANT_CAST_ON_TAPPED:
            return "SPELL_FAILED_CANT_CAST_ON_TAPPED";
        case SPELL_FAILED_CANT_DUEL_WHILE_INVISIBLE:
            return "SPELL_FAILED_CANT_DUEL_WHILE_INVISIBLE";
        case SPELL_FAILED_CANT_DUEL_WHILE_STEALTHED:
            return "SPELL_FAILED_CANT_DUEL_WHILE_STEALTHED";
        case SPELL_FAILED_CANT_STEALTH:
            return "SPELL_FAILED_CANT_STEALTH";
        case SPELL_FAILED_CASTER_AURASTATE:
            return "SPELL_FAILED_CASTER_AURASTATE";
        case SPELL_FAILED_CASTER_DEAD:
            return "SPELL_FAILED_CASTER_DEAD";
        case SPELL_FAILED_CHARMED:
            return "SPELL_FAILED_CHARMED";
        case SPELL_FAILED_CHEST_IN_USE:
            return "SPELL_FAILED_CHEST_IN_USE";
        case SPELL_FAILED_CONFUSED:
            return "SPELL_FAILED_CONFUSED";
        case SPELL_FAILED_DONT_REPORT:
            return "SPELL_FAILED_DONT_REPORT";
        case SPELL_FAILED_EQUIPPED_ITEM:
            return "SPELL_FAILED_EQUIPPED_ITEM";
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
            return "SPELL_FAILED_EQUIPPED_ITEM_CLASS";
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
            return "SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND";
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
            return "SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND";
        case SPELL_FAILED_ERROR:
            return "SPELL_FAILED_ERROR";
        case SPELL_FAILED_FIZZLE:
            return "SPELL_FAILED_FIZZLE";
        case SPELL_FAILED_FLEEING:
            return "SPELL_FAILED_FLEEING";
        case SPELL_FAILED_FOOD_LOWLEVEL:
            return "SPELL_FAILED_FOOD_LOWLEVEL";
        case SPELL_FAILED_HIGHLEVEL:
            return "SPELL_FAILED_HIGHLEVEL";
        case SPELL_FAILED_HUNGER_SATIATED:
            return "SPELL_FAILED_HUNGER_SATIATED";
        case SPELL_FAILED_IMMUNE:
            return "SPELL_FAILED_IMMUNE";
        case SPELL_FAILED_INCORRECT_AREA:
            return "SPELL_FAILED_INCORRECT_AREA";
        case SPELL_FAILED_INTERRUPTED:
            return "SPELL_FAILED_INTERRUPTED";
        case SPELL_FAILED_INTERRUPTED_COMBAT:
            return "SPELL_FAILED_INTERRUPTED_COMBAT";
        case SPELL_FAILED_ITEM_ALREADY_ENCHANTED:
            return "SPELL_FAILED_ITEM_ALREADY_ENCHANTED";
        case SPELL_FAILED_ITEM_GONE:
            return "SPELL_FAILED_ITEM_GONE";
        case SPELL_FAILED_ITEM_NOT_FOUND:
            return "SPELL_FAILED_ITEM_NOT_FOUND";
        case SPELL_FAILED_ITEM_NOT_READY:
            return "SPELL_FAILED_ITEM_NOT_READY";
        case SPELL_FAILED_LEVEL_REQUIREMENT:
            return "SPELL_FAILED_LEVEL_REQUIREMENT";
        case SPELL_FAILED_LINE_OF_SIGHT:
            return "SPELL_FAILED_LINE_OF_SIGHT";
        case SPELL_FAILED_LOWLEVEL:
            return "SPELL_FAILED_LOWLEVEL";
        case SPELL_FAILED_LOW_CASTLEVEL:
            return "SPELL_FAILED_LOW_CASTLEVEL";
        case SPELL_FAILED_MAINHAND_EMPTY:
            return "SPELL_FAILED_MAINHAND_EMPTY";
        case SPELL_FAILED_MOVING:
            return "SPELL_FAILED_MOVING";
        case SPELL_FAILED_NEED_AMMO:
            return "SPELL_FAILED_NEED_AMMO";
        case SPELL_FAILED_NEED_AMMO_POUCH:
            return "SPELL_FAILED_NEED_AMMO_POUCH";
        case SPELL_FAILED_NEED_EXOTIC_AMMO:
            return "SPELL_FAILED_NEED_EXOTIC_AMMO";
        case SPELL_FAILED_NEED_MORE_ITEMS:
            return "SPELL_FAILED_NEED_MORE_ITEMS";
        case SPELL_FAILED_NOPATH:
            return "SPELL_FAILED_NOPATH";
        case SPELL_FAILED_NOT_BEHIND:
            return "SPELL_FAILED_NOT_BEHIND";
        case SPELL_FAILED_NOT_FISHABLE:
            return "SPELL_FAILED_NOT_FISHABLE";
        case SPELL_FAILED_NOT_FLYING:
            return "SPELL_FAILED_NOT_FLYING";
        case SPELL_FAILED_NOT_HERE:
            return "SPELL_FAILED_NOT_HERE";
        case SPELL_FAILED_NOT_INFRONT:
            return "SPELL_FAILED_NOT_INFRONT";
        case SPELL_FAILED_NOT_IN_CONTROL:
            return "SPELL_FAILED_NOT_IN_CONTROL";
        case SPELL_FAILED_NOT_KNOWN:
            return "SPELL_FAILED_NOT_KNOWN";
        case SPELL_FAILED_NOT_MOUNTED:
            return "SPELL_FAILED_NOT_MOUNTED";
        case SPELL_FAILED_NOT_ON_TAXI:
            return "SPELL_FAILED_NOT_ON_TAXI";
        case SPELL_FAILED_NOT_ON_TRANSPORT:
            return "SPELL_FAILED_NOT_ON_TRANSPORT";
        case SPELL_FAILED_NOT_READY:
            return "SPELL_FAILED_NOT_READY";
        case SPELL_FAILED_NOT_SHAPESHIFT:
            return "SPELL_FAILED_NOT_SHAPESHIFT";
        case SPELL_FAILED_NOT_STANDING:
            return "SPELL_FAILED_NOT_STANDING";
        case SPELL_FAILED_NOT_TRADEABLE:
            return "SPELL_FAILED_NOT_TRADEABLE";
        case SPELL_FAILED_NOT_TRADING:
            return "SPELL_FAILED_NOT_TRADING";
        case SPELL_FAILED_NOT_UNSHEATHED:
            return "SPELL_FAILED_NOT_UNSHEATHED";
        case SPELL_FAILED_NOT_WHILE_GHOST:
            return "SPELL_FAILED_NOT_WHILE_GHOST";
        case SPELL_FAILED_NOT_WHILE_LOOTING:
            return "SPELL_FAILED_NOT_WHILE_LOOTING";
        case SPELL_FAILED_NO_AMMO:
            return "SPELL_FAILED_NO_AMMO";
        case SPELL_FAILED_NO_CHARGES_REMAIN:
            return "SPELL_FAILED_NO_CHARGES_REMAIN";
        case SPELL_FAILED_NO_CHAMPION:
            return "SPELL_FAILED_NO_CHAMPION";
        case SPELL_FAILED_NO_COMBO_POINTS:
            return "SPELL_FAILED_NO_COMBO_POINTS";
        case SPELL_FAILED_NO_DUELING:
            return "SPELL_FAILED_NO_DUELING";
        case SPELL_FAILED_NO_ENDURANCE:
            return "SPELL_FAILED_NO_ENDURANCE";
        case SPELL_FAILED_NO_FISH:
            return "SPELL_FAILED_NO_FISH";
        case SPELL_FAILED_NO_ITEMS_WHILE_SHAPESHIFTED:
            return "SPELL_FAILED_NO_ITEMS_WHILE_SHAPESHIFTED";
        case SPELL_FAILED_NO_MOUNTS_ALLOWED:
            return "SPELL_FAILED_NO_MOUNTS_ALLOWED";
        case SPELL_FAILED_NO_PET:
            return "SPELL_FAILED_NO_PET";
        case SPELL_FAILED_NO_POWER:
            return "SPELL_FAILED_NO_POWER";
        case SPELL_FAILED_NOTHING_TO_DISPEL:
            return "SPELL_FAILED_NOTHING_TO_DISPEL";
        case SPELL_FAILED_NOTHING_TO_STEAL:
            return "SPELL_FAILED_NOTHING_TO_STEAL";
        case SPELL_FAILED_ONLY_ABOVEWATER:
            return "SPELL_FAILED_ONLY_ABOVEWATER";
        case SPELL_FAILED_ONLY_DAYTIME:
            return "SPELL_FAILED_ONLY_DAYTIME";
        case SPELL_FAILED_ONLY_INDOORS:
            return "SPELL_FAILED_ONLY_INDOORS";
        case SPELL_FAILED_ONLY_MOUNTED:
            return "SPELL_FAILED_ONLY_MOUNTED";
        case SPELL_FAILED_ONLY_NIGHTTIME:
            return "SPELL_FAILED_ONLY_NIGHTTIME";
        case SPELL_FAILED_ONLY_OUTDOORS:
            return "SPELL_FAILED_ONLY_OUTDOORS";
        case SPELL_FAILED_ONLY_SHAPESHIFT:
            return "SPELL_FAILED_ONLY_SHAPESHIFT";
        case SPELL_FAILED_ONLY_STEALTHED:
            return "SPELL_FAILED_ONLY_STEALTHED";
        case SPELL_FAILED_ONLY_UNDERWATER:
            return "SPELL_FAILED_ONLY_UNDERWATER";
        case SPELL_FAILED_OUT_OF_RANGE:
            return "SPELL_FAILED_OUT_OF_RANGE";
        case SPELL_FAILED_PACIFIED:
            return "SPELL_FAILED_PACIFIED";
        case SPELL_FAILED_POSSESSED:
            return "SPELL_FAILED_POSSESSED";
        case SPELL_FAILED_REAGENTS:
            return "SPELL_FAILED_REAGENTS";
        case SPELL_FAILED_REQUIRES_AREA:
            return "SPELL_FAILED_REQUIRES_AREA";
        case SPELL_FAILED_REQUIRES_SPELL_FOCUS:
            return "SPELL_FAILED_REQUIRES_SPELL_FOCUS";
        case SPELL_FAILED_ROOTED:
            return "SPELL_FAILED_ROOTED";
        case SPELL_FAILED_SILENCED:
            return "SPELL_FAILED_SILENCED";
        case SPELL_FAILED_SPELL_IN_PROGRESS:
            return "SPELL_FAILED_SPELL_IN_PROGRESS";
        case SPELL_FAILED_SPELL_LEARNED:
            return "SPELL_FAILED_SPELL_LEARNED";
        case SPELL_FAILED_SPELL_UNAVAILABLE:
            return "SPELL_FAILED_SPELL_UNAVAILABLE";
        case SPELL_FAILED_STUNNED:
            return "SPELL_FAILED_STUNNED";
        case SPELL_FAILED_TARGETS_DEAD:
            return "SPELL_FAILED_TARGETS_DEAD";
        case SPELL_FAILED_TARGET_AFFECTING_COMBAT:
            return "SPELL_FAILED_TARGET_AFFECTING_COMBAT";
        case SPELL_FAILED_TARGET_AURASTATE:
            return "SPELL_FAILED_TARGET_AURASTATE";
        case SPELL_FAILED_TARGET_DUELING:
            return "SPELL_FAILED_TARGET_DUELING";
        case SPELL_FAILED_TARGET_ENEMY:
            return "SPELL_FAILED_TARGET_ENEMY";
        case SPELL_FAILED_TARGET_ENRAGED:
            return "SPELL_FAILED_TARGET_ENRAGED";
        case SPELL_FAILED_TARGET_FRIENDLY:
            return "SPELL_FAILED_TARGET_FRIENDLY";
        case SPELL_FAILED_TARGET_IN_COMBAT:
            return "SPELL_FAILED_TARGET_IN_COMBAT";
        case SPELL_FAILED_TARGET_IS_PLAYER:
            return "SPELL_FAILED_TARGET_IS_PLAYER";
        case SPELL_FAILED_TARGET_IS_PLAYER_CONTROLLED:
            return "SPELL_FAILED_TARGET_IS_PLAYER_CONTROLLED";
        case SPELL_FAILED_TARGET_NOT_DEAD:
            return "SPELL_FAILED_TARGET_NOT_DEAD";
        case SPELL_FAILED_TARGET_NOT_IN_PARTY:
            return "SPELL_FAILED_TARGET_NOT_IN_PARTY";
        case SPELL_FAILED_TARGET_NOT_LOOTED:
            return "SPELL_FAILED_TARGET_NOT_LOOTED";
        case SPELL_FAILED_TARGET_NOT_PLAYER:
            return "SPELL_FAILED_TARGET_NOT_PLAYER";
        case SPELL_FAILED_TARGET_NO_POCKETS:
            return "SPELL_FAILED_TARGET_NO_POCKETS";
        case SPELL_FAILED_TARGET_NO_WEAPONS:
            return "SPELL_FAILED_TARGET_NO_WEAPONS";
        case SPELL_FAILED_TARGET_NO_RANGED_WEAPONS:
            return "SPELL_FAILED_TARGET_NO_RANGED_WEAPONS";
        case SPELL_FAILED_TARGET_UNSKINNABLE:
            return "SPELL_FAILED_TARGET_UNSKINNABLE";
        case SPELL_FAILED_THIRST_SATIATED:
            return "SPELL_FAILED_THIRST_SATIATED";
        case SPELL_FAILED_TOO_CLOSE:
            return "SPELL_FAILED_TOO_CLOSE";
        case SPELL_FAILED_TOO_MANY_OF_ITEM:
            return "SPELL_FAILED_TOO_MANY_OF_ITEM";
        case SPELL_FAILED_TOTEM_CATEGORY:
            return "SPELL_FAILED_TOTEM_CATEGORY";
        case SPELL_FAILED_TOTEMS:
            return "SPELL_FAILED_TOTEMS";
        case SPELL_FAILED_TRY_AGAIN:
            return "SPELL_FAILED_TRY_AGAIN";
        case SPELL_FAILED_UNIT_NOT_BEHIND:
            return "SPELL_FAILED_UNIT_NOT_BEHIND";
        case SPELL_FAILED_UNIT_NOT_INFRONT:
            return "SPELL_FAILED_UNIT_NOT_INFRONT";
        case SPELL_FAILED_WRONG_PET_FOOD:
            return "SPELL_FAILED_WRONG_PET_FOOD";
        case SPELL_FAILED_NOT_WHILE_FATIGUED:
            return "SPELL_FAILED_NOT_WHILE_FATIGUED";
        case SPELL_FAILED_TARGET_NOT_IN_INSTANCE:
            return "SPELL_FAILED_TARGET_NOT_IN_INSTANCE";
        case SPELL_FAILED_NOT_WHILE_TRADING:
            return "SPELL_FAILED_NOT_WHILE_TRADING";
        case SPELL_FAILED_TARGET_NOT_IN_RAID:
            return "SPELL_FAILED_TARGET_NOT_IN_RAID";
        case SPELL_FAILED_TARGET_FREEFORALL:
            return "SPELL_FAILED_TARGET_FREEFORALL";
        case SPELL_FAILED_NO_EDIBLE_CORPSES:
            return "SPELL_FAILED_NO_EDIBLE_CORPSES";
        case SPELL_FAILED_ONLY_BATTLEGROUNDS:
            return "SPELL_FAILED_ONLY_BATTLEGROUNDS";
        case SPELL_FAILED_TARGET_NOT_GHOST:
            return "SPELL_FAILED_TARGET_NOT_GHOST";
        case SPELL_FAILED_TRANSFORM_UNUSABLE:
            return "SPELL_FAILED_TRANSFORM_UNUSABLE";
        case SPELL_FAILED_WRONG_WEATHER:
            return "SPELL_FAILED_WRONG_WEATHER";
        case SPELL_FAILED_DAMAGE_IMMUNE:
            return "SPELL_FAILED_DAMAGE_IMMUNE";
        case SPELL_FAILED_PREVENTED_BY_MECHANIC:
            return "SPELL_FAILED_PREVENTED_BY_MECHANIC";
        case SPELL_FAILED_PLAY_TIME:
            return "SPELL_FAILED_PLAY_TIME";
        case SPELL_FAILED_REPUTATION:
            return "SPELL_FAILED_REPUTATION";
        case SPELL_FAILED_MIN_SKILL:
            return "SPELL_FAILED_MIN_SKILL";
        case SPELL_FAILED_NOT_IN_ARENA:
            return "SPELL_FAILED_NOT_IN_ARENA";
        case SPELL_FAILED_NOT_ON_SHAPESHIFT:
            return "SPELL_FAILED_NOT_ON_SHAPESHIFT";
        case SPELL_FAILED_NOT_ON_STEALTHED:
            return "SPELL_FAILED_NOT_ON_STEALTHED";
        case SPELL_FAILED_NOT_ON_DAMAGE_IMMUNE:
            return "SPELL_FAILED_NOT_ON_DAMAGE_IMMUNE";
        case SPELL_FAILED_NOT_ON_MOUNTED:
            return "SPELL_FAILED_NOT_ON_MOUNTED";
        case SPELL_FAILED_TOO_SHALLOW:
            return "SPELL_FAILED_TOO_SHALLOW";
        case SPELL_FAILED_TARGET_NOT_IN_SANCTUARY:
            return "SPELL_FAILED_TARGET_NOT_IN_SANCTUARY";
        case SPELL_FAILED_TARGET_IS_TRIVIAL:
            return "SPELL_FAILED_TARGET_IS_TRIVIAL";
        case SPELL_FAILED_BM_OR_INVISGOD:
            return "SPELL_FAILED_BM_OR_INVISGOD";
        case SPELL_FAILED_EXPERT_RIDING_REQUIREMENT:
            return "SPELL_FAILED_EXPERT_RIDING_REQUIREMENT";
        case SPELL_FAILED_ARTISAN_RIDING_REQUIREMENT:
            return "SPELL_FAILED_ARTISAN_RIDING_REQUIREMENT";
        case SPELL_FAILED_NOT_IDLE:
            return "SPELL_FAILED_NOT_IDLE";
        case SPELL_FAILED_NOT_INACTIVE:
            return "SPELL_FAILED_NOT_INACTIVE";
        case SPELL_FAILED_PARTIAL_PLAYTIME:
            return "SPELL_FAILED_PARTIAL_PLAYTIME";
        case SPELL_FAILED_NO_PLAYTIME:
            return "SPELL_FAILED_NO_PLAYTIME";
        case SPELL_FAILED_NOT_IN_BATTLEGROUND:
            return "SPELL_FAILED_NOT_IN_BATTLEGROUND";
        case SPELL_FAILED_NOT_IN_RAID_INSTANCE:
            return "SPELL_FAILED_NOT_IN_RAID_INSTANCE";
        case SPELL_FAILED_ONLY_IN_ARENA:
            return "SPELL_FAILED_ONLY_IN_ARENA";
        case SPELL_FAILED_TARGET_LOCKED_TO_RAID_INSTANCE:
            return "SPELL_FAILED_TARGET_LOCKED_TO_RAID_INSTANCE";
        case SPELL_FAILED_ON_USE_ENCHANT:
            return "SPELL_FAILED_ON_USE_ENCHANT";
        case SPELL_FAILED_NOT_ON_GROUND:
            return "SPELL_FAILED_NOT_ON_GROUND";
        case SPELL_FAILED_CUSTOM_ERROR:
            return "SPELL_FAILED_CUSTOM_ERROR";
        case SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW:
            return "SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW";
        case SPELL_FAILED_TOO_MANY_SOCKETS:
            return "SPELL_FAILED_TOO_MANY_SOCKETS";
        case SPELL_FAILED_INVALID_GLYPH:
            return "SPELL_FAILED_INVALID_GLYPH";
        case SPELL_FAILED_UNIQUE_GLYPH:
            return "SPELL_FAILED_UNIQUE_GLYPH";
        case SPELL_FAILED_GLYPH_SOCKET_LOCKED:
            return "SPELL_FAILED_GLYPH_SOCKET_LOCKED";
        case SPELL_FAILED_NO_VALID_TARGETS:
            return "SPELL_FAILED_NO_VALID_TARGETS";
        case SPELL_FAILED_ITEM_AT_MAX_CHARGES:
            return "SPELL_FAILED_ITEM_AT_MAX_CHARGES";
        case SPELL_FAILED_NOT_IN_BARBERSHOP:
            return "SPELL_FAILED_NOT_IN_BARBERSHOP";
        case SPELL_FAILED_FISHING_TOO_LOW:
            return "SPELL_FAILED_FISHING_TOO_LOW";
        case SPELL_FAILED_ITEM_ENCHANT_TRADE_WINDOW:
            return "SPELL_FAILED_ITEM_ENCHANT_TRADE_WINDOW";
        case SPELL_FAILED_SUMMON_PENDING:
            return "SPELL_FAILED_SUMMON_PENDING";
        case SPELL_FAILED_MAX_SOCKETS:
            return "SPELL_FAILED_MAX_SOCKETS";
        case SPELL_FAILED_PET_CAN_RENAME:
            return "SPELL_FAILED_PET_CAN_RENAME";
        case SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED:
            return "SPELL_FAILED_TARGET_CANNOT_BE_RESURRECTED";
        default:
            return "SPELL_FAILED_UNKNOWN";
    }
}

// VERIFY: 338bf6ad auto
// OFFSET: 0x007FD5F0
bool Spell_C_SpellUsesReagent(const SpellRec* spell, int32_t itemId) {
    for (uint32_t i = 0; i < sizeof(spell->m_reagent) / sizeof(spell->m_reagent[0]); i++) {
        if (spell->m_reagent[i] == itemId && spell->m_reagentCount[i] > 0) {
            return true;
        }
    }

    return false;
}

// VERIFY: 102740d5 auto
// OFFSET: 0x007FD640
PendingSpellCast* Spell_C_GetTargetingSpellCast() {
    return s_targetingSpell;
}

// VERIFY: 1e96df1d auto
// OFFSET: 0x007FD760
int32_t Spell_C_CanTargetItems() {
    return (s_needTargets & 0x4010) != 0;
}

// VERIFY: e4e8f874 auto
// OFFSET: 0x007FD7A0
CM2Model* Spell_C_GetCursorWorldObjectModel() {
    return s_spellWorldModel;
}

// VERIFY: e512de8b auto
// OFFSET: 0x007FD7B0
void Spell_C_SetCursorWorldObjectPosition(const C3Vector& position) {
    s_cursorWorldObjectPosition = position;
}

// VERIFY: 1a823145 auto
// OFFSET: 0x007FD7E0
const C3Vector& Spell_C_GetCursorWorldObjectPosition() {
    return s_cursorWorldObjectPosition;
}

// VERIFY: 9616d0ac manual
// OFFSET: 0x007FDFA0
static bool SpellCancelsAura(const SpellRec* spell, const SpellRec* aura, uint32_t auraEffect) {
    if (!spell || !(spell->m_attributesEx & 0x8000) || (aura->m_attributes & 0x20000000)) {
        return false;
    }

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (spell->m_effect[i] != SPELL_EFFECT_APPLY_AURA) {
            continue;
        }

        int32_t misc = spell->m_effectMiscValue[i];

        switch (spell->m_effectAura[i]) {
            case SPELL_AURA_STATE_IMMUNITY:
                if (misc == aura->m_effectAura[auraEffect]) {
                    return true;
                }

                break;

            case SPELL_AURA_SCHOOL_IMMUNITY:
            case SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL:
                if (!(aura->m_attributesExB & 0x4000000) && (aura->m_schoolMask & misc)) {
                    return true;
                }

                break;

            case SPELL_AURA_DISPEL_IMMUNITY:
                if (misc == aura->m_dispelType) {
                    return true;
                }

                break;

            case SPELL_AURA_MECHANIC_IMMUNITY:
                if (misc == aura->m_mechanic || misc == aura->m_effectMechanic[auraEffect]) {
                    return true;
                }

                break;
        }
    }

    return false;
}

// VERIFY: 4e168416 manual
// OFFSET: 0x007FE3E0
uint32_t GetRequiredAuraVisionBitmask(const SpellRec* spell) {
    uint32_t mask = spell->m_requiredAuraVision > 0 ? 1 << (spell->m_requiredAuraVision - 1) : 0;

    for (uint32_t i = 0; i < sizeof(spell->m_effectAura) / sizeof(spell->m_effectAura[0]); i++) {
        if (spell->m_effectAura[i] == SPELL_AURA_MOD_STEALTH_DETECT) {
            mask |= 0x20;
        }

        if (spell->m_effectAura[i] == SPELL_AURA_MOD_INVISIBILITY_DETECT && (!spell->m_effectMiscValue[i] || spell->m_effectMiscValue[i] == 10)) {
            mask |= 0x40;
        }
    }

    return mask;
}

// VERIFY: 62cca87e manual
// OFFSET: 0x007FE890
bool SpellRec::NotUsableInShapeshiftForm(const SpellRec* spell, int32_t form) {
    if (form < 0) {
        return false;
    }

    return (spell->m_shapeshiftExclude[form / 32] & (1 << (form % 32))) != 0;
}

// VERIFY: 2ba175f5 manual
// OFFSET: 0x007FEA80
void SpellMissingItemCallback(int32_t itemId, const WOWGUID& guid, void* param, bool success) {
    int32_t reason = static_cast<int32_t>(reinterpret_cast<intptr_t>(param));
    UIErrors error = ERR_SPELL_FAILED_S;
    char format[1024];
    SStrCopy(format, FrameScript_GetText(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE), sizeof(format));

    if (reason == SPELL_FAILED_REAGENTS) {
        error = ERR_SPELL_FAILED_REAGENTS;
    } else if (reason == SPELL_FAILED_TOTEMS) {
        error = ERR_SPELL_FAILED_TOTEMS;
    }

    char text[1024];
    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (stats) {
        SStrPrintf(text, sizeof(text), format, stats->m_name[0]);
    } else {
        SStrPrintf(text, sizeof(text), format, "UNKNOWN");
    }

    CGGameUI::DisplayError(error, text);
}

// VERIFY: 2f537c65 auto
// OFFSET: 0x007FEB50
void SpellMissingItemsCallback(int32_t itemId, const WOWGUID& guid, void* param, bool success) {
    char format[1024];
    SStrCopy(format, FrameScript_GetText("SPELL_FAILED_NEED_MORE_ITEMS", -1, GENDER_NOT_APPLICABLE), sizeof(format));

    char text[1024];
    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (stats) {
        SStrPrintf(text, sizeof(text), format, static_cast<int32_t>(reinterpret_cast<intptr_t>(param)), stats->m_name[0]);
    } else {
        SStrPrintf(text, sizeof(text), format, 0, "UNKNOWN");
    }

    CGGameUI::DisplayError(ERR_SPELL_FAILED_S, text);
}

// VERIFY: 52758823 auto
// OFFSET: 0x007FEF10
void GetCustomSpellFailedString(char* buffer, uint32_t size, int32_t customError) {
    char token[64];
    SStrPrintf(token, sizeof(token), "%s_%d", "SPELL_FAILED_CUSTOM_ERROR", customError);
    SStrCopy(buffer, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), size);
    CGGameUI::DisplayError(ERR_SPELL_FAILED_S, buffer);
}

// VERIFY: 0e3e8924 manual
// OFFSET: 0x007FFF60
float Spell_C_GetCursorWorldObjectFacing() {
    if (s_cursorWorldObjectHousing) {
        return s_cursorWorldObjectFacing;
    }

    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_targetingSpell->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return s_cursorWorldObjectFacing;
    }

    s_cursorWorldObjectFacing = caster->GetFacing();
    return s_cursorWorldObjectFacing;
}

// VERIFY: 78608420 manual
// OFFSET: 0x00802A50
void SpellHistory::ClearAll() {
    while (SPELLHISTORY* history = this->m_history.Head()) {
        this->m_free.LinkToTail(history);
    }
}

// VERIFY: 1a68408e manual
// OFFSET: 0x008078E0
SpellHistory::~SpellHistory() {
    this->m_history.Clear();
    this->m_free.Clear();
}

// VERIFY: 7c88da8e auto
// OFFSET: 0x00804010
int32_t Packet_Group_31(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t spellId;
    WOWGUID guid;
    msg->Get(spellId);
    msg->Get(guid.m_guid);
    int32_t pet = guid == CGPetInfo::GetPet(0);

    switch (msgId) {
        case SMSG_COOLDOWN_EVENT:
        case SMSG_CLEAR_COOLDOWN: {
            Spell_C_ModifyCooldown(spellId, time, pet, msgId == SMSG_CLEAR_COOLDOWN);
            break;
        }

        case SMSG_COOLDOWN_CHEAT: {
            s_spellHistory[pet].ClearAll();
            CGSpellBook::UpdateCooldowns();

            if (pet) {
                CGPetInfo::UpdateCooldowns();
            } else {
                CGContainerInfo::UpdateCooldowns();
            }

            break;
        }

        case SMSG_MODIFY_COOLDOWN: {
            uint32_t delay;
            msg->Get(delay);

            if (spellId) {
                Spell_C_DelayCooldown(pet, spellId, delay);
            }

            break;
        }
    }

    return 1;
}

// VERIFY: 81391441 auto
// OFFSET: 0x00804110
int32_t Packet_SMSG_COOLDOWN_CHEAT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    if (guid == ClntObjMgrGetActivePlayer()) {
        s_spellHistory[0].ClearAll();
        CGSpellBook::UpdateCooldowns();
        CGContainerInfo::UpdateCooldowns();
        return 1;
    }

    if (guid == CGPetInfo::GetPet(0)) {
        s_spellHistory[1].ClearAll();
        CGSpellBook::UpdateCooldowns();
        CGPetInfo::UpdateCooldowns();
    }

    return 1;
}

// VERIFY: 7425edb0 manual
// OFFSET: 0x00804430
bool Spell_C_CancelsAuraEffect(const CGUnit_C* unit, const SpellRec* spell, int32_t auraType, int32_t* reason) {
    bool found = false;
    SpellRec aura;

    for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
        int32_t spellId = const_cast<CGUnit_C*>(unit)->m_auras[i].m_spellId;

        if (!spellId || !g_spellDB.GetRecord(spellId, &aura)) {
            continue;
        }

        for (uint32_t j = 0; j < sizeof(aura.m_effectAura) / sizeof(aura.m_effectAura[0]); j++) {
            if (aura.m_effectAura[j] != auraType) {
                continue;
            }

            found = true;

            if (!SpellCancelsAura(spell, &aura, j)) {
                if (reason) {
                    if (aura.m_effectMechanic[j]) {
                        *reason = aura.m_effectMechanic[j];
                    } else if (aura.m_mechanic) {
                        *reason = aura.m_mechanic;
                    }
                }

                return false;
            }
        }
    }

    return found;
}

// VERIFY: 6bd9469f auto
// OFFSET: 0x00804590
bool Spell_C_CancelsStun(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 12, reason);
}

// VERIFY: 65a88e05 auto
// OFFSET: 0x008045C0
bool Spell_C_CancelsSilence(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 27, reason) || Spell_C_CancelsAuraEffect(unit, spell, 60, reason);
}

// VERIFY: c9a4c918 auto
// OFFSET: 0x00804610
bool Spell_C_CancelsPacify(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 25, reason) || Spell_C_CancelsAuraEffect(unit, spell, 60, reason);
}

// VERIFY: 851db6e3 auto
// OFFSET: 0x00804660
bool Spell_C_CancelsFear(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 7, reason);
}

// VERIFY: bb2a34cc auto
// OFFSET: 0x00804690
bool Spell_C_CancelsConfuse(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 5, reason);
}

// VERIFY: 7ba2f519 auto
// OFFSET: 0x008046C0
bool Spell_C_CancelsCharm(const CGUnit_C* unit, const SpellRec* spell, int32_t* reason) {
    if (reason) {
        *reason = 0;
    }

    return Spell_C_CancelsAuraEffect(unit, spell, 6, reason) || Spell_C_CancelsAuraEffect(unit, spell, 177, reason) || Spell_C_CancelsAuraEffect(unit, spell, 2, reason);
}

// VERIFY: 751283a6 manual
// OFFSET: 0x00804720
void SpellHistory::RemoveStartCooldown(int32_t startRecoveryCategory) {
    bool removed = false;

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = this->m_history.Next(history)) {
        if (history->m_spellId == startRecoveryCategory) {
            history->m_startRecoveryCategory = 0;
            history->m_startRecoveryTime = 0;
            removed = true;
        }
    }

    if (removed) {
        this->GarbageCollect(OsGetAsyncTimeMs());
        CGSpellBook::UpdateCooldowns();
    }
}

// VERIFY: 3c3b6e25 auto
// OFFSET: 0x00804780
void Spell_C_PetSpellFailedUpdateCooldowns(int32_t spellId) {
    s_spellHistory[1].RemoveStartCooldown(spellId);
    CGPetInfo::UpdateCooldowns();
}

// VERIFY: 474b58ab manual
// OFFSET: 0x008047A0
void Spell_C_ClearInTransitList() {
    SpellInTransit* spell = s_spellsInTransit.Head();

    while (spell) {
        SpellInTransit* next = s_spellsInTransit.Next(spell);
        s_spellsInTransit.UnlinkNode(spell);
        delete spell;
        spell = next;
    }
}

// VERIFY: e7eb7309 auto
// OFFSET: 0x00804AF0
void Spell_C_ClearCooldowns() {
    s_itemCooldowns.InternalClear(0);

    for (uint32_t i = 0; i < sizeof(s_spellHistory) / sizeof(s_spellHistory[0]); i++) {
        s_spellHistory[i].ClearAll();
    }
}

// VERIFY: 1d8a9dca manual
// OFFSET: 0x00804C10
void Spell_C_UpdateInTransitList() {
    uint32_t now = OsGetAsyncTimeMs();
    SpellInTransit* spell = s_spellsInTransit.Head();

    while (spell) {
        SpellInTransit* next = s_spellsInTransit.Next(spell);

        if (static_cast<int32_t>(now - spell->m_arrivalTime) < 0) {
            break;
        }

        s_spellsInTransit.UnlinkNode(spell);
        delete spell;
        spell = next;
    }
}

// VERIFY: 28603e8e manual
// OFFSET: 0x00804CC0
int32_t Spell_C_IsSpellInTransit(WOWGUID caster, uint32_t spellId, uint32_t castTime) {
    for (SpellInTransit* spell = s_spellsInTransit.Head(); spell; spell = s_spellsInTransit.Next(spell)) {
        if (spell->m_caster == caster && spell->m_spellId == static_cast<int32_t>(spellId) && spell->m_serverTime == castTime) {
            return 1;
        }
    }

    return 0;
}
// VERIFY: dbeddd39 manual
// OFFSET: 0x00809030
bool Spell_C_GetItemCooldown(int32_t itemId, uint32_t* duration, uint32_t* start, uint32_t* enable) {
    WOWGUID player = ClntObjMgrGetActivePlayer();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, player, ItemCheckCooldownCallback, nullptr, true);

    if (!stats) {
        return false;
    }

    for (uint32_t i = 0; i < sizeof(stats->m_spellId) / sizeof(stats->m_spellId[0]); i++) {
        if (static_cast<int32_t>(stats->m_spellId[i]) > 0 && !stats->m_spellTrigger[i]) {
            return s_spellHistory[0].GetSpellCooldown(stats->m_spellId[i], itemId, duration, start, enable);
        }
    }

    return false;
}

// VERIFY: 95db2eb3 auto
// OFFSET: 0x0080ABE0
void ItemCheckCooldownCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success && Spell_C_GetItemCooldown(id, nullptr, nullptr, nullptr)) {
        CGSpellBook::UpdateCooldowns();
        CGContainerInfo::UpdateCooldowns();
    }
}

// VERIFY: 41a8c8a3 manual
// OFFSET: 0x0080E060
void Spell_C_AddPendingItemCooldown(int32_t itemId, int32_t spellId, uint32_t time, bool onHold) {
    HASHKEY_NONE key;
    ITEMCOOLDOWNHASHNODE* node = s_itemCooldowns.Ptr(itemId, key);

    if (!node) {
        node = s_itemCooldowns.New(itemId, key, 0, 0);
    }

    node->m_spellId = spellId;
    node->m_time = time;
    node->m_onHold = onHold;
}

// VERIFY: bcfe0d9d auto
// OFFSET: 0x007FE470
static bool SpellVisualKitHasChainEffect(const SpellVisualKitRec* kit) {
    for (uint32_t i = 0; i < sizeof(kit->m_charProc) / sizeof(kit->m_charProc[0]); i++) {
        if (!kit->m_charProc[i] || kit->m_charProc[i] == 12) {
            return true;
        }
    }

    return false;
}

// VERIFY: 88789792 auto
// OFFSET: 0x00800390
void Spell_C_PetTameFailure(uint8_t reason) {
    const char* token;

    switch (reason) {
        case 1:
            token = "PETTAME_INVALIDCREATURE";
            break;

        case 2:
            token = "PETTAME_TOOMANY";
            break;

        case 3:
            token = "PETTAME_CREATUREALREADYOWNED";
            break;

        case 4:
            token = "PETTAME_NOTTAMEABLE";
            break;

        case 5:
            token = "PETTAME_ANOTHERSUMMONACTIVE";
            break;

        case 6:
            token = "PETTAME_UNITSCANTTAME";
            break;

        case 7:
            token = "PETTAME_NOPETAVAILABLE";
            break;

        case 8:
            token = "PETTAME_INTERNALERROR";
            break;

        case 9:
            token = "PETTAME_TOOHIGHLEVEL";
            break;

        case 10:
            token = "PETTAME_DEAD";
            break;

        case 11:
            token = "PETTAME_NOTDEAD";
            break;

        case 12:
            token = "PETTAME_CANTCONTROLEXOTIC";
            break;

        default:
            token = "PETTAME_UNKNOWNERROR";
            break;
    }

    char text[1024];
    SStrCopy(text, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(text));
    CGGameUI::DisplayError(ERR_TAME_FAILED, text);
}

// VERIFY: af4cb118 manual
// OFFSET: 0x00800470
int32_t Packet_SMSG_SPELL_UPDATE_CHAIN_TARGETS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID caster;
    msg->Get(caster.m_guid);
    int32_t spellId;
    msg->Get(reinterpret_cast<uint32_t&>(spellId));
    uint32_t count = 0;
    msg->Get(count);
    TSStackArray<WOWGUID> targets(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * count)), count, 1);

    for (uint32_t i = 0; i < count; i++) {
        msg->Get(targets.m_data[i].m_guid);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->UpdateChainTargets(targets, spellId);
    }

    return 1;
}

// VERIFY: 40ea9fd4 manual
// OFFSET: 0x00800550
static int32_t UnitOnRemoteMissileTrajectoryCollision(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && (object->m_obj->m_type & TYPE_UNIT)) {
        static_cast<CGUnit_C*>(object)->OnRemoteMissileTrajectoryCollision(static_cast<MissileCollisionData*>(param));
    }

    return 1;
}

// VERIFY: e6b2f017 auto
// OFFSET: 0x008005A0
int32_t Packet_SMSG_SET_PROJECTILE_POSITION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    MissileCollisionData data;
    data.m_position.x = 0.0f;
    data.m_position.y = 0.0f;
    data.m_position.z = 0.0f;
    msg->Get(data.m_casterGuid.m_guid);
    msg->Get(data.m_castID);
    msg->Get(data.m_position.x);
    msg->Get(data.m_position.y);
    msg->Get(data.m_position.z);
    CMissile::OnRemoteMissileTrajectoryCollision(data);
    ClntObjMgrEnumVisibleObjects(UnitOnRemoteMissileTrajectoryCollision, &data);
    return 1;
}

// VERIFY: 679f44cf manual
// OFFSET: 0x00800610
int32_t Packet_SMSG_PLAY_SPELL_VISUAL(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t kitId;
    msg->Get(kitId);
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(kitId);

    if (kit) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            PlaySpellVisuaKitData kitData(nullptr, kit, 0);
            unit->PlaySpellVisualKit(kitData);
        }
    }

    return 1;
}

// VERIFY: d9c2084e manual
// OFFSET: 0x008006C0
int32_t Packet_SMSG_PLAY_SPELL_IMPACT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t kitId;
    msg->Get(kitId);
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(kitId);

    if (kit) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            PlaySpellVisuaKitData kitData(nullptr, kit, 1);
            unit->PlaySpellVisualKit(kitData);
        }
    }

    return 1;
}

// VERIFY: 23ec14f4 auto
// OFFSET: 0x00800950
bool SpellAppliesNonStanceShapeshift(const SpellRec* spell) {
    for (uint32_t i = 0; i < sizeof(spell->m_effectAura) / sizeof(spell->m_effectAura[0]); i++) {
        if (spell->m_effectAura[i] != SPELL_AURA_MOD_SHAPESHIFT) {
            continue;
        }

        SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(spell->m_effectMiscValue[i]);

        if (!form || !(form->m_flags & 0x1)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 28d55e5b manual
// OFFSET: 0x00800A00
void Spell_C_SetAutoRepeatingSpell(int32_t spellId) {
    if (s_autoRepeatingSpell == spellId) {
        return;
    }

    s_autoRepeatingSpell = spellId;

    if (!s_autoRangedCombatSpell) {
        if (spellId) {
            FrameScript_SignalEvent(SCRIPT_EVENT_START_AUTOREPEAT_SPELL, nullptr);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_STOP_AUTOREPEAT_SPELL, nullptr);
        }
    }

    if (s_autoRepeatingSpell) {
        ClntObjMgrGetActivePlayerObj()->MeleeModeLeave();
    }
}

// VERIFY: 9a0a630e manual
// OFFSET: 0x00800BF0
bool Spell_C_SpellVisualHasChainEffect(const SpellVisualRec* visual) {
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_castKit);

    if (kit && SpellVisualKitHasChainEffect(kit)) {
        return true;
    }

    kit = g_spellVisualKitDB.GetRecord(visual->m_impactKit);

    if (kit && SpellVisualKitHasChainEffect(kit)) {
        return true;
    }

    kit = g_spellVisualKitDB.GetRecord(visual->m_stateKit);

    if (kit && SpellVisualKitHasChainEffect(kit)) {
        return true;
    }

    kit = g_spellVisualKitDB.GetRecord(visual->m_channelKit);

    if (kit && SpellVisualKitHasChainEffect(kit)) {
        return true;
    }

    kit = g_spellVisualKitDB.GetRecord(visual->m_casterImpactKit);

    if (kit && SpellVisualKitHasChainEffect(kit)) {
        return true;
    }

    kit = g_spellVisualKitDB.GetRecord(visual->m_targetImpactKit);
    return kit && SpellVisualKitHasChainEffect(kit);
}

// VERIFY: 2d56a1b9 manual
// OFFSET: 0x00800D00
SpellVisualKitRec* Spell_C_GetAppropriateImpactKit(const SpellVisualRec* visual, int32_t caster) {
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(caster ? visual->m_casterImpactKit : visual->m_targetImpactKit);

    if (kit) {
        return kit;
    }

    return g_spellVisualKitDB.GetRecord(visual->m_impactKit);
}

// VERIFY: 50dd76ca manual
// OFFSET: 0x008019C0
float Spell_C_GetSpellRadius() {
    PendingSpellCast* cast = s_targetingSpell;

    if (!cast) {
        return 0.0f;
    }

    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return 0.0f;
    }

    SpellRec spell;
    g_spellDB.GetRecord(cast->m_data.m_spellId, &spell);

    SpellRadiusRec* radiusRec = g_spellRadiusDB.GetRecord(spell.m_effectRadiusIndex[0]);
    float radius = radiusRec ? radiusRec->m_radius + static_cast<int32_t>(caster->m_unit->m_unitFieldLevel) * radiusRec->m_radiusPerLevel : 0.0f;

    radiusRec = g_spellRadiusDB.GetRecord(spell.m_effectRadiusIndex[1]);
    float radius2 = radiusRec ? radiusRec->m_radius + static_cast<int32_t>(caster->m_unit->m_unitFieldLevel) * radiusRec->m_radiusPerLevel : 0.0f;

    if (radius2 >= radius) {
        radius = radius2;
    }

    int32_t flat;
    int32_t pct;

    if (Spell_C_GetSpellModifiers(&spell, 6, flat, pct)) {
        radius = (radius + flat) * pct * 0.01f;
    }

    return radius;
}

// VERIFY: 780f5311 manual
// OFFSET: 0x00801B80
int32_t Packet_SMSG_SPELL_DELAYED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID guid;
    *msg >> guid;
    int32_t delay;
    msg->Get(reinterpret_cast<uint32_t&>(delay));
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        SpellRec spell;

        if (g_spellDB.GetRecord(unit->m_castingSpellId, &spell)) {
            if ((spell.m_attributesExD & 0x8000000) || (!(spell.m_attributes & 0x2) && (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() || (!(spell.m_attributes & 0x180) && !(spell.m_attributesExD & 0x1))))) {
                unit->DelayCast(delay);
                Script_SendUnitSpellSignal(guid, unit->m_castCount, &spell, 329);
            }
        }
    }

    return 1;
}

// VERIFY: 5bdf9b1f auto
// OFFSET: 0x00802090
int32_t Packet_SMSG_PET_TAME_FAILURE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t reason;
    msg->Get(reason);
    Spell_C_PetTameFailure(reason);
    return 1;
}

// VERIFY: ca8bfe75 manual
// OFFSET: 0x008020C0
int32_t Packet_SMSG_UNIT_SPELLCAST_START(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID caster;
    *msg >> caster;
    SmartGUID target;
    *msg >> target;
    int32_t spellId;
    msg->Get(reinterpret_cast<uint32_t&>(spellId));
    uint32_t remaining;
    msg->Get(remaining);
    uint32_t duration;
    msg->Get(duration);
    uint32_t end = OsGetAsyncTimeMs() + remaining;
    uint32_t start = end - duration;
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell)) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(caster, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            int32_t signal;

            if (spell.m_attributesEx & 0x4) {
                unit->SetUIChannelDirect(spellId, start, end);
                signal = 331;
            } else {
                unit->SetUICastingDirect(spellId, target, start, end);
                signal = 324;
            }

            Script_SendUnitSpellSignal(caster, 0, &spell, signal);
        }
    }

    return 1;
}

// VERIFY: 24535fb9 manual
// OFFSET: 0x00802A10
void SpellHistory::DelayCooldown(int32_t spellId, int32_t delay) {
    for (SPELLHISTORY* history = this->m_history.Head(); history; history = this->m_history.Next(history)) {
        if (history->m_spellId == spellId) {
            history->m_time += delay;
            history->m_categoryTime += delay;
        }
    }
}

// VERIFY: 6a53b8e9 auto
// OFFSET: 0x00802BA0
void Spell_C_ModifyCooldown(int32_t spellId, uint32_t time, int32_t index, int32_t remove) {
    s_spellHistory[index].ClearOnHold(spellId, time, remove != 0);
    CGSpellBook::UpdateCooldowns();
    CGSpellBook::UpdateUsable();

    if (index) {
        CGPetInfo::UpdateCooldowns();
    } else {
        CGContainerInfo::UpdateCooldowns();
    }
}

// VERIFY: d71e0c5b auto
// OFFSET: 0x00802BF0
void Spell_C_DelayCooldown(int32_t index, int32_t spellId, int32_t delay) {
    s_spellHistory[index].DelayCooldown(spellId, delay);
    CGSpellBook::UpdateCooldowns();
    CGSpellBook::UpdateUsable();

    if (index) {
        CGPetInfo::UpdateCooldowns();
    } else {
        CGContainerInfo::UpdateCooldowns();
    }
}

// VERIFY: 019f1364 auto
// OFFSET: 0x007FDF20
static bool SpellHasUnitAuraEffect(const SpellRec* spell) {
    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (Spell_IsUnitAuraEffect(spell->m_effect[i])) {
            return true;
        }
    }

    return false;
}

// VERIFY: 76133823 manual
// OFFSET: 0x00802CB0
bool Spell_C_IsActiveSpell(const SpellRec* spell, const CGUnit_C* unit) {
    if (!unit) {
        return false;
    }

    if (!spell->m_activeIconID && !SpellRec::WillSummonCritter(spell)) {
        return false;
    }

    CGUnit_C* vehicle = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->GetVehicleUnit() : nullptr;

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (spell->m_effect[i] != SPELL_EFFECT_SUMMON) {
            continue;
        }

        if (vehicle && vehicle->m_obj->m_entryID == spell->m_effectMiscValue[i] && vehicle->m_unit->m_unitFieldCreatedby == ClntObjMgrGetActivePlayer()) {
            return true;
        }

        SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell->m_effectMiscValueB[0]);

        if (properties && properties->m_slot == 5) {
            return static_cast<int32_t>((unit->m_unit->m_unitFieldCritter.m_guid >> 24) & 0xFFFFFFF) == spell->m_effectMiscValue[i];
        }
    }

    bool active = false;
    CGUnit_C* self = const_cast<CGUnit_C*>(unit);

    for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
        if (self->m_auras[i].m_spellId != spell->m_id) {
            continue;
        }

        if (self->IsActivePlayer() && SpellRec::HasAura(spell, 36) && unit->m_shapeshiftSuppressed) {
            break;
        }

        if (self->m_auras[i].m_flags & 0x10) {
            return true;
        }
    }

    if (spell->m_effectAura[0] == SPELL_AURA_MOUNTED) {
        SpellRec aura;

        for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
            if (g_spellDB.GetRecord(self->m_auras[i].m_spellId, &aura) && aura.m_effectAura[0] == SPELL_AURA_MOUNTED && aura.m_effectMiscValue[0] == spell->m_effectMiscValue[0]) {
                active = true;
                break;
            }
        }
    }

    return active;
}

// VERIFY: 399e67df manual
// OFFSET: 0x00802F80
void Spell_C_CancelAura(const SpellRec* spell, CGUnit_C* unit, int32_t auraIndex) {
    if (!spell || !unit) {
        return;
    }

    if (!unit->IsActivePlayer()) {
        if (SpellHasUnitAuraEffect(spell)) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_PET_CANCEL_AURA));
            msg.Put(unit->m_obj->m_guid.m_guid);
            msg.Put(static_cast<uint32_t>(spell->m_id));
            msg.Finalize();
            ClientServices::Send2(&msg);
        }

        return;
    }

    if (unit->m_unit->m_unitFieldCharmedby) {
        return;
    }

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (spell->m_effect[i] != SPELL_EFFECT_SUMMON) {
            continue;
        }

        SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell->m_effectMiscValueB[0]);

        if (properties && properties->m_slot == 5 && static_cast<int32_t>((unit->m_unit->m_unitFieldCritter.m_guid >> 24) & 0xFFFFFFF) == spell->m_effectMiscValue[i]) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_DISMISS_CRITTER));
            msg.Put(unit->m_unit->m_unitFieldCritter.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    if (!SpellHasUnitAuraEffect(spell)) {
        return;
    }

    if (unit->m_mountDisplayId > 0 && !(unit->m_clientFlags & 0x10000000) && SpellRec::HasAura(spell, 78)) {
        SpellRec mount;

        for (uint32_t i = 0; i < unit->GetAuraCount(); i++) {
            if (g_spellDB.GetRecord(unit->m_auras[i].m_spellId, &mount) && mount.m_effectAura[0] == SPELL_AURA_MOUNTED && mount.m_effectMiscValue[0] == spell->m_effectMiscValue[0] && mount.m_id != spell->m_id) {
                Spell_C_CancelAura(&mount, unit, -1);
                return;
            }
        }

        unit->OnMountDisplayChanged(0);
    }

    if (unit->GetShapeshiftForm() && SpellRec::HasAura(spell, 36)) {
        if (lua_tainted) {
            for (uint32_t i = s_shapeshiftSpells.Count(); i; ) {
                i--;

                if (s_shapeshiftSpells.m_data[i].m_spellId == spell->m_id) {
                    return;
                }
            }
        }

        for (uint32_t i = 0; i < sizeof(spell->m_effectAura) / sizeof(spell->m_effectAura[0]); i++) {
            if (spell->m_effectAura[i] != SPELL_AURA_MOD_SHAPESHIFT) {
                continue;
            }

            SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecord(spell->m_effectMiscValue[i]);

            if (!form || (form->m_flags & 0x2)) {
                return;
            }

            if (unit->GetShapeshiftForm() == static_cast<uint32_t>(spell->m_effectMiscValue[i])) {
                unit->PreemptivelyCancelShapeshift();
                break;
            }
        }
    }

    if ((spell->m_attributesEx & 0x2000) && !(spell->m_attributesEx & 0x4) && (unit->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(unit)->ToggleFarSight(nullptr)) {
        return;
    }

    if (spell->m_id == 2584) {
        FrameScript_SignalEvent(SCRIPT_EVENT_AREA_SPIRIT_HEALER_OUT_OF_RANGE, nullptr);
    }

    for (uint32_t i = 0; i < sizeof(spell->m_effectAura) / sizeof(spell->m_effectAura[0]); i++) {
        if (spell->m_effectAura[i] != SPELL_AURA_CONTROL_VEHICLE) {
            continue;
        }

        const WOWGUID* controlled = &unit->m_unit->m_unitFieldCharm;

        if (!*controlled) {
            controlled = &unit->m_unit->m_unitFieldSummon;
        }

        if (controlled->m_guid == unit->GetTransportGUID()) {
            WOWGUID transport = { unit->GetTransportGUID() };
            CGUnit_C* transportUnit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPE_UNIT, __FILE__, __LINE__));

            if (transportUnit) {
                CVehicle_C* vehicle = transportUnit->m_vehicle;
                uint8_t seat = unit->m_movementData.m_transportSeat;

                if (vehicle) {
                    VehicleSeatRec* seatRec = vehicle->GetVehicleSeatRec(seat);

                    if (seatRec && (seatRec->m_flags & 0x2000000)) {
                        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

                        if (mover) {
                            mover->VehicleExit();

                            if (auraIndex >= 0 && unit->m_obj->m_guid.m_guid == unit->GetAuraCaster(auraIndex)) {
                                return;
                            }
                        }
                    }
                }
            }
        }

        break;
    }

    CGUnit_C* vehicleUnit = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->GetVehicleUnit() : nullptr;

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        if (spell->m_effect[i] == SPELL_EFFECT_SUMMON && vehicleUnit && vehicleUnit->m_obj->m_entryID == spell->m_effectMiscValue[i]) {
            CGInputControl* input = CGInputControl::GetActive();
            CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

            if (mover && input->IsVehicleSeatFlagSet(0x2000000)) {
                mover->VehicleExit();
            }
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CANCEL_AURA));
    msg.Put(static_cast<uint32_t>(spell->m_id));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c6760f6a manual
// OFFSET: 0x008038F0
bool Spell_C_IsCastingInterruptedByCombat(const WOWGUID& guid) {
    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast->m_data.m_caster == guid) {
            SpellRec spell;

            if (g_spellDB.GetRecord(cast->m_data.m_spellId, &spell) && (spell.m_interruptFlags & 0x8)) {
                return true;
            }
        }
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit && unit->m_unit->m_unitChannelSpell) {
        SpellRec spell;

        if (g_spellDB.GetRecord(unit->m_unit->m_unitChannelSpell, &spell) && (spell.m_channelInterruptFlags & 0x1000)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 839db5ad manual
// OFFSET: 0x00803EE0
int32_t Spell_C_HandleTerrainRay(const CTerrainClickEvent& evt, bool checkRange) {
    if (!(s_needTargets & 0x60)) {
        return 1;
    }

    PendingSpellCastData* data = s_targetingSpell ? &s_targetingSpell->m_data : nullptr;
    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(data->m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return 1;
    }

    if (!checkRange) {
        return 0;
    }

    C3Vector position = evt.m_point;

    if (evt.m_guid) {
        CGObject_C* object = ClntObjMgrObjectPtr(evt.m_guid, TYPE_OBJECT, __FILE__, __LINE__);

        if (object) {
            position *= object->GetMatrix();
        }
    }

    float distance = caster->GetDistanceToPos(position);
    float minRange;
    float maxRange;
    Spell_C::GetSpellRange(caster, data->m_spellId, &minRange, &maxRange, nullptr);

    if (distance >= minRange * minRange && maxRange * maxRange >= distance) {
        return 0;
    }

    return maxRange * maxRange * 16.0f >= distance ? 1 : 2;
}

// VERIFY: 047edb41 manual
// OFFSET: 0x008055C0
void Spell_C_FailPendingCast(PendingSpellCast* cast, const SpellRec* spell, int32_t reason) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        HandleSpellFailedOrGo(unit, cast, spell, reason);
        FinishPendingSpellCast(cast, spell, unit);
    }
}

// VERIFY: fee48a29 manual
// OFFSET: 0x00805EF0
bool Spell_C_IsModal(const WOWGUID& item) {
    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast->m_data.m_castItem == item) {
            return true;
        }
    }

    return false;
}

// VERIFY: c6982e6a manual
// OFFSET: 0x00805F30
static bool Spell_C_IsSpellPending(int32_t spellId) {
    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast->m_data.m_spellId == static_cast<uint32_t>(spellId)) {
            return true;
        }
    }

    return false;
}

// VERIFY: a0204cde manual
// OFFSET: 0x00806030
bool Spell_C_IsCurrentSpell(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return false;
    }

    if (Spell_C_IsSpellPending(spellId)) {
        return true;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (spell.m_effect[0] == SPELL_EFFECT_ATTACK) {
        return player && player->m_combat.m_attacking;
    }

    if (spell.m_effect[0] == SPELL_EFFECT_TRADE_SKILL) {
        if (player) {
            SkillLineAbilityRec* ability = player->LookupAbility(spell.m_id);

            if (ability && !CGTradeSkillInfo::m_linkData && CGTradeSkillInfo::m_skillLineID == ability->m_skillLine) {
                return true;
            }
        }

        return false;
    }

    if (!SpellRec::WillSummonCritter(&spell) && !SpellRec::HasAura(&spell, 36)) {
        return false;
    }

    return Spell_C_IsActiveSpell(&spell, player);
}

// VERIFY: 07e6999f manual
// OFFSET: 0x008061A0
PendingSpellCast* Spell_C_GetActivePlayerPendingCast() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return nullptr;
    }

    return GetPendingSpellCast(player->m_castCount, player->m_castSpellId, player->m_obj->m_guid);
}

// VERIFY: fd2d4d6a manual
// OFFSET: 0x00806200
void Spell_C_CancelSpell(PendingSpellCast* cast, bool stopRecast, bool notifyServer, int32_t reason) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(cast->m_data.m_spellId, &spell)) {
        return;
    }

    if (CGTradeInfo::m_enchantingItem && CGTradeInfo::m_enchantingItem == cast->m_data.m_castItem) {
        CGTradeInfo::SetEnchantingItem(nullptr);
    }

    if (cast != s_targetingSpell && notifyServer) {
        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

        if ((caster && caster->GetSpellCastingTime(&spell) > 0) || (spell.m_attributes & 0x404)) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_CANCEL_CAST));
            msg.Put(cast->m_data.m_castCount);
            msg.Put(static_cast<uint32_t>(cast->m_data.m_spellId));
            msg.Finalize();
            ClientServices::Send2(&msg);
        }
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        HandleSpellFailedOrGo(unit, cast, &spell, reason);
        FinishPendingSpellCast(cast, &spell, unit);
    }

    if (stopRecast) {
        CGTradeSkillInfo::StopRecast();
    }
}

// VERIFY: a7ca1637 manual
// OFFSET: 0x00806390
void Spell_C_CancelSpellByCaster(const WOWGUID& caster) {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);

        if (cast->m_data.m_castItem == caster) {
            Spell_C_CancelSpell(cast, true, true, 27);
        }

        cast = next;
    }
}

// VERIFY: 099397b4 manual
// OFFSET: 0x008063E0
void Spell_C_CancelSpellByTarget(const WOWGUID& target, int32_t reason) {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);

        if (cast->m_data.m_objectTarget == target) {
            Spell_C_CancelSpell(cast, true, true, reason);
        }

        cast = next;
    }
}

// VERIFY: 98e4a5c1 manual
// OFFSET: 0x00806440
void Spell_C_CancelSpellsWithId(int32_t spellId) {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);

        if (cast->m_data.m_spellId == static_cast<uint32_t>(spellId)) {
            Spell_C_CancelSpell(cast, true, true, 27);
        }

        cast = next;
    }
}

// VERIFY: 09c12820 manual
// OFFSET: 0x00806480
void Spell_C_CancelMeleeSpells() {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);
        SpellRec spell;

        if (g_spellDB.GetRecord(cast->m_data.m_spellId, &spell) && (spell.m_attributes & 0x404)) {
            Spell_C_CancelSpell(cast, false, true, 27);
        }

        cast = next;
    }
}

// VERIFY: 6f75e1c3 manual
// OFFSET: 0x00806550
void Spell_C_CancelRangedSpells() {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);
        SpellRec spell;

        if (g_spellDB.GetRecord(cast->m_data.m_spellId, &spell) && (spell.m_attributes & 0x2)) {
            Spell_C_CancelSpell(cast, false, true, 27);
        }

        cast = next;
    }
}

// VERIFY: 3a7b6437 manual
// OFFSET: 0x00806620
void Spell_C_CancelCastingSpellByMovement() {
    PendingSpellCast* cast = s_pendingSpellCasts.Head();

    while (cast) {
        PendingSpellCast* next = s_pendingSpellCasts.Next(cast);

        if (cast != s_targetingSpell) {
            SpellRec spell;

            if (g_spellDB.GetRecord(cast->m_data.m_spellId, &spell) && (spell.m_interruptFlags & 0x1)) {
                Spell_C_CancelSpell(cast, false, true, 40);
            }
        }

        cast = next;
    }
}

// VERIFY: 627f2c2d manual
// OFFSET: 0x00806AD0
int32_t Packet_SMSG_SPELL_FAILED_OTHER(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID caster;
    *msg >> caster;
    uint8_t castCount;
    msg->Get(castCount);
    uint32_t spellId;
    msg->Get(spellId);
    uint8_t reason;
    msg->Get(reason);
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 1;
    }

    PendingSpellCast local;
    PendingSpellCast* cast = FindPendingSpellCast(caster, &local, castCount, spellId, caster);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        HandleSpellFailedOrGo(unit, cast, &spell, reason);
        FinishPendingSpellCast(cast, &spell, unit);
    }

    return 1;
}

// VERIFY: 29396807 manual
// OFFSET: 0x00807060
int32_t Packet_SMSG_ITEM_COOLDOWN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    uint32_t spellId;
    msg->Get(spellId);
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell)) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

        if (item) {
            int32_t itemId = item->m_obj->m_entryID;
            SpellHistory& history = s_spellHistory[0];
            SPELLHISTORY* entry = history.m_free.Head();

            if (entry) {
                history.m_free.UnlinkNode(entry);
                history.m_history.LinkToTail(entry);
            } else {
                entry = history.m_history.NewNode(2, 0, 8);
            }

            entry->m_spellId = spellId;
            entry->m_itemId = itemId;
            entry->m_time = time;
            entry->m_recoveryTime = 30000;
            entry->m_category = 0;
            entry->m_categoryTime = 0;
            entry->m_categoryRecoveryTime = 0;
            entry->m_onHold = false;
            entry->m_startRecoveryCategory = 0;
            entry->m_startRecoveryTime = 0;
        }
    }

    CGSpellBook::UpdateCooldowns();
    CGContainerInfo::UpdateCooldowns();
    return 1;
}

// VERIFY: 011808de manual
// OFFSET: 0x008071C0
int32_t Packet_SMSG_MEETINGSTONE_IN_PROGRESS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t cooldown;
    msg->Get(cooldown);

    for (uint32_t i = 0; i < s_numSpells; i++) {
        SpellRec spell;

        if (g_spellDB.GetRecord(GetSpellIdBySlot(i, 0), &spell) && (spell.m_attributes & 0x2) && !(spell.m_attributesExB & 0x20000) && !(spell.m_attributesExD & 0x10000000)) {
            s_spellHistory[0].AddHistory(spell.m_id, 0, OsGetAsyncTimeMs(), cooldown, 0, 0, 0, false, 0, 0);
            CGSpellBook::UpdateCooldowns();
        }
    }

    return 1;
}

// VERIFY: d5a8355c auto
// OFFSET: 0x008073A0
void Spell_C_Destroy() {
    ClientServices::ClearMessageHandler(SMSG_SPELL_START);
    ClientServices::ClearMessageHandler(SMSG_SPELL_GO);
    ClientServices::ClearMessageHandler(SMSG_CAST_FAILED);
    ClientServices::ClearMessageHandler(SMSG_SPELL_FAILURE);
    ClientServices::ClearMessageHandler(SMSG_SPELL_FAILED_OTHER);
    ClientServices::ClearMessageHandler(SMSG_PET_CAST_FAILED);
    ClientServices::ClearMessageHandler(SMSG_SPELL_COOLDOWN);
    ClientServices::ClearMessageHandler(SMSG_ITEM_COOLDOWN);
    ClientServices::ClearMessageHandler(SMSG_COOLDOWN_EVENT);
    ClientServices::ClearMessageHandler(SMSG_CLEAR_COOLDOWN);
    ClientServices::ClearMessageHandler(SMSG_MODIFY_COOLDOWN);
    ClientServices::ClearMessageHandler(SMSG_COOLDOWN_CHEAT);
    ClientServices::ClearMessageHandler(SMSG_PET_TAME_FAILURE);
    ClientServices::ClearMessageHandler(SMSG_SPELL_DELAYED);
    ClientServices::ClearMessageHandler(MSG_CHANNEL_START);
    ClientServices::ClearMessageHandler(MSG_CHANNEL_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_PLAY_SPELL_VISUAL);
    ClientServices::ClearMessageHandler(SMSG_PLAY_SPELL_IMPACT);
    ClientServices::ClearMessageHandler(SMSG_SET_FLAT_SPELL_MODIFIER);
    ClientServices::ClearMessageHandler(SMSG_SET_PCT_SPELL_MODIFIER);
    ClientServices::ClearMessageHandler(SMSG_CANCEL_AUTO_REPEAT);
    ClientServices::ClearMessageHandler(SMSG_GAME_OBJECT_RESET_STATE);
    ClientServices::ClearMessageHandler(SMSG_FEIGN_DEATH_RESISTED);
    ClientServices::ClearMessageHandler(SMSG_SPELL_UPDATE_CHAIN_TARGETS);
    ClientServices::ClearMessageHandler(SMSG_NOTIFY_DEST_LOC_SPELL_CAST);
    ClientServices::ClearMessageHandler(SMSG_ON_CANCEL_EXPECTED_RIDE_VEHICLE_AURA);
    ClientServices::ClearMessageHandler(SMSG_SET_PROJECTILE_POSITION);
    ClientServices::ClearMessageHandler(SMSG_RESET_RANGED_COMBAT_TIMER);
    ClientServices::ClearMessageHandler(SMSG_RESUME_CAST_BAR);
    s_itemCooldowns.InternalClear(0);

    for (uint32_t i = 0; i < sizeof(s_spellHistory) / sizeof(s_spellHistory[0]); i++) {
        s_spellHistory[i].ClearAll();
    }

    if (s_spellWorldObject) {
        World::ObjectDestroy(s_spellWorldObject);
        s_spellWorldObject = nullptr;
    }

    if (s_spellWorldModel) {
        s_spellWorldModel->Release();
        s_spellWorldModel = nullptr;
    }

    s_pendingSpellCasts.Clear();
    s_freePendingSpellCasts.Clear();
    s_periodicClientTriggers.Clear();
    s_destLocSpellCasts.InternalClear(0);
    s_enableErrorSpeechCVar = nullptr;
}

// VERIFY: f60bc498 manual
// OFFSET: 0x00807D40
int32_t SpellHistory::RecentlyCast(int32_t spellId, int32_t itemId) {
    uint32_t now = OsGetAsyncTimeMs();

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = this->m_history.Next(history)) {
        if (history->m_spellId == spellId && history->m_itemId == itemId && static_cast<int32_t>(now - history->m_time - 1500) < 0) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 293fcda8 manual
// OFFSET: 0x007FEA30
ItemSubClassRec* FindAnyItemSubclassRec(int32_t classId, int32_t subclassMask) {
    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        ItemSubClassRec* rec = i < 0 || i >= g_itemSubClassDB.m_numRecords ? nullptr : &g_itemSubClassDB.m_records[i];

        if (rec->m_classID == classId && ((1 << rec->m_subClassID) & subclassMask)) {
            return rec;
        }
    }

    return nullptr;
}

// VERIFY: e5a5e4cb manual
// OFFSET: 0x00805610
void ShowPetSpellFailed(PendingSpellCast* cast, const SpellRec* spell, int32_t reason, int32_t misc1, int32_t misc2) {
    char format[1024];
    char text[1024];

    switch (reason) {
        case 1:
            CGGameUI::DisplayError(ERR_PET_SPELL_AFFECTING_COMBAT);
            break;

        case 23:
            CGGameUI::DisplayError(ERR_PET_SPELL_DEAD);
            break;

        case 29:
        case 30:
        case 31: {
            FormatString(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE, format, sizeof(format));
            bool found = false;

            for (int32_t i = 0; i < g_itemSubClassMaskDB.m_numRecords; i++) {
                ItemSubClassMaskRec* rec = i < 0 || i >= g_itemSubClassMaskDB.m_numRecords ? nullptr : &g_itemSubClassMaskDB.m_records[i];

                if (rec->m_classID == misc1 && rec->m_mask == misc2) {
                    SStrPrintf(text, sizeof(text), format, rec->m_name);
                    CGGameUI::DisplayError(ERR_SPELL_FAILED_S, text);
                    found = true;
                    break;
                }
            }

            if (!found) {
                ItemSubClassRec* rec = FindAnyItemSubclassRec(misc1, misc2);

                if (rec) {
                    SStrPrintf(text, sizeof(text), format, rec->m_displayName);
                    CGGameUI::DisplayError(ERR_SPELL_FAILED_S, text);
                }
            }

            break;
        }

        case 56:
            CGGameUI::DisplayError(ERR_PET_SPELL_NOPATH);
            break;

        case 57:
            CGGameUI::DisplayError(ERR_PET_SPELL_NOT_BEHIND);
            break;

        case 67:
            CGGameUI::DisplayError(static_cast<UIErrors>(((spell->m_attributes & 0x10) | 0x340) >> 4));
            break;

        case 85: {
            PowerDisplayRec* display = g_powerDisplayDB.GetRecord(spell->m_powerDisplayID);

            if (display) {
                char power[128];
                FormatString(display->m_globalStringBaseTag, -1, GENDER_NOT_APPLICABLE, power, 127);
                CGGameUI::DisplayError(ERR_OUT_OF_POWER_DISPLAY, power);
            } else if (spell->m_powerType == POWER_TYPE_HEALTH) {
                CGGameUI::DisplayError(ERR_OUT_OF_HEALTH);
            } else {
                CGGameUI::DisplayError(s_powerErrors[spell->m_powerType]);
            }

            break;
        }

        case 94: {
            FormatString(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            uint32_t count = g_spellShapeshiftFormDB.m_numRecords;

            if (count > 64) {
                count = 64;
            }

            for (uint32_t i = 0; i < count; i++) {
                if (!SpellRec::UsableInShapeshift(spell, i)) {
                    continue;
                }

                SpellShapeshiftFormRec* form = static_cast<int32_t>(i) < 0 || static_cast<int32_t>(i) >= g_spellShapeshiftFormDB.m_numRecords ? nullptr : &g_spellShapeshiftFormDB.m_records[i];

                if (form && form->m_name && *form->m_name) {
                    SStrPrintf(format, sizeof(format), text, form->m_name);
                    CGGameUI::DisplayError(ERR_SPELL_FAILED_SHAPESHIFT_FORM_S, format);
                    break;
                }
            }

            break;
        }

        case 97:
            CGGameUI::DisplayError(ERR_PET_SPELL_OUT_OF_RANGE);
            break;

        case 101: {
            FormatString(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            AreaTableRec* area = g_areaTableDB.GetRecord(misc1);

            if (area) {
                SStrPrintf(format, sizeof(format), text, area->m_areaName);
                CGGameUI::DisplayError(ERR_SPELL_FAILED_S, format);
            }

            break;
        }

        case 102: {
            FormatString(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            SpellFocusObjectRec* focus = g_spellFocusObjectDB.GetRecord(misc1);

            if (focus) {
                SStrPrintf(format, sizeof(format), text, focus->m_name);
                CGGameUI::DisplayError(ERR_SPELL_FAILED_S, format);
            }

            break;
        }

        case 103:
            CGGameUI::DisplayError(ERR_PET_SPELL_ROOTED);
            break;

        case 109:
            CGGameUI::DisplayError(ERR_PET_SPELL_TARGETS_DEAD);
            break;

        case 147: {
            FormatString(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE, text, sizeof(text));
            SpellMechanicRec* mechanic = g_spellMechanicDB.GetRecord(misc1);

            if (mechanic) {
                SStrPrintf(format, sizeof(format), text, mechanic->m_stateName);
                CGGameUI::DisplayError(ERR_SPELL_FAILED_S, format);
            }

            break;
        }

        case 172:
            GetCustomSpellFailedString(text, sizeof(text), misc1);
            break;

        default:
            CGGameUI::DisplayError(ERR_SPELL_FAILED_S, FrameScript_GetText(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE));
            break;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        HandleSpellFailedOrGo(unit, cast, spell, reason);
        FinishPendingSpellCast(cast, spell, unit);
    }
}

// VERIFY: 3494eae5 manual
// OFFSET: 0x00805B10
void Spell_C_SetCooldownLeft(int32_t spellId, uint32_t itemId, uint32_t category, int32_t cooldownLeft, int32_t categoryCooldownLeft, bool onHold, int32_t pet) {
    uint32_t now = OsGetAsyncTimeMs();
    int32_t time = 0;
    int32_t recovery = 0;
    int32_t categoryTime = 0;
    int32_t categoryRecovery = 0;
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    ItemStats_C* stats = nullptr;
    int32_t index = 0;

    if (itemId) {
        WOWGUID none = { 0 };
        stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

        if (stats) {
            for (index = 0; index < 5; index++) {
                if (stats->m_spellId[index] == static_cast<uint32_t>(spellId)) {
                    break;
                }
            }

            if (index >= 5) {
                stats = nullptr;
            }
        }
    }

    if (cooldownLeft > 0) {
        int32_t value;

        if (!stats || (value = static_cast<int32_t>(stats->m_spellCooldown[index])) < 0) {
            value = spell.m_recoveryTime;
        }

        if (value <= cooldownLeft) {
            value = cooldownLeft;
        }

        if (Spell_C_GetSpellModifiers(&spell, 11, time, recovery)) {
            value = (time + value) * recovery / 100;
        }

        recovery = value;
        time = now - (value - cooldownLeft);
    }

    if (categoryCooldownLeft > 0) {
        int32_t value;

        if (!stats || (value = static_cast<int32_t>(stats->m_spellCategoryCooldown[index])) < 0) {
            categoryRecovery = spell.m_categoryRecoveryTime;
            Spell_C_ModifyCategoryRecovery(categoryRecovery, &spell, pet);
            value = categoryRecovery;
        }

        if (value <= categoryCooldownLeft) {
            value = categoryCooldownLeft;
        }

        categoryTime = now - (value - categoryCooldownLeft);
        categoryRecovery = value;
    }

    if (recovery || categoryRecovery || onHold) {
        SpellHistory& history = s_spellHistory[pet];
        SPELLHISTORY* entry = history.m_free.Head();

        if (entry) {
            history.m_free.UnlinkNode(entry);
            history.m_history.LinkToTail(entry);
        } else {
            entry = history.m_history.NewNode(2, 0, 8);
        }

        entry->m_spellId = spellId;
        entry->m_itemId = itemId;
        entry->m_time = time;
        entry->m_recoveryTime = recovery;
        entry->m_category = category;
        entry->m_categoryTime = categoryTime;
        entry->m_categoryRecoveryTime = categoryRecovery;
        entry->m_onHold = onHold;
        entry->m_startRecoveryCategory = 0;
        entry->m_startRecoveryTime = 0;
    }
}

// VERIFY: 8f7722c4 manual
// OFFSET: 0x00805D70
void Spell_C_StartSpellCooldown(const SpellRec* spell, CGUnit_C* unit) {
    if (!spell->m_startRecoveryCategory && !spell->m_startRecoveryTime) {
        return;
    }

    int32_t time = spell->m_startRecoveryTime;
    int32_t recovery = time;
    int32_t flat;
    int32_t pct;

    if (Spell_C_GetSpellModifiers(spell, 21, flat, pct)) {
        recovery = (recovery + flat) * pct / 100;
    }

    if (spell->m_startRecoveryCategory == 133 && time == 1500 && spell->m_defenseType != 2 && spell->m_defenseType != 3 && !(spell->m_attributes & 0x12)) {
        float scaled = recovery * unit->m_unit->m_unitModCastSpeed;
        recovery = CMath::fint_(scaled - 0.5f);

        if (static_cast<uint32_t>(recovery - 1000) > 500) {
            recovery = recovery - 1000 < 0 ? 1000 : 1500;
        }
    }

    if (!recovery) {
        return;
    }

    bool pet = unit->m_obj->m_guid == CGPetInfo::GetPet(0);
    bool onHold = Spell_C_HasCooldownOnEvent(spell, unit->m_obj->m_guid);
    s_spellHistory[pet].AddHistory(spell->m_id, 0, OsGetAsyncTimeMs(), 0, 0, OsGetAsyncTimeMs(), 0, onHold, spell->m_startRecoveryCategory, recovery);
    CGSpellBook::UpdateCooldowns();

    if (spell->m_attributes & 0x2000000) {
        CGSpellBook::UpdateUsable();
    }

    if (pet) {
        CGPetInfo::UpdateCooldowns();
    }
}

// VERIFY: 10210842 manual
// OFFSET: 0x00807DA0
int32_t SpellHistory::IsOnHold(int32_t spellId, int32_t itemId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    int32_t category = spell.m_category;

    if (itemId) {
        WOWGUID none = { 0 };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

        if (stats) {
            for (uint32_t i = 0; i < 5; i++) {
                if (stats->m_spellId[i] == static_cast<uint32_t>(spellId) && static_cast<int32_t>(stats->m_spellCategory[i]) > 0) {
                    category = stats->m_spellCategory[i];
                }
            }
        }
    }

    for (SPELLHISTORY* history = this->m_history.Head(); history; history = this->m_history.Next(history)) {
        if (history->m_spellId == spellId && history->m_itemId == itemId && (history->m_recoveryTime || history->m_onHold) && history->m_onHold) {
            return 1;
        }

        if (history->m_category == category && history->m_categoryRecoveryTime && history->m_onHold) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 50546e74 manual
// OFFSET: 0x00807F10
static UIErrors GetSpellFailedError(int32_t reason, const SpellRec* spell, int32_t misc) {
    UIErrors error = ERR_SPELL_FAILED_S;

    switch (reason) {
        case SPELL_FAILED_ALREADY_AT_FULL_HEALTH:
            return ERR_SPELL_FAILED_ALREADY_AT_FULL_HEALTH;

        case SPELL_FAILED_ALREADY_AT_FULL_MANA:
            return ERR_SPELL_FAILED_ALREADY_AT_FULL_MANA;

        case SPELL_FAILED_ALREADY_AT_FULL_POWER:
            return ERR_SPELL_FAILED_ALREADY_AT_FULL_POWER_S;

        case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
            return ERR_GENERIC_NO_TARGET;

        case SPELL_FAILED_BAD_TARGETS:
            if (spell->m_targets & 0x10) {
                return ERR_INVALID_ITEM_TARGET;
            }

            if (spell->m_targets & 0x20000) {
                return ERR_INVALID_GLYPH_SLOT;
            }

            return error;

        case SPELL_FAILED_EQUIPPED_ITEM:
            return ERR_SPELL_FAILED_EQUIPPED_ITEM;

        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
            return ERR_SPELL_FAILED_EQUIPPED_ITEM_CLASS_S;

        case SPELL_FAILED_ITEM_NOT_READY:
            switch (spell->m_category) {
                case 4:
                case 9:
                    return ERR_POTION_COOLDOWN;

                case 10:
                case 11:
                    return ERR_FOOD_COOLDOWN;

                default:
                    return ERR_ITEM_COOLDOWN;
            }

        case SPELL_FAILED_NEED_AMMO:
            return ERR_NOAMMO_S;

        case SPELL_FAILED_NOT_READY:
            switch (spell->m_category) {
                case 4:
                case 9:
                    error = ERR_POTION_COOLDOWN;
                    break;

                case 10:
                case 11:
                    error = ERR_FOOD_COOLDOWN;
                    break;

                default:
                    error = static_cast<UIErrors>(((spell->m_attributes & 0x10) | 0x340) >> 4);
                    break;
            }

            if (misc || !s_spellHistory[0].IsOnHold(spell->m_id, 0)) {
                return error;
            }

            s_spellHistory[0].ClearOnHold(spell->m_id, 0, true);
            CGSpellBook::UpdateCooldowns();
            CGSpellBook::UpdateUsable();
            CGContainerInfo::UpdateCooldowns();
            return error;

        case SPELL_FAILED_NOT_UNSHEATHED:
            return ERR_SPELL_FAILED_NOTUNSHEATHED;

        case SPELL_FAILED_ONLY_SHAPESHIFT:
            return ERR_SPELL_FAILED_SHAPESHIFT_FORM_S;

        case SPELL_FAILED_OUT_OF_RANGE:
            return ERR_SPELL_OUT_OF_RANGE;

        case SPELL_FAILED_REAGENTS:
            return ERR_SPELL_FAILED_REAGENTS;

        case SPELL_FAILED_TOO_MANY_OF_ITEM:
            if (misc > 0) {
                error = ERR_ITEM_MAX_LIMIT_CATEGORY_COUNT_EXCEEDED_IS;
            }

            return error;

        case SPELL_FAILED_TOTEMS:
            return ERR_SPELL_FAILED_TOTEMS;

        case SPELL_FAILED_PLAY_TIME:
            return ERR_PLAY_TIME_EXCEEDED;

        case SPELL_FAILED_ON_USE_ENCHANT:
            return ERR_BAD_ON_USE_ENCHANT;

        case SPELL_FAILED_TOO_MANY_SOCKETS:
            return ERR_TOO_MANY_SOCKETS;

        case SPELL_FAILED_NO_VALID_TARGETS:
            return ERR_GENERIC_NO_VALID_TARGETS;

        case SPELL_FAILED_MAX_SOCKETS:
            return ERR_MAX_SOCKETS;

        default:
            return error;
    }
}

// VERIFY: 613e78c7 manual
// OFFSET: 0x00808200
void Spell_C_SpellFailed(PendingSpellCast* cast, const SpellRec* spell, int32_t reason, int32_t misc1, int32_t misc2, bool resetCooldown) {
    int32_t spellId = cast->m_data.m_spellId;
    bool suppress = false;

    if (cast == s_delayedTrajectoryCast) {
        s_delayedTrajectoryCast = nullptr;
    }

    UpdateTradeSkillRecast(spellId);

    if (s_autoRepeatingSpell && (reason == SPELL_FAILED_NOT_READY || reason == SPELL_FAILED_SPELL_IN_PROGRESS)) {
        SpellRec autoRepeat;

        if (g_spellDB.GetRecord(s_autoRepeatingSpell, &autoRepeat) && (autoRepeat.m_attributesExC & 0x400000)) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_CANCEL_CAST));
            msg.Put(static_cast<uint8_t>(0));
            msg.Put(static_cast<uint32_t>(s_autoRepeatingSpell));
            msg.Finalize();
            ClientServices::Send2(&msg);
            Spell_C_CancelAutoRepeat(true);
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && reason != SPELL_FAILED_DONT_REPORT) {
        SI2::PlaySpellFizzleSound(spellId, player);
    }

    UIErrors error = GetSpellFailedError(reason, spell, misc1);

    if (player) {
        if (reason == SPELL_FAILED_NEED_AMMO) {
            CGTutorial::TriggerTutorial(44);
        }

        bool autoRepeat = false;

        if (cast == s_targetingSpell) {
            autoRepeat = spellId == s_autoRepeatingSpell;
        } else if (spellId == s_autoRepeatingSpell) {
            autoRepeat = true;
        } else {
            switch (reason) {
                case SPELL_FAILED_LINE_OF_SIGHT:
                case SPELL_FAILED_NEED_AMMO:
                case SPELL_FAILED_NEED_AMMO_POUCH:
                case SPELL_FAILED_NEED_EXOTIC_AMMO:
                case SPELL_FAILED_OUT_OF_RANGE:
                case SPELL_FAILED_TOO_CLOSE:
                case SPELL_FAILED_UNIT_NOT_INFRONT:
                    player->CancelRangedMode();
                    break;
            }
        }

        if (autoRepeat) {
            bool cancel = reason != SPELL_FAILED_DONT_REPORT;
            bool check = true;

            if (spellId == s_autoRangedCombatSpell && resetCooldown) {
                switch (reason) {
                    case SPELL_FAILED_BAD_IMPLICIT_TARGETS:
                    case SPELL_FAILED_BAD_TARGETS:
                    case SPELL_FAILED_CASTER_DEAD:
                    case SPELL_FAILED_EQUIPPED_ITEM:
                    case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
                    case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
                    case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND:
                    case SPELL_FAILED_NEED_AMMO:
                    case SPELL_FAILED_OUT_OF_RANGE:
                    case SPELL_FAILED_TOO_CLOSE:
                    case SPELL_FAILED_UNIT_NOT_INFRONT:
                        break;

                    default:
                        check = false;
                        break;
                }
            }

            if (check && cancel) {
                Spell_C_CancelAutoRepeat(true);

                switch (reason) {
                    case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
                    case SPELL_FAILED_NEED_AMMO:
                    case SPELL_FAILED_NEED_AMMO_POUCH:
                    case SPELL_FAILED_NEED_EXOTIC_AMMO:
                    case SPELL_FAILED_OUT_OF_RANGE:
                    case SPELL_FAILED_TOO_CLOSE:
                    case SPELL_FAILED_UNIT_NOT_INFRONT:
                        player->CancelRangedMode();
                        break;
                }
            }
        }

        if (spellId == s_autoRangedCombatSpell) {
            if (reason == s_lastAutoRangedCombatError) {
                suppress = true;
            }

            s_lastAutoRangedCombatError = reason;
        }

        uint32_t now = OsGetAsyncTimeMs();

        if (static_cast<uint32_t>(spellId) == s_lastFailedSpellId && reason == s_lastFailedReason) {
            if (static_cast<int32_t>(now - s_lastFailedTime - 3000) < 0) {
                suppress = true;
            }
        } else {
            s_lastFailedSpellId = spellId;
            s_lastFailedReason = reason;
        }

        s_lastFailedTime = now;
    }

    if (reason == SPELL_FAILED_DONT_REPORT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            HandleSpellFailedOrGo(unit, cast, spell, 27);
            FinishPendingSpellCast(cast, spell, unit);
        }

        return;
    }

    char format[1024];
    format[0] = 0;
    bool pet = spell->m_effect[0] == SPELL_EFFECT_LEARN_PET_SPELL || (spell->m_effect[0] == SPELL_EFFECT_LEARN_SPELL && spell->m_implicitTargetA[0] == 5);

    if (pet) {
        char token[64];
        SStrPrintf(token, sizeof(token), "%s_PET", GetSpellFailedResult(reason));

        if (player) {
            SStrCopy(format, player->GetFrameScriptText(token, -1), sizeof(format));
        } else {
            SStrCopy(format, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), sizeof(format));
        }
    }

    if (!format[0]) {
        if (reason == SPELL_FAILED_CUSTOM_ERROR) {
            GetCustomSpellFailedString(format, sizeof(format), misc1);
            suppress = true;
        }

        if (!format[0]) {
            if (player) {
                SStrCopy(format, player->GetFrameScriptText(GetSpellFailedResult(reason), -1), sizeof(format));
            } else {
                SStrCopy(format, FrameScript_GetText(GetSpellFailedResult(reason), -1, GENDER_NOT_APPLICABLE), sizeof(format));
            }
        }
    }

    if (resetCooldown) {
        s_spellHistory[0].RemoveStartCooldown(spellId);
    }

    if (spell->m_startRecoveryCategory && spell->m_startRecoveryTime) {
        CGUIMacros::m_state &= ~0x4u;
    }

    char text[1024];
    const char* message = format;

    switch (reason) {
        case SPELL_FAILED_ALREADY_AT_FULL_POWER: {
            if (!player) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            uint32_t powerType = player->m_unit->m_unitFieldBytes0[3];

            if (powerType > POWER_TYPE_RUNIC_POWER) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            const char* power = FrameScript_GetText(GetPowerString(static_cast<POWER_TYPE>(powerType)), -1, GENDER_NOT_APPLICABLE);

            if (!power || !*power) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            SStrCopy(text, power, sizeof(text));
            message = text;
            break;
        }

        case SPELL_FAILED_EQUIPPED_ITEM_CLASS:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND:
        case SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND: {
            bool found = false;

            for (int32_t i = 0; i < g_itemSubClassMaskDB.m_numRecords; i++) {
                ItemSubClassMaskRec* rec = i < 0 || i >= g_itemSubClassMaskDB.m_numRecords ? nullptr : &g_itemSubClassMaskDB.m_records[i];

                if (rec->m_classID == misc1 && rec->m_mask == misc2) {
                    SStrPrintf(text, sizeof(text), format, rec->m_name);
                    message = text;
                    found = true;
                    break;
                }
            }

            if (!found) {
                ItemSubClassRec* rec = FindAnyItemSubclassRec(misc1, misc2);

                if (rec) {
                    SStrPrintf(text, sizeof(text), format, rec->m_displayName);
                    message = text;
                }
            }

            break;
        }

        case SPELL_FAILED_NEED_EXOTIC_AMMO: {
            ItemSubClassRec* rec = FindAnyItemSubclassRec(6, 1 << misc1);

            if (rec) {
                SStrPrintf(text, sizeof(text), format, rec->m_displayName);
                message = text;
            }

            break;
        }

        case SPELL_FAILED_NEED_MORE_ITEMS: {
            WOWGUID guid = { static_cast<uint64_t>(static_cast<int64_t>(spellId)) | 0x1FE0000000000000ull };
            ItemStats_C* stats = g_itemDBCache.GetRecord(misc1, guid, SpellMissingItemsCallback, reinterpret_cast<void*>(static_cast<intptr_t>(misc2)), false);

            if (!stats) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            SStrPrintf(text, sizeof(text), format, misc2, stats->GetName(FrameScript_GetPluralIndex(misc2)));
            message = text;
            break;
        }

        case SPELL_FAILED_NO_POWER: {
            PowerDisplayRec* display = g_powerDisplayDB.GetRecord(spell->m_powerDisplayID);

            if (display) {
                char power[128];
                FormatString(display->m_globalStringBaseTag, -1, GENDER_NOT_APPLICABLE, power, 127);
                CGGameUI::DisplayError(ERR_OUT_OF_POWER_DISPLAY, power);
            } else if (spell->m_powerType == POWER_TYPE_HEALTH) {
                error = ERR_OUT_OF_HEALTH;
            } else {
                error = s_powerErrors[spell->m_powerType];
            }

            text[0] = 0;
            message = text;
            break;
        }

        case SPELL_FAILED_ONLY_SHAPESHIFT: {
            uint32_t count = g_spellShapeshiftFormDB.m_numRecords;

            if (count > 64) {
                count = 64;
            }

            if (!count) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            char forms[512];
            bool first = true;

            for (uint32_t i = 0; i < count; i++) {
                if (!SpellRec::UsableInShapeshift(spell, i)) {
                    continue;
                }

                SpellShapeshiftFormRec* form = g_spellShapeshiftFormDB.GetRecordByIndex(i);

                if (!form || !form->m_name || !*form->m_name) {
                    continue;
                }

                if (first) {
                    SStrCopy(forms, form->m_name, sizeof(forms));
                    first = false;
                } else {
                    SStrPack(forms, ", ", sizeof(forms));
                    SStrPack(forms, form->m_name, sizeof(forms));
                }
            }

            if (first) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            SStrPrintf(text, sizeof(text), format, forms);
            message = text;
            break;
        }

        case SPELL_FAILED_REAGENTS:
        case SPELL_FAILED_TOTEMS: {
            if (reason == SPELL_FAILED_REAGENTS && misc1 == -2) {
                error = static_cast<UIErrors>(0);
                text[0] = 0;
                message = text;
                break;
            }

            WOWGUID guid = { static_cast<uint64_t>(static_cast<int64_t>(spellId)) | 0x1FE0000000000000ull };
            ItemStats_C* stats = g_itemDBCache.GetRecord(misc1, guid, SpellMissingItemCallback, reinterpret_cast<void*>(static_cast<intptr_t>(reason)), false);

            if (!stats) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            SStrPrintf(text, sizeof(text), format, stats->GetName(0));
            message = text;
            break;
        }

        case SPELL_FAILED_REQUIRES_AREA: {
            AreaTableRec* area = g_areaTableDB.GetRecord(misc1);

            if (area) {
                SStrPrintf(text, sizeof(text), format, area->m_areaName);
                message = text;
            }

            break;
        }

        case SPELL_FAILED_REQUIRES_SPELL_FOCUS: {
            SpellFocusObjectRec* focus = g_spellFocusObjectDB.GetRecord(misc1);

            if (focus) {
                SStrPrintf(text, sizeof(text), format, focus->m_name);
                message = text;
            }

            break;
        }

        case SPELL_FAILED_TOO_MANY_OF_ITEM: {
            if (!misc1) {
                break;
            }

            ItemLimitCategoryRec* limit = g_itemLimitCategoryDB.GetRecord(misc1);

            if (!limit) {
                break;
            }

            CGGameUI::DisplayError(error, limit->m_quantity, limit->m_name);
            UnitCombatLogSpellFail(ClntObjMgrGetActivePlayer(), spellId, CGGameUI::LastErrorMessage());
            suppress = true;
            break;
        }

        case SPELL_FAILED_TOTEM_CATEGORY: {
            TotemCategoryRec* totem = g_totemCategoryDB.GetRecord(misc1);

            if (!totem) {
                Spell_C_FailPendingCast(cast, spell, 27);
                return;
            }

            SStrPrintf(text, sizeof(text), format, totem->m_name);
            message = text;
            break;
        }

        case SPELL_FAILED_PREVENTED_BY_MECHANIC: {
            SpellMechanicRec* mechanic = g_spellMechanicDB.GetRecord(misc1);

            if (mechanic) {
                SStrPrintf(text, sizeof(text), format, mechanic->m_stateName);
                message = text;
            }

            break;
        }

        case SPELL_FAILED_MIN_SKILL: {
            SkillLineRec* skill = g_skillLineDB.GetRecord(misc1);
            SStrPrintf(text, sizeof(text), format, skill ? skill->m_displayName : "UNKNOWN", misc2);
            message = text;
            break;
        }

        case SPELL_FAILED_UNIQUE_GLYPH: {
            CGObject_C* object = ClntObjMgrObjectPtr(cast->m_data.m_castItem, TYPE_OBJECT, __FILE__, __LINE__);
            const char* name = spell->m_name;

            if (object && (object->m_obj->m_type & TYPE_ITEM)) {
                WOWGUID none = { 0 };
                ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);

                if (stats && stats->GetName(0)) {
                    name = stats->GetName(0);
                }
            }

            SStrPrintf(text, sizeof(text), format, name);
            break;
        }

        case SPELL_FAILED_FISHING_TOO_LOW:
            SStrPrintf(text, sizeof(text), format, misc1);
            message = text;
            break;
    }

    if (!suppress) {
        CGGameUI::DisplayError(error, message);

        if (*message) {
            UnitCombatLogSpellFail(ClntObjMgrGetActivePlayer(), spellId, message);
        } else {
            UnitCombatLogSpellFail(ClntObjMgrGetActivePlayer(), spellId, CGGameUI::LastErrorMessage());
        }
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        HandleSpellFailedOrGo(unit, cast, spell, reason);
        FinishPendingSpellCast(cast, spell, unit);
    }
}

// VERIFY: 0c19acb4 manual
// OFFSET: 0x00808DE0
void Spell_C_PendingItemCooldownCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    HASHKEY_NONE key;
    ITEMCOOLDOWNHASHNODE* node = s_itemCooldowns.Ptr(id, key);

    if (!node) {
        return;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (stats) {
        uint32_t index;

        for (index = 0; index < 5; index++) {
            if (stats->m_spellId[index] == static_cast<uint32_t>(node->m_spellId)) {
                break;
            }
        }

        if (index < 5) {
            SpellRec spell;
            g_spellDB.GetRecord(node->m_spellId, &spell);
            int32_t recovery = static_cast<int32_t>(stats->m_spellCooldown[index]) >= 0 ? static_cast<int32_t>(stats->m_spellCooldown[index]) : spell.m_recoveryTime;
            int32_t category = 0;
            int32_t categoryRecovery = 0;

            if (param) {
                category = static_cast<int32_t>(stats->m_spellCategory[index]) > 0 ? static_cast<int32_t>(stats->m_spellCategory[index]) : spell.m_category;
                categoryRecovery = static_cast<int32_t>(stats->m_spellCategoryCooldown[index]) >= 0 ? static_cast<int32_t>(stats->m_spellCategoryCooldown[index]) : spell.m_categoryRecoveryTime;
            }

            if (recovery || categoryRecovery) {
                bool onHold = node->m_onHold;
                int32_t spellId = node->m_spellId;
                uint32_t time = node->m_time;

                if (recovery || categoryRecovery || onHold) {
                    SpellHistory& history = s_spellHistory[0];
                    SPELLHISTORY* entry = history.m_free.Head();

                    if (entry) {
                        history.m_free.UnlinkNode(entry);
                        history.m_history.LinkToTail(entry);
                    } else {
                        entry = history.m_history.NewNode(2, 0, 8);
                    }

                    entry->m_spellId = spellId;
                    entry->m_itemId = id;
                    entry->m_time = time;
                    entry->m_recoveryTime = recovery;
                    entry->m_category = category;
                    entry->m_categoryTime = time;
                    entry->m_categoryRecoveryTime = categoryRecovery;
                    entry->m_onHold = onHold;
                    entry->m_startRecoveryCategory = 0;
                    entry->m_startRecoveryTime = 0;
                }
            }
        }

        CGSpellBook::UpdateCooldowns();
        CGContainerInfo::UpdateCooldowns();
    }

    s_itemCooldowns.Delete(node);
}

// VERIFY: 588a8016 auto
// OFFSET: 0x00809100
int32_t Spell_C_WasSpellRecentlyUsed(int32_t spellId, int32_t pet) {
    if (!g_secureAbilityToggleCVar->m_intValue) {
        return 0;
    }

    return s_spellHistory[pet].RecentlyCast(spellId, 0);
}

// VERIFY: 0adfd8d9 manual
// OFFSET: 0x00809130
int32_t Spell_C_WasItemRecentlyUsed(int32_t itemId) {
    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    for (uint32_t i = 0; i < sizeof(stats->m_spellId) / sizeof(stats->m_spellId[0]); i++) {
        if (static_cast<int32_t>(stats->m_spellId[i]) > 0 && !stats->m_spellTrigger[i]) {
            return s_spellHistory[0].RecentlyCast(stats->m_spellId[i], itemId);
        }
    }

    return 0;
}

// VERIFY: 35a53b59 auto
// OFFSET: 0x007FFD60
static int32_t FindAmmoByType(const CGItem_C* item, void* param) {
    AmmoSearch* search = static_cast<AmmoSearch*>(param);

    if (item->GetClassID() == 6 && item->GetSubtypeID() == search->m_ammoType) {
        if (!search->m_ammoId || item->m_obj->m_entryID == search->m_ammoId) {
            search->m_item = item;
            return 1;
        }
    } else if (const_cast<CGItem_C*>(item)->GetBag() && search->m_item) {
        search->m_item = const_cast<CGItem_C*>(item)->GetBag()->FindItem(FindAmmoByType, search, 0);
        return 1;
    }

    return 0;
}

// VERIFY: 58b676ca manual
// OFFSET: 0x007FFDE0
bool Spell_C_IsActivePlayerCasting() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    return player && player->m_castSpellId;
}

// VERIFY: 613e3fb2 manual
// OFFSET: 0x00800BA0
bool OnCancelCombat() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !(player->m_obj->m_type & TYPE_PLAYER) || !(player->m_combatModeFlags & 0x4)) {
        return false;
    }

    player->SetCombatMode();
    return true;
}

// VERIFY: c523775a manual
// OFFSET: 0x00803850
static bool RangeCheck(CGUnit_C* caster, int32_t spellId, CGObject_C* target, uint8_t* tooFar) {
    if (tooFar) {
        *tooFar = 0;
    }

    float minRange;
    float maxRange;
    Spell_C::GetSpellRange(caster, spellId, &minRange, &maxRange, target);
    C3Vector targetPos = target->GetPosition();
    C3Vector casterPos = caster->GetPosition();
    float dx = casterPos.x - targetPos.x;
    float dy = casterPos.y - targetPos.y;
    float dz = casterPos.z - targetPos.z;
    float distance = dx * dx + dz * dz + dy * dy;

    if (minRange * minRange > distance) {
        return false;
    }

    if (maxRange * maxRange >= distance) {
        return true;
    }

    if (tooFar) {
        *tooFar = 1;
    }

    return false;
}

// VERIFY: 9b57db2a auto
// OFFSET: 0x008091D0
bool Spell_C_HaveSpellTokens(CGUnit_C* unit, const SpellRec* spell, bool report, PendingSpellCast* cast) {
    if (!unit->IsActivePlayer()) {
        return true;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(unit);

    for (uint32_t i = 0; i < sizeof(spell->m_totem) / sizeof(spell->m_totem[0]); i++) {
        if (spell->m_totem[i] && !player->m_bag.FindItemOfType(spell->m_totem[i], 0)) {
            if (report) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_TOTEMS, spell->m_totem[i], -1, false);
            }

            return false;
        }
    }

    for (uint32_t i = 0; i < sizeof(spell->m_requiredTotemCategoryID) / sizeof(spell->m_requiredTotemCategoryID[0]); i++) {
        if (spell->m_requiredTotemCategoryID[i] && !player->m_bag.FindTotemCategory(spell->m_requiredTotemCategoryID[i], 0)) {
            if (report) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_TOTEM_CATEGORY, spell->m_requiredTotemCategoryID[i], -1, false);
            }

            return false;
        }
    }

    if (Spell_C_HasNoReagentCost(player, spell)) {
        return true;
    }

    for (uint32_t i = 0; i < sizeof(spell->m_reagent) / sizeof(spell->m_reagent[0]); i++) {
        if (!spell->m_reagent[i]) {
            continue;
        }

        int32_t count = player->m_bag.GetItemTypeCount(spell->m_reagent[i], 0);
        int32_t needed = spell->m_reagentCount[i];
        WOWGUID none = { 0 };
        ItemStats_C* stats = g_itemDBCache.GetRecord(spell->m_reagent[i], none, nullptr, nullptr, false);

        if (stats) {
            for (uint32_t j = 0; j < 5; j++) {
                if (stats->m_spellId[j] == static_cast<uint32_t>(spell->m_id) && stats->m_spellCharges[j]) {
                    needed++;
                }
            }
        }

        if (count < needed) {
            if (report) {
                Spell_C_SpellFailed(cast, spell, SPELL_FAILED_REAGENTS, spell->m_reagent[i], -1, false);
            }

            return false;
        }
    }

    return true;
}

// VERIFY: 3e98536b auto
// OFFSET: 0x008093D0
bool Spell_C_HaveEquippedSpellItems(CGUnit_C* unit, const SpellRec* spell, bool checkAmmo, bool report, PendingSpellCast* cast) {
    if (!unit->IsActivePlayer()) {
        return true;
    }

    if ((spell->m_targets & 0x10) || spell->m_equippedItemClass < 0 || !spell->m_equippedItemSubclass) {
        return true;
    }

    uint32_t slots;

    if (spell->m_attributesExC & 0x400) {
        slots = 0x8000;
    } else if (spell->m_attributesExC & 0x1000000) {
        slots = 0x10000;
    } else {
        slots = (spell->m_attributes & 0x2) ? 0x20000 : 0xFFFFFFFF;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(unit);
    const CGItem_C* item = player->EquippedItemMeetSpellRequirements(spell, slots);

    if (!item) {
        if (report) {
            int32_t reason = (spell->m_attributesExC & 0x400) ? 30 : ((spell->m_attributesExC & 0x1000000) | 0xE800000) >> 23;
            Spell_C_SpellFailed(cast, spell, reason, spell->m_equippedItemClass, spell->m_equippedItemSubclass, false);
        }

        return false;
    }

    if ((spell->m_attributesExC & 0x1000400) == 0x1000400 && !player->EquippedItemMeetSpellRequirements(spell, 0x10000)) {
        if (report) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND, spell->m_equippedItemClass, spell->m_equippedItemSubclass, false);
        }

        return false;
    }

    if (!checkAmmo) {
        return true;
    }

    uint32_t count = g_itemSubClassDB.m_numRecords;

    for (uint32_t i = 0; i < count; i++) {
        ItemSubClassRec* subClass = g_itemSubClassDB.GetRecordByIndex(i);

        if (subClass->m_classID == item->GetClassID() && subClass->m_subClassID == item->GetSubtypeID()) {
            if (subClass->m_flags & 0x10) {
                return true;
            }

            break;
        }
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return true;
    }

    int32_t ammoType = stats->m_ammoType;

    if (!ammoType) {
        return true;
    }

    if (!CGCharacterInfo::GetActivePlayerAmmoID(player)) {
        if (report) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NEED_AMMO, ammoType, -1, false);
        }

        return false;
    }

    AmmoSearch search;
    search.m_ammoType = ammoType;
    search.m_ammoId = CGCharacterInfo::GetActivePlayerAmmoID(player);
    search.m_item = nullptr;

    if (!player->m_bag.FindItem(FindAmmoByType, &search, 0)) {
        if (report) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_NEED_AMMO, ammoType, -1, false);
        }

        return false;
    }

    return true;
}

// VERIFY: 66bd63ba manual
// OFFSET: 0x00809610
bool Spell_C_RangeCheckSelected(CGUnit_C* caster, const SpellRec* spell, WOWGUID target, uint8_t* inRange, bool report, PendingSpellCast* cast) {
    if (!target) {
        target = CGGameUI::m_lockedTarget;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return false;
    }

    uint32_t needTargets = static_cast<uint16_t>(spell->m_targets);
    bool pet = false;

    for (uint32_t i = 0; i < sizeof(spell->m_implicitTargetA) / sizeof(spell->m_implicitTargetA[0]); i++) {
        switch (spell->m_implicitTargetA[i]) {
            case 1:
                if (needTargets & 0x400) {
                    needTargets &= 0xFBFF;
                }

                break;

            case 5:
                pet = true;

                if (needTargets & 0x8000) {
                    needTargets &= 0x7FFF;
                }

                break;

            case 6:
            case 53:
                needTargets |= 0x80;
                break;

            case 21:
            case 45:
                needTargets |= 0x100;
                break;

            case 23:
                needTargets |= 0x800;
                break;

            case 25:
            case 63:
            case 64:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
            case 71:
            case 74:
            case 75:
                needTargets |= 0x2;
                break;

            case 26:
                needTargets |= 0x4000;
                break;

            case 35:
                needTargets |= 0x8;
                break;

            case 57:
            case 61:
                needTargets |= 0x4;
                break;
        }
    }

    if (pet && !(needTargets & 0x2)) {
        object = ClntObjMgrObjectPtr(CGPetInfo::GetPet(0), TYPE_OBJECT, __FILE__, __LINE__);

        if (!object) {
            return false;
        }

        needTargets |= 0x2;
    }

    uint8_t flag = 0;
    uint32_t type = object->m_obj->m_type;
    bool check = false;

    if (type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(object);

        if (spell->m_attributesExE & 0x800) {
            unit = Spell_C_TargetImpliedTarget(unit, spell, static_cast<uint16_t>(needTargets), caster);

            if (!unit) {
                return false;
            }

            object = unit;
        }

        if ((unit->m_unit->m_unitFieldFlags & 0x10000) && !(spell->m_attributesExF & 0x1000000)) {
            return false;
        }

        if (spell->m_targetCreatureType && (!unit->GetCreatureType() || !((1 << (unit->GetCreatureType() - 1)) & spell->m_targetCreatureType))) {
            return false;
        }

        int32_t health = unit->m_unit->m_unitFieldHealth;

        if (health <= 0 && !(unit->m_unit->m_unitDynamicFlags & 0x20) && !(needTargets & 0x8600) && !(spell->m_attributesExB & 0x1)) {
            return false;
        }

        if (health > 0 && (needTargets & 0x400)) {
            return false;
        }

        if ((needTargets & 0x8) && caster->IsInMyPartyOrRaid(unit) && caster->CanAssist(unit, false)) {
            check = true;
        } else if ((needTargets & 0x4) && caster->IsInMyParty(unit) && caster->CanAssist(unit, false)) {
            check = true;
        } else if ((needTargets & 0x100) && caster->CanAssist(unit, (spell->m_attributesExF >> 3) & 1)) {
            check = true;
        } else if ((needTargets & 0x80) && caster->CanAttack(unit)) {
            check = true;
        } else if ((needTargets & 0x2) && !(unit->m_unit->m_unitFieldFlags & 0x100)) {
            check = true;
        } else if ((needTargets & 0x8000) && unit->m_unit->m_unitFieldHealth <= 0 && ((unit->m_obj->m_type & TYPE_PLAYER) || unit->m_unit->m_unitFieldPetnumber)) {
            if (!caster->CanAssist(unit, false)) {
                return false;
            }

            check = true;
        } else if ((needTargets & 0x200) && unit->m_unit->m_unitFieldHealth <= 0 && (unit->m_obj->m_type & TYPE_PLAYER) && !caster->CanAssist(unit, false)) {
            check = true;
        } else {
            return false;
        }
    } else if (type & TYPE_GAMEOBJECT) {
        if (!(needTargets & 0x4800)) {
            return false;
        }

        check = true;
    } else if (type & TYPE_CORPSE) {
        const CGCorpse_C* corpse = static_cast<const CGCorpse_C*>(object);

        if ((needTargets & 0x8000) && !(corpse->m_corpse->m_corpseFieldFlags & 0x1) && caster->CanAssist(corpse)) {
            flag = 1;
        }

        if ((needTargets & 0x200) && !(corpse->m_corpse->m_corpseFieldFlags & 0x1) && !caster->CanAssist(corpse)) {
            check = true;
        } else if (flag) {
            check = true;
        } else {
            return false;
        }
    } else {
        return false;
    }

    bool result = RangeCheck(caster, spell->m_id, object, &flag);
    *inRange = result;

    if (!result && report) {
        if (flag) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_OUT_OF_RANGE, -1, -1, false);
        } else {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_TOO_CLOSE, -1, -1, false);
        }
    }

    return true;
}

// VERIFY: 5b621a4e auto
// OFFSET: 0x00809A60
void Spell_C_StopTargeting() {
    if (s_targetingSpell) {
        Spell_C_CancelSpell(s_targetingSpell, true, true, 27);
    }
}

// VERIFY: 287a51ff manual
// OFFSET: 0x00809A80
void Spell_C_StopCasting() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_castSpellId) {
        Spell_C_CancelSpell(Spell_C_GetActivePlayerPendingCast(), true, true, 27);
    }
}

// VERIFY: 1ec35f12 auto
// OFFSET: 0x00809AC0
void Spell_C_CancelPlayerSpells() {
    WOWGUID player = ClntObjMgrGetActivePlayer();
    Spell_C_CancelSpellByCaster(player);
}

// VERIFY: b8318ed4 manual
// OFFSET: 0x0080DDD0
static int32_t Spell_C_Idle(const void* data, void* param) {
    for (PeriodicClientTrigger* trigger = s_periodicClientTriggers.Head(); trigger; trigger = s_periodicClientTriggers.Next(trigger)) {
        if (static_cast<int32_t>(OsGetAsyncTimeMs() - trigger->m_nextTime) < 0) {
            continue;
        }

        CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(trigger->m_caster, TYPE_UNIT, __FILE__, __LINE__));

        if (caster) {
            WOWGUID none = { 0 };
            Spell_C_CastSpell(caster, trigger->m_spellId, nullptr, none, nullptr, false);

            if (s_targetingSpell) {
                Spell_C_CancelSpell(s_targetingSpell, true, true, 27);
            }
        }

        trigger->m_nextTime = OsGetAsyncTimeMs() + trigger->m_period;
    }

    return 1;
}

// VERIFY: b47f2bb5 manual
// OFFSET: 0x0080DF10
void Spell_C_AddPeriodicClientTrigger(WOWGUID caster, int32_t auraSpellId, int32_t spellId, uint32_t period) {
    if (!s_periodicClientTriggers.Head()) {
        EventRegister(EVENT_ID_IDLE, Spell_C_Idle);
    }

    PeriodicClientTrigger* trigger = s_periodicClientTriggers.NewNode(2, 0, 8);
    trigger->m_auraSpellId = auraSpellId;
    trigger->m_spellId = spellId;
    trigger->m_period = period;
    trigger->m_nextTime = OsGetAsyncTimeMs() + period;
    trigger->m_caster = caster;
}

// VERIFY: 11d8e07f manual
// OFFSET: 0x0080DFE0
void Spell_C_RemovePeriodicClientTrigger(WOWGUID caster, int32_t auraSpellId) {
    PeriodicClientTrigger* trigger = s_periodicClientTriggers.Head();

    while (trigger) {
        if (trigger->m_caster == caster && (trigger->m_auraSpellId == auraSpellId || !auraSpellId)) {
            trigger = s_periodicClientTriggers.DeleteNode(trigger);
        } else {
            trigger = s_periodicClientTriggers.Next(trigger);
        }
    }

    if (!s_periodicClientTriggers.Head()) {
        EventUnregister(EVENT_ID_IDLE, Spell_C_Idle);
    }
}

// VERIFY: e91e9c97 manual
// OFFSET: 0x0080E100
void Spell_C_AddProcessedDestLocSpellCast(WOWGUID caster, uint8_t count, uint32_t time) {
    CHashKeyGUID key;
    key.m_guid = caster;
    DESTLOCSPELLCASTINDEXHASHNODE* node = s_destLocSpellCasts.Ptr(static_cast<uint32_t>(caster.m_guid), key);

    if (!node) {
        node = s_destLocSpellCasts.New(static_cast<uint32_t>(caster.m_guid), key, 0, 0);
    }

    node->m_castCount = count;
    node->m_time = time + 10000;
}

// VERIFY: 1250aa97 auto
// OFFSET: 0x00810050
int32_t Packet_SMSG_NOTIFY_DEST_LOC_SPELL_CAST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    NotifyDestLocSpellCastData data;
    data.m_destLocation = C3Vector();
    data.m_destPosition = C3Vector();
    msg->GetData(&data, sizeof(data));

    if (data.m_caster) {
        CHashKeyGUID key;
        key.m_guid = data.m_caster;
        DESTLOCSPELLCASTINDEXHASHNODE* node = s_destLocSpellCasts.Ptr(static_cast<uint32_t>(data.m_caster.m_guid), key);

        if (node && static_cast<int8_t>(data.unk_0038 - node->m_castCount) <= 0) {
            return 1;
        }

        Spell_C_AddProcessedDestLocSpellCast(data.m_caster, data.unk_0038, OsGetAsyncTimeMs());
    }

    Spell_C_PlayDestLocVisualsWithoutCaster(&data);
    return 1;
}

// VERIFY: 97984ce5 manual
// OFFSET: 0x007FFAC0
static bool CheckEnchantItemLevel(const PendingSpellCast* cast, const SpellRec* spell, const ItemStats_C* stats, const CGItem_C* item, int32_t* reason) {
    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        *reason = 27;
        return false;
    }

    int32_t maxLevel = 0;

    if (cast->m_data.m_castItem != caster->m_obj->m_guid && item && (item->m_obj->m_type & TYPE_ITEM)) {
        maxLevel = spell->m_maxLevel;
    }

    int32_t level = stats->m_requiredLevel ? stats->m_requiredLevel : stats->m_itemLevel;

    if (level < spell->m_baseLevel) {
        if (!(spell->m_attributesExB & 0x8)) {
            *reason = 48;
            return false;
        }
    } else if (maxLevel > 0 && static_cast<int32_t>(stats->m_itemLevel) > maxLevel) {
        *reason = 36;
        return false;
    }

    return true;
}

// VERIFY: f6aa96ed manual
// OFFSET: 0x007FFB60
static bool CheckItemRequirement(const PendingSpellCast* cast, const SpellRec* spell, WOWGUID itemGuid, int32_t* reason) {
    if (!itemGuid) {
        return true;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(itemGuid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        *reason = 178;
        return false;
    }

    WOWGUID none = { 0 };
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return true;
    }

    for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
        int32_t effect = spell->m_effect[i];

        if ((effect == SPELL_EFFECT_ENCHANT_ITEM || effect == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY || effect == SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC) && !CheckEnchantItemLevel(cast, spell, stats, item, reason)) {
            return false;
        }
    }

    return true;
}

// VERIFY: 126c913a manual
// OFFSET: 0x00803500
static bool ValidateSpellTarget(CGUnit_C* caster, const SpellRec* spell, uint32_t count, CGUnit_C** targets, int32_t* reason, bool casterIncluded) {
    if (!count) {
        return true;
    }

    int32_t targetAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_targetAuraSpell);
    int32_t excludeTargetAura = QuestTextParser::Spell_C_SpellGetSpellIDForDifficulty(spell->m_excludeTargetAuraSpell);

    if (spell->m_targetAuraState || spell->m_excludeTargetAuraState || targetAura || excludeTargetAura) {
        int32_t valid = 0;
        uint32_t i = count;

        do {
            CGUnit_C* target = targets[i - 1];
            i--;

            if (!target || !(target->m_obj->m_type & TYPE_UNIT)) {
                continue;
            }

            if (casterIncluded && !i) {
                break;
            }

            if (spell->m_targetAuraState && !(target->m_unit->m_unitFieldAurastate & (1 << (spell->m_targetAuraState - 1))) && !caster->IgnoresAuraState(spell)) {
                continue;
            }

            if (spell->m_excludeTargetAuraState && (target->m_unit->m_unitFieldAurastate & (1 << (spell->m_excludeTargetAuraState - 1))) && !caster->IgnoresAuraState(spell)) {
                continue;
            }

            if (targetAura && !target->HasAuraBySpellId(targetAura)) {
                continue;
            }

            if (excludeTargetAura && target->HasAuraBySpellId(excludeTargetAura)) {
                continue;
            }

            valid++;
        } while (i);

        if (!valid) {
            *reason = 111;
            return false;
        }
    }

    if (spell->m_attributesEx & 0x100) {
        int32_t valid = 0;
        uint32_t i = count;

        do {
            CGUnit_C* target = targets[i - 1];
            i--;

            if (target && (target->m_obj->m_type & TYPE_UNIT) && !(target->m_unit->m_unitFieldFlags & 0x80000)) {
                valid++;
            }
        } while (i);

        if (!valid) {
            *reason = 110;
            return false;
        }
    }

    bool dispel = false;
    bool steal = false;

    for (uint32_t j = 0; j < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); j++) {
        if (spell->m_effect[j] == SPELL_EFFECT_DISPEL || spell->m_effect[j] == SPELL_EFFECT_DISPEL_MECHANIC) {
            dispel = true;
        }

        if (spell->m_effect[j] == SPELL_EFFECT_STEAL_BENEFICIAL_BUFF) {
            steal = true;
        }
    }

    if (!dispel && !steal) {
        return true;
    }

    int32_t found = 0;
    bool immune = false;
    uint32_t i = count;

    do {
        CGUnit_C* target = targets[i - 1];
        i--;

        if (!target || !(target->m_obj->m_type & TYPE_UNIT)) {
            continue;
        }

        for (uint32_t j = 0; j < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); j++) {
            int32_t effect = spell->m_effect[j];

            if (effect != SPELL_EFFECT_DISPEL && effect != SPELL_EFFECT_DISPEL_MECHANIC && effect != SPELL_EFFECT_STEAL_BENEFICIAL_BUFF) {
                continue;
            }

            int32_t misc = spell->m_effectMiscValue[j];
            bool mechanic = effect == SPELL_EFFECT_DISPEL_MECHANIC;
            bool isSteal = effect == SPELL_EFFECT_STEAL_BENEFICIAL_BUFF;
            int32_t harmful;

            if (spell->m_attributesEx & 0x10000) {
                harmful = -1;
            } else if (caster->CanAttack(target)) {
                harmful = 1;
            } else if (caster->CanAssist(target, false)) {
                harmful = 0;
            } else {
                continue;
            }

            if (target->HasDispellableAura(misc, harmful, mechanic, isSteal)) {
                found++;
            } else if ((0x20000000 << j) & spell->m_attributesExD) {
                immune = true;
            }
        }
    } while (i);

    if (found) {
        return true;
    }

    if (!immune) {
        for (uint32_t j = 0; j < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); j++) {
            int32_t effect = spell->m_effect[j];

            if (effect && effect != SPELL_EFFECT_DISPEL && effect != SPELL_EFFECT_DISPEL_MECHANIC && effect != SPELL_EFFECT_STEAL_BENEFICIAL_BUFF) {
                return true;
            }
        }
    }

    *reason = dispel ? 86 : 87;
    return false;
}

// VERIFY: df42f3b1 manual
// OFFSET: 0x00809F80
int32_t Spell_C_PrepareMissileTrajectoryCast(CGUnit_C* caster, PendingSpellCast* cast, const SpellRec* spell, const TSStackArray<CGUnit_C*>* targets, float* pitch, float* speed) {
    if (caster->IsUpdatingMissileTrajectory()) {
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1, false);
        return 0;
    }

    int32_t instant = SpellRec::GetCastTime(spell, caster->m_unit->m_unitFieldPetnumber != 0, 0, 1) <= 0;
    SpellVisualRec visualRec;
    const SpellVisualRec* visual = caster->GetAppropriateSpellVisual(spell, visualRec, 0, 0);
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual ? visual->m_castKit : 0);
    int32_t playNow = 0;

    if (caster->m_castSpellId || cast->m_isTrajectoryCast || !instant || !visual || !visual->m_hasMissile) {
        instant = 0;
    } else {
        instant = 1;
        playNow = kit && kit->m_animID > 0 && !(visual->m_flags & 0x4);
        cast->m_data.m_castFlags |= 0x20000;
        cast->m_isTrajectoryCast = 1;
    }

    if (playNow) {
        if (s_delayedTrajectoryCast || (kit && kit->m_animID >= 0 && caster->m_specialIdleAnim == kit->m_animID)) {
            Spell_C_SpellFailed(cast, spell, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1, false);
            return 0;
        }

        cast->m_data.m_targetFlags |= 0x60;
        cast->m_data.m_sourcePos = C3Vector(0.0f, 0.0f, 0.0f);
        cast->m_data.m_destPos = C3Vector(0.0f, 0.0f, 0.0f);
        cast->m_data.m_elevation = 0.0f;
        cast->m_data.m_missileSpeed = 1.0f;

        if (visual->m_flags & 0x20) {
            caster->AttachPrecastMissileModel(visual, cast->m_data.m_castCount);
        }

        Spell_C_PlayMissileTrajectoryCastVisuals(caster, cast, spell, visual, kit, targets, 1.0f);
        s_delayedTrajectoryCast = cast;
        return 0;
    }

    C3Vector impact(0.0f, 0.0f, 0.0f);
    C3Vector launch(0.0f, 0.0f, 0.0f);
    float time;

    if (!caster->ComputeMissileTrajectory(spell, pitch, speed, &launch, &impact, &time, nullptr, nullptr, 0)) {
        ConsoleWrite("Could not compute missile trajectory", DEFAULT_COLOR);
        Spell_C_SpellFailed(cast, spell, SPELL_FAILED_ERROR, -1, -1, false);
        return 0;
    }

    float bytes;
    uint32_t latency;
    ClientServices::Connection()->GetNetStats(bytes, bytes, latency);
    float arrival = time + latency * 0.001f * 0.5f + 0.2f;
    float limit = time * 1.3f;

    if (limit < arrival) {
        arrival = limit;
    }

    cast->m_data.m_targetFlags |= 0x60;
    cast->m_data.m_sourcePos = launch;
    cast->m_data.m_destPos = impact;
    cast->m_data.m_elevation = *pitch;
    cast->m_data.m_missileSpeed = *speed;

    if (instant) {
        Spell_C_PlayMissileTrajectoryCastVisuals(caster, cast, spell, visual, kit, targets, arrival);
        caster->ReleaseMissiles();
        return 1;
    }

    if (cast->m_isTrajectoryCast) {
        caster->RecordMissileTrajectoryUpdate(cast->m_data.m_castCount, &impact, *pitch, *speed, arrival);
    }

    return 1;
}

// VERIFY: 1c2888dd auto
// OFFSET: 0x0080AC90
void Spell_C::SendPendingCast(PendingSpellCast* cast, bool hideVisuals) {
    WOWGUID castItem = cast->m_data.m_castItem == cast->m_data.m_caster ? WOWGUID{ 0 } : cast->m_data.m_castItem;

    if (s_spellWorldObject) {
        World::ObjectDestroy(s_spellWorldObject);
        s_spellWorldObject = nullptr;
    }

    if (s_spellWorldModel) {
        s_spellWorldModel->Release();
        s_spellWorldModel = nullptr;
    }

    CursorSetResetMode(POINT_CURSOR);
    CursorSetMode(POINT_CURSOR);
    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        if (cast == s_targetingSpell) {
            s_targetingSpell = nullptr;
            CGInputControl* input = CGInputControl::GetActive();
            input->m_mouseModeFlags &= ~0x10u;
            input->UpdateMouseMode(0);
        }

        ReleasePendingSpellCast(cast);
        return;
    }

    if (!castItem) {
        CGSpellBook::GetAppropriateSpellRank(&cast->m_data.m_spellId, caster, static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_objectTarget, TYPE_UNIT, __FILE__, __LINE__)));
    }

    SpellRec spell;
    g_spellDB.GetRecord(cast->m_data.m_spellId, &spell);
    TSStackArray<CGUnit_C*> targets(static_cast<CGUnit_C**>(alloca(sizeof(CGUnit_C*) * 2)), 2, 0);
    bool casterIncluded = false;

    for (uint32_t i = 0; i < sizeof(spell.m_implicitTargetA) / sizeof(spell.m_implicitTargetA[0]); i++) {
        if (spell.m_implicitTargetA[i] == 1 || spell.m_implicitTargetB[i] == 1) {
            targets.m_count = 1;
            targets.m_data[0] = caster;
            casterIncluded = true;
            break;
        }
    }

    if (cast->m_data.m_objectTarget) {
        targets.m_data[targets.m_count++] = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_objectTarget, TYPE_UNIT, __FILE__, __LINE__));
    }

    int32_t reason;

    if (!ValidateSpellTarget(caster, &spell, targets.m_count, targets.m_data, &reason, casterIncluded) || (cast->m_data.m_itemTarget && !(cast->m_data.m_targetFlags & 0x1000) && !CheckItemRequirement(cast, &spell, cast->m_data.m_itemTarget, &reason))) {
        Spell_C_SpellFailed(cast, &spell, reason, -1, -1, false);
        return;
    }

    if (!(spell.m_attributes & 0x8000000) && caster->GetClientStandState()) {
        if (!caster->CanAutoStand()) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_NOT_STANDING, -1, -1, false);
            return;
        }

        caster->ChangeStandState(UNIT_STANDING);
    }

    if (!(spell.m_attributes & 0x1000000) && static_cast<int32_t>(caster->m_unit->m_unitFieldMountdisplayid) > 0 && !caster->MountedCombatAllowed() && caster->m_mountDisplayId > 0 && !(caster->m_clientFlags & 0x10000000)) {
        if (!caster->CanAutoDismount()) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_NOT_MOUNTED, -1, -1, false);
            return;
        }

        caster->Dismount();
    }

    if (caster->HasVehicleTransport() && !(spell.m_attributesExF & 0x1000)) {
        caster->CheckVehicleExitOnCast();
    }

    if (!caster->CheckShapeshiftRules(&spell, &reason)) {
        if (reason == 94 || !caster->CanAutoUnshift()) {
            Spell_C_SpellFailed(cast, &spell, reason, -1, -1, false);
            return;
        }

        caster->CancelShapeshift();
    }

    for (uint32_t i = 0; i < sizeof(spell.m_effectAura) / sizeof(spell.m_effectAura[0]); i++) {
        if (spell.m_effectAura[i] == SPELL_AURA_MOUNTED) {
            if (!caster->CanCurrentFormMount() && caster->CanAutoUnshift()) {
                caster->CancelShapeshiftForMount();
            }

            break;
        }
    }

    if (caster->m_movement->m_flags & 0x2000000) {
        if ((static_cast<int32_t>(caster->m_unit->m_unitFieldMountdisplayid) > 0 && (caster->m_mountDisplayId <= 0 || (caster->m_clientFlags & 0x10000000))) || caster->m_shapeshiftSuppressed) {
            if ((spell.m_attributesEx & 0x44) && (spell.m_channelInterruptFlags & 0x18)) {
                Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_MOVING, -1, -1, false);
                return;
            }

            if ((spell.m_interruptFlags & 0x1) && !(spell.m_attributesExB & 0x20) && (SpellRec::GetCastTime(&spell, caster->m_unit->m_unitFieldPetnumber != 0, 0, 0) || (spell.m_auraInterruptFlags & 0x18))) {
                Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_MOVING, -1, -1, false);
                return;
            }
        }
    }

    CDataStore msg;
    uint8_t castFlags = 0;

    if (castItem) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(castItem, TYPE_ITEM, __FILE__, __LINE__));

        if (!item) {
            ConsoleWrite("Casting item not found", DEFAULT_COLOR);
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_ERROR, -1, -1, false);
            return;
        }

        CGObject_C* container = ClntObjMgrObjectPtr(item->m_item->m_itemFieldContained, TYPE_OBJECT, __FILE__, __LINE__);

        if (!container) {
            ConsoleWrite("Casting item's container not found", DEFAULT_COLOR);
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_ERROR, -1, -1, false);
            return;
        }

        int32_t slot = container->GetBag()->FindItemSlot(item->m_obj->m_guid);

        if (slot < 0) {
            ConsoleWrite("Casting item not found in container", DEFAULT_COLOR);
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_ERROR, -1, -1, false);
            return;
        }

        CGPlayer_C* owner = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(item->m_item->m_itemFieldOwner, TYPE_PLAYER, __FILE__, __LINE__));

        if (!owner) {
            ConsoleWrite("Active player not found", DEFAULT_COLOR);
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_ERROR, -1, -1, false);
            return;
        }

        uint8_t bag = owner->FindSlotIndex(item->m_item->m_itemFieldContained);

        if (item->GetInventoryType() && (bag != 0xFF || slot > 18)) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_EQUIPPED_ITEM, -1, -1, false);
            return;
        }

        msg.Put(static_cast<uint32_t>(CMSG_USE_ITEM));
        msg.Put(bag);
        msg.Put(static_cast<uint8_t>(slot));
        msg.Put(cast->m_data.m_castCount);
        msg.Put(static_cast<uint32_t>(cast->m_data.m_spellId));
        msg.Put(castItem.m_guid);
        msg.Put(cast->m_data.m_glyphSlot);
    } else if (caster->IsActivePlayer() && !(spell.m_attributesExF & 0x40000)) {
        msg.Put(static_cast<uint32_t>(CMSG_CAST_SPELL));
        msg.Put(cast->m_data.m_castCount);
        msg.Put(static_cast<uint32_t>(cast->m_data.m_spellId));
    } else {
        msg.Put(static_cast<uint32_t>(CMSG_PET_CAST_SPELL));
        msg.Put(cast->m_petGuid.m_guid);
        msg.Put(cast->m_data.m_castCount);
        msg.Put(static_cast<uint32_t>(cast->m_data.m_spellId));
    }

    SpellMissileRec* missile = g_spellMissileDB.GetRecord(spell.m_spellMissileID);
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (missile && (missile->m_flags & 0x1)) {
        if (mover != caster) {
            ConsoleWrite("Missile trajectory does not have active mover caster", DEFAULT_COLOR);
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_DONT_REPORT, -1, -1, false);
            return;
        }

        castFlags = 2;
        CGUnit_C::SetMissileTrajectoryTargetingSpell(spell.m_id);
    }

    msg.Put(castFlags);

    if (castFlags & 0x2) {
        float pitch;
        float speed;

        if (!Spell_C_PrepareMissileTrajectoryCast(caster, cast, &spell, &targets, &pitch, &speed)) {
            return;
        }

        Spell_C_EncodeSequenceIntoMissileTrajectory(reinterpret_cast<uint32_t*>(&pitch), reinterpret_cast<uint32_t*>(&speed), reinterpret_cast<uint32_t*>(&cast->m_data.m_sourcePos), reinterpret_cast<uint32_t*>(&cast->m_data.m_destPos));
        PendingSpellCastData::WriteTargetInfo(&cast->m_data, &msg);
        msg.Put(pitch);
        msg.Put(speed);
        WriteMovementPacket(mover, MSG_MOVE_STOP, &msg, 1);
        caster->PreStartUpdatingMissileTrajectory(cast->m_data.m_spellId, OsGetAsyncTimeMs());
    } else {
        PendingSpellCastData::WriteTargetInfo(&cast->m_data, &msg);
    }

    if (cast == s_delayedTrajectoryCast) {
        s_delayedTrajectoryCast = nullptr;
    }

    if (mover && mover == caster && (spell.m_attributesExG & 0x80)) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player && player->m_vehiclePassenger) {
            player->m_vehiclePassenger->StopMovement();
        }

        mover->OnMoveStopLocal(FrameTime::s_curTimeMs);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    s_targetingSpell = nullptr;
    CGInputControl* input = CGInputControl::GetActive();
    input->m_mouseModeFlags &= ~0x10u;
    input->UpdateMouseMode(0);

    if (caster->IsActivePlayer()) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UNIT_SPELLCAST_SENT, "%s%s%s%s", "player", spell.m_name, spell.m_nameSubtext, cast->m_data.m_objectTarget ? GetObjectNameFromGuid(cast->m_data.m_objectTarget) : "");
    }

    if (!hideVisuals) {
        StartSpellCastVisuals(caster, cast, &spell, 0, true);
    }

    Spell_C_StartSpellCooldown(&spell, caster);
}

// VERIFY: 7abd47d0 auto
// OFFSET: 0x007FD410
static void Spell_C_ClearMacroCooldownState(const SpellRec* spell) {
    if (spell->m_startRecoveryCategory && spell->m_startRecoveryTime) {
        CGUIMacros::m_state &= ~0x4u;
    }
}

// VERIFY: 2a122241 manual
// OFFSET: 0x007FE940
static int32_t Spell_C_UpdateSpellWorldObject(void* param, uint32_t flags, uint64_t param64, uintptr_t param32) {
    if (s_spellWorldModel) {
        C44Matrix matrix(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, s_cursorWorldObjectPosition.x, s_cursorWorldObjectPosition.y, s_cursorWorldObjectPosition.z, 1.0f);
        matrix.RotateAroundZ(s_cursorWorldObjectFacing);
        matrix.Scale(s_cursorWorldObjectScale);
        s_spellWorldModel->m_fFlags |= 0x8000;
        s_spellWorldModel->m_worldTransform = matrix;
    }

    return 1;
}

// VERIFY: d383a005 manual
// OFFSET: 0x007FE9E0
static void Spell_C_CreateSpellWorldObject(const GameObjectDisplayInfoRec* display) {
    s_spellWorldModel = World::s_m2Scene->CreateModel(display->m_modelName, 0);

    if (s_spellWorldModel) {
        s_spellWorldObject = CMap::ObjectCreate(s_spellWorldModel, Spell_C_UpdateSpellWorldObject, nullptr, 0, 0, 10);
    }
}

// VERIFY: f7a09138 auto
// OFFSET: 0x007FFE20
static void GameObjectStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!s_targetingSpell || s_targetingSpell->m_data.m_castCount != static_cast<uint8_t>(reinterpret_cast<uintptr_t>(param))) {
        return;
    }

    WOWGUID none = { 0 };
    GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(id, none, nullptr, nullptr, false);

    if (!stats) {
        return;
    }

    GameObjectDisplayInfoRec* display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);

    if (display) {
        s_cursorWorldObjectScale = stats->m_scale;
        Spell_C_CreateSpellWorldObject(display);
    }
}

// VERIFY: 95fdd19a manual
// OFFSET: 0x00805F60
static bool Spell_C_IsSpellPending(const WOWGUID& castItem, int32_t spellId, const PendingSpellCast* except) {
    for (PendingSpellCast* cast = s_pendingSpellCasts.Head(); cast; cast = s_pendingSpellCasts.Next(cast)) {
        if (cast != except && (!castItem || cast->m_data.m_castItem == castItem) && cast->m_data.m_spellId == static_cast<uint32_t>(spellId)) {
            return true;
        }
    }

    return false;
}

// VERIFY: 6fd9b453 manual
// OFFSET: 0x0080C790
static bool Spell_C_BeginCast(CGUnit_C* caster, const SpellRec* spell, WOWGUID target, PendingSpellCast* cast, bool confirmed) {
    uint32_t needTargets = spell->m_targets;
    s_needTargets = needTargets;
    bool noTarget = false;

    switch (spell->m_implicitTargetA[0]) {
        case 1:
            if (needTargets & 0x400) {
                s_needTargets = needTargets & ~0x400u;
            }

            break;

        case 5:
            if (needTargets & 0x8000) {
                s_needTargets = needTargets & ~0x8000u;
            }

            break;

        case 6:
        case 53:
            s_needTargets = needTargets | 0x80;
            break;

        case 16:
            noTarget = true;
            break;

        case 21:
        case 45:
            s_needTargets = needTargets | 0x100;
            break;

        case 23:
            s_needTargets = needTargets | 0x800;
            break;

        case 25:
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 74:
        case 75:
            s_needTargets = needTargets | 0x2;
            break;

        case 26:
            s_needTargets = needTargets | 0x4000;
            break;

        case 35:
            s_needTargets = needTargets | 0x8;
            break;

        case 57:
        case 61:
            s_needTargets = needTargets | 0x4;
            break;

        case 89: {
            SpellMissileRec* missile = g_spellMissileDB.GetRecord(spell->m_spellMissileID);

            if (missile && (missile->m_flags & 0x1)) {
                s_needTargets = needTargets & ~0x40u;
            }

            break;
        }

        case 90:
            s_needTargets = needTargets | 0x10000;
            break;
    }

    if ((s_needTargets & 0x2000) && s_targetString[0]) {
        SStrPrintf(cast->m_data.m_targetString, sizeof(cast->m_data.m_targetString), "%s", s_targetString);
        s_needTargets &= ~0x2000u;
        s_targetString[0] = 0;
        cast->m_data.m_targetFlags |= 0x2000;
    }

    if (!s_needTargets) {
        Spell_C::SendPendingCast(cast, false);
        return true;
    }

    if (spell->m_attributesExE & 0x800) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPE_UNIT, __FILE__, __LINE__));
        CGUnit_C* implied = unit ? Spell_C_TargetImpliedTarget(unit, spell, s_needTargets, caster) : nullptr;

        if (implied) {
            if (implied != unit) {
                target = implied->m_obj->m_guid;
            }
        } else if ((s_needTargets & 0x8) | 0x104) {
            s_needTargets |= 0x80;
        } else if (s_needTargets & 0x80) {
            s_needTargets |= 0x100;
        }
    }

    if (noTarget || (static_cast<uint32_t>(target.m_guid) & static_cast<uint32_t>(target.m_guid >> 32)) == 0xFFFFFFFF || target.m_guid == 0xFFFFFFFFFFFFFFFEull) {
        return false;
    }

    if ((caster->m_obj->m_type & TYPE_PLAYER) && (spell->m_attributes & 0x200)) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(caster);
        bool found = false;

        for (uint32_t i = 0; i < sizeof(spell->m_effect) / sizeof(spell->m_effect[0]); i++) {
            if (spell->m_effect[i] != SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY) {
                continue;
            }

            CGItem_C* items[2];
            items[0] = player->m_bag.GetItemPointer(15);
            items[1] = player->m_bag.GetItemPointer(16);

            if (spell->m_equippedItemSubclass) {
                for (uint32_t j = 0; j < 2; j++) {
                    CGItem_C* item = items[j];

                    if (item && (spell->m_equippedItemClass != item->GetClassID() || !((1 << item->GetSubtypeID()) & spell->m_equippedItemSubclass))) {
                        items[j] = nullptr;
                    }
                }
            }

            CGItem_C* mainHand = items[0];
            CGItem_C* offHand = items[1];
            uint32_t mainEnchant = mainHand ? mainHand->GetEnchantmentId(1) : 0;
            uint32_t offEnchant = offHand ? offHand->GetEnchantmentId(1) : 0;
            CGItem_C* chosen = offHand;

            if (mainHand) {
                if (offHand) {
                    if (!mainEnchant) {
                        chosen = mainHand;
                    } else if (offEnchant) {
                        uint32_t misc = spell->m_effectMiscValue[i];

                        if (mainEnchant == misc) {
                            if (offEnchant == misc) {
                                if (mainHand->GetEnchantmentDuration(1) <= offHand->GetEnchantmentDuration(1)) {
                                    chosen = mainHand;
                                }
                            } else {
                                chosen = mainHand;
                            }
                        } else if (offEnchant != misc) {
                            continue;
                        }
                    }
                } else {
                    chosen = mainHand;
                }
            } else if (!offHand) {
                continue;
            }

            if (!chosen) {
                continue;
            }

            if (chosen->IsBound() && !chosen->IsPermanentlyBoundForTrade() && !confirmed) {
                CGGameUI::SetLastItemSpellTarget(chosen->m_obj->m_guid);
                CGGameUI::SetLastEnchantSpellID(cast->m_data.m_spellId);
                ReleasePendingSpellCast(cast);
                FrameScript_SignalEvent(SCRIPT_EVENT_REPLACE_ENCHANT, "%s", "spellenchant");
                return true;
            }

            target = chosen->m_obj->m_guid;

            if (target) {
                found = true;
                break;
            }
        }

        if (!found) {
            target.m_guid = player->m_bag.m_numSlots > 15 ? player->m_bag.m_slots[15].m_guid : 0;
        }
    } else if (!target) {
        target = CGGameUI::m_lockedTarget;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(target, TYPE_OBJECT, __FILE__, __LINE__);

    if (target && !object) {
        float minRange;
        float maxRange;
        Spell_C::GetSpellRange(caster, spell->m_id, &minRange, &maxRange, nullptr);

        if (100.0f < maxRange) {
            cast->m_data.m_objectTarget = target;
            Spell_C::SendPendingCast(cast, false);
            return true;
        }
    }

    if (Spell_C_HandleSpriteClick(object, cast)) {
        return true;
    }

    if (!g_autoSelfCastCVar->m_intValue) {
        return false;
    }

    if (target && (!object || !(object->m_obj->m_type & TYPE_UNIT) || caster->CanAssist(static_cast<CGUnit_C*>(object), false))) {
        return false;
    }

    if (Spell_C_HandleSpriteClick(ClntObjMgrGetActivePlayerObj(), cast)) {
        return true;
    }

    if (caster->IsActivePlayer()) {
        return false;
    }

    return Spell_C_HandleSpriteClick(caster, cast);
}

// VERIFY: f2938ae3 manual
// OFFSET: 0x0080CCE0
bool Spell_C_CastSpell(CGUnit_C* caster, int32_t spellId, const CGItem_C* item, WOWGUID target, PendingSpellCast* cast, bool confirmed) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x40)) {
        return false;
    }

    CGUnit_C* unit = caster;
    WOWGUID petGuid = caster ? caster->m_obj->m_guid : WOWGUID{ 0 };

    if (!caster) {
        return false;
    }

    if (spell.m_attributesExF & 0x40000) {
        CGUnitData* data = caster->m_unit;
        const WOWGUID* owner = &data->m_unitFieldCharmedby;

        if (!*owner) {
            owner = &data->m_unitFieldCreatedby;
        }

        unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(*owner, TYPE_UNIT, __FILE__, __LINE__));
    }

    if (!unit || unit->IsAlteredFormTransitionPreventingMovement()) {
        return false;
    }

    if (!cast) {
        cast = CreatePendingSpellCast(true);
        cast->m_data.m_spellId = spellId;
        cast->m_data.m_castItem = item ? item->m_obj->m_guid : unit->m_obj->m_guid;
        cast->m_data.m_caster = unit->m_obj->m_guid;
    }

    cast->m_petGuid = petGuid;
    bool melee;

    if (spell.m_effect[0] == SPELL_EFFECT_ATTACK || (spell.m_attributes & 0x2) || (spell.m_attributesExB & 0x20)) {
        melee = true;
    } else {
        melee = false;

        if (!item && (unit->m_obj->m_type & TYPE_PLAYER)) {
            int16_t overrideId = static_cast<int16_t>(static_cast<CGPlayer_C*>(unit)->m_player->m_playerFieldBytes2);

            if (overrideId) {
                OverrideSpellDataRec* rec = g_overrideSpellDataDB.GetRecord(overrideId);

                if (rec && !(rec->m_flags & 0x4) && !Spell_C_IsOverride(spellId, rec)) {
                    Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW, -1, -1, false);
                    return false;
                }
            }
        }
    }

    if (!IsTutorialFlagged(5)) {
        s_meleeTutorialActive = 1;
        s_meleeTutorialCount = 0;
    }

    if ((unit->m_obj->m_type & TYPE_PLAYER) && s_meleeTutorialActive && IsTutorialFlagged(5)) {
        if (melee) {
            s_meleeTutorialCount++;
        } else {
            s_meleeTutorialActive = 0;
        }

        if (s_meleeTutorialCount > 10) {
            CGTutorial::TriggerTutorial(59);
        }
    }

    if (unit->HasVehicleTransport()) {
        WOWGUID transport = { unit->GetTransportGUID() };
        CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(transport, TYPE_UNIT, __FILE__, __LINE__));

        if (vehicle) {
            VehicleSeatRec* seat = vehicle->GetVehicleSeatRec(unit->m_movementData.m_transportSeat);

            if (seat) {
                uint32_t required = 0;

                if (SpellAppliesNonStanceShapeshift(&spell)) {
                    required = 0x2000;
                }

                if (SpellRec::HasAura(&spell, 78)) {
                    required |= 0x1000;
                }

                if (!required) {
                    required = 0x4000;
                }

                if (!(spell.m_attributesExF & 0x1000) && !(spell.m_attributes & 0x1000000) && (seat->m_flags & required) != required) {
                    Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW, -1, -1, false);
                    return false;
                }

                if ((unit->m_obj->m_type & TYPE_PLAYER) && (seat->m_flags & 0x40000800)) {
                    for (uint32_t i = 0; i < sizeof(spell.m_effect) / sizeof(spell.m_effect[0]); i++) {
                        for (int32_t j = 0; j < 4; j++) {
                            if (spell.m_effect[i] != j) {
                                continue;
                            }

                            SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell.m_effectMiscValueB[i]);

                            if (properties && properties->m_control) {
                                Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW, -1, -1, false);
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }

    int32_t effect = spell.m_effect[0];

    if (effect != SPELL_EFFECT_DUEL && effect != SPELL_EFFECT_SUMMON_RAF_FRIEND && effect != SPELL_EFFECT_TALENT_SPEC_SELECT && !CGSpellBook::IsTrackingSpell(spellId) && !SpellRec::WillSummonCritter(&spell)) {
        uint32_t action = spell.m_effect[0] == SPELL_EFFECT_TRADE_SKILL || (spell.m_attributes & 0x20) ? 6 : 2;

        if (!CGGameUI::CanPerformAction(action)) {
            ReleasePendingSpellCast(cast);
            return false;
        }
    }

    effect = spell.m_effect[0];

    if (spell.m_startRecoveryTime && !CGUIMacros::IsActionAllowed(2)) {
        ReleasePendingSpellCast(cast);
        return false;
    }

    if (effect == SPELL_EFFECT_TRADE_SKILL) {
        CGTradeSkillInfo::SetTradeSkill(spell.m_id, unit->m_obj->m_guid, 0, 0, 0);
        Spell_C_FailPendingCast(cast, &spell, 187);
        return false;
    }

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        uint32_t flags = static_cast<CGPlayer_C*>(unit)->m_player->m_playerFlags;

        if (flags & 0x1000000) {
            if (melee || Spell_C_IsRestrictedAbility(&spell)) {
                Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW, -1, -1, false);
                return false;
            }
        } else if (!melee && (flags & 0x800000)) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW, -1, -1, false);
            return false;
        }
    }

    if (unit->IsDeadOrGhost() && !(spell.m_attributes & 0x800000)) {
        Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_CASTER_DEAD, -1, -1, false);
        return false;
    }

    effect = spell.m_effect[0];

    if (effect == SPELL_EFFECT_ATTACK) {
        if (unit->OnAttackIconPressed(target, 0)) {
            Spell_C_FailPendingCast(cast, &spell, 187);
            return false;
        }

        Spell_C_ClearMacroCooldownState(&spell);
        ReleasePendingSpellCast(cast);
        return false;
    }

    if ((effect == SPELL_EFFECT_DISENCHANT || effect == SPELL_EFFECT_PROSPECTING || effect == SPELL_EFFECT_MILLING) && unit->GetComboPointTarget()) {
        Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_NOT_WHILE_LOOTING, -1, -1, false);
        return false;
    }

    if (((effect == SPELL_EFFECT_ENCHANT_ITEM || effect == SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY || effect == SPELL_EFFECT_ENCHANT_HELD_ITEM || effect == SPELL_EFFECT_ENCHANT_ITEM_PRISMATIC) && unit->GetComboPointTarget()) || unit->m_mountTransitionSpellId) {
        Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_SPELL_IN_PROGRESS, -1, -1, false);
        return false;
    }

    WOWGUID casterGuid = unit->m_obj->m_guid;

    if (Spell_C_IsSpellPending(casterGuid, spellId, cast)) {
        if (Spell_C_GetSpellCooldown(spellId, !unit->IsActivePlayer(), nullptr, nullptr, nullptr) || (spell.m_attributes & 0x404)) {
            if (s_enableErrorSpeechCVar && s_enableErrorSpeechCVar->m_intValue) {
                SE2SoundKitProperties properties;
                properties.ResetToDefaults();
                properties.m_suppressorMode = 2;
                SI2::PlaySoundKit("igPlayerInviteDecline", nullptr, nullptr, &properties);
            }

            ReleasePendingSpellCast(cast);
            return false;
        }
    }

    Spell_C_StopTargeting();

    if (item) {
        const_cast<CGItem_C*>(item)->HasUseEffect();
    }

    if ((spell.m_attributesExG & 0x8) && !(unit->m_unit->m_unitFieldFlags2 & 0x40000)) {
        Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_SPELL_UNAVAILABLE, -1, -1, false);
        return false;
    }

    if ((!item || !(const_cast<CGItem_C*>(item)->GetItemStaticFlags(0) & 0x10000000)) && !Spell_C_HaveSpellTokens(unit, &spell, true, cast)) {
        ReleasePendingSpellCast(cast);
        return false;
    }

    if (!Spell_C_HaveEquippedSpellItems(unit, &spell, true, true, cast)) {
        ReleasePendingSpellCast(cast);
        return false;
    }

    if (spell.m_attributes & 0x2) {
        bool usable;

        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            CGItem_C* weapon = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(WOWGUID{ static_cast<CGPlayer_C*>(unit)->m_bag.GetItemGuid(17) }, TYPE_ITEM, __FILE__, __LINE__));
            usable = weapon && weapon->IsUsable();
        } else {
            usable = unit->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false) != nullptr;
        }

        if (!usable) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_EQUIPPED_ITEM, spell.m_equippedItemClass, spell.m_equippedItemSubclass, false);
            return false;
        }
    }

    if (spell.m_attributesEx & 0x500000) {
        bool noComboPoints;

        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer() && !CGGameUI::GetComboPoints(target)) {
            noComboPoints = true;
        } else {
            noComboPoints = unit->m_obj->m_guid == CGPetInfo::GetPet(0) && !CGPetInfo::GetComboPoints(target);
        }

        if (noComboPoints && !unit->IgnoresAuraState(&spell)) {
            Spell_C_SpellFailed(cast, &spell, unit->m_unit->m_unitFieldBytes0[1] == 1 ? SPELL_FAILED_CASTER_AURASTATE : SPELL_FAILED_NO_COMBO_POINTS, -1, -1, false);
            return false;
        }
    }

    bool autoAttack = false;

    if (Spell_C_IsRestrictedAbility(&spell)) {
        bool face = !((spell.m_attributesExF >> 28) & 1);

        if (!unit->IsInMelee() && !unit->VerifyAttackIconPressed(&target, 0, 0, face, true)) {
            Spell_C_ClearMacroCooldownState(&spell);
            ReleasePendingSpellCast(cast);
            return false;
        }

        if (!(spell.m_attributesExB & 0x100000)) {
            autoAttack = true;
        }
    }

    uint8_t inRange;

    if (Spell_C_RangeCheckSelected(unit, &spell, target, &inRange, true, cast) && !inRange) {
        ReleasePendingSpellCast(cast);
        return false;
    }

    if (!unit->CheckAndReportSpellInhibitFlags(cast, &spell, item)) {
        ReleasePendingSpellCast(cast);
        return false;
    }

    if (item) {
        SndInterfacePlayItemSound(ITEMSOUND_USE, const_cast<CGItem_C*>(item));
    } else if (spell.m_attributes & 0x10) {
        SI2::PlaySoundKit("GAMEABILITYACTIVATE", nullptr, nullptr, nullptr);
    } else {
        SI2::PlaySoundKit("GAMESPELLACTIVATE", nullptr, nullptr, nullptr);
    }

    if (!Spell_C_BeginCast(unit, &spell, target, cast, confirmed)) {
        if (target.m_guid == 0xFFFFFFFFFFFFFFFEull) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_BAD_IMPLICIT_TARGETS, -1, -1, false);
            return false;
        }

        if ((spell.m_attributes & 0x200) && (!(unit->m_obj->m_type & TYPE_PLAYER) || !static_cast<CGPlayer_C*>(unit)->m_bag.GetItemGuid(15))) {
            Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_MAINHAND_EMPTY, -1, -1, false);
            return false;
        }

        if ((s_needTargets & 0x80) && (static_cast<uint32_t>(target.m_guid) & static_cast<uint32_t>(target.m_guid >> 32)) != 0xFFFFFFFF && (!(spell.m_attributesExE & 0x800) || !(s_needTargets & 0x10C))) {
            Spell_C_SpellFailed(cast, &spell, CGGameUI::m_lockedTarget ? SPELL_FAILED_BAD_TARGETS : SPELL_FAILED_BAD_IMPLICIT_TARGETS, -1, -1, false);
            return false;
        }

        s_targetingSpell = cast;
        CursorSetResetMode(CAST_CURSOR);
        CursorSetMode(CAST_ERROR_CURSOR);
        CGSpellBook::UpdateSelection();
        CGInputControl* input = CGInputControl::GetActive();
        input->m_mouseModeFlags |= 0x10;
        input->UpdateMouseMode(0);
        s_cursorWorldObjectFacing = 0.0f;
        s_cursorWorldObjectHousing = 0;
        s_cursorWorldObjectScale = 1.0f;

        for (uint32_t i = 0; i < sizeof(spell.m_effect) / sizeof(spell.m_effect[0]); i++) {
            int32_t objectEffect = spell.m_effect[i];

            if (objectEffect == SPELL_EFFECT_TRANS_DOOR || objectEffect == SPELL_EFFECT_SUMMON_OBJECT_SLOT1 || objectEffect == SPELL_EFFECT_SUMMON_OBJECT_SLOT2 || objectEffect == SPELL_EFFECT_SUMMON_OBJECT_SLOT3 || objectEffect == SPELL_EFFECT_SUMMON_OBJECT_SLOT4 || objectEffect == SPELL_EFFECT_SUMMON_OBJECT_WILD || objectEffect == SPELL_EFFECT_CREATE_HOUSE) {
                if (objectEffect == SPELL_EFFECT_CREATE_HOUSE) {
                    s_cursorWorldObjectHousing = 1;
                }

                WOWGUID guid = { static_cast<uint64_t>(static_cast<int64_t>(spell.m_id)) | 0x1FE0000000000000ull };
                GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(spell.m_effectMiscValue[i], guid, GameObjectStatsCallback, reinterpret_cast<void*>(static_cast<uintptr_t>(s_targetingSpell->m_data.m_castCount)), false);

                if (stats) {
                    GameObjectDisplayInfoRec* display = g_gameObjectDisplayInfoDB.GetRecord(stats->m_displayID);

                    if (display) {
                        s_cursorWorldObjectScale = stats->m_scale;
                        Spell_C_CreateSpellWorldObject(display);
                    }
                }

                break;
            }
        }

        CSimpleTop::s_instance->m_checkFocus = 1;
    }

    if (!s_targetingSpell) {
        if (autoAttack && !unit->IsInMelee() && unit->IsActivePlayer()) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (!player->IsInMelee()) {
                unit->OnAttackIconPressed(target, 0);
            }

            player->MeleeModeEnter(target);
        }

        if (!s_targetingSpell && spellId == GetModalNextSpell() && unit->IsActivePlayer() && !ClntObjMgrGetActivePlayerObj()->IsInMelee()) {
            unit->OnAttackIconPressed(target, 0);
        }
    }

    if (spell.m_attributesExB & 0x20) {
        if (unit->IsActivePlayer()) {
            Spell_C_SetAutoRepeatingSpell(spellId);
            return true;
        }
    } else if (s_autoRepeatingSpell) {
        SpellRec autoRepeat;

        if (g_spellDB.GetRecord(s_autoRepeatingSpell, &autoRepeat) && autoRepeat.m_equippedItemClass == 2 && (autoRepeat.m_equippedItemSubclass & 0x80000)) {
            Spell_C_CancelAutoRepeat(true);
        }
    }

    return true;
}

// VERIFY: 5c215492 manual
// OFFSET: 0x0080DA40
bool Spell_C::CastSpell(int32_t spellId, const CGItem_C* item, WOWGUID target, bool confirmed) {
    return Spell_C_CastSpell(ClntObjMgrGetActivePlayerObj(), spellId, item, target, nullptr, confirmed);
}

// VERIFY: e8a1babb manual
// OFFSET: 0x0080BC80
bool Spell_C_HandleSpriteClick(CGObject_C* object, PendingSpellCast* cast) {
    if (!cast) {
        cast = s_targetingSpell;
    }

    if (!object || !cast) {
        return false;
    }

    CGUnit_C* caster = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(cast->m_data.m_caster, TYPE_UNIT, __FILE__, __LINE__));

    if (!caster) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(cast->m_data.m_spellId, &spell)) {
        return false;
    }

    uint32_t needTargets = s_needTargets;

    if (spell.m_effect[0] != SPELL_EFFECT_DUEL) {
        uint32_t action = spell.m_effect[0] == SPELL_EFFECT_TRADE_SKILL || (spell.m_attributes & 0x20) ? 6 : 2;

        if (!CGGameUI::CanPerformAction(action)) {
            return false;
        }
    }

    bool result = false;
    uint32_t type = object->m_obj->m_type;

    if (type & TYPE_UNIT) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(object);

        if (spell.m_attributesExE & 0x800) {
            unit = Spell_C_TargetImpliedTarget(unit, &spell, s_needTargets, caster);

            if (!unit) {
                return false;
            }

            object = unit;

            if (s_needTargets & 0x10C) {
                s_needTargets &= ~0x80u;
            } else if (s_needTargets & 0x80) {
                s_needTargets &= ~0x100u;
            }
        }

        if (unit->m_obj->m_guid == cast->m_data.m_caster && (spell.m_attributesEx & 0x80000)) {
            return false;
        }

        if ((unit->m_unit->m_unitFieldFlags & 0x10000) && !(spell.m_attributesExF & 0x1000000)) {
            return false;
        }

        if (spell.m_targetCreatureType && (!unit->GetCreatureType() || !((1 << (unit->GetCreatureType() - 1)) & spell.m_targetCreatureType))) {
            return false;
        }

        int32_t health = unit->m_unit->m_unitFieldHealth;

        if (health <= 0 && !(unit->m_unit->m_unitDynamicFlags & 0x20) && !(s_needTargets & 0x8600) && !(spell.m_attributesExB & 0x1)) {
            return false;
        }

        if (health > 0 && (s_needTargets & 0x400)) {
            return false;
        }

        if ((s_needTargets & 0x8) && caster->IsInMyPartyOrRaid(unit) && caster->CanAssist(unit, false)) {
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x8u;
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;

            if (unit->m_unit->m_unitFieldHealth <= 0) {
                s_needTargets &= ~0x400u;
            }

            result = true;
        } else if ((s_needTargets & 0x4) && caster->IsInMyParty(unit) && caster->CanAssist(unit, false)) {
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x4u;
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;

            if (unit->m_unit->m_unitFieldHealth <= 0) {
                s_needTargets &= ~0x400u;
            }

            result = true;
        } else if ((s_needTargets & 0x100) && caster->CanAssist(unit, (spell.m_attributesExF >> 3) & 1)) {
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x100u;
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;

            if (unit->m_unit->m_unitFieldHealth <= 0) {
                s_needTargets &= ~0x400u;
            }

            result = true;
        } else if ((s_needTargets & 0x80) && caster->CanAttack(unit)) {
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x80u;
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;

            if (unit->m_unit->m_unitFieldHealth <= 0) {
                s_needTargets &= ~0x400u;
            }

            result = true;
        } else if ((s_needTargets & 0x2) && (!(unit->m_unit->m_unitFieldFlags & 0x100) || (spell.m_attributesExC & 0x10000000) || (spell.m_attributesExE & 0x200000))) {
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x2;
            uint32_t remaining = s_needTargets & ~0x2u;
            s_needTargets = remaining;

            if (unit->m_unit->m_unitFieldHealth <= 0) {
                s_needTargets = remaining & 0xFFFF79FF;
            }

            result = true;
        } else if ((s_needTargets & 0x8000) && unit->m_unit->m_unitFieldHealth <= 0 && ((unit->m_obj->m_type & TYPE_PLAYER) || unit->m_unit->m_unitFieldPetnumber)) {
            if (!caster->CanAssist(unit, false)) {
                return result;
            }

            cast->m_data.m_objectTarget = unit->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x8000u;
            result = true;
        } else if ((s_needTargets & 0x200) && unit->m_unit->m_unitFieldHealth <= 0 && (unit->m_obj->m_type & TYPE_PLAYER)) {
            if (caster->CanAssist(unit, false)) {
                return result;
            }

            cast->m_data.m_objectTarget = unit->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x2;
            s_needTargets &= ~0x200u;
            result = true;
        } else if (s_needTargets & 0x10000) {
            if (unit->GetCreatureType() != 12) {
                return result;
            }

            cast->m_data.m_objectTarget = unit->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x10000;
            s_needTargets &= ~0x10000u;
            result = true;
        } else if ((s_needTargets & 0x100) && (spell.m_attributesExF & 0x200) && unit->IsCharmed() && !caster->CanAttack(unit)) {
            cast->m_data.m_objectTarget = unit->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x100;
            s_needTargets &= ~0x100u;
            result = true;
        } else {
            return result;
        }
    } else if (type & TYPE_ITEM) {
        if (!(s_needTargets & 0x4010)) {
            return result;
        }

        cast->m_data.m_itemTarget = object->m_obj->m_guid;
        cast->m_data.m_targetFlags |= 0x10;
        s_needTargets &= 0xFFFFBFEF;
        result = true;
    } else if (type & TYPE_GAMEOBJECT) {
        if (!(s_needTargets & 0x4800)) {
            return result;
        }

        cast->m_data.m_objectTarget = object->m_obj->m_guid;
        cast->m_data.m_targetFlags |= 0x800;
        s_needTargets &= 0xFFFFB7FF;
        result = true;
    } else if (type & TYPE_CORPSE) {
        CGCorpse_C* corpse = static_cast<CGCorpse_C*>(object);

        if ((s_needTargets & 0x8000) && !(corpse->m_corpse->m_corpseFieldFlags & 0x1) && caster->CanAssist(corpse)) {
            cast->m_data.m_objectTarget = object->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x8000;
            s_needTargets &= ~0x8000u;
            result = true;
        }

        if ((s_needTargets & 0x200) && !(corpse->m_corpse->m_corpseFieldFlags & 0x1) && !caster->CanAssist(corpse)) {
            cast->m_data.m_objectTarget = object->m_obj->m_guid;
            cast->m_data.m_targetFlags |= 0x200;
            s_needTargets &= ~0x200u;
            result = true;
        } else if (!result) {
            return result;
        }
    } else {
        return result;
    }

    if ((cast->m_data.m_targetFlags & 0x18202) && cast->m_data.m_objectTarget == object->m_obj->m_guid) {
        uint8_t tooFar;

        if (!RangeCheck(caster, cast->m_data.m_spellId, object, &tooFar)) {
            Spell_C_SpellFailed(cast, &spell, tooFar ? SPELL_FAILED_OUT_OF_RANGE : SPELL_FAILED_TOO_CLOSE, -1, -1, false);
            cast->m_data.m_objectTarget = WOWGUID{ 0 };
            s_needTargets = needTargets;
            return false;
        }
    }

    if (!s_needTargets) {
        Spell_C::SendPendingCast(cast, false);
    }

    return result;
}

// VERIFY: 0d56c0f6 manual
// OFFSET: 0x0080C3E0
bool Spell_C_HandleGlyphClick(int32_t socket) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return false;
    }

    SpellRec spell;
    PendingSpellCast* cast = s_targetingSpell;

    if (!cast || !g_spellDB.GetRecord(cast->m_data.m_spellId, &spell)) {
        return false;
    }

    CGPlayerLocalData* data = static_cast<CGPlayerLocalData*>(player->m_player);

    if (!((1 << socket) & data->m_playerGlyphsEnabled)) {
        Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_GLYPH_SOCKET_LOCKED, -1, -1, false);
        return false;
    }

    for (uint32_t i = 0; i < sizeof(spell.m_effect) / sizeof(spell.m_effect[0]); i++) {
        if (spell.m_effect[i] != SPELL_EFFECT_APPLY_GLYPH) {
            continue;
        }

        GlyphPropertiesRec* properties = g_glyphPropertiesDB.GetRecord(spell.m_effectMiscValue[i]);
        GlyphSlotRec* slot = g_glyphSlotDB.GetRecord(data->m_playerFieldGlyphSlots1[socket]);

        if (!properties || !slot) {
            cast = s_targetingSpell;
            continue;
        }

        if (slot->m_type != properties->m_glyphSlotFlags) {
            Spell_C_SpellFailed(s_targetingSpell, &spell, SPELL_FAILED_INVALID_GLYPH, -1, -1, false);
            return false;
        }

        for (uint32_t j = 0; j < sizeof(data->m_playerFieldGlyphs1) / sizeof(data->m_playerFieldGlyphs1[0]); j++) {
            if (data->m_playerFieldGlyphs1[j] == static_cast<uint32_t>(spell.m_effectMiscValue[i])) {
                Spell_C_SpellFailed(s_targetingSpell, &spell, SPELL_FAILED_UNIQUE_GLYPH, -1, -1, false);
                return false;
            }
        }

        cast = s_targetingSpell;
        cast->m_data.m_glyphSlot = socket;
        bool result = false;

        if (s_needTargets & 0x20000) {
            cast->m_data.m_targetFlags |= 0x20000;
            s_needTargets &= ~0x20000u;
            result = true;

            if (!s_needTargets) {
                Spell_C::SendPendingCast(cast, false);
            }
        }

        return result;
    }

    Spell_C_SpellFailed(cast, &spell, SPELL_FAILED_INVALID_GLYPH, -1, -1, false);
    return false;
}

// VERIFY: d31bf1c5 auto
// OFFSET: 0x0080C5F0
int32_t Spell_C_TargetTradeItem() {
    if (!(s_needTargets & 0x4010) || !CGTradeInfo::m_targetItemID[6]) {
        return 0;
    }

    PendingSpellCast* cast = s_targetingSpell;
    cast->m_data.m_targetFlags |= 0x1000;
    s_needTargets &= 0xFFFFBFEF;
    cast->m_data.m_itemTarget.m_guid = 6;

    if (!s_needTargets) {
        Spell_C::SendPendingCast(cast, true);
        Spell_C_CancelSpell(cast, false, false, 27);
    }

    return 1;
}

// VERIFY: 25171a69 manual
// OFFSET: 0x0080DA80
void Spell_C_ClickSpell(int32_t spellId, const CGItem_C* item, WOWGUID target) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    if (Spell_C_IsActiveSpell(&spell, player)) {
        if (!g_secureAbilityToggleCVar->m_intValue || !s_spellHistory[0].RecentlyCast(spellId, 0)) {
            Spell_C_CancelAura(&spell, player, -1);
        }

        return;
    }

    if (spellId == GetModalNextSpell()) {
        WOWGUID none = { 0 };
        player->OnAttackIconPressed(none, 0);
        return;
    }

    if (spellId == s_autoRepeatingSpell) {
        Spell_C_CancelAutoRepeat(true);
        return;
    }

    if (spellId == Spell_C_GetTargetingSpell()) {
        Spell_C_StopTargeting();
        return;
    }

    Spell_C::CastSpell(spellId, item, target, false);
}

// VERIFY: 0ae47ac1 manual
// OFFSET: 0x0080DBD0
bool Spell_C_TargetUnitByGuid(const WOWGUID& guid) {
    return Spell_C_HandleSpriteClick(ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__), nullptr);
}

// VERIFY: 88279f66 manual
// OFFSET: 0x0080DCF0
bool Spell_C_HandleItemDrop(CGItem_C* item, CGUnit_C* unit) {
    if (!item || !unit || !unit->m_unit->m_unitFieldPetnumber || !s_feedPetSpell) {
        return false;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || unit->m_unit->m_unitFieldCreatedby != player->m_obj->m_guid) {
        return false;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(s_feedPetSpell, &spell)) {
        return false;
    }

    WOWGUID none = { 0 };

    if (!Spell_C::CastSpell(spell.m_id, nullptr, none, false)) {
        return false;
    }

    return Spell_C_HandleSpriteClick(item, nullptr);
}

// VERIFY: 6676674c manual
// OFFSET: 0x007FFFB0
void CreateMissilesForTargets(const TSStackArray<WOWGUID>& targets, CGUnit_C* unit, const SpellRec* spell, const SpellVisualRec* visual, uint32_t ammoDisplayId, uint32_t ammoInventoryType, PendingSpellCastData* data, const TSStackArray<uint32_t>* missReasons, const TSStackArray<uint32_t>* reflectResults, uint32_t delay) {
    if (!unit || (!visual->m_hasMissile && !ammoDisplayId)) {
        return;
    }

    uint32_t targetFlags = data->m_targetFlags;
    int32_t hit = missReasons == nullptr;
    bool unitTargets = !(targetFlags & 0x60) || (visual->m_flags & 0x1);

    if (targets.m_count && (targetFlags & 0x802) && unitTargets) {
        for (uint32_t i = 0; i < targets.m_count; i++) {
            if (ClntObjMgrObjectPtr(targets.m_data[i], TYPE_OBJECT, __FILE__, __LINE__)) {
                uint32_t reflectResult = reflectResults ? reflectResults->m_data[i] : 0;
                uint32_t missReason = missReasons ? missReasons->m_data[i] : 1;
                unit->CreateMissile(&targets.m_data[i], data, ammoDisplayId, ammoInventoryType, hit, missReason, reflectResult, visual, spell->m_id, &targets, nullptr, nullptr, delay);
            }
        }
    } else if ((targetFlags & 0x60) && (targets.m_count || !(visual->m_flags & 0x100))) {
        WOWGUID none = { 0 };
        unit->CreateMissile(&none, data, ammoDisplayId, ammoInventoryType, missReasons == nullptr, 1, 0, visual, spell->m_id, &targets, missReasons, reflectResults, delay);
    }
}

// VERIFY: 9a06bb93 auto
// OFFSET: 0x00800120
void CreateMissilesForTargetPoints(const TSStackArray<SpellCastTargetPoint>& points, CGUnit_C* unit, const SpellRec* spell, const SpellVisualRec* visual, uint32_t ammoDisplayId, uint32_t ammoInventoryType, PendingSpellCastData* data, uint32_t delay) {
    if (!unit || (!visual->m_hasMissile && !ammoDisplayId)) {
        return;
    }

    for (uint32_t i = 0; i < points.m_count; i++) {
        unit->CreateMissile(&points.m_data[i], data, ammoDisplayId, ammoInventoryType, visual, spell->m_id, delay);
    }
}

// VERIFY: 8b98e965 manual
// OFFSET: 0x00800180
void LaunchMissileVisual(const TSStackArray<WOWGUID>& hits, const TSStackArray<WOWGUID>& misses, CGUnit_C* unit, const SpellRec* spell, const SpellVisualRec* visual, uint32_t ammoDisplayId, uint32_t ammoInventoryType, PendingSpellCastData* data, const TSStackArray<uint32_t>& missReasons, const TSStackArray<uint32_t>& reflectResults, uint32_t delay) {
    if (!unit || (!visual->m_hasMissile && !ammoDisplayId)) {
        return;
    }

    uint32_t count = hits.m_count + misses.m_count;

    TSStackArray<WOWGUID> targets(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * count)), count, 0);

    TSStackArray<uint32_t> reasons(static_cast<uint32_t*>(alloca(sizeof(uint32_t) * count)), count, 0);

    TSStackArray<uint32_t> reflects(static_cast<uint32_t*>(alloca(sizeof(uint32_t) * count)), count, 0);

    uint32_t index = 0;

    if (!(data->m_castFlags & 0x400)) {
        for (; index < hits.m_count; index++) {
            targets.m_data[index] = hits.m_data[index];
            reasons.m_data[index] = 0;
            reflects.m_data[index] = 0;
        }

        for (uint32_t i = 0; i < misses.m_count; i++, index++) {
            targets.m_data[index] = misses.m_data[i];
            reasons.m_data[index] = missReasons.m_data[i];
            reflects.m_data[index] = reflectResults.m_data[i];
        }
    } else {
        for (uint32_t i = 0; i < misses.m_count; i++, index++) {
            targets.m_data[index] = misses.m_data[index];
            reasons.m_data[index] = missReasons.m_data[index];
            reflects.m_data[index] = reflectResults.m_data[index];
        }

        for (uint32_t i = 0; i < hits.m_count; i++, index++) {
            targets.m_data[index] = hits.m_data[i];
            reasons.m_data[index] = 0;
            reflects.m_data[index] = 0;
        }
    }

    WOWGUID none = { 0 };
    unit->CreateMissile(&none, data, ammoDisplayId, ammoInventoryType, 1, 1, 0, visual, spell->m_id, &targets, &reasons, &reflects, delay);
}

// VERIFY: 0b671995 manual
// OFFSET: 0x00800DD0
void Spell_C_PlayMissileTrajectoryCastVisuals(CGUnit_C* unit, PendingSpellCast* cast, const SpellRec* spell, const SpellVisualRec* visual, const SpellVisualKitRec* kit, const TSStackArray<CGUnit_C*>* targets, float time) {
    if (kit) {
        PlaySpellVisuaKitData data(spell, kit, 0);
        unit->PlaySpellVisualKit(data);
    }

    TSStackArray<WOWGUID> guids(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID) * targets->m_count)), targets->m_count, 0);
    guids.m_count = targets->m_count;

    for (uint32_t i = 0; i < targets->m_count; i++) {
        guids.m_data[i] = targets->m_data[i]->m_obj->m_guid;
    }

    TSStackArray<WOWGUID> misses(nullptr, 0, 0);
    TSStackArray<uint32_t> missReasons(nullptr, 0, 0);
    TSStackArray<uint32_t> reflectResults(nullptr, 0, 0);

    if (!visual) {
        return;
    }

    if (spell->m_attributesExD & 0x40000) {
        LaunchMissileVisual(guids, misses, unit, spell, visual, 0, 0, cast ? &cast->m_data : nullptr, missReasons, reflectResults, static_cast<uint32_t>(time * 1000.0f));
    } else {
        CreateMissilesForTargets(guids, unit, spell, visual, 0, 0, cast ? &cast->m_data : nullptr, nullptr, nullptr, static_cast<uint32_t>(time * 1000.0f));
    }
}

// VERIFY: 7426a6ef auto
// OFFSET: 0x00804420
void Spell_C_ClearPetCooldowns() {
    s_spellHistory[1].ClearAll();
}

// VERIFY: b323a3a8 manual
// OFFSET: 0x00801C90
int32_t Packet_MSG_CHANNEL_START(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID smartGuid;
    *msg >> smartGuid;
    WOWGUID guid = smartGuid;
    uint32_t spellId;
    msg->Get(spellId);
    int32_t duration;
    msg->Get(reinterpret_cast<uint32_t&>(duration));

    if (duration <= 0) {
        return 1;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && ((spell.m_attributesExD & 0x8000000) || (!(spell.m_attributes & 0x2) && (guid == ClntObjMgrGetActivePlayer() || (!(spell.m_attributes & 0x180) && !(spell.m_attributesExC & 0x2000) && !(spell.m_attributesExD & 0x1)))))) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->SetUIChannelSpell(spellId, duration);
            WOWGUID signalGuid = guid;
            Script_SendUnitSpellSignal(signalGuid, 0, &spell, SCRIPT_EVENT_UNIT_SPELLCAST_CHANNEL_START);
        }
    }

    return 1;
}

// VERIFY: e57ade70 manual
// OFFSET: 0x00801DB0
int32_t Packet_MSG_CHANNEL_UPDATE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    SmartGUID smartGuid;
    *msg >> smartGuid;
    WOWGUID guid = smartGuid;
    int32_t remaining;
    msg->Get(reinterpret_cast<uint32_t&>(remaining));
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 1;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(unit->m_channelSpellId, &spell) && ((spell.m_attributesExD & 0x8000000) || (!(spell.m_attributes & 0x2) && (guid == ClntObjMgrGetActivePlayer() || (!(spell.m_attributes & 0x180) && !(spell.m_attributesExC & 0x2000) && !(spell.m_attributesExD & 0x1)))))) {
        unit->OnChannelUpdate(remaining);

        if (remaining) {
            WOWGUID signalGuid = guid;
            Script_SendUnitSpellSignal(signalGuid, 0, &spell, SCRIPT_EVENT_UNIT_SPELLCAST_CHANNEL_UPDATE);
        } else {
            unit->ClearUIChannel();
            WOWGUID signalGuid = guid;
            Script_SendUnitSpellSignal(signalGuid, 0, &spell, SCRIPT_EVENT_UNIT_SPELLCAST_CHANNEL_STOP);
        }

        if (!remaining) {
            unit->RemoveChannelEffects(spell.m_id);
        }
    }

    return 1;
}

// VERIFY: 9daeef29 manual
// OFFSET: 0x00801F10
void PlayImpactVisual(const SpellRec* spell, const TSStackArray<WOWGUID>& targets, CGUnit_C* unit, const SpellVisualRec* visual) {
    if (!spell || !visual || !targets.m_count) {
        return;
    }

    SpellVisualKitRec* casterKit = g_spellVisualKitDB.GetRecord(visual->m_casterImpactKit);

    if (!casterKit) {
        casterKit = g_spellVisualKitDB.GetRecord(visual->m_impactKit);
    }

    SpellVisualKitRec* targetKit = g_spellVisualKitDB.GetRecord(visual->m_targetImpactKit);

    if (!targetKit) {
        targetKit = g_spellVisualKitDB.GetRecord(visual->m_impactKit);
    }

    if (!casterKit && !targetKit) {
        return;
    }

    for (uint32_t i = 0; i < targets.m_count; i++) {
        CGObject_C* object = ClntObjMgrObjectPtr(targets.m_data[i], TYPE_OBJECT, __FILE__, __LINE__);

        if (!object) {
            continue;
        }

        SpellVisualKitRec* kit = object == unit ? casterKit : targetKit;

        if (kit) {
            PlaySpellVisuaKitData data(spell, kit, 1);
            data.m_harmful = SpellRec::IsHelpfulOrHarmful(spell) == 2;
            object->PlaySpellVisualKit(data);
        }
    }
}

// VERIFY: 3a61b823 manual
// OFFSET: 0x00802040
void PlayImpactVisualOnTarget(const SpellRec* spell, const SpellVisualRec* visual, CGUnit_C* target, CGUnit_C* caster) {
    TSStackArray<WOWGUID> targets(static_cast<WOWGUID*>(alloca(sizeof(WOWGUID))), 1, 1);
    targets.m_data[0] = target->m_obj->m_guid;
    PlayImpactVisual(spell, targets, caster, visual);
}

// VERIFY: f73e5797 manual
// OFFSET: 0x008022D0
void Spell_C_PlayDestLocVisualsWithoutCaster(NotifyDestLocSpellCastData* data) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(data->m_spellId, &spell)) {
        return;
    }

    SpellVisualRec* visual = g_spellVisualDB.GetRecord(spell.m_spellVisualID[0]);

    if (!visual) {
        return;
    }

    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual->m_instantAreaKit);

    if (kit) {
        uint32_t flags = spell.m_spellMissileID > 0 ? 0x402010 : 0x2010;
        SpellVisualEffectNameRec* effectName;

        if (kit->m_baseEffect && (effectName = g_spellVisualEffectNameDB.GetRecord(kit->m_baseEffect)) != nullptr) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->InitAtPosition(data->m_spellId, kit, effectName, flags, &CGObject_C::EffectOnceHandler, &data->m_destPosition, 0, data->m_destTarget);
            effect->Release();
            flags = (flags & ~0x10u) | 0x400;
        }

        if (kit->m_worldEffect && (effectName = g_spellVisualEffectNameDB.GetRecord(kit->m_worldEffect)) != nullptr) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->InitAtPosition(data->m_spellId, kit, effectName, flags, &CGObject_C::EffectOnceHandler, &data->m_destPosition, 0, data->m_destTarget);
            effect->Release();
        }
    }

    CMissile* missiles = nullptr;

    if (visual->m_hasMissile && visual->m_missileModel && spell.m_speed > 0.0f) {
        float scale = 1.0f;
        CM2Model* model = nullptr;

        if (visual->m_missileModel < -5 || visual->m_missileModel >= 0) {
            SpellVisualEffectNameRec* effectName = g_spellVisualEffectNameDB.GetRecord(visual->m_missileModel);

            if (effectName && *effectName->m_fileName) {
                model = World::s_m2Scene->CreateModel(effectName->m_fileName, 0x40);
                scale = effectName->m_scale;
            }
        }

        if (!model) {
            model = World::s_m2Scene->CreateModel("Spells\\ErrorCube.mdx", 0);
        }

        CMissile* missile = CMissile::NewMissile();
        missile->m_model = model;
        missile->m_casterGuid = data->m_caster;
        missile->m_targetGuid = WOWGUID();
        missile->m_spellID = data->m_spellId;
        missile->m_castID = data->m_castCount;
        missile->m_speed = spell.m_speed;
        missile->m_baseSpeed = spell.m_speed;
        missile->m_hitResult = 0;
        missile->m_missReason = 0;
        missile->m_flags = 1;
        missile->m_scale = scale;
        missile->m_soundKitID = visual->m_missileSound;
        missile->m_impactPos = data->m_destPosition;
        missile->m_attachmentID = -1;
        float height = visual->m_missileFollowGroundHeight * 0.001f;

        if (height <= 0.25f) {
            height = 0.25f;
        }

        missile->m_groundOffset = height;
        float rate = visual->m_missileFollowGroundDropSpeed * 0.001f;

        if (rate <= 0.01f) {
            rate = 0.01f;
        }

        missile->m_followGroundRate = rate;
        missile->m_followGroundStart = fclamp(0.001f * visual->m_missileFollowGroundApproach, 0.0f, 1.0f);
        missile->m_motionFlags = visual->m_missileFollowGroundFlags;

        if (visual->m_missileFollowGroundFlags & 1) {
            missile->m_flags |= 0x7800;
        }

        int32_t missileCount = 1;

        if (visual->m_missileMotion) {
            missile->m_flags |= 0xB800;
            missile->InstallMotionScriptFunction(visual->m_missileMotion);
            SpellMissileMotionRec* motion = g_spellMissileMotionDB.GetRecord(visual->m_missileMotion);

            if (motion && motion->m_missileCount >= 1) {
                missileCount = motion->m_missileCount;
            }
        }

        missile->m_castTime = OsGetAsyncTimeMs();
        missile->SetReadyToFire(&missiles);

        if (data->m_missileSpeed > 0.0f) {
            missile->m_pitch = data->m_elevation;
            missile->m_trajectorySpeed = data->m_missileSpeed;
            missile->m_arrivalTime = data->m_delay + missile->m_castTime;
            missile->m_flags |= 0x10800;
        }

        missile->m_missileCount = missileCount;

        if (missileCount > 1) {
            missile->m_missileIndex = 0;

            for (int32_t i = 1; i < missileCount; i++) {
                missile = CMissile::CopyMissile(missile);
                missile->m_missileIndex = i;
                missile->SetReadyToFire(&missiles);
            }
        }
    }

    if (!missiles) {
        SpellVisualKitRec* impactKit = g_spellVisualKitDB.GetRecord(visual->m_impactAreaKit);

        if (!impactKit) {
            return;
        }

        uint32_t delay = OsGetAsyncTimeMs() + data->m_delay;
        uint32_t flags = spell.m_spellMissileID > 0 ? 0x402010 : 0x2010;
        SpellVisualEffectNameRec* effectName;

        if (impactKit->m_baseEffect && (effectName = g_spellVisualEffectNameDB.GetRecord(impactKit->m_baseEffect)) != nullptr) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->InitAtPosition(data->m_spellId, impactKit, effectName, flags, &CGObject_C::EffectOnceHandler, &data->m_destPosition, delay, data->m_destTarget);
            effect->Release();
            flags = (flags & ~0x10u) | 0x400;
        }

        if (impactKit->m_worldEffect && (effectName = g_spellVisualEffectNameDB.GetRecord(impactKit->m_worldEffect)) != nullptr) {
            CEffect* effect = new (SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0)) CEffect();
            effect->InitAtPosition(data->m_spellId, impactKit, effectName, flags, &CGObject_C::EffectOnceHandler, &data->m_destPosition, delay, data->m_destTarget);
            effect->Release();
        }
    }

    while (missiles) {
        if (missiles->m_model) {
            missiles->m_model->m_fadeAlpha = 1.0f;
        }

        missiles->FireMissiles(&data->m_destLocation);
    }
}

// VERIFY: 7d12c6fb auto
// OFFSET: 0x007FE500
PendingSpellCast* Spell_C_GetDelayedTrajectoryCast() {
    return s_delayedTrajectoryCast;
}

// VERIFY: 4bee51ef auto
// OFFSET: 0x008009B0
bool SpellIsCallCompanion(const SpellRec* spell) {
    if (spell->m_effect[0] == SPELL_EFFECT_SUMMON) {
        SummonPropertiesRec* properties = g_summonPropertiesDB.GetRecord(spell->m_effectMiscValueB[0]);

        if (properties && properties->m_slot == 5) {
            return true;
        }
    }

    return spell->m_effectAura[0] == SPELL_AURA_MOUNTED;
}

// VERIFY: 4f8ab45f manual
// OFFSET: 0x00804D20
void Spell_C_ExpireDestLocSpellCasts(uint32_t time) {
    for (DESTLOCSPELLCASTINDEXHASHNODE* node = s_destLocSpellCasts.m_fulllist.Head(); node; ) {
        DESTLOCSPELLCASTINDEXHASHNODE* next = s_destLocSpellCasts.m_fulllist.Next(node);

        if (static_cast<int32_t>(time - node->m_time) >= 0) {
            s_destLocSpellCasts.Delete(node);
        }

        node = next;
    }
}

// VERIFY: 2e242e97 manual
// OFFSET: 0x0080DE60
int32_t Spell_C_SendDelayedMissileTrajectoryCast(CGUnit_C* unit) {
    PendingSpellCast* cast = s_delayedTrajectoryCast;

    if (!cast) {
        return 0;
    }

    if (unit->m_obj->m_guid != CGUnit_C::s_activeMover) {
        s_delayedTrajectoryCast = nullptr;
        return 0;
    }

    CGItem_C* item;

    if (cast->m_data.m_castItem) {
        CGObject_C* object = ClntObjMgrObjectPtr(cast->m_data.m_castItem, TYPE_OBJECT, __FILE__, __LINE__);

        if (object && !(object->m_obj->m_type & TYPE_ITEM)) {
            object = nullptr;
        }

        item = static_cast<CGItem_C*>(object);
        cast = s_delayedTrajectoryCast;
    } else {
        item = nullptr;
    }

    int32_t result = Spell_C_CastSpell(unit, cast->m_data.m_spellId, item, cast->m_data.unk_0038, cast, false);
    s_delayedTrajectoryCast = nullptr;
    return result;
}
