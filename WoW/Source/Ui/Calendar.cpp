#include "Calendar.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/HolidayNamesRec.h"
#include "WoW/Source/DB/Rec/HolidaysRec.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/Rec/FactionTemplateRec.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "WoW/Source/Ui/CalendarEvent.h"
#include "WoW/Common/WowTime.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/DB/Rec/HolidayDescriptionsRec.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/Object.h"
#include "WoW/Source/Object/Player.h"
#include "Engine/Source/Os/TimeManager.h"
#include <malloc.h>

// STATIC: 0x00AD0340
static TSList<CalendarPendingInvite, TSGetLink<CalendarPendingInvite> > s_pendingInvites;

// STATIC: 0x00AD0350
CalendarEventData s_eventClipboard;

// STATIC: 0x00AD0328
static TSList<CALENDARHOLIDAYNODE, TSGetLink<CALENDARHOLIDAYNODE> > s_holidays;

// STATIC: 0x00AD0334
static TSList<CALENDARRAIDRESETNODE, TSGetLink<CALENDARRAIDRESETNODE> > s_raidResets;

// STATIC: 0x00C20808
static TSGrowableArray<CalendarDayEvent*> s_days[3][31];

// STATIC: 0x00C20DD8
static CalendarEventTables s_eventTables;

// STATIC: 0x00C20F38
TSHashTable<DYNAMICHOLIDAYHASH, HASHKEY_NONE> s_dynamicHolidays;

// STATIC: 0x00C20F60
TSFixedArray<int32_t> s_eventTextures[2];

// STATIC: 0x00C207EC
CGCalendarEvent* CGCalendar::m_curEvent;

// STATIC: 0x00C207F0
uint8_t CGCalendar::m_initialize;

// STATIC: 0x00C207F1
uint8_t CGCalendar::m_openedCalendar;

// STATIC: 0x00C207F2
uint8_t CGCalendar::m_calendarReceived;

// STATIC: 0x00C207E8
CalendarDayEvent* CGCalendar::m_contextEventInfo;

// STATIC: 0x00C207FC
uint32_t CGCalendar::m_lastEventTime;

// STATIC: 0x00C207F4
int32_t CGCalendar::m_numInitialPending;

// STATIC: 0x00C207F3
uint8_t CGCalendar::m_actionPending;

// VERIFY: 339c2c98 auto
// OFFSET: 0x005C4240
void CGCalendar::InitializeGame() {
    CGCalendar::m_curEvent = nullptr;
    CGCalendar::m_initialize = 1;
    CGCalendar::m_openedCalendar = 0;
    CGCalendar::m_calendarReceived = 0;
    CGCalendar::m_numInitialPending = 0;

    ClientServices::SetMessageHandler(SMSG_CALENDAR_SEND_CALENDAR, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_SEND_EVENT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_REMOVED, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_STATUS, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_NOTES, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_FILTER_GUILD, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_ARENA_TEAM, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_ADDED, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_REMOVED, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_UPDATED, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_REMOVED_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_UPDATED_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_SEND_NUM_PENDING, PH_CALENDAR_GROUP, nullptr);
    ClientServices::SetMessageHandler(SMSG_CALENDAR_CLEAR_PENDING_ACTION, PH_CALENDAR_GROUP, nullptr);
}

// VERIFY: d8756e5f auto
// OFFSET: 0x005B9430
const char* GetHolidayName(int32_t holidayId) {
    HolidaysRec* holiday = g_holidaysDB.GetRecord(holidayId);

    if (!holiday) {
        return nullptr;
    }

    HolidayNamesRec* name = g_holidayNamesDB.GetRecord(holiday->m_holidayNameID);
    return name ? name->m_name : nullptr;
}

// STATIC: 0x00ACFFE4
static const char* s_fullDateMonthNames[12] = { "FULLDATE_MONTH_JANUARY", "FULLDATE_MONTH_FEBRUARY", "FULLDATE_MONTH_MARCH", "FULLDATE_MONTH_APRIL", "FULLDATE_MONTH_MAY", "FULLDATE_MONTH_JUNE", "FULLDATE_MONTH_JULY", "FULLDATE_MONTH_AUGUST", "FULLDATE_MONTH_SEPTEMBER", "FULLDATE_MONTH_OCTOBER", "FULLDATE_MONTH_NOVEMBER", "FULLDATE_MONTH_DECEMBER" };

// VERIFY: f2885c19 manual
// OFFSET: 0x005B7E50
size_t CGCalendar::GetDateTimeText(char* buffer, uint32_t size, uint32_t time) {
    WowTime date;
    WowTime::WowDecodeTime(time, &date);
    char dateText[256] = { 0 };

    if (date.m_year >= 0 && date.m_month >= 0 && date.m_monthDay >= 0 && date.m_weekDay >= 0) {
        SStrPrintf(dateText, sizeof(dateText), FrameScript_GetText("FULLDATE", -1, GENDER_NOT_APPLICABLE), FrameScript_GetText(s_weekdayNames[date.m_weekDay], -1, GENDER_NOT_APPLICABLE), FrameScript_GetText(s_fullDateMonthNames[date.m_month], -1, GENDER_NOT_APPLICABLE), date.m_monthDay + 1, date.m_year + 2000, date.m_month + 1);
    }

    char timeText[64] = { 0 };
    int32_t hour = date.m_hour;

    if (hour >= 0) {
        int32_t minute = date.m_minute;

        if (minute >= 0) {
            if (CVar::Lookup("timeMgrUseMilitaryTime")->m_intValue) {
                SStrPrintf(timeText, sizeof(timeText), FrameScript_GetText("TIME_TWENTYFOURHOURS", -1, GENDER_NOT_APPLICABLE), hour, minute);
            } else {
                const char* token = "TIME_TWELVEHOURAM";

                if (!hour) {
                    hour = 12;
                } else if (hour == 12) {
                    token = "TIME_TWELVEHOURPM";
                } else if (hour > 12) {
                    hour -= 12;
                    token = "TIME_TWELVEHOURPM";
                }

                SStrPrintf(timeText, sizeof(timeText), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), hour, minute);
            }
        }
    }

    return SStrPrintf(buffer, size, FrameScript_GetText("FULLDATE_AND_TIME", -1, GENDER_NOT_APPLICABLE), dateText, timeText);
}

// VERIFY: 0d10e0e8 auto
// OFFSET: 0x005B8E50
static int32_t QSortTextures(const void* a, const void* b) {
    LfgDungeonsRec* recA = g_lfgDungeonsDB.GetRecord(*static_cast<const int32_t*>(a));
    LfgDungeonsRec* recB = g_lfgDungeonsDB.GetRecord(*static_cast<const int32_t*>(b));
    int32_t result = recB->m_expansionLevel - recA->m_expansionLevel;

    if (!result) {
        return SStrSortUTF8I(recA->m_name, recB->m_name, STORM_MAX_STR);
    }

    return result;
}

// VERIFY: 14fb4767 auto
// OFFSET: 0x005B9960
void CGCalendar::CalendarError(const char* error, uintptr_t arg) {
    char buffer[3000];

    if (arg) {
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText(error, -1, GENDER_NOT_APPLICABLE), arg);
    } else {
        SStrPrintf(buffer, sizeof(buffer), FrameScript_GetText(error, -1, GENDER_NOT_APPLICABLE));
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_ERROR, "%s", buffer);
    CGCalendar::m_actionPending = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 0);
}

// VERIFY: de197260 manual
// OFFSET: 0x005BBBB0
void CGCalendar::QueryPendingInvites() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_GET_NUM_PENDING));
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 9ea81fd7 manual
// OFFSET: 0x005BE2A0
void CGCalendar::EnterWorld() {
    CGCalendar::m_actionPending = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 0);

    if (!CGCalendar::m_initialize) {
        return;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    memset(&s_eventClipboard, 0, sizeof(s_eventClipboard));
    CGCalendar::QueryPendingInvites();

    uint32_t counts[2] = { 0, 0 };
    int32_t numRecords = g_lfgDungeonsDB.m_numRecords;

    for (int32_t i = 0; i < numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (!rec) {
            continue;
        }

        if (rec->m_faction >= 0) {
            FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(player->m_unit->m_unitFieldFactiontemplate);
            int32_t group = (faction->m_factionGroup & 0x4) ? 0 : ((faction->m_factionGroup & 0x2) ? 1 : -1);

            if (group != rec->m_faction) {
                continue;
            }
        }

        if (rec->m_typeID == 1) {
            counts[0]++;
        } else if (rec->m_typeID == 2) {
            counts[1]++;
        }
    }

    for (uint32_t i = 0; i < 2; i++) {
        s_eventTextures[i].SetCount(counts[i]);
    }

    counts[0] = 0;
    counts[1] = 0;

    for (int32_t i = 0; i < numRecords; i++) {
        LfgDungeonsRec* rec = g_lfgDungeonsDB.GetRecordByIndex(i);

        if (!rec) {
            continue;
        }

        if (rec->m_faction >= 0) {
            FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(player->m_unit->m_unitFieldFactiontemplate);
            int32_t group = (faction->m_factionGroup & 0x4) ? 0 : ((faction->m_factionGroup & 0x2) ? 1 : -1);

            if (group != rec->m_faction) {
                continue;
            }
        }

        if (rec->m_typeID == 1) {
            s_eventTextures[0].m_data[counts[0]++] = rec->m_id;
        } else if (rec->m_typeID == 2) {
            s_eventTextures[1].m_data[counts[1]++] = rec->m_id;
        }
    }

    qsort(s_eventTextures[0].m_data, counts[0], sizeof(int32_t), QSortTextures);
    qsort(s_eventTextures[1].m_data, counts[1], sizeof(int32_t), QSortTextures);
    CGCalendar::m_initialize = 0;
}

// VERIFY: 963a0c94 manual
// OFFSET: 0x005BEC70
uint32_t CGCalendar::CountPendingInvites() {
    uint32_t count = 0;

    for (CalendarPendingInvite* invite = s_pendingInvites.Head(); invite; invite = s_pendingInvites.Next(invite)) {
        if (invite->m_inviterGuid && !FriendList::s_friendList->IsIgnored(invite->m_inviterGuid, false)) {
            count++;
        }
    }

    return count > static_cast<uint32_t>(CGCalendar::m_numInitialPending) ? count : CGCalendar::m_numInitialPending;
}

// VERIFY: 6c9627bd manual
// OFFSET: 0x005C3FE0
int32_t PH_CALENDAR_GROUP(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t clearPending = 1;

    switch (msgId) {
        case SMSG_CALENDAR_SEND_CALENDAR:
            PH_SMSG_CALENDAR_SEND_CALENDAR(msg);
            break;

        case SMSG_CALENDAR_SEND_EVENT:
            CGCalendar::ProcessGetEvent(msg);
            break;

        case SMSG_CALENDAR_FILTER_GUILD:
        case SMSG_CALENDAR_ARENA_TEAM:
            CGCalendar::ProcessInitialInviteList(msg);
            break;

        case SMSG_CALENDAR_EVENT_INVITE:
            CGCalendar::ProcessInvite(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_EVENT_INVITE_REMOVED:
            CGCalendar::ProcessInviteRemoved(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_EVENT_STATUS:
            CGCalendar::ProcessInviteStatus(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_RAID_LOCKOUT_ADDED:
            CGCalendar::ProcessLockoutAdded(msg);
            break;

        case SMSG_CALENDAR_RAID_LOCKOUT_REMOVED:
            CGCalendar::ProcessLockoutRemoved(msg);
            break;

        case SMSG_CALENDAR_EVENT_INVITE_ALERT:
            CGCalendar::ProcessInviteAlert(msg);
            return 1;

        case SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT:
            CGCalendar::ProcessInviteRemovedAlert(msg);
            return 1;

        case SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT:
            CGCalendar::ProcessInviteStatusAlert(msg);
            return 1;

        case SMSG_CALENDAR_EVENT_REMOVED_ALERT:
            CGCalendar::ProcessEventRemovedAlert(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_EVENT_UPDATED_ALERT:
            CGCalendar::ProcessEventUpdatedAlert(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT:
            CGCalendar::ProcessInviteModeratorStatusAlert(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_SEND_NUM_PENDING:
            CGCalendar::UpdatePendingInvites(msg);
            break;

        case SMSG_CALENDAR_EVENT_INVITE_NOTES:
            CGCalendar::ProcessInviteNotes(msg, &clearPending);

            if (!clearPending) {
                return 1;
            }

            break;

        case SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT:
            CGCalendar::ProcessInviteNotesAlert(msg);
            return 1;

        case SMSG_CALENDAR_RAID_LOCKOUT_UPDATED:
            CGCalendar::ProcessLockoutUpdated(msg);
            break;
    }

    CGCalendar::m_actionPending = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 0);
    return 1;
}

// VERIFY: 11872a05 manual
// OFFSET: 0x005C1E40
CalendarEventTables::CalendarEventTables() {
    for (int32_t i = 0; i < 8; i++) {
        this->m_counts[i] = 0;
    }
}

// VERIFY: d895f606 manual
// OFFSET: 0x005BE220
void CalendarEventTables::Clear() {
    for (uint32_t i = 0; i < 8; i++) {
        this->m_counts[i] = 0;
        this->m_tables[i].InternalClear(0);
    }
}

// VERIFY: 27b54cc9 manual
// OFFSET: 0x005BEAD0
void CGCalendar::ShutdownGame() {
    if (CGCalendar::m_curEvent) {
        delete CGCalendar::m_curEvent;
    }

    memset(&s_eventClipboard, 0, sizeof(s_eventClipboard));
    s_eventTables.Clear();
    s_holidays.Clear();
    s_raidResets.Clear();
    s_pendingInvites.Clear();
    s_dynamicHolidays.InternalClear(0);

    for (uint32_t month = 0; month < 3; month++) {
        for (uint32_t day = 0; day < 31; day++) {
            s_days[month][day].m_count = 0;
        }
    }

    for (uint32_t i = 0; i < 2; i++) {
        if (s_eventTextures[i].m_count) {
            s_eventTextures[i].Clear();
        }
    }

    ClientServices::ClearMessageHandler(SMSG_CALENDAR_SEND_CALENDAR);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_SEND_EVENT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_REMOVED);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_STATUS);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_NOTES);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_FILTER_GUILD);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_ARENA_TEAM);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_REMOVED);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_ADDED);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_RAID_LOCKOUT_UPDATED);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_REMOVED_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_STATUS_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_INVITE_NOTES_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_REMOVED_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_UPDATED_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_EVENT_MODERATOR_STATUS_ALERT);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_SEND_NUM_PENDING);
    ClientServices::ClearMessageHandler(SMSG_CALENDAR_CLEAR_PENDING_ACTION);
}

