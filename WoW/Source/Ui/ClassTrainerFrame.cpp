#include "ClassTrainerFrame.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include <cstdlib>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/DB/Rec/SkillLineRec.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameScript/Types.h"

// STATIC: 0x00A13E08
static const char s_tradeSkillServiceHeaders[4][64] = { "", "TRADESKILL_SERVICE_STEP", "TRADESKILL_SERVICE_LEARN", "TRADESKILL_SERVICE_PASSIVE" };

// STATIC: 0x00C0E490
WOWGUID CGClassTrainer::m_trainer;

// STATIC: 0x00C0DC90
char CGClassTrainer::m_greetingText[2048];

// STATIC: 0x00C0E498
TRAINER_TYPE CGClassTrainer::m_trainerType;

// STATIC: 0x00C0E49C
int32_t CGClassTrainer::m_currentSelection;

// STATIC: 0x00C0E4A0
uint32_t CGClassTrainer::m_numServices;

// STATIC: 0x00C0E4A4
int32_t CGClassTrainer::m_numSkillLines;

// STATIC: 0x00C0E4A8
uint32_t CGClassTrainer::m_filteredServices;

// STATIC: 0x00C0E4AC
uint32_t CGClassTrainer::m_skillLineFilter;

// STATIC: 0x00C0E4B0
uint32_t CGClassTrainer::m_expandedSkillLines;

// STATIC: 0x00C0E4B4
TSGrowableArray<TrainerServiceInfo*> CGClassTrainer::m_services;

// STATIC: 0x00C0E4C4
TSGrowableArray<TrainerSkillLineInfo*> CGClassTrainer::m_skillLines;

// VERIFY: 03233b9e auto
// OFFSET: 0x005EEB70
void CGClassTrainer::InitializeGame() {
}

// VERIFY: a50c2177 auto
// OFFSET: 0x005940E0
void CGClassTrainer::SetTrainer(const WOWGUID& trainer, TRAINER_TYPE type) {
    if (trainer == CGClassTrainer::m_trainer) {
        return;
    }

    if (!trainer) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_CLOSED, nullptr);
        CGGameUI::ClearInteractTarget(CGClassTrainer::m_trainer, 0);
        CGClassTrainer::m_trainer = WOWGUID();
        CGClassTrainer::m_numSkillLines = 0;
        CGClassTrainer::m_numServices = 0;
        return;
    }

    if (trainer != ClntObjMgrGetActivePlayer()) {
        CGGameUI::SetInteractTarget(trainer);
    }

    CGClassTrainer::m_trainer = trainer;
    CGClassTrainer::m_trainerType = type;
}

// VERIFY: c25a1094 auto
// OFFSET: 0x00593CA0
void TrainerItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_DESCRIPTION_UPDATE, nullptr);
    }
}

// VERIFY: d723d8b2 auto
// OFFSET: 0x00593CC0
void CGClassTrainer::EnterWorld() {
    CGClassTrainer::m_trainer = WOWGUID{ 0 };
    CGClassTrainer::m_currentSelection = 0;
    CGClassTrainer::m_numServices = 0;
    CGClassTrainer::m_greetingText[0] = '\0';
}

// VERIFY: a1f9f4bf auto
// OFFSET: 0x00594170
int32_t CGClassTrainer::GetSelectionIndex() {
    if (!CGClassTrainer::m_currentSelection) {
        return -1;
    }

    uint32_t i;

    for (i = 0; i < CGClassTrainer::m_numServices; i++) {
        if (CGClassTrainer::m_services.m_data[i]->m_spellID == CGClassTrainer::m_currentSelection) {
            break;
        }
    }

    if (i == CGClassTrainer::m_numServices) {
        return -1;
    }

    return i;
}

// VERIFY: 0107b6eb manual
// OFFSET: 0x00594240
TrainerServiceInfo* CGClassTrainer::GetService(uint32_t index) {
    if (index < CGClassTrainer::m_numServices) {
        return CGClassTrainer::m_services.m_data[index];
    }

    return nullptr;
}

// VERIFY: a0d27311 manual
// OFFSET: 0x00594260
const char* CGClassTrainer::GetServiceType(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return nullptr;
    }

    TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

    if (service->m_spellID < 0) {
        return "header";
    }

    if (!service->m_state) {
        return "available";
    }

    if (service->m_state == TRAINER_SERVICE_USED) {
        return "used";
    }

    return "unavailable";
}

