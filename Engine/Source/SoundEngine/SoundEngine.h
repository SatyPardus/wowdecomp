#ifndef ENGINE_SOURCE_SOUNDENGINE_SOUNDENGINE_H
#define ENGINE_SOURCE_SOUNDENGINE_SOUNDENGINE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/SoundEngine/fmodex/src/fmod.hpp"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class SESound;
class SESoundInternal;
class SESoundUserData;
class SFileImpl;

#define SE_ERRCHECK(result) SE2::ErrCheck(__LINE__, __FILE__, (result))
#define SE_LOG_INIT_FAILED(result) SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################"); SE2::Log_Write(__LINE__, __FILE__, (result), " -######## ERROR INITIALIZING. ALL GAME SOUND DISABLED."); SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################")
#define SE_LOG_CHAT_INIT_FAILED() SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################"); SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -######## ERROR INITIALIZING. VOICE CHAT DISABLED."); SE2::Log_Write(__LINE__, __FILE__, FMOD_OK, " -###########################################################################################")

enum SESOUNDTYPE {
    SESOUNDTYPE_NONE = 0,
    SESOUNDTYPE_FILE = 1,
    SESOUNDTYPE_MEMORY = 2,
    SESOUNDTYPE_CHAT = 3,
};

enum SECHANNELSTATE {
    SECHANNELSTATE_NONE = 0,
    SECHANNELSTATE_LOADING = 1,
    SECHANNELSTATE_PLAYING = 2,
    SECHANNELSTATE_VIRTUAL = 3,
    SECHANNELSTATE_OTHER = 4,
};

struct SEDriverInfo {
    /* 0000 */ char m_name[256];
};

struct SEChannelGroup {
    /* 0000 */ uint32_t m_hash;
    /* 0004 */ int32_t m_parent;
    /* 0008 */ float m_volume;
    /* 000C */ float m_muteVolume;
    /* 0010 */ uint8_t m_dirty;
};

struct SEChannelInfo {
    /* 0000 */ int32_t m_state;
    /* 0004 */ C3Vector m_position;
    /* 0010 */ float m_minDistance;
    /* 0014 */ float m_maxDistance;
    /* 0018 */ uint8_t m_is3D;
    /* 0019 */ uint8_t m_looping;
    /* 001A */ uint8_t m_fadingIn;
    /* 001B */ uint8_t m_fadingOut;
    /* 001C */ SESoundUserData* m_userData;
    /* 0020 */ float m_fadeVolume;
    /* 0024 */ float m_distanceSquared;
    /* 0028 */ float m_volume;
    /* 002C */ int32_t m_priority;
    /* 0030 */ float m_audibility;
    /* 0034 */ float m_channelFadeVolume;
    /* 0038 */ uint8_t m_hardware;
    /* 003C */ int32_t m_cached;
    /* 0040 */ float m_panLevel;
};

class SESoundUserData {
    public:
    virtual ~SESoundUserData();
    virtual void Release();
};

class SECaptureCallbacks {
    public:
    virtual ~SECaptureCallbacks();
    virtual void ProcessData(int16_t* data, uint32_t samples) = 0;
};

class SEStreamCallbacks {
    public:
    virtual ~SEStreamCallbacks();
    virtual bool GetSamples(uint32_t position, uint32_t count, float* buffer) = 0;
    virtual void DiscardSamples(uint32_t position, uint32_t count);
    virtual void Delete();
};

class SEWaveDataNode : public TSLinkedNode<SEWaveDataNode> {
    public:
    /* 0008 */ FMOD::Sound* m_sound;
    /* 000C */ int32_t m_loaded;
    /* 0010 */ char m_name[128];
    /* 0090 */ uint32_t m_hash;
    /* 0094 */ int32_t m_refCount;
    /* 0098 */ uint32_t m_size;

    SEWaveDataNode();
    ~SEWaveDataNode();
};

