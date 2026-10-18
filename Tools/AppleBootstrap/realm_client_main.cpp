#include "WoW/Source/WowServices/GruntLogin.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "Engine/Source/Base/Status.h"
#include "WoW/Source/Ui/AddOns.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Storm/H/storm.h"
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <termios.h>
#include <unistd.h>

namespace {
void ClearSecret(void* data, size_t size) { auto* bytes = static_cast<volatile unsigned char*>(data); while (size--) *bytes++ = 0; }
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
HPROPCONTEXT context;
int32_t Initialize(const void*, void*) {
    context = PropGetSelectedContext();
    EventRegister(EVENT_ID_POLL, [](const void* data, void* param) -> int32_t { RealmConnection::PollNet(data, param); return 1; });
    return 1;
}
struct Runtime {
    Runtime() {
        StormInitialize(); PropInitialize(); EventInitialize(1, 0);
        EventCreateContextEx(0, Initialize, nullptr, 1, 0);
        Require(EventProcessPending(), "Cannot initialize original network event context"); PropSelectContext(context);
        NetClient::SetDisconnectHandler([](NetClient*) -> int32_t { return 1; });
    }
    ~Runtime() {
        NetClient::SetDisconnectHandler(nullptr); PropSelectContext(nullptr);
        EventDestroy(); PropDestroy(); StormDestroy();
    }
};
struct AuthResponse : LoginResponse {
    std::mutex mutex;
    std::condition_variable changed;
    LOGIN_STATE state = LOGIN_STATE_INITIALIZED;
    std::string failure;
    bool realmsReady = false;
    uint32_t realmResult = UINT32_MAX;
    const char* GetLoginServer() override { return "127.0.0.1:3724"; }
    int32_t GetLoginServerType() override { return 0; }
    void JoinRealmResult(uint32_t, int32_t, int32_t, int32_t, int32_t) override {}
    void LoginServerStatus(LOGIN_STATE next, LOGIN_RESULT result, const char*, const char* stateText, const char* resultText, uint16_t) override {
        std::lock_guard<std::mutex> lock(mutex); state = next;
        if (result != LOGIN_OK || next == LOGIN_STATE_FAILED || next == LOGIN_STATE_DISCONNECTED ||
            (next >= LOGIN_STATE_PIN && next <= LOGIN_STATE_TOKEN_WAIT) || next == LOGIN_STATE_DOWNLOADFILE)
            failure = std::string(stateText ? stateText : "Login failed") + ": " + (resultText ? resultText : "unknown result");
        std::fprintf(stderr, "Authentication: %s / %s\n", stateText, resultText);
        changed.notify_all();
    }
    void RealmEnumCallback(uint32_t result) override {
        std::lock_guard<std::mutex> lock(mutex); realmResult = result; realmsReady = true; changed.notify_all();
    }
    void Wait(LOGIN_STATE expected, bool realms = false) {
        std::unique_lock<std::mutex> lock(mutex);
        Require(changed.wait_for(lock, std::chrono::seconds(15), [&] { return !failure.empty() || (realms ? realmsReady : state == expected); }), "Authentication server timed out");
        if (!failure.empty()) throw std::runtime_error(failure);
        if (realms) Require(realmResult == 0, "Realm enumeration failed");
    }
};
struct WorldResponse : RealmResponse {
    bool auth = false, listed = false;
    std::string failure;
    std::vector<CHARACTER_INFO> characters;
    void LoginServerResult(LOGIN_STATE, LOGIN_RESULT, const char*, const char*, const char*) override {}
    void RealmListReceived(TSFixedArray<REALM_INFO>&) override {}
    void HandleAuthResponse(RealmConnection*, uint8_t result) override {
        std::fprintf(stderr, "World authentication result: %u\n", unsigned(result));
        if (result == 12) auth = true;
        else if (result != 27) failure = "World authentication rejected: " + std::to_string(result);
    }
    void AddOnListReceived(RealmConnection*) override {}
    void CharacterListReceived(RealmConnection*, const TSFixedArray<CHARACTER_INFO>& values, int32_t success) override {
        if (!success) { failure = "Invalid character list"; return; }
        if (values.m_count) characters.assign(values.m_data, values.m_data + values.m_count);
        listed = true;
    }
    void GameServerResult(RealmConnection*, const char* a, const char* b, const char* c) override {
        std::fprintf(stderr, "World: %s %s %s\n", a ? a : "", b ? b : "", c ? c : "");
    }
    void MessageReceived(RealmConnection*, uint32_t) override {}
    void OnChatReceived(RealmConnection*, WOWGUID, const char*, uint8_t) override {}
    void OnAccountOnline(RealmConnection*, int32_t, WOWGUID) override {}
    void OnAFKMonitorInfoResponse(RealmConnection*, WOWGUID, int32_t, int32_t, int32_t, int32_t) override {}
    void HandleConnect(RealmConnection*) override {}
    void HandleDisconnect(RealmConnection*) override { failure = "World server disconnected"; }
    void HandleCantConnect(RealmConnection*) override { failure = "Cannot connect to world server"; }
    void Wait(bool& ready) {
        const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(15);
        while (!ready && failure.empty()) {
            Require(std::chrono::steady_clock::now() < limit, "World server timed out");
            Require(EventProcessPending(), "Original network event context stopped"); PropSelectContext(context);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        if (!failure.empty()) throw std::runtime_error(failure);
    }
};
struct WorldConnection : RealmConnection {
    explicit WorldConnection(WorldResponse& response) : RealmConnection(&response) {}
    void HandleCharacterCreate(uint8_t) override {}
    void HandleCharacterDelete(uint8_t) override {}
    void HandleCharacterLoginFailed(uint8_t) override {}
    void HandleLogoutComplete() override {}
    void HandleLogoutResponse(int32_t, int32_t) override {}
    void HandleLogoutAbortAck() override {}
};
struct Secret {
    std::string value;
    ~Secret() { if (!value.empty()) ClearSecret(value.data(), value.size()); }
};
Secret ReadPassword() {
    struct Echo {
        termios saved{}; bool restore = false;
        Echo() {
            if (isatty(STDIN_FILENO)) {
                Require(tcgetattr(STDIN_FILENO, &saved) == 0, "Cannot protect password input");
                auto hidden = saved; hidden.c_lflag &= ~ECHO;
                Require(tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) == 0, "Cannot protect password input"); restore = true;
            }
        }
        ~Echo() { if (restore) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved); std::fputc('\n', stderr); } }
    } echo;
    Secret secret; Require(bool(std::getline(std::cin, secret.value)) && !secret.value.empty() && secret.value.size() <= 128, "Missing or oversized password");
    return secret;
}
}