// VERIFY: 1710d0ee manual
// OFFSET: 0x005942B0
int32_t CGClassTrainer::GetSkillLineIndexFromService(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return -1;
    }

    TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

    if (service->m_spellID >= 0) {
        return -1;
    }

    for (int32_t i = 0; i < CGClassTrainer::m_numSkillLines; i++) {
        if (CGClassTrainer::m_skillLines.m_data[i]->m_skillLine == service->m_skillLine) {
            return i;
        }
    }

    return -1;
}

// VERIFY: b36e2c37 manual
// OFFSET: 0x00594300
int32_t CGClassTrainer::IsCollapsedHeader(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return 0;
    }

    TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

    if (service->m_spellID >= 0) {
        return 0;
    }

    for (int32_t i = 0; i < CGClassTrainer::m_numSkillLines; i++) {
        if (CGClassTrainer::m_skillLines.m_data[i]->m_skillLine == service->m_skillLine) {
            return !(CGClassTrainer::m_expandedSkillLines & (1 << i));
        }
    }

    return 0;
}

// VERIFY: 53d4c1b1 manual
// OFFSET: 0x005947D0
static int32_t QSortServices_General(const void* a, const void* b) {
    const TrainerServiceInfo* serviceA = *static_cast<TrainerServiceInfo* const*>(a);
    const TrainerServiceInfo* serviceB = *static_cast<TrainerServiceInfo* const*>(b);

    if (!serviceA->m_visible || !serviceB->m_visible) {
        if (!serviceA->m_visible && !serviceB->m_visible) {
            return 0;
        }

        return 2 * (serviceB->m_visible != 0) - 1;
    }

    if (CGClassTrainer::m_numSkillLines) {
        int32_t lineA = 0;
        int32_t lineB = 0;

        for (uint32_t i = 0; i < static_cast<uint32_t>(CGClassTrainer::m_numSkillLines); i++) {
            if (CGClassTrainer::m_skillLines.m_data[i]->m_skillLine == serviceA->m_skillLine) {
                lineA = i;
            }

            if (CGClassTrainer::m_skillLines.m_data[i]->m_skillLine == serviceB->m_skillLine) {
                lineB = i;
            }
        }

        if (lineA != lineB) {
            return 2 * (lineA > lineB) - 1;
        }
    }

    if (serviceA->m_spellID < 0 || serviceB->m_spellID < 0) {
        return 2 * (serviceA->m_spellID >= 0) - 1;
    }

    SpellRec spellA;
    SpellRec spellB;

    if (!g_spellDB.GetRecord(serviceA->m_spellID, &spellA) || !g_spellDB.GetRecord(serviceB->m_spellID, &spellB)) {
        return 0;
    }

    if (serviceB->m_levelReq != serviceA->m_levelReq) {
        return serviceB->m_levelReq < serviceA->m_levelReq ? 1 : -1;
    }

    if (static_cast<uint32_t>(serviceB->m_skillRankReq) != static_cast<uint32_t>(serviceA->m_skillRankReq)) {
        return static_cast<uint32_t>(serviceB->m_skillRankReq) < static_cast<uint32_t>(serviceA->m_skillRankReq) ? 1 : -1;
    }

    int32_t result = SStrSortUTF8I(spellA.m_name, spellB.m_name, STORM_MAX_STR);

    if (!result) {
        result = SStrSortUTF8I(spellA.m_nameSubtext, spellB.m_nameSubtext, STORM_MAX_STR);
    }

    return result;
}

