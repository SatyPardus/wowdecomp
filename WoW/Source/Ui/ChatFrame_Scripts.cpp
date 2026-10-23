#include "ChatFrame.h"
#include <cstdint>
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/LanguagesRec.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/DB/Rec/ChatChannelsRec.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/EmotesTextRec.h"
#include "WoW/Source/Ui/DanceStudio.h"
#include "Engine/Source/Os/OsFile.h"
#include "WoW/Source/Ui/ScriptEvents.h"

// STATIC: 0x00AC7A48
static const char* s_channelCategories[3] = { "CHANNEL_CATEGORY_GROUP", "CHANNEL_CATEGORY_WORLD", "CHANNEL_CATEGORY_CUSTOM" };

// VERIFY: 3ea62573 manual
// OFFSET: 0x0050D170
static int32_t Script_SendChatMessage(lua_State* L) {
    if (!CGGameUI::CanPerformAction(10)) {
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SendChatMessage(text [,type] [,language] [,targetPlayer])");
    }

    const char* text = lua_tolstring(L, 1, nullptr);
    int32_t type = CHAT_MSG_SAY;

    if (lua_isstring(L, 2)) {
        if (!StringToChatType(lua_tolstring(L, 2, nullptr), &type)) {
            luaL_error(L, "SendChatMessage(): Unknown chat type");
        }
    }

    if ((!text || !*text) && type != CHAT_MSG_DND && type != CHAT_MSG_AFK) {
        return 0;
    }

    if (!ValidateStringIsUTF8(text)) {
        luaL_error(L, "SendChatMessage(): Chat message must be UTF-8 text");
    }

    const char* target = nullptr;
    uint32_t language = player->GetDefaultLanguage();

    if (lua_isstring(L, 3)) {
        if (!StringToLanguage(lua_tolstring(L, 3, nullptr), &language)) {
            luaL_error(L, "SendChatMessage(): Unknown language");
        }

        uint32_t skill;

        if (!player->GetLanguageSkill(language, skill)) {
            CGGameUI::DisplayError(ERR_CANT_SPEAK_LANGAGE);
            return 0;
        }
    }

    if (lua_isstring(L, 4)) {
        target = lua_tolstring(L, 4, nullptr);
    }

    if (type == CHAT_MSG_WHISPER) {
        if (!target || !*target) {
            luaL_error(L, "SendChatMessage(): Whisper message missing target player!");
        }
    } else if (type == CHAT_MSG_PARTY) {
        if (!CGPartyInfo::InParty() && (!CGRaidInfo::m_numMembers || !CGRaidInfo::IsPVPRaid())) {
            CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
            return 0;
        }

        if (CGPartyInfo::m_leader.m_guid == player->m_obj->m_guid.m_guid) {
            type = CHAT_MSG_PARTY_LEADER;
        }
    } else if (type == CHAT_MSG_RAID_WARNING) {
        if (!CGRaidInfo::m_numMembers) {
            if (!CGPartyInfo::InParty()) {
                CGGameUI::DisplayError(ERR_NOT_IN_GROUP);
                return 0;
            }
        } else {
            WOWGUID guid = player->m_obj->m_guid;

            if (!CGRaidInfo::IsPlayerRaidOfficer(guid)) {
                CGGameUI::DisplayError(ERR_NOT_LEADER);
                return 0;
            }
        }
    } else if (type == CHAT_MSG_RAID || type == CHAT_MSG_BATTLEGROUND) {
        if (!CGRaidInfo::m_numMembers) {
            CGGameUI::DisplayError(type != CHAT_MSG_BATTLEGROUND ? ERR_NOT_IN_RAID : ERR_NOT_IN_BATTLEGROUND);
            return 0;
        }

        if (type == CHAT_MSG_RAID) {
            if (CGPartyInfo::m_leader.m_guid == player->m_obj->m_guid.m_guid) {
                type = CHAT_MSG_RAID_LEADER;
            }
        } else if (CGPartyInfo::m_leader.m_guid == player->m_obj->m_guid.m_guid) {
            type = CHAT_MSG_BATTLEGROUND_LEADER;
        }
    } else if (type == CHAT_MSG_CHANNEL) {
        if (!target || !*target) {
            luaL_error(L, "SendChatMessage(): Channel send missing channel number");
        }

        int32_t number = SStrToInt(target);

        if (number < 1 || number > static_cast<int32_t>(CGChat::s_channels.Count())) {
            return 0;
        }

        ChatChannel* channel = &CGChat::s_channels.m_data[number - 1];

        if (channel->m_channelNumber != number || channel->m_unk9C) {
            return 0;
        }

        target = channel->m_name;
    } else if (type == CHAT_MSG_AFK) {
        if (!CGChat::m_afk && !*text) {
            text = FrameScript_GetText("DEFAULT_AFK_MESSAGE", -1, GENDER_NOT_APPLICABLE);
        }

        if (*text) {
            player->SetAFK(text);
        } else {
            CGPlayer_C::ClearAFK(1);
        }

        return 0;
    }

    if (CGChat::m_afk) {
        CGPlayer_C::ClearAFK(0);
    }

    if (type == CHAT_MSG_DND) {
        if (!(player->m_player->m_playerFlags & 0x4) && !*text) {
            text = FrameScript_GetText("DEFAULT_DND_MESSAGE", -1, GENDER_NOT_APPLICABLE);
        }

        if (*text) {
            char buffer[3000];
            SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText("MARKED_DND", -1, GENDER_NOT_APPLICABLE), text);
            CGChat::AddChatMessage(buffer, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        } else {
            CGChat::AddChatMessage(FrameScript_GetText("CLEARED_DND", -1, GENDER_NOT_APPLICABLE), CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
        }
    }

    if (player->IsDeadOrGhost() && type != CHAT_MSG_WHISPER && type != CHAT_MSG_PARTY && type != CHAT_MSG_PARTY_LEADER && type != CHAT_MSG_RAID && type != CHAT_MSG_RAID_LEADER && type != CHAT_MSG_RAID_WARNING && type != CHAT_MSG_GUILD && type != CHAT_MSG_OFFICER && type != CHAT_MSG_CHANNEL && type != CHAT_MSG_DND && type != CHAT_MSG_BATTLEGROUND && type != CHAT_MSG_BATTLEGROUND_LEADER) {
        CGGameUI::DisplayError(ERR_CHAT_WHILE_DEAD);
        return 0;
    }

    CGTutorial::FlagTutorial(22);

    char parsed[256];
    ChatParserParseText(text, parsed, sizeof(parsed));
    char* message = parsed;
    char drunk[256];

    if (player->IsDrunk()) {
        InebriatedParseText(parsed, drunk, sizeof(drunk), player->GetDrunknessTurnValue());
        message = drunk;
    }

    TruncateNewLines(message);

    for (char* escape = SStrChr(message, '|'); escape;) {
        char* next;

        if (escape[1] == '|') {
            next = escape + 2;
        } else {
            if (escape[1] != 'c') {
                luaL_error(L, "SendChatMessage(): Invalid escape code in chat message");
            }

            char* end = SStrStr(escape, "|r");

            if (!end) {
                *escape = 0;
                break;
            }

            next = end + 2;
        }

        escape = SStrChr(next, '|');
    }

    if ((type == CHAT_MSG_WHISPER || type == CHAT_MSG_CHANNEL || type == CHAT_MSG_BATTLENET) && language != 0xFFFFFFFF) {
        CGChat::ProcessThrottledChat();

        if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_BATTLENET) {
            if (!CGChat::IsInWhisperSourceHistory(target)) {
                uint32_t time = OsGetAsyncTimeMs();

                if (CGChat::IsWhisperThrottled(time)) {
                    CGChat::AddThrottledChat(type, language, target, message);
                    return 0;
                }

                CGChat::AddWhisperThrottled(time);
            }
        } else {
            uint32_t time = OsGetAsyncTimeMs();

            if (CGChat::IsChannelThrottled(target, time)) {
                CGChat::AddThrottledChat(type, language, target, message);
                return 0;
            }

            CGChat::AddChannelThrottled(target, time);
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
    msg.Put(static_cast<uint32_t>(type));
    msg.Put(language);

    if (type == CHAT_MSG_WHISPER || type == CHAT_MSG_CHANNEL) {
        msg.PutString(target);
    }

    msg.PutString(message);
    msg.Finalize();
    ClientServices::Send2(&msg);

    return 0;
}

// VERIFY: fabefd8a auto
// OFFSET: 0x004FCCB0
static int32_t Script_IsVoiceChatAllowedByServer(lua_State* L) {
    if (CGChat::m_voiceChatSystemStatus) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5567fcb2 manual
// OFFSET: 0x00504E10
static int32_t Script_GetNumDisplayChannels(lua_State* L) {
    RebuildDisplayChannelList(0);
    lua_pushnumber(L, s_numDisplayChannels);
    return 1;
}

// VERIFY: ba5138d2 auto
// OFFSET: 0x004FBD40
static int32_t Script_GetActiveVoiceChannel(lua_State* L) {
    if (s_activeVoiceChannel) {
        lua_pushnumber(L, s_activeVoiceChannel + 1);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 4dce6d52 auto
// OFFSET: 0x0050D8A0
static int32_t Script_SendSystemMessage(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SendSystemMessage(\"message\")");
        return 0;
    }

    CGChat::AddChatMessage(lua_tolstring(L, 1, nullptr), CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 0;
}

// VERIFY: 0e953979 manual
// OFFSET: 0x004FFC50
static int32_t Script_ClearChannelWatch(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CLEAR_CHANNEL_WATCH));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5a0c8fb3 manual
// OFFSET: 0x00500910
static int32_t Script_GetDefaultLanguage(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    const LanguagesRec* rec = g_languagesDB.GetRecord(player->GetDefaultLanguage());

    if (!rec) {
        return 0;
    }

    lua_pushstring(L, rec->m_name);
    return 1;
}

// VERIFY: 53c096da manual
// OFFSET: 0x004FE530
static void ChannelDisplayCommand(lua_State* L, uint32_t msgId, const char* name) {
    if (!lua_isnumber(L, 1)) {
        char usage[512];
        SStrPrintf(usage, sizeof(usage), "Usage: %s(\"displayIndex\")", name);
        luaL_error(L, usage);
    }

    int32_t type;
    const char* channel = CGChat::GetChannelStringForDisplay(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, &type);

    if (!channel || type) {
        return;
    }

    CDataStore msg;
    msg.Put(msgId);
    msg.PutString(channel);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1247ad54 manual
// OFFSET: 0x00504E50
static int32_t Script_SetSelectedDisplayChannel(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    int32_t type;
    const char* channel = CGChat::GetChannelStringForDisplay(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, &type);

    if (!channel || type == 4) {
        return 0;
    }

    if (type == s_activeDisplayChannelType && (type || !SStrCmpUTF8I(channel, CGChat::s_activeDisplayChannelName, STORM_MAX_STR))) {
        return 0;
    }

    SetCurrentActiveDisplayChannel(channel, type);

    if (!type) {
        ChannelDisplayCommand(L, CMSG_CHAT_CHANNEL_DISPLAY_LIST, "GetChannelRosterInfo");
    } else if (type == 2) {
        BuildPartyDisplayList();
    } else if (type == 3 || type == 1) {
        BuildRaidDisplayList(type);
    }

    return 0;
}

// VERIFY: da51be8e manual
// OFFSET: 0x005052E0
static int32_t Script_CollapseChannelHeader(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

        if (index < s_numDisplayChannels) {
            uint32_t header = CGChat::s_displayChannels[index].m_headerIndex;

            if (header <= 2) {
                s_collapsedHeaders[header] = 1;
                RebuildDisplayChannelList(1);
            }
        }
    }

    return 0;
}

// VERIFY: bb361d6c manual
// OFFSET: 0x00505360
static int32_t Script_ExpandChannelHeader(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

        if (index < s_numDisplayChannels) {
            uint32_t header = CGChat::s_displayChannels[index].m_headerIndex;

            if (header <= 2) {
                s_collapsedHeaders[header] = 0;
                RebuildDisplayChannelList(1);
            }
        }
    }

    return 0;
}

// VERIFY: 0e7cf4b3 manual
// OFFSET: 0x004FC660
static int32_t Script_SetChatWindowLocked(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetChatWindowLocked(index, locked)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        s_chatWindows[index].m_unkD8 = StringToBOOL(L, 2, 0);
    }

    return 0;
}

// VERIFY: e46cbc4f manual
// OFFSET: 0x004FC6E0
static int32_t Script_SetChatWindowUninteractable(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetChatWindowUninteractable(index, uninteractable)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        s_chatWindows[index].m_unkE4 = StringToBOOL(L, 2, 0);
    }

    return 0;
}

// VERIFY: 15da48c8 manual
// OFFSET: 0x004FC810
static int32_t Script_SetChatWindowShown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetChatWindowShown(index, shown)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        s_chatWindows[index].m_shown = StringToBOOL(L, 2, 1);
    }

    return 0;
}

