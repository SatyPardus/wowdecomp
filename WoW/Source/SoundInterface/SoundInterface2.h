#ifndef WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2_H
#define WOW_SOURCE_SOUNDINTERFACE_SOUNDINTERFACE2_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class CGUnit_C;
class CGUnitVirtualItem;
struct CVar;
class SoundEntriesAdvancedRec;
class SE2SoundKitObject;
class SE2SoundKitProperties;
class VSOUNDKITDEF;

class SE2SoundKitProperties {
    public:
    /* 0000 */ int32_t m_group;
    /* 0004 */ float m_volume;
    /* 0008 */ float m_volumeScale;
    /* 000C */ int32_t m_fileIndex;
    /* 0010 */ float m_fadeInTime;
    /* 0014 */ float m_fadeOutTime;
    /* 0018 */ int32_t m_selectMode;
    /* 001C */ int32_t m_loopMode;
    /* 0020 */ int32_t m_priority;
    /* 0024 */ int32_t m_suppressorMode;
    /* 0028 */ uint32_t m_decodeBufferSize;
    /* 002C */ float m_maxDistance;
    /* 0030 */ int32_t m_maxAudibleBehavior;
    /* 0034 */ float m_muteFadeSpeed;
    /* 0038 */ bool m_streaming;
    /* 003C */ int32_t m_reverb;
    /* 0040 */ int32_t m_noRandomStart;
    /* 0044 */ int32_t m_blockMusicFiles;
    /* 0048 */ int32_t m_software;
    /* 004C */ float m_coneInsideAngle;
    /* 0050 */ float m_coneOutsideAngle;
    /* 0054 */ float m_coneOutsideVolume;
    /* 0058 */ const C3Vector* m_coneOrientation;
    /* 005C */ int32_t m_checkStreaming;
    /* 0060 */ int32_t m_useDspChain;
    /* 0064 */ char m_dspChainName[128];
    /* 00E4 */ int32_t m_forceLoop;

    void ResetToDefaults();
};

class SE2SoundKitObject {
    public:
    /* 0000 */ TSLink<SE2SoundKitObject> m_link;
    /* 0008 */ SESound m_sound;
    /* 000C */ uint32_t m_advancedId;
    /* 0010 */ uint32_t unused_0010;

    SE2SoundKitObject();
    ~SE2SoundKitObject();
};

class SI2SoundUserData : public SESoundUserData {
    public:
    /* 0004 */ const char* m_name;
    /* 0008 */ int32_t m_soundKitId;
    /* 000C */ VSOUNDKITDEF* m_def;
    /* 0010 */ int32_t m_group;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ uint32_t m_advancedId;

    virtual ~SI2SoundUserData();
    virtual void Release();

    SI2SoundUserData();
};

class SI2 {
    public:
    static const char s_channelGroupNames[18][32];
    static const int32_t s_channelGroupMaxSounds[18];
    static int32_t s_channelGroupNumSounds[18];
    static TSGrowableArray<uint32_t> s_suppressors;
    static int32_t sm_bPlayerInside;
    static int32_t sm_bCameraIsUnderwater;
    static int32_t sm_bUnderwaterChanged;
    static uint32_t sm_nextZoneMusicTime;
    static int32_t sm_bLuaMusicPlaying;
    static TSGrowableArray<uint32_t> sm_aIntroMusicNextValidPlayTime;

