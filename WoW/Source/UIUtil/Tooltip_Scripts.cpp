#include "Tooltip.h"
#include <cstdint>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/Types.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/ObjectClient/Container_C.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include <cstdio>
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/TalentRec.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Ui/DanceStudio.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Object/ItemStats.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Object/ObjectClient/LootRoll.h"
#include "WoW/Source/Object/Item.h"
#include "Engine/Source/Tempest/CAaRect.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "WoW/Source/Ui/TotemInfo.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Object/QuestCache.h"
#include "WoW/Source/Ui/ClassTrainerFrame.h"
#include "WoW/Source/DB/Rec/ItemExtendedCostRec.h"
#include <cstring>
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/Object/ObjectClient/Trade_C.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/AuctionHouse.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/Ui/GuildBankFrame.h"
#include "WoW/Source/Ui/CurrencyTypes.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/DB/Rec/CurrencyTypesRec.h"
#include "WoW/Source/Ui/EquipmentManager.h"
#include "WoW/Source/Ui/LFGInfo.h"
#include "WoW/Source/DB/Rec/LfgDungeonsRec.h"

// VERIFY: 30f18f7b manual
// OFFSET: 0x0061A150
static int32_t ReadTexCoordArgs(lua_State* L, int32_t index, CAaRect& coords) {
    coords.minY = static_cast<float>(lua_tonumber(L, index));
    coords.maxY = static_cast<float>(lua_tonumber(L, index + 1));
    coords.minX = static_cast<float>(lua_tonumber(L, index + 2));
    coords.maxX = static_cast<float>(lua_tonumber(L, index + 3));
    return index + 4;
}

// VERIFY: 321f8a1a auto
// OFFSET: 0x0061B470
static int32_t FindItemByGUID(const CGItem_C* item, void* param) {
    return item->m_obj->m_guid == *static_cast<WOWGUID*>(param);
}

// VERIFY: f6c0cb74 auto
// OFFSET: 0x0061D040
static int32_t Script_SetMinimumWidth(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetMinimumWidth(width [,force])", name ? name : "<unnamed>");
        return 0;
    }

    int32_t force = StringToBOOL(L, 3, 0);
    tooltip->m_minimumWidth = lua_tonumber(L, 2);
    tooltip->m_minimumWidthForce = force;
    return 0;
}

// VERIFY: df833e20 auto
// OFFSET: 0x0061D0D0
static int32_t Script_GetMinimumWidth(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    lua_pushnumber(L, tooltip->m_minimumWidth);

    if (tooltip->m_minimumWidthForce) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: dc0e015c auto
// OFFSET: 0x0061D150
static int32_t Script_SetPadding(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    float padding = lua_tonumber(L, 2);
    tooltip->m_padding = NDCToDDCWidth(padding / (CoordinateGetAspectCompensation() * 1024.0f));
    return 0;
}

// VERIFY: 8db86c86 auto
// OFFSET: 0x0061D1C0
static int32_t Script_GetPadding(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    lua_pushnumber(L, tooltip->m_padding);
    return 1;
}

// VERIFY: 3d13683a auto
// OFFSET: 0x0061D210
static int32_t Script_IsOwned(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (lua_type(L, 2) != LUA_TTABLE) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:IsOwned(frame)", name ? name : "<unnamed>");
        return 0;
    }

    lua_rawgeti(L, 2, 0);
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!frame) {
        const char* name = tooltip->GetName();
        luaL_error(L, "%s:IsOwned(): Couldn't find 'this' in frame object", name ? name : "<unnamed>");
        return 0;
    }

    if (!frame->IsA(CSimpleFrame::GetObjectType())) {
        const char* name = tooltip->GetName();
        luaL_error(L, "%s:IsOwned(): Wrong object type, expected frame", name ? name : "<unnamed>");
        return 0;
    }

    if (tooltip->m_owner == frame) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: ef7a4d10 manual
// OFFSET: 0x0061D350
static int32_t Script_GetOwner(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    CSimpleFrame* owner = tooltip->m_owner;

    if (!owner) {
        lua_pushnil(L);
        return 1;
    }

    if (!owner->m_luaRegistered) {
        owner->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, owner->m_luaObjectRef);
    return 1;
}

// VERIFY: a61b06c0 auto
// OFFSET: 0x0061D7D0
static int32_t Script_ClearLines(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    tooltip->ClearTooltip();
    return 0;
}

// VERIFY: 1c7c58ca manual
// OFFSET: 0x0061D940
static int32_t Script_FadeOut(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    tooltip->FadeOut();
    return 0;
}

// VERIFY: 95095d7d manual
// OFFSET: 0x0061D9C0
static int32_t Script_NumLines(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    lua_pushnumber(L, tooltip->m_numLines);
    return 1;
}

// VERIFY: 0c5c962a manual
// OFFSET: 0x0061F1F0
static int32_t Script_IsEquippedItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    WOWGUID guid = tooltip->m_itemGuid;

    if (player && guid != WOWGUID() && player->m_bag.FindItem(FindItemByGUID, &guid, 1)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: c0a54dc6 auto
// OFFSET: 0x0061B4C0
void CGTooltip::DestroyScriptMetaTable() {
    luaL_unref(FrameScript_GetContext(), LUA_REGISTRYINDEX, CGTooltip::s_metatable);
    CGTooltip::s_metatable = LUA_NOREF;
}

// VERIFY: fdbe92dc auto
// OFFSET: 0x0061DBA0
void CGTooltip::CreateScriptMetaTable() {
    CGTooltip::s_metatable = FrameScript_Object::CreateScriptMetaTable(FrameScript_GetContext(), &CGTooltip::RegisterScriptMethods);
}

// VERIFY: 3d12aa78 auto
// OFFSET: 0x0061EB40
static int32_t Script_SetOwner(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    tooltip->Hide();

    if (lua_type(L, 2) != LUA_TTABLE) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetOwner(frame)", name ? name : "<unnamed>");
        return 0;
    }

    lua_rawgeti(L, 2, 0);
    CSimpleFrame* owner = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!owner) {
        const char* name = tooltip->GetName();
        luaL_error(L, "%s:SetOwner(): Couldn't find 'this' in frame object", name ? name : "<unnamed>");
        return 0;
    }

    if (!owner->IsA(CSimpleFrame::GetObjectType())) {
        const char* name = tooltip->GetName();
        luaL_error(L, "%s:SetOwner(): Wrong object type, expected frame", name ? name : "<unnamed>");
        return 0;
    }

    if (owner == tooltip) {
        const char* name = tooltip->GetName();
        luaL_error(L, "%s:SetOwner(): Can't set owner to self", name ? name : "<unnamed>");
        return 0;
    }

    TOOLTIP_ANCHORPOINT anchor = ANCHOR_LEFT;

    if (lua_isstring(L, 3)) {
        const char* point = lua_tolstring(L, 3, nullptr);

        if (!SStrCmpI(point, "ANCHOR_LEFT", STORM_MAX_STR)) {
            anchor = ANCHOR_LEFT;
        } else if (!SStrCmpI(point, "ANCHOR_RIGHT", STORM_MAX_STR)) {
            anchor = ANCHOR_RIGHT;
        } else if (!SStrCmpI(point, "ANCHOR_BOTTOMLEFT", STORM_MAX_STR)) {
            anchor = ANCHOR_BOTTOMLEFT;
        } else if (!SStrCmpI(point, "ANCHOR_BOTTOM", STORM_MAX_STR)) {
            anchor = ANCHOR_BOTTOM;
        } else if (!SStrCmpI(point, "ANCHOR_BOTTOMRIGHT", STORM_MAX_STR)) {
            anchor = ANCHOR_BOTTOMRIGHT;
        } else if (!SStrCmpI(point, "ANCHOR_TOPLEFT", STORM_MAX_STR)) {
            anchor = ANCHOR_TOPLEFT;
        } else if (!SStrCmpI(point, "ANCHOR_TOP", STORM_MAX_STR)) {
            anchor = ANCHOR_TOP;
        } else if (!SStrCmpI(point, "ANCHOR_TOPRIGHT", STORM_MAX_STR)) {
            anchor = ANCHOR_TOPRIGHT;
        } else if (!SStrCmpI(point, "ANCHOR_CURSOR", STORM_MAX_STR)) {
            anchor = ANCHOR_CURSOR;
        } else if (!SStrCmpI(point, "ANCHOR_NONE", STORM_MAX_STR)) {
            anchor = ANCHOR_NONE;
        } else if (!SStrCmpI(point, "ANCHOR_PRESERVE", STORM_MAX_STR)) {
            anchor = ANCHOR_PRESERVE;
        } else if (!SStrCmpI(point, "ANCHOR_CURSOR_RIGHT", STORM_MAX_STR)) {
            anchor = ANCHOR_CURSOR_RIGHT;
        }
    }

    float offsetX = 0.0f;

    if (lua_isnumber(L, 4)) {
        float x = lua_tonumber(L, 4);
        offsetX = NDCToDDCWidth(x / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    float offsetY = 0.0f;

    if (lua_isnumber(L, 5)) {
        float y = lua_tonumber(L, 5);
        offsetY = NDCToDDCWidth(y / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    tooltip->SetOwner(owner, anchor, offsetX, offsetY);
    return 0;
}

// VERIFY: b3dfdf72 auto
// OFFSET: 0x00620340
static int32_t Script_AddLine(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    const char* text = nullptr;
    CImVector color = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);

    if (lua_isstring(L, 2)) {
        text = lua_tolstring(L, 2, nullptr);
    }

    if (lua_isnumber(L, 3)) {
        FrameScript_GetColorNoAlpha(L, 3, color);
    }

    int32_t wrap = StringToBOOL(L, 6, 0);
    tooltip->AddLine(text, nullptr, color, color, wrap);
    return 0;
}

// VERIFY: 33c79c5a manual
// OFFSET: 0x006204E0
static int32_t Script_SetText(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetText(\"text\" [, color])", name ? name : "<unnamed>");
        return 0;
    }

    const char* text = lua_tolstring(L, 2, nullptr);
    CImVector color = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);

    if (lua_isnumber(L, 3)) {
        FrameScript_GetColor(L, 3, color);
    }

    int32_t wrap = StringToBOOL(L, 7, 0);
    tooltip->ClearTooltip();
    tooltip->AddLine(text, nullptr, color, color, wrap);
    tooltip->Show();
    return 0;
}

