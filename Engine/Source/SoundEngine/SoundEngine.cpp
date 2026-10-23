#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/SoundEngine/SoundCache.h"
#include "Engine/Source/SoundEngine/fmodex/src/fmod_errors.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "GameVersion.h"

// STATIC: 0x00D4383C
int32_t SE2::sm_bCaptureInitialized;

// STATIC: 0x00D43460
static FMOD_CREATESOUNDEXINFO s_captureExInfo;

// STATIC: 0x00D434CC
static HASHKEY_NONE s_lookupKey;

// STATIC: 0x00D434D0
static char s_chatInputDeviceName[256];

// STATIC: 0x00D435D0
static char s_chatOutputDeviceName[256];

// STATIC: 0x00D436D0
static char s_gameOutputDeviceName[256];

// STATIC: 0x00D437D0
static HSLOG s_log;

// STATIC: 0x00D437D4
static uint32_t s_logFlags;

// STATIC: 0x00D437D8
static int32_t s_cacheBytes;

// STATIC: 0x00D437DC
static int32_t s_cacheBytesPeak;

// STATIC: 0x00D437E0
static int32_t s_cacheCount;

// STATIC: 0x00D437E4
static int32_t s_cacheCountPeak;

// STATIC: 0x00D437E8
static uint32_t s_unused_unknownStat;

// STATIC: 0x00D437EC
static uint32_t s_cacheHitsLoaded;

// STATIC: 0x00D437F0
static uint32_t s_cacheHitsLoading;

// STATIC: 0x00D437F4
static uint32_t s_soundsCreated;

// STATIC: 0x00D437F8
static uint32_t s_streamsCreated;

// STATIC: 0x00D43800
static FMOD::System* s_system;

// STATIC: 0x00D43804
static FMOD::System* s_enumSystem;

// STATIC: 0x00D43808
static FMOD::System* s_chatSystem;

// STATIC: 0x00D4380C
static bool (*s_positionCallback)(uint64_t, C3Vector*);

// STATIC: 0x00D43810
static float (*s_volumeCallback)(SESoundUserData*);

// STATIC: 0x00D43814
static int32_t s_initialized;

// STATIC: 0x00D43818
static uint32_t s_captureLength;

// STATIC: 0x00D4381C
static uint32_t s_captureChannels;

// STATIC: 0x00D43820
static uint32_t s_captureLastPosition;

// STATIC: 0x00D43824
static int32_t s_capturing;

// STATIC: 0x00D43828
static int32_t s_reverbEnabled;

// STATIC: 0x00D4382C
static FMOD::Sound* s_captureSound;

// STATIC: 0x00D43830
static SECaptureCallbacks* s_primaryCaptureCallbacks;

// STATIC: 0x00D43834
static SECaptureCallbacks* s_alternateCaptureCallbacks;

// STATIC: 0x00D43838
static SECaptureCallbacks* s_captureCallbacks;

// STATIC: 0x00D43840
static FMOD::DSP* s_monitorDSP;

// STATIC: 0x00D43844
static void (*s_userMonitorDSPCallback)(float*, uint32_t);

// STATIC: 0x00D43848
static void (*s_deviceChangeCallback)();

// STATIC: 0x00D4384C
static TSGrowableArray<SEDriverInfo> s_outputDrivers;

// STATIC: 0x00D4385C
static TSGrowableArray<SEDriverInfo> s_chatInputDrivers;

// STATIC: 0x00D4386C
static TSGrowableArray<SEDriverInfo> s_chatOutputDrivers;

// STATIC: 0x00D4387C
static SCritSect s_soundListCritSect;

// STATIC: 0x00D438A4
static SCritSect s_createCritSect;

// STATIC: 0x00D438CC
static TSHashTable<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE> s_soundLookup;

// STATIC: 0x00D438F4
static TSGrowableArray<SEChannelGroup> s_channelGroups;

// STATIC: 0x00D43904
static SCritSect s_heartBeatCritSect;

// STATIC: 0x00D4392C
static int32_t s_errorCount;

// STATIC: 0x00D4394C
static float s_speakerMixLFE;

// STATIC: 0x00B1D690
static uint32_t s_nextSoundID = 1;

// STATIC: 0x00B1D694
static float s_invalid = std::numeric_limits<float>::quiet_NaN();

// STATIC: 0x00B1D698
static TSList<SEWaveDataNode, TSGetLink<SEWaveDataNode> > s_waveDataList;

// STATIC: 0x00B1D6A4
static TSList<SESoundInternal, TSGetLink<SESoundInternal> > s_soundList;

// STATIC: 0x00B1D6B0
static TSExplicitList<SESoundInternal, offsetof(SESoundInternal, m_deleteLink)> s_deleteList;

// STATIC: 0x00B1D6BC
static TSExplicitList<SESoundInternal, offsetof(SESoundInternal, m_objectLink)> s_objectList;

// STATIC: 0x00B1D6C8
static TSExplicitList<SESoundInternal, offsetof(SEFileSound, m_goLink)> s_goList;

// STATIC: 0x00B1D798
static float s_speakerMixFront = 0.6f;

// STATIC: 0x00B1D79C
static float s_speakerMixCenter = 0.001f;

// STATIC: 0x00B1D7A0
static float s_speakerMixBack = 0.2f;

// STATIC: 0x00B1D7A4
static float s_speakerMixSide = 0.001f;

// VERIFY: c1b1c1a2 auto
// OFFSET: 0x00877E10
void SE2::Log_Write(uint32_t line, const char* file, FMOD_RESULT result, const char* format, ...) {
    char buffer[512] = "";
    char text[512];

    if (s_errorCount < 200) {
        if (SStrLen(format)) {
            va_list args;
            va_start(args, format);
            SStrVPrintf(buffer, sizeof(buffer), format, args);
            va_end(args);
        }

        if (result != FMOD_OK) {
            SStrPrintf(text, sizeof(text), " -######## FMOD ERROR! (err %d) %s", result, FMOD_ErrorString(result));

            if (result != FMOD_ERR_FILE_NOTFOUND) {
                SLogWrite(s_log, text);
            }

            DebugLog("%s\n", text);

            if (SStrLen(format)) {
                if (result != FMOD_ERR_FILE_NOTFOUND) {
                    SLogWrite(s_log, buffer);
                }

                DebugLog("%s\n", buffer);
            }

            SStrPrintf(text, sizeof(text), "%s(%d)", file, line);

            if (result != FMOD_ERR_FILE_NOTFOUND) {
                SLogWrite(s_log, text);
            }

            DebugLog("%s\n", text);

            if (result != FMOD_ERR_FILE_NOTFOUND) {
                SLogFlush(s_log);
                s_errorCount++;
            }
        } else {
            SLogWrite(s_log, buffer);
            DebugLog("%s\n", buffer);
            SLogFlush(s_log);
        }
    }

    if (s_errorCount == 200) {
        SStrPrintf(text, sizeof(text), " -######## TOO MANY ERRORS. NO FURTHER ERRORS WILL BE LOGGED.");
        SLogWrite(s_log, text);
        DebugLog("%s\n", text);
        SLogFlush(s_log);
        s_errorCount++;
    }
}

// VERIFY: c34ee9ff manual
// OFFSET: 0x00878010
void SE2::Log_Write(const char* format, ...) {
    char buffer[512] = "";

    if (SStrLen(format)) {
        va_list args;
        va_start(args, format);
        SStrVPrintf(buffer, sizeof(buffer), format, args);
        va_end(args);
    }

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, buffer);
}

// VERIFY: 989f34e0 auto
// OFFSET: 0x00878080
void SE2::ErrCheck(uint32_t line, const char* file, FMOD_RESULT result) {
    if (result != FMOD_OK && result != FMOD_ERR_CHANNEL_STOLEN && result != FMOD_ERR_INVALID_HANDLE && result != FMOD_ERR_OUTPUT_DRIVERCALL) {
        SE2::Log_Write(line, file, result, "");
    }
}

// VERIFY: 8f001a74 auto
// OFFSET: 0x008780B0
static void* F_CALLBACK FSoundAllocCallback(unsigned int size, FMOD_MEMORY_TYPE type) {
    return SMemAlloc(size, "FMod", 0, SMEM_FLAG_ZEROMEMORY);
}

// VERIFY: 49d4c190 auto
// OFFSET: 0x008780D0
static void* F_CALLBACK FSoundReallocCallback(void* ptr, unsigned int size, FMOD_MEMORY_TYPE type) {
    return SMemReAlloc(ptr, size, "FMod", 0, 0);
}

// VERIFY: b13ef644 auto
// OFFSET: 0x008780F0
static void F_CALLBACK FSoundFreeCallback(void* ptr, FMOD_MEMORY_TYPE type) {
    SMemFree(ptr, "FMod", 0, 0);
}

// VERIFY: 8b89c458 manual
// OFFSET: 0x00887370
SEStreamCallbacks::~SEStreamCallbacks() {
}

// VERIFY: d276738b auto
// OFFSET: 0x005EEB70
void SEStreamCallbacks::Delete() {
}

// VERIFY: 762ff4ed manual
// OFFSET: 0x00878110
void SEStreamCallbacks::DiscardSamples(uint32_t position, uint32_t count) {
    float buffer[1024];

    while (count > 0) {
        uint32_t samples = count;

        if (samples >= 1024) {
            samples = 1024;
        }

        this->GetSamples(position, samples, buffer);
        position += samples;
        count -= samples;
    }
}

// VERIFY: acfed1e3 auto
// OFFSET: 0x0087B300
static FMOD_RESULT F_CALLBACK SEOpenCallback(const char* name, int unicode, unsigned int* filesize, void** handle, void** userdata) {
    uint32_t id = atol(name);

    s_soundListCritSect.Enter();

    SOUND_INTERNAL_LOOKUP* lookup = s_soundLookup.Ptr(id, s_lookupKey);

    if (!lookup) {
        s_soundListCritSect.Leave();
        return FMOD_ERR_FILE_NOTFOUND;
    }

    SEFileSound* sound = static_cast<SEFileSound*>(lookup->m_sound);
    *filesize = SFileGetFileSize(sound->m_file, nullptr);
    *handle = sound->m_file;
    *userdata = nullptr;
    sound->m_file = nullptr;

    s_soundListCritSect.Leave();

    return FMOD_OK;
}

// VERIFY: 50552f3b auto
// OFFSET: 0x00878170
static FMOD_RESULT F_CALLBACK SECloseCallback(void* handle, void* userdata) {
    return SFileCloseFile(static_cast<SFileImpl*>(handle)) ? FMOD_OK : FMOD_ERR_INVALID_HANDLE;
}

// VERIFY: d7ddbcdc auto
// OFFSET: 0x00878190
static FMOD_RESULT F_CALLBACK SEReadCallback(void* handle, void* buffer, unsigned int sizebytes, unsigned int* bytesread, void* userdata) {
    uint32_t read = 0;

    SFile::SetReadFatal(false);
    int32_t success = SFileReadFile(static_cast<SFileImpl*>(handle), buffer, sizebytes, &read, nullptr, 0);
    SFile::SetReadFatal(true);

    *bytesread = read;

    if (success) {
        return FMOD_OK;
    }

    return SErrGetLastError() == 38 ? FMOD_ERR_FILE_EOF : FMOD_ERR_INTERNAL;
}

// VERIFY: e534e8af auto
// OFFSET: 0x00878200
static FMOD_RESULT F_CALLBACK SESeekCallback(void* handle, unsigned int pos, void* userdata) {
    SFile::SetFilePointer(static_cast<SFileImpl*>(handle), pos, nullptr, 0);
    return FMOD_OK;
}

// VERIFY: ee2176eb manual
// OFFSET: 0x0087B180
static FMOD_RESULT F_CALLBACK SENonBlockCallback(FMOD_SOUND* fmodSound, FMOD_RESULT result) {
    void* userdata = nullptr;
    FMOD_RESULT r = reinterpret_cast<FMOD::Sound*>(fmodSound)->getUserData(&userdata);

    if (r != FMOD_OK) {
        SE_ERRCHECK(r);
        return r;
    }

    s_soundListCritSect.Enter();

    SOUND_INTERNAL_LOOKUP* lookup = s_soundLookup.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(userdata)), s_lookupKey);

    if (!lookup) {
        s_soundListCritSect.Leave();
        return FMOD_OK;
    }

    SEFileSound* sound = static_cast<SEFileSound*>(lookup->m_sound);

    if (result != FMOD_OK) {
        sound->m_deleteDelay = 0;
        s_deleteList.LinkToTail(sound);

        for (SESoundInternal* other = s_soundList.Head(); other; other = s_soundList.Next(other)) {
            if (other->m_type == SESOUNDTYPE_FILE && static_cast<SEFileSound*>(other)->m_sound == reinterpret_cast<FMOD::Sound*>(fmodSound) && !other->m_channel) {
                s_deleteList.LinkToTail(other);
            }
        }

        s_soundListCritSect.Leave();
        return result;
    }

    for (SESoundInternal* other = s_soundList.Head(); other; other = s_soundList.Next(other)) {
        if (other->m_type == SESOUNDTYPE_FILE && static_cast<SEFileSound*>(other)->m_sound == reinterpret_cast<FMOD::Sound*>(fmodSound) && !other->m_channel) {
            s_goList.LinkToTail(other);
        }
    }

    s_soundListCritSect.Leave();

    s_createCritSect.Enter();

    if (sound->m_cached) {
        sound->m_waveData->m_loaded = 1;
    }

    s_createCritSect.Leave();

    return FMOD_OK;
}

// VERIFY: 3179309f manual
// OFFSET: 0x0087B390
static FMOD_RESULT F_CALLBACK SoundEndedCallback(FMOD_CHANNEL* channel, FMOD_CHANNEL_CALLBACKTYPE type, int command, unsigned int commanddata1, unsigned int commanddata2) {
    if (type != FMOD_CHANNEL_CALLBACKTYPE_END) {
        return FMOD_OK;
    }

    void* userdata = nullptr;
    FMOD_RESULT r = reinterpret_cast<FMOD::Channel*>(channel)->getUserData(&userdata);
    SE_ERRCHECK(r);

    s_soundListCritSect.Enter();

    SOUND_INTERNAL_LOOKUP* lookup = s_soundLookup.Ptr(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(userdata)), s_lookupKey);

    if (!lookup) {
        s_soundListCritSect.Leave();
        return FMOD_OK;
    }

    SESoundInternal* sound = lookup->m_sound;

    {
        SESound handle;

        if (!sound->m_handle) {
            sound->m_handle = &handle;
            handle.m_internal = sound;
        }

        if (sound->m_finishedCallback) {
            sound->m_finishedCallback(sound->m_handle, sound->m_finishedParam);
        }

        if (sound->m_finishedCallback2) {
            sound->m_finishedCallback2(sound->m_handle, sound->m_finishedParam2);
        }

        sound->m_channel = nullptr;
        s_deleteList.LinkToTail(sound);
    }

    s_soundListCritSect.Leave();

    return FMOD_OK;
}

