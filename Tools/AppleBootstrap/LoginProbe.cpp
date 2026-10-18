#include "WoW/Source/WowServices/GruntLogin.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "WoW/Source/WowServices/SRP6_Random.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "WoW/Source/WowServices/BigInteger.h"
#include "Engine/Source/Os/OsSecureRandom.h"
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::vector<uint8_t> Hex(const char* text) {
    std::vector<uint8_t> bytes;
    for (; *text; text += 2) { unsigned byte; Require(std::sscanf(text, "%2x", &byte) == 1, "Invalid login fixture"); bytes.push_back(byte); }
    return bytes;
}
// Synthetic 256-bit field and credentials; expected values were computed
// independently with Python integer modular powers and hashlib, not this code.
const auto prime = Hex("43ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
const auto salt = Hex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
const auto server = Hex("4a12d2cbde24c5e7990074bd63f61e0d2a5ae5511b0552b244c6ba922f494759");
using Bytes = std::vector<uint8_t>;
Bytes Hash(std::initializer_list<Bytes> parts) {
    SHA1_CONTEXT context; SHA1_Init(&context);
    for (const auto& bytes : parts) SHA1_Update(&context, bytes.data(), bytes.size());
    Bytes hash(20); SHA1_Final(hash.data(), &context); return hash;
}
struct Proof { Bytes key, client, server; };
Proof PeerProof(const Bytes& publicKey) {
    using Big = std::unique_ptr<BigData, decltype(&BigIntegerFree)>;
    auto number = [](const Bytes& bytes) { return Big(BigIntegerFromBytes(bytes.data(), bytes.size()), BigIntegerFree); };
    auto n = number(prime), a = number(publicKey);
    auto v = number(Hex("9cd13cac8b7a874022c3fbc93af1e753689d0fc3b4ef2637f6678ff0e55ab233"));
    auto b = number(Hex("efcdab8967452301efcdab896745230100000000000000000000000000000000"));
    auto u = number(Hash({publicKey, server})), secret = number(Bytes{0});
    BigIntegerModExp(secret.get(), v.get(), u.get(), n.get());
    BigIntegerMul(secret.get(), secret.get(), a.get());
    BigIntegerMod(secret.get(), secret.get(), n.get());
    BigIntegerModExp(secret.get(), secret.get(), b.get(), n.get());
    Bytes raw(32); BigIntegerToBytes(secret.get(), raw.data(), raw.size());
    Proof proof; proof.key.resize(40); SHA1_InterleaveHash(proof.key.data(), raw.data(), raw.size());
    auto p = Hash({prime}), g = Hash({Bytes{5}});
    for (size_t i = 0; i < p.size(); ++i) p[i] ^= g[i];
    const char account[] = "APPLECHECK";
    proof.client = Hash({p, Hash({Bytes(account, account + 10)}), salt, publicKey, server, proof.key});
    proof.server = Hash({publicKey, proof.client, proof.key});
    return proof;
}
void CheckSrp() {
    const auto publicKey = Hex("0d4fb3c161d6de83430519828efdb4769d3535154f4de1f1e53e5c271b86417b");
    const auto session = Hex("302f0a1c95799ee149d0ce4b43fed5e6b2568a411dd08177442ea0b028e9d51f56e7e830a2e46802");
    const auto clientProof = Hex("2f786c87e091d60bf10072d792383111d8de64e1");
    auto serverProof = Hex("871f72020b8cbbc07ee22d3c3b001da73cb00906");
    const char seed[] = "Apple SRP fixture";
    const uint8_t generator = 5;
    for (unsigned run = 0; run < 4; ++run) {
        SRP6_Client client{}; SRP6_Random random(seed, sizeof(seed) - 1);
        Require(!client.BeginAuthentication("APPLECHECK", "SYNTHETIC"), "Original SRP credentials failed");
        auto publicInput = server;
        if (run == 2) publicInput.assign(32, 0);
        if (run == 3) publicInput = prime;
        const int result = client.CalculateProof(prime.data(), prime.size(), &generator, 1, salt.data(), salt.size(), publicInput.data(), publicInput.size(), random);
        if (run >= 2) { Require(result == -2, "Original SRP accepted an invalid server public key"); continue; }
        Require(!result && !std::memcmp(client.m_publicKey, publicKey.data(), publicKey.size()) &&
            !std::memcmp(client.m_sessionKey, session.data(), session.size()) &&
            !std::memcmp(client.m_clientProof, clientProof.data(), clientProof.size()), "Original SRP differs from independent reference");
        if (run == 1) serverProof[0] ^= 1;
        Require((client.VerifyServerProof(serverProof.data(), serverProof.size()) == 0) == (run == 0), "Original SRP server proof validation failed");
    }
    std::array<uint8_t, 32> random1{}, random2{};
    OsSecureRandom(random1.data(), random1.size()); OsSecureRandom(random2.data(), random2.size());
    Require(random1 != random2, "Native authentication entropy repeated");
    std::fprintf(stderr, "Original SRP: public key, 40-byte session key, client/server proofs and invalid public keys match independent reference\n");
}
struct Socket { int value = -1; ~Socket() { if (value >= 0) close(value); } };
void Ready(int socket, short events) {
    pollfd fd{socket, events, 0}; int result;
    do { result = poll(&fd, 1, 5000); } while (result < 0 && errno == EINTR);
    Require(result > 0, "Login loopback timed out");
}
void Transfer(int socket, uint8_t* bytes, size_t size, bool output) {
    while (size) {
        Ready(socket, output ? POLLOUT : POLLIN);
        auto count = output ? send(socket, bytes, size, 0) : recv(socket, bytes, size, 0);
        if (count < 0 && (errno == EINTR || errno == EAGAIN)) continue;
        Require(count > 0, "Login loopback transfer failed"); bytes += count; size -= count;
    }
}
struct Response : LoginResponse {
    std::mutex lock;
    std::condition_variable changed;
    LOGIN_STATE state = LOGIN_STATE_INITIALIZED;
    LOGIN_RESULT result = LOGIN_OK;
    unsigned realmReplies = 0;
    uint32_t realmResult = UINT32_MAX;
    const char* GetLoginServer() override { return "127.0.0.1"; }
    int32_t GetLoginServerType() override { return 0; }
    void JoinRealmResult(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void LoginServerStatus(LOGIN_STATE newState, LOGIN_RESULT newResult, const char*, const char*, const char*, uint16_t) override {
        std::lock_guard<std::mutex> guard(lock); state = newState; result = newResult; changed.notify_all();
    }
    void RealmEnumCallback(uint32_t result) override {
        std::lock_guard<std::mutex> guard(lock); realmResult = result; ++realmReplies; changed.notify_all();
    }
    void Wait(LOGIN_STATE expected) {
        std::unique_lock<std::mutex> guard(lock);
        Require(changed.wait_for(guard, std::chrono::seconds(5), [&] { return state == expected; }), "Original Grunt login callback timed out");
    }
    void WaitRealms(unsigned count = 1) {
        std::unique_lock<std::mutex> guard(lock);
        Require(changed.wait_for(guard, std::chrono::seconds(5), [&] { return realmReplies >= count; }), "Original realm list callback timed out");
    }
};
void CheckGrunt() {
    for (unsigned run = 0; run < 3; ++run) {
        WowConnection::InitOsNet(nullptr, nullptr, 2, false);
        struct Network { ~Network() { WowConnection::DestroyOsNet(); } } network;
        Socket listener; listener.value = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{}; address.sin_len = sizeof(address); address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(listener.value >= 0 && !bind(listener.value, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(listener.value, 4), "Cannot create login loopback listener");
        socklen_t length = sizeof(address); getsockname(listener.value, reinterpret_cast<sockaddr*>(&address), &length);
        char target[40]; std::snprintf(target, sizeof(target), "127.0.0.1:%u", ntohs(address.sin_port));
        Response response;
        GruntLogin login; login.Init(&response); login.SetLogonCreds("applecheck", "synthetic");
        login.Logon(target, nullptr);
        Ready(listener.value, POLLIN); Socket peer; peer.value = accept(listener.value, nullptr, nullptr);
        Require(peer.value >= 0 && WowSocketNonBlocking(peer.value), "Cannot accept original Grunt login");
        uint8_t header[4]; Transfer(peer.value, header, sizeof(header), false);
        Require(header[0] == 0 && header[1] == 8, "Original Grunt hello opcode changed");
        const unsigned size = header[2] | (unsigned(header[3]) << 8);
        std::vector<uint8_t> hello(size); Transfer(peer.value, hello.data(), hello.size(), false);
        Require(size == 40 && hello[4] == 3 && hello[5] == 3 && hello[6] == 5 && hello[7] == 0x34 && hello[8] == 0x30 &&
            !std::memcmp(hello.data() + 9, "68x\0niW\0", 8) && hello[29] == 10 && !std::memcmp(hello.data() + 30, "APPLECHECK", 10), "Original Grunt version/dialect/account packing changed");
        if (!run) {
            uint8_t denial[] = {0, 0, Grunt::GRUNT_RESULT_UNKNOWN_ACCOUNT};
            for (auto& byte : denial) Transfer(peer.value, &byte, 1, true);
            response.Wait(LOGIN_STATE_FAILED);
            Require(response.result == LOGIN_UNKNOWN_ACCOUNT, "Original login failure mapping changed");
        } else {
            // Test fixture only: supply an explicit synthetic version checksum;
            // this does not replace the application's real version/signature path.
            Bytes versionChallenge(16, 0x27), versionChecksum(20, 0x41);
            CDataStore challenge;
            challenge.Put(uint8_t(0)).Put(uint8_t(0)).Put(uint8_t(0)).PutData(server.data(), server.size());
            challenge.Put(uint8_t(1)).Put(uint8_t(5)).Put(uint8_t(32)).PutData(prime.data(), prime.size());
            challenge.PutData(salt.data(), salt.size()).PutData(versionChallenge.data(), versionChallenge.size()).Put(uint8_t(0));
            for (uint32_t i = 0; i < challenge.m_size; ++i) Transfer(peer.value, challenge.m_data + i, 1, true);
            response.Wait(LOGIN_STATE_CHECKINGVERSIONS);
            login.ProveVersion(versionChecksum.data());
            Bytes wire(75); Transfer(peer.value, wire.data(), wire.size(), false);
            Bytes publicKey(wire.begin() + 1, wire.begin() + 33); const auto proof = PeerProof(publicKey);
            const auto versionHash = Hash({publicKey, versionChecksum});
            Require(wire[0] == 1 && !std::memcmp(wire.data() + 33, proof.client.data(), 20) &&
                !std::memcmp(wire.data() + 53, versionHash.data(), 20) && !wire[73] && !wire[74], "Original Grunt authentication/version proof differs from peer");
            CDataStore accepted;
            accepted.Put(uint8_t(1)).Put(uint8_t(0)).PutData(proof.server.data(), 20).Put(uint32_t(0)).Put(uint32_t(0)).Put(uint16_t(0));
            for (uint32_t i = 0; i < accepted.m_size; ++i) Transfer(peer.value, accepted.m_data + i, 1, true);
            response.Wait(LOGIN_STATE_AUTHENTICATED);
            Require(!std::memcmp(login.m_sessionKey, proof.key.data(), 40), "Original login published a different session key");
            login.GetRealmList(); uint8_t query[5]; Transfer(peer.value, query, sizeof(query), false);
            Require(query[0] == 0x10 && !query[1] && !query[2] && !query[3] && !query[4], "Original realm query changed");
            CDataStore realms;
            realms.Put(uint8_t(0x10)).Put(uint16_t(0)).Put(uint32_t(0)).Put(uint16_t(1));
            realms.Put(uint8_t(0)).Put(uint8_t(0)).Put(uint8_t(4)).PutString("Apple Loopback").PutString("127.0.0.1:8085");
            realms.Put(0.5f).Put(uint8_t(2)).Put(uint8_t(1)).Put(uint8_t(7));
            realms.Put(uint8_t(3)).Put(uint8_t(3)).Put(uint8_t(5)).Put(uint16_t(12340)).Put(uint16_t(0x10));
            realms.Set(1, uint16_t(realms.m_size - 3));
            for (uint32_t i = 0; i < realms.m_size; ++i) Transfer(peer.value, realms.m_data + i, 1, true);
            response.WaitRealms();
            Require(!response.realmResult && response.m_realmList.m_count == 1, "Original realm enumeration failed");
            const auto& realm = response.m_realmList[0];
            Require(!std::strcmp(realm.m_realmName, "Apple Loopback") && !std::strcmp(realm.m_realmAddress, "127.0.0.1:8085") &&
                realm.m_versionBuild == 12340 && realm.m_numCharacters == 2 && realm.m_population == 0.5f, "Original realm fields changed");
            // A completed short reply followed by an incomplete longer reply
            // must retain its tail while compacting the original receive buffer.
            CDataStore empty;
            empty.Put(uint8_t(0x10)).Put(uint16_t(8)).Put(uint32_t(0)).Put(uint16_t(0)).Put(uint16_t(0x10));
            Bytes combined(empty.m_data, empty.m_data + empty.m_size);
            combined.insert(combined.end(), realms.m_data, realms.m_data + realms.m_size - 1);
            Transfer(peer.value, combined.data(), combined.size(), true);
            response.WaitRealms(2);
            Transfer(peer.value, realms.m_data + realms.m_size - 1, 1, true);
            response.WaitRealms(3);
            Require(!response.realmResult && response.m_realmList.m_count == 1, "Original Grunt lost a partially buffered realm reply");
        }
        std::fprintf(stderr, "Original Grunt cycle %u: 12340 hello, %s and teardown passed\n", run,
            run ? "SRP peer proof, version proof, session key and fragmented realm list" : "fragmented account rejection");
    }
}
}
void CheckGameLogin() { CheckSrp(); CheckGrunt(); }