// VERIFY: 46a6c27d auto
// OFFSET: 0x006203F0
static int32_t Script_AddDoubleLine(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    const char* left = nullptr;
    const char* right = nullptr;
    CImVector leftColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);
    CImVector rightColor = *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor);

    if (lua_isstring(L, 2)) {
        left = lua_tolstring(L, 2, nullptr);
    }

    if (lua_isstring(L, 3)) {
        right = lua_tolstring(L, 3, nullptr);
    }

    if (lua_isnumber(L, 4)) {
        FrameScript_GetColorNoAlpha(L, 4, leftColor);
    }

    if (lua_isnumber(L, 7)) {
        FrameScript_GetColorNoAlpha(L, 7, rightColor);
    }

    int32_t wrap = StringToBOOL(L, 10, 0);
    tooltip->AddLine(left, right, leftColor, rightColor, wrap);
    return 0;
}

// VERIFY: b8889e5f manual
// OFFSET: 0x0061EE90
static int32_t Script_AppendText(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:AppendText(\"text\")", name ? name : "<unnamed>");
        return 0;
    }

    tooltip->AppendText(lua_tolstring(L, 2, nullptr));
    return 0;
}

// VERIFY: 2fd5996e manual
// OFFSET: 0x006259E0
static int32_t Script_SetSpell(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    uint32_t slot;

    if (!lua_isnumber(L, 2) || !lua_isstring(L, 3) || (slot = static_cast<uint32_t>(lua_tonumber(L, 2) - 1.0), slot > 1023)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Invalid spell slot in %s:SetSpell", name ? name : "<unnamed>");
    }

    int32_t petSpell = 0;

    if (!SStrCmpI("pet", lua_tolstring(L, 3, nullptr), STORM_MAX_STR)) {
        petSpell = 1;
    }

    int32_t spellId = GetSpellIdBySlot(slot, petSpell);

    if (spellId > 0) {
        uint32_t duration = 0;
        uint32_t start = 0;
        uint32_t enable = 0;
        Spell_C_GetSpellCooldown(spellId, petSpell, &duration, &start, &enable);
        uint32_t cooldown = !enable || !start || !duration ? 0 : start + duration - OsGetAsyncTimeMs();

        if (tooltip->SetSpell(spellId, 0, cooldown, petSpell, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 12bc9108 manual
// OFFSET: 0x00625630
static int32_t Script_SetShapeshift(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetShapeshift(slot)", name ? name : "<unnamed>");
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2));

    if (slot - 1 < s_shapeshiftSpells.m_count) {
        int32_t spellId = s_shapeshiftSpells.m_data[slot - 1].m_spellId;
        uint32_t duration = 0;
        uint32_t start = 0;
        uint32_t enable = 0;
        Spell_C_GetSpellCooldown(spellId, 0, &duration, &start, &enable);
        int32_t brief = 1;
        CVar* uberTooltips = CVar::Lookup("UberTooltips");

        if (uberTooltips && uberTooltips->m_intValue) {
            brief = 0;
        }

        int32_t result;

        if (enable && start && duration) {
            result = tooltip->SetSpell(spellId, brief, start + duration - OsGetAsyncTimeMs(), 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        } else {
            result = tooltip->SetSpell(spellId, brief, 0, 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        }

        if (result) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: ed4d980b manual
// OFFSET: 0x00625B90
static int32_t Script_SetSpellByID(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    int32_t spellId;

    if (!lua_isnumber(L, 2) || (spellId = lua_tointeger(L, 2), spellId < 0)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Invalid spell ID in %s:SetSpellByID", name ? name : "<unnamed>");
    }

    int32_t petSpell = StringToBOOL(L, 3, 0);
    int32_t showRank = StringToBOOL(L, 4, 0);

    if (HasSpell(spellId, petSpell)) {
        uint32_t duration = 0;
        uint32_t start = 0;
        uint32_t enable = 0;
        Spell_C_GetSpellCooldown(spellId, petSpell, &duration, &start, &enable);
        uint32_t cooldown = !enable || !start || !duration ? 0 : start + duration - OsGetAsyncTimeMs();

        if (tooltip->SetSpell(spellId, 0, cooldown, petSpell, showRank, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: e0a3c738 manual
// OFFSET: 0x0062DAE0
static int32_t Script_SetHyperlink(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetHyperlink(link)", name ? name : "<unnamed>");
        return 0;
    }

    const char* link = lua_tolstring(L, 2, nullptr);
    const char* cursor;

    if ((cursor = SStrStr(link, "item:")) != nullptr) {
        int32_t itemId = SStrToInt(cursor + 5);

        if (tooltip->m_isShown && itemId == tooltip->m_itemId) {
            tooltip->Hide();
            return 0;
        }

        if (itemId > 0) {
            tooltip->m_itemLinkInfo.Clear();
            tooltip->m_itemLinkInfo.GetItemIdFromLink(link);
            WOWGUID requester = { 0 };
            WOWGUID itemGuid = { 0 };
            tooltip->SetItem(itemId, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID{ 0 }, 0, nullptr, 1, &tooltip->m_itemLinkInfo, 0, 0, 0);
        }

        return 0;
    }

    if ((cursor = SStrStr(link, "enchant:")) != nullptr) {
        int32_t spellId = SStrToInt(cursor + 8);

        if (tooltip->m_isShown && spellId == tooltip->m_spellId) {
            tooltip->Hide();
            return 0;
        }

        SpellRec spell;

        if (g_spellDB.GetRecord(spellId, &spell) && (spell.m_attributes & 0x20)) {
            tooltip->SetSpell(spellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        }

        return 0;
    }

    if ((cursor = SStrStr(link, "dance:")) != nullptr) {
        CGDanceStudio::LoadDance(SStrToInt(cursor + 6));
        return 0;
    }

    if ((cursor = SStrStr(link, "spell:")) != nullptr) {
        int32_t spellId = SStrToInt(cursor + 6);

        if (tooltip->m_isShown && spellId == tooltip->m_spellId) {
            tooltip->Hide();
            return 0;
        }

        tooltip->SetSpell(spellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        return 0;
    }

    if ((cursor = SStrStr(link, "unit:")) != nullptr) {
        WOWGUID guid = StringToGuid(cursor + 5);

        if (tooltip->m_isShown && guid == tooltip->m_unitGuid) {
            tooltip->Hide();
            return 0;
        }

        WOWGUID healthGuid = { 0 };
        tooltip->SetUnit(guid, 0, healthGuid, 0);
        return 0;
    }

    if ((cursor = SStrStr(link, "quest:")) != nullptr) {
        int32_t questId = SStrToInt(cursor + 6);

        if (tooltip->m_isShown && questId == tooltip->m_questId) {
            tooltip->Hide();
            return 0;
        }

        tooltip->SetQuest(questId);
        return 0;
    }

    if ((cursor = SStrStr(link, "talent:")) != nullptr) {
        cursor += 7;
        TalentRec* talent = g_talentDB.GetRecord(SStrToInt(cursor));

        if (!talent) {
            return 0;
        }

        int32_t rank = -1;
        const char* separator = SStrChr(cursor, ':');

        if (separator) {
            rank = SStrToInt(separator + 1);
        }

        tooltip->SetTalent(talent, nullptr, 0, 1, rank, 0, 0);
        return 0;
    }

    if ((cursor = SStrStr(link, "trade:")) != nullptr) {
        CGTradeSkillInfo::SetLink(cursor + 6);
        return 0;
    }

    if ((cursor = SStrStr(link, "achievement:")) != nullptr) {
        cursor += 12;
        int32_t achievementId = SStrToInt(cursor);

        if (tooltip->m_isShown && achievementId == tooltip->m_achievementId) {
            tooltip->Hide();
            return 0;
        }

        WOWGUID guid = { 0 };
        WowTime time;
        int32_t completed = 0;
        uint32_t criteria1 = 0;
        uint32_t criteria2 = 0;
        uint32_t criteria3 = 0;
        uint32_t criteria4 = 0;
        const char* field = SStrChr(cursor, ':');

        if (field) {
            field++;
            sscanf(field, "%I64X:", &guid.m_guid);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            completed = SStrToInt(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            time.m_month = SStrToInt(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            time.m_monthDay = SStrToInt(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            time.m_year = SStrToInt(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            criteria1 = SStrToUnsigned(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            criteria2 = SStrToUnsigned(++field);
        }

        if ((field = SStrChr(field, ':')) != nullptr) {
            criteria3 = SStrToUnsigned(++field);
        }

        const char* last = SStrChr(field, ':');

        if (last) {
            criteria4 = SStrToUnsigned(last + 1);
        }

        tooltip->SetAchievement(achievementId, guid, completed != 0, time, criteria1, criteria2, criteria3, criteria4, 0);
        return 0;
    }

    if ((cursor = SStrStr(link, "glyph:")) != nullptr) {
        cursor += 6;
        int32_t glyphId = SStrToInt(cursor);
        const char* separator = SStrChr(cursor, ':');

        if (separator) {
            tooltip->SetGlyph(glyphId, SStrToInt(separator + 1), 1, 1, 0);
        }

        return 0;
    }

    luaL_error(L, "%s:SetHyperlink(): Unknown link type", tooltip->GetDisplayName());
    return 0;
}

// VERIFY: d3c4ca9b manual
// OFFSET: 0x0062E050
static int32_t Script_SetInventoryItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetInventoryItem(unit, slot [, nameOnly])", name ? name : "<unnamed>");
        return 0;
    }

    int32_t slot;

    if (!lua_isnumber(L, 3) || ((slot = static_cast<int32_t>(lua_tonumber(L, 3) - 1.0)) < 0 || slot > 22) && (slot < 39 || slot > 66) && (slot < 67 || slot > 73) && (slot < 86 || slot > 117) && (slot < 118 || slot > 149) && slot != -1) {
        luaL_error(L, "Invalid inventory slot in SetInventoryItem");
        return 0;
    }

    int32_t nameOnly = StringToBOOL(L, 4, 0);
    CGPlayer_C* unit = static_cast<CGPlayer_C*>(GetUnitFromName(lua_tolstring(L, 2, nullptr)));

    if (!CGCharacterInfo::CanInspect(unit)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 3;
    }

    if (slot == -1) {
        int32_t ammoId = CGCharacterInfo::GetActivePlayerAmmoID(unit);
        CGItem_C* ammo;

        if (ammoId && (ammo = unit->GetBag()->FindItemOfType(ammoId, 0)) != nullptr) {
            WOWGUID itemGuid = ammo->m_obj->m_guid;
            WOWGUID requester = unit->m_obj->m_guid;
            tooltip->SetItem(ammoId, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 0, 0, 0);
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        lua_pushnil(L);
        return 2;
    }

    CGItem_C* item = unit->m_bag.GetItemPointer(slot);

    if (!item) {
        InspectItem* inspected = unit->GetInspectedItem(slot);

        if (!inspected || !inspected->m_itemId) {
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushnumber(L, 0.0);
            return 3;
        }

        WOWGUID query = { 0 };
        g_itemDBCache.GetRecord(inspected->m_itemId, query, TooltipItemCallback, tooltip, false);
        ItemLinkInfo& link = tooltip->m_itemLinkInfo;
        link.Clear();
        link.m_randomPropertiesId = inspected->m_randomPropertiesId;

        for (uint32_t i = 0; i < 12; i++) {
            link.m_enchants[i] = inspected->m_enchants[i];
            link.m_enchantDurations[i] = 0;
            link.m_enchantCharges[i] = 0;
        }

        link.m_propertySeed = inspected->m_propertySeed;
        link.m_creator = inspected->m_creator;
        WOWGUID itemGuid = { 0 };
        WOWGUID requester = unit->m_obj->m_guid;
        tooltip->SetItem(inspected->m_itemId, &requester, &itemGuid, nameOnly, 0, 1, 0, unit->m_obj->m_guid, 0, nullptr, 0, nullptr, 0, 0, 0);
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        return 3;
    }

    int32_t result;

    if (item->GetUsableSpellCharges() && !item->GetCurrentSpellCharges()) {
        WOWGUID requester = unit->m_obj->m_guid;
        WOWGUID itemGuid = item->m_obj->m_guid;
        result = tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, nameOnly, 0, 0, 0, unit->m_obj->m_guid, 0, nullptr, 0, nullptr, 0, 0, 0);
    } else {
        uint32_t duration = 0;
        uint32_t start = 0;
        uint32_t enable = 0;
        Spell_C_GetItemCooldown(item, &duration, &start, &enable);

        if (enable && duration && start) {
            tooltip->m_itemLinkInfo.Clear();
            tooltip->m_itemLinkInfo.m_cooldown = start + duration - OsGetAsyncTimeMs();
            tooltip->m_itemLinkInfo.m_spellCharges = item->GetCurrentSpellCharges();
            WOWGUID requester = unit->m_obj->m_guid;
            WOWGUID itemGuid = item->m_obj->m_guid;
            result = tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, nameOnly, 0, 1, 0, unit->m_obj->m_guid, 0, nullptr, 0, nullptr, 0, 0, 0);
        } else {
            WOWGUID requester = unit->m_obj->m_guid;
            WOWGUID itemGuid = item->m_obj->m_guid;
            result = tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, nameOnly, 0, 0, 0, unit->m_obj->m_guid, 0, nullptr, 0, nullptr, 0, 0, 0);
        }
    }

    lua_pushnumber(L, 1.0);

    if (result) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, static_cast<uint32_t>(CGMerchantInfo::GetRepairCost(item)));
    return 3;
}

// VERIFY: eb0546cd manual
// OFFSET: 0x0062F420
static int32_t Script_SetBagItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    CGBag_C* bag;

    if (index == -1) {
        bag = &player->m_bag;
    } else {
        if (static_cast<uint32_t>(index) > 10) {
            return 0;
        }

        CGObject_C* container = ClntObjMgrObjectPtr(GetBagAtIndex(index), TYPE_CONTAINER, __FILE__, __LINE__);

        if (!container) {
            return 0;
        }

        bag = container->GetBag();
    }

    if (!bag) {
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 3)) - 1;

    if (bag == &player->m_bag) {
        slot += 23;
    }

    CGItem_C* item;

    if (slot < 0 || slot >= bag->m_numSlots || (item = bag->GetItemPointer(slot)) == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    int32_t result;
    uint32_t duration;
    uint32_t start;
    uint32_t enable;

    if ((item->GetUsableSpellCharges() && !item->GetCurrentSpellCharges()) || (duration = 0, start = 0, enable = 0, Spell_C_GetItemCooldown(item, &duration, &start, &enable), !enable || !duration || !start)) {
        WOWGUID requester = item->m_obj->m_guid;
        WOWGUID itemGuid = item->m_obj->m_guid;
        result = tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 0, 0, 0);
    } else {
        ItemLinkInfo& link = tooltip->m_itemLinkInfo;
        link.Clear();

        for (uint32_t i = 0; i < 12; i++) {
            link.m_enchants[i] = 0;
            link.m_enchantDurations[i] = 0;
            link.m_enchantCharges[i] = 0;
        }

        link.m_cooldown = duration + start - OsGetAsyncTimeMs();
        link.m_proposedEnchant = 0;
        link.m_creator = item->m_item->m_itemFieldCreator;
        link.m_wrapped = (item->m_item->m_itemFieldFlags >> 3) & 1;
        link.m_giftCreator = item->m_item->m_itemFieldGiftcreator;
        link.m_spellCharges = item->GetCurrentSpellCharges();
        WOWGUID requester = item->m_obj->m_guid;
        WOWGUID itemGuid = item->m_obj->m_guid;
        result = tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID{ 0 }, 0, nullptr, 0, nullptr, 0, 0, 0);
    }

    int32_t repairCost = CGMerchantInfo::GetRepairCost(item);

    if (result) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, repairCost);
    return 2;
}

// VERIFY: 92d6cbe1 manual
// OFFSET: 0x00625D00
static int32_t Script_SetGlyph(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetGlyph(glyphSlot[, groupIndex])", name ? name : "<unnamed>");
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGTalentGroup_C* group = TalentInfoGetTalentGroup(TalentInfoGetScriptGroupIndex(L, 3), 0, 0);

    if (player && group) {
        uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
        tooltip->SetGlyph(player->m_player->m_playerFieldGlyphSlots1[index], group->GetGlyph(index), (player->m_player->m_playerGlyphsEnabled & (1 << index)) != 0, 0, 0);
    }

    return 0;
}

// VERIFY: bb661fe0 manual
// OFFSET: 0x0062F740
static int32_t Script_SetTalent(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetTalent(tabIndex, talentIndex[, isInspect[, isPet[, groupIndex[, isPreview]]]])", name ? name : "<unnamed>");
    }

    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    uint32_t tabIndex = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t talentIndex = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1;
    int32_t inspect = StringToBOOL(L, 4, 0);
    int32_t pet = StringToBOOL(L, 5, 0);
    uint32_t group = TalentInfoGetScriptGroupIndex(L, 6);
    int32_t preview = StringToBOOL(L, 7, 0);
    TalentTabInfo* tab = CGTalentInfo::GetTab(tabIndex, inspect, pet);

    if (tab && talentIndex < tab->m_numTalents) {
        tooltip->SetTalent(&tab->m_talents[talentIndex], TalentInfoGetTalentGroup(group, inspect, pet), inspect, 0, -1, pet, preview);
    }

    return 0;
}

// VERIFY: dc5da273 manual
// OFFSET: 0x00631000
static int32_t Script_SetAction(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetAction(slot)", name ? name : "<unnamed>");
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2) - 1.0);
    uint32_t start;
    uint32_t duration;
    uint32_t enable;
    CGActionBar::GetCooldown(slot, start, duration, enable);
    tooltip->ClearTooltip();

    if (CGActionBar::IsAttackAction(slot)) {
        const CImVector& textColor = *reinterpret_cast<const CImVector*>(&s_tooltipTextColor);
        char text[32];
        SStrCopy(text, FrameScript_GetText("ATTACK", -1, GENDER_NOT_APPLICABLE), sizeof(text));
        tooltip->AddLine(text, nullptr, textColor, textColor, 0);
        tooltip->Show();
        lua_pushnil(L);
        return 1;
    }

    uint32_t action = CGActionBar::m_slotActions[slot];
    int32_t result;

    if (action && (action & 0xF0000000) == 0x80000000) {
        int32_t itemId = CGActionBar::GetItem(slot);
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
        WOWGUID none = { 0 };
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);
        int32_t charges = CGItem_C::NumBaseCharges(itemId);

        if (!charges) {
            CGItem_C* enchantItem = CGActionBar::FindUseEnchantItemPtr(itemId);

            if (enchantItem) {
                charges = enchantItem->GetUsableSpellCharges();
            }
        }

        CGItem_C* item = nullptr;

        if (player && stats) {
            uint32_t flags;

            if (stats->m_inventoryType) {
                flags = charges != -1 && charges ? 161 : 129;
                item = player->m_bag.FindItemOfType(itemId, flags);

                if (!item && (flags & 0x20)) {
                    item = player->m_bag.FindItemOfType(itemId, 32);
                }

                if (!item) {
                    item = player->m_bag.FindItemOfType(itemId, 128);
                }
            } else {
                flags = charges != -1 && charges ? 288 : 256;
                item = player->m_bag.FindItemOfType(itemId, flags);

                if (!item && (flags & 0x20)) {
                    item = player->m_bag.FindItemOfType(itemId, flags & ~0x20);
                }
            }
        }

        if (item) {
            tooltip->m_itemLinkInfo.Clear();
            tooltip->m_itemLinkInfo.m_cooldown = enable && start && duration ? duration + start - OsGetAsyncTimeMs() : 0;

            if (charges != -1 && charges) {
                tooltip->m_itemLinkInfo.m_spellCharges = item->GetCurrentSpellCharges();
            }

            WOWGUID guid = item->m_obj->m_guid;
            result = tooltip->SetItem(item->m_obj->m_entryID, &guid, &guid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
        } else {
            WOWGUID empty = { 0 };
            result = tooltip->SetItem(itemId, &empty, &empty, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
        }
    } else if (action && !(action & 0xF0000000)) {
        int32_t brief = 1;
        CVar* uber = CVar::Lookup("UberTooltips");

        if (uber && uber->m_intValue) {
            brief = 0;
        }

        int32_t petSpell;
        int32_t spellId = CGActionBar::GetSpell(slot, petSpell);

        if (enable && start && duration) {
            result = tooltip->SetSpell(spellId, brief, duration + start - OsGetAsyncTimeMs(), 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        } else {
            result = tooltip->SetSpell(spellId, brief, 0, 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        }
    } else if (CGActionBar::IsMacro(slot)) {
        result = tooltip->SetMacro(CGActionBar::GetMacroID(slot));
    } else if (CGActionBar::IsEquipmentSet(slot)) {
        result = tooltip->SetEquipmentSet(CGActionBar::GetEquipmentSetID(slot));
    } else {
        if (!CGActionBar::IsPetAction(slot)) {
            lua_pushnil(L);
            return 1;
        }

        PetAction* petAction = GetPetAction(CGActionBar::GetSlotSpellAction(slot));

        if (!petAction || !petAction->m_data) {
            lua_pushnil(L);
            return 1;
        }

        uint32_t value = petAction->m_data;
        char token[64];
        const char* text;

        switch ((value >> 24) & 0x3F) {
            case 1:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17: {
                uint32_t petStart = 0;
                uint32_t petDuration = 0;
                uint32_t petEnable = 0;
                Spell_C_GetSpellCooldown(value & 0xFFFFFF, 1, &petDuration, &petStart, &petEnable);

                if (petEnable && petStart && petDuration) {
                    result = tooltip->SetSpell(petAction->m_data & 0xFFFFFF, 0, petStart + petDuration - OsGetAsyncTimeMs(), 1, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
                } else {
                    result = tooltip->SetSpell(petAction->m_data & 0xFFFFFF, 0, 0, 1, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
                }

                goto done;
            }

            case 6: {
                SStrPrintf(token, sizeof(token), "PET_MODE_%s", CGPetInfo::GetModeToken(value & 0xFFFFFF));
                text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
                break;
            }

            case 7: {
                SStrPrintf(token, sizeof(token), "PET_ACTION_%s", CGPetInfo::GetOrdersToken(value & 0xFFFFFF));
                text = FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE);
                break;
            }

            default: {
                lua_pushnil(L);
                return 1;
            }
        }

        tooltip->AddLine(text, nullptr, 0);
        tooltip->Show();
        lua_pushnil(L);
        return 1;
    }

done:
    if (!result) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: c5b104e7 auto
// OFFSET: 0x00626240
static int32_t Script_SetUnitAura(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    ScriptAuraInfo info;

    if (!Script_GetUnitAuraInfo(L, 2, &info)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetUnitAura(\"unit\", [index] or [\"name\", \"rank\"][, \"filter\"])", name ? name : "<unnamed>");
        return 0;
    }

    return SetTooltipUnitAura(tooltip, &info);
}

// VERIFY: 51f1907f manual
// OFFSET: 0x00631B60
static int32_t Script_SetHyperlinkCompareItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetHyperlinkCompareItem(\"hyperlink\" [, offset, shiftButton])", name ? name : "<unnamed>");
        return 0;
    }

    const char* link = lua_tolstring(L, 2, nullptr);
    ItemLinkInfo linkInfo;
    linkInfo.Clear();

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__) && linkInfo.GetItemIdFromLink(link)) {
        int32_t offset = 0;
        int32_t shift = StringToBOOL(L, 4, 0);

        if (lua_isnumber(L, 3)) {
            offset = static_cast<int32_t>(lua_tonumber(L, 3)) - 1;
        }

        if (lua_type(L, 5) == LUA_TTABLE) {
            lua_rawgeti(L, 5, 0);
            CGTooltip* compare = static_cast<CGTooltip*>(lua_touserdata(L, -1));

            if (compare->IsA(CGTooltip::GetObjectType()) && SetCompareItem(tooltip, linkInfo.m_itemId, offset, shift, compare)) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }

        if (SetCompareItem(tooltip, linkInfo.m_itemId, offset, shift, nullptr)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3266ed80 manual
// OFFSET: 0x0062E520
static int32_t Script_SetLootItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Invalid loot slot in SetInventoryItem");
        return 0;
    }

    uint32_t slot = static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 2) - 1.0));
    int32_t itemId = CGLootInfo::GetLootItem(slot);

    if (itemId <= 0) {
        return 0;
    }

    tooltip->m_itemLinkInfo.Clear();
    tooltip->m_itemLinkInfo.m_propertySeed = CGLootInfo::GetLootItemRandomSuffix(slot);
    tooltip->m_itemLinkInfo.m_randomPropertiesId = CGLootInfo::GetLootItemRandomPropertyId(slot);
    tooltip->m_itemLinkInfo.m_stackCount = CGLootInfo::GetLootItemCount(slot);
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (stats) {
        for (uint32_t i = 0; i < 5; i++) {
            if (stats->m_spellId[i] && !stats->m_spellTrigger[i]) {
                tooltip->m_itemLinkInfo.m_spellCharges = stats->m_spellCharges[i];
                break;
            }
        }
    }

    WOWGUID itemGuid = WOWGUID();
    tooltip->SetItem(itemId, &CGLootInfo::m_lootObj, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 4a9b0c14 manual
// OFFSET: 0x006300A0
static int32_t Script_SetLootRollItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetLootRollItem(id)", name ? name : "<unnamed>");
    }

    PendingLootRoll* roll = FindRoll(static_cast<int32_t>(lua_tonumber(L, 2)));

    if (!roll) {
        return 0;
    }

    tooltip->m_itemLinkInfo.Clear();
    tooltip->m_itemLinkInfo.m_propertySeed = roll->m_randomSuffix;
    tooltip->m_itemLinkInfo.m_randomPropertiesId = roll->m_randomPropertyId;
    tooltip->m_itemLinkInfo.m_stackCount = roll->m_count;
    WOWGUID requester = WOWGUID();
    WOWGUID itemGuid = WOWGUID();
    tooltip->SetItem(roll->m_itemId, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 85edc8da auto
// OFFSET: 0x0061DA20
static int32_t Script_IsUnit(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:IsUnit(\"unit\")", name ? name : "<unnamed>");
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 2, nullptr));

    if (guid && guid == tooltip->m_unitGuid) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 18424f75 manual
// OFFSET: 0x0061EF10
static int32_t Script_GetItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!tooltip->m_itemId) {
        return 0;
    }

    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(tooltip->m_itemGuid, TYPE_ITEM, __FILE__, __LINE__));
    ItemLinkInfo* link = tooltip->m_itemUseLinkInfo ? &tooltip->m_itemLinkInfo : nullptr;
    bool wrapped = (item && (item->m_item->m_itemFieldFlags & ITEM_DFLAG_WRAPPED)) || (link && link->m_wrapped);
    int32_t randomPropertiesId = 0;

    if (!wrapped) {
        if (link && link->m_randomPropertiesId) {
            randomPropertiesId = link->m_randomPropertiesId;
        } else if (item) {
            randomPropertiesId = item->m_item->m_itemFieldRandomPropertiesId;
        }
    }

    char name[1024];
    CGItem_C::BuildItemName(name, sizeof(name), tooltip->m_itemId, randomPropertiesId);
    lua_pushstring(L, name);

    if (item) {
        lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
        return 2;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(tooltip->m_itemId, none, nullptr, nullptr, false);
    uint32_t quality = stats ? stats->m_quality : 1;

    if (!link || wrapped) {
        int32_t gems[3] = { 0, 0, 0 };
        lua_pushstring(L, CGTooltip::CreateItemLink(tooltip->m_itemId, quality, 0, gems, 0, 0));
        return 2;
    }

    lua_pushstring(L, CGTooltip::CreateItemLink(tooltip->m_itemId, quality, link->m_enchants[0], reinterpret_cast<const int32_t*>(&link->m_enchants[2]), link->m_randomPropertiesId, link->m_propertySeed));
    return 2;
}

// VERIFY: 65b5ead8 manual
// OFFSET: 0x0061D810
static int32_t Script_AddTexture(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    const char* texture = nullptr;

    if (lua_isstring(L, 2)) {
        texture = lua_tolstring(L, 2, nullptr);
    }

    if (lua_isnumber(L, 3)) {
        if (!lua_isnumber(L, 4) || !lua_isnumber(L, 5) || !lua_isnumber(L, 6)) {
            const char* name = tooltip->GetName();
            luaL_error(L, "Usage: %s:AddTexture(\"filename\" [, minx, maxx, miny, maxy])", name ? name : "<unnamed>");
        }

        CAaRect coords;
        coords.minX = 0.0f;
        coords.minY = 0.0f;
        coords.maxX = 0.0f;
        coords.maxY = 0.0f;
        ReadTexCoordArgs(L, 3, coords);
        tooltip->AddTexture(texture, coords, CImVector(0xFF, 0xFF, 0xFF, 0xFF));
        return 0;
    }

    CAaRect coords;
    coords.minX = 0.0f;
    coords.minY = 0.0f;
    coords.maxX = 1.0f;
    coords.maxY = 1.0f;
    tooltip->AddTexture(texture, coords, CImVector(0xFF, 0xFF, 0xFF, 0xFF));
    return 0;
}

// VERIFY: 07131868 auto
// OFFSET: 0x0061D650
static int32_t Script_GetAnchorType(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    switch (tooltip->m_anchorType) {
        case ANCHOR_LEFT:
            lua_pushstring(L, "ANCHOR_LEFT");
            return 1;

        case ANCHOR_RIGHT:
            lua_pushstring(L, "ANCHOR_RIGHT");
            return 1;

        case ANCHOR_BOTTOMLEFT:
            lua_pushstring(L, "ANCHOR_BOTTOMLEFT");
            return 1;

        case ANCHOR_BOTTOM:
            lua_pushstring(L, "ANCHOR_BOTTOM");
            return 1;

        case ANCHOR_BOTTOMRIGHT:
            lua_pushstring(L, "ANCHOR_BOTTOMRIGHT");
            return 1;

        case ANCHOR_TOPLEFT:
            lua_pushstring(L, "ANCHOR_TOPLEFT");
            return 1;

        case ANCHOR_TOP:
            lua_pushstring(L, "ANCHOR_TOP");
            return 1;

        case ANCHOR_TOPRIGHT:
            lua_pushstring(L, "ANCHOR_TOPRIGHT");
            return 1;

        case ANCHOR_CURSOR:
            lua_pushstring(L, "ANCHOR_CURSOR");
            return 1;

        case ANCHOR_PRESERVE:
            lua_pushstring(L, "ANCHOR_PRESERVE");
            return 1;

        case ANCHOR_CURSOR_RIGHT:
            lua_pushstring(L, "ANCHOR_CURSOR_RIGHT");
            return 1;

        default:
            lua_pushstring(L, "ANCHOR_NONE");
            return 1;
    }
}

// VERIFY: 90320666 auto
// OFFSET: 0x00625940
static int32_t Script_TooltipSetTracking(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    tooltip->ClearTooltip();
    const char* text;

    if (CGMinimapFrame::s_trackingInfo) {
        text = FrameScript_GetText(CGMinimapFrame::s_trackingInfo->m_name, -1, GENDER_NOT_APPLICABLE);
    } else {
        if (CGMinimapFrame::s_trackingSpell) {
            tooltip->SetBuff(CGMinimapFrame::s_trackingSpell, 1, 1, 0, 0, 0);
            tooltip->Show();
            return 0;
        }

        text = FrameScript_GetText("MINIMAP_TRACKING_TOOLTIP_NONE", -1, GENDER_NOT_APPLICABLE);
    }

    tooltip->AddLine(text, nullptr, reinterpret_cast<const CImVector&>(s_tooltipTitleColor), reinterpret_cast<const CImVector&>(s_tooltipTitleColor), 0);
    tooltip->Show();
    return 0;
}

// STATIC: 0x00AD2AE0
// VERIFY: b7d41ab7 auto
// OFFSET: 0x0061D3D0
static int32_t Script_SetAnchorType(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetAnchorType( anchorType [,Xoffset] [,Yoffset] )", name ? name : "<unnamed>");
    }

    int32_t anchorType = 0;

    if (lua_isstring(L, 2)) {
        const char* token = lua_tolstring(L, 2, nullptr);

        if (!SStrCmpI(token, "ANCHOR_LEFT", STORM_MAX_STR)) {
            anchorType = 0;
        } else if (!SStrCmpI(token, "ANCHOR_RIGHT", STORM_MAX_STR)) {
            anchorType = 1;
        } else if (!SStrCmpI(token, "ANCHOR_BOTTOMLEFT", STORM_MAX_STR)) {
            anchorType = 2;
        } else if (!SStrCmpI(token, "ANCHOR_BOTTOM", STORM_MAX_STR)) {
            anchorType = 3;
        } else if (!SStrCmpI(token, "ANCHOR_BOTTOMRIGHT", STORM_MAX_STR)) {
            anchorType = 4;
        } else if (!SStrCmpI(token, "ANCHOR_TOPLEFT", STORM_MAX_STR)) {
            anchorType = 5;
        } else if (!SStrCmpI(token, "ANCHOR_TOP", STORM_MAX_STR)) {
            anchorType = 6;
        } else if (!SStrCmpI(token, "ANCHOR_TOPRIGHT", STORM_MAX_STR)) {
            anchorType = 7;
        } else if (!SStrCmpI(token, "ANCHOR_CURSOR", STORM_MAX_STR)) {
            anchorType = 8;
        } else if (!SStrCmpI(token, "ANCHOR_NONE", STORM_MAX_STR)) {
            anchorType = 9;
        } else if (!SStrCmpI(token, "ANCHOR_PRESERVE", STORM_MAX_STR)) {
            anchorType = 10;
        } else if (!SStrCmpI(token, "ANCHOR_CURSOR_RIGHT", STORM_MAX_STR)) {
            anchorType = 11;
        }
    }

    float offsetX = 0.0f;

    if (lua_isnumber(L, 3)) {
        float value = static_cast<float>(lua_tonumber(L, 3));
        offsetX = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
    }

    float offsetY;

    if (lua_isnumber(L, 4)) {
        float value = static_cast<float>(lua_tonumber(L, 4));
        offsetY = NDCToDDCWidth(value / (CoordinateGetAspectCompensation() * 1024.0f));
    } else {
        offsetY = 0.0f;
    }

    tooltip->m_offsetX = offsetX;
    tooltip->m_offsetY = offsetY;

    if (tooltip->m_owner) {
        tooltip->m_anchorType = anchorType;
    }

    tooltip->ResetPosition(1);
    return 0;
}

// VERIFY: 16f43b69 manual
// OFFSET: 0x0061DAD0
static int32_t Script_GetUnit(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    WOWGUID guid = tooltip->m_unitGuid;
    const char* token = Script_GetTokenFromGUID(&guid);
    const char* name = nullptr;

    if (token && Script_GetNameFromToken(token, &name, true, false)) {
        lua_pushstring(L, name);
        lua_pushstring(L, token);
        return 2;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(tooltip->m_unitGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return 0;
    }

    lua_pushstring(L, unit->GetUnitName(nullptr, 1));
    lua_pushstring(L, token);
    return 2;
}

// VERIFY: 209cfc2d auto
// OFFSET: 0x0061F0F0
static int32_t Script_GetSpell(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    int32_t count = 0;
    SpellRec spell;

    if (g_spellDB.GetRecord(tooltip->m_spellId, &spell)) {
        lua_pushstring(L, spell.m_name);
        lua_pushstring(L, spell.m_nameSubtext);
        lua_pushnumber(L, tooltip->m_spellId);
        count = 3;
    }

    if (g_spellDB.GetRecord(tooltip->m_nextRankSpellId, &spell)) {
        lua_pushstring(L, spell.m_name);
        lua_pushstring(L, spell.m_nameSubtext);
        lua_pushnumber(L, tooltip->m_nextRankSpellId);
        count += 3;
    }

    return count;
}

// VERIFY: 9905ae95 manual
// OFFSET: 0x006201F0
static int32_t Script_AddFontStrings(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    CSimpleFontString* strings[2];

    for (int32_t i = 2; i - 2 < 2; i++) {
        if (lua_type(L, i) != LUA_TTABLE) {
            const char* name = tooltip->GetName();
            luaL_error(L, "Usage: %s:AddFontStrings(leftstring, rightstring)", name ? name : "<unnamed>");
        }

        lua_rawgeti(L, i, 0);
        CSimpleFontString* string = static_cast<CSimpleFontString*>(lua_touserdata(L, -1));
        lua_settop(L, -2);

        if (!string) {
            const char* name = tooltip->GetName();
            luaL_error(L, "%s:AddFontStrings(): Couldn't find 'this' in fontstring", name ? name : "<unnamed>");
        }

        if (!string->IsA(CSimpleFontString::GetObjectType())) {
            const char* name = tooltip->GetName();
            luaL_error(L, "%s:AddFontStrings(): Wrong object type, expected fontstring", name ? name : "<unnamed>");
        }

        strings[i - 2] = string;
    }

    tooltip->AddFontStrings(strings[0], strings[1]);
    return 0;
}

// VERIFY: 263db1d7 manual
// OFFSET: 0x006205C0
static int32_t Script_SetTotem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetTotem(slot)", name ? name : "<unnamed>");
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2));
    TotemInfo* totem = CGGameUI::GetTotemInfo(slot - 1);

    if (!totem || !totem->m_guid) {
        return 0;
    }

    tooltip->ClearTooltip();
    tooltip->AddLine(totem->m_name, nullptr, *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor), *reinterpret_cast<const CImVector*>(&s_tooltipTitleColor), 0);
    char buffer[1024];
    CGTooltip::GetDurationString(buffer, sizeof(buffer), totem->GetSpawnTimeLeft(), "SPELL_TIME_REMAINING", nullptr, 1, 0);
    tooltip->AddLine(buffer, nullptr, *reinterpret_cast<const CImVector*>(&s_tooltipTextColor), *reinterpret_cast<const CImVector*>(&s_tooltipTextColor), 0);
    tooltip->Show();
    return 0;
}

