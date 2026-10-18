#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Storm/H/storm.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
struct Socket {
    int fd = -1;
    ~Socket() { Reset(); }
    void Reset() { if (fd >= 0) close(fd); fd = -1; }
};
void Ready(int fd, short events) {
    pollfd entry{fd, events, 0};
    int result; do { result = poll(&entry, 1, 5000); } while (result < 0 && errno == EINTR);
    Require(result > 0, "Session loopback operation timed out");
}
void Transfer(int fd, uint8_t* bytes, size_t size, bool output) {
    while (size) {
        Ready(fd, output ? POLLOUT : POLLIN);
        auto count = output ? send(fd, bytes, size, 0) : recv(fd, bytes, size, 0);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        Require(count > 0, "Session loopback transfer failed"); bytes += count; size -= count;
    }
}
void SendPacket(int fd, CDataStore& packet, WowConnection* crypto = nullptr) {
    packet.Finalize();
    std::vector<uint8_t> wire{uint8_t(packet.m_size >> 8), uint8_t(packet.m_size)};
    wire.insert(wire.end(), packet.m_data, packet.m_data + packet.m_size);
    if (crypto) SARC4ProcessBuffer(wire.data(), 4, &crypto->m_sendKey, &crypto->m_sendKey);
    Transfer(fd, wire.data(), 1, true); Transfer(fd, wire.data() + 1, wire.size() - 1, true);
}
struct Session : NetClient {
    std::thread::id gameThread = std::this_thread::get_id();
    unsigned connects = 0, disconnects = 0, messages = 0, challenges = 0, failures = 0;
    AuthenticationChallenge challenge{};
    bool* destroyed = nullptr;
    ~Session() override { if (destroyed) *destroyed = true; }
    void OnGameThread() { Require(std::this_thread::get_id() == gameThread, "Session event escaped the game thread"); }
    void HandleAuthChallenge(AuthenticationChallenge* value) override { OnGameThread(); challenge = *value; ++challenges; }
    int32_t HandleConnect() override { OnGameThread(); ++connects; return NetClient::HandleConnect(); }
    int32_t HandleCantConnect() override { OnGameThread(); ++failures; return NetClient::HandleCantConnect(); }
    static int32_t DisconnectEvent(NetClient* client) {
        auto& session = *static_cast<Session*>(client); session.OnGameThread(); ++session.disconnects; return 1;
    }
    static int32_t Message(void* context, NETMESSAGE opcode, uint32_t, CDataStore* packet) {
        auto& session = *static_cast<Session*>(context); session.OnGameThread();
        uint32_t value = 0; packet->Get(value);
        Require(opcode == SMSG_QUERY_TIME_RESPONSE && value == 0x12345678 && packet->IsRead(), "Game-thread opcode or payload changed");
        ++session.messages; return 1;
    }
    template<class Predicate> void Pump(Predicate ready) {
        auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!ready()) {
            Require(std::chrono::steady_clock::now() < limit, "Game-thread session event timed out");
            PollEventQueue(); std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};
}