// VERIFY: 24ff9070 auto
// OFFSET: 0x005B7CE0
void CGCalendar::UpdatePendingInvites(CDataStore* msg) {
    int32_t previous = CGCalendar::m_numInitialPending;
    msg->Get(reinterpret_cast<uint32_t&>(CGCalendar::m_numInitialPending));

    if (CGCalendar::m_numInitialPending != previous) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
    }
}

// STATIC: 0x00ACFF8C
int32_t CGCalendar::m_absMonth = -1;

// STATIC: 0x00ACFF90
int32_t CGCalendar::m_absYear = 4;

// STATIC: 0x00C20800
int32_t CGCalendar::m_absMonthDays;

// STATIC: 0x00C20F78
CalendarEventSelection CGCalendar::m_selectedEvent;

// STATIC: 0x00C20F90
CalendarEventSelection CGCalendar::m_contextEvent;

// VERIFY: e086b5b0 auto
// OFFSET: 0x005B7A10
int32_t CGCalendar::GetDaysInMonth(int32_t month, int32_t year, int32_t* days, int32_t* weekDay) {
    WowTime date;
    date.m_month = month;
    date.m_monthDay = 0;
    date.m_year = year;
    date.AddDays(0, false);
    *weekDay = date.m_weekDay;
    date.m_month = month + 1;

    if (month == 12) {
        year++;
        date.m_month = 0;
    }

    date.m_year = year;
    date.AddDays(-1, false);
    *days = date.m_monthDay + 1;
    return date.m_monthDay + 1;
}

// VERIFY: a29063fa manual
// OFFSET: 0x005B8EC0
int32_t CGCalendar::GetMonth(int32_t offset, int32_t* month, int32_t* year, int32_t* days, int32_t* weekDay) {
    if (CGCalendar::m_absMonthDays) {
        *month = CGCalendar::m_absMonth;
        *year = CGCalendar::m_absYear;
    } else {
        *month = g_clientGameTime.m_month;
        *year = g_clientGameTime.m_year;
    }

    if (offset) {
        *year += offset / 12;
        *month += offset % 12;

        if (*month >= 12) {
            ++*year;
            *month -= 12;
        } else if (*month < 0) {
            --*year;
            *month += 12;
        }
    }

    int32_t clampedYear = *year;

    if (clampedYear >= 30) {
        clampedYear = 30;
    }

    *year = clampedYear;

    if (clampedYear <= 4) {
        *year = 4;
        int32_t clampedMonth = *month;

        if (clampedMonth <= 10) {
            clampedMonth = 10;
        }

        *month = clampedMonth;
    }

    return CGCalendar::GetDaysInMonth(*month, *year, days, weekDay);
}

// VERIFY: 897b7493 manual
// OFFSET: 0x005B8F70
int32_t CGCalendar::ClampAbsMonthDay(int32_t month, int32_t year, int32_t* outMonth, int32_t* outYear, int32_t* days, int32_t* weekDay) {
    int32_t minYear = g_clientGameTime.m_year - 5;
    int32_t maxYear = minYear + 10;

    if (year < minYear) {
        *outYear = minYear;
    } else if (year <= maxYear) {
        *outYear = year;
    } else {
        *outYear = maxYear;
    }

    int32_t clampedYear = *outYear;

    if (clampedYear >= 30) {
        clampedYear = 30;
    }

    *outYear = clampedYear;

    if (clampedYear < 4) {
        *outYear = 4;
    }

    *outMonth = abs(month % 12);
    return CGCalendar::GetDaysInMonth(*outMonth, *outYear, days, weekDay);
}

// VERIFY: e9369a93 manual
// OFFSET: 0x005B9000
bool CGCalendar::ComputeEventIndex(const CalendarEventSelection* selection, int32_t* monthOffset, int32_t* day, int32_t* index) {
    *monthOffset = 0;
    *day = 0;
    *index = 0;

    if (!selection->m_eventID && !selection->m_holidayID && !selection->m_startTime) {
        return false;
    }

    WowTime date;
    WowTime::WowDecodeTime(selection->m_startTime, &date);
    uint32_t month = 12 * (date.m_year - CGCalendar::m_absYear) - CGCalendar::m_absMonth + date.m_month + 1;

    if (month > 2) {
        return false;
    }

    TSGrowableArray<CalendarDayEvent*>* info = &s_days[month][date.m_monthDay];

    for (uint32_t i = 0; i < info->Count(); i++) {
        CalendarDayEvent* event = info->m_data[i];

        if (!event) {
            continue;
        }

        bool match = false;

        if (selection->m_eventID) {
            match = selection->m_eventID == event->m_eventID;
        }

        if (selection->m_holidayID) {
            match = selection->m_holidayID == event->m_holidayID;

            if (!match) {
                continue;
            }

            if (selection->m_eventType < 0) {
                match = selection->m_endTime == event->m_endTime;
            }
        }

        if (!match) {
            continue;
        }

        *monthOffset = month - 1;
        *day = date.m_monthDay;
        *index = i;
        return true;
    }

    return false;
}

// STATIC: 0x00C207F8
uint32_t CGCalendar::m_lastInviteTime;

// VERIFY: adfc66bf manual
// OFFSET: 0x005B7A80
bool GameCalendarGetMaxCreateDate(WowTime* date) {
    int32_t years = (g_clientGameTime.m_month + 13) / 12;
    date->m_month = (g_clientGameTime.m_month + 13) % 12;
    date->m_year = years + g_clientGameTime.m_year;
    date->m_monthDay = 0;
    date->AddDays(-1, false);
    int32_t year = date->m_year;

    if (year >= 31) {
        year = 31;
    }

    date->m_year = year;
    return date->SetHourAndMinutes(23u, 59u);
}

// STATIC: 0x00ACFF94
static CVar** s_eventFilters[8] = { nullptr, nullptr, &s_cvCalendarShowLockouts, &s_cvCalendarShowResets, nullptr, &s_cvCalendarShowWeeklyHolidays, &s_cvCalendarShowDarkmoon, &s_cvCalendarShowBattlegrounds };

// VERIFY: f0f4c3be auto
// OFFSET: 0x005B7820
CalendarDayEvent::CalendarDayEvent() {
    this->m_eventID = 0;
    this->m_startTime = 0;
    this->m_alarmTime = 0;
    this->m_endTime = 0;
    this->m_holidayID = 0;
    this->m_type = 0;
    this->m_flags = 0;
    this->m_eventType = -1;
    this->m_textureID = 0;
    this->m_inviteStatus = 0;
    this->m_inviteID = 0;
    this->m_creatorGUID = WOWGUID();
    this->m_alarmCallback = nullptr;
    this->m_sequenceIndex = 0;
    this->m_sequenceLength = 0;
}

// VERIFY: 5c4f6f0e manual
// OFFSET: 0x005B7890
int32_t CalendarDayEvent::GetSequenceType() const {
    if (this->m_sequenceLength <= 1) {
        return 4;
    }

    if (!this->m_sequenceIndex) {
        return 1;
    }

    return (this->m_sequenceIndex != this->m_sequenceLength - 1) + 2;
}

// VERIFY: a55134c1 manual
// OFFSET: 0x005BFAB0
static bool ComputeEventOccurrence(const WowTime* now, const WowTime* rangeStart, const WowTime* rangeEnd, WowTime* outEnd, HolidaysRec* holiday, int32_t index, bool* weekly, int32_t interval, WowTime* outStart) {
    *weekly = false;
    DYNAMICHOLIDAYHASH* dynamic = s_dynamicHolidays.Ptr(holiday->m_id, HASHKEY_NONE());
    int32_t* durations;
    int32_t* dates;
    int32_t looping;

    if (dynamic) {
        durations = dynamic->m_duration;
        dates = dynamic->m_date;
        looping = dynamic->m_looping;
    } else {
        looping = holiday->m_looping;
        durations = holiday->m_duration;
        dates = holiday->m_date;
    }

    if (index > 0 && !durations[index]) {
        return false;
    }

    int32_t duration = durations[index];

    if (!duration) {
        duration = 24;
    }

    int32_t durationMinutes = 60 * duration;

    for (uint32_t i = 0; i < 26; i++) {
        if (!dates[i]) {
            continue;
        }

        WowTime::WowDecodeTime(dates[i], outStart);
        const WowTime* reference = now;

        if (holiday->m_flags & 0x1) {
            outStart->m_tzHours = g_clientGameTime.m_tzHours;
            outStart->ConvertToLocal(*outStart);
        }

        int32_t days = 0;

        if (outStart->m_weekDay >= 0 && outStart->m_month < 0 && outStart->m_monthDay < 0 && outStart->m_year < 0) {
            reference = rangeStart;
            *weekly = true;
            int32_t diff = outStart->m_weekDay - rangeStart->m_weekDay;
            days = diff + (diff < 0 ? 7 : 0);
        }

        if (outStart->m_minute < 0) {
            outStart->m_minute = reference->m_minute;
        }

        if (outStart->m_hour < 0) {
            outStart->m_hour = reference->m_hour;
        }

        if (outStart->m_monthDay < 0) {
            outStart->m_monthDay = reference->m_monthDay;
        }

        if (outStart->m_month < 0) {
            outStart->m_month = reference->m_month;
        }

        if (outStart->m_year < 0) {
            int32_t year = CGCalendar::m_absYear;

            if (CGCalendar::m_absYear < 0) {
                year = g_clientGameTime.m_year;
                CGCalendar::m_absYear = year;

                if (year < 0) {
                    year = 4;
                    CGCalendar::m_absYear = 4;
                }
            }

            outStart->m_year = year;

            if (!CGCalendar::m_absMonth && outStart->m_month == 11) {
                outStart->m_year--;
            }
        }

        if (days) {
            outStart->AddDays(days, false);
        }

        for (int32_t j = 0; j < index; j++) {
            int32_t previous = durations[j];

            if (!previous) {
                previous = 24;
            }

            outStart->AddHolidayDuration(60 * previous);
        }

        bool after = *rangeEnd < *outStart;

        if (looping) {
            if (after) {
                return false;
            }

            while (true) {
                *outEnd = *outStart;
                outEnd->AddHolidayDuration(durationMinutes);

                if (!(*rangeStart >= *outEnd)) {
                    break;
                }

                outStart->AddHolidayDuration(interval);

                if (*rangeEnd < *outStart) {
                    return false;
                }
            }

            goto found;
        }

        if (!after) {
            *outEnd = *outStart;
            outEnd->AddHolidayDuration(durationMinutes);

            if (!(*outEnd < *rangeStart)) {
                goto found;
            }
        }
    }

    return false;

found:
    if (!outStart->m_hour && !outStart->m_minute) {
        outStart->AddMinutes(1);
    }

    if (!outEnd->m_hour && !outEnd->m_minute) {
        outEnd->AddMinutes(-1);
    }

    return true;
}

// VERIFY: 649c97da manual
// OFFSET: 0x005BFD20
static int QSortEvents(const void* a, const void* b) {
    const CalendarDayEvent* left = *static_cast<CalendarDayEvent* const*>(a);
    const CalendarDayEvent* right = *static_cast<CalendarDayEvent* const*>(b);

    if ((left->m_flags & 0x1) && !(right->m_flags & 0x1)) {
        return -1;
    }

    if (!(left->m_flags & 0x1) && (right->m_flags & 0x1)) {
        return 1;
    }

    if ((left->m_type & 0x543) && !(right->m_type & 0x543)) {
        return -1;
    }

    if (!(left->m_type & 0x543) && (right->m_type & 0x543)) {
        return 1;
    }

    uint32_t leftHoliday = left->m_type & 0x8;

    if (leftHoliday && !(right->m_type & 0x8)) {
        return -1;
    }

    if (!leftHoliday && (right->m_type & 0x8)) {
        return 1;
    }

    int32_t rightPriority = -1;
    int32_t leftPriority = -1;
    HolidaysRec* leftRec = nullptr;

    if (leftHoliday) {
        DYNAMICHOLIDAYHASH* dynamic = s_dynamicHolidays.Ptr(static_cast<uint32_t>(left->m_eventID), HASHKEY_NONE());

        if (dynamic) {
            leftPriority = dynamic->m_priority;
        } else {
            leftRec = g_holidaysDB.GetRecord(static_cast<int32_t>(left->m_eventID));

            if (leftRec) {
                leftPriority = leftRec->m_priority;
            }
        }
    }

    HolidaysRec* rightRec = nullptr;

    if (right->m_type & 0x8) {
        DYNAMICHOLIDAYHASH* dynamic = s_dynamicHolidays.Ptr(static_cast<uint32_t>(right->m_eventID), HASHKEY_NONE());

        if (dynamic) {
            rightPriority = dynamic->m_priority;
        } else {
            rightRec = g_holidaysDB.GetRecord(static_cast<int32_t>(right->m_eventID));

            if (rightRec) {
                rightPriority = rightRec->m_priority;
            }
        }
    }

    int32_t result = rightPriority - leftPriority;

    if (result) {
        return result;
    }

    result = left->GetSequenceType() - right->GetSequenceType();

    if (result) {
        return result;
    }

    result = left->m_startTime - right->m_startTime;

    if (result) {
        return result;
    }

    const char* leftName = left->m_title;
    const char* rightName = right->m_title;

    if (leftRec) {
        HolidayNamesRec* name = g_holidayNamesDB.GetRecord(leftRec->m_holidayNameID);

        if (name) {
            leftName = name->m_name;
        }
    } else if (left->m_type & 0x280) {
        MapRec* map = g_mapDB.GetRecord(left->m_holidayID);

        if (map) {
            leftName = map->m_mapName;
        }
    }

    if (rightRec) {
        HolidayNamesRec* name = g_holidayNamesDB.GetRecord(rightRec->m_holidayNameID);

        if (name) {
            rightName = name->m_name;
        }
    } else if (right->m_type & 0x280) {
        MapRec* map = g_mapDB.GetRecord(right->m_holidayID);

        if (map) {
            rightName = map->m_mapName;
        }
    }

    return SStrCmpUTF8I(leftName, rightName, STORM_MAX_STR);
}