// VERIFY: d1c75157 manual
// OFFSET: 0x00625470
static int32_t Script_SetPetAction(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetPetAction(slot)", name ? name : "<unnamed>");
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;
    PetAction* action = CGPetInfo::GetPet(0) && slot < 10 ? &CGPetInfo::m_actions[slot] : nullptr;

    if (!action || !action->m_data) {
        lua_pushnil(L);
        return 1;
    }

    uint32_t type = (action->m_data >> 24) & 0x3F;
    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    int32_t result;

    if ((type == 1 || type - 8 <= 9) && (Spell_C_GetSpellCooldown(action->m_data & 0xFFFFFF, 1, &duration, &start, &enable), enable) && start && duration) {
        int32_t spellId = action->m_data & 0xFFFFFF;
        result = tooltip->SetSpell(spellId, 0, start + duration - OsGetAsyncTimeMs(), 1, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    } else {
        result = tooltip->SetSpell(action->m_data & 0xFFFFFF, 0, 0, 1, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    }

    if (result) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: a1f2ad30 manual
// OFFSET: 0x006257C0
static int32_t Script_SetPossession(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetPossession(slot)", name ? name : "<unnamed>");
    }

    if (static_cast<int64_t>(lua_tonumber(L, 2)) == 1) {
        uint32_t enable = 0;
        uint32_t start = 0;
        uint32_t duration = 0;
        Spell_C_GetSpellCooldown(CGPetInfo::m_possessSpell, 0, &duration, &start, &enable);
        int32_t brief = 1;
        CVar* uberTooltips = CVar::Lookup("UberTooltips");

        if (uberTooltips && uberTooltips->m_intValue) {
            brief = 0;
        }

        int32_t result;

        if (enable && start && duration) {
            int32_t spellId = CGPetInfo::m_possessSpell;
            result = tooltip->SetSpell(spellId, brief, start + duration - OsGetAsyncTimeMs(), 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        } else {
            result = tooltip->SetSpell(CGPetInfo::m_possessSpell, brief, 0, 0, 1, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        }

        if (result) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c5678e1d auto
// OFFSET: 0x00625E10
static int32_t Script_SetUnit(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetUnit(\"unit\"[, hideStatus])", name ? name : "<unnamed>");
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 2, nullptr));
    int32_t hideStatus = StringToBOOL(L, 3, 0);
    WOWGUID none;

    if (guid && guid.m_guid != 0xFFFFFFFFFFFFFFFEULL && (none = WOWGUID(), tooltip->SetUnit(guid, 1, none, hideStatus))) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: d9810071 auto
// OFFSET: 0x006262C0
static int32_t Script_SetUnitBuff(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    ScriptAuraInfo info;

    if (!Script_GetUnitAuraInfo(L, 2, &info)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetUnitBuff(\"unit\", [index] or [\"name\", \"rank\"][, \"filter\"])", name ? name : "<unnamed>");
    }

    info.m_filter = (info.m_filter & 0xFC) | BUFF_FILTER_HELPFUL;
    return SetTooltipUnitAura(tooltip, &info);
}

// VERIFY: 1ceddf4b auto
// OFFSET: 0x00626350
static int32_t Script_SetUnitDebuff(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    ScriptAuraInfo info;

    if (!Script_GetUnitAuraInfo(L, 2, &info)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetUnitDebuff(\"unit\", [index] or [\"name\", \"rank\"][, \"filter\"])", name ? name : "<unnamed>");
    }

    info.m_filter = (info.m_filter & 0xFC) | BUFF_FILTER_HARMFUL;
    return SetTooltipUnitAura(tooltip, &info);
}

// VERIFY: d2a5d303 manual
// OFFSET: 0x006263E0
static int32_t Script_SetQuestRewardSpell(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (CGQuestInfo::m_rewardSpell) {
        tooltip->SetSpell(CGQuestInfo::m_rewardSpell, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    }

    return 0;
}

// VERIFY: 3f811e34 auto
// OFFSET: 0x00626440
static int32_t Script_SetQuestLogRewardSpell(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    WOWGUID none = WOWGUID();
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest && quest->m_rewardSpellId) {
        tooltip->SetSpell(quest->m_rewardSpellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    }

    return 0;
}

// VERIFY: d655aaef manual
// OFFSET: 0x0062E670
static int32_t Script_SetQuestItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    const char* type;
    uint32_t index;
    int32_t itemId;

    if (!lua_isstring(L, 2) || !lua_isnumber(L, 3) || (type = lua_tolstring(L, 2, nullptr), index = static_cast<uint32_t>(lua_tonumber(L, 3)) - 1, itemId = CGQuestInfo::GetQuestItemID(type, index), itemId <= 0)) {
        luaL_error(L, "Invalid quest item in SetQuestItem(\"type\", index)");
        return 0;
    }

    char texture[260];
    char name[256];
    int32_t count;
    int32_t quality;
    int32_t usable;
    CGQuestInfo::QuestItemInfo(type, index, name, sizeof(name), texture, sizeof(texture), &count, &quality, &usable);
    tooltip->m_itemLinkInfo.Clear();
    WOWGUID itemGuid = WOWGUID();
    tooltip->m_itemLinkInfo.m_stackCount = count;
    tooltip->SetItem(itemId, &CGQuestInfo::m_npc, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 40524ae9 manual
// OFFSET: 0x0062E790
static int32_t Script_SetQuestLogItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    const char* type;
    int32_t index;
    int32_t itemId;

    if (!lua_isstring(L, 2) || !lua_isnumber(L, 3) || (type = lua_tolstring(L, 2, nullptr), index = static_cast<int32_t>(lua_tonumber(L, 3)) - 1, itemId = GetQuestItemRewards(type, index), itemId <= 0)) {
        luaL_error(L, "Invalid quest item in SetQuestLogItem(\"type\", index)");
        return 0;
    }

    tooltip->m_itemLinkInfo.Clear();
    WOWGUID none = WOWGUID();
    QuestCache* quest = g_questDBCache.GetRecord(CGQuestLog::m_selectedQuest, none, nullptr, nullptr, false);

    if (quest && index >= 0) {
        if (!SStrCmpI(type, "reward", STORM_MAX_STR) && index < 4) {
            tooltip->m_itemLinkInfo.m_stackCount = quest->m_rewardItemCount[index];
        } else if (!SStrCmpI(type, "choice", STORM_MAX_STR) && index < 6) {
            tooltip->m_itemLinkInfo.m_stackCount = quest->m_rewardChoiceItemCount[index];
        }
    }

    WOWGUID itemGuid = WOWGUID();
    WOWGUID requester = WOWGUID();
    tooltip->SetItem(itemId, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 0d337e4c auto
// OFFSET: 0x0062E900
static int32_t Script_SetTrainerService(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    TrainerServiceInfo* service;

    if (!lua_isnumber(L, 2) || (service = CGClassTrainer::GetService(static_cast<int32_t>(lua_tonumber(L, 2)) - 1)) == nullptr) {
        luaL_error(L, "Invalid trainer service in SetTrainerService(index)");
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(service->m_spellID, &spell)) {
        luaL_error(L, "Invalid trainer service in SetTrainerService(index)");
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (spell.m_effect[i] != SPELL_EFFECT_LEARN_SPELL && spell.m_effect[i] != SPELL_EFFECT_LEARN_PET_SPELL) {
            continue;
        }

        SpellRec learned;

        if (!g_spellDB.GetRecord(spell.m_effectTriggerSpell[i], &learned)) {
            continue;
        }

        if ((learned.m_attributes & 0x20) && (learned.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || learned.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || learned.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && learned.m_effectItemType[0]) {
            WOWGUID requester;
            requester.m_guid = static_cast<uint64_t>(static_cast<int64_t>(learned.m_id)) | 0x1FE0000000000000ULL;
            WOWGUID itemGuid = WOWGUID();
            tooltip->SetItem(learned.m_effectItemType[0], &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
        } else {
            tooltip->SetSpell(learned.m_id, 0, 0, spell.m_effect[i] == SPELL_EFFECT_LEARN_PET_SPELL, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
        }

        return 0;
    }

    tooltip->SetSpell(spell.m_id, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    return 0;
}

// VERIFY: 0c5a503b manual
// OFFSET: 0x0062EAE0
static int32_t Script_SetTradeSkillItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    uint32_t index;
    TradeSkillInfo* skill;

    if (!lua_isnumber(L, 2) || (index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1, index >= CGTradeSkillInfo::m_numDisplayEntries) || (skill = CGTradeSkillInfo::m_displayList.m_data[index]) == nullptr) {
        luaL_error(L, "Invalid trade skill item in SetTradeSkillItem(index [,reagent])");
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(skill->m_spellId, &spell)) {
        luaL_error(L, "Invalid trade skill item in SetTradeSkillItem(index [,reagent])");
    }

    if (lua_isnumber(L, 3)) {
        int32_t reagent = static_cast<int32_t>(lua_tonumber(L, 3));
        int32_t count = 0;
        uint32_t i;

        for (i = 0; i < 8; i++) {
            if (spell.m_reagent[i] && ++count == reagent) {
                break;
            }
        }

        if (i >= 8) {
            luaL_error(L, "Invalid trade skill item in SetTradeSkillItem(index [,reagent])");
        }

        tooltip->m_itemLinkInfo.Clear();
        tooltip->m_itemLinkInfo.m_stackCount = spell.m_reagentCount[i];
        WOWGUID requester;
        requester.m_guid = static_cast<uint64_t>(static_cast<int64_t>(skill->m_spellId)) | 0x1FE0000000000000ULL;
        WOWGUID itemGuid = WOWGUID();
        tooltip->SetItem(spell.m_reagent[i], &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
        return 0;
    }

    if ((spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM || spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM_2 || spell.m_effect[0] == SPELL_EFFECT_CREATE_RANDOM_ITEM) && spell.m_effectItemType[0]) {
        int32_t minPoints;
        int32_t maxPoints;
        Spell_C_GetMinMaxPoints(&spell, 0, &minPoints, &maxPoints, 0, 0, 0, 0);
        tooltip->m_itemLinkInfo.Clear();
        tooltip->m_itemLinkInfo.m_stackCount = minPoints;
        tooltip->m_itemLinkInfo.m_maxStackCount = maxPoints;

        if (spell.m_effect[0] == SPELL_EFFECT_CREATE_ITEM) {
            tooltip->m_itemLinkInfo.m_spellCharges = CGItem_C::NumBaseCharges(spell.m_effectItemType[0]);
        }

        WOWGUID requester;
        requester.m_guid = static_cast<uint64_t>(static_cast<int64_t>(skill->m_spellId)) | 0x1FE0000000000000ULL;
        WOWGUID itemGuid = WOWGUID();
        tooltip->SetItem(spell.m_effectItemType[0], &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
        return 0;
    }

    tooltip->SetSpell(skill->m_spellId, 0, 0, 0, 0, 0, 0, nullptr, nullptr, 0, 0, -1, -1, 0, 0);
    return 0;
}

// VERIFY: be3a2c61 manual
// OFFSET: 0x0062ED70
static int32_t Script_SetMerchantItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Invalid merchant slot in SetMerchantItem");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);
    MerchantItem* item;

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant || (item = &CGMerchantInfo::m_items[index]) == nullptr || !item->m_itemID) {
        return 0;
    }

    WOWGUID requester = CGMerchantInfo::m_merchant;
    WOWGUID itemGuid = WOWGUID();
    tooltip->SetItem(item->m_itemID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
    return 0;
}

// VERIFY: d770233b manual
// OFFSET: 0x0062EE70
static int32_t Script_SetMerchantCostItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Invalid merchant slot in SetMerchantCostItem");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);
    int32_t costIndex = static_cast<int32_t>(lua_tonumber(L, 3) - 1.0);
    MerchantItem* item;
    ItemExtendedCostRec* cost;

    if (index < 0 || index >= CGMerchantInfo::m_numMerchantItems || !CGMerchantInfo::m_merchant || (item = &CGMerchantInfo::m_items[index]) == nullptr || !item->m_itemID || (cost = g_itemExtendedCostDB.GetRecord(item->m_extendedCostID)) == nullptr) {
        return 0;
    }

    int32_t found = 0;

    for (uint32_t i = 0; i < 5; i++) {
        if (!cost->m_itemID[i]) {
            continue;
        }

        if (found != costIndex) {
            found++;
            continue;
        }

        WOWGUID requester = CGMerchantInfo::m_merchant;
        WOWGUID itemGuid = WOWGUID();
        tooltip->SetItem(cost->m_itemID[i], &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
        return 0;
    }

    return 0;
}

// VERIFY: 6906e91a manual
// OFFSET: 0x0062EFF0
static int32_t Script_SetTradePlayerItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Invalid trade slot in SetTradePlayerItem");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);
    WOWGUID guid = slot > 6 ? WOWGUID() : CGTradeInfo::m_playerItems[slot];
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return 0;
    }

    ItemLinkInfo& link = tooltip->m_itemLinkInfo;
    link.Clear();
    memset(link.m_enchants, 0, sizeof(link.m_enchants));
    memset(link.m_enchantDurations, 0, sizeof(link.m_enchantDurations));
    memset(link.m_enchantCharges, 0, sizeof(link.m_enchantCharges));

    for (uint32_t i = 0; i < 12; i++) {
        link.m_enchants[i] = 0;
        link.m_enchantDurations[i] = 0;
        link.m_enchantCharges[i] = 0;
    }

    link.m_cooldown = 0;
    const CGItemData* mirror = item->m_item;
    link.m_creator = mirror->m_itemFieldCreator;
    link.m_wrapped = (mirror->m_itemFieldFlags >> 3) & 1;
    link.m_giftCreator = mirror->m_itemFieldGiftcreator;
    int32_t onUseSlot = item->GetOnUseSlot();

    if (onUseSlot >= 0) {
        link.m_spellCharges = mirror->m_itemFieldSpellCharges[onUseSlot];
    }

    link.m_proposedEnchant = slot == 6 ? Trade_C_GetEnchantSpell(1) : 0;
    WOWGUID itemGuid = item->m_obj->m_guid;
    WOWGUID requester = item->m_obj->m_guid;
    tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, onUseSlot < 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 75ef8454 manual
// OFFSET: 0x0062F1E0
static int32_t Script_SetTradeTargetItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Invalid trade slot in SetTradeTargetItem");
        return 0;
    }

    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);

    if (slot > 6) {
        return 0;
    }

    int32_t itemId = CGTradeInfo::m_targetItemID[slot];

    if (itemId <= 0) {
        return 0;
    }

    ItemLinkInfo& link = tooltip->m_itemLinkInfo;
    link.Clear();
    memset(link.m_enchants, 0, sizeof(link.m_enchants));
    memset(link.m_enchantDurations, 0, sizeof(link.m_enchantDurations));
    memset(link.m_enchantCharges, 0, sizeof(link.m_enchantCharges));
    link.m_enchants[0] = CGTradeInfo::m_targetEnchant[slot];
    link.m_enchants[2] = CGTradeInfo::m_targetSocketEnchants[slot][0];
    link.m_enchantDurations[2] = 0;
    link.m_enchantCharges[2] = 0;
    link.m_enchants[3] = CGTradeInfo::m_targetSocketEnchants[slot][1];
    link.m_enchantDurations[3] = 0;
    link.m_enchantCharges[3] = 0;
    link.m_enchants[4] = CGTradeInfo::m_targetSocketEnchants[slot][2];
    link.m_enchantDurations[4] = 0;
    link.m_enchantCharges[4] = 0;
    link.m_cooldown = 0;
    link.m_proposedEnchant = 0;
    link.m_creator = CGTradeInfo::m_targetCreator[slot];
    link.m_wrapped = CGTradeInfo::m_targetWrapped[slot];
    link.m_lockId = CGTradeInfo::m_targetLockID[slot];
    link.m_giftCreator = CGTradeInfo::m_targetGiftCreator[slot];
    link.m_propertySeed = CGTradeInfo::m_targetSuffixFactor[slot];
    link.m_randomPropertiesId = CGTradeInfo::m_targetRandomPropertyID[slot];
    link.m_spellCharges = CGTradeInfo::m_targetSpellCharges[slot];
    link.m_proposedEnchant = slot == 6 ? Trade_C_GetEnchantSpell(0) : 0;
    link.m_maxDurability = CGTradeInfo::m_targetMaxDurability[slot];
    link.m_durability = CGTradeInfo::m_targetDurability[slot];
    link.m_stackCount = CGTradeInfo::m_targetCount[slot];
    WOWGUID requester = CGTradeInfo::m_tradingPlayer;
    WOWGUID itemGuid = WOWGUID();
    tooltip->SetItem(itemId, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    return 0;
}

// VERIFY: 7939b284 manual
// OFFSET: 0x0062F8B0
static int32_t Script_SetSendMailItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    uint32_t index;

    if (lua_isnumber(L, 2)) {
        index = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);

        if (index >= 16) {
            lua_pushnil(L);
            return 1;
        }
    } else {
        index = 0;
    }

    WOWGUID guid = CGMailInfo::m_sendItems[index].m_guid;

    if (guid) {
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

        if (item && tooltip->SetItem(item->m_obj->m_entryID, &guid, &guid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 117fc8ce manual
// OFFSET: 0x0062F9E0
static int32_t Script_SetInboxItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetInboxItem(messageIndex, attachmentIndex)", name ? name : "<unnamed>");
    }

    int32_t messageIndex = static_cast<int32_t>(lua_tonumber(L, 2));
    MailboxMessageInfo* message;

    if (static_cast<uint32_t>(messageIndex - 1) >= CGMailInfo::m_numInboxItems || (message = CGMailInfo::m_inbox.m_data[messageIndex - 1]) == nullptr) {
        lua_pushnil(L);
        return 1;
    }

    MailAttachmentInfo* attachment;
    int32_t attachmentIndex;

    if (lua_isnumber(L, 3) && (attachmentIndex = static_cast<int32_t>(lua_tonumber(L, 3))) != 0) {
        if (static_cast<uint32_t>(attachmentIndex - 1) >= 16) {
            lua_pushnil(L);
            return 1;
        }

        attachment = &message->m_attachments[attachmentIndex - 1];
    } else {
        attachment = GetFirstAttachment(message);
    }

    if (attachment && attachment->m_itemID) {
        ItemLinkInfo& link = tooltip->m_itemLinkInfo;
        link.Clear();

        for (uint32_t i = 0; i < 12; i++) {
            if (i >= 7) {
                link.m_enchants[i] = 0;
                link.m_enchantDurations[i] = 0;
                link.m_enchantCharges[i] = 0;
            } else {
                link.m_enchants[i] = attachment->m_enchantIDs[i];
                link.m_enchantDurations[i] = attachment->m_enchantDurations[i];
                link.m_enchantCharges[i] = attachment->m_enchantCharges[i];
            }
        }

        link.m_cooldown = 0;
        link.m_proposedEnchant = 0;
        link.m_creator = WOWGUID();
        link.m_wrapped = 0;
        link.m_giftCreator = WOWGUID();
        link.m_spellCharges = attachment->m_spellCharges;
        link.m_propertySeed = attachment->m_suffixFactor;
        link.m_randomPropertiesId = attachment->m_randomPropertiesID;
        link.m_maxDurability = attachment->m_maxDurability;
        link.m_durability = attachment->m_durability;
        link.m_stackCount = attachment->m_count;
        WOWGUID itemGuid = WOWGUID();
        WOWGUID requester = WOWGUID();

        if (tooltip->SetItem(attachment->m_itemID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, attachment->unk_0078, 0)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: fc10993a manual
// OFFSET: 0x0062FC20
static int32_t Script_SetAuctionSellItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    WOWGUID item = CGAuctionHouse::m_sellItem;
    CGItem_C* object;

    if (CGAuctionHouse::m_sellItem && (object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGAuctionHouse::m_sellItem, TYPE_ITEM, __FILE__, __LINE__))) != nullptr && tooltip->SetItem(object->m_obj->m_entryID, &item, &item, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: e54be545 manual
// OFFSET: 0x0062FCF0
static int32_t Script_SetAuctionItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2) || !lua_isnumber(L, 3)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetAuctionItem(\"type\", index)", name ? name : "<unnamed>");
    }

    const char* type = lua_tolstring(L, 2, nullptr);
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 3)) - 1;
    AuctionItemInfo* auction;

    if (!SStrCmpI(type, "owner", STORM_MAX_STR)) {
        if (index >= CGAuctionHouse::m_ownerList.m_count) {
            lua_pushnil(L);
            return 1;
        }

        auction = &CGAuctionHouse::m_ownerList.m_data[index];
    } else if (!SStrCmpI(type, "bidder", STORM_MAX_STR)) {
        auction = GetBidderAuction(index);
    } else if (!SStrCmpI(type, "list", STORM_MAX_STR)) {
        auction = GetListAuction(index);
    } else {
        lua_pushnil(L);
        return 1;
    }

    if (auction && auction->m_itemID) {
        ItemLinkInfo& link = tooltip->m_itemLinkInfo;
        link.Clear();

        for (uint32_t i = 0; i < 12; i++) {
            if (i >= 7) {
                link.m_enchants[i] = 0;
                link.m_enchantDurations[i] = 0;
                link.m_enchantCharges[i] = 0;
            } else {
                link.m_enchants[i] = auction->m_enchantID[i];
                link.m_enchantDurations[i] = auction->m_enchantDuration[i];
                link.m_enchantCharges[i] = auction->m_enchantCharges[i];
            }
        }

        link.m_cooldown = 0;
        link.m_proposedEnchant = 0;
        link.m_creator = WOWGUID();
        link.m_wrapped = 0;
        link.m_giftCreator = WOWGUID();
        link.m_spellCharges = auction->m_spellCharges;
        link.m_propertySeed = auction->m_suffixFactor;
        link.m_randomPropertiesId = auction->m_randomPropertyID;
        link.m_stackCount = auction->m_count;
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(auction->m_itemID, none, nullptr, nullptr, false);
        link.m_lockId = !stats || (auction->m_flags & 0x4) ? 0 : stats->m_lockId;
        WOWGUID itemGuid = WOWGUID();
        WOWGUID requester = WOWGUID();

        if (tooltip->SetItem(auction->m_itemID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1)) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: dc74da5e manual
// OFFSET: 0x006301A0
static int32_t Script_SetSocketedItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItemSocketInfo::m_socketItem, TYPE_ITEM, __FILE__, __LINE__));

    if (item) {
        WOWGUID itemGuid = item->m_obj->m_guid;
        WOWGUID requester = WOWGUID();
        tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 1, nullptr, 0, nullptr, 0, 0, 0);
    }

    return 0;
}

// VERIFY: 83e79f79 manual
// OFFSET: 0x00630250
static int32_t Script_SetSocketGem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetSocketGem(index)", name ? name : "<unnamed>");
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    WOWGUID gem = index >= 3 ? WOWGUID() : CGItemSocketInfo::m_gemItems[index];
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(gem, TYPE_ITEM, __FILE__, __LINE__));

    if (item) {
        WOWGUID itemGuid = item->m_obj->m_guid;
        WOWGUID requester = WOWGUID();
        tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    }

    return 0;
}

// VERIFY: 32a676a6 manual
// OFFSET: 0x00630370
static int32_t Script_SetExistingSocketGem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetSocketGem(index, [toDestroy])", name ? name : "<unnamed>");
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    int32_t toDestroy = StringToBOOL(L, 3, 0);
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItemSocketInfo::m_socketItem, TYPE_ITEM, __FILE__, __LINE__));

    if (item && index < 3) {
        SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(item->GetEnchantmentId(index + 2));

        if (enchant && enchant->m_srcItemID) {
            WOWGUID itemGuid = WOWGUID();
            WOWGUID requester = WOWGUID();
            tooltip->SetItem(enchant->m_srcItemID, &requester, &itemGuid, 0, toDestroy, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
        }
    }

    return 0;
}

// VERIFY: e89f31ca auto
// OFFSET: 0x006304A0
static int32_t Script_SetGuildBankItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Invalid tab or slot in SetGuildBankItem");
        return 0;
    }

    uint32_t tab = static_cast<int32_t>(lua_tonumber(L, 2) - 1.0);
    uint32_t slot = static_cast<int32_t>(lua_tonumber(L, 3) - 1.0);
    GuildBankSlotInfo* info = CGGuildBankInfo::GetGuildBankItem(tab, slot);

    if (!info) {
        return 0;
    }

    ItemLinkInfo& link = tooltip->m_itemLinkInfo;
    link.Clear();
    link.m_enchants[0] = info->m_enchantId;
    link.m_enchants[2] = info->m_enchants[0];
    link.m_enchants[3] = info->m_enchants[1];
    link.m_enchants[4] = info->m_enchants[2];
    link.m_propertySeed = info->m_suffixFactor;
    link.m_randomPropertiesId = info->m_randomPropertyID;
    link.m_spellCharges = static_cast<uint8_t>(info->m_spellCharges);
    link.m_stackCount = info->m_count;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        WOWGUID itemGuid = player->m_obj->m_guid;
        WOWGUID requester = WOWGUID();
        tooltip->SetItem(info->m_itemID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, info->m_flags & 0x4, 0);
    }

    return 0;
}

