#include "CharacterSelection.h"
#include "WoW/Source/ModCore/ModCore.h"
#include <cstdint>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CSimpleModelFFX.h"
#include "WoW/Source/Glue/CharacterSelectionDisplay.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

// VERIFY: 769d9898 auto
// OFFSET: 0x004E2F60
static int32_t Script_SetCharSelectModelFrame(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetCharSelectModelFrame(\"frameName\")");
        return 0;
    }

    CSimpleModelFFX* frame = static_cast<CSimpleModelFFX*>(CScriptObject::GetScriptObjectByName(lua_tolstring(L, 1, nullptr), CSimpleModel::GetObjectType()));

    if (frame) {
        CCharacterSelection::m_modelFrame = frame;
    }

    return 0;
}

// VERIFY: 68a2a930 manual
// OFFSET: 0x004E3140
static int32_t Script_GetNumCharacters(lua_State* L) {
    lua_pushnumber(L, CCharacterSelection::s_characterList.Count());
    return 1;
}

// VERIFY: c24567ba auto
// OFFSET: 0x004E2FD0
static int32_t Script_UpdateSelectionCustomizationScene(lua_State* L) {
    if (static_cast<uint32_t>(CCharacterSelection::s_selectionIndex) < CCharacterSelection::s_characterList.Count()) {
        CCharacterComponent* component = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component;

        if (component) {
            component->RenderPrep(0);
        }
    }

    return 0;
}

// VERIFY: cc0ab9ef auto
// OFFSET: 0x004E3000
static int32_t Script_GetCharacterSelectFacing(lua_State* L) {
    lua_pushnumber(L, CCharacterSelection::s_charFacing * 57.29578f);
    return 1;
}

// VERIFY: 2d87106c auto
// OFFSET: 0x004E3030
static int32_t Script_SetCharacterSelectFacing(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetCharacterSelectFacing(degrees)");
        return 0;
    }

    CharacterSelectionDisplay::SetCharacterFacing(lua_tonumber(L, 1) * 0.017453292f);
    return 1;
}

// VERIFY: 19fca637 auto
// OFFSET: 0x004E3170
static int32_t Script_GetCharacterInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCharacterInfo(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 10;
    }

    CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList[index];

    lua_pushstring(L, display->m_characterInfo.m_name);

    const char* raceName = CGUnit_C::GetDisplayRaceNameFromRecord(g_chrRacesDB.GetRecord(display->m_characterInfo.m_raceID), display->m_characterInfo.m_sexID, nullptr);
    lua_pushstring(L, raceName ? raceName : "");

    const char* className = CGUnit_C::GetDisplayClassNameFromRecord(g_chrClassesDB.GetRecord(display->m_characterInfo.m_classID), display->m_characterInfo.m_sexID, nullptr);
    lua_pushstring(L, className ? className : "");

    lua_pushnumber(L, display->m_characterInfo.m_experienceLevel);

    AreaTableRec* areaRec = g_areaTableDB.GetRecord(display->m_characterInfo.m_zoneID);

    if (areaRec) {
        lua_pushstring(L, areaRec->m_areaName);
    } else {
        lua_pushnil(L);
    }

    lua_pushnumber(L, g_scriptSexIndex[display->m_characterInfo.m_sexID]);
    lua_pushboolean(L, display->m_characterInfo.m_flags & 0x2000);
    lua_pushboolean(L, display->m_characterInfo.m_customizeFlags & 0x1);
    lua_pushboolean(L, display->m_characterInfo.m_customizeFlags & 0x100000);
    lua_pushboolean(L, display->m_characterInfo.m_customizeFlags & 0x10000);
    return 10;
}

// VERIFY: 927337c9 auto
// OFFSET: 0x004E33A0
static int32_t Script_DeleteCharacter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DeleteCharacter(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index >= 0 && index < static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        CGlueMgr::DeleteCharacter(CCharacterSelection::s_characterList[index].m_characterInfo.m_guid);
    }

    return 0;
}