class SESoundInternal : public TSLinkedNode<SESoundInternal> {
    public:
    /* 0010 */ FMOD::System* m_system;
    /* 0014 */ FMOD::Channel* m_channel;
    /* 0018 */ SESound* m_handle;
    /* 001C */ SESoundUserData* m_userData;
    /* 0020 */ float m_volume;
    /* 0024 */ float m_fadeVolume;
    /* 0028 */ float m_fadeInTime;
    /* 002C */ float m_fadeOutTime;
    /* 0030 */ uint8_t m_fadingIn;
    /* 0031 */ uint8_t m_fadingOut;
    /* 0034 */ uint32_t m_playTime;
    /* 0038 */ int32_t m_cached;
    /* 003C */ int32_t m_type;
    /* 0040 */ TSLink<SESoundInternal> m_deleteLink;
    /* 0048 */ TSLink<SESoundInternal> m_objectLink;
    /* 0050 */ C3Vector m_position;
    /* 005C */ float m_minDistance;
    /* 0060 */ float m_maxDistance;
    /* 0064 */ int32_t m_channelGroup;
    /* 0068 */ FMOD_MODE m_mode;
    /* 006C */ uint8_t m_play;
    /* 006D */ uint8_t m_stop;
    /* 0070 */ float m_frequency;
    /* 0074 */ float m_pitch;
    /* 0078 */ int32_t m_priority;
    /* 007C */ int32_t m_deleteDelay;
    /* 0080 */ int32_t m_reverb;
    /* 0084 */ int32_t m_streaming;
    /* 0088 */ int32_t m_randomStart;
    /* 008C */ int32_t m_software;
    /* 0090 */ float m_coneInsideAngle;
    /* 0094 */ float m_coneOutsideAngle;
    /* 0098 */ float m_coneOutsideVolume;
    /* 009C */ C3Vector m_coneOrientation;
    /* 00A8 */ int32_t m_loaded;
    /* 00AC */ void (*m_finishedCallback)(SESound*, void*);
    /* 00B0 */ void* m_finishedParam;
    /* 00B4 */ void (*m_finishedCallback2)(SESound*, void*);
    /* 00B8 */ void* m_finishedParam2;
    /* 00BC */ void (*m_fadeOutCallback)(SESound*);
    /* 00C0 */ void (*m_dspChainCallback)(FMOD::Channel*, char*);
    /* 00C4 */ char* m_dspChainName;
    /* 00C8 */ uint64_t m_objectGUID;
    /* 00D0 */ uint32_t m_id;

    virtual ~SESoundInternal() = 0;
    virtual int32_t IsReadyForRelease();

    SESoundInternal();
    float GetScaledVolume();
    void Play();
    int32_t Set3DMinMaxDistance(float minDistance, float maxDistance);
    void UpdateVolume();
};

class SEFileSound : public SESoundInternal {
    public:
    /* 00D8 */ TSLink<SESoundInternal> m_goLink;
    /* 00E0 */ SFileImpl* m_file;
    /* 00E4 */ uint32_t unused_00E4;
    /* 00E8 */ uint32_t unused_00E8;
    /* 00EC */ FMOD::Sound* m_sound;
    /* 00F0 */ FMOD::SoundGroup* m_loadSoundGroup;
    /* 00F4 */ FMOD::SoundGroup* m_soundGroup;
    /* 00F8 */ uint8_t m_loadCompleted;
    /* 00FC */ SEWaveDataNode* m_waveData;

    virtual ~SEFileSound();
    virtual int32_t IsReadyForRelease();

    SEFileSound();
    void AbortNonBlockingLoad(FMOD_RESULT result);
    void CompleteNonBlockingLoad();
};

class SEMemorySound : public SESoundInternal {
    public:
    /* 00D8 */ FMOD::Sound* m_sound;

    virtual ~SEMemorySound();

    SEMemorySound();
};

class SEChatSound : public SESoundInternal {
    public:
    static FMOD_RESULT F_CALLBACK DSPReadCallback(FMOD_DSP_STATE* dspState, float* inBuffer, float* outBuffer, unsigned int length, int inChannels, int outChannels);
    static FMOD_RESULT F_CALLBACK DSPSeekCallback(FMOD_DSP_STATE* dspState, unsigned int position);

    /* 00D8 */ SEStreamCallbacks* m_callbacks;
    /* 00DC */ uint32_t m_position;
    /* 00E0 */ FMOD::DSP* m_dsp;

