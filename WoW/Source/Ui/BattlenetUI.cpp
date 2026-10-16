#include "BattlenetUI.h"
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Battlenet/Tumor/TumorManager.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowServices/BattlenetLogin.h"
#include "WoW/Source/WowServices/Login.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00ACC8B0
BattlenetUI::EventCallback BattlenetUI::s_eventCallbacks[26] = {
    { "BattlenetEvent_NewPresence", BattlenetUI::NewPresenceCB, 0 },
    { "BattlenetEvent_ToonOnline", BattlenetUI::SelfToonOnlineCB, 0 },
    { "BattlenetEvent_ToonOffine", BattlenetUI::SelfToonOfflineCB, 0 },
    { "BattlenetEvent_FriendListInitialized", BattlenetUI::FriendListInitializedCB, 0 },
    { "BattlenetEvent_FriendAdded", BattlenetUI::FriendAddedCB, 0 },
    { "BattlenetEvent_FriendRemoved", BattlenetUI::FriendRemovedCB, 0 },
    { "BattlenetEvent_InviteListInitialized", BattlenetUI::FriendInviteListInitializedCB, 0 },
    { "BattlenetEvent_InviteAdded", BattlenetUI::FriendInviteAddedCB, 0 },
    { "BattlenetEvent_InviteRemoved", BattlenetUI::FriendInviteRemovedCB, 0 },
    { "BattlenetEvent_ChatWhisperSent", BattlenetUI::ChatWhisperSentCB, 0 },
    { "BattlenetEvent_ChatWhisperReceived", BattlenetUI::ChatWhisperReceivedCB, 0 },
    { "BattlenetEvent_ChatWhisperUndeliverable", BattlenetUI::ChatWhisperUndeliverableCB, 0 },
    { "BattlenetEvent_ChannelJoined", BattlenetUI::ChatChannelJoinedCB, 0 },
    { "BattlenetEvent_ChannelLeft", BattlenetUI::ChatChannelLeftCB, 0 },
    { "BattlenetEvent_ChannelClosed", BattlenetUI::ChatChannelClosedCB, 0 },
    { "BattlenetEvent_ChatMessage", BattlenetUI::ChatChannelMessageReceivedCB, 0 },
    { "BattlenetEvent_ChatMessageUndeliverable", BattlenetUI::ChatChannelMessageUndeliverableCB, 0 },
    { "BattlenetEvent_ChatMessageBlocked", BattlenetUI::ChatChannelMessageBlockedCB, 0 },
    { "BattlenetEvent_ChannelMemberJoined", BattlenetUI::ChatChannelMemberJoinedCB, 0 },
    { "BattlenetEvent_ChannelMemberLeft", BattlenetUI::ChatChannelMemberLeftCB, 0 },
    { "BattlenetEvent_ChannelMemberUpdated", BattlenetUI::ChatChannelMemberUpdatedCB, 0 },
    { "BattlenetEvent_BlockListInitialized", BattlenetUI::BlockListInitializedCB, 0 },
    { "BattlenetEvent_BlockAdded", BattlenetUI::BlockAddedCB, 0 },
    { "BattlenetEvent_BlockRemoved", BattlenetUI::BlockRemovedCB, 0 },
    { "BattlenetEvent_SystemMessage", BattlenetUI::SystemMessageCB, 0 },
    { "BattlenetEvent_SettingChanged", BattlenetUI::SettingChangedCB, 0 },
};

// STATIC: 0x00ACC9E8
BattlenetUI::PresenceCallback BattlenetUI::s_presenceCallbacks[NUM_BN_PRESENCE_FIELDS] = {
    { 0x10002, BattlenetUI::OnlinePresenceChangedCB, 0 },
    { 0x10003, BattlenetUI::OnlineTimeChangedCB, 0 },
    { 0x10007, BattlenetUI::LastOnlineChangedCB, 0 },
    { 0x10010, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x10011, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x1000C, BattlenetUI::CustomMessageChangedCB, 0 },
    { 0x1000D, BattlenetUI::IgnoredFieldCB, 0 },
    { 0x1000E, BattlenetUI::ToonNameUpdateCB, 0 },
    { 0x10005, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x10004, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x10009, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x10008, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30001, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30002, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30003, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30004, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30005, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x30006, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x1000A, BattlenetUI::FriendInfoChangedCB, 0 },
    { 0x1000B, BattlenetUI::IgnoredFieldCB, 0 },
    { 0x40001, BattlenetUI::FactionUpdateCB, 0 },
};

// STATIC: 0x00ACCCB0
BattlenetUI::FriendInviteSentCB BattlenetUI::s_friendInviteSentCB;

// STATIC: 0x00ACCCB8
int32_t BattlenetUI::s_broadcastCount = 2;

// STATIC: 0x00BD1B00
uint32_t BattlenetUI::s_selfRealm;

// STATIC: 0x00BD1B04
bool BattlenetUI::s_initialized;

// STATIC: 0x00BD1B05
bool BattlenetUI::m_friendListLoaded;

// STATIC: 0x00BD1B08
BSN::BSNEncodedObject<Battlenet::Friends::FriendInviteContainer, Protocol>* BattlenetUI::m_friendInvites;

// STATIC: 0x00BD1B0C
BSN::BSNEncodedObject<Battlenet::Friends::FriendsOfFriendList, Protocol>* BattlenetUI::s_friendsOfFriend;

// STATIC: 0x00BD1B10
GeneralFunction::Callback* BattlenetUI::s_createConversationCB;

// STATIC: 0x00BD1B14
GeneralFunction::Callback* BattlenetUI::s_inviteToConversationCB;

// STATIC: 0x00BD1B18
GeneralFunction::Callback* BattlenetUI::s_friendsOfFriendCB;

// STATIC: 0x00BD1B1C
uint32_t BattlenetUI::s_selectedFriend;

// STATIC: 0x00BD1B20
uint32_t BattlenetUI::s_selectedBlock;

// STATIC: 0x00BD1B24
uint32_t BattlenetUI::s_selectedToonBlock;

// STATIC: 0x00BD1B28
BSN::Array<BattlenetUIFriend, 100> BattlenetUI::m_friends;

// STATIC: 0x00BE5D0C
char BattlenetUI::s_programString[16];

// STATIC: 0x00BE5D1C
float BattlenetUI::s_broadcastTime;

// STATIC: 0x00BE5D20
char BattlenetUI::s_displayName[72];

// VERIFY: d9b6deb2 manual
// OFFSET: 0x00530D00
char* FilterChatLinks(const char* source, char* dest, uint32_t size) {
    if (!source || !dest || !size) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char* out = dest;
    const char* end = &source[SStrLen(source)];

    if (source < end) {
        do {
            if (out >= &dest[size - 1]) {
                break;
            }

            if (*source == '|') {
                switch (source[1]) {
                    case 'T':
                    case 'r':
                    case 't':
                        source += 2;
                        break;

                    case 'c':
                        source += 10;
                        break;

                    default:
                        break;
                }
            }

            if (source >= end) {
                break;
            }

            *out++ = *source++;
        } while (source < end);
    }

    *out = '\0';
    return dest;
}

// VERIFY: af1ed3d3 auto
// OFFSET: 0x00530DC0
static int32_t CountFormatStrings(const char* format) {
    if (!format) {
        return 0;
    }

    int32_t length = SStrLen(format);
    bool inFormat = false;
    int32_t count = 0;

    for (int32_t i = 0; i < length; i++) {
        switch (format[i]) {
            case ' ':
            case '#':
            case '$':
            case '+':
            case '-':
            case '.':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;

            case '%':
                inFormat = !inFormat;
                break;

            case 'S':
            case 's':
                if (inFormat) {
                    count++;
                    inFormat = false;
                }

                break;

            default:
                if (inFormat) {
                    return -1;
                }

                break;
        }
    }

    return count;
}

// VERIFY: 3d11db98 auto
// OFFSET: 0x00530E90
int32_t BattlenetUI::QSortFriendInviteListByTimeCB(const void* a, const void* b) {
    int32_t time1 = static_cast<const Battlenet::Friends::FriendInvite*>(a)->m_time;
    int32_t time2 = static_cast<const Battlenet::Friends::FriendInvite*>(b)->m_time;

    if (time1 > time2) {
        return -1;
    }

    return time1 < time2;
}

