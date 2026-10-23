#include "GlyphInfo.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/GlyphPropertiesRec.h"
#include "WoW/Source/DB/Rec/GlyphSlotRec.h"
#include "WoW/Source/DB/Rec/SpellIconRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// VERIFY: 6144c30d auto
// OFFSET: 0x005B71E0
static int32_t Script_GetNumGlyphSockets(lua_State* L) {
    lua_pushnumber(L, 6.0);
    return 1;
}

// VERIFY: 5f83f5a1 manual
// OFFSET: 0x005B7260
static int32_t Script_GetGlyphSocketInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGlyphSocketInfo(index[, talentGroup])");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGTalentGroup_C* group = TalentInfoGetTalentGroup(TalentInfoGetScriptGroupIndex(L, 2), 0, 0);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (index >= 6 || !group || !player) {
        return 0;
    }

    if (player->m_player->m_playerGlyphsEnabled & (1 << index)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    GlyphSlotRec* slot = g_glyphSlotDB.GetRecord(player->m_player->m_playerFieldGlyphSlots1[index]);

    if (slot) {
        lua_pushnumber(L, slot->m_type + 1);
    } else {
        lua_pushnil(L);
    }

    GlyphPropertiesRec* glyph = g_glyphPropertiesDB.GetRecord(group->GetGlyph(index));

    if (glyph) {
        lua_pushnumber(L, glyph->m_spellID);
        SpellIconRec* icon = g_spellIconDB.GetRecord(glyph->m_spellIconID);

        if (icon) {
            lua_pushstring(L, icon->m_textureFilename);
        } else {
            lua_pushnil(L);
        }
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: dce2d34a auto
// OFFSET: 0x005B7410
static int32_t Script_GlyphMatchesSocket(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GlyphMatchesSocket(socketIndex)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        lua_pushnil(L);
        return 1;
    }

    SpellRec spell;
    PendingSpellCast* cast = Spell_C_GetTargetingSpellCast();

    if (!cast || !g_spellDB.GetRecord(cast->m_data.m_spellId, &spell)) {
        return 0;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] != SPELL_EFFECT_APPLY_GLYPH) {
            continue;
        }

        GlyphPropertiesRec* glyph = g_glyphPropertiesDB.GetRecord(spell.m_effectMiscValue[i]);
        GlyphSlotRec* slot = g_glyphSlotDB.GetRecord(player->m_player->m_playerFieldGlyphSlots1[index]);

        if (!glyph || !slot) {
            continue;
        }

        if (slot->m_type != glyph->m_glyphSlotFlags) {
            return 0;
        }

        lua_pushnumber(L, 1.0);
        return 1;
    }

    return 0;
}

// VERIFY: c5e5b51d manual
// OFFSET: 0x005B75A0
static int32_t Script_PlaceGlyphInSocket(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PlaceGlyphInSocket(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (index >= 6 || !player) {
        return 0;
    }

    if (Spell_C_HandleGlyphClick(index)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: cccb8fa4 manual
// OFFSET: 0x005B7660
static int32_t Script_RemoveGlyphFromSocket(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: RemoveGlyphFromSocket(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (index < 6 && player && player->m_player->m_playerFieldGlyphSlots1[index]) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_REMOVE_GLYPH));
        msg.Put(index);
        msg.Finalize();
        ClientServices::Send2(&msg);
    }

    return 0;
}

// VERIFY: 64be7f0c manual
// OFFSET: 0x005B7740
static int32_t Script_GetGlyphLink(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetGlyphLink(index[, talentGroup])");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    CGTalentGroup_C* group = TalentInfoGetTalentGroup(TalentInfoGetScriptGroupIndex(L, 2), 0, 0);
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (index >= 6 || !player || !group) {
        return 0;
    }

    lua_pushstring(L, CGTooltip::GetGlyphLink(player->m_player->m_playerFieldGlyphSlots1[index], group->GetGlyph(index)));
    return 1;
}

// STATIC: 0x00ACFF54
static FrameScript_Method s_ScriptFunctions[] = {
    { "GetNumGlyphSockets", &Script_GetNumGlyphSockets },
    { "GetGlyphSocketInfo", &Script_GetGlyphSocketInfo },
    { "GlyphMatchesSocket", &Script_GlyphMatchesSocket },
    { "PlaceGlyphInSocket", &Script_PlaceGlyphInSocket },
    { "RemoveGlyphFromSocket", &Script_RemoveGlyphFromSocket },
    { "GetGlyphLink", &Script_GetGlyphLink },
};

// VERIFY: 177745e8 auto
// OFFSET: 0x005B7210
void GlyphInfoRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 8012ebde auto
// OFFSET: 0x005B7240
void GlyphInfoUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
