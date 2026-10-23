#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CBackdropGenerator.h"
#include "Engine/Source/Frame/CSimpleAlphaAnim.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFontable.h"
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTitleRegion.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Ui/UIBindings.h"

// VERIFY: a331a2d8 manual
// OFFSET: 0x0049E5B0
static int32_t Script_GetTitleRegion(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CScriptRegion* titleRegion = frame->m_titleRegion;

    if (!titleRegion) {
        lua_pushnil(L);
        return 1;
    }

    if (!titleRegion->m_luaRegistered) {
        titleRegion->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, titleRegion->m_luaObjectRef);
    return 1;
}

// VERIFY: a960983a auto
// OFFSET: 0x0049E630
static int32_t Script_CreateTitleRegion(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CScriptRegion* titleRegion = frame->m_titleRegion;

    if (titleRegion) {
        titleRegion->MarkFramePointsUnused();
    } else {
        void* data = SMemAlloc(sizeof(CSimpleTitleRegion), __FILE__, __LINE__, 0);
        titleRegion = data ? new (data) CSimpleTitleRegion : nullptr;
        titleRegion->SetParent(frame);
        frame->m_titleRegion = titleRegion;
        titleRegion->SetDeferredResize((frame->m_layoutFlags >> 8) & 2);
    }

    if (!titleRegion->m_luaRegistered) {
        titleRegion->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, titleRegion->m_luaObjectRef);
    return 1;
}

// VERIFY: 3033acd4 manual
// OFFSET: 0x004A2010
static int32_t Script_CreateTexture(lua_State* L) {
    XMLNode* inheritNode = nullptr;
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    const char* name = nullptr;

    if (lua_isstring(L, 2)) {
        name = lua_tolstring(L, 2, nullptr);
    }

    uint32_t drawLayer = 2;

    if (lua_isstring(L, 3)) {
        StringToDrawLayer(lua_tolstring(L, 3, nullptr), drawLayer);
    }

    if (lua_type(L, 4) == LUA_TSTRING) {
        const char* inherits = lua_tolstring(L, 4, nullptr);
        const char* taint;
        bool locked;
        inheritNode = FrameXML_AcquireHashNode(inherits, taint, locked);

        if (!inheritNode) {
            const char* frameName = frame->GetName();
            luaL_error(L, "%s:CreateTexture(): Couldn't find inherited node \"%s\"", frameName ? frameName : "<unnamed>", inherits);
            return 0;
        }

        if (locked) {
            const char* frameName = frame->GetName();
            luaL_error(L, "%s:CreateTexture(): Recursively inherited node \"%s\"", frameName ? frameName : "<unnamed>", inherits);
            return 0;
        }
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(frame, drawLayer, 1) : nullptr;

    if (name && *name) {
        texture->SetName(name);
    }

    if (inheritNode) {
        CStatus status;
        texture->LoadXML(inheritNode, &status);
        texture->PostLoadXML(inheritNode, &status);
        FrameXML_ReleaseHashNode(lua_tolstring(L, 4, nullptr));
    }

    int32_t animating;
    int32_t alphaAnimating;
    texture->GetActiveAnimInfo(CSimpleAlphaAnim::GetObjectType(), animating, alphaAnimating);

    if (animating) {
        texture->AnimActivated(nullptr, 1, alphaAnimating);
    }

    if (!texture->m_luaRegistered) {
        texture->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, texture->m_luaObjectRef);
    return 1;
}

// VERIFY: e8e49864 manual
// OFFSET: 0x004A2240
static int32_t Script_CreateFontString(lua_State* L) {
    XMLNode* inheritNode = nullptr;
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    const char* name = nullptr;

    if (lua_isstring(L, 2)) {
        name = lua_tolstring(L, 2, nullptr);
    }

    uint32_t drawLayer = 2;

    if (lua_isstring(L, 3)) {
        StringToDrawLayer(lua_tolstring(L, 3, nullptr), drawLayer);
    }

    CSimpleFont* font = nullptr;

    if (lua_type(L, 4) == LUA_TSTRING) {
        const char* inherits = lua_tolstring(L, 4, nullptr);
        font = CSimpleFont::GetFont(inherits, 0);

        if (!font) {
            const char* taint;
            bool locked;
            inheritNode = FrameXML_AcquireHashNode(inherits, taint, locked);

            if (!inheritNode) {
                const char* frameName = frame->GetName();
                luaL_error(L, "%s:CreateFontString(): Couldn't find inherited node \"%s\"", frameName ? frameName : "<unnamed>", inherits);
                return 0;
            }

            if (locked) {
                const char* frameName = frame->GetName();
                luaL_error(L, "%s:CreateFontString(): Recursively inherited node \"%s\"", frameName ? frameName : "<unnamed>", inherits);
                return 0;
            }
        }
    }

    void* data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
    CSimpleFontString* string = data ? new (data) CSimpleFontString(frame, drawLayer, true) : nullptr;

    if (name && *name) {
        string->SetName(name);
    }

    if (font) {
        string->SetFontObject(font);
    } else if (inheritNode) {
        CStatus status;
        string->LoadXML(inheritNode, &status);
        string->PostLoadXML(inheritNode, &status);
        FrameXML_ReleaseHashNode(lua_tolstring(L, 4, nullptr));
    }

    int32_t animating;
    int32_t alphaAnimating;
    string->GetActiveAnimInfo(CSimpleAlphaAnim::GetObjectType(), animating, alphaAnimating);

    if (animating) {
        string->AnimActivated(nullptr, 1, alphaAnimating);
    }

    if (!string->m_luaRegistered) {
        string->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, string->m_luaObjectRef);
    return 1;
}

// VERIFY: a2154491 auto
// OFFSET: 0x0049E700
static int32_t Script_GetBoundsRect(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CRect rect = { 3.4028235e38f, 3.4028235e38f, 0.0f, 0.0f };

    if (!frame->GetBoundsRect(rect)) {
        return 0;
    }

    float scale = 1.0f / frame->m_layoutScale;
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * scale * rect.minX));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * scale * rect.minY));
    float width = rect.maxX - rect.minX;
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * scale * width));
    float height = rect.maxY - rect.minY;
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * scale * height));

    return 4;
}

