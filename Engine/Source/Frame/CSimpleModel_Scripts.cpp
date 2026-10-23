#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Model2/Types.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00B2CBD0
int32_t CSimpleModel::s_metatable = LUA_NOREF;

// VERIFY: 454a72eb auto
// OFFSET: 0x009603F0
void CSimpleModel::DestroyScriptMetaTable() {
    luaL_unref(FrameScript_GetContext(), LUA_REGISTRYINDEX, CSimpleModel::s_metatable);
    CSimpleModel::s_metatable = LUA_NOREF;
}

// VERIFY: 1d94299e manual
// OFFSET: 0x00960420
int32_t FrameScript_GetColor(lua_State* L, int32_t index, CImVector& color) {
    float r = lua_tonumber(L, index);
    r = r < 0.0f ? 0.0f : (r >= 1.0f ? 1.0f : r);
    index++;

    float g = lua_tonumber(L, index);
    g = g < 0.0f ? 0.0f : (g >= 1.0f ? 1.0f : g);
    index++;

    float b = lua_tonumber(L, index);
    b = b < 0.0f ? 0.0f : (b >= 1.0f ? 1.0f : b);
    index++;

    float a;

    if (lua_isnumber(L, index)) {
        a = lua_tonumber(L, index);
        a = a < 0.0f ? 0.0f : (a >= 1.0f ? 1.0f : a);
        index++;
    } else {
        a = 1.0f;
    }

    color.Set(a, r, g, b);
    return index;
}

// VERIFY: 51dc23ed manual
// OFFSET: 0x0061BE50
int32_t FrameScript_GetColorNoAlpha(lua_State* L, int32_t index, CImVector& color) {
    float r = lua_tonumber(L, index);
    r = r < 0.0f ? 0.0f : (r >= 1.0f ? 1.0f : r);
    index++;

    float g = lua_tonumber(L, index);
    g = g < 0.0f ? 0.0f : (g >= 1.0f ? 1.0f : g);
    index++;

    float b = lua_tonumber(L, index);
    b = b < 0.0f ? 0.0f : (b >= 1.0f ? 1.0f : b);
    index++;

    color.Set(1.0f, r, g, b);
    return index;
}

// VERIFY: e336fe44 auto
// OFFSET: 0x00960530
static int32_t Script_SetModel(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isstring(L, 2)) {
        const char* fileName = lua_tolstring(L, 2, nullptr);
        model->SetModel(fileName);

        if (!model->m_model) {
            luaL_error(L, "Invalid model file: %s", fileName);
            return 0;
        }

        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetModel(\"file\")", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: ae4a40ed manual
// OFFSET: 0x009605D0
static int32_t Script_GetModel(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (model->m_model) {
        lua_pushstring(L, model->m_model->m_shared->m_fileName);
    }

    return 1;
}

// VERIFY: 02a372b2 manual
// OFFSET: 0x00960620
static int32_t Script_ClearModel(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    model->SetModel(static_cast<CM2Model*>(nullptr));
    return 0;
}

// VERIFY: 5cd4123f manual
// OFFSET: 0x00960660
static int32_t Script_SetPosition(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));
    float x = lua_tonumber(L, 2);
    float y = lua_tonumber(L, 3);
    float z = lua_tonumber(L, 4);

    model->m_position.x = x;
    model->m_position.y = y;
    model->m_position.z = z;

    return 0;
}

// VERIFY: 65d26da8 auto
// OFFSET: 0x009606E0
static int32_t Script_SetFacing(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2)) {
        model->m_facing = lua_tonumber(L, 2);
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetFacing(facing)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: 6e31f619 auto
// OFFSET: 0x00960760
static int32_t Script_SetModelScale(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2)) {
        model->m_modelScale = lua_tonumber(L, 2);
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetScale(scale)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: ffa3052c manual
// OFFSET: 0x009607E0
static int32_t Script_SetSequence(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = model->GetName();
        luaL_error(L, "Usage: %s:SetSequence(sequence)", name ? name : "<unnamed>");
        return 0;
    }

    uint32_t sequence = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (sequence >= 506) {
        const char* name = model->GetName();
        luaL_error(L, "Error: %s:SetSequence(sequence) exceeds valid range of 0 - %d", name ? name : "<unnamed>", 506);
        return 0;
    }

    model->SetSequence(sequence);
    return 0;
}