// VERIFY: 9d18ea8d manual
// OFFSET: 0x006307E0
static int32_t Script_SetBackpackToken(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetBackpackToken(index)", name ? name : "<unnamed>");
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CurrencyTypesRec* currency = CGCurrencyTypes::GetBackpackCurrencyInfo(static_cast<int32_t>(lua_tonumber(L, 2)) - 1);

    if (!tooltip || !player || !currency) {
        return 0;
    }

    for (uint32_t slot = 118; slot <= 149; slot++) {
        WOWGUID guid = static_cast<int32_t>(slot) < player->m_bag.m_numSlots ? player->m_bag.m_slots[slot] : WOWGUID();
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

        if (item && item->m_obj->m_entryID == currency->m_itemID) {
            WOWGUID itemGuid = item->m_obj->m_guid;
            WOWGUID requester = player->m_obj->m_guid;
            tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
            return 0;
        }
    }

    WOWGUID itemGuid = WOWGUID();
    WOWGUID requester = WOWGUID();
    tooltip->SetItem(currency->m_itemID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
    return 0;
}

// VERIFY: 0565efa8 manual
// OFFSET: 0x00630990
static int32_t Script_SetQuestLogSpecialItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetQuestLogSpecialItem(index)", name ? name : "<unnamed>");
    }

    CGItem_C* item = CGQuestLog::GetQuestSpecialItem(static_cast<int32_t>(lua_tonumber(L, 2)) - 1);

    if (item) {
        WOWGUID itemGuid = item->m_obj->m_guid;
        WOWGUID requester = ClntObjMgrGetActivePlayer();
        tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    }

    return 0;
}

