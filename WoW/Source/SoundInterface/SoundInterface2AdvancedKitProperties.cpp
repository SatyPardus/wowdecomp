#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include "WoW/Source/SoundInterface/SoundInterface2AdvancedKitProperties.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/SoundInterface/SoundInterface2Internal.h"

// STATIC: 0x00AC390C
static uint32_t s_nextAdvancedId = 1;

// STATIC: 0x00AC3924
static TSList<SuppressorSound, TSGetLink<SuppressorSound> > s_suppressorSounds;

// STATIC: 0x00B4AD24
static HASHKEY_NONE s_vsoundKey;

// STATIC: 0x00B4AD28
static int32_t s_dayTime;

// STATIC: 0x00B4AD2C
static TSHashTable<VSOUND_LOOKUP, HASHKEY_NONE>* s_vsoundLookup;

// STATIC: 0x00B4AD38
static int32_t s_lastDayTime;

// VERIFY: 3271634f auto
// OFFSET: 0x004C9CD0
void SI2::DestroyVSoundLookup() {
    if (s_vsoundLookup) {
        delete s_vsoundLookup;
        s_vsoundLookup = nullptr;
    }
}

// VERIFY: 80cb0ed8 auto
// OFFSET: 0x004C9CF0
static int32_t IsInTimeWindow(SI2AdvancedSound* sound) {
    int32_t offset = sound->m_randomOffset;
    int32_t now = s_dayTime;

    if (sound->m_timeD + offset >= 86400000 && now < offset + sound->m_timeA) {
        now += 86400000;
    }

    return (sound->m_timeA + offset <= now && sound->m_timeD + offset >= now) || (!sound->m_timeA && !sound->m_timeB && !sound->m_timeC && !sound->m_timeD);
}

// VERIFY: 42218333 auto
// OFFSET: 0x004C9D50
static void StopAdvancedSoundObject(SI2AdvancedSound* sound, int32_t immediate) {
    if (!sound) {
        return;
    }

    sound->m_stopped = 1;

    if (sound->m_object) {
        SI2::StopSound(sound->m_object, immediate, -1.0f, 0);
    }
}

// VERIFY: b3211280 manual
// OFFSET: 0x004C9D90
static int32_t RandomBetween(int32_t min, int32_t max) {
    if (max > min) {
        return min + static_cast<int32_t>((static_cast<uint64_t>(static_cast<uint32_t>(max - min)) * CRandom::uint32_(g_rndSeed)) >> 32);
    }

    return max;
}

// VERIFY: 3233d57a auto
// OFFSET: 0x004C9DC0
int32_t SI2AdvancedSound::Play() {
    SE2SoundKitProperties properties;

    if (this->m_usage) {
        if (static_cast<uint32_t>(this->m_usage - 1) > 1) {
            return 0;
        }

        properties.ResetToDefaults();
        properties.m_fadeInTime = 0.01f;
        properties.m_fadeOutTime = 0.01f;
        properties.m_group = this->m_volumeSliderCategory;
        properties.m_coneInsideAngle = this->m_insideAngle;
        properties.m_loopMode = 2;
        properties.m_coneOutsideAngle = this->m_outsideAngle;
        properties.m_maxAudibleBehavior = 1;
        properties.m_streaming = false;
        properties.m_coneOutsideVolume = this->m_outsideVolume;
        properties.m_software = 1;
        properties.m_coneOrientation = (0.0f == this->m_orientation.x && 0.0f == this->m_orientation.y && 0.0f == this->m_orientation.z) ? nullptr : &this->m_orientation;

        const C3Vector* position = (0.0f == this->m_position.x && 0.0f == this->m_position.y && 0.0f == this->m_position.z) ? nullptr : &this->m_position;

        return SI2::PlaySoundKit(this->m_soundKitId, position, this->m_object, &properties, this->m_id, &this->m_soundId, 1, 1);
    }

    properties.ResetToDefaults();
    properties.m_loopMode = 1;
    properties.m_maxAudibleBehavior = 0;
    properties.m_streaming = true;
    properties.m_software = 1;

    float fade = this->m_quickFade ? 0.7f : 3.0f;
    properties.m_fadeInTime = fade;
    properties.m_fadeOutTime = fade;
    properties.m_group = this->m_volumeSliderCategory;
    properties.m_coneInsideAngle = this->m_insideAngle;
    properties.m_coneOutsideAngle = this->m_outsideAngle;
    properties.m_coneOutsideVolume = this->m_outsideVolume;
    properties.m_coneOrientation = (0.0f == this->m_orientation.x && 0.0f == this->m_orientation.y && 0.0f == this->m_orientation.z) ? nullptr : &this->m_orientation;

    return SI2::PlaySoundKit(this->m_soundKitId, &this->m_position, this->m_object, &properties, this->m_id, &this->m_soundId, 1, 1);
}

