#if defined(__APPLE__)
#include "WoW/Source/WowServices/WowConnectionNet.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "Engine/Source/Storm/H/storm.h"
#include <array>
#include <cstdio>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <vector>

namespace {
struct NativeNetwork {
    int read = -1, write = -1;
    SEvent ready{1, 0};
    unsigned workers = 0;
    ~NativeNetwork() { if (read >= 0) close(read); if (write >= 0) close(write); }
    void Wake() {
        const uint8_t byte = 1;
        // A full nonblocking pipe already represents a pending wakeup.
        while (::write(write, &byte, 1) < 0 && errno == EINTR) {}
    }
    void Drain() {
        uint8_t bytes[128];
        for (;;) {
            const auto count = ::read(read, bytes, sizeof(bytes));
            if (count > 0 || (count < 0 && errno == EINTR)) continue;
            break;
        }
    }
};
NativeNetwork& Native(WowConnectionNet& owner) { return *static_cast<NativeNetwork*>(owner.m_event); }
struct ConnectionRefs {
    std::vector<WowConnection*> values;
    ~ConnectionRefs() { for (auto* connection : values) connection->Release(); }
};
struct Lock {
    SCritSect& value;
    explicit Lock(SCritSect& lock) : value(lock) { value.Enter(); }
    ~Lock() { value.Leave(); }
};
}

void WowConnectionNet::PlatformInit(bool) {
    auto native = std::make_unique<NativeNetwork>();
    int pair[2];
    if (pipe(pair)) throw std::runtime_error("Cannot create native network wakeup pipe");
    native->read = pair[0]; native->write = pair[1];
    for (int fd : pair) {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) || fcntl(fd, F_SETFD, FD_CLOEXEC))
            throw std::runtime_error("Cannot configure native network wakeup pipe");
    }
    m_event = native.release();
}
void WowConnectionNet::PlatformDestroy() { delete static_cast<NativeNetwork*>(m_event); m_event = nullptr; }
void WowConnectionNet::PlatformChangeState(WowConnection*, WOW_CONN_STATE) { if (m_event) Native(*this).Wake(); }
void WowConnectionNet::PlatformWorkerReady() { if (m_event) Native(*this).Wake(); }
void WowConnectionNet::PlatformStop() { PlatformWorkerReady(); }
void WowConnectionNet::PlatformRemove(WowConnection*) { PlatformWorkerReady(); }
void WowConnectionNet::PlatformDestruct(WowConnection*) {}
void WowConnectionNet::PlatformAdd(WowConnection* connection) {
    const int enabled = 1;
    setsockopt(connection->m_sock, IPPROTO_TCP, TCP_NODELAY, &enabled, sizeof(enabled));
    PlatformWorkerReady();
}

void WowConnectionNet::Start() {
    auto& native = Native(*this);
    if (m_numWorkers < 1 || m_numWorkers > 32) throw std::invalid_argument("Native network requires 1 to 32 workers");
    try {
        for (int32_t i = 0; i < m_numWorkers; ++i) {
            auto& worker = m_workers[i];
            worker.m_index = i; worker.m_connection = nullptr; worker.m_quit = false; worker.m_owner = this;
            char name[32]; SStrPrintf(name, sizeof(name), "NetThread %d", i);
            if (!SThread::Create([](void* arg) -> uint32_t {
                auto& value = *static_cast<WORKER*>(arg); value.m_owner->RunWorker(value.m_index); return 0;
            }, &worker, worker.m_thread, name, 0)) throw std::runtime_error("Cannot start native network worker");
            ++native.workers;
        }
        char name[] = "Network";
        if (!SThread::Create([](void* arg) -> uint32_t {
            auto& owner = *static_cast<WowConnectionNet*>(arg);
            try { owner.PlatformRun(); }
            catch (const std::exception& error) {
                owner.m_stop = true;
                std::fprintf(stderr, "Native network poll failed: %s\n", error.what());
            }
            owner.m_stopEvent.Set(); return 0;
        }, this, m_thread, name, 0)) throw std::runtime_error("Cannot start native network poll thread");
        if (native.ready.Wait(10000)) throw std::runtime_error("Native network startup did not complete");
    } catch (...) { Stop(); throw; }
}

