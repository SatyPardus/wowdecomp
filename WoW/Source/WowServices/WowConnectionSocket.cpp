#if defined(WOWDECOMP_ARCH_64) || defined(__APPLE__)

#include "WoW/Source/WowServices/SocketPlatform.h"
#include <cerrno>
#if defined(__APPLE__)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include <new>
#include "WoW/Source/WowServices/WowConnection.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowServices/WowConnectionNet.h"

// OFFSET: 0x00466670
char* WowConnection::PeerAddrToStr(char* buffer, uint32_t size) {
    const sockaddr_in* peer = reinterpret_cast<const sockaddr_in*>(&this->m_peer.peerAddr);
    uint32_t address = peer->sin_addr.s_addr;
    SStrPrintf(buffer, size, "%d.%d.%d.%d:%u", address & 0xFF, (address >> 8) & 0xFF, (address >> 16) & 0xFF, address >> 24, ntohs(peer->sin_port));

    return buffer;
}

// OFFSET: 0x00466DE0
void WowConnection::CloseSocket(int32_t sock) {
    WowSocketClose(sock);

    if (sock >= 0) {
        WowConnection::s_idLock.Enter();
        WowConnection::s_idBits.m_bits.m_data[sock >> 5] &= ~(1u << (sock & 0x1F));
        WowConnection::s_idLock.Leave();
    }
}

// OFFSET: 0x00466F00
void WowConnection::DoStreamReads() {
    uint8_t buffer[4096];
    uint32_t startTime = OsGetAsyncTimeMs();
    int32_t bytes;

    while (true) {
        do {
            bytes = recv(this->m_sock, reinterpret_cast<char*>(buffer), sizeof(buffer), 0);
        } while (bytes < 0 && WowSocketError() == WowSocketInterrupted);

        if (bytes <= 0) {
            break;
        }

        this->AcquireResponseRef();
        WowConnectionResponse* response = this->m_response;
        this->m_lock.Leave();

        if (response) {
            response->WCDataReady(this, OsGetAsyncTimeMs(), buffer, bytes);
        }

        this->m_lock.Enter();
        this->ReleaseResponseRef();

        if (this->m_connState == WOWC_DISCONNECTING || static_cast<int32_t>(OsGetAsyncTimeMs() - startTime) >= 5) {
            return;
        }
    }

    if (bytes < 0 && WowSocketError() == WowSocketWouldBlock) {
        return;
    }

    this->AcquireResponseRef();
    WowConnectionResponse* response = this->m_response;

    WowConnection::s_network->Remove(this);
    this->CloseSocket(this->m_sock);
    this->SetState(WOWC_DISCONNECTED);

    int32_t sock = this->m_sock;
#if defined(__APPLE__)
    this->m_sock = -1;
#endif
    this->m_lock.Leave();

    if (response && sock >= 0) {
        response->WCDisconnected(this, OsGetAsyncTimeMs(), &this->m_peer);
    }

    this->m_lock.Enter();
#if !defined(__APPLE__)
    this->m_sock = -1;
#endif
    this->ReleaseResponseRef();
}

// OFFSET: 0x00467270
WowConnection::WowConnection(int32_t sock, const NETADDR* addr, WowConnectionResponse* response) {
    this->Init(response, nullptr);
    WowSocketLength nameLength = sizeof(this->m_peer.peerAddr);
    getpeername(sock, reinterpret_cast<sockaddr*>(&this->m_peer.peerAddr), &nameLength);
    nameLength = sizeof(this->m_peer.selfAddr);
    getsockname(sock, reinterpret_cast<sockaddr*>(&this->m_peer.selfAddr), &nameLength);
    this->m_sock = sock;
    this->m_connState = WOWC_CONNECTED;
}