// VERIFY: 5cc286cb manual
// OFFSET: 0x005949A0
static int32_t QSortServices_Tradeskill(const void* a, const void* b) {
    const TrainerServiceInfo* serviceA = *static_cast<TrainerServiceInfo* const*>(a);
    const TrainerServiceInfo* serviceB = *static_cast<TrainerServiceInfo* const*>(b);

    if (!serviceA->m_visible || !serviceB->m_visible) {
        if (!serviceA->m_visible && !serviceB->m_visible) {
            return 0;
        }

        return 2 * (serviceB->m_visible != 0) - 1;
    }

    if (serviceA->m_skillLine != serviceB->m_skillLine) {
        return 2 * (serviceA->m_skillLine > serviceB->m_skillLine) - 1;
    }

    if (serviceA->m_spellID < 0 || serviceB->m_spellID < 0) {
        return 2 * (serviceA->m_spellID >= 0) - 1;
    }

    SpellRec spellA;
    SpellRec spellB;

    if (!g_spellDB.GetRecord(serviceA->m_spellID, &spellA) || !g_spellDB.GetRecord(serviceB->m_spellID, &spellB)) {
        return 0;
    }

    if (static_cast<uint32_t>(serviceB->m_skillRankReq) != static_cast<uint32_t>(serviceA->m_skillRankReq)) {
        return static_cast<uint32_t>(serviceB->m_skillRankReq) < static_cast<uint32_t>(serviceA->m_skillRankReq) ? 1 : -1;
    }

    return SStrSortUTF8I(spellA.m_name, spellB.m_name, STORM_MAX_STR);
}

// VERIFY: f786a100 manual
// OFFSET: 0x00594BA0
void CGClassTrainer::FilterAndSortServices() {
    CGClassTrainer::m_filteredServices = CGClassTrainer::m_numServices;
    int32_t filter = g_serviceTypeFilterCVar->m_intValue;

    for (uint32_t i = 0; i < static_cast<uint32_t>(CGClassTrainer::m_numSkillLines); i++) {
        int32_t visible = 0;

        for (uint32_t state = 0; state < NUM_TRAINER_SERVICE_TYPES; state++) {
            if (((1 << state) & filter) && CGClassTrainer::m_skillLines.m_data[i]->m_stateCounts[state]) {
                visible = ((1 << i) & CGClassTrainer::m_skillLineFilter) != 0;
                break;
            }
        }

        CGClassTrainer::m_skillLines.m_data[i]->m_visible = visible;
        CGClassTrainer::m_skillLines.m_data[i]->m_collapsed = (CGClassTrainer::m_expandedSkillLines & (1 << i)) == 0;
    }

    for (uint32_t i = 0; i < CGClassTrainer::m_numServices; i++) {
        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[i];

        if (service->m_spellID >= 0 && !((1 << service->m_state) & filter)) {
            service->m_visible = 0;
            CGClassTrainer::m_filteredServices--;
            continue;
        }

        service->m_visible = 1;

        if (!CGClassTrainer::m_numSkillLines) {
            continue;
        }

        uint32_t index = 0;

        for (uint32_t j = 0; j < static_cast<uint32_t>(CGClassTrainer::m_numSkillLines); j++) {
            if (CGClassTrainer::m_skillLines.m_data[j]->m_skillLine == service->m_skillLine) {
                index = j;
                break;
            }
        }

        TrainerSkillLineInfo* line = CGClassTrainer::m_skillLines.m_data[index];

        if (!line->m_visible || (service->m_spellID >= 0 && line->m_collapsed)) {
            service->m_visible = 0;
            CGClassTrainer::m_filteredServices--;
        }
    }

    if (CGClassTrainer::m_trainerType == TRAINER_TYPE_TRADESKILLS) {
        qsort(CGClassTrainer::m_services.m_data, CGClassTrainer::m_numServices, sizeof(TrainerServiceInfo*), QSortServices_Tradeskill);
    } else {
        qsort(CGClassTrainer::m_services.m_data, CGClassTrainer::m_numServices, sizeof(TrainerServiceInfo*), QSortServices_General);
    }

    uint32_t i;

    for (i = 0; i < CGClassTrainer::m_numServices; i++) {
        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[i];

        if (service && service->m_spellID > 0 && !service->m_state && service->m_visible) {
            break;
        }
    }

    if (i < CGClassTrainer::m_numServices) {
        CGClassTrainer::m_currentSelection = CGClassTrainer::m_services.m_data[i]->m_spellID;

        if (CGClassTrainer::m_currentSelection < 0) {
            CGClassTrainer::m_currentSelection = 0;
        }
    }

    if (i == CGClassTrainer::m_numServices) {
        CGClassTrainer::m_currentSelection = CGClassTrainer::m_numServices > 1 ? CGClassTrainer::m_services.m_data[1]->m_spellID : 0;

        if (CGClassTrainer::m_currentSelection < 0) {
            CGClassTrainer::m_currentSelection = 0;
        }
    }
}

