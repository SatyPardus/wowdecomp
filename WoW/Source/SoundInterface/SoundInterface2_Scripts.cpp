#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/ModCore/ModCore.h"

// VERIFY: 1ef15c2e auto
// OFFSET: 0x009858B0
static int32_t Script_PlaySound(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        SI2::PlayUISound(static_cast<int32_t>(lua_tonumber(L, 1)));
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "Usage: PlaySound(\"sound\")");
        lua_error(L);
    }

    char name[256];
    SStrCopy(name, lua_tolstring(L, 1, nullptr), sizeof(name));
    SI2::PlayUISound(SI2::LookupSoundKitId(name));
    return 0;
}

// VERIFY: 4858a8e8 auto
// OFFSET: 0x00985950
static int32_t Script_PlayMusic(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "Usage: PlayMusic(\"musicfile\")");
        lua_error(L);
    }

    const char* music = lua_tolstring(L, 1, nullptr);
    if (ModCore::Manager().OnPlayMusic(music)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    SI2::PlaySoundFile(music, 5);
    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: ca4839d7 auto
// OFFSET: 0x009859B0
static int32_t Script_PlaySoundFile(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        lua_pushfstring(L, "Usage: PlaySoundFile(\"soundfile\")");
        lua_error(L);
    }

    SI2::PlaySoundFile(lua_tolstring(L, 1, nullptr), 4);
    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 006891da auto
// OFFSET: 0x00985A10
static int32_t Script_StopMusic(lua_State* L) {
    SI2::StopLuaMusic();
    return 0;
}

// VERIFY: 07bc13b7 auto
// OFFSET: 0x00985A20
static int32_t Script_Sound_ChatSystem_GetNumInputDrivers(lua_State* L) {
    lua_pushnumber(L, SI2::GetNumInputDrivers_Cached(1));
    return 1;
}

// VERIFY: 28a8ec83 manual
// OFFSET: 0x00985A50
static int32_t Script_Sound_ChatSystem_GetInputDriverNameByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Sound_ChatSystem_GetInputDriverNameByIndex(InputDriverIndex)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tointeger(L, 1));
    char name[2048];
    memset(name, 0, sizeof(name));
    SI2::GetInputDriverName_Cached(index, name, sizeof(name), 1);
    lua_pushstring(L, name);
    return 1;
}

// VERIFY: 3a2f5e79 auto
// OFFSET: 0x00985AE0
static int32_t Script_Sound_ChatSystem_GetNumOutputDrivers(lua_State* L) {
    lua_pushnumber(L, SI2::GetNumOutputDrivers_Cached(1));
    return 1;
}

// VERIFY: 74c0fb57 manual
// OFFSET: 0x00985B10
static int32_t Script_Sound_ChatSystem_GetOutputDriverNameByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Sound_ChatSystem_GetOutputDriverNameByIndex(OutputDriverIndex)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tointeger(L, 1));
    char name[2048];
    memset(name, 0, sizeof(name));

    if (index >= 0 && index <= SI2::GetNumOutputDrivers_Cached(1)) {
        SI2::GetOutputDriverName_Cached(index, name, sizeof(name), 1);
    }

    lua_pushstring(L, name);
    return 1;
}

// VERIFY: f89b7f68 auto
// OFFSET: 0x00985BB0
static int32_t Script_Sound_GameSystem_GetNumInputDrivers(lua_State* L) {
    lua_pushnumber(L, SI2::GetNumInputDrivers(0));
    return 1;
}

// VERIFY: cbb4519c manual
// OFFSET: 0x00985BE0
static int32_t Script_Sound_GameSystem_GetInputDriverNameByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Sound_GameSystem_GetInputDriverNameByIndex(InputDriverIndex)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tointeger(L, 1));
    char name[2048];
    memset(name, 0, sizeof(name));
    SI2::GetInputDriverName(index, name, sizeof(name), 0);
    lua_pushstring(L, name);
    return 1;
}

// VERIFY: a4609b4f auto
// OFFSET: 0x00985C70
static int32_t Script_Sound_GameSystem_GetNumOutputDrivers(lua_State* L) {
    lua_pushnumber(L, SI2::GetNumOutputDrivers_Cached(0));
    return 1;
}

// VERIFY: d5844aa9 manual
// OFFSET: 0x00985CA0
static int32_t Script_Sound_GameSystem_GetOutputDriverNameByIndex(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: Sound_GetOutputDriverNameByIndex(OutputDriverIndex)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tointeger(L, 1));
    char name[2048];
    memset(name, 0, sizeof(name));
    SI2::GetOutputDriverName_Cached(index, name, sizeof(name), 0);
    lua_pushstring(L, name);
    return 1;
}

// VERIFY: 423ee834 auto
// OFFSET: 0x00985D30
static int32_t Script_Sound_GameSystem_RestartSoundSystem(lua_State* L) {
    SI2::ShutDown(1);
    OsSleep(200);
    SI2::Init(1);
    return 0;
}

// VERIFY: 984c78e6 auto
// OFFSET: 0x00985D50
static int32_t Script_VoiceChat_StopCapture(lua_State* L) {
    SI2::VoiceChat_StopCapture();
    return 0;
}

// VERIFY: e09b1c70 auto
// OFFSET: 0x00985D60
static int32_t Script_VoiceChat_RecordLoopbackSound(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: VoiceChat_RecordLoopbackSound(MaxRecordTimeInSeconds)");
        return 0;
    }

    SI2::VoiceChat_RecordLoopbackSound(static_cast<int32_t>(lua_tonumber(L, 1)));
    return 0;
}

