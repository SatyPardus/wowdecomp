#include "ComSatClient.h"
#include "Engine/Source/SoundEngine/SoundEngine.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Os/Processor.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/ComSat/ComSatInterface.h"
#include <cstring>
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include <new>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "WoW/Source/Game/GameClient/ComSatThread.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"

// STATIC: 0x00D31AA0
COMSATDATA s_comsatData[32];

// STATIC: 0x00D37CA0
int32_t s_voiceActivation;

// STATIC: 0x00AF2A48
static ComSatClientAllocator s_comsatAllocator;

// STATIC: 0x00AF2A4C
static ComSatClientCallback s_comsatCallback;

// STATIC: 0x00D37CA4
static float s_voiceFadeStartTime;

// STATIC: 0x00D37CA8
static float s_voiceFadeDelayRate;

// STATIC: 0x00D37CAC
static float s_voiceFadeRate;

// STATIC: 0x00D37CB0
static int32_t s_talkingPlayerCount;

// STATIC: 0x00D37CB4
static uint8_t s_voiceFadeActive;

// STATIC: 0x00D37CB8
static CVar* s_cvarSoundSFXVolume;

// STATIC: 0x00D37CBC
static CVar* s_cvarSoundMusicVolume;

// STATIC: 0x00D37CC0
static CVar* s_cvarSoundAmbienceVolume;

// STATIC: 0x00D37CC4
static CVar* s_cvarChatSoundVolume;

// STATIC: 0x00D37CC8
static CVar* s_cvarChatMusicVolume;

// STATIC: 0x00D37CCC
static CVar* s_cvarChatAmbienceVolume;

// STATIC: 0x00D37CD0
static float s_sfxVolumeBase;

// STATIC: 0x00D37CD4
static float s_musicVolumeBase;

// STATIC: 0x00D37CD8
static float s_ambienceVolumeBase;

// STATIC: 0x00D37CDC
static float s_sfxVolumeDelta;

// STATIC: 0x00D37CE0
static float s_musicVolumeDelta;

// STATIC: 0x00D37CE4
static float s_ambienceVolumeDelta;

// STATIC: 0x00AF29B8
uint32_t s_comsatSleepTime = 10;

// STATIC: 0x00AF29BC
int32_t s_pushToTalkKey = -1;

// STATIC: 0x00D37CE8
static CVar* s_cvarStartTalkingDelay;

// STATIC: 0x00D37CEC
static CVar* s_cvarStartTalkingTime;

// STATIC: 0x00D37CF0
static CVar* s_cvarStopTalkingDelay;

// STATIC: 0x00D37CF4
static CVar* s_cvarStopTalkingTime;

// STATIC: 0x00D37D08
static uint32_t s_comsatIdleTimeout;

// STATIC: 0x00D37D0C
static CVar* s_cvarComSatSleepTime;

// STATIC: 0x00D31A94
static int32_t s_lastPushToTalkDown;

// STATIC: 0x00D31A98
static int32_t s_localTalking;

// STATIC: 0x00D31A88
uint32_t s_pushToTalkModifiers;

// STATIC: 0x00D37D00
CVar* s_cvarPushToTalkButton;

// STATIC: 0x00D37D04
int32_t s_voiceChatWasEnabled;

// STATIC: 0x00D37D1C
uint32_t s_pushToTalkMouseButton;

// STATIC: 0x00D37D34
static TSGrowableArray<ComSatCallback> s_comsatCallbacks;

// STATIC: 0x00D37D54
static SMutex s_comsatClientMutex;

// STATIC: 0x00D37D88
TSHashTable<TALKINGPLAYER, CHashKeyGUID> s_talkingPlayers;

// STATIC: 0x00D37DB0
static TSHashTable<MUTEDPLAYER, CHashKeyGUID> s_mutedPlayers;

// STATIC: 0x00D37D44
static TSGrowableArray<ComSatEvent> s_comSatEvents;

// STATIC: 0x00D37D10
static WOWGUID s_localPlayer;

// STATIC: 0x00D37D18
int32_t s_comsatDisabledByClient;

// STATIC: 0x00D37D5C
SCritSect s_comsatDeviceLock;

// STATIC: 0x00D37DD8
static int32_t s_lastFoundSession;

// STATIC: 0x00AF29B4
int32_t s_currentSession = -1;

// VERIFY: df726b6e auto
// OFFSET: 0x007DBC70
static int32_t ParseKeyNameToKeyCode(const char* name) {
    if (!name || !*name) {
        return -1;
    }

    if (!name[1] && name[0] > ' ') {
        return name[0];
    }

    if (!SStrCmpI(name, "LSHIFT", STORM_MAX_STR)) {
        return KEY_LSHIFT;
    }

    if (!SStrCmpI(name, "RSHIFT", STORM_MAX_STR)) {
        return KEY_RSHIFT;
    }

    if (!SStrCmpI(name, "LCTRL", STORM_MAX_STR)) {
        return KEY_LCONTROL;
    }

    if (!SStrCmpI(name, "RCTRL", STORM_MAX_STR)) {
        return KEY_RCONTROL;
    }

    if (!SStrCmpI(name, "LALT", STORM_MAX_STR)) {
        return KEY_LALT;
    }

    if (!SStrCmpI(name, "RALT", STORM_MAX_STR)) {
        return KEY_RALT;
    }

    if (!SStrCmpI(name, "SPACE", STORM_MAX_STR)) {
        return KEY_SPACE;
    }

    if (!SStrCmpI(name, "NUMPADPLUS", STORM_MAX_STR)) {
        return KEY_NUMPAD_PLUS;
    }

    if (!SStrCmpI(name, "NUMPADMINUS", STORM_MAX_STR)) {
        return KEY_NUMPAD_MINUS;
    }

    if (!SStrCmpI(name, "NUMPADMULTIPLY", STORM_MAX_STR)) {
        return KEY_NUMPAD_MULTIPLY;
    }

    if (!SStrCmpI(name, "NUMPADDIVIDE", STORM_MAX_STR)) {
        return KEY_NUMPAD_DIVIDE;
    }

    if (!SStrCmpI(name, "NUMPADDECIMAL", STORM_MAX_STR)) {
        return KEY_NUMPAD_DECIMAL;
    }

    if (!SStrCmpI(name, "NUMPADEQUALS", STORM_MAX_STR)) {
        return KEY_NUMPAD_EQUALS;
    }

    if (SStrStrI(name, "NUMPAD")) {
        if (name[6] >= 0 && name[6] <= '9') {
            return name[6] - '0' + KEY_NUMPAD0;
        }

        return -1;
    }

    if (!SStrCmpI(name, "ESCAPE", STORM_MAX_STR)) {
        return KEY_ESCAPE;
    }

    if (!SStrCmpI(name, "ENTER", STORM_MAX_STR)) {
        return KEY_ENTER;
    }

    if (!SStrCmpI(name, "BACKSPACE", STORM_MAX_STR)) {
        return KEY_BACKSPACE;
    }

    if (!SStrCmpI(name, "TAB", STORM_MAX_STR)) {
        return KEY_TAB;
    }

    if (!SStrCmpI(name, "LEFT", STORM_MAX_STR)) {
        return KEY_LEFT;
    }

    if (!SStrCmpI(name, "UP", STORM_MAX_STR)) {
        return KEY_UP;
    }

    if (!SStrCmpI(name, "RIGHT", STORM_MAX_STR)) {
        return KEY_RIGHT;
    }

    if (!SStrCmpI(name, "DOWN", STORM_MAX_STR)) {
        return KEY_DOWN;
    }

    if (!SStrCmpI(name, "INSERT", STORM_MAX_STR)) {
        return KEY_INSERT;
    }

    if (!SStrCmpI(name, "DELETE", STORM_MAX_STR)) {
        return KEY_DELETE;
    }

    if (!SStrCmpI(name, "HOME", STORM_MAX_STR)) {
        return KEY_HOME;
    }

    if (!SStrCmpI(name, "END", STORM_MAX_STR)) {
        return KEY_END;
    }

    if (!SStrCmpI(name, "PAGEUP", STORM_MAX_STR)) {
        return KEY_PAGEUP;
    }

    if (!SStrCmpI(name, "PAGEDOWN", STORM_MAX_STR)) {
        return KEY_PAGEDOWN;
    }

    if (!SStrCmpI(name, "NUMLOCK", STORM_MAX_STR)) {
        return KEY_NUMLOCK;
    }

    if (!SStrCmpI(name, "CAPSLOCK", STORM_MAX_STR)) {
        return KEY_CAPSLOCK;
    }

    if (!SStrCmpI(name, "PRINTSCREEN", STORM_MAX_STR)) {
        return KEY_PRINTSCREEN;
    }

    if (*name == 'F') {
        return SStrToInt(name + 1) + KEY_F1 - 1;
    }

    return -1;
}

