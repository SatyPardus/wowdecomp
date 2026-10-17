#include "CGCooldown.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/ModCore/ModCore.h"

// VERIFY: a013e686 manual
// OFFSET: 0x005EC9A0
static int32_t Script_SetReverse(lua_State* L) {
    CGCooldown* cooldown = static_cast<CGCooldown*>(FrameScript_GetObjectThis(L, CGCooldown::GetObjectType()));
    cooldown->m_reverse = StringToBOOL(L, 2, 1);
    return 0;
}

// VERIFY: 6a92da70 manual
// OFFSET: 0x005EC9F0
static int32_t Script_GetReverse(lua_State* L) {
    CGCooldown* cooldown = static_cast<CGCooldown*>(FrameScript_GetObjectThis(L, CGCooldown::GetObjectType()));
    lua_pushboolean(L, cooldown->m_reverse);
    return 1;
}

// VERIFY: 7b0d18d4 manual
// OFFSET: 0x005ECA30
static int32_t Script_SetDrawEdge(lua_State* L) {
    CGCooldown* cooldown = static_cast<CGCooldown*>(FrameScript_GetObjectThis(L, CGCooldown::GetObjectType()));
    cooldown->m_drawEdge = StringToBOOL(L, 2, 1);
    return 0;
}

// VERIFY: 776af0bc manual
// OFFSET: 0x005ECA80
static int32_t Script_GetDrawEdge(lua_State* L) {
    CGCooldown* cooldown = static_cast<CGCooldown*>(FrameScript_GetObjectThis(L, CGCooldown::GetObjectType()));
    lua_pushboolean(L, cooldown->m_drawEdge);
    return 1;
}

// VERIFY: 4ca46386 manual
// OFFSET: 0x005ECE80
static int32_t Script_SetCooldown(lua_State* L) {
    CGCooldown* cooldown = static_cast<CGCooldown*>(FrameScript_GetObjectThis(L, CGCooldown::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        const char* name = cooldown->GetName();
        luaL_error(L, "Usage: %s:SetCooldown(start, duration)", name ? name : "<unnamed>");
        return 0;
    }

    int32_t start = static_cast<int32_t>(lua_tonumber(L, 2) * 1000.0);
    int32_t duration = static_cast<int32_t>(lua_tonumber(L, 3) * 1000.0);
    ModCore::Manager().OnCooldownSet(cooldown ? cooldown->GetName() : "", static_cast<float>(lua_tonumber(L, 2)), static_cast<float>(lua_tonumber(L, 3)));
    cooldown->SetCooldown(start, duration);
    return 0;
}

// VERIFY: 8c9950ea auto
// OFFSET: 0x005EBE30
void CGCooldown::DestroyScriptMetaTable() {
    luaL_unref(FrameScript_GetContext(), LUA_REGISTRYINDEX, CGCooldown::s_metatable);
    CGCooldown::s_metatable = LUA_NOREF;
}

// VERIFY: 61b90397 auto
// OFFSET: 0x005ECAC0
void CGCooldown::CreateScriptMetaTable() {
    CGCooldown::s_metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), &CGCooldown::RegisterScriptMethods);
}

// STATIC: 0x00AD13A4
static FrameScript_Method s_ScriptMethods[] = {
    { "SetCooldown", &Script_SetCooldown },
    { "SetReverse", &Script_SetReverse },
    { "GetReverse", &Script_GetReverse },
    { "SetDrawEdge", &Script_SetDrawEdge },
    { "GetDrawEdge", &Script_GetDrawEdge },
};

// VERIFY: 778d16d5 auto
// OFFSET: 0x005EBE10
void CGCooldown::RegisterScriptMethods(lua_State* L) {
    CSimpleFrame::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_ScriptMethods, sizeof(s_ScriptMethods) / sizeof(s_ScriptMethods[0]));
}
