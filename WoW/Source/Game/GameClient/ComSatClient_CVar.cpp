#include "ComSatClient.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Game/GameClient/ComSatThread.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00D37CF8
CVar* s_cvarEnableVoiceChat;

// STATIC: 0x00D37CFC
CVar* s_cvarEnableMicrophone;

// VERIFY: ed2e7fbe manual
// OFFSET: 0x007DC770
bool EnableMicrophoneHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
#if defined(__APPLE__)
    // Reject attempts to enable an intentionally unavailable feature.
    return SStrToInt(value) == 0;
#else
    if (!s_cvarEnableVoiceChat) {
        return true;
    }

    int32_t voiceChatEnabled = s_cvarEnableVoiceChat->m_intValue;
    int32_t microphoneEnabled = SStrToInt(value);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_VOICE_SESSION_ENABLE));
    msg.Put(static_cast<uint8_t>(voiceChatEnabled != 0));
    msg.Put(static_cast<uint8_t>(microphoneEnabled != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (voiceChatEnabled && microphoneEnabled && CGChat::IsVoiceChatAllowed()) {
        SI2::VoiceChat_StartCapture();
    } else {
        SI2::VoiceChat_StopCapture();
    }

    return true;
#endif
}

// VERIFY: 02faa7c5 auto
// OFFSET: 0x007DCEC0
bool InboundChatVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    float volume = SStrToFloat(value);

    if (ComSatThread::s_comsat) {
        for (COMSATDATA* data = s_comsatData; data < s_comsatData + sizeof(s_comsatData) / sizeof(s_comsatData[0]); data++) {
            if (data->m_sessionID) {
                AddComSatEvent(1, volume, data->m_sessionID);
            }
        }
    }

    return true;
}

// VERIFY: d2b1377b auto
// OFFSET: 0x007DCF20
bool VoiceChatModeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value) == 1) {
        s_voiceActivation = 1;
        Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_SELF_MUTE);

        CVar* selfMute = CVar::Lookup("VoiceChatSelfMute");

        if (selfMute && selfMute->m_intValue) {
            AddComSatEvent(2, 0u, 0);
        } else {
            AddComSatEvent(2, 1u, 0);
        }

        CVar* sensitivity = CVar::Lookup("VoiceActivationSensitivity");

        if (sensitivity) {
            float level = sensitivity->m_floatValue;

            level = level < 0.0f ? 0.0f : (level < 0.99f ? level : 0.99f);

            AddComSatEvent(3, level, 0);
        }
    } else {
        s_voiceActivation = 0;
        AddComSatEvent(2, 0u, 0);
        AddComSatEvent(3, 0.0f, 0);
    }

    return true;
}

// VERIFY: a9de6883 auto
// OFFSET: 0x007DCFE0
bool VoiceActivationSensitivityHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    CVar* voiceChatMode = CVar::Lookup("VoiceChatMode");

    if (voiceChatMode && voiceChatMode->m_intValue == 1) {
        float level = SStrToFloat(value);

        if (level < 0.0f) {
            level = 0.0f;
        } else if (level >= 0.99f) {
            level = 0.99f;
        }

        AddComSatEvent(3, level, 0);
    }

    return true;
}

// VERIFY: daea7ad1 auto
// OFFSET: 0x007DD040
bool PushToTalkButtonHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    AddComSatEvent(10, 0u, 0);

    return true;
}

// VERIFY: ad238b9c manual
// OFFSET: 0x007DD060
bool SelfMuteHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value)) {
        AddComSatEvent(2, 0u, 0);
    } else {
        AddComSatEvent(2, 1u, 0);
    }

    Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_SELF_MUTE);

    return true;
}

// VERIFY: 041fa9fd manual
// OFFSET: 0x007DFEA0
bool EnableVoiceChatHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
#if defined(__APPLE__)
    // Reject attempts to enable an intentionally unavailable feature.
    return SStrToInt(value) == 0;
#else
    bool wasEnabled = oldValue && SStrToInt(oldValue);
    bool enabled = SStrToInt(value) != 0;
    bool microphoneEnabled = s_cvarEnableMicrophone && s_cvarEnableMicrophone->m_intValue;

    static CVar* s_voiceChatMode = CVar::Lookup("VoiceChatMode");

    if (!wasEnabled && enabled) {
        ComSatClient_Init();

        if (s_voiceChatMode) {
            DebugLog("!!!!!!!!!! HACK\n");
            s_voiceChatMode->Set(s_voiceChatMode->m_intValue ? "0" : "1", true, false, false, true);
            s_voiceChatMode->Set(s_voiceChatMode->m_intValue ? "0" : "1", true, false, false, true);
        }
    }

    if (s_cvarEnableMicrophone) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_VOICE_SESSION_ENABLE));
        msg.Put(static_cast<uint8_t>(enabled));
        msg.Put(static_cast<uint8_t>(microphoneEnabled));
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    if (enabled) {
        s_voiceChatWasEnabled = 1;
    }

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_CHAT_ENABLED_UPDATE, nullptr);
    }

    if (enabled && microphoneEnabled && CGChat::IsVoiceChatAllowed()) {
        SI2::VoiceChat_StartCapture();
    } else {
        SI2::VoiceChat_StopCapture();
    }

    if (wasEnabled && !enabled) {
        ComSatClient_Shutdown();
    }

    return true;
#endif
}

bool OutboundChatVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg) {
    return true;
}