// VERIFY: c9bb5ec3 manual
// OFFSET: 0x00596B00
void CGClassTrainer::RefreshList() {
    if (!CGClassTrainer::m_trainer) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    for (uint32_t s = 0; s < CGClassTrainer::m_numServices; s++) {
        if (CGClassTrainer::m_services.m_data[s]->m_spellID == -1) {
            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(CGClassTrainer::m_services.m_data[s]->m_spellID, &spell)) {
            continue;
        }

        CGClassTrainer::m_services.m_data[s]->m_state = static_cast<uint8_t>(TRAINER_SERVICE_AVAILABLE);
        bool teaches = false;
        int32_t total = 0;
        int32_t known = 0;

        for (uint32_t e = 0; e < 3; e++) {
            int32_t effect = spell.m_effect[e];

            if (!effect) {
                continue;
            }

            if (effect == SPELL_EFFECT_SKILL_STEP) {
                int32_t minPoints;
                int32_t maxPoints;
                Spell_C_GetMinMaxPoints(&spell, e, &minPoints, &maxPoints, 0, 0, 0, 0);
                total++;

                for (int32_t i = 0; i < 128; i++) {
                    uint16_t skillId = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerSkillInfo[i].m_skillId : 0;

                    if (skillId == spell.m_effectMiscValue[e]) {
                        uint16_t step = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerSkillInfo[i].m_step : 0;

                        if (step >= maxPoints) {
                            known++;
                        }

                        break;
                    }
                }

                continue;
            }

            int32_t spellId;

            if (effect == SPELL_EFFECT_LEARN_SPELL && spell.m_implicitTargetA[e] != 5) {
                spellId = spell.m_effectTriggerSpell[e];
                teaches = true;
            } else {
                if (teaches) {
                    continue;
                }

                spellId = spell.m_id;
            }

            total++;

            if (player->IsSpellKnown(spellId) || player->IsSpellSuperceded(spellId)) {
                known++;
            }

            if (spellId == spell.m_id) {
                break;
            }
        }

        if (total > 0 && known == total) {
            CGClassTrainer::m_services.m_data[s]->m_state = static_cast<uint8_t>(TRAINER_SERVICE_USED);
        }

        if (CGClassTrainer::m_services.m_data[s]->m_state) {
            continue;
        }

        if (CGClassTrainer::m_services.m_data[s]->m_skillReq) {
            int32_t maxRank = 0;

            for (int32_t i = 0; i < 128; i++) {
                uint16_t skillId = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerSkillInfo[i].m_skillId : 0;

                if (skillId == CGClassTrainer::m_services.m_data[s]->m_skillReq) {
                    if (player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
                        maxRank = player->m_player->m_playerSkillInfo[i].m_maxRank;

                        if (maxRank > 0) {
                            maxRank += player->m_player->m_playerSkillInfo[i].m_permBonus;
                        }
                    } else {
                        maxRank = 0;
                    }

                    if (static_cast<int32_t>(player->GetSkillValue(i)) < CGClassTrainer::m_services.m_data[s]->m_skillRankReq) {
                        CGClassTrainer::m_services.m_data[s]->m_state = static_cast<uint8_t>(TRAINER_SERVICE_UNAVAILABLE);
                    }

                    break;
                }
            }

            SpellRec stepSpell;

            if (g_spellDB.GetRecord(CGClassTrainer::m_services.m_data[s]->m_stepSpellID, &stepSpell)) {
                for (uint32_t e = 0; e < 3; e++) {
                    if (stepSpell.m_effect[e] == SPELL_EFFECT_SKILL_STEP && stepSpell.m_effectMiscValue[e] == CGClassTrainer::m_services.m_data[s]->m_skillReq) {
                        int32_t minPoints;
                        int32_t maxPoints;
                        Spell_C_GetMinMaxPoints(&stepSpell, e, &minPoints, &maxPoints, 0, 0, 0, 0);

                        if (maxRank < 5 * maxPoints) {
                            CGClassTrainer::m_services.m_data[s]->m_state = static_cast<uint8_t>(TRAINER_SERVICE_UNAVAILABLE);
                        }

                        break;
                    }
                }
            }
        }

        if (CGClassTrainer::m_services.m_data[s]->m_state) {
            continue;
        }

        bool abilitiesKnown = true;

        for (uint32_t i = 0; i < 3; i++) {
            int32_t ability = CGClassTrainer::m_services.m_data[s]->m_abilityReq[i];

            if (ability && !player->IsSpellKnown(ability) && !player->IsSpellSuperceded(ability)) {
                abilitiesKnown = false;
                break;
            }
        }

        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[s];

        if (CGSpellBook::IsUnlearnedSpell(service->m_spellID)) {
            service->m_state = static_cast<uint8_t>(TRAINER_SERVICE_USED);
        } else if (!abilitiesKnown) {
            service->m_state = static_cast<uint8_t>(TRAINER_SERVICE_UNAVAILABLE);
        } else if (service->m_levelReq > 1 && static_cast<int32_t>(player->m_unit->m_unitFieldLevel) < service->m_levelReq) {
            service->m_state = static_cast<uint8_t>(TRAINER_SERVICE_UNAVAILABLE);
        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(CGClassTrainer::m_numSkillLines); i++) {
        TrainerSkillLineInfo* line = CGClassTrainer::m_skillLines.m_data[i];

        for (uint32_t state = 0; state < NUM_TRAINER_SERVICE_TYPES; state++) {
            line->m_stateCounts[state] = 0;
        }

        line->unk_0024 = 1;
    }

    for (uint32_t s = 0; s < CGClassTrainer::m_numServices; s++) {
        TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[s];

        if (service->m_spellID == -1) {
            continue;
        }

        for (uint32_t i = 0; i < static_cast<uint32_t>(CGClassTrainer::m_numSkillLines); i++) {
            if (CGClassTrainer::m_skillLines.m_data[i]->m_skillLine == service->m_skillLine) {
                CGClassTrainer::m_skillLines.m_data[i]->m_stateCounts[service->m_state]++;
                break;
            }
        }
    }

    CGClassTrainer::FilterAndSortServices();
    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
}

// VERIFY: 19f1da85 manual
// OFFSET: 0x00594DA0
void CGPlayer_C::TrainerBuySpell(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return;
    }

    TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

    if (!service || service->m_state) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TRAINER_BUY_SPELL));
    msg.Put(CGClassTrainer::m_trainer.m_guid);
    msg.Put(static_cast<uint32_t>(service->m_spellID));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c1d5fe20 auto
