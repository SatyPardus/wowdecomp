#include "LootRoll.h"
#include <cstring>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"

// STATIC: 0x00ADA7B4
TSExplicitList<PendingLootRoll, offsetof(PendingLootRoll, m_link)> s_pendingLootRolls;

// STATIC: 0x00CA0550
WOWGUID s_masterLootCandidates[40];

// STATIC: 0x00CA0690
static int32_t s_nextLootRollId;

// STATIC: 0x00CA0694
uint32_t s_numShownLootRolls;

// VERIFY: 291d5e6d auto
// OFFSET: 0x006FA770
WOWGUID GetMasterLootCandidate(uint32_t index) {
    if (index >= sizeof(s_masterLootCandidates) / sizeof(s_masterLootCandidates[0])) {
        return WOWGUID();
    }

    return s_masterLootCandidates[index];
}

// VERIFY: d35c8b17 manual
// OFFSET: 0x006FB150
PendingLootRoll* FindRoll(int32_t id) {
    for (PendingLootRoll* roll = s_pendingLootRolls.Head(); roll; roll = roll->Next()) {
        if (roll->m_id == id) {
            return roll;
        }
    }

    return nullptr;
}

// VERIFY: e08c1b05 manual
// OFFSET: 0x006FB180
void LootRollShowPending() {
    if (s_numShownLootRolls >= 4) {
        return;
    }

    for (PendingLootRoll* roll = s_pendingLootRolls.Head(); roll; roll = roll->Next()) {
        if (!roll->m_showPending) {
            continue;
        }

        s_numShownLootRolls++;
        roll->m_showPending = 0;
        FrameScript_SignalEvent(SCRIPT_EVENT_START_LOOT_ROLL, "%d%d", roll->m_id, roll->m_rollTime);

        if (s_numShownLootRolls >= 4) {
            break;
        }
    }
}

// VERIFY: 4f4e9712 manual
// OFFSET: 0x006FBC70
void LootRoll(int32_t id, int32_t rollType, bool confirmed) {
    for (PendingLootRoll* roll = s_pendingLootRolls.Head(); roll; roll = s_pendingLootRolls.Next(roll)) {
        if (roll->m_id != id) {
            continue;
        }

        if (roll->m_finished) {
            return;
        }

        if (rollType && !confirmed) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, none, nullptr, nullptr, false);

            if (stats) {
                if (rollType == 3) {
                    if (stats->m_quality >= 3) {
                        FrameScript_SignalEvent(SCRIPT_EVENT_CONFIRM_DISENCHANT_ROLL, "%d%d", roll->m_id, 3);
                        return;
                    }
                } else if (stats->m_bonding == 1) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_CONFIRM_LOOT_ROLL, "%d%d", roll->m_id, rollType);
                    return;
                }
            }
        }

        if ((1 << rollType) & roll->m_voteMask) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_LOOT_ROLL));
            msg.Put(roll->m_lootGuid.m_guid);
            msg.Put(roll->m_slot);
            msg.Put(static_cast<uint8_t>(rollType));
            msg.Finalize();
            ClientServices::Send2(&msg);
            roll->unused_0030 = 1;
            FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_LOOT_ROLL, "%d", roll->m_id);
            s_numShownLootRolls--;
            roll->m_finished = 1;
            LootRollShowPending();
        }

        return;
    }
}

// VERIFY: f5032c00 manual
// OFFSET: 0x006FAF40
void LootRollDestroy() {
    PendingLootRoll* roll;

    while ((roll = s_pendingLootRolls.Head())) {
        s_pendingLootRolls.DeleteNode(roll);
    }
}

// VERIFY: ab7c1274 auto
// OFFSET: 0x006FAFB0
void LootRollShutdown() {
    LootRollDestroy();
    s_numShownLootRolls = 0;
}

