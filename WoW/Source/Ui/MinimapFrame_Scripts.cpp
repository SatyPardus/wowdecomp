#include "MinimapFrame.h"
#include <cstdint>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Base/Handle.h"

static int32_t Script_SetMaskTexture(lua_State* L);
static int32_t Script_SetIconTexture(lua_State* L);
static int32_t Script_SetBlipTexture(lua_State* L);
static int32_t Script_SetClassBlipTexture(lua_State* L);
static int32_t Script_SetPOIArrowTexture(lua_State* L);
static int32_t Script_SetStaticPOIArrowTexture(lua_State* L);
static int32_t Script_SetCorpsePOIArrowTexture(lua_State* L);
static int32_t Script_SetPlayerTexture(lua_State* L);
static int32_t Script_SetZoom(lua_State* L);
static int32_t Script_PingLocation(lua_State* L);
static int32_t Script_GetPingPosition(lua_State* L);

// VERIFY: 756048df auto
// OFFSET: 0x0057C060
void CGMinimapFrame::DestroyScriptMetaTable() {
    luaL_unref(FrameScript_GetContext(), LUA_REGISTRYINDEX, CGMinimapFrame::s_metatable);
    CGMinimapFrame::s_metatable = LUA_NOREF;
}

// VERIFY: 6485707d auto
// OFFSET: 0x0057E340
void CGMinimapFrame::CreateScriptMetaTable() {
    CGMinimapFrame::s_metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), &CGMinimapFrame::RegisterScriptMethods);
}

// VERIFY: 36da42d2 auto
// OFFSET: 0x0057F170
static int32_t Script_GetNumTrackingTypes(lua_State* L) {
    uint32_t spells = s_trackingSpells.Count();
    lua_pushnumber(L, spells + CGMinimapFrame::CountAvailableTrackingTypes());
    return 1;
}

// VERIFY: e7934d4c auto
// OFFSET: 0x0057F4F0
static int32_t Script_GetTrackingTexture(lua_State* L) {
    char texture[260];

    if (CGMinimapFrame::s_trackingInfo) {
        SStrPrintf(texture, sizeof(texture), "%s\\%s", "Interface\\Minimap\\Tracking", CGMinimapFrame::s_trackingInfo->m_texture);
    } else {
        SpellRec spell;

        if (g_spellDB.GetRecord(CGMinimapFrame::s_trackingSpell, &spell)) {
            SpellIconRec* icon = g_spellIconDB.GetRecord(spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

            if (icon) {
                lua_pushstring(L, icon->m_textureFilename);
                return 1;
            }
        }

        SStrPrintf(texture, sizeof(texture), "%s\\%s", "Interface\\Minimap\\Tracking", "None");
    }

    lua_pushstring(L, texture);
    return 1;
}

// VERIFY: daed492b auto
// OFFSET: 0x0057E1C0
static int32_t Script_SetPlayerTextureHeight(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetPlayerTextureHeight(height)", name ? name : "<unnamed>");
        return 0;
    }

    float height = lua_tonumber(L, 2);
    height = NDCToDDCWidth(height / (CoordinateGetAspectCompensation() * 1024.0f));

    if (minimap->m_playerTexture) {
        minimap->m_playerTexture->SetHeight(height);
    }

    return 0;
}

// VERIFY: e965c33d auto
// OFFSET: 0x0057E280
static int32_t Script_SetPlayerTextureWidth(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetPlayerTextureWidth(Width)", name ? name : "<unnamed>");
        return 0;
    }

    float width = lua_tonumber(L, 2);
    width = NDCToDDCWidth(width / (CoordinateGetAspectCompensation() * 1024.0f));

    if (minimap->m_playerTexture) {
        minimap->m_playerTexture->SetWidth(width);
    }

    return 0;
}

// VERIFY: c8962cb4 manual
// OFFSET: 0x0057F1B0
static int32_t Script_GetTrackingInfo(lua_State* L) {
    int32_t index = static_cast<int32_t>(static_cast<int64_t>(lua_tonumber(L, 1))) - 1;

    if (index >= static_cast<int32_t>(s_trackingSpells.Count())) {
        const TrackingTypeInfo* info = CGMinimapFrame::GetTrackingTypeByIndex(index - s_trackingSpells.Count());

        if (!info) {
            return 0;
        }

        lua_pushstring(L, FrameScript_GetText(info->m_name, -1, GENDER_NOT_APPLICABLE));
        char texture[128];
        SStrPrintf(texture, sizeof(texture), "%s\\%s", "Interface\\Minimap\\Tracking", info->m_texture);
        lua_pushstring(L, texture);

        if (CGMinimapFrame::s_trackingInfo == info) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        lua_pushstring(L, "other");
        return 4;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(s_trackingSpells.m_data[index], &spell)) {
        return 0;
    }

    bool active = CGMinimapFrame::s_trackingSpell == spell.m_id;
    lua_pushstring(L, spell.m_name);
    SpellIconRec* icon = g_spellIconDB.GetRecord(active && spell.m_activeIconID ? spell.m_activeIconID : spell.m_spellIconID);

    if (icon) {
        lua_pushstring(L, icon->m_textureFilename);
    } else {
        lua_pushnil(L);
    }

    if (active) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, "spell");
    return 4;
}