// VERIFY: f498d98a manual
// OFFSET: 0x004FC020
static int32_t Script_GetChatWindowSavedDimensions(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChatWindowSavedDimensions(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    lua_pushnumber(L, s_chatWindows[index].m_width);
    lua_pushnumber(L, s_chatWindows[index].m_height);
    return 2;
}

// VERIFY: 65f475bb manual
// OFFSET: 0x004FC330
static int32_t Script_SetChatWindowName(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetChatWindowName(index, \"name\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        const char* name = "";

        if (lua_isstring(L, 2)) {
            name = lua_tolstring(L, 2, nullptr);
        }

        SStrCopy(s_chatWindows[index].m_name, name, sizeof(s_chatWindows[index].m_name));
    }

    return 0;
}

// VERIFY: 03dc074c manual
// OFFSET: 0x004FC3D0
static int32_t Script_SetChatWindowSize(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetChatWindowSize(index, size)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        int32_t size = static_cast<int32_t>(lua_tonumber(L, 2));

        if (size > 0) {
            s_chatWindows[index].m_unkD0 = size;
        }
    }

    return 0;
}

// VERIFY: 2baa363e manual
// OFFSET: 0x00500BE0
static int32_t Script_RequestRaidInfo(lua_State* L) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_RAID_INFO));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 596d3ecf manual
// OFFSET: 0x004FC5B0
static int32_t Script_SetChatWindowAlpha(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetChatWindowAlpha(index, alpha)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        s_chatWindows[index].m_color.a = static_cast<uint8_t>(lua_tonumber(L, 2) * 255.0);
    }

    return 0;
}

// VERIFY: abac3097 manual
// OFFSET: 0x004FC760
static int32_t Script_SetChatWindowDocked(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetChatWindowDocked(index, docked)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 10) {
        uint32_t docked = 0;

        if (lua_isnumber(L, 2)) {
            docked = static_cast<uint32_t>(lua_tonumber(L, 2));
        }

        s_chatWindows[index].m_docked = docked;
    }

    return 0;
}

// VERIFY: fa92b690 manual
// OFFSET: 0x004FC9A0
static int32_t Script_SetChatWindowSavedDimensions(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: SetChatWindowSavedDimensions(index, width, height)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    ChatWindowInfo* window = &s_chatWindows[index];
    float width = static_cast<float>(lua_tonumber(L, 2));
    float height = static_cast<float>(lua_tonumber(L, 3));
    window->m_width = width;
    window->m_height = height;
    return 0;
}

// VERIFY: 25b35048 manual
// OFFSET: 0x004FBF60
static int32_t Script_GetChatWindowSavedPosition(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChatWindowSavedPosition(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10 || !s_chatWindows[index].m_unkF4) {
        return 0;
    }

    lua_pushstring(L, FramePointToString(static_cast<FRAMEPOINT>(s_chatWindows[index].m_unkE8)));
    lua_pushnumber(L, s_chatWindows[index].m_unkEC);
    lua_pushnumber(L, s_chatWindows[index].m_unkF0);
    return 3;
}