// VERIFY: c125944a auto
// OFFSET: 0x00878220
FMOD_RESULT F_CALLBACK SEChatSound::DSPSeekCallback(FMOD_DSP_STATE* dspState, unsigned int position) {
    void* userdata;
    FMOD_RESULT result = reinterpret_cast<FMOD::DSP*>(dspState->instance)->getUserData(&userdata);

    if (result != FMOD_OK) {
        return result;
    }

    SEChatSound* sound = static_cast<SEChatSound*>(userdata);
    int32_t count = position - sound->m_position;

    if (count > 0) {
        sound->m_callbacks->DiscardSamples(sound->m_position, count);
        sound->m_position = position;
    }

    return FMOD_OK;
}

// VERIFY: 1190c055 manual
// OFFSET: 0x00879650
FMOD_RESULT F_CALLBACK SEChatSound::DSPReadCallback(FMOD_DSP_STATE* dspState, float* inBuffer, float* outBuffer, unsigned int length, int inChannels, int outChannels) {
    void* userdata;
    FMOD_RESULT result = reinterpret_cast<FMOD::DSP*>(dspState->instance)->getUserData(&userdata);

    if (result != FMOD_OK) {
        return result;
    }

    SEChatSound* sound = static_cast<SEChatSound*>(userdata);

    if (!sound || !sound->m_callbacks) {
        return FMOD_OK;
    }

    if (sound->GetScaledVolume() < 0.01f) {
        sound->m_callbacks->DiscardSamples(sound->m_position, length);
    } else if (sound->m_callbacks->GetSamples(sound->m_position, length, outBuffer)) {
        sound->m_position += length;
        return FMOD_OK;
    }

    memset(outBuffer, 0, length * outChannels * sizeof(float));
    sound->m_position += length;

    return FMOD_OK;
}

// VERIFY: 08c7d138 auto
// OFFSET: 0x008782B0
static float ComputeRolloff(float minDistance, float maxDistance, float distance) {
    if (distance >= maxDistance) {
        return 0.0f;
    }

    if (minDistance < distance) {
        float rolloff = minDistance / ((distance - minDistance) * 4.0f + minDistance);

        if (maxDistance * 0.9f < distance) {
            return (1.0f - (distance - maxDistance * 0.9f) / (maxDistance - maxDistance * 0.9f)) * rolloff;
        }

        return rolloff;
    }

    return 1.0f;
}

// VERIFY: 3af3f828 auto
// OFFSET: 0x00878320
static float F_CALLBACK RolloffCallback(FMOD_CHANNEL* channel, float distance) {
    float minDistance;
    float maxDistance;

    reinterpret_cast<FMOD::Channel*>(channel)->get3DMinMaxDistance(&minDistance, &maxDistance);

    return ComputeRolloff(minDistance, maxDistance, distance);
}

// VERIFY: 917e3eb9 auto
// OFFSET: 0x00878360
int32_t SE2::GetNumInputDrivers(int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    int32_t numDrivers = 0;

    if (!system) {
        return 0;
    }

    FMOD_OUTPUTTYPE output;
    system->getOutput(&output);
    system->getRecordNumDrivers(&numDrivers);

    return numDrivers + 1;
}

// VERIFY: 64a21a91 manual
// OFFSET: 0x008783B0
bool SE2::GetInputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    if (!system) {
        return false;
    }

    if (index) {
        system->getRecordDriverName(index - 1, name, nameLength);
        return true;
    }

    char text[64];
    SStrCopy(text, FrameScript_GetText("SYSTEM_DEFAULT", -1, GENDER_NOT_APPLICABLE), sizeof(text));

    if (text[0]) {
        SStrCopy(name, text, nameLength);
    } else {
        SStrCopy(name, "System Default", nameLength);
    }

    return true;
}

// VERIFY: c3da1418 auto
// OFFSET: 0x00878450
int32_t SE2::GetNumOutputDrivers(int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    int32_t numDrivers = 0;

    if (!system) {
        return 0;
    }

    FMOD_OUTPUTTYPE output;
    system->getOutput(&output);
    system->getNumDrivers(&numDrivers);

    return numDrivers + 1;
}

// VERIFY: 3dfdd81c manual
// OFFSET: 0x008784A0
bool SE2::GetOutputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    if (!system) {
        return false;
    }

    if (index) {
        system->getDriverName(index - 1, name, nameLength);
        return true;
    }

    char text[64];
    SStrCopy(text, FrameScript_GetText("SYSTEM_DEFAULT", -1, GENDER_NOT_APPLICABLE), sizeof(text));

    if (text[0]) {
        SStrCopy(name, text, nameLength);
    } else {
        SStrCopy(name, "System Default", nameLength);
    }

    return true;
}

// VERIFY: 4d61055b auto
// OFFSET: 0x00879060
static bool DriverNameMatches(int32_t index, const char* name, int32_t output, int32_t chatSystem) {
    char driverName[512];
    driverName[0] = '\0';

    if (output) {
        SE2::GetOutputDriverName(index, driverName, sizeof(driverName), chatSystem);
    } else {
        SE2::GetInputDriverName(index, driverName, sizeof(driverName), chatSystem);
    }

    return SStrCmp(driverName, name, STORM_MAX_STR) == 0;
}

// VERIFY: a48aeeed auto
// OFFSET: 0x008790C0
static int32_t FindDriverIndex(const char* name, int32_t index, int32_t output, int32_t chatSystem) {
    int32_t count = output ? SE2::GetNumOutputDrivers(chatSystem) : SE2::GetNumInputDrivers(chatSystem);

    if (index >= 0 && index < count && DriverNameMatches(index, name, output, chatSystem)) {
        return index;
    }

    for (int32_t i = 0; i < count; i++) {
        if (i == index) {
            continue;
        }

        char driverName[512];
        driverName[0] = '\0';

        if (output) {
            SE2::GetOutputDriverName(i, driverName, sizeof(driverName), chatSystem);
        } else {
            SE2::GetInputDriverName(i, driverName, sizeof(driverName), chatSystem);
        }

        if (!SStrCmp(driverName, name, STORM_MAX_STR)) {
            return i;
        }
    }

    return 0;
}

// VERIFY: 1f6c3f5f manual
// OFFSET: 0x00879180
static void LogUsingDriver(int32_t index, const char* use, int32_t output, int32_t chatSystem) {
    char driverName[512];
    driverName[0] = '\0';

    if (output) {
        SE2::GetOutputDriverName(index, driverName, sizeof(driverName), chatSystem);
    } else {
        SE2::GetInputDriverName(index, driverName, sizeof(driverName), chatSystem);
    }

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Using [%d] %s for %s", index, driverName, use);
}

// VERIFY: 7d6e3b25 manual
// OFFSET: 0x008791F0
int32_t SE2::GetNumInputDrivers_Cached(int32_t chatSystem) {
    if (chatSystem) {
        return s_chatInputDrivers.Count();
    }

    return SE2::GetNumInputDrivers(0);
}

// VERIFY: 1ea70c0e manual
// OFFSET: 0x00879210
bool SE2::GetInputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    if (!system) {
        return false;
    }

    if (!chatSystem) {
        return SE2::GetInputDriverName(index, name, nameLength, 0);
    }

    const char* text = index ? s_chatInputDrivers[index].m_name : FrameScript_GetText("SYSTEM_DEFAULT", -1, GENDER_NOT_APPLICABLE);
    SStrCopy(name, text, nameLength);

    return true;
}

// VERIFY: 0e97b194 auto
// OFFSET: 0x00879290
int32_t SE2::GetNumOutputDrivers_Cached(int32_t chatSystem) {
    if (chatSystem) {
        return s_chatOutputDrivers.Count();
    }

    return s_outputDrivers.Count();
}

// VERIFY: eb0105e4 manual
// OFFSET: 0x008792B0
bool SE2::GetOutputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem) {
    FMOD::System* system;

    if (chatSystem) {
        system = s_chatSystem;

        if (!system) {
            system = s_enumSystem;
        }
    } else {
        system = s_system;
    }

    if (!system) {
        return false;
    }

    const char* text;

    if (chatSystem) {
        text = index ? s_chatOutputDrivers[index].m_name : FrameScript_GetText("SYSTEM_DEFAULT", -1, GENDER_NOT_APPLICABLE);
    } else {
        text = index ? s_outputDrivers[index].m_name : FrameScript_GetText("SYSTEM_DEFAULT", -1, GENDER_NOT_APPLICABLE);
    }

    SStrCopy(name, text, nameLength);

    return true;
}

// VERIFY: b06917d3 auto
// OFFSET: 0x00878CD0
char* SE2::GetCurrentGameOutputDeviceName() {
    return s_gameOutputDeviceName;
}

// VERIFY: 9a526f4f auto
// OFFSET: 0x00878CE0
char* SE2::GetCurrentChatOutputDeviceName() {
    return s_chatOutputDeviceName;
}

// VERIFY: e4bb0f9f auto
// OFFSET: 0x00878CF0
char* SE2::GetCurrentChatInputDeviceName() {
    return s_chatInputDeviceName;
}

// VERIFY: 2c6d5005 manual
// OFFSET: 0x0087A9B0
static FMOD_RESULT F_CALLBACK SystemCallback(FMOD_SYSTEM* system, FMOD_SYSTEM_CALLBACKTYPE type, void* commanddata1, void* commanddata2) {
    if (type != FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED || !s_deviceChangeCallback) {
        return FMOD_OK;
    }

    int32_t count = SE2::GetNumOutputDrivers(0);
    s_outputDrivers.SetCount(count);

    for (int32_t i = 0; i < count; i++) {
        SE2::GetOutputDriverName(i, s_outputDrivers[i].m_name, sizeof(SEDriverInfo), 0);
    }

    count = SE2::GetNumOutputDrivers(1);
    s_chatOutputDrivers.SetCount(count);

    for (int32_t i = 0; i < count; i++) {
        SE2::GetOutputDriverName(i, s_chatOutputDrivers[i].m_name, sizeof(SEDriverInfo), 1);
    }

    count = SE2::GetNumInputDrivers(1);
    s_chatInputDrivers.SetCount(count);

    for (int32_t i = 0; i < count; i++) {
        SE2::GetInputDriverName(i, s_chatInputDrivers[i].m_name, sizeof(SEDriverInfo), 1);
    }

    s_deviceChangeCallback();

    return FMOD_OK;
}

// VERIFY: 76e44746 auto
// OFFSET: 0x0087ACC0
FMOD_RESULT SE2::SetDeviceChangeCallback(void (*callback)()) {
    s_deviceChangeCallback = callback;
    return s_system->setCallback(FMOD_SYSTEM_CALLBACKTYPE_DEVICELISTCHANGED, SystemCallback);
}

// VERIFY: a64b076d auto
// OFFSET: 0x00878290
void SE2::EnableReverb(int32_t enable) {
    s_reverbEnabled = enable;
}

// VERIFY: 9f66d261 auto
// OFFSET: 0x008782A0
int32_t SE2::IsReverbEnabled() {
    return s_reverbEnabled;
}

// VERIFY: a400a356 auto
// OFFSET: 0x00878540
void SE2::LockSoundList() {
    s_soundListCritSect.Enter();
}

// VERIFY: 05fd4347 auto
// OFFSET: 0x00878550
void SE2::UnlockSoundList() {
    s_soundListCritSect.Leave();
}

// VERIFY: d2076f78 auto
// OFFSET: 0x00878CB0
int32_t SE2::IsInitialized() {
    return s_initialized;
}

// VERIFY: f294f661 auto
// OFFSET: 0x00878CC0
int32_t SE2::IsChatSoundInitialized() {
    return SE2::sm_bCaptureInitialized;
}

// VERIFY: 6032fe76 auto
// OFFSET: 0x00878D00
FMOD_RESULT SE2::CreateDSPByType(FMOD_DSP_TYPE type, FMOD::DSP** dsp) {
    return s_system->createDSPByType(type, dsp);
}

// VERIFY: 8707a03f auto
// OFFSET: 0x00878D20
FMOD_RESULT SE2::SetDspParam(FMOD::DSP* dsp, int32_t index, float value) {
    return dsp->setParameter(index, value);
}

// VERIFY: 9d59e5ff auto
// OFFSET: 0x00878D40
FMOD_RESULT SE2::BypassDsp(FMOD::DSP* dsp, bool bypass) {
    return dsp->setBypass(bypass);
}

// VERIFY: 56a810d4 auto
// OFFSET: 0x00878D60
FMOD_RESULT SE2::EnableDSP(FMOD::DSP* dsp, bool active) {
    return dsp->setActive(active);
}

// VERIFY: 2dcee2b9 auto
// OFFSET: 0x00878D80
FMOD_RESULT SE2::DspDestroy(FMOD::DSP* dsp) {
    dsp->remove();
    dsp->release();
    return FMOD_OK;
}

// VERIFY: 11691829 auto
// OFFSET: 0x00878DA0
FMOD_RESULT SE2::ConnectDspToChannelGroup(FMOD::DSP* dsp, FMOD::ChannelGroup* channelGroup) {
    FMOD::DSP* head;
    FMOD::DSP* output;

    channelGroup->getDSPHead(&head);
    head->getOutput(0, &output);
    head->disconnectAll(false, true);
    output->addInput(dsp);
    dsp->addInput(head);

    return FMOD_OK;
}

// VERIFY: abe7e5f3 auto
// OFFSET: 0x00878DF0
int32_t SE2::GetChannelGroupNumChannels(FMOD::ChannelGroup* channelGroup) {
    int32_t numChannels = 0;
    channelGroup->getNumChannels(&numChannels);
    return numChannels;
}

// VERIFY: c41e4fce auto
// OFFSET: 0x00878E10
FMOD::ChannelGroup* SE2::CreateChannelGroup(const char* name) {
    FMOD::ChannelGroup* channelGroup = nullptr;
    s_system->createChannelGroup(name, &channelGroup);
    return channelGroup;
}

// VERIFY: 7d73280f auto
// OFFSET: 0x00878E40
FMOD_RESULT SE2::DestroyChannelGroup(FMOD::ChannelGroup* channelGroup) {
    return channelGroup->release();
}

// VERIFY: a4d562bb auto
// OFFSET: 0x00878E50
FMOD_RESULT SE2::AddChannelToChannelGroup(FMOD::Channel* channel, FMOD::ChannelGroup* channelGroup) {
    return channel->setChannelGroup(channelGroup);
}

// VERIFY: 91f37a92 auto
// OFFSET: 0x00878E70
FMOD_RESULT SE2::SetFMODChannelGroupVolume(FMOD::ChannelGroup* channelGroup, float volume) {
    return channelGroup->setVolume(volume);
}

// VERIFY: b9738bd4 auto
// OFFSET: 0x00879320
void SE2::SetListenerAttributes(const C3Vector* position, const C3Vector* forward, const C3Vector* up, const C3Vector* velocity) {
    if (!s_initialized) {
        return;
    }

    s_system->set3DListenerAttributes(0, reinterpret_cast<const FMOD_VECTOR*>(position), reinterpret_cast<const FMOD_VECTOR*>(velocity), reinterpret_cast<const FMOD_VECTOR*>(forward), reinterpret_cast<const FMOD_VECTOR*>(up));
}

// VERIFY: 78a13624 manual
// OFFSET: 0x00879350
bool SE2::GetListenerPosition(C3Vector* position) {
    return s_initialized && position && s_system->get3DListenerAttributes(0, reinterpret_cast<FMOD_VECTOR*>(position), nullptr, nullptr, nullptr) == FMOD_OK;
}