// VERIFY: 7bf0ef7a manual
// OFFSET: 0x009608B0
static int32_t Script_SetSequenceTime(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        int32_t time = static_cast<int32_t>(lua_tonumber(L, 3));
        uint32_t sequence = static_cast<uint32_t>(lua_tonumber(L, 2));
        model->SetSequenceTime(sequence, time);
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetSequenceTime(sequence, time)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: e4a5936c manual
// OFFSET: 0x00960970
static int32_t Script_SetCamera(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2)) {
        model->SetCameraByIndex(static_cast<uint32_t>(lua_tonumber(L, 2)));
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetCamera(index)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: 3a823233 manual
// OFFSET: 0x00960A10
int32_t CSimpleModel::SetLightHelper(lua_State* L, int32_t index, CM2Light* light) {
    if (!lua_isnumber(L, index)) {
        return 0;
    }

    light->SetVisible(lua_tonumber(L, index) != 0.0);

    if (!light->m_visible) {
        return 1;
    }

    if (!lua_isnumber(L, index + 1) || !lua_isnumber(L, index + 2) || !lua_isnumber(L, index + 3) || !lua_isnumber(L, index + 4) || !lua_isnumber(L, index + 5)) {
        return 0;
    }

    light->SetLightType(static_cast<M2LIGHTTYPE>(lua_tonumber(L, index + 1) != 0.0));

    if (light->m_type == M2LIGHT_POINT) {
        float x = lua_tonumber(L, index + 2);
        float y = lua_tonumber(L, index + 3);
        float z = lua_tonumber(L, index + 4);
        light->SetPosition(C3Vector(x, y, z));
    } else {
        float x = lua_tonumber(L, index + 2);
        float y = lua_tonumber(L, index + 3);
        float z = lua_tonumber(L, index + 4);
        light->SetDirection(C3Vector(x, y, z));
    }

    float ambIntensity = lua_tonumber(L, index + 5);
    int32_t next = index + 6;

    CImVector ambColor;
    ambColor.b = 0xFF;
    ambColor.g = 0xFF;
    ambColor.r = 0xFF;

    if (fnotequal(ambIntensity, 0.0f) && lua_isnumber(L, next) && lua_isnumber(L, next + 1) && lua_isnumber(L, next + 2)) {
        FrameScript_GetColorNoAlpha(L, next, ambColor);
        next += 3;
    }

    light->m_ambColor.x = ambColor.r * 0.0039215689f * ambIntensity;
    light->m_ambColor.y = ambColor.g * 0.0039215689f * ambIntensity;
    light->m_ambColor.z = ambColor.b * 0.0039215689f * ambIntensity;

    if (lua_isnumber(L, next)) {
        float dirIntensity = lua_tonumber(L, next);
        next++;

        CImVector dirColor;
        dirColor.b = 0xFF;
        dirColor.g = 0xFF;
        dirColor.r = 0xFF;

        if (fnotequal(dirIntensity, 0.0f) && lua_isnumber(L, next) && lua_isnumber(L, next + 1) && lua_isnumber(L, next + 2)) {
            FrameScript_GetColorNoAlpha(L, next, dirColor);
        }

        light->m_dirColor.x = dirColor.r * 0.0039215689f * dirIntensity;
        light->m_dirColor.y = dirColor.g * 0.0039215689f * dirIntensity;
        light->m_dirColor.z = dirColor.b * 0.0039215689f * dirIntensity;
    }

    return 1;
}

// VERIFY: f6c994a8 auto
// OFFSET: 0x00960D20
static int32_t Script_SetLight(lua_State* L) {
    const char* name = "model";
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (model) {
        CM2Light light;

        if (CSimpleModel::SetLightHelper(L, 2, &light)) {
            model->SetLight(light);
            return 0;
        }

        name = model->GetName();

        if (!name) {
            name = "<unnamed>";
        }
    }

    luaL_error(L, "Usage: %s:SetLight(enabled[, omni, dirX, dirY, dirZ, ambIntensity[, ambR, ambG, ambB], dirIntensity[, dirR, dirG, dirB]])", name);
    return 0;
}

// VERIFY: 2fc58ab2 manual
// OFFSET: 0x00960DD0
static int32_t Script_GetLight(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));
    CM2Light* light = &model->m_light;

    lua_checkstack(L, 13);
    lua_pushnumber(L, light->m_visible);

    float x;
    float y;
    float z;

    if (light->m_type == M2LIGHT_POINT) {
        lua_pushnumber(L, 1.0);
        x = light->m_pos.x;
        y = light->m_pos.y;
        z = light->m_pos.z;
    } else {
        lua_pushnumber(L, 0.0);
        x = light->m_dir.x;
        y = light->m_dir.y;
        z = light->m_dir.z;
    }

    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);

    int32_t count;
    float ambR = light->m_ambColor.x;
    float ambG = light->m_ambColor.y;
    float ambB = light->m_ambColor.z;

    if (ambR > 0.0f || ambG > 0.0f || ambB > 0.0f) {
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, ambR);
        lua_pushnumber(L, ambG);
        lua_pushnumber(L, ambB);
        count = 9;
    } else {
        lua_pushnumber(L, 0.0);
        count = 6;
    }

    float dirR = light->m_dirColor.x;
    float dirG = light->m_dirColor.y;
    float dirB = light->m_dirColor.z;

    if (dirR > 0.0f || dirG > 0.0f || dirB > 0.0f) {
        lua_pushnumber(L, 1.0);
        lua_pushnumber(L, dirR);
        lua_pushnumber(L, dirG);
        lua_pushnumber(L, dirB);
        return count + 4;
    }

    lua_pushnumber(L, 0.0);
    return count + 1;
}