    virtual ~SEChatSound();

    SEChatSound();
};

struct SOUND_INTERNAL_LOOKUP : public TSHashObject<SOUND_INTERNAL_LOOKUP, HASHKEY_NONE> {
    /* 0018 */ SESoundInternal* m_sound;
};

class SESound {
    public:
    /* 0000 */ SESoundInternal* m_internal;

    SESound();
    ~SESound();
    void AttachToObject(uint64_t guid);
    void CompleteNonBlockingLoad();
    int32_t Create2D(const char* filename, int32_t looping, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, bool streaming, int32_t reverb, int32_t priority, uint32_t decodeBufferSize);
    int32_t Create3D(const char* filename, int32_t looping, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, bool streaming, int32_t priority, int32_t software, float coneInsideAngle, float coneOutsideAngle, float coneOutsideVolume, const C3Vector* coneOrientation);
    void CreateChatSound(SEStreamCallbacks* callbacks, int32_t chatSystem);
    void Detach();
    void FadeInNow();
    void FadeOutNow();
    uint32_t GetID();
    uint32_t GetPosition();
    SESoundUserData* GetUserData();
    bool Is3D();
    bool IsLooping();
    bool IsPlaying();
    void Play();
    void PlaySoundFromMemory(const char* data, uint32_t length, int32_t chatSystem);
    void Set3DMinMaxDistance(float minDistance, float maxDistance);
    void Set3DPanLevel(float level);
    void Set3DPosition(const C3Vector& position);
    void SetAttachCustomDSPChainCallback(void (*callback)(FMOD::Channel*, char*), const char* name);
    void SetChannelGroup(const char* name, bool childOfMaster);
    void SetFadeInTime(float seconds);
    void SetFadeOutTime(float seconds);
    void SetFinishedCallback(void (*callback)(SESound*, void*), void* param);
    void SetFinishedCallback2(void (*callback)(SESound*, void*), void* param);
    void SetPitch(float pitch);
    void SetPosition(uint32_t position);
    void SetUserData(SESoundUserData* userData);
    void SetVolume(float volume);
    void Stop(int32_t immediate, float fadeOutTime);
};

class SE2 {
    public:
    static int32_t sm_bCaptureInitialized;

