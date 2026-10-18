#if defined(__APPLE__)
#include "Resolver.h"
#include <arpa/inet.h>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <string>

bool Battlenet::ResolveNativeIPv4(const char* hostname, Client::DNSInfo& info) {
    info.m_addresses.SetCount(0);
    info.m_name = "";
    if (!hostname || !*hostname || strnlen(hostname, 256) == 256) return false;
    // A colon cannot belong to the IPv4 DNS name consumed by this protocol.
    // Handle the original numeric address:port fallback before DNS: some DNS
    // proxies synthesize an address even for such invalid host names.
    if (const char* separator = std::strchr(hostname, ':')) {
        const std::string name(hostname, separator);
        in_addr address{};
        if (name.empty() || !inet_aton(name.c_str(), &address) || address.s_addr == INADDR_NONE) return false;
        info.m_addresses.SetCount(1);
        std::memcpy(info.m_addresses[0].m_value, &address, 4);
        return true;
    }
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    addrinfo* result = nullptr;
    const int status = getaddrinfo(hostname, nullptr, &hints, &result);
    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addresses(result, freeaddrinfo);
    if (!status) {
        for (const auto* item = result; item; item = item->ai_next) {
            if (item->ai_family != AF_INET || !item->ai_addr || item->ai_addrlen < sizeof(sockaddr_in)) continue;
            const auto* address = reinterpret_cast<const sockaddr_in*>(item->ai_addr);
            info.m_addresses.SetCount(1);
            std::memcpy(info.m_addresses[0].m_value, &address->sin_addr, 4);
            info.m_name = result->ai_canonname ? result->ai_canonname : hostname;
            return true;
        }
    }
    return false;
}
#endif
