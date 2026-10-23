#include <cstddef>
#include "WoW/Source/WorldClient/WorldConstants.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>
#include "Engine/Source/Base/CmdLine.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CSimpleMovieFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Tempest/CRange.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ResistancesRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/SoundInterface/SoundInterface2AdvancedKitProperties.h"
#include "WoW/Source/SoundInterface/SoundInterface2Internal.h"
#include "WoW/Source/SoundInterface/SoundInterface2ZoneSounds.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WorldClient/AreaList.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00B24180
static const char* s_raceFileNames[] = { nullptr, "Human", "Orc", "Dwarf", "NightElf", "Scourge", "Tauren", "Gnome", "Troll", "Goblin", "BloodElf", "Draenei" };

// STATIC: 0x009F2010
const char SI2::s_channelGroupNames[18][32] = {
    "SFX",
    "MUSIC",
    "AMBIENCE",
    "CINEMATIC",
    "SCRIPTSOUND",
    "SCRIPTMUSIC",
    "RACIALCINEMATIC",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
    "SFX",
};

// STATIC: 0x009F2268
const int32_t SI2::s_channelGroupMaxSounds[18] = {
    0x7FFFFFFF,
    0x7FFFFFFF,
    0x7FFFFFFF,
    0x7FFFFFFF,
    0x7FFFFFFF,
    0x7FFFFFFF,
    1,
    1,
    2,
    1,
    2,
    2,
    1,
    6,
    4,
    1,
    2,
    4,
};

// STATIC: 0x00AC384C
static float s_subchunkRound = 0.5f;

// STATIC: 0x00B4A340
int32_t SI2::s_channelGroupNumSounds[18];

// STATIC: 0x00B4A390
TSGrowableArray<uint32_t> SI2::s_suppressors;

// STATIC: 0x00B4A3A0
static const char* s_savedVoiceSession;

// STATIC: 0x00AC3860
static int32_t s_savedVoiceSessionType = 4;

// STATIC: 0x00B4A3E4
static float s_chaosRestartTimer;

// STATIC: 0x00B4A3E8
static float s_chaosTimer;

// STATIC: 0x00B4A3EC
static int32_t s_chaosElapsedMs;

// STATIC: 0x00B4AA94
int32_t SI2::sm_bPlayerInside;

// STATIC: 0x00B4AA98
int32_t SI2::sm_bCameraIsUnderwater;

// STATIC: 0x00B4AA9C
int32_t SI2::sm_bUnderwaterChanged;

// STATIC: 0x00B4AAA0
uint32_t SI2::sm_nextZoneMusicTime;

// STATIC: 0x00B4AAA4
int32_t SI2::sm_bLuaMusicPlaying;

// VERIFY: 8cb9a901 manual
// OFFSET: 0x004C5B60
static int32_t GetDownloadPriority(int32_t group) {
    if (group <= 0 || group > 2) {
        return 5;
    }

    return 6;
}

// VERIFY: 964e6036 auto
// OFFSET: 0x004C5990
void SE2SoundKitProperties::ResetToDefaults() {
    this->m_volume = -1.0f;
    this->m_volumeScale = 1.0f;
    this->m_fileIndex = -1;
    this->m_priority = -1;
    this->m_fadeInTime = 0.0f;
    this->m_fadeOutTime = 0.0f;
    this->m_group = 0;
    this->m_selectMode = 2;
    this->m_maxDistance = -1.0f;
    this->m_loopMode = 0;
    this->m_muteFadeSpeed = 0.5f;
    this->m_suppressorMode = 0;
    this->m_decodeBufferSize = 0;
    this->m_maxAudibleBehavior = 1;
    this->m_coneInsideAngle = 360.0f;
    this->m_reverb = 1;
    this->m_coneOutsideAngle = 360.0f;
    this->m_streaming = false;
    this->m_noRandomStart = 0;
    this->m_blockMusicFiles = 0;
    this->m_software = 0;
    this->m_coneOutsideVolume = 1.0f;
    this->m_coneOrientation = nullptr;
    this->m_useDspChain = 0;
    this->m_forceLoop = 0;
    this->m_checkStreaming = SFile::IsStreamingMode() != 0;
}

// VERIFY: 24efb61f manual
// OFFSET: 0x004C5A20
float SI2::GetChannelGroupVolume(int32_t group) {
    return SE2::GetChannelGroupVolume(SI2::s_channelGroupNames[group]);
}

// VERIFY: e2c427b3 auto
// OFFSET: 0x004C5A40
void SI2::SetChannelGroupVolume(int32_t group, float volume) {
    SE2::SetChannelGroupVolume(SI2::s_channelGroupNames[group], volume);
}

// VERIFY: 44756337 auto
// OFFSET: 0x004C5A60
int32_t SI2::GetNumInputDrivers(int32_t chatSystem) {
    return SE2::GetNumInputDrivers(chatSystem);
}

// VERIFY: fc888c05 auto
// OFFSET: 0x004C5A70
int32_t SI2::GetNumInputDrivers_Cached(int32_t chatSystem) {
    return SE2::GetNumInputDrivers_Cached(chatSystem);
}

// VERIFY: 7129b2b8 auto
// OFFSET: 0x004C5A80
bool SI2::GetInputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    return SE2::GetInputDriverName(index, name, nameLength, chatSystem);
}

// VERIFY: 13e146d1 auto
// OFFSET: 0x004C5A90
bool SI2::GetInputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    return SE2::GetInputDriverName_Cached(index, name, nameLength, chatSystem);
}

// VERIFY: 13fd08cb auto
// OFFSET: 0x004C5AA0
int32_t SI2::GetNumOutputDrivers(int32_t chatSystem) {
    return SE2::GetNumOutputDrivers(chatSystem);
}

// VERIFY: 7fd5990c auto
// OFFSET: 0x004C5AB0
int32_t SI2::GetNumOutputDrivers_Cached(int32_t chatSystem) {
    return SE2::GetNumOutputDrivers_Cached(chatSystem);
}

