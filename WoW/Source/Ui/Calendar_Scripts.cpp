#include "Calendar.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Game/GameTime.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Common/WowTime.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/CalendarEvent.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/HolidaysRec.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/Object.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"

// STATIC: 0x00ACFFB4
static const char* s_monthNames[12] = { "MONTH_JANUARY", "MONTH_FEBRUARY", "MONTH_MARCH", "MONTH_APRIL", "MONTH_MAY", "MONTH_JUNE", "MONTH_JULY", "MONTH_AUGUST", "MONTH_SEPTEMBER", "MONTH_OCTOBER", "MONTH_NOVEMBER", "MONTH_DECEMBER" };

// STATIC: 0x00AD0014
const char* s_weekdayNames[7] = { "WEEKDAY_SUNDAY", "WEEKDAY_MONDAY", "WEEKDAY_TUESDAY", "WEEKDAY_WEDNESDAY", "WEEKDAY_THURSDAY", "WEEKDAY_FRIDAY", "WEEKDAY_SATURDAY" };

// VERIFY: 98948d66 auto
// OFFSET: 0x005B8C10
static int32_t Script_CalendarIsActionPending(lua_State* L) {
    lua_pushboolean(L, CGCalendar::m_actionPending);
    return 1;
}

// VERIFY: 9224a39b auto
// OFFSET: 0x005B80E0
static int32_t Script_CalendarGetMonthNames(lua_State* L) {
    lua_checkstack(L, 12);

    for (uint32_t i = 0; i < 12; i++) {
        lua_pushstring(L, FrameScript_GetText(s_monthNames[i], -1, GENDER_NOT_APPLICABLE));
    }

    return 12;
}

// VERIFY: 735dbaf4 auto
// OFFSET: 0x005B8120
static int32_t Script_CalendarGetWeekdayNames(lua_State* L) {
    lua_checkstack(L, 7);

    for (uint32_t i = 0; i < 7; i++) {
        lua_pushstring(L, FrameScript_GetText(s_weekdayNames[i], -1, GENDER_NOT_APPLICABLE));
    }

    return 7;
}

// VERIFY: 5afc3ec9 auto
// OFFSET: 0x005B8160
static int32_t Script_CalendarGetDate(lua_State* L) {
    lua_pushnumber(L, g_clientGameTime.m_weekDay + 1);
    lua_pushnumber(L, g_clientGameTime.m_month + 1);
    lua_pushnumber(L, g_clientGameTime.m_monthDay + 1);
    lua_pushnumber(L, g_clientGameTime.m_year + 2000);
    return 4;
}

// VERIFY: 095feebb manual
// OFFSET: 0x005BF830
static int32_t Script_CalendarGetNumPendingInvites(lua_State* L) {
    lua_pushnumber(L, CGCalendar::CountPendingInvites());
    return 1;
}

// STATIC: 0x00A18684
static const int32_t s_expansionMaxLevels[3] = { 60, 70, 80 };

// VERIFY: 93df37f7 auto
// OFFSET: 0x005B81F0
static int32_t Script_CalendarGetMinDate(lua_State* L) {
    WowTime date;
    date.m_month = 10;
    date.m_monthDay = 23;
    date.m_year = 4;
    date.AddDays(0, false);
    date.SetHourAndMinutes(0u, 0u);
    lua_pushnumber(L, date.m_weekDay + 1);
    lua_pushnumber(L, date.m_month + 1);
    lua_pushnumber(L, date.m_monthDay + 1);
    lua_pushnumber(L, date.m_year + 2000);
    return 4;
}

// VERIFY: 7d9608cb auto
// OFFSET: 0x005B84A0
static int32_t Script_CalendarCloseEvent(lua_State* L) {
    CGCalendar::m_selectedEvent.m_eventID = 0;
    CGCalendar::m_selectedEvent.m_startTime = 0;
    CGCalendar::m_selectedEvent.m_endTime = 0;
    CGCalendar::m_selectedEvent.m_holidayID = 0;
    CGCalendar::m_selectedEvent.m_eventType = 0;
    return 0;
}

// VERIFY: e71b5505 auto
// OFFSET: 0x005B8970
static int32_t Script_CalendarEventGetTypes(lua_State* L) {
    lua_checkstack(L, 5);

    for (uint32_t i = 0; i < 5; i++) {
        lua_pushstring(L, FrameScript_GetText(GetEventTypeString(i), -1, GENDER_NOT_APPLICABLE));
    }

    return 5;
}

// VERIFY: a996e5a9 auto
// OFFSET: 0x005B89C0
static int32_t Script_CalendarEventGetRepeatOptions(lua_State* L) {
    lua_checkstack(L, 4);

    for (uint32_t i = 0; i < 4; i++) {
        lua_pushstring(L, FrameScript_GetText(GetEventRepeatString(i), -1, GENDER_NOT_APPLICABLE));
    }

    return 4;
}

// VERIFY: 772e0e5f manual
// OFFSET: 0x005B99F0
static int32_t Script_CalendarGetMonth(lua_State* L) {
    int32_t offset = 0;

    if (lua_isnumber(L, 1)) {
        offset = static_cast<int32_t>(lua_tonumber(L, 1));
    }

    int32_t month = 0;
    int32_t year = 0;
    int32_t days = 0;
    int32_t weekDay = 0;
    CGCalendar::GetMonth(offset, &month, &year, &days, &weekDay);
    lua_pushnumber(L, ++month);
    year += 2000;
    lua_pushnumber(L, year);
    lua_pushnumber(L, static_cast<uint32_t>(days));
    lua_pushnumber(L, weekDay + 1);
    return 4;
}

// VERIFY: 52664e31 auto
// OFFSET: 0x005B9F80
static int32_t Script_CalendarGetEventIndex(lua_State* L) {
    int32_t monthOffset;
    int32_t day;
    int32_t index;

    if (CGCalendar::ComputeEventIndex(&CGCalendar::m_selectedEvent, &monthOffset, &day, &index)) {
        lua_pushinteger(L, monthOffset);
        lua_pushinteger(L, day + 1);
        lua_pushinteger(L, index + 1);
    } else {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    }

    return 3;
}

// VERIFY: c93d6dde auto
// OFFSET: 0x005BA680
static int32_t Script_CalendarContextGetEventIndex(lua_State* L) {
    int32_t monthOffset;
    int32_t day;
    int32_t index;

    if (CGCalendar::ComputeEventIndex(&CGCalendar::m_contextEvent, &monthOffset, &day, &index)) {
        lua_pushinteger(L, monthOffset);
        lua_pushinteger(L, day + 1);
        lua_pushinteger(L, index + 1);
    } else {
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
        lua_pushinteger(L, 0);
    }

    return 3;
}

// VERIFY: 2581947b manual
// OFFSET: 0x005BCDA0
static int32_t Script_CalendarDefaultGuildFilter(lua_State* L) {
    if (ClntObjMgrGetActivePlayerObj()) {
        double level = s_expansionMaxLevels[ClientServices::Connection()->m_accountExpansion];
        lua_pushnumber(L, level);
        lua_pushnumber(L, level);
        lua_pushnumber(L, CGGuildInfo::m_numRanks);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 3;
}

// VERIFY: b5cb7930 manual
// OFFSET: 0x005C43B0
static int32_t Script_CalendarSetAbsMonth(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarSetAbsMonth(month[, year])");
        return 0;
    }

    int32_t month = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t year = CGCalendar::m_absYear;

    if (CGCalendar::m_absYear < 0) {
        year = g_clientGameTime.m_year;
        CGCalendar::m_absYear = year;

        if (year < 0) {
            year = 4;
            CGCalendar::m_absYear = 4;
        }
    }

    if (lua_isnumber(L, 2)) {
        year = static_cast<int32_t>(lua_tonumber(L, 2)) - 2000;
    }

    int32_t weekDay;
    CGCalendar::ClampAbsMonthDay(month, year, &CGCalendar::m_absMonth, &CGCalendar::m_absYear, &CGCalendar::m_absMonthDays, &weekDay);
    CGCalendar::CalculateEvents(true);
    return 0;
}

