#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/Base/CmdLine.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Os/System.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/GameUI.h"

// VERIFY: 416b0558 auto
// OFFSET: 0x004D0BC0
static bool EnableReverbHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value)) {
        SE2::EnableReverb(1);
        SI2::ApplyReverbSettings();
    } else {
        FMOD_REVERB_PROPERTIES properties = FMOD_PRESET_OFF;
        SE2::SetReverb(&properties);
        SE2::EnableReverb(0);
    }

    return true;
}

// VERIFY: 5dc4640a manual
// OFFSET: 0x004D0CB0
static bool VoiceChatInputDriverIndexHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t index = SStrToInt(value);
    char name[256];
    memset(name, 0, sizeof(name));

    if (SE2::GetInputDriverName(index, name, sizeof(name), 1)) {
        static CVar* s_inputDriverName = CVar::Lookup("Sound_VoiceChatInputDriverName");
        s_inputDriverName->Set(name, true, false, false, true);
    }

    return true;
}

// VERIFY: e67c8f37 manual
// OFFSET: 0x004D0D40
static bool VoiceChatOutputDriverIndexHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t index = SStrToInt(value);
    char name[256];
    memset(name, 0, sizeof(name));

    if (SE2::GetOutputDriverName(index, name, sizeof(name), 1)) {
        static CVar* s_outputDriverName = CVar::Lookup("Sound_VoiceChatOutputDriverName");
        s_outputDriverName->Set(name, true, false, false, true);
    }

    return true;
}

// VERIFY: 82450cd2 manual
// OFFSET: 0x004D0DD0
static bool OutputDriverIndexHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t index = SStrToInt(value);
    char name[256];
    memset(name, 0, sizeof(name));

    if (SE2::GetOutputDriverName(index, name, sizeof(name), 0)) {
        static CVar* s_outputDriverName = CVar::Lookup("Sound_OutputDriverName");
        s_outputDriverName->Set(name, true, false, false, true);
    }

    return true;
}

// VERIFY: 65821b05 auto
// OFFSET: 0x004D0E60
static bool MasterVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    SE2::SetMasterVolume(SStrToFloat(value));
    return true;
}

// VERIFY: 1f180752 auto
// OFFSET: 0x004D0E80
static bool SFXVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    SE2::SetChannelGroupVolume(SI2::s_channelGroupNames[0], SStrToFloat(value));
    return true;
}

// VERIFY: 8c1f3d6b auto
// OFFSET: 0x004D0EB0
static bool MusicVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    SE2::SetChannelGroupVolume("MUSIC", SStrToFloat(value));
    SE2::SetChannelGroupVolume("SCRIPTMUSIC", SStrToFloat(value));
    return true;
}

// VERIFY: de0d919f auto
// OFFSET: 0x004D0EF0
static bool AmbienceVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    SE2::SetChannelGroupVolume("AMBIENCE", SStrToFloat(value));
    return true;
}

// VERIFY: 8140baa9 auto
// OFFSET: 0x004D0F20
static bool ChaosModeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    ConsolePrintf("CHAOS!!");
    ConsolePrintf("Modes:");
    ConsolePrintf("0: Off");
    ConsolePrintf("1: Generate 1 random 3d sound per frame");
    ConsolePrintf("2: Generate 2 random 3d sounds per frame");
    ConsolePrintf("3: Start and stop 1 2d and 1 3d sound every frame");
    ConsolePrintf("4: Start and stop 2 2d and 2 3d sounds every frame");
    ConsolePrintf("5: Play the same random sound x20 (in 2d) every second");
    ConsolePrintf("6: Play the same random sound x20 (in 3d) every second");
    ConsolePrintf("7: Play the same random looping sound x20 (in 3d) every second");
    ConsolePrintf("8: Play Alert.mp3 from Data folder a lot");
    ConsolePrintf("9: Restart game sound system (NOT INCLUDING VOICE CHAT SYSTEM) every 10 seconds");
    ConsolePrintf("   ALERT! This mode breaks voice chat, and will cause a crash on game exit.");
    ConsolePrintf("10: Mode 2 + Mode 9");
    ConsolePrintf("11: Generate 1 random 2d sound per frame");
    ConsolePrintf("12: Generate 2 random 2d sounds per frame");
    ConsolePrintf("13: Generate 2 random 2d sounds AND 2 random 3d sounds per frame");
    ConsolePrintf("14: Mode 10 + Mode 6");

    int32_t mode = atol(value);

    static CVar* s_disableStreams = CVar::Lookup("Sound_DisableStreams");

    if (s_disableStreams) {
        if (!mode) {
            s_disableStreams->Set("0", true, false, false, true);
            return true;
        }

        s_disableStreams->Set("1", true, false, false, true);
    }

    return true;
}