// VERIFY: ab40eaf2 manual
// OFFSET: 0x004C5AC0
bool SI2::GetOutputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    if (index < 0) {
        index = 0;
    }

    return SE2::GetOutputDriverName(index, name, nameLength, chatSystem);
}

// VERIFY: 59635ee4 manual
// OFFSET: 0x004C5AF0
bool SI2::GetOutputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    if (index < 0) {
        index = 0;
    }

    return SE2::GetOutputDriverName_Cached(index, name, nameLength, chatSystem);
}

// VERIFY: 8c87872b auto
// OFFSET: 0x004C5B20
int32_t SI2::SetListenerAttributes(const C3Vector* position, const C3Vector* forward, const C3Vector* up, const C3Vector* velocity) {
    SE2::SetListenerAttributes(position, forward, up, velocity);
    return 0;
}

// VERIFY: ae8eb23a manual
// OFFSET: 0x004C5B40
int32_t SI2::GetListenerPosition(C3Vector* position) {
    if (!position) {
        return 1;
    }

    SE2::GetListenerPosition(position);
    return 0;
}

// VERIFY: 899d19d3 auto
// OFFSET: 0x004C5B80
void SI2::StopAllSounds() {
    SE2::StopAllSounds(0);
}

// VERIFY: 696f4de8 auto
// OFFSET: 0x004C5B90
int32_t SI2::Set3DPosition(SE2SoundKitObject* object, const C3Vector* position) {
    if (object->m_advancedId) {
        SI2::SetAdvancedSoundPosition(object->m_advancedId, position);
    }

    object->m_sound.Set3DPosition(*position);
    return 0;
}

// VERIFY: 8ab4a07a auto
// OFFSET: 0x004C5BC0
int32_t SI2::SetVolume(SE2SoundKitObject* object, float volume) {
    object->m_sound.SetVolume(volume);
    return 0;
}

// VERIFY: ff3bc1d9 auto
// OFFSET: 0x004C5BE0
bool SI2::IsPlaying(SE2SoundKitObject* object) {
    return object->m_sound.IsPlaying();
}

// VERIFY: 2a2ff92d auto
// OFFSET: 0x004C5BF0
uint32_t SI2::IsSoundKitLooping(int32_t soundKitId) {
    VSOUNDKITDEF* def = SI2::GetSoundKitDef(soundKitId);

    if (!def) {
        return 0;
    }

    return def->m_flags & 0x200;
}

// VERIFY: dd9e9845 manual
// OFFSET: 0x004C5C10
int32_t SI2::GetSoundKitUsage(int32_t soundKitId) {
    VSOUNDKITDEF* def = SI2::GetSoundKitDef(soundKitId);

    if (def && def->m_advanced) {
        return def->m_advanced->m_usage;
    }

    return 2;
}

// VERIFY: 9ff69350 manual
// OFFSET: 0x004C5C40
bool SI2::IsLooping(SE2SoundKitObject* object) {
    return object->m_sound.IsLooping();
}

// VERIFY: 3dddcdd1 auto
// OFFSET: 0x004C5C60
int32_t SI2::GetNumSoundsInSoundKit(int32_t soundKitId) {
    VSOUNDKITDEF* def = SI2::GetSoundKitDef(soundKitId);

    if (!def) {
        return 0;
    }

    return def->m_numFiles;
}

// VERIFY: c2c19ed1 auto
// OFFSET: 0x004C5C80
int32_t SI2::AttachToObject(SE2SoundKitObject* object, WOWGUID guid) {
    if (!object->m_sound.Is3D()) {
        return 12;
    }

    object->m_sound.AttachToObject(guid.m_guid);
    return 0;
}

// VERIFY: f1864be0 auto
// OFFSET: 0x004C5CC0
void SI2::StopAllSounds(float fadeOutTime) {
    SE2::StopAllSounds(SI2::s_channelGroupNames[0], fadeOutTime);
}

// VERIFY: 218e92b2 auto
// OFFSET: 0x004C5CE0
int32_t SI2ZoneSoundPair::GetCurrentType() {
    return this->m_sounds[this->m_current].m_type;
}

// VERIFY: 702244e3 manual
// OFFSET: 0x004C5D00
bool SI2ZoneSoundPair::IsPlaying() {
    return this->m_sounds[this->m_current].m_object.m_sound.IsPlaying();
}

// VERIFY: 1c86efa7 auto
// OFFSET: 0x004C5D20
void SI2ZoneSoundPair::SetFinishedCallback(void (*callback)(SESound*, void*), void* param) {
    this->m_sounds[this->m_current].m_object.m_sound.SetFinishedCallback2(callback, param);
}

// VERIFY: 0f619923 auto
// OFFSET: 0x004C5DC0
int32_t SI2::OnFocusChanged(const void* data, void* param) {
    static CVar* s_enableSoundWhenGameIsInBG = CVar::Lookup("Sound_EnableSoundWhenGameIsInBG");

    CVar* enableVoiceChat = CVar::Lookup("EnableVoiceChat");
    const int32_t* focus = static_cast<const int32_t*>(data);

    if (!focus) {
        return 1;
    }

    if (*focus) {
        SE2::MuteAllSound(false);
        SE2::MuteChannelGroup("CINEMATIC", false);

        if (CGGameUI::m_initialized && enableVoiceChat && enableVoiceChat->m_intValue && s_savedVoiceSession) {
            CGChat::SetActiveVoiceSession(s_savedVoiceSession, s_savedVoiceSessionType, -1);
            s_savedVoiceSession = nullptr;
            s_savedVoiceSessionType = 4;
        }
    } else if (s_enableSoundWhenGameIsInBG->m_intValue) {
        SE2::MuteAllSound(false);
        SE2::MuteChannelGroup("CINEMATIC", false);
    } else {
        SE2::MuteAllSound(true);
        SE2::MuteChannelGroup("CINEMATIC", true);

        if (CGGameUI::m_initialized && !s_savedVoiceSession) {
            s_savedVoiceSession = CGChat::GetChannelStringForDisplay(CGChat::GetActiveVoiceSession(), &s_savedVoiceSessionType);
            CGChat::SetActiveVoiceSession(nullptr, 4, -1);
        }
    }

    return 1;
}