// VERIFY: 4b73a2c5 manual
// OFFSET: 0x007DCA90
void AddComSatEvent(uint32_t type, float value, uint64_t guid) {
    ComSatEvent* event = s_comSatEvents.New();

    if (event) {
        event->m_floatValue = value;
        event->m_type = type;
        event->m_guid = guid;
    }
}

// VERIFY: ebc8d780 manual
// OFFSET: 0x007DCB10
void AddComSatEvent(uint32_t type, uint32_t value, uint64_t guid) {
    ComSatEvent* event = s_comSatEvents.New();

    if (event) {
        event->m_type = type;
        event->m_guid = guid;
        event->m_uintValue = value;
    }
}

// VERIFY: 80427863 auto
// OFFSET: 0x007DC5C0
static void ResetVoiceVolumeState() {
    s_voiceFadeStartTime = 0.0f;
    s_voiceFadeDelayRate = 0.0f;
    s_talkingPlayerCount = 0;
    s_voiceFadeRate = 0.0f;
    s_sfxVolumeDelta = 0.0f;
    s_musicVolumeDelta = 0.0f;
    s_ambienceVolumeDelta = 0.0f;

    if (s_cvarSoundSFXVolume) {
        s_sfxVolumeBase = s_cvarSoundSFXVolume->m_floatValue;
        SI2::SetChannelGroupVolume(0, s_sfxVolumeBase);
    } else {
        s_sfxVolumeBase = 0.0f;
    }

    if (s_cvarSoundMusicVolume) {
        s_musicVolumeBase = s_cvarSoundMusicVolume->m_floatValue;
        SI2::SetChannelGroupVolume(1, s_musicVolumeBase);
    } else {
        s_musicVolumeBase = 0.0f;
    }

    if (s_cvarSoundAmbienceVolume) {
        s_ambienceVolumeBase = s_cvarSoundAmbienceVolume->m_floatValue;
        SI2::SetChannelGroupVolume(2, s_ambienceVolumeBase);
    } else {
        s_ambienceVolumeBase = 0.0f;
    }
}

// VERIFY: 3564923b manual
// OFFSET: 0x007DC730
void* ComSatClientAllocator::Alloc(uint32_t size) {
    return SMemAlloc(size, __FILE__, __LINE__, 0);
}

// VERIFY: 0d60f70a manual
// OFFSET: 0x007DC750
void ComSatClientAllocator::Free(void* ptr) {
    SMemFree(ptr, __FILE__, __LINE__, 0);
}

// VERIFY: bf8e209f manual
// OFFSET: 0x007DF770
static void ProcessComSatCallbacks() {
    for (uint32_t i = 0; i < s_comsatCallbacks.Count(); i++) {
        ComSatCallback& callback = s_comsatCallbacks[i];

        switch (callback.m_type) {
            case 0:
                Comsat::VoiceStart();
                break;

            case 1:
                Comsat::VoiceStop();
                break;

            case 2:
                Comsat::VoicePlateStart(callback.m_sessionId, callback.m_guid);
                break;

            case 3:
                Comsat::VoicePlateStop(callback.m_sessionId, callback.m_guid);
                break;

            default:
                break;
        }
    }

    s_comsatCallbacks.Clear();
}

// VERIFY: 870a6cad auto
// OFFSET: 0x007DF900
void ComSatClient_Shutdown() {
    if (ComSatThread::s_threadRunning) {
        ComSatThread::s_threadRunning = 0;
        ComSatThread::s_threadShutdownEvent.Set();
        ComSatThread::s_comsatThread.Wait(0xFFFFFFFF);
    }

    ProcessComSatCallbacks();
    EventUnregister(EVENT_ID_IDLE, VoiceChat_TickWrapper);
    ComSat::Interface::Destroy(ComSatThread::s_comsat, s_comsatAllocator);
    ComSatThread::s_comsat = nullptr;
    s_mutedPlayers.InternalClear(0);
    s_talkingPlayers.InternalClear(0);

    if (s_comsatClientMutex.Valid()) {
        s_comsatClientMutex.Close();
        s_comsatClientMutex.Release();
    }

    ResetVoiceVolumeState();
    SI2::VoiceChat_DestroyChatSystem();
}

// VERIFY: 9c9c6262 auto
// OFFSET: 0x007DC9C0
void UpdatePushToTalkKey() {
    s_pushToTalkKey = -1;
    s_pushToTalkMouseButton = 0;
    s_pushToTalkModifiers = 0;

    if (!s_cvarPushToTalkButton) {
        return;
    }

    const char* binding = s_cvarPushToTalkButton->m_stringValue.m_str;

    if (!binding || !*binding) {
        return;
    }

    s_pushToTalkModifiers = 0;
    CGUIBindings::AddModifiers(s_pushToTalkModifiers, binding);
    s_pushToTalkMouseButton = StringToMouseButton(binding);

    if (!s_pushToTalkMouseButton) {
        s_pushToTalkKey = ParseKeyNameToKeyCode(binding);
    }
}

// VERIFY: 968200fd manual
// OFFSET: 0x007DB600
int32_t ComSatClient_IsTalkerSquelched(WOWGUID guid, uint32_t session) {
    if (!guid || session > 31) {
        return 0;
    }

    s_comsatDeviceLock.Enter();
    COMSATDATA& data = s_comsatData[session];

    for (int32_t i = 0; i < static_cast<int32_t>(data.m_numMembers); i++) {
        if (data.m_members[i].m_guid == guid) {
            int32_t result = data.m_members[i].m_flags & 0x8;
            s_comsatDeviceLock.Leave();
            return result;
        }
    }

    s_comsatDeviceLock.Leave();
    return 0;
}

// VERIFY: 1024d571 manual
// OFFSET: 0x007DB6B0
int32_t ComSatClient_IsTalkerSquelchedInChannel(WOWGUID guid, int32_t type, const char* name) {
    int32_t session = ComSatClient_FindSessionByTypeAndName(type, name);
    return session >= 0 ? ComSatClient_IsTalkerSquelched(guid, session) : 0;
}

// VERIFY: 75ebd9f3 manual
// OFFSET: 0x007DB6E0
int32_t ComSatClient_IsMemberVoiceActive(WOWGUID guid, uint32_t session) {
    if (!guid || session > 31) {
        return 0;
    }

    s_comsatDeviceLock.Enter();
    COMSATDATA& data = s_comsatData[session];

    for (int32_t i = 0; i < static_cast<int32_t>(data.m_numMembers); i++) {
        if (data.m_members[i].m_guid == guid) {
            int32_t result = data.m_members[i].m_flags & 0x40;
            s_comsatDeviceLock.Leave();
            return result;
        }
    }

    s_comsatDeviceLock.Leave();
    return 0;
}

