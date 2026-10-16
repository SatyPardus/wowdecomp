#include "FriendList.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <new>
#include <cstdlib>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include <cstdio>
#include <cwctype>
#include <cctype>
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

// STATIC: 0x00C79F98
FriendList* FriendList::s_friendList;

// STATIC: 0x00C79F9C
WhoSortField FriendList::s_whoSort[7];

// STATIC: 0x00C79FD8
WhoResult FriendList::s_whoResults[50];

// STATIC: 0x00C7BFE0
uint32_t FriendList::s_numWhoResults;

// STATIC: 0x00C7BFE4
uint32_t FriendList::s_totalWhoResults;

// STATIC: 0x00C7BFE8
static CVar* s_whoListLimit;

// STATIC: 0x00C7BFEC
int32_t FriendList::s_whoToUI;

// VERIFY: a4507187 manual
// OFFSET: 0x006B3510
FriendInfo* FriendList::GetFriend(WOWGUID guid) {
    if (!guid) {
        return nullptr;
    }

    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            return &this->m_friends[i];
        }
    }

    return nullptr;
}

// VERIFY: 5305c965 auto
// OFFSET: 0x006B34A0
uint32_t FriendList::GetNumFriends() {
    uint32_t count = 0;

    while (count < 100 && this->m_friends[count].m_guid) {
        count++;
    }

    return count;
}

// INLINED
FriendInfo::FriendInfo() {
    this->m_connected = 0;
    this->m_name = nullptr;
    this->m_notes[0] = '\0';
    this->m_guid = WOWGUID();
}

// INLINED
FriendInfo::~FriendInfo() {
    if (this->m_name) {
        SMemFree(this->m_name, __FILE__, __LINE__, 0);
    }
}

// VERIFY: 45a360ac manual
// OFFSET: 0x006B3190
FriendList::FriendList() {
    memset(this->m_friends, 0, sizeof(this->m_friends));
    memset(this->m_ignores, 0, sizeof(this->m_ignores));
    memset(this->m_mutes, 0, sizeof(this->m_mutes));
    memset(this->m_complained, 0, sizeof(this->m_complained));
    this->m_pendingFriendNames = 0;
    this->m_selectedFriend = WOWGUID();
    this->m_pendingIgnoreNames = 0;
    this->m_selectedIgnore = WOWGUID();
    this->m_pendingMuteNames = 0;
    this->m_selectedMute = WOWGUID();
}

// VERIFY: 2f86035c manual
// OFFSET: 0x006B3240
FriendList::~FriendList() {
}

// VERIFY: c0f3513f manual
// OFFSET: 0x006B8990
void FriendList::Initialize() {
    if (FriendList::s_friendList) {
        return;
    }

    void* mem = SMemAlloc(sizeof(FriendList), __FILE__, __LINE__, 0);
    FriendList::s_friendList = mem ? new (mem) FriendList() : nullptr;

    ClientServices::SetMessageHandler(SMSG_WHO, Packet_SMSG_WHO, nullptr);
    ClientServices::SetMessageHandler(SMSG_WHO_IS, Packet_SMSG_WHOIS, nullptr);
    ClientServices::SetMessageHandler(SMSG_RWHOIS, Packet_SMSG_RWHOIS, nullptr);
    ClientServices::SetMessageHandler(SMSG_CONTACT_LIST, Packet_SMSG_CONTACT_LIST, nullptr);
    ClientServices::SetMessageHandler(SMSG_FRIEND_STATUS, Packet_SMSG_FRIEND_STATUS, nullptr);
    ConsoleCommandRegister("whois", CCommand_Whois, CATEGORY_DEBUG, "Ask the server to do an account/real name lookup on a character name");

    for (int32_t i = 0; i < 7; i++) {
        FriendList::s_whoSort[i].m_field = i;
        FriendList::s_whoSort[i].m_reverse = 0;
    }

    FriendList::s_numWhoResults = 0;
    FriendList::s_totalWhoResults = 0;
}

// VERIFY: 7186f5af manual
// OFFSET: 0x006B3F90
FriendInfo* FriendList::ClearRecruitAFriend(WOWGUID guid) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            this->m_friends[i].m_flags &= ~0x8;
            return &this->m_friends[i];
        }
    }

    return nullptr;
}

// VERIFY: e2abf0bf manual
// OFFSET: 0x006B7DD0
void FriendList::ReadPacket(CDataStore* msg) {
    uint32_t friendIndex = 0;
    uint32_t flags;
    uint32_t count;
    msg->Get(flags);
    msg->Get(count);

    if (flags & 0x1) {
        for (uint32_t i = 0; i < 100; i++) {
            if (this->m_friends[i].m_name) {
                SMemFree(this->m_friends[i].m_name, __FILE__, __LINE__, 0);
            }

            this->m_friends[i].m_name = nullptr;
        }
    }

    if (flags & 0x2) {
        memset(this->m_ignores, 0, sizeof(this->m_ignores));
    }

    if (flags & 0x4) {
        memset(this->m_mutes, 0, sizeof(this->m_mutes));
    }

    for (uint32_t n = 0; n < count; n++) {
        WOWGUID guid;
        uint32_t entryFlags;
        char note[512];
        msg->Get(guid.m_guid);
        msg->Get(entryFlags);
        msg->GetString(note, sizeof(note));

        if (entryFlags & 0x1) {
            uint32_t area = 0;
            uint32_t level = 0;
            uint32_t classId = 0;
            uint8_t status;
            msg->Get(status);

            if (status) {
                msg->Get(area);
                msg->Get(level);
                msg->Get(classId);
            }

            if ((flags & 0x1) && friendIndex < 100) {
                FriendInfo& info = this->m_friends[friendIndex];
                info.m_flags = status;
                info.m_guid = guid;
                info.m_area = area;
                info.m_level = level;
                info.m_connected = status != 0;
                info.m_deleted = 0;
                info.m_class = classId;
                SStrCopy(info.m_notes, note, sizeof(info.m_notes));

                NameCache* record = g_nameDBCache.GetRecord(info.m_guid.m_guid, info.m_guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_MAX_VALUE), false);

                if (record) {
                    char name[305];

                    if (!record->m_realmName[0]) {
                        SStrCopy(name, record->m_name, sizeof(name));
                    } else {
                        SStrPrintf(name, sizeof(name), "%s-%s", record->m_name, record->m_realmName);
                    }

                    info.m_name = SStrDupA(name, __FILE__, __LINE__);

                    if (info.m_connected) {
                        CGAutoCompleteName::AddFlags(guid, 0x24, 0, nullptr);
                    } else {
                        CGAutoCompleteName::AddFlags(guid, 0x4, 0, nullptr);
                        CGAutoCompleteName::RemoveFlags(guid, 0x20);
                    }
                } else {
                    if (info.m_connected) {
                        CGAutoCompleteName::AddFlags(guid, 0x24, 0, nullptr);
                    } else {
                        CGAutoCompleteName::AddFlags(guid, 0x4, 0, nullptr);
                        CGAutoCompleteName::RemoveFlags(guid, 0x20);
                    }

                    this->m_pendingFriendNames++;
                }

                friendIndex++;
            }
        }

        if (entryFlags & flags & 0x2) {
            WOWGUID ignoreGuid = guid;

            for (uint32_t i = 0; i < 50; i++) {
                if (!this->m_ignores[i].m_guid.m_guid) {
                    this->m_ignores[i].m_guid = ignoreGuid;
                    this->m_ignores[i].m_flags = 0;
                    CGCalendar::UpdateDisplayList(1);
                    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
                    break;
                }
            }

            VoiceSessionSetTalkerMuted(ignoreGuid, 0);

            if (!g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListIgnoreNameCallback, reinterpret_cast<void*>(ERR_MAX_VALUE), false)) {
                this->m_pendingIgnoreNames++;
            }
        }

        if (entryFlags & flags & 0x4) {
            WOWGUID muteGuid = guid;

            for (uint32_t i = 0; i < 50; i++) {
                if (!this->m_mutes[i].m_guid.m_guid) {
                    this->m_mutes[i].m_guid = muteGuid;
                    this->m_mutes[i].m_flags = 0;
                    break;
                }
            }

            VoiceSessionSetTalkerMuted(guid, 0);

            if (!g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListMuteNameCallback, reinterpret_cast<void*>(ERR_MAX_VALUE), false)) {
                this->m_pendingMuteNames++;
            }
        }
    }

    if ((flags & 0x1) && !this->m_pendingFriendNames) {
        qsort(this->m_friends, 100, sizeof(FriendInfo), QSortFriends);
        FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);
    }

    if ((flags & 0x2) && !this->m_pendingIgnoreNames) {
        qsort(this->m_ignores, 50, sizeof(IgnoreInfo), QSortIgnore);
        FrameScript_SignalEvent(SCRIPT_EVENT_IGNORELIST_UPDATE, nullptr);
    }

    if ((flags & 0x4) && !this->m_pendingMuteNames) {
        qsort(this->m_mutes, 50, sizeof(IgnoreInfo), QSortIgnore);
        FrameScript_SignalEvent(SCRIPT_EVENT_MUTELIST_UPDATE, nullptr);
    }

    if (!msg->IsRead()) {
        uint8_t key[16];
        msg->GetArray(key, sizeof(key));

        if (msg->m_read <= msg->m_size) {
            LoadEncryptedModule(key, sizeof(key), FriendListModuleLoaded);
        }
    }
}

