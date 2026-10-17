#include "fmod_settings.h"
#include "fmod_os_net.h"
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
int Socket(const void* handle) { return int(reinterpret_cast<intptr_t>(handle)) - 1; }
void* Handle(int socket) { return reinterpret_cast<void*>(intptr_t(socket) + 1); }
FMOD_RESULT SocketError() {
    return errno == EAGAIN || errno == EWOULDBLOCK ? FMOD_ERR_NET_WOULD_BLOCK : FMOD_ERR_NET_SOCKET_ERROR;
}
bool Configure(int socket, bool nonblocking) {
    int enabled = 1;
    return !setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) &&
           !fcntl(socket, F_SETFD, FD_CLOEXEC) && !fcntl(socket, F_SETFL, nonblocking ? O_NONBLOCK : 0);
}
}
FMOD_RESULT FMOD_OS_Net_Init() { return FMOD_OK; }
FMOD_RESULT FMOD_OS_Net_Shutdown() { return FMOD_OK; }
FMOD_RESULT FMOD_OS_Net_Connect(const char* host, unsigned short port, void** out) {
    if (!host || !out) return FMOD_ERR_INVALID_PARAM;
    *out = nullptr;
    char service[6];
    snprintf(service, sizeof(service), "%u", unsigned(port));
    addrinfo hints{}, *addresses = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, service, &hints, &addresses)) return FMOD_ERR_NET_URL;
    for (auto* address = addresses; address; address = address->ai_next) {
        int fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
        if (fd < 0) continue;
        bool success = Configure(fd, true);
        if (success && connect(fd, address->ai_addr, address->ai_addrlen)) {
            success = errno == EINPROGRESS;
            if (success) {
                pollfd pending{fd, POLLOUT, 0};
                int ready;
                do { ready = poll(&pending, 1, 10000); } while (ready < 0 && errno == EINTR);
                int error = 0;
                socklen_t length = sizeof(error);
                success = ready > 0 && !getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &length) && !error;
            }
        }
        if (success && Configure(fd, false)) { *out = Handle(fd); break; }
        close(fd);
    }
    freeaddrinfo(addresses);
    return *out ? FMOD_OK : FMOD_ERR_NET_CONNECT;
}
FMOD_RESULT FMOD_OS_Net_Listen(unsigned short port, void** out) {
    if (!out) return FMOD_ERR_INVALID_PARAM;
    *out = nullptr;
    int fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd < 0) return FMOD_ERR_NET_SOCKET_ERROR;
    int dualStack = 0;
    sockaddr_in6 address{};
    address.sin6_len = sizeof(address);
    address.sin6_family = AF_INET6;
    address.sin6_port = htons(port);
    if (!Configure(fd, true) || setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &dualStack, sizeof(dualStack)) ||
        bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) || listen(fd, 3)) {
        close(fd);
        return FMOD_ERR_NET_SOCKET_ERROR;
    }
    *out = Handle(fd);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Net_Accept(const void* handle, void** out) {
    if (!handle || !out) return FMOD_ERR_INVALID_PARAM;
    *out = nullptr;
    int fd;
    do { fd = accept(Socket(handle), nullptr, nullptr); } while (fd < 0 && errno == EINTR);
    if (fd < 0) return SocketError();
    if (!Configure(fd, true)) { close(fd); return FMOD_ERR_NET_SOCKET_ERROR; }
    *out = Handle(fd);
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Net_Close(const void* handle) {
    return handle && !close(Socket(handle)) ? FMOD_OK : FMOD_ERR_NET_SOCKET_ERROR;
}
FMOD_RESULT FMOD_OS_Net_Write(const void* handle, const char* buffer, unsigned length, unsigned* written) {
    if (!handle || !written || (!buffer && length)) return FMOD_ERR_INVALID_PARAM;
    *written = 0;
    while (*written < length) {
        ssize_t count = send(Socket(handle), buffer + *written, length - *written, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) return SocketError();
        if (!count) return FMOD_ERR_NET_SOCKET_ERROR;
        *written += unsigned(count);
    }
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Net_Read(const void* handle, char* buffer, unsigned length, unsigned* read) {
    if (!handle || !read || (!buffer && length)) return FMOD_ERR_INVALID_PARAM;
    *read = 0;
    while (*read < length) {
        ssize_t count = recv(Socket(handle), buffer + *read, length - *read, 0);
        if (count < 0 && errno == EINTR) continue;
        if (count < 0) return SocketError();
        if (!count) return FMOD_ERR_FILE_EOF;
        *read += unsigned(count);
    }
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_Net_ReadLine(const void* handle, char* buffer, unsigned length) {
    if (!buffer || !length) return FMOD_ERR_INVALID_PARAM;
    unsigned position = 0;
    buffer[0] = 0;
    while (position + 1 < length) {
        char character;
        unsigned count;
        FMOD_RESULT result = FMOD_OS_Net_Read(handle, &character, 1, &count);
        if (result != FMOD_OK) return result;
        if (character == '\n') return FMOD_OK;
        if (character != '\r') { buffer[position++] = character; buffer[position] = 0; }
    }
    return FMOD_ERR_NET_SOCKET_ERROR; // Reject a truncated protocol header.
}
