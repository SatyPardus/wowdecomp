#include "GossipInfo.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/Types.h"
#include <malloc.h>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/NPCText.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Ui/MinimapFrame.h"

// STATIC: 0x00A12FD0
static const int32_t s_questGreenRange[20] = { 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };

// STATIC: 0x00BFC968
GossipQuestInfo CGGossipInfo::m_quests[32];

// STATIC: 0x00C00BE8
int32_t CGGossipInfo::m_textPending;

// STATIC: 0x00C016F8
uint32_t CGGossipInfo::m_gossipID;

// STATIC: 0x00C016F0
WOWGUID CGGossipInfo::m_gossip;

// STATIC: 0x00C016FC
uint32_t CGGossipInfo::m_gossipOptionsCount;

// STATIC: 0x00C00BF0
GossipOption CGGossipInfo::m_options[32];

// STATIC: 0x00C00EF0
char CGGossipInfo::m_gossipText[2048];

// VERIFY: ac8f3a04 auto
// OFFSET: 0x0058B6E0
void CGGossipInfo::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_GOSSIP_MESSAGE, Packet_SMSG_GOSSIP_MESSAGE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GOSSIP_COMPLETE, Packet_SMSG_GOSSIP_COMPLETE, nullptr);
    ClientServices::SetMessageHandler(SMSG_GOSSIP_POI, Packet_SMSG_GOSSIP_POI, nullptr);
}

// VERIFY: 8d00a6fa manual
// OFFSET: 0x0058A550
void CGGossipInfo::SetGossip(const WOWGUID& guid) {
    if (guid == CGGossipInfo::m_gossip) {
        return;
    }

    if (guid) {
        CGGameUI::SetInteractTarget(guid);
        CGGossipInfo::m_gossip = guid;
        return;
    }

    if (!CGQuestInfo::m_npc) {
        CGGameUI::ClearInteractTarget(CGGossipInfo::m_gossip, 0);
    }

    CGGossipInfo::m_gossip = WOWGUID();
    CGGossipInfo::m_gossipID = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_GOSSIP_CLOSED, nullptr);
}

// VERIFY: 0cc63f6b manual
// OFFSET: 0x0058A7B0
void CGGossipInfo::ClearOptions() {
    CGGossipInfo::m_gossipOptionsCount = 0;

    for (uint32_t i = 0; i < 32; i++) {
        GossipOption& option = CGGossipInfo::m_options[i];

        if (option.m_text) {
            SMemFree(option.m_text, __FILE__, __LINE__, 0);
            option.m_text = nullptr;
        }

        if (option.m_boxText) {
            SMemFree(option.m_boxText, __FILE__, __LINE__, 0);
            option.m_boxText = nullptr;
        }

        option.m_index = -1;
    }

    for (uint32_t i = 0; i < 32; i++) {
        CGGossipInfo::m_quests[i].m_questID = 0;
    }
}

// VERIFY: c7f9173c auto
// OFFSET: 0x0058AAE0
static void GossipTextCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGGossipInfo::SetGossipText(id);
        return;
    }

    WOWGUID none = { 0 };
    CGGossipInfo::SetGossip(none);
}

// VERIFY: b728608d manual
// OFFSET: 0x0058AC30
void CGGossipInfo::SetGossipText(int32_t textId) {
    NPCText* text = g_npcTextDBCache.GetRecord(textId, CGGossipInfo::m_gossip, GossipTextCallback, nullptr, false);

    if (!text) {
        CGGossipInfo::m_textPending = 1;
        return;
    }

    CGGossipInfo::m_textPending = 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGObject_C* object = ClntObjMgrObjectPtr(CGGossipInfo::m_gossip, TYPE_OBJECT, __FILE__, __LINE__);

    if (!player || !object) {
        return;
    }

    int32_t female;

    if (object->m_obj->m_type & TYPE_UNIT) {
        female = static_cast<CGUnit_C*>(object)->m_unit->m_unitFieldBytes0[2] == UNITSEX_FEMALE;
    } else {
        female = text->m_text[0][0] == nullptr;
    }

    float total = 0.0f;

    for (uint32_t i = 0; i < 8; i++) {
        if (text->m_text[i][female]) {
            total += text->m_probability[i];
        }
    }

    uint32_t bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
    float threshold = (2.0f - *reinterpret_cast<float*>(&bits)) * total;
    float sum = 0.0f;
    uint32_t index;

    for (index = 0; index < 8; index++) {
        if (text->m_text[index][female]) {
            sum += text->m_probability[index];

            if (sum >= threshold) {
                break;
            }
        }
    }

    if (index >= 8) {
        ConsoleWriteA("Missing gossip text!", ERROR_COLOR);
        WOWGUID none = { 0 };
        CGGossipInfo::SetGossip(none);
        return;
    }

    uint32_t emoteCount = 0;

    while (emoteCount < 3 && text->m_emote[index][emoteCount]) {
        emoteCount++;
    }

    if (emoteCount > 0) {
        QUESTGIVEREMOTEQUEUE emotes;
        emotes.m_nodes = static_cast<QUESTGIVEREMOTENODE*>(alloca(emoteCount * sizeof(QUESTGIVEREMOTENODE)));
        emotes.m_count = emoteCount;

        for (uint32_t i = 0; i < emoteCount; i++) {
            emotes.m_nodes[i].m_emote = text->m_emote[index][i];
            emotes.m_nodes[i].m_delay = text->m_emoteDelay[index][i];
        }

        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGGossipInfo::m_gossip, TYPE_UNIT, __FILE__, __LINE__));

        if (unit) {
            unit->PlayEmotes(emotes);
        }
    }

    char buffer[3000];
    WOWGUID playerGuid = ClntObjMgrGetActivePlayer();
    QuestParserParseText(text->m_text[index][female], buffer, sizeof(buffer), playerGuid, nullptr, 0);
    uint32_t language = text->m_language[index];
    uint32_t skill = 0;
    player->GetLanguageSkill(language, skill);
    CGChat::TranslateMessage(language, skill, buffer, CGGossipInfo::m_gossipText, sizeof(CGGossipInfo::m_gossipText), 0, 1);
    FrameScript_SignalEvent(SCRIPT_EVENT_GOSSIP_SHOW, nullptr);
}

