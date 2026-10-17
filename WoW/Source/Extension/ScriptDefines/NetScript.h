#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_NETSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_NETSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
class NetClient;

// A message handler an extension registers with ExtensionMgr::RegisterMessageHandler. Same shape
// as the client's own MESSAGE_HANDLER: param is the registration's user pointer, msgId the
// opcode, time the receive timestamp, msg the stream with the cursor at the payload. The return
// value is passed back to NetClient::ProcessMessage's caller like a stock handler's.
typedef int32_t (*ExtensionMessageHandler)(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);

// The world-server connection (WoW/Source/Net/NetClient/NetClient.cpp): its state changes and
// the two points every message passes through. The message hooks never rewrite a message, and
// the core restores the stream's read cursor after each hook so the engine's own handler sees
// the message exactly as if no script had looked at it. A script may therefore read from msg
// freely with CDataStore::Get. Do not keep a pointer to msg past the hook.
class NetScript : public ScriptObject {
    protected:
    explicit NetScript(const char* name);

    public:
    // NetClient::HandleConnect, once m_netState is NS_CONNECTED: the socket to the world server
    // is up, nothing has been sent yet (the encryption handshake follows with the first
    // messages). Fires for the realm connection and for a redirect connection alike.
    virtual void OnConnected(NetClient& client) {}

    // NetClient::HandleDisconnect, once m_netState is back to NS_INITIALIZED and before the glue
    // is told (CGlueMgr::NetDisconnectHandler): the connection is gone, whatever the cause
    // (server close, logout, network error).
    virtual void OnDisconnected(NetClient& client) {}

    // NetClient::ProcessMessage after OnMessageReceived ran for every script, before the
    // handler dispatch. Return false to drop the message: it is then treated like an opcode with
    // no handler (stream reset, ProcessMessage returns 0) and no engine handler sees it. The
    // first script that returns false ends the vote. Dropping a message the engine relies on
    // (SMSG_AUTH_CHALLENGE, movement, object updates) desynchronises the client; this is for
    // messages the script owns or knowingly suppresses.
    virtual bool CanMessageReceive(NETMESSAGE msgId, uint32_t timeReceived, CDataStore& msg) { return true; }

    // NetClient::ProcessMessage, after the 16-bit opcode is read and before the message is
    // dispatched to the handler registered for it (or dropped if there is none). The read cursor
    // is at the payload, where a handler starts reading. Fires for every message of a connected
    // session, including opcodes nothing handles; msgId may be outside NETMESSAGE if the server
    // sent one. timeReceived is the receive timestamp the connection stamped the packet with.
    virtual void OnMessageReceived(NETMESSAGE msgId, uint32_t timeReceived, CDataStore& msg) {}

    // NetClient::Send, right before the message goes to the connection's SendPacket. The read
    // cursor is at the 32-bit opcode (what the connection serialises); msgId is that opcode. Fires
    // once per message actually handed to the connection: messages held while the client is
    // suspended (realm redirect) fire when the held queue is flushed, not when they were queued.
    virtual void OnMessageSent(NETMESSAGE msgId, CDataStore& msg) {}
};

#endif