// VERIFY: 230dd44d manual
// OFFSET: 0x00500760
static int32_t Script_GetNumLanguages(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t count = 0;
    int32_t numRecords = g_languagesDB.m_numRecords;

    for (int32_t i = 0; i < numRecords; i++) {
        const LanguagesRec* rec = (i >= 0 && i < g_languagesDB.m_numRecords) ? &g_languagesDB.m_records[i] : nullptr;
        uint32_t skill;

        if (rec && player->GetLanguageSkill(rec->m_id, skill)) {
            count++;
        }
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 4f0456ed auto
// OFFSET: 0x00500810
static int32_t Script_GetLanguageByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetLanguageByIndex(index)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    int32_t wanted = static_cast<int32_t>(lua_tonumber(L, 1));
    int32_t numRecords = g_languagesDB.m_numRecords;
    int32_t found = 0;

    for (int32_t i = 0; i < numRecords; i++) {
        const LanguagesRec* rec = (i >= 0 && i < g_languagesDB.m_numRecords) ? &g_languagesDB.m_records[i] : nullptr;
        uint32_t skill;

        if (rec && player->GetLanguageSkill(rec->m_id, skill) && ++found == wanted) {
            lua_pushstring(L, rec->m_name);
            return 1;
        }
    }

    return 0;
}

// VERIFY: e5f9a784 manual
// OFFSET: 0x004FC890
static int32_t Script_SetChatWindowSavedPosition(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
        luaL_error(L, "Usage: SetChatWindowSavedPosition(index, \"point\", xOffsetRatio, yOffsetRatio)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    FRAMEPOINT point;

    if (!StringToFramePoint(lua_tolstring(L, 2, nullptr), point)) {
        luaL_error(L, "Unknown Region Point");
        return 0;
    }

    ChatWindowInfo* window = &s_chatWindows[index];
    window->m_unkE8 = point;
    float x = static_cast<float>(lua_tonumber(L, 3));
    float y = static_cast<float>(lua_tonumber(L, 4));
    window->m_unkEC = x;
    window->m_unkF0 = y;
    window->m_unkF4 = 1;
    return 0;
}

// VERIFY: a7d41f9a auto
// OFFSET: 0x0050DAE0
static int32_t Script_ListChannels(lua_State* L) {
    char text[3000];
    text[0] = 0;

    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (channel->m_channelNumber && !channel->m_unk9C) {
            char entry[138];
            SStrPrintf(entry, sizeof(entry), "[%d. %s] ", channel->m_channelNumber, channel->m_name);

            if (SStrLen(&text[SStrLen(entry)]) >= sizeof(text)) {
                CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL_LIST, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
                text[0] = 0;
            }

            SStrPack(text, entry, sizeof(text));
        }
    }

    CGChat::AddChatMessage(text, CHAT_MSG_CHANNEL_LIST, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    return 0;
}

// VERIFY: 088c2421 manual
// OFFSET: 0x004FC470
static int32_t Script_SetChatWindowColor(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
        luaL_error(L, "Usage: SetChatWindowColor(index, r, g, b)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    ChatWindowInfo* window = &s_chatWindows[index];
    window->m_color.r = static_cast<uint8_t>(lua_tonumber(L, 2) * 255.0);
    window->m_color.g = static_cast<uint8_t>(lua_tonumber(L, 3) * 255.0);
    window->m_color.b = static_cast<uint8_t>(lua_tonumber(L, 4) * 255.0);
    return 0;
}

// VERIFY: b3a66eaa manual
// OFFSET: 0x004FBD90
static int32_t Script_GetChatWindowInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChatWindowInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    lua_pushstring(L, s_chatWindows[index].m_name);
    lua_pushnumber(L, s_chatWindows[index].m_unkD0);
    lua_pushnumber(L, s_chatWindows[index].m_color.r * (1.0f / 255.0f));
    lua_pushnumber(L, s_chatWindows[index].m_color.g * (1.0f / 255.0f));
    lua_pushnumber(L, s_chatWindows[index].m_color.b * (1.0f / 255.0f));
    lua_pushnumber(L, s_chatWindows[index].m_color.a * (1.0f / 255.0f));

    if (s_chatWindows[index].m_shown) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (s_chatWindows[index].m_unkD8) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    int32_t docked = s_chatWindows[index].m_docked;

    if (docked) {
        lua_pushnumber(L, docked);
    } else {
        lua_pushnil(L);
    }

    int32_t uninteractable = s_chatWindows[index].m_unkE4;

    if (uninteractable) {
        lua_pushnumber(L, uninteractable);
    } else {
        lua_pushnil(L);
    }

    return 10;
}

// VERIFY: c8fb4c79 manual
// OFFSET: 0x004FE460
static void ChannelCommand(lua_State* L, uint32_t msgId, const char* name) {
    if (!lua_isstring(L, 1)) {
        char usage[512];
        SStrPrintf(usage, sizeof(usage), "Usage: %s(\"channel\")", name);
        luaL_error(L, usage);
        return;
    }

    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return;
    }

    CDataStore msg;
    msg.Put(msgId);
    msg.PutString(channel);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c193a79d auto
// OFFSET: 0x004FFCC0
static int32_t Script_DeclineInvite(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_DECLINE_INVITE, "DeclineInvite");
    return 0;
}

// VERIFY: 96bea800 manual
// OFFSET: 0x00500560
static int32_t Script_SendAddonMessage(lua_State* L) {
    if (!ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        return 0;
    }

    const char* prefix = lua_tolstring(L, 1, nullptr);
    const char* text = lua_tolstring(L, 2, nullptr);

    if (!prefix) {
        prefix = "";
    }

    if (!text) {
        text = "";
    }

    if (!*prefix && !*text) {
        luaL_error(L, "Usage: SendAddonMessage(\"prefix\", \"message\" [,\"type\"] [,\"target\"])");
    }

    char message[3000];
    SStrPrintf(message, sizeof(message), "%s\t%s", prefix, text);
    int32_t type = CHAT_MSG_PARTY;

    if (lua_isstring(L, 3)) {
        if (!StringToChatType(lua_tolstring(L, 3, nullptr), &type)) {
            luaL_error(L, "Unknown addon chat type");
        } else if (type == CHAT_MSG_RAID) {
            if (!CGRaidInfo::m_numMembers) {
                type = CHAT_MSG_PARTY;
            }
        } else if (type != CHAT_MSG_PARTY && type != CHAT_MSG_GUILD && type != CHAT_MSG_BATTLEGROUND && type != CHAT_MSG_WHISPER) {
            luaL_error(L, "Unknown addon chat type");
        }
    }

    if (type == CHAT_MSG_PARTY && !CGPartyInfo::m_members[0]) {
        return 0;
    }

    const char* target = lua_tolstring(L, 4, nullptr);

    if (type == CHAT_MSG_WHISPER && (!target || !*target)) {
        luaL_error(L, "SendAddonMessage(): Whisper message missing target player!");
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_MESSAGECHAT));
    msg.Put(static_cast<uint32_t>(type));
    msg.Put(static_cast<uint32_t>(0xFFFFFFFF));

    if (type == CHAT_MSG_WHISPER) {
        msg.PutString(target);
    }

    msg.PutString(message);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 0487e16e auto
// OFFSET: 0x004FEAF0
static int32_t Script_ChangeChatColor(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4)) {
        luaL_error(L, "Usage: ChangeChatColor(chatType, r, g, b)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    uint8_t r = static_cast<uint8_t>(lua_tonumber(L, 2) * 255.0);
    uint8_t g = static_cast<uint8_t>(lua_tonumber(L, 3) * 255.0);
    uint8_t b = static_cast<uint8_t>(lua_tonumber(L, 4) * 255.0);
    int32_t found = 0;

    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        if (!SStrCmpI(type, s_chatColors[i].m_name, STORM_MAX_STR)) {
            s_chatColors[i].m_color[0] = r;
            s_chatColors[i].m_color[1] = g;
            s_chatColors[i].m_color[2] = b;
            found = 1;
            break;
        }
    }

    for (uint32_t i = 0; i < s_channelColors.Count(); i++) {
        if (!SStrCmpI(type, s_channelColors.m_data[i].m_name, STORM_MAX_STR)) {
            s_channelColors.m_data[i].m_color[0] = r;
            s_channelColors.m_data[i].m_color[1] = g;
            s_channelColors.m_data[i].m_color[2] = b;
            found = 1;
            break;
        }
    }

    if (found) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", type, r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f));
    }

    return 0;
}

// VERIFY: 9136ba27 auto
// OFFSET: 0x004FEDC0
static int32_t Script_SetChatColorNameByClass(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetChatColorNameByClass(chatType, colorNameByClass)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);
    bool colorNameByClass = StringToBOOL(L, 2, 0) != 0;
    int32_t found = 0;

    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        if (!SStrCmpI(type, s_chatColors[i].m_name, STORM_MAX_STR)) {
            s_chatColors[i].m_unk48 = colorNameByClass;
            found = 1;
            break;
        }
    }

    for (uint32_t i = 0; i < s_channelColors.Count(); i++) {
        if (!SStrCmpI(type, s_channelColors.m_data[i].m_name, STORM_MAX_STR)) {
            s_channelColors.m_data[i].m_unk48 = colorNameByClass;
            found = 1;
            break;
        }
    }

    if (found) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", type, colorNameByClass);
    }

    return 0;
}