// VERIFY: 7ded9ad2 manual
// OFFSET: [0x006264D0, 0x00626546]
static int32_t Script_SetEquipmentSet(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isstring(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetEquipmentSet(\"setName\")", name ? name : "<unnamed>");
        return 0;
    }

    EQUIPMENT_SET* set = CGEquipmentManager::GetSetByName(lua_tolstring(L, 2, nullptr));

    if (set) {
        tooltip->SetEquipmentSet(set->m_id);
    }

    return 0;
}

// VERIFY: 8d7263f7 manual
// OFFSET: 0x00626560
static int32_t Script_SetFrameStack(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (lua_toboolean(L, 2)) {
        tooltip->SetFrameStack(true);
    } else {
        tooltip->SetFrameStack(false);
    }

    return 0;
}

// VERIFY: 90a12da2 manual
// OFFSET: [0x0062FF60, 0x00630061]
static int32_t Script_SetBuybackItem(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetBuybackItem(slot)", name ? name : "<unnamed>");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    uint32_t slot = index < CGMerchantInfo::m_numBuybackItems ? CGMerchantInfo::m_buybackSlots[index] : 0;

    if (!player || !slot) {
        return 0;
    }

    const CGBag_C& bag = player->m_bag;

    if (slot >= static_cast<uint32_t>(bag.m_numSlots) || !bag.m_slots[slot]) {
        return 0;
    }

    const CGItem_C* item = bag.GetItemPointer(slot);

    if (item) {
        WOWGUID itemGuid = item->m_obj->m_guid;
        WOWGUID requester = player->m_obj->m_guid;
        tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    }

    return 0;
}

