#include "QuestFrame.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include <cstdlib>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/FactionRec.h"
#include "WoW/Source/DB/Rec/QuestFactionRewardRec.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Tutorial.h"

// STATIC: 0x00C0173C
int32_t CGQuestInfo::m_numRewardFactions;

// STATIC: 0x00ACF160
static const uint32_t s_rewardFactionMaskBits[5] = { 0x1, 0x2, 0x4, 0x8, 0x10 };

// STATIC: 0x00C01840
int32_t CGQuestInfo::m_rewardFactionList[10];

// STATIC: 0x00C01818
int32_t CGQuestInfo::m_rewardFactionAmounts[10];

// STATIC: 0x00C0D658
QUEST_STATE CGQuestInfo::m_state;

// STATIC: 0x00C0D65C
int32_t CGQuestInfo::m_currentQuest;

// STATIC: 0x00C0D660
int32_t CGQuestInfo::m_canComplete;

// STATIC: 0x00C0D6C0
static char s_questItemTexture[260];

// STATIC: 0x00C0D664
int32_t CGQuestInfo::m_autoLaunched;

// STATIC: 0x00C0D668
int32_t CGQuestInfo::m_startQuestCheat;

// STATIC: 0x00C0D66C
int32_t CGQuestInfo::m_lastChosenItem;

// STATIC: 0x00C0D6AC
int32_t CGQuestInfo::m_responseSent;

// STATIC: 0x00C0D67C
float CGQuestInfo::m_rewardHonorMultiplier;

// STATIC: 0x00C0D680
int32_t CGQuestInfo::m_rewardSpell;

// STATIC: 0x00C0D684
int32_t CGQuestInfo::m_rewardSpellCast;

// STATIC: 0x00C0D688
int32_t CGQuestInfo::m_rewardTitle;

// STATIC: 0x00C0D698
uint32_t CGQuestInfo::m_rewardFactionMask;

// STATIC: 0x00C01728
int32_t CGQuestInfo::m_rewardFactionIDs[5];

// STATIC: 0x00C01714
int32_t CGQuestInfo::m_rewardFactionValues[5];

// STATIC: 0x00C01700
int32_t CGQuestInfo::m_rewardFactionOverrides[5];

// STATIC: 0x00C0D648
WOWGUID CGQuestInfo::m_npc;

// STATIC: 0x00C0D650
WOWGUID CGQuestInfo::m_npcInform;

// STATIC: 0x00C0CC48
char CGQuestInfo::m_greetingText[2048];

// STATIC: 0x00C0D448
char CGQuestInfo::m_questTitle[512];

// STATIC: 0x00C09D68
char CGQuestInfo::m_rewardText[3000];

// STATIC: 0x00C0A920
char CGQuestInfo::m_progressText[3000];

// STATIC: 0x00C0B4D8
char CGQuestInfo::m_questLogText[3000];

// STATIC: 0x00C0C090
char CGQuestInfo::m_questText[3000];

// STATIC: 0x00C0D670
int32_t CGQuestInfo::m_rewardMoney;

// STATIC: 0x00C0D674
int32_t CGQuestInfo::m_rewardXP;

// STATIC: 0x00C0D690
int32_t CGQuestInfo::m_rewardTalents;

// STATIC: 0x00C0D69C
int32_t CGQuestInfo::m_numQuests;

// STATIC: 0x00C0D6A0
int32_t CGQuestInfo::m_numInProgress;

// STATIC: 0x00C0D678
uint32_t CGQuestInfo::m_rewardHonor;

// STATIC: 0x00C0D694
int32_t CGQuestInfo::m_rewardArenaPoints;

// STATIC: 0x00C0D6A4
int32_t CGQuestInfo::m_requiredMoney;

// STATIC: 0x00C0D6B0
int32_t CGQuestInfo::m_suggestedGroupNum;

// STATIC: 0x00C0D6B4
uint32_t CGQuestInfo::m_questFlags;

// STATIC: 0x00C0D6A8
uint32_t CGQuestInfo::m_pendingQuest;

// STATIC: 0x00C01740
QuestItemSlot CGQuestInfo::m_questItems[6];

