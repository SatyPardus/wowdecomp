#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowServices/SocketPlatform.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/BannedAddOnsRec.h"
#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "WoW/Source/Client.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Storm/H/storm.h"
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <poll.h>
#include <ostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <functional>
#include <filesystem>
#include <sstream>

void CheckAccountData(NetClient&, const std::function<void(CDataStore&)>&,
                      const std::function<std::vector<uint8_t>()>&, int);

extern WowClientDB<BannedAddOnsRec> g_bannedAddOnsDB;

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

std::vector<uint8_t> ReceivePacket(int fd, WowConnection* crypto = nullptr) {
    uint8_t header[2]; Transfer(fd, header, 2, false);
    if (crypto) SARC4ProcessBuffer(header, 2, &crypto->m_receiveKey, &crypto->m_receiveKey);
    size_t size = size_t(header[0]) * 256 + header[1];
    Require(size >= 4 && size < 32768, "Invalid client realm packet length");
    std::vector<uint8_t> bytes(size); Transfer(fd, bytes.data(), bytes.size(), false);
    if (crypto) SARC4ProcessBuffer(bytes.data(), 4, &crypto->m_receiveKey, &crypto->m_receiveKey);
    return bytes;
}
struct Response : RealmResponse {
    std::thread::id thread = std::this_thread::get_id();
    unsigned connects = 0, disconnects = 0, auths = 0, addons = 0, lists = 0, failures = 0;
    uint8_t authResult = 0;
    bool validList = false;
    std::vector<CHARACTER_INFO> characters;
    void CheckThread() { Require(std::this_thread::get_id() == thread, "Realm callback escaped original Event thread"); }
    void LoginServerResult(LOGIN_STATE, LOGIN_RESULT, const char*, const char*, const char*) override { throw std::runtime_error("Unexpected login callback"); }
    void RealmListReceived(TSFixedArray<REALM_INFO>&) override { throw std::runtime_error("Unexpected realm-list callback"); }
    void HandleAuthResponse(RealmConnection*, uint8_t result) override { CheckThread(); ++auths; authResult = result; }
    void AddOnListReceived(RealmConnection*) override { CheckThread(); ++addons; }
    void CharacterListReceived(RealmConnection*, const TSFixedArray<CHARACTER_INFO>& values, int32_t success) override {
        CheckThread(); validList = success; characters.clear();
        if (values.m_count) characters.assign(values.m_data, values.m_data + values.m_count);
        ++lists;
    }
    void GameServerResult(RealmConnection*, const char*, const char*, const char*) override { CheckThread(); }
    void MessageReceived(RealmConnection*, uint32_t) override { throw std::runtime_error("Unexpected game message callback"); }
    void OnChatReceived(RealmConnection*, WOWGUID, const char*, uint8_t) override { throw std::runtime_error("Unexpected chat callback"); }
    void OnAccountOnline(RealmConnection*, int32_t, WOWGUID) override { throw std::runtime_error("Unexpected account callback"); }
    void OnAFKMonitorInfoResponse(RealmConnection*, WOWGUID, int32_t, int32_t, int32_t, int32_t) override { throw std::runtime_error("Unexpected AFK callback"); }
    void HandleConnect(RealmConnection*) override { CheckThread(); ++connects; }
    void HandleDisconnect(RealmConnection*) override { CheckThread(); ++disconnects; }
    void HandleCantConnect(RealmConnection*) override { CheckThread(); ++failures; }
};
struct Realm : RealmConnection {
    unsigned loginFailures = 0, logouts = 0;
    uint8_t lastLoginFailure = 0;
    explicit Realm(Response& response) : RealmConnection(&response) {}
    void HandleCharacterCreate(uint8_t) override { throw std::runtime_error("Unexpected creation callback"); }
    void HandleCharacterDelete(uint8_t) override { throw std::runtime_error("Unexpected deletion callback"); }
    void HandleCharacterLoginFailed(uint8_t result) override { ++loginFailures; lastLoginFailure = result; }
    void HandleLogoutComplete() override { ++logouts; }
    void HandleLogoutResponse(int32_t, int32_t) override { throw std::runtime_error("Unexpected logout response callback"); }
    void HandleLogoutAbortAck() override { throw std::runtime_error("Unexpected logout abort callback"); }
};
HPROPCONTEXT realmContext;
template<class Predicate> void Pump(Predicate ready) {
    auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!ready()) {
        Require(std::chrono::steady_clock::now() < limit, "Original realm event timed out");
        Require(EventProcessPending(), "Original Event loop stopped during realm check");
        PropSelectContext(realmContext);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
int32_t Initialize(const void*, void*) {
    realmContext = PropGetSelectedContext();
    EventRegister(EVENT_ID_POLL, [](const void* data, void* param) -> int32_t { RealmConnection::PollNet(data, param); return 1; });
    return 1;
}
void Append32(std::vector<uint8_t>& bytes, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) bytes.push_back(uint8_t(value >> (i * 8)));
}
void Character(CDataStore& packet, unsigned index) {
    packet.Put(uint64_t(0x0102030405060708ull + index)); packet.PutString(index ? "SecondHero" : "AppleHero");
    for (unsigned i = 0; i < 9; ++i) packet.Put(uint8_t(i + 1 + index));
    packet.Put(uint32_t(12)); packet.Put(uint32_t(0));
    packet.Put(float(-9465 + int(index))); packet.Put(float(64)); packet.Put(float(85));
    packet.Put(uint32_t(42)); packet.Put(uint32_t(0x100)); packet.Put(uint32_t(0x8)); packet.Put(uint8_t(1));
    packet.Put(uint32_t(165)); packet.Put(uint32_t(8)); packet.Put(uint32_t(1));
    for (unsigned i = 0; i < 23; ++i) { packet.Put(uint32_t(1000 + i)); packet.Put(uint8_t(i)); packet.Put(uint32_t(2000 + i)); }
}
}