// VERIFY: bcd1e677 manual
// OFFSET: 0x00630620
static int32_t Script_SetCurrencyToken(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        const char* name = tooltip->GetName();
        luaL_error(L, "Usage: %s:SetCurrencyToken(index)", name ? name : "<unnamed>");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CurrencyListEntry* entry = CGCurrencyTypes::GetDisplayListInfo(static_cast<int32_t>(lua_tonumber(L, 2)) - 1);

    if (!tooltip || !player || !entry || entry->m_isHeader) {
        return 0;
    }

    for (uint32_t slot = 118; slot <= 149; slot++) {
        WOWGUID guid = slot < static_cast<uint32_t>(player->m_bag.m_numSlots) ? player->m_bag.m_slots[slot] : WOWGUID();
        CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(guid, TYPE_ITEM, __FILE__, __LINE__));

        if (item && item->m_obj->m_entryID == entry->m_itemID) {
            WOWGUID itemGuid = item->m_obj->m_guid;
            WOWGUID requester = player->m_obj->m_guid;
            tooltip->SetItem(item->m_obj->m_entryID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
            return 0;
        }
    }

    WOWGUID itemGuid = WOWGUID();
    WOWGUID requester = WOWGUID();
    tooltip->SetItem(entry->m_itemID, &requester, &itemGuid, 0, 0, 0, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 1);
    return 0;
}

