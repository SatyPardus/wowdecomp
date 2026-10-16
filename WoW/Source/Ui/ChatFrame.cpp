#include "ChatFrame.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <malloc.h>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/pcre/pcreposix.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChatChannelsRec.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/CKBSystem.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/UILCD/LCD.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/DB/Rec/CharTitlesRec.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/DB/DBClient/GameObjectStats_C.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// STATIC: 0x00BCEFEC
int32_t CGChat::m_afk;

// STATIC: 0x00BCF004
int32_t CGChat::m_voiceChatSystemStatus;

// STATIC: 0x00B4AE40
int32_t CGChat::m_paused;

// STATIC: 0x009FA558
const ChatColorInfo s_chatTypeInfo[CHAT_MSG_MAX] = {
    { "SYSTEM", { 0xFF, 0xFF, 0x00, 0x00 }, 0, 0 },
    { "SAY", { 0xFF, 0xFF, 0xFF, 0x00 }, 0, 0 },
    { "PARTY", { 0xAA, 0xAA, 0xFF, 0x00 }, 0, 0 },
    { "RAID", { 0xFF, 0x7F, 0x00, 0x00 }, 0, 0 },
    { "GUILD", { 0x40, 0xFF, 0x40, 0x00 }, 0, 0 },
    { "OFFICER", { 0x40, 0xC0, 0x40, 0x00 }, 0, 0 },
    { "YELL", { 0xFF, 0x40, 0x40, 0x00 }, 0, 0 },
    { "WHISPER", { 0xFF, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "WHISPER_FOREIGN", { 0xFF, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "WHISPER_INFORM", { 0xFF, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "EMOTE", { 0xFF, 0x80, 0x40, 0x00 }, 0, 0 },
    { "TEXT_EMOTE", { 0xFF, 0x80, 0x40, 0x00 }, 0, 0 },
    { "MONSTER_SAY", { 0xFF, 0xFF, 0x9F, 0x00 }, 0, 0 },
    { "MONSTER_PARTY", { 0xAA, 0xAA, 0xFF, 0x00 }, 3, 0 },
    { "MONSTER_YELL", { 0xFF, 0x40, 0x40, 0x00 }, 0, 0 },
    { "MONSTER_WHISPER", { 0xFF, 0xB5, 0xEB, 0x00 }, 3, 0 },
    { "MONSTER_EMOTE", { 0xFF, 0x80, 0x40, 0x00 }, 0, 0 },
    { "CHANNEL", { 0xFF, 0xC0, 0xC0, 0x00 }, 0, 0 },
    { "CHANNEL_JOIN", { 0xC0, 0x80, 0x80, 0x00 }, 0, 0 },
    { "CHANNEL_LEAVE", { 0xC0, 0x80, 0x80, 0x00 }, 0, 0 },
    { "CHANNEL_LIST", { 0xC0, 0x80, 0x80, 0x00 }, 0, 0 },
    { "CHANNEL_NOTICE", { 0xC0, 0xC0, 0xC0, 0x00 }, 0, 0 },
    { "CHANNEL_NOTICE_USER", { 0xC0, 0xC0, 0xC0, 0x00 }, 0, 0 },
    { "AFK", { 0xFF, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "DND", { 0xFF, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "IGNORED", { 0xFF, 0x00, 0x00, 0x00 }, 0, 0 },
    { "SKILL", { 0x55, 0x55, 0xFF, 0x00 }, 0, 0 },
    { "LOOT", { 0x00, 0xAA, 0x00, 0x00 }, 0, 0 },
    { "MONEY", { 0xFF, 0xFF, 0x00, 0x00 }, 0, 0 },
    { "OPENING", { 0x80, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "TRADESKILLS", { 0xFF, 0xFF, 0xFF, 0x00 }, 0, 0 },
    { "PET_INFO", { 0x80, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "COMBAT_MISC_INFO", { 0x80, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "COMBAT_XP_GAIN", { 0x6F, 0x6F, 0xFF, 0x00 }, 0, 0 },
    { "COMBAT_HONOR_GAIN", { 0xE0, 0xCA, 0x0A, 0x00 }, 0, 0 },
    { "COMBAT_FACTION_CHANGE", { 0x80, 0x80, 0xFF, 0x00 }, 0, 0 },
    { "BG_SYSTEM_NEUTRAL", { 0xFF, 0x78, 0x0A, 0x00 }, 0, 0 },
    { "BG_SYSTEM_ALLIANCE", { 0x00, 0xAE, 0xEF, 0x00 }, 0, 0 },
    { "BG_SYSTEM_HORDE", { 0xFF, 0x00, 0x00, 0x00 }, 0, 0 },
    { "RAID_LEADER", { 0xFF, 0x48, 0x09, 0x00 }, 0, 0 },
    { "RAID_WARNING", { 0xFF, 0x48, 0x00, 0x00 }, 0, 0 },
    { "RAID_BOSS_EMOTE", { 0xFF, 0xDD, 0x00, 0x00 }, 0, 0 },
    { "RAID_BOSS_WHISPER", { 0xFF, 0xDD, 0x00, 0x00 }, 0, 0 },
    { "FILTERED", { 0xFF, 0x00, 0x00, 0x00 }, 0, 0 },
    { "BATTLEGROUND", { 0xFF, 0x7F, 0x00, 0x00 }, 0, 0 },
    { "BATTLEGROUND_LEADER", { 0xFF, 0xDB, 0xB7, 0x00 }, 0, 0 },
    { "RESTRICTED", { 0xFF, 0x00, 0x00, 0x00 }, 0, 0 },
    { "BATTLENET", { 0xFF, 0xFF, 0xFF, 0x00 }, 0, 0 },
    { "ACHIEVEMENT", { 0xFF, 0xFF, 0x00, 0x00 }, 9, 0 },
    { "GUILD_ACHIEVEMENT", { 0x40, 0xFF, 0x40, 0x00 }, 0, 0 },
    { "ARENA_POINTS", { 0xFF, 0xFF, 0xFF, 0x00 }, 0, 0 },
    { "PARTY_LEADER", { 0x76, 0xC8, 0xFF, 0x00 }, 10, 0 },
    { "TARGETICONS", { 0xFF, 0xFF, 0x00, 0x00 }, 11, 0 },
    { "BN_WHISPER", { 0x00, 0xFF, 0xF6, 0x00 }, 12, 0 },
    { "BN_WHISPER_INFORM", { 0x00, 0xFF, 0xF6, 0x00 }, 12, 0 },
    { "BN_CONVERSATION", { 0x00, 0xB1, 0xF0, 0x00 }, 12, 0 },
    { "BN_CONVERSATION_NOTICE", { 0x00, 0xB1, 0xF0, 0x00 }, 12, 0 },
    { "BN_CONVERSATION_LIST", { 0x00, 0xB1, 0xF0, 0x00 }, 12, 0 },
    { "BN_INLINE_TOAST_ALERT", { 0x82, 0xC5, 0xFF, 0x00 }, 13, 0 },
    { "BN_INLINE_TOAST_BROADCAST", { 0x82, 0xC5, 0xFF, 0x00 }, 13, 0 },
    { "BN_INLINE_TOAST_BROADCAST_INFORM", { 0x82, 0xC5, 0xFF, 0x00 }, 13, 0 },
    { "BN_INLINE_TOAST_CONVERSATION", { 0x82, 0xC5, 0xFF, 0x00 }, 13, 0 },
};

// STATIC: 0x009FBE80
static const uint32_t s_chatEvents[CHAT_MSG_MAX] = {
    SCRIPT_EVENT_CHAT_MSG_SYSTEM,
    SCRIPT_EVENT_CHAT_MSG_SAY,
    SCRIPT_EVENT_CHAT_MSG_PARTY,
    SCRIPT_EVENT_CHAT_MSG_RAID,
    SCRIPT_EVENT_CHAT_MSG_GUILD,
    SCRIPT_EVENT_CHAT_MSG_OFFICER,
    SCRIPT_EVENT_CHAT_MSG_YELL,
    SCRIPT_EVENT_CHAT_MSG_WHISPER,
    SCRIPT_EVENT_CHAT_MSG_WHISPER,
    SCRIPT_EVENT_CHAT_MSG_WHISPER_INFORM,
    SCRIPT_EVENT_CHAT_MSG_EMOTE,
    SCRIPT_EVENT_CHAT_MSG_TEXT_EMOTE,
    SCRIPT_EVENT_CHAT_MSG_MONSTER_SAY,
    SCRIPT_EVENT_CHAT_MSG_MONSTER_PARTY,
    SCRIPT_EVENT_CHAT_MSG_MONSTER_YELL,
    SCRIPT_EVENT_CHAT_MSG_MONSTER_WHISPER,
    SCRIPT_EVENT_CHAT_MSG_MONSTER_EMOTE,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL_JOIN,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL_LEAVE,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL_LIST,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL_NOTICE,
    SCRIPT_EVENT_CHAT_MSG_CHANNEL_NOTICE_USER,
    SCRIPT_EVENT_CHAT_MSG_AFK,
    SCRIPT_EVENT_CHAT_MSG_DND,
    SCRIPT_EVENT_CHAT_MSG_IGNORED,
    SCRIPT_EVENT_CHAT_MSG_SKILL,
    SCRIPT_EVENT_CHAT_MSG_LOOT,
    SCRIPT_EVENT_CHAT_MSG_MONEY,
    SCRIPT_EVENT_CHAT_MSG_OPENING,
    SCRIPT_EVENT_CHAT_MSG_TRADESKILLS,
    SCRIPT_EVENT_CHAT_MSG_PET_INFO,
    SCRIPT_EVENT_CHAT_MSG_COMBAT_MISC_INFO,
    SCRIPT_EVENT_CHAT_MSG_COMBAT_XP_GAIN,
    SCRIPT_EVENT_CHAT_MSG_COMBAT_HONOR_GAIN,
    SCRIPT_EVENT_CHAT_MSG_COMBAT_FACTION_CHANGE,
    SCRIPT_EVENT_CHAT_MSG_BG_SYSTEM_NEUTRAL,
    SCRIPT_EVENT_CHAT_MSG_BG_SYSTEM_ALLIANCE,
    SCRIPT_EVENT_CHAT_MSG_BG_SYSTEM_HORDE,
    SCRIPT_EVENT_CHAT_MSG_RAID_LEADER,
    SCRIPT_EVENT_CHAT_MSG_RAID_WARNING,
    SCRIPT_EVENT_CHAT_MSG_RAID_BOSS_EMOTE,
    SCRIPT_EVENT_CHAT_MSG_RAID_BOSS_WHISPER,
    SCRIPT_EVENT_CHAT_MSG_FILTERED,
    SCRIPT_EVENT_CHAT_MSG_BATTLEGROUND,
    SCRIPT_EVENT_CHAT_MSG_BATTLEGROUND_LEADER,
    SCRIPT_EVENT_CHAT_MSG_RESTRICTED,
    SCRIPT_EVENT_UNNAMED_237,
    SCRIPT_EVENT_CHAT_MSG_ACHIEVEMENT,
    SCRIPT_EVENT_CHAT_MSG_GUILD_ACHIEVEMENT,
    658,
    SCRIPT_EVENT_CHAT_MSG_PARTY_LEADER,
    SCRIPT_EVENT_CHAT_MSG_TARGETICONS,
    SCRIPT_EVENT_CHAT_MSG_BN_WHISPER,
    SCRIPT_EVENT_CHAT_MSG_BN_WHISPER_INFORM,
    SCRIPT_EVENT_CHAT_MSG_BN_CONVERSATION,
    SCRIPT_EVENT_CHAT_MSG_BN_CONVERSATION_NOTICE,
    SCRIPT_EVENT_CHAT_MSG_BN_CONVERSATION_LIST,
    SCRIPT_EVENT_CHAT_MSG_BN_INLINE_TOAST_ALERT,
    SCRIPT_EVENT_CHAT_MSG_BN_INLINE_TOAST_BROADCAST,
    SCRIPT_EVENT_CHAT_MSG_BN_INLINE_TOAST_BROADCAST_INFORM,
    SCRIPT_EVENT_CHAT_MSG_BN_INLINE_TOAST_CONVERSATION,
};

// STATIC: 0x009FA510
static const int32_t s_profanityFilterTypes[18] = {
    CHAT_MSG_SAY,
    CHAT_MSG_PARTY,
    CHAT_MSG_PARTY_LEADER,
    CHAT_MSG_RAID,
    CHAT_MSG_RAID_LEADER,
    CHAT_MSG_GUILD,
    CHAT_MSG_OFFICER,
    CHAT_MSG_YELL,
    CHAT_MSG_WHISPER,
    CHAT_MSG_WHISPER_INFORM,
    CHAT_MSG_EMOTE,
    CHAT_MSG_CHANNEL,
    CHAT_MSG_AFK,
    CHAT_MSG_DND,
    CHAT_MSG_RAID_WARNING,
    CHAT_MSG_BATTLEGROUND,
    CHAT_MSG_BATTLEGROUND_LEADER,
    CHAT_MSG_SYSTEM,
};

// STATIC: 0x009FC5B0
static const char s_profanityReplacement[] = "!@#$%^&*";

// STATIC: 0x00AC7A38
static int32_t s_queueChat = 1;

// STATIC: 0x00AC7D30
TSExplicitList<THROTTLEDCHAT, 0> CGChat::s_throttledChat;

// STATIC: 0x00AC7D3C
TSExplicitList<PENDINGCHAT, 0> CGChat::s_pendingChat;

// STATIC: 0x00B759D0
int32_t s_chatLogging[2];

// STATIC: 0x00B759D8
HSLOG s_chatLog[2];

// STATIC: 0x00B75A60
ChatRecord CGChat::m_chatHistory[60];

// STATIC: 0x00BCEB60
WOWGUID CGChat::m_whisperDstHistory[128];

// STATIC: 0x00BCEF60
WOWGUID CGChat::m_whisperSrcHistory[16];

// STATIC: 0x00BCEFF4
uint32_t CGChat::m_currentChatIndex;

// STATIC: 0x00BCEFF8
static int32_t s_chatHistoryInitialized;

// STATIC: 0x00BCF000
static uint32_t s_chatLineID;

// STATIC: 0x00BCF038
static uint32_t s_whisperThrottleIndex;

// STATIC: 0x00BCF03C
static uint32_t s_whisperThrottleTimes[10];

// STATIC: 0x00BCF064
static TSHashTable<CHATTHROTTLE, HASHKEY_STR> s_channelThrottle;

// STATIC: 0x00BCF08C
TSGrowableArray<ChatChannel> CGChat::s_channels;

// STATIC: 0x00BCFB18
static TSHashTable<PROFANITYCHECK, HASHKEY_STR> s_profanityCache;

// STATIC: 0x00BCFB40
static TSHashTable<SPAMCHECK, HASHKEY_STR> s_spamCache;

// STATIC: 0x00BCFB84
static uint32_t s_profanityReplacementIndex;

// STATIC: 0x00D37E94
static TSFixedArray<regex_t> s_expectedSpamRecords;

// STATIC: 0x009FB7C0
const ChatMessageGroup s_chatMessageGroups[46] = {
    { "SYSTEM", true, 0 },
    { "SYSTEM_NOMENU", true, 5 },
    { "SAY", true, 0 },
    { "EMOTE", true, 7 },
    { "YELL", true, 0 },
    { "WHISPER", true, 0 },
    { "PARTY", true, 0 },
    { "PARTY_LEADER", true, 10 },
    { "RAID", true, 7 },
    { "RAID_LEADER", true, 7 },
    { "RAID_WARNING", true, 7 },
    { "BATTLEGROUND", true, 7 },
    { "BATTLEGROUND_LEADER", true, 7 },
    { "GUILD", true, 0 },
    { "OFFICER", true, 7 },
    { "MONSTER_SAY", true, 7 },
    { "MONSTER_YELL", true, 7 },
    { "MONSTER_EMOTE", true, 7 },
    { "MONSTER_WHISPER", true, 7 },
    { "MONSTER_BOSS_EMOTE", true, 7 },
    { "MONSTER_BOSS_WHISPER", true, 7 },
    { "ERRORS", true, 7 },
    { "AFK", true, 7 },
    { "DND", true, 7 },
    { "IGNORED", true, 7 },
    { "BG_HORDE", true, 7 },
    { "BG_ALLIANCE", true, 7 },
    { "BG_NEUTRAL", true, 7 },
    { "COMBAT_FACTION_CHANGE", true, 1 },
    { "SKILL", true, 0 },
    { "LOOT", true, 0 },
    { "MONEY", true, 2 },
    { "CHANNEL", true, 0 },
    { "ACHIEVEMENT", true, 8 },
    { "GUILD_ACHIEVEMENT", true, 8 },
    { "TARGETICONS", false, 11 },
    { "BN_WHISPER", true, 12 },
    { "BN_WHISPER_INFORM", true, 12 },
    { "BN_CONVERSATION", true, 12 },
    { "BN_INLINE_TOAST_ALERT", true, 13 },
    { "OPENING", true, 6 },
    { "TRADESKILLS", true, 6 },
    { "PET_INFO", true, 6 },
    { "COMBAT_XP_GAIN", true, 0 },
    { "COMBAT_HONOR_GAIN", true, 0 },
    { "COMBAT_MISC_INFO", true, 0 },
};

// STATIC: 0x00B743CC
uint32_t s_activeVoiceChannel;

// STATIC: 0x00B743D0
uint8_t s_collapsedHeaders[3];

// STATIC: 0x00B743D3
static bool s_pendingUserListActive;

// STATIC: 0x00B743D4
uint32_t s_activeDisplayChannelFlags;

// STATIC: 0x00B743D8
int32_t s_displayChannelMemberCount;

// STATIC: 0x00B743DC
int32_t s_activeDisplayChannelType;

// STATIC: 0x00B743E0
char CGChat::s_activeDisplayChannelName[128];

// STATIC: 0x00B74460
uint32_t s_numDisplayChannels;

// STATIC: 0x00B74464
static AreaTableRec* s_cityArea;

// STATIC: 0x00B74468
DisplayChannel CGChat::s_displayChannels[20];

// STATIC: 0x00B746E8
ChatColorInfo s_chatColors[CHAT_MSG_MAX];

// STATIC: 0x00B75950
static char s_inviteChannelName[128];

// STATIC: 0x00BCEFE4
int32_t CGChat::m_loaded;

// STATIC: 0x00BCEFE8
int32_t CGChat::m_registered;

// STATIC: 0x00BCEFFC
int32_t CGChat::m_complaintsSystemStatus;

// STATIC: 0x00BCF010
uint32_t s_joinedZoneChannels;

// STATIC: 0x00BCF014
uint32_t s_numSavedInstances;

// STATIC: 0x00BCF09C
TSGrowableArray<ChatColorInfo> s_channelColors;

// STATIC: 0x00BCF0B0
ChatWindowInfo s_chatWindows[10];

// STATIC: 0x00BCFAF0
static TSHashTable<WORDLIST, HASHKEY_LANGUAGE> s_wordLists;

// STATIC: 0x00BCFB68
TSGrowableArray<PENDINGUSERLIST> s_pendingUsers;

// STATIC: 0x00AC7D48
TSExplicitList<PENDINGTEXTEMOTE, 0> CGChat::s_pendingTextEmotes;

// STATIC: 0x00AC7A3C
int32_t CGChat::m_guildRecruitmentMode = 1;

// STATIC: 0x00BCEFF0
int32_t CGChat::m_areaID;

// STATIC: 0x00AC7A54
static const char* s_guildRecruitmentOption = "OPTION_GUILD_RECRUITMENT_CHANNEL";

// STATIC: 0x009FB9E8
static const ZoneChannelDefault s_zoneChannelDefaults[1] = { { 26, 4 } };

// STATIC: 0x00B759E0
static char s_recruitmentChannelName[128];

// STATIC: 0x00BCEFE0
static int32_t s_chatSettingsDirty;

// STATIC: 0x00BCF008
static bool s_recruitmentChannelFound;

// STATIC: 0x00BCF00C
static int32_t s_recruitmentChannelID;

// STATIC: 0x00BCFAB0
static TSGrowableArray<char*> s_pendingChannelNames;

// STATIC: 0x00BCFAC0
static TSGrowableArray<int32_t> s_pendingChannelFlags;

// STATIC: 0x00BCFAD0
static TSGrowableArray<int32_t> s_pendingChannelNumbers;

// STATIC: 0x00BCFAE0
TSGrowableArray<RaidInstanceInfo> s_savedInstances;

// VERIFY: 36ed0e4f auto
// OFFSET: 0x004FBAF0
int32_t CGChat::IsVoiceChatAllowed() {
    return CGChat::m_voiceChatSystemStatus && !ComSatClient_IsComsatDisabledByClient();
}

// VERIFY: 140e97d1 manual
// OFFSET: 0x004FB0F0
static int QSortDisplayListMembers(const void* a, const void* b) {
    if (!a || !b) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WOWGUID none = WOWGUID();
    NameCache* left = g_nameDBCache.GetRecord(static_cast<const PENDINGUSERLIST*>(a)->m_guid.m_guid, none, nullptr, nullptr, false);
    WOWGUID none2 = WOWGUID();
    NameCache* right = g_nameDBCache.GetRecord(static_cast<const PENDINGUSERLIST*>(b)->m_guid.m_guid, none2, nullptr, nullptr, false);

    if (!left) {
        return -1;
    }

    if (!right) {
        return 1;
    }

    if (!left->m_name[0]) {
        return -1;
    }

    return SStrSortUTF8I(left->m_name, right->m_name, STORM_MAX_STR);
}

// VERIFY: 22eb502f auto
// OFFSET: 0x004FB1A0
int32_t CGChat::IsRecordedType(int32_t type) {
    switch (type) {
        case CHAT_MSG_SAY:
        case CHAT_MSG_PARTY:
        case CHAT_MSG_RAID:
        case CHAT_MSG_GUILD:
        case CHAT_MSG_OFFICER:
        case CHAT_MSG_YELL:
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_WHISPER_INFORM:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_TEXT_EMOTE:
        case CHAT_MSG_CHANNEL:
        case CHAT_MSG_AFK:
        case CHAT_MSG_DND:
        case CHAT_MSG_RAID_LEADER:
        case CHAT_MSG_RAID_WARNING:
        case CHAT_MSG_BATTLEGROUND:
        case CHAT_MSG_BATTLEGROUND_LEADER:
        case CHAT_MSG_BATTLENET:
        case CHAT_MSG_PARTY_LEADER:
        case CHAT_MSG_BN_WHISPER:
        case CHAT_MSG_BN_WHISPER_INFORM:
        case CHAT_MSG_BN_CONVERSATION:
            return 1;
        default:
            return 0;
    }
}

// VERIFY: dba1cff5 auto
// OFFSET: 0x004FB6A0
int32_t CGChat::IsInWhisperSourceHistory(const char* name) {
    for (uint32_t i = 0; i < sizeof(CGChat::m_whisperSrcHistory) / sizeof(CGChat::m_whisperSrcHistory[0]); i++) {
        WOWGUID guid = CGChat::m_whisperSrcHistory[i];

        if (guid) {
            WOWGUID requester = WOWGUID();
            NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

            if (record && !SStrCmpUTF8I(name, record->m_name, STORM_MAX_STR)) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: f964daa1 manual
// OFFSET: 0x004FB710
void CGChat::RecordWhisperSource(WOWGUID guid) {
    if (!guid) {
        return;
    }

    uint32_t count = sizeof(CGChat::m_whisperSrcHistory) / sizeof(CGChat::m_whisperSrcHistory[0]);
    uint32_t index;

    for (index = 0; index < count; index++) {
        if (CGChat::m_whisperSrcHistory[index] == guid) {
            break;
        }
    }

    if (index >= count - 1) {
        index = count - 1;
    }

    memmove(&CGChat::m_whisperSrcHistory[1], &CGChat::m_whisperSrcHistory[0], index * sizeof(CGChat::m_whisperSrcHistory[0]));
    CGChat::m_whisperSrcHistory[0] = guid;
}

// VERIFY: a7359be4 manual
// OFFSET: 0x004FB7E0
int32_t CGChat::IsInWhisperHistory(WOWGUID guid) {
    if (!guid) {
        return 0;
    }

    for (uint32_t i = 0; i < sizeof(CGChat::m_whisperDstHistory) / sizeof(CGChat::m_whisperDstHistory[0]); i++) {
        if (CGChat::m_whisperDstHistory[i] == guid) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 1c48012d manual
// OFFSET: 0x004FB820
void CGChat::RecordWhisperDest(WOWGUID guid) {
    if (!guid) {
        return;
    }

    uint32_t count = sizeof(CGChat::m_whisperDstHistory) / sizeof(CGChat::m_whisperDstHistory[0]);
    uint32_t index;

    for (index = 0; index < count; index++) {
        if (CGChat::m_whisperDstHistory[index] == guid) {
            break;
        }
    }

    if (index >= count - 1) {
        index = count - 1;
    }

    memmove(&CGChat::m_whisperDstHistory[1], &CGChat::m_whisperDstHistory[0], index * sizeof(CGChat::m_whisperDstHistory[0]));
    CGChat::m_whisperDstHistory[0] = guid;
}

// VERIFY: 56f83f4b auto
// OFFSET: 0x004FB8F0
int32_t CGChat::IsWhisperThrottled(uint32_t time) {
    uint32_t last = s_whisperThrottleTimes[s_whisperThrottleIndex];
    return last && static_cast<int32_t>(time - last - 10000) < 0;
}

// VERIFY: 2141619b auto
// OFFSET: 0x004FB920
void CGChat::AddWhisperThrottled(uint32_t time) {
    s_whisperThrottleTimes[s_whisperThrottleIndex] = time;
    s_whisperThrottleIndex = (s_whisperThrottleIndex + 1) % (sizeof(s_whisperThrottleTimes) / sizeof(s_whisperThrottleTimes[0]));
}

// VERIFY: a1acf867 auto
// OFFSET: 0x004FCAC0
static void UpdateVoiceMemberFlags(PENDINGUSERLIST* user, uint8_t flags, WOWGUID guid, int32_t type, const char* name) {
    if (!user) {
        return;
    }

    user->m_guid = guid;
    user->m_flags = flags;

    if (type && !ComSatClient_IsTalkerSquelchedInChannel(guid, type, name)) {
        user->m_flags |= 0x10;
    }

    if (ComSatClient_IsMemberInVoiceSession(user->m_guid, type, name)) {
        user->m_flags |= 0x40;
    }

    if (ComSatClient_GetChannelMemberVoiceStatus(user->m_guid, type, name)) {
        user->m_flags |= 0x80;
    }

    if (ComSatClient_IsPlayerMuted(user->m_guid) || ComSatClient_IsTalkerSquelchedInChannel(user->m_guid, type, name)) {
        user->m_flags |= 0x20;
    }
}

// VERIFY: 180a704d auto
// OFFSET: 0x004FCF40
static bool IsInColorCode(const char* text, int32_t length) {
    bool inColor = false;

    while (length) {
        char c = *text;
        length--;
        text++;

        if (c == '|') {
            if (!length--) {
                return inColor;
            }

            c = *text;

            if (c == 'c' || c == 'C') {
                inColor = true;
            } else if ((c == 'r' || c == 'R') && inColor) {
                inColor = false;
            }

            text++;
        }
    }

    return inColor;
}

// VERIFY: 39df9e37 auto
// OFFSET: 0x004FD220
char* NameCache::FullName(char* buffer, uint32_t size) {
    if (this->m_realmName[0]) {
        SStrPrintf(buffer, size, "%s-%s", this->m_name, this->m_realmName);
    } else {
        SStrCopy(buffer, this->m_name, size);
    }

    return buffer;
}

// VERIFY: 6eef4d4c auto
// OFFSET: 0x004FDBC0
static void SendChatEvent(ChatChannel* channel, uint32_t language, const char* text, int32_t type, const char* senderName, const char* channelString, const char* targetName, const char* flags, uint32_t lineID, WOWGUID guid, const ChatConversation* conversation) {
    if (type >= CHAT_MSG_MAX) {
        return;
    }

    const char* languageName = "";
    LanguagesRec* languageRec = g_languagesDB.GetRecord(language);

    if (languageRec) {
        languageName = languageRec->m_name;
    }

    if (type == CHAT_MSG_WHISPER) {
        CGTutorial::TriggerTutorial(16);
    }

    char guidString[20] = {};

    if (!(guid.m_guid & 0xF000000000000000ULL) && (guid.m_guid & 0xF07FFFFFFFFFFFFFULL) && (!flags || SStrCmp(flags, "GM", STORM_MAX_STR) || SStrCmp(flags, "DEV", STORM_MAX_STR))) {
        GUIDToHexString(guid, guidString);
    }

    uint32_t presenceID = conversation ? conversation->m_presenceID : 0;
    int32_t channelIndex;
    uint32_t zoneChannelID;
    const char* channelBaseName;
    uint32_t unused;

    if (channel) {
        channelIndex = channel->m_channelNumber;
        unused = channel->m_unk98;
        zoneChannelID = channel->m_zoneChannelID;
        channelBaseName = channel->m_name;
    } else {
        unused = 0;
        channelBaseName = "";
        channelIndex = conversation && !conversation->m_inactive ? conversation->m_index + 1 : 0;
        zoneChannelID = 0;
    }

    FrameScript_SignalEvent(s_chatEvents[type], "%s%s%s%s%s%s%d%d%s%d%d%s%u", text ? text : "", senderName ? senderName : "", languageName, channelString ? channelString : "", targetName ? targetName : "", flags ? flags : "", zoneChannelID, channelIndex, channelBaseName, unused, lineID, guidString, presenceID);
}

// VERIFY: 4dbf06e6 manual
// OFFSET: 0x004FDD80
void CGChat::RecordChat(const char* text, int32_t type, const char* senderName, WOWGUID senderGuid, uint32_t senderUnk, uint32_t a6, const char* channelName, uint32_t lineID, const CGPlayer_C* player) {
    if (!s_chatHistoryInitialized) {
        for (uint32_t i = 0; i < sizeof(CGChat::m_chatHistory) / sizeof(CGChat::m_chatHistory[0]); i++) {
            ChatRecord* record = &CGChat::m_chatHistory[i];
            record->m_senderGuid = WOWGUID();
            record->m_senderUnk = 0;
            record->m_senderName[0] = 0;
            record->m_description[0] = 0;
            record->m_text[0] = 0;
            record->m_type = 0;
            record->m_unk17B0 = 0;
            record->m_lineID = 0;
            record->m_time = 0;
        }

        s_chatHistoryInitialized = 1;
    }

    if (!text || !CGChat::IsRecordedType(type)) {
        return;
    }

    ChatRecord* record = &CGChat::m_chatHistory[CGChat::m_currentChatIndex];
    WOWGUID playerGuid = player ? player->m_obj->m_guid : WOWGUID();

    SStrPrintf(record->m_description, sizeof(record->m_description), "Type: [%d], Channel: [%s], Player Name: [%s], Sender GUID: [%016I64X], Active player: [%016I64X], Text: [%s]", type, channelName ? channelName : "", senderName ? senderName : "", senderGuid.m_guid, playerGuid.m_guid, text);
    SStrCopy(record->m_text, text, sizeof(record->m_text));
    record->m_time = static_cast<uint32_t>(time(nullptr));
    record->m_type = type;
    record->m_senderGuid = senderGuid;
    record->m_senderUnk = senderUnk;
    record->m_unk17B0 = a6;

    if (senderName) {
        SStrCopy(record->m_senderName, senderName, sizeof(record->m_senderName));
    }

    record->m_lineID = lineID;

    if (++CGChat::m_currentChatIndex >= sizeof(CGChat::m_chatHistory) / sizeof(CGChat::m_chatHistory[0])) {
        CGChat::m_currentChatIndex = 0;
    }
}

// VERIFY: 4b45a3c3 auto
// OFFSET: 0x004FDFC0
ChatChannel* GetChannelByName(const char* name) {
    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        if (!SStrCmpUTF8I(CGChat::s_channels.m_data[i].m_name, name, STORM_MAX_STR)) {
            return &CGChat::s_channels.m_data[i];
        }
    }

    return nullptr;
}

// VERIFY: bc0234ac manual
// OFFSET: 0x004FE2D0
int32_t StringToLanguage(const char* string, uint32_t* language) {
    for (int32_t i = 0; i < g_languagesDB.m_numRecords; i++) {
        LanguagesRec* rec = (i < 0 || i >= g_languagesDB.m_numRecords) ? nullptr : &g_languagesDB.m_records[i];

        if (rec && !SStrCmpUTF8I(string, rec->m_name, STORM_MAX_STR)) {
            *language = rec->m_id;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 8add7663 manual
// OFFSET: 0x00500180
void CGChat::SendIgnoredResponse(WOWGUID guid, uint8_t a2) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAT_REPORT_IGNORED));
    msg.Put(guid.m_guid);
    msg.Put(a2);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 7953750b manual
// OFFSET: 0x005016B0
THROTTLEDCHAT::~THROTTLEDCHAT() {
    SMemFree(this->m_target, __FILE__, __LINE__, 0);
    SMemFree(this->m_text, __FILE__, __LINE__, 0);
}

// VERIFY: f592c07c manual
// OFFSET: 0x00502D00
void CGChat::QueueChatText(const char* text, int32_t type, const char* senderName, const char* targetName, uint32_t language, const char* channelName, const char* flags, int32_t queued, int32_t a9, WOWGUID senderGuid, uint32_t senderUnk, WOWGUID targetGuid, int32_t achievementID, bool isGM, const ChatConversation* conversation) {
    PENDINGCHAT* chat = CGChat::s_pendingChat.NewNode(2, 0, 0);

    chat->m_senderGuid = senderGuid;
    chat->m_type = type;
    chat->m_senderUnk = senderUnk;
    chat->m_text = SStrDupA(text, __FILE__, __LINE__);
    chat->m_senderName = senderName ? SStrDupA(senderName, __FILE__, __LINE__) : nullptr;
    chat->m_targetName = targetName ? SStrDupA(targetName, __FILE__, __LINE__) : nullptr;
    chat->m_language = language;
    chat->m_queued = queued;
    chat->unused_0030 = a9;
    chat->m_achievementID = achievementID;
    chat->m_isGM = isGM;
    chat->m_conversation = conversation ? new (SMemAlloc(sizeof(ChatConversation), __FILE__, __LINE__, 0)) ChatConversation(*conversation) : nullptr;

    if (channelName) {
        SStrCopy(chat->m_channelName, channelName, sizeof(chat->m_channelName));
    } else {
        chat->m_channelName[0] = 0;
    }

    chat->m_targetGuid = targetGuid;

    if (flags) {
        SStrCopy(chat->m_flags, flags, sizeof(chat->m_flags));
    } else {
        chat->m_flags[0] = 0;
    }
}

// VERIFY: 5382bfae auto
// OFFSET: 0x00502F60
int32_t CGChat::IsChannelThrottled(const char* channel, uint32_t time) {
    CHATTHROTTLE* throttle = s_channelThrottle.Ptr(channel);

    if (throttle) {
        uint32_t last = throttle->m_times[throttle->m_index];

        if (last && static_cast<int32_t>(time - last - 10000) < 0) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 90b2a5a1 manual
// OFFSET: 0x00502FA0
void CGChat::AddThrottledChat(int32_t type, uint32_t language, const char* target, const char* text) {
    THROTTLEDCHAT* chat = CGChat::s_throttledChat.NewNode(2, 0, 0);
    chat->m_type = type;
    chat->m_language = language;
    chat->m_target = SStrDupA(target, __FILE__, __LINE__);
    chat->m_text = SStrDupA(text, __FILE__, __LINE__);
}

// VERIFY: 6b0b44c6 manual
// OFFSET: 0x005097D0
void CGChat::AddChannelThrottled(const char* channel, uint32_t time) {
    CHATTHROTTLE* throttle = s_channelThrottle.Ptr(channel);

    if (!throttle) {
        throttle = s_channelThrottle.New(channel, 0, 0);
    }

    throttle->m_times[throttle->m_index] = time;
    throttle->m_index = (throttle->m_index + 1) % (sizeof(throttle->m_times) / sizeof(throttle->m_times[0]));
}

// VERIFY: 52f6851f manual
// OFFSET: 0x005098B0
void CGChat::ProcessThrottledChat() {
    if (!CGChat::s_throttledChat.Head()) {
        return;
    }

    uint32_t time = OsGetAsyncTimeMs();
    THROTTLEDCHAT* next;

    for (THROTTLEDCHAT* chat = CGChat::s_throttledChat.Head(); !CGChat::s_throttledChat.IsEnd(chat); chat = next) {
        next = CGChat::s_throttledChat.RawNext(chat);

        if (chat->m_type == CHAT_MSG_WHISPER || chat->m_type == CHAT_MSG_BATTLENET) {
            if (!CGChat::IsInWhisperSourceHistory(chat->m_target)) {
                if (CGChat::IsWhisperThrottled(time)) {
                    continue;
                }

                CGChat::AddWhisperThrottled(time);
            }
        } else {
            if (CGChat::IsChannelThrottled(chat->m_target, time)) {
                continue;
            }

            CGChat::AddChannelThrottled(chat->m_target, time);
        }

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
        msg.Put(static_cast<uint32_t>(chat->m_type));
        msg.Put(chat->m_language);
        msg.PutString(chat->m_target);
        msg.PutString(chat->m_text);
        msg.Finalize();
        ClientServices::Send2(&msg);

        CGChat::s_throttledChat.DeleteNode(chat);
    }
}

// VERIFY: 5c701e3f manual
// OFFSET: 0x00509A50
bool ProfanityFilter(char* text, bool cache, bool force) {
    if (!force && !g_profanityFilterCVar->m_intValue) {
        return false;
    }

    if (!*text) {
        return false;
    }

    PROFANITYCHECK* check = s_profanityCache.Ptr(text);

    if (check) {
        if (check->m_filtered) {
            SStrCopy(text, check->m_filteredText, STORM_MAX_STR);
        }

        return check->m_filtered;
    }

    if (cache) {
        check = s_profanityCache.New(text, 0, 0);
    }

    bool filtered = false;
    uint32_t count = g_chatProfanityTokens[s_locale].Count();

    for (uint32_t i = 0; i < count; i++) {
        char* current = text;
        regmatch_t match;

        while (!regexec(&g_chatProfanityTokens[s_locale].m_data[i], current, 1, &match, 0)) {
            if (match.rm_so < 0 || IsInColorCode(text, match.rm_so)) {
                break;
            }

            int32_t length = match.rm_eo - match.rm_so;
            current += match.rm_so;
            filtered = true;

            for (; length > 0; length--) {
                *current++ = s_profanityReplacement[s_profanityReplacementIndex++];

                if (s_profanityReplacementIndex >= sizeof(s_profanityReplacement) - 1) {
                    s_profanityReplacementIndex = 0;
                }
            }
        }
    }

    if (check) {
        check->m_filtered = filtered;

        if (filtered) {
            check->m_filteredText = SStrDupA(text, __FILE__, __LINE__);
        }
    }

    return filtered;
}

// VERIFY: 1fe3354e manual
// OFFSET: 0x00509BD0
bool SpamFilter(char* text, bool cache) {
    if (!*text) {
        return false;
    }

    SPAMCHECK* check = s_spamCache.Ptr(text);

    if (check) {
        if (check->m_filtered) {
            SStrCopy(text, check->m_filteredText, STORM_MAX_STR);
        }

        return check->m_filtered;
    }

    if (cache) {
        check = s_spamCache.New(text, 0, 0);
    }

    bool spam = false;

    for (uint32_t i = 0; i < g_chatSpamTokens.Count(); i++) {
        const char* current = text;
        regmatch_t match;

        while (!regexec(&g_chatSpamTokens.m_data[i], current, 1, &match, 0)) {
            if (match.rm_so < 0) {
                break;
            }

            current += match.rm_eo;
            spam = true;
        }
    }

    for (uint32_t i = 0; i < s_expectedSpamRecords.Count(); i++) {
        const char* current = text;
        regmatch_t match;

        while (!regexec(&s_expectedSpamRecords.m_data[i], current, 1, &match, 0)) {
            if (match.rm_so < 0) {
                break;
            }

            current += match.rm_eo;
            spam = true;
        }
    }

    if (check) {
        check->m_filtered = spam;

        if (spam) {
            check->m_filteredText = SStrDupA(text, __FILE__, __LINE__);
        }
    }

    return spam;
}

// VERIFY: 812184e4 manual
// OFFSET: 0x00509DD0
void CGChat::AddChatMessage(const char* text, int32_t type, const char* senderName, uint32_t language, const char* channelName, const char* targetName, const char* flags, WOWGUID senderGuid, uint32_t senderUnk, WOWGUID targetGuid, int32_t achievementID, bool isGM, const ChatConversation* conversation) {
    if (type >= CHAT_MSG_MAX) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!text) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    char senderFullName[305];
    FRAMESCRIPT_GENDER gender = GENDER_NOT_APPLICABLE;

    if (!senderName) {
        if (!(senderGuid.m_guid & 0xF000000000000000ULL) && (senderGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            NameCache* record = g_nameDBCache.GetRecord(senderGuid.m_guid, senderGuid, CGChat::NameQueryCallback, nullptr, false);

            if (record) {
                senderName = record->FullName(senderFullName, sizeof(senderFullName));

                if (record->m_sex == 1) {
                    gender = GENDER_FEMALE;
                }
            }
        } else if ((senderGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((senderGuid.m_guid >> 24) & 0xFFFFFFF), senderGuid, CGChat::NameQueryCallback, nullptr, false);

            if (record) {
                senderName = record->m_name;
            }
        }
    }

    if (language != 0xFFFFFFFF && (type == CHAT_MSG_WHISPER || type == CHAT_MSG_WHISPER_INFORM)) {
        CGAutoCompleteName::AddFlags(senderGuid, 0x30, 1, senderName);
    }

    if (!targetName) {
        if (!(targetGuid.m_guid & 0xF000000000000000ULL) && (targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            NameCache* record = g_nameDBCache.GetRecord(targetGuid.m_guid, targetGuid, CGChat::NameQueryCallback, nullptr, false);

            if (record) {
                targetName = record->m_name;
            }
        } else if ((targetGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((targetGuid.m_guid >> 24) & 0xFFFFFFF), targetGuid, CGChat::NameQueryCallback, nullptr, false);

            if (record) {
                targetName = record->m_name;
            }
        }
    }

    if (s_queueChat
        || (senderGuid && ((!(senderGuid.m_guid & 0xF000000000000000ULL) && (senderGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) || (senderGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) && !senderName)
        || (targetGuid && ((!(targetGuid.m_guid & 0xF000000000000000ULL) && (targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) || (targetGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) && !targetName)) {
        CGChat::QueueChatText(text, type, senderName, targetName, language, channelName, flags, s_queueChat, 0, senderGuid, senderUnk, targetGuid, achievementID, isGM, conversation);
        return;
    }

    char buffer[3000];

    if (language == 0xFFFFFFFF) {
        char message[3000];
        memset(message, 0, sizeof(message));

        char* dest = buffer;

        for (const char* source = text; *source; source++) {
            if (*source == '\t' && dest >= buffer && dest < buffer + sizeof(buffer)) {
                *dest = 0;
                dest = message;
            } else {
                *dest++ = *source;
            }
        }

        *dest = 0;

        const char* channel;

        switch (type) {
            case CHAT_MSG_PARTY:
                channel = "PARTY";
                break;
            case CHAT_MSG_RAID:
                channel = "RAID";
                break;
            case CHAT_MSG_GUILD:
                channel = "GUILD";
                break;
            case CHAT_MSG_WHISPER:
            case CHAT_MSG_WHISPER_FOREIGN:
            case CHAT_MSG_BATTLENET:
                channel = "WHISPER";
                break;
            case CHAT_MSG_BATTLEGROUND:
                channel = "BATTLEGROUND";
                break;
            default:
                channel = "UNKNOWN";
                break;
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_CHAT_MSG_ADDON, "%s%s%s%s", buffer, message, channel, senderName ? senderName : "");
        return;
    }

    if (!ValidateStringIsUTF8(text)) {
        return;
    }

    char questText[3000];

    if ((type == CHAT_MSG_MONSTER_SAY || type == CHAT_MSG_MONSTER_YELL || type == CHAT_MSG_RAID_BOSS_EMOTE || type == CHAT_MSG_RAID_BOSS_WHISPER || type == CHAT_MSG_MONSTER_EMOTE || type == CHAT_MSG_MONSTER_WHISPER || type == CHAT_MSG_MONSTER_PARTY || type == CHAT_MSG_BG_SYSTEM_NEUTRAL || type == CHAT_MSG_BG_SYSTEM_ALLIANCE || type == CHAT_MSG_BG_SYSTEM_HORDE || type == CHAT_MSG_ACHIEVEMENT || type == CHAT_MSG_GUILD_ACHIEVEMENT) && QuestParserParseText(text, questText, sizeof(questText), targetGuid, targetName, achievementID)) {
        text = questText;
    }

    if (type == CHAT_MSG_SAY || type == CHAT_MSG_YELL || type == CHAT_MSG_PARTY || type == CHAT_MSG_PARTY_LEADER) {
        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(senderGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

        if (unit && !(unit->m_unit->m_unitFieldFlags & 0x20000000)) {
            char token[32];

            for (int32_t i = 1;;) {
                SStrPrintf(token, sizeof(token), "LAUGH_WORD%d", i++);
                const char* word = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);

                if (!word || !*word) {
                    break;
                }

                if (!SStrCmpUTF8I(word, text, STORM_MAX_STR)) {
                    unit->RequestTalkEmote(4);
                    goto emoteDone;
                }
            }

            if (type == CHAT_MSG_SAY || type == CHAT_MSG_YELL) {
                int32_t animation = 0;
                uint32_t length = SStrLen(text);

                if (type == CHAT_MSG_YELL) {
                    animation = 3;
                } else if (length) {
                    if (text[length - 1] == '?') {
                        animation = 1;
                    } else if (text[length - 1] == '!') {
                        animation = 2;
                    }
                }

                unit->RequestTalkEmote(animation);
            }
        }
    }

emoteDone:
    if (type == CHAT_MSG_EMOTE || type == CHAT_MSG_MONSTER_EMOTE || type == CHAT_MSG_SYSTEM || type == CHAT_MSG_RAID_BOSS_EMOTE) {
        language = 0;
    }

    if (!*text) {
        switch (type) {
            case CHAT_MSG_SAY:
                text = FrameScript_GetText("CHAT_SAY_UNKNOWN", -1, gender);
                type = CHAT_MSG_EMOTE;
                break;
            case CHAT_MSG_YELL:
                text = FrameScript_GetText("CHAT_YELL_UNKNOWN", -1, gender);
                type = CHAT_MSG_EMOTE;
                break;
            case CHAT_MSG_EMOTE:
                text = FrameScript_GetText("CHAT_EMOTE_UNKNOWN", -1, gender);
                break;
        }
    }

    if (!conversation) {
        for (char* quest = SStrStr(text, "|Hquest:"); quest; quest = SStrStr(quest + 8, "|Hquest:")) {
            if (quest[-1] == '|') {
                continue;
            }

            char* separator = SStrChr(quest, ':');
            char* level = separator ? SStrChr(separator + 1, ':') : nullptr;

            if (!level) {
                SErrDisplayAppFatalMessage("", "", 0);
            }

            int32_t questLevel = SStrToInt(level + 1);
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player && questLevel == -1) {
                questLevel = player->m_unit->m_unitFieldLevel;
            }

            const char* color = CGTooltip::GetQuestDifficultyColorString(questLevel);

            for (char* dest = quest - 10; *color; color++) {
                *dest++ = *color;
            }
        }
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    char translated[3000];

    if (player && (player->m_player->m_playerFlags & 0x8)) {
        language = 0;
        SStrCopy(translated, text, sizeof(translated));
    } else if (language && player) {
        uint32_t skill;
        player->GetLanguageSkill(language, skill);
        CGChat::TranslateMessage(language, skill, text, translated, sizeof(translated), 0, 0);
    } else {
        SStrCopy(translated, text, sizeof(translated));
    }

    char channelString[138] = {};
    ChatChannel* channel = nullptr;
    uint32_t zoneChannelID = 0;

    if (channelName && *channelName) {
        channel = GetChannelByName(channelName);

        if (channel) {
            zoneChannelID = channel->m_zoneChannelID;
            SStrPrintf(channelString, sizeof(channelString), "%d. %s", channel->m_channelNumber, channelName);
        } else {
            SStrCopy(channelString, channelName, sizeof(channelString));
        }

        ProfanityFilter(channelString, false, false);
    }

    if (conversation && !conversation->m_inactive) {
        SStrPrintf(channelString, sizeof(channelString), FrameScript_GetText("CONVERSATION_NAME", -1, GENDER_NOT_APPLICABLE), conversation->m_index + 1);
    }

    bool filterProfanity = false;

    if (g_profanityFilterCVar->m_intValue) {
        for (uint32_t i = 0; i < sizeof(s_profanityFilterTypes) / sizeof(s_profanityFilterTypes[0]); i++) {
            if (type == s_profanityFilterTypes[i]) {
                filterProfanity = true;
                break;
            }
        }
    }

    if (type != CHAT_MSG_FILTERED && (!flags || (SStrCmp(flags, "GM", STORM_MAX_STR) && SStrCmp(flags, "DEV", STORM_MAX_STR))) && (!player || !(player->m_player->m_playerFlags & 0x8)) && g_spamFilterCVar->m_intValue && type && SpamFilter(translated, false)) {
        if (type == CHAT_MSG_WHISPER && senderGuid) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_CHAT_REPORT_FILTERED));
            msg.Put(senderGuid.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
        }

        return;
    }

    if (filterProfanity) {
        ProfanityFilter(translated, false, false);
    }

    uint32_t lineID = ++s_chatLineID;
    CGChat::RecordChat(translated, type, senderName, senderGuid, senderUnk, zoneChannelID, channelName, s_chatLineID, player);
    SendChatEvent(channel, language, translated, type, senderName, channelString, targetName, flags, lineID, senderGuid, conversation);

    if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_BN_WHISPER) {
        CGLCD::OnWhisper(senderName);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(senderGuid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        const char* bubble = translated;

        if (type == CHAT_MSG_MONSTER_SAY || type == CHAT_MSG_MONSTER_YELL) {
            int32_t source = 0;
            int32_t dest = 0;

            do {
                char c = translated[source];

                if (!c) {
                    break;
                }

                buffer[dest] = c;

                if (c == '%' && translated[source + 1] == '%') {
                    source++;
                }

                source++;
                dest++;
            } while (source < static_cast<int32_t>(sizeof(translated)));

            buffer[dest] = 0;
            bubble = buffer;
        }

        unit->ChatBubbleShow(type, bubble);
    }

    if (!s_chatLogging[0]) {
        return;
    }

    char channelPrefix[256];
    char token[64];

    switch (type) {
        case CHAT_MSG_CHANNEL_LIST: {
            if (channelString[0]) {
                SStrPrintf(channelPrefix, sizeof(channelPrefix), FrameScript_GetText("CHAT_CHANNEL_LIST_GET", -1, GENDER_NOT_APPLICABLE), zoneChannelID, channelName);
            } else {
                channelPrefix[0] = 0;
            }

            SLogWrite(s_chatLog[0], "%s%s", channelPrefix, translated);
            return;
        }

        case CHAT_MSG_CHANNEL_NOTICE:
        case CHAT_MSG_CHANNEL_NOTICE_USER: {
            SStrPrintf(token, sizeof(token), "CHAT_%s_NOTICE", translated);
            SLogWrite(s_chatLog[0], FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), channelString, senderName, targetName);
            return;
        }

        case CHAT_MSG_IGNORED: {
            SLogWrite(s_chatLog[0], FrameScript_GetText("CHAT_IGNORED", -1, GENDER_NOT_APPLICABLE), senderName);
            return;
        }

        case CHAT_MSG_FILTERED: {
            SLogWrite(s_chatLog[0], FrameScript_GetText("CHAT_FILTERED", -1, GENDER_NOT_APPLICABLE), senderName);
            return;
        }

        case CHAT_MSG_RESTRICTED: {
            SLogWrite(s_chatLog[0], FrameScript_GetText("CHAT_RESTRICTED", -1, GENDER_NOT_APPLICABLE));
            return;
        }

        case CHAT_MSG_BN_CONVERSATION_NOTICE: {
            char noticeToken[128];
            SStrPrintf(noticeToken, sizeof(noticeToken), "CHAT_CONVERSATION_%s_NOTICE", translated);
            SLogWrite(s_chatLog[0], FrameScript_GetText(noticeToken, -1, GENDER_NOT_APPLICABLE), channelString, senderName);
            return;
        }

        default: {
            if (channelString[0]) {
                SStrPrintf(channelPrefix, sizeof(channelPrefix), "[%s] ", channelString);
                char* dash = SStrStr(channelPrefix, " - ");

                if (dash) {
                    SStrCopy(dash, "] ", STORM_MAX_STR);
                }
            } else {
                channelPrefix[0] = 0;
            }

            SStrPrintf(token, sizeof(token), "CHAT_%s_GET", s_chatTypeInfo[type].m_name);

            char header[304];
            SStrPrintf(header, sizeof(header), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), senderName ? senderName : "");

            if (language && player && language != player->GetDefaultLanguage()) {
                char languageName[256];
                SStrPrintf(languageName, sizeof(languageName), "[%s] ", g_languagesDB.GetRecord(language)->m_name);
                SStrPack(header, languageName, sizeof(header));
            }

            const char* source = LanguageProcess(translated);
            char line[6560];
            SStrPrintf(line, sizeof(line), "%s%s", channelPrefix, header);
            char* dest = &line[SStrLen(line)];

            while (source) {
                char c = *source;

                if (!c || dest >= &line[sizeof(line) - 1]) {
                    break;
                }

                if (c == '|') {
                    if (source[1] == '|') {
                        *dest++ = '|';
                        source += 2;
                        continue;
                    }

                    source = SStrChr(source, '[');

                    if (!source) {
                        break;
                    }

                    while (*source) {
                        c = *source;

                        if (c != '[') {
                            if (c == ']') {
                                source++;

                                if (!SStrCmp(source, "|h", 2)) {
                                    source += 2;
                                }

                                if (!SStrCmp(source, "|r", 2)) {
                                    source += 2;
                                }

                                break;
                            }

                            *dest++ = c;
                        }

                        source++;
                    }
                } else {
                    if (c == '%' && type != CHAT_MSG_MONSTER_SAY && type != CHAT_MSG_MONSTER_YELL && type != CHAT_MSG_RAID_BOSS_EMOTE && type != CHAT_MSG_RAID_BOSS_WHISPER && type != CHAT_MSG_MONSTER_EMOTE && type != CHAT_MSG_MONSTER_WHISPER && type != CHAT_MSG_MONSTER_PARTY) {
                        *dest++ = '%';
                        *dest = '%';
                    } else {
                        *dest = c;
                    }

                    dest++;
                    source++;
                }
            }

            *dest = 0;
            SLogWrite(s_chatLog[0], line, senderName ? senderName : "");
            return;
        }
    }
}

// VERIFY: 71191980 manual
// OFFSET: 0x0050BE70
int32_t CGChat::ChatMessageReceived(CDataStore* msg, bool isGM) {
    char channelName[128];
    memset(channelName, 0, sizeof(channelName));

    const char* senderName = nullptr;
    const char* targetName = nullptr;
    uint8_t chatTag = 0;
    const char* flags = "";
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    uint32_t achievementID = 0;
    WOWGUID senderGuid = WOWGUID();
    uint32_t senderUnk = 0;
    WOWGUID targetGuid = WOWGUID();
    uint8_t type;
    uint32_t language;
    uint32_t length;
    void* data;
    const char* text;

    msg->Get(type);
    msg->Get(language);
    msg->Get(senderGuid.m_guid);
    msg->Get(senderUnk);

    switch (type) {
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_RAID_BOSS_EMOTE:
        case CHAT_MSG_RAID_BOSS_WHISPER:
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_BATTLENET: {
            msg->Get(length);
            msg->GetDataInSitu(data, length);
            senderName = static_cast<const char*>(data);
            msg->Get(targetGuid.m_guid);

            if (targetGuid && ((targetGuid.m_guid & 0xF000000000000000ULL) || !(targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) && (targetGuid.m_guid & 0xF0F0000000000000ULL) != 0xF040000000000000ULL) {
                msg->Get(length);
                msg->GetDataInSitu(data, length);
                targetName = static_cast<const char*>(data);
            }

            msg->Get(length);
            msg->GetDataInSitu(data, length);
            text = static_cast<const char*>(data);
            msg->Get(chatTag);

            if (FriendList::s_friendList && FriendList::s_friendList->IsIgnored(targetGuid, true)) {
                return 1;
            }

            break;
        }

        case CHAT_MSG_WHISPER_FOREIGN: {
            msg->Get(length);
            msg->GetDataInSitu(data, length);
            senderName = static_cast<const char*>(data);
            msg->Get(targetGuid.m_guid);
            msg->Get(length);
            msg->GetDataInSitu(data, length);
            text = static_cast<const char*>(data);
            msg->Get(chatTag);

            if (FriendList::s_friendList && FriendList::s_friendList->IsIgnored(senderGuid, true)) {
                return 1;
            }

            type = CHAT_MSG_WHISPER;
            break;
        }

        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE: {
            msg->Get(targetGuid.m_guid);

            if (targetGuid && ((targetGuid.m_guid & 0xF000000000000000ULL) || !(targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL))) {
                msg->Get(length);
                msg->GetDataInSitu(data, length);
                targetName = static_cast<const char*>(data);
            }

            msg->Get(length);
            msg->GetDataInSitu(data, length);
            text = static_cast<const char*>(data);
            msg->Get(chatTag);
            break;
        }

        case CHAT_MSG_ACHIEVEMENT:
        case CHAT_MSG_GUILD_ACHIEVEMENT: {
            msg->Get(targetGuid.m_guid);
            msg->Get(length);
            msg->GetDataInSitu(data, length);
            text = static_cast<const char*>(data);
            msg->Get(chatTag);
            msg->Get(achievementID);
            break;
        }

        default: {
            if (isGM) {
                msg->Get(length);
                msg->GetDataInSitu(data, length);
                senderName = static_cast<const char*>(data);
            }

            if (type == CHAT_MSG_CHANNEL) {
                msg->GetString(channelName, sizeof(channelName));
            }

            msg->Get(targetGuid.m_guid);
            msg->Get(length);
            msg->GetDataInSitu(data, length);
            text = static_cast<const char*>(data);
            msg->Get(chatTag);

            if (chatTag & 0x10) {
                flags = "DEV";
            } else if (chatTag & 0x4) {
                flags = "GM";
            } else if (chatTag & 0x2) {
                flags = "DND";
            } else if (chatTag & 0x1) {
                flags = "AFK";
            }

            if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_BATTLENET) {
                CGChat::RecordWhisperSource(senderGuid);
            } else if (type == CHAT_MSG_WHISPER_INFORM) {
                CGChat::RecordWhisperDest(senderGuid);
            }

            if ((type == CHAT_MSG_WHISPER || type == CHAT_MSG_BATTLENET) && (chatTag & 0x8) && !(chatTag & 0x4) && language != 0xFFFFFFFF) {
                if ((!player || senderGuid != player->m_obj->m_guid) && (!FriendList::s_friendList || !FriendList::s_friendList->IsFriend(senderGuid)) && !CGChat::IsInWhisperHistory(senderGuid)) {
                    CGChat::SendIgnoredResponse(senderGuid, 1);
                    return 1;
                }
            }

            if (FriendList::s_friendList && FriendList::s_friendList->IsIgnored(senderGuid, true) && !(chatTag & 0x4)) {
                if (type == CHAT_MSG_WHISPER) {
                    if (language == 0xFFFFFFFF) {
                        return 1;
                    }

                    CGChat::SendIgnoredResponse(senderGuid, 0);
                }

                if (type != CHAT_MSG_WHISPER_INFORM && type != CHAT_MSG_IGNORED && type != CHAT_MSG_RESTRICTED) {
                    return 1;
                }
            }

            break;
        }
    }

    if (!msg->IsRead()) {
        msg->Reset();
        return 1;
    }

    CGChat::AddChatMessage(text, type, senderName, language, channelName, targetName, flags, senderGuid, senderUnk, targetGuid, achievementID, isGM, nullptr);
    return 1;
}

// VERIFY: b619309b auto
// OFFSET: 0x0050EBA0
int32_t CGChat::OnMessageChat(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CGChat::ChatMessageReceived(msg, false);
}

// VERIFY: 1c73e338 auto
// OFFSET: 0x0050EBC0
int32_t CGChat::OnGMMessageChat(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    return CGChat::ChatMessageReceived(msg, true);
}

// VERIFY: cb7df305 auto
// OFFSET: 0x004FBB20
int32_t StringToChatType(const char* string, int32_t* type) {
    struct {
        int32_t type;
        const char* name;
    } types[] = {
        { CHAT_MSG_SAY, "SAY" },
        { CHAT_MSG_PARTY, "PARTY" },
        { CHAT_MSG_RAID, "RAID" },
        { CHAT_MSG_GUILD, "GUILD" },
        { CHAT_MSG_OFFICER, "OFFICER" },
        { CHAT_MSG_YELL, "YELL" },
        { CHAT_MSG_WHISPER, "WHISPER" },
        { CHAT_MSG_EMOTE, "EMOTE" },
        { CHAT_MSG_CHANNEL, "CHANNEL" },
        { CHAT_MSG_AFK, "AFK" },
        { CHAT_MSG_DND, "DND" },
        { CHAT_MSG_RAID_WARNING, "RAID_WARNING" },
        { CHAT_MSG_BATTLEGROUND, "BATTLEGROUND" },
        { CHAT_MSG_BATTLENET, "BN" },
        { CHAT_MSG_BN_WHISPER, "BN_WHISPER" },
        { CHAT_MSG_BN_WHISPER_INFORM, "BN_WHISPER_INFORM" },
        { CHAT_MSG_BN_CONVERSATION, "BN_CONVERSATION" },
    };

    for (uint32_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        if (!SStrCmpI(types[i].name, string, STORM_MAX_STR)) {
            *type = types[i].type;
            return 1;
        }
    }

    return 0;
}

// VERIFY: 8eb73ccb manual
// OFFSET: 0x00577D00
bool ChatParserParseText(const char* text, char* buffer, uint32_t size) {
    if (!text) {
        return false;
    }

    bool valid = true;
    *buffer = 0;
    const char* start = text;
    const char* token = SStrChr(text, '%');

    if (token) {
        while (*token) {
            uint32_t length = token - start;

            if (token != start) {
                uint32_t used = SStrLen(buffer);
                SStrPack(buffer, start, size);

                if (used + length < size) {
                    buffer[used + length] = 0;
                }
            }

            const char* code = token + 1;

            switch (*code) {
                case 'F':
                case 'f': {
                    WOWGUID focus = CGGameUI::m_focusTarget;
                    const char* name = nullptr;

                    if (focus) {
                        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(focus, TYPEMASK_UNIT, __FILE__, __LINE__));

                        if (unit) {
                            name = unit->GetUnitName(nullptr, 1);
                        } else {
                            WOWGUID requester = WOWGUID();
                            NameCache* record = g_nameDBCache.GetRecord(focus.m_guid, requester, nullptr, nullptr, false);

                            if (record) {
                                name = record->m_name;
                            }
                        }
                    }

                    SStrPack(buffer, name ? name : FrameScript_GetText("FOCUS_TOKEN_NOT_FOUND", -1, GENDER_NOT_APPLICABLE), size);
                    start = code + 1;
                    break;
                }

                case 'N':
                case 'T':
                case 'n':
                case 't': {
                    WOWGUID target = CGGameUI::m_lockedTarget;
                    const char* name = nullptr;

                    if (target) {
                        CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));

                        if (unit) {
                            name = unit->GetUnitName(nullptr, 1);
                        } else {
                            WOWGUID requester = WOWGUID();
                            NameCache* record = g_nameDBCache.GetRecord(target.m_guid, requester, nullptr, nullptr, false);

                            if (record) {
                                name = record->m_name;
                            }
                        }
                    }

                    SStrPack(buffer, name ? name : FrameScript_GetText("TARGET_TOKEN_NOT_FOUND", -1, GENDER_NOT_APPLICABLE), size);
                    start = code + 1;
                    break;
                }

                default: {
                    valid = false;
                    SStrPack(buffer, "%", size);
                    start = code;
                    break;
                }
            }

            token = SStrChr(start, '%');

            if (!token) {
                break;
            }
        }
    }

    SStrPack(buffer, start, size);
    return valid;
}

// VERIFY: 2df1a620 manual
// OFFSET: 0x004FB410
static void TranslateMessageCase(const char* word, char* dest, uint32_t size, const char* source) {
    if (!size) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    size--;

    while (*dest && size) {
        dest++;
        size--;
    }

    if (s_locale == 1 || (s_locale > 3 && s_locale <= 5)) {
        for (char c = *word; c && *source && size; c = *++word) {
            char s = *source++;

            if (static_cast<uint8_t>(s - 'A') <= 25) {
                *dest = static_cast<char>(ToUpper(c));
            } else {
                *dest = c;
            }

            dest++;
            size--;
        }
    } else {
        while (*word && *source && size) {
            int32_t chars;
            uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(source), &chars);

            if (c == 0xFFFFFFFF) {
                break;
            }

            *dest = static_cast<char>(IsUpper(static_cast<uint16_t>(c)) ? ToUpper(*word) : ToLower(*word));
            source += chars;
            word++;
            dest++;
            size--;
        }
    }

    *dest = 0;
}

// VERIFY: fae215f1 auto
// OFFSET: 0x004FDEE0
int32_t CGChat::GetChannelID(const char* name) {
    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        if (!SStrCmpUTF8I(CGChat::s_channels.m_data[i].m_name, name, STORM_MAX_STR) && !CGChat::s_channels.m_data[i].m_unk9C) {
            return CGChat::s_channels.m_data[i].m_channelNumber;
        }
    }

    return 0;
}

// VERIFY: bddc39b1 manual
// OFFSET: 0x004FE020
ChatChannel* CGChat::GetChannelByChatChannelID(uint32_t id) {
    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        if (CGChat::s_channels.m_data[i].m_zoneChannelID == id) {
            return &CGChat::s_channels.m_data[i];
        }
    }

    return nullptr;
}

// VERIFY: 3156a598 auto
// OFFSET: 0x004FB540
int32_t CGChat::OnChannelMemberCount(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char channel[128];
    uint8_t flags;
    uint32_t count;
    msg->GetString(channel, sizeof(channel));
    msg->Get(flags);
    msg->Get(count);
    return 1;
}

// VERIFY: b739537d auto
// OFFSET: 0x004FB5C0
void CGChat::InviteNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID requester = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

    if (record) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_INVITE_REQUEST, "%s%s", s_inviteChannelName, record->m_name);
        s_inviteChannelName[0] = 0;
    }
}

// VERIFY: 6aee5bf7 auto
// OFFSET: 0x004FB620
int32_t CGChat::OnComsatReconnectAttempt(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_COMSAT_RECONNECT_ATTEMPT);
    return 1;
}

// VERIFY: da0f146b auto
// OFFSET: 0x004FB640
int32_t CGChat::OnComsatDisconnect(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_COMSAT_DISCONNECT);
    return 1;
}

// VERIFY: 0ce846b5 auto
// OFFSET: 0x004FB660
int32_t CGChat::OnComsatConnectFail(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_COMSAT_CONNECT_FAIL);
    return 1;
}

// VERIFY: 21895ed8 auto
// OFFSET: 0x004FB680
int32_t CGChat::OnVoicesessionFull(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGGameUI::DisplayError(ERR_VOICESESSION_FULL);
    return 1;
}

// VERIFY: 396f1f2c auto
// OFFSET: 0x004FB990
int32_t CGChat::OnUpdateInstanceOwnership(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t owns;
    msg->Get(reinterpret_cast<uint32_t&>(owns));
    UpdateInstanceOwnershipPacket(owns);
    return 1;
}

// VERIFY: 76ab0fc8 auto
// OFFSET: 0x004FE100
int32_t CGChat::OnUpdateInstance(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t mapId;
    msg->Get(reinterpret_cast<uint32_t&>(mapId));

    if (g_mapDB.GetRecord(mapId)) {
        uint32_t now = static_cast<uint32_t>(::time(nullptr));
        CGGameUI::SetLastInstanceTime(now, mapId, ClntObjMgrGetMapID());
    }

    return 1;
}

// VERIFY: c0e57704 manual
// OFFSET: 0x00501800
void ChatWindowInfo::Init() {
    CImVector color;
    color.b = 0;
    color.g = 0;
    color.r = 0;
    color.a = 0x28;

    this->m_name[0] = 0;
    this->m_unkD0 = 0;
    this->m_unkD8 = 1;
    this->m_docked = 0;
    this->m_shown = 0;
    this->m_unkE4 = 0;
    this->m_color = color;
    this->m_channels.SetCount(10);
    this->m_channelIDs.SetCount(10);

    for (uint32_t i = 0; i < 10; i++) {
        this->m_channels.m_data[i] = nullptr;
        this->m_channelIDs.m_data[i] = 0;
    }

    this->m_unkE8 = 6;
    this->m_unkEC = 0.0f;
    this->m_unkF0 = 0.0f;
    this->m_unkF4 = 0;
    this->m_width = 430.0f;
    this->m_height = 120.0f;
}

// VERIFY: c4efb049 manual
// OFFSET: 0x00501C70
int32_t CGChat::OnCheckReloadSpamFilter(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count;
    msg->Get(count);
    s_expectedSpamRecords.SetCount(count);

    uint32_t valid = 0;

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        char expression[512];
        msg->GetString(expression, sizeof(expression));

        if (!regcomp(&s_expectedSpamRecords.m_data[valid], expression, REG_ICASE)) {
            valid++;
        }
    }

    s_expectedSpamRecords.SetCount(valid);
    return 1;
}

// VERIFY: 1e90f0ce manual
// OFFSET: 0x00505930
void CGChat::TranslateMessage(uint32_t language, uint32_t skill, const char* text, char* buffer, uint32_t size, int32_t a6, int32_t a7) {
    if (!text || !buffer) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (!language) {
        SStrCopy(buffer, text, size);
        return;
    }

    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__) || skill >= 300) {
        SStrCopy(buffer, text, size);
        return;
    }

    char stripped[3000];
    StripHyperlinks(text, stripped, sizeof(stripped));

    int32_t syllables = s_locale == 4 || s_locale == 5 ? 2 : 0;
    *buffer = 0;

    const char* current = stripped;
    char word[1024];
    int32_t chars;

    while (true) {
        bool spaced = false;

        for (uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars); c != 0xFFFFFFFF; c = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars)) {
            if (IsAlphaLatin1(static_cast<uint16_t>(c)) || IsNumeric(static_cast<uint16_t>(c)) || c == '\'' || c > 0xFF) {
                break;
            }

            if (c == '<' && a6) {
                for (uint32_t t = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars); t != 0xFFFFFFFF; t = sgetu8(reinterpret_cast<const uint8_t*>(current), &chars)) {
                    int32_t i;

                    for (i = 0; i < chars; i++) {
                        word[i] = current[i];
                    }

                    word[i] = 0;
                    SStrPack(buffer, word, size);
                    current += chars;

                    if (t == '>') {
                        break;
                    }
                }
            } else if (a7) {
                int32_t i;

                for (i = 0; i < chars; i++) {
                    word[i] = current[i];
                }

                word[i] = 0;
                SStrPack(buffer, word, size);
                current += chars;
            } else {
                if (!spaced) {
                    SStrPack(buffer, " ", size);
                    spaced = true;
                }

                current += chars;
            }
        }

        if (sgetu8(reinterpret_cast<const uint8_t*>(current), &chars) == 0xFFFFFFFF) {
            return;
        }

        const char* end = current;
        const char* next = current;
        int32_t count = 0;

        for (uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(end), &chars); c != 0xFFFFFFFF; c = sgetu8(reinterpret_cast<const uint8_t*>(end), &chars)) {
            if (!IsAlphaLatin1(static_cast<uint16_t>(c)) && !IsNumeric(static_cast<uint16_t>(c)) && c != '\'' && c <= 0xFF) {
                break;
            }

            end += chars;

            if (syllables > 0 && ++count >= syllables) {
                SStrPack(buffer, " ", size);

                if (++syllables > 5) {
                    syllables = 2;
                }

                break;
            }
        }

        next = end;
        uint32_t length = end - current;

        if (length >= sizeof(word)) {
            length = sizeof(word);
        }

        SStrCopy(word, current, length + 1);
        uint32_t hash = SStrHash(word, 0, 0);

        if (hash % 300 < skill) {
            SStrPack(buffer, word, size);
        } else {
            if (length >= 18) {
                length = 18;
            }

            HASHKEY_LANGUAGE key;
            key.m_length = length;
            key.m_unk04 = 0;
            WORDLIST* list = s_wordLists.Ptr(language, key);

            while (!list && length != 1) {
                key.m_length = --length;
                key.m_unk04 = 0;
                list = s_wordLists.Ptr(language, key);
            }

            if (list) {
                if (!list->m_words.Count()) {
                    SErrDisplayAppFatalMessage("", "", 0);
                }

                TranslateMessageCase(list->m_words.m_data[hash % list->m_words.Count()]->m_word, buffer, size, word);
            }
        }

        current = next;
    }
}

