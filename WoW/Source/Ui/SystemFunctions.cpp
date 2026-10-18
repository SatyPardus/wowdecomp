#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// Shared by the original Glue and in-world UI registration paths.
// VERIFY: 177658fd auto
// OFFSET: 0x006081F0
static int32_t Script_GetTime(lua_State* L) {
    lua_pushnumber(L, OsGetAsyncTimeMs() * 0.001);

    return 1;
}

// VERIFY: 4e5ce7ae auto
// OFFSET: 0x00608230
static int32_t Script_GetGameTime(lua_State* L) {
    lua_pushnumber(L, g_clientGameTime.m_hour);
    lua_pushnumber(L, g_clientGameTime.m_minute);

    return 2;
}

// VERIFY: e5f40d18 auto
// OFFSET: 0x00608270
static int32_t Script_ConsoleExec(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: ConsoleExec(\"console_command\")");
        return 0;
    }

    ConsoleCommandExecute(lua_tolstring(L, 1, nullptr), 0);

    return 0;
}

// VERIFY: 20774f42 auto
// OFFSET: 0x00510BB0
static int32_t Script_ReadFile(lua_State* L) {
    luaL_error(L, "Access Denied");
    return 0;
}

// VERIFY: 972d4262 auto
// OFFSET: 0x00510BB0
static int32_t Script_DeleteFile(lua_State* L) {
    luaL_error(L, "Access Denied");
    return 0;
}

// VERIFY: f25bf2fd auto
// OFFSET: 0x00510BB0
static int32_t Script_AppendToFile(lua_State* L) {
    luaL_error(L, "Access Denied");
    return 0;
}

// VERIFY: bca364a3 auto
// OFFSET: 0x004DDC20
int32_t Script_GetAccountExpansionLevel(lua_State* L) {
    lua_pushnumber(L, ClientServices::Connection()->m_accountExpansion);

    return 1;
}

static FrameScript_Method s_SystemFunctions[] = {
    { "GetTime", &Script_GetTime },
    { "GetGameTime", &Script_GetGameTime },
    { "ConsoleExec", &Script_ConsoleExec },
    { "ReadFile", &Script_ReadFile },
    { "DeleteFile", &Script_DeleteFile },
    { "AppendToFile", &Script_AppendToFile },
    { "GetAccountExpansionLevel", &Script_GetAccountExpansionLevel }
};

// VERIFY: 5cf87403 auto
// OFFSET: 0x0060A120
void SystemRegisterFunctions() {
    for (uint32_t i = 0; i < sizeof(s_SystemFunctions) / sizeof(s_SystemFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_SystemFunctions[i].m_name, s_SystemFunctions[i].m_method);
    }
}

// VERIFY: 570706e7 auto
// OFFSET: 0x0060A150
void SystemUnregisterFunctions() {
    for (uint32_t i = 0; i < sizeof(s_SystemFunctions) / sizeof(s_SystemFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_SystemFunctions[i].m_name);
    }
}

