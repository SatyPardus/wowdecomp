#if !defined(_WIN32)

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include "Engine/Source/ComSat/ComSatNetwork.h"

void ComSat::Network::Address::SetPort(int32_t port) {
    this->m_port = htons(static_cast<uint16_t>(port));
}

int32_t ComSat::Network::Address::GetPort() const {
    return ntohs(this->m_port);
}

void ComSat::Network::Address::SetAddress(const char* address) {
    const char* colon = strchr(address, ':');
    if (colon) {
        if (strlen(address) < 128) {
            char host[128];
            strcpy(host, address);
            host[colon - address] = '\0';
            this->m_ip = inet_addr(host);
            this->m_port = htons(static_cast<uint16_t>(strtol(colon + 1, nullptr, 10)));
        }
    } else {
        this->m_ip = inet_addr(address);
    }
}

#endif