// VERIFY: 762dbd2f manual
// OFFSET: 0x004C9FE0
static float GetDuckVolume(int32_t group, SI2AdvancedSound* exclude) {
    float volume = 1.0f;

    for (SuppressorSound* node = s_suppressorSounds.Head(); node; node = s_suppressorSounds.Next(node)) {
        if (node->m_sound && node->m_sound == exclude) {
            volume = 1.0f;
        } else if (node->m_volume[group] < volume) {
            volume = node->m_volume[group];
        }
    }

    return volume;
}

// VERIFY: 4f46a9dd auto
// OFFSET: 0x004CA040
SI2AdvancedSound::SI2AdvancedSound() {
    this->m_position.x = 0.0f;
    this->m_position.y = 0.0f;
    this->m_position.z = 0.0f;
    this->m_orientation.x = 0.0f;
    this->m_orientation.y = 0.0f;
    this->m_orientation.z = 0.0f;
    this->m_soundId = 0;
    this->m_stopped = 0;
    this->m_finished = 0;
    this->m_innerRadius2d = 0.0f;
    this->m_outerRadius2d = 0.0f;
}

// VERIFY: d2749ee6 manual
// OFFSET: 0x004CA080
SI2AdvancedSound::~SI2AdvancedSound() {
    this->m_inRange = 0;

    for (SuppressorSound* node = s_suppressorSounds.Head(); node;) {
        SuppressorSound* next = s_suppressorSounds.Next(node);

        if (node->m_sound == this) {
            node->m_sound = nullptr;
        }

        node = next;
    }

    if (this->m_object) {
        this->m_object->m_advancedId = 0;
    }

    SE2SoundKitObject* object = this->m_object;

    if (object && !this->m_externalObject) {
        delete object;
        this->m_object = nullptr;
    }
}

// VERIFY: 1ee25e28 auto
// OFFSET: 0x004CA130
int32_t SI2::IsAdvancedSoundNear(int32_t soundKitId, const C3Vector* position) {
    if (!s_vsoundLookup) {
        return 1;
    }

    SE2::LockSoundList();

    for (VSOUND_LOOKUP* node = s_vsoundLookup->m_fulllist.Head(); node; node = s_vsoundLookup->m_fulllist.Next(node)) {
        SI2AdvancedSound* sound = node->m_sound;

        if (sound->m_rec->m_id == soundKitId) {
            float dx = position->x - sound->m_position.x;
            float dy = position->y - sound->m_position.y;
            float dz = position->z - sound->m_position.z;

            if (dz * dz + dy * dy + dx * dx < 6.0f) {
                SE2::UnlockSoundList();
                return 1;
            }
        }
    }

    SE2::UnlockSoundList();
    return 0;
}

