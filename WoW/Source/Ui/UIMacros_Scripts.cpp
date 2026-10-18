#include "UIMacros.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "WoW/Source/Ui/GameUI.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/ModCore/ModCore.h"

// VERIFY: 3dacc7a5 auto
// OFFSET: 0x00564B90
static int32_t Script_StopMacro(lua_State* L) {
    if (CGGameUI::CanPerformAction(3)) {
        CGUIMacros::m_macroButton = nullptr;
        CGUIMacros::m_macroRunning = nullptr;
    }

    return 0;
}

// VERIFY: ca803391 auto
// OFFSET: 0x00564C70
static int32_t Script_GetRunningMacroButton(lua_State* L) {
    lua_pushstring(L, CGUIMacros::m_macroRunning);
    return 1;
}

// VERIFY: a3632573 manual
// OFFSET: 0x00564BB0
static int32_t Script_GetNumMacros(lua_State* L) {
    lua_pushnumber(L, CGUIMacros::m_numMacros[0]);
    lua_pushnumber(L, CGUIMacros::m_numMacros[1]);
    return 2;
}

// VERIFY: c78f099b auto
// OFFSET: 0x00564C10
static int32_t Script_GetRunningMacro(lua_State* L) {
    uint32_t id;

    if (CGUIMacros::m_macroButton && (id = CGUIMacros::m_macroButton->m_id)) {
        lua_pushnumber(L, CGUIMacros::GetMacroIndexByID(id) + 1);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 5ba8c11b manual
// OFFSET: 0x00566DF0
static MACRONODE* GetMacroFromLua(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

        if (index < 72) {
            return CGUIMacros::GetMacroByID(CGUIMacros::m_sortedMacros[index]);
        }
    } else if (lua_isstring(L, 1)) {
        return CGUIMacros::FindMacro(lua_tolstring(L, 1, nullptr));
    }

    return nullptr;
}

// VERIFY: 2e567c78 auto
// OFFSET: 0x00567320
static int32_t Script_SetMacroSpell(lua_State* L) {
    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetMacroSpell(macro, spell [,target])");
        return 0;
    }

    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    int32_t spellId = GetSpellIdByName(lua_tolstring(L, 2, nullptr), &macro->m_spellFlags);

    if (!spellId) {
        spellId = -1;
    }

    WOWGUID target = { 0 };
    Script_GetGUIDFromToken(lua_tolstring(L, 3, nullptr), target, false);

    if (macro->m_itemID > 0 || spellId != macro->m_spellID || target != macro->m_target) {
        macro->m_target = target;
        macro->m_itemID = 0;
        macro->m_spellID = spellId;
        CGActionBar::UpdateMacro(macro->m_id);
    }

    if (macro->m_dynamic) {
        macro->m_dynamic = 0;
        CGUIMacros::RemoveDynamicMacro(macro);
    }

    if (spellId > 0) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 90bfdb92 auto
// OFFSET: 0x00567450
static int32_t Script_GetMacroSpell(lua_State* L) {
    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(macro->m_spellID, &spell)) {
        lua_pushstring(L, spell.m_name);
        lua_pushstring(L, spell.m_nameSubtext);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: cbd63495 auto
// OFFSET: 0x00564AE0
static int32_t Script_SecureCmdOptionParse(lua_State* L) {
    const char* options = lua_tolstring(L, 1, nullptr);

    if (!options) {
        luaL_error(L, "Usage: SecureCmdOptionParse(\"options\")");
        return 0;
    }

    MacroSubstring result = { nullptr, nullptr };
    MacroSubstring target = { nullptr, nullptr };

    if (!EvaluateMacroOptions(options, options + SStrLen(options), &result, &target)) {
        return 0;
    }

    lua_pushlstring(L, result.m_start, result.m_end - result.m_start);

    if (target.m_end == target.m_start) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, target.m_start, target.m_end - target.m_start);
    }

    return 2;
}

// VERIFY: 2de408c7 auto
// OFFSET: 0x00564E90
static int32_t Script_GetMacroIconInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMacroIconInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* icon;

    if (index < s_macroIcons.m_count && (icon = s_macroIcons.m_data[index]) && *icon) {
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s", s_macroIconPath, icon);
        lua_pushstring(L, texture);
        return 1;
    }

    lua_pushstring(L, "");
    return 1;
}

// VERIFY: 7c42f7b9 auto
// OFFSET: 0x00564F60
static int32_t Script_GetMacroItemIconInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMacroItemIconInfo(index)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* icon;

    if (index < s_macroItemIcons.m_count && (icon = s_macroItemIcons.m_data[index]) && *icon) {
        char texture[260];
        SStrPrintf(texture, sizeof(texture), "%s%s", s_macroIconPath, icon);
        lua_pushstring(L, texture);
        return 1;
    }

    lua_pushstring(L, "");
    return 1;
}

