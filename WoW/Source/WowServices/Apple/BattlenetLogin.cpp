#if defined(__APPLE__)
#include "WoW/Source/WowServices/BattlenetLogin.h"
#include "WoW/Source/Battlenet/Apple/Resolver.h"
#include "WoW/Source/Battlenet/Tumor/TumorManager.h"
#include "Engine/Source/Storm/H/storm.h"

uint32_t BattlenetApplication::Resolve(uint32_t token, const char* hostname) {
    if (!m_login || !g_tumorManager) SErrDisplayAppFatalMessage("", "", 0);
    Battlenet::Client::DNSInfo info;
    if (!Battlenet::ResolveNativeIPv4(hostname, info)) {
        BattlenetLogin::LogConnectionStatus("Failed to resolve %s", hostname ? hostname : "");
    }
    g_tumorManager->m_tumor.Resolved(token, info);
    return 0;
}
#endif
