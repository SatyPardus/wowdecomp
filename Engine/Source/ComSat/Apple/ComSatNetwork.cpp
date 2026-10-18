#if defined(__APPLE__)
#include "Engine/Source/ComSat/ComSatNetwork.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr uintptr_t InvalidSocket = uintptr_t(-1);
// Keep ComSat's original 16-byte Windows address representation inside game
// objects. Darwin sockaddr_in begins with separate length/family bytes.
sockaddr_in Native(const ComSat::Network::Address& address) {
    sockaddr_in native{};
    native.sin_len = sizeof(native); native.sin_family = AF_INET;
    native.sin_port = address.m_port; native.sin_addr.s_addr = address.m_ip;
    return native;
}
}

void ComSat::Network::Address::SetPort(int32_t port) { m_port = htons(uint16_t(port)); }
int32_t ComSat::Network::Address::GetPort() const { return ntohs(m_port); }
void ComSat::Network::Address::SetAddress(const char* address) {
    if (!address) { errno = EINVAL; m_ip = INADDR_NONE; return; }
    const char* colon = std::strchr(address, ':');
    if (colon) {
        if (std::strlen(address) < 128) {
            char host[128]; std::strcpy(host, address); host[colon - address] = 0;
            m_ip = inet_addr(host); m_port = htons(uint16_t(std::strtol(colon + 1, nullptr, 10)));
        }
    } else m_ip = inet_addr(address);
}

ComSat::Network::SocketPosix::SocketPosix() : m_socket(InvalidSocket) {
    const int descriptor = socket(AF_INET, SOCK_DGRAM, 0);
    if (descriptor < 0) return;
    const int size = 0x8000, one = 1;
    if (fcntl(descriptor, F_SETFD, FD_CLOEXEC) || fcntl(descriptor, F_SETFL, O_NONBLOCK) ||
        setsockopt(descriptor, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) ||
        setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one))) {
        const int error = errno; close(descriptor); errno = error; return;
    }
    m_socket = uintptr_t(descriptor);
}
ComSat::Network::SocketPosix::~SocketPosix() {
    if (m_socket != InvalidSocket) close(int(m_socket));
    m_socket = InvalidSocket;
}
bool ComSat::Network::SocketPosix::Bind(int32_t port) {
    if (m_socket == InvalidSocket) { errno = EBADF; return false; }
    Address address; address.SetPort(port);
    auto native = Native(address);
    const int reuse = 1;
    if (setsockopt(int(m_socket), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse))) return false;
    return bind(int(m_socket), reinterpret_cast<const sockaddr*>(&native), sizeof(native)) == 0;
}
int32_t ComSat::Network::SocketPosix::SendTo(const Address& address, const uint8_t* data, uint32_t size) {
    if (m_socket == InvalidSocket) { errno = EBADF; return -1; }
    if (address.m_family != 2 || address.m_length != 16 || (!data && size)) { errno = EINVAL; return -1; }
    if (size > 65507) { errno = EMSGSIZE; return -1; }
    const auto native = Native(address);
    ssize_t sent;
    do { sent = sendto(int(m_socket), data, size, 0, reinterpret_cast<const sockaddr*>(&native), sizeof(native)); }
    while (sent < 0 && errno == EINTR);
    return int32_t(sent);
}
bool ComSat::Network::SocketPosix::RecvFrom(Address& address, uint8_t* data, uint32_t& size) {
    if (m_socket == InvalidSocket) { errno = EBADF; return false; }
    if (!data || !size) { errno = EINVAL; return false; }
    std::memset(data, 0, size); // Original failed/short reads clear the spare bytes.
    sockaddr_in native{};
    iovec buffer{data, size};
    msghdr message{}; message.msg_name = &native; message.msg_namelen = sizeof(native);
    message.msg_iov = &buffer; message.msg_iovlen = 1;
    ssize_t received;
    do { received = recvmsg(int(m_socket), &message, 0); } while (received < 0 && errno == EINTR);
    if (received <= 0) return false;
    if (message.msg_flags & MSG_TRUNC) { errno = EMSGSIZE; return false; }
    if (message.msg_namelen < sizeof(native) || native.sin_family != AF_INET) { errno = EAFNOSUPPORT; return false; }
    address.m_length = 16; address.m_family = 2;
    address.m_port = native.sin_port; address.m_ip = native.sin_addr.s_addr;
    std::memset(address.m_zero, 0, sizeof(address.m_zero));
    size = uint32_t(received);
    return true;
}
#endif