// VERIFY: e79c8713 manual
// OFFSET: 0x005BFF30
void CGCalendar::UpdateDisplayList(bool signal) {
    if (!CGCalendar::m_absMonthDays) {
        int32_t year = g_clientGameTime.m_year;
        int32_t month = g_clientGameTime.m_month;
        CGCalendar::m_absMonth = month;
        CGCalendar::m_absYear = year;

        if (year < 30) {
            if (year <= 4) {
                year = 4;
                CGCalendar::m_absYear = 4;

                if (g_clientGameTime.m_month <= 10) {
                    month = 10;
                    CGCalendar::m_absMonth = 10;
                }
            }
        } else {
            year = 30;
            CGCalendar::m_absYear = 30;
        }

        int32_t weekDay;
        CGCalendar::GetDaysInMonth(month, year, &CGCalendar::m_absMonthDays, &weekDay);
        CGCalendar::CalculateEvents(true);
        return;
    }

    int32_t month = (CGCalendar::m_absMonth + 11) % 12;
    int32_t year = month == 11 ? CGCalendar::m_absYear - 1 : CGCalendar::m_absYear;

    for (uint32_t m = 0; m < 3; m++) {
        for (uint32_t d = 0; d < 31; d++) {
            TSGrowableArray<CalendarDayEvent*>& list = s_days[m][d];
            uint32_t count = 0;
            list.m_count = 0;

            WowTime date;
            date.m_month = month;
            date.m_monthDay = d;
            date.m_year = year;
            uint32_t hash;
            WowTime::WowEncodeTime(hash, &date);

            for (uint32_t t = 0; t < 8; t++) {
                CVar** filter = s_eventFilters[t];

                if (filter && !(*filter && (*filter)->m_intValue)) {
                    continue;
                }

                CALENDAREVENTHASH* entry = s_eventTables.m_tables[t].Ptr(hash, HASHKEY_NONE());

                if (!entry) {
                    continue;
                }

                uint32_t numEvents = entry->m_events.Count();
                list.SetCount(count + numEvents);

                for (uint32_t k = 0; k < numEvents; k++) {
                    CalendarDayEvent* event = entry->m_events.m_data[k];

                    if (!FriendList::s_friendList->IsIgnored(event->m_creatorGUID, false)) {
                        list.m_data[count++] = event;
                    }
                }
            }

            list.SetCount(count);
        }

        for (uint32_t d = 0; d < 31; d++) {
            qsort(s_days[m][d].m_data, s_days[m][d].Count(), sizeof(CalendarDayEvent*), QSortEvents);
        }

        month = (month + 1) % 12;

        if (!month && ++year >= 31) {
            break;
        }
    }

    if (signal) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
    }
}

// VERIFY: c7d9d86f manual
// OFFSET: 0x005C0240
void CGCalendar::SetFilter(CVar* var, int32_t value) {
    if (!var || !CGCalendar::m_absMonthDays) {
        return;
    }

    int32_t month = (CGCalendar::m_absMonth + 11) % 12;
    int32_t year = month == 11 ? CGCalendar::m_absYear - 1 : CGCalendar::m_absYear;

    for (uint32_t m = 0; m < 3; m++) {
        for (uint32_t d = 0; d < 31; d++) {
            TSGrowableArray<CalendarDayEvent*>& list = s_days[m][d];
            uint32_t count = 0;
            list.m_count = 0;

            WowTime date;
            date.m_month = month;
            date.m_monthDay = d;
            date.m_year = year;
            uint32_t hash;
            WowTime::WowEncodeTime(hash, &date);

            for (uint32_t t = 0; t < 8; t++) {
                CVar** filter = s_eventFilters[t];

                if (filter && *filter && !(*filter == var ? value : (*filter)->m_intValue)) {
                    continue;
                }

                CALENDAREVENTHASH* entry = s_eventTables.m_tables[t].Ptr(hash, HASHKEY_NONE());

                if (!entry) {
                    continue;
                }

                uint32_t numEvents = entry->m_events.Count();
                list.SetCount(count + numEvents);

                for (uint32_t k = 0; k < numEvents; k++) {
                    CalendarDayEvent* event = entry->m_events.m_data[k];

                    if (!FriendList::s_friendList->IsIgnored(event->m_creatorGUID, false)) {
                        list.m_data[count++] = event;
                    }
                }
            }

            list.SetCount(count);
        }

        for (uint32_t d = 0; d < 31; d++) {
            qsort(s_days[m][d].m_data, s_days[m][d].Count(), sizeof(CalendarDayEvent*), QSortEvents);
        }

        month = (month + 1) % 12;

        if (!month && ++year >= 31) {
            break;
        }
    }
}

// VERIFY: 6146a69c manual
// OFFSET: 0x005C04F0
void CGCalendar::InitHolidays() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    int32_t factionMask = 0;
    int32_t faction = player->GetPVPFactionIndex();

    if (faction == 1) {
        factionMask = 1;
    } else if (faction == 0) {
        factionMask = 2;
    }

    if (!(factionMask & 3)) {
        return;
    }

    int8_t region = static_cast<int8_t>(s_region);

    for (int32_t i = 0; i < g_holidaysDB.m_numRecords; i++) {
        HolidaysRec* holiday = i < 0 || i >= g_holidaysDB.m_numRecords ? nullptr : &g_holidaysDB.m_records[i];
        DYNAMICHOLIDAYHASH* dynamic = s_dynamicHolidays.Ptr(holiday->m_id, HASHKEY_NONE());
        int32_t regionMask = dynamic ? dynamic->m_region : holiday->m_region;

        if (regionMask && !((1 << (region - 1)) & regionMask)) {
            continue;
        }

        int32_t looping;
        int32_t filterType;
        int32_t* flags;
        int32_t* durations;

        if (dynamic) {
            looping = dynamic->m_looping;
            flags = dynamic->m_calendarFlags;
            durations = dynamic->m_duration;
            filterType = dynamic->m_calendarFilterType;
        } else {
            looping = holiday->m_looping;
            filterType = holiday->m_calendarFilterType;
            flags = holiday->m_calendarFlags;
            durations = holiday->m_duration;
        }

        for (int32_t j = 0; j < 10; j++) {
            if (!(factionMask & flags[j])) {
                continue;
            }

            CALENDARHOLIDAYNODE* node = s_holidays.NewNode(2, 0, 0);

            if (!node) {
                continue;
            }

            node->m_holiday = holiday;
            node->m_index = j;

            if (looping) {
                int32_t total = durations[j];

                if (!total) {
                    total = 24;
                }

                node->m_duration = total;

                for (int32_t k = 0; k < 10; k++) {
                    if (k != j) {
                        node->m_duration += durations[k];
                    }

                    if (!k && !durations[0]) {
                        node->m_duration += 3;
                    }
                }

                node->m_duration *= 60;
            }

            if (filterType == 0) {
                node->m_type = 5;
            } else if (filterType == 1) {
                node->m_type = 6;
            } else if (filterType == 2) {
                node->m_type = 7;
            }
        }
    }
}

// VERIFY: 0650d640 manual
// OFFSET: 0x005C2030
void CGCalendar::InsertEvent(CalendarDayEvent* event, int32_t table) {
    uint32_t originalEnd = event->m_endTime;
    WowTime start;
    WowTime end;
    WowTime::WowDecodeTime(event->m_startTime, &start);

    if (event->m_endTime) {
        WowTime::WowDecodeTime(event->m_endTime, &end);

        if (end.m_year == -1) {
            end.m_year = 31;
        }
    } else {
        end = start;
    }

    WowTime day = start;
    uint32_t numDays = 0;

    while (day <= end) {
        numDays++;
        day.AddDays(1, false);
    }

    int32_t month = start.m_month;
    uint32_t sequence = 0;
    bool first = true;
    TSHashTable<CALENDAREVENTHASH, HASHKEY_NONE>* events = &s_eventTables.m_tables[table];

    do {
        WowTime key;
        key.m_month = month;
        key.m_monthDay = start.m_monthDay;
        key.m_year = start.m_year;
        uint32_t hash;
        WowTime::WowEncodeTime(hash, &key);
        CALENDAREVENTHASH* entry = events->Ptr(hash, HASHKEY_NONE());

        if (!entry) {
            s_eventTables.m_counts[table]++;
            entry = events->New(hash, HASHKEY_NONE(), 0, 0);
        }

        if (!first) {
            void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);

            if (mem) {
                memcpy(mem, event, sizeof(CalendarDayEvent));
                event = static_cast<CalendarDayEvent*>(mem);
            } else {
                event = nullptr;
            }
        }

        event->m_sequenceIndex = sequence;
        event->m_sequenceLength = numDays;
        sequence++;

        CalendarDayEvent** slot = entry->m_events.New();

        if (slot) {
            *slot = event;
        }

        if (first) {
            memset(&start, 0xFF, 12);
            first = false;
        }

        start.AddDays(1, false);

        if (start > end) {
            break;
        }

        month = start.m_month;
    } while (start.m_month != end.m_month || start.m_monthDay != end.m_monthDay || end.m_hour || end.m_minute);

    if (numDays > 1) {
        event->m_startTime = originalEnd;
    }
}

// VERIFY: 1cd1e699 manual
// OFFSET: 0x005C23C0
void CGCalendar::CalculateEvents(bool signal) {
    s_eventTables.m_counts[4] = 0;
    s_eventTables.m_tables[4].InternalClear(0);
    s_eventTables.m_counts[5] = 0;
    s_eventTables.m_tables[5].InternalClear(0);
    s_eventTables.m_counts[6] = 0;
    s_eventTables.m_tables[6].InternalClear(0);
    s_eventTables.m_counts[7] = 0;
    s_eventTables.m_tables[7].InternalClear(0);
    s_eventTables.m_counts[3] = 0;
    s_eventTables.m_tables[3].InternalClear(0);

    WowTime start;
    WowTime end;
    WowTime rangeStart;
    WowTime rangeEnd;
    rangeStart.m_year = CGCalendar::m_absYear;
    rangeStart.m_month = CGCalendar::m_absMonth;
    rangeStart.m_monthDay = 0;
    rangeStart.SetHourAndMinutes(0u, 0u);
    rangeStart.AddDays(-6, false);
    rangeEnd.m_month = CGCalendar::m_absMonth;
    rangeEnd.m_monthDay = 0;
    rangeEnd.m_year = CGCalendar::m_absYear;
    rangeEnd.SetHourAndMinutes(0u, 0u);
    rangeEnd.AddDays(42, false);

    if (!s_holidays.Head()) {
        CGCalendar::InitHolidays();
    }

    for (CALENDARHOLIDAYNODE* node = s_holidays.Head(); node; node = s_holidays.Next(node)) {
        bool weekly = false;

        if (ComputeEventOccurrence(&g_clientGameTime, &rangeStart, &rangeEnd, &end, node->m_holiday, node->m_index, &weekly, node->m_duration, &start)) {
            if (weekly) {
                node->m_duration = 10080;
            } else {
                start.AddDays(0, false);
                end.AddDays(0, false);
            }

            WowTime::WowEncodeTime(node->m_startTime, &start);
            WowTime::WowEncodeTime(node->m_endTime, &end);
        } else {
            node->m_startTime = 0;
            node->m_endTime = 0;
        }
    }

    for (CALENDARHOLIDAYNODE* node = s_holidays.Head(); node; node = s_holidays.Next(node)) {
        if (!node->m_startTime) {
            continue;
        }

        HolidaysRec* holiday = g_holidaysDB.GetRecord(node->m_holiday->m_id);

        if (!holiday) {
            continue;
        }

        HolidayNamesRec* name = g_holidayNamesDB.GetRecord(holiday->m_holidayNameID);

        if (!name || !name->m_name) {
            continue;
        }

        if (node->m_duration) {
            WowTime::WowDecodeTime(node->m_startTime, &start);
            WowTime::WowDecodeTime(node->m_endTime, &end);

            if (end.m_year == -1) {
                end.m_year = 31;
            }

            if (!(start < rangeEnd)) {
                continue;
            }

            while (start.m_year < 31) {
                uint32_t startTime;
                uint32_t endTime;
                WowTime::WowEncodeTime(startTime, &start);
                WowTime::WowEncodeTime(endTime, &end);

                void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
                CalendarDayEvent* event = mem ? new (mem) CalendarDayEvent() : nullptr;
                event->m_eventID = node->m_holiday->m_id;
                event->m_title[0] = '\0';
                event->m_startTime = startTime;
                event->m_endTime = endTime;
                event->m_type = 8;
                CGCalendar::InsertEvent(event, node->m_type);
                start.AddHolidayDuration(node->m_duration);
                end.AddHolidayDuration(node->m_duration);

                if (!(start < rangeEnd)) {
                    break;
                }
            }
        } else {
            void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
            CalendarDayEvent* event = mem ? new (mem) CalendarDayEvent() : nullptr;
            event->m_eventID = node->m_holiday->m_id;
            event->m_title[0] = '\0';
            event->m_startTime = node->m_startTime;
            event->m_endTime = node->m_endTime;
            event->m_type = 8;
            CGCalendar::InsertEvent(event, node->m_type);
        }
    }

    WowTime reset;

    for (CALENDARRAIDRESETNODE* node = s_raidResets.Head(); node; node = s_raidResets.Next(node)) {
        if (!node->m_startTime || !node->m_interval) {
            continue;
        }

        WowTime::WowDecodeTime(node->m_startTime, &reset);
        reset.AddHolidayDuration(node->m_interval);

        while (reset < rangeEnd && reset.m_year < 31) {
            if (reset > rangeStart) {
                void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
                CalendarDayEvent* event = mem ? new (mem) CalendarDayEvent() : nullptr;
                event->m_holidayID = node->m_mapID;
                event->m_title[0] = '\0';
                event->m_type = 512;
                WowTime::WowEncodeTime(event->m_startTime, &reset);
                CGCalendar::InsertEvent(event, 3);
            }

            reset.AddHolidayDuration(node->m_interval);
        }
    }

    CGCalendar::UpdateDisplayList(signal);
}

