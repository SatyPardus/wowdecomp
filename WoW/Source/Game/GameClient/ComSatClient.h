#ifndef WOW_SOURCE_GAME_GAMECLIENT_COMSATCLIENT_H
#define WOW_SOURCE_GAME_GAMECLIENT_COMSATCLIENT_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/ComSat/ComSatInterface.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"

struct CVar;
struct lua_State;

struct ComSatEvent {
    /* 0000 */ uint32_t m_type;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint64_t m_guid;
    /* 0010 */ union {
        uint32_t m_uintValue;
        float m_floatValue;
    };
    /* 0014 */ uint32_t unused_0014;
};

struct ComSatCallback {
    /* 0000 */ uint32_t m_type;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint64_t m_sessionId;
    /* 0010 */ uint64_t m_guid;
};

struct MUTEDPLAYER : public TSHashObject<MUTEDPLAYER, CHashKeyGUID> {
};

struct TALKINGPLAYER : public TSHashObject<TALKINGPLAYER, CHashKeyGUID> {
};

class ComSatClientAllocator : public ComSat::Allocator {
    public:
    void* Alloc(uint32_t size) override;
    void Free(void* ptr) override;
};

class ComSatClientCallback : public ComSat::Interface::Callback {
    public:
    void NotifyLocalTalkerStart(uint8_t localTalker) override;
    void NotifyLocalTalkerStop(uint8_t localTalker) override;
    void NotifyTalkerStart(uint8_t localTalker, uint64_t session, uint64_t talker) override;
    void NotifyTalkerStop(uint8_t localTalker, uint64_t session, uint64_t talker) override;
    void NotifyLocalTalkerVoiceLevel(uint8_t localTalker, float level, bool talking) override;
    void NotifyDeviceError(uint8_t localTalker) override;
};

int32_t ComSatClient_HasSession(int32_t type, const char* name);
int32_t ComSatClient_FindSessionByTypeAndName(int32_t type, const char* name);
int32_t ComSatClient_IsMemberInVoiceSession(WOWGUID guid, int32_t type, const char* name);
int32_t ComSatClient_IsTalkerSquelchedInCurrent(WOWGUID guid);
int32_t ComSatClient_DetermineSessionTypeFromName(const char* name);
int32_t ComSatClient_GetChannelMemberVoiceStatus(WOWGUID guid, int32_t type, const char* name);
int32_t ComSatClient_IsTalkerSquelched(WOWGUID guid, uint32_t session);
int32_t ComSatClient_IsMemberVoiceActive(WOWGUID guid, uint32_t session);
int32_t ComSatClient_GetMemberVoiceStatus(WOWGUID guid, uint32_t session);
bool ComSatClient_IsPlayerMuted(WOWGUID guid);
int32_t ComSatClient_IsTalkerSquelchedInChannel(WOWGUID guid, int32_t type, const char* name);
int32_t ComSatClient_GetCurrentSessionType();
const char* ComSatClient_GetCurrentSessionName();

namespace Comsat {
void VoicePlateStart(uint64_t sessionId, uint64_t guid);
void VoicePlateStop(uint64_t sessionId, uint64_t guid);
void VoiceStart();
void VoiceStop();
}

struct COMSATMEMBER {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint8_t m_flags;
    /* 0009 */ uint8_t unused_0009[7];
};

struct COMSATDATA {
    /* 0000 */ uint64_t m_sessionID;
    /* 0008 */ int32_t m_type;
    /* 000C */ char m_name[128];
    /* 008C */ uint32_t m_numMembers;
    /* 0090 */ COMSATMEMBER m_members[40];
};

extern COMSATDATA s_comsatData[32];
extern int32_t s_currentSession;
extern TSHashTable<TALKINGPLAYER, CHashKeyGUID> s_talkingPlayers;
extern CVar* s_cvarEnableMicrophone;
extern CVar* s_cvarEnableVoiceChat;
extern CVar* s_cvarPushToTalkButton;
extern uint32_t s_comsatSleepTime;
extern int32_t s_currentOutputDevice;
extern int32_t s_currentCaptureDevice;
extern int32_t s_pushToTalkDown;
extern int32_t s_localTalkerActive;
bool ComSatClient_IsMutedPlayer(WOWGUID guid);
extern int32_t s_pushToTalkKey;
extern uint32_t s_pushToTalkModifiers;
extern uint32_t s_pushToTalkMouseButton;
extern int32_t s_voiceActivation;
extern int32_t s_voiceChatWasEnabled;
extern const char* s_sessionTypeNames[3];

void AddComSatEvent(uint32_t type, uint32_t value, uint64_t guid);
void AddComSatCallback(uint32_t type, uint64_t sessionId, uint64_t guid);
void ComSatClient_SendEvent(int32_t event, WOWGUID guid);
int32_t ComSatClient_SetCurrentSession(int32_t type, const char* name);
void ComSatClient_DisableCurrentSession();
void ComSatClient_LeaveAllSessions(WOWGUID guid);
void AddComSatEvent(uint32_t type, float value, uint64_t guid);
void ComSatClient_SetCaptureDevice(int32_t index);
void ComSatClient_SetOutputDevice(int32_t index);
bool EnableMicrophoneHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool EnableVoiceChatHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool InboundChatVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool OutboundChatVolumeHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool PushToTalkButtonHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool SelfMuteHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool VoiceActivationSensitivityHandler(CVar* var, const char* oldValue, const char* value, void* arg);
bool VoiceChatModeHandler(CVar* var, const char* oldValue, const char* value, void* arg);

void ComSatClient_Init();
int32_t ComSatClient_IsComsatDisabledByClient();
void ComSatClient_Shutdown();
void UpdatePushToTalkKey();
int32_t VoiceChat_TickWrapper(const void* data, void* param);

int32_t Script_GetVoiceSessionMemberInfoBySessionID(lua_State* L);

extern int32_t s_comsatDisabledByClient;

class SCritSect;
extern SCritSect s_comsatDeviceLock;
void VoiceRegisterScriptFunctions();
void VoiceUnregisterScriptFunctions();

void VoiceSessionSetTalkerMuted(WOWGUID guid, int32_t muted);

struct VoiceRosterMember {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ uint8_t m_priority;
    /* 0009 */ char m_address[32];
    /* 0029 */ uint8_t m_talkerPriority;
    /* 002A */ uint8_t m_flags;
    /* 002B */ uint8_t unused_002B[5];
};

int32_t ComSatClient_IsCurrentSession(uint64_t session);
void ComSatClient_PopulateSessionDataWithoutJoin(uint64_t session, int32_t type, const char* name, uint64_t self);
void VoiceSessionRosterUpdate(uint64_t session, uint16_t networkId, int32_t type, const char* name, const uint8_t* key, uint64_t self, uint8_t selfNetworkId, uint8_t selfFlags, int32_t count, VoiceRosterMember* members);
void VoiceSessionLeave(uint64_t talker, uint64_t session, int32_t type);

#endif