// VERIFY: 908e27b2 auto
// OFFSET: 0x00879460
void SE2::SetMasterVolume(float volume) {
    if (s_initialized && volume >= 0.0f && volume <= 1.0f) {
        s_channelGroups[0].m_volume = volume;
        s_channelGroups[0].m_dirty = 1;
    }
}

// VERIFY: c5ed9679 auto
// OFFSET: 0x008794A0
void SE2::MuteAllSound(bool mute) {
    if (s_initialized) {
        s_channelGroups[0].m_muteVolume = mute ? 0.0f : 1.0f;
        s_channelGroups[0].m_dirty = 1;
    }
}

// VERIFY: d860b528 auto
// OFFSET: 0x008794E0
void SE2::SetReverb(const FMOD_REVERB_PROPERTIES* properties) {
    if (!s_initialized) {
        return;
    }

    s_system->setReverbProperties(properties);
}

// VERIFY: d9256c5e manual
// OFFSET: 0x0087A6F0
static SEChannelGroup* FindOrCreateChannelGroup(const char* name, bool create, bool childOfMaster) {
    if (!s_initialized) {
        return nullptr;
    }

    uint32_t hash = SStrHashHT(name);

    for (uint32_t i = 1; i < s_channelGroups.Count(); i++) {
        if (hash == s_channelGroups[i].m_hash) {
            return &s_channelGroups[i];
        }
    }

    if (!create) {
        return nullptr;
    }

    SEChannelGroup* group = s_channelGroups.New();
    group->m_volume = 1.0f;
    group->m_hash = hash;
    group->m_muteVolume = 1.0f;
    group->m_dirty = 1;
    group->m_parent = childOfMaster ? 0 : -1;

    return group;
}

// VERIFY: 84a7ffbf auto
// OFFSET: 0x0087A850
void SE2::SetChannelGroupVolume(const char* name, float volume) {
    if (s_initialized && volume >= 0.0f && volume <= 1.0f && name) {
        SEChannelGroup* group = FindOrCreateChannelGroup(name, true, true);
        group->m_volume = volume;
        group->m_dirty = 1;
    }
}

// VERIFY: f2659f25 auto
// OFFSET: 0x0087A8A0
float SE2::GetChannelGroupVolume(const char* name) {
    if (!s_initialized) {
        return 0.0f;
    }

    if (name) {
        SEChannelGroup* group = FindOrCreateChannelGroup(name, false, false);

        if (group) {
            return group->m_muteVolume * group->m_volume;
        }
    }

    return 1.0f;
}

// VERIFY: 9f67fc4f auto
// OFFSET: 0x0087A8E0
void SE2::MuteChannelGroup(const char* name, bool mute) {
    if (s_initialized && name) {
        SEChannelGroup* group = FindOrCreateChannelGroup(name, false, false);

        if (group) {
            group->m_dirty = 1;
            group->m_muteVolume = mute ? 0.0f : 1.0f;
        }
    }
}

// VERIFY: 9432539d manual
// OFFSET: 0x0087A920
void SE2::StopAllSounds(const char* channelGroup, float fadeOutTime) {
    if (!s_initialized) {
        return;
    }

    SEChannelGroup* group = FindOrCreateChannelGroup(channelGroup, false, false);

    if (!group) {
        return;
    }

    int32_t index = group - s_channelGroups.Ptr();

    s_soundListCritSect.Enter();

    for (SESoundInternal* sound = s_soundList.Head(); sound; sound = s_soundList.Next(sound)) {
        if (sound->m_channelGroup == index) {
            sound->m_fadeOutTime = fadeOutTime;
            sound->m_fadingIn = 0;
            sound->m_fadingOut = 1;
        }
    }

    s_soundListCritSect.Leave();
}

// VERIFY: ad3ee713 manual
// OFFSET: 0x00878AD0
FMOD::SoundGroup* SE2::CreateSoundGroup(const char* name, int32_t maxAudible) {
    FMOD::SoundGroup* soundGroup = nullptr;

    FMOD_RESULT result = s_system->createSoundGroup(name, &soundGroup);
    SE_ERRCHECK(result);

    result = soundGroup->setMaxAudible(maxAudible);
    SE_ERRCHECK(result);

    return soundGroup;
}

// VERIFY: d9c047a9 manual
// OFFSET: 0x00878B60
void SE2::SetSoundGroupMaxAudibleBehavior(FMOD::SoundGroup* soundGroup, int32_t behavior, float muteFadeSpeed) {
    if (!soundGroup) {
        return;
    }

    FMOD_RESULT result;

    switch (behavior) {
        case 0:
            result = soundGroup->setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR_MUTE);
            SE_ERRCHECK(result);
            result = soundGroup->setMuteFadeSpeed(muteFadeSpeed);
            SE_ERRCHECK(result);
            break;

        case 1:
            result = soundGroup->setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR_STEALLOWEST);
            SE_ERRCHECK(result);
            break;

        case 2:
            result = soundGroup->setMaxAudibleBehavior(FMOD_SOUNDGROUP_BEHAVIOR_FAIL);
            SE_ERRCHECK(result);
            break;
    }
}

// VERIFY: e1f2bd0a auto
// OFFSET: 0x00878C90
FMOD_RESULT SE2::ReleaseSoundGroup(FMOD::SoundGroup* soundGroup) {
    if (soundGroup) {
        return soundGroup->release();
    }

    return FMOD_OK;
}

// VERIFY: 3a3e49bd auto
// OFFSET: 0x00878930
void SE2::SetUserMonitorDSPCallback(void (*callback)(float*, uint32_t)) {
    s_userMonitorDSPCallback = callback;
}

// VERIFY: 73c92b66 auto
// OFFSET: 0x00878940
FMOD_RESULT SE2::AttachMonitorDSP(SESound* sound) {
    return sound->m_internal->m_channel->addDSP(s_monitorDSP);
}

// VERIFY: 82b7c600 auto
// OFFSET: 0x00878960
static FMOD_RESULT F_CALLBACK MonitorDSPCallback(FMOD_DSP_STATE* dspState, float* inBuffer, float* outBuffer, unsigned int length, int inChannels, int outChannels) {
    memcpy(outBuffer, inBuffer, length * sizeof(float));

    if (s_userMonitorDSPCallback) {
        s_userMonitorDSPCallback(inBuffer, length);
    }

    return FMOD_OK;
}

// VERIFY: da7f71ea manual
// OFFSET: 0x008789A0
static FMOD::DSP* CreateMonitorDSP() {
    FMOD_DSP_DESCRIPTION description;
    memset(&description, 0, sizeof(description));

    SStrCopy(description.name, "Monitor DSP", sizeof(description.name));
    description.channels = 0;
    description.read = MonitorDSPCallback;
    description.userdata = reinterpret_cast<void*>(0x12345);

    s_chatSystem->createDSP(&description, &s_monitorDSP);

    return s_monitorDSP;
}

// VERIFY: b952975e auto
// OFFSET: 0x00878A20
void SE2::SetPrimaryCaptureCallback(SECaptureCallbacks* callbacks) {
    s_primaryCaptureCallbacks = callbacks;
}

// VERIFY: 2b6f05ba auto
// OFFSET: 0x00878A30
void SE2::SetAlternateCaptureCallback(SECaptureCallbacks* callbacks) {
    s_alternateCaptureCallbacks = callbacks;
}

// VERIFY: 2d10e8bd auto
// OFFSET: 0x00878A40
void SE2::ActivateAlternateCaptureCallback() {
    s_heartBeatCritSect.Enter();
    s_captureCallbacks = s_alternateCaptureCallbacks;
    s_heartBeatCritSect.Leave();
}

// VERIFY: 94f19c43 auto
// OFFSET: 0x00878A60
bool SE2::AlternateCaptureCallbackIsActive() {
    return s_captureCallbacks == s_alternateCaptureCallbacks;
}

// VERIFY: af0e40be auto
// OFFSET: 0x00878A80
void SE2::ActivatePrimaryCaptureCallback() {
    s_heartBeatCritSect.Enter();
    s_captureCallbacks = s_primaryCaptureCallbacks;
    s_heartBeatCritSect.Leave();
}

// VERIFY: c9d66fe5 manual
// OFFSET: 0x00878AA0
bool SE2::IsCapturing() {
    bool recording = false;

    if (!s_chatSystem) {
        return false;
    }

    s_chatSystem->isRecording(&recording);

    return recording;
}

// VERIFY: 114728bb manual
// OFFSET: 0x008787E0
void SE2::CaptureWork() {
    if (!s_capturing) {
        return;
    }

    s_heartBeatCritSect.Enter();

    uint32_t position = 0;
    FMOD_RESULT result = s_chatSystem->getRecordPosition(&position);
    SE_ERRCHECK(result);

    if (position != s_captureLastPosition) {
        int32_t samples = position - s_captureLastPosition;

        if (samples < 0) {
            samples += s_captureLength;
        }

        if (s_captureCallbacks) {
            void* ptr1;
            void* ptr2;
            uint32_t len1;
            uint32_t len2;

            result = s_captureSound->lock(s_captureLastPosition * s_captureChannels * sizeof(int16_t), samples * s_captureChannels * sizeof(int16_t), &ptr1, &ptr2, &len1, &len2);

            if (ptr1 && len1) {
                s_captureCallbacks->ProcessData(static_cast<int16_t*>(ptr1), len1 / sizeof(int16_t));
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "*** SoundEngine_CaptureWork : Lock failed! (%d, %d) %s", ptr1, len1, FMOD_ErrorString(result));
            }

            if (ptr2 && len2) {
                s_captureCallbacks->ProcessData(static_cast<int16_t*>(ptr2), len2 / sizeof(int16_t));
            }

            s_captureSound->unlock(ptr1, ptr2, len1, len2);
        }
    }

    s_captureLastPosition = position;

    s_heartBeatCritSect.Leave();
}

// VERIFY: 07f8cfc0 manual
// OFFSET: 0x00879500
void SE2::StartCapture() {
    if (!s_chatSystem) {
        return;
    }

    s_capturing = 1;

    if (!SE2::sm_bCaptureInitialized) {
        return;
    }

    bool recording = false;
    s_chatSystem->isRecording(&recording);

    if (!recording) {
        FMOD_RESULT result = s_chatSystem->recordStart(s_captureSound, true);
        SE_ERRCHECK(result);
    }
}

// VERIFY: a900c6d0 manual
// OFFSET: 0x00879580
void SE2::StopCapture() {
    if (!s_chatSystem || !SE2::sm_bCaptureInitialized) {
        return;
    }

    bool recording = false;
    s_chatSystem->isRecording(&recording);

    if (recording) {
        FMOD_RESULT result = s_chatSystem->recordStop();
        SE_ERRCHECK(result);
    }
}

// VERIFY: cb40798f auto
// OFFSET: 0x0087C540
void SE2::ShutDownCapture() {
    s_capturing = 0;

    bool recording = false;

    if (s_chatSystem) {
        s_chatSystem->isRecording(&recording);

        if (recording) {
            SE2::StopCapture();
        }
    }

    SE2::HeartBeat(nullptr, nullptr);
}

// VERIFY: be2a4fb0 manual
// OFFSET: 0x0087AD40
SEWaveDataNode::SEWaveDataNode() {
    this->m_sound = nullptr;
    this->m_hash = 0;
    this->m_refCount = 0;
    this->m_loaded = 0;
    s_waveDataList.LinkToTail(this);
}

// VERIFY: 8989fc79 manual
// OFFSET: 0x0087ACE0
SEWaveDataNode::~SEWaveDataNode() {
    this->m_sound = nullptr;
    this->m_hash = 0;
    this->m_refCount = 0;
}

// VERIFY: f5aa5a13 manual
// OFFSET: 0x0087AD70
SESoundInternal::SESoundInternal() {
    this->m_position.x = s_invalid;
    this->m_position.y = s_invalid;
    this->m_position.z = s_invalid;
    this->m_minDistance = s_invalid;
    this->m_maxDistance = s_invalid;
    this->m_channelGroup = 0;
    this->m_mode = 0;
    this->m_frequency = 0.0f;
    this->m_play = 0;
    this->m_stop = 0;
    this->m_pitch = 1.0f;
    this->m_randomStart = 0;
    this->m_software = 0;
    this->m_priority = -1;
    this->m_coneOrientation.x = 0.0f;
    this->m_coneOrientation.y = 0.0f;
    this->m_coneOrientation.z = 0.0f;
    this->m_volume = 1.0f;
    this->m_userData = nullptr;
    this->m_fadingIn = 0;
    this->m_fadeVolume = 0.0f;
    this->m_fadingOut = 0;
    this->m_fadeInTime = 0.0f;
    this->m_playTime = 0;
    this->m_fadeOutTime = 0.0f;
    this->m_cached = 0;
    this->m_finishedCallback = nullptr;
    this->m_coneInsideAngle = 360.0f;
    this->m_finishedCallback2 = nullptr;
    this->m_coneOutsideAngle = 360.0f;
    this->m_fadeOutCallback = nullptr;
    this->m_finishedParam = nullptr;
    this->m_coneOutsideVolume = 1.0f;
    this->m_finishedParam2 = nullptr;
    this->m_dspChainCallback = nullptr;
    this->m_dspChainName = nullptr;
    this->m_objectGUID = 0;
    this->m_channel = nullptr;
    this->m_handle = nullptr;
    this->m_type = SESOUNDTYPE_NONE;
    this->m_reverb = 0;
    this->m_loaded = 0;
    this->m_id = s_nextSoundID;
    this->m_deleteDelay = 500;
    s_nextSoundID++;
}

// VERIFY: 86527066 manual
// OFFSET: 0x0087BC80
SESoundInternal::~SESoundInternal() {
    s_soundListCritSect.Enter();

    SOUND_INTERNAL_LOOKUP* lookup = s_soundLookup.Ptr(this->m_id, s_lookupKey);

    if (lookup) {
        s_soundLookup.Delete(lookup);
    } else {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
    }

    s_soundListCritSect.Leave();

    if (this->m_handle) {
        this->m_handle->Detach();
    }

    if (this->m_channel) {
        this->m_channel->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, nullptr, 0);
        this->m_channel->stop();
        this->m_channel = nullptr;
    }

    if (this->m_userData) {
        this->m_userData->Release();
        this->m_userData = nullptr;
    }

    if (this->m_dspChainName) {
        delete[] this->m_dspChainName;
        this->m_dspChainName = nullptr;
    }
}

// VERIFY: 7abdae88 auto
// OFFSET: 0x008C8DE0
int32_t SESoundInternal::IsReadyForRelease() {
    return 1;
}

// VERIFY: 3135d777 auto
// OFFSET: 0x008785A0
int32_t SESoundInternal::Set3DMinMaxDistance(float minDistance, float maxDistance) {
    this->m_minDistance = minDistance;
    this->m_maxDistance = maxDistance;

    FMOD_RESULT result = FMOD_ERR_INTERNAL;

    if (this->m_channel) {
        result = this->m_channel->set3DMinMaxDistance(minDistance, maxDistance);
    }

    return result == FMOD_OK;
}