// VERIFY: 0e8aff06 manual
// OFFSET: 0x005B7B30
bool CGCalendar::ValidateDate(const WowTime* date) {
    WowTime check = *date;
    check.m_tzHours = date->m_tzHours;

    WowTime limit;
    limit.m_month = 10;
    limit.m_monthDay = 23;
    limit.m_year = 4;
    limit.AddDays(0, false);
    limit.SetHourAndMinutes(0u, 0u);
    check.SetHourAndMinutes(limit.m_hour, limit.m_minute);

    if (check < limit) {
        return false;
    }

    limit.m_month = 0;
    limit.m_monthDay = 0;
    limit.m_year = 31;
    limit.AddDays(-1, false);
    limit.SetHourAndMinutes(23u, 59u);
    check.SetHourAndMinutes(limit.m_hour, limit.m_minute);
    return !(check > limit);
}

// VERIFY: 06530d5a auto
// OFFSET: 0x005B91A0
bool CGCalendar::GetDayEvent(uint32_t offset, uint32_t day) {
    if (offset + 1 > 2 || day >= 31) {
        return false;
    }

    WowTime date;
    int32_t month = 0;
    int32_t year = 0;
    int32_t days;
    int32_t weekDay;
    CGCalendar::GetMonth(offset, &month, &year, &days, &weekDay);
    date.SetDate(month, day, year);
    return CGCalendar::ValidateDate(&date);
}

// VERIFY: b060f4e8 auto
// OFFSET: 0x005B9220
int32_t CGCalendar::GetNumDayEvents(uint32_t offset, uint32_t day) {
    if (!CGCalendar::GetDayEvent(offset, day)) {
        return 0;
    }

    return s_days[offset + 1][day].Count();
}

// VERIFY: 28aef737 auto
// OFFSET: 0x005B92B0
int32_t CGCalendar::GetFirstPendingInviteIndex(uint32_t offset, uint32_t day) {
    if (!CGCalendar::GetDayEvent(offset, day)) {
        return -1;
    }

    TSGrowableArray<CalendarDayEvent*>& list = s_days[offset + 1][day];

    for (uint32_t i = 0; i < list.Count(); i++) {
        if (list.m_data[i] && (list.m_data[i]->m_flags & 0x1)) {
            return i;
        }
    }

    return -1;
}

// VERIFY: b580c245 auto
// OFFSET: 0x005B7910
static void __stdcall FireEventAlarm(CGameTime* time, void* param) {
    CalendarDayEvent* event = static_cast<CalendarDayEvent*>(param);
    WowTime alarm;
    WowTime::WowDecodeTime(event->m_alarmTime, &alarm);
    alarm.m_tzHours = 0;

    if (*time == alarm && (event->m_inviteStatus == 3 || event->m_inviteStatus == 1)) {
        WowTime start;
        WowTime::WowDecodeTime(event->m_startTime, &start);
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_EVENT_ALARM, "%s%d%d", event->m_title, start.m_hour, start.m_minute);
        CGameTime::GameTimeUnregisterCallback(event->m_alarmCallback);
        event->m_alarmCallback = nullptr;
    }
}

// VERIFY: fecc7261 auto
// OFFSET: 0x005B79B0
void CalendarDayEvent::ScheduleAlarm() {
    WowTime alarm;
    WowTime::WowDecodeTime(this->m_startTime, &alarm);
    alarm.m_tzHours = 0;
    alarm.AddMinutes(-15);
    WowTime::WowEncodeTime(this->m_alarmTime, &alarm);
    this->m_alarmCallback = g_clientGameTime.GameTimeRegisterCallback(&alarm, FireEventAlarm, this);
}

// VERIFY: 4db791b6 manual
// OFFSET: 0x005B8D10
CalendarDayEvent::~CalendarDayEvent() {
    if (this->m_alarmCallback) {
        CGameTime::GameTimeUnregisterCallback(this->m_alarmCallback);
        this->m_alarmCallback = nullptr;
    }
}

// VERIFY: f6c769b5 auto
// OFFSET: 0x005B7AF0
static uint32_t EncodeDate(int32_t month, int32_t monthDay, int32_t year) {
    WowTime date;
    date.m_month = month;
    date.m_monthDay = monthDay;
    date.m_year = year;
    uint32_t value;
    WowTime::WowEncodeTime(value, &date);
    return value;
}

// VERIFY: 5341ec83 manual
// OFFSET: 0x005B7C00
bool CGCalendar::ValidateCreateDate(const WowTime* date) {
    if (!CGCalendar::ValidateDate(date)) {
        return false;
    }

    WowTime check = *date;
    check.m_tzHours = date->m_tzHours;
    WowTime limit;
    limit = g_clientGameTime;
    check.SetHourAndMinutes(limit.m_hour, limit.m_minute);

    if (check < limit) {
        return false;
    }

    GameCalendarGetMaxCreateDate(&limit);
    check.SetHourAndMinutes(limit.m_hour, limit.m_minute);
    return !(check > limit);
}

// VERIFY: 5f5b9bac auto
// OFFSET: 0x005B7CC0
void CGCalendar::ProcessInitialInviteList(CDataStore* msg) {
    CGCalendar::m_curEvent->ProcessInitialInviteList(msg);
}

// VERIFY: ce5df57d manual
// OFFSET: 0x005B7D20
bool CGCalendar::CanEditCurrentEvent() {
    if (!CGCalendar::m_curEvent) {
        return false;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        return CGGuildInfo::CanEditGuildEvent();
    }

    return CGCalendar::m_curEvent->IsModerator();
}

// VERIFY: 784b0912 manual
// OFFSET: 0x005B7D60
void CGCalendar::SetInviteStatus(uint32_t index, uint32_t status) {
    if (!CGCalendar::m_curEvent) {
        return;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return;
    }

    if (CGCalendar::m_curEvent->m_new || !CGCalendar::m_actionPending) {
        CGCalendar::m_curEvent->SetInviteStatus(index, status);
    }
}

// VERIFY: 60d9ba2b manual
// OFFSET: 0x005B7DC0
void CGCalendar::SetModeratorStatus(uint32_t index, uint32_t status) {
    if (!CGCalendar::m_curEvent) {
        return;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return;
    }

    if (CGCalendar::m_curEvent->m_new || !CGCalendar::m_actionPending) {
        CGCalendar::m_curEvent->SetModeratorStatus(index, status);
    }
}

// VERIFY: 4354a74f auto
// OFFSET: 0x005B7E20
void CGCalendar::SetActionPending(uint8_t pending) {
    CGCalendar::m_actionPending = pending;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", pending);
}

// VERIFY: 7820a65b auto
// OFFSET: 0x005B7FD0
const char* CGCalendar::GetCalendarTypeString(uint32_t flags) {
    if (flags & 0x1) {
        return "PLAYER";
    }

    if (flags & 0x40) {
        return "GUILD_ANNOUNCEMENT";
    }

    if (flags & 0x400) {
        return "GUILD_EVENT";
    }

    if (flags & 0x4) {
        return "SYSTEM";
    }

    if (flags & 0x8) {
        return "HOLIDAY";
    }

    if (flags & 0x80) {
        return "RAID_LOCKOUT";
    }

    if (flags & 0x200) {
        return "RAID_RESET";
    }

    if (flags & 0x2) {
        return "PLAYER";
    }

    if (flags & 0x100) {
        return "PLAYER";
    }

    return "";
}

// VERIFY: ab94a6ff auto
// OFFSET: 0x005B9140
uint32_t CGCalendar::GetDateKey(uint32_t time) {
    WowTime date;
    WowTime::WowDecodeTime(time, &date);
    WowTime key;
    key.m_month = date.m_month;
    key.m_monthDay = date.m_monthDay;
    key.m_year = date.m_year;
    uint32_t value;
    WowTime::WowEncodeTime(value, &key);
    return value;
}

// VERIFY: 9cf4e561 auto
// OFFSET: 0x005B9260
CalendarDayEvent* CGCalendar::GetDayEvent(uint32_t offset, uint32_t day, uint32_t index) {
    if (CGCalendar::GetDayEvent(offset, day) && index < s_days[offset + 1][day].Count()) {
        return s_days[offset + 1][day].m_data[index];
    }

    return nullptr;
}

// VERIFY: 9bc3f579 auto
// OFFSET: 0x005B9310
void CGCalendar::GetEvent(uint32_t offset, uint32_t day, uint32_t index) {
    if (CGCalendar::m_actionPending || offset + 1 > 2 || day >= 31 || index >= s_days[offset + 1][day].Count()) {
        return;
    }

    CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

    if (event && (event->m_type & 0x543)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_GET_EVENT));
        msg.Put(event->m_eventID);
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGCalendar::SetActionPending(1);
        return;
    }

    CGCalendar::m_selectedEvent.m_eventID = event->m_eventID;
    CGCalendar::m_selectedEvent.m_startTime = event->m_startTime;
    CGCalendar::m_selectedEvent.m_endTime = event->m_endTime;
    CGCalendar::m_selectedEvent.m_holidayID = event->m_holidayID;
    CGCalendar::m_selectedEvent.m_eventType = event->m_type;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_OPEN_EVENT, "%s", CGCalendar::GetCalendarTypeString(event->m_type));
}

// VERIFY: b3fb02e5 auto
// OFFSET: 0x005B9490
const char* GetHolidayDescription(int32_t holidayId) {
    HolidaysRec* holiday = g_holidaysDB.GetRecord(holidayId);

    if (!holiday) {
        return nullptr;
    }

    HolidayDescriptionsRec* description = g_holidayDescriptionsDB.GetRecord(holiday->m_holidayDescriptionID);
    return description ? description->m_description : nullptr;
}

// VERIFY: 820bfaa2 manual
// OFFSET: 0x005B94F0
bool CGCalendar::SetEventDate(uint32_t month, uint32_t monthDay, uint32_t year) {
    if (!CGCalendar::m_curEvent) {
        return false;
    }

    bool canEdit = (CGCalendar::m_curEvent->m_flags & 0x40) ? CGGuildInfo::CanEditGuildEvent() : CGCalendar::m_curEvent->IsModerator();

    if (!canEdit) {
        return false;
    }

    return CGCalendar::m_curEvent->SetDate(month, monthDay, year);
}

// VERIFY: 613d12ff manual
// OFFSET: 0x005B9540
bool CGCalendar::SetEventTime(uint32_t hour, uint32_t minute) {
    if (!CGCalendar::m_curEvent) {
        return false;
    }

    bool canEdit = (CGCalendar::m_curEvent->m_flags & 0x40) ? CGGuildInfo::CanEditGuildEvent() : CGCalendar::m_curEvent->IsModerator();

    if (!canEdit) {
        return false;
    }

    return CGCalendar::m_curEvent->SetTime(hour, minute);
}

// VERIFY: 8d81e954 manual
// OFFSET: 0x005B9590
bool CGCalendar::SetEventLockoutDate(uint32_t month, uint32_t monthDay, uint32_t year) {
    if (!CGCalendar::m_curEvent) {
        return false;
    }

    bool canEdit = (CGCalendar::m_curEvent->m_flags & 0x40) ? CGGuildInfo::CanEditGuildEvent() : CGCalendar::m_curEvent->IsModerator();

    if (!canEdit) {
        return false;
    }

    return CGCalendar::m_curEvent->SetLockoutDate(month, monthDay, year);
}

// VERIFY: 0685eff6 manual
// OFFSET: 0x005B95E0
bool CGCalendar::SetEventLockoutTime(uint32_t hour, uint32_t minute) {
    if (!CGCalendar::m_curEvent) {
        return false;
    }

    bool canEdit = (CGCalendar::m_curEvent->m_flags & 0x40) ? CGGuildInfo::CanEditGuildEvent() : CGCalendar::m_curEvent->IsModerator();

    if (!canEdit) {
        return false;
    }

    return CGCalendar::m_curEvent->SetLockoutTime(hour, minute);
}

// VERIFY: a4ee2027 manual
// OFFSET: 0x005B9630
void CGCalendar::SetEventTextureID(uint32_t index) {
    if (!CGCalendar::m_curEvent) {
        return;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return;
    }

    uint32_t list;

    if (CGCalendar::m_curEvent->m_type == 1) {
        list = 0;
    } else if (CGCalendar::m_curEvent->m_type == 0) {
        list = 1;
    } else {
        return;
    }

    if (index < s_eventTextures[list].Count()) {
        CGCalendar::m_curEvent->SetTextureID(s_eventTextures[list].m_data[index]);
    }
}

// VERIFY: 51286ab1 auto
// OFFSET: 0x005B96A0
void CGCalendar::RemoveEvent() {
    if (CGCalendar::m_actionPending || !CGCalendar::m_curEvent || CGCalendar::m_curEvent->m_new) {
        return;
    }

    if (CGCalendar::CanEditCurrentEvent()) {
        if (!CGCalendar::m_curEvent->RemoveEvent()) {
            return;
        }
    } else if (!CGCalendar::m_curEvent->RemoveInviteByGUID(ClntObjMgrGetActivePlayer())) {
        return;
    }

    CGCalendar::SetActionPending(1);
}

// VERIFY: efc33489 auto
// OFFSET: 0x005B9700
int32_t CGCalendar::GetTextureIndex(uint32_t type, int32_t textureID) {
    uint32_t list;

    if (type == 1) {
        list = 0;
    } else if (type == 0) {
        list = 1;
    } else {
        return -1;
    }

    for (uint32_t i = 0; i < s_eventTextures[list].Count(); i++) {
        if (s_eventTextures[list].m_data[i] == textureID) {
            return i;
        }
    }

    return -1;
}

