#ifndef WOW_SOURCE_UI_CALENDAR_H
#define WOW_SOURCE_UI_CALENDAR_H

#include <cstdint>
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

class CDataStore;
class CGCalendarEvent;
class CGameTime;
class WowTime;
struct TIMESTAMPCALLBACK;
struct CVar;
struct EventInviteInfo;
struct lua_State;

int32_t PH_CALENDAR_GROUP(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
void PH_SMSG_CALENDAR_SEND_CALENDAR(CDataStore* msg);

class HolidaysRec;
struct CalendarDayEvent;

struct CALENDARHOLIDAYNODE : public TSLinkedNode<CALENDARHOLIDAYNODE> {
    /* 0008 */ HolidaysRec* m_holiday;
    /* 000C */ int32_t m_index;
    /* 0010 */ uint32_t m_startTime;
    /* 0014 */ uint32_t m_endTime;
    /* 0018 */ int32_t m_duration;
    /* 001C */ int32_t m_type;

    CALENDARHOLIDAYNODE();
};

// INLINED
inline CALENDARHOLIDAYNODE::CALENDARHOLIDAYNODE() {
    this->m_holiday = nullptr;
    this->m_index = 0;
    this->m_startTime = 0;
    this->m_endTime = 0;
    this->m_duration = 0;
    this->m_type = 4;
}

struct CALENDARRAIDRESETNODE : public TSLinkedNode<CALENDARRAIDRESETNODE> {
    /* 0008 */ int32_t m_mapID;
    /* 000C */ uint32_t m_startTime;
    /* 0010 */ int32_t m_interval;
};

struct DYNAMICHOLIDAYHASH : public TSHashObject<DYNAMICHOLIDAYHASH, HASHKEY_NONE> {
    /* 0018 */ int32_t m_region;
    /* 001C */ int32_t m_looping;
    /* 0020 */ int32_t m_priority;
    /* 0024 */ int32_t m_calendarFilterType;
    /* 0028 */ int32_t m_date[26];
    /* 0090 */ int32_t m_duration[10];
    /* 00B8 */ int32_t m_calendarFlags[10];
    /* 00E0 */ char m_textureFileName[64];
};

struct CALENDAREVENTHASH : public TSHashObject<CALENDAREVENTHASH, HASHKEY_NONE> {
    /* 0018 */ TSGrowableArray<CalendarDayEvent*> m_events;

    ~CALENDAREVENTHASH();
};

struct CalendarEventData {
    /* 0000 */ uint64_t m_eventID;
    /* 0008 */ char m_title[128];
    /* 0088 */ uint32_t m_startTime;
    /* 008C */ uint32_t m_alarmTime;
    /* 0090 */ uint32_t m_endTime;
    /* 0094 */ uint32_t m_holidayID;
    /* 0098 */ uint32_t m_type;
    /* 009C */ uint32_t m_flags;
    /* 00A0 */ int32_t m_eventType;
    /* 00A4 */ int32_t m_textureID;
    /* 00A8 */ uint8_t m_inviteStatus;
    /* 00B0 */ uint64_t m_inviteID;
    /* 00B8 */ WOWGUID m_creatorGUID;
    /* 00C0 */ TIMESTAMPCALLBACK* m_alarmCallback;
    /* 00C4 */ uint32_t m_sequenceIndex;
    /* 00C8 */ uint32_t m_sequenceLength;
};

struct CalendarDayEvent : public CalendarEventData {
    CalendarDayEvent();
    ~CalendarDayEvent();
    int32_t GetSequenceType() const;
    void ScheduleAlarm();
};

struct CalendarEventSelection {
    /* 0000 */ uint64_t m_eventID;
    /* 0008 */ uint32_t m_startTime;
    /* 000C */ uint32_t m_endTime;
    /* 0010 */ uint32_t m_holidayID;
    /* 0014 */ int32_t m_eventType;
};

class CalendarEventTables {
    public:
    /* 0000 */ TSHashTable<CALENDAREVENTHASH, HASHKEY_NONE> m_tables[8];
    /* 0140 */ int32_t m_counts[8];

    CalendarEventTables();
    void Clear();
    bool Delete(int32_t table, uint32_t date, uint64_t eventID);
    CalendarDayEvent* GetEvent(int32_t table, uint32_t date, uint64_t eventID);
};

struct CalendarPendingInvite : public TSLinkedNode<CalendarPendingInvite> {
    /* 0008 */ uint64_t m_eventID;
    /* 0010 */ uint64_t m_inviteID;
    /* 0018 */ WOWGUID m_inviterGuid;
    /* 0020 */ uint8_t m_status;
    /* 0021 */ uint8_t m_rank;
    /* 0022 */ uint8_t m_inviteType;
    /* 0023 */ uint8_t m_valid;

    CalendarPendingInvite();
};

// INLINED
inline CalendarPendingInvite::CalendarPendingInvite() {
    this->m_eventID = 0;
}

class CGCalendar {
    public:
    static void LeaveWorld();
    static void GuildChanged();
    static uint32_t CountPendingInvites();
    static void UpdateDisplayList(bool signal);
    static uint8_t m_actionPending;
    static void CalendarError(const char* error, uintptr_t arg);
    static size_t GetDateTimeText(char* buffer, uint32_t size, uint32_t time);
    static CGCalendarEvent* m_curEvent;
    static uint8_t m_initialize;
    static uint8_t m_openedCalendar;
    static uint8_t m_calendarReceived;
    static CalendarDayEvent* m_contextEventInfo;
    static uint32_t m_lastEventTime;
    static int32_t m_numInitialPending;

    static void EnterWorld();
    static void InitializeGame();
    static void ShutdownGame();
    static void QueryPendingInvites();
    static void SetFilter(CVar* var, int32_t value);
    static void ProcessGetEvent(CDataStore* msg);
    static void ProcessInitialInviteList(CDataStore* msg);
    static void ProcessInvite(CDataStore* msg, uint8_t* clearPending);
    static void ProcessInviteRemoved(CDataStore* msg, uint8_t* clearPending);
    static void ProcessInviteStatus(CDataStore* msg, uint8_t* clearPending);
    static void ProcessLockoutAdded(CDataStore* msg);
    static void ProcessLockoutRemoved(CDataStore* msg);
    static void ProcessLockoutUpdated(CDataStore* msg);
    static void ProcessInviteAlert(CDataStore* msg);
    static void ProcessInviteRemovedAlert(CDataStore* msg);
    static void ProcessInviteStatusAlert(CDataStore* msg);
    static void ProcessEventRemovedAlert(CDataStore* msg, uint8_t* clearPending);
    static void ProcessEventUpdatedAlert(CDataStore* msg, uint8_t* clearPending);
    static void ProcessInviteModeratorStatusAlert(CDataStore* msg, uint8_t* clearPending);
    static void ProcessInviteNotes(CDataStore* msg, uint8_t* clearPending);
    static void ProcessInviteNotesAlert(CDataStore* msg);
    static void UpdatePendingInvites(CDataStore* msg);
    static int32_t m_absMonth;
    static uint32_t m_lastInviteTime;
    static int32_t m_absYear;
    static int32_t m_absMonthDays;
    static CalendarEventSelection m_selectedEvent;
    static CalendarEventSelection m_contextEvent;
    static int32_t GetDaysInMonth(int32_t month, int32_t year, int32_t* days, int32_t* weekDay);
    static int32_t GetMonth(int32_t offset, int32_t* month, int32_t* year, int32_t* days, int32_t* weekDay);
    static int32_t ClampAbsMonthDay(int32_t month, int32_t year, int32_t* outMonth, int32_t* outYear, int32_t* days, int32_t* weekDay);
    static bool ComputeEventIndex(const CalendarEventSelection* selection, int32_t* monthOffset, int32_t* day, int32_t* index);
    static void CalculateEvents(bool a1);
    static void InitHolidays();
    static bool ValidateDate(const WowTime* date);
    static bool GetDayEvent(uint32_t offset, uint32_t day);
    static int32_t GetNumDayEvents(uint32_t offset, uint32_t day);
    static int32_t GetFirstPendingInviteIndex(uint32_t offset, uint32_t day);
    static void InsertEvent(CalendarDayEvent* event, int32_t table);
    static bool ValidateCreateDate(const WowTime* date);
    static bool CanEditCurrentEvent();
    static void SetInviteStatus(uint32_t index, uint32_t status);
    static void SetModeratorStatus(uint32_t index, uint32_t status);
    static void SetActionPending(uint8_t pending);
    static const char* GetCalendarTypeString(uint32_t flags);
    static uint32_t GetDateKey(uint32_t time);
    static CalendarDayEvent* GetDayEvent(uint32_t offset, uint32_t day, uint32_t index);
    static void GetEvent(uint32_t offset, uint32_t day, uint32_t index);
    static bool SetEventDate(uint32_t month, uint32_t monthDay, uint32_t year);
    static bool SetEventTime(uint32_t hour, uint32_t minute);
    static bool SetEventLockoutDate(uint32_t month, uint32_t monthDay, uint32_t year);
    static bool SetEventLockoutTime(uint32_t hour, uint32_t minute);
    static void SetEventTextureID(uint32_t index);
    static void RemoveEvent();
    static int32_t GetTextureIndex(uint32_t type, int32_t textureID);
    static EventInviteInfo* GetCurrentEventInvite(WOWGUID guid);
    static void RemoveInvite(uint32_t index);
    static void SelectContextEvent(uint32_t offset, uint32_t day, uint32_t index);
    static void ContextMenuInviteRSVP(CalendarDayEvent* event, uint32_t status);
    static void GetCalendar();
    static void AddEvent();
    static void CreatePlayerEvent();
    static void CreateGuildAnnouncement();
    static void CreateGuildEvent();
    static void GuildFilter(uint32_t minLevel, uint32_t maxLevel, uint32_t minRank);
    static void ArenaTeam(uint32_t index);
    static void UpdateEvent();
    static void Invite(const char* name);
    static void InviteRSVP(uint32_t status);
    static void EventSignUp();
    static void ContextMenuInviteRemove(CalendarDayEvent* event);
    static void ContextMenuEventSignUp(CalendarDayEvent* event, uint8_t tentative);
    static void ContextMenuEventRemove(CalendarDayEvent* event);
    static void ContextMenuEventCopy(CalendarDayEvent* event);
    static void ContextMenuEventPaste(int32_t offset, uint32_t day);
    static bool CanComplainAboutEvent(CalendarDayEvent* event);
    static void Complain(CalendarDayEvent* event);
    static bool HasPendingInvite(uint64_t eventID);
    static void InvalidatePlayer(WOWGUID guid);
    static void AddPendingInvite(CalendarDayEvent* event);
    static bool DeletePendingInvite(uint64_t eventID);
    static void SetInviteFlag(uint64_t eventID, uint32_t flag, bool set);
    static void InsertLockoutEvent(int32_t mapID, int32_t difficulty, uint64_t instanceID, uint32_t time);
};

void CalendarRegisterScriptFunctions();
void CalendarUnregisterScriptFunctions();
const char* GetHolidayName(int32_t holidayId);
const char* GetEventTypeString(uint32_t type);
bool GameCalendarGetMaxCreateDate(WowTime* date);
const char* GetHolidayDescription(int32_t holidayId);
DYNAMICHOLIDAYHASH* CalendarGetHolidayInfo(int32_t holidayId);
const char* GetEventRepeatString(uint32_t repeat);

extern const char* s_weekdayNames[7];
extern CalendarEventData s_eventClipboard;
extern TSHashTable<DYNAMICHOLIDAYHASH, HASHKEY_NONE> s_dynamicHolidays;
extern TSFixedArray<int32_t> s_eventTextures[2];

#endif