// VERIFY: 5e47b873 auto
// OFFSET: 0x005326D0
void BattlenetUI::SendScriptEvent(uint32_t event, const char* format, ...) {
    va_list args;
    va_start(args, format);

    if (CGGameUI::m_initialized) {
        vFrameScript_SignalEvent(event, format, args);
    }
    va_end(args);
}

// VERIFY: 0ddbd7df auto
// OFFSET: 0x00532700
int32_t BattlenetUI::FindFriendIndex(uint32_t presenceId) {
    for (uint32_t i = 0; i < BattlenetUI::m_friends.m_count; i++) {
        if (BattlenetUI::m_friends[i].m_presenceID == presenceId) {
            return i;
        }
    }

    return -1;
}

// VERIFY: f5ec582e auto
// OFFSET: 0x00532750
BattlenetUIFriend* BattlenetUI::FindFriend(uint32_t presenceId) {
    for (uint32_t i = 0; i < BattlenetUI::m_friends.m_count; i++) {
        BattlenetUIFriend& entry = BattlenetUI::m_friends[i];

        if (entry.m_presenceID == presenceId) {
            return &entry;
        }
    }

    return nullptr;
}

// VERIFY: 9eaa7f3c auto
// OFFSET: 0x00532800
int32_t BattlenetUI::QSortDisambiguateFriendListCB(const void* a, const void* b) {
    const BattlenetUIFriend* friend1 = static_cast<const BattlenetUIFriend*>(a);
    const BattlenetUIFriend* friend2 = static_cast<const BattlenetUIFriend*>(b);

    int32_t result = SStrCmpUTF8I(friend1->m_givenName.m_data, friend2->m_givenName.m_data, 40);

    if (!result) {
        result = SStrCmpUTF8I(friend1->m_sortSurname.m_data, friend2->m_sortSurname.m_data, 40);

        if (!result) {
            return friend1->m_program - friend2->m_program;
        }
    }

    return result;
}

// VERIFY: 98854ef8 auto
// OFFSET: 0x00532850
void BattlenetUI::SortFriendInviteList() {
    if (!BattlenetUI::m_friendInvites) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::Array<Battlenet::Friends::FriendInvite, 100>& invites = BattlenetUI::m_friendInvites->m_data.m_invites;

    if (invites.m_count > 1) {
        qsort(invites.m_data, invites.m_count, sizeof(Battlenet::Friends::FriendInvite), BattlenetUI::QSortFriendInviteListByTimeCB);
    }
}

// VERIFY: 96eab942 auto
// OFFSET: 0x00532890
void BattlenetUI_LeaveRealm() {
    BattlenetUI::m_friendListLoaded = false;
    BattlenetUI::m_friends.m_count = 0;
    BattlenetUI::s_selfRealm = 0;
}

// VERIFY: 1faee593 manual
// OFFSET: 0x005328B0
void BattlenetUI::SetDisconnectedFromBattlenet(bool disconnected) {
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_DISCONNECTED, "%b", disconnected);

    for (AUTOCOMPLETE_NAMENODE* node = CGAutoCompleteName::m_list.Head(); node;) {
        AUTOCOMPLETE_NAMENODE* next = CGAutoCompleteName::m_list.Next(node);

        if (node->m_presenceId) {
            CGAutoCompleteName::m_list.DeleteNode(node);
        }

        node = next;
    }
}

// VERIFY: 9dde2faf auto
// OFFSET: 0x005328D0
void BattlenetUI::SetBattlenetConnectionRecovered() {
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CONNECTED, nullptr);
}

// VERIFY: ad919403 manual
// OFFSET: 0x00533D40
bool BattlenetUI::AddToFriendList(uint32_t presenceId) {
    if (BattlenetUI::FindFriendIndex(presenceId) >= 0) {
        return true;
    }

    BattlenetUIFriend* entry = BattlenetUI::m_friends.New();
    entry->m_presenceID = presenceId;

    Battlenet::Presence::FieldVal accountName = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ACCOUNT_NAME].m_field);

    if (accountName.m_tag == 18) {
        SStrPrintf(entry->m_name.m_data, sizeof(entry->m_name.m_data), FrameScript_GetText("BATTLENET_NAME_FORMAT", -1, GENDER_NOT_APPLICABLE), entry->m_givenName.m_data, entry->m_surname.m_data);
        entry->m_givenName = accountName.m_accountName.m_givenName.m_data;
        entry->m_surname = accountName.m_accountName.m_surname.m_data;
        entry->m_sortSurname = accountName.m_accountName.m_surname.m_data;
    } else {
        entry->m_name = "";
        entry->m_givenName = "";
        entry->m_surname = "";
        entry->m_sortSurname = "";
    }

    Battlenet::Presence::FieldVal program = BattlenetAPI::GetPresenceValue(entry->m_presenceID, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_PROGRAM].m_field);
    entry->m_program = program.m_tag != 5 ? 0 : program.m_fourCC.m_value;

    return true;
}

// VERIFY: 1797aa8b auto
// OFFSET: 0x00533F10
int32_t BattlenetUI::UpdateAutoComplete(uint32_t& presenceId, uint32_t flags, int32_t add, const char* name) {
    if (BattlenetAPI::IsSelf(presenceId)) {
        return CGAutoCompleteName::RemoveFlags(presenceId, 0x7FFFFFFF);
    }

    const char* displayName = name;

    if (BattlenetAPI::HasRIDFriendship(presenceId) && !BattlenetAPI::IsRIDFriend(presenceId)) {
        presenceId = BattlenetAPI::GetAccountPresenceId(presenceId);
        BattlenetUIFriend* entry = BattlenetUI::FindFriend(presenceId);

        if (entry) {
            displayName = entry->m_name.m_data;
        }
    }

    int32_t result = CGAutoCompleteName::AddFlags(presenceId, flags, add, displayName);

    if (displayName && *displayName) {
        return CGAutoCompleteName::SetName(presenceId, displayName, true);
    }

    return result;
}

// VERIFY: 4e033cce auto
// OFFSET: 0x00533FA0
uint32_t BattlenetUI::GetSelfRealmID() {
    if (!BattlenetUI::s_selfRealm) {
        Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(BattlenetAPI::GetPresenceIDForCurrentToon(), BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);

        if (toon.m_tag == 17) {
            BattlenetUI::s_selfRealm = toon.m_toonName.m_realm;
            return BattlenetUI::s_selfRealm;
        }
    }

    return BattlenetUI::s_selfRealm;
}

// VERIFY: 4a0688b2 manual
// OFFSET: 0x00533FF0
int32_t BattlenetUI::QSortFriendListCB(const void* a, const void* b) {
    const BattlenetUIFriend* friend1 = static_cast<const BattlenetUIFriend*>(a);
    const BattlenetUIFriend* friend2 = static_cast<const BattlenetUIFriend*>(b);

    Battlenet::Presence::FieldVal online1 = BattlenetAPI::GetPresenceValue(friend1->m_presenceID, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE].m_field);
    Battlenet::Presence::FieldVal online2 = BattlenetAPI::GetPresenceValue(friend2->m_presenceID, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE].m_field);

    bool isOnline1 = false;
    bool isOnline2 = false;

    if (online1.m_tag == 1) {
        isOnline1 = online1.m_bool != 0;
    }

    if (online2.m_tag == 1) {
        isOnline2 = online2.m_bool != 0;
    }

    if (isOnline1) {
        if (!isOnline2) {
            return -1;
        }
    } else if (isOnline2) {
        return 1;
    }

    return SStrCmpUTF8I(friend1->m_name.m_data, friend2->m_name.m_data, 72);
}