// OFFSET: 0x004675F0
int32_t WowConnection::SendPacket(CDataStore* msg, int32_t headroom) {
    int32_t size = msg->m_size;
    void* data;
    msg->GetDataInSitu(data, size);
    WowConnection::s_sendBytes += size;

    int32_t result = 2;

    this->m_lock.Enter();

    if (size <= 0) {
        result = 2;
    } else if (this->m_connState == WOWC_CONNECTED) {
        if (WowConnection::s_lagTestDelayMin || this->m_sendList.Head()) {
            WowConnection::SENDNODE* node = this->NewSendNodePacket(data, size, false);

            if (this->m_encrypt) {
                uint32_t length = this->m_uint375 - node->m_datasize + node->m_size;

                if (length > node->m_size) {
                    length = node->m_size;
                }

                SARC4ProcessBuffer(node->m_data, length, &this->m_sendKey, &this->m_sendKey);
            }

            this->m_sendList.LinkToTail(node);
            this->m_sendDepth++;
            this->m_sendBytes += node->m_size;
            WowConnection::s_network->PlatformChangeState(this, this->m_connState);

            if (this->m_sendDepth >= this->m_maxSendDepth) {
                this->SetState(WOWC_DISCONNECTING);
                result = 2;

                if (this->m_response) {
                    this->m_response->NotifySendDepthExceeded(this);
                }

                char peer[80];
                this->PeerAddrToStr(peer, sizeof(peer));
                DebugLog("WowConnection: MaxSendDepth Reached! m_sendDepth=%d, m_maxSendDepth=%d, Dropping connection to %s", this->m_sendDepth, this->m_maxSendDepth, peer);
            } else {
                result = 1;
            }
        } else {
            WowConnection::SENDNODE* node;
            bool stackNode;

            if (msg->m_size >= 100000) {
                stackNode = false;
                node = this->NewSendNodePacket(data, size, false);
            } else {
                stackNode = true;

                if (headroom < 3) {
                    void* buffer = alloca(size + sizeof(WowConnection::SENDNODE) + 3);
                    node = new (buffer) WowConnection::SENDNODE(data, size, static_cast<uint8_t*>(buffer) + sizeof(WowConnection::SENDNODE), false);
                } else {
                    void* buffer = alloca(sizeof(WowConnection::SENDNODE));
                    node = new (buffer) WowConnection::SENDNODE(nullptr, size, static_cast<uint8_t*>(data), false);
                }
            }

            if (this->m_encrypt) {
                uint32_t length = this->m_uint375 - node->m_datasize + node->m_size;

                if (length > node->m_size) {
                    length = node->m_size;
                }

                this->Encrypt(node->m_data, length);
            }

            int32_t sent = WowSocketSend(this->m_sock, reinterpret_cast<char*>(node->m_data), node->m_size);

            if (sent == static_cast<int32_t>(node->m_size)) {
                if (!stackNode) {
                    this->FreeSendNode(node);
                }

                result = 0;
            } else {
                bool wouldBlock = WowSocketError() == WowSocketWouldBlock;

                if (stackNode) {
                    node = this->NewSendNodeCopy(node, size);
                }

                if (sent > 0) {
                    node->m_offset += sent;
                    result = 1;
                } else if (!wouldBlock) {
                    this->SetState(WOWC_DISCONNECTING);
                    result = 2;
                }

                this->m_sendList.LinkToTail(node);
                this->m_sendDepth++;
                this->m_sendBytes += node->m_size;

                if (this->m_sendDepth >= this->m_maxSendDepth) {
                    this->SetState(WOWC_DISCONNECTING);
                    result = 2;

                    if (this->m_response) {
                        this->m_response->NotifySendDepthExceeded(this);
                    }

                    char peer[80];
                    this->PeerAddrToStr(peer, sizeof(peer));
                    DebugLog("WowConnection: MaxSendDepth Reached! m_sendDepth=%d, m_maxSendDepth=%d, Dropping connection to %s", this->m_sendDepth, this->m_maxSendDepth, peer);
                }

                WowConnection::s_network->PlatformChangeState(this, this->m_connState);
            }
        }
    }

    this->m_lock.Leave();

    return result;
}

