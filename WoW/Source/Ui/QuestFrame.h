#ifndef WOW_SOURCE_UI_QUESTFRAME_H
#define WOW_SOURCE_UI_QUESTFRAME_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"

enum QUEST_STATE : int32_t {
    QUEST_GREETING = 0,
    QUEST_OFFER = 1,
    QUEST_ACCEPTED = 2,
    QUEST_REWARD = 3,
    // QUEST_STATE_NUM_TYPES = 4, // Unused reference member.
};

struct QuestItemInfo {
    /* 0000 */ int32_t m_itemID;
    /* 0004 */ int32_t m_displayID;
    /* 0008 */ int32_t m_count;
};

struct QuestItemSlot {
    /* 0000 */ QuestItemInfo m_reward;
    /* 000C */ QuestItemInfo m_choice;
    /* 0018 */ QuestItemInfo m_required;
};

struct QuestGiverQuest {
    /* 0000 */ int32_t m_questID;
    /* 0004 */ int32_t m_level;
    /* 0008 */ int32_t m_flags;
    /* 000C */ char m_title[512];
    /* 020C */ int32_t m_turnIn;
    /* 0210 */ int32_t m_repeatable;
};

class CGQuestInfo {
    public:
    static int32_t m_rewardFactionList[10];
    static int32_t m_rewardFactionAmounts[10];
    static int32_t IsLowLevel(int32_t level);
    static QUEST_STATE m_state;
    static int32_t m_currentQuest;
    static int32_t m_autoLaunched;
    static int32_t m_startQuestCheat;
    static int32_t m_lastChosenItem;
    static int32_t m_responseSent;
    static float m_rewardHonorMultiplier;
    static int32_t m_rewardSpell;
    static int32_t m_rewardSpellCast;
    static int32_t m_rewardTitle;
    static uint32_t m_rewardFactionMask;
    static int32_t m_rewardFactionIDs[5];
    static int32_t m_rewardFactionValues[5];
    static int32_t m_rewardFactionOverrides[5];
    static void ClearState();
    static void ClearQuestItems();
    static void ProcessQuestLogRewardFactions();
    static void InitRewRepFields(uint32_t index, int32_t faction, int32_t value, int32_t override);
    static void InitRewRepShowMask(uint32_t mask);
    static int32_t m_numRewardFactions;
    static void SetState(WOWGUID npc, WOWGUID inform, QUEST_STATE state, const char* text, int32_t questId, int32_t startCheat);
    static void SetLogDescription(const char* text);
    static void AddReward(const char* title, const uint32_t* choiceItems, const uint32_t* choiceDisplays, const uint32_t* choiceCounts, int32_t numChoices, const uint32_t* rewardItems, const uint32_t* rewardDisplays, const uint32_t* rewardCounts, int32_t numRewards, int32_t money, int32_t xp, uint32_t honor, float honorMultiplier, int32_t spell, int32_t spellCast, int32_t autoLaunched, int32_t suggestedGroupNum, int32_t rewardTitle, int32_t talents, int32_t arenaPoints, uint32_t factionMask, const uint32_t* factionIDs, const uint32_t* factionValues, const uint32_t* factionOverrides, uint32_t questFlags);
    static void QuestGiverFinished(int32_t keepOpen, uint32_t questId, int32_t notify);
    static void EnterWorld();
    static WOWGUID m_npc;
    static WOWGUID m_npcInform;
    static char m_greetingText[2048];
    static char m_questTitle[512];
    static char m_rewardText[3000];
    static char m_progressText[3000];
    static char m_questLogText[3000];
    static char m_questText[3000];
    static int32_t m_rewardMoney;
    static int32_t m_rewardXP;
    static int32_t m_rewardTalents;
    static int32_t m_numQuests;
    static int32_t m_numInProgress;
    static QuestItemSlot m_questItems[6];
    static QuestGiverQuest m_inProgressQuests[32];
    static QuestGiverQuest m_availableQuests[32];
    static uint32_t m_rewardHonor;
    static int32_t m_rewardArenaPoints;
    static int32_t m_requiredMoney;
    static int32_t m_suggestedGroupNum;
    static uint32_t m_questFlags;
    static uint32_t m_pendingQuest;

    static void ConfirmAcceptQuest(int32_t questId, const char* title, const WOWGUID* guid);
    static void LeaveWorld();
    static int32_t m_canComplete;
    static void AddQuest(int32_t questId, const char* title, int32_t level, int32_t flags, int32_t turnIn, int32_t repeatable);
    static void AddQuestInProgress(int32_t questId, const char* title, int32_t level, int32_t flags, int32_t repeatable);
    static void EndQuestList();
    static void AddItemRequest(const char* title, const uint32_t* itemIds, const uint32_t* itemCounts, const uint32_t* itemDisplays, int32_t numItems, int32_t requiredMoney, int32_t canComplete, int32_t autoLaunched, int32_t suggestedGroupNum, uint32_t questFlags);
    static int32_t GetNumQuestChoices();
    static int32_t GetQuestItemID(const char* type, uint32_t index);
    static bool IsQuestCompleteAt(uint32_t index);
    static int32_t IsCompletable();
    static void QueryQuest(uint32_t index);
    static void CompleteQuest(uint32_t index);
    static void AcceptQuest();
    static void DeclineQuest();
    static void GiveQuestItems();
    static int32_t QuestItemInfo(const char* type, uint32_t index, char* name, uint32_t nameSize, char* texture, uint32_t textureSize, int32_t* count, int32_t* quality, int32_t* usable);
    static int32_t GetReward(int32_t choice);
};
struct lua_State;

void QuestInfoRegisterScriptFunctions();
void QuestInfoUnregisterScriptFunctions();
void ProcessQuestRewardFactions(int32_t factionId, int32_t amount, int32_t upperRank, int32_t lowerRank, int32_t source, int32_t noSpillover);

#endif