// VERIFY: e5b8f14f manual
// OFFSET: 0x004C5F00
static C3Vector RandomListenerOffset() {
    C3Vector listener;
    listener.x = 0.0f;
    listener.y = 0.0f;
    listener.z = 0.0f;

    float scale = CRandom::sfloat_(g_rndSeed);
    C3Vector direction = CRandom::C3Vector_(g_rndSeed);
    float x = direction.x * scale * 50.0f;
    float y = direction.y * scale * 50.0f;
    float z = direction.z * scale * 50.0f;

    SE2::GetListenerPosition(&listener);

    return C3Vector(x + listener.x, y + listener.y, z + listener.z);
}

// VERIFY: ecf8304e manual
// OFFSET: 0x004C5FB0
void SI2::ValidateVoiceChatOutputDriver() {
    char name[256];
    char current[256];

    int32_t count = SE2::GetNumOutputDrivers_Cached(1);
    int32_t missing = 1;

    for (int32_t i = 0; i < count; i++) {
        SE2::GetOutputDriverName_Cached(i, current, sizeof(current), 1);

        if (!strcmp(SE2::GetCurrentChatOutputDeviceName(), current)) {
            missing = 0;
            break;
        }
    }

    static CVar* s_outputDriverIndex = CVar::Lookup("Sound_VoiceChatOutputDriverIndex");

    if (missing || (!s_outputDriverIndex->m_intValue && (SE2::GetOutputDriverName_Cached(1, name, sizeof(name), 1), strcmp(SE2::GetCurrentChatOutputDeviceName(), name)))) {
        s_outputDriverIndex->Set("0", true, false, false, true);
        ComSatClient_SetOutputDevice(0);
    }
}

// VERIFY: ded1a63e manual
// OFFSET: 0x004C60F0
void SI2::ValidateVoiceChatInputDriver() {
    char name[256];
    char current[256];

    int32_t count = SE2::GetNumInputDrivers_Cached(1);
    int32_t missing = 1;

    for (int32_t i = 0; i < count; i++) {
        SE2::GetInputDriverName_Cached(i, current, sizeof(current), 1);

        if (!strcmp(SE2::GetCurrentChatInputDeviceName(), current)) {
            missing = 0;
            break;
        }
    }

    static CVar* s_inputDriverIndex = CVar::Lookup("Sound_VoiceChatInputDriverIndex");

    if (missing || (!s_inputDriverIndex->m_intValue && (SE2::GetInputDriverName_Cached(1, name, sizeof(name), 1), strcmp(SE2::GetCurrentChatInputDeviceName(), name)))) {
        s_inputDriverIndex->Set("0", true, false, false, true);
        ComSatClient_SetCaptureDevice(0);
    }
}

// VERIFY: 26e5585d manual
// OFFSET: 0x004C6230
int32_t SI2::ShutDown(int32_t restart) {
    EventUnregister(EVENT_ID_FOCUS, SI2::OnFocusChanged);
    EventUnregister(EVENT_ID_POLL, SI2::UpdateAdvancedSounds);

    if (!restart) {
        SI2::ClearSuppressorSounds();
    }

    SE2::StopAllSounds(restart);

    if (!restart) {
        SI2::FreeWorldStateZoneSoundOverrides();
        SI2::FreeZoneSoundOverrides();
        SI2::FreeFootstepSounds();
        SI2::FreeEmoteSounds();
    }

    SI2::DSP_ShutDown();
    SE2::UnregisterHeartBeat();
    SI2::ReleaseSoundGroups();

    if (!restart) {
        SI2::FreeSoundKitDefs();
    }

    SE2::ShutDown(restart);

    if (!restart) {
        SI2::DestroyVSoundLookup();
    }

    return 0;
}

// VERIFY: a240cc52 auto
// OFFSET: 0x004C62B0
void SI2::SoundFinished(SESound* sound, void* param) {
    SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(sound->GetUserData());

    if (!userData) {
        return;
    }

    SI2::s_channelGroupNumSounds[userData->m_group]--;

    if (userData->m_advancedId && userData->m_def && userData->m_def->m_advanced && userData->m_def->m_advanced->m_usage == 2) {
        SI2::MarkAdvancedSoundStopped(userData->m_advancedId);
    }

    VSOUNDKITDEF* def = userData->m_def;

    if (def && (def->m_flags & 0x20)) {
        int32_t id = def->m_id;

        for (uint32_t i = 0; i < SI2::s_suppressors.Count(); i++) {
            if (SI2::s_suppressors[i] == id) {
                SI2::s_suppressors[i] = 0;
                return;
            }
        }
    }
}

// VERIFY: 551d00f9 auto
// OFFSET: 0x004C6330
int32_t SI2::ResetSoundKitVariants(const char* name) {
    int32_t soundKitId = SI2::LookupSoundKitId(name);

    if (!soundKitId) {
        return 7;
    }

    VSOUNDKITDEF* def = SI2::GetSoundKitDef(soundKitId);

    if (!def) {
        return 5;
    }

    if (!def->m_numFiles) {
        return 6;
    }

    def->ResetWeights();
    def->m_lastIndex = -1;
    return 0;
}

// VERIFY: 2c0f24c2 auto
// OFFSET: 0x004C6390
int32_t SI2::StopSound(SE2SoundKitObject* object, int32_t immediate, float fadeOutTime, int32_t stopAdvanced) {
    SI2SoundUserData* userData = static_cast<SI2SoundUserData*>(object->m_sound.GetUserData());

    if (userData) {
        VSOUNDKITDEF* def = userData->m_def;

        if (def && (def->m_flags & 0x20)) {
            int32_t id = def->m_id;

            for (uint32_t i = 0; i < SI2::s_suppressors.Count(); i++) {
                if (SI2::s_suppressors[i] == id) {
                    SI2::s_suppressors[i] = 0;
                    break;
                }
            }
        }
    }

    object->m_sound.Stop(immediate, fadeOutTime);

    if (stopAdvanced) {
        SI2::StopAdvancedSound(object->m_advancedId);
    }

    return 0;
}