// VERIFY: cc56d1d4 manual
// OFFSET: 0x007DB770
int32_t ComSatClient_IsMemberInVoiceSession(WOWGUID guid, int32_t type, const char* name) {
    int32_t session = ComSatClient_FindSessionByTypeAndName(type, name);
    return session >= 0 ? ComSatClient_IsMemberVoiceActive(guid, session) : 0;
}

// VERIFY: 8c8597f1 manual
// OFFSET: 0x007DB7A0
int32_t ComSatClient_GetMemberVoiceStatus(WOWGUID guid, uint32_t session) {
    if (!guid || session > 31) {
        return 0;
    }

    s_comsatDeviceLock.Enter();
    COMSATDATA& data = s_comsatData[session];

    for (int32_t i = 0; i < static_cast<int32_t>(data.m_numMembers); i++) {
        if (data.m_members[i].m_guid == guid) {
            int32_t result = data.m_members[i].m_flags & 0x4;
            s_comsatDeviceLock.Leave();
            return result;
        }
    }

    s_comsatDeviceLock.Leave();
    return 0;
}

// VERIFY: d2bfb55b manual
// OFFSET: 0x007DB830
int32_t ComSatClient_GetChannelMemberVoiceStatus(WOWGUID guid, int32_t type, const char* name) {
    int32_t session = ComSatClient_FindSessionByTypeAndName(type, name);
    return session >= 0 ? ComSatClient_GetMemberVoiceStatus(guid, session) : 0;
}

// VERIFY: bc5e270c auto
// OFFSET: 0x007DBAB0
int32_t ComSatClient_IsComsatDisabledByClient() {
    return s_comsatDisabledByClient;
}

// VERIFY: fe028376 auto
// OFFSET: 0x007DADE0
void ComSatClient_SetOutputDevice(int32_t index) {
    ComSat::DeviceInfo devices[20];
    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (comsat && index < comsat->EnumerateOutputDevices(devices, 20)) {
        comsat->LocalTalkerChangeOutputDevice(0, index);
    }

    s_comsatDeviceLock.Leave();
}

// VERIFY: a22afe03 auto
// OFFSET: 0x007DAF30
void ComSatClient_SetCaptureDevice(int32_t index) {
    ComSat::DeviceInfo devices[20];
    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (comsat && index < comsat->EnumerateCaptureDevices(devices, 20)) {
        comsat->LocalTalkerChangeCaptureDevice(0, index);
    }

    s_comsatDeviceLock.Leave();
}

// VERIFY: 6dafafe1 manual
// OFFSET: 0x007DB5C0
int32_t ComSatClient_GetCurrentSessionType() {
    if (s_currentSession >= 0) {
        return s_comsatData[s_currentSession].m_type;
    }

    return 4;
}

// VERIFY: 706dd5de manual
// OFFSET: 0x007DB5E0
const char* ComSatClient_GetCurrentSessionName() {
    if (s_currentSession >= 0) {
        return s_comsatData[s_currentSession].m_name;
    }

    return nullptr;
}

// VERIFY: a1da55e8 manual
// OFFSET: 0x007DB560
int32_t ComSatClient_HasSession(int32_t type, const char* name) {
    return ComSatClient_FindSessionByTypeAndName(type, name) >= 0;
}

// VERIFY: 1f2e0bf3 manual
// OFFSET: 0x007DB400
int32_t ComSatClient_FindSessionByTypeAndName(int32_t type, const char* name) {
    int32_t cached = s_lastFoundSession;

    if (s_comsatData[cached].m_type == type) {
        if (type) {
            return cached;
        }

        if (!SStrCmpUTF8I(s_comsatData[cached].m_name, name, 0x7FFFFFFF)) {
            return s_lastFoundSession;
        }
    }

    if (!type && !name) {
        return -1;
    }

    for (int32_t i = 0; i < 32; i++) {
        COMSATDATA* data = &s_comsatData[i];

        if (data->m_sessionID && data->m_type == type && (type || !SStrCmpUTF8I(data->m_name, name, 0x7FFFFFFF))) {
            s_lastFoundSession = i;
            return i;
        }
    }

    return -1;
}

// VERIFY: 43d91719 auto
// OFFSET: 0x007DB4A0
static void NameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        ComSatClient_SendEvent(static_cast<int32_t>(reinterpret_cast<intptr_t>(param)), guid);
    }
}

// VERIFY: 9683c504 auto
// OFFSET: 0x007DB4D0
void ComSatClient_SendEvent(int32_t event, WOWGUID guid) {
    if (!CGGameUI::m_initialized) {
        return;
    }

    const char* name = reinterpret_cast<const char*>(g_nameDBCache.GetRecord(guid.m_guid, guid, NameQueryCallback, reinterpret_cast<void*>(static_cast<intptr_t>(event)), false));

    if (!name) {
        name = FrameScript_GetText("UNKNOWN", -1, GENDER_NOT_APPLICABLE);
    }

    const char* token = Script_GetTokenFromGUID(&guid);

    if (!name || !*name) {
        return;
    }

    if (token) {
        FrameScript_SignalEvent(event, "%s%s", name, token);
    } else {
        FrameScript_SignalEvent(event, "%s", name);
    }
}

// VERIFY: e9df536e auto
// OFFSET: 0x007DEEC0
bool ComSatClient_IsPlayerMuted(WOWGUID guid) {
    return s_mutedPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid)) != nullptr;
}

// VERIFY: b6621cc2 manual
// OFFSET: 0x007DF150
static void ComSatClient_SetCurrentSessionI(int32_t index) {
    int32_t type;
    const char* name;

    if (index == -1) {
        AddComSatEvent(5, 0u, 0ull);
        s_currentSession = -1;
        type = 4;
        name = "";
        WOWGUID player = ClntObjMgrGetActivePlayer();

        if (s_talkingPlayers.Ptr(static_cast<uint32_t>(player.m_guid), CHashKeyGUID(player.m_guid))) {
            Script_SendUnitSignal(player, SCRIPT_EVENT_VOICE_STOP);
            ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, player);
        }
    } else {
        AddComSatEvent(5, 0u, s_comsatData[index].m_sessionID);
        s_currentSession = index;
        type = s_comsatData[index].m_type;
        name = s_comsatData[index].m_name;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_ACTIVE_VOICE_CHANNEL));
    msg.Put(static_cast<uint32_t>(type));
    msg.PutString(name);
    msg.Finalize();
    ClientServices::Send2(&msg);

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_SESSIONS_UPDATE, nullptr);
    }
}

// VERIFY: 26a86cb9 manual
// OFFSET: 0x007DF290
int32_t ComSatClient_SetCurrentSession(int32_t type, const char* name) {
    int32_t index = ComSatClient_FindSessionByTypeAndName(type, name);

    if (index < 0) {
        return 0;
    }

    ComSatClient_SetCurrentSessionI(index);
    return 1;
}

// VERIFY: 4c0e0114 manual
// OFFSET: 0x007DF2C0
void ComSatClient_DisableCurrentSession() {
    ComSatClient_SetCurrentSessionI(-1);
}

// VERIFY: ee86b5ab auto
// OFFSET: 0x007DB690
int32_t ComSatClient_IsTalkerSquelchedInCurrent(WOWGUID guid) {
    return ComSatClient_IsTalkerSquelched(guid, s_currentSession);
}