// VERIFY: eee7ac59 manual
// OFFSET: 0x005B9760
EventInviteInfo* CGCalendar::GetCurrentEventInvite(WOWGUID guid) {
    return CGCalendar::m_curEvent ? CGCalendar::m_curEvent->GetInvite(guid) : nullptr;
}

// VERIFY: 2ca20a00 auto
// OFFSET: 0x005B9780
void CGCalendar::RemoveInvite(uint32_t index) {
    if (CGCalendar::m_actionPending || !CGCalendar::m_curEvent || !CGCalendar::m_curEvent->RemoveInvite(index)) {
        return;
    }

    if (CGCalendar::m_curEvent->m_new) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_INVITE_LIST, nullptr);
    } else {
        CGCalendar::SetActionPending(1);
    }
}

// VERIFY: 783bd655 auto
// OFFSET: 0x005B97D0
void CGCalendar::SelectContextEvent(uint32_t offset, uint32_t day, uint32_t index) {
    if (!CGCalendar::GetDayEvent(offset, day) || index >= s_days[offset + 1][day].Count()) {
        return;
    }

    CalendarDayEvent* event = s_days[offset + 1][day].m_data[index];

    if (!event) {
        return;
    }

    CGCalendar::m_contextEvent.m_eventID = event->m_eventID;
    CGCalendar::m_contextEvent.m_startTime = event->m_startTime;
    CGCalendar::m_contextEvent.m_endTime = event->m_endTime;
    CGCalendar::m_contextEvent.m_holidayID = event->m_holidayID;
    CGCalendar::m_contextEvent.m_eventType = event->m_type;
    CGCalendar::m_contextEventInfo = event;
}

// VERIFY: 11d7b720 manual
// OFFSET: 0x005B9860
void CGCalendar::ContextMenuInviteRSVP(CalendarDayEvent* event, uint32_t status) {
    if (!event || CGCalendar::m_actionPending) {
        return;
    }

    if ((event->m_type & 0x400) && (event->m_flags & 0x8)) {
        return;
    }

    if (status != 1 && status != 2 && status != 8) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_RSVP));
    msg.Put(event->m_eventID);
    msg.Put(event->m_inviteID);
    msg.Put(status);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::m_actionPending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
}

// VERIFY: 148806d4 manual
// OFFSET: 0x005BBB40
void CGCalendar::GetCalendar() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_GET_CALENDAR));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::m_openedCalendar = 1;
}

// VERIFY: 18bd4700 manual
// OFFSET: 0x005BBC20
void CGCalendar::ProcessInviteNotes(CDataStore* msg, uint8_t* clearPending) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        msg->m_read = msg->m_size;
        return;
    }

    SmartGUID guid;
    *msg >> guid;
    uint64_t eventID;
    msg->Get(eventID);
    char notes[128];
    msg->GetString(notes, STORM_MAX_STR);
    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        CGCalendar::m_curEvent->UpdateInviteNotes(guid, notes);
    }
}

// VERIFY: 6d3709b0 manual
// OFFSET: 0x005BBCD0
void CGCalendar::ProcessInviteNotesAlert(CDataStore* msg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        msg->m_read = msg->m_size;
        return;
    }

    uint64_t eventID;
    msg->Get(eventID);
    char notes[128];
    msg->GetString(notes, STORM_MAX_STR);

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        CGCalendar::m_curEvent->UpdateInviteNotes(player->m_obj->m_guid, notes);
    }
}

// VERIFY: eae011c1 auto
// OFFSET: 0x005BBD60
void CGCalendar::AddEvent() {
    if (CGCalendar::m_actionPending) {
        return;
    }

    if (!CGCalendar::m_curEvent) {
        CGCalendar::CalendarError("CALENDAR_ERROR_INTERNAL", 0);
        return;
    }

    uint32_t now;

    if ((unk_BD09AC && unk_BD09AC->m_intValue) || (now = OsGetAsyncTimeMs(), !CGCalendar::m_lastEventTime) || now - CGCalendar::m_lastEventTime >= 5000) {
        if (CGCalendar::ValidateCreateDate(&CGCalendar::m_curEvent->m_date) && CGCalendar::m_curEvent->AddEvent()) {
            CGCalendar::m_lastEventTime = OsGetAsyncTimeMs();
            CGCalendar::SetActionPending(1);
        }
    } else {
        CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_THROTTLED", 0);
    }
}

// VERIFY: e5b81e7f manual
// OFFSET: 0x005BBE50
void CGCalendar::CreatePlayerEvent() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (CGCalendar::m_curEvent) {
        delete CGCalendar::m_curEvent;
    }

    void* mem = SMemAlloc(sizeof(CGCalendarEvent), __FILE__, __LINE__, 0);
    CGCalendar::m_curEvent = mem ? new (mem) CGCalendarEvent(player->m_obj->m_guid, player->m_unit->m_unitFieldLevel, 0) : nullptr;
    CGCalendar::m_selectedEvent.m_eventID = 0;
    CGCalendar::m_selectedEvent.m_startTime = 0;
    CGCalendar::m_selectedEvent.m_endTime = 0;
    CGCalendar::m_selectedEvent.m_holidayID = 0;
    CGCalendar::m_selectedEvent.m_eventType = 0;

    if (CGCalendar::m_curEvent) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_INVITE_LIST, "%b", 1);
    }
}

// VERIFY: 4fb18508 manual
// OFFSET: 0x005BBF20
void CGCalendar::CreateGuildAnnouncement() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!player->m_player->m_playerGuildid) {
        CGCalendar::CalendarError("ERR_GUILD_PLAYER_NOT_IN_GUILD", 0);
        return;
    }

    if (CGCalendar::m_curEvent) {
        delete CGCalendar::m_curEvent;
    }

    void* mem = SMemAlloc(sizeof(CGCalendarEvent), __FILE__, __LINE__, 0);
    CGCalendar::m_curEvent = mem ? new (mem) CGCalendarEvent(WOWGUID(), 0, 1) : nullptr;
    CGCalendar::m_selectedEvent.m_eventID = 0;
    CGCalendar::m_selectedEvent.m_startTime = 0;
    CGCalendar::m_selectedEvent.m_endTime = 0;
    CGCalendar::m_selectedEvent.m_holidayID = 0;
    CGCalendar::m_selectedEvent.m_eventType = 0;
}

// VERIFY: 8c223278 manual
// OFFSET: 0x005BC030
void CGCalendar::CreateGuildEvent() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!player->m_player->m_playerGuildid) {
        CGCalendar::CalendarError("ERR_GUILD_PLAYER_NOT_IN_GUILD", 0);
        return;
    }

    if (CGCalendar::m_curEvent) {
        delete CGCalendar::m_curEvent;
    }

    void* mem = SMemAlloc(sizeof(CGCalendarEvent), __FILE__, __LINE__, 0);
    CGCalendar::m_curEvent = mem ? new (mem) CGCalendarEvent(player->m_obj->m_guid, player->m_unit->m_unitFieldLevel, 2) : nullptr;
    CGCalendar::m_selectedEvent.m_eventID = 0;
    CGCalendar::m_selectedEvent.m_startTime = 0;
    CGCalendar::m_selectedEvent.m_endTime = 0;
    CGCalendar::m_selectedEvent.m_holidayID = 0;
    CGCalendar::m_selectedEvent.m_eventType = 0;
}

// VERIFY: f0dee297 manual
// OFFSET: 0x005BC160
void CGCalendar::GuildFilter(uint32_t minLevel, uint32_t maxLevel, uint32_t minRank) {
    if (CGCalendar::m_actionPending) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (!player->m_player->m_playerGuildid) {
        CGCalendar::CalendarError("ERR_GUILD_PLAYER_NOT_IN_GUILD", 0);
        return;
    }

    if (!CGCalendar::m_curEvent || !CGCalendar::m_curEvent->m_new || (CGCalendar::m_curEvent->m_flags & 0x440)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_GUILD_FILTER));
    msg.Put(minLevel);
    msg.Put(maxLevel);
    msg.Put(minRank);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::SetActionPending(1);
}

// VERIFY: bb90e389 manual
// OFFSET: 0x005BC240
void CGCalendar::ArenaTeam(uint32_t index) {
    if (CGCalendar::m_actionPending) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || index >= 3) {
        return;
    }

    uint32_t* info = &player->m_player->m_playerFieldArenaTeamInfo11[index * 7];

    if (!info || !*info || !CGCalendar::m_curEvent || !CGCalendar::m_curEvent->m_new || (CGCalendar::m_curEvent->m_flags & 0x440)) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_ARENA_TEAM));
    msg.Put(*info);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::SetActionPending(1);
}

// VERIFY: acb138ea auto
// OFFSET: 0x005BC310
void CGCalendar::UpdateEvent() {
    if (CGCalendar::m_actionPending || !CGCalendar::m_curEvent || CGCalendar::m_curEvent->m_new) {
        return;
    }

    if (!CGCalendar::CanEditCurrentEvent()) {
        CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
        return;
    }

    if (CGCalendar::m_curEvent->Update()) {
        CGCalendar::SetActionPending(1);
    }
}

// VERIFY: 0cd94ee0 auto
// OFFSET: 0x005BC360
void CGCalendar::Invite(const char* name) {
    if (CGCalendar::m_actionPending || !CGCalendar::m_curEvent || !CGCalendar::m_curEvent->IsModerator()) {
        return;
    }

    uint32_t now;

    if ((unk_BD09AC && unk_BD09AC->m_intValue) || (now = OsGetAsyncTimeMs(), !CGCalendar::m_lastInviteTime) || now - CGCalendar::m_lastInviteTime >= 2000) {
        if (CGCalendar::m_curEvent->Invite(name)) {
            CGCalendar::m_lastInviteTime = OsGetAsyncTimeMs();
            CGCalendar::SetActionPending(1);
        }
    } else {
        CGCalendar::CalendarError("CALENDAR_ERROR_INVITE_THROTTLED", 0);
    }
}

// VERIFY: 535dc77b manual
// OFFSET: 0x005BC3E0
void CGCalendar::InviteRSVP(uint32_t status) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || CGCalendar::m_actionPending || !CGCalendar::m_curEvent) {
        return;
    }

    bool sent;

    if ((CGCalendar::m_curEvent->m_flags & 0x400) && CGCalendar::m_curEvent->m_guildID == player->m_player->m_playerGuildid) {
        if (status != 8) {
            return;
        }

        sent = CGCalendar::m_curEvent->SignUp(1);
    } else {
        sent = CGCalendar::m_curEvent->InviteRSVP(status);
    }

    if (sent) {
        CGCalendar::SetActionPending(1);
    }
}

// VERIFY: 22c17537 manual
// OFFSET: 0x005BC460
void CGCalendar::EventSignUp() {
    if (CGCalendar::m_actionPending || !CGCalendar::m_curEvent || !(CGCalendar::m_curEvent->m_flags & 0x400)) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if ((!player || !CGCalendar::GetCurrentEventInvite(player->m_obj->m_guid)) && CGCalendar::m_curEvent->SignUp(0)) {
        CGCalendar::SetActionPending(1);
    }
}

// VERIFY: be3b8c9d manual
// OFFSET: 0x005BC4D0
void CGCalendar::ContextMenuInviteRemove(CalendarDayEvent* event) {
    if (!event || CGCalendar::m_actionPending) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    SmartGUID guid;
    guid.m_guid = player->m_obj->m_guid.m_guid;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_REMOVE_INVITE));
    msg << guid;
    msg.Put(event->m_inviteID);
    msg.Put(static_cast<uint64_t>(0));
    msg.Put(event->m_eventID);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::m_actionPending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
}

// VERIFY: 3bea9afb manual
// OFFSET: 0x005BC5E0
void CGCalendar::ContextMenuEventSignUp(CalendarDayEvent* event, uint8_t tentative) {
    if (!event || CGCalendar::m_actionPending || ((event->m_type & 0x400) && event->m_inviteStatus != 7)) {
        return;
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_SIGN_UP));
    msg.Put(event->m_eventID);
    msg.Put(tentative);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::m_actionPending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
}

// VERIFY: 4ead8e12 manual
// OFFSET: 0x005BC6D0
void CGCalendar::ContextMenuEventRemove(CalendarDayEvent* event) {
    if (!event || CGCalendar::m_actionPending) {
        return;
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_REMOVE_EVENT));
    msg.Put(event->m_eventID);
    msg.Put(event->m_inviteID);
    msg.Put(event->m_type);
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGCalendar::m_actionPending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
}

// VERIFY: aab52edd auto
// OFFSET: 0x005BC7C0
void CGCalendar::ContextMenuEventCopy(CalendarDayEvent* event) {
    if (!event) {
        return;
    }

    bool allowed = (event->m_type & 0x440) ? CGGuildInfo::CanEditGuildEvent() : (event->m_flags & 0x6) != 0;

    if (!allowed) {
        memset(&s_eventClipboard, 0, sizeof(s_eventClipboard));
        CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
        return;
    }

    s_eventClipboard = *event;
}

// VERIFY: 5a5ef0de auto
// OFFSET: 0x005BC870
void CGCalendar::ContextMenuEventPaste(int32_t offset, uint32_t day) {
    if (CGCalendar::m_actionPending || !s_eventClipboard.m_eventID) {
        return;
    }

    int32_t month = 0;
    int32_t year = 0;
    int32_t days = 0;
    uint32_t now;

    if ((unk_BD09AC && unk_BD09AC->m_intValue) || (now = OsGetAsyncTimeMs(), !CGCalendar::m_lastEventTime) || now - CGCalendar::m_lastEventTime >= 5000) {
        int32_t weekDay;
        CGCalendar::GetMonth(offset, &month, &year, &days, &weekDay);

        if (day >= static_cast<uint32_t>(days)) {
            return;
        }

        WowTime date;
        WowTime::WowDecodeTime(s_eventClipboard.m_startTime, &date);
        date.m_tzHours = 0;
        date.m_month = month;
        date.m_monthDay = day;
        date.m_year = year;
        date.AddDays(0, false);

        if (!CGCalendar::ValidateCreateDate(&date)) {
            return;
        }

        uint32_t time;
        WowTime::WowEncodeTime(time, &date);
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_COPY_EVENT));
        msg.Put(s_eventClipboard.m_eventID);
        msg.Put(s_eventClipboard.m_inviteID);
        msg.Put(time);
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGCalendar::m_lastEventTime = OsGetAsyncTimeMs();
        CGCalendar::SetActionPending(1);
    } else {
        CGCalendar::CalendarError("CALENDAR_ERROR_EVENT_THROTTLED", 0);
    }
}