// VERIFY: 90f38e2c auto
// OFFSET: 0x005B8410
static int32_t Script_CalendarGetMaxCreateDate(lua_State* L) {
    WowTime date;
    GameCalendarGetMaxCreateDate(&date);
    lua_pushnumber(L, date.m_weekDay + 1);
    lua_pushnumber(L, date.m_month + 1);
    lua_pushnumber(L, date.m_monthDay + 1);
    lua_pushnumber(L, date.m_year + 2000);
    return 4;
}

// VERIFY: 6bc9ca17 auto
// OFFSET: 0x005B8B30
static int32_t Script_CalendarCanSendInvite(lua_State* L) {
    uint32_t now;

    if (CGCalendar::m_actionPending || ((!unk_BD09AC || !unk_BD09AC->m_intValue) && (now = OsGetAsyncTimeMs(), CGCalendar::m_lastInviteTime) && now - CGCalendar::m_lastInviteTime < 2000)) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, 1);
    }

    return 1;
}

// VERIFY: 76d1ef9d auto
// OFFSET: 0x005BCC00
static int32_t Script_CalendarSetMonth(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarSetMonth(offsetMonths)");
        return 0;
    }

    int32_t weekDay;
    CGCalendar::GetMonth(static_cast<int32_t>(lua_tonumber(L, 1)), &CGCalendar::m_absMonth, &CGCalendar::m_absYear, &CGCalendar::m_absMonthDays, &weekDay);
    CGCalendar::CalculateEvents(true);
    return 0;
}

// VERIFY: 5443ddcb manual
// OFFSET: 0x005B9BF0
static int32_t Script_CalendarGetNumDayEvents(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CalendarGetNumDayEvents([-1,0,1], monthDay)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    lua_pushnumber(L, CGCalendar::GetNumDayEvents(offset, day));
    return 1;
}

// VERIFY: 52f86bfc auto
// OFFSET: 0x005B9E10
static int32_t Script_CalendarGetFirstPendingInvite(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CalendarGetDay([-1,0,1], monthDay)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    lua_pushinteger(L, CGCalendar::GetFirstPendingInviteIndex(offset, day) + 1);
    return 1;
}

// VERIFY: 6931d0c9 auto
// OFFSET: 0x005B78D0
static void NameCacheCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID key = WOWGUID();

    if (g_nameDBCache.GetRecord(guid.m_guid, key, nullptr, nullptr, false)) {
        Script_SendUniqueSignal(SCRIPT_EVENT_CALENDAR_UPDATE_EVENT_LIST);
    }
}

// VERIFY: 38875a8f auto
// OFFSET: 0x005B8040
static const char* GetSequenceLabel(uint32_t type, uint32_t index, uint32_t length) {
    if (!(type & 0x8) || length <= 1) {
        return "";
    }

    if (!index) {
        return "START";
    }

    if (index == length - 1) {
        return "END";
    }

    return "ONGOING";
}

// VERIFY: 876fe382 auto
// OFFSET: 0x005B8080
static const char* GetInviteRoleLabel(uint32_t flags) {
    if (flags & 0x4) {
        return "CREATOR";
    }

    if (flags & 0x2) {
        return "MODERATOR";
    }

    return "";
}

// VERIFY: 38d50b7e auto
// OFFSET: 0x005B80B0
static bool IsSignUpInvite(const CalendarDayEvent* event) {
    return (event->m_type & 0x400) && (event->m_flags & 0x8);
}

// VERIFY: cba45807 auto
// OFFSET: 0x005B82A0
static int32_t Script_CalendarGetMaxDate(lua_State* L) {
    WowTime date;
    date.m_month = 0;
    date.m_monthDay = 0;
    date.m_year = 31;
    date.AddDays(-1, false);
    date.SetHourAndMinutes(23u, 59u);
    lua_pushnumber(L, date.m_weekDay + 1);
    lua_pushnumber(L, date.m_month + 1);
    lua_pushnumber(L, date.m_monthDay + 1);
    lua_pushnumber(L, date.m_year + 2000);
    return 4;
}

// VERIFY: da43e929 auto
// OFFSET: 0x005B8350
static int32_t Script_CalendarGetMinHistoryDate(lua_State* L) {
    WowTime date;
    date.m_weekDay = g_clientGameTime.m_weekDay;
    date.m_month = g_clientGameTime.m_month;
    date.m_monthDay = g_clientGameTime.m_monthDay;
    date.m_year = g_clientGameTime.m_year;
    date.AddDays(-14, false);
    date.SetHourAndMinutes(0u, 0u);
    lua_pushnumber(L, date.m_weekDay + 1);
    lua_pushnumber(L, date.m_month + 1);
    lua_pushnumber(L, date.m_monthDay + 1);
    lua_pushnumber(L, date.m_year + 2000);
    return 4;
}

// VERIFY: e9039fb7 manual
// OFFSET: 0x005B84D0
static int32_t Script_CalendarEventGetNumInvites(lua_State* L) {
    lua_pushnumber(L, CGCalendar::m_curEvent ? CGCalendar::m_curEvent->m_numInvites : 0);
    return 1;
}

// VERIFY: ebc8d474 auto
// OFFSET: 0x005B8510
static int32_t Script_CalendarContextDeselectEvent(lua_State* L) {
    CGCalendar::m_contextEvent.m_eventID = 0;
    CGCalendar::m_contextEvent.m_startTime = 0;
    CGCalendar::m_contextEvent.m_endTime = 0;
    CGCalendar::m_contextEvent.m_holidayID = 0;
    CGCalendar::m_contextEvent.m_eventType = 0;
    CGCalendar::m_contextEventInfo = nullptr;
    return 0;
}

// VERIFY: 7a8c4b46 manual
// OFFSET: 0x005B8540
static int32_t Script_CalendarContextEventClipboard(lua_State* L) {
    lua_pushboolean(L, s_eventClipboard.m_eventID != 0);
    return 1;
}

// VERIFY: ff6674b8 auto
// OFFSET: 0x005B8570
static int32_t Script_CalendarEventSortInvites(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: CalendarEventSortInvites(\"criteria\", reverse)");
        return 0;
    }

    uint32_t criterion = 3;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(name, "name", STORM_MAX_STR)) {
        criterion = 0;
    } else if (!SStrCmpI(name, "level", STORM_MAX_STR)) {
        criterion = 1;
    } else if (!SStrCmpI(name, "class", STORM_MAX_STR)) {
        criterion = 2;
    } else if (!SStrCmpI(name, "status", STORM_MAX_STR)) {
        criterion = 3;
    } else if (!SStrCmpI(name, "party", STORM_MAX_STR)) {
        criterion = 4;
    } else if (!SStrCmpI(name, "notes", STORM_MAX_STR)) {
        criterion = 5;
    }

    int32_t reverse = StringToBOOL(L, 2, 1);

    if (CGCalendar::m_curEvent && CGCalendar::m_curEvent->SortInvites(criterion, reverse != 0)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_UPDATE_INVITE_LIST, nullptr);
    }

    return 0;
}