// OFFSET: 0x00594E50
void CGClassTrainer::BuyService() {
    for (uint32_t i = 0; i < CGClassTrainer::m_filteredServices; i++) {
        CGPlayer_C::TrainerBuySpell(i);
    }
}

// VERIFY: f21793b5 manual
// OFFSET: 0x00594E80
const char* CGClassTrainer::GetServiceName(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return nullptr;
    }

    TrainerServiceInfo* service = CGClassTrainer::m_services.m_data[index];

    if (service->m_spellID < 0) {
        if (CGClassTrainer::m_trainerType == TRAINER_TYPE_TRADESKILLS) {
            return FrameScript_GetText(s_tradeSkillServiceHeaders[service->m_skillLine], -1, GENDER_NOT_APPLICABLE);
        }

        SkillLineRec* skillLine = g_skillLineDB.GetRecord(service->m_skillLine);
        return skillLine ? skillLine->m_displayName : nullptr;
    }

    SpellRec spell;
    const char* name = g_spellDB.GetRecord(CGClassTrainer::m_services.m_data[index]->m_spellID, &spell) ? spell.m_name : nullptr;
    return name;
}

// VERIFY: bba747ef auto
// OFFSET: 0x00594F60
const char* CGClassTrainer::GetServiceSubtext(uint32_t index) {
    if (index >= CGClassTrainer::m_numServices) {
        return nullptr;
    }

    if (CGClassTrainer::m_services.m_data[index]->m_spellID < 0) {
        return "";
    }

    SpellRec spell;
    const char* subtext = g_spellDB.GetRecord(CGClassTrainer::m_services.m_data[index]->m_spellID, &spell) ? spell.m_nameSubtext : nullptr;
    return subtext;
}

// VERIFY: 2bfe5774 auto
// OFFSET: 0x00594FE0
void CGClassTrainer::SetServiceTypeFilter(int32_t filter) {
    g_serviceTypeFilterCVar->Set(filter, true, false, false, true);
    CGClassTrainer::FilterAndSortServices();
    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
}

// VERIFY: 46387f38 auto
// OFFSET: 0x00595010
void CGClassTrainer::SetSkillLineFilter(uint32_t filter) {
    CGClassTrainer::m_skillLineFilter = filter;
    CGClassTrainer::FilterAndSortServices();
    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_UPDATE, nullptr);
}

