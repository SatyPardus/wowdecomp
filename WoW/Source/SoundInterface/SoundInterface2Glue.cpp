#include <cstddef>
#include <cstdint>
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/SoundInterface/SoundInterface2Internal.h"

static const float FADEOUT_TIME = 3.0f;

// STATIC: 0x00DCE8E8
static char s_creditsMusicName[128];

// STATIC: 0x00DCE968
static char s_glueMusicName[128];

// STATIC: 0x00DCE9E8
static char s_glueAmbienceName[128];

// STATIC: 0x00DCEA68
static SE2SoundKitObject s_creditsMusic;

// STATIC: 0x00DCEA7C
static SE2SoundKitObject s_glueMusic;

// STATIC: 0x00DCEA90
static SE2SoundKitObject s_glueAmbience;

// VERIFY: f19203b8 auto
// OFFSET: 0x00985F70
void SI2::StopGlueAmbience(float fadeOutTime) {
    if (fadeOutTime < 0.0f) {
        fadeOutTime = -1.0f;
    }

    SI2::StopSound(&s_glueAmbience, 0, fadeOutTime, 1);
}

// VERIFY: 57028033 auto
// OFFSET: 0x00985FB0
void SI2::StartGlueAmbience(const char* name, float fadeInTime) {
    if (SI2::IsPlaying(&s_glueAmbience)) {
        VSOUNDKITDEF* def = SI2::GetSoundKitDef(SI2::LookupSoundKitId(name));
        SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(s_glueAmbience.m_sound.GetUserData());

        if (def && userData && def->m_id == userData->m_soundKitId) {
            return;
        }
    }

    SI2::StopSound(&s_glueAmbience, 0, FADEOUT_TIME, 1);

    if (!name) {
        return;
    }

    SStrCopy(s_glueAmbienceName, name, sizeof(s_glueAmbienceName));

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_fadeInTime = fadeInTime;
    properties.m_group = 2;
    properties.m_loopMode = 1;
    properties.m_decodeBufferSize = 0;

    SI2::PlaySoundKit(s_glueAmbienceName, nullptr, &s_glueAmbience, &properties);
}

// VERIFY: ba080d04 auto
// OFFSET: 0x00986080
int32_t SI2::GlueMusicUpdate(const void* data, void* param) {
    if (!SI2::IsPlaying(&s_glueMusic)) {
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_group = 1;
        properties.m_decodeBufferSize = 0;

        SI2::PlaySoundKit(s_glueMusicName, nullptr, &s_glueMusic, &properties);
    }

    return 1;
}

// VERIFY: 6c059ee6 auto
// OFFSET: 0x009860E0
void SI2::StopGlueMusic(float fadeOutTime) {
    EventUnregister(EVENT_ID_POLL, SI2::GlueMusicUpdate);
    s_glueMusicName[0] = '\0';

    if (fadeOutTime < 0.0f) {
        fadeOutTime = -1.0f;
    }

    SI2::StopSound(&s_glueMusic, 0, fadeOutTime, 1);
}

// VERIFY: 8d99a4e4 auto
// OFFSET: 0x00986130
int32_t SI2::CreditsMusicUpdate(const void* data, void* param) {
    if (!SI2::IsPlaying(&s_creditsMusic)) {
        SE2SoundKitProperties properties;
        properties.ResetToDefaults();
        properties.m_group = 1;
        properties.m_selectMode = 0;

        SI2::PlaySoundKit(s_creditsMusicName, nullptr, &s_creditsMusic, &properties);
    }

    return 1;
}

// VERIFY: c72e61eb auto
// OFFSET: 0x00986190
void SI2::StopCreditsMusic() {
    EventUnregister(EVENT_ID_POLL, SI2::CreditsMusicUpdate);
    SI2::StopSound(&s_creditsMusic, 0, FADEOUT_TIME, 1);
}

// VERIFY: b62c8d0a auto
// OFFSET: 0x009861C0
void SI2::StartGlueMusic(const char* name) {
    if (SI2::IsPlaying(&s_glueMusic)) {
        VSOUNDKITDEF* def = SI2::GetSoundKitDef(SI2::LookupSoundKitId(name));
        SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(s_glueMusic.m_sound.GetUserData());

        if (def && userData && def->m_id == userData->m_soundKitId) {
            return;
        }
    }

    EventUnregister(EVENT_ID_POLL, SI2::CreditsMusicUpdate);
    SI2::StopSound(&s_creditsMusic, 0, FADEOUT_TIME, 1);

    if (!name || !SStrCmpI(name, s_glueMusicName, 0x7FFFFFFF)) {
        return;
    }

    SI2::StopGlueMusic(-1.0f);
    SStrCopy(s_glueMusicName, name, sizeof(s_glueMusicName));

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_group = 1;
    properties.m_decodeBufferSize = 0;

    SI2::PlaySoundKit(s_glueMusicName, nullptr, &s_glueMusic, &properties);
    EventRegister(EVENT_ID_POLL, SI2::GlueMusicUpdate);
}

// VERIFY: f023fa69 auto
// OFFSET: 0x009862C0
void SI2::StartCreditsMusic(const char* name) {
    EventUnregister(EVENT_ID_POLL, SI2::CreditsMusicUpdate);
    SI2::StopSound(&s_creditsMusic, 0, FADEOUT_TIME, 1);
    EventUnregister(EVENT_ID_POLL, SI2::GlueMusicUpdate);
    s_glueMusicName[0] = '\0';
    SI2::StopSound(&s_glueMusic, 0, FADEOUT_TIME, 1);

    if (!name) {
        return;
    }

    SStrCopy(s_creditsMusicName, name, sizeof(s_creditsMusicName));
    SI2::ResetSoundKitVariants(s_creditsMusicName);

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_group = 1;
    properties.m_selectMode = 0;

    SI2::PlaySoundKit(s_creditsMusicName, nullptr, &s_creditsMusic, &properties);
    EventRegister(EVENT_ID_POLL, SI2::CreditsMusicUpdate);
}