// STATIC: 0x00ADC098
const char* s_sessionTypeNames[3] = { "Battleground", "Party", "Raid" };

// VERIFY: c896b767 auto
// OFFSET: 0x007DB860
int32_t ComSatClient_DetermineSessionTypeFromName(const char* name) {
    if (!name || !*name) {
        return 4;
    }

    if (!SStrCmpI(name, s_sessionTypeNames[1], STORM_MAX_STR)) {
        return 2;
    }

    if (!SStrCmpI(name, s_sessionTypeNames[2], STORM_MAX_STR)) {
        return 3;
    }

    return !SStrCmpI(name, s_sessionTypeNames[0], STORM_MAX_STR);
}

// VERIFY: 6730c4d0 auto
// OFFSET: 0x007DEB00
static void ComSatClient_LeaveSessionNotifyAll(int32_t index) {
    for (int32_t i = 0; i < static_cast<int32_t>(s_comsatData[index].m_numMembers); i++) {
        WOWGUID guid = s_comsatData[index].m_members[i].m_guid;

        if (!guid || guid == ClntObjMgrGetActivePlayer()) {
            continue;
        }

        Script_SendUnitSignal(guid, SCRIPT_EVENT_VOICE_STOP);
        ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, guid);
        TALKINGPLAYER* talker = s_talkingPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

        if (talker) {
            s_talkingPlayers.Delete(talker);
        }

        if (--s_talkingPlayerCount < 0) {
            s_talkingPlayerCount = 0;
        }
    }
}

// VERIFY: 3d78fa9a manual
// OFFSET: 0x007DED60
void ComSatClient_LeaveAllSessions(WOWGUID guid) {
    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (comsat) {
        int32_t index = 0;

        while (!static_cast<uint32_t>(s_comsatData[index].m_sessionID)) {
            if (++index >= 32) {
                break;
            }
        }

        if (index < 32) {
            comsat->SessionRemove(s_comsatData[index].m_sessionID, guid.m_guid);
            comsat->SessionDestroy(s_comsatData[index].m_sessionID);
            ComSatClient_LeaveSessionNotifyAll(index);
            s_comsatData[index].m_sessionID = 0;
            s_comsatData[index].m_name[0] = 0;
            s_comsatData[index].m_type = 4;
            s_comsatData[index].m_numMembers = 0;

            if (s_currentSession == index) {
                s_currentSession = -1;
                WOWGUID player = ClntObjMgrGetActivePlayer();

                if (s_talkingPlayers.Ptr(static_cast<uint32_t>(player.m_guid), CHashKeyGUID(player.m_guid))) {
                    Script_SendUnitSignal(player, SCRIPT_EVENT_VOICE_STOP);
                    ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, player);
                }
            }
        }

        if (CGGameUI::m_initialized) {
            FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_LEFT_SESSION, nullptr);

            if (CGGameUI::m_initialized) {
                FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_SESSIONS_UPDATE, nullptr);
            }
        }
    }

    s_comsatDeviceLock.Leave();
}

// VERIFY: 02135c1e manual
// OFFSET: 0x007DB580
int32_t ComSatClient_IsCurrentSession(uint64_t session) {
    return s_currentSession >= 0 && s_comsatData[s_currentSession].m_sessionID == session;
}

// VERIFY: 242ea58b manual
// OFFSET: 0x007DC830
static bool FillComsatData(uint64_t session, int32_t type, const char* name) {
    int32_t index = -1;
    int32_t i;

    for (i = 0; i < 32; i++) {
        if (s_comsatData[i].m_sessionID == session) {
            break;
        }

        if (index == -1 && !static_cast<uint32_t>(s_comsatData[i].m_sessionID)) {
            index = i;
        }
    }

    if (i < 32) {
        index = i;
        s_comsatData[index].m_sessionID = 0;
    }

    if (index == -1) {
        return false;
    }

    s_comsatData[index].m_sessionID = session;
    s_comsatData[index].m_type = type;
    s_comsatData[index].m_numMembers = 0;

    if (type) {
        s_comsatData[index].m_name[0] = 0;
    } else {
        SStrCopy(s_comsatData[index].m_name, name, sizeof(s_comsatData[index].m_name));
    }

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_SESSIONS_UPDATE, nullptr);
    }

    CGChat::NewVoiceSession(name, type);
    return true;
}

// VERIFY: 75f2517c auto
// OFFSET: 0x007DD6F0
static bool ComSatClient_IsPlayerTalking(WOWGUID guid) {
    return s_talkingPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid)) != nullptr;
}

// VERIFY: 6f6aa26e auto
// OFFSET: 0x007DE440
void ComSatClient_PopulateSessionDataWithoutJoin(uint64_t session, int32_t type, const char* name, uint64_t self) {
    if (!name) {
        return;
    }

    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (!comsat) {
        s_comsatDeviceLock.Leave();
        return;
    }

    if (comsat->SessionCreate(session)) {
        comsat->SessionAddLocal(session, 0, self);

        if (!FillComsatData(session, type, name)) {
            s_comsatDeviceLock.Leave();
            return;
        }

        if (type == 2 || type == 3 || type == 1) {
            Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_STATUS_UPDATE);
        }

        WOWGUID player = ClntObjMgrGetActivePlayer();

        if (ComSatClient_IsPlayerTalking(player)) {
            Script_SendUnitSignal(player, SCRIPT_EVENT_VOICE_START);
            ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_START, player);
        }

        if ((type == 2 || type == 3 || type == 1) && (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers)) {
            CGChat::JoiningParty(type == 3 || type == 1, type == 1);
        }
    }

    CGChat::VoiceSessionUpdate(type ? "" : name, type);
    s_comsatDeviceLock.Leave();
}