// VERIFY: 754a3d6c manual
// OFFSET: 0x004A1E80
static int32_t Script_GetNumRegions(lua_State* L) {
    uint32_t count = 0;
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    for (CSimpleRegion* region = frame->m_regions.Head(); region; region = frame->m_regions.Next(region)) {
        count++;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: bd6ee0f5 manual
// OFFSET: 0x004A1F10
static int32_t Script_GetRegions(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    int32_t count = 0;

    for (CSimpleRegion* region = frame->m_regions.Head(); region; region = frame->m_regions.Next(region)) {
        count++;
    }

    if (!lua_checkstack(L, count)) {
        const char* name = frame->GetName();
        luaL_error(L, "%s:GetRegions(): Stack overflow", name ? name : "<unnamed>");
        return 0;
    }

    for (CSimpleRegion* region = frame->m_regions.Head(); region; region = frame->m_regions.Next(region)) {
        if (!region->m_luaRegistered) {
            region->RegisterScriptObject(nullptr);
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, region->m_luaObjectRef);
    }

    return count;
}

// VERIFY: bc25e0de manual
// OFFSET: 0x004A2490
static int32_t Script_GetNumChildren(lua_State* L) {
    uint32_t count = 0;
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    for (SIMPLEFRAMENODE* node = frame->m_children.Head(); node; node = node->m_link.Next()) {
        count++;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: 5247ce8d manual
// OFFSET: 0x004A2510
static int32_t Script_GetChildren(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    int32_t count = 0;

    for (SIMPLEFRAMENODE* node = frame->m_children.Head(); node; node = node->m_link.Next()) {
        count++;
    }

    if (!lua_checkstack(L, count)) {
        const char* name = frame->GetName();
        luaL_error(L, "%s:GetChildren(): Stack overflow", name ? name : "<unnamed>");
        return 0;
    }

    for (SIMPLEFRAMENODE* node = frame->m_children.Head(); node; node = node->m_link.Next()) {
        CSimpleFrame* child = node->m_frame;

        if (!child->m_luaRegistered) {
            child->RegisterScriptObject(nullptr);
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, child->m_luaObjectRef);
    }

    return count;
}

// VERIFY: 4b50cd06 manual
// OFFSET: 0x0049E830
static int32_t Script_GetFrameStrata(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushstring(L, FrameStrataToString(static_cast<FRAME_STRATA>(frame->m_strata)));
    return 1;
}

// VERIFY: 8fa95d1a manual
// OFFSET: 0x0049E880
static int32_t Script_SetFrameStrata(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetFrameStrata(level)", name ? name : "<unnamed>");
        return 0;
    }

    FRAME_STRATA strata;

    if (!StringToFrameStrata(lua_tolstring(L, 2, nullptr), strata)) {
        const char* name = frame->GetName();
        luaL_error(L, "%s:SetFrameStrata(): Unknown frame strata: %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
        return 0;
    }

    frame->SetFrameStrata(strata);
    return 0;
}

// VERIFY: afff24c6 manual
// OFFSET: 0x0049E980
static int32_t Script_GetFrameLevel(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushnumber(L, frame->m_level);
    return 1;
}

// VERIFY: 2dedbf38 manual
// OFFSET: 0x0049E9D0
static int32_t Script_SetFrameLevel(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetFrameLevel(level)", name ? name : "<unnamed>");
        return 0;
    }

    int32_t level = static_cast<int32_t>(lua_tonumber(L, 2));

    if (level < 0) {
        const char* name = frame->GetName();
        luaL_error(L, "%s:SetFrameLevel(): Passed negative frame level: %d", name ? name : "<unnamed>", level);
        return 0;
    }

    frame->SetFrameLevel(level, 1);
    return 0;
}

// VERIFY: 24a44dfc auto
// OFFSET: 0x0049EAB0
static int32_t Script_HasScript(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:HasScript(\"type\")", name ? name : "<unnamed>");
        return 0;
    }

    FrameScript_Object::ScriptData data;

    if (frame->GetScriptByName(lua_tolstring(L, 2, nullptr), data)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 6acdff0d auto
// OFFSET: 0x0049EB70
static int32_t Script_GetScript(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:GetScript(\"type\")", name ? name : "<unnamed>");
        return 0;
    }

    FrameScript_Object::ScriptData data;
    FrameScript_Object::ScriptIx* script = frame->GetScriptByName(lua_tolstring(L, 2, nullptr), data);

    if (!script) {
        const char* name = frame->GetName();
        luaL_error(L, "%s doesn't have a \"%s\" script", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
        return 0;
    }

    if (script->m_tainted && lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = script->m_tainted;
    }

    if (script->m_luaRef > 0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, script->m_luaRef);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 95f390db auto
// OFFSET: 0x0049EC80
static int32_t Script_SetScript(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2) || (lua_type(L, 3) != LUA_TFUNCTION && lua_type(L, 3) != LUA_TNIL)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetScript(\"type\", function)", name ? name : "<unnamed>");
        return 0;
    }

    FrameScript_Object::ScriptData data;
    FrameScript_Object::ScriptIx* script = frame->GetScriptByName(lua_tolstring(L, 2, nullptr), data);

    if (!script) {
        const char* name = frame->GetName();
        luaL_error(L, "%s doesn't have a \"%s\" script", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
        return 0;
    }

    if (script->m_luaRef) {
        luaL_unref(L, LUA_REGISTRYINDEX, script->m_luaRef);
    }

    int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
    script->m_luaRef = ref > 0 ? ref : 0;
    script->m_tainted = lua_tainted;

    return 0;
}

// VERIFY: 287347b4 auto
// OFFSET: 0x0049EDB0
static int32_t Script_HookScript(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2) || lua_type(L, 3) != LUA_TFUNCTION) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:HookScript(\"type\", function)", name ? name : "<unnamed>");
        return 0;
    }

    FrameScript_Object::ScriptData data;
    FrameScript_Object::ScriptIx* script = frame->GetScriptByName(lua_tolstring(L, 2, nullptr), data);

    if (!script) {
        const char* name = frame->GetName();
        luaL_error(L, "%s doesn't have a \"%s\" script", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
        return 0;
    }

    if (!script->m_luaRef) {
        int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);
        script->m_luaRef = ref > 0 ? ref : 0;
        script->m_tainted = lua_tainted;
        return 0;
    }

    lua_remove(L, 2);

    const char* hookTaint = lua_tainted;

    if (++lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = nullptr;
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, script->m_luaRef);
    luaL_unref(L, LUA_REGISTRYINDEX, script->m_luaRef);

    const char* scriptTaint = script->m_tainted;
    const char* taint = lua_tainted;

    if (++lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = scriptTaint;
    }

    lua_insert(L, 2);
    lua_settaint(L, 2, scriptTaint);
    lua_settaint(L, 3, hookTaint);
    lua_pushcclosure(L, &FrameScript_HookWrapper, 2);
    lua_settaint(L, 2, scriptTaint);

    if (lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = taint;
    }

    if (--lua_taintexpected <= 0) {
        lua_taintexpected = 0;
    }

    script->m_luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

    if (lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = hookTaint;
    }

    if (--lua_taintexpected <= 0) {
        lua_taintexpected = 0;
    }

    return 0;
}

// VERIFY: 0a32559b manual
// OFFSET: 0x0049EFE0
static int32_t Script_RegisterEvent(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:RegisterEvent(\"event\")", name ? name : "<unnamed>");
        return 0;
    }

    frame->RegisterScriptEvent(lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 1edce814 manual
// OFFSET: 0x0049F060
static int32_t Script_UnregisterEvent(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:UnregisterEvent(\"event\")", name ? name : "<unnamed>");
        return 0;
    }

    frame->UnregisterScriptEvent(lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: ea0c6858 manual
// OFFSET: 0x0049F0E0
static int32_t Script_RegisterAllEvents(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->RegisterAllScriptEvents();
    return 0;
}

// VERIFY: a0775ed2 manual
// OFFSET: 0x0049F120
static int32_t Script_UnregisterAllEvents(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->UnregisterAllScriptEvents();
    return 0;
}

// VERIFY: d19d3294 manual
// OFFSET: 0x0049F160
static int32_t Script_IsEventRegistered(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:IsEventRegistered(\"event\")", name ? name : "<unnamed>");
        return 0;
    }

    if (frame->IsScriptEventRegistered(lua_tolstring(L, 2, nullptr))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 42b41dfb manual
// OFFSET: 0x0049F210
static int32_t Script_AllowAttributeChanges(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_tainted) {
        frame->AllowAttributeChanges(1);
    }

    return 0;
}

// VERIFY: 5986af16 auto
// OFFSET: 0x0049F260
static int32_t Script_CanChangeAttribute(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->ProtectedFunctionsAllowed() || frame->AttributeChangesAllowed()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: da0c30ad manual
// OFFSET: 0x0049F2D0
static int32_t Script_GetAttribute(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (lua_gettop(L) == 4 && lua_isstring(L, 3)) {
        size_t prefixLength;
        size_t nameLength;
        size_t suffixLength;
        const char* prefix = lua_tolstring(L, 2, &prefixLength);
        const char* name = lua_tolstring(L, 3, &nameLength);
        const char* suffix = lua_tolstring(L, 4, &suffixLength);

        char buffer[256];
        char* dest = buffer;
        size_t space = sizeof(buffer) - 1;

        if (prefixLength) {
            size_t count = prefixLength > space ? space : prefixLength;
            memcpy(buffer, prefix, count);
            dest = buffer + count;
            space -= count;
        }

        if (nameLength) {
            size_t count = space < nameLength ? space : nameLength;
            memcpy(dest, name, count);
            dest += count;
            space -= count;
        }

        if (suffixLength) {
            size_t count = space < suffixLength ? space : suffixLength;
            memcpy(dest, suffix, count);
            dest += count;
        }

        *dest = '\0';
        int32_t luaRef;

        if (frame->GetAttribute(buffer, luaRef)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
            return 1;
        }

        buffer[0] = '*';
        dest = buffer + 1;
        space = sizeof(buffer) - 2;

        if (nameLength) {
            size_t count = nameLength > space ? space : nameLength;
            memcpy(buffer + 1, name, count);
            dest = buffer + 1 + count;
            space -= count;
        }

        if (suffixLength) {
            size_t count = space < suffixLength ? space : suffixLength;
            memcpy(dest, suffix, count);
            dest += count;
        }

        *dest = '\0';

        if (frame->GetAttribute(buffer, luaRef)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
            return 1;
        }

        dest = buffer;
        space = sizeof(buffer) - 2;

        if (prefixLength) {
            size_t count = prefixLength > space ? space : prefixLength;
            memcpy(buffer, prefix, count);
            dest = buffer + count;
            space -= count;
        }

        if (nameLength) {
            size_t count = space < nameLength ? space : nameLength;
            memcpy(dest, name, count);
            dest += count;
        }

        dest[0] = '*';
        dest[1] = '\0';

        if (frame->GetAttribute(buffer, luaRef)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
            return 1;
        }

        buffer[0] = '*';
        dest = buffer + 1;

        if (nameLength) {
            size_t count = nameLength > sizeof(buffer) - 3 ? sizeof(buffer) - 3 : nameLength;
            memcpy(buffer + 1, name, count);
            dest = buffer + 1 + count;
        }

        dest[0] = '*';
        dest[1] = '\0';

        if (frame->GetAttribute(buffer, luaRef) || frame->GetAttribute(name, luaRef)) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }

    if (!lua_isstring(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:GetAttribute(\"name\")", name ? name : "<unnamed>");
        return 0;
    }

    int32_t luaRef;

    if (frame->GetAttribute(lua_tolstring(L, 2, nullptr), luaRef)) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 0e32dc1d auto
// OFFSET: 0x0049F610
static int32_t Script_SetAttribute(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed() && !frame->AttributeChangesAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    lua_settop(L, 3);

    if (!lua_isstring(L, 2) || lua_type(L, 3) == LUA_TNONE) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetAttribute(\"name\", value)", name ? name : "<unnamed>");
        return 0;
    }

    const char* name = lua_tolstring(L, 2, nullptr);
    int32_t luaRef;

    if (frame->GetAttribute(name, luaRef)) {
        luaL_unref(L, LUA_REGISTRYINDEX, luaRef);
    }

    const char* taint = lua_tainted;

    if (++lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = nullptr;
    }

    lua_settaint(L, 3, nullptr);
    luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

    if (lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = taint;
    }

    if (--lua_taintexpected <= 0) {
        lua_taintexpected = 0;
    }

    frame->SetAttribute(name, luaRef);
    return 0;
}

// VERIFY: 2a342b1f manual
// OFFSET: 0x0049F790
static int32_t Script_GetEffectiveScale(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushnumber(L, frame->m_layoutScale);
    return 1;
}

// VERIFY: 4c57c3d1 manual
// OFFSET: 0x0049F7D0
static int32_t Script_GetScale(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushnumber(L, frame->m_frameScale);
    return 1;
}

// VERIFY: adb5dccc manual
// OFFSET: 0x0049F820
static int32_t Script_SetScale(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetScale(scale)", name ? name : "<unnamed>");
        return 0;
    }

    double scale = lua_tonumber(L, 2);

    if (scale <= 0.0) {
        const char* name = frame->GetName();
        luaL_error(L, "%s:SetScale(): Scale must be > 0", name ? name : "<unnamed>");
        return 0;
    }

    frame->SetFrameScale(static_cast<float>(scale), false);
    return 0;
}

// VERIFY: 64ebae78 manual
// OFFSET: 0x0049F900
static int32_t Script_GetEffectiveAlpha(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    uint8_t alpha = frame->m_alpha * frame->m_parentAlpha / 255;
    lua_pushnumber(L, alpha / 255.0);
    return 1;
}

// VERIFY: a54be776 manual
// OFFSET: 0x0049F980
static int32_t Script_GetAlpha(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushnumber(L, frame->m_alpha / 255.0);
    return 1;
}

// VERIFY: 01f637f0 manual
// OFFSET: 0x0049F9E0
static int32_t Script_SetAlpha(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetAlpha(alpha 0 to 1)", name ? name : "<unnamed>");
        return 0;
    }

    float alpha = lua_tonumber(L, 2);

    if (alpha < 0.0f) {
        alpha = 0.0f;
    } else if (alpha >= 1.0f) {
        alpha = 1.0f;
    }

    frame->SetFrameAlpha(static_cast<uint8_t>(alpha * 255.0f));
    return 0;
}

// VERIFY: 0dd1fea4 manual
// OFFSET: 0x0049FAB0
static int32_t Script_GetID(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    lua_pushnumber(L, frame->m_id);
    return 1;
}

// VERIFY: f0877b88 manual
// OFFSET: 0x0049FB00
static int32_t Script_SetID(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (!lua_isnumber(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetID(ID)", name ? name : "<unnamed>");
        return 0;
    }

    frame->m_id = static_cast<int32_t>(lua_tonumber(L, 2));
    return 0;
}

// VERIFY: 8a7deb8f auto
// OFFSET: 0x0049FBB0
static int32_t Script_SetToplevel(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    frame->SetFrameFlag(0x1, StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: 985841a3 auto
// OFFSET: 0x0049FC30
static int32_t Script_IsToplevel(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 105092bf auto
// OFFSET: 0x0049FC90
static int32_t Script_EnableDrawLayer(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    uint32_t drawLayer = 2;

    if (lua_isstring(L, 2)) {
        StringToDrawLayer(lua_tolstring(L, 2, nullptr), drawLayer);
    }

    frame->EnableDrawLayer(drawLayer);
    return 0;
}

// VERIFY: b06b8e8b auto
// OFFSET: 0x0049FD00
static int32_t Script_DisableDrawLayer(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    uint32_t drawLayer = 2;

    if (lua_isstring(L, 2)) {
        StringToDrawLayer(lua_tolstring(L, 2, nullptr), drawLayer);
    }

    frame->DisableDrawLayer(drawLayer);
    return 0;
}

// VERIFY: f7adca17 auto
// OFFSET: 0x0049FD70
static int32_t Script_Show(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    frame->Show();
    return 0;
}

// VERIFY: cd733f36 auto
// OFFSET: 0x0049FDD0
static int32_t Script_Hide(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    frame->Hide();
    return 0;
}

// VERIFY: 6895f0d3 auto
// OFFSET: 0x0049FE30
static int32_t Script_IsVisible(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_isVisible) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1c358ce5 auto
// OFFSET: 0x0049FE90
static int32_t Script_IsShown(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_isShown) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1266fd3b auto
// OFFSET: 0x0049FEF0
static int32_t Script_Raise(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    frame->Raise();
    return 0;
}

// VERIFY: e0dae823 auto
// OFFSET: 0x0049FF50
static int32_t Script_Lower(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    frame->Lower();
    return 0;
}

// VERIFY: be552d32 auto
// OFFSET: 0x0049FFB0
static int32_t Script_GetHitRectInsets(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    float left;
    float right;
    float top;
    float bottom;
    frame->GetHitRectInsets(left, right, top, bottom);

    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * left));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * right));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * top));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bottom));

    return 4;
}