// VERIFY: 5a67a606 auto
// OFFSET: 0x006B8700
int32_t Packet_SMSG_CONTACT_LIST(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    FriendList::s_friendList->ReadPacket(msg);
    return 1;
}

// VERIFY: 40fabbbe auto
// OFFSET: 0x006B3630
uint32_t FriendList::GetNumIgnores() {
    uint32_t count = 0;

    while (count < 50 && this->m_ignores[count].m_guid) {
        count++;
    }

    return count;
}

// VERIFY: bbaa58d9 manual
// OFFSET: 0x006B3750
void FriendList::AddComplained(WOWGUID guid) {
    uint32_t index;

    for (index = 0; index < 32; index++) {
        if (this->m_complained[index] == guid.m_guid) {
            break;
        }
    }

    uint32_t count = index >= 31 ? 31 : index;
    memmove(&this->m_complained[1], &this->m_complained[0], count * sizeof(uint64_t));
    this->m_complained[0] = guid.m_guid;
}

// VERIFY: dfd1e135 manual
// OFFSET: 0x006B3960
int32_t QSortFriends(const void* a, const void* b) {
    if (!a || !b) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    const FriendInfo* friendA = static_cast<const FriendInfo*>(a);
    const FriendInfo* friendB = static_cast<const FriendInfo*>(b);

    if (!friendA->m_guid && !friendB->m_guid) {
        return 0;
    }

    if (!friendA->m_guid) {
        return 1;
    }

    if (!friendB->m_guid) {
        return -1;
    }

    if (friendA->m_connected != friendB->m_connected) {
        return friendA->m_connected ? -1 : 1;
    }

    if (friendA->m_name && friendB->m_name) {
        return SStrSortUTF8I(friendA->m_name, friendB->m_name, 0x7FFFFFFF);
    }

    if (!friendA->m_name && !friendB->m_name) {
        return 0;
    }

    return friendA->m_name ? -1 : 1;
}

// VERIFY: 5c7fff6f manual
// OFFSET: 0x006B3A60
int32_t QSortIgnore(const void* a, const void* b) {
    if (!a || !b) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WOWGUID guidA = static_cast<const IgnoreInfo*>(a)->m_guid;
    WOWGUID guidB = static_cast<const IgnoreInfo*>(b)->m_guid;

    if (!guidA) {
        return guidB ? 1 : 0;
    }

    if (!guidB) {
        return -1;
    }

    NameCache* nameA = g_nameDBCache.GetRecord(guidA.m_guid, WOWGUID(), nullptr, nullptr, false);
    NameCache* nameB = g_nameDBCache.GetRecord(guidB.m_guid, WOWGUID(), nullptr, nullptr, false);

    if (!nameA) {
        return nameB ? 1 : 0;
    }

    if (!nameB) {
        return -1;
    }

    char fullNameA[305];
    char fullNameB[305];
    return SStrCmp(nameA->FullName(fullNameA, sizeof(fullNameA)), nameB->FullName(fullNameB, sizeof(fullNameB)), 0x7FFFFFFF);
}

// VERIFY: 851ea1b0 manual
// OFFSET: 0x006B51B0
void FriendList::Destroy() {
    if (!FriendList::s_friendList) {
        return;
    }

    ClientServices::ClearMessageHandler(SMSG_WHO);
    ClientServices::ClearMessageHandler(SMSG_WHO_IS);
    ClientServices::ClearMessageHandler(SMSG_RWHOIS);
    ClientServices::ClearMessageHandler(SMSG_CONTACT_LIST);
    ClientServices::ClearMessageHandler(SMSG_FRIEND_STATUS);
    ConsoleCommandUnregister("whois");
    delete FriendList::s_friendList;
    FriendList::s_friendList = nullptr;
}

// VERIFY: 862d3015 auto
// OFFSET: 0x006B3700
bool FriendList::IsComplained(WOWGUID guid) {
    if (!guid) {
        return false;
    }

    for (uint32_t i = 0; i < 32; i++) {
        if (this->m_complained[i] == guid.m_guid) {
            return true;
        }
    }

    return false;
}

// VERIFY: 95f270e1 manual
// OFFSET: 0x006B52E0
bool FriendList::IsIgnored(WOWGUID guid, bool includeComplained) {
    if (!guid) {
        return false;
    }

    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_ignores[i].m_guid == guid) {
            return true;
        }
    }

    return includeComplained && CGChat::m_complaintsSystemStatus == 2 && this->IsComplained(guid);
}

// VERIFY: de50a749 auto
// OFFSET: 0x006B4C80
int32_t CompareWhoResults(const void* a, const void* b) {
    if (!a || !b) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    WhoResult left = *static_cast<const WhoResult*>(a);
    WhoResult right = *static_cast<const WhoResult*>(b);

    for (int32_t i = 0; i < 7; i++) {
        int32_t result;

        switch (FriendList::s_whoSort[i].m_field) {
            case 0: {
                AreaTableRec* leftArea = g_areaTableDB.GetRecord(left.m_zone);
                AreaTableRec* rightArea = g_areaTableDB.GetRecord(right.m_zone);

                if (!leftArea || !rightArea) {
                    continue;
                }

                result = SStrSortUTF8I(leftArea->m_areaName, rightArea->m_areaName, STORM_MAX_STR);
                break;
            }

            case 1:
                if (left.m_level == right.m_level) {
                    continue;
                }

                result = 2 * (left.m_level >= right.m_level) - 1;
                break;

            case 2: {
                ChrClassesRec* leftClass = g_chrClassesDB.GetRecord(left.m_class);
                ChrClassesRec* rightClass = g_chrClassesDB.GetRecord(right.m_class);

                if (!leftClass || !rightClass) {
                    continue;
                }

                result = SStrSortUTF8I(leftClass->m_name, rightClass->m_name, STORM_MAX_STR);
                break;
            }

            case 4:
                result = SStrCmpUTF8I(left.m_name, right.m_name, STORM_MAX_STR);
                break;

            case 5: {
                ChrRacesRec* leftRace = g_chrRacesDB.GetRecord(left.m_race);
                ChrRacesRec* rightRace = g_chrRacesDB.GetRecord(right.m_race);

                if (!leftRace || !rightRace) {
                    continue;
                }

                result = SStrSortUTF8I(leftRace->m_name, rightRace->m_name, STORM_MAX_STR);
                break;
            }

            case 6:
                result = SStrSortUTF8I(left.m_guild, right.m_guild, STORM_MAX_STR);
                break;

            default:
                continue;
        }

        if (result) {
            return FriendList::s_whoSort[i].m_reverse ? -result : result;
        }
    }

    return 0;
}

