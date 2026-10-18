#include "CGCooldown.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

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

static uint32_t CooldownMilliseconds(double seconds) {
    const double milliseconds = seconds * 1000.0;
    // Original 0x5ECEF0/0x5ECF1B truncates to signed 64 bits with x87 FISTP,
    // then passes the low 32 bits. In particular, uptime above INT32_MAX is
    // valid. An invalid conversion produces integer indefinite (low bits 0).
    if (!(milliseconds >= -0x1p63 && milliseconds < 0x1p63)) {
        return 0;
    }
    return static_cast<uint32_t>(static_cast<int64_t>(milliseconds));
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

    uint32_t start = CooldownMilliseconds(lua_tonumber(L, 2));
    uint32_t duration = CooldownMilliseconds(lua_tonumber(L, 3));
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