void CheckSignature() {
    // Independent Python pow/SHA1 fixture, p=2^521-1, q=2^127-1,
    // e=65537; this key belongs only to the probe, never asset policy.
    const uint8_t modulus[] = {
        0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xfd,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,
    };
    const uint8_t signature[] = {
        0xef,0xbd,0x53,0x9e,0x48,0x4d,0xf8,0x7a,0x2d,0x68,0x22,0x05,0x30,0xfc,0x10,0x67,
        0xc2,0xfb,0x63,0x46,0x16,0x42,0xf1,0x99,0xaa,0x56,0x47,0xc5,0xff,0x8c,0x8b,0xd6,
        0xa3,0x5b,0xdb,0xad,0x3f,0x17,0x91,0x6d,0xf8,0x07,0x07,0x4d,0x96,0x65,0x58,0xbc,
        0xe5,0x77,0x6a,0x5c,0x65,0x73,0xb0,0xff,0x91,0xb3,0xc9,0xd8,0x29,0x72,0xa8,0xce,
        0x0a,0xa3,0xd4,0xd5,0xab,0xd9,0x48,0x81,0x66,0xbc,0x26,0x43,0x43,0xc4,0xf8,0x98,
        0x55,
    };

    const uint8_t exponent[] = {1, 0, 1, 0};
    std::vector<uint8_t> bytes;
    for (unsigned i = 0; i < 300; ++i) bytes.push_back(uint8_t(i * 31 + 17));
    bytes.insert(bytes.end(), {'N', 'G', 'I', 'S'});
    bytes.insert(bytes.end(), signature, signature + sizeof(signature));
    auto verify = [&](const std::vector<uint8_t>& input, unsigned chunk) {
        SSignatureData* token = nullptr; SSignatureVerifyStream_Begin(&token, sizeof(modulus), sizeof(exponent));
        Require(SSignatureVerifyStream_GetSignatureLength(token) == sizeof(modulus) + 4, "Original signature trailer length changed");
        for (size_t offset = 0; offset < input.size();) {
            const size_t length = std::min(size_t(chunk), input.size() - offset);
            SSignatureVerifyStream_ProvideData(token, input.data() + offset, length); offset += length;
        }
        return SSignatureVerifyStream_Finish(token, modulus, exponent) != 0;
    };
    for (unsigned chunk : {1u, 63u, 64u, 65u, 84u, 85u, 4096u}) {
        Require(verify(bytes, chunk), "Original RSA/SHA1 stream signature rejected independent fixture");
        auto changed = bytes; changed[139] ^= 1; Require(!verify(changed, chunk), "Modified signed payload was accepted");
        changed = bytes; changed[300] ^= 1; Require(!verify(changed, chunk), "Modified signature marker was accepted");
        changed = bytes; changed.back() ^= 1; Require(!verify(changed, chunk), "Modified signature was accepted");
        changed.pop_back(); Require(!verify(changed, chunk), "Truncated signature was accepted");
    }
    Require(SCrcCompute("123456789", 9, nullptr, 7) == 0xcbf43926, "Original addon CRC differs from standard reference");
    std::fprintf(stderr, "Original addon signature: independent RSA/SHA1 fixture, seven stream boundaries and modified/truncated rejection passed\n");
}