// VERIFY: fa3f6068 auto
// OFFSET: 0x00566400
static int32_t Script_RunMacroText(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: RunMacroText(\"text\" [, \"button\"])");
        return 0;
    }

    const char* text = lua_tolstring(L, 1, nullptr);
    if (ModCore::Manager().OnMacroExecute(nullptr, text)) {
        return 0;
    }

    MACRONODE macro;
    SStrCopy(macro.m_body, text, sizeof(macro.m_body));
    CGUIMacros::RunMacro(&macro, lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 23abff1e manual
// OFFSET: 0x00566490
static int32_t Script_GetNumMacroIcons(lua_State* L) {
    if (!s_macroIcons.m_count) {
        CGUIMacros::LoadIcons();
    }

    lua_pushnumber(L, s_macroIcons.m_count);
    return 1;
}

// VERIFY: 5f8cee6e manual
// OFFSET: 0x005664E0
static int32_t Script_GetNumMacroItemIcons(lua_State* L) {
    if (!s_macroItemIcons.m_count) {
        CGUIMacros::LoadIcons();
    }

    lua_pushnumber(L, s_macroItemIcons.m_count);
    return 1;
}

// VERIFY: bec93d83 auto
// OFFSET: 0x005666A0
static int32_t Script_GetMacroIndexByName(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetMacroIndexByName(name)");
        return 0;
    }

    lua_pushnumber(L, CGUIMacros::GetSortedIndexByName(lua_tolstring(L, 1, nullptr)) + 1);
    return 1;
}

