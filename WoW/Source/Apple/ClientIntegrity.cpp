#if defined(__APPLE__)
#include "WoW/Source/Client.h"
#include "WoW/Source/Apple/ClientIntegrity.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include "WoW/Source/Net/Types.h"
#include "Engine/Source/Os/OsVersionHash.h"
#include <stdexcept>
#include <cstdio>
#include <cerrno>

void RejectNativeIntegrityRequest(const char* reason) {
    SErrSetLastError(ENOTSUP);
    std::fprintf(stderr, "%s; disconnecting native client\n", reason);
    if (ClientServices::s_currentConnection) ClientServices::s_currentConnection->Disconnect();
}

// Explicit unavailable boundaries for platform-dependent integrity services.
// Never use the reconstructed Win64 no-result placeholders as successful
// native checks, and never synthesize a module response or executable digest.
int32_t IOsVersionChecksum(IOsVersionHashCallback&, const char*, int32_t) {
    throw std::runtime_error("Native executable challenge verification is not implemented");
}
void InitTokenSeed(const uint8_t*) {
    RejectNativeIntegrityRequest("Server requested an unsupported native integrity token operation");
}
void LoadEncryptedModule(const uint8_t*, uint32_t, void (*)()) {
    RejectNativeIntegrityRequest("Server requested an unsupported encrypted integrity module");
}
int32_t Packet_SMSG_CHECK_FOR_BOTS(void*, NETMESSAGE, uint32_t, CDataStore*) {
    RejectNativeIntegrityRequest("Server requested an unsupported native integrity scan");
    return 0;
}
int32_t GatherAdditionalChecks(uint8_t*, uint8_t*, uint8_t*) {
    RejectNativeIntegrityRequest("Native integrity result collection is not implemented");
    return 0; // No result was produced; the caller must not read/send the outputs.
}
void AdditionalCheck1() {
    // This hook is called from normal minimap updates even when no scan is
    // enabled or armed. Only an actual request reaches the unavailable path.
    if (!s_botCheckEnabled || s_botCheckDelay <= 0 || s_botCheckDelay > 420 ||
        __atomic_load_n(&s_additionalCheckArmed, __ATOMIC_RELAXED) != 1) return;
    RejectNativeIntegrityRequest("Native armed integrity scan is not implemented");
}
#endif
