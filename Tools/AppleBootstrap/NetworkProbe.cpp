#include "WoW/Source/WowServices/WowConnection.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Storm/H/storm.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <vector>
#include <cstring>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Socket {
    int fd = -1;
    ~Socket() { if (fd >= 0) close(fd); }
    void Reset() { if (fd >= 0) close(fd); fd = -1; }
};
void Ready(int fd, short events) {
    pollfd entry{fd, events, 0};
    int result; do { result = poll(&entry, 1, 5000); } while (result < 0 && errno == EINTR);
    Require(result > 0, "Loopback network operation timed out");
}
void Transfer(int fd, uint8_t* bytes, size_t size, bool output) {
    while (size) {
        Ready(fd, output ? POLLOUT : POLLIN);
        const auto count = output ? send(fd, bytes, size, 0) : recv(fd, bytes, size, 0);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        Require(count > 0, "Loopback network transfer failed"); bytes += count; size -= count;
    }
}
struct Response : WowConnectionResponse {
    std::mutex mutex;
    std::condition_variable changed;
    bool connected = false, failed = false, disconnected = false;
    unsigned connectCount = 0;
    std::vector<std::vector<uint8_t>> messages;
    std::vector<uint8_t> stream;
    void WCMessageReady(WowConnection*, uint32_t, CDataStore* data) override {
        std::lock_guard<std::mutex> lock(mutex);
        messages.emplace_back(data->m_data, data->m_data + data->m_size); changed.notify_all();
    }
    void WCConnected(WowConnection*, WowConnection*, uint32_t, const NETCONNADDR*) override {
        std::lock_guard<std::mutex> lock(mutex); connected = true; ++connectCount; changed.notify_all();
    }
    void WCCantConnect(WowConnection*, uint32_t, const NETCONNADDR*) override {
        std::lock_guard<std::mutex> lock(mutex); failed = true; changed.notify_all();
    }
    void WCDisconnected(WowConnection*, uint32_t, const NETCONNADDR*) override {
        std::lock_guard<std::mutex> lock(mutex); disconnected = true; changed.notify_all();
    }
    void WCDataReady(WowConnection*, uint32_t, uint8_t* bytes, int32_t size) override {
        std::lock_guard<std::mutex> lock(mutex); stream.insert(stream.end(), bytes, bytes + size); changed.notify_all();
    }
    template<class Predicate> void Wait(Predicate ready) {
        std::unique_lock<std::mutex> lock(mutex);
        Require(changed.wait_for(lock, std::chrono::seconds(5), ready), "Original connection callback timed out");
    }
};
struct Connection {
    WowConnection* value;
    explicit Connection(Response& response) : value(new (SMemAlloc(sizeof(WowConnection), __FILE__, __LINE__, 0)) WowConnection(&response, nullptr)) {}
    ~Connection() { value->SetResponse(nullptr, false); value->Disconnect(); value->Release(); }
};
struct ReconnectResponse : Response {
    uint16_t port = 0;
    bool retry = true;
    void WCDisconnected(WowConnection* connection, uint32_t time, const NETCONNADDR* address) override {
        Response::WCDisconnected(connection, time, address);
        if (retry) { retry = false; connection->Connect("localhost", port, 5000); }
    }
};
}