// VERIFY: d9ee1080 auto
// OFFSET: 0x00878FD0
float SESoundInternal::GetScaledVolume() {
    if (!s_initialized) {
        return 0.0f;
    }

    float volume = this->m_volume;

    if (this->m_fadingIn || this->m_fadingOut) {
        volume *= this->m_fadeVolume;
    }

    if (s_channelGroups.Count()) {
        for (int32_t group = this->m_channelGroup; group != -1; group = s_channelGroups[group].m_parent) {
            volume = volume * s_channelGroups[group].m_volume * s_channelGroups[group].m_muteVolume;
        }
    } else {
        volume = 0.0f;
    }

    if (this->m_type != SESOUNDTYPE_CHAT && s_volumeCallback) {
        return s_volumeCallback(this->m_userData) * volume;
    }

    return volume;
}

// VERIFY: fe1b36c1 auto
// OFFSET: 0x008796F0
void SESoundInternal::UpdateVolume() {
    if (this->m_channel) {
        this->m_channel->setVolume(this->GetScaledVolume());
    }
}

// VERIFY: a6a3b94a manual
// OFFSET: 0x00879760
void SESoundInternal::Play() {
    FMOD_MODE mode;

    if (this->m_type == SESOUNDTYPE_FILE && this->m_randomStart && this->m_channel->getMode(&mode) == FMOD_OK && (mode & FMOD_LOOP_NORMAL)) {
        uint32_t length = 0;
        static_cast<SEFileSound*>(this)->m_sound->getLength(&length, FMOD_TIMEUNIT_PCMBYTES);
        uint32_t random = CRandom::uint32_(g_rndSeed);
        this->m_channel->setPosition(static_cast<uint32_t>((static_cast<uint64_t>(random) * (length >> 1)) >> 32) & ~7u, FMOD_TIMEUNIT_PCMBYTES);
    }

    if (this->m_fadeInTime > 0.0f) {
        this->m_fadingOut = 0;
        this->m_fadingIn = 1;
    } else {
        this->m_fadeVolume = 1.0f;
    }

    this->UpdateVolume();

    FMOD_RESULT result = this->m_channel->setPaused(false);
    SE_ERRCHECK(result);
}

// VERIFY: 75888758 manual
// OFFSET: 0x0087EBC0
SEFileSound::SEFileSound() {
    this->m_file = nullptr;
    this->unused_00E4 = 0;
    this->unused_00E8 = 0;
    this->m_sound = nullptr;
    this->m_loadSoundGroup = nullptr;
    this->m_soundGroup = nullptr;
    this->m_loadCompleted = 0;
    this->m_waveData = nullptr;

    s_soundListCritSect.Enter();
    s_soundList.LinkToTail(this);
    s_soundLookup.New(this->m_id, s_lookupKey, 0, 0)->m_sound = this;
    s_soundListCritSect.Leave();

    // STATIC: 0x00D43978
    static CRndSeed s_seed(0xFEAFDADA);
    this->m_deleteDelay += CRandom::uint32_(s_seed) & 0x1FF;
}

// VERIFY: 2bfa301f auto
// OFFSET: [0x0087BDF0, 0x0087C400]
SEFileSound::~SEFileSound() {
    if (this->m_waveData) {
        this->m_waveData->m_refCount--;
    }

    if (!this->m_waveData && this->m_sound) {
        this->m_sound->release();
        this->m_sound = nullptr;
    }
}

// VERIFY: c201109e auto
// OFFSET: 0x0087BE80
int32_t SEFileSound::IsReadyForRelease() {
    bool ready;

    if (!this->m_sound) {
        return 1;
    }

    this->m_sound->readyForRelease(&ready);

    return ready;
}

// VERIFY: 3a718434 manual
// OFFSET: 0x0087A070
void SEFileSound::AbortNonBlockingLoad(FMOD_RESULT result) {
    if (this->m_sound) {
        if (result != FMOD_ERR_MAXAUDIBLE) {
            char name[256];
            this->m_sound->getName(name, sizeof(name));
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "##### ERROR In CompleteNonBlockingLoad()! Aborting this sound! %s", name);
        }

        if (this->m_channel) {
            this->m_channel->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, nullptr, 0);
            this->m_channel->stop();
            this->m_channel = nullptr;
        }
    }

    s_soundListCritSect.Enter();

    if (this->m_waveData) {
        for (SESoundInternal* sound = s_soundList.Head(); sound; sound = s_soundList.Next(sound)) {
            if (sound->m_type == SESOUNDTYPE_FILE && static_cast<SEFileSound*>(sound)->m_sound == this->m_sound && !sound->m_channel) {
                sound->m_deleteDelay = 0;
                s_deleteList.LinkToTail(sound);
            }
        }
    } else {
        s_deleteList.LinkToTail(this);
    }

    s_soundListCritSect.Leave();
}

// VERIFY: 6efd50e7 manual
// OFFSET: 0x0087B5F0
void SEFileSound::CompleteNonBlockingLoad() {
    this->m_loadCompleted = 1;

    if (this->m_stop) {
        this->AbortNonBlockingLoad(FMOD_ERR_MAXAUDIBLE);
        return;
    }

    if (!this->m_sound) {
        SE2::Log_Write(__LINE__, __FILE__, FMOD_ERR_INTERNAL, "##### ERROR! INVALID m_pFMODSound");
        this->AbortNonBlockingLoad(FMOD_OK);
        return;
    }

    FMOD_RESULT result = this->m_system->playSound(FMOD_CHANNEL_FREE, this->m_sound, true, &this->m_channel);

    if (result != FMOD_OK) {
        if (result != FMOD_ERR_MAXAUDIBLE) {
            SE2::Log_Write(__LINE__, __FILE__, result, "CNBL: playSound Failed");
        }

        this->AbortNonBlockingLoad(result);
        return;
    }

    result = this->m_channel->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(this->m_id)));

    if (result != FMOD_OK) {
        SE2::Log_Write(__LINE__, __FILE__, result, "CNBL: setUserData Failed");
        this->AbortNonBlockingLoad(result);
        return;
    }

    result = this->m_channel->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, SoundEndedCallback, 0);

    if (result != FMOD_OK) {
        SE2::Log_Write(__LINE__, __FILE__, result, "CNBL: setCallback Failed");
        this->AbortNonBlockingLoad(result);
        return;
    }

    FMOD_MODE mode;
    result = this->m_channel->getMode(&mode);

    if (result != FMOD_OK) {
        SE2::Log_Write(__LINE__, __FILE__, result, "CNBL: getMode Failed");
        this->AbortNonBlockingLoad(result);
        return;
    }

    FMOD_REVERB_CHANNELPROPERTIES properties = { 0, 0, 0, 0, 0, 0.0f, 0, 0.25f, 1.5f, 1.0f, 0, 1.0f, 0, 0.0f, 0.0f, 0.0f, 1.0f, FMOD_REVERB_CHANNELFLAGS_DEFAULT };
    this->m_channel->setReverbProperties(&properties);

    if (mode & FMOD_2D) {
        if (mode & FMOD_SOFTWARE) {
            result = this->m_channel->setSpeakerMix(s_speakerMixFront, s_speakerMixFront, s_speakerMixCenter, s_speakerMixLFE, s_speakerMixBack, s_speakerMixBack, s_speakerMixSide, s_speakerMixSide);

            if (result != FMOD_OK) {
                SE2::Log_Write(__LINE__, __FILE__, result, "");
                this->AbortNonBlockingLoad(result);
                return;
            }
        }

        if (!this->m_reverb) {
            this->m_channel->getReverbProperties(&properties);
            properties.Room = -10000;
            this->m_channel->setReverbProperties(&properties);
        }
    } else if (mode & FMOD_SOFTWARE) {
        if (this->m_software) {
            result = this->m_channel->setSpeakerMix(s_speakerMixFront, s_speakerMixFront, s_speakerMixCenter, s_speakerMixLFE, s_speakerMixBack, s_speakerMixBack, s_speakerMixSide, s_speakerMixSide);

            if (result != FMOD_OK) {
                SE2::Log_Write(__LINE__, __FILE__, result, "");
                this->AbortNonBlockingLoad(result);
                return;
            }
        } else {
            result = this->m_channel->setSpeakerMix(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

            if (result != FMOD_OK) {
                SE2::Log_Write(__LINE__, __FILE__, result, "");
                this->AbortNonBlockingLoad(result);
                return;
            }
        }
    }

    if (!IsNaN(this->m_position.x)) {
        result = this->m_channel->set3DAttributes(reinterpret_cast<FMOD_VECTOR*>(&this->m_position), nullptr);

        if (result != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, result, "");
            this->AbortNonBlockingLoad(result);
            return;
        }
    }

    if (!IsNaN(this->m_minDistance) && !IsNaN(this->m_maxDistance) && !this->Set3DMinMaxDistance(this->m_minDistance, this->m_maxDistance)) {
        SE2::Log_Write(__LINE__, __FILE__, FMOD_ERR_INTERNAL, "");
        this->AbortNonBlockingLoad(result);
        return;
    }

    this->UpdateVolume();

    if (this->m_pitch != 1.0f) {
        FMOD_RESULT frequencyResult = this->m_channel->getFrequency(&this->m_frequency);

        if (frequencyResult != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, frequencyResult, "");
            this->AbortNonBlockingLoad(frequencyResult);
            return;
        }

        result = this->m_channel->setFrequency(this->m_pitch * this->m_frequency);

        if (result != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, result, "");
            this->AbortNonBlockingLoad(result);
            return;
        }
    }

    if (static_cast<uint32_t>(this->m_priority) != 128 && static_cast<uint32_t>(this->m_priority) <= 256 && this->m_channel) {
        result = this->m_channel->setPriority(this->m_priority);

        if (result != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, result, "");
            this->AbortNonBlockingLoad(result);
            return;
        }
    }

    if (this->m_software && (this->m_coneOrientation.x != 0.0f || this->m_coneOrientation.y != 0.0f || this->m_coneOrientation.z != 0.0f)) {
        FMOD_RESULT coneResult = this->m_channel->set3DConeSettings(this->m_coneInsideAngle, this->m_coneOutsideAngle, this->m_coneOutsideVolume);

        if (coneResult != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, coneResult, "");
            this->AbortNonBlockingLoad(coneResult);
            return;
        }

        if (sqrt(this->m_coneOrientation.x * this->m_coneOrientation.x + this->m_coneOrientation.y * this->m_coneOrientation.y + this->m_coneOrientation.z * this->m_coneOrientation.z) < 0.0001f) {
            this->m_coneOrientation.x = 0.0f;
            this->m_coneOrientation.y = 0.0f;
            this->m_coneOrientation.z = 1.0f;
        } else {
            this->m_coneOrientation.Normalize();
        }

        FMOD_VECTOR orientation;
        orientation.x = this->m_coneOrientation.x;
        orientation.y = this->m_coneOrientation.y;
        orientation.z = -this->m_coneOrientation.z;

        result = this->m_channel->set3DConeOrientation(&orientation);

        if (result != FMOD_OK) {
            SE2::Log_Write(__LINE__, __FILE__, result, "");
            this->AbortNonBlockingLoad(result);
            return;
        }
    }

    if (this->m_dspChainCallback) {
        this->m_dspChainCallback(this->m_channel, this->m_dspChainName);
    }

    this->m_playTime = OsGetAsyncTimeMs();

    if (this->m_play) {
        this->Play();
    }
}

// VERIFY: 34ec1267 manual
// OFFSET: 0x0087ECF0
SEMemorySound::SEMemorySound() {
    this->m_sound = nullptr;

    s_soundListCritSect.Enter();
    s_soundList.LinkToTail(this);
    s_soundLookup.New(this->m_id, s_lookupKey, 0, 0)->m_sound = this;
    s_soundListCritSect.Leave();
}

// VERIFY: d8ebd52a manual
// OFFSET: 0x0087C430
SEMemorySound::~SEMemorySound() {
    if (this->m_sound) {
        this->m_sound->release();
        this->m_sound = nullptr;
    }
}

// VERIFY: cc1e3079 manual
// OFFSET: 0x0087EDA0
SEChatSound::SEChatSound() {
    this->m_callbacks = nullptr;
    this->m_position = 0;
    this->m_dsp = nullptr;

    s_soundListCritSect.Enter();
    s_soundList.LinkToTail(this);
    s_soundLookup.New(this->m_id, s_lookupKey, 0, 0)->m_sound = this;
    s_soundListCritSect.Leave();
}

// VERIFY: 8dda86da auto
// OFFSET: [0x0087BEB0, 0x0087C480]
SEChatSound::~SEChatSound() {
    if (s_chatSystem && this->m_dsp) {
        this->m_dsp->release();
        this->m_dsp = nullptr;
    }

    if (this->m_callbacks) {
        this->m_callbacks->Delete();
        this->m_callbacks = nullptr;
    }
}

// INLINED
SESound::SESound() {
    this->m_internal = nullptr;
}

// VERIFY: 117baaa5 manual
// OFFSET: 0x008799E0
SESound::~SESound() {
    if (this->m_internal) {
        this->m_internal->m_handle = nullptr;

        if (this->IsLooping()) {
            this->Stop(0, 0.5f);
        }
    }

    this->m_internal = nullptr;
}

// VERIFY: 9aa39190 auto
// OFFSET: 0x00878270
void SESound::Detach() {
    if (this->m_internal) {
        this->m_internal->m_handle = nullptr;
        this->m_internal = nullptr;
    }
}

// VERIFY: 4d2a7376 auto
// OFFSET: 0x00878560
void SESound::SetUserData(SESoundUserData* userData) {
    if (this->m_internal) {
        this->m_internal->m_userData = userData;
    }
}

// VERIFY: 0fc7828e auto
// OFFSET: 0x00878580
SESoundUserData* SESound::GetUserData() {
    if (this->m_internal) {
        return this->m_internal->m_userData;
    }

    return nullptr;
}

// VERIFY: 8be60cb2 auto
// OFFSET: 0x00878590
uint32_t SESound::GetID() {
    if (this->m_internal) {
        return this->m_internal->m_id;
    }

    return 0;
}

// VERIFY: eeac044f auto
// OFFSET: 0x008785F0
void SESound::SetFadeInTime(float seconds) {
    if (this->m_internal) {
        this->m_internal->m_fadeInTime = seconds;
    }
}

// VERIFY: 5c2eeb49 auto
// OFFSET: 0x00878610
void SESound::SetFadeOutTime(float seconds) {
    if (this->m_internal) {
        this->m_internal->m_fadeOutTime = seconds;
    }
}

// VERIFY: 0db996e0 auto
// OFFSET: 0x00878630
uint32_t SESound::GetPosition() {
    if (!this->m_internal) {
        return 0;
    }

    uint32_t position = 0;

    if (this->m_internal->m_channel) {
        this->m_internal->m_channel->getPosition(&position, FMOD_TIMEUNIT_MS);
    }

    return position;
}

// VERIFY: 6d3ec9fa auto
// OFFSET: 0x00878660
void SESound::SetPosition(uint32_t position) {
    if (this->m_internal && this->m_internal->m_channel) {
        this->m_internal->m_channel->setPosition(position, FMOD_TIMEUNIT_MS);
    }
}

// VERIFY: 50de3d0d auto
// OFFSET: 0x00878680
void SESound::SetFinishedCallback(void (*callback)(SESound*, void*), void* param) {
    if (this->m_internal) {
        this->m_internal->m_finishedCallback = callback;
        this->m_internal->m_finishedParam = param;
    }
}