// OFFSET: 0x00467990
int32_t WowConnection::Send(uint8_t* data, int32_t size, bool raw) {
    WowConnection::s_sendBytes += size;

    int32_t result = 2;

    this->m_lock.Enter();

    if (!raw && this->m_encrypt) {
        int32_t length = this->m_uint375 + (size - 2 > 0x7FFF) + 2;

        if (length > size) {
            length = size;
        }

        SARC4ProcessBuffer(data, length, &this->m_sendKey, &this->m_sendKey);
    }

    if (size > 0 && this->m_connState == WOWC_CONNECTED) {
        if (!this->m_sendList.Head()) {
            int32_t sent = WowSocketSend(this->m_sock, reinterpret_cast<char*>(data), size);

            if (sent == size) {
                this->m_lock.Leave();
                return 0;
            }

            if (sent < 0) {
                if (WowSocketError() == WowSocketWouldBlock) {
                    sent = 0;
                } else if (this->m_sendDepth < this->m_maxSendDepth) {
                    this->SetState(WOWC_DISCONNECTING);
                    this->m_lock.Leave();
                    return 2;
                }
            }

            WowConnection::SENDNODE* node = this->NewSendNodePacket(data + sent, size - sent, true);
            this->m_sendList.LinkToTail(node);
            result = 1;
            this->m_sendDepth++;
            this->m_sendBytes += node->m_size;
            WowConnection::s_network->PlatformChangeState(this, this->m_connState);
        } else {
            result = 1;

            WowConnection::SENDNODE* node = this->NewSendNodePacket(data, size, true);
            this->m_sendList.LinkToTail(node);
            this->m_sendDepth++;
            this->m_sendBytes += node->m_size;

            if (this->m_sendDepth >= this->m_maxSendDepth) {
                this->SetState(WOWC_DISCONNECTING);

                if (this->m_response) {
                    this->m_response->NotifySendDepthExceeded(this);
                }

                char peer[80];
                this->PeerAddrToStr(peer, sizeof(peer));
                DebugLog("WowConnection: MaxSendDepth Reached! m_sendDepth=%d, m_maxSendDepth=%d, Dropping connection to %s", this->m_sendDepth, this->m_maxSendDepth, peer);

                this->m_lock.Leave();
                return 2;
            }

            WowConnection::s_network->PlatformChangeState(this, this->m_connState);
        }
    }

    this->m_lock.Leave();

    return result;
}