// VERIFY: 73df7f0a manual
// OFFSET: 0x0058B1B0
int32_t Packet_SMSG_GOSSIP_MESSAGE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);
    CGGossipInfo::SetGossip(guid);
    CGGossipInfo::ClearOptions();
    msg->Get(CGGossipInfo::m_gossipID);

    uint32_t textId;
    msg->Get(textId);

    uint32_t count;
    msg->Get(count);

    if (count > 32) {
        count = 32;
    }

    CGGossipInfo::m_gossipOptionsCount = count;
    char text[2048];

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        GossipOption& option = CGGossipInfo::m_options[i];
        msg->Get(reinterpret_cast<uint32_t&>(option.m_index));

        uint8_t type;
        msg->Get(type);

        uint8_t coded;
        msg->Get(coded);

        option.m_type = type;
        option.m_coded = coded;
        msg->Get(option.m_boxMoney);
        msg->GetString(text, sizeof(text));

        if (option.m_text) {
            SMemFree(option.m_text, __FILE__, __LINE__, 0);
        }

        option.m_text = SStrDupA(text, __FILE__, __LINE__);
        msg->GetString(text, sizeof(text));

        if (option.m_boxText) {
            SMemFree(option.m_boxText, __FILE__, __LINE__, 0);
        }

        option.m_boxText = text[0] ? SStrDupA(text, __FILE__, __LINE__) : nullptr;
    }

    msg->Get(count);

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        GossipQuestInfo& quest = CGGossipInfo::m_quests[i];
        msg->Get(quest.m_questID);
        msg->Get(quest.m_icon);
        msg->Get(reinterpret_cast<uint32_t&>(quest.m_level));
        msg->Get(quest.m_flags);

        uint8_t repeatable = 0;
        msg->Get(repeatable);
        quest.m_repeatable = repeatable;
        msg->GetString(quest.m_title, sizeof(quest.m_title));
    }

    CGGossipInfo::SetGossipText(textId);
    return 1;
}

// VERIFY: a6fc8431 manual
// OFFSET: 0x0058A5D0
int32_t CGGossipInfo::GetNumAvailableQuests() {
    int32_t count = 0;

    for (uint32_t i = 0; i < 32; i++) {
        if (!CGGossipInfo::m_quests[i].m_questID) {
            break;
        }

        if (CGGossipInfo::m_quests[i].m_icon != 3 && CGGossipInfo::m_quests[i].m_icon != 4) {
            count++;
        }
    }

    return count;
}

// VERIFY: 419ae02f auto
// OFFSET: 0x0058A660
GossipQuestInfo* CGGossipInfo::GetAvailableQuest(int32_t index) {
    int32_t found = 0;

    for (uint32_t i = 0; i < 32; i++) {
        if (!CGGossipInfo::m_quests[i].m_questID) {
            return nullptr;
        }

        if (CGGossipInfo::m_quests[i].m_icon != 3 && CGGossipInfo::m_quests[i].m_icon != 4) {
            if (found == index) {
                return &CGGossipInfo::m_quests[i];
            }

            found++;
        }
    }

    return nullptr;
}

// VERIFY: 16fb1584 manual
// OFFSET: 0x0058A6C0
int32_t CGGossipInfo::GetNumActiveQuests() {
    int32_t count = 0;

    for (uint32_t i = 0; i < 32; i++) {
        if (!CGGossipInfo::m_quests[i].m_questID) {
            break;
        }

        if (CGGossipInfo::m_quests[i].m_icon == 3 || CGGossipInfo::m_quests[i].m_icon == 4) {
            count++;
        }
    }

    return count;
}

