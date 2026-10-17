#ifndef WOW_SOURCE_WOWSERVICES_WOWCONNECTIONNET_H
#define WOW_SOURCE_WOWSERVICES_WOWCONNECTIONNET_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "WoW/Source/WowServices/Types.h"
#include "WoW/Source/WowServices/WowConnection.h"

class WowConnectionNet {
    public:
// The 32-bit layout packs m_lock at 0x15 like Wow.exe. 64-bit builds keep natural alignment: their offsets
// differ anyway, and a misaligned lock faults on arm64 (pthread mutexes must be 8-byte aligned).
#if defined(WOWDECOMP_ARCH_32)
#pragma pack(push, 1)
#endif
    struct WORKER {
        /* 0000 */ WowConnectionNet* m_owner;
        /* 0004 */ SThread m_thread;
        /* 0008 */ int32_t m_index;
        /* 000C */ WowConnection* m_connection;
        /* 0010 */ SEvent m_event;
        /* 0014 */ bool m_quit;
        /* 0015 */ SCritSect m_lock;
        /* 003D */ uint8_t unused_003D[3];
        /* 0040 */ uint32_t m_lastTime;

        WORKER();
    };
#if defined(WOWDECOMP_ARCH_32)
#pragma pack(pop)
#endif

    /* 0000 */ SThread m_thread;
    /* 0004 */ SEvent m_stopEvent;
    /* 0008 */ bool m_stop;
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ int32_t m_numWorkers;
    /* 0010 */ WowConnectionNet::WORKER m_workers[32];
    /* 0890 */ SCritSect m_connectionsLock;
    /* 08B8 */ TSExplicitList<WowConnection, offsetof(WowConnection, m_netLink)> m_connections;
    /* 08C4 */ SSemaphore m_workerSemaphore;
    /* 08C8 */ TSList<WowConnection::SENDNODE> unused_08C8;
    /* 08D4 */ TSList<WowConnection::SENDNODE> unused_08D4;
    /* 08E0 */ SSemaphore m_serviceSemaphore;
    /* 08E4 */ void (*m_threadInit)();
    /* 08E8 */ void* m_event;
    /* 08EC */ uint8_t unused_08EC;
    /* 08ED */ bool m_networkStarted;
    /* 08EE */ uint8_t unused_08EE[2];

    WowConnectionNet(int32_t numThreads, void (*threadInit)());
    ~WowConnectionNet();
    void Add(WowConnection* connection);
    void Delete(WowConnection* connection);
    void PlatformAdd(WowConnection* connection);
    void PlatformChangeState(WowConnection* connection, WOW_CONN_STATE state);
    void PlatformDestroy();
    void PlatformDestruct(WowConnection* connection);
    void PlatformInit(bool useEngine);
    void PlatformRun();
    void PlatformStop();
    void PlatformRemove(WowConnection* connection);
    void PlatformWorkerReady();
    void Remove(WowConnection* connection);
    void RunWorker(int32_t index);
    void Service(WowConnection* connection, uint32_t flags);
    void Stop();
    void SignalWorker(WowConnection* connection, uint32_t flags);
    void Start();
};

#endif