// VERIFY: c3f85278 manual
// OFFSET: 0x004FEEE0
static int32_t Script_GetChatTypeIndex(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetChatTypeIndex(type)");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        if (!SStrCmpI(type, s_chatColors[i].m_name, STORM_MAX_STR)) {
            lua_pushnumber(L, i + 1);
            return 1;
        }
    }

    for (uint32_t i = 0; i < s_channelColors.Count(); i++) {
        if (!SStrCmpI(type, s_channelColors.m_data[i].m_name, STORM_MAX_STR)) {
            lua_pushnumber(L, i + CHAT_MSG_MAX + 1);
            return 1;
        }
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: d8fe385f auto
// OFFSET: 0x004FE650
static int32_t Script_GetChannelList(lua_State* L) {
    int32_t count = 0;

    for (uint32_t i = 0; i < CGChat::s_channels.Count(); i++) {
        ChatChannel* channel = &CGChat::s_channels.m_data[i];

        if (channel->m_channelNumber && !channel->m_unk9C) {
            lua_pushnumber(L, channel->m_channelNumber);
            ChatChannelsRec* rec = channel->m_zoneChannelID ? g_chatChannelsDB.GetRecord(channel->m_zoneChannelID) : nullptr;

            if (rec) {
                lua_pushstring(L, rec->m_shortcut);
            } else {
                lua_pushstring(L, channel->m_name);
            }

            count++;
        }
    }

    return 2 * count;
}

// VERIFY: 1a9f7d0e manual
// OFFSET: 0x004FE850
static int32_t Script_GetChannelName(lua_State* L) {
    ChatChannel* channel;

    if (lua_isnumber(L, 1)) {
        int32_t number = static_cast<int32_t>(lua_tonumber(L, 1));

        if (number < 1 || number > static_cast<int32_t>(CGChat::s_channels.Count())) {
            channel = nullptr;
        } else {
            channel = &CGChat::s_channels.m_data[number - 1];

            if (channel->m_channelNumber != number || channel->m_unk9C) {
                channel = nullptr;
            }
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetChannelName([channelIndex] or [channelName])");
            return 0;
        }

        channel = GetChannelByName(lua_tolstring(L, 1, nullptr));
    }

    lua_pushnumber(L, channel ? channel->m_channelNumber : 0);
    lua_pushstring(L, channel ? channel->m_name : nullptr);
    lua_pushnumber(L, channel ? channel->m_unk98 : 0);
    return 3;
}

// VERIFY: 2937169a auto
// OFFSET: 0x004FCC60
static int32_t Script_IsVoiceChatAllowed(lua_State* L) {
    if (!CGChat::m_voiceChatSystemStatus || ComSatClient_IsComsatDisabledByClient()) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: b67ad7f0 auto
// OFFSET: 0x004FCBF0
static int32_t Script_IsVoiceChatEnabled(lua_State* L) {
    int32_t enabled = s_cvarEnableVoiceChat && s_cvarEnableVoiceChat->m_intValue;
    int32_t disabledByClient = ComSatClient_IsComsatDisabledByClient();

    if (CGChat::m_voiceChatSystemStatus && enabled && !disabledByClient) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 055b4930 manual
// OFFSET: 0x004FC0C0
static int32_t Script_GetChatWindowMessages(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChatWindowMessages(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    int32_t count = 0;

    for (uint32_t i = 0; i < 46; i++) {
        if (s_chatWindows[index].m_messageGroups[i]) {
            lua_checkstack(L, 1);
            lua_pushstring(L, s_chatMessageGroups[i].m_name);
            count++;
        }
    }

    return count;
}

// VERIFY: f915b6c6 manual
// OFFSET: 0x004FC170
static int32_t Script_AddChatWindowMessages(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: AddChatWindowMessages(index, \"messageGroup\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    const char* group = lua_tolstring(L, 2, nullptr);

    for (uint32_t i = 0; i < 46; i++) {
        if (!SStrCmpI(group, s_chatMessageGroups[i].m_name, STORM_MAX_STR)) {
            s_chatWindows[index].m_messageGroups[i] = 1;
            return 0;
        }
    }

    return 0;
}

// VERIFY: c7212d54 manual
// OFFSET: 0x004FC250
static int32_t Script_RemoveChatWindowMessages(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: RemoveChatWindowMessages(index, \"messageGroup\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    const char* group = lua_tolstring(L, 2, nullptr);

    for (uint32_t i = 0; i < 46; i++) {
        if (!SStrCmpI(group, s_chatMessageGroups[i].m_name, STORM_MAX_STR)) {
            s_chatWindows[index].m_messageGroups[i] = 0;
            return 0;
        }
    }

    return 0;
}

// VERIFY: c470c6cf manual
// OFFSET: 0x004FF580
static int32_t Script_GetChannelDisplayInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChannelDisplayInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= s_numDisplayChannels) {
        return 0;
    }

    DisplayChannel* display = &CGChat::s_displayChannels[index];

    if (display->m_headerIndex >= 0) {
        lua_pushstring(L, FrameScript_GetText(s_channelCategories[display->m_headerIndex], -1, GENDER_NOT_APPLICABLE));
        lua_pushnumber(L, 1.0);

        if (display->m_collapsed) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        lua_pushnil(L);

        if (display->m_count) {
            lua_pushnumber(L, display->m_count);
        } else {
            lua_pushnil(L);
        }

        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 9;
    }

    int32_t channelIndex = display->m_channelIndex;

    if (channelIndex < 0) {
        const char* token;

        switch (channelIndex) {
            case -1:
                token = "CHAT_MSG_PARTY";
                break;

            case -2:
                token = "CHAT_MSG_RAID";
                break;

            case -3:
                token = "CHAT_MSG_BATTLEGROUND";
                break;

            default:
                for (int32_t i = 0; i < 9; i++) {
                    lua_pushnil(L);
                }

                return 9;
        }

        lua_pushstring(L, FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE));
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);

        if (display->m_count) {
            lua_pushnumber(L, display->m_count);
        } else {
            lua_pushnil(L);
        }

        if (display->m_enabled) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        lua_pushstring(L, s_channelCategories[0]);
    } else {
        const char* name = CGChat::GetChannelShortName(channelIndex + 1, true);
        lua_pushstring(L, name ? name : "");
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, CGChat::s_channels.m_data[display->m_channelIndex].m_channelNumber);

        if (display->m_count) {
            lua_pushnumber(L, display->m_count);
        } else {
            lua_pushnil(L);
        }

        if (display->m_enabled) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        lua_pushstring(L, display->m_flags & 1 ? s_channelCategories[1] : s_channelCategories[2]);
    }

    if (display->m_voiceEnabled) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (static_cast<uint32_t>(s_activeVoiceChannel) == index) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 9;
}

// VERIFY: 8f46d408 auto
// OFFSET: 0x004FFA10
static int32_t Script_GetSelectedDisplayChannel(lua_State* L) {
    int32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

    if (!index) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, index);
    return 1;
}