// VERIFY: 3bdaf6af manual
// OFFSET: 0x00507880
void CGChat::JoinedChannel(uint32_t id, const char* name, uint32_t a3, uint8_t flags) {
    ChatChannel* channel = id ? CGChat::GetChannelByChatChannelID(id) : GetChannelByName(name);

    if (!channel || !channel->m_channelNumber) {
        return;
    }

    uint32_t zoneChannelID = channel->m_zoneChannelID;
    channel->m_unk9C = 0;

    if (zoneChannelID) {
        s_joinedZoneChannels |= 1 << (zoneChannelID - 1);
        SStrCopy(channel->m_name, name, sizeof(channel->m_name));
    }

    channel->m_unk98 = a3;
    channel->m_flags = flags;
    CGChat::OnChannelsUpdated();
}

// VERIFY: e67412a6 manual
// OFFSET: 0x00507900
void CGChat::LeftChannel(uint32_t id, const char* name) {
    ChatChannel* channel = id ? CGChat::GetChannelByChatChannelID(id) : GetChannelByName(name);

    if (!channel) {
        return;
    }

    channel->m_unk9C = 1;

    if (s_activeVoiceChannel) {
        int32_t type;
        const char* voiceChannel = CGChat::GetChannelStringForDisplay(s_activeVoiceChannel, &type);

        if (!type && voiceChannel && !SStrCmpUTF8I(channel->m_name, voiceChannel, STORM_MAX_STR)) {
            if (CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled) {
                FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d", s_activeVoiceChannel + 1, CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled);
            } else {
                FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d", s_activeVoiceChannel + 1);
            }

            s_activeVoiceChannel = 0;
        }
    }

    if (!s_activeDisplayChannelType && !SStrCmpUTF8I(channel->m_name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        memset(CGChat::s_activeDisplayChannelName, 0, sizeof(CGChat::s_activeDisplayChannelName));
        s_activeDisplayChannelType = 4;
        s_pendingUsers.Clear();
        s_activeDisplayChannelFlags = 0;
    }

    CGChat::OnChannelsUpdated();
}

// VERIFY: 16505c71 auto
// OFFSET: 0x00507A30
void CGChat::ResetChannel(uint32_t id, const char* name) {
    ChatChannel* channel = id ? CGChat::GetChannelByChatChannelID(id) : GetChannelByName(name);

    if (channel) {
        channel->m_channelNumber = 0;
        channel->m_name[0] = 0;
        channel->m_zoneChannelID = 0;
        channel->m_unk9C = 1;
        channel->m_unkA0 = 1;
    }

    CGChat::OnChannelsUpdated();
}

// VERIFY: 408f4de4 auto
// OFFSET: 0x0050B010
int32_t CGChat::OnServerFirstAchievement(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char name[96];
    WOWGUID guid;
    uint32_t achievementID;
    uint32_t linked;
    char text[2048];

    msg->GetString(name, sizeof(name));
    msg->Get(guid.m_guid);
    msg->Get(achievementID);
    msg->Get(linked);

    if (linked) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("PLAYER_SERVER_FIRST_ACHIEVEMENT", -1, GENDER_NOT_APPLICABLE), name, name);
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SERVER_FIRST_ACHIEVEMENT", -1, GENDER_NOT_APPLICABLE), name);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_ACHIEVEMENT, name, 0, nullptr, nullptr, nullptr, guid, 0, guid, achievementID, false, nullptr);
    return 1;
}