    static int32_t AddAdvancedSound(SoundEntriesAdvancedRec* rec, int32_t soundKitId, const C3Vector* position, const C3Vector* orientation, SE2SoundKitObject* object, int32_t quickFade, int32_t a7, int32_t checkDistance, int32_t forceLoop);
    static void AIMusicFinished(SESound* sound, void* param);
    static void ApplyReverbSettings();
    static int32_t AttachToObject(SE2SoundKitObject* object, WOWGUID guid);
    static int32_t ChaosModeUpdate(const void* data, void* param);
    static void ClearSuppressors();
    static void ClearSuppressorSounds();
    static void CreateSoundGroups();
    static void CreateVSoundLookup();
    static int32_t CreditsMusicUpdate(const void* data, void* param);
    static void DestroyVSoundLookup();
    static void Detach(SE2SoundKitObject* object);
    static void DSP_AttachCustomDspChain(FMOD::Channel* channel, char* name);
    static void DSP_CreateDsp(const char* name, int32_t type, const float* params);
    static void DSP_Init();
    static void DSP_MakeDSPsExpire(float elapsed);
    static void UpdateTaxiSounds(float elapsed);
    static void DSP_ShutDown();
    static void FreeEmoteSounds();
    static void FreeFootstepSounds();
    static void FreeSoundKitDefs();
    static void FreeWorldStateZoneSoundOverrides();
    static void FreeZoneIntros();
    static void FreeZoneSoundOverrides();
    static float GetChannelGroupVolume(int32_t group);
    static int32_t GetDesiredAmbienceKitID(int32_t* type);
    static int32_t GetDesiredMusicKitID(int32_t* type, int32_t* silenceMin, int32_t* silenceMax);
    static bool GetInputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static bool GetInputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static int32_t GetListenerPosition(C3Vector* position);
    static int32_t GetNumInputDrivers(int32_t chatSystem);
    static int32_t GetNumInputDrivers_Cached(int32_t chatSystem);
    static int32_t GetNumOutputDrivers(int32_t chatSystem);
    static int32_t GetNumOutputDrivers_Cached(int32_t chatSystem);
    static int32_t GetNumSoundsInSoundKit(int32_t soundKitId);
    static bool GetObjectPosition(uint64_t guid, C3Vector* position);
    static bool GetOutputDriverName(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static bool GetOutputDriverName_Cached(int32_t index, char* name, int32_t nameLength, int32_t chatSystem);
    static VSOUNDKITDEF* GetSoundKitDef(int32_t soundKitId);
    static int32_t GetSoundKitUsage(int32_t soundKitId);
    static float GetSoundVolume(SESoundUserData* userData);
    static int32_t GetTextEmoteSoundID(uint32_t emotesTextId, uint32_t race, uint32_t sex);
    static int32_t GetTextEmoteSoundIndex(uint32_t emotesTextId, uint32_t race, uint32_t sex);
    static int32_t GetZoneReverb();
    static int32_t GetChunkLocation(int32_t* mapId, int32_t* chunkX, int32_t* chunkY, int32_t* subchunkX, int32_t* subchunkY);
    static int32_t GetZoneSoundOverride(int32_t mapId, int32_t chunkX, int32_t chunkY, int32_t subchunkX, int32_t subchunkY, int32_t* zoneIntroMusicId, int32_t* zoneMusicId, int32_t* soundAmbienceId, int32_t* soundProviderPreferencesId);
    static int32_t GlueMusicUpdate(const void* data, void* param);
    static int32_t HandleSoundDeviceChange(const void* data, void* param);
    static int32_t HandleWaterSounds(const void* data, void* param);
    static int32_t Init(int32_t restart);
    static void InitEmoteSounds();
    static void InitFootstepSounds();
    static void InitSoundKitLookup();
    static void InitVocalErrorSounds(int32_t race, uint32_t sex);
    static void InitWeaponImpactCombatSounds();
    static void InitWeaponSwingCombatSounds();
    static void InitWorldStateZoneSoundOverrides();
    static void InitZoneIntros();
    static void InitZoneSoundOverrides();
    static void InitZoneSoundsHandler();
    static void IntroMusicFinished(SESound* sound, void* param);
    static int32_t IntroMusicOkToPlay(int32_t soundKitId, int32_t type);
    static int32_t IsAdvancedSoundNear(int32_t soundKitId, const C3Vector* position);
    static bool IsLooping(SE2SoundKitObject* object);
    static bool IsPlaying(SE2SoundKitObject* object);
    static uint32_t IsSoundKitLooping(int32_t soundKitId);
    static int32_t IsTooCloseToExistingSoundKit(int32_t soundKitId, const C3Vector* position);
    static int32_t IsUnderwater();
    static int32_t LookupSoundKitId(const char* name);
    static void MarkAdvancedSoundStopped(uint32_t id);
    static int32_t OnFocusChanged(const void* data, void* param);
    static void PlayAIMusic(int32_t soundKitId);
    static int32_t PlayFoleySound(int32_t materialId, const C3Vector* position, int32_t self);
    static void PlayFootstepSound(int32_t footstepId, const C3Vector* position, int32_t terrainType, int32_t splash, int32_t loud, CGUnit_C* unit);
    static void PlayHitSound(const CGUnitVirtualItem* weapon, int32_t impactType, int32_t crit, const C3Vector* position, int32_t self);
    static void PlayItemSound(int32_t type, int32_t displayId);
    static int32_t PlayMissedSound(bool oneHand, const C3Vector* position, int32_t self);
    static void PlayParrySound(const CGUnitVirtualItem* weapon, const CGUnitVirtualItem* defender, int32_t crit, const C3Vector* position, int32_t self);
    static int32_t PlaySheatheSound(const CGUnitVirtualItem* item, int32_t sheathe, const C3Vector* position, int32_t self);
    static int32_t PlaySoundFile(const char* filename, int32_t group);
    static int32_t PlaySoundKit(const char* name, const C3Vector* position, SE2SoundKitObject* object, SE2SoundKitProperties* properties);
    static void PlayVocalErrorSound(int32_t error);
    static void PlaySpellFizzleSound(uint32_t spellId, const void* object);
    static void DK_GetPlayerFilterName(CGUnit_C* unit, char* name, uint32_t size);
    static int32_t PlaySoundKit(int32_t soundKitId, const C3Vector* position, SE2SoundKitObject* object, SE2SoundKitProperties* properties, uint32_t advancedId, uint32_t* soundId, int32_t checkDistance, int32_t keepPlaying);
    static int32_t PlayUISound(int32_t soundKitId);
    static int32_t PlayWeaponSwooshSound(int32_t type, int32_t crit, const C3Vector* position, int32_t quiet, int32_t self);
    static void RegisterCVars();
    static void RegisterDeviceChangeHandler();
    static void RegisterScriptFunctions();
    static void RegisterSoundEngineCVars();
    static void ReleaseSoundGroups();
    static int32_t ResetSoundKitVariants(const char* name);
    static int32_t Set3DPosition(SE2SoundKitObject* object, const C3Vector* position);
    static void SetAdvancedSoundPosition(uint32_t id, const C3Vector* position);
    static void SetChannelGroupVolume(int32_t group, float volume);
    static int32_t SetListenerAttributes(const C3Vector* position, const C3Vector* forward, const C3Vector* up, const C3Vector* velocity);
    static void OverrideZoneSounds();
    static void SetOverrideZoneSounds(int32_t soundAmbienceId, int32_t zoneMusicId);
    static void SetPitch(SE2SoundKitObject* object, float pitch);
    static void SetPlayerInside(bool inside);
    static void SetUnderwaterStatus(int32_t underwater);
    static int32_t SetVolume(SE2SoundKitObject* object, float volume);
    static void SetWeatherKitID(int32_t soundKitId);
    static void ForceUpdateZoneSounds();
    static void SetZoneAmbienceID(int32_t type, int32_t soundAmbienceId);
    static void SetZoneIntroMusicID(int32_t type, int32_t zoneIntroMusicId);
    static void SetZoneMusicID(int32_t type, int32_t zoneMusicId);
    static void SetZoneReverb(int32_t type, int32_t reverb);
    static void SetZoneUnderwaterReverb(int32_t reverb);
    static int32_t ShutDown(int32_t restart);
    static void ShutdownZoneSoundsHandler();
    static void SoundFinished(SESound* sound, void* param);
    static void StartCreditsMusic(const char* name);
    static void StartGlueAmbience(const char* name, float fadeInTime);
    static void StartGlueMusic(const char* name);
    static void StopAdvancedSound(uint32_t id);
    static void StopAdvancedSound(uint32_t id, int32_t remove, float fadeOutTime);
    static void StopAIMusic();
    static void StopAllMusic();
    static void StopAllSounds();
    static void StopAllSounds(float fadeOutTime);
    static void StopCreditsMusic();
    static void StopGlueAmbience(float fadeOutTime);
    static void StopGlueMusic(float fadeOutTime);
    static void StopLuaMusic();
    static int32_t StopSound(SE2SoundKitObject* object, int32_t immediate, float fadeOutTime, int32_t stopAdvanced);
    static void StopZoneAmbience();
    static void UnregisterScriptFunctions();
    static int32_t UpdateAdvancedSounds(const void* data, void* param);
    static int32_t UpdateZoneSoundsForWorldState(int32_t areaId, int32_t zoneId, int32_t wmoAreaId, int32_t wmoGroupAreaId, int32_t inside);
    static void UpdateDucking(int32_t elapsed);
    static void UpdateReverb();
    static void ValidateOutputDriver();
    static void ValidateVoiceChatInputDriver();
    static void ValidateVoiceChatOutputDriver();
    static void VoiceChat_ActivatePrimaryCaptureCallback();
    static void VoiceChat_AmplifyMicrophoneInput(int16_t* data, uint32_t samples);
    static void VoiceChat_CleanUpLoopbackSound();
    static int32_t VoiceChat_CreateChatSound(SE2SoundKitObject* object, SEStreamCallbacks* callbacks, int32_t chatSystem);
    static void VoiceChat_DestroyChatSystem();
    static int32_t VoiceChat_GetCurrentMicrophoneSignalLevel();
    static void VoiceChat_InitCapture(float sampleRate);
    static bool VoiceChat_IsPlayingLoopbackSound();
    static bool VoiceChat_IsRecordingLoopbackSound();
    static int32_t VoiceChat_IsVoiceChatInitialized();
    static int32_t VoiceChat_Play(SE2SoundKitObject* object);
    static void VoiceChat_PlayLoopbackSound();
    static void VoiceChat_RecordLoopbackSound(int32_t seconds);
    static void VoiceChat_SetCaptureDevice(int32_t index);
    static void VoiceChat_SetOutputDevice(int32_t index);
    static void VoiceChat_SetPrimaryCaptureCallback(SECaptureCallbacks* callbacks);
    static void VoiceChat_ShutDownCapture();
    static void VoiceChat_StartCapture();
    static void VoiceChat_StopCapture();
    static void VoiceChat_StopPlayingLoopbackSound();
    static void VoiceChat_StopRecordingLoopbackSound();
    static void WaterShutdown();
    static int32_t ZoneMusicDelayElapsed(int32_t type);
    static void ZoneMusicFinished(SESound* sound, void* param);
    static int32_t ZoneSoundsUpdate(const void* data, void* param);
};

class CGItem_C;
int32_t SndInterfacePlayItemSound(int32_t type, CGItem_C* item);

#endif