// VERIFY: 9f0ddfd9 auto
// OFFSET: 0x004D1050
void SI2::RegisterSoundEngineCVars() {
    CVar::Register("StartTalkingDelay", "", 0, "0.0", nullptr, 5, false, nullptr, false);
    CVar::Register("StartTalkingTime", "", 0, "1.0", nullptr, 5, false, nullptr, false);
    CVar::Register("StopTalkingDelay", "", 0, "0.0", nullptr, 5, false, nullptr, false);
    CVar::Register("StopTalkingTime", "", 0, "2.0", nullptr, 5, false, nullptr, false);
    CVar::Register("OutboundChatVolume", "The software amplification factor (0.0 - 2.0)", 0, "1.0", OutboundChatVolumeHandler, 5, false, nullptr, false);
    CVar::Register("InboundChatVolume", "The volume of all other chat you hear (0.0 - 1.0)", 0, "1.0", InboundChatVolumeHandler, 5, false, nullptr, false);
    CVar::Register("VoiceChatMode", "Push to talk(0) or voice activation(1)", 0, "0", VoiceChatModeHandler, 5, false, nullptr, false);
    CVar::Register("VoiceActivationSensitivity", "Sensitivity of the microphone (0.0 - 1.0)", 0, "0.4", VoiceActivationSensitivityHandler, 5, false, nullptr, false);
#if defined(__APPLE__)
    CVar::Register("EnableMicrophone", "Enables the microphone so you can speak.", 0, "0", EnableMicrophoneHandler, 5, false, nullptr, false);
#else
    CVar::Register("EnableMicrophone", "Enables the microphone so you can speak.", 0, "1", EnableMicrophoneHandler, 5, false, nullptr, false);
#endif
    CVar::Register("EnableVoiceChat", "Enables the voice chat feature.", 0, "0", EnableVoiceChatHandler, 5, false, nullptr, false);
    CVar::Register("VoiceChatSelfMute", "Turn off your ability to talk.", 0, "0", SelfMuteHandler, 5, false, nullptr, false);
    CVar::Register("PushToTalkButton", "String representation of the Push-To-Talk button.", 0, "`", PushToTalkButtonHandler, 5, false, nullptr, false);
    CVar::Register("Sound_OutputQuality", "sound quality, default 1 (medium)", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_NumChannels", "number of sound channels", 0, "32", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableReverb", "", 0, "0", EnableReverbHandler, 7, false, nullptr, false);
    CVar::Register("Sound_EnableSoftwareHRTF", "", 0, "0", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_VoiceChatInputDriverIndex", "", 0, "0", VoiceChatInputDriverIndexHandler, 7, false, nullptr, false);
    CVar::Register("Sound_VoiceChatInputDriverName", "", 0, "Primary Sound Capture Driver", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_VoiceChatOutputDriverIndex", "", 0, "0", VoiceChatOutputDriverIndexHandler, 7, false, nullptr, false);
    CVar::Register("Sound_VoiceChatOutputDriverName", "", 0, "Primary Sound Driver", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_OutputDriverIndex", "", 0, "0", OutputDriverIndexHandler, 7, false, nullptr, false);
    CVar::Register("Sound_OutputDriverName", "", 0, "Primary Sound Driver", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_DSPBufferSize", "sound buffer size, default 0", 0, "0", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableHardware", "Enables Hardware", 0, "0", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableMode2", "test", 0, "0", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableMixMode2", "test", 0, "0", nullptr, 7, false, nullptr, false);
}

// VERIFY: 3f7da3da auto
// OFFSET: 0x004D13E0
static bool EnableAllSoundHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value)) {
        static CVar* s_enableSFX = CVar::Lookup("Sound_EnableSFX");
        static CVar* s_enableMusic = CVar::Lookup("Sound_EnableMusic");
        static CVar* s_enableAmbience = CVar::Lookup("Sound_EnableAmbience");

        SE2::MuteChannelGroup(SI2::s_channelGroupNames[0], s_enableSFX->m_intValue == 0);
        SE2::MuteChannelGroup("MUSIC", s_enableMusic->m_intValue == 0);
        SE2::MuteChannelGroup("AMBIENCE", s_enableAmbience->m_intValue == 0);
        return true;
    }

    SE2::MuteChannelGroup(SI2::s_channelGroupNames[0], true);
    SE2::MuteChannelGroup("MUSIC", true);
    SE2::MuteChannelGroup("AMBIENCE", true);
    return true;
}

// VERIFY: d8927d35 manual
// OFFSET: 0x004D14E0
static bool EnableSFXHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    CVar* enableAllSound = CVar::Lookup("Sound_EnableAllSound");

    if (enableAllSound) {
        SE2::MuteChannelGroup(SI2::s_channelGroupNames[0], !SStrToInt(value) || !enableAllSound->m_intValue);
    }

    return true;
}