// VERIFY: bea02848 manual
// OFFSET: 0x00534090
void BattlenetUI::DisambiguateFriendList() {
    if (BattlenetUI::m_friends.m_count > 1) {
        qsort(BattlenetUI::m_friends.m_data, BattlenetUI::m_friends.m_count, sizeof(BattlenetUIFriend), BattlenetUI::QSortDisambiguateFriendListCB);
    }

    uint32_t duplicates = 0;

    for (uint32_t i = 0; i < BattlenetUI::m_friends.m_count; i++) {
        if (i != BattlenetUI::m_friends.m_count - 1
            && !SStrCmpUTF8I(BattlenetUI::m_friends[i].m_givenName.m_data, BattlenetUI::m_friends[i + 1].m_givenName.m_data, 40)
            && !SStrCmpUTF8I(BattlenetUI::m_friends[i].m_sortSurname.m_data, BattlenetUI::m_friends[i + 1].m_sortSurname.m_data, 40)) {
            if (++duplicates == 1) {
                BattlenetUI::m_friends[i].m_surname = BattlenetUI::m_friends[i].m_sortSurname;
                BattlenetUI::m_friends[i].m_surname += " 1";
            }

            char suffix[16];
            SStrPrintf(suffix, sizeof(suffix), " %d", duplicates + 1);
            BattlenetUI::m_friends[i + 1].m_surname = BattlenetUI::m_friends[i + 1].m_sortSurname;
            BattlenetUI::m_friends[i + 1].m_surname += suffix;
        } else {
            if (!duplicates) {
                BattlenetUI::m_friends[i].m_surname = BattlenetUI::m_friends[i].m_sortSurname;
            }

            duplicates = 0;
        }

        BattlenetUIFriend& entry = BattlenetUI::m_friends[i];
        SStrPrintf(entry.m_name.m_data, sizeof(entry.m_name.m_data), FrameScript_GetText("BATTLENET_NAME_FORMAT", -1, GENDER_NOT_APPLICABLE), entry.m_givenName.m_data, entry.m_surname.m_data);
    }
}

// VERIFY: f8478297 auto
// OFFSET: 0x005342D0
bool BattlenetUI_IsAvailable() {
    Login* login = ClientServices::LoginConnection();
    return login && login->GetLoginServerType() == 1 && !login->IsTrialAccount() && BattlenetAPI::IsRIDEnabled() && static_cast<BattlenetLogin*>(login)->IsConnected();
}

// VERIFY: 9732c054 manual
// OFFSET: 0x00534320
void BattlenetUI::Uninit() {
    BattlenetUI::s_initialized = false;

    for (uint32_t i = 0; i < 26; i++) {
        BattlenetAPI::UnregisterEvent(BattlenetUI::s_eventCallbacks[i].m_handle);
        BattlenetUI::s_eventCallbacks[i].m_handle = 0;
    }

    for (uint32_t i = 0; i < NUM_BN_PRESENCE_FIELDS; i++) {
        BattlenetAPI::UnregisterPresenceUpdateCallback(BattlenetUI::s_presenceCallbacks[i].m_handle);
        BattlenetUI::s_presenceCallbacks[i].m_handle = 0;
    }

    if (BattlenetUI::s_inviteToConversationCB) {
        BattlenetUI::s_inviteToConversationCB->DecRef();
        BattlenetUI::s_inviteToConversationCB = nullptr;
    }

    if (BattlenetUI::s_createConversationCB) {
        BattlenetUI::s_createConversationCB->DecRef();
        BattlenetUI::s_createConversationCB = nullptr;
    }

    if (BattlenetUI::s_friendsOfFriendCB) {
        BattlenetUI::s_friendsOfFriendCB->DecRef();
        BattlenetUI::s_friendsOfFriendCB = nullptr;
    }

    if (BattlenetUI::m_friendInvites) {
        BattlenetUI::m_friendInvites->m_data.m_invites.m_count = 0;
        delete BattlenetUI::m_friendInvites;
        BattlenetUI::m_friendInvites = nullptr;
    }

    if (BattlenetUI::s_friendsOfFriend) {
        BattlenetUI::s_friendsOfFriend->m_data.m_friends.m_count = 0;
        delete BattlenetUI::s_friendsOfFriend;
        BattlenetUI::s_friendsOfFriend = nullptr;
    }
}

// VERIFY: 64e9664d manual
// OFFSET: 0x005346F0
int32_t BattlenetUI::BNGetFriendInfoHelper(lua_State* L, uint32_t index) {
    if (index >= BattlenetUI::m_friends.m_count) {
        return 0;
    }

    uint32_t presenceId = BattlenetUI::m_friends.m_data[index].m_presenceID;
    lua_pushnumber(L, presenceId);

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);
    Battlenet::Presence::FieldVal online = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE].m_field);
    Battlenet::Presence::FieldVal lastOnline = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_LAST_ONLINE].m_field);
    Battlenet::Presence::FieldVal afk = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_AFK].m_field);
    Battlenet::Presence::FieldVal dnd = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_DND].m_field);
    Battlenet::Presence::FieldVal broadcast = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_BROADCAST].m_field);
    Battlenet::Presence::FieldVal note = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_NOTE].m_field);

    lua_pushstring(L, BattlenetUI::m_friends[index].m_givenName.m_data);
    lua_pushstring(L, BattlenetUI::m_friends[index].m_surname.m_data);

    if (toon.m_tag == 17) {
        uint32_t toonId = BattlenetAPI::GetPresenceIDForToon(&toon.m_toonName);
        const char* toonName = BattlenetAPI::GetToonNameForPresenceId(presenceId, false);

        if (!toonName) {
            toonName = toon.Get<17, Battlenet::Toon::FullName>().m_name.m_data;
        }

        lua_pushstring(L, toonName);
        lua_pushnumber(L, toonId);

        BSN::FourCC program = toon.Get<17, Battlenet::Toon::FullName>().m_program;
        program.ToString(BattlenetUI::s_programString, sizeof(BattlenetUI::s_programString));
        lua_pushstring(L, BattlenetUI::s_programString);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
    }

    bool isOnline = false;

    if (online.m_tag == 1) {
        isOnline = online.m_bool != 0;
    }

    lua_pushboolean(L, isOnline);

    uint32_t lastOnlineTime = 0;

    if (lastOnline.m_tag == 6) {
        lastOnlineTime = lastOnline.m_int;
    }

    lua_pushnumber(L, lastOnlineTime);

    uint8_t isAFK = 0;

    if (afk.m_tag == 11) {
        isAFK = afk.m_bool;
    }

    lua_pushboolean(L, isAFK);

    uint8_t isDND = 0;

    if (dnd.m_tag == 11) {
        isDND = dnd.m_bool;
    }

    lua_pushboolean(L, isDND);

    if (broadcast.m_tag == 13) {
        lua_pushstring(L, broadcast.m_string.m_data);
    } else {
        lua_pushnil(L);
    }

    if (note.m_tag == 13) {
        lua_pushstring(L, note.m_string.m_data);
    } else {
        lua_pushnil(L);
    }

    lua_pushboolean(L, BattlenetAPI::IsRIDFriend(presenceId));

    afk = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_BROADCAST_TIME].m_field);
    lua_pushnumber(L, afk.m_tag == 6 ? afk.m_int : 0.0);

    return 14;
}

// VERIFY: 51d722d8 manual
// OFFSET: 0x00534B60
int32_t BattlenetUI::BNGetToonInfoHelper(lua_State* L, uint32_t presenceId) {
    lua_pushboolean(L, BattlenetAPI::GetFocusedToon(BattlenetAPI::GetAccountPresenceId(presenceId)) == presenceId);

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);

    if (toon.m_tag == 17) {
        const char* toonName = BattlenetAPI::GetToonNameForPresenceId(presenceId, false);

        if (!toonName) {
            toonName = toon.Get<17, Battlenet::Toon::FullName>().m_name.m_data;
        }

        lua_pushstring(L, toonName);

        BSN::FourCC program = toon.Get<17, Battlenet::Toon::FullName>().m_program;
        program.ToString(BattlenetUI::s_programString, sizeof(BattlenetUI::s_programString));
        lua_pushstring(L, BattlenetUI::s_programString);
    } else {
        lua_pushstring(L, "");
        lua_pushstring(L, "");
    }

    Battlenet::Presence::FieldVal realm = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_REALM_NAME].m_field);
    lua_pushstring(L, realm.m_tag == 13 ? realm.m_string.m_data : "");

    Battlenet::Presence::FieldVal value = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_FACTION].m_field);
    lua_pushnumber(L, value.m_tag == 1 ? value.m_bool : -1.0);

    Battlenet::Presence::FieldVal race = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_RACE].m_field);
    lua_pushstring(L, race.m_tag == 13 ? race.m_string.m_data : "");

    Battlenet::Presence::FieldVal classValue = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_CLASS].m_field);
    lua_pushstring(L, classValue.m_tag == 13 ? classValue.m_string.m_data : "");

    Battlenet::Presence::FieldVal guild = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_GUILD].m_field);
    lua_pushstring(L, guild.m_tag == 13 ? guild.m_string.m_data : "");

    Battlenet::Presence::FieldVal area = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_AREA].m_field);
    lua_pushstring(L, area.m_tag == 13 ? area.m_string.m_data : "");

    value = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_LEVEL].m_field);

    if (value.m_tag == 1) {
        char level[8];
        SStrPrintf(level, sizeof(level), "%u", value.m_bool);
        lua_pushstring(L, level);
    } else {
        lua_pushstring(L, "");
    }

    Battlenet::Presence::FieldVal gameText = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_GAME_TEXT].m_field);
    lua_pushstring(L, gameText.m_tag == 13 ? gameText.m_string.m_data : "");

    Battlenet::Presence::FieldVal broadcast = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_BROADCAST].m_field);
    lua_pushstring(L, broadcast.m_tag == 13 ? broadcast.m_string.m_data : "");

    value = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_BROADCAST_TIME].m_field);
    lua_pushnumber(L, value.m_tag == 6 ? value.m_int : 0.0);

    value = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE].m_field);

    bool isOnline = false;

    if (value.m_tag == 1) {
        isOnline = value.m_bool != 0;
    }

    lua_pushboolean(L, isOnline);

    return 14;
}