// OFFSET: 0x00467BA0
void WowConnection::DoMessageReads() {
    if (!this->m_readBuffer) {
        this->m_readBuffer = static_cast<uint8_t*>(SMemAlloc(1024, __FILE__, __LINE__, 0));
        this->m_readBufferSize = 1024;
        this->m_readBytes = 0;
    }

    uint32_t timeStamp = OsGetAsyncTimeMs();

    this->AcquireResponseRef();

    if (this->m_response) {
        this->m_response->NotifyAboutToDoReads();
    }

    this->ReleaseResponseRef();

    while (true) {
        WowConnection::SENDNODE* node = this->m_lagTestRecvList.Head();

        if (!node || static_cast<int32_t>(timeStamp - node->m_sendTime) < 0) {
            break;
        }

        CDataStore msg(node->m_data, node->m_size);

        this->AcquireResponseRef();
        WowConnectionResponse* response = this->m_response;

        if (response) {
            this->m_lock.Leave();
            response->WCMessageReady(this, timeStamp, &msg);
            this->m_lock.Enter();
        }

        this->ReleaseResponseRef();
        this->m_lagTestRecvList.UnlinkNode(node);
        WDataStore::FreeBuffer(node, node->m_datasize + sizeof(WowConnection::SENDNODE) + 3);
    }

    int32_t bytes;

    while (true) {
        int32_t headerSize = 2;
        int32_t packetSize = -1;

        if (this->m_readBytes >= 2) {
            uint8_t* buffer = this->m_readBuffer;

            if (buffer[0] & 0x80) {
                headerSize = 3;

                if (this->m_readBytes >= 3) {
                    packetSize = (buffer[2] | ((buffer[1] | ((buffer[0] & 0x7F) << 8)) << 8)) + 3;
                }
            } else {
                packetSize = (buffer[1] | ((buffer[0] & 0x7F) << 8)) + 2;
            }
        }

        if (this->m_readBytes >= this->m_readBufferSize) {
            this->m_readBuffer = static_cast<uint8_t*>(SMemReAlloc(this->m_readBuffer, 2 * this->m_readBufferSize, __FILE__, __LINE__, 0));
            this->m_readBufferSize *= 2;

            if (static_cast<uint32_t>(this->m_readBufferSize) > 32000000) {
                this->Disconnect();
                return;
            }
        }

        int32_t toRead;

        if (packetSize >= 0) {
            toRead = packetSize - this->m_readBytes;

            if (this->m_readBufferSize - this->m_readBytes < toRead) {
                toRead = this->m_readBufferSize - this->m_readBytes;
            }
        } else {
            toRead = headerSize - this->m_readBytes;
        }

        if (toRead <= 0) {
            bytes = 0;
        } else {
            do {
                bytes = recv(this->m_sock, reinterpret_cast<char*>(this->m_readBuffer + this->m_readBytes), toRead, 0);
            } while (bytes < 0 && WowSocketError() == WowSocketInterrupted);

            if (bytes <= 0) {
                break;
            }
        }

        if (this->m_encrypt) {
            int32_t length = headerSize + this->m_uint376 - this->m_readBytes;

            if (length <= 0) {
                length = 0;
            }

            if (length >= bytes) {
                length = bytes;
            }

            SARC4ProcessBuffer(this->m_readBuffer + this->m_readBytes, length, &this->m_receiveKey, &this->m_receiveKey);
        }

        this->m_readBytes += bytes;

        if (packetSize >= 0 && this->m_readBytes >= packetSize) {
            CDataStore msg(this->m_readBuffer + headerSize, packetSize - headerSize);

            this->AcquireResponseRef();
            WowConnectionResponse* response = this->m_response;

            if (WowConnection::s_lagTestDelayMin) {
                WowConnection::SENDNODE* node = this->NewSendNodePacket(this->m_readBuffer + headerSize, packetSize - headerSize, true);
                this->m_lagTestRecvList.LinkToTail(node);
            } else if (response) {
                this->m_lock.Leave();
                response->WCMessageReady(this, timeStamp, &msg);
                this->m_lock.Enter();
            }

            this->m_readBytes = 0;
            this->ReleaseResponseRef();
        }

        if (bytes <= 0) {
            return;
        }
    }

    if (bytes < 0 && WowSocketError() == WowSocketWouldBlock) {
        return;
    }

    this->AcquireResponseRef();
    WowConnectionResponse* response = this->m_response;

    WowConnection::s_network->Remove(this);
    this->CloseSocket(this->m_sock);
    this->SetState(WOWC_DISCONNECTED);

    int32_t sock = this->m_sock;
#if defined(__APPLE__)
    this->m_sock = -1;
#endif
    this->m_lock.Leave();

    if (response && sock >= 0) {
        response->WCDisconnected(this, timeStamp, &this->m_peer);
    }

    this->m_lock.Enter();
#if !defined(__APPLE__)
    this->m_sock = -1;
#endif
    this->ReleaseResponseRef();
}

// OFFSET: 0x00468390
void WowConnection::CheckConnect() {
    int32_t error;
    WowSocketLength length = sizeof(error);

    if (getsockopt(this->m_sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &length)) {
        return;
    }

    if (error) {
        WowConnection::s_network->Remove(this);
        this->CloseSocket(this->m_sock);
        this->m_sock = -1;
        this->SetState(WOWC_DISCONNECTED);
        this->AddRef();
        this->AcquireResponseRef();

        WowConnectionResponse* response = this->m_response;
        this->m_lock.Leave();

        if (response) {
            response->WCCantConnect(this, OsGetAsyncTimeMs(), &this->m_peer);
        }
    } else {
        this->SetState(WOWC_CONNECTED);
        this->AddRef();
        this->AcquireResponseRef();

        WowConnectionResponse* response = this->m_response;
        this->m_lock.Leave();

        WowSocketLength nameLength = sizeof(this->m_peer.peerAddr);
        getpeername(this->m_sock, reinterpret_cast<sockaddr*>(&this->m_peer.peerAddr), &nameLength);
        nameLength = sizeof(this->m_peer.selfAddr);
        getsockname(this->m_sock, reinterpret_cast<sockaddr*>(&this->m_peer.selfAddr), &nameLength);

        if (response) {
            response->WCConnected(this, nullptr, OsGetAsyncTimeMs(), &this->m_peer);
        }
    }

    this->m_lock.Enter();
    this->ReleaseResponseRef();
    this->Release();
}