// VERIFY: 978a6468 auto
// OFFSET: 0x004A0090
static int32_t Script_SetHitRectInsets(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetHitRectInsets(left, right, top, bottom)", name ? name : "<unnamed>");
        return 0;
    }

    float left = lua_tonumber(L, 2);
    left = NDCToDDCWidth(left / (CoordinateGetAspectCompensation() * 1024.0f));
    float right = lua_tonumber(L, 3);
    right = NDCToDDCWidth(right / (CoordinateGetAspectCompensation() * 1024.0f));
    float top = lua_tonumber(L, 4);
    top = NDCToDDCWidth(top / (CoordinateGetAspectCompensation() * 1024.0f));
    float bottom = lua_tonumber(L, 5);
    bottom = NDCToDDCWidth(bottom / (CoordinateGetAspectCompensation() * 1024.0f));

    frame->SetHitRectInsets(left, right, top, bottom);
    return 0;
}

// VERIFY: 16d0f612 auto
// OFFSET: 0x004A0230
static int32_t Script_GetClampRectInsets(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    float left;
    float right;
    float top;
    float bottom;
    frame->GetClampRectInsets(left, right, top, bottom);

    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * left));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * right));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * top));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bottom));

    return 4;
}

// VERIFY: b63fefda auto
// OFFSET: 0x004A0310
static int32_t Script_SetClampRectInsets(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 5)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetClampRectInsets(left, right, top, bottom)", name ? name : "<unnamed>");
        return 0;
    }

    float left = lua_tonumber(L, 2);
    left = NDCToDDCWidth(left / (CoordinateGetAspectCompensation() * 1024.0f));
    float right = lua_tonumber(L, 3);
    right = NDCToDDCWidth(right / (CoordinateGetAspectCompensation() * 1024.0f));
    float top = lua_tonumber(L, 4);
    top = NDCToDDCWidth(top / (CoordinateGetAspectCompensation() * 1024.0f));
    float bottom = lua_tonumber(L, 5);
    bottom = NDCToDDCWidth(bottom / (CoordinateGetAspectCompensation() * 1024.0f));

    frame->SetClampRectInsets(left, right, top, bottom);
    return 0;
}