// VERIFY: d767a536 auto
// OFFSET: 0x0050C3C0
int32_t CGChat::OnZoneUnderAttack(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t areaId;
    msg->Get(reinterpret_cast<uint32_t&>(areaId));

    AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

    if (!area) {
        return 1;
    }

    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("ZONE_UNDER_ATTACK", -1, GENDER_NOT_APPLICABLE), area->m_areaName);

    if (area->m_parentAreaID) {
        areaId = area->m_parentAreaID;
    }

    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (!channel->m_channelNumber || channel->m_unk9C) {
            continue;
        }

        ChatChannelsRec* rec = g_chatChannelsDB.GetRecord(channel->m_zoneChannelID);

        if (rec && (rec->m_flags & 0x10000) && (!(rec->m_flags & 0x2) || areaId == CGGameUI::m_areaID)) {
            CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL, nullptr, 0, channel->m_name, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }
    }

    return 1;
}

// VERIFY: 1ce62758 auto
// OFFSET: 0x0050C810
int32_t CGChat::OnDurabilityDamageDeath(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGChat::AddChatMessage(FrameScript_GetText("DURABILITYDAMAGE_DEATH", -1, GENDER_NOT_APPLICABLE), CHAT_MSG_COMBAT_MISC_INFO, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 2f5725dd auto
// OFFSET: 0x0050C850
int32_t CGChat::OnDefenseMessage(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t areaId;
    uint32_t length;
    void* data;

    msg->Get(reinterpret_cast<uint32_t&>(areaId));
    msg->Get(length);
    msg->GetDataInSitu(data, length);
    const char* text = static_cast<const char*>(data);

    AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

    if (area && area->m_parentAreaID) {
        areaId = area->m_parentAreaID;
    }

    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (!channel->m_channelNumber || channel->m_unk9C) {
            continue;
        }

        ChatChannelsRec* rec = g_chatChannelsDB.GetRecord(channel->m_zoneChannelID);

        if (rec && (rec->m_flags & 0x10000) && (!(rec->m_flags & 0x2) || areaId == CGGameUI::m_areaID)) {
            CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL, nullptr, 0, channel->m_name, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }
    }

    return 1;
}

// VERIFY: 3a6e96ec auto
// OFFSET: 0x0050C980
int32_t CGChat::OnServerMessage(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t id;
    char argument[1024];
    char text[1024];

    msg->Get(reinterpret_cast<uint32_t&>(id));
    msg->GetString(argument, sizeof(argument));

    ServerMessagesRec* rec = g_serverMessagesDB.GetRecord(id);

    if (rec) {
        if (argument[0]) {
            SStrPrintf(text, sizeof(text), rec->m_text, argument);
        } else {
            SStrCopy(text, rec->m_text, sizeof(text));
        }
    } else {
        SStrPrintf(text, sizeof(text), "[%d]: %s", id, argument);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    CKBSystem::CacheServerMessage(text, id);
    return 1;
}

// VERIFY: 2c298b13 auto
// OFFSET: 0x0050CA80
int32_t CGChat::OnRaidInstanceMessage(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t type;
    int32_t mapId;
    int32_t difficulty;
    int32_t seconds;
    char mapName[256];
    char text[3000];

    msg->Get(reinterpret_cast<uint32_t&>(type));
    msg->Get(reinterpret_cast<uint32_t&>(mapId));
    msg->Get(reinterpret_cast<uint32_t&>(difficulty));
    msg->Get(reinterpret_cast<uint32_t&>(seconds));
    CGRaidInfo::GetMapNameWithDifficulty(mapName, sizeof(mapName), mapId, difficulty);

    switch (type) {
        case 1: {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("RAID_INSTANCE_WARNING_HOURS", -1, GENDER_NOT_APPLICABLE), mapName, seconds / 3600);
            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            break;
        }

        case 2: {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("RAID_INSTANCE_WARNING_MIN", -1, GENDER_NOT_APPLICABLE), mapName, seconds / 60);
            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            break;
        }

        case 3: {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("RAID_INSTANCE_WARNING_MIN_SOON", -1, GENDER_NOT_APPLICABLE), mapName, seconds / 60);
            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            break;
        }

        case 4: {
            uint8_t locked;
            uint8_t extended;
            msg->Get(locked);
            msg->Get(extended);
            FrameScript_SignalEvent(SCRIPT_EVENT_RAID_INSTANCE_WELCOME, "%s%d%d%d", mapName, seconds, locked != 0, extended != 0);
            break;
        }

        case 5: {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("RAID_INSTANCE_EXPIRED", -1, GENDER_NOT_APPLICABLE), mapName);
            CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            break;
        }
    }

    return 1;
}

