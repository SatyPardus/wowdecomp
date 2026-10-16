#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2ADVANCEDKITPROPERTIES_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2ADVANCEDKITPROPERTIES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class SE2SoundKitObject;
class SI2AdvancedSound;
class SoundEntriesAdvancedRec;

class SI2AdvancedSound {
    public:
    /* 0000 */ TSLink<SI2AdvancedSound> m_link;
    /* 0008 */ int32_t unused_0008;
    /* 000C */ int32_t m_stopped;
    /* 0010 */ int32_t m_finished;
    /* 0014 */ SE2SoundKitObject* m_object;
    /* 0018 */ int32_t m_externalObject;
    /* 001C */ SoundEntriesAdvancedRec* m_rec;
    /* 0020 */ int32_t m_soundKitId;
    /* 0024 */ uint32_t m_soundId;
    /* 0028 */ C3Vector m_position;
    /* 0034 */ float m_innerRadius2d;
    /* 0038 */ float m_outerRadius2d;
    /* 003C */ int32_t m_timeA;
    /* 0040 */ int32_t m_timeB;
    /* 0044 */ int32_t m_timeC;
    /* 0048 */ int32_t m_timeD;
    /* 004C */ int32_t m_randomOffsetRange;
    /* 0050 */ int32_t m_randomOffset;
    /* 0054 */ float m_volume;
    /* 0058 */ int32_t m_usage;
    /* 005C */ int32_t m_nextInterval;
    /* 0060 */ int32_t m_intervalMin;
    /* 0064 */ int32_t m_intervalMax;
    /* 0068 */ int32_t m_volumeSliderCategory;
    /* 006C */ float m_duck[3];
    /* 0078 */ float m_innerRadiusOfInfluence;
    /* 007C */ float m_outerRadiusOfInfluence;
    /* 0080 */ int32_t m_inRange;
    /* 0084 */ int32_t m_timeToDuck;
    /* 0088 */ int32_t m_timeToUnduck;
    /* 008C */ float m_insideAngle;
    /* 0090 */ float m_outsideAngle;
    /* 0094 */ float m_outsideVolume;
    /* 0098 */ C3Vector m_orientation;
    /* 00A4 */ int32_t m_quickFade;
    /* 00A8 */ uint32_t m_id;

    SI2AdvancedSound();
    ~SI2AdvancedSound();
    void AddSuppressor();
    int32_t Play();
    void Stop(int32_t remove, float fadeOutTime);
};

class SuppressorSound : public TSLinkedNode<SuppressorSound> {
    public:
    /* 0008 */ SI2AdvancedSound* m_sound;
    /* 000C */ float m_volume[3];
    /* 0018 */ float m_distance;
    /* 001C */ int32_t m_fadeTime;

    SuppressorSound();
};

struct VSOUND_LOOKUP : public TSHashObject<VSOUND_LOOKUP, HASHKEY_NONE> {
    /* 0018 */ SI2AdvancedSound* m_sound;

    ~VSOUND_LOOKUP();
};

#endif