// VERIFY: a07b4562 manual
// OFFSET: 0x005963C0
void CGClassTrainer::ShutdownGame() {
    for (uint32_t i = 0; i < CGClassTrainer::m_services.m_count; i++) {
        delete CGClassTrainer::m_services.m_data[i];
    }

    CGClassTrainer::m_services.m_count = 0;

    for (uint32_t i = 0; i < CGClassTrainer::m_skillLines.m_count; i++) {
        delete CGClassTrainer::m_skillLines.m_data[i];
    }

    CGClassTrainer::m_skillLines.m_count = 0;
}

// VERIFY: 2d160ac6 manual
// OFFSET: 0x00593CE0
static int32_t QSortTradeSkillTypes(const void* a, const void* b) {
    int32_t lineA = (*static_cast<TrainerSkillLineInfo* const*>(a))->m_skillLine;
    int32_t lineB = (*static_cast<TrainerSkillLineInfo* const*>(b))->m_skillLine;

    if (lineA == lineB) {
        return 0;
    }

    return lineA > lineB ? 1 : -1;
}

// VERIFY: cc874a7b auto
// OFFSET: 0x005941B0
static int32_t QSortSkillLines(const void* a, const void* b) {
    const TrainerSkillLineInfo* infoA = *static_cast<TrainerSkillLineInfo* const*>(a);
    const TrainerSkillLineInfo* infoB = *static_cast<TrainerSkillLineInfo* const*>(b);

    if (infoA->m_skillLine == infoB->m_skillLine) {
        return 0;
    }

    if (infoA->unk_0024 != infoB->unk_0024) {
        return infoA->unk_0024 ? 1 : -1;
    }

    SkillLineRec* lineA = g_skillLineDB.GetRecord(infoA->m_skillLine);
    SkillLineRec* lineB = g_skillLineDB.GetRecord(infoB->m_skillLine);

    if (lineA && lineB) {
        return SStrSortUTF8I(lineA->m_displayName, lineB->m_displayName, STORM_MAX_STR);
    }

    return 0;
}

// VERIFY: 2b6f7341 manual
// OFFSET: 0x00594AE0
static int32_t GetSkillLineFromService(int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return 0;
    }

    int32_t effect = -1;

    for (int32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] == SPELL_EFFECT_LEARN_SPELL && spell.m_implicitTargetA[i] != 5) {
            effect = i;
            break;
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    return player->GetAbilitySkillLine(effect < 0 ? spell.m_id : spell.m_effectTriggerSpell[effect]);
}