// VERIFY: 5e6aafa6 manual
// OFFSET: 0x004CA1E0
void SI2AdvancedSound::AddSuppressor() {
    if ((this->m_duck[0] >= 0.0f && this->m_duck[0] < 1.0f) || (this->m_duck[1] >= 0.0f && this->m_duck[1] < 1.0f) || (this->m_duck[2] >= 0.0f && this->m_duck[2] < 1.0f)) {
        for (SuppressorSound* node = s_suppressorSounds.Head(); node; node = s_suppressorSounds.Next(node)) {
            if (node->m_sound == this) {
                return;
            }
        }

        SuppressorSound* node = new (SMemAlloc(sizeof(SuppressorSound), __FILE__, __LINE__, 0)) SuppressorSound();
        node->m_sound = this;
        node->m_fadeTime = this->m_timeToUnduck;
        s_suppressorSounds.LinkToTail(node);
    }
}

// VERIFY: e5be26cc auto
// OFFSET: 0x004CA4E0
void SI2::ClearSuppressors() {
    s_suppressorSounds.Clear();
}

// VERIFY: bdd84a6b manual
// OFFSET: 0x004CA4F0
void SI2::UpdateDucking(int32_t elapsed) {
    C3Vector listener;
    listener.x = 0.0f;
    listener.y = 0.0f;
    listener.z = 0.0f;

    SE2::GetListenerPosition(&listener);

    for (SuppressorSound* node = s_suppressorSounds.Head(); node;) {
        SuppressorSound* next = s_suppressorSounds.Next(node);

        for (int32_t i = 0; i < 3; i++) {
            SI2AdvancedSound* sound = node->m_sound;
            float* volume = &node->m_volume[i];

            if (sound) {
                bool atOrigin = 0.0f == sound->m_position.x && 0.0f == sound->m_position.y && 0.0f == sound->m_position.z;
                float dx = listener.x - sound->m_position.x;
                float dy = listener.y - sound->m_position.y;
                float dz = listener.z - sound->m_position.z;
                float distance = sqrt(dz * dz + dy * dy + dx * dx);
                node->m_distance = distance;

                if (atOrigin || distance < sound->m_innerRadiusOfInfluence || (distance < sound->m_outerRadiusOfInfluence && sound->m_inRange)) {
                    if (sound->m_duck[i] != *volume) {
                        int32_t time = sound->m_timeToDuck;

                        if (time > 0) {
                            *volume = *volume - static_cast<float>(elapsed) / time;

                            if (*volume < node->m_sound->m_duck[i]) {
                                *volume = node->m_sound->m_duck[i];
                                node->m_sound->m_inRange = 1;
                                continue;
                            }
                        } else {
                            *volume = sound->m_duck[i];
                        }
                    }

                    node->m_sound->m_inRange = 1;
                } else {
                    sound->m_inRange = 0;
                    SI2AdvancedSound* removed = node->m_sound;

                    for (SuppressorSound* other = s_suppressorSounds.Head(); other;) {
                        SuppressorSound* otherNext = s_suppressorSounds.Next(other);

                        if (other->m_sound == removed) {
                            other->m_sound = nullptr;
                        }

                        other = otherNext;
                    }
                }
            } else if (1.0f != *volume) {
                int32_t time = node->m_fadeTime;

                if (time <= 0 || (*volume = static_cast<float>(elapsed) / time + *volume) > 1.0f) {
                    *volume = 1.0f;
                }
            }
        }

        if (!node->m_sound && 1.0f == node->m_volume[0] && 1.0f == node->m_volume[1] && 1.0f == node->m_volume[2]) {
            delete node;
        }

        node = next;
    }
}