// STATIC: 0x00C01868
QuestGiverQuest CGQuestInfo::m_inProgressQuests[32];

// STATIC: 0x00C05AE8
QuestGiverQuest CGQuestInfo::m_availableQuests[32];

// VERIFY: 63c859fb manual
// OFFSET: 0x0058CA70
void CGQuestInfo::QuestGiverFinished(int32_t keepOpen, uint32_t questId, int32_t notify) {
    if (!CGQuestInfo::m_npc) {
        return;
    }

    if (notify && ClntObjMgrObjectPtr(CGQuestInfo::m_npc, TYPE_PLAYER, __FILE__, __LINE__)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_QUEST_PUSH_RESULT));
        msg.Put(CGQuestInfo::m_npcInform ? CGQuestInfo::m_npcInform.m_guid : CGQuestInfo::m_npc.m_guid);
        msg.Put(questId);
        msg.Put(static_cast<uint8_t>(3));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (keepOpen) {
        return;
    }

    WOWGUID npc = CGQuestInfo::m_npc;
    CGQuestInfo::m_npc = WOWGUID();
    CGQuestInfo::m_npcInform = WOWGUID();

    if (!CGGossipInfo::m_gossip) {
        CGGameUI::ClearInteractTarget(npc, 0);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_FINISHED, nullptr);
}

// VERIFY: fa332c43 manual
// OFFSET: 0x0058B720
void CGQuestInfo::ClearState() {
    for (int32_t i = 0; i < 32; i++) {
        CGQuestInfo::m_availableQuests[i].m_questID = 0;
        CGQuestInfo::m_availableQuests[i].m_level = 0;
        CGQuestInfo::m_availableQuests[i].m_flags = 0;
        CGQuestInfo::m_availableQuests[i].m_title[0] = 0;
        CGQuestInfo::m_availableQuests[i].m_turnIn = 0;
        CGQuestInfo::m_availableQuests[i].m_repeatable = 0;
        CGQuestInfo::m_inProgressQuests[i].m_questID = 0;
        CGQuestInfo::m_inProgressQuests[i].m_level = 0;
        CGQuestInfo::m_inProgressQuests[i].m_flags = 0;
        CGQuestInfo::m_inProgressQuests[i].m_title[0] = 0;
        CGQuestInfo::m_inProgressQuests[i].m_turnIn = 0;
        CGQuestInfo::m_inProgressQuests[i].m_repeatable = 0;
    }

    CGQuestInfo::m_numQuests = 0;
    CGQuestInfo::m_numInProgress = 0;
    CGQuestInfo::m_greetingText[0] = 0;
    CGQuestInfo::m_questTitle[0] = 0;
    CGQuestInfo::m_questText[0] = 0;
    CGQuestInfo::m_questLogText[0] = 0;
    CGQuestInfo::m_progressText[0] = 0;
    CGQuestInfo::m_rewardText[0] = 0;
    CGQuestInfo::m_suggestedGroupNum = 0;
    CGQuestInfo::m_autoLaunched = 0;
    CGQuestInfo::m_questFlags = 0;
}

// VERIFY: d07a48c9 auto
// OFFSET: 0x0058B7D0
void CGQuestInfo::ClearQuestItems() {
    for (int32_t i = 0; i < 6; i++) {
        CGQuestInfo::m_questItems[i].m_reward.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_reward.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_reward.m_count = 0;
        CGQuestInfo::m_questItems[i].m_choice.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_choice.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_choice.m_count = 0;
        CGQuestInfo::m_questItems[i].m_required.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_required.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_required.m_count = 0;
    }
}