// OFFSET: 0x00468580
void WowConnection::CheckAccept() {
    for (int32_t i = 0; i < 1000; i++) {
        NETADDR addr;
        WowSocketLength addrLength = sizeof(addr);
        int32_t sock = static_cast<int32_t>(accept(this->m_sock, reinterpret_cast<sockaddr*>(&addr), &addrLength));

        if (sock < 0) {
            break;
        }

        if (WowConnection::s_verifyAddr) {
            NETADDR peer;
            WowSocketLength peerLength = sizeof(peer);
            getpeername(sock, reinterpret_cast<sockaddr*>(&peer), &peerLength);

            if (!WowConnection::s_verifyAddr(&peer)) {
                WowSocketClose(sock);
                continue;
            }
        }

        if (!WowSocketNonBlocking(sock)) { WowSocketClose(sock); continue; }
        WowConnection::MarkSocketId(sock);

        WowConnection* inbound = new (__FILE__, __LINE__) WowConnection(sock, &addr, this->m_response);
        inbound->AddRef();
        this->AddRef();
        this->AcquireResponseRef();

        WowConnectionResponse* response = this->m_response;
        this->m_lock.Leave();

        if (response) {
            response->WCConnected(this, inbound, OsGetAsyncTimeMs(), &inbound->m_peer);
        }

        WowConnection::s_network->Add(inbound);
        WowConnection::s_network->PlatformChangeState(inbound, inbound->m_connState);
        inbound->Release();

        this->m_lock.Enter();
        this->ReleaseResponseRef();
        this->Release();
    }
}

// OFFSET: 0x00468790
void WowConnection::DoWrites() {
    this->AddRef();
    this->m_lock.Enter();

    bool disconnected = false;

    if (this->m_connState == WOWC_CONNECTING) {
        this->CheckConnect();
    } else if (this->m_sendList.Head()) {
        while (true) {
            WowConnection::SENDNODE* node = this->m_sendList.Head();

            if (!node) {
                break;
            }

            uint32_t now = OsGetAsyncTimeMs();

            if (WowConnection::s_lagTestDelayMin && static_cast<int32_t>(now - node->m_sendTime) < 0) {
                break;
            }

            uint32_t bytes = node->m_size - node->m_offset;

            if (bytes >= 0x400) {
                bytes = 0x400;
            }

            if (bytes) {
                int32_t sent = WowSocketSend(this->m_sock, reinterpret_cast<char*>(node->m_data + node->m_offset), bytes);

                if (sent != static_cast<int32_t>(bytes)) {
                    if (sent > 0) {
                        node->m_offset += sent;
                        break;
                    }

                    if (sent >= 0 || WowSocketError() == WowSocketWouldBlock) {
                        break;
                    }

                    disconnected = true;
                }
            }

            if (!disconnected) {
                if (bytes == node->m_size - node->m_offset) {
                    this->m_sendList.UnlinkNode(node);
                    this->m_sendDepth--;
                    this->m_sendBytes -= node->m_size;
                    WDataStore::FreeBuffer(node, node->m_datasize + sizeof(WowConnection::SENDNODE) + 3);
                } else {
                    node->m_offset += bytes;
                }
            }

            if (!this->m_sendList.Head() || disconnected) {
                break;
            }
        }
    } else {
        WowConnection::s_network->PlatformChangeState(this, this->m_connState);
    }

    WowConnectionResponse* response = nullptr;
    int32_t sock = -1;
    bool writeReady = false;

    if (disconnected) {
        this->AcquireResponseRef();
        response = this->m_response;
        WowConnection::s_network->Remove(this);
        this->CloseSocket(this->m_sock);
        sock = this->m_sock;
#if defined(__APPLE__)
        this->m_sock = -1;
#endif
        this->SetState(WOWC_DISCONNECTED);
    } else if (this->m_notifyWriteReady && !this->m_sendList.Head()) {
        writeReady = true;
        this->m_notifyWriteReady = 0;
        this->AcquireResponseRef();
        response = this->m_response;
    }

    this->m_lock.Leave();

    if (disconnected) {
        if (response && sock >= 0) {
            response->WCDisconnected(this, OsGetAsyncTimeMs(), &this->m_peer);
        }

        this->m_lock.Enter();
#if !defined(__APPLE__)
        this->m_sock = -1;
#endif
        this->ReleaseResponseRef();
        this->m_lock.Leave();
    } else if (writeReady) {
        if (response) {
            response->WCWriteReady(this);
        }

        this->m_lock.Enter();
        this->ReleaseResponseRef();
        this->m_lock.Leave();
    }

    this->Release();
}