// VERIFY: db1b0757 auto
// OFFSET: 0x0050CCD0
int32_t CGChat::OnInstanceReset(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t mapId;
    char mapName[256];
    char text[3000];

    CGGameUI::ClearInstanceResetMap();
    msg->Get(reinterpret_cast<uint32_t&>(mapId));

    MapRec* map = g_mapDB.GetRecord(mapId);

    if (map) {
        SStrCopy(mapName, map->m_mapName, sizeof(mapName));
    } else {
        SStrPrintf(mapName, sizeof(mapName), "%d", mapId);
    }

    SStrPrintf(text, sizeof(text), FrameScript_GetText("INSTANCE_RESET_SUCCESS", -1, GENDER_NOT_APPLICABLE), mapName);
    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: d58bd1d9 manual
// OFFSET: 0x0050CDA0
int32_t CGChat::OnInstanceResetFailed(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    int32_t reason;
    int32_t mapId;
    char mapName[256];
    char text[3000];

    msg->Get(reinterpret_cast<uint32_t&>(reason));
    msg->Get(reinterpret_cast<uint32_t&>(mapId));

    MapRec* map = g_mapDB.GetRecord(mapId);

    if (map) {
        SStrCopy(mapName, map->m_mapName, sizeof(mapName));
    } else {
        SStrPrintf(mapName, sizeof(mapName), "%d", mapId);
    }

    if (!reason) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("INSTANCE_RESET_FAILED", -1, GENDER_NOT_APPLICABLE), mapName);
    } else if (reason == 1) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("INSTANCE_RESET_FAILED_OFFLINE", -1, GENDER_NOT_APPLICABLE), mapName);
    } else if (reason == 2) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("INSTANCE_RESET_FAILED_ZONING", -1, GENDER_NOT_APPLICABLE), mapName);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 80b6aa89 auto
// OFFSET: 0x0050CEE0
int32_t CGChat::OnResetFailedNotify(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGChat::AddChatMessage(FrameScript_GetText("RESET_FAILED_NOTIFY", -1, GENDER_NOT_APPLICABLE), CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 4fe41964 manual
// OFFSET: 0x0050E120
int32_t CGChat::OnChannelNotify(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t notify;
    char channelName[128];
    char displayName[128];
    char text[3000];
    char extra[516];
    uint8_t flags;
    uint32_t zoneChannelID = 0;
    uint32_t channelFlags;
    uint8_t oldFlags;
    uint8_t newFlags;
    uint8_t suspended;

    msg->Get(notify);
    msg->GetString(channelName, sizeof(channelName));
    SStrCopy(displayName, channelName, sizeof(displayName));

    int32_t type = 0;
    WOWGUID guid = WOWGUID();
    WOWGUID targetGuid = WOWGUID();
    const char* senderName = nullptr;
    text[0] = 0;
    int32_t reset = 0;
    int32_t rebuild = 0;

    switch (notify) {
        case 0: {
            type = CHAT_MSG_CHANNEL_JOIN;
            msg->Get(guid.m_guid);

            if (!CGChat::GetChannelID(channelName)) {
                return 1;
            }

            break;
        }

        case 1: {
            type = CHAT_MSG_CHANNEL_LEAVE;
            msg->Get(guid.m_guid);

            if (!CGChat::GetChannelID(channelName)) {
                return 1;
            }

            break;
        }

        case 2: {
            msg->Get(flags);
            msg->Get(zoneChannelID);
            msg->Get(channelFlags);

            if (msg->IsRead()) {
                extra[0] = 0;
            } else {
                msg->GetString(extra, sizeof(extra));
            }

            type = CHAT_MSG_CHANNEL_NOTICE;
            ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(channelName);

            if (!channel || channel->m_unk9C) {
                SStrCopy(text, "YOU_JOINED", sizeof(text));
            } else {
                SStrCopy(text, "YOU_CHANGED", sizeof(text));

                if (!s_activeDisplayChannelType && !SStrCmpUTF8I(channel->m_name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
                    SetCurrentActiveDisplayChannel(nullptr, 0);
                }
            }

            CGChat::JoinedChannel(zoneChannelID, channelName, channelFlags, flags);
            break;
        }

        case 3: {
            msg->Get(zoneChannelID);
            msg->Get(suspended);
            type = CHAT_MSG_CHANNEL_NOTICE;
            ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(channelName);

            if (channel) {
                if (channel->m_unkA0 || !suspended) {
                    SStrCopy(text, "YOU_LEFT", sizeof(text));
                    reset = 1;
                } else {
                    SStrCopy(text, "SUSPENDED", sizeof(text));
                }
            }

            CGChat::LeftChannel(zoneChannelID, channelName);
            break;
        }

        case 4: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "WRONG_PASSWORD", sizeof(text));
            reset = 1;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_PASSWORD_REQUEST, "%s", displayName);
            break;
        }

        case 5: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "NOT_MEMBER", sizeof(text));
            break;
        }

        case 6: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "NOT_MODERATOR", sizeof(text));
            break;
        }

        case 7: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PASSWORD_CHANGED", sizeof(text));
            break;
        }

        case 8: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "OWNER_CHANGED", sizeof(text));
            break;
        }

        case 9: {
            msg->GetString(extra, sizeof(extra));
            senderName = extra;
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_NOT_FOUND", sizeof(text));
            break;
        }

        case 10: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "NOT_OWNER", sizeof(text));
            break;
        }

        case 11: {
            msg->GetString(extra, sizeof(extra));
            senderName = extra;
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "CHANNEL_OWNER", sizeof(text));
            break;
        }

        case 12: {
            msg->Get(guid.m_guid);
            msg->Get(oldFlags);
            msg->Get(newFlags);
            CGChat::HandleFlagsChanged(guid, oldFlags, newFlags, channelName);
            return 1;
        }

        case 13: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "ANNOUNCEMENTS_ON", sizeof(text));
            break;
        }

        case 14: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "ANNOUNCEMENTS_OFF", sizeof(text));
            break;
        }

        case 15: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "MODERATION_ON", sizeof(text));
            break;
        }

        case 16: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "MODERATION_OFF", sizeof(text));
            break;
        }

        case 17: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "MUTED", sizeof(text));
            break;
        }

        case 18: {
            msg->Get(guid.m_guid);
            msg->Get(targetGuid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_KICKED", sizeof(text));
            break;
        }

        case 19: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "BANNED", sizeof(text));
            reset = 1;
            break;
        }

        case 20: {
            msg->Get(guid.m_guid);
            msg->Get(targetGuid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_BANNED", sizeof(text));
            break;
        }

        case 21: {
            msg->Get(guid.m_guid);
            msg->Get(targetGuid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_UNBANNED", sizeof(text));
            break;
        }

        case 22: {
            msg->GetString(extra, sizeof(extra));
            senderName = extra;
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_NOT_BANNED", sizeof(text));
            break;
        }

        case 23: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_ALREADY_MEMBER", sizeof(text));
            break;
        }

        case 24: {
            msg->Get(guid.m_guid);

            if ((FriendList::s_friendList && FriendList::s_friendList->IsIgnored(guid, true)) || (guid.m_guid & 0xF000000000000000ULL) || !(guid.m_guid & 0xF07FFFFFFFFFFFFFULL) || s_inviteChannelName[0]) {
                return 1;
            }

            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "INVITE", sizeof(text));
            NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, guid, CGChat::InviteNameQueryCallback, nullptr, false);

            if (record) {
                FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_INVITE_REQUEST, "%s%s", displayName, record->m_name);
            } else {
                SStrCopy(s_inviteChannelName, displayName, sizeof(s_inviteChannelName));
            }

            break;
        }

        case 25: {
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "INVITE_WRONG_FACTION", sizeof(text));
            break;
        }

        case 26: {
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "WRONG_FACTION", sizeof(text));
            reset = 1;
            break;
        }

        case 27: {
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "INVALID_NAME", sizeof(text));
            reset = 1;
            break;
        }

        case 28: {
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "NOT_MODERATED", sizeof(text));
            break;
        }

        case 29: {
            msg->GetString(extra, sizeof(extra));
            senderName = extra;
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_INVITED", sizeof(text));
            break;
        }

        case 30: {
            msg->GetString(extra, sizeof(extra));
            senderName = extra;
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "PLAYER_INVITE_BANNED", sizeof(text));
            break;
        }

        case 31: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "THROTTLED", sizeof(text));
            break;
        }

        case 32: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "NOT_IN_AREA", sizeof(text));
            reset = 1;
            break;
        }

        case 33: {
            type = CHAT_MSG_CHANNEL_NOTICE;
            SStrCopy(text, "NOT_IN_LFG", sizeof(text));
            reset = 1;
            break;
        }

        case 34: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "VOICE_ON", sizeof(text));
            ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(channelName);

            if (channel) {
                channel->m_flags |= 0x80;
                rebuild = 1;
            }

            break;
        }

        case 35: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            SStrCopy(text, "VOICE_OFF", sizeof(text));
            ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(channelName);

            if (channel) {
                channel->m_flags &= ~0x80;
                rebuild = 1;
            }

            break;
        }

        case 36: {
            msg->Get(guid.m_guid);
            type = CHAT_MSG_CHANNEL_NOTICE_USER;
            ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(channelName);

            if (channel) {
                channel->m_flags |= 0x80;
                rebuild = 1;
            }

            if (rebuild) {
                RebuildDisplayChannelList(1);
            }

            return 1;
        }
    }

    CGChat::AddChatMessage(text, type, senderName, 0, displayName, nullptr, nullptr, guid, 0, targetGuid, 0, false, nullptr);

    if (reset) {
        CGChat::ResetChannel(zoneChannelID, channelName);
    }

    if (notify == 2 && extra[0]) {
        CGChat::AddChatMessage(extra, type, nullptr, 0, channelName, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (rebuild) {
        RebuildDisplayChannelList(1);
    }

    return 1;
}

// VERIFY: 34edfdb5 manual
// OFFSET: 0x0050EDD0
void CGChat::InitializeGame() {
    for (int32_t i = 0; i < g_languageWordsDB.m_numRecords; i++) {
        LanguageWordsRec* rec = (i < 0 || i >= g_languageWordsDB.m_numRecords) ? nullptr : &g_languageWordsDB.m_records[i];

        if (!rec || !rec->m_word || !*rec->m_word) {
            continue;
        }

        HASHKEY_LANGUAGE key;
        key.m_length = SStrLen(rec->m_word);
        key.m_unk04 = 0;
        WORDLIST* list = s_wordLists.Ptr(rec->m_languageID, key);

        if (!list) {
            list = s_wordLists.New(rec->m_languageID, key, 0, 0);
        }

        *list->m_words.New() = rec;
    }

    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        SStrCopy(s_chatColors[i].m_name, s_chatTypeInfo[i].m_name, sizeof(s_chatColors[i].m_name));
        s_chatColors[i].m_color[0] = s_chatTypeInfo[i].m_color[0];
        s_chatColors[i].m_color[1] = s_chatTypeInfo[i].m_color[1];
        s_chatColors[i].m_color[2] = s_chatTypeInfo[i].m_color[2];
        s_chatColors[i].m_unk48 = s_chatTypeInfo[i].m_unk48;
    }

    s_channelColors.SetCount(10);

    for (uint32_t i = 0; i < 10; i++) {
        SStrPrintf(s_channelColors.m_data[i].m_name, sizeof(s_channelColors.m_data[i].m_name), "CHANNEL%d", i + 1);
        s_channelColors.m_data[i].m_color[0] = s_chatColors[CHAT_MSG_CHANNEL].m_color[0];
        s_channelColors.m_data[i].m_color[1] = s_chatColors[CHAT_MSG_CHANNEL].m_color[1];
        s_channelColors.m_data[i].m_color[2] = s_chatColors[CHAT_MSG_CHANNEL].m_color[2];
        s_channelColors.m_data[i].m_unk48 = s_chatColors[CHAT_MSG_CHANNEL].m_unk48;
    }

    for (uint32_t i = 0; i < sizeof(s_chatWindows) / sizeof(s_chatWindows[0]); i++) {
        s_chatWindows[i].Init();

        if (i == 0) {
            s_chatWindows[i].m_shown = 1;
            s_chatWindows[i].m_docked = 1;
            memset(s_chatWindows[i].m_messageGroups, 1, 40);
        } else if (i == 1) {
            s_chatWindows[i].m_shown = 1;
            s_chatWindows[i].m_docked = 2;

            for (uint32_t j = 40; j < 46; j++) {
                s_chatWindows[i].m_messageGroups[j] = s_chatMessageGroups[j].m_default != 0;
            }
        } else {
            memset(s_chatWindows[i].m_messageGroups, 0, sizeof(s_chatWindows[i].m_messageGroups));
        }
    }

    for (int32_t i = g_areaTableDB.m_numRecords; i;) {
        i--;
        AreaTableRec* area = (i < 0 || i >= g_areaTableDB.m_numRecords) ? nullptr : &g_areaTableDB.m_records[i];

        if (area->m_flags & 0x200) {
            s_cityArea = area;
            break;
        }
    }

    CGChat::m_loaded = 0;
    CGChat::m_registered = 0;
    CGChat::m_complaintsSystemStatus = 0;
    s_joinedZoneChannels = 0;
    memset(CGChat::m_whisperSrcHistory, 0, sizeof(CGChat::m_whisperSrcHistory));
    memset(CGChat::m_whisperDstHistory, 0, sizeof(CGChat::m_whisperDstHistory));
    s_whisperThrottleIndex = 0;

    for (uint32_t i = 0; i < sizeof(s_whisperThrottleTimes) / sizeof(s_whisperThrottleTimes[0]); i++) {
        s_whisperThrottleTimes[i] = 0;
    }

    CGChat::m_voiceChatSystemStatus = 0;
    memset(CGChat::s_displayChannels, 0, sizeof(CGChat::s_displayChannels));
    s_numDisplayChannels = 0;
    memset(CGChat::s_activeDisplayChannelName, 0, sizeof(CGChat::s_activeDisplayChannelName));
    s_activeDisplayChannelType = 4;
    s_pendingUserListActive = false;
    s_displayChannelMemberCount = 0;
    s_pendingUsers.Clear();
    s_activeDisplayChannelFlags = 0;
    memset(s_collapsedHeaders, 0, sizeof(s_collapsedHeaders));
    s_activeVoiceChannel = 0;
    s_inviteChannelName[0] = 0;

    ClientServices::SetMessageHandler(SMSG_CHANNEL_NOTIFY, CGChat::OnChannelNotify, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHANNEL_LIST, CGChat::OnChannelList, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT, CGChat::OnMessageChat, nullptr);
    ClientServices::SetMessageHandler(SMSG_TEXT_EMOTE, CGChat::OnTextEmote, nullptr);
    ClientServices::SetMessageHandler(SMSG_ZONE_UNDER_ATTACK, CGChat::OnZoneUnderAttack, nullptr);
    ClientServices::SetMessageHandler(SMSG_DEFENSE_MESSAGE, CGChat::OnDefenseMessage, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHAT_SERVER_MESSAGE, CGChat::OnServerMessage, nullptr);
    ClientServices::SetMessageHandler(SMSG_RAID_INSTANCE_INFO, CGChat::OnRaidInstanceInfo, nullptr);
    ClientServices::SetMessageHandler(SMSG_RAID_INSTANCE_MESSAGE, CGChat::OnRaidInstanceMessage, nullptr);
    ClientServices::SetMessageHandler(SMSG_INSTANCE_RESET, CGChat::OnInstanceReset, nullptr);
    ClientServices::SetMessageHandler(SMSG_INSTANCE_RESET_FAILED, CGChat::OnInstanceResetFailed, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_LAST_INSTANCE, CGChat::OnUpdateInstance, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_INSTANCE_OWNERSHIP, CGChat::OnUpdateInstanceOwnership, nullptr);
    ClientServices::SetMessageHandler(SMSG_EXPECTED_SPAM_RECORDS, CGChat::OnCheckReloadSpamFilter, nullptr);
    ClientServices::SetMessageHandler(SMSG_TITLE_EARNED, CGChat::OnTitleEarned, nullptr);
    ClientServices::SetMessageHandler(SMSG_RESET_FAILED_NOTIFY, CGChat::OnResetFailedNotify, nullptr);
    ClientServices::SetMessageHandler(SMSG_GM_MESSAGECHAT, CGChat::OnGMMessageChat, nullptr);
    ClientServices::SetMessageHandler(SMSG_LOG_XP_GAIN, Packet_SMSG_LOG_XPGAIN, nullptr);
    ClientServices::SetMessageHandler(SMSG_DURABILITY_DAMAGE_DEATH, CGChat::OnDurabilityDamageDeath, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHANNEL_MEMBER_COUNT, CGChat::OnChannelMemberCount, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMSAT_RECONNECT_TRY, CGChat::OnComsatReconnectAttempt, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMSAT_DISCONNECT, CGChat::OnComsatDisconnect, nullptr);
    ClientServices::SetMessageHandler(SMSG_COMSAT_CONNECT_FAIL, CGChat::OnComsatConnectFail, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICE_CHAT_STATUS, CGChat::OnVoiceStatusChanged, nullptr);
    ClientServices::SetMessageHandler(SMSG_USERLIST_ADD, CGChat::OnUserListAdded, nullptr);
    ClientServices::SetMessageHandler(SMSG_USERLIST_REMOVE, CGChat::OnUserListRemove, nullptr);
    ClientServices::SetMessageHandler(SMSG_USERLIST_UPDATE, CGChat::OnUserListUpdate, nullptr);
    ClientServices::SetMessageHandler(SMSG_VOICESESSION_FULL, CGChat::OnVoicesessionFull, nullptr);
    ClientServices::SetMessageHandler(SMSG_SERVER_FIRST_ACHIEVEMENT, CGChat::OnServerFirstAchievement, nullptr);
}

// VERIFY: c17ad011 auto
// OFFSET: 0x0050B660
void CGChat::OnLogXPGain(int32_t amount) {
    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("COMBATLOG_XPLOSS_FIRSTPERSON_UNNAMED", -1, GENDER_NOT_APPLICABLE), -amount);
    CGChat::AddChatMessage(text, 33, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: d2c71361 manual
// OFFSET: 0x0050B0F0
void CGChat::InformTargetIcon(WOWGUID setter, WOWGUID target, int32_t index) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(setter.m_guid, none, nullptr, nullptr, false);
    const char* unitName = unit->GetVisibleUnitName(nullptr, 1);

    if (!unitName) {
        unitName = FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);
    }

    if (name) {
        char buffer[3000];
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("TARGET_ICON_SET", -1, GENDER_NOT_APPLICABLE), name->m_name, name->m_name, index + 1, unitName);
        CGChat::AddChatMessage(buffer, 52, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }
}