// VERIFY: 34387ca8 manual
// OFFSET: 0x004D1530
static bool EnableAmbienceHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    CVar* enableAllSound = CVar::Lookup("Sound_EnableAllSound");

    if (enableAllSound) {
        bool mute = !SStrToInt(value) || !enableAllSound->m_intValue || CGGameUI::m_inMovie;
        SE2::MuteChannelGroup("AMBIENCE", mute);

        if (mute) {
            SI2::StopZoneAmbience();
        }
    }

    return true;
}

// VERIFY: c5c182ec auto
// OFFSET: 0x004D1590
static bool EnableMusicHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    CVar* enableAllSound = CVar::Lookup("Sound_EnableAllSound");

    if (enableAllSound) {
        bool mute = !SStrToInt(value) || !enableAllSound->m_intValue || CGGameUI::m_inMovie;
        SE2::MuteChannelGroup("MUSIC", mute);

        if (mute) {
            SI2::StopAllMusic();
            SI2::StopAIMusic();
            SI2::StopLuaMusic();
        }
    }

    return true;
}

// VERIFY: 7b1b9794 auto
// OFFSET: 0x004D1600
void SI2::RegisterCVars() {
    CVar::Register("ChatMusicVolume", "music volume (0.0 to 1.0)", 0, "0.3", nullptr, 7, false, nullptr, false);
    CVar::Register("ChatSoundVolume", "sound volume (0.0 to 1.0)", 0, "0.4", nullptr, 7, false, nullptr, false);
    CVar::Register("ChatAmbienceVolume", "Ambience Volume (0.0 to 1.0)", 0, "0.3", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableSFX", "", 0, "1", EnableSFXHandler, 7, false, nullptr, false);
    CVar::Register("Sound_EnableAmbience", "Enable Ambience", 0, "1", EnableAmbienceHandler, 7, false, nullptr, false);
    CVar::Register("Sound_EnableErrorSpeech", "error speech", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableMusic", "Enables music", 0, "1", EnableMusicHandler, 7, false, nullptr, false);
    CVar::Register("Sound_EnableAllSound", "", 0, "1", EnableAllSoundHandler, 7, false, nullptr, false);
    CVar::Register("Sound_MasterVolume", "master volume (0.0 to 1.0)", 0, "1.0", MasterVolumeHandler, 7, false, nullptr, false);
    CVar::Register("Sound_SFXVolume", "sound volume (0.0 to 1.0)", 0, "1.0", SFXVolumeHandler, 7, false, nullptr, false);
    CVar::Register("Sound_MusicVolume", "music volume (0.0 to 1.0)", 0, "0.4", MusicVolumeHandler, 7, false, nullptr, false);
    CVar::Register("Sound_AmbienceVolume", "Ambience Volume (0.0 to 1.0)", 0, "0.6", AmbienceVolumeHandler, 7, false, nullptr, false);
    CVar::Register("Sound_ListenerAtCharacter", "lock listener at character", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableEmoteSounds", "", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_ZoneMusicNoDelay", "", 0, "0", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableArmorFoleySoundForSelf", "", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableArmorFoleySoundForOthers", "", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableDSPEffects", "", 0, OsGetNumberOfProcessors() <= 1 ? "0" : "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnablePetSounds", "Enables pet sounds", 0, "1", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_EnableSoundWhenGameIsInBG", "Enable Sound When Game Is In Background", 0, "0", nullptr, 7, false, nullptr, false);

    if (CmdLineGetBool(static_cast<CMDOPT>(27))) {
        CVar::Register("Sound_ChaosMode", "Testing to break sound engine", 0, "0", ChaosModeHandler, 7, false, nullptr, false);
        CVar::Lookup("Sound_ChaosMode")->Set("0", true, false, false, true);
    }

    CVar::Register("Sound_MaxCacheSizeInBytes", "Max cache size in bytes", 0, "16777216", nullptr, 7, false, nullptr, false);
    CVar::Register("Sound_MaxCacheableSizeInBytes", "Max sound size that will be cached, larger files will be streamed instead", 0, "1048576", nullptr, 7, false, nullptr, false);

    static CVar* s_enableAllSound = CVar::Lookup("Sound_EnableAllSound");
    static CVar* s_enableSFX = CVar::Lookup("Sound_EnableSFX");
    static CVar* s_enableErrorSpeech = CVar::Lookup("Sound_EnableErrorSpeech");
    static CVar* s_enableEmoteSounds = CVar::Lookup("Sound_EnableEmoteSounds");
    static CVar* s_enableMusic = CVar::Lookup("Sound_EnableMusic");
    static CVar* s_zoneMusicNoDelay = CVar::Lookup("Sound_ZoneMusicNoDelay");
    static CVar* s_enableAmbience = CVar::Lookup("Sound_EnableAmbience");
    static CVar* s_listenerAtCharacter = CVar::Lookup("Sound_ListenerAtCharacter");
    static CVar* s_enableSoundWhenGameIsInBG = CVar::Lookup("Sound_EnableSoundWhenGameIsInBG");
    static CVar* s_enableReverb = CVar::Lookup("Sound_EnableReverb");
    static CVar* s_enableSoftwareHRTF = CVar::Lookup("Sound_EnableSoftwareHRTF");
    static CVar* s_outputQuality = CVar::Lookup("Sound_OutputQuality");
    static CVar* s_numChannels = CVar::Lookup("Sound_NumChannels");
    static CVar* s_enableHardware = CVar::Lookup("Sound_EnableHardware");
    static CVar* s_masterVolume = CVar::Lookup("Sound_MasterVolume");
    static CVar* s_sfxVolume = CVar::Lookup("Sound_SFXVolume");
    static CVar* s_musicVolume = CVar::Lookup("Sound_MusicVolume");
    static CVar* s_ambienceVolume = CVar::Lookup("Sound_AmbienceVolume");
    static CVar* s_enableMixMode2 = CVar::Lookup("Sound_EnableMixMode2");
    static CVar* s_armorFoleySelf = CVar::Lookup("Sound_EnableArmorFoleySoundForSelf");
    static CVar* s_armorFoleyOthers = CVar::Lookup("Sound_EnableArmorFoleySoundForOthers");
    static CVar* s_dspBufferSize = CVar::Lookup("Sound_DSPBufferSize");

    SE2::Log_Write("=> User Settings Report:");
    SE2::Log_Write(" - ========= PLAYBACK =========");
    SE2::Log_Write(" - Enable All Sound         [%d]", s_enableAllSound->m_intValue);
    SE2::Log_Write(" - Enable SFX               [%d]", s_enableSFX->m_intValue);
    SE2::Log_Write(" --- Enable Error Speech    [%d]", s_enableErrorSpeech->m_intValue);
    SE2::Log_Write(" --- Enable Emote Sounds    [%d]", s_enableEmoteSounds->m_intValue);
    SE2::Log_Write(" - Enable Music             [%d]", s_enableMusic->m_intValue);
    SE2::Log_Write(" --- Loop Music             [%d]", s_zoneMusicNoDelay->m_intValue);
    SE2::Log_Write(" - Enable Ambient Sounds    [%d]", s_enableAmbience->m_intValue);
    SE2::Log_Write(" - Sound at Character       [%d]", s_listenerAtCharacter->m_intValue);
    SE2::Log_Write(" - Sound in Background      [%d]", s_enableSoundWhenGameIsInBG->m_intValue);
    SE2::Log_Write(" - Enable Reverb            [%d]", s_enableReverb->m_intValue);
    SE2::Log_Write(" - Enable Software HRTF     [%d]", s_enableSoftwareHRTF->m_intValue);
    SE2::Log_Write(" - Sound Quality            [%d]", s_outputQuality->m_intValue);
    SE2::Log_Write(" -");
    SE2::Log_Write(" - ========== VOLUME ==========");
    SE2::Log_Write(" - Master Volume         [%.2f]", s_masterVolume->m_floatValue);
    SE2::Log_Write(" - SFX Volume            [%.2f]", s_sfxVolume->m_floatValue);
    SE2::Log_Write(" - Music Volume          [%.2f]", s_musicVolume->m_floatValue);
    SE2::Log_Write(" - Ambience Volume       [%.2f]", s_ambienceVolume->m_floatValue);
    SE2::Log_Write(" -");
    SE2::Log_Write(" - =========== MISC ===========");
    SE2::Log_Write(" - Sound Channels          [%d]", s_numChannels->m_intValue);
    SE2::Log_Write(" - Use Hardware             [%d]", s_enableHardware->m_intValue);
    SE2::Log_Write(" - Mix Mode 2               [%d]", s_enableMixMode2->m_intValue);

    if (s_dspBufferSize->m_intValue) {
        SE2::Log_Write(" - DSP Buffer Size          [%d]", s_enableAmbience->m_intValue);
    } else {
        SE2::Log_Write(" - DSP Buffer Size       [AUTO]");
    }

    SE2::Log_Write(" - Armor Foley SFX (Self)   [%d]", s_armorFoleySelf->m_intValue);
    SE2::Log_Write(" - Armor Foley SFX (Others) [%d]", s_armorFoleyOthers->m_intValue);
    SE2::Log_Write("=> End of User Settings Report");
}