// VERIFY: 4afcfcfa auto
// OFFSET: 0x00566E80
static int32_t Script_RunMacro(lua_State* L) {
    MACRONODE* macro = GetMacroFromLua(L);

    if (macro) {
        if (ModCore::Manager().OnMacroExecute(macro->m_name, macro->m_body)) {
            return 0;
        }
        CGUIMacros::RunMacro(macro, lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 618b5785 auto
// OFFSET: 0x00566EB0
static int32_t Script_GetMacroInfo(lua_State* L) {
    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    lua_pushstring(L, macro->m_name);
    char texture[260];
    CGUIMacros::GetFullIconPath(macro->m_id, texture, sizeof(texture));

    if (texture[0]) {
        lua_pushstring(L, texture);
    } else {
        lua_pushnil(L);
    }

    lua_pushstring(L, macro->m_body);
    return 3;
}

// VERIFY: d7423f73 manual
// OFFSET: 0x00566F40
static int32_t Script_GetMacroBody(lua_State* L) {
    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    lua_pushstring(L, macro->m_body);
    return 1;
}

// VERIFY: 83e2ca27 auto
// OFFSET: 0x00566F70
static int32_t Script_DeleteMacro(lua_State* L) {
    if (CGGameUI::CanPerformAction(12)) {
        MACRONODE* macro = GetMacroFromLua(L);

        if (macro) {
            CGUIMacros::DeleteMacro(macro->m_id);
        }
    }

    return 0;
}

// VERIFY: 5b5c7cba manual
// OFFSET: 0x00566FA0
static int32_t Script_EditMacro(lua_State* L) {
    if (!CGGameUI::CanPerformAction(12)) {
        return 0;
    }

    uint32_t id = 0;
    MACRONODE* macro = GetMacroFromLua(L);

    if (macro) {
        id = macro->m_id;

        if (id) {
            const char* name = nullptr;
            const char* icon = nullptr;
            const char* body = nullptr;

            if (lua_isstring(L, 2)) {
                name = lua_tolstring(L, 2, nullptr);
            }

            if (lua_isnumber(L, 3)) {
                double index = lua_tonumber(L, 3);

                if (!s_macroIcons.m_count) {
                    CGUIMacros::LoadIcons();
                }

                if (static_cast<uint32_t>(index - 1.0) < s_macroIcons.m_count) {
                    icon = GetMacroIcon(static_cast<uint32_t>(index - 1.0));
                }
            }

            if (lua_isstring(L, 4)) {
                body = lua_tolstring(L, 4, nullptr);
            }

            CGUIMacros::EditMacro(id, name, icon, body);
        } else {
            id = 0;
        }
    }

    int32_t index;

    for (index = 0; CGUIMacros::m_sortedMacros[index] != id; ) {
        if (++index >= 72) {
            index = -1;
            break;
        }
    }

    lua_pushnumber(L, index + 1);
    return 1;
}

// VERIFY: 8a7babb3 auto
// OFFSET: 0x005670D0
static int32_t Script_SetMacroItem(lua_State* L) {
    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Usage: SetMacroItem(macro, item [,target])");
        return 0;
    }

    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    int32_t itemId = GetItemIDByName(lua_tolstring(L, 2, nullptr));
    WOWGUID target = { 0 };
    Script_GetGUIDFromToken(lua_tolstring(L, 3, nullptr), target, false);

    if (macro->m_spellID > 0 || itemId != macro->m_itemID || target != macro->m_target) {
        macro->m_target = target;
        macro->m_itemID = itemId;
        macro->m_spellID = (itemId != 0) - 1;
        CGActionBar::UpdateMacro(macro->m_id);
    }

    if (macro->m_dynamic) {
        macro->m_dynamic = 0;
        CGUIMacros::RemoveDynamicMacro(macro);
    }

    if (itemId > 0) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 0f420647 manual
// OFFSET: 0x00567200
static int32_t Script_GetMacroItem(lua_State* L) {
    MACRONODE* macro = GetMacroFromLua(L);

    if (!macro) {
        return 0;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(macro->m_itemID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    char name[1024];
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        CGItem_C* item = player->m_bag.FindItemOfType(macro->m_itemID, 0);

        if (item) {
            lua_pushstring(L, item->GetFullItemName(name, sizeof(name)));
            lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
            return 2;
        }
    }

    lua_pushstring(L, CGItem_C::BuildItemName(name, sizeof(name), macro->m_itemID, 0));
    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(macro->m_itemID, stats->m_quality, 0, 0, 0));
    return 2;
}

// VERIFY: 0a84a578 auto
// OFFSET: 0x005674F0
static int32_t Script_PickupMacro(lua_State* L) {
    if (CGGameUI::CanPerformAction(14)) {
        MACRONODE* macro = GetMacroFromLua(L);

        if (macro) {
            CGGameUI::SetCursorMacro(macro->m_id);
        }
    }

    return 0;
}

// VERIFY: c8955690 manual
// OFFSET: 0x00568160
static int32_t Script_CreateMacro(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CreateMacro(name, iconIndex, body, perCharacter)");
        return 0;
    }

    if (!CGGameUI::CanPerformAction(12)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    const char* icon = nullptr;

    if (!name || !*name) {
        luaL_error(L, "CreateMacro() failed, no name specified");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (index < GetNumMacroIcons()) {
        icon = GetMacroIcon(index);
    }

    const char* body = lua_isstring(L, 3) ? lua_tolstring(L, 3, nullptr) : nullptr;
    uint32_t id = CGUIMacros::CreateMacro(name, icon, body, StringToBOOL(L, 4, 0));

    if (!id) {
        luaL_error(L, "CreateMacro() failed, already have %d macros", 36);
        return 0;
    }

    lua_pushnumber(L, CGUIMacros::GetMacroIndexByID(id) + 1);
    return 1;
}

// STATIC: 0x00ACE1F0
static FrameScript_Method s_ScriptFunctions[] = {
    { "SecureCmdOptionParse", &Script_SecureCmdOptionParse },
    { "RunMacro", &Script_RunMacro },
    { "RunMacroText", &Script_RunMacroText },
    { "StopMacro", &Script_StopMacro },
    { "CreateMacro", &Script_CreateMacro },
    { "GetNumMacros", &Script_GetNumMacros },
    { "GetMacroInfo", &Script_GetMacroInfo },
    { "GetMacroBody", &Script_GetMacroBody },
    { "DeleteMacro", &Script_DeleteMacro },
    { "EditMacro", &Script_EditMacro },
    { "SetMacroItem", &Script_SetMacroItem },
    { "GetMacroItem", &Script_GetMacroItem },
    { "SetMacroSpell", &Script_SetMacroSpell },
    { "GetMacroSpell", &Script_GetMacroSpell },
    { "GetNumMacroIcons", &Script_GetNumMacroIcons },
    { "GetNumMacroItemIcons", &Script_GetNumMacroItemIcons },
    { "GetMacroIconInfo", &Script_GetMacroIconInfo },
    { "GetMacroItemIconInfo", &Script_GetMacroItemIconInfo },
    { "PickupMacro", &Script_PickupMacro },
    { "GetMacroIndexByName", &Script_GetMacroIndexByName },
    { "GetRunningMacro", &Script_GetRunningMacro },
    { "GetRunningMacroButton", &Script_GetRunningMacroButton },
};

// VERIFY: 72f0afdb auto
// OFFSET: 0x00564C90
void UIMacrosRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: bb01b596 auto
// OFFSET: 0x00564CC0
void UIMacrosUnegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