// VERIFY: eb18a89b auto
// OFFSET: 0x00537B90
void BattlenetUI::SelfToonOnlineCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_SELF_ONLINE, "%u", presenceId);
    DebugLog("BNUI Self Toon Online, ID %u", presenceId);
}

// VERIFY: 8aafec95 auto
// OFFSET: 0x00537C00
void BattlenetUI::SelfToonOfflineCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_SELF_OFFLINE, nullptr);
    DebugLog("BNUI: Self Toon Offline");
}

// VERIFY: 03c80fe7 auto
// OFFSET: 0x00537C60
const char* BattlenetUI::GetNameForPresenceIdHelper(uint32_t presenceId) {
    uint32_t accountId = presenceId;

    if (BattlenetAPI::HasRIDFriendship(presenceId)) {
        if (!BattlenetAPI::IsRIDFriend(presenceId)) {
            accountId = BattlenetAPI::GetAccountPresenceId(presenceId);
        }

        BattlenetUIFriend* entry = BattlenetUI::FindFriend(accountId);

        if (entry) {
            return entry->m_name.m_data;
        }

        return BattlenetAPI::GetNameForPresenceId(accountId, false);
    }

    if (BattlenetAPI::IsSelf(presenceId)) {
        return BattlenetAPI::GetNameForPresenceId(presenceId, false);
    }

    uint32_t realm = BattlenetUI::GetSelfRealmID();

    if (!realm) {
        return BattlenetAPI::GetNameForPresenceId(presenceId, false);
    }

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);

    if (toon.m_tag != 17 || (realm == toon.m_toonName.m_realm && toon.m_toonName.m_program.m_value == Battlenet::Program::WOW.m_value)) {
        return BattlenetAPI::GetNameForPresenceId(presenceId, false);
    }

    SStrPrintf(BattlenetUI::s_displayName, sizeof(BattlenetUI::s_displayName), "%s*", BattlenetAPI::GetNameForPresenceId(presenceId, false));
    return BattlenetUI::s_displayName;
}

// VERIFY: e675c286 manual
// OFFSET: 0x00537D40
void BattlenetUI::FriendInviteSentCB::Call(int32_t argc, const GeneralFunction::Arg* args) const {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    UIErrors error = ERR_BN_FRIEND_REQUEST_SENT;
    uint16_t result = static_cast<uint16_t>(args[0].ToUInt64());

    if (result == 604) {
        error = ERR_BN_FRIEND_ALREADY;
    } else if (result == 617) {
        error = ERR_BN_FRIEND_SELF;
    } else if (result == 623) {
        error = ERR_BN_FRIEND_BLOCKED;
    }

    CGGameUI::DisplayError(error);

    DebugLog("BNUI: Friend Invite Send Result: %s", result ? "failure" : "success");
}

// VERIFY: 0926afe1 auto
// OFFSET: 0x005EEB70
void BattlenetUI::FriendInviteSentCB::Destroy() const {
}

// VERIFY: 4c0454a7 auto
// OFFSET: 0x005EEB70
void BattlenetUI::IgnoredFieldCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
}

// VERIFY: 8eee8765 auto
// OFFSET: 0x00537DF0
void BattlenetUI::FriendInviteListInitializedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized || !BattlenetUI::m_friendInvites) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::m_friendInvites->m_data.m_invites.m_count = 0;
    BattlenetUI::m_friendInvites->WalkAndGet(args[0].ToObject());
    BattlenetUI::SortFriendInviteList();
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INVITE_LIST_INITIALIZED, "%d", BattlenetUI::m_friendInvites->m_data.m_invites.m_count);

    if (s_cvShowToastFriendRequest && s_cvShowToastFriendRequest->m_intValue) {
        CGChat::AddChatMessage("FRIEND_PENDING", CHAT_MSG_BN_INLINE_TOAST_ALERT, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    DebugLog("BNUI: Friend Invite List Initialized");
}

// VERIFY: 1fc21d99 auto
// OFFSET: 0x00537F00
void BattlenetUI::FriendInviteAddedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized || !BattlenetUI::m_friendInvites) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());

    if (!presenceId) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::Friends::FriendInvite invite = BattlenetAPI::GetFriendInvite(presenceId);

    if (!invite.m_presenceId) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    *BattlenetUI::m_friendInvites->m_data.m_invites.New() = invite;
    BattlenetUI::SortFriendInviteList();

    Battlenet::Presence::FieldVal onlineTime = BattlenetAPI::GetPresenceValue(BattlenetAPI::GetPresenceIDForCurrentToon(), BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE_TIME].m_field);

    if (onlineTime.m_tag != 6 || invite.m_time < onlineTime.Get<6, int32_t>()) {
        return;
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INVITE_ADDED, "%u", presenceId);

    if (s_cvShowToastFriendRequest && s_cvShowToastFriendRequest->m_intValue) {
        CGChat::AddChatMessage("FRIEND_REQUEST", CHAT_MSG_BN_INLINE_TOAST_ALERT, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    }

    DebugLog("BNUI: Friend Invite Added. ID %u", presenceId);
}

// VERIFY: 7069b0c1 manual
// OFFSET: 0x00538060
void BattlenetUI::FriendInviteRemovedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId;

    if (!BattlenetUI::s_initialized || !BattlenetUI::m_friendInvites || (presenceId = static_cast<uint32_t>(args[0].ToUInt64())) == 0) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::Array<Battlenet::Friends::FriendInvite, 100>& invites = BattlenetUI::m_friendInvites->m_data.m_invites;
    int32_t index = 0;

    for (; static_cast<uint32_t>(index) < invites.m_count; index++) {
        if (invites[index].m_presenceId == presenceId) {
            if (index >= 0) {
                invites.Remove(index);
                BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INVITE_REMOVED, "%d%u", index, presenceId);
            }

            break;
        }
    }

    DebugLog("BNUI: Friend Invite Removed. index %d, ID %u", index, presenceId);
}

// VERIFY: 6a08abf9 auto
// OFFSET: 0x00538160
void BattlenetUI::FriendInfoChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());
    const char* name = BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId);

    if (update.m_data.m_field == BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ACCOUNT_NAME].m_field) {
        if (!BattlenetAPI::HasRIDFriendship(update.m_data.m_presenceId) || BattlenetAPI::IsRIDFriend(update.m_data.m_presenceId)) {
            CGAutoCompleteName::SetName(update.m_data.m_presenceId, name, true);
        } else {
            CGAutoCompleteName::RemoveFlags(update.m_data.m_presenceId, 0x7FFFFFFF);
        }
    }

    int32_t index = BattlenetUI::FindFriendIndex(update.m_data.m_presenceId);

    if (index >= 0) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INFO_CHANGED, "%d", index + 1);
    }
}