// VERIFY: b0b93c11 auto
// OFFSET: 0x008786B0
void SESound::SetFinishedCallback2(void (*callback)(SESound*, void*), void* param) {
    if (this->m_internal) {
        this->m_internal->m_finishedCallback2 = callback;
        this->m_internal->m_finishedParam2 = param;
    }
}

// VERIFY: a553fb84 manual
// OFFSET: 0x008786E0
void SESound::SetAttachCustomDSPChainCallback(void (*callback)(FMOD::Channel*, char*), const char* name) {
    if (this->m_internal) {
        this->m_internal->m_dspChainCallback = callback;
        this->m_internal->m_dspChainName = static_cast<char*>(SMemAlloc(128, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));
        SStrCopy(this->m_internal->m_dspChainName, name, 128);
    }
}

// VERIFY: f5c21050 manual
// OFFSET: 0x00878730
bool SESound::IsPlaying() {
    bool isPlaying = false;

    if (!this->m_internal) {
        return false;
    }

    isPlaying = this->m_internal->m_play;

    if (this->m_internal->m_channel) {
        this->m_internal->m_channel->isPlaying(&isPlaying);
    }

    return isPlaying;
}

// VERIFY: 3ce55de5 auto
// OFFSET: 0x00878760
void SESound::SetPitch(float pitch) {
    if (!this->m_internal) {
        return;
    }

    this->m_internal->m_pitch = pitch;

    if (this->m_internal->m_channel) {
        if (this->m_internal->m_frequency == 0.0f) {
            this->m_internal->m_channel->getFrequency(&this->m_internal->m_frequency);
        }

        this->m_internal->m_channel->setFrequency(this->m_internal->m_frequency * pitch);
    }
}

// VERIFY: 0cdbb285 auto
// OFFSET: 0x008787B0
void SESound::Set3DPanLevel(float level) {
    if (this->m_internal && this->m_internal->m_channel) {
        this->m_internal->m_channel->set3DPanLevel(level);
    }
}

// VERIFY: 7dcd0c8f auto
// OFFSET: 0x00879390
void SESound::Set3DMinMaxDistance(float minDistance, float maxDistance) {
    if (this->m_internal) {
        this->m_internal->Set3DMinMaxDistance(minDistance, maxDistance);
    }
}

// VERIFY: 5d840bfb auto
// OFFSET: 0x008793C0
void SESound::Set3DPosition(const C3Vector& position) {
    if (this->m_internal) {
        this->m_internal->m_position = position;

        if (this->m_internal->m_channel) {
            this->m_internal->m_channel->set3DAttributes(reinterpret_cast<const FMOD_VECTOR*>(&position), nullptr);
        }
    }
}

// VERIFY: 131148dd auto
// OFFSET: 0x00879400
void SESound::FadeInNow() {
    if (this->m_internal) {
        this->m_internal->m_fadingOut = 0;
        this->m_internal->m_fadingIn = 1;
    }
}

// VERIFY: 9978edc1 auto
// OFFSET: 0x00879410
void SESound::FadeOutNow() {
    if (this->m_internal) {
        this->m_internal->m_fadingIn = 0;
        this->m_internal->m_fadingOut = 1;
    }
}

// VERIFY: 9ecc861e manual
// OFFSET: 0x00879420
bool SESound::Is3D() {
    return this->m_internal && (this->m_internal->m_mode & FMOD_3D);
}

// VERIFY: 3738544c manual
// OFFSET: 0x00879440
bool SESound::IsLooping() {
    return this->m_internal && (this->m_internal->m_mode & FMOD_LOOP_NORMAL);
}

// VERIFY: 34a01b76 auto
// OFFSET: 0x00879710
void SESound::SetVolume(float volume) {
    if (!this->m_internal) {
        return;
    }

    float clamped = 0.0f;

    if (volume >= 0.0f) {
        clamped = volume;

        if (volume > 1.0f) {
            clamped = 1.0f;
        }
    }

    this->m_internal->m_volume = clamped;
    this->m_internal->UpdateVolume();
}

// VERIFY: 048d9e21 manual
// OFFSET: 0x00879840
void SESound::Stop(int32_t immediate, float fadeOutTime) {
    if (!this->m_internal) {
        return;
    }

    if (fadeOutTime >= 0.0f) {
        this->m_internal->m_fadeOutTime = fadeOutTime;
    }

    if (immediate || !(this->m_internal->m_fadeOutTime > 0.0f)) {
        this->m_internal->m_stop = 1;

        if (this->m_internal->m_channel) {
            this->m_internal->m_channel->stop();
            this->m_internal->m_channel = nullptr;
        }
    } else {
        this->FadeOutNow();
    }

    this->Detach();
}

// VERIFY: e48bd7ae auto
// OFFSET: 0x00879A60
void SESound::Play() {
    if (!this->m_internal) {
        return;
    }

    this->m_internal->UpdateVolume();
    this->m_internal->m_play = 1;

    if (this->m_internal->m_channel) {
        this->m_internal->Play();
    }
}

// VERIFY: 6a068ae9 auto
// OFFSET: 0x00879AA0
void SESound::AttachToObject(uint64_t guid) {
    this->m_internal->m_objectGUID = guid;

    s_soundListCritSect.Enter();
    s_objectList.LinkToTail(this->m_internal);
    s_soundListCritSect.Leave();
}

// VERIFY: e4e47b40 auto
// OFFSET: 0x0087A7E0
void SESound::SetChannelGroup(const char* name, bool childOfMaster) {
    if (s_initialized && this->m_internal && name) {
        this->m_internal->m_channelGroup = FindOrCreateChannelGroup(name, true, childOfMaster) - s_channelGroups.Ptr();
        this->m_internal->UpdateVolume();
    }
}

// VERIFY: 977f6a36 auto
// OFFSET: 0x0087BF00
void SESound::CompleteNonBlockingLoad() {
    if (this->m_internal && this->m_internal->m_type == SESOUNDTYPE_FILE && this->m_internal->m_loaded) {
        static_cast<SEFileSound*>(this->m_internal)->CompleteNonBlockingLoad();
    }
}