// VERIFY: d50e8704 auto
// OFFSET: 0x005B8690
static int32_t Script_CalendarEventGetInviteSortCriterion(lua_State* L) {
    InviteSortCriterion* criterion = CGCalendar::m_curEvent ? &CGCalendar::m_curEvent->m_sortCriteria[CGCalendar::m_curEvent->m_sortType] : nullptr;
    uint8_t reverse = 0;
    const char* name = "";

    if (criterion) {
        switch (criterion->m_criterion) {
            case 0:
                name = "name";
                break;

            case 1:
                name = "level";
                break;

            case 2:
                name = "class";
                break;

            case 3:
                name = "status";
                break;

            case 4:
                name = "party";
                break;

            case 5:
                name = "notes";
                break;
        }

        reverse = criterion->m_reverse;
    }

    lua_pushstring(L, name);
    lua_pushboolean(L, reverse);
    return 2;
}

// VERIFY: 302e38ba manual
// OFFSET: 0x005B8730
static int32_t Script_CalendarEventSetStatus(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CalendarEventSetStatus(index, status)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t status = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    CGCalendar::SetInviteStatus(index, status);
    return 0;
}

// VERIFY: 38509d1b auto
// OFFSET: 0x005B87D0
static int32_t Script_CalendarEventSetModerator(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventSetModerator(index)");
        return 0;
    }

    CGCalendar::SetModeratorStatus(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1, 1);
    return 0;
}

// VERIFY: 7005f67e auto
// OFFSET: 0x005B8840
static int32_t Script_CalendarEventClearModerator(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventClearModerator(index)");
        return 0;
    }

    CGCalendar::SetModeratorStatus(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1, 0);
    return 0;
}

// VERIFY: bbba4c0e auto
// OFFSET: 0x005B88B0
static int32_t Script_CalendarEventIsModerator(lua_State* L) {
    lua_pushboolean(L, CGCalendar::m_curEvent ? CGCalendar::m_curEvent->IsModerator() : false);
    return 1;
}

// VERIFY: bbcdba11 auto
// OFFSET: 0x005B88E0
static int32_t Script_CalendarEventCanModerate(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventCanModerate(index)");
        return 0;
    }

    double index = lua_tonumber(L, 1);
    lua_pushboolean(L, CGCalendar::m_curEvent ? CGCalendar::m_curEvent->CanModerate(static_cast<uint32_t>(index - 1)) : false);
    return 1;
}

// VERIFY: af140e99 auto
// OFFSET: 0x005B8A10
static int32_t Script_CalendarEventHaveSettingsChanged(lua_State* L) {
    if (CGCalendar::m_curEvent) {
        lua_pushboolean(L, CGCalendar::m_curEvent->m_changed);
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

// VERIFY: d3312f5c manual
// OFFSET: 0x005B8A50
static int32_t Script_CalendarEventCanEdit(lua_State* L) {
    bool canEdit = false;

    if (CGCalendar::m_curEvent) {
        canEdit = (CGCalendar::m_curEvent->m_flags & 0x40) ? CGGuildInfo::CanEditGuildEvent() : CGCalendar::m_curEvent->IsModerator();
    }

    lua_pushboolean(L, canEdit);
    return 1;
}

// VERIFY: c6dc5211 auto
// OFFSET: 0x005B8AA0
static int32_t Script_CalendarEventGetCalendarType(lua_State* L) {
    if (CGCalendar::m_curEvent) {
        lua_pushstring(L, CGCalendar::GetCalendarTypeString(CGCalendar::m_curEvent->m_flags));
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 60d41a94 auto
// OFFSET: 0x005B8AE0
static int32_t Script_CalendarEventSelectInvite(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventSelectInvite(index)");
        return 0;
    }

    uint32_t index = lua_tointeger(L, 1) - 1;

    if (CGCalendar::m_curEvent) {
        CGCalendar::m_curEvent->SelectInvite(index);
    }

    return 0;
}

// VERIFY: ce471ee3 auto
// OFFSET: 0x005B8BA0
static int32_t Script_CalendarCanAddEvent(lua_State* L) {
    uint32_t now;

    if (CGCalendar::m_actionPending || ((!unk_BD09AC || !unk_BD09AC->m_intValue) && (now = OsGetAsyncTimeMs(), CGCalendar::m_lastEventTime) && now - CGCalendar::m_lastEventTime < 5000)) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, 1);
    }

    return 1;
}

// VERIFY: 30c19b24 manual
// OFFSET: 0x005B9AC0
static int32_t Script_CalendarGetAbsMonth(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarGetAbsMonth(month[, year])");
        return 0;
    }

    int32_t month = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    int32_t year = CGCalendar::m_absYear;

    if (CGCalendar::m_absYear < 0) {
        year = g_clientGameTime.m_year;
        CGCalendar::m_absYear = year;

        if (year < 0) {
            year = 4;
            CGCalendar::m_absYear = 4;
        }
    }

    if (lua_isnumber(L, 2)) {
        year = static_cast<int32_t>(lua_tonumber(L, 2)) - 2000;
    }

    int32_t outMonth = 0;
    int32_t outYear = 0;
    int32_t days = 0;
    int32_t weekDay = 0;
    CGCalendar::ClampAbsMonthDay(month, year, &outMonth, &outYear, &days, &weekDay);
    lua_pushnumber(L, ++outMonth);
    outYear += 2000;
    lua_pushnumber(L, outYear);
    lua_pushnumber(L, static_cast<uint32_t>(days));
    lua_pushnumber(L, weekDay + 1);
    return 4;
}