// VERIFY: 2ce3fb4d manual
// OFFSET: 0x0058C5A0
void CGQuestInfo::SetState(WOWGUID npc, WOWGUID inform, QUEST_STATE state, const char* text, int32_t questId, int32_t startCheat) {
    CGObject_C* object = ClntObjMgrObjectPtr(npc, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object || !(object->m_obj->m_type & (TYPE_ITEM | TYPE_UNIT | TYPE_GAMEOBJECT))) {
        return;
    }

    if (!state) {
        CGQuestInfo::ClearState();
    }

    char buffer[3000];
    WOWGUID player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(text, buffer, sizeof(buffer), player, nullptr, 0);

    if (!buffer[0]) {
        SStrCopy(buffer, " ", sizeof(buffer));
    }

    switch (state) {
        case QUEST_GREETING:
            SStrCopy(CGQuestInfo::m_greetingText, buffer, sizeof(CGQuestInfo::m_greetingText));
            break;

        case QUEST_OFFER:
            SStrCopy(CGQuestInfo::m_questText, buffer, sizeof(CGQuestInfo::m_questText));
            break;

        case QUEST_ACCEPTED:
            SStrCopy(CGQuestInfo::m_progressText, buffer, sizeof(CGQuestInfo::m_progressText));
            break;

        case QUEST_REWARD:
            SStrCopy(CGQuestInfo::m_rewardText, buffer, sizeof(CGQuestInfo::m_rewardText));
            break;

        default:
            break;
    }

    CGQuestInfo::m_state = state;
    CGQuestInfo::m_currentQuest = questId;
    CGQuestInfo::m_startQuestCheat = startCheat;

    if (!(object->m_obj->m_type & TYPE_GAMEOBJECT) || npc != CGGameUI::m_interactTarget) {
        CGGameUI::SetInteractTarget(npc);
    }

    CGQuestInfo::m_npc = npc;
    CGQuestInfo::m_npcInform = inform;
    CGQuestInfo::m_lastChosenItem = 0;
    CGQuestInfo::m_responseSent = 0;
}

// VERIFY: 2ebdd245 auto
// OFFSET: 0x0058C720
void CGQuestInfo::SetLogDescription(const char* text) {
    if (!text || !*text) {
        CGQuestInfo::m_questLogText[0] = 0;
        return;
    }

    char buffer[3000];
    WOWGUID player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(text, buffer, sizeof(buffer), player, nullptr, 0);
    SStrCopy(CGQuestInfo::m_questLogText, buffer, sizeof(CGQuestInfo::m_questLogText));
}

// VERIFY: 99119eef manual
// OFFSET: 0x0058DD50
void CGQuestInfo::AddReward(const char* title, const uint32_t* choiceItems, const uint32_t* choiceDisplays, const uint32_t* choiceCounts, int32_t numChoices, const uint32_t* rewardItems, const uint32_t* rewardDisplays, const uint32_t* rewardCounts, int32_t numRewards, int32_t money, int32_t xp, uint32_t honor, float honorMultiplier, int32_t spell, int32_t spellCast, int32_t autoLaunched, int32_t suggestedGroupNum, int32_t rewardTitle, int32_t talents, int32_t arenaPoints, uint32_t factionMask, const uint32_t* factionIDs, const uint32_t* factionValues, const uint32_t* factionOverrides, uint32_t questFlags) {
    CGQuestInfo::ClearQuestItems();
    int32_t i = 0;

    for (; i < numChoices; i++) {
        CGQuestInfo::m_questItems[i].m_choice.m_itemID = choiceItems[i];
        CGQuestInfo::m_questItems[i].m_choice.m_displayID = choiceDisplays[i];
        CGQuestInfo::m_questItems[i].m_choice.m_count = choiceCounts[i];
    }

    for (; i < 6; i++) {
        CGQuestInfo::m_questItems[i].m_choice.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_choice.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_choice.m_count = 1;
    }

    i = 0;

    for (; i < numRewards; i++) {
        CGQuestInfo::m_questItems[i].m_reward.m_itemID = rewardItems[i];
        CGQuestInfo::m_questItems[i].m_reward.m_displayID = rewardDisplays[i];
        CGQuestInfo::m_questItems[i].m_reward.m_count = rewardCounts[i];
    }

    for (; i < 6; i++) {
        CGQuestInfo::m_questItems[i].m_reward.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_reward.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_reward.m_count = 1;
    }

    if (title) {
        SStrCopy(CGQuestInfo::m_questTitle, *title ? title : " ", sizeof(CGQuestInfo::m_questTitle));
    }

    CGQuestInfo::m_autoLaunched = autoLaunched;

    if (money < 0) {
        CGQuestInfo::m_rewardMoney = 0;
        CGQuestInfo::m_requiredMoney = -money;
    } else {
        CGQuestInfo::m_rewardMoney = money;
        CGQuestInfo::m_requiredMoney = 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && static_cast<int32_t>(player->m_unit->m_unitFieldLevel) < static_cast<int32_t>(player->m_player->m_playerFieldMaxLevel)) {
        CGQuestInfo::m_rewardXP = xp;
    } else {
        CGQuestInfo::m_rewardXP = 0;
    }

    CGQuestInfo::m_rewardHonorMultiplier = honorMultiplier;
    CGQuestInfo::m_rewardSpell = spell;
    CGQuestInfo::m_rewardHonor = honor;
    CGQuestInfo::m_rewardTalents = talents;
    CGQuestInfo::m_rewardSpellCast = spellCast;
    CGQuestInfo::m_rewardTitle = rewardTitle;
    CGQuestInfo::m_rewardArenaPoints = arenaPoints;
    CGQuestInfo::m_rewardFactionMask = factionMask;

    for (uint32_t j = 0; j < 5; j++) {
        CGQuestInfo::m_rewardFactionIDs[j] = factionIDs[j];
    }

    for (uint32_t j = 0; j < 5; j++) {
        CGQuestInfo::m_rewardFactionValues[j] = factionValues[j];
    }

    for (uint32_t j = 0; j < 5; j++) {
        CGQuestInfo::m_rewardFactionOverrides[j] = factionOverrides[j];
    }

    CGQuestInfo::ProcessQuestLogRewardFactions();
    CGQuestInfo::m_suggestedGroupNum = suggestedGroupNum;
    CGQuestInfo::m_questFlags = questFlags;

    if (CGQuestInfo::m_state == QUEST_OFFER) {
        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_DETAIL, nullptr);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_COMPLETE, nullptr);
    }
}