// VERIFY: 59202aa4 manual
// OFFSET: 0x006FAFD0
void CloseAllLootRolls() {
    PendingLootRoll* roll = s_pendingLootRolls.Head();

    while (roll) {
        PendingLootRoll* next = s_pendingLootRolls.Next(roll);

        if (!roll->m_finished) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_LOOT_ROLL));
            msg.Put(roll->m_lootGuid.m_guid);
            msg.Put(roll->m_slot);
            msg.Put(static_cast<uint8_t>(0));
            msg.Finalize();
            ClientServices::Send2(&msg);

            if (!roll->m_showPending) {
                FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_LOOT_ROLL, "%d", roll->m_id);
            }
        }

        roll = next;
    }

    LootRollDestroy();
    s_numShownLootRolls = 0;
}

// VERIFY: f5e08283 auto
// OFFSET: 0x006FA690
int32_t LootRollMasterList(CDataStore* msg) {
    memset(s_masterLootCandidates, 0, sizeof(s_masterLootCandidates));
    uint8_t count;
    msg->Get(count);
    bool raid = CGRaidInfo::m_numMembers != 0;

    for (uint8_t i = 0; i < count; i++) {
        WOWGUID guid;
        msg->Get(guid.m_guid);

        if (!raid) {
            s_masterLootCandidates[i] = guid;
            continue;
        }

        CGRaidMember* member = CGRaidInfo::FindMemberByGuid(guid);

        if (!member) {
            continue;
        }

        uint32_t first = 5 * member->m_subgroup;
        uint32_t last = first + 5;

        for (uint32_t j = first; j < last && j < 40; j++) {
            if (!s_masterLootCandidates[j]) {
                s_masterLootCandidates[j] = guid;
                break;
            }
        }
    }

    return 1;
}

// VERIFY: 62fad677 manual
// OFFSET: 0x006FA7A0
static void LootRollCalculateReasons(PendingLootRoll* roll, const ItemStats_C* stats) {
    roll->unused_0051 = 0;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    bool invalid = !player->m_bag.ItemCountValid(roll->m_itemId, stats, roll->m_count, false);
    uint8_t mask = roll->m_voteMask;

    if (mask & 0x2) {
        roll->m_reasonNeed = 0;
    } else if (invalid) {
        roll->m_reasonNeed = 2;
    } else {
        roll->m_reasonNeed = static_cast<uint8_t>(((stats->m_flags[1] >> 6) & 0x4) | 0x1);
    }

    roll->m_reasonGreed = ~(mask >> 1) & 0x2;

    if (mask & 0x8) {
        roll->m_reasonDisenchant = 0;
    } else {
        roll->m_reasonDisenchant = (static_cast<int32_t>(stats->m_requiredDisenchantSkill) >= 0) + 3;
    }
}

