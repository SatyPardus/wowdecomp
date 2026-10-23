#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2ZONESOUNDS_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2ZONESOUNDS_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

class SE2SoundKitProperties;

struct SI2ZoneSoundInfo {
    /* 0000 */ int32_t m_music[NUM_AMBIENCES];
    /* 0008 */ int32_t m_silenceMin[NUM_AMBIENCES];
    /* 0010 */ int32_t m_silenceMax[NUM_AMBIENCES];
    /* 0018 */ int32_t m_ambience[NUM_AMBIENCES];
    /* 0020 */ int32_t m_reverb;

    SI2ZoneSoundInfo();
    ~SI2ZoneSoundInfo();
};

struct SI2ZoneMusicTimer {
    /* 0000 */ int32_t m_soundKitId;
    /* 0004 */ int32_t m_silenceMin;
    /* 0008 */ int32_t m_silenceMax;
};

class SI2ZoneSound {
    public:
    /* 0000 */ int32_t m_soundKitId;
    /* 0004 */ int32_t m_type;
    /* 0008 */ SE2SoundKitObject m_object;

    SI2ZoneSound();
};

class SI2ZoneSoundPair {
    public:
    /* 0000 */ SI2ZoneSound m_sounds[2];
    /* 0038 */ uint32_t m_current;
    /* 003C */ uint32_t m_next;

    SI2ZoneSoundPair();
    ~SI2ZoneSoundPair();
    int32_t GetCurrentType();
    int32_t GetPlayingSoundKitId();
    bool IsPlaying();
    void SetFinishedCallback(void (*callback)(SESound*, void*), void* param);
    void Stop(int32_t immediate);
    int32_t Transition(int32_t soundKitId, int32_t type, SE2SoundKitProperties* properties, int32_t immediate, float fadeOutTime);
};

#endif