// VERIFY: 1412bfc6 manual
// OFFSET: 0x004A0480
static int32_t Script_GetMinResize(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CRect bounds = frame->m_resizeRect;

    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bounds.minX));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bounds.minY));

    return 2;
}

// VERIFY: cb5dfa41 auto
// OFFSET: 0x004A0520
static int32_t Script_SetMinResize(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CRect bounds = frame->m_resizeRect;

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetMinResize(minWidth, minHeight)", name ? name : "<unnamed>");
        return 0;
    }

    float width = lua_tonumber(L, 2);
    bounds.minX = NDCToDDCWidth(width / (CoordinateGetAspectCompensation() * 1024.0f));
    float height = lua_tonumber(L, 3);
    bounds.minY = NDCToDDCWidth(height / (CoordinateGetAspectCompensation() * 1024.0f));

    frame->m_resizeRect = bounds;
    return 0;
}

// VERIFY: a538f7cc manual
// OFFSET: 0x004A0640
static int32_t Script_GetMaxResize(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CRect bounds = frame->m_resizeRect;

    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bounds.maxX));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bounds.maxY));

    return 2;
}

// VERIFY: b6a4680e auto
// OFFSET: 0x004A06E0
static int32_t Script_SetMaxResize(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CRect bounds = frame->m_resizeRect;

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetMaxResize(maxWidth, maxHeight)", name ? name : "<unnamed>");
        return 0;
    }

    float width = lua_tonumber(L, 2);
    bounds.maxX = NDCToDDCWidth(width / (CoordinateGetAspectCompensation() * 1024.0f));
    float height = lua_tonumber(L, 3);
    bounds.maxY = NDCToDDCWidth(height / (CoordinateGetAspectCompensation() * 1024.0f));

    frame->m_resizeRect = bounds;
    return 0;
}