// VERIFY: a99b6877 auto
// OFFSET: 0x004E3410
static int32_t Script_RenameCharacter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: RenameCharacter(index, name)");
        return 0;
    }

    if (!lua_isstring(L, 2)) {
        return 0;
    }

    const char* name = lua_tolstring(L, 2, nullptr);

    if (!name || !*name) {
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        return 0;
    }

    CharacterSelectionDisplay* display = GetCharacterInfo(index);

    if (!display || !(display->m_characterInfo.m_flags & 0x4000)) {
        return 0;
    }

    if (!SStrCmpUTF8I(name, display->m_characterInfo.m_name, STORM_MAX_STR)) {
        FrameScript_SignalEvent(GLUE_EVENT_FORCE_RENAME_CHARACTER, "%s", "CHAR_CREATE_NAME_IN_USE");
        return 0;
    }

    if (CGlueMgr::RenameCharacter(GetCharacterInfo(index)->m_characterInfo.m_guid, name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: f6ef29a9 auto
// OFFSET: 0x004E3530
static int32_t Script_DeclineCharacter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DeclineCharacter(index, name1, name2, name3, name4, name5)");
        return 0;
    }

    const char* names[5];

    for (int32_t i = 0; i < 5; i++) {
        names[i] = lua_tolstring(L, i + 2, nullptr);

        if (!names[i] || !*names[i]) {
            return 0;
        }
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < 0 || index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        return 0;
    }

    CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList[index];

    if (!display || (display->m_characterInfo.m_flags & 0x2000000)) {
        return 0;
    }

    if (CGlueMgr::DeclineCharacter(display->m_characterInfo.m_guid, names)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 433a84a4 manual
// OFFSET: 0x004E3620
static int32_t Script_GetSelectBackgroundModel(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetSelectBackgroundModel(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (SFile::IsTrial()) {
        lua_pushstring(L, "CharacterSelect");
        return 1;
    }

    ChrRacesRec* raceRec;

    if (index >= 0 && index < static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        CharacterSelectionDisplay* display = CCharacterSelection::GetCharacterDisplay(index);

        if (display->m_characterInfo.m_classID == 6) {
            ChrClassesRec* classRec = g_chrClassesDB.GetRecord(6);

            if (classRec) {
                lua_pushstring(L, classRec->m_filename);
                return 1;
            }

            lua_pushstring(L, "");
            return 1;
        }

        uint32_t raceID = display->m_characterInfo.m_raceID;

        if (raceID == 7) {
            raceID = 3;
        } else if (raceID == 8) {
            raceID = 2;
        }

        raceRec = g_chrRacesDB.GetRecord(raceID);
    } else {
        raceRec = g_chrRacesDB.GetRecord(2);
    }

    if (raceRec) {
        lua_pushstring(L, raceRec->m_clientFileString);
        return 1;
    }

    lua_pushstring(L, "");
    return 1;
}

// VERIFY: bd068651 auto
// OFFSET: 0x004E44E0
static int32_t Script_SetCharSelectBackground(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetCharSelectBackground(\"filename\")");
        return 0;
    }

    CCharacterSelection::SetBackgroundModel(lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: e07b5113 auto
// OFFSET: 0x004E4530
static int32_t Script_GetCharacterListUpdate(lua_State* L) {
    CCharacterSelection::s_characterList.SetCount(0);

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(0);
    }

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(1);
    }

    CGlueMgr::GetCharacterList();
    return 0;
}

// VERIFY: 4842f40a auto
// OFFSET: 0x004E4580
static int32_t Script_SelectCharacter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SelectCharacter(index)");
        return 0;
    }

    int32_t index = static_cast<int32_t>(lua_tonumber(L, 1)) - 1;

    if (index < -1 || index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        index = -1;
    }

    index = index < 0 ? 0 : index;

    if (index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        index = 0;
    }

    CCharacterSelection::s_selectionIndex = index;
    CCharacterSelection::ShowCharacter();
    FrameScript_SignalEvent(GLUE_EVENT_UPDATE_SELECTED_CHARACTER, "%d", CCharacterSelection::s_selectionIndex + 1);
    ModCore::Manager().OnCharacterSelect(index);
    return 0;
}

// STATIC: 0x00AC4378
static FrameScript_Method s_scriptFunctions[] = {
    { "SetCharSelectModelFrame", &Script_SetCharSelectModelFrame },
    { "SetCharSelectBackground", &Script_SetCharSelectBackground },
    { "GetCharacterListUpdate", &Script_GetCharacterListUpdate },
    { "GetNumCharacters", &Script_GetNumCharacters },
    { "GetCharacterInfo", &Script_GetCharacterInfo },
    { "SelectCharacter", &Script_SelectCharacter },
    { "DeleteCharacter", &Script_DeleteCharacter },
    { "RenameCharacter", &Script_RenameCharacter },
    { "DeclineCharacter", &Script_DeclineCharacter },
    { "UpdateSelectionCustomizationScene", &Script_UpdateSelectionCustomizationScene },
    { "GetCharacterSelectFacing", &Script_GetCharacterSelectFacing },
    { "SetCharacterSelectFacing", &Script_SetCharacterSelectFacing },
    { "GetSelectBackgroundModel", &Script_GetSelectBackgroundModel },
};

// VERIFY: 55f0de4b auto
// OFFSET: 0x004E2570
void CharSelectRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_scriptFunctions) / sizeof(s_scriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_scriptFunctions[i].m_name, s_scriptFunctions[i].m_method);
    }
}

// VERIFY: 96271309 auto
// OFFSET: 0x004E25A0
void CharSelectUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_scriptFunctions) / sizeof(s_scriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_scriptFunctions[i].m_name);
    }
}
