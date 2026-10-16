#ifndef WOW_SOURCE_GAME_GAMETIME_H
#define WOW_SOURCE_GAME_GAMETIME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Common/WowTime.h"

class CGameTime;

struct TIMESTAMPCALLBACK : public TSLinkedNode<TIMESTAMPCALLBACK> {
    /* 0008 */ void* m_param;
    /* 000C */ void (__stdcall* m_callback)(CGameTime* time, void* param);
};

class TIMESTAMPSTRUCT : public TSHashObject<TIMESTAMPSTRUCT, HASHKEY_NONE> {
    public:
    /* 0018 */ TSList<TIMESTAMPCALLBACK, TSGetLink<TIMESTAMPCALLBACK>> m_callbacks;
    /* 0024 */ uint32_t unused_0024[3];

    ~TIMESTAMPSTRUCT();
};

class CGameTime : public WowTime {
    public:
    /* 0020 */ int32_t unused_0020;
    /* 0024 */ int32_t m_minuteOffset;
    /* 0028 */ int32_t m_dayOffset;
    /* 002C */ int32_t m_minuteTicks;
    /* 0030 */ float m_minutesPerSecond;
    /* 0034 */ float m_fracMinutes;
    /* 0038 */ uint32_t m_elapsedMinutes;
    /* 003C */ uint32_t m_lastTickMs;
    /* 0040 */ uint8_t m_running;
    /* 0041 */ uint8_t unused_0041[3];
    /* 0044 */ float m_baseMinuteOfDay;
    /* 0048 */ float unused_0048;
    /* 004C */ float unused_004C;
    /* 0050 */ TSHashTable<TIMESTAMPSTRUCT, HASHKEY_NONE> m_timeStamps;

    static void __stdcall GameTimeUnregisterCallback(TIMESTAMPCALLBACK* callback);

    CGameTime();
    float GameTimeGetDayProgression();
    TIMESTAMPCALLBACK* GameTimeRegisterCallback(const WowTime* time, void (__stdcall* callback)(CGameTime* time, void* param), void* param);
    float GameTimeSetMinutesPerSecond(float minutesPerSecond);
    void GameTimeSetTime(const WowTime& time, bool tick);
    void GameTimeSync(const WowTime& time, bool a2);
    void GameTimeUpdate(float elapsed);
    bool IsDaytime();
    void TickMinute();
    void UpdateTimeStamps(int32_t minutes);
};

extern CGameTime g_clientGameTime;

void ClientDestroyGameTime();
int32_t ClientGameTimeTickHandler(const void* data, void* param);
void ClientInitializeGameTime();

#endif