// VERIFY: 917ab7d3 manual
// OFFSET: 0x004A0800
static int32_t Script_SetMovable(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->SetFrameFlag(0x100, StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: dd01c476 auto
// OFFSET: 0x004A0850
static int32_t Script_IsMovable(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x100) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a268a2c0 manual
// OFFSET: 0x004A08C0
static int32_t Script_SetDontSavePosition(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->SetFrameFlag(0x80000, StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: 5284d4fe auto
// OFFSET: 0x004A0910
static int32_t Script_GetDontSavePosition(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x80000) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5d104c1c manual
// OFFSET: 0x004A0980
static int32_t Script_SetResizable(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->SetFrameFlag(0x200, StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: 7bdfa91f auto
// OFFSET: 0x004A09D0
static int32_t Script_IsResizable(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x200) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9cf30e0a auto
// OFFSET: 0x004A0A40
static int32_t Script_StartMoving(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!(frame->m_frameFlags & 0x100)) {
        const char* name = frame->GetName();
        luaL_error(L, "Frame %s is not movable", name ? name : "<unnamed>");
        return 0;
    }

    CSimpleTop* top = frame->m_simpleTop;

    if (!top->m_moveResizeFrame) {
        C2Vector position = frame->m_mousePosition;

        if (0.0f == position.x && 0.0f == position.y) {
            top->GetCursorPosition(position);
        }

        top->StartMoveOrResizeFrame(frame, MOVERESIZE_REASON_SCRIPT, position.x, position.y, FRAMEPOINT_CENTER);
    }

    return 0;
}

// VERIFY: 37d00c11 auto
// OFFSET: 0x004A0B10
static int32_t Script_StartSizing(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!(frame->m_frameFlags & 0x200)) {
        const char* name = frame->GetName();
        luaL_error(L, "Frame %s is not resizable", name ? name : "<unnamed>");
        return 0;
    }

    FRAMEPOINT point = FRAMEPOINT_BOTTOMRIGHT;

    if (lua_isstring(L, 2)) {
        StringToFramePoint(lua_tolstring(L, 2, nullptr), point);
    }

    CSimpleTop* top = frame->m_simpleTop;

    if (!top->m_moveResizeFrame) {
        C2Vector position = frame->m_mousePosition;

        if (0.0f == position.x && 0.0f == position.y) {
            top->GetCursorPosition(position);
        }

        top->StartMoveOrResizeFrame(frame, MOVERESIZE_REASON_SCRIPT, position.x, position.y, point);
    }

    return 0;
}

// VERIFY: 371dc747 manual
// OFFSET: 0x004A0C20
static int32_t Script_StopMovingOrSizing(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CSimpleTop* top = frame->m_simpleTop;

    if (top->m_moveResizeFrame == frame) {
        top->StopMoveOrResizeFrame();
    }

    return 0;
}

// VERIFY: 3ea1c472 auto
// OFFSET: 0x004A0C70
static int32_t Script_SetUserPlaced(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!(frame->m_frameFlags & 0x300)) {
        const char* name = frame->GetName();
        luaL_error(L, "Frame %s is not movable or resizable", name ? name : "<unnamed>");
        return 0;
    }

    frame->SetFrameFlag(0x1000, StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: 91daf374 auto
// OFFSET: 0x004A0D00
static int32_t Script_IsUserPlaced(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x1000) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 7f7fa83b manual
// OFFSET: 0x004A0D70
static int32_t Script_SetClampedToScreen(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    frame->SetClampedToScreen(StringToBOOL(L, 2, 1));
    return 0;
}

// VERIFY: 567c57a2 auto
// OFFSET: 0x004A0DC0
static int32_t Script_IsClampedToScreen(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_layoutFlags & 0x1000) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: cb95c496 auto
// OFFSET: 0x004A0E20
static int32_t Script_RegisterForDrag(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    uint32_t buttons = 0;

    for (int32_t i = 2; lua_isstring(L, i); i++) {
        buttons |= StringToMouseButton(lua_tolstring(L, i, nullptr));
    }

    frame->m_dragButtons = buttons;
    return 0;
}

// VERIFY: 9efb8fdc auto
// OFFSET: 0x004A0EC0
static int32_t Script_EnableKeyboard(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (StringToBOOL(L, 2, 1)) {
        frame->EnableEvent(SIMPLE_EVENT_KEY, -1);
        frame->EnableEvent(SIMPLE_EVENT_CHAR, -1);
    } else {
        frame->DisableEvent(SIMPLE_EVENT_KEY);
        frame->DisableEvent(SIMPLE_EVENT_CHAR);
    }

    return 0;
}

// VERIFY: 71851cc6 manual
// OFFSET: 0x004A0F60
static int32_t Script_IsKeyboardEnabled(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (((frame->m_enabledEventsBitmask >> SIMPLE_EVENT_KEY) & 1) || ((frame->m_enabledEventsBitmask >> SIMPLE_EVENT_CHAR) & 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2c085835 auto
// OFFSET: 0x004A0FD0
static int32_t Script_EnableMouse(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (StringToBOOL(L, 2, 1)) {
        frame->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
    } else {
        frame->DisableEvent(SIMPLE_EVENT_MOUSE);
    }

    return 0;
}

// VERIFY: 4b5eab9b manual
// OFFSET: 0x004A1060
static int32_t Script_IsMouseEnabled(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if ((frame->m_enabledEventsBitmask >> SIMPLE_EVENT_MOUSE) & 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: cf928975 auto
// OFFSET: 0x004A10D0
static int32_t Script_EnableMouseWheel(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (StringToBOOL(L, 2, 1)) {
        frame->EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, -1);
    } else {
        frame->DisableEvent(SIMPLE_EVENT_MOUSEWHEEL);
    }

    return 0;
}

// VERIFY: e5a1e6f9 manual
// OFFSET: 0x004A1160
static int32_t Script_IsMouseWheelEnabled(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if ((frame->m_enabledEventsBitmask >> SIMPLE_EVENT_MOUSEWHEEL) & 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 63f09962 auto
// OFFSET: 0x004A11D0
static int32_t Script_EnableJoystick(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!frame->ProtectedFunctionsAllowed()) {
        if (CSimpleTop::s_instance->m_actionBlockedCallback) {
            CSimpleTop::s_instance->m_actionBlockedCallback(frame);
        }

        return 0;
    }

    if (StringToBOOL(L, 2, 1)) {
        frame->EnableEvent(SIMPLE_EVENT_JOYSTICK, -1);
    } else {
        frame->DisableEvent(SIMPLE_EVENT_JOYSTICK);
    }

    return 0;
}

// VERIFY: ea522235 manual
// OFFSET: 0x004A1260
static int32_t Script_IsJoystickEnabled(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if ((frame->m_enabledEventsBitmask >> SIMPLE_EVENT_JOYSTICK) & 1) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b6dbd478 auto
// OFFSET: 0x004A12D0
static int32_t Script_GetBackdrop(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CBackdropGenerator* backdrop = frame->m_backdropGenerator;

    if (!backdrop) {
        return 0;
    }

    if (lua_type(L, 2) != LUA_TTABLE) {
        lua_createtable(L, 0, 0);
    }

    const char* bgFile = backdrop->m_bgFile.m_str ? backdrop->m_bgFile.m_str : "";
    const char* edgeFile = backdrop->m_edgeFile.m_str ? backdrop->m_edgeFile.m_str : "";
    uint32_t tile = backdrop->m_tile;

    lua_pushstring(L, "bgFile");
    lua_pushstring(L, bgFile);
    lua_settable(L, -3);

    lua_pushstring(L, "edgeFile");
    lua_pushstring(L, edgeFile);
    lua_settable(L, -3);

    lua_pushstring(L, "tile");

    if (tile) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_settable(L, -3);

    lua_pushstring(L, "tileSize");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * backdrop->m_tileSize));
    lua_settable(L, -3);

    lua_pushstring(L, "edgeSize");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * backdrop->m_edgeSize));
    lua_settable(L, -3);

    lua_pushstring(L, "insets");
    lua_gettable(L, -2);

    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_settop(L, -2);
        lua_pushstring(L, "insets");
        lua_createtable(L, 0, 0);
        lua_settable(L, -3);
        lua_pushstring(L, "insets");
        lua_gettable(L, -2);
    }

    float left = backdrop->m_insetLeft;
    float right = backdrop->m_insetRight;
    float top = backdrop->m_insetTop;
    float bottom = backdrop->m_insetBottom;

    lua_pushstring(L, "left");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * left));
    lua_settable(L, -3);

    lua_pushstring(L, "right");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * right));
    lua_settable(L, -3);

    lua_pushstring(L, "top");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * top));
    lua_settable(L, -3);

    lua_pushstring(L, "bottom");
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * bottom));
    lua_settable(L, -3);

    lua_settop(L, -2);
    return 1;
}