// VERIFY: 8b12827a manual
// OFFSET: 0x006FA840
static void RolledStatsAndName(const NameCache* name, PendingLootRoll* roll, const ItemStats_C* stats) {
    if (!g_showLootSpamCVar->m_intValue && (roll->m_rollerGuid != ClntObjMgrGetActivePlayer() || !roll->m_autoPass)) {
        delete roll;
        return;
    }

    char text[3000];

    if (roll->m_rollerGuid == ClntObjMgrGetActivePlayer()) {
        if (roll->m_rollType == 2 && roll->m_rollNumber < 0) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_GREED_SELF", -1, GENDER_NOT_APPLICABLE), CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 3 && roll->m_rollNumber < 0) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_DISENCHANT_SELF", -1, GENDER_NOT_APPLICABLE), CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType != 3 && roll->m_rollNumber < 0) {
            const char* format = roll->m_autoPass ? FrameScript_GetText("LOOT_ROLL_PASSED_SELF_AUTO", -1, GENDER_NOT_APPLICABLE) : FrameScript_GetText("LOOT_ROLL_PASSED_SELF", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(text, sizeof(text), format, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (!roll->m_rollType) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_NEED_SELF", -1, GENDER_NOT_APPLICABLE), CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 2) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_GREED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        } else if (roll->m_rollType == 3) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_DE", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        } else {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_NEED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        }
    } else {
        if (roll->m_rollType == 2 && roll->m_rollNumber < 0) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_GREED", -1, GENDER_NOT_APPLICABLE), name->m_name, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 3 && roll->m_rollNumber < 0) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_DISENCHANT", -1, GENDER_NOT_APPLICABLE), name->m_name, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType != 3 && roll->m_rollNumber < 0) {
            const char* format = roll->m_autoPass ? FrameScript_GetText("LOOT_ROLL_PASSED_AUTO", -1, static_cast<FRAMESCRIPT_GENDER>((name->m_sex != 0) + 2)) : FrameScript_GetText("LOOT_ROLL_PASSED", -1, GENDER_NOT_APPLICABLE);
            SStrPrintf(text, sizeof(text), format, name->m_name, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (!roll->m_rollType) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_NEED", -1, GENDER_NOT_APPLICABLE), name->m_name, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 2) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_GREED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        } else if (roll->m_rollType == 3) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_DE", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        } else {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ROLLED_NEED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix), name->m_name);
        }
    }

    CGChat::AddChatMessage(text, 27, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    delete roll;
}

// VERIFY: c30cf867 manual
// OFFSET: 0x006FACD0
static void RolledNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);
    WOWGUID none2 = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, none2, nullptr, nullptr, false);
    RolledStatsAndName(name, roll, stats);
}

// VERIFY: e9349118 manual
// OFFSET: 0x006FAD80
static void RolledCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    NameCache* name = g_nameDBCache.GetRecord(roll->m_rollerGuid.m_guid, roll->m_rollerGuid, &RolledNameCallback, roll, false);

    if (name) {
        RolledStatsAndName(name, roll, stats);
    }
}

// VERIFY: da030717 manual
// OFFSET: 0x006FAE40
int32_t LootRollRolled(CDataStore* msg) {
    PendingLootRoll* roll = new (__FILE__, __LINE__) PendingLootRoll;
    msg->Get(roll->m_lootGuid.m_guid);
    msg->Get(roll->m_slot);
    msg->Get(roll->m_rollerGuid.m_guid);
    msg->Get(roll->m_itemId);
    msg->Get(roll->m_randomSuffix);
    msg->Get(roll->m_randomPropertyId);
    uint8_t value = 0;
    msg->Get(value);
    roll->m_rollNumber = value;
    value = 0;
    msg->Get(value);
    roll->m_rollType = value;
    value = 0;
    msg->Get(value);
    roll->m_autoPass = value != 0;
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, roll->m_lootGuid, &RolledCallback, roll, false);

    if (stats) {
        NameCache* name = g_nameDBCache.GetRecord(roll->m_rollerGuid.m_guid, roll->m_rollerGuid, &RolledNameCallback, roll, false);

        if (name) {
            RolledStatsAndName(name, roll, stats);
        }
    }

    return 1;
}

// VERIFY: 38b54e2e manual
// OFFSET: 0x006FB100
static PendingLootRoll* FindRollBySlot(WOWGUID lootGuid, uint32_t slot) {
    for (PendingLootRoll* roll = s_pendingLootRolls.Head(); roll; roll = roll->Next()) {
        if (roll->m_lootGuid == lootGuid && roll->m_slot == slot) {
            return roll;
        }
    }

    return nullptr;
}

// VERIFY: 6f123543 manual
// OFFSET: 0x006FB1F0
static void StartCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    LootRollCalculateReasons(roll, stats);

    if (s_numShownLootRolls >= 4) {
        roll->m_showPending = 1;
    } else {
        s_numShownLootRolls++;
        FrameScript_SignalEvent(SCRIPT_EVENT_START_LOOT_ROLL, "%d%d", roll->m_id, roll->m_rollTime);
    }
}