// VERIFY: e417bf52 manual
// OFFSET: 0x0057BF50
static int32_t Script_GetZoomLevels(lua_State* L) {
    lua_pushnumber(L, static_cast<uint32_t>(MinimapGetZoomLevels()));
    return 1;
}

// VERIFY: de4c09b0 manual
// OFFSET: 0x0057BF90
static int32_t Script_GetZoom(lua_State* L) {
    lua_pushnumber(L, static_cast<uint32_t>(MinimapGetZoom()));
    return 1;
}

// VERIFY: e16a0b62 manual
// OFFSET: 0x0057BFD0
static int32_t Script_SetZoom(lua_State* L) {
    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: SetZoom(level)");
        return 0;
    }

    MinimapSetZoom(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 2))));
    return 0;
}

// VERIFY: 3937ee2d manual
// OFFSET: 0x0057E100
static int32_t Script_SetPlayerTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetPlayerTexture(\"file\")", name ? name : "<unnamed>");
    }

    if (!minimap->m_playerTexture->SetTexture(lua_tolstring(L, 2, nullptr), 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetPlayerTexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: cf4a0724 manual
// OFFSET: 0x0057ED70
static int32_t Script_PingLocation(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    float x = 0.0f;
    float y = 0.0f;
    float radius = MinimapGetViewRadius();
    float diameter = radius + radius;

    if (lua_isnumber(L, 2) && lua_isnumber(L, 3)) {
        CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));
        y = lua_tonumber(L, 2);
        y = -NDCToDDCWidth(y / (CoordinateGetAspectCompensation() * 1024.0f));
        x = lua_tonumber(L, 3);
        x = NDCToDDCWidth(x / (CoordinateGetAspectCompensation() * 1024.0f));
        x = x / minimap->GetWidth() * diameter;
        y = y / minimap->GetHeight() * diameter;

        if (g_rotateMinimapCVar->m_intValue) {
            C44Matrix matrix;
            matrix *= C44Matrix::Rotation(CGMinimapFrame::s_facing, C3Vector(0.0f, 0.0f, 1.0f), true);
            C3Vector rotated = TransformDirection(C3Vector(x, y, 0.0f), matrix);
            x = rotated.x;
            y = rotated.y;
        }
    }

    x = player->GetPosition().x + x;
    y = player->GetPosition().y + y;

    if (CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(MSG_MINIMAP_PING));
        msg.Put(x);
        msg.Put(y);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    WOWGUID guid = ClntObjMgrGetActivePlayer();
    C2Vector position;
    position.x = x;
    position.y = y;
    CGMinimapFrame::SetPingPosition(guid, position);
    return 0;
}

// VERIFY: fa6140d8 manual
// OFFSET: 0x0057EFE0
static int32_t Script_GetPingPosition(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        return 2;
    }

    float radius = MinimapGetViewRadius();
    float diameter = radius + radius;
    C2Vector position(player->GetPosition());
    float x = CGMinimapFrame::s_pingPosition.x - position.x;
    float y = CGMinimapFrame::s_pingPosition.y - position.y;

    if (g_rotateMinimapCVar->m_intValue) {
        C44Matrix matrix;
        matrix *= C44Matrix::Rotation(-CGMinimapFrame::s_facing, C3Vector(0.0f, 0.0f, 1.0f), true);
        C3Vector rotated = C3Vector(x, y, 0.0f) * matrix;
        x = rotated.x;
        y = rotated.y;
    }

    double scale = 1.0 / diameter;
    x = x * scale;
    lua_pushnumber(L, -(scale * y));
    lua_pushnumber(L, x);
    return 2;
}

// VERIFY: 77953830 manual
// OFFSET: 0x0057F380
static int32_t Script_SetTracking(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        int32_t spellId = CGMinimapFrame::s_trackingSpell;

        if (spellId) {
            SpellRec spell;

            if (g_spellDB.GetRecord(spellId, &spell)) {
                Spell_C_CancelAura(&spell, ClntObjMgrGetActivePlayerObj(), -1);
                CGMinimapFrame::s_trackingSpell = 0;
                Script_SendUniqueSignal(SCRIPT_EVENT_MINIMAP_UPDATE_TRACKING);
            }
        }

        CGMinimapFrame::LoadTrackingInfo(nullptr);
        return 0;
    }

    int32_t index = static_cast<int32_t>(static_cast<int64_t>(lua_tonumber(L, 1))) - 1;

    if (index < static_cast<int32_t>(s_trackingSpells.Count())) {
        Spell_C::CastSpell(s_trackingSpells.m_data[index], nullptr, WOWGUID(), false);
        return 0;
    }

    const TrackingTypeInfo* info = CGMinimapFrame::GetTrackingTypeByIndex(index - s_trackingSpells.Count());

    if (!info) {
        return 0;
    }

    int32_t spellId = CGMinimapFrame::s_trackingSpell;

    if (spellId) {
        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell)) {
            Spell_C_CancelAura(&spell, ClntObjMgrGetActivePlayerObj(), -1);
        }
    }

    CGMinimapFrame::LoadTrackingInfo(info);
    return 0;
}