// VERIFY: 2e8555db manual
// OFFSET: 0x004CA7B0
int32_t SI2::UpdateAdvancedSounds(const void* data, void* param) {
    int32_t now = static_cast<int32_t>(g_clientGameTime.GameTimeGetDayProgression() * 86400000.0f);
    int32_t elapsed = now - s_lastDayTime;
    s_dayTime = now;

    if (elapsed < 0) {
        elapsed = 0;
    }

    s_lastDayTime = now;

    SI2::UpdateDucking(elapsed);

    int32_t created = 0;
    SE2::LockSoundList();

    VSOUND_LOOKUP* node = s_vsoundLookup->m_fulllist.Head();

    while (node) {
        SI2AdvancedSound* sound = node->m_sound;

        if (sound->m_finished) {
            VSOUND_LOOKUP* next = s_vsoundLookup->m_fulllist.Next(node);
            s_vsoundLookup->DeleteNode(node);
            node = next;
            continue;
        }

        SESound* object = sound->m_object ? &sound->m_object->m_sound : nullptr;
        created = 0;

        if (object && object->IsPlaying()) {
            if (!IsInTimeWindow(sound)) {
                node = s_vsoundLookup->m_fulllist.Next(node);

                if (sound->m_usage) {
                    continue;
                }

                int32_t immediate = sound->m_volume <= 0.0f;
                sound->m_stopped = 1;

                if (sound->m_object) {
                    SI2::StopSound(sound->m_object, immediate, -1.0f, 0);
                }

                continue;
            }

            SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(sound->m_object->m_sound.GetUserData());

            if (!sound->m_usage && userData && userData->m_def->m_numFiles > 1 && sound->m_intervalMin > 0.0f && sound->m_intervalMax > 0.0f) {
                sound->m_nextInterval -= elapsed;

                if (sound->m_nextInterval < 0) {
                    StopAdvancedSoundObject(sound, 0);
                    sound->Play();
                    sound->m_nextInterval = RandomBetween(sound->m_intervalMin, sound->m_intervalMax);
                }
            }

            SESound* live = SE2::GetSoundFromID(node->m_sound->m_soundId, &created);

            if (live) {
                float pan = 1.0f;

                if (node->m_sound->m_outerRadius2d > 0.0f) {
                    C3Vector listener;
                    listener.x = 0.0f;
                    listener.y = 0.0f;
                    listener.z = 0.0f;
                    SI2::GetListenerPosition(&listener);

                    SI2AdvancedSound* advanced = node->m_sound;
                    float dx = listener.x - advanced->m_position.x;
                    float dy = listener.y - advanced->m_position.y;
                    float dz = listener.z - advanced->m_position.z;
                    float distanceSq = dz * dz + dy * dy + dx * dx;
                    float level;

                    if (advanced->m_innerRadius2d * advanced->m_innerRadius2d < distanceSq) {
                        level = node->m_sound->m_outerRadius2d * node->m_sound->m_outerRadius2d < distanceSq ? 1.0f : (sqrt(distanceSq) - node->m_sound->m_innerRadius2d) / (node->m_sound->m_outerRadius2d - node->m_sound->m_innerRadius2d);
                    } else {
                        level = 0.0f;
                    }

                    pan = level;

                    if (!(node->m_sound->m_innerRadiusOfInfluence * node->m_sound->m_innerRadiusOfInfluence < distanceSq)) {
                        node->m_sound->AddSuppressor();
                        pan = level;
                    }
                }

                live->Set3DPanLevel(pan);

                if (created) {
                    live->Detach();
                    delete live;
                }
            }

            if (sound->m_usage != 2 && (sound->m_timeA || sound->m_timeB || sound->m_timeC || sound->m_timeD)) {
                int32_t offset = sound->m_randomOffset;
                int32_t time = s_dayTime;

                if (sound->m_timeD + offset >= 86400000 && time < offset + sound->m_timeA) {
                    time += 86400000;
                }

                if (time < offset + sound->m_timeB) {
                    sound->m_volume = static_cast<float>(time - sound->m_timeA) / (sound->m_timeB - sound->m_timeA);

                    if (sound->m_volume > 1.0f) {
                        sound->m_volume = 1.0f;
                    }

                    if (sound->m_volume < 0.0f) {
                        sound->m_volume = 0.0f;
                    }
                } else if (time < offset + sound->m_timeC) {
                    sound->m_volume = 1.0f;
                } else {
                    sound->m_volume = 1.0f - static_cast<float>(time - sound->m_timeC) / (sound->m_timeD - sound->m_timeC);

                    if (sound->m_volume > 1.0f) {
                        sound->m_volume = 1.0f;
                    }

                    if (sound->m_volume < 0.0f) {
                        sound->m_volume = 0.0f;
                    }
                }
            }

            node = s_vsoundLookup->m_fulllist.Next(node);
            continue;
        }

        SI2AdvancedSound* advanced = node->m_sound;
        int32_t usage = advanced->m_usage;

        if (!advanced->m_object || usage == 2 || (!usage && advanced->m_stopped)) {
            VSOUND_LOOKUP* next = s_vsoundLookup->m_fulllist.Next(node);
            s_vsoundLookup->DeleteNode(node);
            node = next;
            continue;
        }

        if (IsInTimeWindow(advanced)) {
            if (!usage) {
                node->m_sound->Play();
            } else if (usage == 1) {
                node->m_sound->m_nextInterval -= elapsed;

                if (node->m_sound->m_nextInterval < 0) {
                    node->m_sound->Play();
                    sound->m_nextInterval = RandomBetween(sound->m_intervalMin, sound->m_intervalMax);
                }
            }
        }

        node = s_vsoundLookup->m_fulllist.Next(node);
    }

    SE2::UnlockSoundList();
    return 1;
}