// VERIFY: 07f063f9 manual
// OFFSET: 0x006FB2C0
int32_t LootRollStart(CDataStore* msg) {
    WOWGUID lootGuid;
    int32_t mapId;
    msg->Get(lootGuid.m_guid);
    msg->Get(reinterpret_cast<uint32_t&>(mapId));

    if (mapId != CGGameUI::m_iCurrentMapID) {
        msg->m_read = msg->m_size;
        return 1;
    }

    PendingLootRoll* roll = s_pendingLootRolls.NewNode(2, 0, 0);
    roll->m_lootGuid = lootGuid;
    roll->m_id = s_nextLootRollId++;
    roll->unused_0030 = 0;
    roll->m_showPending = 0;
    roll->m_finished = 0;
    msg->Get(roll->m_slot);
    msg->Get(roll->m_itemId);
    msg->Get(roll->m_randomSuffix);
    msg->Get(roll->m_randomPropertyId);
    msg->Get(roll->m_count);
    msg->Get(roll->m_rollTime);
    msg->Get(roll->m_voteMask);
    roll->m_expireTime = FrameTime::s_curTimeMs + roll->m_rollTime;
    roll->m_rollType = 0;
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, roll->m_lootGuid, &StartCallback, roll, false);

    if (stats) {
        LootRollCalculateReasons(roll, stats);

        if (s_numShownLootRolls < 4) {
            s_numShownLootRolls++;
            FrameScript_SignalEvent(SCRIPT_EVENT_START_LOOT_ROLL, "%d%d", roll->m_id, roll->m_rollTime);
            return 1;
        }

        roll->m_showPending = 1;
    }

    return 1;
}

// VERIFY: be15e21c auto
// OFFSET: 0x006FB470
static void AllPassedStats(PendingLootRoll* roll, const ItemStats_C* stats) {
    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_ALL_PASSED", -1, GENDER_NOT_APPLICABLE), CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    CGChat::AddChatMessage(text, 27, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);

    if (!roll->m_finished) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_LOOT_ROLL, "%d", roll->m_id);
        s_numShownLootRolls--;
    }

    delete roll;
    LootRollShowPending();
}

// VERIFY: dfebe358 manual
// OFFSET: 0x006FB560
static void PassedCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    AllPassedStats(roll, stats);
}

// VERIFY: 33641577 auto
// OFFSET: 0x006FB600
int32_t LootRollAllPassed(CDataStore* msg) {
    WOWGUID lootGuid;
    uint32_t slot;
    uint32_t itemId;
    uint32_t randomPropertyId;
    uint32_t randomSuffix;
    msg->Get(lootGuid.m_guid);
    msg->Get(slot);
    msg->Get(itemId);
    msg->Get(randomPropertyId);
    msg->Get(randomSuffix);
    PendingLootRoll* roll = FindRollBySlot(lootGuid, slot);

    if (!roll) {
        roll = new (__FILE__, __LINE__) PendingLootRoll;
        roll->m_lootGuid = lootGuid;
        roll->m_slot = slot;
        roll->m_itemId = itemId;
        roll->m_id = -1;
        roll->m_randomPropertyId = randomPropertyId;
        roll->m_randomSuffix = randomSuffix;
        roll->m_finished = 1;
        roll->m_rollType = 0;
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, lootGuid, &PassedCallback, roll, false);

    if (stats) {
        AllPassedStats(roll, stats);
    }

    CGPlayer_C::ClearLootItemSlotType(lootGuid, slot);
    return 1;
}

