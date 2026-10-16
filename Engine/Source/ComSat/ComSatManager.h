#ifndef ENGINE_SOURCE_COMSAT_COMSATMANAGER_H
#define ENGINE_SOURCE_COMSAT_COMSATMANAGER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/ComSat/ComSatInterface.h"
#include "Engine/Source/ComSat/ComSatNetwork.h"
#include "Engine/Source/ComSat/ComSatSession.h"
#include "Engine/Source/ComSat/ComSatSoundManager.h"
#include "Engine/Source/ComSat/ComSatVector.h"

namespace ComSat {

class Manager : public Interface, public Sound::Manager::Callback, public SessionCallback {
    public:
    class LocalTalker {
        public:
        struct Frame {
            /* 0000 */ uint16_t m_sequence;
            /* 0002 */ uint16_t m_bits;
            /* 0004 */ uint8_t m_data[124];
            /* 0080 */ uint16_t m_size;
        };

        /* 0000 */ uint64_t m_guid;
        /* 0008 */ Session* m_session;
        /* 000C */ Frame m_frames[100];
        /* 32D4 */ uint32_t m_frameCount;
        /* 32D8 */ uint32_t m_time;
        /* 32DC */ bool m_talking;
        /* 32E0 */ Vector<uint64_t> m_sessions;
        /* 32F0 */ bool m_deviceError;

        LocalTalker(Allocator& allocator);
    };

    /* 000C */ uint32_t m_maxLocalPlayers;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ Sound::Manager* m_soundManager;
    /* 0018 */ Network::Manager* m_networkManager;
    /* 001C */ Allocator* m_allocator;
    /* 0020 */ uint16_t m_defaultPort;
    /* 0024 */ Vector<Session*> m_sessions;
    /* 0034 */ LocalTalker** m_localTalkers;
    /* 0038 */ Interface::Callback* m_callback;

    virtual ~Manager();
    virtual void Work(uint32_t elapsed);
    virtual int32_t EnumerateOutputDevices(DeviceInfo* devices, int32_t maxDevices);
    virtual int32_t EnumerateCaptureDevices(DeviceInfo* devices, int32_t maxDevices);
    virtual void CallbackSet(Interface::Callback& callback);
    virtual void CallbackClear();
    virtual bool SessionCreate(uint64_t session);
    virtual bool SessionDestroy(uint64_t session);
    virtual bool SessionSetNetworkId(uint64_t session, uint16_t networkId);
    virtual bool SessionAddLocal(uint64_t session, uint8_t localTalker, uint64_t talker);
    virtual bool SessionAddRemote(uint64_t session, uint64_t talker, uint8_t networkId, const uint8_t (&key)[16], const char* address);
    virtual bool SessionSetTalkerNetworkIdLocal(uint64_t session, uint64_t talker, uint8_t networkId);
    virtual bool SessionSetTalkerNetworkIdRemote(uint64_t session, uint64_t talker, uint8_t networkId);
    virtual bool SessionRemove(uint64_t session, uint64_t talker);
    virtual int32_t SessionNumMembers(uint64_t session);
    virtual bool SessionGetTalkerByIndex(uint64_t session, int32_t index, uint64_t& talker, uint8_t& localTalker);
    virtual bool SessionSetTalkerPriority(uint64_t session, uint64_t talker, uint8_t priority);
    virtual bool LocalTalkerCreate(uint8_t localTalker, uint64_t talker, float threshold, int32_t outputDevice, int32_t captureDevice);
    virtual bool LocalTalkerDestroy(uint8_t localTalker);
    virtual bool LocalTalkerChangeOutputDevice(uint8_t localTalker, int32_t device);
    virtual bool LocalTalkerChangeCaptureDevice(uint8_t localTalker, int32_t device);
    virtual int32_t LocalTalkerGetCurrentOutputDeviceIndex(uint8_t localTalker);
    virtual int32_t LocalTalkerGetCurrentCaptureDeviceIndex(uint8_t localTalker);
    virtual bool LocalTalkerEnable(uint8_t localTalker, bool enable);
    virtual bool LocalTalkerThreshold(uint8_t localTalker, float threshold);
    virtual bool LocalTalkerEnableVoiceLevelCallback(uint8_t localTalker, bool enable);
    virtual bool LocalTalkerSelectSession(uint8_t localTalker, uint64_t session);
    virtual bool LocalTalkerSetSessionPriority(uint8_t localTalker, uint64_t session, uint8_t priority);
    virtual bool LocalTalkerSetRemoteTalkerVolume(uint8_t localTalker, uint64_t talker, float volume);
    virtual bool LocalTalkerSetSessionVolume(uint8_t localTalker, uint64_t session, float volume);
    virtual bool MuteTalker(uint8_t localTalker, uint64_t talker, MuteSetting setting);
    virtual bool IsTalkerMuted(uint8_t localTalker, uint64_t talker);
    virtual bool MuteSession(uint8_t localTalker, uint64_t session, MuteSetting setting);
    virtual bool IsSessionMuted(uint8_t localTalker, uint64_t session);
    virtual bool IsTalkerTalkingInSession(uint64_t session, uint8_t localTalker, uint64_t talker);
    virtual bool DoesTalkerHaveHeadset(uint64_t talker);
    virtual bool DoesLocalPlayerHaveHeadset(uint8_t localTalker);
    virtual void DebugSetSequenceDelay(int32_t delay);
    virtual void DebugSetSoundOutputDelay(uint8_t localTalker, int32_t delay);
    virtual void MicrophoneStatusUpdate(uint8_t player, Sound::HeadsetDeviceStatus status);
    virtual void LocalChatDataReady(uint8_t player, uint8_t* data, uint32_t size, uint32_t time);
    virtual void RemotePlayerTalkNotifyStart(uint8_t player, uint64_t talker, uint64_t session);
    virtual void RemotePlayerTalkNotifyStop(uint8_t player, uint64_t talker, uint64_t session);
    virtual void LocalTalkerVoiceLevel(uint8_t player, float level, bool talking);
    virtual void DeviceError(uint8_t player);
    virtual void IncomingVoiceData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size);

    Manager(Sound::Manager& soundManager, Network::Manager& networkManager, const Initialization& init, Allocator& allocator);
    Session* FindSession(uint64_t id);
    uint8_t GetLocalTalkerIndex(uint64_t talker);
    bool PackBitpackedVariableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size);
    bool PackStableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size);
    bool PackVariableBitrateData(uint8_t player, uint8_t* buffer, int32_t& size);
    void SendVoiceDataToCurrentSession(uint8_t player);
    bool UnpackBitpackedVariableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size);
    bool UnpackStableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size);
    bool UnpackVariableBitrateData(uint64_t session, uint64_t talker, const uint8_t* data, uint32_t size);
};

}

#endif
