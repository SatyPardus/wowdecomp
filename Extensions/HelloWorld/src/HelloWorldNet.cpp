#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// Two opcodes outside the stock protocol (NUM_MSG_TYPES is 0x51F): the server-to-client one the
// extension handles, the client-to-server one it sends. A server has to implement both for the
// round trip to happen; the stock server ignores or refuses them.
const uint32_t SMSG_HELLO_WORLD = 0x5F0;
const uint32_t CMSG_HELLO_WORLD = 0x5F1;

uint32_t s_hellosSent;

// The handler for SMSG_HELLO_WORLD: a uint32 counter followed by a string. Same shape as a stock
// handler; the cursor is at the payload.
int32_t HandleHelloWorld(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t counter = 0;
    char text[256] = "";
    msg->Get(counter);
    msg->GetString(text, sizeof(text));
    sExtensionMgr->Log("HelloWorld: SMSG_HELLO_WORLD #%u \"%s\" (t=%u)", counter, text, time);

    return 1;
}

// HelloWorld_SendHello([text]): sends CMSG_HELLO_WORLD with a counter and the text, returns the
// counter, or nil when there is no connection. Registered in the game state only.
int32_t Script_HelloWorld_SendHello(lua_State* L) {
    const char* text = lua_tolstring(L, 1, nullptr);
    CDataStore payload;
    payload.Put(++s_hellosSent);
    payload.PutString(text ? text : "hello");
    payload.Finalize();

    if (!sExtensionMgr->SendClientMessage(CMSG_HELLO_WORLD, payload)) {
        s_hellosSent--;
        lua_pushnil(L);
        return 1;
    }

    sExtensionMgr->Log("HelloWorld: CMSG_HELLO_WORLD #%u sent", s_hellosSent);
    lua_pushnumber(L, s_hellosSent);

    return 1;
}

// Logs the first few messages in each direction with their opcode and payload size, then only
// counts. The sizes read the stream's own fields; nothing is consumed from it.
class HelloWorldNetScript : public NetScript {
    public:
    HelloWorldNetScript() : NetScript("HelloWorldNetScript") {}

    void OnConnected(NetClient& client) override {
        sExtensionMgr->Log("HelloWorld: OnConnected");
    }

    void OnDisconnected(NetClient& client) override {
        sExtensionMgr->Log("HelloWorld: OnDisconnected after %u received, %u sent", m_received, m_sent);
    }

    // Lets everything through; a real script would compare msgId against the opcodes it owns.
    bool CanMessageReceive(NETMESSAGE msgId, uint32_t timeReceived, CDataStore& msg) override {
        return true;
    }

    void OnMessageReceived(NETMESSAGE msgId, uint32_t timeReceived, CDataStore& msg) override {
        m_received++;

        if (m_received <= 5) {
            sExtensionMgr->Log("HelloWorld: OnMessageReceived opcode 0x%03X, %u payload byte(s), t=%u", msgId, msg.m_size - msg.m_read, timeReceived);
        } else if (m_received % 500 == 0) {
            sExtensionMgr->Log("HelloWorld: %u message(s) received, %u sent", m_received, m_sent);
        }
    }

    void OnMessageSent(NETMESSAGE msgId, CDataStore& msg) override {
        m_sent++;

        if (m_sent <= 5) {
            sExtensionMgr->Log("HelloWorld: OnMessageSent opcode 0x%03X, %u byte(s)", msgId, msg.m_size - msg.m_read);
        }
    }

    private:
    uint32_t m_received = 0;
    uint32_t m_sent = 0;
};

}

void AddHelloWorldNetScripts() {
    sExtensionMgr->RegisterMessageHandler(SMSG_HELLO_WORLD, HandleHelloWorld, nullptr);
    sExtensionMgr->RegisterLuaFunction("HelloWorld_SendHello", Script_HelloWorld_SendHello, FRAMESCRIPT_CONTEXTS_GAME);
    new HelloWorldNetScript();
}