// VERIFY: 977eb3ba manual
// OFFSET: 0x00960FC0
static int32_t Script_GetPosition(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));
    float y = model->m_position.y;
    float z = model->m_position.z;

    lua_pushnumber(L, model->m_position.x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);

    return 3;
}

// VERIFY: dce02567 manual
// OFFSET: 0x00961040
static int32_t Script_GetFacing(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    lua_pushnumber(L, model->m_facing);
    return 1;
}

// VERIFY: a77a5170 manual
// OFFSET: 0x00961090
static int32_t Script_GetModelScale(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    lua_pushnumber(L, model->m_modelScale);
    return 1;
}

// VERIFY: d55ea3ef manual
// OFFSET: 0x009610E0
static int32_t Script_AdvanceTime(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    model->AdvanceTime();
    return 0;
}

// VERIFY: ff5597e0 auto
// OFFSET: 0x00961120
static int32_t Script_ReplaceIconTexture(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isstring(L, 2)) {
        model->ReplaceTexture(M2TEX_ITEM_ICON, lua_tolstring(L, 2, nullptr));
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:ReplaceIconTexture(\"texture\")", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: 20d26a7b manual
// OFFSET: 0x009611A0
static int32_t Script_SetFogColor(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));
    CImVector color;

    FrameScript_GetColor(L, 2, color);
    model->m_flags |= CSimpleModel::MODEL_FOGGED;
    model->m_fogColor = color;

    return 0;
}

