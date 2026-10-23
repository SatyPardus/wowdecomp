#include "ItemTextFrame.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/Rec/PageTextMaterialRec.h"
#include "WoW/Source/Game/GameClient/NameCache.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"

// VERIFY: d3517090 auto
// OFFSET: 0x00589B50
static int32_t Script_ItemTextGetPage(lua_State* L) {
    lua_pushnumber(L, CGItemText::m_currentPage + 1);
    return 1;
}

// VERIFY: ad27c4fc manual
// OFFSET: 0x00589C40
static int32_t Script_ItemTextGetItem(lua_State* L) {
    const char* name = nullptr;
    CGObject_C* object = ClntObjMgrObjectPtr(CGItemText::m_itemGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        name = object->GetObjectName();
    }

    lua_pushstring(L, name);
    return 1;
}

// VERIFY: 4f9393fe auto
// OFFSET: 0x00589B90
static int32_t Script_ItemTextGetText(lua_State* L) {
    lua_pushstring(L, CGItemText::m_text);
    return 1;
}

// VERIFY: 8b69700f auto
// OFFSET: 0x00589D70
static int32_t Script_ItemTextHasNextPage(lua_State* L) {
    if (CGItemText::m_hasItemText || !CGItemText::m_pages.m_data[CGItemText::m_currentPage + 1]) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: debaf905 manual
// OFFSET: 0x0058A480
static int32_t Script_ItemTextGetCreator(lua_State* L) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGItemText::m_itemGUID, TYPE_ITEM, __FILE__, __LINE__));
    NameCache* name;

    if (item && item->m_item->m_itemFieldCreator && (name = g_nameDBCache.GetRecord(item->m_item->m_itemFieldCreator.m_guid, item->m_obj->m_guid, CGItemText::ItemTextCallback, nullptr, false))) {
        lua_pushstring(L, name->m_name);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: eebcc62e manual
// OFFSET: 0x00589C90
static int32_t Script_ItemTextGetMaterial(lua_State* L) {
    CGObject_C* object = ClntObjMgrObjectPtr(CGItemText::m_itemGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        int32_t material;

        if (object->m_obj->m_type & TYPE_ITEM) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);

            if (!stats) {
                lua_pushnil(L);
                return 1;
            }

            material = stats->m_pageMaterial;
        } else if (object->m_obj->m_type & TYPE_GAMEOBJECT) {
            material = static_cast<CGGameObject_C*>(object)->GetPageTextMaterial();
        } else {
            lua_pushnil(L);
            return 1;
        }

        if (material > 0) {
            PageTextMaterialRec* rec = g_pageTextMaterialDB.GetRecord(material);

            if (rec) {
                lua_pushstring(L, rec->m_name);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 68986f5f manual
// OFFSET: 0x0058A110
static int32_t Script_ItemTextPrevPage(lua_State* L) {
    if (CGItemText::m_itemGUID && !CGItemText::m_hasItemText && CGItemText::m_currentPage) {
        CGItemText::m_currentPage--;
        CGItemText::DisplayText(CGItemText::m_itemGUID, 1);
    }

    return 0;
}

// VERIFY: 97ca59f6 auto
// OFFSET: 0x0058A150
static int32_t Script_ItemTextNextPage(lua_State* L) {
    if (CGItemText::m_itemGUID && !CGItemText::m_hasItemText && CGItemText::m_pages.m_data[CGItemText::m_currentPage + 1] > 0) {
        CGItemText::m_currentPage++;
        CGItemText::DisplayText(CGItemText::m_itemGUID, 1);
    }

    return 0;
}

// VERIFY: 0804cf2f auto
// OFFSET: 0x0058A400
static int32_t Script_CloseItemText(lua_State* L) {
    WOWGUID none = WOWGUID();
    CGItemText::SetItem(none, 0);
    return 0;
}

// STATIC: 0x00ACEEC0
static FrameScript_Method s_ScriptFunctions[] = {
    { "ItemTextGetItem", &Script_ItemTextGetItem },
    { "ItemTextGetCreator", &Script_ItemTextGetCreator },
    { "ItemTextGetMaterial", &Script_ItemTextGetMaterial },
    { "ItemTextGetPage", &Script_ItemTextGetPage },
    { "ItemTextGetText", &Script_ItemTextGetText },
    { "ItemTextHasNextPage", &Script_ItemTextHasNextPage },
    { "ItemTextPrevPage", &Script_ItemTextPrevPage },
    { "ItemTextNextPage", &Script_ItemTextNextPage },
    { "CloseItemText", &Script_CloseItemText },
};

// VERIFY: 65411387 auto
// OFFSET: 0x00589BB0
void ItemTextRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: 7b892804 auto
// OFFSET: 0x00589BE0
void ItemTextUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
