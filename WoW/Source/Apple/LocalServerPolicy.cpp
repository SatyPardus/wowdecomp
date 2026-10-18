#if defined(__APPLE__)
#include "LocalServerPolicy.h"
#include <atomic>
#include <cstring>

namespace { std::atomic<bool> localOnly{false}; }
void EnableNativeLocalServerPolicy() { localOnly.store(true); }
bool NativeLocalServerPolicy() { return localOnly.load(); }
bool NativeServerEndpointAllowed(bool authentication, const char* address) {
    if (!NativeLocalServerPolicy()) return true;
    if (!address) return false;
    if (authentication) return !std::strcmp(address, "127.0.0.1:3724");
    // Require a literal loopback address; DNS aliases cannot redirect this mode.
    constexpr char prefix[] = "127.0.0.1:";
    if (std::strncmp(address, prefix, sizeof(prefix) - 1)) return false;
    const char* port = address + sizeof(prefix) - 1;
    unsigned number = 0, digits = 0;
    for (; *port; ++port) {
        if (*port < '0' || *port > '9' || ++digits > 5) return false;
        number = number * 10 + unsigned(*port - '0');
    }
    return digits && number && number <= 65535;
}
#endif