// VERIFY: 1177c447 manual
// OFFSET: 0x007DE5A0
void VoiceSessionRosterUpdate(uint64_t session, uint16_t networkId, int32_t type, const char* name, const uint8_t* key, uint64_t self, uint8_t selfNetworkId, uint8_t selfFlags, int32_t count, VoiceRosterMember* members) {
    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (!comsat || !name) {
        s_comsatDeviceLock.Leave();
        return;
    }

    if (comsat->SessionCreate(session)) {
        comsat->SessionAddLocal(session, 0, self);

        if (!FillComsatData(session, type, name)) {
            s_comsatDeviceLock.Leave();
            return;
        }

        WOWGUID player = ClntObjMgrGetActivePlayer();

        if (ComSatClient_IsPlayerTalking(player)) {
            Script_SendUnitSignal(player, SCRIPT_EVENT_VOICE_START);
            ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_START, player);
        }

        if ((type == 2 || type == 3 || type == 1) && (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers)) {
            CGChat::JoiningParty(type == 3 || type == 1, type == 1);
        }
    }

    comsat->SessionSetNetworkId(session, networkId);
    comsat->SessionSetTalkerNetworkIdLocal(session, self, selfNetworkId);
    int32_t index;

    for (index = 0; index < 32; index++) {
        if (s_comsatData[index].m_sessionID == session) {
            break;
        }
    }

    if (index < 32 && s_comsatData[index].m_type != type) {
        s_comsatData[index].m_type = type;

        if (type) {
            s_comsatData[index].m_name[0] = 0;
        } else {
            SStrCopy(s_comsatData[index].m_name, name, sizeof(s_comsatData[index].m_name));
        }
    }

    int32_t talkers = comsat->SessionNumMembers(session);

    for (int32_t i = 0; i < talkers; i++) {
        uint64_t talker;
        uint8_t flags;

        if (!comsat->SessionGetTalkerByIndex(session, i, talker, flags) || flags != 0xFF) {
            continue;
        }

        int32_t j = 0;

        if (count >= 0) {
            for (; j <= count; j++) {
                if (members[j].m_guid.m_guid == talker) {
                    break;
                }
            }
        }

        if (j != count) {
            continue;
        }

        comsat->SessionRemove(session, talker);
        i--;
        talkers--;

        if (s_talkingPlayers.Ptr(static_cast<uint32_t>(talker), CHashKeyGUID(talker))) {
            WOWGUID guid;
            guid.m_guid = talker;
            Script_SendUnitSignal(guid, SCRIPT_EVENT_VOICE_STOP);
            ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, guid);
            TALKINGPLAYER* talking = s_talkingPlayers.Ptr(static_cast<uint32_t>(talker), CHashKeyGUID(talker));

            if (talking) {
                s_talkingPlayers.Delete(talking);
            }
        }
    }

    uint8_t sessionKey[16];
    memcpy(sessionKey, key, sizeof(sessionKey));

    for (int32_t i = 0; i < count; i++) {
        comsat->SessionAddRemote(session, members[i].m_guid.m_guid, members[i].m_priority, sessionKey, members[i].m_address);
        comsat->SessionSetTalkerPriority(session, members[i].m_guid.m_guid, members[i].m_talkerPriority);
    }

    COMSATDATA* data = &s_comsatData[index];
    data->m_members[0].m_guid.m_guid = self;
    data->m_members[0].m_flags = selfFlags;
    data->m_numMembers = count + 1;

    for (uint32_t k = 1; k < data->m_numMembers; k++) {
        data->m_members[k].m_guid = members[k - 1].m_guid;
        data->m_members[k].m_flags = members[k - 1].m_flags;
    }

    for (int32_t i = 0; i < count; i++) {
        uint32_t j;

        for (j = 0; j < FriendList::s_friendList->GetNumIgnores(); j++) {
            if (members[i].m_guid.m_guid == FriendList::s_friendList->GetIgnore(j)) {
                break;
            }
        }

        if (j < FriendList::s_friendList->GetNumIgnores()) {
            comsat->MuteTalker(0, members[i].m_guid.m_guid, static_cast<ComSat::MuteSetting>(0));
            continue;
        }

        for (uint32_t k = 0; k < FriendList::s_friendList->GetNumMutes(); k++) {
            if (members[i].m_guid.m_guid == FriendList::s_friendList->GetIgnore(k)) {
                comsat->MuteTalker(0, members[i].m_guid.m_guid, static_cast<ComSat::MuteSetting>(0));
            }
        }
    }

    if (s_voiceChatWasEnabled) {
        switch (type) {
            case 2:
                CGChat::JoiningParty(false, 0);
                s_voiceChatWasEnabled = 0;
                break;
            case 3:
                CGChat::JoiningParty(true, 0);
                s_voiceChatWasEnabled = 0;
                break;
            case 1:
                CGChat::JoiningParty(true, 1);
                s_voiceChatWasEnabled = 0;
                break;
            default:
                break;
        }
    }

    CGChat::VoiceSessionUpdate(type ? "" : name, type);

    if (type == 2 || type == 3 || type == 1) {
        Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_STATUS_UPDATE);
    } else {
        Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_CHANNEL_STATUS_UPDATE);
    }

    s_comsatDeviceLock.Leave();
}

// VERIFY: 62882824 manual
// OFFSET: 0x007DEBF0
void VoiceSessionLeave(uint64_t talker, uint64_t session, int32_t type) {
    s_comsatDeviceLock.Enter();
    ComSat::Interface* comsat = ComSatThread::s_comsat;

    if (!comsat) {
        s_comsatDeviceLock.Leave();
        return;
    }

    comsat->SessionRemove(session, talker);
    comsat->SessionDestroy(session);
    int32_t index;

    for (index = 0; index < 32; index++) {
        if (s_comsatData[index].m_sessionID == session || (type == s_comsatData[index].m_type && (type == 2 || type == 3))) {
            break;
        }
    }

    if (index < 32) {
        if (s_currentSession == index) {
            ComSatClient_LeaveSessionNotifyAll(index);

            if (CGGameUI::m_initialized) {
                FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_LEFT_SESSION, nullptr);
            }

            s_currentSession = -1;
            WOWGUID player = ClntObjMgrGetActivePlayer();

            if (s_talkingPlayers.Ptr(static_cast<uint32_t>(player.m_guid), CHashKeyGUID(player.m_guid))) {
                Script_SendUnitSignal(player, SCRIPT_EVENT_VOICE_STOP);
                ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, player);
            }
        }

        s_comsatData[index].m_sessionID = 0;
        s_comsatData[index].m_name[0] = 0;
        s_comsatData[index].m_type = 4;
        s_comsatData[index].m_numMembers = 0;
    }

    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_VOICE_SESSIONS_UPDATE, nullptr);
    }

    s_comsatDeviceLock.Leave();
}

// VERIFY: c227625a auto
// OFFSET: 0x007DD6A0
static void RemoveTalkingPlayer(WOWGUID guid) {
    TALKINGPLAYER* talker = s_talkingPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (talker) {
        s_talkingPlayers.Delete(talker);
    }
}

// VERIFY: c8281370 auto
// OFFSET: 0x007DE2B0
void Comsat::VoiceStop() {
    if (!s_localPlayer && !(s_localPlayer = ClntObjMgrGetActivePlayer())) {
        return;
    }

    Script_SendUnitSignal(s_localPlayer, SCRIPT_EVENT_VOICE_STOP);
    ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, s_localPlayer);
    RemoveTalkingPlayer(s_localPlayer);
}

// VERIFY: a2e28c9f manual
// OFFSET: 0x007DE310
void Comsat::VoicePlateStop(uint64_t sessionId, uint64_t guid) {
    WOWGUID none = { 0 };
    NameCache* name = g_nameDBCache.GetRecord(guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    int32_t index;

    for (index = 0; index < 32; index++) {
        if (s_comsatData[index].m_sessionID == sessionId) {
            break;
        }
    }

    if (index >= 32) {
        return;
    }

    Script_SendUnitSignal(name->m_guid, SCRIPT_EVENT_VOICE_STOP);
    ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_STOP, name->m_guid);
    WOWGUID player = name->m_guid;
    TALKINGPLAYER* talker = s_talkingPlayers.Ptr(static_cast<uint32_t>(player.m_guid), CHashKeyGUID(player.m_guid));

    if (talker) {
        s_talkingPlayers.Delete(talker);
    }

    if (--s_talkingPlayerCount < 0) {
        s_talkingPlayerCount = 0;
    }
}

// VERIFY: 89c87ddd manual
// OFFSET: 0x007DF410
static void AddTalkingPlayer(WOWGUID guid) {
    if (!s_talkingPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid))) {
        s_talkingPlayers.New(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid), 0, 0);
    }
}

// VERIFY: da6e9fc2 auto
// OFFSET: 0x007DF580
void Comsat::VoiceStart() {
    if ((s_localPlayer || (s_localPlayer = ClntObjMgrGetActivePlayer())) && s_currentSession >= 0) {
        Script_SendUnitSignal(s_localPlayer, SCRIPT_EVENT_VOICE_START);
        ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_START, s_localPlayer);
        AddTalkingPlayer(s_localPlayer);
    }
}

// VERIFY: ec112893 manual
// OFFSET: 0x007DF5F0
void Comsat::VoicePlateStart(uint64_t sessionId, uint64_t guid) {
    WOWGUID none = { 0 };
    NameCache* name = g_nameDBCache.GetRecord(guid, none, nullptr, nullptr, false);

    if (!name) {
        return;
    }

    int32_t index;

    for (index = 0; index < 32; index++) {
        if (s_comsatData[index].m_sessionID == sessionId) {
            break;
        }
    }

    if (index >= 32) {
        return;
    }

    Script_SendUnitSignal(name->m_guid, SCRIPT_EVENT_VOICE_START);
    ComSatClient_SendEvent(SCRIPT_EVENT_VOICE_PLATE_START, name->m_guid);
    AddTalkingPlayer(name->m_guid);
    s_talkingPlayerCount++;
}