// VERIFY: e7a90da2 auto
// OFFSET: 0x00985DD0
static int32_t Script_VoiceChat_StopRecordingLoopbackSound(lua_State* L) {
    SI2::VoiceChat_StopRecordingLoopbackSound();
    return 0;
}

// VERIFY: 6b24a31b auto
// OFFSET: 0x00985DE0
static int32_t Script_VoiceChat_PlayLoopbackSound(lua_State* L) {
    SI2::VoiceChat_PlayLoopbackSound();
    return 0;
}

// VERIFY: 2be65d3f auto
// OFFSET: 0x00985DF0
static int32_t Script_VoiceChat_StopPlayingLoopbackSound(lua_State* L) {
    SI2::VoiceChat_StopPlayingLoopbackSound();
    return 0;
}

// VERIFY: 86215d48 auto
// OFFSET: 0x00985E00
static int32_t Script_VoiceChat_IsRecordingLoopbackSound(lua_State* L) {
    lua_pushnumber(L, SI2::VoiceChat_IsRecordingLoopbackSound());
    return 1;
}

// VERIFY: 54e707f8 auto
// OFFSET: 0x00985E30
static int32_t Script_VoiceChat_IsPlayingLoopbackSound(lua_State* L) {
    lua_pushnumber(L, SI2::VoiceChat_IsPlayingLoopbackSound());
    return 1;
}

// VERIFY: 19bda182 auto
// OFFSET: 0x00985E60
static int32_t Script_VoiceChat_GetCurrentMicrophoneSignalLevel(lua_State* L) {
    lua_pushnumber(L, SI2::VoiceChat_GetCurrentMicrophoneSignalLevel());
    return 1;
}

// VERIFY: 8173801c auto
// OFFSET: 0x00985E90
static int32_t Script_VoiceChat_ActivatePrimaryCaptureCallback(lua_State* L) {
    SI2::VoiceChat_ActivatePrimaryCaptureCallback();
    return 0;
}

// VERIFY: 7e048fcd auto
// OFFSET: 0x00985EF0
static int32_t Script_VoiceChat_StartCapture(lua_State* L) {
    static CVar* s_enableMicrophone = CVar::Lookup("EnableMicrophone");
    static CVar* s_enableVoiceChat = CVar::Lookup("EnableVoiceChat");

    if (s_enableVoiceChat && s_enableVoiceChat->m_intValue && s_enableMicrophone && s_enableMicrophone->m_intValue && CGChat::IsVoiceChatAllowed()) {
        SI2::VoiceChat_StartCapture();
    }

    return 0;
}

// STATIC: 0x00B2D928
static FrameScript_Method s_ScriptFunctions[] = {
    { "PlaySound", &Script_PlaySound },
    { "PlayMusic", &Script_PlayMusic },
    { "PlaySoundFile", &Script_PlaySoundFile },
    { "StopMusic", &Script_StopMusic },
    { "Sound_GameSystem_GetNumInputDrivers", &Script_Sound_GameSystem_GetNumInputDrivers },
    { "Sound_GameSystem_GetInputDriverNameByIndex", &Script_Sound_GameSystem_GetInputDriverNameByIndex },
    { "Sound_GameSystem_GetNumOutputDrivers", &Script_Sound_GameSystem_GetNumOutputDrivers },
    { "Sound_GameSystem_GetOutputDriverNameByIndex", &Script_Sound_GameSystem_GetOutputDriverNameByIndex },
    { "Sound_GameSystem_RestartSoundSystem", &Script_Sound_GameSystem_RestartSoundSystem },
    { "Sound_ChatSystem_GetNumInputDrivers", &Script_Sound_ChatSystem_GetNumInputDrivers },
    { "Sound_ChatSystem_GetInputDriverNameByIndex", &Script_Sound_ChatSystem_GetInputDriverNameByIndex },
    { "Sound_ChatSystem_GetNumOutputDrivers", &Script_Sound_ChatSystem_GetNumOutputDrivers },
    { "Sound_ChatSystem_GetOutputDriverNameByIndex", &Script_Sound_ChatSystem_GetOutputDriverNameByIndex },
    { "VoiceChat_StartCapture", &Script_VoiceChat_StartCapture },
    { "VoiceChat_StopCapture", &Script_VoiceChat_StopCapture },
    { "VoiceChat_RecordLoopbackSound", &Script_VoiceChat_RecordLoopbackSound },
    { "VoiceChat_StopRecordingLoopbackSound", &Script_VoiceChat_StopRecordingLoopbackSound },
    { "VoiceChat_PlayLoopbackSound", &Script_VoiceChat_PlayLoopbackSound },
    { "VoiceChat_StopPlayingLoopbackSound", &Script_VoiceChat_StopPlayingLoopbackSound },
    { "VoiceChat_IsRecordingLoopbackSound", &Script_VoiceChat_IsRecordingLoopbackSound },
    { "VoiceChat_IsPlayingLoopbackSound", &Script_VoiceChat_IsPlayingLoopbackSound },
    { "VoiceChat_GetCurrentMicrophoneSignalLevel", &Script_VoiceChat_GetCurrentMicrophoneSignalLevel },
    { "VoiceChat_ActivatePrimaryCaptureCallback", &Script_VoiceChat_ActivatePrimaryCaptureCallback },
};

// VERIFY: db289b49 auto
// OFFSET: 0x00985EA0
void SI2::RegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: bd7c6065 auto
// OFFSET: 0x00985ED0
void SI2::UnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