// VERIFY: 44b861fd auto
// OFFSET: 0x006B5030
static void PrintWho(const char* name, int32_t race, int32_t classId, const char* guild, int32_t level, int32_t zone, int32_t gender) {
    const char* raceName = CGUnit_C::GetDisplayRaceNameFromRecord(g_chrRacesDB.GetRecord(race), gender, nullptr);

    if (!raceName) {
        raceName = FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
    }

    const char* className = CGUnit_C::GetDisplayClassNameFromRecord(g_chrClassesDB.GetRecord(classId), gender, nullptr);

    if (!className) {
        className = FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
    }

    AreaTableRec* area = g_areaTableDB.GetRecord(zone);
    const char* zoneName = area ? area->m_areaName : FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
    char text[3000];

    if (guild && *guild) {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("WHO_LIST_GUILD_FORMAT", -1, GENDER_NOT_APPLICABLE), name, name, level, raceName, className, guild, zoneName);
    } else {
        SStrPrintf(text, sizeof(text), FrameScript_GetText("WHO_LIST_FORMAT", -1, GENDER_NOT_APPLICABLE), name, name, level, raceName, className, zoneName);
    }

    CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
}

// VERIFY: b37debf7 manual
// OFFSET: 0x006B5400
void __stdcall FriendList::AddFriend(const char* name, const char* note) {
    CGTutorial::FlagTutorial(21);
    char noteBuffer[512];
    SStrCopy(noteBuffer, note, sizeof(noteBuffer));
    char nameBuffer[308];
    SStrCopy(nameBuffer, name, 305);
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ADD_FRIEND));
    msg.PutString(nameBuffer);
    msg.PutString(noteBuffer);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 5f2944e4 manual