// VERIFY: 8b75cf26 manual
// OFFSET: 0x004C6410
int32_t SI2ZoneSoundPair::GetPlayingSoundKitId() {
    if (this->m_sounds[this->m_current].m_object.m_sound.IsPlaying()) {
        return this->m_sounds[this->m_current].m_soundKitId;
    }

    return 0;
}

// VERIFY: 71a7805b manual
// OFFSET: 0x004C6450
void SI2ZoneSoundPair::Stop(int32_t immediate) {
    SI2::StopSound(&this->m_sounds[this->m_current].m_object, immediate, -1.0f, 1);
    this->m_sounds[this->m_current].m_type = 13;
    this->m_sounds[this->m_current].m_soundKitId = 0;

    SI2::StopSound(&this->m_sounds[this->m_next].m_object, immediate, -1.0f, 1);
    this->m_sounds[this->m_next].m_type = 13;
    this->m_sounds[this->m_next].m_soundKitId = 0;
}

// VERIFY: 42b96170 auto
// OFFSET: 0x004C6500
SI2SoundUserData::SI2SoundUserData() {
    this->m_name = nullptr;
    this->m_soundKitId = 0;
    this->m_def = nullptr;
    this->m_group = 0;
    this->unused_0014 = 0;
    this->m_advancedId = 0;
}

// VERIFY: d9247093 manual
// OFFSET: 0x004C6520
SI2SoundUserData::~SI2SoundUserData() {
    this->m_name = nullptr;
    this->m_soundKitId = 0;
    this->m_def = nullptr;
    this->m_group = 0;
    this->unused_0014 = 0;
    this->m_advancedId = 0;
}

// VERIFY: 43b346c5 auto
// OFFSET: 0x00532950
void SI2SoundUserData::Release() {
    delete this;
}

// VERIFY: a4a74c7f manual
// OFFSET: 0x004C6560
void SI2::ValidateOutputDriver() {
    char name[256];
    char current[256];

    int32_t count = SE2::GetNumOutputDrivers_Cached(0);
    int32_t missing = 1;

    for (int32_t i = 0; i < count; i++) {
        SE2::GetOutputDriverName_Cached(i, current, sizeof(current), 0);

        if (!strcmp(SE2::GetCurrentGameOutputDeviceName(), current)) {
            missing = 0;
            break;
        }
    }

    static CVar* s_outputDriverIndex = CVar::Lookup("Sound_OutputDriverIndex");

    if (missing || (!s_outputDriverIndex->m_intValue && (SE2::GetOutputDriverName_Cached(1, name, sizeof(name), 0), strcmp(SE2::GetCurrentGameOutputDeviceName(), name)))) {
        s_outputDriverIndex->Set("0", true, false, false, true);
        int32_t restart = count != 0;
        SI2::ShutDown(restart);
        SI2::Init(restart);
    }
}

// VERIFY: bc4053ef auto
// OFFSET: 0x004C6700
void SI2::RegisterDeviceChangeHandler() {
    EventRegister(EVENT_ID_POLL, SI2::HandleSoundDeviceChange);
}