// OFFSET: 0x00468BA0
void WowConnection::StartConnect() {
#if defined(__APPLE__)
    this->m_lock.Enter();
    if (!s_network || s_network->m_stop) { this->m_lock.Leave(); return; }
    ++this->m_socketSerial;
    this->FreeSendList();
    this->m_sendDepth = this->m_sendBytes = 0;
#endif
    if (this->m_sock >= 0) {
        if (this->m_netLink.IsLinked()) {
            WowConnection::s_network->Remove(this);
        }

        this->CloseSocket(this->m_sock);
        this->m_sock = -1;
    }

#if !defined(__APPLE__)
    this->m_lock.Enter();
#endif

    int32_t sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        SErrDisplayErrorFmt(0x85100000, __FILE__, __LINE__, 0, 1, "socket() returned %d, reason: %s", sock, "can't create socket (No network connection available?)");
    }

    WowConnection::MarkSocketId(sock);
    this->m_sock = sock;

    if (sock >= 0 && !WowSocketNonBlocking(sock)) {
        this->CloseSocket(sock); this->m_sock = sock = -1;
    }
    if (sock >= 0) {

        sockaddr_in address{};
#if defined(__APPLE__)
        address.sin_len = sizeof(address);
#endif
        address.sin_family = AF_INET;
        address.sin_port = htons(this->m_connectPort);
        address.sin_addr.s_addr = this->m_connectAddress;

        if (!this->m_netLink.IsLinked()) {
            WowConnection::s_network->Add(this);
        }

        this->m_readBytes = 0;
        this->SetState(WOWC_CONNECTING);

        if (connect(this->m_sock, reinterpret_cast<sockaddr*>(&address), sizeof(address)) >= 0 || WowSocketConnectPending()) {
            this->m_lock.Leave();
            return;
        }

        static_cast<void>(errno);

        WowConnection::s_network->Remove(this);
        this->CloseSocket(this->m_sock);
        this->m_sock = -1;
    }

    this->SetState(WOWC_ERROR);
    this->m_lock.Leave();
}

// OFFSET: 0x00468D20
bool WowConnection::Connect(const char* host, uint16_t port, int32_t timeout) {
#if defined(__APPLE__)
    // Preserve the IPv4 wire/address API while avoiding gethostbyname's shared
    // resolver storage. A failed lookup must not silently connect to 0.0.0.0.
    addrinfo hints{}, *addresses = nullptr;
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_STREAM;
    if (!s_network || s_network->m_stop || !host || getaddrinfo(host, nullptr, &hints, &addresses)) return false;
    const auto address = reinterpret_cast<const sockaddr_in*>(addresses->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(addresses);
    return this->Connect(address, port, timeout);
#else
    uint32_t address = inet_addr(host);

    if (address == INADDR_NONE || !address) {
        hostent* entry = gethostbyname(host);

        if (entry) {
            const uint8_t* bytes = reinterpret_cast<const uint8_t*>(entry->h_addr_list[0]);
            this->m_connectAddress = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
        } else {
            this->m_connectAddress = 0;
        }
    } else {
        this->m_connectAddress = address;
    }

    this->m_connectPort = port;
    this->StartConnect();

    return true;
#endif
}

#endif