// VERIFY: 38ca5713 manual
// OFFSET: 0x005BC9D0
bool CGCalendar::CanComplainAboutEvent(CalendarDayEvent* event) {
    if (!event) {
        return false;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    return player && CGChat::m_complaintsSystemStatus && !(event->m_type & 0x6CC) && event->m_creatorGUID != player->m_obj->m_guid && !FriendList::s_friendList->IsFriend(event->m_creatorGUID);
}

// VERIFY: 9cab6517 manual
// OFFSET: 0x005BCA50
void CGCalendar::Complain(CalendarDayEvent* event) {
    if (CGCalendar::m_actionPending || !CGCalendar::CanComplainAboutEvent(event)) {
        return;
    }

    {
        CDataStore msg;

        if (FriendList::s_friendList->IsComplained(event->m_creatorGUID)) {
            CGCalendar::CalendarError("COMPLAINT_ADDED", 0);
            SmartGUID guid;
            guid.m_guid = ClntObjMgrGetActivePlayerObj()->m_obj->m_guid.m_guid;
            msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_REMOVE_INVITE));
            msg << guid;
            msg.Put(event->m_inviteID);
            msg.Put(static_cast<uint64_t>(0));
            msg.Put(event->m_eventID);
        } else {
            FriendList::s_friendList->AddComplained(event->m_creatorGUID);
            msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_COMPLAIN));
            msg.Put(event->m_creatorGUID.m_guid);
            msg.Put(event->m_eventID);
            msg.Put(event->m_inviteID);
        }

        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    CGCalendar::m_actionPending = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
}

// VERIFY: 46672b96 manual
// OFFSET: 0x005BD850
bool CGCalendar::HasPendingInvite(uint64_t eventID) {
    for (CalendarPendingInvite* invite = s_pendingInvites.Head(); invite; invite = s_pendingInvites.Next(invite)) {
        if (invite->m_eventID == eventID) {
            return true;
        }
    }

    return false;
}

// VERIFY: 5f0f7f59 manual
// OFFSET: 0x005BDDE0
CALENDAREVENTHASH::~CALENDAREVENTHASH() {
    uint32_t count = this->m_events.Count();

    for (uint32_t i = 0; i < count; i++) {
        delete this->m_events.m_data[i];
    }
}

// VERIFY: ec6c6961 manual
// OFFSET: 0x005BDE60
void CGCalendar::InvalidatePlayer(WOWGUID guid) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    for (CALENDAREVENTHASH* entry = s_eventTables.m_tables[0].m_fulllist.Head(); entry; entry = s_eventTables.m_tables[0].Next(entry)) {
        for (uint32_t i = 0; i < entry->m_events.Count(); i++) {
            if (entry->m_events.m_data[i]->m_creatorGUID == guid) {
                CGCalendar::GetCalendar();
                return;
            }
        }
    }
}

// VERIFY: eded04a1 manual
// OFFSET: 0x005BE590
void CGCalendar::AddPendingInvite(CalendarDayEvent* event) {
    CalendarPendingInvite* invite = s_pendingInvites.NewNode(2, 0, 0);

    if (!invite) {
        return;
    }

    event->m_flags |= 0x1;
    invite->m_eventID = event->m_eventID;
    invite->m_inviteID = event->m_inviteID;
    invite->m_inviterGuid = event->m_creatorGUID;
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
}

// VERIFY: 2878db21 manual
// OFFSET: 0x005BE950
bool CalendarEventTables::Delete(int32_t table, uint32_t date, uint64_t eventID) {
    CALENDAREVENTHASH* entry = this->m_tables[table].Ptr(date, HASHKEY_NONE());

    if (!entry) {
        return false;
    }

    uint32_t count = entry->m_events.Count();

    if (!count) {
        return false;
    }

    uint32_t index = 0;

    while (entry->m_events.m_data[index]->m_eventID != eventID) {
        if (++index >= count) {
            return false;
        }
    }

    delete entry->m_events.m_data[index];

    for (uint32_t i = index; i < count - 1; i++) {
        entry->m_events.m_data[i] = entry->m_events.m_data[i + 1];
    }

    entry->m_events.SetCount(count - 1);
    return true;
}

// VERIFY: 7034ef82 manual
// OFFSET: 0x005BEA50
CalendarDayEvent* CalendarEventTables::GetEvent(int32_t table, uint32_t date, uint64_t eventID) {
    CALENDAREVENTHASH* entry = this->m_tables[table].Ptr(date, HASHKEY_NONE());

    if (!entry) {
        return nullptr;
    }

    for (uint32_t i = 0; i < entry->m_events.Count(); i++) {
        if (entry->m_events.m_data[i]->m_eventID == eventID) {
            return entry->m_events.m_data[i];
        }
    }

    return nullptr;
}

// VERIFY: c611b0b3 manual
// OFFSET: 0x005BECD0
bool CGCalendar::DeletePendingInvite(uint64_t eventID) {
    for (CalendarPendingInvite* invite = s_pendingInvites.Head(); invite; invite = s_pendingInvites.Next(invite)) {
        if (invite->m_eventID == eventID) {
            s_pendingInvites.DeleteNode(invite);
            return true;
        }
    }

    return false;
}

// VERIFY: daa3164f manual
// OFFSET: 0x005BED20
void CGCalendar::ProcessGetEvent(CDataStore* msg) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return;
    }

    if (CGCalendar::m_curEvent) {
        delete CGCalendar::m_curEvent;
    }

    void* mem = SMemAlloc(sizeof(CGCalendarEvent), __FILE__, __LINE__, 0);
    CGCalendar::m_curEvent = mem ? new (mem) CGCalendarEvent() : nullptr;

    if (!CGCalendar::m_curEvent) {
        return;
    }

    uint8_t sendType;
    msg->Get(sendType);
    int32_t status = -1;

    if (!CGCalendar::m_curEvent->ProcessGetEvent(msg, &status)) {
        return;
    }

    uint64_t eventID = CGCalendar::m_curEvent->m_eventID;

    if (!status && CGCalendar::m_curEvent->m_date > g_clientGameTime && !CGCalendar::HasPendingInvite(eventID)) {
        CalendarPendingInvite* invite = s_pendingInvites.NewNode(2, 0, 0);

        if (invite) {
            invite->m_eventID = eventID;
            CalendarDayEvent* event = s_eventTables.GetEvent((CGCalendar::m_curEvent->m_flags & 0x440) != 0, EncodeDate(CGCalendar::m_curEvent->m_date.m_month, CGCalendar::m_curEvent->m_date.m_monthDay, CGCalendar::m_curEvent->m_date.m_year), eventID);

            if (event) {
                event->m_flags |= 0x1;
                invite->m_inviterGuid = event->m_creatorGUID;
            }

            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
        }
    }

    if (sendType == 1 || sendType == 2) {
        if (CGCalendar::m_curEvent->m_pendingNames) {
            CGCalendar::m_curEvent->SetSignalType(sendType == 1 ? 2 : 3);
        } else {
            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_NEW_EVENT, "%b", sendType == 2);
        }

        return;
    }

    CGCalendar::m_selectedEvent.m_startTime = 0;
    CGCalendar::m_selectedEvent.m_endTime = 0;
    CGCalendar::m_selectedEvent.m_holidayID = 0;
    CGCalendar::m_selectedEvent.m_eventType = 0;
    CGCalendar::m_selectedEvent.m_eventID = eventID;
    WowTime::WowEncodeTime(CGCalendar::m_selectedEvent.m_startTime, &CGCalendar::m_curEvent->m_date);

    if (CGCalendar::m_curEvent->m_pendingNames) {
        CGCalendar::m_curEvent->SetSignalType(4);
    } else {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_OPEN_EVENT, "%s", CGCalendar::GetCalendarTypeString(CGCalendar::m_curEvent->m_flags));
    }
}

// VERIFY: aa310072 manual
// OFFSET: 0x005BF030
void CGCalendar::ProcessInvite(CDataStore* msg, uint8_t* clearPending) {
    uint32_t responseTime = 0;
    SmartGUID guid;
    *msg >> guid;
    uint64_t eventID;
    msg->Get(eventID);
    uint64_t inviteID;
    msg->Get(inviteID);
    uint8_t level;
    msg->Get(level);
    uint8_t status;
    msg->Get(status);
    uint8_t inviteType;
    msg->Get(inviteType);

    if (inviteType == 1) {
        msg->Get(responseTime);
    }

    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    if (inviteType == 1) {
        if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
            CalendarDayEvent* event = s_eventTables.GetEvent((CGCalendar::m_curEvent->m_flags & 0x440) != 0, EncodeDate(CGCalendar::m_curEvent->m_date.m_month, CGCalendar::m_curEvent->m_date.m_monthDay, CGCalendar::m_curEvent->m_date.m_year), eventID);

            if (event) {
                event->m_inviteID = inviteID;
                event->m_inviteStatus = status;
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
            }
        } else {
            for (CALENDAREVENTHASH* entry = s_eventTables.m_tables[1].m_fulllist.Head(); entry; entry = s_eventTables.m_tables[1].Next(entry)) {
                for (uint32_t i = 0; i < entry->m_events.Count(); i++) {
                    if (entry->m_events.m_data[i]->m_eventID == eventID) {
                        entry->m_events.m_data[i]->m_inviteID = inviteID;
                        entry->m_events.m_data[i]->m_inviteStatus = status;
                        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
                        return;
                    }
                }
            }
        }
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        CGCalendar::m_curEvent->AddInvite(inviteID, guid, level, status, inviteType, responseTime);
    }
}

// VERIFY: fd09c36e manual
// OFFSET: 0x005BF250
void CGCalendar::ProcessInviteRemoved(CDataStore* msg, uint8_t* clearPending) {
    SmartGUID guid;
    *msg >> guid;
    uint64_t eventID;
    msg->Get(eventID);
    uint32_t flags;
    msg->Get(flags);
    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    if (flags & 0x400) {
        if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
            CalendarDayEvent* event = s_eventTables.GetEvent((CGCalendar::m_curEvent->m_flags & 0x440) != 0, EncodeDate(CGCalendar::m_curEvent->m_date.m_month, CGCalendar::m_curEvent->m_date.m_monthDay, CGCalendar::m_curEvent->m_date.m_year), eventID);

            if (event) {
                event->m_inviteID = 0;
                event->m_inviteStatus = 7;
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
            }
        } else {
            for (CALENDAREVENTHASH* entry = s_eventTables.m_tables[1].m_fulllist.Head(); entry; entry = s_eventTables.m_tables[1].Next(entry)) {
                for (uint32_t i = 0; i < entry->m_events.Count(); i++) {
                    if (entry->m_events.m_data[i]->m_eventID == eventID) {
                        entry->m_events.m_data[i]->m_inviteID = 0;
                        entry->m_events.m_data[i]->m_inviteStatus = 7;
                        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
                        return;
                    }
                }
            }
        }
    }

    if (CGCalendar::m_curEvent && CGCalendar::m_curEvent->m_eventID == eventID) {
        CGCalendar::m_curEvent->InviteRemoved(guid);
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_INVITE_LIST, nullptr);
    }
}

// VERIFY: d7b01422 manual
// OFFSET: 0x005BF420
void CGCalendar::ProcessInviteStatus(CDataStore* msg, uint8_t* clearPending) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        msg->m_read = msg->m_size;
        return;
    }

    SmartGUID guid;
    *msg >> guid;
    uint64_t eventID;
    msg->Get(eventID);
    uint32_t date;
    msg->Get(date);
    uint32_t flags;
    msg->Get(flags);
    uint8_t status;
    msg->Get(status);
    uint8_t clear;
    msg->Get(clear);
    uint32_t responseTime;
    msg->Get(responseTime);

    if (!clear) {
        *clearPending = 0;
    }

    CalendarDayEvent* event = s_eventTables.GetEvent((flags & 0x440) != 0, CGCalendar::GetDateKey(date), eventID);

    if (!event) {
        return;
    }

    if (guid == player->m_obj->m_guid) {
        event->m_inviteStatus = status;

        if (status) {
            if (CGCalendar::DeletePendingInvite(eventID)) {
                event->m_flags &= ~0x1;
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
            }
        } else {
            CGCalendar::AddPendingInvite(event);
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        CGCalendar::m_curEvent->UpdateInviteStatus(guid, status, responseTime);
    }
}

// VERIFY: dd6e1abd manual
// OFFSET: 0x005BF590
void CGCalendar::ProcessInviteStatusAlert(CDataStore* msg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        msg->m_read = msg->m_size;
        return;
    }

    uint64_t eventID;
    msg->Get(eventID);
    uint32_t date;
    msg->Get(date);
    uint32_t flags;
    msg->Get(flags);
    uint8_t status;
    msg->Get(status);

    CalendarDayEvent* event = s_eventTables.GetEvent((flags & 0x440) != 0, CGCalendar::GetDateKey(date), eventID);

    if (!event) {
        if (!CGCalendar::m_calendarReceived) {
            CGCalendar::QueryPendingInvites();
        }

        return;
    }

    event->m_inviteStatus = status;

    if (!(event->m_type & 0x10)) {
        if (status) {
            if (CGCalendar::DeletePendingInvite(eventID)) {
                event->m_flags &= ~0x1;
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
            }
        } else if (s_pendingInvites.NewNode(2, 0, 0)) {
            CGCalendar::AddPendingInvite(event);
        }
    }

    if (CGCalendar::m_curEvent && CGCalendar::m_curEvent->m_eventID == eventID) {
        CGCalendar::m_curEvent->UpdateInviteStatus(player->m_obj->m_guid, status, 0);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
}