// VERIFY: 5c43095a manual
// OFFSET: 0x004C6A40
int32_t SI2::PlaySoundKit(int32_t soundKitId, const C3Vector* position, SE2SoundKitObject* object, SE2SoundKitProperties* properties, uint32_t advancedId, uint32_t* soundId, int32_t checkDistance, int32_t keepPlaying) {
    if (soundKitId <= 0) {
        return 5;
    }

    VSOUNDKITDEF* def = SI2::GetSoundKitDef(soundKitId);

    if (!def) {
        return 5;
    }

    if (!def->m_numFiles) {
        return 6;
    }

    if (!SE2::IsInitialized()) {
        return 17;
    }

    static CVar* s_enableAllSound = CVar::Lookup("Sound_EnableAllSound");

    if (!s_enableAllSound || !s_enableAllSound->m_intValue || CGGameUI::m_inMovie) {
        return 17;
    }

    SE2SoundKitProperties defaults;
    defaults.ResetToDefaults();

    if (!properties) {
        properties = &defaults;
    }

    if (SI2::s_channelGroupNumSounds[properties->m_group] >= SI2::s_channelGroupMaxSounds[properties->m_group]) {
        return 14;
    }

    if (!strcmp(SI2::s_channelGroupNames[properties->m_group], "SFX")) {
        static CVar* s_enableSFX = CVar::Lookup("Sound_EnableSFX");

        if (!s_enableSFX || !s_enableSFX->m_intValue) {
            return 9;
        }
    }

    if (!strcmp(SI2::s_channelGroupNames[properties->m_group], "MUSIC") || !strcmp(SI2::s_channelGroupNames[properties->m_group], "SCRIPTMUSIC")) {
        static CVar* s_enableMusic = CVar::Lookup("Sound_EnableMusic");

        if (!s_enableMusic || !s_enableMusic->m_intValue) {
            return 10;
        }
    }

    if (properties->m_group == 2) {
        static CVar* s_enableAmbience = CVar::Lookup("Sound_EnableAmbience");

        if (!s_enableAmbience || !s_enableAmbience->m_intValue) {
            return 11;
        }
    }

    if (properties->m_group >= 18) {
        return 8;
    }

    if (CGChat::IsPaused() && !position) {
        return 9;
    }

    if (properties->m_suppressorMode == 2) {
        def->m_flags |= 0x20;
    }

    if (properties->m_suppressorMode == 1) {
        def->m_flags &= ~0x20u;
    }

    if (def->m_flags & 0x20) {
        for (uint32_t i = 0; i < SI2::s_suppressors.Count(); i++) {
            if (SI2::s_suppressors[i] == static_cast<uint32_t>(soundKitId)) {
                return 15;
            }
        }
    }

    SESound localSound;
    SESound* sound;

    if (object) {
        sound = &object->m_sound;

        if (!keepPlaying) {
            SI2::StopSound(object, 1, -1.0f, 1);
        }
    } else {
        sound = &localSound;
        properties->m_loopMode = 2;
    }

    const char* file = def->SelectFile(properties->m_selectMode, nullptr, properties->m_fileIndex, GetDownloadPriority(properties->m_group));

    if (!file) {
        return 16;
    }

    if (properties->m_checkStreaming) {
        int32_t local = SFile::FileIsLocal(file, GetDownloadPriority(properties->m_group));

        if (local != 1) {
            if (properties->m_group == 6) {
                if (!SFile::MakeFileLocal(file, 2, true)) {
                    return 16;
                }
            } else if (local == 0 || local == 2) {
                if (local == 0) {
                    SFile::MakeFileLocal(file, GetDownloadPriority(properties->m_group), false);
                }

                def->m_downloadPending = 1;
                return 20;
            }
        }
    }

    def->m_downloadPending = 0;

    if (properties->m_blockMusicFiles && (SStrStrI(file, "music") || SStrStrI(file, "ambience") || SStrStrI(file, "cinematic"))) {
        return 16;
    }

    int32_t looping = 0;

    if (properties->m_loopMode == 0) {
        looping = def->m_flags & 0x200;
    } else if (properties->m_loopMode == 1) {
        looping = 1;
    }

    if (properties->m_forceLoop) {
        looping = 1;
        properties->m_maxAudibleBehavior = 0;
        properties->m_streaming = true;
        properties->m_software = 1;
    }

    if (looping && !object) {
        DebugLog("XXXX\nXXXX ERROR!! Sound Kit ID %d is a looping sound but there is no user handle! This sound will play forever!! \nXXXX\n", soundKitId);
    }

    SE2::SetSoundGroupMaxAudibleBehavior(def->m_loadSoundGroup, 1, 0.0f);
    SE2::SetSoundGroupMaxAudibleBehavior(def->m_soundGroup, properties->m_maxAudibleBehavior, properties->m_muteFadeSpeed);

    if (def->m_advanced && !advancedId) {
        SI2::AddAdvancedSound(def->m_advanced, soundKitId, position, properties->m_coneOrientation, object, 0, 0, checkDistance, properties->m_loopMode == 1);
        return 0;
    }

    bool randomStart = properties->m_group != 1 && properties->m_group != 5 && !properties->m_noRandomStart;

    if (position) {
        if (!sound->Create3D(file, looping, def->m_loadSoundGroup, def->m_soundGroup, randomStart, properties->m_streaming, properties->m_priority, properties->m_software, properties->m_coneInsideAngle, properties->m_coneOutsideAngle, properties->m_coneOutsideVolume, properties->m_coneOrientation)) {
            return 16;
        }

        sound->Set3DPosition(*position);
        sound->Set3DMinMaxDistance(def->m_minDistance, properties->m_maxDistance > -1.0f ? properties->m_maxDistance : def->m_distanceCutoff);
    } else if (!sound->Create2D(file, looping, def->m_loadSoundGroup, def->m_soundGroup, randomStart, properties->m_streaming, properties->m_reverb, properties->m_priority, properties->m_decodeBufferSize)) {
        return 16;
    }

    sound->SetChannelGroup(SI2::s_channelGroupNames[properties->m_group], properties->m_group != 3);

    SI2SoundUserData* userData = new (SMemAlloc(sizeof(SI2SoundUserData), __FILE__, __LINE__, 0)) SI2SoundUserData();
    userData->m_name = def->m_name ? def->m_name : "<UNKNOWN ERROR>";
    userData->m_soundKitId = soundKitId;
    userData->m_def = def;
    userData->m_advancedId = advancedId;
    userData->m_group = properties->m_group;
    sound->SetUserData(userData);

    float volume;

    if (properties->m_volume < 0.0f || properties->m_volume > 1.0f) {
        volume = def->m_volume * properties->m_volumeScale;

        if (def->m_flags & 0x800) {
            CRange range;
            range.m_l = -0.15f;
            range.m_h = 0.15f;
            volume = range.Random(g_rndSeed) + volume;
        }
    } else {
        volume = properties->m_volume;
    }

    sound->SetVolume(volume);

    if (def->m_flags & 0x400) {
        CRange range;
        range.m_l = 0.85f;
        range.m_h = 1.15f;
        sound->SetPitch(range.Random(g_rndSeed));
    }

    if (properties->m_fadeInTime >= 0.0f) {
        sound->SetFadeInTime(properties->m_fadeInTime);
    }

    if (properties->m_fadeOutTime >= 0.0f) {
        sound->SetFadeOutTime(properties->m_fadeOutTime);
    }

    SI2::s_channelGroupNumSounds[properties->m_group]++;
    sound->SetFinishedCallback(SI2::SoundFinished, nullptr);

    if (def->m_flags & 0x20) {
        uint32_t i;

        for (i = 0; i < SI2::s_suppressors.Count(); i++) {
            if (!SI2::s_suppressors[i]) {
                SI2::s_suppressors[i] = soundKitId;
                break;
            }
        }

        if (i >= SI2::s_suppressors.Count()) {
            *SI2::s_suppressors.New() = soundKitId;
        }
    }

    if (properties->m_useDspChain) {
        sound->SetAttachCustomDSPChainCallback(SI2::DSP_AttachCustomDspChain, properties->m_dspChainName);
    }

    sound->CompleteNonBlockingLoad();

    if (soundId) {
        *soundId = sound->GetID();
    }

    sound->Play();
    return 0;
}