int main(int argc, char** argv) {
    // Explicit development-server policy; never substitute this for executable
    // verification in the normal client or silently fall back after rejection.
    if (argc != 3 || std::strcmp(argv[1], "--local-unchecked")) {
        std::fprintf(stderr, "Usage: AppleRealmClient --local-unchecked /path/to/Data\n"
            "Connects only to 127.0.0.1:3724; requires the local server's existing StrictVersionCheck=0 policy.\n"
            "Reads account and password from stdin, lists realms/characters, then disconnects.\n"); return 2;
    }
    try {
        std::string account;
        std::fputs("Account: ", stderr); Require(bool(std::getline(std::cin, account)) && !account.empty() && account.size() <= 127, "Missing or oversized account");
        SStrUpper(account.data());
        std::fputs("Password: ", stderr); auto password = ReadPassword();
        Runtime runtime; ClientData data(argv[2], "");
        SFile::Initialize(); InitializeAddOns(account.c_str());
        struct AddOns { ~AddOns() { ShutdownAddOns(); } } addons;
        WorldResponse worldResponse; WorldConnection world(worldResponse);
        AuthResponse response; GruntLogin login; login.Init(&response);
        login.SetLogonCreds(account.c_str(), password.value.c_str()); ClearSecret(password.value.data(), password.value.size()); password.value.clear();
        login.Logon(response.GetLoginServer(), nullptr);
        response.Wait(LOGIN_STATE_CHECKINGVERSIONS);
        const uint8_t uncheckedVersion[20]{};
        std::fputs("Local development policy: executable verification is not performed; SRP proofs and encrypted world authentication remain required.\n", stderr);
        login.ProveVersion(uncheckedVersion);
        response.Wait(LOGIN_STATE_AUTHENTICATED);
        login.GetRealmList(); response.Wait(LOGIN_STATE_AUTHENTICATED, true);
        Require(response.m_realmList.m_count != 0, "No realms are available");
        const REALM_INFO* selected = nullptr;
        for (uint32_t i = 0; i < response.m_realmList.m_count; ++i) {
            const auto& realm = response.m_realmList[i];
            std::printf("Realm: %s; endpoint %s; characters %u; flags %u\n", realm.m_realmName, realm.m_realmAddress, unsigned(realm.m_numCharacters), unsigned(realm.m_flags));
            if (!(realm.m_flags & 3) && !std::strncmp(realm.m_realmAddress, "127.0.0.1:", 10) && !selected) selected = &realm;
        }
        Require(selected, "No online loopback realm is available; no remote connection was attempted");
        LoginData credentials{};
        Require(login.Wait(nullptr, credentials), "Cannot obtain the authenticated session");
        world.SetLoginData(&credentials); ClearSecret(&credentials, sizeof(credentials));
        world.SetSelectedRealm(selected->m_regionId, selected->m_battlegroupId, selected->m_realmId);
        world.Connect(selected->m_realmAddress); worldResponse.Wait(worldResponse.auth);
        world.RequestCharacterEnum(); worldResponse.Wait(worldResponse.listed);
        for (const auto& character : worldResponse.characters)
            std::printf("Character: %s; level %u; race %u; class %u; map %u; position %.3f %.3f %.3f\n", character.m_name,
                unsigned(character.m_experienceLevel), unsigned(character.m_raceID), unsigned(character.m_classID), character.m_mapID,
                character.m_position.x, character.m_position.y, character.m_position.z);
        std::printf("Live realm session completed: %zu character(s); no character creation, deletion or world-entry request sent.\n", worldResponse.characters.size());
        return 0;
    } catch (const std::exception& error) { std::fprintf(stderr, "Live realm session failed: %s\n", error.what()); return 1; }
}