// VERIFY: 81b27631 manual
// OFFSET: 0x0087F7A0
int32_t SESound::Create2D(const char* filename, int32_t looping, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, bool streaming, int32_t reverb, int32_t priority, uint32_t decodeBufferSize) {
    if (!s_initialized) {
        return 0;
    }

    return SE2::CreateWaveData(s_system, filename, (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | FMOD_2D | FMOD_SOFTWARE, this, loadSoundGroup, soundGroup, randomStart, priority, reverb, streaming, decodeBufferSize, 0, 360.0f, 360.0f, 1.0f, nullptr);
}

// VERIFY: ea443bba manual
// OFFSET: 0x0087F810
int32_t SESound::Create3D(const char* filename, int32_t looping, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, bool streaming, int32_t priority, int32_t software, float coneInsideAngle, float coneOutsideAngle, float coneOutsideVolume, const C3Vector* coneOrientation) {
    if (!s_initialized) {
        return 0;
    }

    return SE2::CreateWaveData(s_system, filename, (looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF) | (software ? FMOD_SOFTWARE : FMOD_HARDWARE) | FMOD_3D, this, loadSoundGroup, soundGroup, randomStart, priority, 0, streaming, 0, software, coneInsideAngle, coneOutsideAngle, coneOutsideVolume, coneOrientation);
}

// VERIFY: cc377922 auto
// OFFSET: 0x0087F890
void SESound::PlaySoundFromMemory(const char* data, uint32_t length, int32_t chatSystem) {
    if (s_initialized) {
        SE2::PlaySoundFromMemory(chatSystem ? s_chatSystem : s_system, data, length, FMOD_LOOP_OFF | FMOD_2D | FMOD_SOFTWARE | FMOD_OPENMEMORY, this);
    }
}

// VERIFY: 32afecb8 auto
// OFFSET: 0x0087F8D0
void SESound::CreateChatSound(SEStreamCallbacks* callbacks, int32_t chatSystem) {
    if (SE2::sm_bCaptureInitialized) {
        SE2::CreateChatSound(chatSystem ? s_chatSystem : s_system, callbacks, FMOD_LOOP_OFF | FMOD_2D | FMOD_SOFTWARE, this);
    }
}

// VERIFY: 6b45523f manual
// OFFSET: 0x0087B580
SESound* SE2::GetSoundFromID(uint32_t id, int32_t* created) {
    *created = 0;

    SOUND_INTERNAL_LOOKUP* lookup = s_soundLookup.Ptr(id, s_lookupKey);

    if (!lookup) {
        return nullptr;
    }

    if (!lookup->m_sound->m_handle) {
        SESound* handle = new (SMemAlloc(sizeof(SESound), __FILE__, __LINE__, 0)) SESound();
        lookup->m_sound->m_handle = handle;
        handle->m_internal = lookup->m_sound;
        *created = 1;
    }

    return lookup->m_sound->m_handle;
}

// VERIFY: 13fd5be3 manual
// OFFSET: 0x00879AE0
int32_t SE2::GetChannelInfo(bool fadingOnly, SEChannelInfo* info, int32_t maxCount, int32_t playing, int32_t isVirtual, int32_t loading, int32_t other, uint32_t* hardwareCount, uint32_t* softwareCount, uint32_t* virtualCount, uint32_t* loadingCount, uint32_t* otherCount) {
    if (!info || maxCount <= 0) {
        return 0;
    }

    C3Vector listener(0.0f, 0.0f, 0.0f);

    *hardwareCount = 0;
    *softwareCount = 0;
    *virtualCount = 0;
    *loadingCount = 0;
    *otherCount = 0;

    int32_t count = 0;

    if (s_initialized) {
        s_system->get3DListenerAttributes(0, reinterpret_cast<FMOD_VECTOR*>(&listener), nullptr, nullptr, nullptr);
    }

    s_soundListCritSect.Enter();

    for (SESoundInternal* sound = s_soundList.Head(); sound; sound = s_soundList.Next(sound)) {
        memset(info, 0, sizeof(SEChannelInfo));

        info->m_looping = (sound->m_mode & FMOD_LOOP_NORMAL) != 0;
        info->m_userData = sound->m_userData;
        info->m_fadingIn = sound->m_fadingIn;
        info->m_fadingOut = sound->m_fadingOut;
        info->m_fadeVolume = sound->m_fadeVolume;
        info->m_is3D = (sound->m_mode & FMOD_3D) != 0;

        int32_t include;

        if (sound->m_channel) {
            bool isPlaying;
            FMOD_RESULT result = sound->m_channel->isPlaying(&isPlaying);
            SE_ERRCHECK(result);

            result = sound->m_channel->getAudibility(&info->m_audibility);
            SE_ERRCHECK(result);

            result = sound->m_channel->getFadeVolume(&info->m_channelFadeVolume);
            SE_ERRCHECK(result);

            if (info->m_is3D) {
                result = sound->m_channel->get3DPanLevel(&info->m_panLevel);
                SE_ERRCHECK(result);
            }

            if (isPlaying && info->m_is3D) {
                result = sound->m_channel->get3DAttributes(reinterpret_cast<FMOD_VECTOR*>(&info->m_position), nullptr);
                SE_ERRCHECK(result);

                float dz = listener.z - info->m_position.z;
                float dy = listener.y - info->m_position.y;
                float dx = listener.x - info->m_position.x;
                info->m_distanceSquared = dx * dx + dy * dy + dz * dz;

                result = sound->m_channel->get3DMinMaxDistance(&info->m_minDistance, &info->m_maxDistance);
                SE_ERRCHECK(result);
            } else {
                info->m_distanceSquared = 0.0f;
                info->m_minDistance = 0.0f;
                info->m_maxDistance = 0.0f;
            }

            bool virt;
            result = sound->m_channel->isVirtual(&virt);
            SE_ERRCHECK(result);

            result = sound->m_channel->getVolume(&info->m_volume);
            SE_ERRCHECK(result);

            result = sound->m_channel->getPriority(&info->m_priority);
            SE_ERRCHECK(result);

            FMOD_MODE mode;

            if (sound->m_channel->getMode(&mode) == FMOD_OK) {
                info->m_hardware = (mode & FMOD_HARDWARE) != 0;
            } else {
                info->m_hardware = 0;
            }

            info->m_cached = sound->m_cached;

            if (!isPlaying) {
                (*otherCount)++;
                info->m_state = SECHANNELSTATE_OTHER;
                include = other;
            } else if (virt) {
                (*virtualCount)++;
                info->m_state = SECHANNELSTATE_VIRTUAL;
                include = isVirtual;
            } else {
                (*(info->m_hardware ? hardwareCount : softwareCount))++;
                info->m_state = SECHANNELSTATE_PLAYING;
                include = playing;
            }
        } else {
            (*loadingCount)++;
            info->m_state = SECHANNELSTATE_LOADING;
            include = loading;
        }

        if (!include) {
            continue;
        }

        if (sound->m_channel && fadingOnly && !info->m_fadingIn && !info->m_fadingOut) {
            continue;
        }

        count++;
        info++;

        if (count == maxCount) {
            break;
        }
    }

    s_soundListCritSect.Leave();

    return count;
}

// VERIFY: 3893736c auto
// OFFSET: 0x00879F30
int32_t SE2::GetChannelInfo(SEChannelInfo* info, int32_t maxCount, int32_t playing, int32_t isVirtual, int32_t loading, int32_t other, uint32_t* hardwareCount, uint32_t* softwareCount, uint32_t* virtualCount, uint32_t* loadingCount, uint32_t* otherCount) {
    return SE2::GetChannelInfo(false, info, maxCount, playing, isVirtual, loading, other, hardwareCount, softwareCount, virtualCount, loadingCount, otherCount);
}

// VERIFY: 6bb8b6bc manual
// OFFSET: 0x00879F70
static void UpdateObjectPositions() {
    s_soundListCritSect.Enter();

    for (SESoundInternal* sound = s_objectList.Head(); sound; sound = s_objectList.Next(sound)) {
        C3Vector position;

        if (!s_positionCallback) {
            continue;
        }

        if (s_positionCallback(sound->m_objectGUID, &position)) {
            sound->m_position = position;

            if (sound->m_channel) {
                sound->m_channel->set3DAttributes(reinterpret_cast<FMOD_VECTOR*>(&position), nullptr);
            }
        } else {
            sound->m_objectLink.Unlink();
        }
    }

    s_soundListCritSect.Leave();
}

// VERIFY: c0cea4f0 manual
// OFFSET: 0x0087A170
static void StopChatSounds() {
    s_soundListCritSect.Enter();

    for (SESoundInternal* sound = s_deleteList.Head(); sound; sound = s_deleteList.Next(sound)) {
        if (sound->m_type == SESOUNDTYPE_CHAT) {
            static_cast<SEChatSound*>(sound)->m_dsp = nullptr;
            sound->m_deleteDelay = 0;
            sound->m_stop = 1;
            s_deleteList.LinkToTail(sound);
        }
    }

    s_soundListCritSect.Leave();
}

// VERIFY: aff3b6de auto
// OFFSET: 0x0087A210
static void ProcessDeleteList(int32_t force) {
    // STATIC: 0x00D43930
    static uint32_t s_lastTime = OsGetAsyncTimeMs();

    uint32_t now = OsGetAsyncTimeMs();
    int32_t elapsed = now - s_lastTime;

    s_soundListCritSect.Enter();

    SESoundInternal* sound = s_deleteList.Head();

    while (sound) {
        SESoundInternal* next = s_deleteList.Next(sound);

        sound->m_deleteDelay -= elapsed;

        if (force || (sound->m_deleteDelay <= 0 && sound->IsReadyForRelease())) {
            if (sound->m_channel) {
                bool isPlaying = false;
                sound->m_channel->isPlaying(&isPlaying);

                if (isPlaying) {
                    sound->m_channel->stop();
                }
            }

            sound->m_channel = nullptr;
            sound->~SESoundInternal();
            SMemFree(sound, typeid(SESoundInternal).raw_name(), SLOG_OBJECT, 0);
        }

        sound = next;
    }

    s_soundListCritSect.Leave();

    s_lastTime = now;
}

// VERIFY: 74b11876 manual
// OFFSET: 0x0087A2F0
static void ProcessFadeList() {
    if (!s_initialized) {
        return;
    }

    // STATIC: 0x00D43938
    static uint32_t s_lastTime = OsGetAsyncTimeMs();

    uint32_t now = OsGetAsyncTimeMs();
    int32_t elapsed = now - s_lastTime;
    float seconds = elapsed * 0.001f;

    if (elapsed < 0) {
        s_lastTime = now;
        return;
    }

    uint32_t groupCount = s_channelGroups.Count();
    uint8_t* dirty = static_cast<uint8_t*>(alloca(groupCount));
    memset(dirty, 0, groupCount);

    for (uint32_t i = 0; i < s_channelGroups.Count(); i++) {
        if (s_channelGroups[i].m_dirty) {
            s_channelGroups[i].m_dirty = 0;
            dirty[i] = 1;
        }
    }

    bool changed;

    do {
        changed = false;

        for (uint32_t i = 0; i < s_channelGroups.Count(); i++) {
            if (!dirty[i] && dirty[s_channelGroups[i].m_parent]) {
                dirty[i] = 1;
                changed = true;
            }
        }
    } while (changed);

    s_soundListCritSect.Enter();

    SESoundInternal* sound = s_soundList.Head();

    while (sound) {
        SESoundInternal* next = s_soundList.Next(sound);

        sound->UpdateVolume();

        if (sound->m_fadingOut) {
            if (sound->m_fadeOutTime <= 0.0f || (sound->m_fadeVolume -= seconds / sound->m_fadeOutTime) < 0.0f) {
                sound->m_fadeVolume = 0.0f;
            }

            if (sound->m_fadeVolume == 0.0f) {
                SESound handle;

                if (!sound->m_handle) {
                    sound->m_handle = &handle;
                    handle.m_internal = sound;
                }

                sound->m_fadingOut = 0;

                if (sound->m_fadeOutCallback) {
                    sound->m_fadeOutCallback(sound->m_handle);
                }

                if (sound->m_handle) {
                    sound->m_handle->Stop(1, -1.0f);
                }
            }
        } else if (sound->m_fadingIn) {
            sound->m_fadeVolume += seconds / sound->m_fadeInTime;

            if (sound->m_fadeVolume > 1.0f) {
                sound->m_fadeVolume = 1.0f;
            }

            sound->UpdateVolume();

            if (sound->m_fadeVolume == 1.0f) {
                sound->m_fadingIn = 0;
            }
        } else if (dirty[sound->m_channelGroup]) {
            sound->UpdateVolume();
        }

        sound = next;
    }

    s_soundListCritSect.Leave();

    s_lastTime = now;
}

// VERIFY: f1ebb685 manual
// OFFSET: 0x0087A580
static void KillVirtualSounds() {
    uint32_t now = OsGetAsyncTimeMs();

    s_soundListCritSect.Enter();

    SESoundInternal* sound = s_soundList.Head();

    while (sound) {
        SESoundInternal* next = s_soundList.Next(sound);

        if (sound->m_type == SESOUNDTYPE_FILE) {
            SEFileSound* fileSound = static_cast<SEFileSound*>(sound);

            if (fileSound->m_loadCompleted && fileSound->m_channel) {
                bool isVirtual = false;
                fileSound->m_channel->isVirtual(&isVirtual);

                if (fileSound->m_priority == 128 || fileSound->m_priority == -1) {
                    bool isPlaying = false;
                    fileSound->m_channel->isPlaying(&isPlaying);

                    if (isPlaying && !isVirtual) {
                        FMOD_MODE mode;
                        fileSound->m_channel->getMode(&mode);

                        if (!(mode & FMOD_LOOP_NORMAL)) {
                            fileSound->m_priority = 127;
                            fileSound->m_channel->setPriority(127);
                        }
                    }
                }

                // STATIC: 0x00D43940
                static CVar* s_virtualPlayWindow = CVar::Lookup("Sound_VirtualPlayWindow");

                if (!fileSound->m_streaming) {
                    uint32_t window = s_virtualPlayWindow ? s_virtualPlayWindow->m_intValue : 400;

                    if (now - fileSound->m_playTime > window) {
                        if (fileSound->m_loadSoundGroup && fileSound->m_sound) {
                            fileSound->m_sound->setSoundGroup(fileSound->m_soundGroup);
                            fileSound->m_loadSoundGroup = nullptr;
                        }

                        if (isVirtual) {
                            fileSound->m_channel->stop();
                            fileSound->m_channel = nullptr;
                        }
                    }
                }
            }
        }

        sound = next;
    }

    s_soundListCritSect.Leave();
}

// VERIFY: bd8b4ce2 manual
// OFFSET: 0x0087B010
static void ProcessWaveDataDeleteList(int32_t force) {
    // STATIC: 0x00D43948
    static CVar* s_maxCacheSize;

    int32_t maxCacheSize = 0x1000000;

    if (s_maxCacheSize || (s_maxCacheSize = CVar::Lookup("Sound_MaxCacheSizeInBytes")) != nullptr) {
        maxCacheSize = s_maxCacheSize->m_intValue;

        if (maxCacheSize < 0x400000) {
            maxCacheSize = 0x400000;
        } else if (maxCacheSize > 100 * 1024 * 1024) {
            maxCacheSize = 0x8000000;
        }
    }

    if (s_cacheBytes <= maxCacheSize && !force) {
        return;
    }

    SEWaveDataNode* node = s_waveDataList.Head();

    while (node) {
        SEWaveDataNode* next = s_waveDataList.Next(node);

        if (!node->m_refCount) {
            if (s_cacheBytes <= maxCacheSize && !force) {
                return;
            }

            node->m_sound->release();
            s_cacheCount--;
            s_cacheBytes -= node->m_size;
            node->m_sound = reinterpret_cast<FMOD::Sound*>(0xBEEFEE);
            delete node;
        }

        node = next;
    }
}

// VERIFY: ddc4fdc8 manual
// OFFSET: 0x0087BF20
static void ProcessGoGoGoList() {
    do {
        s_soundListCritSect.Enter();

        SESoundInternal* sound = s_goList.Head();

        if (sound) {
            s_goList.UnlinkNode(sound);
        }

        s_soundListCritSect.Leave();

        if (sound) {
            static_cast<SEFileSound*>(sound)->CompleteNonBlockingLoad();
        }
    } while (s_goList.Head());
}

// VERIFY: 1311c084 auto
// OFFSET: 0x0087C4B0
int32_t SE2::HeartBeat(const void* data, void* param) {
    if (!s_initialized) {
        s_system->update();
        return 1;
    }

    s_heartBeatCritSect.Enter();

    ProcessDeleteList(0);
    ProcessWaveDataDeleteList(0);
    ProcessFadeList();
    UpdateObjectPositions();
    ProcessGoGoGoList();
    g_soundCache.Update();

    s_system->update();

    if (SE2::sm_bCaptureInitialized && s_chatSystem != s_system) {
        s_chatSystem->update();
    }

    KillVirtualSounds();

    s_heartBeatCritSect.Leave();

    return 1;
}

// VERIFY: 72a25361 auto
// OFFSET: 0x0087E010
void SE2::UnregisterHeartBeat() {
    EventUnregister(EVENT_ID_POLL, SE2::HeartBeat);
}

// VERIFY: 1035981c manual
// OFFSET: 0x0087DED0
void SE2::StopAllSounds(int32_t restart) {
    int32_t tries = 0;

    while (true) {
        s_soundListCritSect.Enter();

        for (SESoundInternal* sound = s_soundList.Head(); sound; sound = s_soundList.Next(sound)) {
            if (sound->m_type != SESOUNDTYPE_CHAT) {
                sound->m_stop = 1;

                if (sound->m_channel) {
                    sound->m_channel->stop();
                }
            }
        }

        s_soundListCritSect.Leave();

        SE2::HeartBeat(nullptr, nullptr);
        ProcessDeleteList(1);

        int32_t remaining = 0;

        s_soundListCritSect.Enter();

        for (SESoundInternal* sound = s_soundList.Head(); sound; sound = s_soundList.Next(sound)) {
            if (sound->m_type != SESOUNDTYPE_CHAT) {
                remaining = 1;
                break;
            }
        }

        s_soundListCritSect.Leave();

        OsSleep(100);
        tries++;

        if (!remaining) {
            break;
        }

        if (tries >= 20) {
            TSGrowableArray<FMOD::Sound*> pendingRelease;

            s_soundListCritSect.Enter();

            SESoundInternal* sound = s_soundList.Head();

            while (sound) {
                if (sound->m_type == SESOUNDTYPE_CHAT) {
                    sound = s_soundList.Next(sound);
                } else {
                    SESoundInternal* next = s_soundList.Next(sound);

                    if (sound->m_type == SESOUNDTYPE_FILE) {
                        SEFileSound* fileSound = static_cast<SEFileSound*>(sound);

                        if (!fileSound->m_waveData && fileSound->m_sound) {
                            *pendingRelease.New() = fileSound->m_sound;
                            fileSound->m_sound = nullptr;
                        }
                    } else if (sound->m_type == SESOUNDTYPE_MEMORY) {
                        SEMemorySound* memorySound = static_cast<SEMemorySound*>(sound);

                        if (memorySound->m_sound) {
                            *pendingRelease.New() = memorySound->m_sound;
                            memorySound->m_sound = nullptr;
                        }
                    }

                    sound->~SESoundInternal();
                    SMemFree(sound, typeid(SESoundInternal).raw_name(), SLOG_OBJECT, 0);
                    sound = next;
                }
            }

            s_soundListCritSect.Leave();

            for (uint32_t i = 0; i < pendingRelease.Count(); i++) {
                pendingRelease[i]->release();
            }

            break;
        }
    }

    SE2::HeartBeat(nullptr, nullptr);
}

// VERIFY: 675bfdff manual
// OFFSET: 0x0087EE60
int32_t SE2::CreateWaveData(FMOD::System* system, const char* filename, FMOD_MODE mode, SESound* sound, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, int32_t priority, int32_t reverb, bool streaming, uint32_t decodeBufferSize, int32_t software, float coneInsideAngle, float coneOutsideAngle, float coneOutsideVolume, const C3Vector* coneOrientation) {
    s_createCritSect.Enter();

    SEWaveDataNode* waveData = nullptr;

    if (!sound || !system || !filename || !*filename) {
        s_createCritSect.Leave();
        return 0;
    }

    sound->Stop(1, -1.0f);

    SEFileSound* internal = new (SMemAlloc(sizeof(SEFileSound), __FILE__, __LINE__, 0)) SEFileSound();

    internal->m_coneInsideAngle = coneInsideAngle;
    internal->m_coneOutsideAngle = coneOutsideAngle;
    internal->m_priority = priority;
    internal->m_coneOutsideVolume = coneOutsideVolume;
    internal->m_system = system;
    internal->m_mode = mode;
    internal->m_type = SESOUNDTYPE_FILE;
    internal->m_reverb = reverb;
    internal->m_streaming = streaming;
    internal->m_cached = 0;
    internal->m_channel = nullptr;

    if (coneOrientation) {
        internal->m_coneOrientation = *coneOrientation;
    }

    internal->m_handle = sound;
    sound->m_internal = internal;

    FMOD_CREATESOUNDEXINFO exinfo;
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = 0;
    exinfo.fileoffset = 0;
    exinfo.numchannels = 0;
    exinfo.defaultfrequency = 0;
    exinfo.format = FMOD_SOUND_FORMAT_NONE;
    exinfo.decodebuffersize = decodeBufferSize;
    exinfo.initialsubsound = 0;
    exinfo.numsubsounds = 0;
    exinfo.inclusionlist = nullptr;
    exinfo.inclusionlistnum = 0;
    exinfo.pcmreadcallback = nullptr;
    exinfo.pcmsetposcallback = nullptr;
    exinfo.nonblockcallback = SENonBlockCallback;
    exinfo.dlsname = nullptr;
    exinfo.encryptionkey = nullptr;
    exinfo.maxpolyphony = 0;
    exinfo.userdata = reinterpret_cast<void*>(static_cast<uintptr_t>(internal->m_id));
    exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_UNKNOWN;
    exinfo.useropen = SEOpenCallback;
    exinfo.userclose = SECloseCallback;
    exinfo.userread = SEReadCallback;
    exinfo.userseek = SESeekCallback;
    exinfo.speakermap = FMOD_SPEAKERMAPTYPE_DEFAULT;
    exinfo.initialsoundgroup = reinterpret_cast<FMOD_SOUNDGROUP*>(streaming ? soundGroup : loadSoundGroup);
    exinfo.initialseekposition = 0;
    exinfo.initialseekpostype = 0;

    bool cacheable = true;
    const char* extension = SStrChrR(const_cast<char*>(filename), '.');

    if (extension) {
        if (!SStrCmpI(extension, ".wav", STORM_MAX_STR)) {
            exinfo.suggestedsoundtype = SFile::IsTrial() ? FMOD_SOUND_TYPE_UNKNOWN : FMOD_SOUND_TYPE_WAV;
        } else if (!SStrCmpI(extension, ".mp3", STORM_MAX_STR)) {
            mode |= FMOD_MPEGSEARCH;
            cacheable = false;
            exinfo.suggestedsoundtype = FMOD_SOUND_TYPE_MPEG;
        }
    }

    char name[300];
    SStrPrintf(name, sizeof(name), "%-24d%s", internal->m_id, filename);

    char key[400];
    SStrPrintf(key, sizeof(key), "%s%s%s", filename, (mode & FMOD_2D) ? "_2D_" : "_3D_", (mode & FMOD_LOOP_NORMAL) ? "_LOOP_" : "_NOLOOP_");

    internal->m_randomStart = randomStart;

    uint32_t hash = SStrHash(key, 0, 0);
    int32_t loaded;

    if (cacheable) {
        for (SEWaveDataNode* node = s_waveDataList.Head(); node; node = s_waveDataList.Next(node)) {
            if (hash == node->m_hash) {
                internal->m_sound = node->m_sound;
                internal->m_cached = 1;
                internal->m_waveData = node;
                node->m_refCount++;

                loaded = 0;

                if (node->m_loaded) {
                    s_cacheHitsLoaded++;
                    loaded = 1;
                } else {
                    s_cacheHitsLoading++;
                }

                goto done;
            }
        }
    }

    if (!SFile::OpenEx(nullptr, filename, 1, &internal->m_file)) {
        SE2::Log_Write(__LINE__, __FILE__, FMOD_ERR_FILE_NOTFOUND, filename);
        s_soundListCritSect.Enter();
        goto fail;
    }

    {
        FMOD_MODE createMode = mode | FMOD_VIRTUAL_PLAYFROMSTART | FMOD_IGNORETAGS | FMOD_NONBLOCKING;
        loaded = 0;

        // STATIC: 0x00D43984
        static CVar* s_maxCacheableSize;

        uint32_t maxCacheableSize = 0x100000;

        if (s_maxCacheableSize || (s_maxCacheableSize = CVar::Lookup("Sound_MaxCacheableSizeInBytes")) != nullptr) {
            maxCacheableSize = s_maxCacheableSize->m_intValue;

            if (maxCacheableSize > 0x200000) {
                maxCacheableSize = 0x200000;
            }
        }

        uint32_t fileSize = 0;
        FMOD_RESULT result;

        if ((internal->m_file && (fileSize = SFileGetFileSize(internal->m_file, nullptr)) > maxCacheableSize) || !cacheable) {
            cacheable = false;
            result = system->createStream(name, createMode, &exinfo, &internal->m_sound);
            s_streamsCreated++;
        } else {
            result = system->createSound(name, createMode, &exinfo, &internal->m_sound);
            s_soundsCreated++;
        }

        if (result != FMOD_OK) {
            if (result != FMOD_ERR_OUTPUT_CREATEBUFFER) {
                SE2::Log_Write(__LINE__, __FILE__, result, filename);
            }

            s_soundListCritSect.Enter();
            SFileCloseFile(internal->m_file);
            internal->m_file = nullptr;
            goto fail;
        }

        SE_ERRCHECK(result);

        if (cacheable) {
            waveData = new (SMemAlloc(sizeof(SEWaveDataNode), __FILE__, __LINE__, 0)) SEWaveDataNode();
            waveData->m_sound = internal->m_sound;
            waveData->m_hash = hash;
            SStrCopy(waveData->m_name, filename, sizeof(waveData->m_name));

            internal->m_cached = 1;
            waveData->m_size = fileSize;

            s_cacheBytes += fileSize;

            if (s_cacheBytes > s_cacheBytesPeak) {
                s_cacheBytesPeak = s_cacheBytes;
            }

            s_cacheCount++;

            if (s_cacheCount > s_cacheCountPeak) {
                s_cacheCountPeak = s_cacheCount;
            }

            internal->m_waveData = waveData;
            waveData->m_refCount = 1;
        }
    }

done:
    internal->m_soundGroup = soundGroup;
    internal->m_loadSoundGroup = streaming ? nullptr : loadSoundGroup;
    internal->m_software = software;
    internal->m_loaded = loaded;

    s_createCritSect.Leave();

    return 1;

fail:
    internal->m_deleteDelay = 0;
    s_deleteList.LinkToTail(internal);
    s_soundListCritSect.Leave();

    s_createCritSect.Leave();

    return 0;
}

// VERIFY: 18ad04a7 manual
// OFFSET: 0x0087F380
void SE2::PlaySoundFromMemory(FMOD::System* system, const char* data, uint32_t length, FMOD_MODE mode, SESound* sound) {
    if (!sound || !system) {
        return;
    }

    sound->Stop(1, -1.0f);

    SEMemorySound* internal = new (SMemAlloc(sizeof(SEMemorySound), __FILE__, __LINE__, 0)) SEMemorySound();

    internal->m_mode = mode;
    internal->m_system = system;
    internal->m_type = SESOUNDTYPE_MEMORY;

    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(exinfo));
    exinfo.cbsize = sizeof(exinfo);
    exinfo.length = length;
    exinfo.userdata = internal;

    FMOD_RESULT result = system->createStream(data, mode, &exinfo, &internal->m_sound);

    if (result != FMOD_OK) {
        SE_ERRCHECK(result);

        s_soundListCritSect.Enter();
        internal->m_deleteDelay = 0;
        s_deleteList.LinkToTail(internal);
        s_soundListCritSect.Leave();

        s_createCritSect.Leave();

        return;
    }

    result = system->playSound(FMOD_CHANNEL_FREE, internal->m_sound, true, &internal->m_channel);
    SE_ERRCHECK(result);

    result = internal->m_channel->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(internal->m_id)));
    SE_ERRCHECK(result);

    result = internal->m_channel->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, SoundEndedCallback, 0);

    if (result != FMOD_OK) {
        SE_ERRCHECK(result);

        internal->m_sound->release();

        s_soundListCritSect.Enter();
        internal->m_deleteDelay = 0;
        s_deleteList.LinkToTail(internal);
        s_soundListCritSect.Leave();

        return;
    }

    internal->m_channel->getFrequency(&internal->m_frequency);

    internal->m_handle = sound;
    sound->m_internal = internal;
}