// VERIFY: 4ec0a358 manual
// OFFSET: 0x004C7140
int32_t SI2ZoneSoundPair::Transition(int32_t soundKitId, int32_t type, SE2SoundKitProperties* properties, int32_t immediate, float fadeOutTime) {
    if (type >= 11) {
        SI2::StopSound(&this->m_sounds[this->m_current].m_object, 0, -1.0f, 1);
        this->m_sounds[this->m_current].m_type = type;
        this->m_sounds[this->m_current].m_soundKitId = 0;
        return 18;
    }

    if (immediate) {
        SI2::StopSound(&this->m_sounds[this->m_current].m_object, 1, -1.0f, 1);
        SI2::StopSound(&this->m_sounds[this->m_next].m_object, 1, -1.0f, 1);

        int32_t result = SI2::PlaySoundKit(soundKitId, nullptr, &this->m_sounds[this->m_current].m_object, properties, 0, nullptr, 1, 0);

        if (!result) {
            this->m_sounds[this->m_current].m_soundKitId = soundKitId;
            this->m_sounds[this->m_current].m_type = type;
            return 0;
        }

        return result;
    }

    if (soundKitId == this->m_sounds[this->m_current].m_soundKitId && this->m_sounds[this->m_current].m_object.m_sound.IsPlaying()) {
        return 15;
    }

    if (soundKitId == this->m_sounds[this->m_next].m_soundKitId && this->m_sounds[this->m_next].m_object.m_sound.IsPlaying()) {
        if (fadeOutTime >= 0.0f) {
            this->m_sounds[this->m_current].m_object.m_sound.SetFadeOutTime(fadeOutTime);
        }

        this->m_sounds[this->m_current].m_object.m_sound.FadeOutNow();

        if (properties) {
            this->m_sounds[this->m_next].m_object.m_sound.SetFadeInTime(properties->m_fadeInTime);
        }

        this->m_sounds[this->m_next].m_object.m_sound.FadeInNow();

        uint32_t current = this->m_current;
        this->m_next = current;
        this->m_current = 1 - current;
        return 21;
    }

    SI2::StopSound(&this->m_sounds[this->m_next].m_object, 0, -1.0f, 1);

    if (fadeOutTime >= 0.0f) {
        this->m_sounds[this->m_current].m_object.m_sound.SetFadeOutTime(fadeOutTime);
    }

    this->m_sounds[this->m_current].m_object.m_sound.FadeOutNow();

    int32_t result = SI2::PlaySoundKit(soundKitId, nullptr, &this->m_sounds[this->m_next].m_object, properties, 0, nullptr, 1, 0);

    if (!result) {
        this->m_sounds[this->m_next].m_soundKitId = soundKitId;
        this->m_sounds[this->m_next].m_type = type;

        uint32_t current = this->m_current;
        this->m_next = current;
        this->m_current = 1 - current;
        return 0;
    }

    return result;
}

// VERIFY: 212be192 auto
// OFFSET: 0x004C7410
SE2SoundKitObject::SE2SoundKitObject() {
    this->m_advancedId = 0;
    this->unused_0010 = 0;
}

// VERIFY: 19a1e756 auto
// OFFSET: 0x004C7440
SE2SoundKitObject::~SE2SoundKitObject() {
    if (this->m_advancedId) {
        SI2::StopAdvancedSound(this->m_advancedId);
    }
}

// VERIFY: cf497004 auto
// OFFSET: 0x004C74A0
int32_t SI2::PlaySoundKit(const char* name, const C3Vector* position, SE2SoundKitObject* object, SE2SoundKitProperties* properties) {
    if (!SE2::IsInitialized()) {
        return 17;
    }

    int32_t soundKitId = SI2::LookupSoundKitId(name);

    if (!soundKitId) {
        return 7;
    }

    return SI2::PlaySoundKit(soundKitId, position, object, properties, 0, nullptr, 1, 0);
}