// VERIFY: 738e45f8 manual
// OFFSET: 0x004A15A0
static int32_t Script_SetBackdrop(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (lua_type(L, 2) == LUA_TNIL) {
        frame->SetBackdrop(nullptr);
        return 0;
    }

    if (lua_type(L, 2) != LUA_TTABLE) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetBackdrop(nil or {bgFile = \"bgFile\", edgeFile = \"edgeFile\", tile = false, tileSize = 0, edgeSize = 32, insets = { left = 0, right = 0, top = 0, bottom = 0 }})", name ? name : "<unnamed>");
        return 0;
    }

    void* data = SMemAlloc(sizeof(CBackdropGenerator), __FILE__, __LINE__, 0);
    CBackdropGenerator* backdrop = data ? new (data) CBackdropGenerator() : nullptr;

    lua_pushstring(L, "bgFile");
    lua_gettable(L, -2);
    const char* bgFile = lua_tolstring(L, -1, nullptr);
    lua_settop(L, -2);

    lua_pushstring(L, "edgeFile");
    lua_gettable(L, -2);
    const char* edgeFile = lua_tolstring(L, -1, nullptr);
    lua_settop(L, -2);

    lua_pushstring(L, "tile");
    lua_gettable(L, -2);
    int32_t tile = StringToBOOL(L, -1, 0);
    lua_settop(L, -2);

    uint32_t pieces = edgeFile && *edgeFile ? 0xFF : 0;
    backdrop->m_bgFile.Copy(bgFile);
    backdrop->m_pieces = pieces;
    backdrop->m_tile = tile;
    backdrop->m_edgeFile.Copy(edgeFile);
    backdrop->m_alphaMode = GxBlend_Alpha;

    lua_pushstring(L, "tileSize");
    lua_gettable(L, -2);

    if (lua_isnumber(L, -1)) {
        float tileSize = lua_tonumber(L, -1);
        backdrop->m_tileSize = NDCToDDCWidth(tileSize / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    lua_settop(L, -2);

    lua_pushstring(L, "edgeSize");
    lua_gettable(L, -2);

    if (lua_isnumber(L, -1)) {
        float edgeSize = lua_tonumber(L, -1);
        backdrop->m_edgeSize = NDCToDDCWidth(edgeSize / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    lua_settop(L, -2);

    lua_pushstring(L, "insets");
    lua_gettable(L, -2);

    if (lua_type(L, -1) == LUA_TTABLE) {
        float left = 0.0f;
        float right = 0.0f;
        float top = 0.0f;
        float bottom = 0.0f;

        lua_pushstring(L, "left");
        lua_gettable(L, -2);

        if (lua_isnumber(L, -1)) {
            left = lua_tonumber(L, -1);
            left = NDCToDDCWidth(left / (CoordinateGetAspectCompensation() * 1024.0f));
        }

        lua_settop(L, -2);

        lua_pushstring(L, "right");
        lua_gettable(L, -2);

        if (lua_isnumber(L, -1)) {
            right = lua_tonumber(L, -1);
            right = NDCToDDCWidth(right / (CoordinateGetAspectCompensation() * 1024.0f));
        }

        lua_settop(L, -2);

        lua_pushstring(L, "top");
        lua_gettable(L, -2);

        if (lua_isnumber(L, -1)) {
            top = lua_tonumber(L, -1);
            top = NDCToDDCWidth(top / (CoordinateGetAspectCompensation() * 1024.0f));
        }

        lua_settop(L, -2);

        lua_pushstring(L, "bottom");
        lua_gettable(L, -2);

        if (lua_isnumber(L, -1)) {
            bottom = lua_tonumber(L, -1);
            bottom = NDCToDDCWidth(bottom / (CoordinateGetAspectCompensation() * 1024.0f));
        }

        lua_settop(L, -2);

        backdrop->m_insetLeft = left;
        backdrop->m_insetRight = right;
        backdrop->m_insetTop = top;
        backdrop->m_insetBottom = bottom;
    }

    lua_settop(L, -2);

    frame->SetBackdrop(backdrop);

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (frame->GetRect(&rect)) {
        backdrop->Generate(&rect);
    }

    return 0;
}

// VERIFY: 7c7423df manual
// OFFSET: 0x004A19A0
static int32_t Script_GetBackdropColor(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CBackdropGenerator* backdrop = frame->m_backdropGenerator;

    if (!backdrop) {
        return 0;
    }

    CImVector color = backdrop->m_backgroundColor;
    lua_pushnumber(L, color.r * 0.00392156f);
    lua_pushnumber(L, color.g * 0.00392156f);
    lua_pushnumber(L, color.b * 0.00392156f);
    lua_pushnumber(L, color.a * 0.00392156f);

    return 4;
}

// VERIFY: 6a2539aa auto
// OFFSET: 0x004A1A80
static int32_t Script_SetBackdropColor(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CImVector color;

    FrameScript_GetColor(L, 2, color);

    if (frame->m_backdropGenerator) {
        frame->m_backdropGenerator->SetVertexColor(color);
    }

    return 0;
}

// VERIFY: 30dbb765 manual
// OFFSET: 0x004A1AF0
static int32_t Script_GetBackdropBorderColor(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CBackdropGenerator* backdrop = frame->m_backdropGenerator;

    if (!backdrop) {
        return 0;
    }

    CImVector color = backdrop->m_borderColor;
    lua_pushnumber(L, color.r * 0.00392156f);
    lua_pushnumber(L, color.g * 0.00392156f);
    lua_pushnumber(L, color.b * 0.00392156f);
    lua_pushnumber(L, color.a * 0.00392156f);

    return 4;
}

// VERIFY: c0bee394 auto
// OFFSET: 0x004A1BD0
static int32_t Script_SetBackdropBorderColor(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));
    CImVector color;

    FrameScript_GetColor(L, 2, color);

    if (frame->m_backdropGenerator) {
        frame->m_backdropGenerator->SetBorderVertexColor(color);
    }

    return 0;
}

// VERIFY: 0177d245 manual
// OFFSET: 0x004A1C40
static int32_t Script_SetDepth(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:SetDepth(additiveDepth)", name ? name : "<unnamed>");
        return 0;
    }

    frame->SetFrameDepth(static_cast<float>(lua_tonumber(L, 2)), false);
    return 0;
}

// VERIFY: c5dd7db9 manual
// OFFSET: 0x004A1CC0
static int32_t Script_GetDepth(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x40000) {
        return 0;
    }

    lua_pushnumber(L, frame->m_depth);
    return 1;
}