// VERIFY: 3e8839d1 auto
// OFFSET: 0x00538290
void BattlenetUI::FactionUpdateCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());

    if (!BattlenetAPI::IsSelf(update.m_data.m_presenceId)) {
        BattlenetUI::UpdateAutoComplete(update.m_data.m_presenceId, 0x20, 0, nullptr);
    }
}

// VERIFY: 63f6b0df auto
// OFFSET: 0x00538320
void BattlenetUI::NewPresenceCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());

    if (!BattlenetUI::m_friendListLoaded) {
        DebugLog("BNUI: New Presence ID %u before Friend List initialized.", presenceId);
        return;
    }

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);

    if (toon.m_tag == 17) {
        if (BattlenetAPI::HasRIDFriendship(presenceId)) {
            BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_TOON_ONLINE, "%u", presenceId);
            DebugLog("BNUI: >> TOON %s is now ONLINE. ID %u (new presence)", BattlenetUI::GetNameForPresenceIdHelper(presenceId), presenceId);
        } else {
            BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_NEW_PRESENCE, "%u%s", presenceId, toon.m_toonName.m_name.m_data);
            DebugLog("BNUI: New Presence ID %u, name %s", presenceId, toon.m_toonName.m_name.m_data);
        }
    } else {
        const char* name = BattlenetUI::GetNameForPresenceIdHelper(presenceId);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_NEW_PRESENCE, "%u%s", presenceId, name);
        DebugLog("BNUI: New Non-Toon Presence ID %u, name %s", presenceId, name);
    }
}

// VERIFY: 9bb49d4e auto
// OFFSET: 0x00538460
void BattlenetUI::ToonNameUpdateCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());
    bool friendship = BattlenetAPI::HasRIDFriendship(update.m_data.m_presenceId);

    if (update.m_data.m_value.m_tag == 17) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_TOON_NAME_UPDATED, "%u%s%b", update.m_data.m_presenceId, update.m_data.m_value.m_toonName.m_name.m_data, friendship);
        DebugLog("BNUI: Toon Name Updated, %s friend. ID %u, name %s", friendship ? "" : "non", update.m_data.m_presenceId, update.m_data.m_value.m_toonName.m_name.m_data);
    }

    if (!BattlenetAPI::IsSelf(update.m_data.m_presenceId)) {
        BattlenetUI::UpdateAutoComplete(update.m_data.m_presenceId, 0x20, 0, nullptr);
    }
}

// VERIFY: bf4a6401 auto
// OFFSET: 0x00538580
void BattlenetUI::OnlineTimeChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());

    if (BattlenetAPI::IsSelf(update.m_data.m_presenceId) || update.m_data.m_value.m_tag != 6) {
        return;
    }

    int32_t onlineTime = update.m_data.m_value.m_int;
    bool ridFriend = BattlenetAPI::IsRIDFriend(update.m_data.m_presenceId);
    bool cidFriend = BattlenetAPI::IsCIDFriend(update.m_data.m_presenceId);

    if (!ridFriend && !cidFriend) {
        return;
    }

    Battlenet::Presence::FieldVal selfTime = BattlenetAPI::GetPresenceValue(BattlenetAPI::GetPresenceIDForCurrentToon(), BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE_TIME].m_field);

    if (selfTime.m_tag != 6) {
        return;
    }

    int32_t selfOnlineTime = selfTime.Get<6, int32_t>();

    if (selfOnlineTime && onlineTime && onlineTime > selfOnlineTime) {
        if (ridFriend) {
            BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_ACCOUNT_ONLINE, "%u", update.m_data.m_presenceId);

            if (s_cvShowToastOnline && s_cvShowToastOnline->m_intValue) {
                ChatConversation conversation;
                conversation.m_presenceID = update.m_data.m_presenceId;
                conversation.m_inactive = 1;
                CGChat::AddChatMessage("FRIEND_ONLINE", CHAT_MSG_BN_INLINE_TOAST_ALERT, BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
            }

            DebugLog("BNUI: >> ACCOUNT %s Online Time changed. ID %u", BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), update.m_data.m_presenceId);
        } else if (cidFriend) {
            BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_TOON_ONLINE, "%u", update.m_data.m_presenceId);
            DebugLog("BNUI: >> TOON %s is now ONLINE. ID %u", BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), update.m_data.m_presenceId);
        }
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INFO_CHANGED, nullptr);
}

// VERIFY: a04df2e0 auto
// OFFSET: 0x00538790
void BattlenetUI::LastOnlineChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());

    if (update.m_data.m_value.m_tag == 6) {
        DebugLog("BNUI: Last Online for ID %u is now %d", update.m_data.m_presenceId, update.m_data.m_value.m_int);
    } else {
        DebugLog("BNUI: Last Online for ID %u changed", update.m_data.m_presenceId);
    }
}

// VERIFY: 6cd97fa7 manual
// OFFSET: 0x00538850
void BattlenetUI::CustomMessageChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());
    uint32_t toonId = BattlenetAPI::GetPresenceIDForCurrentToon();

    Battlenet::Presence::FieldVal value = BattlenetAPI::GetPresenceValue(toonId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE_TIME].m_field);
    int32_t onlineTime = 0;

    if (value.m_tag == 6) {
        onlineTime = value.m_int;
    }

    const char* message = update.m_data.m_value.m_tag == 13 ? update.m_data.m_value.m_string.m_data : "";
    int32_t broadcastTime = 0;

    value = BattlenetAPI::GetPresenceValue(update.m_data.m_presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_BROADCAST_TIME].m_field);

    if (value.m_tag == 6) {
        broadcastTime = value.m_int;
    }

    char text[512];

    if (toonId != update.m_data.m_presenceId) {
        if (BattlenetAPI::IsRIDFriend(update.m_data.m_presenceId)) {
            if (!onlineTime || broadcastTime < onlineTime) {
                return;
            }

            Battlenet::Presence::FieldVal friendTime = BattlenetAPI::GetPresenceValue(update.m_data.m_presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE_TIME].m_field);

            if (friendTime.m_tag == 6 && broadcastTime < friendTime.Get<6, int32_t>()) {
                return;
            }

            BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CUSTOM_MESSAGE_CHANGED, "%u", update.m_data.m_presenceId);

            if (s_cvShowToastBroadcast && s_cvShowToastBroadcast->m_intValue) {
                FilterChatLinks(message, text, 509);

                ChatConversation conversation;
                conversation.m_presenceID = update.m_data.m_presenceId;
                conversation.m_inactive = 1;
                CGChat::AddChatMessage(text, CHAT_MSG_BN_INLINE_TOAST_BROADCAST, BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
            }
        }

        DebugLog("BNUI: Custom Message Changed");
        return;
    }

    if (onlineTime && broadcastTime >= onlineTime) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CUSTOM_MESSAGE_CHANGED, nullptr);
        FilterChatLinks(message, text, 509);
        CGChat::AddChatMessage(text, CHAT_MSG_BN_INLINE_TOAST_BROADCAST_INFORM, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CUSTOM_MESSAGE_LOADED, nullptr);
    }
}

// VERIFY: 4b72e85c manual
// OFFSET: 0x00538AB0
void BattlenetUI::ChatWhisperSentCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    const char* message = args[1].ToString();

    if (!message) {
        message = "";
    }

    BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);
    const char* flags = nullptr;
    Battlenet::Presence::FieldVal afk = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_AFK].m_field);
    Battlenet::Presence::FieldVal dnd = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_DND].m_field);

    if (afk.m_tag == 11 && afk.m_bool) {
        flags = "AFK";
    } else if (dnd.m_tag == 11 && dnd.m_bool) {
        flags = "DND";
    }

    char text[1024];
    FilterChatLinks(message, text, 1021);

    ChatConversation conversation;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 1;
    CGChat::AddChatMessage(text, CHAT_MSG_BN_WHISPER_INFORM, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, flags, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);

    DebugLog("BNUI: Whisper Sent to %s, ID %u. %s", toon.m_tag == 17 ? toon.m_toonName.m_name.m_data : "unknown", presenceId, message);
}

