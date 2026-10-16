#if defined(__APPLE__)

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <vector>
#include "WoW/Source/WowServices/WowConnectionNet.h"

namespace {

struct WakePipe {
    int m_read = -1;
    int m_write = -1;
};

static WakePipe* GetWakePipe(void* event) {
    return static_cast<WakePipe*>(event);
}

static void WakeNetwork(void* event) {
    WakePipe* pipe = GetWakePipe(event);

    if (!pipe || pipe->m_write < 0) {
        return;
    }

    uint8_t byte = 1;
    while (write(pipe->m_write, &byte, sizeof(byte)) < 0 && errno == EINTR) {
    }
}

static void DrainWakePipe(WakePipe* pipe) {
    if (!pipe || pipe->m_read < 0) {
        return;
    }

    uint8_t buffer[64];

    while (read(pipe->m_read, buffer, sizeof(buffer)) > 0) {
    }
}

static void SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

}

void WowConnectionNet::PlatformChangeState(WowConnection* connection, WOW_CONN_STATE state) {
    static_cast<void>(connection);
    static_cast<void>(state);
    WakeNetwork(this->m_event);
}

void WowConnectionNet::PlatformRun() {
    this->m_networkStarted = true;

    if (this->m_stop) {
        return;
    }

    WakePipe* wakePipe = GetWakePipe(this->m_event);
    uint32_t timeout = 500;

    do {
        std::vector<WowConnection*> connections;
        std::vector<pollfd> fds;

        if (wakePipe && wakePipe->m_read >= 0) {
            pollfd wakeFd = {};
            wakeFd.fd = wakePipe->m_read;
            wakeFd.events = POLLIN;
            fds.push_back(wakeFd);
            connections.push_back(nullptr);
        }

        this->m_connectionsLock.Enter();

        for (WowConnection* connection = this->m_connections.Head(); connection; connection = this->m_connections.Next(connection)) {
            if (connection->m_serviceCount || connection->m_sock < 0) {
                continue;
            }

            pollfd fd = {};
            fd.fd = connection->m_sock;

            switch (connection->m_connState) {
                case WOWC_CONNECTING:
                    fd.events = POLLOUT;
                    break;

                case WOWC_LISTENING:
                    fd.events = POLLIN;
                    break;

                case WOWC_CONNECTED:
                    fd.events = POLLIN;

                    if (connection->m_sendList.Head() || connection->m_notifyWriteReady) {
                        fd.events |= POLLOUT;
                    }

                    if (connection->m_lagTestRecvList.Head()) {
                        timeout = 0;
                    }

                    break;

                case WOWC_DISCONNECTING:
                    timeout = 0;
                    fd.events = POLLIN | POLLOUT;
                    break;

                default:
                    continue;
            }

            fd.events |= POLLERR | POLLHUP;
            connection->m_refCount.Increment();
            fds.push_back(fd);
            connections.push_back(connection);
        }

        this->m_connectionsLock.Leave();

        int result;

        do {
            result = poll(fds.data(), fds.size(), timeout);
        } while (result < 0 && errno == EINTR);

        if (result > 0) {
            for (size_t i = 0; i < fds.size(); i++) {
                WowConnection* connection = connections[i];

                if (!connection) {
                    if (fds[i].revents) {
                        DrainWakePipe(wakePipe);
                    }

                    continue;
                }

                uint32_t flags = 0;

                if (fds[i].revents & (POLLIN | POLLHUP | POLLERR)) {
                    flags |= 2;
                }

                if (fds[i].revents & (POLLOUT | POLLHUP | POLLERR)) {
                    flags |= 1;
                }

                if (connection->m_connState == WOWC_DISCONNECTING) {
                    flags |= 8;
                }

                if (WowConnection::s_lagTestDelayMin || WowConnection::s_lagTestDelayMax) {
                    flags |= 3;
                    timeout = 10;
                } else {
                    timeout = 500;
                }

                if (flags) {
                    this->SignalWorker(connection, flags);
                }
            }
        } else if (result == 0) {
            timeout = 500;
        }

        for (WowConnection* connection : connections) {
            if (connection) {
                connection->Release();
            }
        }
    } while (!this->m_stop);
}

void WowConnectionNet::PlatformInit(bool useEngine) {
    static_cast<void>(useEngine);

    WakePipe* wakePipe = new WakePipe();
    int fds[2] = { -1, -1 };

    if (!pipe(fds)) {
        wakePipe->m_read = fds[0];
        wakePipe->m_write = fds[1];
        SetNonBlocking(wakePipe->m_read);
        SetNonBlocking(wakePipe->m_write);
    }

    this->m_event = wakePipe;
}

void WowConnectionNet::PlatformDestruct(WowConnection* connection) {
    connection->m_event = nullptr;
}

void WowConnectionNet::PlatformWorkerReady() {
    WakeNetwork(this->m_event);
}

void WowConnectionNet::PlatformAdd(WowConnection* connection) {
    int noDelay = 1;
    setsockopt(connection->m_sock, IPPROTO_TCP, TCP_NODELAY, &noDelay, sizeof(noDelay));

    int noSigPipe = 1;
    setsockopt(connection->m_sock, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof(noSigPipe));

    WakeNetwork(this->m_event);
}

void WowConnectionNet::PlatformRemove(WowConnection* connection) {
    static_cast<void>(connection);
    WakeNetwork(this->m_event);
}

void WowConnectionNet::PlatformStop() {
    WakeNetwork(this->m_event);
}

void WowConnectionNet::PlatformDestroy() {
    WakePipe* wakePipe = GetWakePipe(this->m_event);

    if (wakePipe) {
        if (wakePipe->m_read >= 0) {
            close(wakePipe->m_read);
        }

        if (wakePipe->m_write >= 0) {
            close(wakePipe->m_write);
        }

        delete wakePipe;
    }

    this->m_event = nullptr;
}

#endif