// VERIFY: bb42bd2f manual
// OFFSET: 0x0050B1D0
static void UnitCombatLogShowXPGained(CGUnit_C* unit, int32_t amount, float groupMult) {
    char text[3000];
    int32_t base = static_cast<int32_t>(amount / groupMult);

    if (unit && (unit->m_obj->m_type & TYPEMASK_UNIT)) {
        if (base == amount) {
            SStrPrintf(text, sizeof(text), FrameScript_GetText("COMBATLOG_XPGAIN_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE), unit->GetUnitName(nullptr, 1), amount);
        } else {
            const char* token = base >= amount ? "COMBATLOG_XPGAIN_FIRSTPERSON_RAID" : "COMBATLOG_XPGAIN_FIRSTPERSON_GROUP";
            SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), unit->GetUnitName(nullptr, 1), amount, abs(amount - base));
        }
    } else if (base == amount) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("COMBATLOG_XPGAIN_FIRSTPERSON_UNNAMED", -1, GENDER_NOT_APPLICABLE), amount);
    } else {
        const char* token = base >= amount ? "COMBATLOG_XPGAIN_FIRSTPERSON_UNNAMED_RAID" : "COMBATLOG_XPGAIN_FIRSTPERSON_UNNAMED_GROUP";
        SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), amount, abs(amount - base));
    }

    CGChat::AddChatMessage(text, 33, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// STATIC: 0x00AC7FB4
static const char* s_exhaustionTokens[7] = { nullptr, "COMBATLOG_XPGAIN_EXHAUSTION1", "COMBATLOG_XPGAIN_EXHAUSTION2", "NULL", "COMBATLOG_XPGAIN_EXHAUSTION4", "COMBATLOG_XPGAIN_EXHAUSTION5", "COMBATLOG_XPGAIN_EXHAUSTION1" };

// VERIFY: 610143a3 manual
// OFFSET: 0x0050B300
void CGChat::LogXPGain(CGUnit_C* unit, int32_t amount, int32_t type, int32_t bonus, float groupMult, bool referAFriend) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->AddWorldXPGainText(amount);
    }

    if (unit && !type) {
        if (!player) {
            return;
        }

        uint32_t restState = player->m_player->m_playerBytes2[3];

        if (referAFriend) {
            restState = 6;
        }

        ExhaustionRec* exhaustion = g_exhaustionDB.GetRecord(restState);

        if (exhaustion && !fequal(exhaustion->m_factor, 0.0f) && !fequal(exhaustion->m_factor, 1.0f)) {
            if (restState >= 7) {
                SErrDisplayAppFatalMessage("", "", 0);
            }

            char token[256];
            int32_t base = static_cast<int32_t>(bonus / groupMult);

            if (base == bonus) {
                SStrCopy(token, s_exhaustionTokens[restState], sizeof(token));
            } else if (base < bonus) {
                SStrPrintf(token, sizeof(token), "%s_GROUP", s_exhaustionTokens[restState]);
            } else {
                SStrPrintf(token, sizeof(token), "%s_RAID", s_exhaustionTokens[restState]);
            }

            char bonusText[32];
            SStrPrintf(bonusText, sizeof(bonusText), "%s%d", amount - bonus >= 0 ? "+" : "", amount - bonus);
            char text[3000];

            if (base == bonus) {
                SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), unit->GetUnitName(nullptr, 1), amount, bonusText, exhaustion->m_name);
            } else {
                SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), unit->GetUnitName(nullptr, 1), amount, bonusText, exhaustion->m_name, abs(bonus - base));
            }

            CGChat::AddChatMessage(text, 33, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            return;
        }

        UnitCombatLogShowXPGained(unit, amount, groupMult);
        return;
    }

    if (!referAFriend || (type != 1 && type != 6)) {
        UnitCombatLogShowXPGained(unit, amount, groupMult);
        return;
    }

    int32_t rafBonus = CMath::fint_(amount * 0.6666667f);
    char bonusText[32];
    SStrPrintf(bonusText, sizeof(bonusText), "%s%d", rafBonus >= 0 ? "+" : "", rafBonus);
    ExhaustionRec* exhaustion = g_exhaustionDB.GetRecord(6);
    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("COMBATLOG_XPGAIN_QUEST", -1, GENDER_NOT_APPLICABLE), amount, bonusText, exhaustion->m_name, rafBonus);
    CGChat::AddChatMessage(text, 33, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 294836e5 manual
// OFFSET: 0x0050C720
int32_t Packet_SMSG_LOG_XPGAIN(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid = WOWGUID();
    msg->Get(guid.m_guid);
    uint32_t amount = 0;
    msg->Get(amount);
    uint8_t type = 0;
    msg->Get(type);
    float groupMult = 1.0f;
    uint32_t bonus = 0;

    if (!type) {
        msg->Get(bonus);
        msg->Get(groupMult);
    }

    uint8_t referAFriend = 0;
    msg->Get(referAFriend);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (guid == ClntObjMgrGetActivePlayer()) {
        CGChat::OnLogXPGain(amount);
    } else {
        CGChat::LogXPGain(unit, amount, type, bonus, groupMult, referAFriend != 0);
    }

    return 1;
}

// VERIFY: 743763ce manual
// OFFSET: 0x00577F60
bool InebriatedParseText(const char* text, char* buffer, uint32_t size, float drunkness) {
    if (!text) {
        return false;
    }

    char* dest = buffer;
    uint32_t remaining = size;
    bool wordStart = true;

    while (*text && remaining > 1) {
        char c = *text;
        *dest++ = c;
        remaining--;
        text++;

        switch (c) {
            case ' ': {
                wordStart = true;
                continue;
            }

            case 'S':
            case 's': {
                if (wordStart) {
                    if (remaining > 1 && *text != ' ' && *text != 'S' && *text != 's' && CRandom::ufloat_(g_rndSeed) < drunkness) {
                        char word[260];
                        word[0] = 's';
                        word[1] = 'h';
                        uint32_t length = 2;

                        for (const char* source = text; *source && *source != ' '; source++) {
                            word[length++] = *source;
                        }

                        word[length] = 0;

                        if (!ProfanityFilter(word, false, true)) {
                            *dest++ = 'h';
                            remaining--;
                        }
                    }
                } else if (remaining > 1 && *text == ' ' && CRandom::ufloat_(g_rndSeed) < drunkness) {
                    *dest++ = 'h';
                    remaining--;
                }

                break;
            }

            case '|': {
                if (*text == 'c') {
                    const char* end = SStrStr(text, "|r");

                    if (end) {
                        uint32_t length = end - text + 2;

                        if (length >= remaining) {
                            length = remaining - 1;
                        }

                        SStrCopy(dest, text, length + 1);
                        dest += length;
                        remaining -= length;
                        text = end + 2;
                    }
                }

                break;
            }
        }

        wordStart = false;
    }

    *dest = 0;

    if (drunkness * 0.25f > CRandom::ufloat_(g_rndSeed)) {
        char* copy = static_cast<char*>(alloca(size));
        SStrCopy(copy, buffer, STORM_MAX_STR);
        SStrPrintf(buffer, size, FrameScript_GetText("SLURRED_SPEECH", -1, GENDER_NOT_APPLICABLE), copy);
    }

    return true;
}

// VERIFY: ea4fbe56 manual
// OFFSET: 0x00503660
void CGChat::AddTextEmoteMessage(const WOWGUID& guid, uint32_t emote, const char* target, uint32_t emoteNum) {
    if (!target) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (s_queueChat) {
        CGChat::QueueTextEmote(guid, emote, target, 1, emoteNum);
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    WOWGUID requester = WOWGUID();
    NameCache* sender = g_nameDBCache.GetRecord(guid.m_guid, requester, nullptr, nullptr, false);

    if (!sender) {
        return;
    }

    EmotesTextRec* emoteText = g_emotesTextDB.GetRecord(emote);

    if (!emoteText) {
        return;
    }

    uint32_t index = 0;
    int32_t hasSender = 1;
    int32_t hasTarget = 1;

    if (guid.m_guid == ClntObjMgrGetActivePlayer().m_guid) {
        index = 2;
        hasSender = 0;
    } else if (!SStrCmp(target, player->GetUnitName(nullptr, 1), STORM_MAX_STR)) {
        index = 1;
        hasTarget = 0;
    }

    if (!*target) {
        index |= 4;
    }

    uint32_t gender = sender->m_sex != 1 ? 0 : 8;
    EmotesTextDataRec* text = g_emotesTextDataDB.GetRecord(emoteText->m_emoteText[index | gender]);

    if (!text || !*text->m_text) {
        text = g_emotesTextDataDB.GetRecord(emoteText->m_emoteText[index]);
    }

    if (!text || !*text->m_text) {
        index = (index & ~5) | 4;
        text = g_emotesTextDataDB.GetRecord(emoteText->m_emoteText[index | gender]);
    }

    if (!text || !*text->m_text) {
        text = g_emotesTextDataDB.GetRecord(emoteText->m_emoteText[index]);
    }

    if (text && *text->m_text) {
        char buffer[256];

        if (hasSender) {
            if (hasTarget) {
                SStrPrintf(buffer, sizeof(buffer), text->m_text, sender->m_name, target);
            } else {
                SStrPrintf(buffer, sizeof(buffer), text->m_text, sender->m_name);
            }
        } else if (hasTarget) {
            SStrPrintf(buffer, sizeof(buffer), text->m_text, target);
        } else {
            SStrPrintf(buffer, sizeof(buffer), text->m_text);
        }

        const char* languageName = "";
        LanguagesRec* language = g_languagesDB.GetRecord(0);

        if (language) {
            languageName = language->m_name;
        }

        char guidString[20] = {};

        if (!(guid.m_guid & 0xF000000000000000ULL) && (guid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            GUIDToHexString(guid, guidString);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_CHAT_MSG_TEXT_EMOTE, "%s%s%s%s%s%s%d%d%s%d%d%s%u", buffer, sender->m_name, languageName, "", "", "", 0, 0, "", 0, 0, guidString, 0);

        if (s_chatLogging[0]) {
            SLogWrite(s_chatLog[0], "%s", buffer);
        }
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (unit) {
        unit->PlayTextEmoteSound(emote, emoteNum);
    }
}

// VERIFY: d2a42dd2 auto
// OFFSET: 0x00504070
int32_t CGChat::OnTextEmote(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint32_t emote;
    uint32_t emoteNum;
    uint32_t length;
    void* data;

    msg->Get(guid.m_guid);
    msg->Get(emote);
    msg->Get(emoteNum);
    msg->Get(length);
    msg->GetDataInSitu(data, length);
    const char* target = static_cast<const char*>(data);

    if (FriendList::s_friendList && FriendList::s_friendList->IsIgnored(guid, true)) {
        return 1;
    }

    if (!g_nameDBCache.GetRecord(guid.m_guid, guid, CGChat::TextEmoteNameQueryCallback, nullptr, false)) {
        CGChat::QueueTextEmote(guid, emote, target, 0, emoteNum);
        return 1;
    }

    CGChat::AddTextEmoteMessage(guid, emote, target, emoteNum);
    return 1;
}

// VERIFY: 8e08498f manual
// OFFSET: 0x00505DC0
int32_t CGChat::OnChannelList(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t display = 0;
    msg->Get(display);

    if (display) {
        return CGChat::OnDisplayChannelList(msg);
    }

    char name[128];
    msg->GetString(name, sizeof(name));
    ChatChannel* channel = GetChannelByName(name);

    if (!channel) {
        msg->m_read = msg->m_size;
        return 1;
    }

    msg->Get(channel->m_flags);

    uint32_t count;
    msg->Get(count);

    int32_t pending = 0;
    s_pendingUserListActive = false;

    for (int32_t i = 0; i < static_cast<int32_t>(count); i++) {
        PENDINGUSERLIST* user = channel->m_pendingUsers.NewNode(2, 0, 0);
        msg->Get(user->m_guid.m_guid);
        msg->Get(user->m_flags);

        if (!g_nameDBCache.GetRecord(user->m_guid.m_guid, user->m_guid, CGChat::NameQueryCallback, nullptr, false)) {
            pending++;
            s_pendingUserListActive = true;
        }
    }

    if (!pending) {
        CGChat::DisplayPendingUserList(channel);
    }

    return 1;
}

// VERIFY: 21e5990a auto
// OFFSET: 0x004FB530
void CGChat::UpdateLanguages() {
    FrameScript_SignalEvent(SCRIPT_EVENT_LANGUAGE_LIST_CHANGED, nullptr);
}

// VERIFY: ab07ffc0 manual
// OFFSET: 0x004FE1C0
const char* CGChat::GetChannelStringForDisplay(int32_t session, int32_t* type) {
    *type = 4;

    if (static_cast<uint32_t>(session) >= s_numDisplayChannels) {
        return nullptr;
    }

    int32_t displayType = CGChat::s_displayChannels[session].m_type;

    if (displayType) {
        if (displayType == 2 || displayType == 3 || displayType == 1) {
            *type = displayType;
            return "";
        }

        return nullptr;
    }

    int32_t index = CGChat::s_displayChannels[session].m_channelIndex;

    if (index < 0 || static_cast<uint32_t>(index) >= CGChat::s_channels.Count()) {
        return nullptr;
    }

    int32_t number = CGChat::s_channels.m_data[index].m_channelNumber;

    if (number < 1 || static_cast<uint32_t>(number) > CGChat::s_channels.Count()) {
        return nullptr;
    }

    ChatChannel* channel = &CGChat::s_channels.m_data[number - 1];

    if (channel->m_channelNumber != number) {
        return nullptr;
    }

    *type = 0;
    return channel->m_name;
}

// VERIFY: 5db308f7 manual
// OFFSET: 0x00501710
PENDINGCHAT::~PENDINGCHAT() {
    if (this->m_text) {
        SMemFree(this->m_text, __FILE__, __LINE__, 0);
    }

    if (this->m_senderName) {
        SMemFree(this->m_senderName, __FILE__, __LINE__, 0);
    }

    if (this->m_conversation) {
        delete this->m_conversation;
    }
}

// VERIFY: 4dc0f51c manual
// OFFSET: 0x005017A0
PENDINGTEXTEMOTE::~PENDINGTEXTEMOTE() {
    if (this->m_target) {
        SMemFree(this->m_target, __FILE__, __LINE__, 0);
    }
}

// VERIFY: ab068cfe manual
// OFFSET: 0x00502EA0
void CGChat::QueueTextEmote(const WOWGUID& guid, uint32_t emote, const char* target, int32_t queued, uint32_t emoteNum) {
    PENDINGTEXTEMOTE* pending = CGChat::s_pendingTextEmotes.NewNode(2, 0, 0);
    pending->m_guid = guid;
    pending->m_emote = emote;
    pending->m_target = SStrDupA(target, __FILE__, __LINE__);
    pending->m_queued = queued;
    pending->m_emoteNum = emoteNum;
}

// VERIFY: 268ab1ec manual
// OFFSET: 0x00503C80
void CGChat::NameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PENDINGCHAT* next;

    for (PENDINGCHAT* chat = CGChat::s_pendingChat.Head(); !CGChat::s_pendingChat.IsEnd(chat); chat = next) {
        next = CGChat::s_pendingChat.RawNext(chat);

        if (chat->m_queued) {
            continue;
        }

        if (chat->m_senderGuid != guid && chat->m_targetGuid != guid) {
            continue;
        }

        const char* senderName = nullptr;
        const char* targetName = nullptr;
        WOWGUID senderGuid = chat->m_senderGuid;

        if (!(senderGuid.m_guid & 0xF000000000000000ULL) && (senderGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            WOWGUID requester = WOWGUID();
            NameCache* record = g_nameDBCache.GetRecord(senderGuid.m_guid, requester, nullptr, nullptr, false);

            if (record) {
                senderName = record->m_name;
            }
        } else if ((senderGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((senderGuid.m_guid >> 24) & 0xFFFFFFF), chat->m_senderGuid, nullptr, nullptr, false);

            if (record) {
                senderName = record->m_name;
            }
        }

        WOWGUID targetGuid = chat->m_targetGuid;

        if (!(targetGuid.m_guid & 0xF000000000000000ULL) && (targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            WOWGUID requester = WOWGUID();
            NameCache* record = g_nameDBCache.GetRecord(targetGuid.m_guid, requester, nullptr, nullptr, false);

            if (record) {
                targetName = record->m_name;
            }
        } else if ((targetGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((targetGuid.m_guid >> 24) & 0xFFFFFFF), chat->m_targetGuid, nullptr, nullptr, false);

            if (record) {
                targetName = record->m_name;
            }
        }

        if ((!static_cast<uint32_t>(senderGuid.m_guid) || senderName || chat->m_senderName) && (!chat->m_targetGuid || targetName || chat->m_targetName)) {
            if (s_queueChat) {
                chat->m_queued = 1;
                continue;
            }

            CGChat::AddChatMessage(chat->m_text, chat->m_type, chat->m_senderName ? chat->m_senderName : senderName, chat->m_language, chat->m_channelName, chat->m_targetName ? chat->m_targetName : targetName, chat->m_flags, senderGuid, chat->m_senderUnk, chat->m_targetGuid, chat->m_achievementID, chat->m_isGM, chat->m_conversation);
            CGChat::s_pendingChat.DeleteNode(chat);
        } else if ((senderGuid == guid && !senderName) || (chat->m_targetGuid == guid && !targetName)) {
            CGChat::s_pendingChat.DeleteNode(chat);
        }
    }

    if (!s_pendingUserListActive) {
        return;
    }

    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];
        PENDINGUSERLIST* user = channel->m_pendingUsers.Head();

        if (!user) {
            continue;
        }

        while (!channel->m_pendingUsers.IsEnd(user)) {
            if (user->m_guid == guid && !success) {
                user = channel->m_pendingUsers.DeleteNode(user);
            } else {
                user = channel->m_pendingUsers.RawNext(user);
            }
        }

        CGChat::DisplayPendingUserList(channel);
    }
}

// VERIFY: 52e22629 manual
// OFFSET: 0x00503FB0
void CGChat::TextEmoteNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    PENDINGTEXTEMOTE* next;

    for (PENDINGTEXTEMOTE* pending = CGChat::s_pendingTextEmotes.Head(); !CGChat::s_pendingTextEmotes.IsEnd(pending); pending = next) {
        next = CGChat::s_pendingTextEmotes.RawNext(pending);

        if (pending->m_queued || pending->m_guid != guid) {
            continue;
        }

        WOWGUID requester = WOWGUID();

        if (g_nameDBCache.GetRecord(pending->m_guid.m_guid, requester, nullptr, nullptr, false)) {
            if (s_queueChat) {
                pending->m_queued = 1;
                continue;
            }

            CGChat::AddTextEmoteMessage(pending->m_guid, pending->m_emote, pending->m_target, pending->m_emoteNum);
        }

        CGChat::s_pendingTextEmotes.DeleteNode(pending);
    }
}

// VERIFY: dd511458 manual
// OFFSET: 0x00504490
void SetCurrentActiveDisplayChannel(const char* name, int32_t type) {
    if (!name) {
        memset(CGChat::s_activeDisplayChannelName, 0, sizeof(CGChat::s_activeDisplayChannelName));
        s_activeDisplayChannelType = 4;
        s_pendingUsers.Clear();
        s_activeDisplayChannelFlags = 0;
        return;
    }

    if (type == s_activeDisplayChannelType && (type || !SStrCmpUTF8I(name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR))) {
        return;
    }

    SStrCopy(CGChat::s_activeDisplayChannelName, name, sizeof(CGChat::s_activeDisplayChannelName));
    s_activeDisplayChannelType = type;
    s_pendingUsers.Clear();
    s_activeDisplayChannelFlags = 0;

    if (!type) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_SET_CHANNEL_WATCH));
        msg.PutString(CGChat::s_activeDisplayChannelName);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }
}

// VERIFY: 0d38fbc2 manual
// OFFSET: 0x005045C0
uint32_t BuildPartyDisplayList() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    s_pendingUsers.Clear();

    if (!CGPartyInfo::m_members[0]) {
        return 0;
    }

    s_pendingUsers.SetCount(CGPartyInfo::GetNumPartyMembers() + 1);
    PENDINGUSERLIST* self = s_pendingUsers.m_data;
    WOWGUID guid = player->m_obj->m_guid;
    UpdateVoiceMemberFlags(self, guid == CGPartyInfo::m_leader ? 5 : 4, guid, 2, "");
    s_activeDisplayChannelFlags = self->m_flags;
    uint32_t count = 1;

    for (uint32_t i = 0; i < 5; i++) {
        if (!GetPartyMemberGUID(i)) {
            continue;
        }

        uint8_t flags = GetPartyMemberGUID(i) == CGPartyInfo::m_leader ? 5 : 4;
        UpdateVoiceMemberFlags(&s_pendingUsers.m_data[count], flags, GetPartyMemberGUID(i), 2, "");
        count++;
    }

    qsort(s_pendingUsers.m_data, count, sizeof(PENDINGUSERLIST), QSortDisplayListMembers);
    return count;
}

// VERIFY: c259157e manual
// OFFSET: 0x00504720
uint32_t BuildRaidDisplayList(int32_t type) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    s_pendingUsers.Clear();

    if (!CGRaidInfo::m_numMembers) {
        return 0;
    }

    s_pendingUsers.SetCount(CGRaidInfo::m_numMembers);

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (!member) {
            continue;
        }

        PENDINGUSERLIST* user = &s_pendingUsers.m_data[i];
        uint8_t flags = 4;

        if (member->m_rank == 2) {
            flags = 7;
        } else if (member->m_rank == 1) {
            flags = 6;
        }

        UpdateVoiceMemberFlags(user, flags, member->m_guid, type, "");

        if (user->m_guid == player->m_obj->m_guid) {
            s_activeDisplayChannelFlags = user->m_flags;
        }
    }

    qsort(s_pendingUsers.m_data, CGRaidInfo::m_numMembers, sizeof(PENDINGUSERLIST), QSortDisplayListMembers);
    return CGRaidInfo::m_numMembers;
}

// VERIFY: 9dd5e9ea manual
// OFFSET: 0x00504840
void RebuildDisplayChannelList(int32_t signal) {
    memset(CGChat::s_displayChannels, 0, sizeof(CGChat::s_displayChannels));

    for (uint32_t i = 0; i < sizeof(CGChat::s_displayChannels) / sizeof(CGChat::s_displayChannels[0]); i++) {
        CGChat::s_displayChannels[i].m_headerIndex = -1;
    }

    int32_t battleground;
    int32_t party;
    int32_t raid;

    if (CGRaidInfo::m_numMembers && CGRaidInfo::IsPVPRaid()) {
        battleground = 1;
        party = ComSatClient_HasSession(2, "") || CGPartyInfo::m_members[0];
        raid = ComSatClient_HasSession(3, "");
    } else {
        battleground = 0;
        party = CGPartyInfo::m_members[0].m_guid != 0;
        raid = CGRaidInfo::m_numMembers != 0;
    }

    uint32_t count = 1;
    DisplayChannel* group = &CGChat::s_displayChannels[0];
    group->m_channelIndex = 0;
    group->m_headerIndex = 0;
    group->m_collapsed = s_collapsedHeaders[0];
    group->m_flags = 0;
    group->m_type = 4;
    group->m_voiceEnabled = 0;
    group->m_count = party;

    if (raid) {
        group->m_count++;
    }

    if (battleground) {
        group->m_count++;
    }

    if (!s_collapsedHeaders[0]) {
        if (party && !raid) {
            DisplayChannel* entry = &CGChat::s_displayChannels[1];
            entry->m_channelIndex = -1;
            entry->m_headerIndex = -1;
            entry->m_collapsed = 0;
            entry->m_flags = 0;
            entry->m_enabled = 1;
            entry->m_type = 2;
            entry->m_voiceEnabled = ComSatClient_HasSession(2, "");
            entry->m_count = CGPartyInfo::GetNumPartyMembers() + 1;

            if (!s_collapsedHeaders[0]) {
                group->m_collapsed = 0;
            }

            count = 2;
        } else if (raid) {
            DisplayChannel* entry = &CGChat::s_displayChannels[1];
            entry->m_channelIndex = -2;
            entry->m_headerIndex = -1;
            entry->m_collapsed = 0;
            entry->m_flags = 0;
            entry->m_enabled = 1;
            entry->m_type = 3;
            entry->m_voiceEnabled = ComSatClient_HasSession(3, "");
            entry->m_count = CGRaidInfo::m_numMembers;

            if (!s_collapsedHeaders[0]) {
                group->m_collapsed = 0;
            }

            count = 2;
        }

        if (battleground) {
            DisplayChannel* entry = &CGChat::s_displayChannels[count];
            entry->m_channelIndex = -3;
            entry->m_headerIndex = -1;
            entry->m_collapsed = 0;
            entry->m_flags = 0;
            entry->m_enabled = 1;
            entry->m_type = 1;
            entry->m_voiceEnabled = ComSatClient_HasSession(1, "");
            entry->m_count = CGRaidInfo::m_numMembers;

            if (!s_collapsedHeaders[0]) {
                group->m_collapsed = 0;
            }

            count++;
        }
    }

    int32_t zoneHeader = -1;
    int32_t customHeader = -1;

    for (uint32_t header = 1; header < 3; header++) {
        DisplayChannel* entry = &CGChat::s_displayChannels[count];
        entry->m_channelIndex = 0;
        entry->m_headerIndex = header;
        entry->m_collapsed = s_collapsedHeaders[header];
        entry->m_flags = 0;
        entry->m_enabled = 0;
        entry->m_type = 4;
        entry->m_voiceEnabled = 0;
        entry->m_count = 0;

        if (header == 1) {
            zoneHeader = count;
        } else if (header == 2) {
            customHeader = count;
        }

        count++;

        for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
            ChatChannel* channel = &CGChat::s_channels.m_data[i];

            if (!channel->m_channelNumber) {
                continue;
            }

            int32_t parent;

            if (channel->m_zoneChannelID && header == 1) {
                if (!s_collapsedHeaders[1]) {
                    entry = &CGChat::s_displayChannels[count++];
                    entry->m_flags |= 1;
                    entry->m_collapsed = 0;
                    entry->m_channelIndex = i;
                    entry->m_headerIndex = -1;
                    entry->m_enabled = channel->m_unk9C == 0;
                    entry->m_type = 0;
                    entry->m_voiceEnabled = 0;
                    entry->m_count = 0;

                    if (zoneHeader < 0) {
                        continue;
                    }

                    CGChat::s_displayChannels[zoneHeader].m_collapsed = 0;
                }

                parent = zoneHeader;
            } else {
                if (channel->m_zoneChannelID || header != 2) {
                    continue;
                }

                if (!s_collapsedHeaders[2]) {
                    if (channel->m_unkA0 == 1) {
                        continue;
                    }

                    entry = &CGChat::s_displayChannels[count++];
                    entry->m_channelIndex = i;
                    entry->m_collapsed = 0;
                    entry->m_headerIndex = -1;
                    entry->m_flags = 0;
                    entry->m_enabled = channel->m_unkA0 == 0;
                    entry->m_type = 0;
                    entry->m_voiceEnabled = ComSatClient_HasSession(0, channel->m_name);
                    entry->m_count = 0;

                    if (customHeader < 0) {
                        continue;
                    }

                    CGChat::s_displayChannels[customHeader].m_collapsed = 0;
                }

                parent = customHeader;
            }

            if (parent >= 0) {
                CGChat::s_displayChannels[parent].m_count++;
            }
        }
    }

    s_numDisplayChannels = count;
    s_activeVoiceChannel = 0;

    int32_t sessionType = ComSatClient_GetCurrentSessionType();
    const char* sessionName = sessionType ? "" : ComSatClient_GetCurrentSessionName();

    for (uint32_t i = 0; i < s_numDisplayChannels; i++) {
        DisplayChannel* entry = &CGChat::s_displayChannels[i];
        const char* name = entry->m_type ? "" : CGChat::s_channels.m_data[entry->m_channelIndex].m_name;

        if (sessionType == entry->m_type && (sessionType || (sessionName && name && !SStrCmpUTF8I(sessionName, name, STORM_MAX_STR)))) {
            s_activeVoiceChannel = i;
            break;
        }
    }

    int32_t type = s_activeDisplayChannelType;

    if (type == 2) {
        type = BuildPartyDisplayList();
    }

    if (type == 3 || type == 1) {
        BuildRaidDisplayList(type);
    }

    if (signal) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_UI_UPDATE, nullptr);
    }
}

// VERIFY: e86d74b1 manual
// OFFSET: 0x00505D40
void CGChat::OnChannelsUpdated() {
    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (!channel->m_channelNumber || channel->m_unk9C) {
            continue;
        }

        ChatChannelsRec* rec = g_chatChannelsDB.GetRecord(channel->m_zoneChannelID);

        if (rec && (rec->m_flags & 0x10000) && (rec->m_flags & 0x2)) {
            break;
        }
    }

    RebuildDisplayChannelList(1);
}