// VERIFY: 1522defd manual
// OFFSET: 0x004C74F0
int32_t SI2::ChaosModeUpdate(const void* data, void* param) {
    static CVar* s_chaosMode = CVar::Lookup("Sound_ChaosMode");

    int32_t mode = 0;

    if (s_chaosMode) {
        mode = s_chaosMode->m_intValue;
    }

    static uint32_t s_maxSoundKitId = g_soundEntriesDB.m_maxID;

    C3Vector position;
    position.x = 0.0f;
    position.y = 0.0f;
    position.z = 0.0f;

    uint32_t soundKitId = static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32);

    static SE2SoundKitObject s_object2D;
    static SE2SoundKitObject s_object3D;
    static SE2SoundKitObject s_loopObjects[80];
    static uint32_t s_lastTime = OsGetAsyncTimeMs();

    uint32_t now = OsGetAsyncTimeMs();
    s_chaosElapsedMs += now - s_lastTime;
    float elapsed = static_cast<int32_t>(now - s_lastTime) * 0.001f;

    SE2SoundKitProperties properties;
    properties.ResetToDefaults();
    properties.m_volumeScale = 0.3f;
    properties.m_loopMode = 2;
    properties.m_streaming = false;
    properties.m_blockMusicFiles = 1;

    SE2SoundKitProperties loopProperties;
    loopProperties.ResetToDefaults();
    loopProperties.m_volumeScale = 0.3f;
    loopProperties.m_loopMode = 1;
    loopProperties.m_streaming = false;
    loopProperties.m_blockMusicFiles = 1;

    switch (mode) {
        case 2:
            if (s_chaosElapsedMs < 100) {
                break;
            }

            properties.m_streaming = true;
            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);

        case 1:
            if (s_chaosElapsedMs >= 100) {
                properties.m_streaming = false;
                position = RandomListenerOffset();
                SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);
                s_chaosElapsedMs = 0;
            }

            break;

        case 13:
            if (s_chaosElapsedMs < 100) {
                break;
            }

            properties.m_streaming = true;
            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);
            properties.m_streaming = false;
            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);

        case 12:
            if (s_chaosElapsedMs < 100) {
                break;
            }

            properties.m_streaming = true;
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), nullptr, nullptr, &properties, 0, nullptr, 1, 0);

        case 11:
            if (s_chaosElapsedMs >= 100) {
                properties.m_streaming = false;
                SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), nullptr, nullptr, &properties, 0, nullptr, 1, 0);
                s_chaosElapsedMs = 0;
            }

            break;

        case 4:
            SI2::PlaySoundKit(soundKitId, nullptr, &s_object2D, &properties, 0, nullptr, 1, 0);
            SI2::StopSound(&s_object2D, 1, -1.0f, 1);
            SI2::PlaySoundKit(soundKitId, nullptr, &s_object3D, &properties, 0, nullptr, 1, 0);
            SI2::StopSound(&s_object3D, 1, -1.0f, 1);

        case 3:
            SI2::PlaySoundKit(soundKitId, nullptr, &s_object2D, &properties, 0, nullptr, 1, 0);
            SI2::StopSound(&s_object2D, 1, -1.0f, 1);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(soundKitId, &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            SI2::StopSound(&s_object3D, 1, -1.0f, 1);
            break;

        case 5:
            s_chaosTimer += elapsed;

            if (s_chaosTimer <= 1.0f) {
                break;
            }

            for (int32_t i = 80; i; i--) {
                SI2::PlaySoundKit(soundKitId, nullptr, nullptr, &properties, 0, nullptr, 1, 0);
            }

            s_chaosTimer = 0.0f;
            break;

        case 6:
            s_chaosTimer += elapsed;

            if (s_chaosTimer <= 1.0f) {
                break;
            }

            for (int32_t i = 80; i; i--) {
                position = RandomListenerOffset();
                SI2::PlaySoundKit(soundKitId, &position, nullptr, &properties, 0, nullptr, 1, 0);
            }

            s_chaosTimer = 0.0f;
            break;

        case 7:
            s_chaosTimer += elapsed;

            if (s_chaosTimer <= 1.0f) {
                break;
            }

            for (SE2SoundKitObject* object = s_loopObjects; object < &s_loopObjects[80]; object++) {
                if (!object->m_sound.IsPlaying()) {
                    position = RandomListenerOffset();
                    SI2::PlaySoundKit(soundKitId, &position, object, &loopProperties, 0, nullptr, 1, 0);
                }
            }

            s_chaosTimer = 0.0f;
            break;

        case 8:
            s_chaosTimer += elapsed;

            if (s_chaosTimer <= 0.05f) {
                break;
            }

            printf("----------playing...\n");
            SI2::PlaySoundFile("Data\\Alert.mp3", 4);
            s_chaosTimer = 0.0f;
            break;

        case 9:
            s_chaosRestartTimer += elapsed;

            if (s_chaosRestartTimer <= 10.0f) {
                break;
            }

            DebugLog("##RESTART SOUND ENGINE!!\n");
            SI2::ShutDown(1);
            SI2::Init(1);
            s_chaosRestartTimer = 0.0f;

            {
                static CVar* s_chaosMode9 = CVar::Lookup("Sound_ChaosMode");
                s_chaosMode9->Set("9", true, false, false, true);
            }

            break;

        case 10:
            s_chaosRestartTimer += elapsed;

            if (s_chaosElapsedMs < 100) {
                break;
            }

            s_chaosElapsedMs = 0;
            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);

            if (s_chaosRestartTimer <= 10.0f) {
                break;
            }

            DebugLog("##RESTART SOUND ENGINE!!\n");
            SI2::ShutDown(1);
            SI2::Init(1);
            s_chaosRestartTimer = 0.0f;

            {
                static CVar* s_chaosMode10 = CVar::Lookup("Sound_ChaosMode");
                s_chaosMode10->Set("10", true, false, false, true);
            }

            break;

        case 14: {
            s_chaosRestartTimer += elapsed;

            if (s_chaosElapsedMs < 100) {
                break;
            }

            s_chaosElapsedMs = 0;
            uint32_t sameId = static_cast<uint32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32);

            for (int32_t i = 20; i; i--) {
                position = RandomListenerOffset();
                SI2::PlaySoundKit(sameId, &position, nullptr, &properties, 0, nullptr, 1, 0);
            }

            position = RandomListenerOffset();
            SI2::PlaySoundKit(static_cast<int32_t>((static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * s_maxSoundKitId) >> 32), &position, nullptr, &properties, 0, nullptr, 1, 0);

            if (s_chaosRestartTimer <= 10.0f) {
                break;
            }

            DebugLog("##RESTART SOUND ENGINE!!\n");
            SI2::ShutDown(1);
            SI2::Init(1);
            s_chaosRestartTimer = 0.0f;

            static CVar* s_chaosMode14 = CVar::Lookup("Sound_ChaosMode");
            s_chaosMode14->Set("14", true, false, false, true);
            break;
        }

        case 15: {
            int32_t ids[3] = { 13061, 15969, 12173 };

            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);
            position = RandomListenerOffset();
            SI2::PlaySoundKit(ids[(static_cast<uint64_t>(CRandom::uint32_(g_rndSeed)) * 3) >> 32], &position, &s_object3D, &properties, 0, nullptr, 1, 0);

            break;
        }

        default:
            break;
    }

    s_lastTime = now;
    return 1;
}