void CheckGameSession() {
    PropInitialize();
    auto context = PropCreateContext(); PropSelectContext(context);
    struct Context { HPROPCONTEXT value; ~Context() { PropSelectContext(nullptr); PropDeleteContext(value); PropDestroy(); } } properties{context};
    NetClient::SetDisconnectHandler(Session::DisconnectEvent);
    struct Host { ~Host() { NetClient::SetDisconnectHandler(nullptr); } } host;
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Socket listener; listener.fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{}; address.sin_len = sizeof(address); address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(listener.fd >= 0 && !bind(listener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(listener.fd, 4), "Cannot bind game-session loopback listener");
        socklen_t length = sizeof(address); Require(!getsockname(listener.fd, reinterpret_cast<sockaddr*>(&address), &length), "Cannot query game-session port");
        Session session; Require(session.Initialize(), "Original NetClient initialization failed");
        LoginData login{}; std::strcpy(login.m_account, "APPLECHECK");
        for (unsigned i = 0; i < sizeof(login.m_sessionKey); ++i) login.m_sessionKey[i] = uint8_t(i + 1);
        session.SetLoginData(&login);
        session.SetMessageHandler(SMSG_QUERY_TIME_RESPONSE, Session::Message, &session);
        session.ConnectInternal("127.0.0.1", ntohs(address.sin_port));
        Ready(listener.fd, POLLIN); Socket peer; peer.fd = accept(listener.fd, nullptr, nullptr);
        Require(peer.fd >= 0 && WowSocketNonBlocking(peer.fd), "Cannot accept game session");
        Require(session.connects == 0, "Connection event ran before polling");
        session.Pump([&] { return session.connects == 1; });
        CDataStore challenge; challenge.Put(uint16_t(SMSG_AUTH_CHALLENGE)); challenge.Put(uint32_t(0)); challenge.Put(uint32_t(0x87654321));
        for (uint32_t i = 0; i < 8; ++i) challenge.Put(i + 1);
        SendPacket(peer.fd, challenge);
        session.Pump([&] { return session.challenges == 1; });
        Require(session.challenge.m_serverSeed == 0x87654321 && session.challenge.m_proof == 0, "Original authentication challenge changed");
        CDataStore first; first.Put(uint32_t(CMSG_QUERY_TIME)); first.Finalize(); session.Send(&first);
        uint8_t initial[6]; Transfer(peer.fd, initial, sizeof(initial), false);
        Require(initial[0] == 0 && initial[1] == 4 && initial[2] == uint8_t(CMSG_QUERY_TIME) && initial[3] == uint8_t(CMSG_QUERY_TIME >> 8), "First session packet must precede header encryption");
        WowConnection crypto(nullptr, nullptr); crypto.SetEncryptionKey(login.m_sessionKey, sizeof(login.m_sessionKey), 0, nullptr, 0);
        CDataStore response; response.Put(uint16_t(SMSG_QUERY_TIME_RESPONSE)); response.Put(uint32_t(0x12345678)); SendPacket(peer.fd, response, &crypto);
        session.Pump([&] { return session.messages == 1; });
        CDataStore second; second.Put(uint32_t(CMSG_QUERY_TIME)); second.Finalize(); session.Send(&second);
        uint8_t encrypted[6]; Transfer(peer.fd, encrypted, sizeof(encrypted), false);
        SARC4ProcessBuffer(encrypted, sizeof(encrypted), &crypto.m_receiveKey, &crypto.m_receiveKey);
        Require(!std::memcmp(initial, encrypted, sizeof(initial)), "Subsequent session header encryption changed payload");
        peer.Reset(); session.Pump([&] { return session.disconnects == 1; });
        Require(session.m_netState == NS_INITIALIZED, "Disconnected session did not return to initialized state");
        session.ConnectInternal("127.0.0.1", ntohs(address.sin_port));
        Ready(listener.fd, POLLIN); peer.fd = accept(listener.fd, nullptr, nullptr);
        Require(peer.fd >= 0, "Cannot accept pending session");
        auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        bool queued = false;
        while (!queued) {
            Require(std::chrono::steady_clock::now() < limit, "Pending connection event timed out");
            session.m_netEventQueue->m_critSect.Enter();
            queued = !session.m_netEventQueue->m_eventQueue.IsEmpty();
            session.m_netEventQueue->m_critSect.Leave();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        session.Disconnect();
        Require(session.m_refCount.m_value == 0 && session.connects == 1, "Canceling queued connection leaked a reference or dispatched it");
        session.Destroy(); Require(NetClient::s_clientCount == 0, "Session left the network initialized");
        std::fprintf(stderr, "Original NetClient: cycle %u game-thread connect/challenge/opcode/disconnect, cipher transition and canceled event passed\n", cycle + 1);
    }
    bool deleted = false;
    auto* deferred = new Session(); deferred->destroyed = &deleted;
    Require(deferred->Initialize() && deferred->DelayedDelete(), "Cannot queue session deletion");
    deferred->PollEventQueue();
    Require(deleted && NetClient::s_clientCount == 0, "Deferred session deletion did not finish");
    std::fprintf(stderr, "Original NetClient: deferred deletion released the queue and native network\n");
}