// VERIFY: 66f4735b auto
// OFFSET: 0x007DBAC0
static int32_t AcquireSingleInstanceMutex() {
    s_comsatClientMutex.Create(1, "WOWCOMSATCLIENT12340");

    if (s_comsatClientMutex.Valid()) {
        if (!s_comsatClientMutex.Wait(0)) {
            return 1;
        }

        s_comsatClientMutex.Close();
        s_comsatClientMutex.Release();
    }

    return 0;
}

// VERIFY: aa10d573 auto
// OFFSET: 0x007DC380
static void InitSoundGroupFadeRates() {
    if (s_voiceFadeActive) {
        return;
    }

    s_voiceFadeActive = 1;
    float delay = s_cvarStartTalkingDelay ? s_cvarStartTalkingDelay->m_floatValue : 0.0f;
    s_voiceFadeDelayRate = 0.0f == delay ? 0.0f : 1.0f / delay;
    float time = s_cvarStartTalkingTime ? s_cvarStartTalkingTime->m_floatValue : 0.2f;
    s_voiceFadeRate = 0.0f != time ? 1.0f / time : 0.0f;
    s_sfxVolumeBase = SI2::GetChannelGroupVolume(0);
    s_musicVolumeBase = SI2::GetChannelGroupVolume(1);
    s_ambienceVolumeBase = SI2::GetChannelGroupVolume(2);

    if (s_cvarSoundSFXVolume && s_cvarChatSoundVolume) {
        s_sfxVolumeDelta = s_cvarChatSoundVolume->m_floatValue * s_cvarSoundSFXVolume->m_floatValue - s_sfxVolumeBase;
    }

    if (s_cvarSoundMusicVolume && s_cvarChatMusicVolume) {
        s_musicVolumeDelta = s_cvarChatMusicVolume->m_floatValue * s_cvarSoundMusicVolume->m_floatValue - s_musicVolumeBase;
    }

    if (s_cvarSoundAmbienceVolume && s_cvarChatAmbienceVolume) {
        s_ambienceVolumeDelta = s_cvarChatAmbienceVolume->m_floatValue * s_cvarSoundAmbienceVolume->m_floatValue - s_ambienceVolumeBase;
    }
}

// VERIFY: b160843b auto
// OFFSET: 0x007DC4A0
static void UpdateSoundGroupFadeRates() {
    if (!s_voiceFadeActive) {
        return;
    }

    s_voiceFadeActive = 0;
    s_voiceFadeStartTime = static_cast<float>(OsGetAsyncTimeMs() * 0.001);
    float delay = s_cvarStopTalkingDelay ? s_cvarStopTalkingDelay->m_floatValue : 3.0f;
    s_voiceFadeDelayRate = 0.0f == delay ? 0.0f : 1.0f / delay;
    float time = s_cvarStopTalkingTime ? s_cvarStopTalkingTime->m_floatValue : 3.0f;
    s_voiceFadeRate = 0.0f != time ? 1.0f / time : 0.0f;
    s_sfxVolumeBase = SI2::GetChannelGroupVolume(0);
    s_musicVolumeBase = SI2::GetChannelGroupVolume(1);
    s_ambienceVolumeBase = SI2::GetChannelGroupVolume(2);

    if (s_cvarSoundSFXVolume) {
        s_sfxVolumeDelta = s_cvarSoundSFXVolume->m_floatValue - s_sfxVolumeBase;
    }

    if (s_cvarSoundMusicVolume) {
        s_musicVolumeDelta = s_cvarSoundMusicVolume->m_floatValue - s_musicVolumeBase;
    }

    if (s_cvarSoundAmbienceVolume) {
        s_ambienceVolumeDelta = s_cvarSoundAmbienceVolume->m_floatValue - s_ambienceVolumeBase;
    }
}

// VERIFY: 6adfea94 auto
// OFFSET: 0x007DC680
static void UpdateConnectionState() {
    if (s_pushToTalkDown != s_lastPushToTalkDown) {
        if (s_pushToTalkDown) {
            Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_PUSH_TO_TALK_START);
        } else {
            Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_PUSH_TO_TALK_STOP);
        }
    }

    if (ComSatThread::s_comsat) {
        s_currentOutputDevice = ComSatThread::s_comsat->LocalTalkerGetCurrentOutputDeviceIndex(0);
        s_currentCaptureDevice = ComSatThread::s_comsat->LocalTalkerGetCurrentCaptureDeviceIndex(0);
        s_lastPushToTalkDown = s_pushToTalkDown;
        s_localTalking = s_localTalkerActive;
    } else {
        s_currentOutputDevice = -1;
        s_currentCaptureDevice = -1;
        s_lastPushToTalkDown = 0;
        s_localTalking = 0;
    }

    if (s_cvarComSatSleepTime) {
        s_comsatSleepTime = s_cvarComSatSleepTime->m_intValue;
    } else {
        s_comsatSleepTime = 10;
    }
}

// VERIFY: 958cb9ef auto
// OFFSET: 0x007DCA40
static void DoComSatWork() {
    if (ComSatThread::s_threadRunning) {
        return;
    }

    ComSatThread::s_threadRunning = 1;
    ComSatThread::s_threadShutdownEvent.Reset();
    SThread::Create(ComSatThread::Work, nullptr, ComSatThread::s_comsatThread, const_cast<char*>("ComSatThread"), 0);
    ComSatThread::s_comsatThread.SetPriority(4);
}

// VERIFY: be7752b7 manual
// OFFSET: 0x007DCC20
static void DispatchComSatEvents() {
    if (ComSatThread::s_comsat) {
        for (uint32_t i = 0; i < s_comSatEvents.Count(); i++) {
            ComSatEvent& event = s_comSatEvents[i];

            switch (event.m_type) {
                case 1:
                    ComSatThread::s_comsat->LocalTalkerSetSessionVolume(0, event.m_guid, event.m_floatValue);
                    break;

                case 2:
                    s_pushToTalkDown = 0;
                    ComSatThread::s_comsat->LocalTalkerEnable(0, event.m_uintValue != 0);
                    break;

                case 3:
                    ComSatThread::s_comsat->LocalTalkerThreshold(0, event.m_floatValue);
                    break;

                case 4:
                    ComSatThread::s_comsat->MuteTalker(0, event.m_guid, static_cast<ComSat::MuteSetting>(event.m_uintValue));
                    break;

                case 5:
                    ComSatThread::s_comsat->LocalTalkerSelectSession(0, event.m_guid);
                    break;

                case 6:
                    ComSatThread::s_comsat->LocalTalkerSetSessionPriority(0, event.m_guid, static_cast<uint8_t>(event.m_uintValue));
                    break;

                case 7:
                    ComSatThread::s_comsat->MuteSession(0, event.m_guid, static_cast<ComSat::MuteSetting>(event.m_uintValue));
                    break;

                case 8:
                    ComSatThread::s_comsat->DebugSetSequenceDelay(event.m_uintValue);
                    break;

                case 9:
                    ComSatThread::s_comsat->DebugSetSoundOutputDelay(0, event.m_uintValue);
                    break;

                case 10:
                    UpdatePushToTalkKey();
                    break;
            }
        }
    }

    s_comSatEvents.Clear();
}