// OFFSET: 0x006B5630
void FriendList::SendWho(const char* filter) {
    if (!filter) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    int32_t minLevel = 0;
    int32_t maxLevel = 100;
    int32_t raceMask = -1;
    int32_t classMask = -1;
    uint32_t zoneCount = 0;
    int32_t zones[10];
    char name[48];
    char guild[96];
    char words[4][128];
    char text[128];
    name[0] = '\0';
    guild[0] = '\0';
    SStrCopy(text, filter, sizeof(text));
    uint32_t length = SStrLen(text);
    text[length] = ' ';
    text[length + 1] = '\0';
    uint32_t wordCount = 0;
    uint32_t wordLength = 0;
    const char* cursor = text;
    int32_t chars;

    while (iswspace(sgetu8(reinterpret_cast<const uint8_t*>(cursor), &chars))) {
        cursor += chars;
    }

    int32_t inQuote = 0;
    uint32_t c = sgetu8(reinterpret_cast<const uint8_t*>(cursor), &chars);

    while (c != 0xFFFFFFFF) {
        char* word = words[wordCount];

        if (c == '"') {
            cursor += chars;
            inQuote = !inQuote;
        } else if (!inQuote && iswspace(c)) {
            word[wordLength] = '\0';
            wordLength = 0;
            bool plain = false;
            const char* tag = FrameScript_GetText("WHO_TAG_NAME", -1, GENDER_NOT_APPLICABLE);

            if (!SStrCmpUTF8I(word, tag, SStrLenUTF8(tag))) {
                SStrCopy(name, &word[SStrLen(tag)], 48);
            } else {
                tag = FrameScript_GetText("WHO_TAG_GUILD", -1, GENDER_NOT_APPLICABLE);

                if (!SStrCmpUTF8I(word, tag, SStrLenUTF8(tag))) {
                    SStrCopy(guild, &word[SStrLen(tag)], 48);
                } else {
                    tag = FrameScript_GetText("WHO_TAG_ZONE", -1, GENDER_NOT_APPLICABLE);

                    if (!SStrCmpUTF8I(word, tag, SStrLenUTF8(tag))) {
                        const char* zoneText = &word[SStrLen(tag)];

                        if (zoneCount < 10) {
                            for (int32_t i = 0; i < g_areaTableDB.m_numRecords; i++) {
                                AreaTableRec* area = g_areaTableDB.GetRecordByIndex(i);

                                if (!area->m_parentAreaID && zoneCount < 10 && SStrStrUTF8I(area->m_areaName, zoneText)) {
                                    zones[zoneCount++] = area->m_id;
                                }
                            }
                        }

                        if (!zoneCount) {
                            zones[0] = 0;
                            zoneCount = 1;
                        }
                    } else {
                        tag = FrameScript_GetText("WHO_TAG_RACE", -1, GENDER_NOT_APPLICABLE);

                        if (!SStrCmpUTF8I(word, tag, SStrLenUTF8(tag))) {
                            const char* raceText = &word[SStrLen(tag)];

                            if (raceMask == -1) {
                                raceMask = 0;
                            }

                            for (int32_t i = 0; i < g_chrRacesDB.m_numRecords; i++) {
                                ChrRacesRec* race = g_chrRacesDB.GetRecordByIndex(i);

                                if (SStrStrUTF8I(race->m_name, raceText)) {
                                    raceMask |= 1 << race->m_id;
                                }
                            }
                        } else {
                            tag = FrameScript_GetText("WHO_TAG_CLASS", -1, GENDER_NOT_APPLICABLE);

                            if (!SStrCmpUTF8I(word, tag, SStrLenUTF8(tag))) {
                                const char* classText = &word[SStrLen(tag)];

                                if (classMask == -1) {
                                    classMask = 0;
                                }

                                for (int32_t i = 0; i < g_chrClassesDB.m_numRecords; i++) {
                                    ChrClassesRec* chrClass = g_chrClassesDB.GetRecordByIndex(i);

                                    if (SStrStrUTF8I(chrClass->m_name, classText)) {
                                        classMask |= 1 << chrClass->m_id;
                                    }
                                }
                            } else {
                                bool isLevel = false;
                                const char* next;

                                if (iswdigit(static_cast<uint8_t>(word[0]))) {
                                    minLevel = SStrToInt(word);
                                    maxLevel = minLevel;
                                    isLevel = true;
                                    next = &word[1];

                                    while (iswdigit(static_cast<uint8_t>(*next))) {
                                        next++;
                                    }

                                    if (*next == '-') {
                                        next++;
                                        maxLevel = 100;
                                    }
                                } else if (word[0] == '-') {
                                    next = &word[1];
                                } else {
                                    next = nullptr;
                                    plain = true;
                                }

                                if (!plain) {
                                    if (next && isdigit(static_cast<uint8_t>(*next))) {
                                        maxLevel = SStrToInt(next);
                                    } else if (!isLevel) {
                                        plain = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (plain) {
                wordCount++;
            }

            if (wordCount >= 4) {
                break;
            }

            while (iswspace(sgetu8(reinterpret_cast<const uint8_t*>(cursor), &chars))) {
                cursor += chars;
            }
        } else {
            for (; chars > 0; chars--) {
                word[wordLength++] = *cursor++;
            }
        }

        c = sgetu8(reinterpret_cast<const uint8_t*>(cursor), &chars);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_WHO));
    msg.Put(static_cast<uint32_t>(minLevel));
    this->m_whoMinLevel = minLevel;
    msg.Put(static_cast<uint32_t>(maxLevel));
    this->m_whoMaxLevel = maxLevel;
    msg.PutString(name);
    strcpy(this->m_whoName, name);
    msg.PutString(guild);
    strcpy(this->m_whoGuild, guild);
    msg.Put(static_cast<uint32_t>(raceMask));
    this->m_whoRaceMask = raceMask;
    msg.Put(static_cast<uint32_t>(classMask));
    this->m_whoClassMask = classMask;
    msg.Put(zoneCount);

    for (uint32_t i = 0; i < zoneCount; i++) {
        msg.Put(static_cast<uint32_t>(zones[i]));
    }

    msg.Put(wordCount);
    this->m_whoStringCount = wordCount <= 10 ? wordCount : 10;

    for (uint32_t i = 0; i < wordCount; i++) {
        msg.PutString(words[i]);

        if (i < 10) {
            strcpy(this->m_whoStrings[i], words[i]);
        }
    }

    for (uint32_t i = wordCount; i < 10; i++) {
        this->m_whoStrings[i][0] = '\0';
    }

    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 8063cfa3 auto
// OFFSET: 0x006B5CA0
bool __stdcall FriendList::MatchesFilter(CGPlayer_C* player, const char* filter) {
    NameCache* name = g_nameDBCache.GetRecord(player->m_obj->m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);

    if (!name) {
        return false;
    }

    if (SStrStrUTF8I(name->m_name, filter)) {
        return true;
    }

    ChrRacesRec* race = g_chrRacesDB.GetRecord(player->m_unit->m_unitFieldBytes0[0]);

    if (race && SStrStrUTF8I(race->m_name, filter)) {
        return true;
    }

    ChrClassesRec* chrClass = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]);

    if (chrClass && SStrStrUTF8I(chrClass->m_name, filter)) {
        return true;
    }

    AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    if (area && SStrStrUTF8I(area->m_areaName, filter)) {
        return true;
    }

    const char* guildName = player->GetPlayerGuildNameText();
    return guildName && SStrStrUTF8I(guildName, filter);
}

// VERIFY: 31f81d53 auto
// OFFSET: 0x006B6FF0
bool FriendList::AllMatchFilter(CGPlayer_C* player) {
    for (int32_t i = 0; i < this->m_whoStringCount; i++) {
        if (!FriendList::MatchesFilter(player, this->m_whoStrings[i])) {
            return false;
        }
    }

    return true;
}

// VERIFY: e1bf8edd manual
// OFFSET: 0x006B8290
int32_t FriendList::DoLocalWho() {
    int32_t added = 0;
    CHARACTER_INFO* info = ClientServices::GetCharacterInfo();
    CGPlayer_C* self = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(info->m_guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!self) {
        return 0;
    }

    NameCache* selfName = g_nameDBCache.GetRecord(info->m_guid.m_guid, WOWGUID(), nullptr, nullptr, false);
    int32_t print = 0;

    if (!FriendList::s_whoToUI) {
        int32_t limit = s_whoListLimit ? s_whoListLimit->m_intValue : 3;

        if (limit < 0 || limit >= static_cast<int32_t>(FriendList::s_totalWhoResults)) {
            print = 1;
        }
    }

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        if (FriendList::s_numWhoResults >= 50) {
            return added;
        }

        WOWGUID guid = GetRaidMemberGUID(i);

        if (!guid) {
            continue;
        }

        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_PLAYER, __FILE__, __LINE__));

        if (!player) {
            continue;
        }

        uint8_t race = player->m_unit->m_unitFieldBytes0[0];
        uint8_t classId = player->m_unit->m_unitFieldBytes0[1];

        if (!((1 << race) & this->m_whoRaceMask) || !((1 << classId) & this->m_whoClassMask)) {
            continue;
        }

        NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, WOWGUID(), nullptr, nullptr, false);

        if (!name) {
            continue;
        }

        char* nameFilter = this->m_whoName;

        if (SStrLen(nameFilter) > 0) {
            uint32_t j = 0;

            while (nameFilter[j] != '-') {
                if (++j >= SStrLen(nameFilter)) {
                    goto filtered;
                }
            }

            nameFilter[j] = '\0';
        }

    filtered:
        if (SStrLen(nameFilter) && !SStrStrUTF8I(name->m_name, nameFilter)) {
            continue;
        }

        if (SStrLen(this->m_whoGuild) && (!player->m_player->m_playerGuildid || !SStrStrUTF8I(player->GetPlayerGuildNameText(), this->m_whoGuild))) {
            continue;
        }

        if (player == self && !self->CanCooperate(player)) {
            continue;
        }

        int32_t level = player->m_unit->m_unitFieldLevel;

        if (level < this->m_whoMinLevel || level > this->m_whoMaxLevel) {
            continue;
        }

        if (!strcmp(selfName->m_realmName, name->m_realmName) || !this->AllMatchFilter(player)) {
            continue;
        }

        char buffer[256];
        sprintf(buffer, "%s-", name->m_name);
        uint32_t length = strlen(name->m_name) + 1;

        for (const char* realm = name->m_realmName; *realm; realm++) {
            if (*realm != ' ') {
                buffer[length++] = *realm;
            }
        }

        buffer[length] = '\0';

        if (FriendList::s_numWhoResults < 50) {
            WhoResult& result = FriendList::s_whoResults[FriendList::s_numWhoResults];
            strncpy(result.m_name, buffer, sizeof(result.m_name));

            if (player->GetPlayerGuildNameText()) {
                strncpy(FriendList::s_whoResults[FriendList::s_numWhoResults].m_guild, player->GetPlayerGuildNameText(), sizeof(result.m_guild));
            } else {
                FriendList::s_whoResults[FriendList::s_numWhoResults].m_guild[0] = '\0';
            }

            WhoResult& entry = FriendList::s_whoResults[FriendList::s_numWhoResults];
            entry.m_level = player->m_unit->m_unitFieldLevel;
            entry.m_class = player->m_unit->m_unitFieldBytes0[1];
            entry.m_race = player->m_unit->m_unitFieldBytes0[0];
            entry.m_gender = player->GetDisplaySex();
            entry.m_zone = CGGameUI::m_areaID;
            FriendList::s_totalWhoResults++;
            added++;
            FriendList::s_numWhoResults++;
        }

        if (print) {
            PrintWho(buffer, player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[1], player->GetPlayerGuildNameText(), player->m_unit->m_unitFieldLevel, CGGameUI::m_areaID, player->GetDisplaySex());
        }
    }

    return added;
}

// VERIFY: 17af1591 manual
// OFFSET: 0x006B8720
int32_t Packet_SMSG_WHO(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    if (!msg) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint32_t count;
    uint32_t total;
    msg->Get(count);
    msg->Get(total);
    FriendList::s_numWhoResults = count >= 50 ? 50 : count;
    FriendList::s_totalWhoResults = total;
    int32_t print = 0;

    if (!FriendList::s_whoToUI) {
        int32_t limit = s_whoListLimit ? s_whoListLimit->m_intValue : 3;

        if (limit < 0 || limit >= static_cast<int32_t>(count)) {
            print = 1;
        }
    }

    for (uint32_t i = 0; i < count; i++) {
        char name[48];
        char guild[96];
        msg->GetString(name, sizeof(name));
        msg->GetString(guild, sizeof(guild));
        uint32_t level = 0;
        msg->Get(level);
        uint32_t classId = 0;
        msg->Get(classId);
        uint32_t race = 0;
        msg->Get(race);
        uint8_t gender = 0;
        msg->Get(gender);
        uint32_t zone = 0;
        msg->Get(zone);

        if (i < 50) {
            WhoResult& result = FriendList::s_whoResults[i];
            SStrCopy(result.m_name, name, sizeof(result.m_name));
            SStrCopy(result.m_guild, guild, sizeof(result.m_guild));
            result.m_level = level;
            result.m_class = classId;
            result.m_race = race;
            result.m_gender = gender;
            result.m_zone = zone;
        }

        if (print) {
            PrintWho(name, race, classId, guild, level, zone, gender);
        }
    }

    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if ((map && map->m_instanceType == 3) || map->m_instanceType == 4) {
        if (FriendList::s_friendList) {
            count += FriendList::s_friendList->DoLocalWho();
        }
    }

    qsort(FriendList::s_whoResults, FriendList::s_numWhoResults, sizeof(WhoResult), CompareWhoResults);

    if (print) {
        char text[3000];
        SStrPrintf(text, sizeof(text), FrameScript_GetText("WHO_NUM_RESULTS", -1, GENDER_NOT_APPLICABLE), count);
        CGChat::AddChatMessage(text, 0, nullptr, 0, nullptr, nullptr, nullptr, WOWGUID(), 0, WOWGUID(), 0, false, nullptr);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_WHO_LIST_UPDATE, nullptr);
    }

    return 1;
}