    static void ActivateAlternateCaptureCallback();
    static void ActivatePrimaryCaptureCallback();
    static FMOD_RESULT AddChannelToChannelGroup(FMOD::Channel* channel, FMOD::ChannelGroup* channelGroup);
    static bool AlternateCaptureCallbackIsActive();
    static FMOD_RESULT AttachMonitorDSP(SESound* sound);
    static FMOD_RESULT BypassDsp(FMOD::DSP* dsp, bool bypass);
    static void CaptureWork();
    static FMOD_RESULT ConnectDspToChannelGroup(FMOD::DSP* dsp, FMOD::ChannelGroup* channelGroup);
    static FMOD::ChannelGroup* CreateChannelGroup(const char* name);
    static FMOD_RESULT CreateDSPByType(FMOD_DSP_TYPE type, FMOD::DSP** dsp);
    static FMOD::SoundGroup* CreateSoundGroup(const char* name, int32_t maxAudible);
    static int32_t CreateWaveData(FMOD::System* system, const char* filename, FMOD_MODE mode, SESound* sound, FMOD::SoundGroup* loadSoundGroup, FMOD::SoundGroup* soundGroup, bool randomStart, int32_t priority, int32_t reverb, bool streaming, uint32_t decodeBufferSize, int32_t software, float coneInsideAngle, float coneOutsideAngle, float coneOutsideVolume, const C3Vector* coneOrientation);
    static FMOD_RESULT DestroyChannelGroup(FMOD::ChannelGroup* channelGroup);
    static void DestroyChatSystem();
    static FMOD_RESULT DspDestroy(FMOD::DSP* dsp);
    static FMOD_RESULT EnableDSP(FMOD::DSP* dsp, bool active);
    static void EnableReverb(int32_t enable);
    static void ErrCheck(uint32_t line, const char* file, FMOD_RESULT result);
    static int32_t GetChannelGroupNumChannels(FMOD::ChannelGroup* channelGroup);
    static float GetChannelGroupVolume(const char* name);
    static int32_t GetChannelInfo(bool fadingOnly, SEChannelInfo* info, int32_t maxCount, int32_t playing, int32_t isVirtual, int32_t loading, int32_t other, uint32_t* hardwareCount, uint32_t* softwareCount, uint32_t* virtualCount, uint32_t* loadingCount, uint32_t* otherCount);
    static int32_t GetChannelInfo(SEChannelInfo* info, int32_t maxCount, int32_t playing, int32_t isVirtual, int32_t loading, int32_t other, uint32_t* hardwareCount, uint32_t* softwareCount, uint32_t* virtualCount, uint32_t* loadingCount, uint32_t* otherCount);
    static char* GetCurrentChatInputDeviceName();
    static char* GetCurrentChatOutputDeviceName();
    static char* GetCurrentGameOutputDeviceName();
    static bool GetInputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static bool GetInputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static bool GetListenerPosition(C3Vector* position);
    static int32_t GetNumInputDrivers(int32_t chatSystem);
    static int32_t GetNumInputDrivers_Cached(int32_t chatSystem);
    static int32_t GetNumOutputDrivers(int32_t chatSystem);
    static int32_t GetNumOutputDrivers_Cached(int32_t chatSystem);
    static bool GetOutputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static bool GetOutputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static SESound* GetSoundFromID(uint32_t id, int32_t* created);
    static int32_t HeartBeat(const void* data, void* param);
    static int32_t Init(int32_t maxChannels, bool (*positionCallback)(uint64_t, C3Vector*), int32_t enableReverb, int32_t enableSoftwareHRTF, int32_t* numChannels, int32_t* outputDriverIndex, const char* outputDriverName, float (*volumeCallback)(SESoundUserData*), int32_t restart);
    static void InitCapture(int32_t* outputDriverIndex, const char* outputDriverName, int32_t* inputDriverIndex, const char* inputDriverName, float sampleRate);
    static bool IsCapturing();
    static int32_t IsChatSoundInitialized();
    static int32_t IsInitialized();
    static int32_t IsReverbEnabled();
    static void LockSoundList();
    static void Log_Write(const char* format, ...);
    static void Log_Write(uint32_t line, const char* file, FMOD_RESULT result, const char* format, ...);
    static void MuteAllSound(bool mute);
    static void MuteChannelGroup(const char* name, bool mute);
    static void PlaySoundFromMemory(FMOD::System* system, const char* data, uint32_t length, FMOD_MODE mode, SESound* sound);
    static void CreateChatSound(FMOD::System* system, SEStreamCallbacks* callbacks, FMOD_MODE mode, SESound* sound);
    static FMOD_RESULT ReleaseSoundGroup(FMOD::SoundGroup* soundGroup);
    static void SetAlternateCaptureCallback(SECaptureCallbacks* callbacks);
    static void SetChannelGroupVolume(const char* name, float volume);
    static FMOD_RESULT SetDeviceChangeCallback(void (*callback)());
    static FMOD_RESULT SetDspParam(FMOD::DSP* dsp, int32_t index, float value);
    static FMOD_RESULT SetFMODChannelGroupVolume(FMOD::ChannelGroup* channelGroup, float volume);
    static void SetListenerAttributes(const C3Vector* position, const C3Vector* forward, const C3Vector* up, const C3Vector* velocity);
    static void SetMasterVolume(float volume);
    static void SetPrimaryCaptureCallback(SECaptureCallbacks* callbacks);
    static void SetReverb(const FMOD_REVERB_PROPERTIES* properties);
    static void SetSoundGroupMaxAudibleBehavior(FMOD::SoundGroup* soundGroup, int32_t behavior, float muteFadeSpeed);
    static void SetUserMonitorDSPCallback(void (*callback)(float*, uint32_t));
    static void ShutDown(int32_t restart);
    static void ShutDownCapture();
    static void StartCapture();
    static void StopAllSounds(int32_t restart);
    static void StopAllSounds(const char* channelGroup, float fadeOutTime);
    static void StopCapture();
    static void UnlockSoundList();
    static void UnregisterHeartBeat();
};

#endif
