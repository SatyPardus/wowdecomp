#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"
#include "WoW/Source/WowServices/Apple/ClientSettings.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/ConsoleCommand.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Base/Prop.h"
#include "ThirdParty/zlib/zlib.h"
#include <CommonCrypto/CommonDigest.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Host final : AccountDataHost {
    NetClient& connection;
    std::filesystem::path root;
    std::string previous;
    bool hadOverride = false;
    bool initialized = false;
    explicit Host(NetClient& value) : connection(value) {
        if (const char* setting = std::getenv("WOW_SETTINGS_ROOT")) { hadOverride = true; previous = setting; }
        std::string pattern = (std::filesystem::temp_directory_path() / "wow-settings-XXXXXX").string();
        Require(mkdtemp(pattern.data()), "Cannot create account-settings check root"); root = pattern;
        setenv("WOW_SETTINGS_ROOT", pattern.c_str(), 1);
        Require(!CVar::m_initialized, "Account settings check requires its own CVar lifecycle");
        CVar::Initialize((root / "Config.wtf").c_str());
        AccountDataInitializeBasicSystem(*this); initialized = true;
        AccountDataInitialize(true); AccountDataInitialize(false);
    }
    ~Host() {
        if (initialized) { AccountDataDestroy(false); AccountDataDestroy(true); AccountDataShutdownBasicSystem(); }
        CVar::m_needsSave = false; CVar::Destroy(); ConsoleCommandDestroy(); CStringManager::DestroyManager(); CVar::s_configFile = nullptr;
        if (hadOverride) setenv("WOW_SETTINGS_ROOT", previous.c_str(), 1); else unsetenv("WOW_SETTINGS_ROOT");
        std::error_code error; std::filesystem::remove_all(root, error);
    }
    NetClient& Connection() const override { return connection; }
    const char* AccountName() const override { return "APPLECHECK"; }
    const char* RealmName() const override { return "Realm"; }
    const char* CharacterName() const override { return "Hero"; }
    WOWGUID ActiveCharacter() const override { WOWGUID guid; guid.m_guid = 0x0102030405060708ull; return guid; }
};
void Pump(const std::function<bool()>& ready) {
    HPROPCONTEXT context = PropGetSelectedContext();
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!ready()) {
        Require(std::chrono::steady_clock::now() < limit, "Account data dispatch timed out");
        Require(EventProcessPending(), "Event stopped during account data synchronization");
        PropSelectContext(context);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
struct Loaded {
    unsigned calls = 0;
    std::string text;
    static void Callback(HOSFILE local, HOSFILE cache, void* param) {
        auto& self = *static_cast<Loaded*>(param); ++self.calls; self.text.clear();
        HOSFILE file = cache == OS_INVALID_FILE ? local : cache;
        if (file == OS_INVALID_FILE) return;
        auto size = OsGetFileSize(file); Require(size < 1024 * 1024, "Unexpected settings file size");
        self.text.resize(size); uint32_t read = 0;
        Require(OsReadFile(file, self.text.data(), uint32_t(size), &read) && read == size, "Account callback could not read original file");
    }
};
void Save(HOSFILE local, HOSFILE cache, void* param) {
    const auto& text = *static_cast<const std::string*>(param); uint32_t written = 0;
    HOSFILE file = cache == OS_INVALID_FILE ? local : cache;
    Require(file != OS_INVALID_FILE && OsWriteFile(file, text.data(), text.size(), &written) && written == text.size(), "Account save callback could not write original file");
}
std::string Read(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary); Require(bool(file), "Missing account settings file");
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
void Times(uint32_t type, uint32_t time, const std::function<void(CDataStore&)>& send) {
    CDataStore packet; packet.Put(uint16_t(SMSG_ACCOUNT_DATA_TIMES)); packet.Put(uint32_t(200)); packet.Put(uint8_t(1));
    packet.Put(uint32_t(1u << type)); packet.Put(time); send(packet);
}
void Update(uint32_t type, uint64_t guid, uint32_t time, const std::string& text, const std::function<void(CDataStore&)>& send, bool corrupt = false, uint32_t declaredExtra = 0) {
    uint32_t count = compressBound(text.size()); std::vector<uint8_t> compressed(count);
    Require(!zlib_compress(compressed.data(), &count, reinterpret_cast<const uint8_t*>(text.data()), text.size(), 1), "Cannot compress settings fixture");
    if (corrupt) compressed[0] ^= 0xff;
    CDataStore packet; packet.Put(uint16_t(SMSG_UPDATE_ACCOUNT_DATA)); packet.Put(guid); packet.Put(type); packet.Put(time);
    packet.Put(uint32_t(text.size()) + declaredExtra); packet.PutData(compressed.data(), count); send(packet);
}
}

