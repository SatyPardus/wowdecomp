#include "Engine/Source/ComSat/ComSatNetwork.h"
#include "Engine/Source/ComSat/ComSatBitPacker.h"
#include <arpa/inet.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <vector>
#include <unistd.h>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct Allocator : ComSat::Allocator {
    std::unordered_set<void*> live;
    void* Alloc(uint32_t size) override { auto* p = std::malloc(size); if (!p) throw std::bad_alloc(); live.insert(p); return p; }
    void Free(void* p) override { if (p) { Require(live.erase(p) == 1, "ComSat freed an unowned allocation"); std::free(p); } }
};
using Address = ComSat::Network::Address;
using Socket = ComSat::Network::SocketPosix;
Address Local(const Socket& socket) {
    sockaddr_in native{}; socklen_t size = sizeof(native);
    Require(!getsockname(int(socket.m_socket), reinterpret_cast<sockaddr*>(&native), &size) && native.sin_port, "ComSat did not bind an actual UDP port");
    Address result; result.SetAddress("127.0.0.1"); result.m_port = native.sin_port; return result;
}
bool Receive(Socket& socket, Address& source, uint8_t* bytes, uint32_t& size) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    const uint32_t capacity = size;
    do {
        size = capacity;
        if (socket.RecvFrom(source, bytes, size)) return true;
        if (errno != EAGAIN && errno != EWOULDBLOCK) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}
void Bits() {
    for (uint32_t offset = 0; offset < 8; ++offset) for (uint32_t width = 1; width <= 65; ++width) {
        const uint32_t total = offset + width + 8;
        std::vector<uint8_t> bytes((total + 7) / 8, 0xCD), reference(bytes.size(), 0);
        std::array<uint8_t, 9> value{}; for (size_t i = 0; i < value.size(); ++i) value[i] = uint8_t(37 * i + 211);
        uint8_t prefix = 0xB5, tail = 0x93; uint32_t cursor = 0;
        auto append = [&](const uint8_t* source, uint32_t bits) {
            for (uint32_t i = 0; i < bits; ++i, ++cursor) reference[cursor / 8] |= ((source[i / 8] >> (i % 8)) & 1) << (cursor % 8);
        };
        append(&prefix, offset); append(value.data(), width); append(&tail, 8);
        ComSat::BitPacker pack(bytes.data(), total);
        Require(pack.PackBits(&prefix, offset) && pack.PackBits(value.data(), width) && pack.PackBits(&tail, 8) &&
            !pack.PackBits(&tail, 1) && !pack.PackBits(&tail, UINT32_MAX) && pack.Finalize() == bytes.size() && bytes == reference,
            "Original ComSat bit packing differs from the little-endian wire bit stream");
        ComSat::BitUnpacker unpack(bytes.data(), total);
        uint8_t first = 0, last = 0; std::array<uint8_t, 9> decoded{};
        Require(unpack.UnpackBits(&first, offset) && unpack.UnpackBits(decoded.data(), width) && unpack.UnpackBits(&last, 8) &&
            first == (prefix & ((1u << offset) - 1)) && last == tail && !unpack.UnpackBits(&last, UINT32_MAX),
            "Original ComSat bit unpacking lost a byte boundary or capacity check");
        for (uint32_t i = 0; i < width; ++i) Require(((decoded[i / 8] ^ value[i / 8]) & (1u << (i % 8))) == 0, "ComSat decoded bit changed");
    }
    // Exactly one byte, with no accessible spill byte for the 3+5 split.
    auto* one = static_cast<uint8_t*>(std::malloc(1)); *one = 0xFF;
    ComSat::BitPacker edge(one, 8); const uint8_t a = 3, b = 21;
    Require(edge.PackBits(&a, 3) && edge.PackBits(&b, 5) && edge.Finalize() == 1 && *one == 0xAB, "ComSat exact byte-end packing changed");
    std::free(one);
}
struct Callback : ComSat::Network::Session::Callback {
    uint64_t talker = 0;
    std::vector<std::vector<uint8_t>> received;
    void Recv(uint64_t id, const uint8_t* bytes, uint32_t size) override { talker = id; received.emplace_back(bytes, bytes + size); }
};
}

