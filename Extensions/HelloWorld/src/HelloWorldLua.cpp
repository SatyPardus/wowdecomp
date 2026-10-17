#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

uint32_t s_pingCount;

// HelloWorld_Ping(): a global in both Lua states, registered once through the registration API
// and re-registered by the core into every state. It signals HELLO_WORLD_PING with the call
// count to every frame that registered for it, and returns the count. From the UI:
//   /run local f = CreateFrame("Frame") f:RegisterEvent("HELLO_WORLD_PING") f:SetScript("OnEvent", function(_, e, n) print(e, n) end)
//   /run print(HelloWorld_Ping())
int32_t Script_HelloWorld_Ping(lua_State* L) {
    s_pingCount++;

    // The event's index belongs to the current state; the core knows the array it built for it.
    uint32_t index = sExtensionMgr->GetFrameScriptEventIndex("HELLO_WORLD_PING");

    if (index != UINT32_MAX) {
        FrameScript_SignalEvent(index, "%d", s_pingCount);
    }

    lua_pushnumber(L, s_pingCount);

    return 1;
}

// Observes the state lifecycle; the registrations above need no hook. Shows the per-hook
// declaration path too: HELLO_WORLD_STATE is declared here, inside the hook, and gets an index
// in the same array as the registered event.
class HelloWorldLuaScript : public LuaScript {
    public:
    HelloWorldLuaScript() : LuaScript("HelloWorldLuaScript") {}

    void OnFrameScriptInitialized(lua_State* L, FrameScriptContext context) override {
        uint32_t stateEvent = sExtensionMgr->DeclareFrameScriptEvent("HELLO_WORLD_STATE");

        // The registered function is already a global when the hook runs.
        lua_getglobal(L, "HelloWorld_Ping");
        const char* pingState = lua_isfunction(L, -1) ? "registered" : "missing";
        lua_pop(L, 1);

        sExtensionMgr->Log("HelloWorld: OnFrameScriptInitialized (%s state), HELLO_WORLD_STATE is event %u, HelloWorld_Ping is %s", context == FRAMESCRIPT_CONTEXT_GLUE ? "glue" : "game", stateEvent, pingState);
    }

    void OnFrameScriptDestroy(lua_State* L) override {
        sExtensionMgr->Log("HelloWorld: OnFrameScriptDestroy after %u ping(s)", s_pingCount);
    }
};

}

void AddHelloWorldLuaScripts() {
    sExtensionMgr->RegisterLuaFunction("HelloWorld_Ping", Script_HelloWorld_Ping);
    sExtensionMgr->RegisterFrameScriptEvent("HELLO_WORLD_PING");
    new HelloWorldLuaScript();
}
