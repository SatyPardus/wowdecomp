#include "WoW/Source/Extension/ScriptDefines/LuaScript.h"
#include <vector>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

// Functions, events and script metatables registered once by extensions and applied to every
// FrameScript state the client creates for the contexts in their mask. The name pointers are the
// callers' string literals, which outlive the tables.
struct EXTENSIONLUAFUNCTION {
    const char* m_name;
    lua_CFunction m_function;
    uint32_t m_contexts;
};

struct EXTENSIONLUAEVENT {
    const char* m_name;
    uint32_t m_contexts;
};

struct EXTENSIONSCRIPTMETATABLE {
    int32_t* m_metatable;
    void (*m_registerMethods)(lua_State*);
    uint32_t m_contexts;
};

static std::vector<EXTENSIONLUAFUNCTION> s_luaFunctions;
static std::vector<EXTENSIONLUAEVENT> s_luaEvents;
static std::vector<EXTENSIONSCRIPTMETATABLE> s_scriptMetaTables;

// The event array of the current state: the engine's names first (their indices are the
// engine's enum values and must not move), the declared names after them. Built while
// OnFrameScriptInitialized runs (s_declaring), kept until the state is destroyed so
// GetFrameScriptEventIndex can answer, and empty while no state exists.
static std::vector<const char*> s_eventNames;
static bool s_declaring;

static uint32_t FindEventName(const char* name) {
    for (uint32_t i = 0; i < s_eventNames.size(); i++) {
        if (s_eventNames[i] && !SStrCmpI(s_eventNames[i], name, STORM_MAX_STR)) {
            return i;
        }
    }

    return UINT32_MAX;
}

void ExtensionMgr::RegisterLuaFunction(const char* name, lua_CFunction function, uint32_t contexts) {
    if (!name || !function || !(contexts & FRAMESCRIPT_CONTEXTS_ALL)) {
        return;
    }

    for (EXTENSIONLUAFUNCTION& entry : s_luaFunctions) {
        if (!SStrCmp(entry.m_name, name, STORM_MAX_STR)) {
            Log("Extensions: Lua function %s replaced", name);
            entry.m_function = function;
            entry.m_contexts = contexts;
            return;
        }
    }

    s_luaFunctions.push_back({ name, function, contexts });

    // A state that already exists gets the function at once when it matches; the next
    // OnFrameScriptInitialized covers every later state.
    if (!s_eventNames.empty() && (contexts & (1u << GetFrameScriptContext()))) {
        FrameScript_RegisterFunction(name, function);
    }
}

void ExtensionMgr::RegisterFrameScriptEvent(const char* name, uint32_t contexts) {
    if (!name || !(contexts & FRAMESCRIPT_CONTEXTS_ALL)) {
        return;
    }

    for (EXTENSIONLUAEVENT& entry : s_luaEvents) {
        if (!SStrCmpI(entry.m_name, name, STORM_MAX_STR)) {
            entry.m_contexts |= contexts;
            return;
        }
    }

    // An event can only join the array when it is built; one registered while a state is live
    // becomes addressable at the next state creation.
    s_luaEvents.push_back({ name, contexts });
}

void ExtensionMgr::RegisterScriptMetaTable(int32_t* metatable, void (*registerMethods)(lua_State*), uint32_t contexts) {
    if (!metatable || !registerMethods || !(contexts & FRAMESCRIPT_CONTEXTS_ALL)) {
        return;
    }

    for (const EXTENSIONSCRIPTMETATABLE& entry : s_scriptMetaTables) {
        if (entry.m_metatable == metatable) {
            return;
        }
    }

    s_scriptMetaTables.push_back({ metatable, registerMethods, contexts });
    *metatable = LUA_NOREF;

    // A matching live state gets its table now; the base types' tables it builds on
    // (CSimpleFrame::RegisterScriptMethods and the like) already exist in such a state.
    if (!s_eventNames.empty() && (contexts & (1u << GetFrameScriptContext()))) {
        *metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), registerMethods);
    }
}