// VERIFY: 2fc2c58a manual
// OFFSET: 0x0050CFC0
void CGChat::HandleFlagsChanged(WOWGUID guid, uint8_t oldFlags, uint8_t newFlags, const char* channel) {
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, guid, CGChat::NameQueryCallback, nullptr, false);

    if (!record) {
        return;
    }

    if (guid == ClntObjMgrGetActivePlayer() && !s_activeDisplayChannelType && channel && !SStrCmpUTF8I(channel, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        s_activeDisplayChannelFlags = newFlags;
    }

    if (!(oldFlags & 0x2) && (newFlags & 0x2)) {
        CGChat::AddChatMessage("SET_MODERATOR", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else if ((oldFlags & 0x2) && !(newFlags & 0x2)) {
        CGChat::AddChatMessage("UNSET_MODERATOR", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (!(oldFlags & 0x4) && (newFlags & 0x4)) {
        CGChat::AddChatMessage("SET_VOICE", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else if ((oldFlags & 0x4) && !(newFlags & 0x4)) {
        CGChat::AddChatMessage("UNSET_VOICE", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    if (!(oldFlags & 0x10) && (newFlags & 0x10)) {
        CGChat::AddChatMessage("SET_SPEAK", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else if ((oldFlags & 0x10) && !(newFlags & 0x10)) {
        CGChat::AddChatMessage("UNSET_SPEAK", CHAT_MSG_CHANNEL_NOTICE_USER, record->m_name, 0, channel, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }
}

// VERIFY: 4866cbb1 manual
// OFFSET: 0x0050AE30
void CGChat::ProcessPendingChatQueue() {
    if (s_queueChat) {
        return;
    }

    PENDINGCHAT* next;

    for (PENDINGCHAT* chat = CGChat::s_pendingChat.Head(); !CGChat::s_pendingChat.IsEnd(chat); chat = next) {
        next = CGChat::s_pendingChat.RawNext(chat);

        if (!chat->m_queued) {
            continue;
        }

        chat->m_queued = 0;
        const char* senderName = nullptr;
        WOWGUID senderGuid = chat->m_senderGuid;

        if (!(senderGuid.m_guid & 0xF000000000000000ULL) && (senderGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            NameCache* record = g_nameDBCache.GetRecord(senderGuid.m_guid, chat->m_senderGuid, CGChat::NameQueryCallback, nullptr, false);

            if (!record) {
                continue;
            }

            senderName = record->m_name;
        } else if ((senderGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((senderGuid.m_guid >> 24) & 0xFFFFFFF), chat->m_senderGuid, CGChat::NameQueryCallback, nullptr, false);

            if (!record) {
                continue;
            }

            senderName = record->m_name;
        }

        const char* targetName;
        WOWGUID targetGuid = chat->m_targetGuid;

        if (!(targetGuid.m_guid & 0xF000000000000000ULL) && (targetGuid.m_guid & 0xF07FFFFFFFFFFFFFULL)) {
            NameCache* record = g_nameDBCache.GetRecord(targetGuid.m_guid, chat->m_targetGuid, CGChat::NameQueryCallback, nullptr, false);

            if (!record) {
                continue;
            }

            targetName = record->m_name;
        } else if ((targetGuid.m_guid & 0xF0F0000000000000ULL) == 0xF040000000000000ULL) {
            PetNameCache* record = g_petNameCache.GetRecord(static_cast<int32_t>((targetGuid.m_guid >> 24) & 0xFFFFFFF), chat->m_targetGuid, CGChat::NameQueryCallback, nullptr, false);

            if (!record) {
                continue;
            }

            targetName = record->m_name;
        } else {
            targetName = chat->m_targetName;
        }

        CGChat::AddChatMessage(chat->m_text, chat->m_type, !senderName || chat->m_isGM ? chat->m_senderName : senderName, chat->m_language, chat->m_channelName, targetName, chat->m_flags, chat->m_senderGuid, chat->m_senderUnk, chat->m_targetGuid, chat->m_achievementID, chat->m_isGM, chat->m_conversation);
        CGChat::s_pendingChat.DeleteNode(chat);
    }
}

// VERIFY: d5d65c21 manual
// OFFSET: 0x0050DF80
void CGChat::EnterWorld() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));
    CGChat::m_afk = player && (player->m_player->m_playerFlags & 0x2) ? 1 : 0;
    s_queueChat = 0;
    CGChat::ProcessPendingChatQueue();
}

// VERIFY: bff5dc80 manual
// OFFSET: 0x0050BA10
void CGChat::LogReactionChanged(int32_t faction, int32_t reaction) {
    FactionRec* rec = g_factionDB.GetRecord(faction);

    if (!rec) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    char token[64];
    SStrPrintf(token, sizeof(token), "FACTION_STANDING_LABEL%d", reaction + 1);
    const char* label = player->GetFrameScriptText(token, -1);
    char text[3000];
    SStrPrintf(text, sizeof(text), FrameScript_GetText("FACTION_STANDING_CHANGED", -1, GENDER_NOT_APPLICABLE), label, rec->m_name);
    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 2ec817c2 auto
// OFFSET: 0x0050B8C0
void CGChat::LogFactionChanged(int32_t faction, int32_t delta, int32_t generic, float bonus) {
    FactionRec* rec = g_factionDB.GetRecord(faction);

    if (!rec || !delta) {
        return;
    }

    char text[3000];

    if (generic) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText(delta > 0 ? "FACTION_STANDING_INCREASED_GENERIC" : "FACTION_STANDING_DECREASED_GENERIC", -1, GENDER_NOT_APPLICABLE), rec->m_name);
        CGChat::AddChatMessage(text, CHAT_MSG_COMBAT_FACTION_CHANGE, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        return;
    }

    const char* token;

    if (delta <= 0) {
        token = "FACTION_STANDING_DECREASED";
    } else if (bonus > 0.0f) {
        token = "FACTION_STANDING_INCREASED_BONUS";
    } else {
        token = "FACTION_STANDING_INCREASED";
    }

    SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), rec->m_name, abs(delta), bonus);
    CGChat::AddChatMessage(text, CHAT_MSG_COMBAT_FACTION_CHANGE, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    SendToCombatText(36, rec->m_name, delta);
}

// VERIFY: 5920534b auto
// OFFSET: 0x004FB400
int32_t CGChat::IsChatLogging(int32_t type) {
    return s_chatLogging[type];
}

// VERIFY: f328dd5b auto
// OFFSET: 0x004FB210
ChatRecord* GetChatRecord(int32_t index) {
    return &CGChat::m_chatHistory[(CGChat::m_currentChatIndex + index) % 60];
}

// VERIFY: c767ca52 auto
// OFFSET: 0x004FB3C0
void CGChat::AddCombatMessage(const char* text) {
    if (!text) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (s_chatLogging[1]) {
        SLogWrite(s_chatLog[1], "%s", text);
    }
}

// VERIFY: 4cc22e44 auto
// OFFSET: 0x0050BBF0
void CGChat::LogPetHappinessDecrease(const WOWGUID& caster, const WOWGUID& target, int32_t amount) {
    char text[3000];

    if (caster == ClntObjMgrGetActivePlayer()) {
        const char* targetName = GetObjectNameFromGuid(target);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SPELLHAPPINESSDRAINSELF", -1, GENDER_NOT_APPLICABLE), targetName, amount);
    } else {
        const char* targetName = GetObjectNameFromGuid(target);
        const char* casterName = GetObjectNameFromGuid(caster);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SPELLHAPPINESSDRAINOTHER", -1, GENDER_NOT_APPLICABLE), casterName, targetName, amount);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_PET_INFO, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID{ 0 }, 0, WOWGUID{ 0 }, 0, false, nullptr);
}

// VERIFY: 0afea745 auto
// OFFSET: 0x0050BCC0
void CGChat::LogDismissPet(const WOWGUID& guid, const WOWGUID& pet) {
    char text[3000];

    if (guid == ClntObjMgrGetActivePlayer()) {
        const char* petName = GetObjectNameFromGuid(pet);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SPELLDISMISSPETSELF", -1, GENDER_NOT_APPLICABLE), petName);
    } else {
        const char* petName = GetObjectNameFromGuid(pet);
        const char* name = GetObjectNameFromGuid(guid);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("SPELLDISMISSPETOTHER", -1, GENDER_NOT_APPLICABLE), name, petName);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_PET_INFO, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID{ 0 }, 0, WOWGUID{ 0 }, 0, false, nullptr);
}

// VERIFY: 5985c2bf manual
// OFFSET: 0x004FD260
void ChatWindowInfo::RemoveChannel(const char* name) {
    for (uint32_t i = 0; i < this->m_channels.m_count; i++) {
        if (this->m_channels.m_data[i] && !SStrCmpUTF8I(this->m_channels.m_data[i], name, STORM_MAX_STR)) {
            if (this->m_channels.m_data[i]) {
                SMemFree(this->m_channels.m_data[i], __FILE__, __LINE__, 0);
            }

            this->m_channels.m_data[i] = nullptr;
            this->m_channelIDs.m_data[i] = 0;
            return;
        }
    }
}

// VERIFY: 107c4716 auto
// OFFSET: 0x004FE160
const char* CGChat::GetChannelString(const char* name) {
    if (!name || !*name) {
        return nullptr;
    }

    int32_t number = SStrToInt(name);

    if (!number) {
        return name;
    }

    if (number >= 1 && number <= static_cast<int32_t>(CGChat::s_channels.m_count)) {
        ChatChannel* channel = &CGChat::s_channels.m_data[number - 1];

        if (channel->m_channelNumber == number && !channel->m_unk9C) {
            return channel->m_name;
        }
    }

    return nullptr;
}

// VERIFY: c7274882 manual
// OFFSET: 0x004FE260
static bool FindGuildRecruitmentChannel() {
    bool found = s_recruitmentChannelFound;

    if (!found) {
        for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
            ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

            if (rec && (rec->m_flags & 0x20000)) {
                s_recruitmentChannelID = rec->m_id;
                SStrCopy(s_recruitmentChannelName, rec->m_shortcut, sizeof(s_recruitmentChannelName));
                found = true;
                s_recruitmentChannelFound = true;
            }
        }
    }

    return found;
}

// VERIFY: 48808181 manual
// OFFSET: 0x00501A70
void ChatWindowInfo::AddChannel(const char* name, int32_t id) {
    for (uint32_t i = 0; i < this->m_channels.m_count; i++) {
        if (this->m_channels.m_data[i] && !SStrCmpUTF8I(this->m_channels.m_data[i], name, STORM_MAX_STR)) {
            return;
        }
    }

    uint32_t index;

    for (index = 0; index < this->m_channels.m_count; index++) {
        if (!this->m_channels.m_data[index]) {
            this->m_channels.m_data[index] = SStrDupA(name, __FILE__, __LINE__);
            this->m_channelIDs.m_data[index] = id;
            return;
        }
    }

    this->m_channels.SetCount(this->m_channels.m_count * 2);
    this->m_channelIDs.SetCount(this->m_channelIDs.m_count * 2);
    this->m_channels.m_data[index] = SStrDupA(name, __FILE__, __LINE__);
    this->m_channelIDs.m_data[index] = id;

    for (uint32_t i = index + 1; i < this->m_channels.m_count; i++) {
        this->m_channels.m_data[i] = nullptr;
        this->m_channelIDs.m_data[i] = 0;
    }
}

// VERIFY: f2e39696 manual
// OFFSET: 0x005076B0
int32_t CGChat::AddChatChannel(const char* name, uint32_t id, bool silent, bool a4, bool a5, uint32_t number) {
    if (GetChannelByName(name)) {
        return 0;
    }

    if (id && CGChat::GetChannelByChatChannelID(id)) {
        return 0;
    }

    uint32_t slot;

    if (number && number <= 10) {
        if (number > CGChat::s_channels.m_count) {
            CGChat::s_channels.SetCount(number);
            slot = number - 1;
        } else if (!CGChat::s_channels.m_data[number - 1].m_channelNumber) {
            slot = number - 1;
        } else {
            slot = CGChat::s_channels.m_count;
        }
    } else {
        slot = CGChat::s_channels.m_count;
    }

    if (slot == CGChat::s_channels.m_count) {
        for (slot = 0; slot < CGChat::s_channels.m_count; slot++) {
            if (!CGChat::s_channels.m_data[slot].m_channelNumber) {
                break;
            }
        }

        if (slot == CGChat::s_channels.m_count) {
            if (CGChat::s_channels.m_count == 10) {
                if (!silent) {
                    CGGameUI::DisplayError(ERR_TOO_MANY_CHAT_CHANNELS);
                }

                return 0;
            }

            CGChat::s_channels.SetCount(CGChat::s_channels.m_count + 1);
        }
    }

    ChatChannel* channel = &CGChat::s_channels.m_data[slot];
    channel->m_channelNumber = slot + 1;
    SStrCopy(channel->m_name, name, sizeof(channel->m_name));
    channel->m_zoneChannelID = id;
    channel->m_unk9C = 1;
    channel->m_unkA0 = 0;
    channel->m_unkA4 = a4;
    channel->m_unkA5 = a5;
    return 1;
}

// VERIFY: 8b7bb244 manual
// OFFSET: 0x00507A80
bool InternalJoinChannel(const char* channelName, const char* password, uint32_t window, bool checkRecruitment, const char** zoneChannelName, int32_t* zoneChannelID, bool silent, int32_t a8, int32_t a9, uint32_t number) {
    *zoneChannelName = nullptr;
    *zoneChannelID = 0;

    char name[256];
    SStrCopy(name, channelName, sizeof(name));
    const char* zoneText = CGGameUI::m_zoneText;
    AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (SStrCmpUTF8I(rec->m_shortcut, channelName, STORM_MAX_STR)) {
            continue;
        }

        const char* text = area && (rec->m_flags & 0x20) && (area->m_flags & 0x100) && s_cityArea ? s_cityArea->m_areaName : zoneText;

        if (!text || !*text) {
            return false;
        }

        *zoneChannelName = rec->m_shortcut;
        *zoneChannelID = rec->m_id;
        SStrPrintf(name, sizeof(name), rec->m_name, text);
        password = "";

        if (checkRecruitment && (rec->m_flags & 0x20000)) {
            CGChat::SetGuildRecruitmentMode(0, true);
        }

        break;
    }

    ChatChannel* channel = *zoneChannelID ? CGChat::GetChannelByChatChannelID(*zoneChannelID) : GetChannelByName(name);

    if (!channel) {
        if (!CGChat::AddChatChannel(name, *zoneChannelID, silent, a8 != 0, a9 != 0, number)) {
            return false;
        }

        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CHAT_JOIN_CHANNEL));
        msg.Put(static_cast<uint32_t>(*zoneChannelID));
        msg.Put(static_cast<uint8_t>(silent));
        msg.Put(static_cast<uint8_t>(a8));
        msg.PutString(name);
        msg.PutString(password);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (window < 10) {
        s_chatWindows[window].AddChannel(*zoneChannelName ? *zoneChannelName : name, *zoneChannelID);
        s_chatSettingsDirty = 1;
    }

    return true;
}

// VERIFY: 3d422a41 manual
// OFFSET: 0x00507D30
void CGChat::UpdateGuildStatus() {
    if (CGChat::m_guildRecruitmentMode != 1 || !CGChat::m_registered) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player || !FindGuildRecruitmentChannel()) {
        return;
    }

    ChatChannel* channel = CGChat::GetChannelByChatChannelID(s_recruitmentChannelID);

    if (player->m_player->m_playerGuildid) {
        if (channel) {
            InternalLeaveChannel(s_recruitmentChannelName, false, true);
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_WINDOWS, nullptr);
        }
    } else if (!channel) {
        const char* zoneChannelName = nullptr;
        int32_t zoneChannelID = 0;
        InternalJoinChannel(s_recruitmentChannelName, "", 0, false, &zoneChannelName, &zoneChannelID, true, 0, 1, 0);
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_WINDOWS, nullptr);
    }
}

// VERIFY: 3583052f manual
// OFFSET: 0x00508090
void CGChat::ZoneUpdate() {
    if (!CGChat::m_loaded) {
        return;
    }

    if (CGGameUI::m_zoneText && *CGGameUI::m_zoneText) {
        CGChat::m_areaID = CGGameUI::m_areaID;

        if (!CGChat::m_registered) {
            if (!g_areaTableDB.GetRecord(CGGameUI::m_areaID)) {
                return;
            }

            const char* zoneChannelName;
            int32_t zoneChannelID;

            for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
                ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

                if ((1 << (rec->m_id - 1)) & s_joinedZoneChannels) {
                    InternalJoinChannel(rec->m_shortcut, "", 10, false, &zoneChannelName, &zoneChannelID, true, 0, 1, 0);
                }
            }

            if (s_pendingChannelNames.m_count) {
                for (uint32_t i = 0; i < s_pendingChannelNames.m_count; i++) {
                    InternalJoinChannel(s_pendingChannelNames.m_data[i], "", 10, false, &zoneChannelName, &zoneChannelID, true, s_pendingChannelFlags.m_data[i], 1, s_pendingChannelNumbers.m_data[i]);

                    if (s_pendingChannelNames.m_data[i]) {
                        SMemFree(s_pendingChannelNames.m_data[i], __FILE__, __LINE__, 0);
                    }
                }

                s_pendingChannelNames.Clear();
                s_pendingChannelFlags.Clear();
                s_pendingChannelNumbers.Clear();
            }

            CGChat::m_registered = 1;
        }
    }

    CGChat::UpdateGuildStatus();
}

// VERIFY: ce71b96d auto
// OFFSET: 0x00508260
void CGChat::SetGuildRecruitmentMode(int32_t mode, bool fromJoin) {
    if (CGChat::m_guildRecruitmentMode == mode) {
        return;
    }

    CGChat::m_guildRecruitmentMode = mode;

    if (!CGChat::m_registered) {
        return;
    }

    if (mode == 1) {
        CGChat::UpdateGuildStatus();
    } else if (!fromJoin && FindGuildRecruitmentChannel()) {
        InternalLeaveChannel(s_recruitmentChannelName, false, true);
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_WINDOWS, nullptr);
    }
}

// VERIFY: a08ae344 manual
// OFFSET: 0x00508320
void CGChat::LoadChatSettingsCallback(HOSFILE localFile, HOSFILE file, void* param) {
    int32_t version = 0;
    int32_t addedVersion = 0;
    uint32_t allChannels = 0;

    for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (rec->m_id > 32) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        allChannels |= 1 << (rec->m_id - 1);
    }

    char* buffer = nullptr;
    uint32_t size = 0;

    if (file != OS_INVALID_FILE) {
        size = static_cast<uint32_t>(OsGetFileSize(file));

        if (size) {
            buffer = static_cast<char*>(alloca(size + 1));
            uint32_t bytesRead;

            if (!OsReadFile(file, buffer, size, &bytesRead)) {
                buffer = nullptr;
            } else if (bytesRead != size) {
                SErrDisplayAppFatalMessage("", "", 0);
                buffer = nullptr;
            }
        }
    }

    if (buffer) {
        buffer[size] = '\0';

        for (uint32_t i = 0; i < s_pendingChannelNames.m_count; i++) {
            if (s_pendingChannelNames.m_data[i]) {
                SMemFree(s_pendingChannelNames.m_data[i], __FILE__, __LINE__, 0);
            }
        }

        s_pendingChannelNames.Clear();
        s_pendingChannelFlags.Clear();
        s_pendingChannelNumbers.Clear();

        int32_t state = 0;
        uint32_t window = 0;
        const char* cursor = buffer;
        char line[128];
        char token[256];
        char word[256];
        char value[256];
        SStrTokenize(&cursor, line, sizeof(line), "\r\n", nullptr);

        if (!SStrCmpI(line, "VERSION", SStrLen("VERSION"))) {
            const char* rest = &line[SStrLen("VERSION")];
            SStrTokenize(&rest, value, sizeof(value), " \r\n", nullptr);
            version = SStrToInt(value);

            if (version >= 2) {
                SStrTokenize(&cursor, line, sizeof(line), "\r\n", nullptr);

                if (!SStrCmpI(line, "ADDEDVERSION", SStrLen("ADDEDVERSION"))) {
                    const char* added = &line[SStrLen("ADDEDVERSION")];
                    SStrTokenize(&added, value, sizeof(value), " \r\n", nullptr);
                    addedVersion = SStrToInt(value);
                }
            }
        } else {
            cursor = buffer;
        }

        for (uint32_t i = 0; i < 10; i++) {
            memset(s_chatWindows[i].m_messageGroups, 0, sizeof(s_chatWindows[i].m_messageGroups));
        }

        do {
            SStrTokenize(&cursor, line, sizeof(line), "\r\n", nullptr);

            if (!line[0]) {
                continue;
            }

            if (!state) {
                if (!SStrCmpI(line, "CHANNELS", STORM_MAX_STR)) {
                    state = 1;
                    continue;
                }

                if (!SStrCmpI(line, "COLORS", STORM_MAX_STR)) {
                    state = 2;
                    continue;
                }

                if (!SStrCmpI(line, "WINDOW", SStrLen("WINDOW"))) {
                    if (SStrLen(line) < SStrLen("WINDOW") + 2) {
                        continue;
                    }

                    uint32_t index = SStrToInt(&line[SStrLen("WINDOW") + 1]) - 1;

                    if (index > 10) {
                        continue;
                    }

                    window = index;
                    state = 3;
                }

                if (!SStrCmpI(line, "ZONECHANNELS", SStrLen("ZONECHANNELS"))) {
                    const char* rest = &line[SStrLen("ZONECHANNELS")];
                    SStrTokenize(&rest, value, sizeof(value), " \r\n", nullptr);
                    s_joinedZoneChannels = SStrToInt(value) & allChannels;
                    continue;
                }

                if (SStrCmpI(line, s_guildRecruitmentOption, SStrLen(s_guildRecruitmentOption))) {
                    continue;
                }

                const char* rest = &line[SStrLen(s_guildRecruitmentOption)];
                SStrTokenize(&rest, token, sizeof(token), " \r\n", nullptr);
                int32_t mode = 1;

                if (!SStrCmpI(token, "STANDARD", STORM_MAX_STR)) {
                    mode = 0;
                } else {
                    SStrCmpI(token, "AUTO", STORM_MAX_STR);
                }

                if (CGChat::m_guildRecruitmentMode == mode) {
                    continue;
                }

                CGChat::m_guildRecruitmentMode = mode;

                if (CGChat::m_registered && mode == 1) {
                    CGChat::UpdateGuildStatus();
                }

                continue;
            }

            if (state == 3) {
                if (!SStrCmpI(line, "CHANNELS", STORM_MAX_STR)) {
                    state = 4;
                    continue;
                }

                if (!SStrCmpI(line, "MESSAGES", STORM_MAX_STR)) {
                    state = 5;
                    continue;
                }
            }

            if (!SStrCmpI(line, "END", STORM_MAX_STR)) {
                state = state == 4 || state == 5 ? 3 : 0;
                continue;
            }

            const char* rest = line;
            SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);

            if (!word[0]) {
                continue;
            }

            switch (state) {
                case 1: {
                    if (version < 3) {
                        break;
                    }

                    token[0] = '\0';
                    SStrTokenize(&rest, token, sizeof(token), " \r\n", nullptr);
                    value[0] = '\0';
                    SStrTokenize(&rest, value, sizeof(value), " \r\n", nullptr);
                    s_pendingChannelNames.SetCount(s_pendingChannelNames.m_count + 1);
                    s_pendingChannelNames.m_data[s_pendingChannelNames.m_count - 1] = SStrDupA(word, __FILE__, __LINE__);
                    s_pendingChannelFlags.SetCount(s_pendingChannelFlags.m_count + 1);
                    s_pendingChannelFlags.m_data[s_pendingChannelFlags.m_count - 1] = atol(token);
                    s_pendingChannelNumbers.SetCount(s_pendingChannelNumbers.m_count + 1);
                    s_pendingChannelNumbers.m_data[s_pendingChannelNumbers.m_count - 1] = atol(value);
                    break;
                }

                case 2: {
                    ChatColorInfo* color = nullptr;

                    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
                        if (!SStrCmpI(word, s_chatColors[i].m_name, STORM_MAX_STR)) {
                            color = &s_chatColors[i];
                            break;
                        }
                    }

                    if (!color) {
                        if (SStrCmpI(word, "CHANNEL", SStrLen("CHANNEL"))) {
                            break;
                        }

                        uint32_t index = SStrToInt(&word[SStrLen("CHANNEL")]);

                        if (!index || index > s_channelColors.m_count) {
                            break;
                        }

                        color = &s_channelColors.m_data[index - 1];

                        if (!color) {
                            break;
                        }
                    }

                    SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                    color->m_color[0] = SStrToInt(word);
                    SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                    color->m_color[1] = SStrToInt(word);
                    SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                    color->m_color[2] = SStrToInt(word);
                    SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);

                    if (word[0] == 'Y' || word[0] == 'N') {
                        color->m_unk48 = word[0] == 'Y';
                    }

                    break;
                }

                case 3: {
                    ChatWindowInfo* info = &s_chatWindows[window];

                    if (!SStrCmpI(word, "NAME", STORM_MAX_STR)) {
                        SStrTokenize(&rest, info->m_name, sizeof(info->m_name), "\r\n", nullptr);
                        break;
                    }

                    if (!SStrCmpI(word, "SIZE", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_unkD0 = SStrToInt(word);
                        break;
                    }

                    if (!SStrCmpI(word, "COLOR", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_color.r = SStrToInt(word);
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_color.g = SStrToInt(word);
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_color.b = SStrToInt(word);
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_color.a = SStrToInt(word);
                    }

                    if (!SStrCmpI(word, "LOCKED", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_unkD8 = SStrToInt(word);
                    }

                    if (!SStrCmpI(word, "UNINTERACTABLE", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), "\r\n", nullptr);
                        info->m_unkE4 = SStrToInt(word);
                    }

                    if (!SStrCmpI(word, "DOCKED", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_docked = SStrToInt(word);
                    }

                    if (!SStrCmpI(word, "SHOWN", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        info->m_shown = SStrToInt(word);
                    }

                    if (!SStrCmpI(word, "POSITION", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        StringToFramePoint(word, reinterpret_cast<FRAMEPOINT&>(info->m_unkE8));
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        float x = SStrToFloat(word);
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        float y = SStrToFloat(word);
                        info->m_unkEC = x;
                        info->m_unkF0 = y;
                        info->m_unkF4 = 1;
                    }

                    if (!SStrCmpI(word, "DIMENSIONS", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        float width = SStrToFloat(word);
                        SStrTokenize(&rest, word, sizeof(word), " \r\n", nullptr);
                        float height = SStrToFloat(word);
                        info->m_width = width;
                        info->m_height = height;
                        info->m_unkF4 = 1;
                    }

                    if (!SStrCmpI(word, "ZONECHANNELS", STORM_MAX_STR)) {
                        SStrTokenize(&rest, word, sizeof(word), "\r\n", nullptr);
                        uint32_t channels = SStrToInt(word) & allChannels;

                        for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
                            ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

                            if (channels & (1 << (rec->m_id - 1))) {
                                info->AddChannel(rec->m_shortcut, rec->m_id);
                            }
                        }
                    }

                    break;
                }

                case 4: {
                    s_chatWindows[window].AddChannel(word, 0);
                    break;
                }

                case 5: {
                    for (uint32_t i = 0; i < sizeof(s_chatMessageGroups) / sizeof(s_chatMessageGroups[0]); i++) {
                        if (!SStrCmpI(word, s_chatMessageGroups[i].m_name, STORM_MAX_STR)) {
                            s_chatWindows[window].m_messageGroups[i] = 1;
                            break;
                        }
                    }

                    break;
                }

                default: {
                    break;
                }
            }
        } while (cursor && *cursor);

        if (addedVersion < 13) {
            for (uint32_t i = 0; i < sizeof(s_zoneChannelDefaults) / sizeof(s_zoneChannelDefaults[0]); i++) {
                if (s_zoneChannelDefaults[i].m_addedVersion <= addedVersion) {
                    continue;
                }

                ChatChannelsRec* rec = g_chatChannelsDB.GetRecord(s_zoneChannelDefaults[i].m_channelID);

                if (!rec || !(rec->m_flags & 1)) {
                    continue;
                }

                if (rec->m_id > 32) {
                    SErrDisplayAppFatalMessage("", "", 0);
                }

                uint32_t bit = 1 << (rec->m_id - 1);

                if (!(bit & s_joinedZoneChannels)) {
                    s_joinedZoneChannels |= bit;
                    s_chatWindows[0].AddChannel(rec->m_shortcut, rec->m_id);
                }
            }

            for (uint32_t i = 0; i < sizeof(s_chatMessageGroups) / sizeof(s_chatMessageGroups[0]); i++) {
                if (s_chatMessageGroups[i].m_default && s_chatMessageGroups[i].m_type > addedVersion) {
                    s_chatWindows[i < 40 ? 0 : 1].m_messageGroups[i] = 1;
                }
            }

            for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
                if (s_chatTypeInfo[i].m_unk44 > addedVersion) {
                    s_chatColors[i].m_color[0] = s_chatTypeInfo[i].m_color[0];
                    s_chatColors[i].m_color[1] = s_chatTypeInfo[i].m_color[1];
                    s_chatColors[i].m_color[2] = s_chatTypeInfo[i].m_color[2];
                    s_chatColors[i].m_unk48 = s_chatTypeInfo[i].m_unk48;
                }
            }
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_WINDOWS, nullptr);

        for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", s_chatColors[i].m_name, s_chatColors[i].m_color[0] * (1.0f / 255.0f), s_chatColors[i].m_color[1] * (1.0f / 255.0f), s_chatColors[i].m_color[2] * (1.0f / 255.0f));
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", s_chatColors[i].m_name, s_chatColors[i].m_unk48);
        }

        for (uint32_t i = 0; i < s_channelColors.m_count; i++) {
            ChatColorInfo* color = &s_channelColors.m_data[i];
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", color->m_name, color->m_color[0] * (1.0f / 255.0f), color->m_color[1] * (1.0f / 255.0f), color->m_color[2] * (1.0f / 255.0f));
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", color->m_name, color->m_unk48);
        }
    } else {
        for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
            ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

            if (!(rec->m_flags & 1)) {
                continue;
            }

            if (rec->m_id >= 32) {
                SErrDisplayAppFatalMessage("", "", 0);
            }

            s_joinedZoneChannels |= 1 << (rec->m_id - 1);
            s_chatWindows[0].AddChannel(rec->m_shortcut, rec->m_id);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_WINDOWS, nullptr);

        for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", s_chatColors[i].m_name, s_chatColors[i].m_color[0] * (1.0f / 255.0f), s_chatColors[i].m_color[1] * (1.0f / 255.0f), s_chatColors[i].m_color[2] * (1.0f / 255.0f));
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", s_chatColors[i].m_name, s_chatColors[i].m_unk48);
        }

        for (uint32_t i = 0; i < s_channelColors.m_count; i++) {
            ChatColorInfo* color = &s_channelColors.m_data[i];
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", color->m_name, color->m_color[0] * (1.0f / 255.0f), color->m_color[1] * (1.0f / 255.0f), color->m_color[2] * (1.0f / 255.0f));
            FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", color->m_name, color->m_unk48);
        }
    }

    CGChat::m_loaded = 1;
    CGChat::ZoneUpdate();
}