// VERIFY: 869184a5 manual
// OFFSET: 0x00538C40
void BattlenetUI::ChatWhisperReceivedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    const char* message = args[1].ToString();

    if (!message) {
        message = "";
    }

    BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));

    Battlenet::Presence::FieldVal toon = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_TOON_NAME].m_field);
    const char* flags = nullptr;
    Battlenet::Presence::FieldVal afk = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_AFK].m_field);
    Battlenet::Presence::FieldVal dnd = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_DND].m_field);

    if (afk.m_tag == 11 && afk.m_bool) {
        flags = "AFK";
    } else if (dnd.m_tag == 11 && dnd.m_bool) {
        flags = "DND";
    }

    char text[1024];
    FilterChatLinks(message, text, 1021);

    ChatConversation conversation;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 1;
    CGChat::AddChatMessage(text, CHAT_MSG_BN_WHISPER, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, flags, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);

    DebugLog("BNUI: Whisper Received From %s, ID %u. %s", toon.m_tag == 17 ? toon.m_toonName.m_name.m_data : "unknown", presenceId, message);
}

// VERIFY: 3c7d99a1 manual
// OFFSET: 0x00538DD0
void BattlenetUI::ChatChannelJoinedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    int32_t type = static_cast<int32_t>(args[1].ToInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    Battlenet::Chat::MemberList members = BattlenetAPI::GetChatMemberList(channel);
    uint32_t count = members.m_members.m_count;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t presenceId = members.m_members.m_data[i];
        BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));
    }

    if (type == 2) {
        ChatConversation conversation;
        conversation.m_presenceID = BattlenetAPI::GetPresenceIDForCurrentAccount();
        conversation.m_index = channel;
        conversation.m_inactive = 0;
        CGChat::AddChatMessage("YOU_JOINED_CONVERSATION", CHAT_MSG_BN_CONVERSATION_NOTICE, BattlenetUI::GetNameForPresenceIdHelper(BattlenetAPI::GetPresenceIDForCurrentAccount()), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_JOINED, "%d", channel + 1);

        if (s_cvShowToastConversation && s_cvShowToastConversation->m_intValue) {
            uint32_t accountId = BattlenetAPI::GetPresenceIDForCurrentAccount();
            uint32_t toonId = BattlenetAPI::GetPresenceIDForCurrentToon();
            const char* delimiter = FrameScript_GetText("PLAYER_LIST_DELIMITER", -1, GENDER_NOT_APPLICABLE);
            char text[3000];
            bool first = true;

            for (uint32_t i = 0; i < count; i++) {
                uint32_t presenceId = members.m_members.m_data[i];

                if (presenceId != accountId && presenceId != toonId) {
                    if (first) {
                        SStrCopyUTF8(text, BattlenetUI::GetNameForPresenceIdHelper(presenceId), sizeof(text), sizeof(text));
                        first = false;
                    } else {
                        SStrPrintf(text, sizeof(text), "%s%s%s", text, delimiter, BattlenetUI::GetNameForPresenceIdHelper(presenceId));
                    }
                }
            }

            CGChat::AddChatMessage(text, CHAT_MSG_BN_INLINE_TOAST_CONVERSATION, BattlenetUI::GetNameForPresenceIdHelper(BattlenetAPI::GetPresenceIDForCurrentAccount()), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
        }
    }

    DebugLog("BNUI: Chat Channel Joined. channel %d", channel + 1);
}

// VERIFY: a5450f66 auto
// OFFSET: 0x00539020
void BattlenetUI::ChatChannelLeftCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ChatConversation conversation;
    conversation.m_presenceID = BattlenetAPI::GetPresenceIDForCurrentAccount();
    conversation.m_index = channel;
    conversation.m_inactive = 0;
    CGChat::AddChatMessage("YOU_LEFT_CONVERSATION", CHAT_MSG_BN_CONVERSATION_NOTICE, BattlenetUI::GetNameForPresenceIdHelper(BattlenetAPI::GetPresenceIDForCurrentAccount()), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_LEFT, "%d", channel + 1);
    DebugLog("BNUI: Chat Channel Left. Channel %d", channel + 1);
}

// VERIFY: 169ab59f auto
// OFFSET: 0x00539100
void BattlenetUI::ChatChannelClosedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ChatConversation conversation;
    conversation.m_index = channel;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 0;
    CGChat::AddChatMessage("CONVERSATION_CONVERTED_TO_WHISPER", CHAT_MSG_BN_CONVERSATION_NOTICE, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_CLOSED, "%d%s", channel + 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));
    DebugLog("BNUI: Chat Channel Closed channel, index %d, remaining ID %u", channel + 1, presenceId);
}

// VERIFY: bbcce2b9 manual
// OFFSET: 0x005391F0
void BattlenetUI::ChatChannelMessageReceivedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());
    const char* message = args[2].ToString();

    if (!message) {
        message = "";
    }

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char text[1024];
    FilterChatLinks(message, text, 1021);
    BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));

    ChatConversation conversation;
    conversation.m_index = channel;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 0;
    CGChat::AddChatMessage(text, CHAT_MSG_BN_CONVERSATION, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
    DebugLog("BNUI: Chat Channel Message Recieved. channel %d, ID %u, msg: %s", channel + 1, presenceId, message);
}

// VERIFY: 808d4862 auto
// OFFSET: 0x00539320
void BattlenetUI::ChatChannelMessageBlockedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_MESSAGE_BLOCKED, "%d%u", channel + 1, presenceId);
    DebugLog("BNUI: Chat Channel Message Blocked. channel %d, id %u", channel + 1, presenceId);
}

// VERIFY: c2ba753f auto
// OFFSET: 0x005393C0
void BattlenetUI::ChatChannelMemberJoinedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));

    ChatConversation conversation;
    conversation.m_index = channel;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 0;
    CGChat::AddChatMessage("MEMBER_JOINED", CHAT_MSG_BN_CONVERSATION_NOTICE, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_MEMBER_JOINED, "%d%u", channel + 1, presenceId);
    DebugLog("BNUI: Chat Channel Member Joined. channel %d, id %u", channel + 1, presenceId);
}

// VERIFY: 6d58d1f8 auto
// OFFSET: 0x005394D0
void BattlenetUI::ChatChannelMemberLeftCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ChatConversation conversation;
    conversation.m_index = channel;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 0;
    CGChat::AddChatMessage("MEMBER_LEFT", CHAT_MSG_BN_CONVERSATION_NOTICE, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_MEMBER_LEFT, "%d%u", channel + 1, presenceId);
    DebugLog("BNUI: Chat Channel Member Left. channel %d, id %u", channel + 1, presenceId);
}

// VERIFY: 6952ac52 auto
// OFFSET: 0x005395C0
void BattlenetUI::ChatChannelMemberUpdatedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());
    uint32_t presenceId = static_cast<uint32_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_MEMBER_UPDATED, "%d%u", channel + 1, presenceId);
    DebugLog("BNUI: Chat Channel Member Updated. channel %d, id %u", channel + 1, presenceId);
}

// VERIFY: 15d74ce1 auto
// OFFSET: 0x00539660
void BattlenetUI::DisplayError(uint16_t error, uint32_t presenceId) {
    if (!error) {
        return;
    }

    const char* format = BattlenetAPI::GetErrorString(error);

    if (!format) {
        return;
    }

    char text[1024];
    int32_t count = CountFormatStrings(format);

    if (count == 0) {
        SStrCopyUTF8(text, format, sizeof(text), sizeof(text));
    } else if (count == 1) {
        const char* name = BattlenetUI::GetNameForPresenceIdHelper(presenceId);

        if (!name) {
            name = FrameScript_GetText("UNKNOWNOBJECT", -1, GENDER_NOT_APPLICABLE);
        }

        SStrPrintf(text, sizeof(text), format, name);
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("INTERNAL_STRING_ERROR", -1, GENDER_NOT_APPLICABLE), error);
    }

    CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);

    if (error != 318) {
        CGGameUI::AddErrorMessage(text, 0);
    }
}

// VERIFY: 74ed918e manual
// OFFSET: 0x00539770
void BattlenetUI::FriendsOfFriendCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_friendsOfFriend) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint16_t error = static_cast<uint16_t>(args[0].ToUInt64());

    if (error) {
        BattlenetUI::DisplayError(error, 0);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_REQUEST_FOF_FAILED, nullptr);
        DebugLog("BNUI: FOF info request failed.");
    } else {
        BattlenetUI::s_friendsOfFriend->WalkAndGet(args[1].ToObject());
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_REQUEST_FOF_SUCCEEDED, nullptr);
        DebugLog("BNUI: Received FOF info for ID %u. %d friends in list", BattlenetUI::s_friendsOfFriend->m_data.m_presenceId, BattlenetUI::s_friendsOfFriend->m_data.m_friends.m_count);
    }
}