void CheckGameNetwork() {
    {
        uint8_t bytes[sizeof(NETADDR) + 1]; auto* address = reinterpret_cast<NETADDR*>(bytes + 1);
        OsNetAddrMake(htonl(INADDR_LOOPBACK), 3724, address);
        uint16_t port; char text[32];
        Require(OsNetAddrGetAddress(address, &port) == htonl(INADDR_LOOPBACK) && port == 3724 &&
            !std::strcmp(OsNetAddrToStr(address, text, sizeof(text)), "127.0.0.1:3724"), "Native game address conversion failed");
    }
    // Independent byte-wise RC4/HMAC-SHA1 reference, key 01..08, original
    // default directional seeds, first sixteen bytes after dropping 1024.
    const uint8_t reference[2][16] = {
        {0xd9,0xcf,0x23,0x64,0x7b,0xa8,0x00,0x0f,0xb7,0x8f,0x55,0x7f,0xde,0x15,0xac,0x16},
        {0x1a,0x82,0xae,0x33,0xaa,0xf0,0xb0,0x84,0x10,0xf6,0xac,0xd6,0x9f,0xe5,0x8f,0xb3}
    };
    const uint8_t key[] = {1,2,3,4,5,6,7,8};
    {
        WowConnection crypto(nullptr, nullptr);
        crypto.SetEncryptionKey(key, sizeof(key), 0, nullptr, 0);
        for (unsigned direction = 0; direction < 2; ++direction) {
            uint8_t unaligned[18]{};
            auto* state = direction ? &crypto.m_receiveKey : &crypto.m_sendKey;
            SARC4ProcessBuffer(unaligned + 1, 7, state, state);
            SARC4ProcessBuffer(unaligned + 8, 9, state, state);
            Require(!std::memcmp(unaligned + 1, reference[direction], 16), "Original header cipher differs from independent reference");
        }
    }
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        WowConnection::InitOsNet(nullptr, nullptr, 2, false);
        struct Network { ~Network() { WowConnection::DestroyOsNet(); } } network;
        Socket listener; listener.fd = socket(AF_INET, SOCK_STREAM, 0); Require(listener.fd >= 0, "Cannot create loopback listener");
        sockaddr_in address{}; address.sin_len = sizeof(address); address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(!bind(listener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(listener.fd, 4), "Cannot bind loopback listener");
        socklen_t length = sizeof(address); Require(!getsockname(listener.fd, reinterpret_cast<sockaddr*>(&address), &length), "Cannot query loopback port");
        Response response; Connection client(response);
        Require(client.value->Connect("127.0.0.1", ntohs(address.sin_port), 5000), "Original connection rejected loopback");
        Ready(listener.fd, POLLIN); Socket peer; peer.fd = accept(listener.fd, nullptr, nullptr);
        Require(peer.fd >= 0, "Cannot accept original connection"); WowSocketNonBlocking(peer.fd);
        response.Wait([&] { return response.connected || response.failed; }); Require(response.connected && !response.failed, "Original connect completion failed");
        int small = 1024; setsockopt(client.value->m_sock, SOL_SOCKET, SO_SNDBUF, &small, sizeof(small));
        for (unsigned size : {7u, 100000u}) {
            std::vector<uint8_t> payload(size); for (unsigned i = 0; i < size; ++i) payload[i] = uint8_t(i * 17 + cycle);
            CDataStore packet(payload.data(), payload.size());
            Require(client.value->SendPacket(&packet, 0) <= 1, "Original packet send failed");
            const unsigned header = size > 0x7fff ? 3 : 2;
            std::vector<uint8_t> wire(size + header); Transfer(peer.fd, wire.data(), wire.size(), false);
            Require(wire[0] == (header == 3 ? (0x80 | (size >> 16)) : (size >> 8)) &&
                wire[header - 2] == uint8_t(size >> 8) && wire[header - 1] == uint8_t(size) &&
                std::equal(payload.begin(), payload.end(), wire.begin() + header), "Original packet header or queued payload changed");
            // Split both header and body at transport boundaries.
            for (size_t offset = 0; offset < wire.size();) {
                const size_t part = std::min(wire.size() - offset, offset < header ? size_t(1) : size_t(137));
                Transfer(peer.fd, wire.data() + offset, part, true); offset += part;
            }
            const size_t expected = size == 7 ? 1 : 2;
            response.Wait([&] { return response.messages.size() >= expected; });
            std::lock_guard<std::mutex> lock(response.mutex);
            Require(response.messages.back() == payload, "Original fragmented message assembly changed payload bytes");
        }
        // Encrypt the original directional headers/opcodes while preserving
        // remaining payload bytes; use the opposite peer direction.
        WowConnection crypto(nullptr, nullptr);
        client.value->SetEncryptionKey(key, sizeof(key), 1, nullptr, 0); client.value->SetEncryption(true);
        crypto.SetEncryptionKey(key, sizeof(key), 0, nullptr, 0); crypto.SetEncryption(true);
        client.value->m_lock.Enter();
        client.value->m_uint375 = 4; client.value->m_uint376 = 2; // Original NetClient opcode sizes.
        client.value->m_lock.Leave();
        for (unsigned size : {5u, 40000u}) {
            std::vector<uint8_t> payload(size); for (unsigned i = 0; i < size; ++i) payload[i] = uint8_t(i * 7);
            CDataStore packet(payload.data(), payload.size());
            Require(client.value->SendPacket(&packet, 0) <= 1, "Original encrypted send failed");
            const unsigned header = size > 0x7fff ? 3 : 2;
            std::vector<uint8_t> wire(payload.size() + header); Transfer(peer.fd, wire.data(), wire.size(), false);
            SARC4ProcessBuffer(wire.data(), header + 4, &crypto.m_receiveKey, &crypto.m_receiveKey);
            Require(wire[0] == (header == 3 ? (0x80 | (size >> 16)) : (size >> 8)) && wire[header - 1] == uint8_t(size) &&
                std::equal(payload.begin(), payload.end(), wire.begin() + header), "Directional opcode decryption changed protocol bytes");
            SARC4ProcessBuffer(wire.data(), header + 2, &crypto.m_sendKey, &crypto.m_sendKey);
            Transfer(peer.fd, wire.data(), 1, true); Transfer(peer.fd, wire.data() + 1, wire.size() - 1, true);
            response.Wait([&] { return response.messages.size() == (size == 5 ? 3 : 4); });
            { std::lock_guard<std::mutex> lock(response.mutex); Require(response.messages.back() == payload, "Encrypted message reassembly failed"); }
        }
        client.value->SetType(WOWC_TYPE_STREAM); client.value->SetEncryption(false);
        std::vector<uint8_t> stream(17003, 0x93); Transfer(peer.fd, stream.data(), stream.size(), true);
        response.Wait([&] { return response.stream.size() == stream.size(); });
        { std::lock_guard<std::mutex> lock(response.mutex); Require(response.stream == stream, "Original raw stream delivery changed bytes"); }
        peer.Reset(); response.Wait([&] { return response.disconnected; });
        // A socket number can be reused immediately. Reconnect from the original
        // callback, while the prior transport still has an unsent queue.
        {
            ReconnectResponse retry; retry.port = ntohs(address.sin_port); Connection reconnect(retry);
            reconnect.value->Connect("localhost", retry.port, 5000);
            Ready(listener.fd, POLLIN); Socket oldPeer; oldPeer.fd = accept(listener.fd, nullptr, nullptr);
            retry.Wait([&] { return retry.connectCount == 1; });
            setsockopt(reconnect.value->m_sock, SOL_SOCKET, SO_SNDBUF, &small, sizeof(small));
            std::vector<uint8_t> oldBytes(100000, 0x66); CDataStore oldPacket(oldBytes.data(), oldBytes.size());
            reconnect.value->SendPacket(&oldPacket, 0);
            reconnect.value->Disconnect();
            Ready(listener.fd, POLLIN); Socket newPeer; newPeer.fd = accept(listener.fd, nullptr, nullptr);
            Require(newPeer.fd >= 0 && WowSocketNonBlocking(newPeer.fd), "Cannot accept reconnected socket");
            retry.Wait([&] { return retry.connectCount == 2; });
            uint8_t fresh[] = {3, 4, 5}; CDataStore freshPacket(fresh, sizeof(fresh));
            Require(reconnect.value->SendPacket(&freshPacket, 0) <= 1, "Reconnected transport lost its descriptor");
            uint8_t bytes[5]; Transfer(newPeer.fd, bytes, sizeof(bytes), false);
            Require(bytes[0] == 0 && bytes[1] == 3 && !std::memcmp(bytes + 2, fresh, 3), "Reconnect retained bytes from the previous transport");
        }
        // A refused nonblocking connection must issue the original failure
        // callback, without waiting for the poller's next unrelated event.
        listener.Reset(); Response refused; Connection failure(refused);
        failure.value->Connect("127.0.0.1", ntohs(address.sin_port), 5000);
        refused.Wait([&] { return refused.failed; });
        // Stop with an open socket and queued data. Owners can release their
        // connection after the network threads have joined during app teardown.
        Socket shutdownListener; shutdownListener.fd = socket(AF_INET, SOCK_STREAM, 0);
        address.sin_port = 0;
        Require(!bind(shutdownListener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(shutdownListener.fd, 4), "Cannot bind shutdown listener");
        length = sizeof(address); getsockname(shutdownListener.fd, reinterpret_cast<sockaddr*>(&address), &length);
        Response shutdownResponse; Connection shutdownClient(shutdownResponse);
        shutdownClient.value->Connect("127.0.0.1", ntohs(address.sin_port), 5000);
        Ready(shutdownListener.fd, POLLIN); Socket shutdownPeer; shutdownPeer.fd = accept(shutdownListener.fd, nullptr, nullptr);
        shutdownResponse.Wait([&] { return shutdownResponse.connected; });
        setsockopt(shutdownClient.value->m_sock, SOL_SOCKET, SO_SNDBUF, &small, sizeof(small));
        std::vector<uint8_t> pending(100000, 0x55); CDataStore pendingPacket(pending.data(), pending.size());
        shutdownClient.value->SendPacket(&pendingPacket, 0);
        shutdownClient.value->m_lock.Enter();
        const bool queued = shutdownClient.value->m_sendDepth > 0;
        shutdownClient.value->m_lock.Leave();
        Require(queued, "Shutdown check did not reach a queued send");
        WDataStore retainedMessage; retainedMessage.Put(uint32_t(0x12345678));
        WowConnection::DestroyOsNet();
        Require(shutdownResponse.disconnected && shutdownClient.value->m_sock == -1 && shutdownClient.value->m_sendDepth == 0,
            "Native network shutdown retained an open socket or pending queue");
        std::fprintf(stderr, "Original network cycle %u: 2/3-byte headers, 100000-byte queued send, fragmented/encrypted opcodes, raw stream, callback reconnect, refusal and active shutdown passed\n", cycle);
    }
}