// VERIFY: d579125f auto
// OFFSET: 0x007DD720
bool ComSatClient_IsMutedPlayer(WOWGUID guid) {
    return s_mutedPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid)) != nullptr;
}

// VERIFY: d5d21c9a auto
// OFFSET: 0x007DDFF0
static bool IsVoiceActive() {
    if (s_talkingPlayerCount > 0) {
        return true;
    }

    WOWGUID player = s_localPlayer;

    if (!s_talkingPlayers.Ptr(static_cast<uint32_t>(player.m_guid), CHashKeyGUID(player.m_guid)) && !SI2::VoiceChat_IsRecordingLoopbackSound() && !SI2::VoiceChat_IsPlayingLoopbackSound()) {
        return false;
    }

    return true;
}

// VERIFY: 03c08e6c auto
// OFFSET: 0x007DE050
static void ApplySoundGroupFade(uint32_t time) {
    double now = time * 0.001;

    if (s_voiceFadeDelayRate == 0.0f) {
        if (s_voiceFadeRate == 0.0f) {
            if (IsVoiceActive()) {
                if (s_cvarSoundSFXVolume && s_cvarChatSoundVolume) {
                    float volume = s_cvarChatSoundVolume->m_floatValue * s_cvarSoundSFXVolume->m_floatValue;

                    if (volume != SI2::GetChannelGroupVolume(0)) {
                        SI2::SetChannelGroupVolume(0, s_cvarChatSoundVolume->m_floatValue * s_cvarSoundSFXVolume->m_floatValue);
                    }
                }

                if (s_cvarSoundMusicVolume && s_cvarChatMusicVolume) {
                    float volume = s_cvarChatMusicVolume->m_floatValue * s_cvarSoundMusicVolume->m_floatValue;

                    if (volume != SI2::GetChannelGroupVolume(1)) {
                        SI2::SetChannelGroupVolume(1, s_cvarChatMusicVolume->m_floatValue * s_cvarSoundMusicVolume->m_floatValue);
                    }
                }

                if (s_cvarSoundAmbienceVolume && s_cvarChatAmbienceVolume) {
                    float volume = s_cvarChatAmbienceVolume->m_floatValue * s_cvarSoundAmbienceVolume->m_floatValue;

                    if (volume != SI2::GetChannelGroupVolume(2)) {
                        SI2::SetChannelGroupVolume(2, s_cvarChatAmbienceVolume->m_floatValue * s_cvarSoundAmbienceVolume->m_floatValue);
                    }
                }
            }
        } else {
            float fade = static_cast<float>(s_voiceFadeRate * (now - s_voiceFadeStartTime));

            if (fade >= 1.0f) {
                fade = 1.0f;
                s_voiceFadeRate = 0.0f;
                s_voiceFadeStartTime = 0.0f;
            }

            SI2::SetChannelGroupVolume(0, fade * s_sfxVolumeDelta + s_sfxVolumeBase);
            SI2::SetChannelGroupVolume(1, s_musicVolumeDelta * fade + s_musicVolumeBase);
            SI2::SetChannelGroupVolume(2, s_ambienceVolumeDelta * fade + s_ambienceVolumeBase);
        }
    } else if (s_voiceFadeDelayRate * (now - s_voiceFadeStartTime) >= 1.0) {
        s_voiceFadeDelayRate = 0.0f;
        s_voiceFadeStartTime = static_cast<float>(now);
    }

    if (!s_voiceFadeActive) {
        if (IsVoiceActive()) {
            InitSoundGroupFadeRates();
            return;
        }

        if (!s_voiceFadeActive) {
            return;
        }
    }

    if (!IsVoiceActive()) {
        UpdateSoundGroupFadeRates();
    }
}

// VERIFY: 4a995b3c manual
// OFFSET: 0x007DF4B0
static void MarkPlayerMuted(WOWGUID guid, int32_t muted) {
    MUTEDPLAYER* entry = s_mutedPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid));

    if (entry) {
        if (!muted) {
            s_mutedPlayers.Delete(entry);
        }
    } else if (muted) {
        s_mutedPlayers.New(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid), 0, 0);
    }
}

// VERIFY: af9c2575 auto
// OFFSET: 0x007DF6F0
void VoiceSessionSetTalkerMuted(WOWGUID guid, int32_t muted) {
    AddComSatEvent(4, static_cast<uint32_t>(muted), guid.m_guid);

    if (muted == 2) {
        MarkPlayerMuted(guid, s_mutedPlayers.Ptr(static_cast<uint32_t>(guid.m_guid), CHashKeyGUID(guid.m_guid)) == nullptr);
    } else {
        MarkPlayerMuted(guid, muted == 0);
    }

    if (CGGameUI::m_initialized) {
        CGChat::UpdateRosterMember(guid);
        Script_SendUniqueSignal(SCRIPT_EVENT_VOICE_STATUS_UPDATE);
    }
}

// VERIFY: 71e5c0a2 auto
// OFFSET: 0x007DF830
static void VoiceChat_Tick() {
    s_comsatDeviceLock.Enter();
    ProcessComSatCallbacks();
    UpdateConnectionState();
    DispatchComSatEvents();
    int32_t active = s_localTalking | (CGChat::IsVoiceChatEnabled() && s_currentSession != -1);
    s_comsatDeviceLock.Leave();

    if (active) {
        DoComSatWork();
        s_comsatIdleTimeout = OsGetAsyncTimeMs() + 3000;
    } else if (!s_comsatIdleTimeout || OsGetAsyncTimeMs() > s_comsatIdleTimeout) {
        if (ComSatThread::s_threadRunning) {
            ComSatThread::s_threadRunning = 0;
            ComSatThread::s_threadShutdownEvent.Set();
            ComSatThread::s_comsatThread.Wait(0xFFFFFFFF);
        }

        SE2::CaptureWork();
        s_comsatIdleTimeout = 0;
    }
}

// VERIFY: e26b5a8c auto
// OFFSET: 0x007DF8E0
int32_t VoiceChat_TickWrapper(const void* data, void* param) {
    if (ComSatThread::s_comsat) {
        VoiceChat_Tick();
    }

    ApplySoundGroupFade(OsGetAsyncTimeMs());
    return 1;
}

// VERIFY: 77a116ad manual
// OFFSET: 0x007DCB90
void AddComSatCallback(uint32_t type, uint64_t sessionId, uint64_t guid) {
    ComSatCallback* callback = s_comsatCallbacks.New();

    if (callback) {
        callback->m_type = type;
        callback->m_sessionId = sessionId;
        callback->m_guid = guid;
    }
}

// VERIFY: 5b40d473 auto
// OFFSET: 0x007DCDC0
void ComSatClientCallback::NotifyLocalTalkerStart(uint8_t localTalker) {
    if (localTalker == 0 && s_currentSession >= 0) {
        DebugLog("COMSAT_START: NotifyLocalTalkerStart\r\n");
        s_localTalkerActive = 1;
        AddComSatCallback(0, 0, 0);
    }
}

// VERIFY: 9ec2a4a5 auto
// OFFSET: 0x007DCE00
void ComSatClientCallback::NotifyLocalTalkerStop(uint8_t localTalker) {
    DebugLog("COMSAT_STOP: NotifyLocalTalkerStop\r\n");

    if (localTalker == 0) {
        s_localTalkerActive = 0;
        AddComSatCallback(1, 0, 0);
    }
}

// VERIFY: 46118eb3 auto
// OFFSET: 0x007DCE40
void ComSatClientCallback::NotifyTalkerStart(uint8_t localTalker, uint64_t session, uint64_t talker) {
    DebugLog("COMSAT_START: NotifyTalkerStart\r\n");

    if (localTalker == 0) {
        AddComSatCallback(2, session, talker);
    }
}