// VERIFY: 77a9b802 manual
// OFFSET: 0x005093F0
int32_t InternalLeaveChannel(const char* channelName, bool clearRecruitment, bool force) {
    const char* channelString = force ? channelName : CGChat::GetChannelString(channelName);

    if (!channelString) {
        return 0;
    }

    const char* zoneChannelName = nullptr;
    char name[256];
    SStrCopy(name, channelString, sizeof(name));
    const char* zoneText = CGGameUI::m_zoneText;
    int32_t zoneChannelID = 0;
    AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (SStrCmpUTF8I(rec->m_shortcut, channelName, STORM_MAX_STR)) {
            continue;
        }

        const char* text = area && (rec->m_flags & 0x20) && (area->m_flags & 0x100) && s_cityArea ? s_cityArea->m_areaName : zoneText;

        if (!text || !*text) {
            return 0;
        }

        zoneChannelName = rec->m_shortcut;
        zoneChannelID = rec->m_id;
        SStrPrintf(name, sizeof(name), rec->m_name, text);

        if (clearRecruitment && (rec->m_flags & 0x20000) && CGChat::m_guildRecruitmentMode) {
            CGChat::m_guildRecruitmentMode = 0;
        }

        break;
    }

    for (uint32_t i = 0; i < 10; i++) {
        s_chatWindows[i].RemoveChannel(zoneChannelName ? zoneChannelName : name);
    }

    ChatChannel* channel = zoneChannelID ? CGChat::GetChannelByChatChannelID(zoneChannelID) : GetChannelByName(name);

    if (channel && (!channel->m_unk9C || force)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CHAT_LEAVE_CHANNEL));
        msg.Put(static_cast<uint32_t>(channel->m_zoneChannelID));
        msg.PutString(channel->m_name);
        msg.Finalize();
        ClientServices::Send2(&msg);

        uint32_t id = channel->m_zoneChannelID;
        channel->m_unkA0 = 1;

        if (id) {
            s_joinedZoneChannels &= ~(1 << (id - 1));
        }

        if (channel->m_unk9C == 1) {
            ChatChannel* entry = id ? CGChat::GetChannelByChatChannelID(id) : GetChannelByName(channel->m_name);

            if (entry) {
                entry->m_channelNumber = 0;
                entry->m_name[0] = '\0';
                entry->m_zoneChannelID = 0;
                entry->m_unk9C = 1;
                entry->m_unkA0 = 1;
            }

            CGChat::OnChannelsUpdated();
        }
    }

    s_chatSettingsDirty = 1;
    return 0;
}

// VERIFY: 2a2301d1 auto
// OFFSET: 0x00509780
void CGChat::LoadChatSettings() {
    LoadAccountData(7, CGChat::LoadChatSettingsCallback, nullptr);
}

// VERIFY: 024941a0 auto
// OFFSET: 0x004FDF50
const char* CGChat::GetChannelShortName(int32_t number, bool includeInactive) {
    if (number < 1 || number > static_cast<int32_t>(CGChat::s_channels.m_count)) {
        return nullptr;
    }

    ChatChannel* channel = &CGChat::s_channels.m_data[number - 1];

    if (channel->m_channelNumber != number || (!includeInactive && channel->m_unk9C)) {
        return nullptr;
    }

    if (channel->m_zoneChannelID) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecord(channel->m_zoneChannelID);

        if (rec) {
            return rec->m_shortcut;
        }
    }

    return channel->m_name;
}

// VERIFY: 6aa5aad3 auto
// OFFSET: 0x004FF4F0
uint32_t CGChat::FindDisplayChannelIndex(const char* name, int32_t type) {
    for (uint32_t i = 0; i < s_numDisplayChannels; i++) {
        if (static_cast<uint32_t>(CGChat::s_displayChannels[i].m_channelIndex) > CGChat::s_channels.m_count || CGChat::s_displayChannels[i].m_headerIndex >= 0) {
            continue;
        }

        int32_t displayType;
        const char* channel = CGChat::GetChannelStringForDisplay(i, &displayType);

        if (!channel || displayType == 4 || displayType != type) {
            continue;
        }

        if (displayType) {
            return i + 1;
        }

        if (name && !SStrCmpUTF8I(channel, name, STORM_MAX_STR)) {
            return i + 1;
        }
    }

    return 0;
}

// VERIFY: ab2de665 manual
// OFFSET: 0x004FB580
bool CGChat::ChatTokenToGUID(const char* token, WOWGUID& guid) {
    NameCache* rec = g_nameDBCache.GetRecordByName(token);

    if (!rec) {
        return Script_GetGUIDFromToken(token, guid, false);
    }

    guid = rec->m_guid;
    return true;
}

// VERIFY: 8d38700f auto
// OFFSET: 0x004FB9C0
CImVector CGChat::GetChatColor(int32_t type) {
    CImVector color;

    if (type < CHAT_MSG_MAX) {
        color.b = s_chatColors[type].m_color[2];
        color.g = s_chatColors[type].m_color[1];
        color.a = 0xFF;
        color.r = s_chatColors[type].m_color[0];
    } else {
        color.b = 0xFF;
        color.g = 0xFF;
        color.a = 0xFF;
        color.r = 0xFF;
    }

    return color;
}

// VERIFY: 50e36d6f manual
// OFFSET: 0x004FBA10
int32_t FormatTime(char* buffer, uint32_t size, int32_t seconds) {
    int32_t days = seconds / 86400;
    int32_t hours = seconds % 86400 / 3600;
    int32_t minutes = (seconds - 3600 * (hours + 24 * days)) / 60;
    return SStrPrintf(buffer, size, "%dd %dh %dm %ds", days, hours, minutes, seconds % 60);
}

// VERIFY: 55b356a1 manual
// OFFSET: 0x00501030
int32_t CGChat::OnRaidInstanceInfo(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t count;
    msg->Get(count);

    if (count) {
        if (count >= s_savedInstances.Count()) {
            s_savedInstances.SetCount(count);
        }

        for (uint32_t i = 0; i < count; i++) {
            RaidInstanceInfo& info = s_savedInstances.m_data[i];
            msg->Get(info.m_mapID);
            msg->Get(info.m_difficulty);
            msg->Get(info.m_instanceID);
            uint8_t locked = 0;
            msg->Get(locked);
            info.m_locked = locked;
            uint8_t extended = 0;
            msg->Get(extended);
            info.m_extended = extended;
            msg->Get(info.m_resetTime);
        }
    }

    s_numSavedInstances = count;
    FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_INSTANCE_INFO, nullptr);
    return 1;
}

// VERIFY: 7fc804e5 manual
// OFFSET: 0x005019D0
void ChatWindowInfo::Reset() {
    for (uint32_t i = 0; i < this->m_channels.m_count; i++) {
        if (this->m_channels.m_data[i]) {
            SMemFree(this->m_channels.m_data[i], __FILE__, __LINE__, 0);
        }

        this->m_channels.m_data[i] = nullptr;
    }

    this->m_channels.Clear();
    this->m_channelIDs.Clear();
}

// VERIFY: 69e7a8fd manual
// OFFSET: 0x00505F50
void CGChat::JoiningParty(bool raid, int32_t pvp) {
    bool voiceEnabled = s_cvarEnableVoiceChat && s_cvarEnableVoiceChat->m_intValue;
    int32_t disabled = ComSatClient_IsComsatDisabledByClient();

    if (!CGChat::m_voiceChatSystemStatus || !voiceEnabled || disabled) {
        return;
    }

    if (pvp) {
        if (g_autojoinBGVoiceCVar->m_intValue && ComSatClient_GetCurrentSessionType() == 4) {
            CGChat::SetActiveVoiceSession("", 1, -1);
            RebuildDisplayChannelList(0);
        }
    } else if (g_autojoinPartyVoiceCVar->m_intValue && ComSatClient_GetCurrentSessionType() == 4) {
        CGChat::SetActiveVoiceSession("", raid ? 3 : 2, -1);
        RebuildDisplayChannelList(0);
    }
}

// VERIFY: 9faa2cc3 manual
// OFFSET: 0x005072B0
void CGChat::ShutdownGame() {
    for (uint32_t i = 0; i < 2; i++) {
        if (s_chatLog[i]) {
            SLogClose(s_chatLog[i]);
        }

        s_chatLog[i] = nullptr;
        s_chatLogging[i] = 0;
    }

    s_wordLists.Clear();
    s_profanityCache.Clear();
    s_spamCache.Clear();
    CGChat::s_pendingChat.Clear();
    CGChat::s_channels.SetCount(0);
    s_channelColors.Clear();

    for (uint32_t i = 0; i < 10; i++) {
        s_chatWindows[i].Reset();
    }

    for (uint32_t i = 0; i < s_pendingChannelNames.m_count; i++) {
        if (s_pendingChannelNames.m_data[i]) {
            SMemFree(s_pendingChannelNames.m_data[i], __FILE__, __LINE__, 0);
        }
    }

    s_pendingChannelNames.Clear();
    s_pendingChannelFlags.Clear();
    s_pendingChannelNumbers.Clear();
    s_numSavedInstances = 0;
    s_savedInstances.Clear();
    s_channelThrottle.Clear();
    CGChat::s_throttledChat.Clear();
    ClientServices::ClearMessageHandler(SMSG_CHANNEL_NOTIFY);
    ClientServices::ClearMessageHandler(SMSG_CHANNEL_LIST);
    ClientServices::ClearMessageHandler(SMSG_CHAT);
    ClientServices::ClearMessageHandler(SMSG_TEXT_EMOTE);
    ClientServices::ClearMessageHandler(SMSG_ZONE_UNDER_ATTACK);
    ClientServices::ClearMessageHandler(SMSG_DEFENSE_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_CHAT_SERVER_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_RAID_INSTANCE_INFO);
    ClientServices::ClearMessageHandler(SMSG_RAID_INSTANCE_MESSAGE);
    ClientServices::ClearMessageHandler(SMSG_INSTANCE_RESET);
    ClientServices::ClearMessageHandler(SMSG_INSTANCE_RESET_FAILED);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_LAST_INSTANCE);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_INSTANCE_OWNERSHIP);
    ClientServices::ClearMessageHandler(SMSG_EXPECTED_SPAM_RECORDS);
    ClientServices::ClearMessageHandler(SMSG_TITLE_EARNED);
    ClientServices::ClearMessageHandler(SMSG_RESET_FAILED_NOTIFY);
    ClientServices::ClearMessageHandler(SMSG_GM_MESSAGECHAT);
    ClientServices::ClearMessageHandler(SMSG_LOG_XP_GAIN);
    ClientServices::ClearMessageHandler(SMSG_DURABILITY_DAMAGE_DEATH);
    ClientServices::ClearMessageHandler(SMSG_CHANNEL_MEMBER_COUNT);
    ClientServices::ClearMessageHandler(SMSG_COMSAT_RECONNECT_TRY);
    ClientServices::ClearMessageHandler(SMSG_COMSAT_DISCONNECT);
    ClientServices::ClearMessageHandler(SMSG_COMSAT_CONNECT_FAIL);
    ClientServices::ClearMessageHandler(SMSG_VOICE_CHAT_STATUS);
    ClientServices::ClearMessageHandler(SMSG_USERLIST_ADD);
    ClientServices::ClearMessageHandler(SMSG_USERLIST_REMOVE);
    ClientServices::ClearMessageHandler(SMSG_USERLIST_UPDATE);
    ClientServices::ClearMessageHandler(SMSG_VOICESESSION_FULL);
    ClientServices::ClearMessageHandler(SMSG_SERVER_FIRST_ACHIEVEMENT);
}

// VERIFY: 328a6334 manual
// OFFSET: 0x00500380
int32_t CGChat::OnUserListUpdate(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint8_t memberFlags;
    uint8_t channelFlags;
    uint32_t count;
    char channelName[128];
    msg->Get(guid.m_guid);
    msg->Get(memberFlags);
    msg->Get(channelFlags);
    msg->Get(count);
    msg->GetString(channelName, sizeof(channelName));

    if (s_activeDisplayChannelType || SStrCmpUTF8I(channelName, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        msg->m_read = msg->m_size;
        return 1;
    }

    uint32_t displayIndex = CGChat::FindDisplayChannelIndex(channelName, 0);

    if (displayIndex) {
        ChatChannel* channel = &CGChat::s_channels.m_data[CGChat::s_displayChannels[displayIndex - 1].m_channelIndex];

        if (channel->m_flags != channelFlags) {
            channel->m_flags = channelFlags;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_FLAGS_UPDATED, "%d", displayIndex);
        }
    }

    for (uint32_t i = 0; i < s_pendingUsers.m_count; i++) {
        PENDINGUSERLIST* user = &s_pendingUsers.m_data[i];

        if (user->m_guid != guid) {
            continue;
        }

        user->m_flags = memberFlags;

        if (ComSatClient_IsMemberInVoiceSession(user->m_guid, 0, channelName)) {
            user->m_flags |= 0x40;
        }

        if (ComSatClient_GetChannelMemberVoiceStatus(user->m_guid, 0, channelName)) {
            user->m_flags |= 0x80;
        }

        if (ComSatClient_IsPlayerMuted(user->m_guid) || ComSatClient_IsTalkerSquelchedInChannel(user->m_guid, 0, channelName)) {
            user->m_flags |= 0x20;
        }

        if (displayIndex) {
            CGChat::s_displayChannels[displayIndex - 1].m_count = s_pendingUsers.m_count;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_COUNT_UPDATE, "%d%d", displayIndex, s_pendingUsers.m_count);
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", displayIndex, s_pendingUsers.m_count);
        }

        break;
    }

    return 1;
}

// VERIFY: e9ce2aa4 manual
// OFFSET: 0x004FD300
void CGChat::SaveChatSettingsCallback(HOSFILE localFile, HOSFILE file, void* param) {
    if (file == OS_INVALID_FILE) {
        return;
    }

    uint32_t written = 0;
    char line[256];
    SStrPrintf(line, sizeof(line), "VERSION %d\n\n", 5);
    OsWriteFile(file, line, SStrLen(line), &written);
    SStrPrintf(line, sizeof(line), "ADDEDVERSION %d\n\n", 13);
    OsWriteFile(file, line, SStrLen(line), &written);
    SStrPrintf(line, sizeof(line), "%s %s\n\n", s_guildRecruitmentOption, CGChat::m_guildRecruitmentMode == 1 ? "AUTO" : "STANDARD");
    OsWriteFile(file, line, SStrLen(line), &written);
    OsWriteFile(file, "CHANNELS\n", SStrLen("CHANNELS\n"), &written);

    for (uint32_t i = 0; i < CGChat::s_channels.m_count; i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (channel->m_channelNumber && !channel->m_unk9C && !channel->m_zoneChannelID && channel->m_unkA5) {
            SStrPrintf(line, sizeof(line), "%s %d %d\n", channel->m_name, channel->m_unkA4 != 0, channel->m_channelNumber);
            OsWriteFile(file, line, SStrLen(line), &written);
        }
    }

    OsWriteFile(file, "END\n\n", SStrLen("END\n\n"), &written);
    SStrPrintf(line, sizeof(line), "ZONECHANNELS %d\n\n", s_joinedZoneChannels);
    OsWriteFile(file, line, SStrLen(line), &written);
    OsWriteFile(file, "COLORS\n", SStrLen("COLORS\n"), &written);

    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        ChatColorInfo* color = &s_chatColors[i];

        if (color->m_name[0]) {
            SStrPrintf(line, sizeof(line), "%s %d %d %d %c\n", color->m_name, color->m_color[0], color->m_color[1], color->m_color[2], color->m_unk48 ? 'Y' : 'N');
            OsWriteFile(file, line, SStrLen(line), &written);
        }
    }

    for (uint32_t i = 0; i < s_channelColors.m_count; i++) {
        ChatColorInfo* color = &s_channelColors.m_data[i];

        if (color->m_name[0]) {
            SStrPrintf(line, sizeof(line), "%s %d %d %d %c\n", color->m_name, color->m_color[0], color->m_color[1], color->m_color[2], color->m_unk48 ? 'Y' : 'N');
            OsWriteFile(file, line, SStrLen(line), &written);
        }
    }

    OsWriteFile(file, "END\n\n", SStrLen("END\n\n"), &written);

    for (uint32_t w = 0; w < 10; w++) {
        ChatWindowInfo* info = &s_chatWindows[w];
        SStrPrintf(line, sizeof(line), "WINDOW %d\n", w + 1);
        OsWriteFile(file, line, SStrLen(line), &written);

        if (info->m_name[0]) {
            SStrPrintf(line, sizeof(line), "NAME %s\n", info->m_name);
            OsWriteFile(file, line, SStrLen(line), &written);
        }

        SStrPrintf(line, sizeof(line), "SIZE %d\n", info->m_unkD0);
        OsWriteFile(file, line, SStrLen(line), &written);
        SStrPrintf(line, sizeof(line), "COLOR %d %d %d %d\n", info->m_color.r, info->m_color.g, info->m_color.b, info->m_color.a);
        OsWriteFile(file, line, SStrLen(line), &written);
        SStrPrintf(line, sizeof(line), "LOCKED %d\n", info->m_unkD8 != 0);
        OsWriteFile(file, line, SStrLen(line), &written);
        SStrPrintf(line, sizeof(line), "UNINTERACTABLE %d\n", info->m_unkE4 != 0);
        OsWriteFile(file, line, SStrLen(line), &written);
        SStrPrintf(line, sizeof(line), "DOCKED %d\n", info->m_docked);
        OsWriteFile(file, line, SStrLen(line), &written);
        SStrPrintf(line, sizeof(line), "SHOWN %d\n", info->m_shown != 0);
        OsWriteFile(file, line, SStrLen(line), &written);

        if (info->m_unkF4) {
            SStrPrintf(line, sizeof(line), "POSITION %s %f %f\n", FramePointToString(static_cast<FRAMEPOINT>(info->m_unkE8)), info->m_unkEC, info->m_unkF0);
            OsWriteFile(file, line, SStrLen(line), &written);
            SStrPrintf(line, sizeof(line), "DIMENSIONS %f %f\n\n", info->m_width, info->m_height);
            OsWriteFile(file, line, SStrLen(line), &written);
        }

        OsWriteFile(file, "MESSAGES\n", SStrLen("MESSAGES\n"), &written);

        for (uint32_t i = 0; i < 46; i++) {
            if (info->m_messageGroups[i]) {
                SStrPrintf(line, sizeof(line), "%s\n", s_chatMessageGroups[i].m_name);
                OsWriteFile(file, line, SStrLen(line), &written);
            }
        }

        OsWriteFile(file, "END\n\n", SStrLen("END\n\n"), &written);
        OsWriteFile(file, "CHANNELS\n", SStrLen("CHANNELS\n"), &written);
        uint32_t zoneChannels = 0;

        for (uint32_t i = 0; i < info->m_channels.m_count; i++) {
            if (!info->m_channels.m_data[i]) {
                continue;
            }

            int32_t channelId = info->m_channelIDs.m_data[i];

            if (channelId) {
                zoneChannels |= 1 << (channelId - 1);
            }

            for (uint32_t j = 0; j < CGChat::s_channels.m_count; j++) {
                if (!SStrCmpUTF8I(CGChat::s_channels.m_data[j].m_name, info->m_channels.m_data[i], STORM_MAX_STR) && !CGChat::s_channels.m_data[j].m_unkA0) {
                    SStrPrintf(line, sizeof(line), "%s\n", info->m_channels.m_data[i]);
                    OsWriteFile(file, line, SStrLen(line), &written);
                    break;
                }
            }
        }

        OsWriteFile(file, "END\n\n", SStrLen("END\n\n"), &written);
        SStrPrintf(line, sizeof(line), "ZONECHANNELS %d\n\n", zoneChannels & s_joinedZoneChannels);
        OsWriteFile(file, line, SStrLen(line), &written);
        OsWriteFile(file, "END\n\n", SStrLen("END\n\n"), &written);
    }
}

// VERIFY: 86438221 auto
// OFFSET: 0x004FFDA0
void CGChat::SaveChatSettings() {
    s_chatSettingsDirty = 0;
    SaveAccountData(7, CGChat::SaveChatSettingsCallback, nullptr);
}

// VERIFY: 13cd8cd3 auto
// OFFSET: 0x004FBB10
int32_t CGChat::GetActiveVoiceSession() {
    return s_activeVoiceChannel;
}

// VERIFY: 838b84c3 manual
// OFFSET: 0x00500030
int32_t CGChat::SetActiveVoiceSession(const char* name, int32_t type, int32_t index) {
    if (type == 4) {
        if (CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled) {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d", s_activeVoiceChannel + 1, CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d", s_activeVoiceChannel + 1);
        }

        s_activeVoiceChannel = 0;
        ComSatClient_DisableCurrentSession();
        return 1;
    }

    if (index < 0) {
        uint32_t display = CGChat::FindDisplayChannelIndex(name, type);
        index = display ? display - 1 : 0;
    }

    int32_t result = ComSatClient_SetCurrentSession(type, name);

    if (!result) {
        return result;
    }

    if (s_activeVoiceChannel == index) {
        return 1;
    }

    if (s_activeVoiceChannel) {
        int32_t enabled = CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled;

        if (enabled) {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d", s_activeVoiceChannel + 1, enabled);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d", s_activeVoiceChannel + 1);
        }
    }

    if (index) {
        int32_t enabled = CGChat::s_displayChannels[index].m_voiceEnabled;

        if (enabled) {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d%d", index + 1, enabled, 1);
            s_activeVoiceChannel = index;
            return 1;
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d", index + 1);
    }

    s_activeVoiceChannel = index;
    return 1;
}

// VERIFY: 4967472f auto
// OFFSET: 0x004FCA90
bool ChannelNamesEqual(const char* activeName, const char* name, int32_t activeType, int32_t type) {
    return type == activeType && (type || (name && activeName && !SStrCmpUTF8I(name, activeName, STORM_MAX_STR)));
}

// VERIFY: 145d9311 manual
// OFFSET: 0x004FFE30
void CGChat::NameDisplayQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!s_displayChannelMemberCount) {
        return;
    }

    for (uint32_t i = 0; i < s_pendingUsers.Count(); i++) {
        if (s_pendingUsers.m_data[i].m_guid == guid) {
            if (!--s_displayChannelMemberCount) {
                qsort(s_pendingUsers.m_data, s_pendingUsers.Count(), sizeof(PENDINGUSERLIST), QSortDisplayListMembers);
                uint32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

                if (index) {
                    CGChat::s_displayChannels[index - 1].m_count = s_pendingUsers.Count();
                    FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_COUNT_UPDATE, "%d%d", index, s_pendingUsers.Count());
                    FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, s_pendingUsers.Count());
                }
            }

            return;
        }
    }
}