// VERIFY: 71bef8e1 manual
// OFFSET: 0x00961200
static int32_t Script_GetFogColor(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));
    CImVector color = model->m_fogColor;

    lua_pushnumber(L, color.r * 0.00392156f);
    lua_pushnumber(L, color.g * 0.00392156f);
    lua_pushnumber(L, color.b * 0.00392156f);
    lua_pushnumber(L, color.a * 0.00392156f);

    return 4;
}

// VERIFY: ae801d20 auto
// OFFSET: 0x009612D0
static int32_t Script_SetFogNear(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2)) {
        model->m_fogNear = lua_tonumber(L, 2);
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetFogNear(value)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: 5bca5b94 manual
// OFFSET: 0x00961350
static int32_t Script_GetFogNear(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    lua_pushnumber(L, model->m_fogNear);
    return 1;
}

// VERIFY: 210e6aec auto
// OFFSET: 0x009613A0
static int32_t Script_SetFogFar(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (lua_isnumber(L, 2)) {
        model->m_fogFar = lua_tonumber(L, 2);
        return 0;
    }

    const char* name = model->GetName();
    luaL_error(L, "Usage: %s:SetFogFar(value)", name ? name : "<unnamed>");
    return 0;
}

// VERIFY: a181e0ec manual
// OFFSET: 0x00961420
static int32_t Script_GetFogFar(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    lua_pushnumber(L, model->m_fogFar);
    return 1;
}

// VERIFY: a99f07c5 manual
// OFFSET: 0x00961470
static int32_t Script_ClearFog(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    model->m_flags &= ~CSimpleModel::MODEL_FOGGED;
    return 0;
}

// VERIFY: afcbfafd manual
// OFFSET: 0x009614B0
static int32_t Script_SetGlow(lua_State* L) {
    CSimpleModel* model = static_cast<CSimpleModel*>(FrameScript_GetObjectThis(L, CSimpleModel::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = model->GetName();
        luaL_error(L, "Usage: %s:SetGlow(value)", name ? name : "<unnamed>");
        return 0;
    }

    model->SetGlow(lua_tonumber(L, 2));
    return 0;
}

// VERIFY: 7cc86a8e auto
// OFFSET: 0x00961540
void CSimpleModel::CreateScriptMetaTable() {
    CSimpleModel::s_metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), &CSimpleModel::RegisterScriptMethods);
}

// STATIC: 0x00B2CB10
static FrameScript_Method s_ScriptMethods[] = {
    { "SetModel", &Script_SetModel },
    { "GetModel", &Script_GetModel },
    { "ClearModel", &Script_ClearModel },
    { "SetPosition", &Script_SetPosition },
    { "SetFacing", &Script_SetFacing },
    { "SetModelScale", &Script_SetModelScale },
    { "SetSequence", &Script_SetSequence },
    { "SetSequenceTime", &Script_SetSequenceTime },
    { "SetCamera", &Script_SetCamera },
    { "SetLight", &Script_SetLight },
    { "GetLight", &Script_GetLight },
    { "GetPosition", &Script_GetPosition },
    { "GetFacing", &Script_GetFacing },
    { "GetModelScale", &Script_GetModelScale },
    { "AdvanceTime", &Script_AdvanceTime },
    { "ReplaceIconTexture", &Script_ReplaceIconTexture },
    { "SetFogColor", &Script_SetFogColor },
    { "GetFogColor", &Script_GetFogColor },
    { "SetFogNear", &Script_SetFogNear },
    { "GetFogNear", &Script_GetFogNear },
    { "SetFogFar", &Script_SetFogFar },
    { "GetFogFar", &Script_GetFogFar },
    { "ClearFog", &Script_ClearFog },
    { "SetGlow", &Script_SetGlow },
};

// VERIFY: 89102caf auto
// OFFSET: 0x009603D0
void CSimpleModel::RegisterScriptMethods(lua_State* L) {
    CSimpleFrame::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_ScriptMethods, sizeof(s_ScriptMethods) / sizeof(s_ScriptMethods[0]));
}