// VERIFY: 456c9b21 auto
// OFFSET: 0x007DCE80
void ComSatClientCallback::NotifyTalkerStop(uint8_t localTalker, uint64_t session, uint64_t talker) {
    DebugLog("COMSAT_STOP: NotifyTalkerStop\r\n");

    if (localTalker == 0) {
        AddComSatCallback(3, session, talker);
    }
}

// VERIFY: 05ae74ae auto
// OFFSET: 0x0047ADD0
void ComSatClientCallback::NotifyLocalTalkerVoiceLevel(uint8_t localTalker, float level, bool talking) {
}

// VERIFY: 39561cba auto
// OFFSET: 0x00632050
void ComSatClientCallback::NotifyDeviceError(uint8_t localTalker) {
}

// VERIFY: e3ca2f75 manual
// OFFSET: 0x007DF9A0
void ComSatThread::Init() {
    s_pushToTalkKey = -1;
    s_pushToTalkMouseButton = 0;

    ComSat::Initialization init = {};
    init.m_maxLocalPlayers = 1;
    init.unk_0004 = 50;
    init.m_time = OsGetAsyncTimeMs();
    init.m_window = g_theGxDevicePtr->DeviceWindow();
    init.m_port = 0;
    init.m_maxChannels = 4;

    ComSatThread::s_comsat = ComSat::Interface::Create(init, s_comsatAllocator);

    if (!ComSatThread::s_comsat) {
        return;
    }

    ComSatThread::s_comsat->CallbackSet(s_comsatCallback);
    ComSat::DeviceInfo devices[8];
    int32_t numDevices = ComSatThread::s_comsat->EnumerateOutputDevices(devices, 8);
    UpdateConnectionState();

    if (numDevices > 0) {
        WOWGUID player = ClntObjMgrGetActivePlayer();
        ComSatThread::s_comsat->LocalTalkerCreate(0, player.m_guid, 0.6f, 0, 0);
        EventRegister(EVENT_ID_IDLE, VoiceChat_TickWrapper);
    }

    CVar* voiceChatMode = CVar::Lookup("VoiceChatMode");

    if (voiceChatMode && voiceChatMode->m_intValue) {
        s_voiceActivation = 1;
    } else {
        s_voiceActivation = 0;
    }

    ComSatThread::s_comsat->LocalTalkerEnable(0, false);
    ComSatThread::s_comsat->DebugSetSequenceDelay(2);
    ComSatThread::s_comsat->DebugSetSoundOutputDelay(0, 150);
    s_voiceChatWasEnabled = 0;
}

// VERIFY: 38899865 auto
// OFFSET: 0x007DFAF0
void ComSatClient_Init() {
    s_comsatDisabledByClient = 0;
    s_cvarStartTalkingDelay = CVar::Lookup("StartTalkingDelay");
    s_cvarStartTalkingTime = CVar::Lookup("StartTalkingTime");
    s_cvarStopTalkingDelay = CVar::Lookup("StopTalkingDelay");
    s_cvarStopTalkingTime = CVar::Lookup("StopTalkingTime");
    ResetVoiceVolumeState();
    CVar* outboundVolume = CVar::Lookup("OutboundChatVolume");
    CVar* inboundVolume = CVar::Lookup("InboundChatVolume");
    CVar* voiceChatMode = CVar::Lookup("VoiceChatMode");
    CVar* sensitivity = CVar::Lookup("VoiceActivationSensitivity");
    s_cvarEnableMicrophone = CVar::Lookup("EnableMicrophone");
    s_cvarEnableVoiceChat = CVar::Lookup("EnableVoiceChat");
    CVar::Lookup("VoiceChatSelfMute");
    s_cvarPushToTalkButton = CVar::Lookup("PushToTalkButton");
    s_cvarComSatSleepTime = nullptr;

    if (!(OsGetProcessorFeatures() & 4)) {
        s_comsatDisabledByClient = 1;
        SE2::Log_Write(" -###########################################################################################");
        SE2::Log_Write(" -# ERROR! SSE support is required for voice chat!");
        SE2::Log_Write(" -# Voice Chat DISABLED.");
        SE2::Log_Write(" -###########################################################################################");
        return;
    }

    if (!AcquireSingleInstanceMutex()) {
        s_comsatDisabledByClient = 1;
        SE2::Log_Write(" -###########################################################################################");
        SE2::Log_Write(" -# ERROR! Voice Chat does not support multiple clients on one computer.");
        SE2::Log_Write(" -# Voice Chat DISABLED for this client.");
        SE2::Log_Write(" -###########################################################################################");
        return;
    }

    ComSatThread::Init();
    s_pushToTalkModifiers = 0;
    memset(s_comsatData, 0, sizeof(s_comsatData));
    s_cvarSoundSFXVolume = CVar::Lookup("Sound_SFXVolume");
    s_cvarSoundMusicVolume = CVar::Lookup("Sound_MusicVolume");
    s_cvarSoundAmbienceVolume = CVar::Lookup("Sound_AmbienceVolume");
    s_cvarChatSoundVolume = CVar::Lookup("ChatSoundVolume");
    s_cvarChatMusicVolume = CVar::Lookup("ChatMusicVolume");
    s_cvarChatAmbienceVolume = CVar::Lookup("ChatAmbienceVolume");

    SE2::Log_Write(" ");
    SE2::Log_Write("=> Voice Chat User Setting Report:");
    SE2::Log_Write(" - ===========================");
    SE2::Log_Write(" - Enable Voice Chat  [%d]", s_cvarEnableVoiceChat->m_intValue);
    SE2::Log_Write(" ");
    SE2::Log_Write(" - ========= TALKING =========");
    SE2::Log_Write(" - Enable Microphone  [%d]", s_cvarEnableMicrophone->m_intValue);
    SE2::Log_Write(" - Microphone Volume  [%.2f]", outboundVolume->m_floatValue);
    SE2::Log_Write(" - Chat Mode          [%s]", voiceChatMode->m_intValue ? "VOICE ACTIVATED" : "PUSH TO TALK");

    if (voiceChatMode->m_intValue) {
        SE2::Log_Write(" - Sensitivity        [%.2f]", 1.0f - sensitivity->m_floatValue);
    } else {
        SE2::Log_Write(" - Push To Talk Key   [%s]", s_cvarPushToTalkButton->m_stringValue.m_str);
    }

    SE2::Log_Write(" ");
    SE2::Log_Write(" - ======== LISTENING ========");
    SE2::Log_Write(" - Speaker Volume     [%.2f]", inboundVolume->m_floatValue);
    SE2::Log_Write(" - Sound ducking      [%.2f]", s_cvarChatSoundVolume->m_floatValue);
    SE2::Log_Write(" - Music ducking      [%.2f]", s_cvarChatMusicVolume->m_floatValue);
    SE2::Log_Write(" - Ambience ducking   [%.2f]", s_cvarChatAmbienceVolume->m_floatValue);
    SE2::Log_Write("=> End of Voice Chat User Setting Report");
    SE2::Log_Write(" ");

    s_mutedPlayers.InternalClear(0);
    s_talkingPlayers.InternalClear(0);

    int32_t voiceChatEnabled = s_cvarEnableVoiceChat->m_intValue;
    int32_t microphoneEnabled = s_cvarEnableMicrophone->m_intValue;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_VOICE_SESSION_ENABLE));
    msg.Put(static_cast<uint8_t>(voiceChatEnabled != 0));
    msg.Put(static_cast<uint8_t>(microphoneEnabled != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    s_localPlayer = ClntObjMgrGetActivePlayer();
}
