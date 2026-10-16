#if defined(WOWDECOMP_ARCH_64) && defined(_WIN32)

#include <winsock2.h>
#include <windows.h>
#include <cstring>
#include "Engine/Source/ComSat/ComSatNetwork.h"

typedef int (WSAAPI* WSAIOCTLPROC)(SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);

// OFFSET: 0x00888790
static void DisableConnectionReset(ComSat::Network::SocketPosix* socket) {
    HMODULE library = LoadLibraryA("ws2_32.dll");
    if (library) {
        WSAIOCTLPROC wsaIoctl = reinterpret_cast<WSAIOCTLPROC>(GetProcAddress(library, "WSAIoctl"));
        if (wsaIoctl) {
            BOOL behavior = FALSE;
            DWORD bytesReturned = 0;
            wsaIoctl(socket->m_socket, 0x9800000C, &behavior, sizeof(behavior), nullptr, 0, &bytesReturned, nullptr, nullptr);
        }
        FreeLibrary(library);
    }
}

// OFFSET: 0x008888F0
ComSat::Network::SocketPosix::SocketPosix() {
    this->m_socket = socket(AF_INET, SOCK_DGRAM, 0);
    DisableConnectionReset(this);
    u_long value = 1;
    ioctlsocket(this->m_socket, FIONBIO, &value);
    value = 0x8000;
    setsockopt(this->m_socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&value), sizeof(value));
}

// OFFSET: 0x00888960
ComSat::Network::SocketPosix::~SocketPosix() {
    closesocket(this->m_socket);
}

// OFFSET: 0x00888800
bool ComSat::Network::SocketPosix::Bind(int32_t port) {
    ComSat::Network::Address address;
    address.SetPort(port);
    int32_t reuse = 1;
    setsockopt(this->m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof(reuse));
    return bind(this->m_socket, reinterpret_cast<const sockaddr*>(&address.m_family), address.m_length) != SOCKET_ERROR;
}

// OFFSET: 0x00888860
int32_t ComSat::Network::SocketPosix::SendTo(const ComSat::Network::Address& address, const uint8_t* data, uint32_t size) {
    return sendto(this->m_socket, reinterpret_cast<const char*>(data), size, 0, reinterpret_cast<const sockaddr*>(&address.m_family), address.m_length);
}

// OFFSET: 0x00888890
bool ComSat::Network::SocketPosix::RecvFrom(ComSat::Network::Address& address, uint8_t* data, uint32_t& size) {
    memset(data, 0, size);
    int32_t received = recvfrom(this->m_socket, reinterpret_cast<char*>(data), size, 0, reinterpret_cast<sockaddr*>(&address.m_family), &address.m_length);
    if (received == SOCKET_ERROR) {
        WSAGetLastError();
        return false;
    }
    if (!received) {
        return false;
    }
    size = received;
    return true;
}

#endif