// VERIFY: c5b8f724 auto
// OFFSET: 0x0058BB60
void CGQuestInfo::InitRewRepFields(uint32_t index, int32_t faction, int32_t value, int32_t override) {
    if (index <= 4) {
        CGQuestInfo::m_rewardFactionIDs[index] = faction;
        CGQuestInfo::m_rewardFactionValues[index] = value;
        CGQuestInfo::m_rewardFactionOverrides[index] = override;
    }
}

// VERIFY: 12701ac3 auto
// OFFSET: 0x0058BB90
void CGQuestInfo::InitRewRepShowMask(uint32_t mask) {
    CGQuestInfo::m_rewardFactionMask = mask;
}

// VERIFY: 9b223ab0 manual
// OFFSET: 0x0058C790
void ProcessQuestRewardFactions(int32_t factionId, int32_t amount, int32_t upperRank, int32_t lowerRank, int32_t source, int32_t noSpillover) {
    FactionRec* faction = g_factionDB.GetRecord(factionId);

    if (!faction) {
        return;
    }

    int32_t current = GetRepListRepValue(factionId);
    int32_t result = current + amount;

    if (amount <= 0) {
        if (lowerRank >= 0) {
            int32_t limit = 100 * CGReputationInfo::m_reactionThresholds[lowerRank];
            int32_t floor = current >= limit ? limit : current;

            if (result <= floor) {
                if (current < limit) {
                    return;
                }

                result = limit;
            }
        }
    } else if (upperRank >= 0) {
        int32_t limit = 100 * CGReputationInfo::m_reactionThresholds[upperRank + 1] - 100;
        int32_t ceiling = current <= limit ? limit : current;

        if (result >= ceiling) {
            if (current > limit) {
                return;
            }

            result = limit;
        }
    }

    if (result == current || static_cast<uint32_t>(CGQuestInfo::m_numRewardFactions) >= 10) {
        return;
    }

    CGQuestInfo::m_rewardFactionAmounts[CGQuestInfo::m_numRewardFactions] = amount;
    CGQuestInfo::m_rewardFactionList[CGQuestInfo::m_numRewardFactions] = factionId;
    CGQuestInfo::m_numRewardFactions++;

    if (noSpillover) {
        return;
    }

    int32_t parent = faction->m_parentFactionID;

    if (parent && parent != source) {
        float spill = amount * faction->m_parentFactionMod[1];

        if (spill != 0.0f) {
            ProcessQuestRewardFactions(parent, static_cast<int32_t>(spill), faction->m_parentFactionCap[1], -1, faction->m_id, 0);
        }
    }

    TSGrowableArray<const FactionRec*>* children = CGReputationInfo::GetFactionChildren(faction->m_id);

    if (!children) {
        return;
    }

    for (uint32_t i = children->m_count; i; i--) {
        const FactionRec* child = children->m_data[i - 1];

        if (child->m_id == source) {
            continue;
        }

        float spill = amount * child->m_parentFactionMod[0];

        if (spill != 0.0f) {
            ProcessQuestRewardFactions(child->m_id, static_cast<int32_t>(spill), child->m_parentFactionCap[0], -1, faction->m_id, 0);
        }
    }
}

