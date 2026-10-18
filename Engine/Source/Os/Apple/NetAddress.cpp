#if defined(__APPLE__)
#include "Engine/Source/Os/OsTcp.h"
#include "Engine/Source/Storm/H/storm.h"
#include <arpa/inet.h>
#include <cstring>

// NETADDR is an opaque 16-byte address, not a serialized Windows sockaddr.
// Copy through aligned native storage because NETADDR itself has alignment 1.
static_assert(sizeof(sockaddr_in) == sizeof(NETADDR));
void OsNetAddrMake(uint32_t address, uint16_t port, NETADDR* output) {
    sockaddr_in native{};
    native.sin_len = sizeof(native); native.sin_family = AF_INET;
    native.sin_addr.s_addr = address; native.sin_port = htons(port);
    std::memcpy(output, &native, sizeof(native));
}
uint32_t OsNetAddrGetAddress(const NETADDR* input, uint16_t* port) {
    sockaddr_in native; std::memcpy(&native, input, sizeof(native));
    if (port) *port = ntohs(native.sin_port);
    return native.sin_addr.s_addr;
}
const char* OsNetAddrToStr(const NETADDR* input, char* buffer, uint32_t size) {
    sockaddr_in native; std::memcpy(&native, input, sizeof(native));
    char address[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &native.sin_addr, address, sizeof(address));
    SStrPrintf(buffer, size, "%s:%u", address, ntohs(native.sin_port));
    return buffer;
}
#endif