// VERIFY: 5ad292f1 auto
// OFFSET: 0x00539850
void BattlenetUI::BlockListInitializedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_BLOCK_LIST_UPDATED, nullptr);
    DebugLog("BNUI: Block List Initialized");
}

// VERIFY: 40fe5192 auto
// OFFSET: 0x005398B0
void BattlenetUI::BlockAddedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_BLOCK_LIST_UPDATED, nullptr);
    DebugLog("BNUI: Block Added for ID %u", presenceId);
}

// VERIFY: ecddaef1 auto
// OFFSET: 0x00539910
void BattlenetUI::BlockRemovedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_BLOCK_LIST_UPDATED, nullptr);
    DebugLog("BNUI: Block Removed for ID %u", presenceId);
}

// VERIFY: 13cb927d auto
// OFFSET: 0x00539970
void BattlenetUI::SystemMessageCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    const char* type = "unknown";
    uint32_t value = 0;

    switch (static_cast<uint32_t>(args[0].ToUInt64())) {
        case 3800:
            type = "shutdown minutes";
            value = static_cast<uint32_t>(args[1].ToUInt64());
            break;

        case 3801:
            type = "shutdown seconds";
            value = static_cast<uint32_t>(args[1].ToUInt64());
            break;

        case 3802:
            type = "shutdown now";
            break;
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_SYSTEM_MESSAGE, "%s%u", type, value);
    DebugLog("BNUI: SYSTEM MESSAGE RECEIVED: %s %u", type, value);
}

// VERIFY: 2598718e manual
// OFFSET: 0x00539A20
void BattlenetUI::SettingChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!SStrCmpI(args[1].ToString(), "Chat.ProfanityFilterEnabled", STORM_MAX_STR)) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_MATURE_LANGUAGE_FILTER, "%b", args[2].ToBool() != 0);
    }
}

// VERIFY: 473b1361 manual
// OFFSET: 0x00539AD0
void BattlenetUI::PopulateAutoComplete() {
    uint32_t count = BattlenetUI::m_friends.m_count;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t presenceId = BattlenetUI::m_friends[i].m_presenceID;

        if (!presenceId) {
            SErrDisplayAppFatalMessage("", "", 0);
        }

        const char* name = BattlenetUI::GetNameForPresenceIdHelper(presenceId);
        bool online = false;
        Battlenet::Presence::FieldVal value = BattlenetAPI::GetPresenceValue(presenceId, BattlenetUI::s_presenceCallbacks[BN_PRESENCE_ONLINE].m_field);

        if (value.m_tag == 1) {
            online = value.m_bool != 0;
        }

        BattlenetUI::UpdateAutoComplete(presenceId, (online ? 0x20 : 0) | 0x4, 0, name);
    }

    for (uint8_t channel = 0; channel < 6; channel++) {
        Battlenet::Chat::MemberList members = BattlenetAPI::GetChatMemberList(channel);
        uint32_t memberCount = members.m_members.m_count;

        for (uint32_t i = 0; i < memberCount; i++) {
            uint32_t presenceId = members.m_members.m_data[i];
            BattlenetUI::UpdateAutoComplete(presenceId, 0x30, 1, BattlenetUI::GetNameForPresenceIdHelper(presenceId));
        }
    }
}

// VERIFY: 63ed8bee manual
// OFFSET: 0x0053A750
void BattlenetUI::FriendListInitializedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    BSN::BSNEncodedObject<Battlenet::Friends::FriendList, Protocol> friendList;
    friendList.WalkAndGet(args[0].ToObject());

    BattlenetUI::m_friends.m_count = 0;

    for (uint32_t i = 0; i < friendList.m_data.m_friends.m_count; i++) {
        BattlenetUI::AddToFriendList(friendList.m_data.m_friends.m_data[i]);
    }

    BattlenetUI::DisambiguateFriendList();

    if (BattlenetUI::m_friends.m_count > 1) {
        qsort(BattlenetUI::m_friends.m_data, BattlenetUI::m_friends.m_count, sizeof(BattlenetUIFriend), BattlenetUI::QSortFriendListCB);
    }

    BattlenetUI::m_friendListLoaded = true;

    for (uint32_t i = 0; i < BattlenetUI::m_friends.m_count; i++) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_TOON_ONLINE, "%u", BattlenetUI::m_friends.m_data[i].m_presenceID);
        DebugLog("BNUI: >> TOON %s is now ONLINE. ID %u (friend list initialized)", BattlenetUI::GetNameForPresenceIdHelper(BattlenetUI::m_friends[i].m_presenceID));
    }

    BattlenetUI::PopulateAutoComplete();
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_LIST_SIZE_CHANGED, nullptr);
    DebugLog("BNUI: Friend List Initialized. Num friends = %d", BattlenetUI::m_friends.m_count);
}

// VERIFY: 07dfe513 auto
// OFFSET: 0x0053A940
void BattlenetUI::FriendAddedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId;

    if (!BattlenetUI::s_initialized || (presenceId = static_cast<uint32_t>(args[0].ToUInt64())) == 0) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (BattlenetAPI::IsRIDFriend(presenceId)) {
        BattlenetUI::AddToFriendList(presenceId);
        BattlenetUI::DisambiguateFriendList();

        if (BattlenetUI::m_friends.m_count > 1) {
            qsort(BattlenetUI::m_friends.m_data, BattlenetUI::m_friends.m_count, sizeof(BattlenetUIFriend), BattlenetUI::QSortFriendListCB);
        }

        BattlenetUI::PopulateAutoComplete();
        DebugLog("BNUI: Friend Added. ID = %u", presenceId);
    } else {
        DebugLog("BNUI: Friend Add, %u not friend", presenceId);
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_LIST_SIZE_CHANGED, nullptr);

    ChatConversation conversation;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 1;
    CGChat::AddChatMessage("FRIEND_ADDED", CHAT_MSG_BN_INLINE_TOAST_ALERT, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
}

// VERIFY: dda529af auto
// OFFSET: 0x0053AA70
void BattlenetUI::FriendRemovedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId;

    if (!BattlenetUI::s_initialized || (presenceId = static_cast<uint32_t>(args[0].ToUInt64())) == 0) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    int32_t index = BattlenetUI::FindFriendIndex(presenceId);

    if (index < 0) {
        DebugLog("BNUI: Friend Removed: ID % not found", presenceId);
        return;
    }

    BattlenetUI::m_friends.Remove(index);
    DebugLog("BNUI: Friend Removed. Index %d, ID %u", index, presenceId);
    BattlenetUI::DisambiguateFriendList();

    if (BattlenetUI::m_friends.m_count > 1) {
        qsort(BattlenetUI::m_friends.m_data, BattlenetUI::m_friends.m_count, sizeof(BattlenetUIFriend), BattlenetUI::QSortFriendListCB);
    }

    BattlenetUI::PopulateAutoComplete();
    CGAutoCompleteName::RemoveFlags(presenceId, 0x4);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_LIST_SIZE_CHANGED, nullptr);

    ChatConversation conversation;
    conversation.m_presenceID = presenceId;
    conversation.m_inactive = 1;
    CGChat::AddChatMessage("FRIEND_REMOVED", CHAT_MSG_BN_INLINE_TOAST_ALERT, BattlenetUI::GetNameForPresenceIdHelper(presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
}

// VERIFY: 36078bc2 auto
// OFFSET: 0x0053ABC0
void BattlenetUI::OnlinePresenceChangedCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    if (!BattlenetUI::s_initialized) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BSN::BSNEncodedObject<Battlenet::Presence::FieldUpdate, Protocol> update(args[0].ToObject());
    bool online = false;

    if (update.m_data.m_value.m_tag == 1) {
        online = update.m_data.m_value.m_bool != 0;
    }

    bool ridFriend = BattlenetAPI::IsRIDFriend(update.m_data.m_presenceId);
    bool cidFriend = BattlenetAPI::IsCIDFriend(update.m_data.m_presenceId);

    BattlenetUI::DisambiguateFriendList();

    if (BattlenetUI::m_friends.m_count > 1) {
        qsort(BattlenetUI::m_friends.m_data, BattlenetUI::m_friends.m_count, sizeof(BattlenetUIFriend), BattlenetUI::QSortFriendListCB);
    }

    if (online) {
        BattlenetUI::UpdateAutoComplete(update.m_data.m_presenceId, 0x20, 0, nullptr);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INFO_CHANGED, nullptr);
        return;
    }

    CGAutoCompleteName::RemoveFlags(update.m_data.m_presenceId, 0x20);

    if (ridFriend) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_ACCOUNT_OFFLINE, "%u", update.m_data.m_presenceId);

        if (s_cvShowToastOffline && s_cvShowToastOffline->m_intValue) {
            ChatConversation conversation;
            conversation.m_presenceID = update.m_data.m_presenceId;
            conversation.m_inactive = 1;
            CGChat::AddChatMessage("FRIEND_OFFLINE", CHAT_MSG_BN_INLINE_TOAST_ALERT, BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, &conversation);
        }

        DebugLog("BNUI: >> ACCOUNT %s is now OFFLINE. ID %u", BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), update.m_data.m_presenceId);
    } else if (cidFriend) {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_TOON_OFFLINE, "%u", update.m_data.m_presenceId);
        DebugLog("BNUI: >> TOON %s is now OFFLINE. ID %u", BattlenetUI::GetNameForPresenceIdHelper(update.m_data.m_presenceId), update.m_data.m_presenceId);
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_FRIEND_INFO_CHANGED, nullptr);
}