// VERIFY: 46eb3054 auto
// OFFSET: 0x006FB700
static void WonStatsAndName(const NameCache* name, PendingLootRoll* roll, const ItemStats_C* stats) {
    char text[3000];

    if (roll->m_rollerGuid == ClntObjMgrGetActivePlayer()) {
        if (g_showLootSpamCVar->m_intValue) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_YOU_WON", -1, GENDER_NOT_APPLICABLE), CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 1) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_YOU_WON_NO_SPAM_NEED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 2) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_YOU_WON_NO_SPAM_GREED", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        } else if (roll->m_rollType == 3) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_YOU_WON_NO_SPAM_DE", -1, GENDER_NOT_APPLICABLE), roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
        }
    } else if (g_showLootSpamCVar->m_intValue) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_WON", -1, GENDER_NOT_APPLICABLE), name->m_name, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    } else if (roll->m_rollType == 1) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_WON_NO_SPAM_NEED", -1, GENDER_NOT_APPLICABLE), name->m_name, roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    } else if (roll->m_rollType == 2) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_WON_NO_SPAM_GREED", -1, GENDER_NOT_APPLICABLE), name->m_name, roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    } else if (roll->m_rollType == 3) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("LOOT_ROLL_WON_NO_SPAM_DE", -1, GENDER_NOT_APPLICABLE), name->m_name, roll->m_rollNumber, CGTooltip::GetItemHyperlinkString(roll->m_itemId, stats->m_quality, 0, roll->m_randomPropertyId, roll->m_randomSuffix));
    }

    CGChat::AddChatMessage(text, 27, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);

    if (!roll->m_finished) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CANCEL_LOOT_ROLL, "%d", roll->m_id);
        s_numShownLootRolls--;
    }

    delete roll;
    LootRollShowPending();
}

// VERIFY: 0c781528 manual
// OFFSET: 0x006FB9B0
static void WonNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);
    WOWGUID none2 = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(roll->m_itemId, none2, nullptr, nullptr, false);
    WonStatsAndName(name, roll, stats);
}

// VERIFY: a0045dcb manual
// OFFSET: 0x006FBA60
static void WonCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PendingLootRoll* roll = static_cast<PendingLootRoll*>(param);

    if (!success) {
        delete roll;
        return;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(id, none, nullptr, nullptr, false);
    NameCache* name = g_nameDBCache.GetRecord(roll->m_rollerGuid.m_guid, roll->m_rollerGuid, &WonNameCallback, roll, false);

    if (name) {
        WonStatsAndName(name, roll, stats);
    }
}

// VERIFY: d2b7d1d9 auto
// OFFSET: 0x006FBB10
int32_t LootRollWon(CDataStore* msg) {
    WOWGUID lootGuid;
    uint32_t slot;
    uint32_t itemId;
    uint32_t randomSuffix;
    uint32_t randomPropertyId;
    WOWGUID winner;
    uint8_t rollNumber;
    uint8_t rollType;
    msg->Get(lootGuid.m_guid);
    msg->Get(slot);
    msg->Get(itemId);
    msg->Get(randomSuffix);
    msg->Get(randomPropertyId);
    msg->Get(winner.m_guid);
    msg->Get(rollNumber);
    msg->Get(rollType);
    PendingLootRoll* roll = FindRollBySlot(lootGuid, slot);

    if (!roll) {
        roll = new (__FILE__, __LINE__) PendingLootRoll;
        roll->m_lootGuid = lootGuid;
        roll->m_slot = slot;
        roll->m_itemId = itemId;
        roll->m_id = -1;
        roll->m_randomPropertyId = randomPropertyId;
        roll->m_randomSuffix = randomSuffix;
        roll->m_finished = 1;
        roll->m_rollType = 0;
    }

    roll->m_rollNumber = rollNumber;
    roll->m_rollType = rollType;
    roll->m_rollerGuid = winner;
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, lootGuid, &WonCallback, roll, false);

    if (stats) {
        NameCache* name = g_nameDBCache.GetRecord(roll->m_rollerGuid.m_guid, roll->m_rollerGuid, &WonNameCallback, roll, false);

        if (name) {
            WonStatsAndName(name, roll, stats);
        }
    }

    CGPlayer_C::ClearLootItemSlotType(lootGuid, slot);
    return 1;
}
