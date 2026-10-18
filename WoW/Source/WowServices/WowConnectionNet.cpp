#include "WowConnectionNet.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 1491f4aa auto
// OFFSET: 0x00469060
static uint32_t STORMAPI NetworkThreadProc(void* param) {
    WowConnectionNet* network = static_cast<WowConnectionNet*>(param);
    network->PlatformRun();
    network->m_stopEvent.Set();

    return 0;
}

// VERIFY: d0e3b08a auto
// OFFSET: 0x00469300
static uint32_t STORMAPI WorkerThreadProc(void* param) {
    WowConnectionNet::WORKER* worker = static_cast<WowConnectionNet::WORKER*>(param);
    worker->m_owner->RunWorker(worker->m_index);

    return 0;
}

// INLINED
WowConnectionNet::WORKER::WORKER() : m_event(0, 0) {
}

// VERIFY: 87918bed manual
// OFFSET: 0x00469320
#if !defined(__APPLE__)
void WowConnectionNet::Start() {
    char name[32];
    uint32_t i = 0;

    for (; static_cast<int32_t>(i) < this->m_numWorkers; i++) {
        WowConnectionNet::WORKER& worker = this->m_workers[i];
        worker.m_index = i;
        worker.m_connection = nullptr;
        worker.m_quit = false;
        worker.m_owner = this;
        worker.m_lastTime = OsGetAsyncTimeMs();
        SStrPrintf(name, sizeof(name), "NetThread %d", i);
        SThread::Create(WorkerThreadProc, &worker, worker.m_thread, name, 0);
    }

    for (; i < sizeof(this->m_workers) / sizeof(this->m_workers[0]); i++) {
        WowConnectionNet::WORKER& worker = this->m_workers[i];
        worker.m_index = 0;
        worker.m_connection = nullptr;
        worker.m_quit = true;
        worker.m_owner = this;
    }

    SStrPrintf(name, sizeof(name), "Network");
    this->m_networkStarted = false;
    SThread::Create(NetworkThreadProc, this, this->m_thread, name, 0);

    while (!this->m_networkStarted) {
        OsSleep(100);
    }
}
#endif

// VERIFY: c90eb59b auto
// OFFSET: 0x00468FF0
void WowConnectionNet::Service(WowConnection* connection, uint32_t flags) {
    do {
        if (flags & 0x1) {
            connection->DoWrites();
        }

        if (flags & 0x2) {
            connection->DoReads();
        }

        if (flags & 0x4) {
            connection->DoExceptions();
        }

        if (flags & 0x8) {
            connection->DoDisconnect();
        }

        this->m_connectionsLock.Enter();
        flags = connection->m_serviceFlags;
        connection->m_serviceFlags = 0;
        this->m_connectionsLock.Leave();
    } while (flags);
}

// VERIFY: 3c6bf49d auto
// OFFSET: 0x00469160
void WowConnectionNet::RunWorker(int32_t index) {
    if (this->m_threadInit) {
        this->m_threadInit();
    }

    this->m_workerSemaphore.Signal(1);

    WowConnectionNet::WORKER& worker = this->m_workers[index];

    while (true) {
        do {
            worker.m_lastTime = OsGetAsyncTimeMs();
        } while (worker.m_event.Wait(1000));

        if (worker.m_quit) {
            break;
        }

        uint32_t serviceFlags = worker.m_connection->m_serviceFlags;
        worker.m_connection->m_serviceFlags = 0;
        this->Service(worker.m_connection, serviceFlags);

        worker.m_lock.Enter();
#if defined(__APPLE__)
        --worker.m_connection->m_serviceCount;
#else
        InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&worker.m_connection->m_serviceCount), -1);
#endif
        worker.m_connection->Release();
        worker.m_connection = nullptr;
        worker.m_lock.Leave();

        this->m_workerSemaphore.Signal(1);
        this->PlatformWorkerReady();
    }
}

// VERIFY: d5522788 manual
// OFFSET: 0x00469220
void WowConnectionNet::SignalWorker(WowConnection* connection, uint32_t flags) {
    if (this->m_workerSemaphore.Wait(500)) {
        return;
    }

    connection->m_refCount.Increment();
    connection->m_serviceFlags = flags;
#if defined(__APPLE__)
    ++connection->m_serviceCount;
#else
    InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&connection->m_serviceCount), 1);
#endif

    for (int32_t i = 0; i < this->m_numWorkers; i++) {
        WowConnectionNet::WORKER& worker = this->m_workers[i];
        worker.m_lock.Enter();

        if (!worker.m_connection) {
            worker.m_connection = connection;
            worker.m_lock.Leave();
            worker.m_event.Set();
            return;
        }

        worker.m_lock.Leave();
    }
}

// VERIFY: e172a3d8 manual
// OFFSET: 0x004692C0
void WowConnectionNet::Delete(WowConnection* connection) {
    this->m_connectionsLock.Enter();

#if defined(__APPLE__)
    if (!__atomic_load_n(&connection->m_refCount.m_value, __ATOMIC_SEQ_CST)) {
#else
    if (!connection->m_refCount.m_value) {
#endif
        delete connection;
    }

    this->m_connectionsLock.Leave();
}

// VERIFY: da8d942b manual
// OFFSET: 0x004695F0
WowConnectionNet::WowConnectionNet(int32_t numThreads, void (*threadInit)()) : m_stopEvent(1, 0), m_workerSemaphore(0, numThreads), m_serviceSemaphore(0, 0x7FFFFFFF) {
    this->m_numWorkers = numThreads;
    this->m_threadInit = threadInit;
    this->m_stop = false;
    this->m_event = nullptr;
}

// VERIFY: 88d38822 manual
// OFFSET: 0x00469420
void WowConnectionNet::Add(WowConnection* connection) {
    this->m_connectionsLock.Enter();

    if (!this->m_connections.IsLinked(connection)) {
        this->m_connections.LinkToTail(connection);
        this->PlatformAdd(connection);
    }

    this->m_connectionsLock.Leave();
}

// VERIFY: f12734a2 manual
// OFFSET: 0x00469470
void WowConnectionNet::Remove(WowConnection* connection) {
    this->m_connectionsLock.Enter();

    if (this->m_connections.IsLinked(connection)) {
        this->m_connections.UnlinkNode(connection);
    }

    this->PlatformRemove(connection);
    this->m_connectionsLock.Leave();
}

// VERIFY: eb055e2d manual
// OFFSET: 0x00469080
#if !defined(__APPLE__)
void WowConnectionNet::Stop() {
    this->m_stop = true;
    this->PlatformStop();
    this->PlatformWorkerReady();

    if (this->m_stopEvent.Wait(10000)) {
        ConsolePrintf("Main network thread did not send stop event normally");
    }

    if (this->m_thread.Wait(5000)) {
        ConsolePrintf("Main network thread did not exit normally");
    }

    for (int32_t i = 0; i < this->m_numWorkers; i++) {
        WowConnectionNet::WORKER& worker = this->m_workers[i];
        worker.m_quit = true;
        worker.m_event.Set();
        this->m_serviceSemaphore.Signal(1);

        if (worker.m_thread.Wait(10000)) {
            ConsolePrintf("Network thread %d did not exit normally", i);
        } else if (worker.m_connection) {
            InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&worker.m_connection->m_serviceCount), -1);
            worker.m_connection->Release();
            worker.m_connection = nullptr;
        }
    }
}
#endif

// VERIFY: 91633125 manual
// OFFSET: 0x004694C0
WowConnectionNet::~WowConnectionNet() {
}