void CheckClientCache();
void CheckGameRealm() {
    CheckClientCache();
    CheckSignature();
    PropInitialize(); EventInitialize(1, 0);
    struct Context { ~Context() { PropSelectContext(nullptr); EventDestroy(); PropDestroy(); NetClient::SetDisconnectHandler(nullptr); } } context;
    EventCreateContextEx(0, Initialize, nullptr, 1, 0); EventProcessPending(); PropSelectContext(realmContext);
    NetClient::SetDisconnectHandler([](NetClient*) -> int32_t { return 1; });
    for (unsigned cycle = 0; cycle < 3; ++cycle) {
        Socket listener; listener.fd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{}; address.sin_len = sizeof(address); address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Require(listener.fd >= 0 && !bind(listener.fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) && !listen(listener.fd, 4), "Cannot bind realm loopback listener");
        socklen_t length = sizeof(address); Require(!getsockname(listener.fd, reinterpret_cast<sockaddr*>(&address), &length), "Cannot query realm port");
        Response response; Realm realm(response);
        LoginData login{}; std::strcpy(login.m_account, "APPLECHECK"); login.m_loginServerID = 2; login.m_loginServerType = 1;
        for (unsigned i = 0; i < sizeof(login.m_sessionKey); ++i) login.m_sessionKey[i] = uint8_t(i + 1);
        realm.SetLoginData(&login); realm.SetSelectedRealm(3, 4, 5);
        realm.ConnectInternal("127.0.0.1", ntohs(address.sin_port));
        Ready(listener.fd, POLLIN); Socket peer; peer.fd = accept(listener.fd, nullptr, nullptr);
        Require(peer.fd >= 0 && WowSocketNonBlocking(peer.fd), "Cannot accept realm connection");
        Pump([&] { return response.connects == 1; });
        CDataStore challenge; challenge.Put(uint16_t(SMSG_AUTH_CHALLENGE)); challenge.Put(uint32_t(0)); challenge.Put(uint32_t(0x87654321));
        for (uint32_t i = 0; i < 8; ++i) challenge.Put(i + 1);
        SendPacket(peer.fd, challenge);
        Pump([&] { return realm.m_serverConnection->m_encrypt; });
        auto bytes = ReceivePacket(peer.fd); CDataStore auth(bytes.data(), bytes.size());
        uint32_t opcode, build, server, type, seed, region, group, id; uint64_t proof; char account[128];
        auth.Get(opcode); auth.Get(build); auth.Get(server); auth.GetString(account, sizeof(account)); auth.Get(type); auth.Get(seed);
        auth.Get(region); auth.Get(group); auth.Get(id); auth.Get(proof); uint8_t digest[20]; auth.GetArray(digest, sizeof(digest));
        Require(opcode == CMSG_AUTH_SESSION && build == 12340 && server == 2 && !std::strcmp(account, login.m_account) && type == 1 && region == 3 && group == 4 && id == 5 && proof == 0 && auth.IsRead(), "Original realm authentication packet fields changed");
        std::vector<uint8_t> input(login.m_account, login.m_account + std::strlen(login.m_account));
        Append32(input, 0); Append32(input, seed); Append32(input, 0x87654321); input.insert(input.end(), login.m_sessionKey, login.m_sessionKey + 40);
        uint8_t expected[20]; CC_SHA1(input.data(), static_cast<CC_LONG>(input.size()), expected);
        Require(!std::memcmp(digest, expected, sizeof(digest)), "Realm session proof differs from independent system SHA1");
        WowConnection crypto(nullptr, nullptr); crypto.SetEncryptionKey(login.m_sessionKey, sizeof(login.m_sessionKey), 0, nullptr, 0);
        if (cycle == 2) {
            CDataStore denied; denied.Put(uint16_t(SMSG_AUTH_RESPONSE)); denied.Put(uint8_t(13)); SendPacket(peer.fd, denied, &crypto);
            Pump([&] { return response.auths == 1; });
            Require(response.authResult == 13 && !realm.m_authenticated, "Rejected realm authentication became authenticated");
            peer.Reset(); Pump([&] { return response.disconnects == 1; });
            std::fprintf(stderr, "Original realm: authentication rejection and disconnect passed\n");
            continue;
        }
        CDataStore queued; queued.Put(uint16_t(SMSG_AUTH_RESPONSE)); queued.Put(uint8_t(27)); queued.Put(uint32_t(90)); queued.Put(uint8_t(1)); queued.Put(uint32_t(45)); queued.Put(uint8_t(2)); queued.Put(uint32_t(7)); queued.Put(uint8_t(1)); SendPacket(peer.fd, queued, &crypto);
        Pump([&] { return response.auths == 1; });
        Require(response.authResult == 27 && !realm.m_authenticated && realm.m_queuePosition == 7 && realm.m_freeCharacterMigration == 1, "Original realm queue response changed");
        CDataStore accepted; accepted.Put(uint16_t(SMSG_AUTH_RESPONSE)); accepted.Put(uint8_t(12)); accepted.Put(uint32_t(80)); accepted.Put(uint8_t(2)); accepted.Put(uint32_t(40)); accepted.Put(uint8_t(2)); SendPacket(peer.fd, accepted, &crypto);
        Pump([&] { return response.auths == 2; });
        Require(response.authResult == 12 && realm.m_authenticated && realm.m_billingTimeRemaining == 80 && realm.m_billingFlags == 2 && realm.m_billingTimeRested == 40 && realm.m_accountExpansion == 2, "Original authentication state/billing changed");
        CDataStore addon; addon.Put(uint16_t(SMSG_ADDON_INFO)); addon.Put(uint32_t(0)); SendPacket(peer.fd, addon, &crypto);
        Pump([&] { return response.addons == 1; });
        CDataStore cache; cache.Put(uint16_t(SMSG_CACHE_VERSION)); cache.Put(uint32_t(cycle + 123)); SendPacket(peer.fd, cache, &crypto);
        Pump([&] { return g_creatureDBCache.m_version == cycle + 123; });
        Require(g_itemDBCache.m_version == cycle + 123 && g_gameObjectDBCache.m_version == cycle + 123, "Original cache invalidation was not dispatched");
        realm.RequestCharacterEnum(); bytes = ReceivePacket(peer.fd, &crypto); CDataStore request(bytes.data(), bytes.size()); request.Get(opcode);
        Require(opcode == CMSG_ENUM_CHARACTERS && request.IsRead(), "Original character enumeration request changed");
        CDataStore characters; characters.Put(uint16_t(SMSG_ENUM_CHARACTERS_RESULT)); characters.Put(uint8_t(2)); Character(characters, 0); Character(characters, 1);
        for (uint32_t i = 0; i < 10; ++i) characters.Put(i + 20);
        SendPacket(peer.fd, characters, &crypto); Pump([&] { return response.lists == 1; });
        Require(response.validList && response.characters.size() == 2 && realm.m_restrictHuman == 20 && realm.m_restrictBloodElf == 29, "Character list or race restrictions changed");
        const auto& hero = response.characters[0];
        Require(hero.m_guid.m_guid == 0x0102030405060708ull && !std::strcmp(hero.m_name, "AppleHero") && hero.m_experienceLevel == 9 && hero.m_position.x == -9465 && hero.m_inventoryItemDisplayID[22] == 1022 && hero.m_inventoryItemType[22] == 22 && hero.m_inventoryItemVisualID[22] == 2022, "Original character details changed");
        CheckAccountData(realm, [&](CDataStore& packet) { SendPacket(peer.fd, packet, &crypto); },
            [&] { return ReceivePacket(peer.fd, &crypto); }, peer.fd);
        realm.RequestCharacterLogin(hero.m_guid, 0); bytes = ReceivePacket(peer.fd, &crypto); CDataStore enter(bytes.data(), bytes.size()); uint64_t guid; enter.Get(opcode); enter.Get(guid);
        Require(opcode == CMSG_PLAYER_LOGIN && guid == hero.m_guid.m_guid && enter.IsRead(), "Original selected-character login request changed");
        CDataStore rejected; rejected.Put(uint16_t(SMSG_CHARACTER_LOGIN_FAILED)); rejected.Put(uint8_t(0x51)); SendPacket(peer.fd, rejected, &crypto);
        Pump([&] { return realm.loginFailures == 1; }); Require(realm.lastLoginFailure == 0x51, "Character login failure was lost");
        CDataStore truncated; truncated.Put(uint16_t(SMSG_ENUM_CHARACTERS_RESULT)); truncated.Put(uint8_t(1)); truncated.Put(uint32_t(0)); SendPacket(peer.fd, truncated, &crypto);
        Pump([&] { return response.lists == 2; }); Require(!response.validList && response.characters.empty(), "Truncated character list was published");
        CDataStore logout; logout.Put(uint16_t(SMSG_LOGOUT_COMPLETE)); SendPacket(peer.fd, logout, &crypto); Pump([&] { return realm.logouts == 1; });
        peer.Reset(); Pump([&] { return response.disconnects == 1; });
        std::fprintf(stderr, "Original realm cycle %u: Event poll, 12340 session proof, encrypted queue/auth/addons/cache/characters, login request/rejection and logout passed\n", cycle + 1);
    }
    Require(NetClient::s_clientCount == 0 && RealmConnection::s_allRealmConnections.IsEmpty(), "Original realm teardown retained a connection");
}