// VERIFY: 3a7d0c7f manual
// OFFSET: 0x004CB160
void SI2::CreateVSoundLookup() {
    if (!s_vsoundLookup) {
        s_vsoundLookup = new (SMemAlloc(sizeof(TSHashTable<VSOUND_LOOKUP, HASHKEY_NONE>), __FILE__, __LINE__, 0)) TSHashTable<VSOUND_LOOKUP, HASHKEY_NONE>();
    }
}

// VERIFY: 00aa6854 auto
// OFFSET: 0x004CB1A0
void SI2::MarkAdvancedSoundStopped(uint32_t id) {
    SE2::LockSoundList();

    VSOUND_LOOKUP* node = s_vsoundLookup->Ptr(id, s_vsoundKey);

    if (node) {
        node->m_sound->m_finished = 1;
    }

    SE2::UnlockSoundList();
}

// VERIFY: 4e03a953 auto
// OFFSET: 0x004CB1D0
void SI2::StopAdvancedSound(uint32_t id) {
    if (!id || !s_vsoundLookup) {
        return;
    }

    SE2::LockSoundList();

    TSHashTable<VSOUND_LOOKUP, HASHKEY_NONE>* lookup = s_vsoundLookup;
    VSOUND_LOOKUP* node = lookup->Ptr(id, s_vsoundKey);

    if (node) {
        lookup->DeleteNode(node);
    }

    SE2::UnlockSoundList();
}

// VERIFY: 9c993a65 auto
// OFFSET: 0x004CB220
float SI2::GetSoundVolume(SESoundUserData* userData) {
    if (!userData) {
        return 1.0f;
    }

    SI2SoundUserData* data = static_cast<SI2SoundUserData*>(userData);
    float volume = 1.0f;
    SI2AdvancedSound* sound;

    if (s_vsoundLookup) {
        SE2::LockSoundList();
        VSOUND_LOOKUP* node = s_vsoundLookup->Ptr(data->m_advancedId, s_vsoundKey);
        SE2::UnlockSoundList();

        if (node) {
            sound = node->m_sound;

            if (sound) {
                volume = sound->m_volume;
            }
        } else {
            sound = nullptr;
        }
    } else {
        sound = nullptr;
    }

    int32_t group = data->m_group;

    if (!group || group >= 7) {
        return GetDuckVolume(0, sound) * volume;
    }

    if (group == 1) {
        return GetDuckVolume(1, sound) * volume;
    }

    if (group == 2) {
        return GetDuckVolume(2, sound) * volume;
    }

    return volume;
}