// VERIFY: a59c3800 manual
// OFFSET: 0x005B9CA0
static int32_t Script_CalendarGetDayEventSequenceInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarGetDayEventSequenceInfo([-1.0.1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

    if (event) {
        lua_pushnumber(L, event->m_sequenceIndex);
        lua_pushnumber(L, event->m_sequenceLength);
        lua_pushstring(L, GetSequenceLabel(event->m_type, event->m_sequenceIndex, event->m_sequenceLength));
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
    }

    return 3;
}

// VERIFY: cea63837 manual
// OFFSET: 0x005B9EB0
static int32_t Script_CalendarOpenEvent(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarOpenEvent([-1,0,1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CGCalendar::GetEvent(offset, day, index);
    return 0;
}

// VERIFY: 6fc906ba manual
// OFFSET: 0x005BA000
static int32_t Script_CalendarGetRaidInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarGetRaidInfo([-1,0,1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

    if (!event) {
        lua_pushnil(L);
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        return 7;
    }

    MapRec* map = g_mapDB.GetRecord(event->m_holidayID);
    lua_pushstring(L, map ? map->m_mapName : "");
    lua_pushstring(L, CGCalendar::GetCalendarTypeString(event->m_type));
    lua_pushnumber(L, static_cast<int32_t>(event->m_eventID));
    WowTime date;
    WowTime::WowDecodeTime(event->m_startTime, &date);
    lua_pushnumber(L, date.m_hour);
    lua_pushnumber(L, date.m_minute);
    lua_pushnumber(L, static_cast<int32_t>(event->m_endTime + 1));
    MapDifficultyRec* difficulty = GetMapDifficultyEntryByMapIdAndDifficulty(event->m_holidayID, event->m_endTime, nullptr);

    if (!difficulty) {
        lua_pushstring(L, "");
        return 7;
    }

    lua_pushstring(L, FrameScript_GetText(difficulty->m_difficultystring, -1, GENDER_NOT_APPLICABLE));
    return 7;
}

// VERIFY: 04f3797b manual
// OFFSET: 0x005BA220
static int32_t Script_CalendarEventGetInvite(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventGetInvite(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGCalendarEvent* event = CGCalendar::m_curEvent;
    EventInviteInfo* invite;

    if (!event || index >= event->m_numInvites || !(invite = event->m_invites.m_data[index])) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushboolean(L, 0);
        lua_pushinteger(L, 0);
        lua_pushnil(L);
        return 9;
    }

    WOWGUID key = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(invite->m_guid.m_guid, key, nullptr, nullptr, false);
    lua_pushstring(L, name ? name->m_name : "");
    lua_pushnumber(L, invite->m_level);
    ChrClassesRec* classRec = name ? g_chrClassesDB.GetRecord(name->m_class) : nullptr;

    if (classRec) {
        lua_pushstring(L, classRec->m_name);
        lua_pushstring(L, classRec->m_filename);
    } else {
        lua_pushstring(L, "");
        lua_pushstring(L, "");
    }

    if (event) {
        lua_pushinteger(L, invite->m_status + 1);
    } else {
        lua_pushinteger(L, 0);
    }

    if (invite->m_moderatorStatus == 1) {
        lua_pushstring(L, "MODERATOR");
    } else if (invite->m_moderatorStatus == 2) {
        lua_pushstring(L, "CREATOR");
    } else {
        lua_pushstring(L, "");
    }

    lua_pushboolean(L, ClntObjMgrGetActivePlayer() == invite->m_guid);
    lua_pushinteger(L, invite->m_inviteType + 1);
    lua_pushstring(L, invite->m_notes);
    return 9;
}

// VERIFY: aa48e091 manual
// OFFSET: 0x005BA420
static int32_t Script_CalendarEventGetInviteResponseTime(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventGetInviteResponseTime(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    EventInviteInfo* invite;

    if (CGCalendar::m_curEvent && index < CGCalendar::m_curEvent->m_numInvites && (invite = CGCalendar::m_curEvent->m_invites.m_data[index]) && invite->m_responseTime) {
        WowTime date;
        WowTime::WowDecodeTime(invite->m_responseTime, &date);
        lua_pushnumber(L, date.m_weekDay + 1);
        lua_pushnumber(L, date.m_month + 1);
        lua_pushnumber(L, date.m_monthDay + 1);
        lua_pushnumber(L, date.m_year + 2000);
        lua_pushnumber(L, date.m_hour);
        lua_pushnumber(L, date.m_minute);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    return 6;
}

// VERIFY: 7b0245c7 auto
// OFFSET: 0x005BA5A0
static int32_t Script_CalendarRemoveEvent(lua_State* L) {
    CGCalendar::RemoveEvent();
    return 0;
}

// VERIFY: 5a1f2874 manual
// OFFSET: 0x005BA5B0
static int32_t Script_CalendarContextSelectEvent(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarContextSelectEvent([-1.0.1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CGCalendar::SelectContextEvent(offset, day, index);
    return 0;
}

// VERIFY: b16e6979 manual
// OFFSET: 0x005BA700
static int32_t Script_CalendarContextInviteIsPending(lua_State* L) {
    uint8_t pending = 0;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

        if (event && (event->m_flags & 0x1)) {
            pending = 1;
        }
    } else if (!CGCalendar::m_contextEventInfo) {
        luaL_error(L, "Usage: CalendarContextInviteIsPending([-1.0.1], monthDay, index)");
    }

    lua_pushboolean(L, pending);
    return 1;
}

// VERIFY: 7095cce2 manual
// OFFSET: 0x005BA7F0
static int32_t Script_CalendarContextInviteModeratorStatus(lua_State* L) {
    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

        if (event) {
            if (event->m_flags & 0x4) {
                lua_pushstring(L, "CREATOR");
                return 1;
            }

            if (event->m_flags & 0x2) {
                lua_pushstring(L, "MODERATOR");
                return 1;
            }
        }
    } else if (!CGCalendar::m_contextEventInfo) {
        luaL_error(L, "Usage: CalendarContextInviteModeratorStatus([-1.0.1], monthDay, index)");
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 96a9b84e manual
// OFFSET: 0x005BA910
static int32_t Script_CalendarContextInviteStatus(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            lua_pushinteger(L, 0);
            return 1;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextInviteStatus([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    lua_pushinteger(L, event->m_inviteStatus + 1);
    return 1;
}

// VERIFY: 0d48cc71 manual
// OFFSET: 0x005BAA20
static int32_t Script_CalendarContextInviteType(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            lua_pushinteger(L, 0);
            return 1;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextInviteType([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if ((event->m_type & 0x400) && (event->m_flags & 0x8)) {
        lua_pushinteger(L, 2);
    } else {
        lua_pushinteger(L, 1);
    }

    return 1;
}

// VERIFY: 68a1b131 manual
// OFFSET: 0x005BAB50
static int32_t Script_CalendarContextInviteAvailable(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextInviteAvailable([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if (!CGCalendar::m_actionPending && (!(event->m_type & 0x400) || !(event->m_flags & 0x8))) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_RSVP));
        msg.Put(event->m_eventID);
        msg.Put(event->m_inviteID);
        msg.Put(static_cast<uint32_t>(1));
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGCalendar::m_actionPending = 1;
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
    }

    return 0;
}

// VERIFY: 93c40940 manual
// OFFSET: 0x005BAD00
static int32_t Script_CalendarContextInviteDecline(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextInviteDecline([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if (!CGCalendar::m_actionPending && (!(event->m_type & 0x400) || !(event->m_flags & 0x8))) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_CALENDAR_EVENT_RSVP));
        msg.Put(event->m_eventID);
        msg.Put(event->m_inviteID);
        msg.Put(static_cast<uint32_t>(2));
        msg.Finalize();
        ClientServices::Send2(&msg);
        CGCalendar::m_actionPending = 1;
        FrameScript_SignalEvent(SCRIPT_EVENT_CALENDAR_ACTION_PENDING, "%b", 1);
    }

    return 0;
}

// VERIFY: 519921f4 manual
// OFFSET: 0x005BAEB0
static int32_t Script_CalendarContextEventCanEdit(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            lua_pushboolean(L, 0);
            return 1;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextEventCanEdit([-1.0.1], monthDay, index)");
        }
    }

    bool canEdit = (event->m_type & 0x440) ? CGGuildInfo::CanEditGuildEvent() : (event->m_flags & 0x6) != 0;
    lua_pushboolean(L, canEdit);
    return 1;
}

// VERIFY: 7a11a5c5 manual
// OFFSET: 0x005BAFC0
static int32_t Script_CalendarContextEventGetCalendarType(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            lua_pushnil(L);
            return 1;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextEventGetCalendarType([-1.0.1], monthDay, index)");
        }
    }

    lua_pushstring(L, CGCalendar::GetCalendarTypeString(event->m_type));
    return 1;
}

// VERIFY: 1f224d8d auto
// OFFSET: 0x005BB0C0
static int32_t Script_CalendarEventRemoveInvite(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventRemoveInvite(index)");
        return 0;
    }

    CGCalendar::RemoveInvite(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 3dbff87f auto
// OFFSET: 0x005BB130
static int32_t Script_CalendarEventGetStatusOptions(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventGetStatusOptions(index)");
        return 0;
    }

    double index = lua_tonumber(L, 1);
    CGCalendarEvent* event = CGCalendar::m_curEvent;

    if (!event) {
        return 0;
    }

    if (!event->IsModerator()) {
        CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
        return 0;
    }

    int32_t count = 0;

    for (uint32_t i = 0; i < 9; i++) {
        if (event->CanSetInviteStatus(static_cast<uint32_t>(index - 1), i)) {
            lua_pushinteger(L, i + 1);
            lua_pushstring(L, FrameScript_GetText(GetEventStatusString(i), -1, GENDER_NOT_APPLICABLE));
            count += 2;
        }
    }

    return count;
}

// VERIFY: cf28788c manual
// OFFSET: 0x005BB210
static int32_t Script_CalendarEventSetTitle(lua_State* L) {
    const char* title = nullptr;

    if (lua_isstring(L, 1)) {
        title = lua_tolstring(L, 1, nullptr);
    }

    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetTitle(title);
    return 0;
}

// VERIFY: 25ceecec manual
// OFFSET: 0x005BB280
static int32_t Script_CalendarEventSetDescription(lua_State* L) {
    const char* description = nullptr;

    if (lua_isstring(L, 1)) {
        description = lua_tolstring(L, 1, nullptr);
    }

    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetDescription(description);
    return 0;
}

// VERIFY: d21c122b manual
// OFFSET: 0x005BB2F0
static int32_t Script_CalendarEventSetType(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    double type = lua_tonumber(L, 1);

    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetType(static_cast<uint32_t>(type - 1));
    return 0;
}

// VERIFY: cca8fd6c manual
// OFFSET: 0x005BB380
static int32_t Script_CalendarEventSetRepeatOption(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    double repeatOption = lua_tonumber(L, 1);

    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetRepeatOption(static_cast<uint32_t>(repeatOption - 1));
    return 0;
}

// VERIFY: 8038ff09 manual
// OFFSET: 0x005BB410
static int32_t Script_CalendarEventSetSize(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    double size = lua_tonumber(L, 1);

    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetSize(static_cast<uint32_t>(size));
    return 0;
}

// VERIFY: c9bd4e1d manual
// OFFSET: 0x005BB4A0
static int32_t Script_CalendarEventSetDate(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarEventSetDate(month, day, year)");
        return 0;
    }

    uint32_t month = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t monthDay = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t year = static_cast<uint32_t>(lua_tonumber(L, 3)) - 2000;

    if (!CGCalendar::SetEventDate(month, monthDay, year)) {
        CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_DATE", 0);
    }

    return 0;
}

// VERIFY: c38beb47 auto
// OFFSET: 0x005BB5A0
static int32_t Script_CalendarEventSetTime(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CalendarEventSetTime(hour, minute)");
        return 0;
    }

    uint32_t hour = static_cast<uint32_t>(lua_tonumber(L, 1));
    uint32_t minute = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (!CGCalendar::SetEventTime(hour, minute)) {
        CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_TIME", 0);
    }

    return 0;
}

// VERIFY: 81ea6cd9 manual
// OFFSET: 0x005BB650
static int32_t Script_CalendarEventSetLockoutDate(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarEventSetLockoutDate(month, day, year)");
        return 0;
    }

    uint32_t month = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t monthDay = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t year = static_cast<uint32_t>(lua_tonumber(L, 3)) - 2000;

    if (!CGCalendar::SetEventLockoutDate(month, monthDay, year)) {
        CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_DATE", 0);
    }

    return 0;
}

// VERIFY: 72d0c4f8 auto
// OFFSET: 0x005BB750
static int32_t Script_CalendarEventSetLockoutTime(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CalendarEvenSettLockoutTime(hour, minute)");
        return 0;
    }

    uint32_t hour = static_cast<uint32_t>(lua_tonumber(L, 1));
    uint32_t minute = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (!CGCalendar::SetEventLockoutTime(hour, minute)) {
        CGCalendar::CalendarError("CALENDAR_ERROR_INVALID_TIME", 0);
    }

    return 0;
}

// VERIFY: 87bf79dd auto
// OFFSET: 0x005BB800
static int32_t Script_CalendarEventSetTextureID(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventSetTextureID(textureIndex)");
        return 0;
    }

    CGCalendar::SetEventTextureID(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: 20d6b24e manual
// OFFSET: 0x005BB870
static int32_t Script_CalendarEventSetLocked(lua_State* L) {
    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetFlag(0x10);
    return 0;
}

// VERIFY: 0d84b1df manual
// OFFSET: 0x005BB8B0
static int32_t Script_CalendarEventClearLocked(lua_State* L) {
    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->ClearFlag(0x10);
    return 0;
}

// VERIFY: 3a4a63e9 manual
// OFFSET: 0x005BB8F0
static int32_t Script_CalendarEventSetAutoApprove(lua_State* L) {
    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->SetFlag(0x20);
    return 0;
}

// VERIFY: b8e3ea42 manual
// OFFSET: 0x005BB930
static int32_t Script_CalendarEventClearAutoApprove(lua_State* L) {
    if (!CGCalendar::m_curEvent) {
        return 0;
    }

    if (CGCalendar::m_curEvent->m_flags & 0x40) {
        if (!CGGuildInfo::CanEditGuildEvent()) {
            return 0;
        }
    } else if (!CGCalendar::m_curEvent->IsModerator()) {
        return 0;
    }

    CGCalendar::m_curEvent->ClearFlag(0x20);
    return 0;
}

// VERIFY: beadfddc manual
// OFFSET: 0x005BB970
static int32_t Script_CalendarEventGetTextures(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarEventGetTextures(eventType)");
        return 0;
    }

    double type = lua_tonumber(L, 1);
    uint32_t list;

    if (type == 2) {
        list = 0;
    } else if (type == 1) {
        list = 1;
    } else {
        return 0;
    }

    TSFixedArray<int32_t>* textures = &s_eventTextures[list];
    uint32_t count = textures->Count();

    if (!count) {
        return 0;
    }

    lua_checkstack(L, 4 * count);

    for (uint32_t i = 0; i < count; i++) {
        LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(textures->m_data[i]);

        if (dungeon) {
            lua_pushstring(L, dungeon->m_name);
            lua_pushstring(L, dungeon->m_textureFilename);
            lua_pushinteger(L, dungeon->m_expansionLevel);
            MapDifficultyRec* difficulty;

            if (type == 1 && (difficulty = GetMapDifficultyEntryByMapIdAndDifficulty(dungeon->m_mapID, dungeon->m_difficulty, nullptr))) {
                lua_pushstring(L, FrameScript_GetText(difficulty->m_difficultystring, -1, GENDER_NOT_APPLICABLE));
            } else {
                lua_pushstring(L, "");
            }
        } else {
            lua_pushstring(L, "");
            lua_pushstring(L, "");
            lua_pushinteger(L, -1);
            lua_pushstring(L, "");
        }
    }

    return 4 * count;
}

// VERIFY: 38118389 auto
// OFFSET: 0x005BBB00
static int32_t Script_CalendarEventGetSelectedInvite(lua_State* L) {
    if (CGCalendar::m_curEvent) {
        lua_pushinteger(L, CGCalendar::m_curEvent->GetSelectedInviteIndex() + 1);
    } else {
        lua_pushinteger(L, 0);
    }

    return 1;
}

// VERIFY: a3403abe auto
// OFFSET: 0x005BCC70
static int32_t Script_CalendarAddEvent(lua_State* L) {
    if ((unk_BD09AC && unk_BD09AC->m_intValue) || CGGameUI::CanPerformAction(19)) {
        CGCalendar::AddEvent();
    }

    return 0;
}

// VERIFY: c17b27f8 auto
// OFFSET: 0x005BCCA0
static int32_t Script_CalendarNewEvent(lua_State* L) {
    CGCalendar::CreatePlayerEvent();
    return 0;
}

// VERIFY: d26ce326 auto
// OFFSET: 0x005BCCB0
static int32_t Script_CalendarMassInviteGuild(lua_State* L) {
    if (!lua_isnumber(L, 1) && !lua_isnumber(L, 2) && !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarMassInviteGuild(minLevel, maxLevel, minRank)");
        return 0;
    }

    uint32_t minLevel = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t maxLevel = static_cast<int32_t>(lua_tonumber(L, 2));
    uint32_t minRank = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CGCalendar::GuildFilter(minLevel, maxLevel, minRank);
    return 0;
}

// VERIFY: d59cfbfe auto
// OFFSET: 0x005BCD80
static int32_t Script_CalendarNewGuildAnnouncement(lua_State* L) {
    CGCalendar::CreateGuildAnnouncement();
    return 0;
}

// VERIFY: 0cbef7d1 auto
// OFFSET: 0x005BCD90
static int32_t Script_CalendarNewGuildEvent(lua_State* L) {
    CGCalendar::CreateGuildEvent();
    return 0;
}

// VERIFY: cfd24fb4 auto
// OFFSET: 0x005BCE60
static int32_t Script_CalendarMassInviteArenaTeam(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CalendarNewArenaTeamEvent(index)");
        return 0;
    }

    CGCalendar::ArenaTeam(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);
    return 0;
}

// VERIFY: a049e522 auto
// OFFSET: 0x005BCED0
static int32_t Script_CalendarUpdateEvent(lua_State* L) {
    if ((unk_BD09AC && unk_BD09AC->m_intValue) || CGGameUI::CanPerformAction(19)) {
        CGCalendar::UpdateEvent();
    }

    return 0;
}

// VERIFY: 49208bb9 auto
// OFFSET: 0x005BCF00
static int32_t Script_CalendarEventInvite(lua_State* L) {
    const char* name = lua_tolstring(L, 1, nullptr);

    if (name) {
        CGCalendar::Invite(name);
    }

    return 0;
}

// VERIFY: 758a8bea manual
// OFFSET: 0x005BCF30
static int32_t Script_CalendarContextInviteTentative(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: Script_CalendarContextInviteTentative([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if ((event->m_type & 0x400) && (event->m_flags & 0x8)) {
        CGCalendar::ContextMenuEventSignUp(event, 1);
    } else {
        CGCalendar::ContextMenuInviteRSVP(event, 8);
    }

    return 0;
}

// VERIFY: ff942296 manual
// OFFSET: 0x005BD040
static int32_t Script_CalendarContextInviteRemove(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextInviteRemove([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if (event->m_type & 0x401) {
        CGCalendar::ContextMenuInviteRemove(event);
    }

    return 0;
}

// VERIFY: 40eb314d manual
// OFFSET: 0x005BD130
static int32_t Script_CalendarContextEventSignUp(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextEventSignUp([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if (event->m_type & 0x400) {
        CGCalendar::ContextMenuEventSignUp(event, 0);
    }

    return 0;
}

// VERIFY: 117fedbb manual
// OFFSET: 0x005BD220
static int32_t Script_CalendarContextEventRemove(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: Script_CalendarContextEventRemove([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    bool allowed = (event->m_type & 0x440) ? CGGuildInfo::CanEditGuildEvent() : (event->m_flags & 0x6) != 0;

    if (!allowed) {
        CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
        return 0;
    }

    CGCalendar::ContextMenuEventRemove(event);
    return 0;
}

// VERIFY: 5ba74e47 manual
// OFFSET: 0x005BD340
static int32_t Script_CalendarContextEventCopy(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);

        if (!event) {
            return 0;
        }
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: Script_CalendarContextEventCopy([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    bool allowed = (event->m_type & 0x440) ? CGGuildInfo::CanEditGuildEvent() : (event->m_flags & 0x6) != 0;

    if (!allowed) {
        CGCalendar::CalendarError("CALENDAR_ERROR_PERMISSIONS", 0);
        return 0;
    }

    CGCalendar::ContextMenuEventCopy(event);
    return 0;
}

// VERIFY: 69d55813 auto
// OFFSET: 0x005BD460
static int32_t Script_CalendarContextEventPaste(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: Script_CalendarContextEventPaste([-1.0.1], monthDay)");
        return 0;
    }

    int32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    CGCalendar::ContextMenuEventPaste(offset, day);
    return 0;
}

// VERIFY: 4eace431 manual
// OFFSET: 0x005BD4F0
static int32_t Script_CalendarContextEventCanComplain(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextEventCanComplain([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    if (CGCalendar::m_actionPending || !CGCalendar::CanComplainAboutEvent(event)) {
        lua_pushboolean(L, 0);
    } else {
        lua_pushboolean(L, 1);
    }

    return 1;
}

// VERIFY: 084e34b8 manual
// OFFSET: 0x005BD600
static int32_t Script_CalendarContextEventComplain(lua_State* L) {
    CalendarDayEvent* event;

    if (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
        uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
        event = CGCalendar::GetDayEvent(offset, day, index);
    } else {
        event = CGCalendar::m_contextEventInfo;

        if (!event) {
            luaL_error(L, "Usage: CalendarContextEventComplain([-1.0.1], monthDay, index)");
            return 0;
        }
    }

    CGCalendar::Complain(event);
    return 0;
}

// VERIFY: 0adb7ecc manual
// OFFSET: 0x005BD6E0
static int32_t Script_CalendarEventAvailable(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && !CGCalendar::m_actionPending && CGCalendar::m_curEvent && (!(CGCalendar::m_curEvent->m_flags & 0x400) || CGCalendar::m_curEvent->m_guildID != player->m_player->m_playerGuildid)) {
        if (CGCalendar::m_curEvent->InviteRSVP(1)) {
            CGCalendar::SetActionPending(1);
        }
    }

    return 0;
}

// VERIFY: 2d764a19 auto
// OFFSET: 0x005BD750
static int32_t Script_CalendarEventTentative(lua_State* L) {
    CGCalendar::InviteRSVP(8);
    return 0;
}

// VERIFY: 5ef72a73 manual
// OFFSET: 0x005BD760
static int32_t Script_CalendarEventDecline(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && !CGCalendar::m_actionPending && CGCalendar::m_curEvent && (!(CGCalendar::m_curEvent->m_flags & 0x400) || CGCalendar::m_curEvent->m_guildID != player->m_player->m_playerGuildid)) {
        if (CGCalendar::m_curEvent->InviteRSVP(2)) {
            CGCalendar::SetActionPending(1);
        }
    }

    return 0;
}

// VERIFY: 3c288e81 auto
// OFFSET: 0x005BD7D0
static int32_t Script_CalendarEventSignUp(lua_State* L) {
    CGCalendar::EventSignUp();
    return 0;
}

// VERIFY: 08db3b12 auto
// OFFSET: 0x005BD7E0
static int32_t Script_OpenCalendar(lua_State* L) {
    if (!CGCalendar::m_openedCalendar) {
        CGCalendar::GetCalendar();
    }

    return 0;
}

// VERIFY: df735662 manual
// OFFSET: 0x005BD8A0
static int32_t Script_CalendarGetEventInfo(lua_State* L) {
    CGCalendarEvent* event = CGCalendar::m_curEvent;

    if (!event) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushstring(L, "");

        for (uint32_t i = 0; i < 16; i++) {
            lua_pushnumber(L, 0.0);
        }

        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        return 25;
    }

    char title[128];
    SStrCopy(title, event->m_title, STORM_MAX_STR);
    ProfanityFilter(title, false, false);
    lua_pushstring(L, title);
    char description[1024];
    SStrCopy(description, event->m_description, STORM_MAX_STR);
    ProfanityFilter(description, false, false);
    lua_pushstring(L, description);
    WOWGUID key = WOWGUID();
    NameCache* creator = g_nameDBCache.GetRecord(event->m_creatorGUID.m_guid, key, NameCacheCallback, nullptr, false);
    lua_pushstring(L, creator ? creator->m_name : "");
    lua_pushnumber(L, static_cast<int32_t>(event->m_type + 1));
    lua_pushnumber(L, static_cast<int32_t>(event->m_repeatOption + 1));
    lua_pushnumber(L, event->m_maxSize);
    lua_pushnumber(L, CGCalendar::GetTextureIndex(event->m_type, event->m_textureID) + 1);
    lua_pushnumber(L, event->m_date.m_weekDay + 1);
    lua_pushnumber(L, event->m_date.m_month + 1);
    lua_pushnumber(L, event->m_date.m_monthDay + 1);
    lua_pushnumber(L, event->m_date.m_year + 2000);
    lua_pushnumber(L, event->m_date.m_hour);
    lua_pushnumber(L, event->m_date.m_minute);
    lua_pushnumber(L, event->m_lockoutDate.m_weekDay + 1);
    lua_pushnumber(L, event->m_lockoutDate.m_month + 1);
    lua_pushnumber(L, event->m_lockoutDate.m_monthDay + 1);
    lua_pushnumber(L, event->m_lockoutDate.m_year + 2000);
    lua_pushnumber(L, event->m_lockoutDate.m_hour);
    lua_pushnumber(L, event->m_lockoutDate.m_minute);

    if (event->m_flags & 0x10) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (event->m_flags & 0x20) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (CGCalendar::HasPendingInvite(event->m_eventID)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    EventInviteInfo* invite = event->GetInvite(ClntObjMgrGetActivePlayer());

    if (invite) {
        lua_pushnumber(L, invite->m_status + 1);
        lua_pushnumber(L, invite->m_inviteType + 1);
    } else if (event->m_flags & 0x400) {
        lua_pushnumber(L, 8.0);
        lua_pushnumber(L, 2.0);
    } else {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
    }

    lua_pushstring(L, CGCalendar::GetCalendarTypeString(event->m_flags));
    return 25;
}

// VERIFY: fed5193b auto
// OFFSET: 0x005BDD90
static int32_t Script_CalendarEventHasPendingInvite(lua_State* L) {
    if (CGCalendar::m_curEvent) {
        lua_pushboolean(L, CGCalendar::HasPendingInvite(CGCalendar::m_curEvent->m_eventID));
    } else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

// VERIFY: a3de76cf manual
// OFFSET: 0x005C0FB0
static void PushDayEventTexture(lua_State* L, CalendarDayEvent* event) {
    const char* texture = "";

    if (event) {
        if (event->m_type & 0x8) {
            DYNAMICHOLIDAYHASH* dynamic = s_dynamicHolidays.Ptr(static_cast<uint32_t>(event->m_eventID), HASHKEY_NONE());

            if (dynamic) {
                lua_pushstring(L, dynamic->m_textureFileName);
                return;
            }

            HolidaysRec* holiday = g_holidaysDB.GetRecord(static_cast<int32_t>(event->m_eventID));

            if (holiday) {
                lua_pushstring(L, holiday->m_textureFileName);
                return;
            }
        } else if ((event->m_type & 0x543) && event->m_textureID >= 0) {
            LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(event->m_textureID);

            if (dungeon) {
                texture = dungeon->m_textureFilename;
            }
        }
    }

    lua_pushstring(L, texture);
}

// VERIFY: 67f9a0da manual
// OFFSET: 0x005C1070
static int32_t Script_CalendarGetDayEvent(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarGetDayEvent([-1.0.1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);

    if (!event) {
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        return 15;
    }

    WowTime start;
    WowTime::WowDecodeTime(event->m_startTime, &start);

    if (event->m_type & 0x8) {
        lua_pushstring(L, GetHolidayName(static_cast<int32_t>(event->m_eventID)));
    } else if (event->m_type & 0x280) {
        MapRec* map = g_mapDB.GetRecord(event->m_holidayID);

        if (map) {
            lua_pushstring(L, map->m_mapName);
        } else {
            lua_pushstring(L, "");
        }
    } else {
        char title[128];
        SStrCopy(title, event->m_title, STORM_MAX_STR);
        ProfanityFilter(title, false, false);
        lua_pushstring(L, title);
    }

    lua_pushnumber(L, start.m_hour);
    lua_pushnumber(L, start.m_minute);
    lua_pushstring(L, CGCalendar::GetCalendarTypeString(event->m_type));
    lua_pushstring(L, GetSequenceLabel(event->m_type, event->m_sequenceIndex, event->m_sequenceLength));
    lua_pushnumber(L, event->m_eventType + 1);
    PushDayEventTexture(L, event);
    lua_pushstring(L, GetInviteRoleLabel(event->m_flags));
    lua_pushnumber(L, event->m_inviteStatus + 1);
    WOWGUID key = WOWGUID();
    NameCache* creator = g_nameDBCache.GetRecord(event->m_creatorGUID.m_guid, key, NameCacheCallback, nullptr, false);
    lua_pushstring(L, creator ? creator->m_name : "");
    lua_pushnumber(L, (event->m_type & 0x280) ? static_cast<int32_t>(event->m_endTime + 1) : 0.0);
    lua_pushnumber(L, IsSignUpInvite(event) + 1);
    lua_pushnumber(L, event->m_sequenceIndex + 1);
    lua_pushnumber(L, event->m_sequenceLength);
    MapDifficultyRec* difficulty;

    if (!(event->m_type & 0x280) || !(difficulty = GetMapDifficultyEntryByMapIdAndDifficulty(event->m_holidayID, event->m_endTime, nullptr))) {
        lua_pushstring(L, "");
        return 15;
    }

    lua_pushstring(L, FrameScript_GetText(difficulty->m_difficultystring, -1, GENDER_NOT_APPLICABLE));
    return 15;
}

// VERIFY: a260d7b8 manual
// OFFSET: 0x005C1480
static int32_t Script_CalendarGetHolidayInfo(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: CalendarGetHolidayInfo([-1,0,1], monthDay, index)");
        return 0;
    }

    uint32_t offset = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t day = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    CalendarDayEvent* event = CGCalendar::GetDayEvent(offset, day, index);
    const char* name;
    const char* description;

    if (!event || !(name = GetHolidayName(static_cast<int32_t>(event->m_eventID)), description = GetHolidayDescription(static_cast<int32_t>(event->m_eventID)), name) || !description) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushstring(L, name);
    lua_pushstring(L, description);
    const char* texture = "";
    DYNAMICHOLIDAYHASH* dynamic = CalendarGetHolidayInfo(static_cast<int32_t>(event->m_eventID));

    if (dynamic) {
        texture = dynamic->m_textureFileName;
    } else {
        HolidaysRec* holiday = g_holidaysDB.GetRecord(static_cast<int32_t>(event->m_eventID));

        if (holiday) {
            texture = holiday->m_textureFileName;
        }
    }

    lua_pushstring(L, texture);
    return 3;
}

// STATIC: 0x00AD0030
static FrameScript_Method s_ScriptFunctions[] = {
    { "CalendarGetMonthNames", &Script_CalendarGetMonthNames },
    { "CalendarGetWeekdayNames", &Script_CalendarGetWeekdayNames },
    { "CalendarGetDate", &Script_CalendarGetDate },
    { "CalendarGetMinDate", &Script_CalendarGetMinDate },
    { "CalendarGetMaxDate", &Script_CalendarGetMaxDate },
    { "CalendarGetMinHistoryDate", &Script_CalendarGetMinHistoryDate },
    { "CalendarGetMaxCreateDate", &Script_CalendarGetMaxCreateDate },
    { "CalendarGetMonth", &Script_CalendarGetMonth },
    { "CalendarGetAbsMonth", &Script_CalendarGetAbsMonth },
    { "CalendarSetMonth", &Script_CalendarSetMonth },
    { "CalendarSetAbsMonth", &Script_CalendarSetAbsMonth },
    { "CalendarGetNumDayEvents", &Script_CalendarGetNumDayEvents },
    { "CalendarGetDayEvent", &Script_CalendarGetDayEvent },
    { "CalendarGetDayEventSequenceInfo", &Script_CalendarGetDayEventSequenceInfo },
    { "CalendarGetFirstPendingInvite", &Script_CalendarGetFirstPendingInvite },
    { "CalendarOpenEvent", &Script_CalendarOpenEvent },
    { "CalendarGetEventIndex", &Script_CalendarGetEventIndex },
    { "CalendarCloseEvent", &Script_CalendarCloseEvent },
    { "CalendarGetEventInfo", &Script_CalendarGetEventInfo },
    { "CalendarGetHolidayInfo", &Script_CalendarGetHolidayInfo },
    { "CalendarGetRaidInfo", &Script_CalendarGetRaidInfo },
    { "CalendarGetNumPendingInvites", &Script_CalendarGetNumPendingInvites },
    { "CalendarEventGetNumInvites", &Script_CalendarEventGetNumInvites },
    { "CalendarEventGetInvite", &Script_CalendarEventGetInvite },
    { "CalendarEventGetInviteResponseTime", &Script_CalendarEventGetInviteResponseTime },
    { "CalendarAddEvent", &Script_CalendarAddEvent },
    { "CalendarNewEvent", &Script_CalendarNewEvent },
    { "CalendarMassInviteGuild", &Script_CalendarMassInviteGuild },
    { "CalendarMassInviteArenaTeam", &Script_CalendarMassInviteArenaTeam },
    { "CalendarNewGuildAnnouncement", &Script_CalendarNewGuildAnnouncement },
    { "CalendarNewGuildEvent", &Script_CalendarNewGuildEvent },
    { "CalendarDefaultGuildFilter", &Script_CalendarDefaultGuildFilter },
    { "CalendarUpdateEvent", &Script_CalendarUpdateEvent },
    { "CalendarRemoveEvent", &Script_CalendarRemoveEvent },
    { "CalendarEventSelectInvite", &Script_CalendarEventSelectInvite },
    { "CalendarEventGetSelectedInvite", &Script_CalendarEventGetSelectedInvite },
    { "CalendarContextSelectEvent", &Script_CalendarContextSelectEvent },
    { "CalendarContextDeselectEvent", &Script_CalendarContextDeselectEvent },
    { "CalendarContextGetEventIndex", &Script_CalendarContextGetEventIndex },
    { "CalendarContextInviteIsPending", &Script_CalendarContextInviteIsPending },
    { "CalendarContextInviteModeratorStatus", &Script_CalendarContextInviteModeratorStatus },
    { "CalendarContextInviteStatus", &Script_CalendarContextInviteStatus },
    { "CalendarContextInviteType", &Script_CalendarContextInviteType },
    { "CalendarContextInviteAvailable", &Script_CalendarContextInviteAvailable },
    { "CalendarContextInviteTentative", &Script_CalendarContextInviteTentative },
    { "CalendarContextInviteDecline", &Script_CalendarContextInviteDecline },
    { "CalendarContextInviteRemove", &Script_CalendarContextInviteRemove },
    { "CalendarContextEventSignUp", &Script_CalendarContextEventSignUp },
    { "CalendarContextEventRemove", &Script_CalendarContextEventRemove },
    { "CalendarContextEventCopy", &Script_CalendarContextEventCopy },
    { "CalendarContextEventPaste", &Script_CalendarContextEventPaste },
    { "CalendarContextEventClipboard", &Script_CalendarContextEventClipboard },
    { "CalendarContextEventCanComplain", &Script_CalendarContextEventCanComplain },
    { "CalendarContextEventComplain", &Script_CalendarContextEventComplain },
    { "CalendarContextEventCanEdit", &Script_CalendarContextEventCanEdit },
    { "CalendarContextEventGetCalendarType", &Script_CalendarContextEventGetCalendarType },
    { "CalendarEventInvite", &Script_CalendarEventInvite },
    { "CalendarEventRemoveInvite", &Script_CalendarEventRemoveInvite },
    { "CalendarEventAvailable", &Script_CalendarEventAvailable },
    { "CalendarEventTentative", &Script_CalendarEventTentative },
    { "CalendarEventDecline", &Script_CalendarEventDecline },
    { "CalendarEventSignUp", &Script_CalendarEventSignUp },
    { "CalendarEventSortInvites", &Script_CalendarEventSortInvites },
    { "CalendarEventGetInviteSortCriterion", &Script_CalendarEventGetInviteSortCriterion },
    { "CalendarEventGetStatusOptions", &Script_CalendarEventGetStatusOptions },
    { "CalendarEventSetStatus", &Script_CalendarEventSetStatus },
    { "CalendarEventSetModerator", &Script_CalendarEventSetModerator },
    { "CalendarEventClearModerator", &Script_CalendarEventClearModerator },
    { "CalendarEventCanModerate", &Script_CalendarEventCanModerate },
    { "CalendarEventIsModerator", &Script_CalendarEventIsModerator },
    { "CalendarEventGetTypes", &Script_CalendarEventGetTypes },
    { "CalendarEventGetRepeatOptions", &Script_CalendarEventGetRepeatOptions },
    { "CalendarEventSetTitle", &Script_CalendarEventSetTitle },
    { "CalendarEventSetDescription", &Script_CalendarEventSetDescription },
    { "CalendarEventSetType", &Script_CalendarEventSetType },
    { "CalendarEventSetRepeatOption", &Script_CalendarEventSetRepeatOption },
    { "CalendarEventSetSize", &Script_CalendarEventSetSize },
    { "CalendarEventSetDate", &Script_CalendarEventSetDate },
    { "CalendarEventSetTime", &Script_CalendarEventSetTime },
    { "CalendarEventSetLockoutDate", &Script_CalendarEventSetLockoutDate },
    { "CalendarEventSetLockoutTime", &Script_CalendarEventSetLockoutTime },
    { "CalendarEventSetTextureID", &Script_CalendarEventSetTextureID },
    { "CalendarEventSetLocked", &Script_CalendarEventSetLocked },
    { "CalendarEventClearLocked", &Script_CalendarEventClearLocked },
    { "CalendarEventSetAutoApprove", &Script_CalendarEventSetAutoApprove },
    { "CalendarEventClearAutoApprove", &Script_CalendarEventClearAutoApprove },
    { "CalendarEventGetTextures", &Script_CalendarEventGetTextures },
    { "CalendarEventHasPendingInvite", &Script_CalendarEventHasPendingInvite },
    { "CalendarEventHaveSettingsChanged", &Script_CalendarEventHaveSettingsChanged },
    { "CalendarEventCanEdit", &Script_CalendarEventCanEdit },
    { "CalendarEventGetCalendarType", &Script_CalendarEventGetCalendarType },
    { "CalendarCanSendInvite", &Script_CalendarCanSendInvite },
    { "CalendarCanAddEvent", &Script_CalendarCanAddEvent },
    { "CalendarIsActionPending", &Script_CalendarIsActionPending },
    { "OpenCalendar", &Script_OpenCalendar },
};

// VERIFY: 70bc33d7 auto
// OFFSET: 0x005B8C30
void CalendarRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: e4c5acbe auto
// OFFSET: 0x005B8C60
void CalendarUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