// VERIFY: a407c9d6 manual
// OFFSET: 0x006B5350
bool FriendList::IsMuted(WOWGUID guid) {
    if (!guid) {
        return false;
    }

    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_mutes[i].m_guid == guid) {
            return true;
        }
    }

    return CGChat::m_complaintsSystemStatus == 2 && this->IsComplained(guid);
}

// VERIFY: 1c11f1a1 auto
// OFFSET: 0x006B53B0
bool FriendList::IsIgnoredOrMuted(WOWGUID guid) {
    if (!guid) {
        return false;
    }

    if (this->IsIgnored(guid, true)) {
        return true;
    }

    return this->IsMuted(guid);
}

// VERIFY: 5521ae21 manual
// OFFSET: 0x006B3880
bool FriendList::GetFriendDeleted(WOWGUID guid) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            return this->m_friends[i].m_deleted;
        }
    }

    return false;
}

// VERIFY: 97135ee3 manual
// OFFSET: 0x006B38D0
void FriendList::SetFriendDeleted(WOWGUID guid, uint8_t deleted) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            this->m_friends[i].m_deleted |= deleted;
            return;
        }
    }
}

// VERIFY: 736677c9 auto
// OFFSET: 0x006B3A30
void FriendList::SortIgnoreList() {
    qsort(this->m_ignores, 50, sizeof(IgnoreInfo), QSortIgnore);
    FrameScript_SignalEvent(SCRIPT_EVENT_IGNORELIST_UPDATE, nullptr);
}

// VERIFY: 3ccfe601 auto
// OFFSET: 0x006B3B60
void FriendList::SortMuteList() {
    qsort(this->m_mutes, 50, sizeof(IgnoreInfo), QSortIgnore);
    FrameScript_SignalEvent(SCRIPT_EVENT_MUTELIST_UPDATE, nullptr);
}

// VERIFY: b1ba3a48 manual
// OFFSET: 0x006B3B90
void FriendList::FriendRemoved(WOWGUID guid) {
    for (uint32_t i = 0; i < 100; i++) {
        FriendInfo* info = &this->m_friends[i];

        if (info->m_guid == guid) {
            info->m_guid = WOWGUID();
            info->m_deleted = 0;

            if (info->m_name) {
                SMemFree(info->m_name, __FILE__, __LINE__, 0);
            }

            info->m_name = nullptr;
            return;
        }
    }
}

// VERIFY: 49427bf9 auto
// OFFSET: 0x006B3C10
uint32_t FriendList::GetFriendIndexByGuid(WOWGUID guid) {
    int32_t empty = -1;
    uint32_t index;

    for (index = 0; index < 100; index++) {
        if (this->m_friends[index].m_guid == guid) {
            return index;
        }

        if (!this->m_friends[index].m_guid.m_guid) {
            empty = index;
            break;
        }
    }

    FriendInfo* info = &this->m_friends[empty];
    info->m_guid = guid;
    info->m_connected = 0;
    info->m_deleted = 0;
    return empty;
}

// VERIFY: 880391d6 manual
// OFFSET: 0x006B3C90
void FriendList::SetName(WOWGUID guid, const char* name) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            if (this->m_friends[i].m_name) {
                SMemFree(this->m_friends[i].m_name, __FILE__, __LINE__, 0);
            }

            this->m_friends[i].m_name = SStrDupA(name, __FILE__, __LINE__);
            return;
        }
    }
}

// VERIFY: 3ca076e7 auto
// OFFSET: 0x006B3D10
void FriendList::SetConnected(WOWGUID guid, uint8_t connected) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            this->m_friends[i].m_connected = connected;

            if (connected) {
                CGAutoCompleteName::AddFlags(guid, 0x20, 0, nullptr);
            } else {
                CGAutoCompleteName::RemoveFlags(guid, 0x20);
            }

            return;
        }
    }
}

// VERIFY: 9e24e65d manual
// OFFSET: 0x006B3D90
void FriendList::DecrementPendingFriendName() {
    uint32_t pending = this->m_pendingFriendNames;

    if (pending) {
        this->m_pendingFriendNames = pending - 1;

        if (pending == 1) {
            qsort(this->m_friends, 100, sizeof(FriendInfo), QSortFriends);
            FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);
        }
    }
}

// VERIFY: eb501484 manual
// OFFSET: 0x006B3DD0
void FriendList::DecrementPendingIgnoreName() {
    uint32_t pending = this->m_pendingIgnoreNames;

    if (pending) {
        this->m_pendingIgnoreNames = pending - 1;

        if (pending == 1) {
            qsort(this->m_ignores, 50, sizeof(IgnoreInfo), QSortIgnore);
            FrameScript_SignalEvent(SCRIPT_EVENT_IGNORELIST_UPDATE, nullptr);
        }
    }
}

// VERIFY: 69f932ed manual
// OFFSET: 0x006B3E10
void FriendList::DecrementPendingMuteName() {
    uint32_t pending = this->m_pendingMuteNames;

    if (pending) {
        this->m_pendingMuteNames = pending - 1;

        if (pending == 1) {
            qsort(this->m_mutes, 50, sizeof(IgnoreInfo), QSortIgnore);
            FrameScript_SignalEvent(SCRIPT_EVENT_MUTELIST_UPDATE, nullptr);
        }
    }
}

// VERIFY: 72bf46c4 manual
// OFFSET: 0x006B3E50
bool FriendList::GetIgnoreDeleted(WOWGUID guid) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_ignores[i].m_guid == guid) {
            return this->m_ignores[i].m_flags;
        }
    }

    return false;
}

// VERIFY: 0026cef4 manual
// OFFSET: 0x006B3EA0
void FriendList::SetIgnoreDeleted(WOWGUID guid, uint8_t deleted) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_ignores[i].m_guid == guid) {
            this->m_ignores[i].m_flags |= deleted;
            return;
        }
    }
}

// VERIFY: ec2c3074 manual
// OFFSET: 0x006B3EF0
bool FriendList::GetMuteDeleted(WOWGUID guid) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_mutes[i].m_guid == guid) {
            return this->m_mutes[i].m_flags;
        }
    }

    return false;
}

// VERIFY: f36c792f manual
// OFFSET: 0x006B3F40
void FriendList::SetMuteDeleted(WOWGUID guid, uint8_t deleted) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_mutes[i].m_guid == guid) {
            this->m_mutes[i].m_flags |= deleted;
            return;
        }
    }
}

// VERIFY: 96e1ed5b manual
// OFFSET: 0x006B54C0
void __stdcall FriendList::RemoveFriend(WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DEL_FRIEND));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 0f7b89ae manual
// OFFSET: 0x006B60B0
void __stdcall FriendList::DelIgnore(WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DEL_IGNORE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 381eafe3 manual
// OFFSET: 0x006B6410
void __stdcall FriendList::DelVoiceIgnore(WOWGUID guid) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_VOICE_DEL_IGNORE));
    msg.Put(guid.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 92b2143c manual
// OFFSET: 0x006B6490
void FriendList::IgnoreRemoved(WOWGUID guid) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_ignores[i].m_guid == guid) {
            this->m_ignores[i].m_guid = WOWGUID();
            this->m_ignores[i].m_flags = 0;
            CGCalendar::UpdateDisplayList(1);
            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);

            if (!this->m_pendingIgnoreNames) {
                qsort(this->m_ignores, 50, sizeof(IgnoreInfo), QSortIgnore);
                FrameScript_SignalEvent(SCRIPT_EVENT_IGNORELIST_UPDATE, nullptr);
            }

            break;
        }
    }

    if (!this->IsMuted(guid)) {
        VoiceSessionSetTalkerMuted(guid, 1);
    }
}

