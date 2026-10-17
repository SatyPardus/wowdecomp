#include "WoW/Source/Extension/ScriptDefines/NetScript.h"
#include <vector>
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// Handlers registered for opcodes the stock client does not handle. Consulted by
// NetClient::ProcessMessage after the stock m_handlers table came up empty for the opcode, so a
// registration never shadows a stock handler; one handler per opcode.
struct EXTENSIONMESSAGEHANDLER {
    uint32_t m_opcode;
    ExtensionMessageHandler m_handler;
    void* m_param;
};

static std::vector<EXTENSIONMESSAGEHANDLER> s_messageHandlers;

static EXTENSIONMESSAGEHANDLER* FindMessageHandler(uint32_t opcode) {
    for (EXTENSIONMESSAGEHANDLER& entry : s_messageHandlers) {
        if (entry.m_opcode == opcode) {
            return &entry;
        }
    }

    return nullptr;
}

void ExtensionMgr::RegisterMessageHandler(uint32_t opcode, ExtensionMessageHandler handler, void* param) {
    if (!handler) {
        return;
    }

    // The wire opcode is 16 bits; anything above cannot arrive.
    if (opcode > 0xFFFF) {
        Log("Extensions: RegisterMessageHandler(0x%X) ignored, opcodes are 16-bit", opcode);
        return;
    }

    if (EXTENSIONMESSAGEHANDLER* entry = FindMessageHandler(opcode)) {
        Log("Extensions: message handler for opcode 0x%03X replaced", opcode);
        entry->m_handler = handler;
        entry->m_param = param;
        return;
    }

    s_messageHandlers.push_back({ opcode, handler, param });
    Log("Extensions: message handler registered for opcode 0x%03X", opcode);
}

void ExtensionMgr::UnregisterMessageHandler(uint32_t opcode) {
    for (size_t i = 0; i < s_messageHandlers.size(); i++) {
        if (s_messageHandlers[i].m_opcode == opcode) {
            s_messageHandlers.erase(s_messageHandlers.begin() + i);
            return;
        }
    }
}

bool ExtensionMgr::DispatchRegisteredMessage(NETMESSAGE msgId, uint32_t timeReceived, CDataStore* msg, int32_t& result) {
    EXTENSIONMESSAGEHANDLER* entry = FindMessageHandler(static_cast<uint32_t>(msgId));

    if (!entry) {
        return false;
    }

    result = entry->m_handler(entry->m_param, msgId, timeReceived, msg);

    return true;
}

bool ExtensionMgr::SendClientMessage(uint32_t opcode, const void* payload, uint32_t bytes) {
    // s_currentConnection is the realm connection of the current session; null in the login
    // glue before a realm is joined and after a disconnect.
    if (!ClientServices::s_currentConnection) {
        Log("Extensions: SendClientMessage(0x%03X) with no connection, dropped", opcode);
        return false;
    }

    // Same layout as every stock sender: a 32-bit opcode, then the payload; Finalize rewinds the
    // cursor so Send serialises from the start. Send itself drops the message when the connection
    // is not in the connected state, or holds it while the client is suspended for a redirect.
    CDataStore msg;
    msg.Put(opcode);

    if (bytes && payload) {
        msg.PutData(payload, bytes);
    }

    msg.Finalize();
    ClientServices::s_currentConnection->Send(&msg);

    return true;
}

bool ExtensionMgr::SendClientMessage(uint32_t opcode, CDataStore& payload) {
    uint32_t bytes = payload.m_size - payload.m_read;
    void* data = nullptr;

    if (bytes) {
        payload.GetDataInSitu(data, bytes);
    }

    return SendClientMessage(opcode, data, bytes);
}

void ExtensionMgr::ClearMessageHandlers() {
    s_messageHandlers.clear();
}

void ExtensionMgr::OnConnected(NetClient* client) {
    CALL_HOOKS(NetScript, script->OnConnected(*client));
}

void ExtensionMgr::OnDisconnected(NetClient* client) {
    CALL_HOOKS(NetScript, script->OnDisconnected(*client));
}

void ExtensionMgr::OnMessageReceived(NETMESSAGE msgId, uint32_t timeReceived, CDataStore* msg) {
    if (ScriptRegistry<NetScript>::ScriptPointerList.empty()) {
        return;
    }

    // The cursor is at the payload. Each script gets it there, and the handler the client
    // dispatches to afterwards gets it there too, whatever the scripts read.
    uint32_t read = msg->m_read;

    CALL_HOOKS(NetScript, (msg->m_read = read, script->OnMessageReceived(msgId, timeReceived, *msg)));

    msg->m_read = read;
}

bool ExtensionMgr::CanMessageReceive(NETMESSAGE msgId, uint32_t timeReceived, CDataStore* msg) {
    if (ScriptRegistry<NetScript>::ScriptPointerList.empty()) {
        return true;
    }

    uint32_t read = msg->m_read;
    bool allowed = true;

    // First refusal ends the vote; the cursor is restored for the dispatch either way.
    for (NetScript* script : ScriptRegistry<NetScript>::ScriptPointerList) {
        msg->m_read = read;

        if (!script->CanMessageReceive(msgId, timeReceived, *msg)) {
            allowed = false;
            break;
        }
    }

    msg->m_read = read;

    return allowed;
}

void ExtensionMgr::OnMessageSent(CDataStore* msg) {
    if (ScriptRegistry<NetScript>::ScriptPointerList.empty()) {
        return;
    }

    // Outgoing messages start with the opcode as a 32-bit value (every sender does
    // msg.Put(static_cast<uint32_t>(CMSG_...))); the connection serialises it from the cursor.
    uint32_t read = msg->m_read;
    uint32_t opcode = 0;

    if (msg->m_size - read >= sizeof(opcode)) {
        msg->Get(opcode);
    }

    CALL_HOOKS(NetScript, (msg->m_read = read, script->OnMessageSent(static_cast<NETMESSAGE>(opcode), *msg)));

    msg->m_read = read;
}

NetScript::NetScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<NetScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered NetScript %s", name);
}