// VERIFY: 07926f96 manual
// OFFSET: 0x00503990
int32_t CGChat::OnDisplayChannelList(CDataStore* msg) {
    char name[128];
    msg->GetString(name, sizeof(name));

    if (s_activeDisplayChannelType || SStrCmpUTF8I(name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        msg->m_read = msg->m_size;
        return 1;
    }

    uint8_t flags;
    uint32_t count;
    msg->Get(flags);
    msg->Get(count);
    s_displayChannelMemberCount = 0;
    s_pendingUsers.Clear();
    s_activeDisplayChannelFlags = 0;
    s_pendingUsers.SetCount(count);

    uint32_t index = CGChat::FindDisplayChannelIndex(name, 0);

    if (index) {
        ChatChannel* channel = &CGChat::s_channels.m_data[CGChat::s_displayChannels[index - 1].m_channelIndex];

        if (channel->m_flags != flags) {
            channel->m_flags = flags;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_FLAGS_UPDATED, "%d", index);
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        PENDINGUSERLIST* user = &s_pendingUsers.m_data[i];
        WOWGUID guid;
        uint8_t userFlags;
        msg->Get(guid.m_guid);
        msg->Get(userFlags);

        if (user) {
            user->m_guid = guid;
            user->m_flags = userFlags;

            if (ComSatClient_IsMemberInVoiceSession(user->m_guid, 0, name)) {
                user->m_flags |= 0x40;
            }

            if (ComSatClient_GetChannelMemberVoiceStatus(user->m_guid, 0, name)) {
                user->m_flags |= 0x80;
            }

            if (ComSatClient_IsPlayerMuted(user->m_guid) || ComSatClient_IsTalkerSquelchedInChannel(user->m_guid, 0, name)) {
                user->m_flags |= 0x20;
            }
        }

        if (ClntObjMgrGetActivePlayer() && ClntObjMgrGetActivePlayerObj()->m_obj->m_guid) {
            if (user->m_guid == ClntObjMgrGetActivePlayerObj()->m_obj->m_guid) {
                s_activeDisplayChannelFlags = user->m_flags;

                if (index) {
                    FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_FLAGS_UPDATED, "%d", index);
                }
            }
        }

        if (!g_nameDBCache.GetRecord(user->m_guid.m_guid, user->m_guid, CGChat::NameDisplayQueryCallback, nullptr, false)) {
            s_displayChannelMemberCount++;
        }
    }

    if (!s_displayChannelMemberCount) {
        qsort(s_pendingUsers.m_data, count, sizeof(PENDINGUSERLIST), QSortDisplayListMembers);

        if (index) {
            CGChat::s_displayChannels[index - 1].m_count = count;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_COUNT_UPDATE, "%d%d", index, count);
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, count);
        }
    }

    return 1;
}

// VERIFY: 2c5750ac auto
// VERIFY: 13c41e04 auto
// OFFSET: 0x004CFB80
void CGChat::Pause() {
    CGChat::m_paused++;
}

// VERIFY: 62ef2d2f auto
// OFFSET: 0x004CFB90
void CGChat::Resume() {
    CGChat::m_paused--;
}

// VERIFY: 7732767c auto
// OFFSET: 0x004CFBA0
int32_t CGChat::IsPaused() {
    return CGChat::m_paused;
}

// VERIFY: 969b4800 manual
// OFFSET: 0x00502960
ChatChannel::ChatChannel() {
    this->m_channelNumber = 0;
    this->m_name[0] = 0;
    this->m_pendingUsers.Clear();
    this->m_flags = 0;
    this->m_zoneChannelID = 0;
    this->m_unk98 = 0;
    this->m_unkA4 = 0;
    this->m_unkA5 = 0;
    this->m_unk9C = 1;
    this->m_unkA0 = 1;
}

// VERIFY: 2c5750ac auto
// OFFSET: 0x004FB190
void CGChat::LeaveWorld() {
    s_queueChat = 1;
}

// VERIFY: 32bfa443 manual
// OFFSET: 0x004FB240
static uint32_t GetComplaintPriority(int32_t type, uint32_t channel) {
    switch (type) {
        case CHAT_MSG_WHISPER:
        case CHAT_MSG_BATTLENET:
            return 0;

        case CHAT_MSG_CHANNEL:
            return channel ? 1 : 2;

        case CHAT_MSG_BATTLEGROUND_LEADER:
            return 3;

        case CHAT_MSG_BATTLEGROUND:
            return 4;

        case CHAT_MSG_RAID_LEADER:
            return 5;

        case CHAT_MSG_RAID_WARNING:
            return 6;

        case CHAT_MSG_RAID:
            return 7;

        case CHAT_MSG_PARTY_LEADER:
            return 8;

        case CHAT_MSG_PARTY:
            return 9;

        case CHAT_MSG_YELL:
            return 10;

        case CHAT_MSG_SAY:
            return 11;

        case CHAT_MSG_EMOTE:
            return 12;

        case CHAT_MSG_AFK:
            return 13;
    }

    return type != CHAT_MSG_DND ? 0xFFFFFFFF : 14;
}

// VERIFY: 75e6d3dd auto
// OFFSET: 0x004FB300
ChatRecord* GetChatRecordEx(const char* name, const char* text, uint32_t lineID) {
    ChatRecord* best = nullptr;
    uint32_t bestPriority = 0xFFFFFFFF;

    for (uint32_t i = 0; i < 60; i++) {
        ChatRecord* record = &CGChat::m_chatHistory[(i + CGChat::m_currentChatIndex) % 60];

        if (!record->m_description[0]) {
            continue;
        }

        uint32_t priority = GetComplaintPriority(record->m_type, record->m_unk17B0);

        if (priority < bestPriority && (!name || !SStrCmpI(name, record->m_senderName, STORM_MAX_STR)) && (!text || !SStrCmpI(text, record->m_text, STORM_MAX_STR)) && (!lineID || lineID == record->m_lineID)) {
            best = record;
            bestPriority = priority;
        }
    }

    return best;
}

// VERIFY: 1d592a1a auto
// OFFSET: 0x004FB950
static void LogFeedPetCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGChat::LogFeedPet(guid, id);
    }
}

// VERIFY: 617b0344 auto
// OFFSET: 0x004FB970
static void LogTradeSkillCreateCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        CGChat::LogTradeSkillCreate(guid, id);
    }
}

// VERIFY: 16533f83 auto
// OFFSET: 0x004FBAB0
int32_t CGChat::IsVoiceChatEnabled() {
    int32_t enabled = s_cvarEnableVoiceChat && s_cvarEnableVoiceChat->m_intValue;
    int32_t disabled = ComSatClient_IsComsatDisabledByClient();
    return CGChat::m_voiceChatSystemStatus && enabled && !disabled;
}

// VERIFY: f266388b auto
// OFFSET: 0x004FCB60
static void RebuildDisplayListMember(PENDINGUSERLIST* user, int32_t type, const char* name) {
    if (!user) {
        return;
    }

    user->m_flags &= 0xF;

    if (!ComSatClient_IsTalkerSquelchedInChannel(user->m_guid, type, name)) {
        user->m_flags |= 0x10;
    }

    if (ComSatClient_IsMemberInVoiceSession(user->m_guid, type, name)) {
        user->m_flags |= 0x40;
    }

    if (ComSatClient_GetChannelMemberVoiceStatus(user->m_guid, type, name)) {
        user->m_flags |= 0x80;
    }

    if (ComSatClient_IsPlayerMuted(user->m_guid) || ComSatClient_IsTalkerSquelchedInChannel(user->m_guid, type, name)) {
        user->m_flags |= 0x20;
    }
}

// VERIFY: 83c49f73 auto
// OFFSET: 0x004FE070
void CGRaidInfo::GetMapNameWithDifficulty(char* buffer, uint32_t size, int32_t mapId, int32_t difficulty) {
    *buffer = 0;
    MapRec* map = g_mapDB.GetRecord(mapId);

    if (!map) {
        return;
    }

    MapDifficultyRec* entry = GetMapDifficultyEntryByMapIdAndDifficulty(mapId, difficulty, nullptr);

    if (entry && entry->m_difficultystring && *entry->m_difficultystring) {
        const char* difficultyName = FrameScript_GetText(entry->m_difficultystring, -1, GENDER_NOT_APPLICABLE);
        SStrPrintf(buffer, size, FrameScript_GetText("DUNGEON_NAME_WITH_DIFFICULTY", -1, GENDER_NOT_APPLICABLE), map->m_mapName, difficultyName);
    } else {
        SStrCopy(buffer, map->m_mapName, size);
    }
}

// VERIFY: 6b8a22b7 manual
// OFFSET: 0x004FFDC0
bool CGChat::CanComplain(WOWGUID guid) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !CGChat::m_complaintsSystemStatus) {
        return false;
    }

    if (guid == player->m_obj->m_guid || (FriendList::s_friendList && FriendList::s_friendList->IsFriend(guid))) {
        return false;
    }

    return true;
}

// VERIFY: ea0c7104 auto
// OFFSET: 0x004FFF00
void CGChat::VoiceSessionUpdate(const char* name, int32_t type) {
    if (type != s_activeDisplayChannelType || (!type && (!name || SStrCmpUTF8I(name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)))) {
        return;
    }

    for (uint32_t i = 0; i < s_pendingUsers.Count(); i++) {
        RebuildDisplayListMember(&s_pendingUsers.m_data[i], type, name);
    }

    uint32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

    if (index) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, s_pendingUsers.Count());
    }
}

// VERIFY: 18ba94db manual
// OFFSET: 0x004FFFC0
void CGChat::NewVoiceSession(const char* name, int32_t type) {
    uint32_t index = CGChat::FindDisplayChannelIndex(name, type);

    if (!index) {
        return;
    }

    CGChat::s_displayChannels[index - 1].m_voiceEnabled = 1;

    if (s_activeVoiceChannel == static_cast<int32_t>(index - 1)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d%d", index, 1, 1);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d", index, 1);
    }
}

// VERIFY: e215464a manual
// OFFSET: 0x00500240
int32_t CGChat::OnVoiceStatusChanged(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t status;
    msg->Get(status);

    if (CGChat::m_voiceChatSystemStatus == (status != 0)) {
        return 1;
    }

    if (status) {
        CGChat::m_voiceChatSystemStatus = 1;

        if (CGGameUI::m_initialized) {
            FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_CHAT_ENABLED_UPDATE, nullptr);
        }

        static CVar* s_enableMicrophone = CVar::Lookup("EnableMicrophone");
        static CVar* s_enableVoiceChat = CVar::Lookup("EnableVoiceChat");

        if (s_enableVoiceChat && s_enableVoiceChat->m_intValue && s_enableMicrophone && s_enableMicrophone->m_intValue && !ComSatClient_IsComsatDisabledByClient()) {
            SI2::VoiceChat_StartCapture();
        }

        return 1;
    }

    CGChat::m_voiceChatSystemStatus = 0;

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_CHAT_ENABLED_UPDATE, nullptr);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    ComSatClient_LeaveAllSessions(player->m_obj->m_guid);
    SI2::VoiceChat_StopCapture();
    return 1;
}

// VERIFY: 8a5c30c2 manual
// OFFSET: 0x00500D80
void CGChat::SilencePartyMember(bool silence, const char* name) {
    if (!CGRaidInfo::m_numMembers && !CGPartyInfo::m_members[0]) {
        return;
    }

    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPEMASK_PLAYER, CGRaidInfo::m_numMembers ? 6 : 5, 0, 3.4028235e38f, 0);

    if (!guid) {
        CGGameUI::DisplayError(ERR_TARGET_NOT_IN_GROUP_S, name);
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (CGRaidInfo::m_numMembers) {
        WOWGUID self = player->m_obj->m_guid;

        if (!CGRaidInfo::IsPlayerRaidOfficer(self)) {
            return;
        }
    } else if (CGPartyInfo::m_leader != player->m_obj->m_guid) {
        return;
    }

    bool pvp = CGRaidInfo::IsPVPRaid() != 0;

    if (silence) {
        CGPlayer_C::SilencePlayer(guid, pvp);
    } else {
        CGPlayer_C::UnSilencePlayer(guid, pvp);
    }
}

// VERIFY: 9e75253f auto
// OFFSET: 0x00500FB0
void CGChat::UpdateRosterMember(WOWGUID guid) {
    for (uint32_t i = 0; i < s_pendingUsers.Count(); i++) {
        if (s_pendingUsers.m_data[i].m_guid != guid) {
            continue;
        }

        RebuildDisplayListMember(&s_pendingUsers.m_data[i], s_activeDisplayChannelType, CGChat::s_activeDisplayChannelName);
        uint32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

        if (index) {
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, s_pendingUsers.Count());
        }

        return;
    }
}

// VERIFY: 26845b67 manual
// OFFSET: 0x005029C0
void CGChat::DisplayPendingUserList(ChatChannel* channel) {
    for (PENDINGUSERLIST* user = channel->m_pendingUsers.Head(); user; user = channel->m_pendingUsers.Next(user)) {
        if (!g_nameDBCache.GetRecord(user->m_guid.m_guid, WOWGUID(), nullptr, nullptr, false)) {
            return;
        }
    }

    s_pendingUserListActive = false;
    char text[256];
    text[0] = 0;
    int32_t count = 0;
    char channelName[128];
    SStrCopy(channelName, channel->m_name, sizeof(channelName));
    TSExplicitList<PENDINGUSERLIST, 0> users;
    users.Combine(&channel->m_pendingUsers, 2, nullptr);

    for (PENDINGUSERLIST* user = users.Head(); user; user = users.Next(user)) {
        NameCache* name = g_nameDBCache.GetRecord(user->m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

        if (!name) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        const char* prefix = "";

        if (user->m_flags & 0x1) {
            prefix = "*";
        } else if (user->m_flags & 0x2) {
            prefix = "@";
        } else if (!(user->m_flags & 0x4)) {
            prefix = "#";
        }

        const char* separator = text[0] ? ", " : "";
        char fullName[305];

        if (name->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", name->m_name, name->m_realmName);
        } else {
            SStrCopy(fullName, name->m_name, sizeof(fullName));
        }

        char entry[51];
        SStrPrintf(entry, sizeof(entry), "%s%s%s", separator, prefix, fullName);

        if (SStrLen(entry) + SStrLen(text) < sizeof(text)) {
            SStrPack(text, entry, STORM_MAX_STR);
            count++;
        } else {
            CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL_LIST, nullptr, 0, channelName, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            text[0] = 0;

            if (entry[0] == ',') {
                memmove(entry, &entry[2], sizeof(entry));
            }

            SStrPack(text, entry, STORM_MAX_STR);
            count = 1;
        }
    }

    if (count) {
        CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL_LIST, nullptr, 0, channelName, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    users.Clear();
}

// VERIFY: b8067ca6 manual
// OFFSET: 0x00504130
int32_t CGChat::OnUserListAdded(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint8_t flags;
    uint8_t channelFlags;
    uint32_t count;
    char name[128];
    msg->Get(guid.m_guid);
    msg->Get(flags);
    msg->Get(channelFlags);
    msg->Get(count);
    msg->GetString(name, sizeof(name));

    if (s_activeDisplayChannelType || SStrCmpUTF8I(name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        msg->m_read = msg->m_size;
        return 1;
    }

    PENDINGUSERLIST user;
    UpdateVoiceMemberFlags(&user, flags, guid, 0, name);
    s_pendingUsers.New(user);
    uint32_t index = CGChat::FindDisplayChannelIndex(name, 0);

    if (index) {
        ChatChannel* channel = &CGChat::s_channels.m_data[CGChat::s_displayChannels[index - 1].m_channelIndex];

        if (channel->m_flags != channelFlags) {
            channel->m_flags = channelFlags;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_FLAGS_UPDATED, "%d", index);
        }
    }

    if (g_nameDBCache.GetRecord(guid.m_guid, guid, CGChat::NameDisplayQueryCallback, nullptr, false)) {
        qsort(s_pendingUsers.m_data, s_pendingUsers.Count(), sizeof(PENDINGUSERLIST), QSortDisplayListMembers);

        if (index) {
            CGChat::s_displayChannels[index - 1].m_count = s_pendingUsers.Count();
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_COUNT_UPDATE, "%d%d", index, s_pendingUsers.Count());
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, s_pendingUsers.Count());
        }
    } else {
        s_displayChannelMemberCount++;
    }

    return 1;
}

// VERIFY: 89bbe7cb manual
// OFFSET: 0x005042F0
int32_t CGChat::OnUserListRemove(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    uint8_t channelFlags;
    uint32_t count;
    char name[128];
    msg->Get(guid.m_guid);
    msg->Get(channelFlags);
    msg->Get(count);
    msg->GetString(name, sizeof(name));

    if (s_activeDisplayChannelType || SStrCmpUTF8I(name, CGChat::s_activeDisplayChannelName, STORM_MAX_STR)) {
        msg->m_read = msg->m_size;
        return 1;
    }

    uint32_t total = s_pendingUsers.Count();
    uint32_t index = CGChat::FindDisplayChannelIndex(name, 0);

    if (index) {
        ChatChannel* channel = &CGChat::s_channels.m_data[CGChat::s_displayChannels[index - 1].m_channelIndex];

        if (channel->m_flags != channelFlags) {
            channel->m_flags = channelFlags;
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_FLAGS_UPDATED, "%d", index);
        }
    }

    for (uint32_t i = 0; i < s_pendingUsers.Count(); i++) {
        if (s_pendingUsers.m_data[i].m_guid != guid) {
            continue;
        }

        if (i < total - 1) {
            memmove(&s_pendingUsers.m_data[i], &s_pendingUsers.m_data[i + 1], sizeof(PENDINGUSERLIST) * (total - i - 1));
        }

        s_pendingUsers.SetCount(s_pendingUsers.Count() - 1);

        if (index) {
            CGChat::s_displayChannels[index - 1].m_count = s_pendingUsers.Count();
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_COUNT_UPDATE, "%d%d", index, s_pendingUsers.Count());
            FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_ROSTER_UPDATE, "%d%d", index, s_pendingUsers.Count());
        }

        return 1;
    }

    return 1;
}

// VERIFY: 1cc83c20 manual
// OFFSET: 0x0050B6D0
void CGChat::LogHonorGain(CGUnit_C* unit, const WOWGUID& victim, int32_t rank, int32_t honor) {
    char text[3000];

    if (!victim) {
        const char* format = FrameScript_GetText("COMBATLOG_HONORAWARD", -1, GENDER_NOT_APPLICABLE);

        if (format && *format) {
            SStrPrintf(text, sizeof(text), format, honor);
            CGChat::AddChatMessage(text, CHAT_MSG_COMBAT_HONOR_GAIN, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
            SendToCombatText(43, honor);
        }

        return;
    }

    CGUnit_C* target = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(victim, TYPEMASK_UNIT, __FILE__, __LINE__));

    if (!target || honor <= 0) {
        return;
    }

    const char* format = rank < 0 ? FrameScript_GetText("COMBATLOG_HONORGAIN_NO_RANK", -1, GENDER_NOT_APPLICABLE) : FrameScript_GetText("COMBATLOG_HONORGAIN", -1, GENDER_NOT_APPLICABLE);

    if (!format || !*format) {
        return;
    }

    char token[64];
    FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(target->m_unit->m_unitFieldFactiontemplate);

    if (faction) {
        int32_t side = (faction->m_factionGroup & 0x4) ? 0 : ((faction->m_factionGroup & 0x2) ? 1 : -1);

        if (side >= 0) {
            SStrPrintf(token, sizeof(token), "PVP_RANK_%d_%d", rank, side);
        }
    }

    const char* rankName;

    if (rank < 0) {
        rankName = nullptr;
        SStrPrintf(text, sizeof(text), format, target->GetUnitName(nullptr, 1), honor);
    } else {
        rankName = unit->GetFrameScriptText(token, -1);
        SStrPrintf(text, sizeof(text), format, target->GetUnitName(nullptr, 1), rankName, honor);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_COMBAT_HONOR_GAIN, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    SendToCombatText(43, honor);

    if (rank >= 0) {
        unit->AddWorldHonorGainText(honor, rank, rankName);
    }
}

// VERIFY: e436cf39 auto
// OFFSET: 0x0050BAF0
void CGChat::LogFeedPet(const WOWGUID& guid, int32_t itemId) {
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, LogFeedPetCallback, nullptr, false);

    if (!stats) {
        return;
    }

    char text[3000];

    if (guid == ClntObjMgrGetActivePlayer()) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("FEEDPET_LOG_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE), stats->m_name[0]);
    } else {
        NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, WOWGUID(), nullptr, nullptr, false);

        if (!name) {
            return;
        }

        SStrPrintf(text, sizeof(text), FrameScript_GetText("FEEDPET_LOG_THIRDPERSON", -1, GENDER_NOT_APPLICABLE), name->m_name, stats->m_name[0]);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_PET_INFO, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: b7cddf46 auto
// OFFSET: 0x0050BD90
void CGChat::LogTradeSkillCreate(const WOWGUID& guid, int32_t itemId) {
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, guid, LogTradeSkillCreateCallback, nullptr, false);

    if (!stats) {
        return;
    }

    char text[3000];

    if (guid == ClntObjMgrGetActivePlayer()) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("TRADESKILL_LOG_FIRSTPERSON", -1, GENDER_NOT_APPLICABLE), stats->m_name[0]);
    } else {
        const char* name = GetObjectNameFromGuid(guid);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("TRADESKILL_LOG_THIRDPERSON", -1, GENDER_NOT_APPLICABLE), name, stats->m_name[0]);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_TRADESKILLS, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 719ef96a manual
// OFFSET: 0x0050C520
int32_t CGChat::OnTitleEarned(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t titleId;
    uint32_t earned;
    msg->Get(titleId);
    msg->Get(earned);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CharTitlesRec* title = g_charTitlesDB.GetRecord(CGUnit_C::GetRecordForTitle(titleId));

    if (!player || !title) {
        return 1;
    }

    char name[256];

    if (player->m_player->m_playerBytes3[0] == 1) {
        if (strcmp(title->m_name1, "")) {
            SStrPrintf(name, sizeof(name), title->m_name1, player->GetUnitName(nullptr, 1));
        } else {
            SStrPrintf(name, sizeof(name), title->m_name, player->GetUnitName(nullptr, 1));
        }
    } else if (strcmp(title->m_name, "")) {
        SStrPrintf(name, sizeof(name), title->m_name, player->GetUnitName(nullptr, 1));
    } else {
        SStrPrintf(name, sizeof(name), title->m_name1, player->GetUnitName(nullptr, 1));
    }

    char text[3000];

    if (earned) {
        void* memory = SMemAlloc(sizeof(CEffect), __FILE__, __LINE__, 0);
        CEffect* effect = memory ? new (memory) CEffect() : nullptr;
        WOWGUID guid = player->m_obj->m_guid;
        effect->AddSpecialEffect(9, guid, CGObject_C::EffectOnceHandler);
        effect->m_flags |= 0x20;
        effect->Release();
        SStrPrintf(text, sizeof(text), FrameScript_GetText("NEW_TITLE_EARNED", -1, GENDER_NOT_APPLICABLE), name);
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("OLD_TITLE_LOST", -1, GENDER_NOT_APPLICABLE), name);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 1;
}

// VERIFY: 42dad006 auto
// OFFSET: 0x0050CF20
void CGChat::DisplayParentControlsNotice(UIErrors error) {
    char text[3000];

    if (error == ERR_VOICE_CHAT_PARENTAL_DISABLE_ALL) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ERR_VOICE_CHAT_PARENTAL_DISABLE_ALL", -1, GENDER_NOT_APPLICABLE));
    } else if (error == ERR_VOICE_CHAT_PARENTAL_DISABLE_MIC) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("ERR_VOICE_CHAT_PARENTAL_DISABLE_MIC", -1, GENDER_NOT_APPLICABLE));
    } else {
        return;
    }

    CGGameUI::DisplayError(error);
    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 82a9e56c auto
// OFFSET: 0x0050DDE0
void CGChat::LogOpenLock(const WOWGUID& guid, const SpellRec* spell, const ItemStats_C* stats) {
    if (!stats) {
        return;
    }

    char text[3000];

    if (guid == ClntObjMgrGetActivePlayer()) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("OPEN_LOCK_SELF", -1, GENDER_NOT_APPLICABLE), spell->m_name, stats->m_name[0]);
    } else {
        const char* name = GetObjectNameFromGuid(guid);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("OPEN_LOCK_OTHER", -1, GENDER_NOT_APPLICABLE), name, spell->m_name, stats->m_name[0]);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_OPENING, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: e2b160fd auto
// OFFSET: 0x0050DEB0
void CGChat::LogOpenLock(const WOWGUID& guid, const SpellRec* spell, const GameObjectStats_C* stats) {
    if (!stats) {
        return;
    }

    char text[3000];

    if (guid == ClntObjMgrGetActivePlayer()) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("OPEN_LOCK_SELF", -1, GENDER_NOT_APPLICABLE), spell->m_name, stats->m_name[0]);
    } else {
        const char* name = GetObjectNameFromGuid(guid);
        SStrPrintf(text, sizeof(text), FrameScript_GetText("OPEN_LOCK_OTHER", -1, GENDER_NOT_APPLICABLE), name, spell->m_name, stats->m_name[0]);
    }

    CGChat::AddChatMessage(text, CHAT_MSG_OPENING, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: 5efc6e4a manual
// OFFSET: 0x0050DFD0
void ComplainChat(const char* name, const char* text, uint32_t lineID) {
    ChatRecord* record = GetChatRecordEx(name, text, lineID);

    if (!record || !CGChat::CanComplain(record->m_senderGuid) || !FriendList::s_friendList) {
        return;
    }

    if (FriendList::s_friendList->IsComplained(record->m_senderGuid)) {
        CGChat::AddChatMessage(FrameScript_GetText("COMPLAINT_ADDED", -1, GENDER_NOT_APPLICABLE), CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        return;
    }

    FriendList::s_friendList->AddComplained(record->m_senderGuid);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_COMPLAINT));
    msg.Put(static_cast<uint8_t>(1));
    msg.Put(record->m_senderGuid.m_guid);
    msg.Put(record->m_senderUnk);
    msg.Put(static_cast<uint32_t>(record->m_type));
    msg.Put(record->m_unk17B0);
    msg.Put(static_cast<uint32_t>(::time(nullptr) - record->m_time));
    msg.PutString(record->m_description);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: fd684479 manual
// OFFSET: 0x0050EBE0
void CombatLog::GetSpellLogLinkInfo(const WOWGUID& source, const WOWGUID& dest, int32_t spellId) {
    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell) || (spell.m_attributes & 0x180) || !*spell.m_name) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(dest, TYPEMASK_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if (object->m_obj->m_type & TYPEMASK_ITEM) {
        ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, WOWGUID(), nullptr, nullptr, false);
        CGChat::LogOpenLock(source, &spell, stats);
    } else if (object->m_obj->m_type & TYPEMASK_GAMEOBJECT) {
        GameObjectStats_C* stats = g_gameObjectDBCache.GetRecord(object->m_obj->m_entryID, WOWGUID(), nullptr, nullptr, false);
        CGChat::LogOpenLock(source, &spell, stats);
    }
}
