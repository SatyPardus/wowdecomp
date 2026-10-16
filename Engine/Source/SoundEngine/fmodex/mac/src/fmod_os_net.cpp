#if defined(__APPLE__)

#include "fmod_net.h"
#include "fmod_os_misc.h"
#include "fmod_time.h"
#include "fmod_memory.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static bool FMOD_OS_Net_Initialised = false;
FMOD_OS_CRITICALSECTION *gResolveCrit = 0;

static int FMOD_OS_NetHandleToSocket(const void *handle)
{
    return (int)((intptr_t)handle - 1);
}

static void *FMOD_OS_SocketToNetHandle(int sock)
{
    return (void *)(intptr_t)(sock + 1);
}

static FMOD_RESULT FMOD_OS_Net_SetNonBlocking(int sock, bool nonblocking)
{
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0)
    {
        return FMOD_ERR_NET_SOCKET_ERROR;
    }

    flags = nonblocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(sock, F_SETFL, flags) == 0 ? FMOD_OK : FMOD_ERR_NET_SOCKET_ERROR;
}

FMOD_RESULT FMOD_OS_Net_Init()
{
    if (!FMOD_OS_Net_Initialised)
    {
        FMOD_RESULT result = FMOD_OS_CriticalSection_Create(&gResolveCrit);
        if (result != FMOD_OK)
        {
            return result;
        }
        FMOD_OS_Net_Initialised = true;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Shutdown()
{
    if (FMOD_OS_Net_Initialised)
    {
        FMOD_OS_CriticalSection_Free(gResolveCrit);
        gResolveCrit = 0;
        FMOD_OS_Net_Initialised = false;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Connect(const char *host, const unsigned short port, void **handle)
{
    if (!host || !handle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return FMOD_ERR_MEMORY;
    }

    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));

    sockaddr_in server;
    FMOD_memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, host, &server.sin_addr) != 1)
    {
        FMOD_RESULT result = FMOD_OS_CriticalSection_Enter(gResolveCrit);
        if (result != FMOD_OK)
        {
            close(sock);
            return result;
        }

        addrinfo hints;
        addrinfo *res = 0;
        FMOD_memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        int gai = getaddrinfo(host, NULL, &hints, &res);
        if (gai != 0 || !res)
        {
            FMOD_OS_CriticalSection_Leave(gResolveCrit);
            close(sock);
            return FMOD_ERR_NET_URL;
        }

        server.sin_addr = ((sockaddr_in *)res->ai_addr)->sin_addr;
        freeaddrinfo(res);
        FMOD_OS_CriticalSection_Leave(gResolveCrit);
    }

    FMOD_RESULT result = FMOD_OS_Net_SetNonBlocking(sock, true);
    if (result != FMOD_OK)
    {
        close(sock);
        return result;
    }

    int rc = connect(sock, (sockaddr *)&server, sizeof(server));
    if (rc < 0 && errno != EINPROGRESS && errno != EWOULDBLOCK)
    {
        close(sock);
        return FMOD_ERR_NET_CONNECT;
    }

    fd_set writefd;
    FD_ZERO(&writefd);
    FD_SET(sock, &writefd);

    timeval tv;
    tv.tv_sec = FMOD_Net_Timeout / 1000;
    tv.tv_usec = (FMOD_Net_Timeout % 1000) * 1000;

    rc = select(sock + 1, NULL, &writefd, NULL, &tv);
    if (rc <= 0)
    {
        close(sock);
        return FMOD_ERR_NET_CONNECT;
    }

    int error = 0;
    socklen_t errorlen = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &errorlen) != 0 || error != 0)
    {
        close(sock);
        return FMOD_ERR_NET_CONNECT;
    }

    result = FMOD_OS_Net_SetNonBlocking(sock, false);
    if (result != FMOD_OK)
    {
        close(sock);
        return result;
    }

    *handle = FMOD_OS_SocketToNetHandle(sock);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Listen(const unsigned short port, void **listenhandle)
{
    if (!listenhandle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return FMOD_ERR_MEMORY;
    }

    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));

    FMOD_RESULT result = FMOD_OS_Net_SetNonBlocking(sock, true);
    if (result != FMOD_OK)
    {
        close(sock);
        return result;
    }

    sockaddr_in address;
    FMOD_memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr *)&address, sizeof(address)) != 0 || listen(sock, 3) != 0)
    {
        close(sock);
        return FMOD_ERR_NET_SOCKET_ERROR;
    }

    *listenhandle = FMOD_OS_SocketToNetHandle(sock);
    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Accept(const void *listenhandle, void **clienthandle)
{
    if (!clienthandle)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    int listensocket = FMOD_OS_NetHandleToSocket(listenhandle);
    if (listensocket < 0)
    {
        return FMOD_ERR_NET_SOCKET_ERROR;
    }

    int clientsocket = accept(listensocket, NULL, NULL);
    if (clientsocket < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
        {
            return FMOD_ERR_NET_WOULD_BLOCK;
        }
        return FMOD_ERR_NET_SOCKET_ERROR;
    }

    int on = 1;
    setsockopt(clientsocket, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    *clienthandle = FMOD_OS_SocketToNetHandle(clientsocket);

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Close(const void *handle)
{
    int sock = FMOD_OS_NetHandleToSocket(handle);
    if (sock >= 0)
    {
        close(sock);
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Write(const void *handle, const char *buf, const unsigned int len, unsigned int *byteswritten)
{
    int sock = FMOD_OS_NetHandleToSocket(handle);
    if (sock < 0)
    {
        return FMOD_ERR_NET_SOCKET_ERROR;
    }
    if (!buf || !byteswritten)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *byteswritten = 0;
    unsigned int bytestowrite = len;
    while (bytestowrite)
    {
        ssize_t written = send(sock, buf, bytestowrite, 0);
        if (written < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return FMOD_ERR_NET_WOULD_BLOCK;
            }
            return FMOD_ERR_NET_SOCKET_ERROR;
        }

        *byteswritten += (unsigned int)written;
        bytestowrite -= (unsigned int)written;
        buf += written;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_Read(const void *handle, char *buf, const unsigned int len, unsigned int *bytesread)
{
    int sock = FMOD_OS_NetHandleToSocket(handle);
    if (sock < 0)
    {
        return FMOD_ERR_NET_SOCKET_ERROR;
    }
    if (!buf || len <= 0 || !bytesread)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    *bytesread = 0;
    unsigned int bytestoread = len;
    while (bytestoread)
    {
        ssize_t rd = recv(sock, buf, bytestoread, 0);
        if (rd < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                return FMOD_ERR_NET_WOULD_BLOCK;
            }
            return FMOD_ERR_NET_SOCKET_ERROR;
        }
        else if (rd == 0)
        {
            return FMOD_ERR_FILE_EOF;
        }

        *bytesread += (unsigned int)rd;
        bytestoread -= (unsigned int)rd;
        buf += rd;
    }

    return FMOD_OK;
}

FMOD_RESULT FMOD_OS_Net_ReadLine(const void *handle, char *buf, const unsigned int len)
{
    unsigned int pos, read;
    char c = 0;

    if (!buf || len <= 0)
    {
        return FMOD_ERR_INVALID_PARAM;
    }

    buf[0] = 0;
    if (FMOD_OS_NetHandleToSocket(handle) < 0)
    {
        return FMOD_ERR_NET_SOCKET_ERROR;
    }

    pos = 0;
    while (pos < len)
    {
        read = 0;
        FMOD_RESULT result = FMOD_OS_Net_Read(handle, &c, 1, &read);
        if (result == FMOD_ERR_NET_WOULD_BLOCK)
        {
            return FMOD_ERR_NET_WOULD_BLOCK;
        }

        if (read == 1)
        {
            if (c == '\n')
            {
                break;
            }
            else if (c != '\r')
            {
                buf[pos++] = c;
            }
        }
        else
        {
            break;
        }
    }

    if (pos >= len)
    {
        pos = len - 1;
    }

    buf[pos] = 0;
    return FMOD_OK;
}

#endif