// VERIFY: c3f6bde5 manual
// OFFSET: 0x004CB2D0
void SI2::SetAdvancedSoundPosition(uint32_t id, const C3Vector* position) {
    SE2::LockSoundList();

    VSOUND_LOOKUP* node = s_vsoundLookup->Ptr(id, s_vsoundKey);

    if (node) {
        node->m_sound->m_position = *position;
    }

    SE2::UnlockSoundList();
}

// VERIFY: 05979585 auto
// OFFSET: 0x004CB3B0
void SI2::ClearSuppressorSounds() {
    s_suppressorSounds.Clear();

    if (!s_vsoundLookup) {
        return;
    }

    SE2::LockSoundList();

    TSHashTable<VSOUND_LOOKUP, HASHKEY_NONE>* lookup = s_vsoundLookup;

    for (VSOUND_LOOKUP* node = lookup->Head(); node; node = lookup->Head()) {
        uint32_t id = node->m_sound->m_id;

        if (id && lookup) {
            SI2::StopAdvancedSound(id);
            lookup = s_vsoundLookup;
        }
    }

    SE2::UnlockSoundList();
}

// VERIFY: 524c166b auto
// OFFSET: 0x004CB440
void SI2AdvancedSound::Stop(int32_t remove, float fadeOutTime) {
    if (remove) {
        SI2::StopAdvancedSound(this->m_id);
        return;
    }

    this->m_stopped = 1;

    if (this->m_object) {
        if (fadeOutTime >= 0.0f) {
            this->m_object->m_sound.SetFadeOutTime(fadeOutTime);
        }

        this->m_object->m_sound.FadeOutNow();
    }
}

// VERIFY: 8741a1fc auto
// OFFSET: 0x004CB4E0
void SI2::StopAdvancedSound(uint32_t id, int32_t remove, float fadeOutTime) {
    SE2::LockSoundList();

    VSOUND_LOOKUP* node = s_vsoundLookup->Ptr(id, s_vsoundKey);

    if (node) {
        node->m_sound->Stop(remove, fadeOutTime);
    }

    SE2::UnlockSoundList();
}