// VERIFY: 3e5c36df auto
// OFFSET: 0x004C82E0
int32_t SI2::Init(int32_t restart) {
    if (CmdLineGetBool(static_cast<CMDOPT>(26))) {
        SI2::RegisterSoundEngineCVars();
        SI2::RegisterCVars();
        return 17;
    }

    if (!restart) {
        SI2::CreateVSoundLookup();
        SI2::InitFootstepSounds();
        SI2::InitEmoteSounds();
        SI2::InitWeaponImpactCombatSounds();
        SI2::InitWeaponSwingCombatSounds();
        SI2::RegisterSoundEngineCVars();
    }

    int32_t enableReverb = 0;
    int32_t enableSoftwareHRTF = 0;
    int32_t numChannels = 4;

    static CVar* s_enableReverb = CVar::Lookup("Sound_EnableReverb");

    if (s_enableReverb) {
        enableReverb = s_enableReverb->m_intValue;
    }

    static CVar* s_enableSoftwareHRTF = CVar::Lookup("Sound_EnableSoftwareHRTF");

    if (s_enableSoftwareHRTF) {
        enableSoftwareHRTF = s_enableSoftwareHRTF->m_intValue;
    }

    static CVar* s_numChannels = CVar::Lookup("Sound_NumChannels");

    if (s_numChannels) {
        numChannels = s_numChannels->m_intValue;
    }

    int32_t outputDriverIndex = 0;
    const char* outputDriverName = "";

    static CVar* s_outputDriverIndex = CVar::Lookup("Sound_OutputDriverIndex");

    if (s_outputDriverIndex) {
        outputDriverIndex = s_outputDriverIndex->m_intValue;
    }

    static CVar* s_outputDriverName = CVar::Lookup("Sound_OutputDriverName");

    if (s_outputDriverName) {
        outputDriverName = s_outputDriverName->m_stringValue.m_str;
    }

    SE2::Init(512, SI2::GetObjectPosition, enableReverb, enableSoftwareHRTF, &numChannels, &outputDriverIndex, outputDriverName, SI2::GetSoundVolume, restart);
    SI2::ClearSuppressors();

    if (s_outputDriverIndex) {
        char value[32];
        SStrPrintf(value, sizeof(value), "%d", outputDriverIndex);
        s_outputDriverIndex->Set(value, true, false, false, true);
    }

    static CVar* s_masterVolume = CVar::Lookup("Sound_MasterVolume");

    if (s_masterVolume) {
        SE2::SetMasterVolume(s_masterVolume->m_floatValue);
    }

    EventRegister(EVENT_ID_POLL, SI2::UpdateAdvancedSounds);
    EventRegister(EVENT_ID_FOCUS, SI2::OnFocusChanged);
    SI2::RegisterCVars();

    if (!restart) {
        SI2::InitSoundKitLookup();
        SI2::InitWorldStateZoneSoundOverrides();
        SI2::InitZoneSoundOverrides();
    }

    SI2::CreateSoundGroups();

    if (CmdLineGetBool(static_cast<CMDOPT>(27))) {
        EventRegister(EVENT_ID_POLL, SI2::ChaosModeUpdate);
    }

    SE2::SetDeviceChangeCallback(SI2::RegisterDeviceChangeHandler);

    static CVar* s_enableDSPEffects = CVar::Lookup("Sound_EnableDSPEffects");

    if (s_enableDSPEffects->m_intValue) {
        SI2::DSP_Init();
    }

    return 0;
}

// VERIFY: db4e7763 manual
// OFFSET: 0x004CFF30
void SI2::PlaySpellFizzleSound(uint32_t spellId, const void* object) {
    if (!object) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    for (int32_t i = 0; i < 7; i++) {
        if (!((1 << i) & spell.m_schoolMask)) {
            continue;
        }

        ResistancesRec* resistance = g_resistancesDB.GetRecord(i);

        if (resistance) {
            C3Vector position = static_cast<const CGObject_C*>(object)->GetPosition();
            SI2::PlaySoundKit(resistance->m_fizzleSoundID, &position, nullptr, nullptr, 0, nullptr, 1, 0);
            return;
        }
    }
}

// VERIFY: d94cc51b manual
// OFFSET: 0x004C6710
void SI2::ForceUpdateZoneSounds() {
    CGObject_C* unit = ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__);

    if (!unit) {
        unit = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__);

        if (!unit) {
            return;
        }
    }

    CMapEntity* entity = unit->m_worldObject;
    uint32_t areaId = 0;

    if (!CMap::QueryAreaId(entity, areaId)) {
        return;
    }

    const AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

    if (!area) {
        return;
    }

    const AreaTableRec* zone;

    if (area->m_parentAreaID) {
        zone = g_areaTableDB.GetRecord(area->m_parentAreaID);
    } else {
        zone = area;
        area = nullptr;
    }

    WMOAreaTableRec* groupRec = nullptr;
    WMOAreaTableRec* rootRec = nullptr;
    int32_t wmoGroupId = 0;
    int32_t inside = World::QueryObjectInside(entity);
    World::QueryMapObjAreaTable(entity, &groupRec, &rootRec, &wmoGroupId);
    AreaListUpdateSounds(zone, area, rootRec, groupRec, inside);
}


// VERIFY: 34fea195 manual
// OFFSET: 0x004C5D60
bool SI2::GetObjectPosition(uint64_t guid, C3Vector* position) {
    CGObject_C* object = ClntObjMgrObjectPtr(WOWGUID{ guid }, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        *position = object->GetPosition();
    }

    return object != nullptr;
}

// VERIFY: 94fd8429 manual
// OFFSET: 0x004C6810
int32_t SI2::GetChunkLocation(int32_t* mapId, int32_t* chunkX, int32_t* chunkY, int32_t* subchunkX, int32_t* subchunkY) {
    *mapId = World::QueryMapId();
    CGObject_C* player = ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__);

    if (!player) {
        return 0;
    }

    C3Vector position = player->GetPosition();
    float x = -(position.x - 17066.666f);
    float y = -(position.y - 17066.666f) * OO_COORD_TO_CHUNK;
    *subchunkX = CMath::fint_(y - s_subchunkRound);
    x *= OO_COORD_TO_CHUNK;
    *subchunkY = CMath::fint_(x - s_subchunkRound);
    *chunkX = (*subchunkX >> 4) & 0x3F;
    *chunkY = (*subchunkY >> 4) & 0x3F;
    *subchunkX &= 0xF;
    *subchunkY &= 0xF;
    return 1;
}

// VERIFY: a83589e1 manual
// OFFSET: 0x004CDA20
void SI2::DK_GetPlayerFilterName(CGUnit_C* unit, char* name, uint32_t size) {
    uint8_t race = unit->m_unit->m_unitFieldBytes0[0];
    SStrPrintf(name, size, "Death Knight %s %s", s_raceFileNames[race], unit->m_unit->m_unitFieldBytes0[2] ? "Female" : "Male");
}

// VERIFY: 8858e33e auto
// OFFSET: 0x004C66B0
int32_t SI2::HandleSoundDeviceChange(const void* data, void* param) {
    EventUnregister(EVENT_ID_POLL, SI2::HandleSoundDeviceChange);
    CSimpleMovieFrame::AudioDeviceChanged_ShutDown();
    SI2::ValidateOutputDriver();
    SI2::ValidateVoiceChatOutputDriver();
    SI2::ValidateVoiceChatInputDriver();

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_SOUND_DEVICE_UPDATE, nullptr);
    }

    CSimpleMovieFrame::AudioDeviceChanged_Restart();
    return 1;
}