// VERIFY: 1ad27240 manual
// OFFSET: 0x004A1D20
static int32_t Script_GetEffectiveDepth(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_frameFlags & 0x40000) {
        return 0;
    }

    lua_pushnumber(L, frame->m_parentDepth + frame->m_depth);
    return 1;
}

// VERIFY: 63402433 manual
// OFFSET: 0x004A1D80
static int32_t Script_IgnoreDepth(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (lua_type(L, 2) != LUA_TBOOLEAN) {
        const char* name = frame->GetName();
        luaL_error(L, "Usage: %s:IgnoreDepth(ignore)", name ? name : "<unnamed>");
        return 0;
    }

    frame->m_ignoreDepth = lua_toboolean(L, 2);
    return 0;
}

// VERIFY: 721e2faf auto
// OFFSET: 0x004A1E00
static int32_t Script_IsIgnoringDepth(lua_State* L) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(FrameScript_GetObjectThis(L, CSimpleFrame::GetObjectType()));

    if (frame->m_ignoreDepth) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e54f54a8 auto
// OFFSET: 0x004A1E60
void CSimpleFrame::CreateScriptMetaTable() {
    CSimpleFrame::s_metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), &CSimpleFrame::RegisterScriptMethods);
}

// VERIFY: d0ab2718 auto
// OFFSET: 0x0049E560
void CSimpleFrame::DestroyScriptMetaTable() {
    luaL_unref(FrameScript_GetContext(), LUA_REGISTRYINDEX, CSimpleFrame::s_metatable);
    CSimpleFrame::s_metatable = LUA_NOREF;
}