// VERIFY: e311adfb manual
// OFFSET: 0x0058D050
void CGQuestInfo::ProcessQuestLogRewardFactions() {
    CGQuestInfo::m_numRewardFactions = 0;

    for (uint32_t i = 0; i < 10; i++) {
        CGQuestInfo::m_rewardFactionList[i] = 0;
    }

    for (uint32_t i = 0; i < 10; i++) {
        CGQuestInfo::m_rewardFactionAmounts[i] = 0;
    }

    for (uint32_t i = 0; i < 5; i++) {
        FactionRec* faction = g_factionDB.GetRecord(CGQuestInfo::m_rewardFactionIDs[i]);

        if (!faction) {
            continue;
        }

        int32_t value = CGQuestInfo::m_rewardFactionValues[i];
        QuestFactionRewardRec* reward = g_questFactionRewardDB.GetRecord((value < 0) + 1);
        uint32_t index = abs(value);

        if (!reward || index >= 10 || !reward->m_difficulty[index]) {
            continue;
        }

        int32_t amount = CGQuestInfo::m_rewardFactionOverrides[i];

        if (!amount) {
            amount = 100 * reward->m_difficulty[index];
        }

        ProcessQuestRewardFactions(faction->m_id, amount, -1, -1, 0, (CGQuestInfo::m_rewardFactionMask & s_rewardFactionMaskBits[i]) != 0);
    }
}

// VERIFY: a3658565 auto
// OFFSET: 0x0058B8F0
static void QuestItemStatsCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = { 0 };

    if (g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_ITEM_UPDATE, nullptr);
    }
}

// VERIFY: 0e16cb6e auto
// OFFSET: 0x0058B990
void CGQuestInfo::AddQuest(int32_t questId, const char* title, int32_t level, int32_t flags, int32_t turnIn, int32_t repeatable) {
    CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_questID = questId;
    CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_level = level;
    CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_flags = flags;

    if (title) {
        SStrCopy(CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_title, title, sizeof(CGQuestInfo::m_availableQuests[0].m_title));
    }

    CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_turnIn = turnIn;
    CGQuestInfo::m_availableQuests[CGQuestInfo::m_numQuests].m_repeatable = repeatable;
    CGQuestInfo::m_numQuests++;
}

// VERIFY: 199b28f6 auto
// OFFSET: 0x0058BA00
void CGQuestInfo::AddQuestInProgress(int32_t questId, const char* title, int32_t level, int32_t flags, int32_t repeatable) {
    CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_questID = questId;
    CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_level = level;
    CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_flags = flags;

    if (title) {
        SStrCopy(CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_title, title, sizeof(CGQuestInfo::m_inProgressQuests[0].m_title));
    }

    CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_turnIn = 0;
    CGQuestInfo::m_inProgressQuests[CGQuestInfo::m_numInProgress].m_repeatable = repeatable;
    CGQuestInfo::m_numInProgress++;
}

// VERIFY: b8734c98 auto
// OFFSET: 0x0058BA70
void CGQuestInfo::EndQuestList() {
    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_GREETING, nullptr);
}

