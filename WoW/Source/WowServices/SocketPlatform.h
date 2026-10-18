#pragma once
#if defined(__APPLE__)
#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
using WowSocketLength = socklen_t;
constexpr int WowSocketInterrupted = EINTR, WowSocketWouldBlock = EWOULDBLOCK;
inline int WowSocketError() { return errno; }
inline int WowSocketClose(int socket) { return close(socket); }
inline bool WowSocketConnectPending() { return errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EINTR; }
inline bool WowSocketNonBlocking(int socket) {
    const int flags = fcntl(socket, F_GETFL);
    if (flags < 0 || fcntl(socket, F_SETFL, flags | O_NONBLOCK) || fcntl(socket, F_SETFD, FD_CLOEXEC)) return false;
    const int enabled = 1;
    return setsockopt(socket, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) == 0;
}
inline int WowSocketSend(int socket, const char* data, int size) {
    int result;
    do { result = send(socket, data, size, 0); } while (result < 0 && errno == EINTR);
    return result;
}
#else
#include <winsock2.h>
#include <windows.h>
using WowSocketLength = int;
constexpr int WowSocketInterrupted = WSAEINTR, WowSocketWouldBlock = WSAEWOULDBLOCK;
inline int WowSocketError() { return WSAGetLastError(); }
inline int WowSocketClose(int socket) { return closesocket(socket); }
inline bool WowSocketConnectPending() { return WSAGetLastError() == WSAEWOULDBLOCK; }
inline bool WowSocketNonBlocking(int socket) { u_long value = 1; ioctlsocket(socket, FIONBIO, &value); return true; }
inline int WowSocketSend(int socket, const char* data, int size) { return send(socket, data, size, 0); }
#endif