// VERIFY: 24a792d1 manual
// OFFSET: 0x00596450
void CGClassTrainer::AddServices(uint32_t count, const int32_t* spellIds, const int32_t* moneyCosts, const int32_t* const* pointCosts, const uint8_t* levelReqs, const int32_t* skillReqs, const int32_t* skillRankReqs, const int32_t* stepSpells, const int32_t* const* abilityReqs, const uint8_t* states, const char* greeting) {
    for (uint32_t i = CGClassTrainer::m_services.Count(); i < count; i++) {
        TrainerServiceInfo* info = static_cast<TrainerServiceInfo*>(SMemAlloc(sizeof(TrainerServiceInfo), __FILE__, __LINE__, 0));
        *CGClassTrainer::m_services.New() = info;
    }

    CGClassTrainer::m_skillLineFilter = 0xFFFFFFFF;
    CGClassTrainer::m_expandedSkillLines = 0xFFFFFFFF;
    CGClassTrainer::m_numSkillLines = 0;
    uint32_t skipped = 0;

    for (uint32_t i = 0; i + skipped < count; i++) {
        uint32_t source = i + skipped;
        TrainerServiceInfo* info = CGClassTrainer::m_services.m_data[i];
        info->m_spellID = spellIds[source];
        info->m_moneyCost = moneyCosts[source];
        info->m_talentCost = pointCosts[0][source];
        info->m_professionCost = pointCosts[1][source];
        info->m_levelReq = levelReqs[source];
        info->m_skillReq = skillReqs[source];
        info->m_skillRankReq = skillRankReqs[source];
        info->m_stepSpellID = stepSpells[source];
        info->m_abilityReq[0] = abilityReqs[0][source];
        info->m_abilityReq[1] = abilityReqs[1][source];
        info->m_abilityReq[2] = abilityReqs[2][source];
        info->m_state = states[source];
        int32_t skillLine;

        if (CGClassTrainer::m_trainerType != TRAINER_TYPE_TRADESKILLS) {
            skillLine = GetSkillLineFromService(info->m_spellID);
        } else {
            skillLine = 2;
            SpellRec spell;

            if (g_spellDB.GetRecord(info->m_spellID, &spell)) {
                if (spell.m_attributes & 0x40) {
                    skillLine = 3;
                } else {
                    for (int32_t j = 0; j < 3; j++) {
                        if (spell.m_effect[j] == SPELL_EFFECT_SKILL_STEP) {
                            skillLine = 1;
                            break;
                        }
                    }
                }
            }
        }

        info->m_skillLine = skillLine;

        if (!skillLine) {
            i--;
            skipped++;
            continue;
        }

        int32_t line;

        for (line = 0; line < CGClassTrainer::m_numSkillLines; line++) {
            TrainerSkillLineInfo* lineInfo = CGClassTrainer::m_skillLines.m_data[line];

            if (lineInfo->m_skillLine != skillLine) {
                continue;
            }

            lineInfo->m_stateCounts[info->m_state]++;

            if (lineInfo->unk_0024) {
                lineInfo->unk_0024 = info->m_talentCost || info->m_professionCost;
            }

            break;
        }

        if (line != CGClassTrainer::m_numSkillLines) {
            continue;
        }

        if (static_cast<uint32_t>(CGClassTrainer::m_numSkillLines) >= CGClassTrainer::m_skillLines.Count()) {
            TrainerSkillLineInfo* lineInfo = static_cast<TrainerSkillLineInfo*>(SMemAlloc(sizeof(TrainerSkillLineInfo), __FILE__, __LINE__, 0));
            *CGClassTrainer::m_skillLines.New() = lineInfo;
        }

        TrainerSkillLineInfo* lineInfo = CGClassTrainer::m_skillLines.m_data[CGClassTrainer::m_numSkillLines];
        lineInfo->m_skillLine = skillLine;

        for (uint32_t k = 0; k < NUM_TRAINER_SERVICE_TYPES; k++) {
            lineInfo->m_stateCounts[k] = 0;
        }

        lineInfo->unk_0024 = 1;
        lineInfo->m_stateCounts[info->m_state]++;
        lineInfo->unk_0024 = info->m_talentCost || info->m_professionCost;
        CGClassTrainer::m_numSkillLines++;
    }

    CGClassTrainer::m_numServices = count - skipped;
    qsort(CGClassTrainer::m_skillLines.m_data, CGClassTrainer::m_numSkillLines, sizeof(TrainerSkillLineInfo*), CGClassTrainer::m_trainerType != TRAINER_TYPE_TRADESKILLS ? QSortSkillLines : QSortTradeSkillTypes);

    for (uint32_t i = CGClassTrainer::m_services.Count(); i < CGClassTrainer::m_numSkillLines + CGClassTrainer::m_numServices; i++) {
        TrainerServiceInfo* info = static_cast<TrainerServiceInfo*>(SMemAlloc(sizeof(TrainerServiceInfo), __FILE__, __LINE__, 0));
        *CGClassTrainer::m_services.New() = info;
    }

    for (int32_t i = 0; i < CGClassTrainer::m_numSkillLines; i++) {
        TrainerServiceInfo* header = CGClassTrainer::m_services.m_data[CGClassTrainer::m_numServices + i];
        header->m_spellID = -1;
        header->m_skillLine = CGClassTrainer::m_skillLines.m_data[i]->m_skillLine;
    }

    CGClassTrainer::m_numServices += CGClassTrainer::m_numSkillLines;
    CGClassTrainer::FilterAndSortServices();

    if (greeting) {
        QuestParserParseText(greeting, CGClassTrainer::m_greetingText, sizeof(CGClassTrainer::m_greetingText), ClntObjMgrGetActivePlayer(), nullptr, 0);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_TRAINER_SHOW, nullptr);
}

// VERIFY: 0b0f7a6c auto
// OFFSET: 0x005947B0
void CGClassTrainer::LeaveWorld() {
    WOWGUID trainer = { 0 };
    CGClassTrainer::SetTrainer(trainer, TRAINER_TYPE_GENERAL);
}