// VERIFY: 5c45d117 auto
// OFFSET: 0x0053ADC0
void BattlenetUI::ChatWhisperUndeliverableCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint32_t presenceId = static_cast<uint32_t>(args[0].ToUInt64());
    BattlenetUI::DisplayError(static_cast<uint16_t>(args[1].ToUInt64()), presenceId);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_WHISPER_UNDELIVERABLE, "%u", presenceId);
}

// VERIFY: 0ef6cc1e manual
// OFFSET: 0x0053AE30
void BattlenetUI::ChatChannelCreateCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint16_t error = static_cast<uint16_t>(args[0].ToUInt64());

    if (error) {
        uint32_t presenceId = 0;

        if (argc >= 2) {
            if (args[1].m_type == GeneralFunction::Arg::TYPE_OBJECT) {
                BSN::BSNEncodedObject<Battlenet::Toon::FullName, Protocol> toon;
                toon.WalkAndGet(args[1].ToObject());
                presenceId = BattlenetAPI::GetPresenceIDForToon(&toon.m_data);
            } else if (args[1].m_type == GeneralFunction::Arg::TYPE_INT || args[1].m_type == GeneralFunction::Arg::TYPE_UINT) {
                presenceId = static_cast<uint32_t>(args[1].ToUInt64());
            }
        }

        BattlenetUI::DisplayError(error, presenceId);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_CREATE_FAILED, "%u", presenceId);
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[1].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_CREATE_SUCCEEDED, "%d", channel + 1);
    DebugLog("BNUI: Chat Channel Create Succeeded. channel = %d", channel + 1);
}

// VERIFY: 70e39278 manual
// OFFSET: 0x0053AF70
void BattlenetUI::ChatChannelInviteCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint16_t error = static_cast<uint16_t>(args[0].ToUInt64());
    BSN::BSNEncodedObject<Battlenet::Toon::FullName, Protocol> toon;
    toon.WalkAndGet(args[1].ToObject());
    uint32_t presenceId = BattlenetAPI::GetPresenceIDForToon(&toon.m_data);
    uint8_t channel = static_cast<uint8_t>(args[2].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (error) {
        BattlenetUI::DisplayError(error, presenceId);
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_INVITE_FAILED, "%d%u", channel + 1, presenceId);
        DebugLog("BNUI: Chat Channel Invite Failed. channel %d, id %d", channel + 1, presenceId);
    } else {
        BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_INVITE_SUCCEEDED, "%d%u", channel + 1, presenceId);
        DebugLog("BNUI: Chat Channel Invite Succeeded. channel %d, id %u", channel + 1, presenceId);
    }
}

// VERIFY: fa26f5b8 auto
// OFFSET: 0x0053B0D0
void BattlenetUI::ChatChannelMessageUndeliverableCB(void* param, int32_t argc, const GeneralFunction::Arg* args) {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled()) {
        return;
    }

    uint8_t channel = static_cast<uint8_t>(args[0].ToUInt64());

    if (channel >= 6) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::DisplayError(static_cast<uint16_t>(args[1].ToUInt64()), 0);
    BattlenetUI::SendScriptEvent(SCRIPT_EVENT_BN_CHAT_CHANNEL_MESSAGE_UNDELIVERABLE, "%d", channel + 1);
    DebugLog("BNUI: Chat Channel Message Undeliverable. channel %d", channel + 1);
}

// VERIFY: 1081d8e3 manual
// OFFSET: 0x0053B170
void BattlenetUI::Init() {
    if (!g_tumorManager) {
        return;
    }

    if (BattlenetUI::m_friendInvites) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    BattlenetUI::m_friendInvites = new (__FILE__, __LINE__) BSN::BSNEncodedObject<Battlenet::Friends::FriendInviteContainer, Protocol>();
    BattlenetUI::s_friendsOfFriend = new (__FILE__, __LINE__) BSN::BSNEncodedObject<Battlenet::Friends::FriendsOfFriendList, Protocol>();

    for (uint32_t i = 0; i < 26; i++) {
        GeneralFunction::SimpleCallback* callback = new (__FILE__, __LINE__) GeneralFunction::SimpleCallback(BattlenetUI::s_eventCallbacks[i].m_func, nullptr);
        BattlenetUI::s_eventCallbacks[i].m_handle = BattlenetAPI::RegisterEvent(BattlenetUI::s_eventCallbacks[i].m_event, callback);

        if (!BattlenetUI::s_eventCallbacks[i].m_handle) {
            SErrDisplayAppFatalMessage("", "", 0);
        }
    }

    for (uint32_t i = 0; i < NUM_BN_PRESENCE_FIELDS; i++) {
        GeneralFunction::SimpleCallback* callback = new (__FILE__, __LINE__) GeneralFunction::SimpleCallback(BattlenetUI::s_presenceCallbacks[i].m_func, nullptr);
        BattlenetUI::s_presenceCallbacks[i].m_handle = BattlenetAPI::RegisterPresenceUpdateCallback(0, BattlenetUI::s_presenceCallbacks[i].m_field, callback);

        if (!BattlenetUI::s_presenceCallbacks[i].m_handle) {
            SErrDisplayAppFatalMessage("", "", 0);
        }
    }

    BattlenetUI::s_createConversationCB = new (__FILE__, __LINE__) GeneralFunction::SimpleCallback(BattlenetUI::ChatChannelCreateCB, nullptr);
    BattlenetUI::s_createConversationCB->IncRef();

    BattlenetUI::s_inviteToConversationCB = new (__FILE__, __LINE__) GeneralFunction::SimpleCallback(BattlenetUI::ChatChannelInviteCB, nullptr);
    BattlenetUI::s_inviteToConversationCB->IncRef();

    BattlenetUI::s_friendsOfFriendCB = new (__FILE__, __LINE__) GeneralFunction::SimpleCallback(BattlenetUI::FriendsOfFriendCB, nullptr);
    BattlenetUI::s_friendsOfFriendCB->IncRef();

    BattlenetUI::s_initialized = true;
    DebugLog("BNUI: Init");
}

// VERIFY: fa7e6db5 auto
// OFFSET: 0x0053B370
void BattlenetUI_EnterRealm() {
    Login* login = ClientServices::LoginConnection();

    if (!login || login->GetLoginServerType() != 1 || login->IsTrialAccount() || !BattlenetAPI::IsRIDEnabled() || !static_cast<BattlenetLogin*>(login)->IsConnected()) {
        return;
    }

    BattlenetUI::PopulateAutoComplete();

    const char* nameFormat = FrameScript_GetText("BATTLENET_NAME_FORMAT", -1, GENDER_NOT_APPLICABLE);

    if (*nameFormat) {
        BattlenetAPI::SetAccountNameFormatString(nameFormat);
    }
}