// VERIFY: 0e4cb043 manual
// OFFSET: 0x004FEFF0
static int32_t Script_GetChatWindowChannels(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetChatWindowChannels(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    ChatWindowInfo& window = s_chatWindows[index];
    int32_t count = 0;

    for (uint32_t i = 0; i < window.m_channels.Count(); i++) {
        if (window.m_channels.m_data[i]) {
            lua_checkstack(L, 2);
            lua_pushstring(L, window.m_channels.m_data[i]);
            lua_pushnumber(L, window.m_channelIDs.m_data[i]);
            count++;
        }
    }

    return 2 * count;
}

// VERIFY: 75fea573 manual
// OFFSET: 0x004FCA50
static int32_t Script_GetNumSavedInstances(lua_State* L) {
    lua_pushnumber(L, s_numSavedInstances);
    return 1;
}

// VERIFY: ed393229 auto
// OFFSET: 0x004FCDF0
static int32_t Script_GetMuteStatus(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return 0;
    }

    WOWGUID guid;

    if (FriendList::s_friendList && (guid = WOWGUID(), CGChat::ChatTokenToGUID(lua_tolstring(L, 1, nullptr), guid), guid) && (FriendList::s_friendList->IsIgnoredOrMuted(guid) || (lua_isstring(L, 2) ? ComSatClient_IsTalkerSquelchedInChannel(guid, ComSatClient_DetermineSessionTypeFromName(lua_tolstring(L, 2, nullptr)), lua_tolstring(L, 2, nullptr)) : ComSatClient_IsTalkerSquelchedInCurrent(guid)))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5681a1a2 manual
// OFFSET: 0x00504F30
static int32_t Script_GetChannelRosterInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        return 0;
    }

    int32_t type;
    const char* name = CGChat::GetChannelStringForDisplay(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, &type);

    if (!name || type == 4) {
        return 0;
    }

    if (!ChannelNamesEqual(CGChat::s_activeDisplayChannelName, name, s_activeDisplayChannelType, type)) {
        SetCurrentActiveDisplayChannel(name, type);

        switch (type) {
            case 0:
                ChannelDisplayCommand(L, CMSG_CHAT_CHANNEL_DISPLAY_LIST, "GetChannelRosterInfo");
                return 0;

            case 2:
                BuildPartyDisplayList();
                return 0;

            case 1:
            case 3:
                BuildRaidDisplayList(type);
                break;
        }

        return 0;
    }

    if (s_displayChannelMemberCount) {
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (index >= s_pendingUsers.Count()) {
        return 0;
    }

    PENDINGUSERLIST* user = &s_pendingUsers.m_data[index];

    if (!user) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(user->m_guid.m_guid, none, nullptr, nullptr, false);

    if (record && record->m_name[0]) {
        lua_pushstring(L, record->m_name);
    } else {
        lua_pushstring(L, FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE));
    }

    if (user->m_flags & 0x1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (user->m_flags & 0x2) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (user->m_flags & 0x20) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (user->m_flags & 0x40) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (user->m_flags & 0x80) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 6;
}

// STATIC: 0x00AC7A40
static const char* s_chatLogNames[2] = { "Logs\\WoWChatLog.txt", "Logs\\WoWCombatLog.txt" };

// VERIFY: 2b604692 auto
// OFFSET: 0x004FBC60
static int32_t EnableLogging(lua_State* L, int32_t index) {
    if (lua_gettop(L) > 0) {
        s_chatLogging[index] = StringToBOOL(L, 1, 1);

        if (s_chatLogging[index] && !s_chatLog[index]) {
            OsCreateDirectory("Logs", 0);

            if (!SLogCreate(s_chatLogNames[index], 4, &s_chatLog[index])) {
                s_chatLogging[index] = 0;
            }
        }
    }

    if (s_chatLogging[index]) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: feb7e35b auto
// OFFSET: 0x004FBD00
static int32_t Script_LoggingChat(lua_State* L) {
    return EnableLogging(L, 0);
}

// VERIFY: 70d23c41 auto
// OFFSET: 0x004FBD20
static int32_t Script_LoggingCombat(lua_State* L) {
    return EnableLogging(L, 1);
}

// VERIFY: c5c8be87 auto
// OFFSET: 0x004FCCF0
static int32_t Script_UnitIsSilenced(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return 0;
    }

    WOWGUID guid = WOWGUID();
    const char* name = lua_tolstring(L, 1, nullptr);
    NameCache* record = g_nameDBCache.GetRecordByName(name);

    if (record) {
        guid = record->m_guid;
    } else {
        Script_GetGUIDFromToken(name, guid, false);
    }

    if (guid) {
        int32_t silenced;

        if (lua_type(L, 2)) {
            int32_t type = ComSatClient_DetermineSessionTypeFromName(lua_tolstring(L, 2, nullptr));
            silenced = ComSatClient_IsTalkerSquelchedInChannel(guid, type, lua_tolstring(L, 2, nullptr));
        } else {
            silenced = ComSatClient_IsTalkerSquelchedInCurrent(guid);
        }

        if (silenced) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3c1405f5 manual
// OFFSET: 0x004FE330
static void ChannelPlayerCommand(lua_State* L, uint32_t msgId, const char* name) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        char usage[512];
        SStrPrintf(usage, sizeof(usage), "Usage: %s(\"channel\", \"name\")", name);
        luaL_error(L, usage);
        return;
    }

    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return;
    }

    const char* player = lua_tolstring(L, 2, nullptr);

    if (player && SStrLen(player) > 48) {
        luaL_error(L, "Name too long");
        return;
    }

    CDataStore msg;
    msg.Put(msgId);
    msg.PutString(channel);
    msg.PutString(player);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: a6a85309 auto
// OFFSET: 0x004FE630
static int32_t Script_ListChannelByName(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_LIST, "ListChannelByName");
    return 0;
}

// VERIFY: 26c935c6 manual
// OFFSET: 0x004FE720
static int32_t Script_SetChannelPassword(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetChannelPassword(\"name\", \"password\")");
        return 0;
    }

    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAT_CHANNEL_PASSWORD));
    msg.PutString(channel);
    msg.PutString(lua_tolstring(L, 2, nullptr));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: e28cce39 auto
// OFFSET: 0x004FE810
static int32_t Script_SetChannelOwner(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_SET_OWNER, "SetChannelOwner");
    return 0;
}

// VERIFY: 790ff67b auto
// OFFSET: 0x004FE830
static int32_t Script_DisplayChannelOwner(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_OWNER, "DisplayChannelOwner");
    return 0;
}

// VERIFY: 9a576864 auto
// OFFSET: 0x004FE950
static int32_t Script_ChannelModerator(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_MODERATOR, "ChannelModerator");
    return 0;
}

// VERIFY: 93b3844b auto
// OFFSET: 0x004FE970
static int32_t Script_ChannelUnmoderator(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_UNMODERATOR, "ChannelUnmoderator");
    return 0;
}

// VERIFY: c8fe72f5 auto
// OFFSET: 0x004FE990
static int32_t Script_ChannelMute(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_MUTE, "ChannelMute");
    return 0;
}

// VERIFY: b0eb619c auto
// OFFSET: 0x004FE9B0
static int32_t Script_ChannelUnmute(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_UNMUTE, "ChannelUnmute");
    return 0;
}

// VERIFY: 3e482345 auto
// OFFSET: 0x004FE9D0
static int32_t Script_ChannelInvite(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_INVITE, "ChannelInvite");
    return 0;
}

// VERIFY: e731c8b1 auto
// OFFSET: 0x004FE9F0
static int32_t Script_ChannelKick(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_KICK, "ChannelKick");
    return 0;
}

// VERIFY: b999d424 auto
// OFFSET: 0x004FEA10
static int32_t Script_ChannelBan(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_BAN, "ChannelBan");
    return 0;
}

// VERIFY: 1fa630d1 auto
// OFFSET: 0x004FEA30
static int32_t Script_ChannelUnban(lua_State* L) {
    ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_UNBAN, "ChannelUnban");
    return 0;
}

// VERIFY: df0661fb auto
// OFFSET: 0x004FEA50
static int32_t Script_ChannelToggleAnnouncements(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_ANNOUNCEMENTS, "ChannelToggleAnnouncements");
    return 0;
}

// VERIFY: 2cbb7fbd auto
// OFFSET: 0x004FEA70
static int32_t Script_ChannelVoiceOn(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_VOICE_ON, "ChannelVoiceOn");
    return 0;
}

// VERIFY: 3973fbf1 auto
// OFFSET: 0x004FEA90
static int32_t Script_ChannelVoiceOff(lua_State* L) {
    ChannelCommand(L, CMSG_CHAT_CHANNEL_VOICE_OFF, "ChannelVoiceOff");
    return 0;
}

// VERIFY: f4e8a33f auto
// OFFSET: 0x004FEAB0
static int32_t Script_DisplayChannelVoiceOn(lua_State* L) {
    ChannelDisplayCommand(L, CMSG_CHAT_CHANNEL_VOICE_ON, "DisplayChannelVoiceOn");
    return 0;
}

// VERIFY: 34bb18fb auto
// OFFSET: 0x004FEAD0
static int32_t Script_DisplayChannelVoiceOff(lua_State* L) {
    ChannelDisplayCommand(L, CMSG_CHAT_CHANNEL_VOICE_OFF, "DisplayChannelVoiceOff");
    return 0;
}

// VERIFY: 4923e2e0 manual
// OFFSET: 0x004FED10
static int32_t Script_ResetChatColors(lua_State* L) {
    for (uint32_t i = 0; i < CHAT_MSG_MAX; i++) {
        SStrCopy(s_chatColors[i].m_name, s_chatTypeInfo[i].m_name, sizeof(s_chatColors[i].m_name));
        s_chatColors[i].m_color[0] = s_chatTypeInfo[i].m_color[0];
        s_chatColors[i].m_color[1] = s_chatTypeInfo[i].m_color[1];
        s_chatColors[i].m_color[2] = s_chatTypeInfo[i].m_color[2];
    }

    for (uint32_t i = 0; i < s_channelColors.Count(); i++) {
        s_channelColors.m_data[i].m_color[0] = s_chatColors[CHAT_MSG_CHANNEL].m_color[0];
        s_channelColors.m_data[i].m_color[1] = s_chatColors[CHAT_MSG_CHANNEL].m_color[1];
        s_channelColors.m_data[i].m_color[2] = s_chatColors[CHAT_MSG_CHANNEL].m_color[2];
    }

    return 0;
}