// STATIC: 0x00AC1550
static FrameScript_Method s_ScriptMethods[] = {
    { "GetTitleRegion", &Script_GetTitleRegion },
    { "CreateTitleRegion", &Script_CreateTitleRegion },
    { "CreateTexture", &Script_CreateTexture },
    { "CreateFontString", &Script_CreateFontString },
    { "GetBoundsRect", &Script_GetBoundsRect },
    { "GetNumRegions", &Script_GetNumRegions },
    { "GetRegions", &Script_GetRegions },
    { "GetNumChildren", &Script_GetNumChildren },
    { "GetChildren", &Script_GetChildren },
    { "GetFrameStrata", &Script_GetFrameStrata },
    { "SetFrameStrata", &Script_SetFrameStrata },
    { "GetFrameLevel", &Script_GetFrameLevel },
    { "SetFrameLevel", &Script_SetFrameLevel },
    { "HasScript", &Script_HasScript },
    { "GetScript", &Script_GetScript },
    { "SetScript", &Script_SetScript },
    { "HookScript", &Script_HookScript },
    { "RegisterEvent", &Script_RegisterEvent },
    { "UnregisterEvent", &Script_UnregisterEvent },
    { "RegisterAllEvents", &Script_RegisterAllEvents },
    { "UnregisterAllEvents", &Script_UnregisterAllEvents },
    { "IsEventRegistered", &Script_IsEventRegistered },
    { "AllowAttributeChanges", &Script_AllowAttributeChanges },
    { "CanChangeAttribute", &Script_CanChangeAttribute },
    { "GetAttribute", &Script_GetAttribute },
    { "SetAttribute", &Script_SetAttribute },
    { "GetEffectiveScale", &Script_GetEffectiveScale },
    { "GetScale", &Script_GetScale },
    { "SetScale", &Script_SetScale },
    { "GetEffectiveAlpha", &Script_GetEffectiveAlpha },
    { "GetAlpha", &Script_GetAlpha },
    { "SetAlpha", &Script_SetAlpha },
    { "GetID", &Script_GetID },
    { "SetID", &Script_SetID },
    { "SetToplevel", &Script_SetToplevel },
    { "IsToplevel", &Script_IsToplevel },
    { "EnableDrawLayer", &Script_EnableDrawLayer },
    { "DisableDrawLayer", &Script_DisableDrawLayer },
    { "Show", &Script_Show },
    { "Hide", &Script_Hide },
    { "IsVisible", &Script_IsVisible },
    { "IsShown", &Script_IsShown },
    { "Raise", &Script_Raise },
    { "Lower", &Script_Lower },
    { "GetHitRectInsets", &Script_GetHitRectInsets },
    { "SetHitRectInsets", &Script_SetHitRectInsets },
    { "GetClampRectInsets", &Script_GetClampRectInsets },
    { "SetClampRectInsets", &Script_SetClampRectInsets },
    { "GetMinResize", &Script_GetMinResize },
    { "SetMinResize", &Script_SetMinResize },
    { "GetMaxResize", &Script_GetMaxResize },
    { "SetMaxResize", &Script_SetMaxResize },
    { "SetMovable", &Script_SetMovable },
    { "IsMovable", &Script_IsMovable },
    { "SetDontSavePosition", &Script_SetDontSavePosition },
    { "GetDontSavePosition", &Script_GetDontSavePosition },
    { "SetResizable", &Script_SetResizable },
    { "IsResizable", &Script_IsResizable },
    { "StartMoving", &Script_StartMoving },
    { "StartSizing", &Script_StartSizing },
    { "StopMovingOrSizing", &Script_StopMovingOrSizing },
    { "SetUserPlaced", &Script_SetUserPlaced },
    { "IsUserPlaced", &Script_IsUserPlaced },
    { "SetClampedToScreen", &Script_SetClampedToScreen },
    { "IsClampedToScreen", &Script_IsClampedToScreen },
    { "RegisterForDrag", &Script_RegisterForDrag },
    { "EnableKeyboard", &Script_EnableKeyboard },
    { "IsKeyboardEnabled", &Script_IsKeyboardEnabled },
    { "EnableMouse", &Script_EnableMouse },
    { "IsMouseEnabled", &Script_IsMouseEnabled },
    { "EnableMouseWheel", &Script_EnableMouseWheel },
    { "IsMouseWheelEnabled", &Script_IsMouseWheelEnabled },
    { "EnableJoystick", &Script_EnableJoystick },
    { "IsJoystickEnabled", &Script_IsJoystickEnabled },
    { "GetBackdrop", &Script_GetBackdrop },
    { "SetBackdrop", &Script_SetBackdrop },
    { "GetBackdropColor", &Script_GetBackdropColor },
    { "SetBackdropColor", &Script_SetBackdropColor },
    { "GetBackdropBorderColor", &Script_GetBackdropBorderColor },
    { "SetBackdropBorderColor", &Script_SetBackdropBorderColor },
    { "SetDepth", &Script_SetDepth },
    { "GetDepth", &Script_GetDepth },
    { "GetEffectiveDepth", &Script_GetEffectiveDepth },
    { "IgnoreDepth", &Script_IgnoreDepth },
    { "IsIgnoringDepth", &Script_IsIgnoringDepth },
};

// VERIFY: c7ad299d auto
// OFFSET: 0x0049E540
void CSimpleFrame::RegisterScriptMethods(lua_State* L) {
    CScriptRegion::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_ScriptMethods, sizeof(s_ScriptMethods) / sizeof(s_ScriptMethods[0]));
}