// VERIFY: d3d02168 manual
// OFFSET: 0x00630A80
static int32_t Script_SetLFGDungeonReward(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: Tooltip:SetLFGDungeonReward(dungeonID, lootIndex)");
        return 0;
    }

    int32_t dungeonId = lua_tointeger(L, 2);
    int32_t index = lua_tointeger(L, 3) - 1;
    LfgDungeonsRec* dungeon = g_lfgDungeonsDB.GetRecord(dungeonId);

    if (!dungeon) {
        return 0;
    }

    LFGRewardItem* reward = CGLookingForGroup::GetDungeonRewardItem((dungeon->m_id & 0xFFFFFF) | (dungeon->m_typeID << 24), index);

    if (reward) {
        tooltip->m_itemLinkInfo.Clear();
        tooltip->m_itemLinkInfo.m_stackCount = reward->m_quantity;
        WOWGUID itemGuid = WOWGUID();
        WOWGUID requester = WOWGUID();
        tooltip->SetItem(reward->m_itemID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
    }

    return 0;
}

// VERIFY: d1656f9d manual
// OFFSET: 0x00630B90
static int32_t Script_SetLFGCompletionReward(lua_State* L) {
    CGTooltip* tooltip = static_cast<CGTooltip*>(FrameScript_GetObjectThis(L, CGTooltip::GetObjectType()));

    if (!lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: Tooltip:SetLFGCompletionReward(lootIndex)");
        return 0;
    }

    int32_t index = lua_tointeger(L, 2) - 1;

    for (LFGRewardItem* reward = CGLookingForGroup::m_reward.m_items.Head(); reward; reward = CGLookingForGroup::m_reward.m_items.Next(reward)) {
        if (!index--) {
            tooltip->m_itemLinkInfo.Clear();
            tooltip->m_itemLinkInfo.m_stackCount = reward->m_quantity;
            WOWGUID itemGuid = WOWGUID();
            WOWGUID requester = WOWGUID();
            tooltip->SetItem(reward->m_itemID, &requester, &itemGuid, 0, 0, 1, 0, WOWGUID(), 0, nullptr, 0, nullptr, 0, 0, 0);
            return 0;
        }
    }

    return 0;
}

