#include "UIBindings.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Ui/Types.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Ui/MODIFIEDCLICK.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#if defined(_WIN32)
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 47beb07e auto
// OFFSET: 0x0055E9B0
static int32_t Script_GetBindingKey(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetBindingKey(\"COMMAND\"[, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_ACTIVE;

    if (lua_isnumber(L, 2)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

        if (index <= BINDING_MODE_3) {
            mode = static_cast<BINDING_MODE>(index);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    int32_t count = 0;
    const char* command = lua_tolstring(L, 1, nullptr);

    for (const char* key = bindings->GetCommandKey(mode, command, 0); key; key = bindings->GetCommandKey(mode, command, count)) {
        count++;
        lua_pushstring(L, key);
    }

    return count;
}

// VERIFY: 81c3e64f auto
// OFFSET: 0x0055DC00
static int32_t Script_GetNumBindings(lua_State* L) {
    lua_pushnumber(L, CGUIBindings::s_bindings->m_numCommands);
    return 1;
}

// VERIFY: d8644d01 auto
// OFFSET: 0x0055DC30
static int32_t Script_GetCurrentBindingSet(lua_State* L) {
    lua_pushnumber(L, CGUIBindings::s_bindings->m_saveSet);
    return 1;
}

// VERIFY: 9d1e9643 auto
// OFFSET: 0x0055DC60
static int32_t Script_GetNumModifiedClickActions(lua_State* L) {
    lua_pushnumber(L, CGUIBindings::s_bindings->m_numModifiedClicks);
    return 1;
}

// VERIFY: 4918368e auto
// OFFSET: 0x00564070
static int32_t Script_LoadBindings(lua_State* L) {
    int32_t set = static_cast<int32_t>(lua_tonumber(L, 1));

    if (set >= 3) {
        luaL_error(L, "Usage: LoadBindings(0||1||2)");
        return 0;
    }

    CGUIBindings::s_bindings->CopyBindings(static_cast<BINDING_SET>(set), BINDING_SET_CURRENT);
    return 0;
}

// VERIFY: 9a8cf3bd manual
// OFFSET: 0x0055EA70
static int32_t Script_GetModifiedClickAction(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetModifiedClickAction(index)");
        return 0;
    }

    lua_pushstring(L, CGUIBindings::s_bindings->GetModifiedClick(static_cast<int32_t>(lua_tonumber(L, 1)) - 1));
    return 1;
}

// VERIFY: 56bf68fd auto
// OFFSET: 0x0055FCC0
static int32_t Script_IsModifiedClick(lua_State* L) {
    if (CGUIBindings::s_bindings->IsModifiedClick(lua_tolstring(L, 1, nullptr))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a6e5f57d auto
// OFFSET: 0x005640C0
static int32_t Script_SaveBindings(lua_State* L) {
    int32_t set = static_cast<int32_t>(lua_tonumber(L, 1));

    if (set != 1 && set != 2) {
        luaL_error(L, "Usage: SaveBindings(1||2)");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    bindings->m_saveSet = set;
    bindings->CopyBindings(BINDING_SET_CURRENT, static_cast<BINDING_SET>(set));

    if (set == 1) {
        bindings->CopyBindings(BINDING_SET_ACCOUNT, BINDING_SET_CHARACTER);
    }

    return 0;
}

// VERIFY: 9e9a649e auto
// OFFSET: 0x0055FB90
static int32_t Script_SetModifiedClick(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetModifiedClick(\"action\", \"binding\")");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* action = lua_tolstring(L, 1, nullptr);
    const char* binding = lua_tolstring(L, 2, nullptr);
    MODIFIEDCLICK* click;

    if (!action || !(click = bindings->m_modifiedClicks.Ptr(action)) || !click->SetBinding(BINDING_SET_CURRENT, binding)) {
        luaL_error(L, "SetModifiedClick(): Unknown action (%s) or binding (%s)", action, binding);
        return 0;
    }

    return 0;
}

// VERIFY: 8c63faae auto
// OFFSET: 0x0055FC20
static int32_t Script_GetModifiedClick(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetModifiedClick(\"action\")");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* action = lua_tolstring(L, 1, nullptr);
    MODIFIEDCLICK* click;

    if (action && (click = bindings->m_modifiedClicks.Ptr(action))) {
        char binding[128];
        click->GetBinding(BINDING_SET_CURRENT, binding, sizeof(binding), nullptr);
        lua_pushstring(L, binding);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: faa35395 auto
// OFFSET: 0x00562550
static int32_t Script_GetBindingAction(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetBindingAction(\"KEY\"[, checkOverride][, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_ACTIVE;

    if (lua_isnumber(L, 3)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    int32_t checkOverride = StringToBOOL(L, 2, 0);
    const char* action = bindings->GetCommandAction(mode, lua_tolstring(L, 1, nullptr), checkOverride);

    if (!action) {
        action = "";
    }

    lua_pushstring(L, action);
    return 1;
}

// VERIFY: 783d8193 auto
// OFFSET: 0x00560560
static int32_t Script_ClearOverrideBindings(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType())) {
        luaL_error(L, "Usage: ClearOverrideBindings(owner)");
        return 0;
    }

    CGUIBindings::s_bindings->ClearOverrideBindings(owner, true);
    return 0;
}

// VERIFY: 9b83a5dc manual
// OFFSET: 0x005625F0
static int32_t Script_GetBindingByKey(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetBindingByKey(\"action\"[, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_ACTIVE;

    if (lua_isnumber(L, 2)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    char key[128];
    KEYBINDING* binding = bindings->GetReducedKeyBinding(mode, lua_tolstring(L, 1, nullptr), key, sizeof(key));

    if (!binding) {
        return 0;
    }

    lua_pushstring(L, bindings->GetBindingCommand(binding, mode));
    return 1;
}

// VERIFY: 23d2b797 manual
// OFFSET: 0x0055FAD0
static int32_t Script_RunBinding(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: RunBinding(\"COMMAND\")");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    int32_t isDown = 1;
    float pressure;

    if (!lua_isstring(L, 2) || SStrCmpI(lua_tolstring(L, 2, nullptr), "up", STORM_MAX_STR)) {
        pressure = 1.0f;
    } else {
        pressure = 0.0f;
        isDown = 0;
    }

    const char* taint = lua_tainted;
    bindings->ExecCommand(lua_tolstring(L, 1, nullptr), isDown, pressure, 1, 0, 0, -1.0f, 0.0f, taint);
    return 0;
}

// VERIFY: 8dba3261 auto
// OFFSET: 0x00563520
static int32_t Script_SetBinding(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetBinding(\"KEY\"[, \"COMMAND\"][, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 3)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* command = lua_tolstring(L, 2, nullptr);

    if (bindings->Bind(BINDING_SET_CURRENT, mode, lua_tolstring(L, 1, nullptr), command)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ed0ea2f2 auto
// OFFSET: 0x0055E8D0
static int32_t Script_GetBinding(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetBinding(index[, mode])");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    int32_t count = 0;
    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;
    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 2)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    const char* command = "";
    bindings->GetCommand(index, &command);
    lua_pushstring(L, command);

    for (const char* key = bindings->GetCommandKey(mode, command, 0); key; key = bindings->GetCommandKey(mode, command, count)) {
        count++;
        lua_pushstring(L, key);
    }

    return count + 1;
}

// VERIFY: 44fa649a auto
// OFFSET: 0x00563A90
static int32_t Script_SetOverrideBinding(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType()) || !lua_isstring(L, 3)) {
        luaL_error(L, "Usage: SetOverrideBinding(owner, isPriority, \"KEY\"[, \"COMMAND\"])");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* command = nullptr;

    if (lua_isstring(L, 4)) {
        command = lua_tolstring(L, 4, nullptr);
    }

    const char* key = lua_tolstring(L, 3, nullptr);
    bindings->SetOverrideBinding(owner, StringToBOOL(L, 2, 0), key, command, true);
    return 0;
}

// VERIFY: 72a9de4c auto
// OFFSET: 0x005635E0
static int32_t Script_SetBindingSpell(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetBindingSpell(\"KEY\", \"spellname\"[, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 3)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 2, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "SPELL %s", argument);

    if (bindings->Bind(BINDING_SET_CURRENT, mode, lua_tolstring(L, 1, nullptr), command)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e02e1678 auto
// OFFSET: 0x00563700
static int32_t Script_SetBindingItem(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetBindingItem(\"KEY\", \"itemname\"[, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 3)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 2, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "ITEM %s", argument);

    if (bindings->Bind(BINDING_SET_CURRENT, mode, lua_tolstring(L, 1, nullptr), command)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a6cf9194 auto
// OFFSET: 0x00563820
static int32_t Script_SetBindingMacro(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetBindingMacro(\"KEY\", \"macroname\"|macroid[, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 3)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 2, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "MACRO %s", argument);

    if (bindings->Bind(BINDING_SET_CURRENT, mode, lua_tolstring(L, 1, nullptr), command)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 93460342 auto
// OFFSET: 0x00563940
static int32_t Script_SetBindingClick(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetBindingClick(\"KEY\", \"buttonName\"[, \"mouseButton\"][, mode])");
        return 0;
    }

    BINDING_MODE mode = BINDING_MODE_0;

    if (lua_isnumber(L, 4)) {
        uint32_t value = static_cast<uint32_t>(lua_tonumber(L, 4)) - 1;

        if (value <= 3) {
            mode = static_cast<BINDING_MODE>(value);
        }
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* button = lua_tolstring(L, 2, nullptr);
    const char* mouseButton = lua_tolstring(L, 3, nullptr);

    if (!mouseButton) {
        mouseButton = "LeftButton";
    }

    size_t length = SStrLen(button) + SStrLen(mouseButton) + 8;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "CLICK %s:%s", button, mouseButton);

    if (bindings->Bind(BINDING_SET_CURRENT, mode, lua_tolstring(L, 1, nullptr), command)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 8c961e80 auto
// OFFSET: 0x00563B80
static int32_t Script_SetOverrideBindingSpell(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType()) || !lua_isstring(L, 3) || !lua_isstring(L, 4)) {
        luaL_error(L, "Usage: SetOverrideBindingSpell(owner, isPriority, \"KEY\", \"spellname\")");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 4, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "SPELL %s", argument);
    const char* key = lua_tolstring(L, 3, nullptr);
    bindings->SetOverrideBinding(owner, StringToBOOL(L, 2, 0), key, command, true);
    return 0;
}

// VERIFY: 7ede6409 auto
// OFFSET: 0x00563CB0
static int32_t Script_SetOverrideBindingItem(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType()) || !lua_isstring(L, 3) || !lua_isstring(L, 4)) {
        luaL_error(L, "Usage: SetOverrideBindingItem(owner, isPriority, \"KEY\", \"itemname\")");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 4, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "ITEM %s", argument);
    const char* key = lua_tolstring(L, 3, nullptr);
    bindings->SetOverrideBinding(owner, StringToBOOL(L, 2, 0), key, command, true);
    return 0;
}

// VERIFY: 08f60cfe auto
// OFFSET: 0x00563DE0
static int32_t Script_SetOverrideBindingMacro(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType()) || !lua_isstring(L, 3) || !lua_isstring(L, 4)) {
        luaL_error(L, "Usage: SetOverrideBindingMacro(owner, isPriority, \"KEY\", \"macroname\"|macroid)");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* argument = lua_tolstring(L, 4, nullptr);
    size_t length = SStrLen(argument) + 7;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "MACRO %s", argument);
    const char* key = lua_tolstring(L, 3, nullptr);
    bindings->SetOverrideBinding(owner, StringToBOOL(L, 2, 0), key, command, true);
    return 0;
}

// VERIFY: 12320864 auto
// OFFSET: 0x00563F10
static int32_t Script_SetOverrideBindingClick(lua_State* L) {
    CScriptObject* owner = nullptr;

    if (lua_type(L, 1) == LUA_TTABLE) {
        lua_rawgeti(L, 1, 0);
        owner = static_cast<CScriptObject*>(lua_touserdata(L, -1));
        lua_settop(L, -2);
    }

    if (!owner || !owner->IsA(CSimpleFrame::GetObjectType()) || !lua_isstring(L, 3) || !lua_isstring(L, 4)) {
        luaL_error(L, "Usage: SetOverrideBindingClick(owner, isPriority, \"KEY\", \"buttonName\"[, \"mouseButton\"])");
        return 0;
    }

    CGUIBindings* bindings = CGUIBindings::s_bindings;
    const char* button = lua_tolstring(L, 4, nullptr);
    const char* mouseButton = lua_tolstring(L, 5, nullptr);

    if (!mouseButton) {
        mouseButton = "LeftButton";
    }

    size_t length = SStrLen(button) + SStrLen(mouseButton) + 8;
    char* command = static_cast<char*>(alloca(length));
    SStrPrintf(command, length, "CLICK %s:%s", button, mouseButton);
    const char* key = lua_tolstring(L, 3, nullptr);
    bindings->SetOverrideBinding(owner, StringToBOOL(L, 2, 0), key, command, true);
    return 0;
}

// VERIFY: 848027c4 manual
// OFFSET: 0x00564130
static int32_t Script_GetClickFrame(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetClickFrame(\"name\")");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    CSimpleFrame* frame = GetClickFrame(L, name, SStrLen(name));

    if (!frame) {
        lua_pushnil(L);
        return 1;
    }

    if (!frame->m_luaRegistered) {
        frame->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, frame->m_luaObjectRef);
    return 1;
}

// STATIC: 0x00ACDE38
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumBindings", &Script_GetNumBindings },
    { "GetBinding", &Script_GetBinding },
    { "SetBinding", &Script_SetBinding },
    { "SetBindingSpell", &Script_SetBindingSpell },
    { "SetBindingItem", &Script_SetBindingItem },
    { "SetBindingMacro", &Script_SetBindingMacro },
    { "SetBindingClick", &Script_SetBindingClick },
    { "SetOverrideBinding", &Script_SetOverrideBinding },
    { "SetOverrideBindingSpell", &Script_SetOverrideBindingSpell },
    { "SetOverrideBindingItem", &Script_SetOverrideBindingItem },
    { "SetOverrideBindingMacro", &Script_SetOverrideBindingMacro },
    { "SetOverrideBindingClick", &Script_SetOverrideBindingClick },
    { "ClearOverrideBindings", &Script_ClearOverrideBindings },
    { "GetBindingKey", &Script_GetBindingKey },
    { "GetBindingAction", &Script_GetBindingAction },
    { "GetBindingByKey", &Script_GetBindingByKey },
    { "RunBinding", &Script_RunBinding },
    { "GetCurrentBindingSet", &Script_GetCurrentBindingSet },
    { "LoadBindings", &Script_LoadBindings },
    { "SaveBindings", &Script_SaveBindings },
    { "GetNumModifiedClickActions", &Script_GetNumModifiedClickActions },
    { "GetModifiedClickAction", &Script_GetModifiedClickAction },
    { "SetModifiedClick", &Script_SetModifiedClick },
    { "GetModifiedClick", &Script_GetModifiedClick },
    { "IsModifiedClick", &Script_IsModifiedClick },
    { "GetClickFrame", &Script_GetClickFrame },
};

// VERIFY: 8e440f04 auto
// OFFSET: 0x0055DC90
void UIBindingsRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 3982d8aa auto
// OFFSET: 0x0055DCC0
void UIBindingsUnegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