uint32_t ExtensionMgr::GetFrameScriptEventIndex(const char* name) {
    return name ? FindEventName(name) : UINT32_MAX;
}

uint32_t ExtensionMgr::DeclareFrameScriptEvent(const char* name) {
    if (!s_declaring || !name) {
        Log("Extensions: DeclareFrameScriptEvent(%s) outside LuaScript::OnFrameScriptInitialized, ignored", name ? name : "null");
        return UINT32_MAX;
    }

    // FrameScript_CreateEvents maps equal names to one event object, so two indices for the same
    // name would both work; one index per name keeps FrameScript_SignalEvent callers consistent.
    uint32_t existing = FindEventName(name);

    if (existing != UINT32_MAX) {
        return existing;
    }

    s_eventNames.push_back(name);

    return static_cast<uint32_t>(s_eventNames.size() - 1);
}

static FrameScriptContext s_currentContext = FRAMESCRIPT_CONTEXT_GLUE;

FrameScriptContext ExtensionMgr::GetFrameScriptContext() {
    return s_currentContext;
}

void ExtensionMgr::OnFrameScriptInitialized(FrameScriptContext context, const char** names, uint32_t count) {
    uint32_t mask = 1u << context;
    s_currentContext = context;
    s_eventNames.assign(names, names + count);
    s_declaring = true;

    // Registered functions and events first, so a LuaScript hook can rely on them.
    for (const EXTENSIONLUAFUNCTION& entry : s_luaFunctions) {
        if (entry.m_contexts & mask) {
            FrameScript_RegisterFunction(entry.m_name, entry.m_function);
        }
    }

    for (const EXTENSIONLUAEVENT& entry : s_luaEvents) {
        if (entry.m_contexts & mask) {
            DeclareFrameScriptEvent(entry.m_name);
        }
    }

    lua_State* L = FrameScript_GetContext();

    // The engine's own metatables for this context are built before this call (LoadScriptFunctions
    // in the game, RegisterSimpleFrameScriptMethods in the glue), so a registerMethods that starts
    // from CSimpleFrame::RegisterScriptMethods finds them.
    for (const EXTENSIONSCRIPTMETATABLE& entry : s_scriptMetaTables) {
        if (entry.m_contexts & mask) {
            *entry.m_metatable = FrameScript_Object::CreateScriptMetaTable(L, entry.m_registerMethods);
        }
    }

    CALL_HOOKS(LuaScript, script->OnFrameScriptInitialized(L, context));

    s_declaring = false;

    // One call for the engine's events and the declared ones: the array is replaced wholesale,
    // so the two sets have to arrive together.
    FrameScript_CreateEvents(s_eventNames.data(), static_cast<uint32_t>(s_eventNames.size()));

    if (s_eventNames.size() > count) {
        Log("Extensions: %u FrameScript event(s) declared in the %s state", static_cast<uint32_t>(s_eventNames.size() - count), context == FRAMESCRIPT_CONTEXT_GLUE ? "glue" : "game");
    }
}

void ExtensionMgr::OnFrameScriptDestroy(lua_State* L) {
    CALL_HOOKS(LuaScript, script->OnFrameScriptDestroy(L));

    // The registry references die with the state; the owners' variables go back to "none" so a
    // GetScriptMetaTable between states returns LUA_NOREF rather than a stale reference.
    for (const EXTENSIONSCRIPTMETATABLE& entry : s_scriptMetaTables) {
        if (*entry.m_metatable != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, *entry.m_metatable);
            *entry.m_metatable = LUA_NOREF;
        }
    }

    s_eventNames.clear();
}

void ExtensionMgr::ClearLuaRegistrations() {
    s_luaFunctions.clear();
    s_luaEvents.clear();
    s_scriptMetaTables.clear();
    s_eventNames.clear();
}

LuaScript::LuaScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<LuaScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered LuaScript %s", name);
}