// VERIFY: 130812de manual
// OFFSET: 0x006B6540
void FriendList::MuteRemoved(WOWGUID guid) {
    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_mutes[i].m_guid == guid) {
            this->m_mutes[i].m_guid = WOWGUID();
            this->m_mutes[i].m_flags = 0;

            if (!this->m_pendingMuteNames) {
                qsort(this->m_mutes, 50, sizeof(IgnoreInfo), QSortIgnore);
                FrameScript_SignalEvent(SCRIPT_EVENT_MUTELIST_UPDATE, nullptr);
            }

            break;
        }
    }

    if (guid) {
        for (uint32_t i = 0; i < 50; i++) {
            if (this->m_ignores[i].m_guid == guid) {
                return;
            }
        }

        if (CGChat::m_complaintsSystemStatus == 2) {
            for (uint32_t i = 0; i < 32; i++) {
                if (this->m_complained[i] == guid.m_guid) {
                    return;
                }
            }
        }
    }

    VoiceSessionSetTalkerMuted(guid, 1);
}

// VERIFY: a1b41c36 auto
// OFFSET: 0x006B6630
void FriendListNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    UIErrors error = static_cast<UIErrors>(reinterpret_cast<uintptr_t>(param));

    if (!success) {
        if (error != ERR_MAX_VALUE) {
            CGGameUI::DisplayError(ERR_FRIEND_ERROR);
        }

        if (FriendList::s_friendList) {
            FriendList::s_friendList->SetFriendDeleted(guid, 1);
            FriendList::RemoveFriend(guid);
        }

        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!record) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char name[308];

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    CGAutoCompleteName::SetName(guid, name, false);

    if (FriendList::s_friendList) {
        FriendList::s_friendList->SetName(guid, name);
    }

    if (error != ERR_MAX_VALUE) {
        CGGameUI::DisplayError(error, name, name);
    }
}

// VERIFY: 51072989 auto
// OFFSET: 0x006B6760
void FriendListNameCallbackWithSort(int32_t id, const WOWGUID& guid, void* param, bool success) {
    UIErrors error = static_cast<UIErrors>(reinterpret_cast<uintptr_t>(param));

    if (!success) {
        if (error != ERR_MAX_VALUE) {
            CGGameUI::DisplayError(ERR_FRIEND_ERROR);
        }

        if (FriendList::s_friendList) {
            FriendList::s_friendList->DecrementPendingFriendName();
            FriendList::s_friendList->SetFriendDeleted(guid, 1);
            FriendList::RemoveFriend(guid);
        }

        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!record) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char name[308];

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    CGAutoCompleteName::SetName(guid, name, false);

    if (FriendList::s_friendList) {
        FriendList::s_friendList->SetName(guid, name);
        FriendList::s_friendList->DecrementPendingFriendName();
    }

    if (error != ERR_MAX_VALUE) {
        CGGameUI::DisplayError(error, name, name);
    }
}

// VERIFY: c79746f8 auto
// OFFSET: 0x006B68B0
void FriendListIgnoreNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    UIErrors error = static_cast<UIErrors>(reinterpret_cast<uintptr_t>(param));

    if (!success) {
        if (error != ERR_MAX_VALUE) {
            CGGameUI::DisplayError(ERR_FRIEND_ERROR);
        }

        if (FriendList::s_friendList) {
            FriendList::s_friendList->SetIgnoreDeleted(guid, 1);
            FriendList::DelIgnore(guid);
            FriendList::s_friendList->DecrementPendingIgnoreName();
        }

        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!record) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char name[308];

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    if (FriendList::s_friendList) {
        FriendList::s_friendList->DecrementPendingIgnoreName();
    }

    if (error != ERR_MAX_VALUE) {
        CGGameUI::DisplayError(error, name, name);
    }
}

// VERIFY: 5e238ee6 auto
// OFFSET: 0x006B69D0
void FriendListMuteNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    UIErrors error = static_cast<UIErrors>(reinterpret_cast<uintptr_t>(param));

    if (!success) {
        if (error != ERR_MAX_VALUE) {
            CGGameUI::DisplayError(ERR_FRIEND_ERROR);
        }

        if (FriendList::s_friendList) {
            FriendList::s_friendList->SetMuteDeleted(guid, 1);
            FriendList::DelVoiceIgnore(guid);
            FriendList::s_friendList->DecrementPendingMuteName();
        }

        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* record = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (!record) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    char name[308];

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    if (FriendList::s_friendList) {
        FriendList::s_friendList->DecrementPendingMuteName();
    }

    if (error != ERR_MAX_VALUE) {
        CGGameUI::DisplayError(error, name, name);
    }
}

// VERIFY: 68690c6c manual
// OFFSET: 0x006B7200
void FriendList::IgnoreAdded(WOWGUID guid, int32_t lookupName) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!this->m_ignores[i].m_guid) {
            this->m_ignores[i].m_guid = guid;
            this->m_ignores[i].m_flags = 0;
            CGCalendar::UpdateDisplayList(1);
            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);

            if (lookupName) {
                if (!g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListIgnoreNameCallback, reinterpret_cast<void*>(ERR_MAX_VALUE), false)) {
                    this->m_pendingIgnoreNames++;
                }

                if (!this->m_pendingIgnoreNames) {
                    qsort(this->m_ignores, 50, sizeof(IgnoreInfo), QSortIgnore);
                    FrameScript_SignalEvent(SCRIPT_EVENT_IGNORELIST_UPDATE, nullptr);
                }
            }

            break;
        }
    }

    VoiceSessionSetTalkerMuted(guid, 0);
}

// VERIFY: 0e14c72a manual
// OFFSET: 0x006B72E0
void FriendList::MuteAdded(WOWGUID guid, int32_t lookupName) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!this->m_mutes[i].m_guid) {
            this->m_mutes[i].m_guid = guid;
            this->m_mutes[i].m_flags = 0;

            if (lookupName) {
                if (!g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListMuteNameCallback, reinterpret_cast<void*>(ERR_MAX_VALUE), false)) {
                    this->m_pendingMuteNames++;
                }

                if (!this->m_pendingMuteNames) {
                    qsort(this->m_mutes, 50, sizeof(IgnoreInfo), QSortIgnore);
                    FrameScript_SignalEvent(SCRIPT_EVENT_MUTELIST_UPDATE, nullptr);
                }
            }

            break;
        }
    }

    VoiceSessionSetTalkerMuted(guid, 0);
}