// VERIFY: 1a14e0c3 manual
// OFFSET: 0x0058BA80
void CGQuestInfo::AddItemRequest(const char* title, const uint32_t* itemIds, const uint32_t* itemCounts, const uint32_t* itemDisplays, int32_t numItems, int32_t requiredMoney, int32_t canComplete, int32_t autoLaunched, int32_t suggestedGroupNum, uint32_t questFlags) {
    int32_t i = 0;

    for (; i < numItems; i++) {
        CGQuestInfo::m_questItems[i].m_required.m_itemID = itemIds[i];
        CGQuestInfo::m_questItems[i].m_required.m_displayID = itemDisplays[i];
        CGQuestInfo::m_questItems[i].m_required.m_count = itemCounts[i];
    }

    for (; i < 6; i++) {
        CGQuestInfo::m_questItems[i].m_required.m_itemID = 0;
        CGQuestInfo::m_questItems[i].m_required.m_displayID = 0;
        CGQuestInfo::m_questItems[i].m_required.m_count = 1;
    }

    if (title) {
        if (*title) {
            SStrCopy(CGQuestInfo::m_questTitle, title, sizeof(CGQuestInfo::m_questTitle));
        } else {
            SStrCopy(CGQuestInfo::m_questTitle, " ", sizeof(CGQuestInfo::m_questTitle));
        }
    }

    CGQuestInfo::m_requiredMoney = requiredMoney;
    CGQuestInfo::m_canComplete = canComplete;
    CGQuestInfo::m_autoLaunched = autoLaunched;
    CGQuestInfo::m_suggestedGroupNum = suggestedGroupNum;
    CGQuestInfo::m_questFlags = questFlags;
    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_PROGRESS, nullptr);
}

// VERIFY: 834f12df auto
// OFFSET: 0x0058BBA0
int32_t CGQuestInfo::GetNumQuestChoices() {
    int32_t count = 0;

    for (uint32_t i = 0; i < 6; i++) {
        if (!CGQuestInfo::m_questItems[i].m_choice.m_itemID) {
            break;
        }

        count++;
    }

    return count;
}

// VERIFY: 5df087bb auto
// OFFSET: 0x0058BBC0
int32_t CGQuestInfo::GetQuestItemID(const char* type, uint32_t index) {
    if (index > 6) {
        return 0;
    }

    if (!SStrCmpI(type, "reward", STORM_MAX_STR)) {
        return CGQuestInfo::m_questItems[index].m_reward.m_itemID;
    }

    if (!SStrCmpI(type, "choice", STORM_MAX_STR)) {
        return CGQuestInfo::m_questItems[index].m_choice.m_itemID;
    }

    if (SStrCmpI(type, "required", STORM_MAX_STR)) {
        return 0;
    }

    return CGQuestInfo::m_questItems[index].m_required.m_itemID;
}

// VERIFY: ed6659d1 manual
// OFFSET: 0x0058BCB0
bool CGQuestInfo::IsQuestCompleteAt(uint32_t index) {
    if (index >= CGQuestInfo::m_numInProgress) {
        return false;
    }

    uint32_t logIndex = CGQuestLog::GetQuestIndex(CGQuestInfo::m_inProgressQuests[index].m_questID) - 1;
    return CGQuestLog::IsQuestCompleteEx(logIndex, 1) || CGQuestLog::HasNoObjectives(logIndex);
}

// VERIFY: db67485a manual
// OFFSET: 0x0058CBB0
int32_t CGQuestInfo::IsCompletable() {
    if (!CGQuestInfo::m_canComplete) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    for (uint32_t i = 0; i < 6; i++) {
        int32_t itemId = CGQuestInfo::m_questItems[i].m_required.m_itemID;

        if (!itemId) {
            return 1;
        }

        if (player->m_bag.GetItemTypeCount(itemId, 8) < CGQuestInfo::m_questItems[i].m_required.m_count) {
            return 0;
        }
    }

    return 1;
}