// VERIFY: d3d99b53 auto
// OFFSET: 0x0058A750
GossipQuestInfo* CGGossipInfo::GetActiveQuest(int32_t index) {
    int32_t found = 0;

    for (uint32_t i = 0; i < 32; i++) {
        if (!CGGossipInfo::m_quests[i].m_questID) {
            return nullptr;
        }

        if (CGGossipInfo::m_quests[i].m_icon == 3 || CGGossipInfo::m_quests[i].m_icon == 4) {
            if (found == index) {
                return &CGGossipInfo::m_quests[i];
            }

            found++;
        }
    }

    return nullptr;
}

// VERIFY: 19587bce manual
// OFFSET: 0x0058ABA0
int32_t CGQuestInfo::IsLowLevel(int32_t level) {
    if (level == -1) {
        return 0;
    }

    uint32_t band = static_cast<int32_t>(static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__))->m_unit->m_unitFieldLevel) / 5;
    int32_t range = band < 20 ? s_questGreenRange[band] : 8;
    return level + range < static_cast<int32_t>(static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__))->m_unit->m_unitFieldLevel);
}

// VERIFY: fd70ce48 manual
// OFFSET: 0x0058AF10
void CGGossipInfo::SelectGossipOption(int32_t index, const char* code, int32_t confirmed) {
    if (CGGossipInfo::m_textPending || index < 0 || index >= 32) {
        return;
    }

    GossipOption& option = CGGossipInfo::m_options[index];

    if (option.m_index == -1) {
        return;
    }

    if (option.m_boxText && !confirmed) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GOSSIP_CONFIRM, "%d%s%d", index + 1, option.m_boxText, option.m_boxMoney);
        return;
    }

    if ((option.m_coded & 1) && (!code || !*code)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_GOSSIP_ENTER_CODE, "%d", index + 1);
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && option.m_boxMoney > player->GetCoinage()) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GOSSIP_SELECT_OPTION));
    msg.Put(CGGossipInfo::m_gossip.m_guid);
    msg.Put(CGGossipInfo::m_gossipID);
    msg.Put(static_cast<uint32_t>(option.m_index));

    if (option.m_coded & 1) {
        msg.PutString(code);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGossipInfo::ClearOptions();
    FrameScript_SignalEvent(SCRIPT_EVENT_GOSSIP_CONFIRM_CANCEL, nullptr);
}

// VERIFY: b9f3085b manual
// OFFSET: 0x0058B070
void CGGossipInfo::SelectGossipAvailableQuest(int32_t index) {
    int32_t found = 0;

    for (uint32_t i = 0; i < 32; i++) {
        GossipQuestInfo& quest = CGGossipInfo::m_quests[i];

        if (!quest.m_questID) {
            break;
        }

        if (quest.m_icon == 3 || quest.m_icon == 4) {
            continue;
        }

        if (found == index && ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
            WOWGUID gossip = CGGossipInfo::m_gossip;

            if (quest.m_icon) {
                CGPlayer_C::QueryQuest(gossip, quest.m_questID);
            } else {
                CGPlayer_C::CompleteQuest(gossip, quest.m_questID);
            }
        }

        found++;
    }
}

// VERIFY: c971dae1 manual
// OFFSET: 0x0058B120
void CGGossipInfo::SelectGossipActiveQuest(int32_t index) {
    int32_t found = 0;

    for (uint32_t i = 0; i < 32; i++) {
        GossipQuestInfo& quest = CGGossipInfo::m_quests[i];

        if (!quest.m_questID) {
            break;
        }

        if (quest.m_icon != 3 && quest.m_icon != 4) {
            continue;
        }

        if (found == index && ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
            WOWGUID gossip = CGGossipInfo::m_gossip;
            CGPlayer_C::CompleteQuest(gossip, quest.m_questID);
        }

        found++;
    }
}

// VERIFY: adc27a92 auto
// OFFSET: 0x0058A520
void CGGossipInfo::ShutdownGame() {
    ClientServices::ClearMessageHandler(SMSG_GOSSIP_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_GOSSIP_COMPLETE);
    ClientServices::ClearMessageHandler(SMSG_GOSSIP_POI);
}

// VERIFY: afffe065 auto
// OFFSET: 0x0058A840
int32_t Packet_SMSG_GOSSIP_COMPLETE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGossipInfo::SetGossip(WOWGUID());
    return 1;
}

// VERIFY: 06b6b0c5 auto
// OFFSET: 0x0058A870
int32_t Packet_SMSG_GOSSIP_POI(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t flags;
    float x;
    float y;
    uint32_t icon;
    uint32_t importance;
    char name[64];
    msg->Get(flags);
    msg->Get(x);
    msg->Get(y);
    msg->Get(icon);
    msg->Get(importance);
    msg->GetString(name, sizeof(name));
    MinimapSetDynamicPOI(1, flags, x, y, icon, importance, name);
    return 1;
}

// VERIFY: f5f2eac6 auto
// OFFSET: 0x0058AB20
void CGGossipInfo::LeaveWorld() {
    CGGossipInfo::SetGossip(WOWGUID{ 0 });
    CGGossipInfo::ClearOptions();
}