// VERIFY: 9b2127c2 manual
// OFFSET: 0x0087F5E0
void SE2::CreateChatSound(FMOD::System* system, SEStreamCallbacks* callbacks, FMOD_MODE mode, SESound* sound) {
    if (!SE2::sm_bCaptureInitialized || !sound || !system || !callbacks) {
        return;
    }

    sound->Stop(1, -1.0f);

    SEChatSound* internal = new (SMemAlloc(sizeof(SEChatSound), __FILE__, __LINE__, 0)) SEChatSound();

    internal->m_callbacks = callbacks;
    internal->m_mode = mode;
    internal->m_system = system;
    internal->m_type = SESOUNDTYPE_CHAT;
    internal->m_handle = sound;
    sound->m_internal = internal;

    FMOD_DSP_DESCRIPTION description = { "SEStreamedSound", 1000, 1, nullptr, nullptr, nullptr, SEChatSound::DSPReadCallback, SEChatSound::DSPSeekCallback, 0, nullptr, nullptr, nullptr, nullptr, 0, 0, internal };

    FMOD_RESULT result = system->createDSP(&description, &internal->m_dsp);
    SE_ERRCHECK(result);

    result = system->playDSP(FMOD_CHANNEL_FREE, internal->m_dsp, true, &internal->m_channel);
    SE_ERRCHECK(result);

    if (internal->m_channel) {
        result = internal->m_channel->setCallback(FMOD_CHANNEL_CALLBACKTYPE_END, SoundEndedCallback, 0);
        SE_ERRCHECK(result);

        result = internal->m_channel->setUserData(reinterpret_cast<void*>(static_cast<uintptr_t>(internal->m_id)));
        SE_ERRCHECK(result);
    }

    internal->m_frequency = 1.0f;
    internal->m_channel->setFrequency(1.0f);
    internal->m_deleteDelay = 0;
}

// VERIFY: 5f70d806 manual
// OFFSET: 0x0087C710
int32_t SE2::Init(int32_t maxChannels, bool (*positionCallback)(uint64_t, C3Vector*), int32_t enableReverb, int32_t enableSoftwareHRTF, int32_t* numChannels, int32_t* outputDriverIndex, const char* outputDriverName, float (*volumeCallback)(SESoundUserData*), int32_t restart) {
    FMOD_INITFLAGS flags = FMOD_INIT_NORMAL;
    int32_t success = 1;

    s_gameOutputDeviceName[0] = '\0';
    s_chatOutputDeviceName[0] = '\0';
    s_chatInputDeviceName[0] = '\0';

    s_cacheBytes = 0;
    s_cacheBytesPeak = 0;
    s_cacheCount = 0;
    s_cacheCountPeak = 0;
    s_unused_unknownStat = 0;
    s_cacheHitsLoaded = 0;
    s_cacheHitsLoading = 0;
    s_soundsCreated = 0;
    s_streamsCreated = 0;
    s_initialized = 0;

    OsCreateDirectory("Logs", 0);
    SLogCreate("Logs\\SESound.log", s_logFlags, &s_log);
    s_logFlags |= SLOG_FLAG_APPEND;
    OsDeleteFile("Logs\\Sound.log");

    if (!restart) {
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Version %s (%s) %s", GAME_VERSION, GAME_BUILD_STRING, GAME_BUILD_DATE);
    }

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " ");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Setting up Game Sound:");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - SESound Engine Init");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - FMOD Memory Init");

    FMOD_Memory_Initialize(nullptr, 0, FSoundAllocCallback, FSoundReallocCallback, FSoundFreeCallback);

    g_soundCache.Initialize();

    s_system = nullptr;

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - FMOD System Create");

    FMOD_RESULT result = FMOD::System_Create(&s_system);

    if (result != FMOD_OK) {
        SE_ERRCHECK(result);
        SE_LOG_INIT_FAILED(result);
        goto disable;
    }

    result = FMOD::System_Create(&s_enumSystem);
    SE_ERRCHECK(result);

    if (s_enumSystem && s_enumSystem->init(4, FMOD_INIT_NORMAL, nullptr) != FMOD_OK) {
        s_enumSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
    }

    s_system->setOutput(FMOD_OUTPUTTYPE_AUTODETECT);

    EventRegister(EVENT_ID_POLL, SE2::HeartBeat);

    {
        uint32_t version;
        result = s_system->getVersion(&version);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_INIT_FAILED(result);
            goto disable;
        }

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - FMOD version %08x detected", version);

        if (version != FMOD_VERSION) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -######## ERROR! FMOD version mismatch! This program requires version %08x", FMOD_VERSION);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE_LOG_INIT_FAILED(FMOD_ERR_VERSION);
            goto disable;
        }
    }

    {
        // STATIC: 0x00D43964
        static CVar* s_enableMode2 = CVar::Lookup("Sound_EnableMode2");
        // STATIC: 0x00D43960
        static CVar* s_enableHardware = CVar::Lookup("Sound_EnableHardware");
        // STATIC: 0x00D4395C
        static CVar* s_enableOpenAL = CVar::Lookup("Sound_EnableOpenAL");

        int32_t hardware = s_enableHardware->m_intValue;
        int32_t mode2 = s_enableMode2->m_intValue;
        int32_t openAL = s_enableOpenAL ? s_enableOpenAL->m_intValue : 0;

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Hardware is %s", hardware ? "ENABLED" : "DISABLED");

        if (openAL) {
            if (hardware) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - OpenAL is ENABLED");
                SE_ERRCHECK(s_system->setOutput(FMOD_OUTPUTTYPE_OPENAL));
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# ERROR! OpenAL Mode requires Hardware to be enabled! NOT using OpenAL.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            }
        }

        s_reverbEnabled = enableReverb;
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Reverb is %s", enableReverb ? "ENABLED" : "DISABLED");

        int32_t numDrivers = SE2::GetNumOutputDrivers(0);
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - %d Output drivers detected", numDrivers);
        s_outputDrivers.SetCount(numDrivers);

        if (!numDrivers) {
            SE_LOG_INIT_FAILED(FMOD_ERR_OUTPUT_INIT);
            goto disable;
        }

        for (int32_t i = 0; i < numDrivers; i++) {
            char driverName[256];
            SE2::GetOutputDriverName(i, driverName, sizeof(driverName), 0);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " --- [%d] %s", i, driverName);
            SStrCopy(s_outputDrivers[i].m_name, driverName, sizeof(SEDriverInfo));
        }

        *outputDriverIndex = FindDriverIndex(outputDriverName, *outputDriverIndex, 1, 0);

        int32_t driver = *outputDriverIndex ? *outputDriverIndex - 1 : 0;

        s_system->getDriverName(driver, s_gameOutputDeviceName, sizeof(s_gameOutputDeviceName));
        result = s_system->setDriver(driver);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_INIT_FAILED(result);
            s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
            success = 0;
            goto failed;
        }

        LogUsingDriver(*outputDriverIndex, "Game Output", 1, 0);

        if (*numChannels > 128) {
            *numChannels = 128;
        } else if (*numChannels < 12) {
            *numChannels = 12;
        }

        if (hardware) {
            if (mode2) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Sound Engine Mode 2 DISABLED because Hardware is enabled.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                mode2 = 0;
            }
        } else if (mode2) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Using Sound Engine Mode 2.");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        } else {
            goto software;
        }

        {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - %d Channels Requested.", *numChannels);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Querying Sound Driver for Available Hardware Channels.", *numChannels);

            result = s_system->init(256, FMOD_INIT_3D_RIGHTHANDED, nullptr);

            if (result != FMOD_OK) {
                SE_ERRCHECK(result);
                SE_LOG_INIT_FAILED(result);
                goto nosound;
            }

            int32_t numHardwareChannels = 0;
            int32_t num3DHardwareChannels = 0;
            result = s_system->getHardwareChannels(nullptr, &numHardwareChannels, &num3DHardwareChannels);
            SE_ERRCHECK(result);

            if (result != FMOD_OK) {
                SE_LOG_INIT_FAILED(result);
                goto nosound;
            }

            s_system->close();

            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Sound Driver Reports: %d Hardware Channels Available.", numHardwareChannels);

            if (numHardwareChannels >= 12) {
                if (numHardwareChannels < *numChannels) {
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Too many channels requested. Using max available instead");
                    *numChannels = numHardwareChannels;
                }

                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Using %d channels.", *numChannels);

                result = s_system->setHardwareChannels(0, 0, *numChannels, *numChannels);
                SE_ERRCHECK(result);

                if (result != FMOD_OK) {
                    SE_LOG_INIT_FAILED(result);
                    goto nosound;
                }

                result = s_system->setSoftwareChannels(16);
                SE_ERRCHECK(result);

                if (result != FMOD_OK) {
                    SE_LOG_INIT_FAILED(result);
                    goto nosound;
                }

                if (hardware || mode2) {
                    goto speakers;
                }
            } else if (mode2) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Mode 2 Support Not Detected. Reverting to Default Engine Mode.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                s_enableMode2->Set("0", true, false, false, true);
                mode2 = 0;

                if (hardware) {
                    goto speakers;
                }
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Insufficient Available Hardware Channels.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           World of Warcraft Requires At Least 12 Hardware Channels to Function Properly in Hardware Mode.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Enabling Software Channels.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            }
        }

    software:
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Using %d channels.", *numChannels);

        if (*numChannels > 64) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   You are using a very high number of channels.");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           This may lead to poor sound performance.");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        }

        result = s_system->setHardwareChannels(0, 0, 0, 0);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

        result = s_system->setSoftwareChannels(*numChannels);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

    speakers:
        if (enableSoftwareHRTF) {
            flags = FMOD_INIT_SOFTWARE_HRTF;
        }

        const char* speakerModes[] = { "RAW", "MONO", "STEREO", "QUAD", "SURROUND", "5.1", "7.1", "PROLOGIC" };

        FMOD_CAPS caps;
        FMOD_SPEAKERMODE speakerMode;
        result = s_system->getDriverCaps(driver, &caps, nullptr, nullptr, &speakerMode);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

        // STATIC: 0x00D43958
        static CVar* s_dspBufferSize = CVar::Lookup("Sound_DSPBufferSize");

        if (s_dspBufferSize) {
            int32_t numBuffers = s_dspBufferSize->m_intValue;

            if (numBuffers) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - DSPBufferSize = %d [Valid values are 0 = AUTO DETECT (default), 4 through 10]", s_dspBufferSize->m_intValue);
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - DSPBufferSize = AUTO DETECT");
            }

            if (caps & FMOD_CAPS_HARDWARE_EMULATED) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Your Sound Driver is running in EMULATED mode.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           This may lead to poor sound performance.");

                if (!numBuffers) {
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Using Max DSP Buffer size (10) to avoid stuttering.");
                    numBuffers = 10;
                }

                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Also in EMULATED mode some speaker configurations like 5.1 may not be available.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            }

            if (numBuffers) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   You are using a custom DSP Buffer size");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Higher settings may reduce sound stuttering and crackling,");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           but may affect sound timing.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Do not change this setting from default unless you really need to.");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");

                if (numBuffers < 4 || numBuffers > 10) {
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:    DSPBufferSize out of range!! Valid range is 4 to 10, or 0 for AUTO DETECT");
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#            Using default DSPBufferSize");
                    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                    s_dspBufferSize->Set("4", true, false, false, true);
                    numBuffers = 4;
                }
            } else {
                numBuffers = 4;
            }

            result = s_system->setDSPBufferSize(1024, numBuffers);
            SE_ERRCHECK(result);

            if (result != FMOD_OK) {
                SE_LOG_INIT_FAILED(result);
                s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
                success = 0;
                goto failed;
            }
        }

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Detected Control Panel Speaker Mode = %s", speakerModes[speakerMode]);
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Setting WoW Speaker Mode to %s", speakerModes[speakerMode]);

        result = s_system->setSpeakerMode(speakerMode);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

        int32_t sampleRate = 48000;

        // STATIC: 0x00D43954
        static CVar* s_outputQuality = CVar::Lookup("Sound_OutputQuality");

        if (s_outputQuality) {
            if (s_outputQuality->m_intValue == 0) {
                sampleRate = 22050;
            } else if (s_outputQuality->m_intValue == 1) {
                sampleRate = 44100;
            }
        }

        FMOD_DSP_RESAMPLER resampler = FMOD_DSP_RESAMPLER_LINEAR;

        if (s_outputQuality) {
            resampler = static_cast<FMOD_DSP_RESAMPLER>(s_outputQuality->m_intValue == 2);
        }

        // STATIC: 0x00D43950
        static CVar* s_enableMixMode2 = CVar::Lookup("Sound_EnableMixMode2");

        if (s_enableMixMode2 && s_enableMixMode2->m_intValue) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Using Mix Mode 2.");

            if (s_enableMixMode2->m_intValue == 2) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 2 PCM24");
                SE_ERRCHECK(s_system->setSoftwareFormat(sampleRate, FMOD_SOUND_FORMAT_PCM24, 0, 0, resampler));
            } else if (s_enableMixMode2->m_intValue == 3) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 3 PCM32");
                SE_ERRCHECK(s_system->setSoftwareFormat(sampleRate, FMOD_SOUND_FORMAT_PCM32, 0, 0, resampler));
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 1 PCMFLOAT");
                SE_ERRCHECK(s_system->setSoftwareFormat(sampleRate, FMOD_SOUND_FORMAT_PCMFLOAT, 0, 0, resampler));
            }

            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        }

        result = s_system->init(maxChannels, flags | FMOD_INIT_3D_RIGHTHANDED, nullptr);

        if (result == FMOD_ERR_OUTPUT_CREATEBUFFER) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   FMOD System Init failed!");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Control Panel Speaker Mode %s FAILED to initialize!", speakerModes[speakerMode]);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Reverting to STEREO");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           This most commonly happens when SURROUND is selected.");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           SURROUND is actually 5.0 (no subwoofer).");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Many sound drivers do not support SURROUND even though you can select it in the control panel");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Some speaker modes will fail to initialize if the sound driver is running in EMULATED mode.");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");

            SE_ERRCHECK(s_system->close());

            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - Setting WoW Speaker Mode to STEREO");

            result = s_system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO);
            SE_ERRCHECK(result);

            if (result != FMOD_OK) {
                SE_LOG_INIT_FAILED(result);
                s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
                success = 0;
                goto failed;
            }

            result = s_system->init(maxChannels, flags | FMOD_INIT_3D_RIGHTHANDED, nullptr);
            SE_ERRCHECK(result);

            if (result != FMOD_OK) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   FMOD System Init failed!");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           Reverting to most compatible configuration (Hardware DISABLED,STEREO,MixMode2 DISABLED,Control Panel default output device)");
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");

                s_system->getDriverName(0, s_gameOutputDeviceName, sizeof(s_gameOutputDeviceName));
                SE_ERRCHECK(s_system->setDriver(0));
                SE_ERRCHECK(s_system->setSpeakerMode(FMOD_SPEAKERMODE_STEREO));
                SE_ERRCHECK(s_system->setSoftwareFormat(sampleRate, FMOD_SOUND_FORMAT_PCM16, 0, 0, resampler));
                SE_ERRCHECK(s_system->setHardwareChannels(0, 0, 0, 0));
                SE_ERRCHECK(s_system->setSoftwareChannels(*numChannels));

                result = s_system->init(maxChannels, flags | FMOD_INIT_3D_RIGHTHANDED, nullptr);
            }
        }

        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

        s_positionCallback = positionCallback;
        s_volumeCallback = volumeCallback;

        result = s_system->set3DSettings(1.0f, 1.0f, 4.0f);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
            success = 0;
            goto failed;
        }

        result = s_system->set3DRolloffCallback(RolloffCallback);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_INIT_FAILED(result);
            goto nosound;
        }

        if (!s_channelGroups.Count()) {
            s_channelGroups.Clear();

            SEChannelGroup* master = s_channelGroups.New();
            uint32_t hash = SStrHashHT("<master>");
            master->m_volume = 1.0f;
            master->m_hash = hash;
            master->m_muteVolume = 1.0f;
            master->m_parent = -1;
            master->m_dirty = 0;
        }

        FMOD_Debug_SetLevel(FMOD_DEBUG_LEVEL_NONE);

        s_initialized = 1;

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - FMOD System Init OK!");
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Game Sound is ready.");
        goto done;
    }