void WowConnectionNet::Stop() {
    m_stop = true; PlatformStop();
    if (m_thread.Valid()) m_thread.Wait(UINT32_MAX);
    auto& native = Native(*this);
    for (unsigned i = 0; i < native.workers; ++i) { m_workers[i].m_quit = true; m_workers[i].m_event.Set(); }
    for (unsigned i = 0; i < native.workers; ++i) {
        auto& worker = m_workers[i]; worker.m_thread.Wait(UINT32_MAX);
        if (worker.m_connection) {
            --worker.m_connection->m_serviceCount;
            worker.m_connection->Release(); worker.m_connection = nullptr;
        }
    }
    native.workers = 0;
    // No poller/worker can retain or dispatch these sockets after this point.
    // Close before destroying global network state; owners may release later.
    for (;;) {
        WowConnection* connection;
        {
            Lock lock(m_connectionsLock);
            connection = m_connections.Head();
            if (!connection) break;
            connection->AddRef();
        }
        connection->DoDisconnect();
        {
            Lock lock(connection->m_lock);
            connection->FreeSendList();
            connection->m_sendBytes = connection->m_sendDepth = 0;
        }
        connection->Release();
    }
}

void WowConnectionNet::PlatformRun() {
    auto& native = Native(*this);
    m_networkStarted = true; native.ready.Set();
    while (!m_stop) {
        ConnectionRefs references;
        // Never acquire a connection lock while holding the list lock:
        // socket callbacks remove themselves with the opposite lock order.
        {
            Lock lock(m_connectionsLock);
            for (auto* connection = m_connections.Head(); connection; connection = m_connections.Next(connection)) {
                if (!connection->m_serviceCount) {
                    references.values.push_back(connection); connection->AddRef();
                }
            }
        }
        std::vector<pollfd> descriptors(references.values.size() + 1);
        std::vector<uint64_t> serials(references.values.size());
        descriptors[0] = {native.read, POLLIN, 0};
        int timeout = -1;
        for (size_t i = 0; i < references.values.size(); ++i) {
            auto* connection = references.values[i];
            connection->m_lock.Enter();
            serials[i] = connection->m_socketSerial;
            auto& descriptor = descriptors[i + 1]; descriptor.fd = -1;
            switch (connection->m_connState) {
                case WOWC_CONNECTING: descriptor = {connection->m_sock, POLLOUT, 0}; break;
                case WOWC_LISTENING: descriptor = {connection->m_sock, POLLIN, 0}; break;
                case WOWC_CONNECTED:
                    descriptor = {connection->m_sock, short(POLLIN | ((connection->m_sendList.Head() || connection->m_notifyWriteReady) ? POLLOUT : 0)), 0};
                    if (connection->m_lagTestRecvList.Head() || WowConnection::s_lagTestDelayMin || WowConnection::s_lagTestDelayMax) timeout = 10;
                    break;
                case WOWC_DISCONNECTING: timeout = 0; break;
                default: break;
            }
            connection->m_lock.Leave();
        }
        int result;
        do { result = poll(descriptors.data(), descriptors.size(), timeout); } while (result < 0 && errno == EINTR && !m_stop);
        if (m_stop) break;
        if (result < 0) throw std::runtime_error("poll returned an error");
        if (descriptors[0].revents) native.Drain();
        for (size_t i = 0; i < references.values.size(); ++i) {
            auto* connection = references.values[i]; const auto& descriptor = descriptors[i + 1]; uint32_t flags = 0;
            connection->m_lock.Enter();
            if (connection->m_connState == WOWC_DISCONNECTING) flags = 8;
            else if (connection->m_sock == descriptor.fd && descriptor.fd >= 0 && connection->m_socketSerial == serials[i]) {
                if (descriptor.revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) flags |= 2;
                if (descriptor.revents & (POLLOUT | POLLHUP | POLLERR | POLLNVAL)) flags |= 1;
                if (connection->m_lagTestRecvList.Head() || WowConnection::s_lagTestDelayMin || WowConnection::s_lagTestDelayMax) flags |= 3;
            }
            // Publish while holding the connection lock: a reconnect cannot
            // replace the descriptor between readiness validation and dispatch.
            if (flags && !m_stop) SignalWorker(connection, flags);
            connection->m_lock.Leave();
        }
    }
}
#endif