void CheckRealmMetadata(const ClientData& client, std::ostream& out) {
    Require(CheckDataFilesExist() && client.Locale() == s_localeCode,
        "Native cache metadata did not select the mounted installation and locale");
    std::vector<const ClientData::Mount*> listed;
    const ClientData::Mount* interface = nullptr;
    const ClientData::Mount* localized = nullptr;
    uint8_t expansion = 0;
    for (const auto& mount : client.Mounts()) {
        if (mount.priority >= 64) { listed.push_back(&mount); continue; }
        const auto name = std::filesystem::path(mount.path).filename().string();
        if (!SStrCmpI(name.c_str(), "interface.MPQ", STORM_MAX_STR)) interface = &mount;
        if (!SStrCmpI(name.c_str(), ("locale-" + client.Locale() + ".MPQ").c_str(), STORM_MAX_STR)) localized = &mount;
        if (!SStrCmpI(name.c_str(), "expansion.MPQ", STORM_MAX_STR)) expansion = std::max<uint8_t>(expansion, 1);
        if (!SStrCmpI(name.c_str(), "lichking.MPQ", STORM_MAX_STR)) expansion = 2;
    }
    if (interface || localized) listed.push_back(interface ? interface : localized);
    Require(ClientData::Selected() == &client && GetClientExpansionLevel() == expansion,
            "Native expansion capability differs from mounted base archives");
    const char* prefix = "Interface\\AddOns\\";
    std::vector<std::string> reference, stoppedReference;
    for (const auto* mount : listed) {
        SArchive borrowed{0, mount->archive.get()};
        SFileImpl* file = nullptr;
        if (!SFile::OpenEx(&borrowed, "(listfile)", 0, &file)) continue;
        struct Close { SFileImpl* file; ~Close() { SFileCloseFile(file); } } close{file};
        const auto length = SFileGetFileSize(file, nullptr);
        Require(length < 32 * 1024 * 1024, "Unexpected interface listfile size");
        std::string contents(length, '\0');
        uint32_t read = 0;
        Require(SFileReadFile(file, contents.data(), length, &read, nullptr, 0) && read == length,
                "Cannot read interface listfile reference bytes");
        std::istringstream stream(contents);
        std::string line;
        bool first = true;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (SStrCmpI(line.c_str(), prefix, std::strlen(prefix))) continue;
            const char* extension = std::strrchr(line.c_str(), '.');
            if (extension && !SStrCmpI(extension, ".md5", STORM_MAX_STR)) continue;
            reference.push_back(line);
            if (first) { stoppedReference.push_back(line); first = false; }
        }
    }
    const auto collect = [](const char* name, void* parameter) -> int32_t {
        static_cast<std::vector<std::string>*>(parameter)->emplace_back(name); return 1;
    };
    std::vector<std::string> actual, folded, stopped;
    ListInterfaceArchive(prefix, collect, &actual);
    ListInterfaceArchive("interface\\addons\\", collect, &folded);
    ListInterfaceArchive(prefix, [](const char* name, void* parameter) -> int32_t {
        static_cast<std::vector<std::string>*>(parameter)->emplace_back(name); return 0;
    }, &stopped);
    Require(!actual.empty() && actual == reference && folded == reference && stopped == stoppedReference,
            "Original interface enumeration changed patch/base order, casing or per-archive stopping");
    size_t tocCount = 0;
    for (const auto& name : actual) {
        const char* extension = std::strrchr(name.c_str(), '.');
        if (extension && !SStrCmpI(extension, ".toc", STORM_MAX_STR)) ++tocCount;
    }
    Require(tocCount > 0, "Interface enumeration found no original addon manifests");
    std::string root = std::string(SFile::m_basePath) + SFile::GetDataPath();
    std::replace(root.begin(), root.end(), '\\', '/');
    {
        ClientData nested(root, client.Locale());
        Require(ClientData::Selected() == &nested && GetClientExpansionLevel() == expansion,
                "Nested resource installation did not select native capabilities");
    }
    Require(ClientData::Selected() == &client && GetClientExpansionLevel() == expansion,
            "Native resource installation did not restore its previous selection");
    std::vector<std::string> restored;
    ListInterfaceArchive(prefix, collect, &restored);
    Require(restored == actual, "Restoring the resource installation changed interface enumeration");
    out << "Original interface enumeration: " << actual.size() << " ordered entries / " << tocCount
        << " addon manifests, case-insensitive prefix, per-archive stop and mount restoration; expansion " << unsigned(expansion) << " passed.\n";
    const auto bytes = client.Read(BannedAddOnsRec::GetFilename(), 4 * 1024 * 1024).bytes;
    Require(bytes.size() >= 20 && !std::memcmp(bytes.data(), "WDBC", 4), "Invalid addon policy DBC");
    uint32_t count, rowSize; std::memcpy(&count, bytes.data() + 4, 4); std::memcpy(&rowSize, bytes.data() + 12, 4);
    Require(rowSize == 44 && 20 + uint64_t(count) * rowSize <= bytes.size(), "Invalid addon policy record range");
    if (!g_bannedAddOnsDB.m_loaded) g_bannedAddOnsDB.Load(__FILE__, __LINE__);
    Require(g_bannedAddOnsDB.m_numRecords == int32_t(count), "Original addon policy record count changed");
    static_assert(sizeof(BannedAddOnsRec) == 44);
    for (uint32_t i = 0; i < count; ++i) {
        Require(!std::memcmp(&g_bannedAddOnsDB.m_records[i], bytes.data() + 20 + i * 44, 44),
            "Original addon policy record changed");
    }
    out << "Original addon policy: " << count << " records, selected installation and " << s_localeCode << " locale passed.\n";
}