nosound:
    s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
    success = 0;
    goto failed;

disable:
    s_system->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
    success = 0;

failed:
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Game Sound Init Failed.");

done:
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " ");

    if (!success) {
        s_system->init(32, flags | FMOD_INIT_3D_RIGHTHANDED, nullptr);
    }

    return success;
}

// VERIFY: c81668c2 manual
// OFFSET: 0x0087B490
void SE2::ShutDown(int32_t restart) {
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " ");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Shutting Down Game Sound");

    if (!restart) {
        s_channelGroups.Clear();
    }

    if (s_system) {
        ProcessWaveDataDeleteList(1);

        s_system->close();
        s_system->release();
        s_system = nullptr;

        if (s_enumSystem) {
            s_enumSystem->close();
            s_enumSystem->release();
            s_enumSystem = nullptr;
        }
    }

    g_soundCache.Clear();

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Done Shutting Down Game Sound");

    SLogFlush(s_log);
    SLogClose(s_log);
}

// VERIFY: 11e89f39 manual
// OFFSET: 0x0087E020
void SE2::InitCapture(int32_t* outputDriverIndex, const char* outputDriverName, int32_t* inputDriverIndex, const char* inputDriverName, float sampleRate) {
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " ");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Setting up Voice Chat:");

    if (!s_initialized) {
        return;
    }

    if (!(OsGetProcessorFeatures() & 4)) {
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# ERROR! SSE support is required for voice chat!");
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# Voice Chat DISABLED.");
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        SE_LOG_CHAT_INIT_FAILED();
        s_chatSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
        goto failed;
    }

    {
        FMOD_RESULT result;

        if (!s_chatSystem && (result = FMOD::System_Create(&s_chatSystem)) != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            s_chatSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
            goto failed;
        }

        uint32_t version;
        result = s_chatSystem->getVersion(&version);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            s_chatSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);
            goto failed;
        }

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - FMOD version %08x detected", version);

        if (version != FMOD_VERSION) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -######## ERROR! FMOD version mismatch! This program requires version %08x", FMOD_VERSION);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        int32_t numOutputDrivers = SE2::GetNumOutputDrivers(1);
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - %d Output drivers detected", numOutputDrivers);
        s_chatOutputDrivers.SetCount(numOutputDrivers);

        if (!numOutputDrivers) {
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        for (int32_t i = 0; i < numOutputDrivers; i++) {
            char driverName[256];
            SE2::GetOutputDriverName(i, driverName, sizeof(driverName), 1);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " --- [%d] %s", i, driverName);
            SStrCopy(s_chatOutputDrivers[i].m_name, driverName, sizeof(SEDriverInfo));
        }

        int32_t numInputDrivers = SE2::GetNumInputDrivers(1);
        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " - %d Input drivers detected", numInputDrivers);
        s_chatInputDrivers.SetCount(numInputDrivers);

        for (int32_t i = 0; i < numInputDrivers; i++) {
            char driverName[256];
            SE2::GetInputDriverName(i, driverName, sizeof(driverName), 1);
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " --- [%d] %s", i, driverName);
            SStrCopy(s_chatInputDrivers[i].m_name, driverName, sizeof(SEDriverInfo));
        }

        if (!numInputDrivers) {
            // STATIC: 0x00D43970
            static CVar* s_enableMicrophone = CVar::Lookup("EnableMicrophone");
            s_enableMicrophone->Set("0", true, false, false, true);

            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#############################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   No input devices detected! ");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#           You will be able to hear others chat, but you will not be able to talk to anyone!");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -#############################################################################################");
        }

        result = s_chatSystem->setHardwareChannels(0, 0, 0, 0);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        result = s_chatSystem->setSoftwareChannels(8);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        *outputDriverIndex = FindDriverIndex(outputDriverName, *outputDriverIndex, 1, 1);
        *inputDriverIndex = FindDriverIndex(inputDriverName, *inputDriverIndex, 0, 1);

        int32_t outputDriver = *outputDriverIndex ? *outputDriverIndex - 1 : 0;

        s_chatSystem->getDriverName(outputDriver, s_chatOutputDeviceName, sizeof(s_chatOutputDeviceName));
        result = s_chatSystem->setDriver(outputDriver);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        int32_t inputDriver = *inputDriverIndex;

        if (inputDriver) {
            inputDriver--;
        }

        s_chatSystem->getRecordDriverName(inputDriver, s_chatInputDeviceName, sizeof(s_chatInputDeviceName));
        result = s_chatSystem->setRecordDriver(inputDriver);
        SE_ERRCHECK(result);

        int32_t outputRate = 48000;

        // STATIC: 0x00D4396C
        static CVar* s_outputQuality = CVar::Lookup("Sound_OutputQuality");

        if (s_outputQuality) {
            if (s_outputQuality->m_intValue == 0) {
                outputRate = 22050;
            } else if (s_outputQuality->m_intValue == 1) {
                outputRate = 44100;
            }
        }

        FMOD_DSP_RESAMPLER resampler = FMOD_DSP_RESAMPLER_LINEAR;

        if (s_outputQuality) {
            resampler = static_cast<FMOD_DSP_RESAMPLER>(s_outputQuality->m_intValue == 2);
        }

        CVar* enableMixMode2 = CVar::Lookup("Sound_EnableMixMode2");

        if (enableMixMode2 && enableMixMode2->m_intValue) {
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# NOTICE:   Using Mix Mode 2.");

            if (enableMixMode2->m_intValue == 2) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 2 PCM24");
                SE_ERRCHECK(s_system->setSoftwareFormat(outputRate, FMOD_SOUND_FORMAT_PCM24, 0, 0, resampler));
            } else if (enableMixMode2->m_intValue == 3) {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 3 PCM32");
                SE_ERRCHECK(s_system->setSoftwareFormat(outputRate, FMOD_SOUND_FORMAT_PCM32, 0, 0, resampler));
            } else {
                SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -# 1 PCMFLOAT");
                SE_ERRCHECK(s_system->setSoftwareFormat(outputRate, FMOD_SOUND_FORMAT_PCMFLOAT, 0, 0, resampler));
            }

            SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################");
        }

        result = s_chatSystem->init(8, FMOD_INIT_3D_RIGHTHANDED, nullptr);

        if (result != FMOD_OK) {
            SE_ERRCHECK(result);
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        LogUsingDriver(*outputDriverIndex, "Voice Chat Output", 1, 1);

        if (numInputDrivers) {
            LogUsingDriver(*inputDriverIndex, "Voice Chat Input", 0, 1);
        }

        memset(&s_captureExInfo, 0, sizeof(s_captureExInfo));
        s_captureExInfo.cbsize = sizeof(s_captureExInfo);
        s_captureExInfo.format = FMOD_SOUND_FORMAT_PCM16;
        s_captureExInfo.defaultfrequency = static_cast<int32_t>(sampleRate);
        s_captureExInfo.numchannels = 1;
        s_captureExInfo.length = s_captureExInfo.defaultfrequency * sizeof(int16_t) * 2;

        result = s_chatSystem->createSound(nullptr, FMOD_2D | FMOD_SOFTWARE | FMOD_OPENUSER, &s_captureExInfo, &s_captureSound);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        uint32_t length;
        s_captureSound->getLength(&length, FMOD_TIMEUNIT_PCM);

        s_captureChannels = s_captureExInfo.numchannels;
        s_captureLength = length;
        s_capturing = 1;
        s_primaryCaptureCallbacks = nullptr;
        s_alternateCaptureCallbacks = nullptr;
        s_captureCallbacks = nullptr;

        CreateMonitorDSP();

        result = s_chatSystem->set3DSettings(1.0f, 1.0f, 4.0f);
        SE_ERRCHECK(result);

        if (result != FMOD_OK) {
            SE_LOG_CHAT_INIT_FAILED();
            goto nosound;
        }

        FMOD_Debug_SetLevel(FMOD_DEBUG_LEVEL_NONE);

        SE2::sm_bCaptureInitialized = 1;

        SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Voice Chat is ready.");
        return;
    }

nosound:
    s_chatSystem->setOutput(FMOD_OUTPUTTYPE_NOSOUND);

failed:
    if (s_chatSystem && s_chatSystem != s_system) {
        SE2::DestroyChatSystem();
    }

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=>###########################################################################################");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=># Voice Chat Init FAILED.");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=>###########################################################################################");
}

// VERIFY: 1cc3690f manual
// OFFSET: 0x0087C580
void SE2::DestroyChatSystem() {
    if (!s_chatSystem) {
        return;
    }

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " ");
    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Shutting Down Voice Chat");

    s_capturing = 0;

    bool recording = false;

    if (s_chatSystem) {
        s_chatSystem->isRecording(&recording);

        if (recording) {
            SE2::StopCapture();
        }
    }

    SE2::HeartBeat(nullptr, nullptr);

    StopChatSounds();
    ProcessDeleteList(1);

    if (s_captureSound) {
        s_captureSound->release();
        s_captureSound = nullptr;
    }

    if (s_monitorDSP) {
        s_monitorDSP->release();
        s_monitorDSP = nullptr;
    }

    s_chatSystem->close();
    s_chatSystem->release();
    s_chatSystem = nullptr;

    s_userMonitorDSPCallback = nullptr;
    SE2::sm_bCaptureInitialized = 0;

    SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, "=> Done Shutting Down Voice Chat");
}

// VERIFY: 5722bcf3 manual
// OFFSET: 0x009C1820
SESoundUserData::~SESoundUserData() {
}

void SESoundUserData::Release() {
}

// INLINED
SECaptureCallbacks::~SECaptureCallbacks() {
}