// VERIFY: 10b40153 manual
// OFFSET: 0x005BF6F0
void CGCalendar::SetInviteFlag(uint64_t eventID, uint32_t flag, bool set) {
    CalendarDayEvent* event = nullptr;

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        uint32_t flags = CGCalendar::m_curEvent->m_flags;
        WowTime date;
        date.m_month = CGCalendar::m_curEvent->m_date.m_month;
        date.m_monthDay = CGCalendar::m_curEvent->m_date.m_monthDay;
        date.m_year = CGCalendar::m_curEvent->m_date.m_year;
        uint32_t key;
        WowTime::WowEncodeTime(key, &date);
        event = s_eventTables.GetEvent((flags & 0x440) != 0, key, eventID);
    } else {
        for (CALENDAREVENTHASH* entry = s_eventTables.m_tables[0].m_fulllist.Head(); entry && !event; entry = s_eventTables.m_tables[0].Next(entry)) {
            for (uint32_t i = 0; i < entry->m_events.Count(); i++) {
                if (entry->m_events.m_data[i]->m_eventID == eventID) {
                    event = entry->m_events.m_data[i];
                    break;
                }
            }
        }
    }

    if (!event) {
        return;
    }

    if (set) {
        event->m_flags |= flag;
    } else {
        event->m_flags &= ~flag;
    }
}

// VERIFY: f9d8441a auto
// OFFSET: 0x005BFA90
DYNAMICHOLIDAYHASH* CalendarGetHolidayInfo(int32_t holidayId) {
    return s_dynamicHolidays.Ptr(holidayId, HASHKEY_NONE());
}

// VERIFY: ebd51fc1 manual
// OFFSET: 0x005C07E0
void CGCalendar::ProcessLockoutRemoved(CDataStore* msg) {
    int32_t mapID = -1;
    int32_t difficulty = 0;
    int32_t resetTime = 0;
    uint64_t instanceID = 0;
    msg->Get(reinterpret_cast<uint32_t&>(mapID));

    if (mapID < 0) {
        for (CALENDAREVENTHASH* entry = s_eventTables.m_tables[2].m_fulllist.Head(); entry; entry = s_eventTables.m_tables[2].Next(entry)) {
            uint32_t count = entry->m_events.Count();

            if (!count) {
                continue;
            }

            CalendarDayEvent** keep = static_cast<CalendarDayEvent**>(alloca(count * sizeof(CalendarDayEvent*)));
            uint32_t kept = 0;

            for (uint32_t i = 0; i < count; i++) {
                CalendarDayEvent* event = entry->m_events.m_data[i];

                if (event->m_type & 0x80) {
                    delete event;
                } else {
                    keep[kept++] = event;
                }
            }

            if (kept < count) {
                for (uint32_t i = 0; i < kept; i++) {
                    entry->m_events.m_data[i] = keep[i];
                }

                entry->m_events.SetCount(kept);
            }
        }
    } else {
        msg->Get(reinterpret_cast<uint32_t&>(difficulty));
        msg->Get(reinterpret_cast<uint32_t&>(resetTime));
        msg->Get(instanceID);
        WowTime reset = g_clientGameTime;
        reset.m_tzHours = g_clientGameTime.m_tzHours;
        reset.AddMinutes(resetTime / 60);
        WowTime key;
        key.m_month = reset.m_month;
        key.m_monthDay = reset.m_monthDay;
        key.m_year = reset.m_year;
        uint32_t hash;
        WowTime::WowEncodeTime(hash, &key);
        CALENDAREVENTHASH* entry = s_eventTables.m_tables[2].Ptr(hash, HASHKEY_NONE());

        if (entry) {
            uint32_t count = entry->m_events.Count();

            if (count) {
                CalendarDayEvent** keep = static_cast<CalendarDayEvent**>(alloca(count * sizeof(CalendarDayEvent*)));
                uint32_t kept = 0;

                for (uint32_t i = 0; i < count; i++) {
                    CalendarDayEvent* event = entry->m_events.m_data[i];

                    if ((event->m_type & 0x80) && event->m_holidayID == static_cast<uint32_t>(mapID) && (difficulty == -1 || event->m_endTime == static_cast<uint32_t>(difficulty)) && (instanceID == ~0ull || event->m_eventID == instanceID)) {
                        delete event;
                    } else {
                        keep[kept++] = event;
                    }
                }

                if (kept < count) {
                    for (uint32_t i = 0; i < kept; i++) {
                        entry->m_events.m_data[i] = keep[i];
                    }

                    entry->m_events.SetCount(kept);
                }
            }
        }
    }

    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: 5cf06944 manual
// OFFSET: 0x005C0AE0
void CGCalendar::ProcessInviteRemovedAlert(CDataStore* msg) {
    uint64_t eventID;
    msg->Get(eventID);
    uint32_t date;
    msg->Get(date);
    uint32_t flags;
    msg->Get(flags);
    uint8_t status;
    msg->Get(status);

    bool remove = true;
    uint32_t key = CGCalendar::GetDateKey(date);
    CalendarDayEvent* event = s_eventTables.GetEvent((flags & 0x440) != 0, key, eventID);

    if (event && (event->m_type & 0x400) && (event->m_flags & 0x8)) {
        event->m_inviteStatus = 7;

        if (CGCalendar::m_curEvent && CGCalendar::m_curEvent->m_eventID == eventID) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player) {
                CGCalendar::m_curEvent->InviteRemoved(player->m_obj->m_guid);
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_INVITE_LIST, nullptr);
            }
        }

        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
        remove = false;
    } else {
        if (CGCalendar::m_selectedEvent.m_eventID == eventID) {
            CGCalendar::m_selectedEvent.m_eventID = 0;
            CGCalendar::m_selectedEvent.m_startTime = 0;
            CGCalendar::m_selectedEvent.m_endTime = 0;
            CGCalendar::m_selectedEvent.m_holidayID = 0;
            CGCalendar::m_selectedEvent.m_eventType = 0;
        }

        if (CGCalendar::m_curEvent && CGCalendar::m_curEvent->m_eventID == eventID) {
            delete CGCalendar::m_curEvent;
            CGCalendar::m_curEvent = nullptr;
            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_CLOSE_EVENT, nullptr);
        }

        if (CGCalendar::m_contextEvent.m_eventID == eventID) {
            CGCalendar::m_contextEvent.m_eventID = 0;
            CGCalendar::m_contextEvent.m_startTime = 0;
            CGCalendar::m_contextEvent.m_endTime = 0;
            CGCalendar::m_contextEvent.m_holidayID = 0;
            CGCalendar::m_contextEvent.m_eventType = 0;
            CGCalendar::m_contextEventInfo = nullptr;
        }
    }

    if (status && status != 9) {
        CGMailInfo::ReceivedMail(0.0f);
    }

    CGCalendar::DeletePendingInvite(eventID);
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);

    if (remove && s_eventTables.Delete((flags & 0x440) != 0, key, eventID)) {
        CGCalendar::UpdateDisplayList(true);
    }
}

// VERIFY: 6eb2e64f manual
// OFFSET: 0x005C0CF0
void CGCalendar::ProcessEventRemovedAlert(CDataStore* msg, uint8_t* clearPending) {
    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    uint64_t eventID;
    msg->Get(eventID);
    uint32_t date;
    msg->Get(date);

    if (CGCalendar::m_selectedEvent.m_eventID == eventID) {
        CGCalendar::m_selectedEvent.m_eventID = 0;
        CGCalendar::m_selectedEvent.m_startTime = 0;
        CGCalendar::m_selectedEvent.m_endTime = 0;
        CGCalendar::m_selectedEvent.m_holidayID = 0;
        CGCalendar::m_selectedEvent.m_eventType = 0;
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        delete CGCalendar::m_curEvent;
        CGCalendar::m_curEvent = nullptr;
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_CLOSE_EVENT, nullptr);
    }

    if (CGCalendar::m_contextEvent.m_eventID == eventID) {
        CGCalendar::m_contextEvent.m_eventID = 0;
        CGCalendar::m_contextEvent.m_startTime = 0;
        CGCalendar::m_contextEvent.m_endTime = 0;
        CGCalendar::m_contextEvent.m_holidayID = 0;
        CGCalendar::m_contextEvent.m_eventType = 0;
        CGCalendar::m_contextEventInfo = nullptr;
    }

    if (s_eventClipboard.m_eventID == eventID) {
        memset(&s_eventClipboard, 0, sizeof(s_eventClipboard));
    }

    uint32_t key = CGCalendar::GetDateKey(date);
    int32_t table = 0;

    while (!s_eventTables.Delete(table, key, eventID)) {
        if (++table > 1) {
            return;
        }
    }

    CGCalendar::DeletePendingInvite(eventID);
    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: 4f00ed9f manual
// OFFSET: 0x005C0E70
void CGCalendar::ProcessInviteModeratorStatusAlert(CDataStore* msg, uint8_t* clearPending) {
    SmartGUID guid;
    *msg >> guid;
    uint64_t eventID;
    msg->Get(eventID);
    uint8_t status;
    msg->Get(status);
    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        CGCalendar::m_curEvent->UpdateInviteModeratorStatus(guid, status);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->m_obj->m_guid == guid) {
        CGCalendar::SetInviteFlag(eventID, 0x2, status == 1);
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST, nullptr);
    }
}

// VERIFY: b5246e45 manual
// OFFSET: 0x005C0F40
void CGCalendar::GuildChanged() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if (player->m_player->m_playerGuildid) {
        CGCalendar::GetCalendar();
        return;
    }

    s_eventTables.m_counts[1] = 0;
    s_eventTables.m_tables[1].InternalClear(0);

    if (s_eventClipboard.m_type & 0x440) {
        memset(&s_eventClipboard, 0, sizeof(s_eventClipboard));
    }

    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: cdc89b97 manual
// OFFSET: 0x005C2350
void CGCalendar::InsertLockoutEvent(int32_t mapID, int32_t difficulty, uint64_t instanceID, uint32_t time) {
    void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
    CalendarDayEvent* event = mem ? new (mem) CalendarDayEvent() : nullptr;
    event->m_holidayID = mapID;
    event->m_endTime = difficulty;
    event->m_eventID = instanceID;
    event->m_title[0] = '\0';
    event->m_startTime = time;
    event->m_type = 0x80;
    CGCalendar::InsertEvent(event, 2);
}