// VERIFY: 11ef127c manual
// OFFSET: 0x0058CC20
void CGQuestInfo::QueryQuest(uint32_t index) {
    if (index > CGQuestInfo::m_numQuests || CGQuestInfo::m_responseSent) {
        return;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    if (CGQuestInfo::m_availableQuests[index].m_turnIn) {
        CGPlayer_C::CompleteQuest(CGQuestInfo::m_npc, CGQuestInfo::m_availableQuests[index].m_questID);
    } else {
        CGPlayer_C::QueryQuest(CGQuestInfo::m_npc, CGQuestInfo::m_availableQuests[index].m_questID);
    }

    CGQuestInfo::m_responseSent = 1;
}

// VERIFY: bd530fb2 manual
// OFFSET: 0x0058CCB0
void CGQuestInfo::CompleteQuest(uint32_t index) {
    if (index > CGQuestInfo::m_numInProgress || CGQuestInfo::m_responseSent) {
        return;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    CGTutorial::TriggerTutorial(0x21);
    CGPlayer_C::CompleteQuest(CGQuestInfo::m_npc, CGQuestInfo::m_inProgressQuests[index].m_questID);
    CGQuestInfo::m_responseSent = 1;
}

// VERIFY: 900e22ab manual
// OFFSET: 0x0058CD20
void CGQuestInfo::AcceptQuest() {
    if (CGQuestInfo::m_state != QUEST_OFFER) {
        return;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) || CGQuestInfo::m_responseSent) {
        return;
    }

    CGPlayer_C::AcceptQuest(CGQuestInfo::m_npcInform ? CGQuestInfo::m_npcInform : CGQuestInfo::m_npc, CGQuestInfo::m_currentQuest, CGQuestInfo::m_startQuestCheat);
    CGQuestInfo::m_startQuestCheat = 0;
    CGQuestInfo::m_responseSent = 1;

    if (!CGQuestInfo::m_npc) {
        return;
    }

    WOWGUID npc = CGQuestInfo::m_npc;
    CGQuestInfo::m_npc = WOWGUID();
    CGQuestInfo::m_npcInform = WOWGUID();

    if (!CGGossipInfo::m_gossip) {
        CGGameUI::ClearInteractTarget(npc, 0);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_FINISHED, nullptr);
}

// VERIFY: 0ea917e1 manual
// OFFSET: 0x0058CE00
void CGQuestInfo::DeclineQuest() {
    if (CGQuestInfo::m_responseSent) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(CGQuestInfo::m_npc, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if ((object->m_obj->m_type & TYPE_UNIT) && (static_cast<CGUnit_C*>(object)->m_unit->m_unitNpcFlags & 1) && ClntObjMgrGetActivePlayerObj()) {
        CGPlayer_C::TalkToGossip(CGQuestInfo::m_npc);
        CGQuestInfo::m_responseSent = 1;
        return;
    }

    uint32_t type = object->m_obj->m_type;

    if ((type & (TYPE_ITEM | TYPE_PLAYER)) || CGQuestInfo::m_autoLaunched) {
        CGQuestInfo::QuestGiverFinished(0, CGQuestInfo::m_currentQuest, 1);
        return;
    }

    if (type & TYPE_GAMEOBJECT) {
        object->OnRightClick();
    } else {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_QUEST_GIVER_HELLO));
        msg.Put(CGQuestInfo::m_npc.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    CGQuestInfo::m_responseSent = 1;
}

// VERIFY: 84d8d008 manual
// OFFSET: 0x0058CF40
void CGQuestInfo::GiveQuestItems() {
    if (CGQuestInfo::m_state != QUEST_ACCEPTED || CGQuestInfo::m_responseSent) {
        return;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return;
    }

    CGTutorial::TriggerTutorial(0x21);
    CGPlayer_C::GiveQuestItems(CGQuestInfo::m_npc, CGQuestInfo::m_currentQuest);
    CGQuestInfo::m_responseSent = 1;
}

// VERIFY: 840a6082 manual
// OFFSET: 0x0058CFA0
int32_t CGQuestInfo::GetReward(int32_t choice) {
    if (CGQuestInfo::m_state != QUEST_REWARD) {
        return 1;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) || CGQuestInfo::m_responseSent) {
        return 1;
    }

    int32_t numChoices = CGQuestInfo::GetNumQuestChoices();

    if (numChoices > 0) {
        if (choice >= numChoices || choice < 0) {
            return 0;
        }

        CGQuestInfo::m_lastChosenItem = CGQuestInfo::m_questItems[choice].m_choice.m_itemID;
    }

    CGTutorial::TriggerTutorial(0x21);
    CGPlayer_C::GetQuestReward(CGQuestInfo::m_npc, CGQuestInfo::m_currentQuest, choice <= 0 ? 0 : choice);
    CGQuestInfo::m_responseSent = 1;
    return 1;
}

// VERIFY: 5dc83631 manual
// OFFSET: 0x0058D190
int32_t CGQuestInfo::QuestItemInfo(const char* type, uint32_t index, char* name, uint32_t nameSize, char* texture, uint32_t textureSize, int32_t* count, int32_t* quality, int32_t* usable) {
    *name = 0;
    *texture = 0;
    *count = 1;
    *quality = 0;
    *usable = 1;

    if (index > 6) {
        return 0;
    }

    int32_t itemId;
    int32_t displayId;

    if (!SStrCmpI(type, "reward", STORM_MAX_STR)) {
        itemId = CGQuestInfo::m_questItems[index].m_reward.m_itemID;
        displayId = CGQuestInfo::m_questItems[index].m_reward.m_displayID;
        *count = CGQuestInfo::m_questItems[index].m_reward.m_count;
    } else if (!SStrCmpI(type, "choice", STORM_MAX_STR)) {
        itemId = CGQuestInfo::m_questItems[index].m_choice.m_itemID;
        displayId = CGQuestInfo::m_questItems[index].m_choice.m_displayID;
        *count = CGQuestInfo::m_questItems[index].m_choice.m_count;
    } else if (!SStrCmpI(type, "required", STORM_MAX_STR)) {
        itemId = CGQuestInfo::m_questItems[index].m_required.m_itemID;
        displayId = CGQuestInfo::m_questItems[index].m_required.m_displayID;
        *count = CGQuestInfo::m_questItems[index].m_required.m_count;
    } else {
        return 0;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, CGQuestInfo::m_npc, QuestItemStatsCallback, nullptr, false);

    if (stats) {
        SStrCopy(name, stats->m_name[0], nameSize);
        *quality = stats->m_inventoryType ? stats->m_quality : -1;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    SStrPrintf(s_questItemTexture, sizeof(s_questItemTexture), "%s%s%s", path, *path ? "\\" : "", CGItem_C::GetInventoryArt(displayId));
    SStrCopy(texture, s_questItemTexture, textureSize);

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t error;

    if (player && stats && !player->CanUseItem(stats, error)) {
        *usable = 0;
    }

    return 1;
}

// VERIFY: 1981b73a auto
// OFFSET: 0x0058B940
void CGQuestInfo::EnterWorld() {
    CGQuestInfo::m_npc = WOWGUID{ 0 };
    CGQuestInfo::m_npcInform = WOWGUID{ 0 };
    CGQuestInfo::m_state = QUEST_GREETING;
    CGQuestInfo::m_currentQuest = 0;
    CGQuestInfo::m_canComplete = 0;
    CGQuestInfo::m_autoLaunched = 0;
    CGQuestInfo::m_requiredMoney = 0;
    CGQuestInfo::m_lastChosenItem = 0;
    CGQuestInfo::m_questFlags = 0;
    CGQuestInfo::ClearState();
    CGQuestInfo::ClearQuestItems();
}

// VERIFY: 8b9ea6e9 auto
// OFFSET: 0x0058BC50
void CGQuestInfo::ConfirmAcceptQuest(int32_t questId, const char* title, const WOWGUID* guid) {
    CGQuestInfo::m_pendingQuest = questId;
    NameCache* name = g_nameDBCache.GetRecord(guid->m_guid, WOWGUID{ 0 }, nullptr, nullptr, false);

    if (name) {
        FrameScript_SignalEvent(SCRIPT_EVENT_QUEST_ACCEPT_CONFIRM, "%s%s", name->m_name, title);
    }
}

// VERIFY: 5ae7745b auto
// OFFSET: 0x0058DD30
void CGQuestInfo::LeaveWorld() {
    CGQuestInfo::QuestGiverFinished(0, CGQuestInfo::m_currentQuest, 1);
}