static FrameScript_Method s_ScriptMethods[] = {
    { "AddFontStrings", &Script_AddFontStrings },
    { "SetMinimumWidth", &Script_SetMinimumWidth },
    { "GetMinimumWidth", &Script_GetMinimumWidth },
    { "SetPadding", &Script_SetPadding },
    { "GetPadding", &Script_GetPadding },
    { "IsOwned", &Script_IsOwned },
    { "GetOwner", &Script_GetOwner },
    { "SetOwner", &Script_SetOwner },
    { "GetAnchorType", &Script_GetAnchorType },
    { "SetAnchorType", &Script_SetAnchorType },
    { "ClearLines", &Script_ClearLines },
    { "AddLine", &Script_AddLine },
    { "AddDoubleLine", &Script_AddDoubleLine },
    { "AddTexture", &Script_AddTexture },
    { "SetText", &Script_SetText },
    { "AppendText", &Script_AppendText },
    { "FadeOut", &Script_FadeOut },
    { "SetHyperlink", &Script_SetHyperlink },
    { "SetAction", &Script_SetAction },
    { "SetPetAction", &Script_SetPetAction },
    { "SetShapeshift", &Script_SetShapeshift },
    { "SetPossession", &Script_SetPossession },
    { "SetTracking", &Script_TooltipSetTracking },
    { "SetSpell", &Script_SetSpell },
    { "SetSpellByID", &Script_SetSpellByID },
    { "SetGlyph", &Script_SetGlyph },
    { "SetInventoryItem", &Script_SetInventoryItem },
    { "SetLootItem", &Script_SetLootItem },
    { "SetQuestItem", &Script_SetQuestItem },
    { "SetQuestLogItem", &Script_SetQuestLogItem },
    { "SetTrainerService", &Script_SetTrainerService },
    { "SetTradeSkillItem", &Script_SetTradeSkillItem },
    { "SetMerchantItem", &Script_SetMerchantItem },
    { "SetMerchantCostItem", &Script_SetMerchantCostItem },
    { "SetTradePlayerItem", &Script_SetTradePlayerItem },
    { "SetTradeTargetItem", &Script_SetTradeTargetItem },
    { "SetBagItem", &Script_SetBagItem },
    { "SetUnit", &Script_SetUnit },
    { "SetUnitBuff", &Script_SetUnitBuff },
    { "SetUnitDebuff", &Script_SetUnitDebuff },
    { "SetUnitAura", &Script_SetUnitAura },
    { "SetTalent", &Script_SetTalent },
    { "SetSendMailItem", &Script_SetSendMailItem },
    { "SetInboxItem", &Script_SetInboxItem },
    { "SetAuctionSellItem", &Script_SetAuctionSellItem },
    { "SetAuctionItem", &Script_SetAuctionItem },
    { "NumLines", &Script_NumLines },
    { "SetQuestRewardSpell", &Script_SetQuestRewardSpell },
    { "SetQuestLogRewardSpell", &Script_SetQuestLogRewardSpell },
    { "SetHyperlinkCompareItem", &Script_SetHyperlinkCompareItem },
    { "SetBuybackItem", &Script_SetBuybackItem },
    { "SetLootRollItem", &Script_SetLootRollItem },
    { "SetSocketedItem", &Script_SetSocketedItem },
    { "SetSocketGem", &Script_SetSocketGem },
    { "SetExistingSocketGem", &Script_SetExistingSocketGem },
    { "SetGuildBankItem", &Script_SetGuildBankItem },
    { "IsUnit", &Script_IsUnit },
    { "GetUnit", &Script_GetUnit },
    { "GetItem", &Script_GetItem },
    { "GetSpell", &Script_GetSpell },
    { "SetTotem", &Script_SetTotem },
    { "SetCurrencyToken", &Script_SetCurrencyToken },
    { "SetBackpackToken", &Script_SetBackpackToken },
    { "IsEquippedItem", &Script_IsEquippedItem },
    { "SetQuestLogSpecialItem", &Script_SetQuestLogSpecialItem },
    { "SetEquipmentSet", &Script_SetEquipmentSet },
    { "SetFrameStack", &Script_SetFrameStack },
    { "SetLFGDungeonReward", &Script_SetLFGDungeonReward },
    { "SetLFGCompletionReward", &Script_SetLFGCompletionReward },
};

// VERIFY: 483706ce auto
// OFFSET: 0x0061B4A0
void CGTooltip::RegisterScriptMethods(lua_State* L) {
    CSimpleFrame::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_ScriptMethods, sizeof(s_ScriptMethods) / sizeof(s_ScriptMethods[0]));
}