// VERIFY: 730b124b manual
// OFFSET: 0x006B73A0
void FriendList::HandleStatus(uint8_t status, WOWGUID guid, CDataStore* msg) {
    UIErrors error = ERR_MAX_VALUE;
    bool friendChange = false;
    bool ignoreChange = false;
    bool muteChange = false;
    bool display;
    NameCache* record;

    switch (status) {
        case 0:
        case 4:
            CGGameUI::DisplayError(ERR_FRIEND_NOT_FOUND);
            return;

        case 1:
            CGGameUI::DisplayError(ERR_FRIEND_LIST_FULL);
            return;

        case 2: {
            error = ERR_FRIEND_ONLINE_SS;
            uint8_t flags;
            uint32_t area;
            uint32_t level;
            uint32_t classId;
            msg->Get(flags);
            msg->Get(area);
            msg->Get(level);
            msg->Get(classId);

            for (uint32_t i = 0; i < 100; i++) {
                if (this->m_friends[i].m_guid == guid) {
                    this->m_friends[i].m_flags = flags;
                    this->m_friends[i].m_area = area;
                    this->m_friends[i].m_level = level;
                    this->m_friends[i].m_class = classId;
                    break;
                }
            }

            this->SetConnected(guid, 1);
            friendChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_FRIEND_ONLINE_SS), false);
            break;
        }

        case 3:
            error = ERR_FRIEND_OFFLINE_S;
            this->SetConnected(guid, 0);
            friendChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_FRIEND_OFFLINE_S), false);
            break;

        case 5: {
            bool deleted = this->GetFriendDeleted(guid);
            display = !deleted;
            error = static_cast<UIErrors>(4 * deleted + ERR_FRIEND_REMOVED_S);
            this->FriendRemoved(guid);
            CGAutoCompleteName::RemoveFlags(guid, 0x4);
            friendChange = true;
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (unit) {
                unit->UpdateUnitNameColor();
            }

            goto lookup;
        }

        case 6: {
            error = ERR_FRIEND_ADDED_S;
            char note[512];
            uint8_t flags;
            uint32_t area;
            uint32_t level;
            uint32_t classId;
            msg->GetString(note, sizeof(note));
            msg->Get(flags);
            msg->Get(area);
            msg->Get(level);
            msg->Get(classId);
            int32_t index = this->GetFriendIndexByGuid(guid);

            if (index >= 0) {
                FriendInfo* info = &this->m_friends[index];
                SStrCopy(info->m_notes, note, sizeof(info->m_notes));
                info->m_flags = flags;
                info->m_area = area;
                info->m_level = level;
                info->m_class = classId;
            }

            CGAutoCompleteName::AddFlags(guid, 0x4, 0, nullptr);
            this->SetConnected(guid, 1);
            friendChange = true;
            CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_UNIT, __FILE__, __LINE__));

            if (!unit) {
                record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(error), false);
                break;
            }

            unit->UpdateUnitNameColor();
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_FRIEND_ADDED_S), false);
            break;
        }

        case 7: {
            error = ERR_FRIEND_ADDED_S;
            char note[512];
            msg->GetString(note, sizeof(note));
            int32_t index = this->GetFriendIndexByGuid(guid);

            if (index >= 0) {
                SStrCopy(this->m_friends[index].m_notes, note, sizeof(this->m_friends[index].m_notes));
            }

            CGAutoCompleteName::AddFlags(guid, 0x4, 0, nullptr);
            this->SetConnected(guid, 0);
            friendChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_FRIEND_ADDED_S), false);
            break;
        }

        case 8:
            error = ERR_FRIEND_ALREADY_S;
            this->GetFriendIndexByGuid(guid);
            friendChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(ERR_FRIEND_ALREADY_S), false);
            break;

        case 9:
            CGGameUI::DisplayError(ERR_FRIEND_SELF);
            return;

        case 10:
            CGGameUI::DisplayError(ERR_FRIEND_WRONG_FACTION);
            return;

        case 11:
            CGGameUI::DisplayError(ERR_IGNORE_FULL);
            return;

        case 12:
            CGGameUI::DisplayError(ERR_IGNORE_SELF);
            return;

        case 13:
            CGGameUI::DisplayError(ERR_IGNORE_NOT_FOUND);
            return;

        case 14:
            error = ERR_IGNORE_ALREADY_S;
            ignoreChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListIgnoreNameCallback, reinterpret_cast<void*>(ERR_IGNORE_ALREADY_S), false);
            break;

        case 15:
            error = ERR_IGNORE_ADDED_S;
            this->IgnoreAdded(guid, 1);
            ignoreChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListIgnoreNameCallback, reinterpret_cast<void*>(ERR_IGNORE_ADDED_S), false);
            break;

        case 16: {
            bool deleted = this->GetIgnoreDeleted(guid);
            display = !deleted;
            error = static_cast<UIErrors>(2 * deleted + ERR_IGNORE_REMOVED_S);
            this->IgnoreRemoved(guid);
            ignoreChange = true;
            goto lookup;
        }

        case 17:
            CGGameUI::DisplayError(ERR_IGNORE_AMBIGUOUS);
            return;

        case 18:
            CGGameUI::DisplayError(ERR_VOICE_IGNORE_FULL);
            return;

        case 19:
            CGGameUI::DisplayError(ERR_VOICE_IGNORE_SELF);
            return;

        case 20:
            CGGameUI::DisplayError(ERR_VOICE_IGNORE_NOT_FOUND);
            return;

        case 21:
            error = ERR_VOICE_IGNORE_ALREADY_S;
            muteChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListMuteNameCallback, reinterpret_cast<void*>(ERR_VOICE_IGNORE_ALREADY_S), false);
            break;

        case 22:
            error = ERR_VOICE_IGNORE_ADDED_S;
            this->MuteAdded(guid, 1);
            muteChange = true;
            record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListMuteNameCallback, reinterpret_cast<void*>(ERR_VOICE_IGNORE_ADDED_S), false);
            break;

        case 23: {
            bool deleted = this->GetMuteDeleted(guid);
            display = !deleted;
            error = static_cast<UIErrors>(2 * deleted + ERR_VOICE_IGNORE_REMOVED_S);
            this->MuteRemoved(guid);
            muteChange = true;
            goto lookup;
        }

        case 24:
            CGGameUI::DisplayError(ERR_VOICE_IGNORE_AMBIGUOUS);
            return;

        case 25:
            CGGameUI::DisplayError(error);
            return;

        case 26: {
            uint8_t flags;
            msg->Get(flags);

            for (uint32_t i = 0; i < 100; i++) {
                if (this->m_friends[i].m_guid == guid) {
                    this->m_friends[i].m_flags = flags;
                    break;
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);
            CGGameUI::DisplayError(error);
            return;
        }

        case 27: {
            uint32_t area;
            msg->Get(area);

            for (uint32_t i = 0; i < 100; i++) {
                if (this->m_friends[i].m_guid == guid) {
                    this->m_friends[i].m_area = area;
                    break;
                }
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);
            CGGameUI::DisplayError(error);
            return;
        }

        default:
            CGGameUI::DisplayError(ERR_FRIEND_ERROR);
            return;
    }

    goto resolved;

lookup:
    if (!display) {
        CGGameUI::DisplayError(error);
        return;
    }

    if (muteChange) {
        record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListMuteNameCallback, reinterpret_cast<void*>(error), false);
    } else if (ignoreChange) {
        record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListIgnoreNameCallback, reinterpret_cast<void*>(error), false);
    } else if (friendChange) {
        record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallbackWithSort, reinterpret_cast<void*>(error), false);
    } else {
        record = g_nameDBCache.GetRecord(guid.m_guid, guid, FriendListNameCallback, reinterpret_cast<void*>(error), false);
    }

resolved:
    if (!record) {
        if (friendChange) {
            this->m_pendingFriendNames++;
        } else if (ignoreChange) {
            this->m_pendingIgnoreNames++;
        } else if (muteChange) {
            this->m_pendingMuteNames++;
        }

        return;
    }

    char name[308];

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    if (record->m_realmName[0]) {
        SStrPrintf(name, 305, "%s-%s", record->m_name, record->m_realmName);
    } else {
        SStrCopy(name, record->m_name, 305);
    }

    CGAutoCompleteName::SetName(guid, name, false);

    if (error == ERR_FRIEND_ADDED_S) {
        this->SetName(guid, name);
    }

    if (friendChange) {
        qsort(this->m_friends, 100, sizeof(FriendInfo), QSortFriends);
        FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);
    } else if (ignoreChange) {
        this->SortIgnoreList();
    } else if (muteChange) {
        this->SortMuteList();
    }

    CGGameUI::DisplayError(error, name, name);
}

// VERIFY: c080ab2c auto
// OFFSET: 0x006B86B0
int32_t Packet_SMSG_FRIEND_STATUS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t status;
    WOWGUID guid;
    msg->Get(status);
    msg->Get(guid.m_guid);

    if (FriendList::s_friendList) {
        FriendList::s_friendList->HandleStatus(status, guid, msg);
    }

    return 1;
}

// VERIFY: 762d6418 manual
// OFFSET: 0x006B3920
bool FriendList::IsFriend(WOWGUID guid) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            return true;
        }
    }

    return false;
}

// VERIFY: 72aa79dc auto
// OFFSET: 0x006B3280
int32_t Packet_SMSG_WHOIS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    char text[256];
    msg->GetString(text, sizeof(text));
    ConsoleWrite(text, DEFAULT_COLOR);
    return 1;
}