// VERIFY: c522fef8 manual
// OFFSET: 0x005C29C0
void PH_SMSG_CALENDAR_SEND_CALENDAR(CDataStore* msg) {
    CGCalendar::m_numInitialPending = 0;
    s_pendingInvites.Clear();

    uint32_t numInvites;
    msg->Get(numInvites);

    for (uint32_t i = 0; i < numInvites; i++) {
        uint64_t eventID;
        msg->Get(eventID);
        uint64_t inviteID;
        msg->Get(inviteID);
        uint8_t status;
        msg->Get(status);
        uint8_t rank;
        msg->Get(rank);
        uint8_t inviteType;
        msg->Get(inviteType);
        SmartGUID inviter;
        *msg >> inviter;

        CalendarPendingInvite* invite = s_pendingInvites.NewNode(2, 0, 0);

        if (invite) {
            invite->m_eventID = eventID;
            invite->m_inviteID = inviteID;
            invite->m_inviterGuid = inviter;
            invite->m_status = status;
            invite->m_rank = rank;
            invite->m_inviteType = inviteType;
            invite->m_valid = 0;
        }
    }

    s_eventTables.m_counts[0] = 0;
    s_eventTables.m_tables[0].InternalClear(0);
    s_eventTables.m_counts[1] = 0;
    s_eventTables.m_tables[1].InternalClear(0);

    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();
    uint32_t numEvents;
    msg->Get(numEvents);

    for (uint32_t i = 0; i < numEvents; i++) {
        CalendarDayEvent event;
        msg->Get(event.m_eventID);
        msg->GetString(event.m_title, sizeof(event.m_title));
        msg->Get(reinterpret_cast<uint32_t&>(event.m_eventType));
        msg->Get(event.m_startTime);
        event.m_endTime = 0;
        msg->Get(event.m_type);
        msg->Get(reinterpret_cast<uint32_t&>(event.m_textureID));
        SmartGUID creator;
        *msg >> creator;
        event.m_flags = (activePlayer && creator == activePlayer) ? 0x4 : 0;
        event.m_inviteID = 0;
        event.m_creatorGUID = WOWGUID();

        if (event.m_type & 0x40) {
            void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
            CalendarDayEvent* copy = mem ? new (mem) CalendarDayEvent() : nullptr;
            static_cast<CalendarEventData&>(*copy) = event;
            WowTime alarm;
            WowTime::WowDecodeTime(copy->m_startTime, &alarm);
            alarm.m_tzHours = 0;
            alarm.AddMinutes(-15);
            WowTime::WowEncodeTime(copy->m_alarmTime, &alarm);
            copy->m_alarmCallback = g_clientGameTime.GameTimeRegisterCallback(&alarm, FireEventAlarm, copy);
            copy->m_creatorGUID = creator;
            CGCalendar::InsertEvent(copy, (event.m_type & 0x440) != 0);
            continue;
        }

        CalendarPendingInvite* invite = s_pendingInvites.Head();

        while (invite && invite->m_eventID != event.m_eventID) {
            invite = s_pendingInvites.Next(invite);
        }

        if (invite) {
            event.m_inviteStatus = invite->m_status;
            event.m_inviteID = invite->m_inviteID;

            if (invite->m_rank) {
                event.m_flags |= 0x2;

                if (invite->m_rank == 2) {
                    event.m_flags |= 0x4;
                }
            }

            if (invite->m_inviteType == 1) {
                event.m_flags |= 0x8;
                event.m_creatorGUID = creator;
            } else {
                event.m_creatorGUID = invite->m_inviterGuid;
            }

            WowTime start;
            WowTime::WowDecodeTime(event.m_startTime, &start);
            start.m_tzHours = 0;

            if (!invite->m_status && start > g_clientGameTime && !(event.m_type & 0x10)) {
                event.m_flags |= 0x1;
                invite->m_valid = 1;
            } else {
                s_pendingInvites.DeleteNode(invite);
                numInvites--;
            }
        } else {
            if (!(event.m_type & 0x400)) {
                continue;
            }

            event.m_flags |= 0x8;
            event.m_creatorGUID = creator;
            event.m_inviteStatus = 7;
        }

        void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
        CalendarDayEvent* copy = mem ? new (mem) CalendarDayEvent() : nullptr;
        static_cast<CalendarEventData&>(*copy) = event;
        WowTime alarm;
        WowTime::WowDecodeTime(copy->m_startTime, &alarm);
        alarm.m_tzHours = 0;
        alarm.AddMinutes(-15);
        WowTime::WowEncodeTime(copy->m_alarmTime, &alarm);
        copy->m_alarmCallback = g_clientGameTime.GameTimeRegisterCallback(&alarm, FireEventAlarm, copy);
        CGCalendar::InsertEvent(copy, (event.m_type & 0x440) != 0);
        WOWGUID none = WOWGUID();
        g_nameDBCache.GetRecord(event.m_creatorGUID.m_guid, none, nullptr, nullptr, false);
    }

    CalendarPendingInvite* invite = s_pendingInvites.Head();

    while (invite) {
        CalendarPendingInvite* next = s_pendingInvites.Next(invite);

        if (!invite->m_valid || !invite->m_inviterGuid) {
            s_pendingInvites.DeleteNode(invite);
            numInvites--;
        } else if (FriendList::s_friendList->IsIgnored(invite->m_inviterGuid, false)) {
            numInvites--;
        }

        invite = next;
    }

    WowTime serverTime;
    uint32_t serverNow = 0;
    msg->Get(serverNow);
    uint32_t serverDate;
    msg->Get(serverDate);
    WowTime::WowDecodeTime(serverDate, &serverTime);

    s_eventTables.m_counts[2] = 0;
    s_eventTables.m_tables[2].InternalClear(0);

    uint32_t numLockouts;
    msg->Get(numLockouts);

    for (uint32_t i = 0; i < numLockouts; i++) {
        int32_t mapID;
        msg->Get(reinterpret_cast<uint32_t&>(mapID));
        int32_t difficulty;
        msg->Get(reinterpret_cast<uint32_t&>(difficulty));
        int32_t resetTime;
        msg->Get(reinterpret_cast<uint32_t&>(resetTime));
        uint64_t instanceID;
        msg->Get(instanceID);
        WowTime reset = serverTime;
        reset.m_tzHours = serverTime.m_tzHours;
        reset.AddMinutes(resetTime / 60);
        uint32_t time;
        WowTime::WowEncodeTime(time, &reset);
        CGCalendar::InsertLockoutEvent(mapID, difficulty, instanceID, time);
    }

    uint32_t baseTime = 0;
    msg->Get(baseTime);
    s_raidResets.Clear();
    uint32_t numResets;
    msg->Get(numResets);

    for (uint32_t i = 0; i < numResets; i++) {
        int32_t mapID;
        msg->Get(reinterpret_cast<uint32_t&>(mapID));
        uint32_t period;
        msg->Get(period);
        uint32_t offset;
        msg->Get(offset);

        CALENDARRAIDRESETNODE* node = s_raidResets.NewNode(2, 0, 0);

        if (node) {
            WowTime reset = serverTime;
            reset.m_tzHours = serverTime.m_tzHours;
            uint32_t start = baseTime + offset;
            uint32_t periods = static_cast<uint32_t>(static_cast<float>(serverNow - start) / static_cast<float>(period));
            reset.AddMinutes(static_cast<int32_t>(start + periods * period - serverNow) / 60);
            uint32_t time;
            WowTime::WowEncodeTime(time, &reset);
            node->m_mapID = mapID;
            node->m_interval = period / 60;
            node->m_startTime = time;
        }
    }

    uint32_t numHolidays;
    msg->Get(numHolidays);

    if (numHolidays) {
        s_dynamicHolidays.InternalClear(0);
        s_holidays.Clear();

        for (uint32_t i = 0; i < numHolidays; i++) {
            uint32_t id;
            msg->Get(id);
            int32_t region;
            msg->Get(reinterpret_cast<uint32_t&>(region));
            int32_t looping;
            msg->Get(reinterpret_cast<uint32_t&>(looping));
            int32_t priority;
            msg->Get(reinterpret_cast<uint32_t&>(priority));
            int32_t filterType;
            msg->Get(reinterpret_cast<uint32_t&>(filterType));

            int32_t dates[26];

            for (uint32_t j = 0; j < 26; j++) {
                msg->Get(reinterpret_cast<uint32_t&>(dates[j]));
            }

            int32_t durations[10];

            for (uint32_t j = 0; j < 10; j++) {
                msg->Get(reinterpret_cast<uint32_t&>(durations[j]));
            }

            int32_t flags[10];

            for (uint32_t j = 0; j < 10; j++) {
                msg->Get(reinterpret_cast<uint32_t&>(flags[j]));
            }

            char textureFileName[64];
            msg->GetString(textureFileName, sizeof(textureFileName));

            DYNAMICHOLIDAYHASH* holiday = s_dynamicHolidays.New(id, HASHKEY_NONE(), 0, 0);
            holiday->m_region = region;
            holiday->m_looping = looping;
            holiday->m_priority = priority;
            holiday->m_calendarFilterType = filterType;

            for (uint32_t j = 0; j < 26; j++) {
                holiday->m_date[j] = dates[j];
            }

            for (uint32_t j = 0; j < 10; j++) {
                holiday->m_duration[j] = durations[j];
            }

            for (uint32_t j = 0; j < 10; j++) {
                holiday->m_calendarFlags[j] = flags[j];
            }

            SStrCopy(holiday->m_textureFileName, textureFileName, sizeof(holiday->m_textureFileName));
        }
    }

    CGCalendar::CalculateEvents(true);

    if (numInvites) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
    }

    CGCalendar::m_calendarReceived = 1;
}

// VERIFY: b0bed908 auto
// OFFSET: 0x005C37D0
void CGCalendar::ProcessLockoutAdded(CDataStore* msg) {
    WowTime date;
    uint32_t time;
    msg->Get(time);
    WowTime::WowDecodeTime(time, &date);
    int32_t mapID;
    msg->Get(reinterpret_cast<uint32_t&>(mapID));
    int32_t difficulty;
    msg->Get(reinterpret_cast<uint32_t&>(difficulty));
    int32_t resetTime;
    msg->Get(reinterpret_cast<uint32_t&>(resetTime));
    uint64_t instanceID;
    msg->Get(instanceID);
    date.AddMinutes(resetTime / 60);
    WowTime::WowEncodeTime(time, &date);
    CGCalendar::InsertLockoutEvent(mapID, difficulty, instanceID, time);
    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: fa479f46 manual
// OFFSET: 0x005C3880
void CGCalendar::ProcessLockoutUpdated(CDataStore* msg) {
    int32_t mapID = -1;
    int32_t difficulty = 0;
    int32_t oldReset = 0;
    int32_t newReset = 0;
    WowTime oldDate;
    uint32_t time;
    msg->Get(time);
    WowTime::WowDecodeTime(time, &oldDate);
    WowTime newDate = oldDate;
    newDate.m_tzHours = oldDate.m_tzHours;
    msg->Get(reinterpret_cast<uint32_t&>(mapID));
    msg->Get(reinterpret_cast<uint32_t&>(difficulty));
    msg->Get(reinterpret_cast<uint32_t&>(oldReset));
    msg->Get(reinterpret_cast<uint32_t&>(newReset));
    oldDate.AddMinutes(oldReset / 60);
    newDate.AddMinutes(newReset / 60);

    WowTime key;
    key.m_month = oldDate.m_month;
    key.m_monthDay = oldDate.m_monthDay;
    key.m_year = oldDate.m_year;
    uint32_t hash;
    WowTime::WowEncodeTime(hash, &key);
    CALENDAREVENTHASH* entry = s_eventTables.m_tables[2].Ptr(hash, HASHKEY_NONE());

    if (entry) {
        uint32_t count = entry->m_events.Count();

        if (count) {
            CalendarDayEvent** keep = static_cast<CalendarDayEvent**>(alloca(count * sizeof(CalendarDayEvent*)));
            uint32_t kept = 0;

            for (uint32_t i = 0; i < count; i++) {
                CalendarDayEvent* event = entry->m_events.m_data[i];

                if ((event->m_type & 0x80) && event->m_holidayID == static_cast<uint32_t>(mapID) && event->m_endTime == static_cast<uint32_t>(difficulty) && EncodeDate(newDate.m_month, newDate.m_monthDay, newDate.m_year) != hash) {
                    WowTime::WowEncodeTime(time, &newDate);
                    event->m_startTime = time;
                    CGCalendar::InsertEvent(event, 2);
                } else {
                    keep[kept++] = event;
                }
            }

            if (kept < count) {
                for (uint32_t i = 0; i < kept; i++) {
                    entry->m_events.m_data[i] = keep[i];
                }

                entry->m_events.SetCount(kept);
            }
        }
    }

    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: 266adeae manual
// OFFSET: 0x005C3AB0
void CGCalendar::ProcessInviteAlert(CDataStore* msg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
    CalendarDayEvent* event = mem ? new (mem) CalendarDayEvent() : nullptr;
    event->m_flags = 0;
    msg->Get(event->m_eventID);
    msg->GetString(event->m_title, sizeof(event->m_title));
    msg->Get(event->m_startTime);
    event->m_endTime = 0;
    msg->Get(event->m_type);
    msg->Get(reinterpret_cast<uint32_t&>(event->m_eventType));
    msg->Get(reinterpret_cast<uint32_t&>(event->m_textureID));
    msg->Get(event->m_inviteID);
    msg->Get(event->m_inviteStatus);
    uint8_t rank;
    msg->Get(rank);
    SmartGUID creator;
    *msg >> creator;
    SmartGUID inviter;
    *msg >> inviter;
    event->ScheduleAlarm();

    bool pending = false;

    if (event->m_inviteStatus == 7 || event->m_inviteStatus == 6) {
        event->m_creatorGUID = creator;
        event->m_flags = 0x8;
    } else {
        event->m_creatorGUID = inviter;

        if (!event->m_inviteStatus && !(event->m_type & 0x40)) {
            pending = true;
            event->m_flags = 0x1;
        }
    }

    if (rank) {
        event->m_flags |= 0x2;
    }

    if (creator == player->m_obj->m_guid || rank == 2) {
        event->m_flags |= 0x4;
    }

    CGCalendar::InsertEvent(event, (event->m_type & 0x440) != 0);

    if (pending) {
        CalendarPendingInvite* invite = s_pendingInvites.NewNode(2, 0, 0);

        if (invite) {
            invite->m_eventID = event->m_eventID;
            invite->m_inviterGuid = event->m_creatorGUID;

            if (!FriendList::s_friendList->IsIgnored(event->m_creatorGUID, false)) {
                FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
            }
        }
    }

    CGCalendar::UpdateDisplayList(true);
}

// VERIFY: 00a05b2b manual
// OFFSET: 0x005C3D30
void CGCalendar::ProcessEventUpdatedAlert(CDataStore* msg, uint8_t* clearPending) {
    uint8_t clear;
    msg->Get(clear);

    if (!clear) {
        *clearPending = 0;
    }

    uint64_t eventID;
    msg->Get(eventID);
    uint32_t oldDate;
    msg->Get(oldDate);
    uint32_t read = msg->m_read;
    uint32_t flags;
    msg->Get(flags);
    uint32_t newDate;
    msg->Get(newDate);
    uint8_t eventType;
    msg->Get(eventType);
    uint32_t textureID;
    msg->Get(textureID);
    char title[128];
    msg->GetString(title, sizeof(title));

    int32_t table = (flags & 0x440) != 0;
    uint32_t key = CGCalendar::GetDateKey(oldDate);
    CalendarDayEvent* event = s_eventTables.GetEvent(table, key, eventID);

    if (event) {
        bool locked = (event->m_type & 0x10) != 0;

        if (oldDate != newDate) {
            void* mem = SMemAlloc(sizeof(CalendarDayEvent), __FILE__, __LINE__, 0);
            CalendarDayEvent* moved = mem ? new (mem) CalendarDayEvent() : nullptr;
            moved->m_eventID = eventID;
            moved->m_startTime = newDate;
            moved->m_endTime = 0;
            moved->m_flags = event->m_flags;
            moved->m_inviteStatus = event->m_inviteStatus;
            moved->m_inviteID = event->m_inviteID;
            moved->m_creatorGUID = event->m_creatorGUID;
            moved->m_sequenceIndex = event->m_sequenceIndex;
            moved->m_sequenceLength = event->m_sequenceLength;
            moved->ScheduleAlarm();
            CGCalendar::InsertEvent(moved, table);
            s_eventTables.Delete(table, key, eventID);

            if (CGCalendar::m_contextEventInfo == event) {
                CGCalendar::m_contextEventInfo = moved;
            }

            event = moved;
        }

        SStrCopy(event->m_title, title, sizeof(event->m_title));
        event->m_type = flags;
        event->m_eventType = eventType;
        event->m_textureID = textureID;

        if (locked != ((flags & 0x10) != 0) && !event->m_inviteStatus) {
            if (flags & 0x10) {
                if (CGCalendar::DeletePendingInvite(eventID)) {
                    event->m_flags &= ~0x1;
                    FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_PENDING_INVITES, nullptr);
                }
            } else {
                CGCalendar::AddPendingInvite(event);
            }
        }

        CGCalendar::UpdateDisplayList(true);
    } else if (!CGCalendar::m_calendarReceived) {
        CGCalendar::QueryPendingInvites();
    }

    if (CGCalendar::m_curEvent && eventID == CGCalendar::m_curEvent->m_eventID) {
        msg->m_read = read;

        if (CGCalendar::m_curEvent->ProcessUpdateEvent(msg)) {
            FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT, nullptr);
        }
    } else {
        msg->m_read = msg->m_size;
    }
}

// VERIFY: 8e83d321 auto
// OFFSET: 0x005EEB70
void CGCalendar::LeaveWorld() {
}