// VERIFY: 2a971c41 manual
// OFFSET: 0x00583860
static int32_t Script_SetMaskTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetMaskTexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_maskTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_maskTexture));
    }

    s_maskTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetMaskTexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 481d932d manual
// OFFSET: 0x00583980
static int32_t Script_SetPOIArrowTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetPOITexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_poiArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_poiArrowTexture));
    }

    s_poiArrowTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetPOITexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 3c5ea1bd manual
// OFFSET: 0x00583AA0
static int32_t Script_SetStaticPOIArrowTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetStaticPOITexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_staticPOIArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_staticPOIArrowTexture));
    }

    s_staticPOIArrowTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetStaticPOITexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 3a47b4f3 manual
// OFFSET: 0x00583BC0
static int32_t Script_SetCorpsePOIArrowTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetCorpsePOITexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_corpseArrowTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_corpseArrowTexture));
    }

    s_corpseArrowTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetCorspePOITexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 0c522adf manual
// OFFSET: 0x00583CE0
static int32_t Script_SetIconTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetIconTexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_iconTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_iconTexture));
    }

    s_iconTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetIconTexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 41ead87f manual
// OFFSET: 0x00583E00
static int32_t Script_SetBlipTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetBlipTexture(\"file\")", name ? name : "<unnamed>");
    }

    const char* fileName = lua_tolstring(L, 2, nullptr);

    if (!fileName || !*fileName) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetBlipTexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_blipTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_blipTexture));
    }

    s_blipTexture = TextureCreate(fileName, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetBlipTexture(): Couldn't load the file %s", name ? name : "<unnamed>", fileName);
    }

    return 0;
}

// VERIFY: dfb626e3 manual
// OFFSET: 0x00583F60
static int32_t Script_SetClassBlipTexture(lua_State* L) {
    CGMinimapFrame* minimap = static_cast<CGMinimapFrame*>(FrameScript_GetObjectThis(L, CGMinimapFrame::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = minimap->GetName();
        luaL_error(L, "Usage: %s:SetClassBlipTexture(\"file\")", name ? name : "<unnamed>");
    }

    CStatus status;

    if (s_classBlipTexture) {
        HandleClose(reinterpret_cast<HOBJECT>(s_classBlipTexture));
    }

    s_classBlipTexture = TextureCreate(lua_tolstring(L, 2, nullptr), CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1), &status, 3);

    if (status.m_highestSeverity >= STATUS_ERROR) {
        const char* name = minimap->GetName();
        luaL_error(L, "%s:SetClassBlipTexture(): Couldn't load the file %s", name ? name : "<unnamed>", lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// STATIC: 0x00ACEBB8
static FrameScript_Method s_ScriptMethods[] = {
    { "SetMaskTexture", &Script_SetMaskTexture },
    { "SetIconTexture", &Script_SetIconTexture },
    { "SetBlipTexture", &Script_SetBlipTexture },
    { "SetClassBlipTexture", &Script_SetClassBlipTexture },
    { "SetPOIArrowTexture", &Script_SetPOIArrowTexture },
    { "SetStaticPOIArrowTexture", &Script_SetStaticPOIArrowTexture },
    { "SetCorpsePOIArrowTexture", &Script_SetCorpsePOIArrowTexture },
    { "SetPlayerTexture", &Script_SetPlayerTexture },
    { "SetPlayerTextureHeight", &Script_SetPlayerTextureHeight },
    { "SetPlayerTextureWidth", &Script_SetPlayerTextureWidth },
    { "GetZoomLevels", &Script_GetZoomLevels },
    { "GetZoom", &Script_GetZoom },
    { "SetZoom", &Script_SetZoom },
    { "PingLocation", &Script_PingLocation },
    { "GetPingPosition", &Script_GetPingPosition },
};

// VERIFY: acea5034 auto
// OFFSET: 0x0057C040
void CGMinimapFrame::RegisterScriptMethods(lua_State* L) {
    CSimpleFrame::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_ScriptMethods, sizeof(s_ScriptMethods) / sizeof(s_ScriptMethods[0]));
}

// STATIC: 0x00ACEC30
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumTrackingTypes", &Script_GetNumTrackingTypes },
    { "GetTrackingInfo", &Script_GetTrackingInfo },
    { "SetTracking", &Script_SetTracking },
    { "GetTrackingTexture", &Script_GetTrackingTexture },
};

// VERIFY: 41ea4a09 auto
// OFFSET: 0x0057C090
void MinimapRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 8113deb2 auto
// OFFSET: 0x0057C0C0
void MinimapUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