void CheckComSatNetwork() {
    Bits();
    uint8_t digest[16]; ComSat::MD5 md5; md5.Prepare(); md5.Process("a", 1); md5.Process("bc", 2); md5.Finish(digest);
    constexpr uint8_t abc[] = {0x90,0x01,0x50,0x98,0x3c,0xd2,0x4f,0xb0,0xd6,0x96,0x3f,0x7d,0x28,0xe1,0x7f,0x72};
    Require(!std::memcmp(digest, abc, sizeof(abc)), "Original ComSat MD5 differs from the standard vector");
    uint8_t key[16]; for (unsigned i = 0; i < 16; ++i) key[i] = i;
    constexpr uint8_t packet[] = {0xde,0x24,0x06,0x69,0x23,0x34,0x12,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    Require(ComSat::Network::ComputeSignature(key, packet, sizeof(packet)) == 0x690624DE, "ComSat folded HMAC-MD5 differs from the independent fixture");
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        Allocator allocator;
        int firstFD = -1, secondFD = -1;
        {
            ComSat::Initialization init{}; init.m_time = 123;
            ComSat::Network::Manager first(init, allocator), second(init, allocator);
            auto& a = *static_cast<Socket*>(first.m_socket); auto& b = *static_cast<Socket*>(second.m_socket);
            firstFD = int(a.m_socket); secondFD = int(b.m_socket);
            Require((fcntl(firstFD, F_GETFD) & FD_CLOEXEC) && (fcntl(firstFD, F_GETFL) & O_NONBLOCK), "Native ComSat socket lost nonblocking/close-on-exec flags");
            const auto addrA = Local(a), addrB = Local(b);
            Address numeric; numeric.SetAddress((std::string("127.0.0.1:") + std::to_string(addrA.GetPort())).c_str());
            Require(numeric == addrA && numeric.m_family == 2 && numeric.m_length == 16, "Native ComSat address layout/byte order changed");
            std::array<uint8_t, 1030> raw{}; Address source; uint32_t size = raw.size();
            Require(!b.RecvFrom(source, raw.data(), size) && size == raw.size(), "Empty native UDP read changed caller capacity");
            Require(first.Send(0x1234, 0x23, key, addrB, packet + 7, sizeof(packet) - 7) && Receive(b, source, raw.data(), size) &&
                size == sizeof(packet) && source == addrA && !std::memcmp(raw.data(), packet, sizeof(packet)), "Original ComSat packet bytes changed on native UDP");
            Require(std::all_of(raw.begin() + size, raw.end(), [](uint8_t v) { return v == 0; }), "Native ComSat receive left stale spare bytes");
            Require(a.SendTo(addrB, packet, sizeof(packet)) == sizeof(packet), "Native UDP send failed");
            size = 3;
            Require(!Receive(b, source, raw.data(), size) && errno == EMSGSIZE && size == 3, "Truncated native UDP datagram was treated as complete");
            Require(!first.Send(1, 2, key, addrB, raw.data(), UINT32_MAX) && !first.Send(1, 2, key, addrB, nullptr, 1), "ComSat packet length wrapped or accepted a missing payload");
            Callback callbackA, callbackB;
            auto* sessionA = first.CreateSession(0x100000001ull, callbackA);
            auto* sessionB = second.CreateSession(0x100000002ull, callbackB);
            sessionA->SetNetworkId(0x1234); sessionB->SetNetworkId(0x1234);
            constexpr uint64_t talkerA = 0xFEDCBA9876543210ull, talkerB = 0x123456789ABCDEF0ull;
            Require(sessionA->LocalAdd(talkerA) && sessionB->LocalAdd(talkerB), "Cannot add original local voice talkers");
            sessionA->SetTalkerNetworkIdLocal(talkerA, 0x23); sessionB->SetTalkerNetworkIdLocal(talkerB, 0x42);
            Require(sessionA->Add(talkerB, 0x42, key, addrB) && sessionB->Add(talkerA, 0x23, key, addrA), "Cannot add original remote voice endpoints");
            first.Work(123); second.Work(123); // Consume the original empty keepalives.
            std::vector<std::vector<uint8_t>> expected;
            for (unsigned i = 0; i < 12; ++i) {
                expected.emplace_back(i % 3 == 0 ? 2 : i % 3 == 1 ? 257 : 1017, uint8_t(i + 1));
                Require(sessionA->Send(talkerA, expected.back().data(), expected.back().size()), "Original voice session could not enqueue a packet");
            }
            second.Work(124);
            Require(callbackB.received.size() == 10, "ComSat changed the ten-packet dispatch budget or truncated a larger following packet");
            second.Work(125);
            Require(callbackB.received == expected && callbackB.talker == talkerA, "ComSat dropped the next packet or changed a 64-bit talker/payload");
            uint8_t wrongKey[16]{};
            first.Send(0x1234, 0x23, wrongKey, addrB, raw.data(), 1);
            first.Send(0x1235, 0x23, key, addrB, raw.data(), 1);
            first.Send(0x1234, 0x24, key, addrB, raw.data(), 1);
            a.SendTo(addrB, raw.data(), 3);
            Socket stranger; Require(stranger.Bind(0), "Cannot bind wrong-origin voice fixture");
            stranger.SendTo(addrB, packet, sizeof(packet));
            second.Work(126);
            Require(callbackB.received == expected, "Voice callback accepted a wrong signature/session/talker/origin or short packet");
            Require(sessionB->Add(talkerA + 1, 0x24, key, addrA) && sessionB->m_endpoints.Count() == 1 && sessionB->Remove(talkerA),
                "Original voice endpoint sharing/removal changed");
            uint64_t resolved = 0; const uint8_t (*resolvedKey)[16] = nullptr;
            Require(sessionB->Resolve(0x24, addrA, resolved, resolvedKey) && resolved == talkerA + 1 && !std::memcmp(*resolvedKey, key, 16),
                "Removing a talker released a shared voice endpoint");
            first.DestroySession(sessionA); second.DestroySession(sessionB);
        }
        Require(allocator.live.empty(), "Original voice network teardown leaked owned allocations");
        Require(fcntl(firstFD, F_GETFD) == -1 && errno == EBADF && fcntl(secondFD, F_GETFD) == -1 && errno == EBADF,
            "Original virtual voice socket teardown did not close native descriptors");
    }
    std::fputs("Original ComSat network: 520 bit-boundary streams, MD5/HMAC and exact UDP bytes, 64-bit talkers, mixed-size ten-packet batches, invalid packet rejection, endpoint sharing and two native lifetimes passed\n", stderr);
}