void CheckAccountData(NetClient& connection, const std::function<void(CDataStore&)>& send,
                      const std::function<std::vector<uint8_t>()>& receive, int peer) {
    Host host(connection);
    Loaded global, character;
    LoadAccountData(0, Loaded::Callback, &global); LoadAccountData(1, Loaded::Callback, &character);
    Require(!global.calls && !character.calls, "Account loads did not wait for original server times");
    auto request = [&](uint32_t type) {
        Pump([&] { pollfd poller{peer, POLLIN, 0}; return poll(&poller, 1, 0) > 0; });
        auto bytes = receive(); CDataStore packet(bytes.data(), bytes.size()); uint32_t opcode, index; packet.Get(opcode); packet.Get(index);
        Require(opcode == CMSG_REQUEST_ACCOUNT_DATA && index == type && packet.IsRead(), "Original account-data download request changed");
    };
    std::string globalText = "SET cameraDistanceMaxFactor \"1.5\"\n";
    const std::string characterText = "SET chatStyle \"classic\"\n";
    Times(0, 10, send); request(0); Update(0, 0, 10, globalText, send); Pump([&] { return global.calls == 1; });
    Times(1, 11, send); request(1); Update(1, host.ActiveCharacter().m_guid, 11, characterText, send); Pump([&] { return character.calls == 1; });
    const auto globalPath = host.root / "Account/APPLECHECK/config-cache.wtf";
    const auto characterPath = host.root / "Account/APPLECHECK/Realm/Hero/config-cache.wtf";
    Require(global.text == globalText && character.text == characterText && Read(globalPath) == globalText && Read(characterPath) == characterText,
        "Original account/character settings download or partition changed");
    std::string changed;
    for (unsigned i = 0; i < 900; ++i) changed += "SET testValue \"" + std::to_string(i) + "\"\n";
    SaveAccountData(0, Save, &changed); AccountDataFlush();
    auto bytes = receive(); CDataStore uploaded(bytes.data(), bytes.size()); uint32_t opcode, type, time, size;
    uploaded.Get(opcode); uploaded.Get(type); uploaded.Get(time); uploaded.Get(size);
    Require(opcode == CMSG_UPDATE_ACCOUNT_DATA && type == 0 && time == 200 && size == changed.size(), "Original account settings upload header changed");
    std::string decoded(size, '\0'); uint32_t count = size;
    Require(!zlib_uncompress(reinterpret_cast<uint8_t*>(decoded.data()), &count, uploaded.m_data + uploaded.m_read, uploaded.m_size - uploaded.m_read) && count == size && decoded == changed,
        "Original account settings upload did not preserve file contents");
    const auto checksums = Read(host.root / "Account/APPLECHECK/cache.md5"); uint8_t digest[16]; CC_MD5(changed.data(), changed.size(), digest);
    Require(checksums.size() == 60 && !std::memcmp(checksums.data(), digest, 16), "Original account checksum file changed");
    SaveAccountData(0, Save, &changed); AccountDataFlush(); pollfd quiet{peer, POLLIN, 0};
    Require(poll(&quiet, 1, 20) == 0, "Unchanged settings uploaded unnecessarily");
    s_cvSynchronizeSettings->Set("0", true, false, false, true);
    std::string local = "Local only\n"; SaveAccountData(1, Save, &local); AccountDataFlush();
    Require(Read(characterPath) == local && poll(&quiet, 1, 20) == 0, "Disabled synchronization did not preserve local-only saves");
    s_cvSynchronizeSettings->Set("1", true, false, false, true);
    std::string layout = "Frame layout local state\n"; SaveAccountData(6, Save, &layout); AccountDataFlush();
    Require(Read(host.root / "Account/APPLECHECK/Realm/Hero/layout-local.txt") == layout && poll(&quiet, 1, 20) == 0, "Local layout was uploaded or lost");

    DownloadAccountData(0); request(0);
    CDataStore invalidType; invalidType.Put(uint16_t(SMSG_UPDATE_ACCOUNT_DATA)); invalidType.Put(uint64_t(0));
    invalidType.Put(uint32_t(0xffffffff)); invalidType.Put(uint32_t(300)); invalidType.Put(uint32_t(0)); send(invalidType);
    CDataStore truncated; truncated.Put(uint16_t(SMSG_UPDATE_ACCOUNT_DATA)); truncated.Put(uint64_t(0)); truncated.Put(uint32_t(0)); send(truncated);
    Update(0, 0, 300, "Invalid compression must preserve the last valid file", send, true);
    Pump([&] { return global.calls == 2; });
    Require(Read(globalPath) == changed && global.text == changed, "Malformed compressed settings destroyed the existing configuration");
    Require(Read(host.root / "Account/APPLECHECK/cache.md5") == checksums, "Rejected settings changed the persisted checksum or time");
    DownloadAccountData(0); request(0); Update(0, 0, 300, "Wrong declared size", send, false, 1);
    Pump([&] { return global.calls == 3; }); Require(global.text == changed, "Wrong decoded settings size was accepted");
    DownloadAccountData(0); request(0);
    CDataStore oversized; oversized.Put(uint16_t(SMSG_UPDATE_ACCOUNT_DATA)); oversized.Put(uint64_t(0)); oversized.Put(uint32_t(0));
    oversized.Put(uint32_t(300)); oversized.Put(uint32_t(0xffffffff)); send(oversized);
    Pump([&] { return global.calls == 4; }); Require(global.text == changed, "Over-budget settings update was accepted");
    DownloadAccountData(0); request(0); Update(0, 0, 300, globalText, send);
    Pump([&] { return global.calls == 5; }); Require(global.text == globalText, "Valid retry did not recover account settings download");
    // Both scopes reload their existing files/checksums; matching server times
    // deliver cached settings without downloading again.
    AccountDataDestroy(true); AccountDataDestroy(false); AccountDataInitialize(true); AccountDataInitialize(false);
    Require(Read(host.root / "Account/APPLECHECK/config-cache.old") == globalText, "Original settings backup was not created on reload");
    Loaded reloaded; LoadAccountData(0, Loaded::Callback, &reloaded);
    Times(0, 300, send); Pump([&] { return reloaded.calls == 1; });
    Require(reloaded.text == globalText && poll(&quiet, 1, 20) == 0, "Matching server time did not reuse persisted settings");
    std::string path; Require(!NativeSettingsFile(path, "config-cache.wtf", true, "../escape"), "Account path escaped application settings");
    Require(!NativeSettingsFile(path, "../config-cache.wtf", true, "APPLECHECK"), "Settings filename escaped application settings");
    // Shutdown must cancel a pending save before the host/session is destroyed.
    SaveAccountData(0, Save, &changed);
    AccountDataDestroy(true); AccountDataFlush();
    Require(poll(&quiet, 1, 20) == 0, "Destroyed account still uploaded a pending save");
    std::fprintf(stderr, "Original account settings: encrypted download/upload, account/character files, compression, MD5, unchanged saves, synchronization switch and malformed-data preservation passed\n");
}