// VERIFY: dac42b84 manual
// OFFSET: 0x004FF0D0
static int32_t Script_RemoveChatWindowChannel(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: RemoveChatWindowChannel(index, \"channel\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 2, nullptr));

    if (!channel) {
        return 0;
    }

    const char* zoneText = CGGameUI::m_zoneText;

    for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (SStrCmpUTF8I(rec->m_shortcut, channel, STORM_MAX_STR)) {
            continue;
        }

        if (!zoneText || !*zoneText) {
            return 0;
        }

        if (rec->m_shortcut) {
            s_chatWindows[index].RemoveChannel(rec->m_shortcut);
            return 0;
        }

        break;
    }

    s_chatWindows[index].RemoveChannel(channel);
    return 0;
}

// VERIFY: 42e197eb manual
// OFFSET: 0x004FF200
static int32_t Script_EnumerateServerChannels(lua_State* L) {
    int32_t count = g_chatChannelsDB.m_numRecords;
    lua_checkstack(L, count);
    AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    if (!area) {
        return 0;
    }

    int32_t pushed = 0;

    for (int32_t i = 0; i < count; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (!(rec->m_flags & 0x10) || (area->m_flags & 0x8)) {
            lua_pushstring(L, *rec->m_shortcut ? rec->m_shortcut : rec->m_name);
            pushed++;
        }
    }

    return pushed;
}

// VERIFY: 73493072 manual
// OFFSET: 0x004FF2C0
static int32_t Script_GetSavedInstanceInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetSavedInstanceInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    MapRec* map;

    if (index >= s_numSavedInstances || !(map = g_mapDB.GetRecord(s_savedInstances.m_data[index].m_mapID))) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushboolean(L, 0);
        lua_pushboolean(L, 0);
        lua_pushnumber(L, 0.0);
        lua_pushboolean(L, 0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        return 10;
    }

    RaidInstanceInfo* info = &s_savedInstances.m_data[index];
    int32_t mapId = info->m_mapID;
    int32_t difficulty = info->m_difficulty;
    lua_pushstring(L, map->m_mapName);
    lua_pushnumber(L, static_cast<uint32_t>(info->m_instanceID));
    lua_pushnumber(L, info->m_resetTime);
    lua_pushnumber(L, difficulty + 1);
    lua_pushboolean(L, info->m_locked);
    lua_pushboolean(L, info->m_extended);
    lua_pushnumber(L, static_cast<uint32_t>(info->m_instanceID >> 32));
    lua_pushboolean(L, map->m_instanceType == 2);
    lua_pushnumber(L, GetMaxPlayersForMap(map, info->m_difficulty));
    MapDifficultyRec* entry = GetMapDifficultyEntryByMapIdAndDifficulty(mapId, difficulty, nullptr);

    if (!entry) {
        lua_pushstring(L, "");
        return 10;
    }

    lua_pushstring(L, FrameScript_GetText(entry->m_difficultystring, -1, GENDER_NOT_APPLICABLE));
    return 10;
}