// VERIFY: 87c34bf4 manual
// OFFSET: 0x004CB580
int32_t SI2::AddAdvancedSound(SoundEntriesAdvancedRec* rec, int32_t soundKitId, const C3Vector* position, const C3Vector* orientation, SE2SoundKitObject* object, int32_t quickFade, int32_t a7, int32_t checkDistance, int32_t forceLoop) {
    if (!rec) {
        return 24;
    }

    if (!s_vsoundLookup) {
        return 17;
    }

    C3Vector zero;
    zero.x = 0.0f;
    zero.y = 0.0f;
    zero.z = 0.0f;

    if (!orientation) {
        orientation = &zero;
    }

    if (position && !quickFade && checkDistance && SI2::IsAdvancedSoundNear(rec->m_id, position)) {
        return 15;
    }

    SI2AdvancedSound* sound = new (SMemAlloc(sizeof(SI2AdvancedSound), __FILE__, __LINE__, 0)) SI2AdvancedSound();

    if (!sound) {
        return 22;
    }

    if (object) {
        sound->m_object = object;
        sound->m_externalObject = 1;
    } else {
        sound->m_externalObject = 0;
        sound->m_object = new (SMemAlloc(sizeof(SE2SoundKitObject), __FILE__, __LINE__, 0)) SE2SoundKitObject();

        if (!sound->m_object) {
            sound->~SI2AdvancedSound();
            SMemFree(sound, "delete", -1, 0);
            return 22;
        }
    }

    sound->m_quickFade = quickFade;
    sound->m_rec = rec;
    sound->m_soundKitId = soundKitId;
    sound->m_innerRadius2d = rec->m_innerRadius2d;
    sound->m_outerRadius2d = rec->m_outerRadius2d;
    sound->m_timeA = rec->m_timeA;
    sound->m_timeB = rec->m_timeB;
    sound->m_timeC = rec->m_timeC;
    sound->m_timeD = rec->m_timeD;

    if (sound->m_timeB < sound->m_timeA) {
        sound->m_timeB = rec->m_timeB + 86400000;
    }

    if (sound->m_timeC < sound->m_timeB) {
        sound->m_timeC = rec->m_timeC + 86400000;
    }

    if (sound->m_timeD < sound->m_timeC) {
        sound->m_timeD += 86400000;
    }

    sound->m_randomOffsetRange = rec->m_randomOffsetRange;
    sound->m_volume = 1.0f;
    sound->m_randomOffset = RandomBetween(0, sound->m_randomOffsetRange);
    sound->m_usage = forceLoop ? 0 : rec->m_usage;
    sound->m_intervalMin = rec->m_timeIntervalMin;
    sound->m_intervalMax = rec->m_timeIntervalMax;
    sound->m_nextInterval = RandomBetween(sound->m_intervalMin, sound->m_intervalMax);
    sound->m_volumeSliderCategory = rec->m_volumeSliderCategory;
    sound->m_duck[0] = (rec->m_duckToSfx < 0.0f || rec->m_duckToSfx > 1.0f) ? 1.0f : rec->m_duckToSfx;
    sound->m_duck[1] = (rec->m_duckToMusic < 0.0f || rec->m_duckToMusic > 1.0f) ? 1.0f : rec->m_duckToMusic;
    sound->m_duck[2] = (rec->m_duckToAmbience < 0.0f || rec->m_duckToAmbience > 1.0f) ? 1.0f : rec->m_duckToAmbience;
    sound->m_innerRadiusOfInfluence = rec->m_innerRadiusOfInfluence;
    sound->m_outerRadiusOfInfluence = rec->m_outerRadiusOfInfluence;

    if (sound->m_outerRadiusOfInfluence < sound->m_innerRadiusOfInfluence) {
        sound->m_innerRadiusOfInfluence = sound->m_outerRadiusOfInfluence;
    }

    sound->m_inRange = 0;
    sound->m_timeToDuck = rec->m_timeToDuck;
    sound->m_timeToUnduck = rec->m_timeToUnduck;
    sound->m_insideAngle = rec->m_insideAngle;
    sound->m_outsideAngle = rec->m_outsideAngle;

    if (sound->m_outsideAngle < sound->m_insideAngle) {
        sound->m_insideAngle = sound->m_outsideAngle;
    }

    sound->m_outsideVolume = rec->m_outsideVolume;

    if (position) {
        sound->m_position = *position;
    } else {
        sound->m_position.x = 0.0f;
        sound->m_position.y = 0.0f;
        sound->m_position.z = 0.0f;
    }

    sound->m_orientation = *orientation;
    sound->unused_0008 = a7;

    sound->m_id = s_nextAdvancedId;
    sound->m_object->m_advancedId = s_nextAdvancedId;
    s_nextAdvancedId++;

    uint32_t id = sound->m_id;
    VSOUND_LOOKUP* node = s_vsoundLookup->InternalNewNode(id, 0, 0);
    node->m_hashval = id;
    node->m_sound = sound;

    if (sound->m_usage == 2) {
        sound->Play();

        if (SI2::IsPlaying(sound->m_object)) {
            sound->AddSuppressor();
        }
    }

    return 0;
}

// INLINED
SuppressorSound::SuppressorSound() {
    this->m_volume[0] = 1.0f;
    this->m_volume[1] = 1.0f;
    this->m_volume[2] = 1.0f;
    this->m_sound = nullptr;
    this->m_distance = 0.0f;
    this->m_fadeTime = 1000;
}

// INLINED
VSOUND_LOOKUP::~VSOUND_LOOKUP() {
    if (this->m_sound) {
        delete this->m_sound;
        this->m_sound = nullptr;
    }
}
