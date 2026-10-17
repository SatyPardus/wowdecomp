#if defined(__APPLE__)

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include "Engine/Source/Os/OsTcp.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

void OsNetAddrMake(uint32_t address, uint16_t port, NETADDR* addr) {
    if (!addr) {
        return;
    }
    sockaddr_in* in = reinterpret_cast<sockaddr_in*>(addr);
    memset(in, 0, sizeof(*in));
    in->sin_family = AF_INET;
    in->sin_port = htons(port);
    in->sin_addr.s_addr = address;
}

uint32_t OsNetAddrGetAddress(const NETADDR* addr, uint16_t* port) {
    if (!addr) {
        if (port) {
            *port = 0;
        }
        return 0;
    }
    const sockaddr_in* in = reinterpret_cast<const sockaddr_in*>(addr);
    if (port) {
        *port = ntohs(in->sin_port);
    }
    return in->sin_addr.s_addr;
}

const char* OsNetAddrToStr(const NETADDR* addr, char* buffer, uint32_t size) {
    if (!buffer || !size) {
        return buffer;
    }
    if (!addr) {
        SStrCopy(buffer, "0.0.0.0:0", size);
        return buffer;
    }
    const sockaddr_in* in = reinterpret_cast<const sockaddr_in*>(addr);
    char text[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &in->sin_addr, text, sizeof(text));
    SStrPrintf(buffer, size, "%s:%u", text[0] ? text : "0.0.0.0", ntohs(in->sin_port));
    return buffer;
}

void OsNetPump(uint32_t timeout) {
    if (timeout) {
        OsSleep(timeout);
    }
}

#endif