// VERIFY: cd698c9b manual
// OFFSET: 0x006B32C0
int32_t Packet_SMSG_RWHOIS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t accounts = 0;
    msg->Get(accounts);

    if (accounts == 0xFFFFFFFF) {
        ConsoleWriteA("RWhoIs failed\n", ERROR_COLOR);
        return 1;
    }

    if (!accounts) {
        ConsoleWriteA("Not found\n", WARNING_COLOR);
        return 1;
    }

    for (int32_t i = 0; i < static_cast<int32_t>(accounts); i++) {
        char account[1280];
        msg->GetString(account, sizeof(account));
        ConsolePrintf("Account: %s\n", account);
        uint32_t characters = 0;
        msg->Get(characters);
        uint32_t current = 0;
        msg->Get(current);

        for (int32_t j = 0; j < static_cast<int32_t>(characters); j++) {
            char name[48];
            msg->GetString(name, sizeof(name));
            ConsoleWriteA("  %s\n", j != static_cast<int32_t>(current) ? DEFAULT_COLOR : WARNING_COLOR, name);
        }
    }

    return 1;
}

// VERIFY: 775234b1 manual
// OFFSET: 0x006B3570
FriendInfo* FriendList::GetFriendInfoByName(const char* name) {
    if (!name) {
        return nullptr;
    }

    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_name && !SStrCmpUTF8I(this->m_friends[i].m_name, name, STORM_MAX_STR)) {
            return &this->m_friends[i];
        }
    }

    return nullptr;
}

// VERIFY: e93b4d10 manual
// OFFSET: 0x006B35E0
int32_t FriendList::GetFriendSelectionIndex() {
    if (!this->m_selectedFriend) {
        return -1;
    }

    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == this->m_selectedFriend) {
            return i;
        }
    }

    return -1;
}

// VERIFY: 7765f6be auto
// OFFSET: 0x006B3680
uint32_t FriendList::GetNumMutes() {
    uint32_t count = 0;

    while (count < 50 && this->m_mutes[count].m_guid) {
        count++;
    }

    return count;
}

// VERIFY: 7cdd7e91 manual
// OFFSET: 0x006B36D0
uint64_t FriendList::GetIgnore(uint32_t index) {
    if (index > 50) {
        return 0;
    }

    return this->m_ignores[index].m_guid.m_guid;
}

// VERIFY: c2df352a manual
// OFFSET: 0x006B3800
int32_t FriendList::GetIgnoreSelectionIndex() {
    if (!this->m_selectedIgnore) {
        return -1;
    }

    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_ignores[i].m_guid == this->m_selectedIgnore) {
            return i;
        }
    }

    return -1;
}

// VERIFY: e4ca9a3b manual
// OFFSET: 0x006B3840
int32_t FriendList::GetMuteSelectionIndex() {
    if (!this->m_selectedMute) {
        return -1;
    }

    for (uint32_t i = 0; i < 50; i++) {
        if (this->m_mutes[i].m_guid == this->m_selectedMute) {
            return i;
        }
    }

    return -1;
}

// VERIFY: bbb66701 manual
// OFFSET: 0x006B3FE0
int32_t CCommand_Whois(const char* command, const char* arguments) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_WHO_IS));
    msg.PutString(arguments);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 1;
}

// VERIFY: a2847f86 manual
// OFFSET: 0x006B5220
void FriendListModuleLoaded() {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_BOT_DETECTED2));
    msg.Put(static_cast<uint32_t>(0x2BD76673));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1f5c9d24 manual
// OFFSET: 0x006B5540
void FriendList::SetNotes(WOWGUID guid, const char* notes) {
    uint32_t i;

    for (i = 0; i < 100; i++) {
        if (this->m_friends[i].m_guid == guid) {
            break;
        }
    }

    if (i >= 100) {
        return;
    }

    char* text = this->m_friends[i].m_notes;
    SStrCopy(text, notes, sizeof(this->m_friends[i].m_notes));
    FrameScript_SignalEvent(SCRIPT_EVENT_FRIENDLIST_UPDATE, nullptr);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_CONTACT_NOTES));
    msg.Put(guid.m_guid);
    msg.PutString(text);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: c5c51c69 manual
// OFFSET: 0x006B5DE0
void FriendList::AddOrDelIgnore(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_ignores[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_ignores[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_DEL_IGNORE));
            msg.Put(this->m_ignores[i].m_guid.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
            return;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ADD_IGNORE));
    msg.PutString(name);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: e1657bfc manual
// OFFSET: 0x006B5F70
void FriendList::AddIgnore(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_ignores[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_ignores[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            CGGameUI::DisplayError(ERR_IGNORE_ALREADY_S, record->m_name);
            return;
        }
    }

    char text[256];
    SStrCopy(text, name, sizeof(text));
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ADD_IGNORE));
    msg.PutString(text);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 6e20d65d manual
// OFFSET: 0x006B6130
void FriendList::AddOrDelVoiceIgnore(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_mutes[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_mutes[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            CDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_VOICE_DEL_IGNORE));
            msg.Put(this->m_mutes[i].m_guid.m_guid);
            msg.Finalize();
            ClientServices::Send2(&msg);
            return;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_VOICE_ADD_IGNORE));
    msg.PutString(name);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: fa35cca2 manual
// OFFSET: 0x006B62C0
void FriendList::AddVoiceIgnore(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_mutes[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_mutes[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            CGGameUI::DisplayError(ERR_VOICE_IGNORE_ALREADY_S, record->m_name);
            return;
        }
    }

    char text[305];
    SStrCopy(text, name, sizeof(text));
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_VOICE_ADD_IGNORE));
    msg.PutString(text);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: b6a1d181 manual
// OFFSET: 0x006B6E10
void FriendList::AddOrRemoveFriend(const char* name, const char* notes) {
    for (uint32_t i = 0; i < 100; i++) {
        if (!static_cast<uint32_t>(this->m_friends[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_friends[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            FriendList::RemoveFriend(this->m_friends[i].m_guid);
            return;
        }
    }

    FriendList::AddFriend(name, notes);
}

// VERIFY: 24072892 manual
// OFFSET: 0x006B6F00
void FriendList::RemoveFriend(const char* name) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_name && !SStrCmpUTF8I(this->m_friends[i].m_name, name, STORM_MAX_STR)) {
            FriendList::RemoveFriend(this->m_friends[i].m_guid);
            return;
        }
    }

    CGGameUI::DisplayError(ERR_FRIEND_NOT_FOUND);
}

// VERIFY: 835f4662 manual
// OFFSET: 0x006B6F70
void FriendList::SetFriendNotes(const char* name, const char* notes) {
    for (uint32_t i = 0; i < 100; i++) {
        if (this->m_friends[i].m_name && !SStrCmpUTF8I(this->m_friends[i].m_name, name, STORM_MAX_STR)) {
            this->SetNotes(this->m_friends[i].m_guid, notes);
            return;
        }
    }

    CGGameUI::DisplayError(ERR_FRIEND_NOT_FOUND);
}

// VERIFY: e018fa92 manual
// OFFSET: 0x006B7040
void FriendList::DelIgnore(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_ignores[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_ignores[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            FriendList::DelIgnore(this->m_ignores[i].m_guid);
            return;
        }
    }

    CGGameUI::DisplayError(ERR_IGNORE_NOT_FOUND);
}

// VERIFY: d0866dbb manual
// OFFSET: 0x006B7120
void FriendList::DelMute(const char* name) {
    for (uint32_t i = 0; i < 50; i++) {
        if (!static_cast<uint32_t>(this->m_mutes[i].m_guid.m_guid)) {
            continue;
        }

        WOWGUID none = WOWGUID();
        NameCache* record = g_nameDBCache.GetRecord(this->m_mutes[i].m_guid.m_guid, none, nullptr, nullptr, false);

        if (!record) {
            continue;
        }

        char fullName[305];

        if (record->m_realmName[0]) {
            SStrPrintf(fullName, sizeof(fullName), "%s-%s", record->m_name, record->m_realmName);
        } else {
            SStrCopy(fullName, record->m_name, sizeof(fullName));
        }

        if (!SStrCmpUTF8I(fullName, name, STORM_MAX_STR)) {
            FriendList::DelVoiceIgnore(this->m_mutes[i].m_guid);
            return;
        }
    }

    CGGameUI::DisplayError(ERR_VOICE_IGNORE_NOT_FOUND);
}
