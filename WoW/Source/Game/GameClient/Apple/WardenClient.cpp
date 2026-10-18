#if defined(__APPLE__)
#include "WoW/Source/Game/GameClient/WardenClient.h"
#include "WoW/Source/Apple/ClientIntegrity.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Net/Types.h"

namespace {
int32_t UnsupportedRequest(void*, NETMESSAGE, uint32_t, CDataStore*) {
    RejectNativeIntegrityRequest("Server requested Warden; native module execution is not implemented");
    return 0;
}
}
bool WardenClient_Initialize() {
    WardenClient_Destroy();
    // Register a rejecting handler so a server request cannot be silently
    // consumed. false explicitly reports that no module runtime was created.
    ClientServices::SetMessageHandler(SMSG_WARDEN_DATA, UnsupportedRequest, nullptr);
    return false;
}
void WardenClient_Destroy() { ClientServices::ClearMessageHandler(SMSG_WARDEN_DATA); }
void WardenClient_Process() {} // No runtime or pending work exists on this backend.
#endif
