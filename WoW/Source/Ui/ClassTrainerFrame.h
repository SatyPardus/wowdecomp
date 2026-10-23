#ifndef WOW_SOURCE_UI_CLASSTRAINERFRAME_H
#define WOW_SOURCE_UI_CLASSTRAINERFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

enum TRAINER_TYPE : int32_t {
    TRAINER_TYPE_GENERAL = 0,
    TRAINER_TYPE_TRADESKILLS = 2,

    // Unused 0.5.3 names; current meanings of 1/3 are unverified.
    // TRAINER_TYPE_TALENTS = 1,
    // TRAINER_TYPE_PET = 3,
    // NUM_TRAINER_TYPES = 4,
};

enum TRAINER_SERVICE : int32_t {
    TRAINER_SERVICE_AVAILABLE = 0,
    TRAINER_SERVICE_UNAVAILABLE = 1,
    TRAINER_SERVICE_USED = 2,
    NUM_TRAINER_SERVICE_TYPES = 6,

    // Unused 0.5.3 names; current meanings of slots 3..5 are unverified.
    // TRAINER_SERVICE_NOT_SHOWN = 3,
    // TRAINER_SERVICE_NEVER = 4,
    // TRAINER_SERVICE_NO_PET = 5,
};

struct lua_State;

struct TrainerServiceInfo {
    /* 0000 */ int32_t m_spellID;
    /* 0004 */ int32_t m_skillLine;
    /* 0008 */ int32_t m_moneyCost;
    /* 000C */ int32_t m_talentCost;
    /* 0010 */ int32_t m_professionCost;
    /* 0014 */ uint8_t m_levelReq;
    /* 0015 */ uint8_t unused_0015[3];
    /* 0018 */ int32_t m_skillReq;
    /* 001C */ int32_t m_skillRankReq;
    /* 0020 */ int32_t m_stepSpellID;
    /* 0024 */ int32_t m_abilityReq[3];
    /* 0030 */ uint8_t m_state;
    /* 0031 */ uint8_t unused_0031[3];
    /* 0034 */ int32_t m_visible;
};

struct TrainerSkillLineInfo {
    /* 0000 */ int32_t m_skillLine;
    /* 0004 */ uint32_t m_stateCounts[NUM_TRAINER_SERVICE_TYPES];
    /* 001C */ int32_t m_visible;
    /* 0020 */ int32_t m_collapsed;
    /* 0024 */ int32_t unk_0024;
};

class CGClassTrainer {
    public:
    static void ShutdownGame();
    static TrainerServiceInfo* GetService(uint32_t index);
    static WOWGUID m_trainer;
    static char m_greetingText[2048];
    static TRAINER_TYPE m_trainerType;
    static uint32_t m_filteredServices;
    static int32_t m_currentSelection;
    static uint32_t m_numServices;
    static int32_t m_numSkillLines;
    static uint32_t m_skillLineFilter;
    static uint32_t m_expandedSkillLines;
    static TSGrowableArray<TrainerServiceInfo*> m_services;
    static TSGrowableArray<TrainerSkillLineInfo*> m_skillLines;

    static void SetTrainer(const WOWGUID& trainer, TRAINER_TYPE type);
    static void EnterWorld();
    static void LeaveWorld();
    static void FilterAndSortServices();
    static void RefreshList();
    static void InitializeGame();
    static int32_t GetSelectionIndex();
    static const char* GetServiceType(uint32_t index);
    static int32_t GetSkillLineIndexFromService(uint32_t index);
    static int32_t IsCollapsedHeader(uint32_t index);
    static void BuyService();
    static const char* GetServiceName(uint32_t index);
    static const char* GetServiceSubtext(uint32_t index);
    static void SetServiceTypeFilter(int32_t filter);
    static void SetSkillLineFilter(uint32_t filter);
    static void AddServices(uint32_t count, const int32_t* spellIds, const int32_t* moneyCosts, const int32_t* const* pointCosts, const uint8_t* levelReqs, const int32_t* skillReqs, const int32_t* skillRankReqs, const int32_t* stepSpells, const int32_t* const* abilityReqs, const uint8_t* states, const char* greeting);
};

void TrainerItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success);

void ClassTrainerRegisterScriptFunctions();
void ClassTrainerUnregisterScriptFunctions();

#endif