// VERIFY: d4942645 auto
// OFFSET: 0x004FFA70
static int32_t Script_IsDisplayChannelOwner(lua_State* L) {
    uint32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

    if (index && CGChat::s_displayChannels[index - 1].m_headerIndex < 0 && (s_activeDisplayChannelFlags & 0x1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e21c4119 auto
// OFFSET: 0x004FFAE0
static int32_t Script_IsDisplayChannelModerator(lua_State* L) {
    uint32_t index = CGChat::FindDisplayChannelIndex(CGChat::s_activeDisplayChannelName, s_activeDisplayChannelType);

    if (index && CGChat::s_displayChannels[index - 1].m_headerIndex < 0 && (s_activeDisplayChannelFlags & 0x3)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e3f46617 manual
// OFFSET: 0x004FFB50
static int32_t Script_IsSilenced(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return 0;
    }

    WOWGUID guid = WOWGUID();
    const char* name = lua_tolstring(L, 1, nullptr);
    NameCache* record = g_nameDBCache.GetRecordByName(name);

    if (record) {
        guid = record->m_guid;
    } else {
        Script_GetGUIDFromToken(name, guid, false);
    }

    if (guid) {
        for (uint32_t i = 0; i < s_pendingUsers.Count(); i++) {
            if (s_pendingUsers.m_data[i].m_guid != guid) {
                continue;
            }

            if (!(s_pendingUsers.m_data[i].m_flags & 0x10)) {
                lua_pushnumber(L, 1.0);
                return 1;
            }

            break;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 18b5285f auto
// OFFSET: 0x004FFC30
static int32_t Script_SetChannelWatch(lua_State* L) {
    ChannelCommand(L, CMSG_SET_CHANNEL_WATCH, "SetChannelWatch");
    return 0;
}

// VERIFY: 4d3fdf6e auto
// OFFSET: 0x00500980
static int32_t Script_DoEmote(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: DoEmote(\"emote\"[, \"target\"])");
        return 0;
    }

    const char* emote = lua_tolstring(L, 1, nullptr);
    const char* dance;

    if (!SStrCmpI(emote, "DANCE", STORM_MAX_STR) && (dance = lua_tolstring(L, 2, nullptr)) && CGDanceStudio::HasDance(dance)) {
        PlayDance(dance);
        return 0;
    }

    int32_t i = g_emotesTextDB.m_numRecords;

    if (!i) {
        return 0;
    }

    EmotesTextRec* rec;

    while (true) {
        i--;
        rec = g_emotesTextDB.GetRecordByIndex(i);

        if (!SStrCmpI(emote, rec->m_name, STORM_MAX_STR)) {
            break;
        }

        if (!i) {
            return 0;
        }
    }

    WOWGUID target = CGGameUI::m_lockedTarget;
    const char* token = lua_tolstring(L, 2, nullptr);

    if (token && *token && *token != '%') {
        target = CGGameUI::ClosestObjectMatch(token, TYPE_PLAYER, 0, 0, 3.4028235e38f, 0);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->SendTextEmote(rec, target);
    }

    return 0;
}

// VERIFY: 0d3e664e manual
// OFFSET: 0x00500AE0
static int32_t Script_SetActiveVoiceChannel(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    if (lua_tonumber(L, 1)) {
        int32_t type;
        int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
        const char* channel = CGChat::GetChannelStringForDisplay(index, &type);

        if (channel && CGChat::s_displayChannels[index].m_voiceEnabled) {
            CGChat::SetActiveVoiceSession(channel, type, index);
        }

        return 0;
    }

    if (CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d%d", s_activeVoiceChannel + 1, CGChat::s_displayChannels[s_activeVoiceChannel].m_voiceEnabled);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_CHANNEL_VOICE_UPDATE, "%d", s_activeVoiceChannel + 1);
    }

    s_activeVoiceChannel = 0;
    ComSatClient_DisableCurrentSession();
    return 0;
}

// VERIFY: 9e73392f auto
// OFFSET: 0x00500C80
static int32_t Script_CanComplainChat(lua_State* L) {
    const char* text = nullptr;
    ChatRecord* record;

    if (lua_isnumber(L, 1)) {
        record = GetChatRecordEx(nullptr, nullptr, static_cast<uint32_t>(lua_tonumber(L, 1)));
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: CanComplainChat(lineID) or CanComplainChat(name) or CanComplainChat(name, text)");
            return 0;
        }

        const char* name = lua_tolstring(L, 1, nullptr);

        if (lua_isstring(L, 2)) {
            text = lua_tolstring(L, 2, nullptr);
        }

        record = GetChatRecordEx(name, text, 0);
    }

    if (record && CGChat::CanComplain(record->m_senderGuid)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 3ec7fd25 auto
// OFFSET: 0x00501140
static int32_t Script_ChannelSilenceVoice(lua_State* L) {
    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return 0;
    }

    if (SStrCmpI(channel, s_sessionTypeNames[1], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[2], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[0], STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("PARTY", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("RAID", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("BATTLEFIELDS", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR)) {
        ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_SILENCE_VOICE, "ChannelSilenceVoice");
        return 0;
    }

    CGChat::SilencePartyMember(true, lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 0fd7e161 auto
// OFFSET: 0x00501250
static int32_t Script_ChannelSilenceAll(lua_State* L) {
    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return 0;
    }

    if (SStrCmpI(channel, s_sessionTypeNames[1], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[2], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[0], STORM_MAX_STR)) {
        ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_SILENCE_ALL, "ChannelSilenceAll");
        return 0;
    }

    CGChat::SilencePartyMember(true, lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: a21194b2 auto
// OFFSET: 0x005012F0
static int32_t Script_ChannelUnSilenceVoice(lua_State* L) {
    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return 0;
    }

    if (SStrCmpI(channel, s_sessionTypeNames[1], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[2], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[0], STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("PARTY", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("RAID", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR) && SStrCmpI(channel, FrameScript_GetText("BATTLEFIELDS", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR)) {
        ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_UNSILENCE_VOICE, "ChannelUnSilenceVoice");
        return 0;
    }

    CGChat::SilencePartyMember(false, lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: b7b4f70f auto
// OFFSET: 0x00501400
static int32_t Script_ChannelUnSilenceAll(lua_State* L) {
    const char* channel = CGChat::GetChannelString(lua_tolstring(L, 1, nullptr));

    if (!channel) {
        return 0;
    }

    if (SStrCmpI(channel, s_sessionTypeNames[1], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[2], STORM_MAX_STR) && SStrCmpI(channel, s_sessionTypeNames[0], STORM_MAX_STR)) {
        ChannelPlayerCommand(L, CMSG_CHAT_CHANNEL_UNSILENCE_ALL, "ChannelUnSilenceAll");
        return 0;
    }

    CGChat::SilencePartyMember(false, lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 53a0772a manual
// OFFSET: 0x00501DB0
static int32_t Script_AddChatWindowChannel(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: AddChatWindowChannel(index, \"channel\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 10) {
        return 0;
    }

    const char* channel = lua_tolstring(L, 2, nullptr);
    const char* zoneChannel = nullptr;
    int32_t channelId = 0;
    const char* zoneText = CGGameUI::m_zoneText;

    for (int32_t i = 0; i < g_chatChannelsDB.m_numRecords; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (SStrCmpUTF8I(rec->m_shortcut, channel, STORM_MAX_STR)) {
            continue;
        }

        if (!zoneText || !*zoneText) {
            return 0;
        }

        zoneChannel = rec->m_shortcut;
        channelId = rec->m_id;
        break;
    }

    lua_pushnumber(L, channelId);
    s_chatWindows[index].AddChannel(zoneChannel ? zoneChannel : channel, channelId);
    return 1;
}

// VERIFY: fcd817cd manual
// OFFSET: 0x00501F00
static int32_t Script_ResetChatWindows(lua_State* L) {
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

    for (uint32_t i = 0; i < 10; i++) {
        memset(s_chatWindows[i].m_messageGroups, 0, sizeof(s_chatWindows[i].m_messageGroups));
        s_chatWindows[i].Reset();
        s_chatWindows[i].Init();

        if (i == 0) {
            s_chatWindows[0].m_shown = 1;

            for (uint32_t j = 0; j < 40; j++) {
                s_chatWindows[0].m_messageGroups[j] = s_chatMessageGroups[j].m_default != 0;
            }

            s_chatWindows[0].m_docked = 1;
        } else if (i == 1) {
            s_chatWindows[1].m_shown = 1;
            s_chatWindows[1].m_docked = 2;
            s_chatWindows[1].m_messageGroups[40] = 1;
            s_chatWindows[1].m_messageGroups[41] = 1;
            s_chatWindows[1].m_messageGroups[42] = 1;
            s_chatWindows[1].m_messageGroups[43] = 1;
            s_chatWindows[1].m_messageGroups[44] = 1;
            s_chatWindows[1].m_messageGroups[45] = 1;
        }
    }

    int32_t count = g_chatChannelsDB.m_numRecords;

    for (int32_t i = 0; i < count; i++) {
        ChatChannelsRec* rec = g_chatChannelsDB.GetRecordByIndex(i);

        if (!(rec->m_flags & 0x1)) {
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

    for (uint32_t i = 0; i < s_channelColors.Count(); i++) {
        ChatColorInfo* color = &s_channelColors.m_data[i];
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR, "%s%f%f%f", color->m_name, color->m_color[0] * (1.0f / 255.0f), color->m_color[1] * (1.0f / 255.0f), color->m_color[2] * (1.0f / 255.0f));
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_CHAT_COLOR_NAME_BY_CLASS, "%s%b", color->m_name, color->m_unk48);
    }

    return 0;
}

// VERIFY: 08d3e881 manual
// OFFSET: 0x00505190
static int32_t Script_GetNumChannelMembers(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        lua_pushnil(L);
        return 1;
    }

    int32_t type;
    const char* channel = CGChat::GetChannelStringForDisplay(static_cast<int32_t>(lua_tonumber(L, 1)) - 1, &type);

    if (!channel || type == 4) {
        lua_pushnil(L);
        return 1;
    }

    uint32_t count;

    if (!ChannelNamesEqual(CGChat::s_activeDisplayChannelName, channel, s_activeDisplayChannelType, type)) {
        SetCurrentActiveDisplayChannel(channel, type);

        switch (type) {
            case 0:
                ChannelDisplayCommand(L, CMSG_CHAT_CHANNEL_DISPLAY_LIST, "GetNumChannelMembers");
                lua_pushnil(L);
                return 1;

            case 2:
                count = BuildPartyDisplayList();
                break;

            case 1:
            case 3:
                count = BuildRaidDisplayList(type);
                break;

            default:
                lua_pushnil(L);
                return 1;
        }
    } else {
        if (s_displayChannelMemberCount) {
            lua_pushnil(L);
            return 1;
        }

        count = s_pendingUsers.Count();
    }

    if (!count) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 4c22838b manual
// OFFSET: 0x005096D0
static int32_t Script_LeaveChannelByName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: LeaveChannelByName(\"name\")");
        return 0;
    }

    InternalLeaveChannel(lua_tolstring(L, 1, nullptr), true, false);
    return 0;
}

// VERIFY: 64995a1c manual
// OFFSET: 0x0050D900
static int32_t JoinChannelFromScript(lua_State* L, int32_t permanent) {
    if (!lua_isstring(L, 1)) {
        char function[1024];
        luaL_error(L, "Usage: %s(\"name\" [,\"password\"] [,index] [,hasVoice])", FrameScript_GetCurrentFunction(L, function, sizeof(function)));
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    const char* password = lua_isstring(L, 2) ? lua_tolstring(L, 2, nullptr) : "";

    if (SStrChr(name, ' ') || SStrLen(name) >= 128 || SStrLen(password) >= 128) {
        return 0;
    }

    char filtered[128];
    strcpy(filtered, name);

    if (ProfanityFilter(filtered, false, true)) {
        CGGameUI::DisplayError(ERR_PROFANE_CHAT_NAME);
        return 0;
    }

    uint32_t window = 0xFFFFFFFF;

    if (lua_isnumber(L, 3)) {
        window = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    }

    int32_t voice = StringToBOOL(L, 4, 0);
    const char* zoneChannelName = nullptr;
    int32_t zoneChannelID = 0;

    if (!InternalJoinChannel(name, password, window, true, &zoneChannelName, &zoneChannelID, false, voice, permanent, 0)) {
        return 0;
    }

    lua_pushnumber(L, zoneChannelID);

    if (zoneChannelName) {
        lua_pushstring(L, zoneChannelName);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 31cbcc26 auto
// OFFSET: 0x0050DAA0
static int32_t Script_JoinChannelByName(lua_State* L) {
    return JoinChannelFromScript(L, 0);
}

// VERIFY: c8a46eac auto
// OFFSET: 0x0050DAA0
static int32_t Script_JoinTemporaryChannel(lua_State* L) {
    return JoinChannelFromScript(L, 0);
}

// VERIFY: feab0619 auto
// OFFSET: 0x0050DAC0
static int32_t Script_JoinPermanentChannel(lua_State* L) {
    return JoinChannelFromScript(L, 1);
}

// VERIFY: ccef5a4d manual
// OFFSET: 0x0050DC10
static int32_t Script_SetSavedInstanceExtend(lua_State* L) {
    if (!lua_isnumber(L, 1) || lua_type(L, 2) != LUA_TBOOLEAN) {
        luaL_error(L, "Usage: SetSavedInstanceExtend(index, extend)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    int32_t extend = lua_toboolean(L, 2);

    if (index >= s_numSavedInstances) {
        return 0;
    }

    RaidInstanceInfo* info = &s_savedInstances.m_data[index];
    info->m_extended = extend;
    char mapName[1024];
    CGRaidInfo::GetMapNameWithDifficulty(mapName, sizeof(mapName), info->m_mapID, info->m_difficulty);
    const char* format;

    if (extend) {
        format = FrameScript_GetText("RAID_INSTANCE_LOCK_EXTENDED", -1, GENDER_NOT_APPLICABLE);
    } else {
        format = FrameScript_GetText("RAID_INSTANCE_LOCK_NOT_EXTENDED", -1, GENDER_NOT_APPLICABLE);
    }

    char text[1024];
    SStrPrintf(text, sizeof(text), format, mapName);
    CGChat::AddChatMessage(text, CHAT_MSG_SYSTEM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_SAVED_INSTANCE_EXTEND));
    msg.Put(info->m_mapID);
    msg.Put(info->m_difficulty);
    msg.Put(static_cast<uint8_t>(extend != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: eccacdba manual
// OFFSET: 0x0050ED00
static int32_t Script_ComplainChat(lua_State* L) {
    const char* text = nullptr;

    if (lua_isnumber(L, 1)) {
        ComplainChat(nullptr, nullptr, static_cast<uint32_t>(lua_tonumber(L, 1)));
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: ComplainChat(lineID) or ComplainChat(name) or ComplainChat(name, text)");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (lua_isstring(L, 2)) {
        text = lua_tolstring(L, 2, nullptr);
    }

    ComplainChat(name, text, 0);
    return 0;
}

// STATIC: 0x00AC7A58
static FrameScript_Method s_ScriptFunctions[] = {
    { "SendChatMessage", &Script_SendChatMessage },
    { "SendAddonMessage", &Script_SendAddonMessage },
    { "SendSystemMessage", &Script_SendSystemMessage },
    { "GetNumLanguages", &Script_GetNumLanguages },
    { "GetLanguageByIndex", &Script_GetLanguageByIndex },
    { "GetDefaultLanguage", &Script_GetDefaultLanguage },
    { "DoEmote", &Script_DoEmote },
    { "LoggingChat", &Script_LoggingChat },
    { "LoggingCombat", &Script_LoggingCombat },
    { "JoinChannelByName", &Script_JoinChannelByName },
    { "JoinTemporaryChannel", &Script_JoinTemporaryChannel },
    { "JoinPermanentChannel", &Script_JoinPermanentChannel },
    { "LeaveChannelByName", &Script_LeaveChannelByName },
    { "ListChannelByName", &Script_ListChannelByName },
    { "ListChannels", &Script_ListChannels },
    { "GetChannelList", &Script_GetChannelList },
    { "SetChannelPassword", &Script_SetChannelPassword },
    { "SetChannelOwner", &Script_SetChannelOwner },
    { "DisplayChannelOwner", &Script_DisplayChannelOwner },
    { "GetChannelName", &Script_GetChannelName },
    { "ChannelModerator", &Script_ChannelModerator },
    { "ChannelUnmoderator", &Script_ChannelUnmoderator },
    { "ChannelMute", &Script_ChannelMute },
    { "ChannelUnmute", &Script_ChannelUnmute },
    { "ChannelInvite", &Script_ChannelInvite },
    { "ChannelKick", &Script_ChannelKick },
    { "ChannelBan", &Script_ChannelBan },
    { "ChannelUnban", &Script_ChannelUnban },
    { "ChannelToggleAnnouncements", &Script_ChannelToggleAnnouncements },
    { "ChannelSilenceVoice", &Script_ChannelSilenceVoice },
    { "ChannelSilenceAll", &Script_ChannelSilenceAll },
    { "ChannelUnSilenceVoice", &Script_ChannelUnSilenceVoice },
    { "ChannelUnSilenceAll", &Script_ChannelUnSilenceAll },
    { "ChangeChatColor", &Script_ChangeChatColor },
    { "ResetChatColors", &Script_ResetChatColors },
    { "GetChatTypeIndex", &Script_GetChatTypeIndex },
    { "GetChatWindowInfo", &Script_GetChatWindowInfo },
    { "GetChatWindowSavedPosition", &Script_GetChatWindowSavedPosition },
    { "GetChatWindowSavedDimensions", &Script_GetChatWindowSavedDimensions },
    { "GetChatWindowMessages", &Script_GetChatWindowMessages },
    { "GetChatWindowChannels", &Script_GetChatWindowChannels },
    { "AddChatWindowMessages", &Script_AddChatWindowMessages },
    { "RemoveChatWindowMessages", &Script_RemoveChatWindowMessages },
    { "AddChatWindowChannel", &Script_AddChatWindowChannel },
    { "RemoveChatWindowChannel", &Script_RemoveChatWindowChannel },
    { "SetChatWindowName", &Script_SetChatWindowName },
    { "SetChatWindowSize", &Script_SetChatWindowSize },
    { "SetChatWindowColor", &Script_SetChatWindowColor },
    { "SetChatWindowAlpha", &Script_SetChatWindowAlpha },
    { "SetChatWindowLocked", &Script_SetChatWindowLocked },
    { "SetChatWindowDocked", &Script_SetChatWindowDocked },
    { "SetChatWindowUninteractable", &Script_SetChatWindowUninteractable },
    { "SetChatWindowShown", &Script_SetChatWindowShown },
    { "SetChatWindowSavedPosition", &Script_SetChatWindowSavedPosition },
    { "SetChatWindowSavedDimensions", &Script_SetChatWindowSavedDimensions },
    { "EnumerateServerChannels", &Script_EnumerateServerChannels },
    { "RequestRaidInfo", &Script_RequestRaidInfo },
    { "GetNumSavedInstances", &Script_GetNumSavedInstances },
    { "GetSavedInstanceInfo", &Script_GetSavedInstanceInfo },
    { "SetSavedInstanceExtend", &Script_SetSavedInstanceExtend },
    { "ResetChatWindows", &Script_ResetChatWindows },
    { "CanComplainChat", &Script_CanComplainChat },
    { "ComplainChat", &Script_ComplainChat },
    { "GetNumDisplayChannels", &Script_GetNumDisplayChannels },
    { "GetChannelDisplayInfo", &Script_GetChannelDisplayInfo },
    { "GetSelectedDisplayChannel", &Script_GetSelectedDisplayChannel },
    { "SetSelectedDisplayChannel", &Script_SetSelectedDisplayChannel },
    { "GetChannelRosterInfo", &Script_GetChannelRosterInfo },
    { "GetNumChannelMembers", &Script_GetNumChannelMembers },
    { "SetActiveVoiceChannel", &Script_SetActiveVoiceChannel },
    { "GetActiveVoiceChannel", &Script_GetActiveVoiceChannel },
    { "CollapseChannelHeader", &Script_CollapseChannelHeader },
    { "ExpandChannelHeader", &Script_ExpandChannelHeader },
    { "ChannelVoiceOn", &Script_ChannelVoiceOn },
    { "ChannelVoiceOff", &Script_ChannelVoiceOff },
    { "DisplayChannelVoiceOn", &Script_DisplayChannelVoiceOn },
    { "DisplayChannelVoiceOff", &Script_DisplayChannelVoiceOff },
    { "IsDisplayChannelOwner", &Script_IsDisplayChannelOwner },
    { "IsDisplayChannelModerator", &Script_IsDisplayChannelModerator },
    { "IsVoiceChatEnabled", &Script_IsVoiceChatEnabled },
    { "IsVoiceChatAllowed", &Script_IsVoiceChatAllowed },
    { "IsVoiceChatAllowedByServer", &Script_IsVoiceChatAllowedByServer },
    { "IsSilenced", &Script_IsSilenced },
    { "GetMuteStatus", &Script_GetMuteStatus },
    { "UnitIsSilenced", &Script_UnitIsSilenced },
    { "SetChannelWatch", &Script_SetChannelWatch },
    { "ClearChannelWatch", &Script_ClearChannelWatch },
    { "DeclineInvite", &Script_DeclineInvite },
    { "SetChatColorNameByClass", &Script_SetChatColorNameByClass },
};

// VERIFY: 19fef056 auto
// OFFSET: 0x004FCEF0
void ChatRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: bbd469da auto
// OFFSET: 0x004FCF20
void ChatUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
