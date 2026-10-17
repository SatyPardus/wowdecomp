#if !defined(_WIN32)

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include "Engine/Source/ComSat/ComSatNetwork.h"

// Address mirrors the Windows sockaddr_in layout; BSD sockaddr_in starts with sin_len, so convert explicitly.
static sockaddr_in ToSockAddr(const ComSat::Network::Address& address) {
    sockaddr_in result;
    memset(&result, 0, sizeof(result));
#if defined(__APPLE__)
    result.sin_len = sizeof(result);
#endif
    result.sin_family = AF_INET;
    result.sin_port = address.m_port;
    result.sin_addr.s_addr = address.m_ip;
    return result;
}

static int SocketFd(const ComSat::Network::SocketPosix* socket) {
    return static_cast<int>(static_cast<intptr_t>(socket->m_socket));
}

ComSat::Network::SocketPosix::SocketPosix() {
    int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    this->m_socket = static_cast<uintptr_t>(static_cast<intptr_t>(fd));
    if (fd < 0) {
        return;
    }
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
    int value = 0x8000;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &value, sizeof(value));
}

ComSat::Network::SocketPosix::~SocketPosix() {
    int fd = SocketFd(this);
    if (fd >= 0) {
        close(fd);
    }
}

bool ComSat::Network::SocketPosix::Bind(int32_t port) {
    ComSat::Network::Address address;
    address.SetPort(port);
    int reuse = 1;
    setsockopt(SocketFd(this), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in native = ToSockAddr(address);
    return bind(SocketFd(this), reinterpret_cast<const sockaddr*>(&native), sizeof(native)) == 0;
}

int32_t ComSat::Network::SocketPosix::SendTo(const ComSat::Network::Address& address, const uint8_t* data, uint32_t size) {
    sockaddr_in native = ToSockAddr(address);
    return static_cast<int32_t>(sendto(SocketFd(this), data, size, 0, reinterpret_cast<const sockaddr*>(&native), sizeof(native)));
}

bool ComSat::Network::SocketPosix::RecvFrom(ComSat::Network::Address& address, uint8_t* data, uint32_t& size) {
    memset(data, 0, size);
    sockaddr_in native;
    memset(&native, 0, sizeof(native));
    socklen_t length = sizeof(native);
    ssize_t received = recvfrom(SocketFd(this), data, size, 0, reinterpret_cast<sockaddr*>(&native), &length);
    if (received <= 0) {
        return false;
    }
    address.m_length = 16;
    address.m_family = AF_INET;
    address.m_port = native.sin_port;
    address.m_ip = native.sin_addr.s_addr;
    memset(address.m_zero, 0, sizeof(address.m_zero));
    size = static_cast<uint32_t>(received);
    return true;
}

#endif
