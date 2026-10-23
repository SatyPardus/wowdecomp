#include "GameUI.h"
#include <cmath>
#include <cstdarg>
#include <cstring>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Event/CSizeEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/SimpleFrameScript.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/Console/ConsoleClient.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/CharTitlesRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ExhaustionRec.h"
#include "WoW/Source/DB/Rec/ItemClassRec.h"
#include "WoW/Source/DB/Rec/ItemSubClassRec.h"
#include "WoW/Source/DB/Rec/MapDifficultyRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Game/GameClient/ComSatClient.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CWOWClientStatus.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Object_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/SimpleScript.h"
#include "WoW/Source/Object/ObjectClient/Trade_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/AchievementInfo.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/Ui/ArenaTeamInfo.h"
#include "WoW/Source/Ui/AuctionHouse.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/BattlenetUI.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/CGBankInfo.h"
#include "WoW/Source/Ui/CGCooldown.h"
#include "WoW/Source/Ui/CGMinigameInfo.h"
#include "WoW/Source/Ui/CGQuestPOIFrame.h"
#include "WoW/Source/Ui/CGTabardCreationFrame.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/CharacterModelBase.h"
#include "WoW/Source/Ui/ChatBubbleFrame.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/ClassTrainerFrame.h"
#include "WoW/Source/Ui/CommentatorFrame.h"
#include "WoW/Source/Ui/ContainerFrame.h"
#include "WoW/Source/Ui/CurrencyTypes.h"
#include "WoW/Source/Ui/DanceStudio.h"
#include "WoW/Source/Ui/DressUpModelFrame.h"
#include "WoW/Source/Ui/DuelInfo.h"
#include "WoW/Source/Ui/EquipmentManager.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GlyphInfo.h"
#include "WoW/Source/Ui/GMSurveyInfo.h"
#include "WoW/Source/Ui/GMTicketInfo.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/GuildBankFrame.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include "WoW/Source/Ui/GuildRegistrar.h"
#include "WoW/Source/Ui/InstanceEncounter_C.h"
#include "WoW/Source/Ui/ItemSocketInfo.h"
#include "WoW/Source/Ui/ItemTextFrame.h"
#include "WoW/Source/Ui/KnowledgeBase.h"
#include "WoW/Source/Ui/LFGInfo.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/PaperDollInfoFrame.h"
#include "WoW/Source/Ui/PartyFrame.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "WoW/Source/Ui/PetitionInfo.h"
#include "WoW/Source/Ui/PetitionVendor.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/ReputationInfo.h"
#include "WoW/Source/Ui/RuneInfo.h"
#include "WoW/Source/Ui/Screen.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/SkillInfo.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/StableInfo.h"
#include "WoW/Source/Ui/TabardModelFrame.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/TaxiMapFrame.h"
#include "WoW/Source/Ui/TotemInfo.h"
#include "WoW/Source/Ui/TradeFrame.h"
#include "WoW/Source/Ui/TradeSkillFrame.h"
#include "WoW/Source/Ui/Tutorial.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/UILCD/LCD.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/Ui/VideoOptions.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowServices/WDataStore.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FriendList.h"
#include <cctype>
#include "WoW/Source/DB/Rec/GMTicketCategoryRec.h"
#include "WoW/Source/DB/Rec/ItemLimitCategoryRec.h"
#include "WoW/Source/DB/Rec/ItemRec.h"
#include "WoW/Source/DB/Rec/SpellItemEnchantmentRec.h"
#include "WoW/Source/Game/ValidateName.h"
#include <cfloat>
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/DB/Rec/PaperDollItemFrameRec.h"
#include "WoW/Source/Object/WGUID.h"
#include "GameVersion.h"

// STATIC: 0x00BD0CC4
static CWOWClientStatus* s_loadAddOnStatus;

// STATIC: 0x00BD0CC8
static int32_t s_loadAddOnDepth;

// STATIC: 0x00AC8074
static const char* s_instanceTypeTokens[5] = { "none", "party", "raid", "pvp", "arena" };

// VERIFY: 2dd9570a auto
// OFFSET: 0x005109D0
static int32_t Script_BeginTrade(lua_State* L) {
    Trade_C_BeginTrade();
    return 0;
}

// VERIFY: 3e32da49 auto
// OFFSET: 0x005109E0
static int32_t Script_CancelTrade(lua_State* L) {
    Trade_C_CancelTrade();
    return 0;
}

// VERIFY: 64da4b3d auto
// OFFSET: 0x00510A00
static int32_t Script_ForceQuit(lua_State* L) {
    ClientPostClose(0);
    return 0;
}

// VERIFY: f79cdb8d auto
// OFFSET: 0x00510B90
static int32_t Script_GetTimeToWellRested(lua_State* L) {
    lua_pushnil(L);
    return 1;
}

// VERIFY: 8710af84 auto
// OFFSET: 0x00510DC0
int32_t Script_RestoreVideoResolutionDefaults(lua_State* L) {
    ConsoleDeviceDefaults(OPTION_PAGE_0);
    return 0;
}

// VERIFY: f8aec6b2 auto
// OFFSET: 0x00510DD0
int32_t Script_RestoreVideoEffectsDefaults(lua_State* L) {
    ConsoleDeviceDefaults(OPTION_PAGE_1);
    return 0;
}

// VERIFY: e74fa653 auto
// OFFSET: 0x00511830
static int32_t Script_ResetCPUUsage(lua_State* L) {
    FrameScript_ResetCPUUsage();
    return 0;
}

// VERIFY: 572cb276 auto
// OFFSET: 0x00515560
static int32_t Script_TargetDirectionFinished(lua_State* L) {
    CGGameUI::m_directionalTarget = WOWGUID();
    return 0;
}

// VERIFY: 0f8c89fc auto
// OFFSET: 0x00517D20
static int32_t Script_DeclineLevelGrant(lua_State* L) {
    CGGameUI::m_pendingLevelGranter = WOWGUID();
    return 0;
}

// VERIFY: 8972f8b9 auto
// OFFSET: 0x00522F90
static int32_t Script_NotWhileDeadError(lua_State* L) {
    CGGameUI::DisplayError(ERR_PLAYER_DEAD);
    return 0;
}

// VERIFY: 1e86b873 auto
// OFFSET: 0x0050FE80
static int32_t Script_IsDebugBuild(lua_State* L) {
    lua_pushboolean(L, 0);
    return 1;
}

// VERIFY: 362d8ac1 auto
// OFFSET: 0x00510DE0
static int32_t Script_SetEuropeanNumbers(lua_State* L) {
    SetDecimalConversion(StringToBOOL(L, 1, 1));
    return 0;
}

// VERIFY: 8c53a6db auto
// OFFSET: 0x00510E00
static int32_t Script_GetRealmName(lua_State* L) {
    lua_pushstring(L, ClientServices::GetSelectedRealmName());
    return 1;
}

// VERIFY: 5e600bbc auto
// OFFSET: 0x00511510
static int32_t Script_UpdateAddOnMemoryUsage(lua_State* L) {
    lua_gcupdatesizes(L);
    return 0;
}

// VERIFY: 94964fee auto
// OFFSET: 0x00511640
static int32_t Script_UpdateAddOnCPUUsage(lua_State* L) {
    lua_gcupdatetimes(L);
    return 0;
}

// VERIFY: 201e323d auto
// OFFSET: 0x00511A40
static int32_t Script_ResetDisabledAddOns(lua_State* L) {
    LoadAddOnEnableState(ClientServices::GetCharacterName());
    return 0;
}

// VERIFY: dd437cf6 auto
// OFFSET: 0x0051A3B0
static int32_t Script_ClearCursor(lua_State* L) {
    CGGameUI::ClearCursor(1, 1);
    return 1;
}

// VERIFY: 82f766f4 auto
// OFFSET: 0x005101D0
static int32_t Script_SetWaterDetail(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetWaterDetail(value)");
        return 0;
    }

    return 0;
}

// VERIFY: 218d4185 auto
// OFFSET: 0x00510200
static int32_t Script_GetFarclip(lua_State* L) {
    lua_pushnumber(L, CVar::Lookup("farclip")->m_floatValue);
    return 1;
}

// VERIFY: 7624bb8f auto
// OFFSET: 0x005102B0
static int32_t Script_GetTexLodBias(lua_State* L) {
    lua_pushnumber(L, CVar::Lookup("texLodBias")->m_floatValue);
    return 1;
}

// VERIFY: 92892b19 auto
// OFFSET: 0x00510360
static int32_t Script_GetBaseMip(lua_State* L) {
    lua_pushnumber(L, 1.0 - CVar::Lookup("baseMip")->m_floatValue);
    return 1;
}

// VERIFY: 43339e6b auto
// OFFSET: 0x00511610
static int32_t Script_GetScriptCPUUsage(lua_State* L) {
    lua_pushnumber(L, FrameScript_GetScriptCPUUsage());
    return 1;
}

// VERIFY: 47e47f12 manual
// OFFSET: 0x00511FB0
static int32_t Script_IsDesaturateSupported(lua_State* L) {
    if (!CSimpleTexture::GetImageModePixelShader(ImageMode_Desaturate)) {
        return 0;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: cee80fa8 auto
// OFFSET: 0x00515570
static int32_t Script_GetZoneText(lua_State* L) {
    lua_pushstring(L, CGGameUI::m_zoneText ? CGGameUI::m_zoneText : "");
    return 1;
}

// VERIFY: 74ec3f6a auto
// OFFSET: 0x005155A0
static int32_t Script_GetRealZoneText(lua_State* L) {
    lua_pushstring(L, CGGameUI::m_realZoneText ? CGGameUI::m_realZoneText : "");
    return 1;
}

// VERIFY: 56b17cd5 manual
// OFFSET: 0x0051AA90
static int32_t Script_RepopMe(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->HandleRepopRequest(false);
    }

    return 0;
}

// VERIFY: 89954634 manual
// OFFSET: 0x0051AAC0
static int32_t Script_AcceptResurrect(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::AcceptResurrectRequest(1);
    }

    return 0;
}

// VERIFY: 9e0bca08 manual
// OFFSET: 0x0051AAF0
static int32_t Script_DeclineResurrect(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::AcceptResurrectRequest(0);
    }

    return 0;
}

// VERIFY: 8786f8a9 manual
// OFFSET: 0x0051ABA0
static int32_t Script_DeclineGroup(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::DeclineGroup();
    }

    return 0;
}

// VERIFY: 6ca6b3d8 manual
// OFFSET: 0x0051ABD0
static int32_t Script_AcceptGuild(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::AcceptGuild();
    }

    return 0;
}

// VERIFY: 12010dbc manual
// OFFSET: 0x0051AC00
static int32_t Script_DeclineGuild(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::DeclineGuild();
    }

    return 0;
}

// VERIFY: c7f92481 manual
// OFFSET: 0x0051AC30
static int32_t Script_AcceptArenaTeam(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::AcceptArenaTeam();
    }

    return 0;
}

// VERIFY: ad1300af manual
// OFFSET: 0x0051AC60
static int32_t Script_DeclineArenaTeam(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::DeclineArenaTeam();
    }

    return 0;
}

// VERIFY: 3d89e6f6 auto
// OFFSET: 0x00522950
static int32_t Script_DropCursorMoney(lua_State* L) {
    if (CGGameUI::m_cursorMoney && CGGameUI::m_hasControl && CGGameUI::m_cursorItemType == UICURSOR_MONEY) {
        CGGameUI::ClearCursor(1, 1);
    }

    return 0;
}

// VERIFY: b7f1abc8 auto
// OFFSET: 0x00608560
static int32_t Script_GetWaterDetail(lua_State* L) {
    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: c070e858 manual
// OFFSET: 0x00511C40
int32_t Script_GetBillingTimeRested(lua_State* L) {
    lua_pushnumber(L, ClientServices::Connection()->m_billingTimeRested);
    return 1;
}

// VERIFY: d99d63b5 auto
// OFFSET: 0x005150E0
static int32_t Script_Screenshot(lua_State* L) {
    ScrnScreenshot(&CGGameUI::HandleScreenshot, reinterpret_cast<uint8_t*>(ClientServices::GetClientStamp()), 88);
    return 0;
}

// VERIFY: e7e4ff0d auto
// OFFSET: 0x00515100
static int32_t Script_CursorHasItem(lua_State* L) {
    if (CGGameUI::m_cursorItemType == UICURSOR_ITEM || CGGameUI::m_cursorItemType == UICURSOR_AMMO) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: e744d727 auto
// OFFSET: 0x00515140
static int32_t Script_CursorHasSpell(lua_State* L) {
    if (CGGameUI::m_cursorItemType == UICURSOR_SPELL) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b1fc85d9 auto
// OFFSET: 0x00515180
static int32_t Script_CursorHasMacro(lua_State* L) {
    if (CGGameUI::m_cursorItemType == UICURSOR_MACRO) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2230e17f auto
// OFFSET: 0x005151C0
static int32_t Script_CursorHasMoney(lua_State* L) {
    if (CGGameUI::m_cursorItemType == UICURSOR_MONEY) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: fc971425 auto
// OFFSET: 0x00515750
static int32_t Script_GetInstanceDifficulty(lua_State* L) {
    lua_pushnumber(L, CGGameUI::m_instanceDifficulty + 1);
    return 1;
}

// VERIFY: a8667e56 auto
// OFFSET: 0x00516760
static int32_t Script_InCinematic(lua_State* L) {
    if (CGGameUI::m_inCinematic) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: acdd7b2e auto
// OFFSET: 0x00517D30
static int32_t Script_IsThreatWarningEnabled(lua_State* L) {
    if (CGGameUI::m_threatWarningEnabled) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 99d6c5bc auto
// OFFSET: 0x00512090
static int32_t Script_ConsoleAddMessage(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: ConsoleAddMessage(string)");
    }

    const char* text = lua_tolstring(L, 1, nullptr);

    if (text && *text) {
        ConsolePrintf("%s", text);
    }

    return 0;
}

// VERIFY: 612c638a auto
// OFFSET: 0x0050F830
static int32_t Script_FrameXML_Debug(lua_State* L) {
    int32_t level = FrameXML_GetDebugLevel();

    if (lua_isnumber(L, 1)) {
        level = static_cast<int32_t>(lua_tonumber(L, 1));
        FrameXML_SetDebugLevel(level);
    }

    lua_pushnumber(L, level);
    return 1;
}

// VERIFY: 4100f310 auto
// OFFSET: 0x005144C0
static int32_t Script_IsLeftShiftKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = CGUIBindings::s_bindings->m_heldModifiers & 1;
    } else {
        down = EventIsKeyDown(KEY_LSHIFT);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: c4e94562 auto
// OFFSET: 0x00514520
static int32_t Script_IsRightShiftKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = (CGUIBindings::s_bindings->m_heldModifiers >> 1) & 1;
    } else {
        down = EventIsKeyDown(KEY_RSHIFT);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 898d5169 auto
// OFFSET: 0x00514610
static int32_t Script_IsLeftControlKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = (CGUIBindings::s_bindings->m_heldModifiers >> 2) & 1;
    } else {
        down = EventIsKeyDown(KEY_LCONTROL);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 946c00ee auto
// OFFSET: 0x00514670
static int32_t Script_IsRightControlKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = (CGUIBindings::s_bindings->m_heldModifiers >> 3) & 1;
    } else {
        down = EventIsKeyDown(KEY_RCONTROL);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 81c65012 auto
// OFFSET: 0x00514760
static int32_t Script_IsLeftAltKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = (CGUIBindings::s_bindings->m_heldModifiers >> 4) & 1;
    } else {
        down = EventIsKeyDown(KEY_LALT);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5d240461 auto
// OFFSET: 0x005147C0
static int32_t Script_IsRightAltKeyDown(lua_State* L) {
    int32_t down;

    if (CGUIBindings::s_bindings->m_keyStateOverride) {
        down = (CGUIBindings::s_bindings->m_heldModifiers >> 5) & 1;
    } else {
        down = EventIsKeyDown(KEY_RALT);
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 94368160 manual
// OFFSET: 0x00516280
static int32_t Script_GetCorpseRecoveryDelay(lua_State* L) {
    int32_t remaining;

    if (!CGGameUI::m_corpseReclaimDelay || (remaining = CGGameUI::m_corpseReclaimDelay - OsGetAsyncTimeMs()) <= 0) {
        remaining = 0;
    }

    lua_pushnumber(L, static_cast<uint32_t>(remaining) / 1000);
    return 1;
}

// VERIFY: a39caf14 manual
// OFFSET: 0x005162E0
static int32_t Script_GetInstanceBootTimeRemaining(lua_State* L) {
    int32_t remaining;

    if (!CGGameUI::m_instanceBootTime || (remaining = CGGameUI::m_instanceBootTime - OsGetAsyncTimeMs()) <= 0) {
        remaining = 0;
    }

    lua_pushnumber(L, static_cast<uint32_t>(remaining) / 1000);
    return 1;
}

// VERIFY: 6a68c02d manual
// OFFSET: 0x005164B0
static int32_t Script_GetSummonConfirmTimeLeft(lua_State* L) {
    int32_t remaining;

    if (!CGGameUI::m_summonConfirmTime || (remaining = CGGameUI::m_summonConfirmTime - OsGetAsyncTimeMs()) <= 0) {
        remaining = 0;
    }

    lua_pushnumber(L, static_cast<uint32_t>(remaining) / 1000);
    return 1;
}

// VERIFY: 5b2771f3 auto
// OFFSET: 0x00516580
static int32_t Script_GetSummonConfirmAreaName(lua_State* L) {
    const AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_summonConfirmArea);

    if (area) {
        lua_pushstring(L, area->m_areaName);
        return 1;
    }

    lua_pushstring(L, "");
    return 1;
}

// VERIFY: 5b10428b auto
// OFFSET: 0x005167E0
static int32_t Script_IsSubZonePVPPOI(lua_State* L) {
    const AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_subzoneID);

    if (area && (area->m_flags & 0x8000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 264561d8 auto
// OFFSET: 0x00516B90
static int32_t Script_GetAreaSpiritHealerTime(lua_State* L) {
    int32_t remaining = 0;

    if (CGGameUI::m_areaSpiritHealerTime) {
        remaining = CGGameUI::m_areaSpiritHealerTime - OsGetAsyncTimeMs();

        if (remaining < 0) {
            remaining = 0;
        }
    }

    lua_pushnumber(L, remaining / 1000);
    return 1;
}

// VERIFY: 2ceedaf7 manual
// OFFSET: 0x0051B6E0
static int32_t Script_CheckSpiritHealerDist(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->CheckSpiritHealerDistance()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3bf7a0f7 manual
// OFFSET: 0x0051B740
static int32_t Script_CheckTalentMasterDist(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->CheckTalentMasterDistance()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 06a05e3b manual
// OFFSET: 0x0051B7A0
static int32_t Script_CheckBinderDist(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->CheckBinderDistance()) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: ffe47e4b manual
// OFFSET: 0x0051BD00
static int32_t Script_GetPVPTimer(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t timer = player ? player->GetPVPToggleTimer() + 1000 : 0;
    lua_pushnumber(L, timer);
    return 1;
}

// VERIFY: 4d714989 auto
// OFFSET: 0x0054C810
int32_t Script_CanHearthAndResurrectFromArea(lua_State* L) {
    const AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    if (area && (area->m_flags & 0x8000000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3e89a8d8 manual
// OFFSET: 0x00510AC0
static int32_t Script_GetNetStats(lua_State* L) {
    float bytesIn;
    float bytesOut;
    uint32_t latency;
    ClientServices::Connection()->GetNetStats(bytesIn, bytesOut, latency);
    lua_pushnumber(L, bytesIn);
    lua_pushnumber(L, bytesOut);
    lua_pushnumber(L, latency);
    return 3;
}

// VERIFY: 07999454 manual
// OFFSET: 0x005117C0
static int32_t Script_GetEventCPUUsage(lua_State* L) {
    double time = 0.0;
    uint32_t count = 0;
    FrameScript_GetEventCPUUsage(lua_tolstring(L, 1, nullptr), &time, &count);
    lua_pushnumber(L, time);
    lua_pushnumber(L, count);
    return 2;
}

// VERIFY: 63c2507c auto
// OFFSET: 0x00514BA0
static int32_t Script_GetMouseButtonName(lua_State* L) {
    if (lua_isnumber(L, 1)) {
        lua_pushstring(L, GetMouseButtonName(IndexToMouseButton(static_cast<uint32_t>(lua_tonumber(L, 1)))));
        return 1;
    }

    if (lua_isstring(L, 1)) {
        lua_settop(L, 1);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 35d652a0 manual
// OFFSET: 0x00515630
static int32_t Script_ResetInstances(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_RESET_INSTANCES));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 0785053a auto
// OFFSET: 0x00515810
static int32_t Script_GetRaidDifficulty(lua_State* L) {
    uint32_t difficulty = CGPartyInfo::m_raidDifficulty;

    if (!CGRaidInfo::m_numMembers) {
        difficulty = CGGameUI::m_raidDifficulty;
    }

    lua_pushnumber(L, difficulty + 1);
    lua_pushnumber(L, CGGameUI::m_raidDifficulty + 1);
    return 2;
}

// VERIFY: 37009759 manual
// OFFSET: 0x00515B70
static int32_t Script_GuildLeave(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_LEAVE));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: bf377447 manual
// OFFSET: 0x00515BE0
static int32_t Script_GuildDisband(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_DISBAND));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: e7ba93f8 manual
// OFFSET: 0x00515C50
static int32_t Script_GuildInfo(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_INFO));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: c8a56a12 auto
// OFFSET: 0x00516210
static int32_t Script_GetReleaseTimeRemaining(lua_State* L) {
    int32_t remaining;

    if (CGGameUI::m_deadNoRepopTimer) {
        remaining = -1000;
    } else if (!CGGameUI::m_repopTime || (remaining = CGGameUI::m_repopTime - OsGetAsyncTimeMs()) <= 0) {
        remaining = 0;
    }

    lua_pushnumber(L, remaining / 1000);
    return 1;
}

// VERIFY: ec7c8150 manual
// OFFSET: 0x005166F0
static int32_t Script_OpeningCinematic(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_OPENING_CINEMATIC));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5578a0c0 manual
// OFFSET: 0x00516840
static int32_t Script_TogglePVP(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TOGGLE_PVP));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 1e926a4a auto
// OFFSET: 0x00516B20
static int32_t Script_GetBindLocation(lua_State* L) {
    const AreaTableRec* area = g_areaTableDB.GetRecord(CGPlayer_C::GetBindAreaID());

    if (area) {
        lua_pushstring(L, area->m_areaName);
        return 1;
    }

    lua_pushstring(L, FrameScript_GetText("HOME_INN", -1, GENDER_NOT_APPLICABLE));
    return 1;
}

// VERIFY: 1ab86e70 manual
// OFFSET: 0x0051BD60
static int32_t Script_IsPVPTimerRunning(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_player->m_playerFlags & 0x40000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 7aea54e4 manual
// OFFSET: 0x0051BFD0
static int32_t Script_ShowingHelm(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && !(player->m_player->m_playerFlags & 0x400)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: b0c5ab4d manual
// OFFSET: 0x0051C040
static int32_t Script_ShowingCloak(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && !(player->m_player->m_playerFlags & 0x800)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: b6e39278 manual
// OFFSET: 0x0051D100
static int32_t Script_GetTaxiBenchmarkMode(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_player->m_playerFlags & 0x20000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c8a55e32 manual
// OFFSET: 0x0051D5A0
static int32_t Script_GetAllowLowLevelRaid(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && (player->m_player->m_playerFlags & 0x10000)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 2631915b manual
// OFFSET: 0x0051CB30
static int32_t Script_PartialPlayTime(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t partial;

    if (player && (partial = (player->m_player->m_playerFlags & 0x1000) != 0)) {
        lua_pushnumber(L, partial);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: a028dc32 manual
// OFFSET: 0x0051CBA0
static int32_t Script_NoPlayTime(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t none;

    if (player && (none = (player->m_player->m_playerFlags & 0x2000) != 0)) {
        lua_pushnumber(L, none);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 40453f65 manual
// OFFSET: 0x0051BCA0
static int32_t Script_GetPVPDesired(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t desired = player ? (player->m_player->m_playerFlags >> 9) & 1 : 0;
    lua_pushnumber(L, desired);
    return 1;
}

// VERIFY: d93c6665 auto
// OFFSET: 0x00510230
static int32_t Script_SetFarclip(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetFarclip(value)");
        return 0;
    }

    CVar* var = CVar::Lookup("farclip");
    char value[16];
    SStrPrintf(value, sizeof(value), "%f", lua_tonumber(L, 1));
    var->Set(value, true, false, false, true);
    return 0;
}

// VERIFY: 17168813 auto
// OFFSET: 0x005102E0
static int32_t Script_SetTexLodBias(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetTexLodBias(value)");
        return 0;
    }

    CVar* var = CVar::Lookup("texLodBias");
    char value[16];
    SStrPrintf(value, sizeof(value), "%f", lua_tonumber(L, 1));
    var->Set(value, true, false, false, true);
    return 0;
}

// VERIFY: af1c16bf auto
// OFFSET: 0x00510390
static int32_t Script_SetBaseMip(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetBaseMip(value)");
        return 0;
    }

    CVar* var = CVar::Lookup("baseMip");
    char value[16];
    SStrPrintf(value, sizeof(value), "%d", 1.0 - lua_tonumber(L, 1));
    var->Set(value, true, false, false, true);
    return 0;
}

// VERIFY: 26416364 auto
// OFFSET: 0x00515790
static int32_t Script_GetDungeonDifficulty(lua_State* L) {
    uint32_t difficulty;

    if (!CGPartyInfo::m_members[0] || (difficulty = CGPartyInfo::m_dungeonDifficulty, CGRaidInfo::m_numMembers)) {
        difficulty = CGGameUI::m_dungeonDifficulty;
    }

    lua_pushnumber(L, difficulty + 1);
    lua_pushnumber(L, CGGameUI::m_dungeonDifficulty + 1);
    return 2;
}

// VERIFY: d333f20d manual
// OFFSET: 0x00515880
static int32_t Script_RequestTimePlayed(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_PLAYED_TIME));
    msg.Put(static_cast<uint8_t>(1));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 6aaefa51 manual
// OFFSET: 0x0051AB20
static int32_t Script_AcceptGroup(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        uint32_t roles = 0;

        if (StringToBOOL(L, 1, 0)) {
            roles = 2;
        }

        if (StringToBOOL(L, 2, 0)) {
            roles |= 4;
        }

        if (StringToBOOL(L, 3, 0)) {
            roles |= 8;
        }

        CGPlayer_C::AcceptGroup(roles);
    }

    return 0;
}

// VERIFY: 135c28b3 manual
// OFFSET: 0x00517B50
static int32_t Script_SetTaxiBenchmarkMode(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TAXI_BENCHMARK_MODE));
    msg.Put(static_cast<uint8_t>(StringToBOOL(L, 1, 1) != 0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 77bfe9c9 auto
// OFFSET: 0x00517D70
static int32_t Script_CanMapChangeDifficulty(lua_State* L) {
    int32_t mapId;

    if (lua_isnumber(L, 1)) {
        mapId = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        mapId = CGGameUI::m_iCurrentMapID;
    }

    const MapRec* map = g_mapDB.GetRecord(mapId);

    if (map && (map->m_flags & 0x100)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 3260815c manual
// OFFSET: 0x005262E0
static int32_t Script_TargetTotem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TargetTotem(slot)");
        return 0;
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 3) {
        luaL_error(L, "Totem slot must be in range of 1 to %d", 4);
        return 0;
    }

    TotemInfo* totem = &CGGameUI::m_totems[slot];

    if (!totem || !totem->m_guid) {
        return 0;
    }

    CGGameUI::Target(totem->m_guid);
    return 0;
}

// VERIFY: 17f07d34 manual
// OFFSET: 0x00511720
static int32_t Script_GetFunctionCPUUsage(lua_State* L) {
    if (lua_type(L, 1) != LUA_TFUNCTION) {
        luaL_error(L, "Usage: GetFunctionCPUUsage(function[, includeSubroutines])");
        return 0;
    }

    int32_t includeSubroutines = StringToBOOL(L, 2, 1);
    lua_settop(L, 1);
    double time;
    double totalTime;
    int32_t calls;
    lua_gcgetfunctiontime(L, &time, &totalTime, &calls);
    lua_pushnumber(L, includeSubroutines ? totalTime : time);
    lua_pushnumber(L, static_cast<uint32_t>(calls));
    return 2;
}

// VERIFY: a17fd64f manual
// OFFSET: 0x0051AFB0
static int32_t Script_GetDamageBonusStat(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    const ChrClassesRec* rec;

    if (player && (rec = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]))) {
        lua_pushnumber(L, rec->m_damageBonusStat + 1);
        return 1;
    }

    lua_pushnumber(L, 0.0);
    return 1;
}

// VERIFY: ca8c83c5 manual
// OFFSET: 0x0051B800
static int32_t Script_RetrieveCorpse(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::AcceptResurrectRequest(0);
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_RECLAIM_CORPSE));
    msg.Put(CGGameUI::m_corpseGUID.m_guid);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 92ce34ec manual
// OFFSET: 0x005168B0
static int32_t Script_SetPVP(lua_State* L) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_TOGGLE_PVP));
    uint8_t enable;

    if (lua_isnumber(L, 1)) {
        enable = static_cast<uint8_t>(lua_tonumber(L, 1));
    } else {
        enable = 0;
    }

    msg.Put(enable);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5cacdc88 manual
// OFFSET: 0x0051B110
static int32_t Script_CancelSummon(lua_State* L) {
    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SUMMON_RESPONSE));
    msg.Put(CGGameUI::m_summonConfirmGUID.m_guid);
    msg.Put(static_cast<uint8_t>(0));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: a76967ea manual
// OFFSET: 0x00522A20
static int32_t Script_GuildInvite(lua_State* L) {
    const char* target = lua_tolstring(L, 1, nullptr);

    if (!target) {
        CGGameUI::DisplayError(ERR_COMMAND_NEEDS_TARGET);
        return 0;
    }

    if (SStrLen(target) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_INVITE));
    msg.PutString(target);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: e1f01be5 manual
// OFFSET: 0x00522AF0
static int32_t Script_GuildUninvite(lua_State* L) {
    const char* target = lua_tolstring(L, 1, nullptr);

    if (!target) {
        CGGameUI::DisplayError(ERR_COMMAND_NEEDS_TARGET);
        return 0;
    }

    if (SStrLen(target) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_OFFICER_REMOVE_MEMBER));
    msg.PutString(target);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 1cc6457e manual
// OFFSET: 0x00522BC0
static int32_t Script_GuildPromote(lua_State* L) {
    const char* target = lua_tolstring(L, 1, nullptr);

    if (!target) {
        CGGameUI::DisplayError(ERR_COMMAND_NEEDS_TARGET);
        return 0;
    }

    if (SStrLen(target) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_PROMOTE_MEMBER));
    msg.PutString(target);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 1cc6e053 manual
// OFFSET: 0x00522C90
static int32_t Script_GuildDemote(lua_State* L) {
    const char* target = lua_tolstring(L, 1, nullptr);

    if (!target) {
        CGGameUI::DisplayError(ERR_COMMAND_NEEDS_TARGET);
        return 0;
    }

    if (SStrLen(target) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_DEMOTE_MEMBER));
    msg.PutString(target);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 796ea6cf manual
// OFFSET: 0x00522D60
static int32_t Script_GuildSetLeader(lua_State* L) {
    const char* target = lua_tolstring(L, 1, nullptr);

    if (!target) {
        CGGameUI::DisplayError(ERR_COMMAND_NEEDS_TARGET);
        return 0;
    }

    if (SStrLen(target) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_LEADER));
    msg.PutString(target);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 5c44edc4 auto
// OFFSET: 0x00516610
static int32_t Script_RandomRoll(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: RandomRoll(\"max\") or RandomRoll(\"min\", \"max\")");
        return 0;
    }

    int32_t min = SStrToInt(lua_tolstring(L, 1, nullptr));
    int32_t max = SStrToInt(lua_tolstring(L, 2, nullptr));

    if ((min <= 0 && max <= 0) || max < min || max > 1000000) {
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_RANDOM_ROLL));
    msg.Put(static_cast<uint32_t>(min));
    msg.Put(static_cast<uint32_t>(max));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 62d98ba4 manual
// OFFSET: 0x0050FF50
static int32_t Script_GetCVarInfo(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarInfo(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 4;
    }

    lua_pushstring(L, var->m_stringValue.m_str);
    lua_pushstring(L, var->m_defaultValue.m_str);

    if (var->m_flags & 0x10) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (var->m_flags & 0x20) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 4;
}

// VERIFY: 8bba5f56 manual
// OFFSET: 0x0051D600
static int32_t Script_SetAllowLowLevelRaid(lua_State* L) {
    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        uint8_t allow = StringToBOOL(L, 1, 1) != 0;
        CDataStore msg1;
        msg1.Put(static_cast<uint32_t>(CMSG_LOW_LEVEL_RAID1));
        msg1.Put(allow);
        msg1.Finalize();
        ClientServices::Send2(&msg1);
        CDataStore msg2;
        msg2.Put(static_cast<uint32_t>(CMSG_LOW_LEVEL_RAID2));
        msg2.Put(allow);
        msg2.Finalize();
        ClientServices::Send2(&msg2);
    }

    return 0;
}

// VERIFY: c3d049ab manual
// OFFSET: 0x005148B0
static int32_t Script_IsMouseButtonDown(lua_State* L) {
    int32_t down;

    if (lua_isnumber(L, 1)) {
        down = EventIsButtonDown(IndexToMouseButton(static_cast<uint32_t>(lua_tonumber(L, 1))));
    } else if (lua_isstring(L, 1)) {
        down = EventIsButtonDown(StringToMouseButton(lua_tolstring(L, 1, nullptr)));
    } else {
        down = EventIsButtonDown(static_cast<MOUSEBUTTON>(0x1)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x2)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x4)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x8)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x10)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x20)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x40)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x80)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x100)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x200)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x400)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x800)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x1000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x2000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x4000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x8000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x10000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x20000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x40000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x80000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x100000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x200000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x400000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x800000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x1000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x2000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x4000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x8000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x10000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x20000000)) || EventIsButtonDown(static_cast<MOUSEBUTTON>(0x40000000));
    }

    if (down) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 00d6780a manual
// OFFSET: 0x00510E20
static int32_t Script_GetItemQualityColor(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetItemQualityColor(index)");
        return 0;
    }

    uint32_t quality = static_cast<int32_t>(lua_tonumber(L, 1));
    const CImVector* color = CGTooltip::GetItemQualityColor(quality);
    lua_pushnumber(L, color->r * 0.00392156862745098);
    lua_pushnumber(L, color->g * 0.00392156862745098);
    lua_pushnumber(L, color->b * 0.00392156862745098);
    lua_pushstring(L, CGTooltip::GetItemQualityColorString(quality));
    return 4;
}

// VERIFY: 37c22dd4 auto
// OFFSET: 0x00511B30
static int32_t Script_IsAddOnLoaded(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: IsAddOnLoaded(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    if (IsAddOnLoading(name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (IsAddOnLoaded(name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: b380dd1d manual
// OFFSET: 0x00515A50
static int32_t Script_GetCursorMoney(lua_State* L) {
    lua_pushnumber(L, CGGameUI::m_cursorMoney);
    return 1;
}

// VERIFY: 1c08ff80 manual
// OFFSET: 0x00510040
static int32_t Script_GetCVar(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVar(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, var->m_stringValue.m_str);
    return 1;
}

// VERIFY: 968466d6 manual
// OFFSET: 0x005100C0
static int32_t Script_GetCVarBool(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarBool(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (var && !(var->m_flags & 0x40) && StringToBOOL(var->m_stringValue.m_str)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: b062b828 manual
// OFFSET: 0x00510150
static int32_t Script_GetCVarDefault(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarDefault(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    lua_pushstring(L, var->m_defaultValue.m_str);
    return 1;
}

// VERIFY: f62939f8 auto
// OFFSET: 0x00510A10
static int32_t Script_GetCursorPosition(lua_State* L) {
    float x = 0.0f;
    float y = 0.0f;

    if (!CSimpleTop::s_instance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    NDCToDDC(CSimpleTop::s_instance->m_mousePosition.m_x, CSimpleTop::s_instance->m_mousePosition.m_y, &x, &y);
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * x));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * y));
    return 2;
}

// VERIFY: 19aabffe auto
// OFFSET: 0x00514430
static int32_t Script_IsModifierKeyDown(lua_State* L) {
    for (int32_t key = KEY_LSHIFT; key <= KEY_RALT; key++) {
        if (CGGameUI::IsKeyDown(static_cast<KEY>(key))) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 16fc0e95 auto
// OFFSET: 0x00514580
static int32_t Script_IsShiftKeyDown(lua_State* L) {
    if (CGGameUI::IsKeyDown(KEY_LSHIFT) || CGGameUI::IsKeyDown(KEY_RSHIFT)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: e03161c3 auto
// OFFSET: 0x005146D0
static int32_t Script_IsControlKeyDown(lua_State* L) {
    if (CGGameUI::IsKeyDown(KEY_LCONTROL) || CGGameUI::IsKeyDown(KEY_RCONTROL)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5cf9d269 auto
// OFFSET: 0x00514820
static int32_t Script_IsAltKeyDown(lua_State* L) {
    if (CGGameUI::IsKeyDown(KEY_LALT) || CGGameUI::IsKeyDown(KEY_RALT)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9094ae9a manual
// OFFSET: 0x00514C10
static int32_t Script_SetCVar(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetCVar(\"cvar\", value [, \"scriptCvar\")");
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    CVar* var = CVar::LookupRegistered(name);

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", name);
        return 0;
    }

    if ((var->m_flags & 0x4) || (var->m_flags & 0x100)) {
        luaL_error(L, "\"%s\" is read-only", name);
        return 0;
    }

    if ((var->m_flags & 0x8) && !CSimpleTop::GetInstance()->m_allowProtectedActions) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 2);
        return 0;
    }

    const char* value = lua_tolstring(L, 2, nullptr);

    if (!value) {
        value = "0";
    }

    var->Set(value, true, false, false, true);

    if (lua_isstring(L, 3)) {
        FrameScript_SignalEvent(SCRIPT_EVENT_CVAR_UPDATE, "%s%s", lua_tolstring(L, 3, nullptr), value);
    }

    return 0;
}

// VERIFY: c3cc832c manual
// OFFSET: 0x00514D40
static int32_t Script_GetCVarMin(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarMin(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    const char* name = var->m_key.m_str;

    if (!SStrCmpI(name, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnumber(L, 0.0);
    } else if (!SStrCmpI(name, "farclip", STORM_MAX_STR)) {
        lua_pushnumber(L, 177.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2b500bc1 manual
// OFFSET: 0x00514E30
static int32_t Script_GetCVarMax(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarMax(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    const char* name = var->m_key.m_str;

    if (!SStrCmpI(name, "extShadowQuality", STORM_MAX_STR)) {
        uint32_t mode = 0;
        uint32_t maxMode = CShadowCache::GetMaxShadowMode();

        do {
            if (!CShadowCache::ValidateShadowMode(static_cast<EShadowMode>(mode))) {
                break;
            }

            mode++;
        } while (mode <= maxMode);

        lua_pushnumber(L, mode - 1);
    } else if (!SStrCmpI(name, "farclip", STORM_MAX_STR)) {
        lua_pushnumber(L, 1277.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: fb7bea1c auto
// OFFSET: 0x0051AEF0
static int32_t Script_GetScreenWidth(lua_State* L) {
    float width = NDCToDDCWidth(1.0f);
    float scale = CGGameUI::m_UISimpleParent ? CGGameUI::m_UISimpleParent->m_layoutScale : 1.0f;
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * (width / scale)));
    return 1;
}

// VERIFY: 53f1b9fb auto
// OFFSET: 0x0051AF50
static int32_t Script_GetScreenHeight(lua_State* L) {
    float height = NDCToDDCHeight(1.0f);
    float scale = CGGameUI::m_UISimpleParent ? CGGameUI::m_UISimpleParent->m_layoutScale : 1.0f;
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * (height / scale)));
    return 1;
}

// VERIFY: 2ffb3aeb manual
// OFFSET: 0x00528920
static int32_t Script_LoadAddOn(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<int32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: LoadAddOn(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    if (!s_loadAddOnDepth) {
        s_loadAddOnStatus = new (__FILE__, __LINE__) CWOWClientStatus("Logs\\FrameXML.log", 1);
    }

    s_loadAddOnDepth++;
    bool loaded = LoadAddOn(name, true, s_loadAddOnStatus);

    if (--s_loadAddOnDepth == 0) {
        if (s_loadAddOnStatus) {
            delete s_loadAddOnStatus;
        }

        s_loadAddOnStatus = nullptr;
    }

    if (loaded) {
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
        return 2;
    }

    lua_pushnil(L);

    UIADDON_LOADABLE_REASON reason;
    UIADDON_LOADABLE_REASON dependencyReason;

    if (!IsAddOnLoadable(name, true, reason, dependencyReason, ClientServices::GetCharacterName()) && (reason || dependencyReason)) {
        char buffer[128];
        lua_pushstring(L, AddOnUnloadableToken(reason, dependencyReason, buffer, sizeof(buffer)));
        return 2;
    }

    lua_pushstring(L, "UNKNOWN_ERROR");
    return 2;
}

// VERIFY: a3c4eae5 auto
// OFFSET: 0x005155D0
static int32_t Script_GetSubZoneText(lua_State* L) {
    lua_pushstring(L, CGGameUI::m_subZoneText ? CGGameUI::m_subZoneText : "");
    return 1;
}

// VERIFY: 76cc0988 auto
// OFFSET: 0x00515600
static int32_t Script_GetMinimapZoneText(lua_State* L) {
    lua_pushstring(L, CGGameUI::m_minimapZoneText ? CGGameUI::m_minimapZoneText : "");
    return 1;
}

// VERIFY: 198cd81d auto
// OFFSET: 0x005156A0
static int32_t Script_IsInInstance(lua_State* L) {
    MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

    if (!map) {
        lua_pushnil(L);
        lua_pushstring(L, s_instanceTypeTokens[0]);
        return 2;
    }

    if (map->m_instanceType) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    uint32_t type = map->m_instanceType;
    lua_pushstring(L, type < sizeof(s_instanceTypeTokens) / sizeof(s_instanceTypeTokens[0]) ? s_instanceTypeTokens[type] : s_instanceTypeTokens[0]);
    return 2;
}

// VERIFY: 46914859 manual
// OFFSET: 0x00516BF0
static int32_t Script_GetMouseFocus(lua_State* L) {
    if (!CSimpleTop::s_instance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CSimpleFrame* focus = CSimpleTop::s_instance->m_mouseFocus;

    if (!focus) {
        lua_pushnil(L);
        return 1;
    }

    if (!focus->m_luaRegistered) {
        focus->RegisterScriptObject(nullptr);
    }

    lua_rawgeti(L, LUA_REGISTRYINDEX, focus->m_luaObjectRef);
    return 1;
}

// VERIFY: 03727723 manual
// OFFSET: 0x0051BA50
static int32_t Script_GetZonePVPInfo(lua_State* L) {
    int32_t flags = -1;
    int32_t subzone = 0;
    AreaTableRec* subzoneRec = g_areaTableDB.GetRecord(CGGameUI::m_subzoneID);

    if (subzoneRec) {
        flags = subzoneRec->m_flags;
        subzone = 1;
    }

    AreaTableRec* area = g_areaTableDB.GetRecord(CGGameUI::m_areaID);

    if (flags == -1) {
        subzone = 0;
        flags = area ? area->m_flags : 0;
    }

    const char* type = nullptr;

    if (flags & 0x800) {
        type = "sanctuary";
    } else if (flags & 0x80) {
        type = "arena";
    } else if (flags & 0x1000000) {
        type = "combat";
    }

    if (type) {
        lua_pushstring(L, type);

        if (subzone) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        return 2;
    }

    int32_t contested = (flags & 0x40000) != 0;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!area || !player || (!s_isPvPRealm && !(area->m_flags & 0x10))) {
        return 0;
    }

    if (!contested) {
        FactionTemplateRec* faction = g_factionTemplateDB.GetRecord(player->m_unit->m_unitFieldFactiontemplate);

        if (!faction || (area->m_factionGroupMask & faction->m_friendGroup)) {
            type = "friendly";
        } else if (area->m_factionGroupMask & faction->m_enemyGroup) {
            type = "hostile";
        }
    }

    lua_pushstring(L, type ? type : "contested");
    const char* factionName = "";

    for (int32_t i = 0; i < g_factionGroupDB.m_numRecords; i++) {
        FactionGroupRec* group = i >= 0 && i < g_factionGroupDB.m_numRecords ? &g_factionGroupDB.m_records[i] : nullptr;

        if (((1 << group->m_maskID) & area->m_factionGroupMask) && *group->m_name) {
            factionName = group->m_name;
            break;
        }
    }

    lua_pushnil(L);
    lua_pushstring(L, factionName);
    return 3;
}

// VERIFY: cfaa1624 auto
// OFFSET: 0x0050F950
static int32_t Script_GetMouseButtonClicked(lua_State* L) {
    if (!CSimpleTop::s_instance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    lua_pushstring(L, CSimpleTop::s_instance->m_mouseButtonName);
    return 1;
}

// VERIFY: 5a4a2960 manual
// OFFSET: 0x00517BE0
static int32_t ToggleInteractModelIcon(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object) {
        object->SetInteractIconVisible(CGGameUI::m_visible);
    }

    return 1;
}

// VERIFY: acc56927 auto
// OFFSET: 0x00517C20
static int32_t Script_SetUIVisibility(lua_State* L) {
    int32_t visible = StringToBOOL(L, 1, 1);

    if (CGGameUI::m_visible != visible) {
        CGGameUI::m_visible = visible;
        CGGameUI::UpdateUnitHighlights();
        ClntObjMgrEnumVisibleObjects(ToggleInteractModelIcon, nullptr);
    }

    return 0;
}

// VERIFY: 0647bfed auto
// OFFSET: 0x0051AC90
static int32_t Script_CancelLogout(lua_State* L) {
    if (lua_tainted && CGGameUI::m_simpleTop && !CGGameUI::m_simpleTop->unused_1250) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 1);
        return 0;
    }

    ClientServices::Connection()->CharacterAbortLogout();
    return 0;
}

// VERIFY: fe990a77 auto
// OFFSET: 0x00522480
static int32_t Script_FocusUnit(lua_State* L) {
    WOWGUID guid;

    if (Script_GetGUIDFromToken(lua_tolstring(L, 1, nullptr), guid, true)) {
        CGGameUI::SetFocusTarget(guid);
    }

    return 0;
}

// VERIFY: fc534941 auto
// OFFSET: 0x0050F8F0
static int32_t Script_SetLayoutMode(lua_State* L) {
    int32_t enabled = 1;

    if (lua_isnumber(L, 1)) {
        enabled = static_cast<int32_t>(lua_tonumber(L, 1));
    }

    if (!CSimpleTop::s_instance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    CSimpleTop::s_instance->m_moveResizeEnabled = enabled;
    return 0;
}

// VERIFY: 07ccda23 auto
// OFFSET: 0x00511CC0
static int32_t Script_InCombatLockdown(lua_State* L) {
    if (!CSimpleTop::s_instance) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    if (CSimpleTop::s_instance->m_allowProtectedActions) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 1.0);
    }

    return 1;
}

// VERIFY: 5f4164d6 auto
// OFFSET: 0x0051D520
static int32_t Script_DestroyTotem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: DestroyTotem(slot)");
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(slot - 1) > 3) {
        luaL_error(L, "Totem slot must be in range of 1 to %d", 4);
        return 0;
    }

    CGGameUI::DestroyTotem(slot - 1);
    return 0;
}

// VERIFY: 014257ea manual
// OFFSET: 0x00515CC0
static int32_t Script_ArenaTeamInviteByName(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: ArenaTeamInviteByName(team, name)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* playerName = lua_tolstring(L, 2, nullptr);
    uint32_t teamId = index >= 3 ? 0 : CGArenaTeamInfo::m_arenaTeams[index].m_teamId;

    if (!playerName || !*playerName) {
        return 0;
    }

    if (SStrLen(playerName) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_INVITE));
    msg.Put(teamId);
    msg.PutString(playerName);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 19afb6de manual
// OFFSET: 0x00515DD0
static int32_t Script_ArenaTeamLeave(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ArenaTeamLeave(team)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t teamId = index >= 3 ? 0 : CGArenaTeamInfo::m_arenaTeams[index].m_teamId;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_LEAVE));
    msg.Put(teamId);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: d7c6b55a manual
// OFFSET: 0x00515EB0
static int32_t Script_ArenaTeamUninviteByName(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: ArenaTeamUninviteByName(team, name)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* playerName = lua_tolstring(L, 2, nullptr);

    if (playerName && SStrLen(playerName) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    uint32_t teamId = index >= 3 ? 0 : CGArenaTeamInfo::m_arenaTeams[index].m_teamId;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_REMOVE));
    msg.Put(teamId);
    msg.PutString(playerName);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: c68517ac manual
// OFFSET: 0x00515FF0
static int32_t Script_ArenaTeamSetLeaderByName(lua_State* L) {
    if (!lua_isnumber(L, 1) || !lua_isstring(L, 2)) {
        luaL_error(L, "Usage: ArenaTeamSetLeaderByName(team, name)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    const char* playerName = lua_tolstring(L, 2, nullptr);

    if (playerName && SStrLen(playerName) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    uint32_t teamId = index >= 3 ? 0 : CGArenaTeamInfo::m_arenaTeams[index].m_teamId;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_LEADER));
    msg.Put(teamId);
    msg.PutString(playerName);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 275e4a2d manual
// OFFSET: 0x00516130
static int32_t Script_ArenaTeamDisband(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ArenaTeamDisband(team)");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;
    uint32_t teamId = index >= 3 ? 0 : CGArenaTeamInfo::m_arenaTeams[index].m_teamId;
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ARENA_TEAM_DISBAND));
    msg.Put(teamId);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 6acc0cbe manual
// OFFSET: 0x0051D1B0
static int32_t Script_IsReferAFriendLinked(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (player && player->IsRAFPlayer(guid)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a6b340a9 manual
// OFFSET: 0x00511D30
static int32_t Script_GetSummonFriendCooldown(lua_State* L) {
    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;
    Spell_C_GetSpellCooldown(CGSpellBook::m_summonFriendSpell, 0, &duration, &start, &enable);
    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: 768dc8b2 manual
// OFFSET: 0x0051D330
static int32_t Script_GetTotemInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTotemInfo(slot)");
        return 0;
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 3) {
        luaL_error(L, "Totem slot must be in range of 1 to %d", 4);
        return 0;
    }

    TotemInfo* totem = slot < sizeof(CGGameUI::m_totems) / sizeof(CGGameUI::m_totems[0]) ? &CGGameUI::m_totems[slot] : nullptr;

    if (totem && totem->m_guid) {
        lua_pushboolean(L, 1);
        lua_pushstring(L, totem->m_name);
        lua_pushnumber(L, totem->m_startTime / 1000);
        lua_pushnumber(L, totem->m_duration / 1000);
        lua_pushstring(L, totem->m_icon);
        return 5;
    }

    bool found = false;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        int32_t category = 0;

        switch (slot) {
            case 0:
                category = 4;
                break;

            case 1:
                category = 2;
                break;

            case 2:
                category = 5;
                break;

            case 3:
                category = 3;
                break;
        }

        found = player->m_bag.FindTotemCategory(category, 8);
    }

    lua_pushboolean(L, found);
    lua_pushstring(L, "");
    lua_pushnumber(L, 0.0);
    lua_pushnumber(L, 0.0);
    lua_pushstring(L, "");
    return 5;
}

// VERIFY: 277ddc33 auto
// OFFSET: 0x00511C80
static int32_t Script_GetExistingLocales(lua_State* L) {
    int32_t count = 0;

    for (int32_t i = 0; i < NUM_DB_LOCALES; i++) {
        if (g_hasIsoLocale[i]) {
            lua_pushstring(L, g_localeNames[i]);
            count++;
        }
    }

    return count;
}

// VERIFY: 432ef71b manual
// OFFSET: 0x0051A8C0
static int32_t Script_GetInstanceInfo(lua_State* L) {
    int32_t mapId = CGGameUI::m_iCurrentMapID;
    MapRec* map = g_mapDB.GetRecord(mapId);

    if (!map) {
        lua_pushnil(L);
        lua_pushstring(L, s_instanceTypeTokens[0]);
        lua_pushnumber(L, 0.0);
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
        lua_pushnumber(L, 0.0);
        lua_pushnil(L);
        return 7;
    }

    lua_pushstring(L, map->m_mapName);
    uint32_t type = map->m_instanceType;
    lua_pushstring(L, type < 5 ? s_instanceTypeTokens[type] : s_instanceTypeTokens[0]);
    uint32_t difficulty = CGGameUI::m_instanceDifficulty;
    lua_pushnumber(L, difficulty + 1);
    MapDifficultyRec* rec = GetMapDifficultyEntryByMapIdAndDifficulty(mapId, difficulty, nullptr);

    if (rec) {
        lua_pushstring(L, FrameScript_GetText(rec->m_difficultystring, -1, GENDER_NOT_APPLICABLE));
        lua_pushnumber(L, rec->m_maxPlayers);
    } else {
        lua_pushstring(L, "");
        lua_pushnumber(L, 0.0);
    }

    lua_pushnumber(L, CGPartyInfo::m_dynamicDifficulty);
    map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);
    lua_pushboolean(L, map && (map->m_flags & 0x100));
    return 7;
}

// VERIFY: 0ee08026 manual
// OFFSET: 0x0051B8B0
static int32_t Script_GetRestState(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    ExhaustionRec* rec = player ? g_exhaustionDB.GetRecord(player->m_player->m_playerBytes2[3]) : nullptr;

    if (!rec) {
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushnil(L);
        return 3;
    }

    lua_pushnumber(L, rec->m_id);
    lua_pushstring(L, rec->m_name);
    lua_pushnumber(L, rec->m_factor);
    return 3;
}

// VERIFY: 20f4f8ae manual
// OFFSET: 0x0051B960
static int32_t Script_GetXPExhaustion(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    ExhaustionRec* rested = g_exhaustionDB.GetRecord(1);

    if (!player || player->m_player->m_playerBytes2[3] != 1) {
        lua_pushnil(L);
        return 1;
    }

    uint32_t experience = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerRestStateExperience : 0;
    lua_pushnumber(L, experience * rested->m_factor);
    return 1;
}

// VERIFY: 94225d6a auto
// OFFSET: 0x00525FC0
static int32_t Script_ClearTarget(lua_State* L) {
    int32_t hadTarget = CGGameUI::m_lockedTarget != WOWGUID();
    CGGameUI::unused_BD08CC = 0;

    if (CGGameUI::m_lockedTarget) {
        CGGameUI::m_lastTarget = CGGameUI::m_lockedTarget;
        CGGameUI::ClearTarget(CGGameUI::m_lockedTarget, 1);

        if (Spell_C_GetAutoRepeatingSpell()) {
            Spell_C_CancelAutoRepeat(true);
        }
    }

    if (hadTarget) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: a9e1a777 auto
// OFFSET: 0x00510450
static int32_t Script_Quit(lua_State* L) {
    ClientServices::Connection()->CharacterLogout(true, false);
    return 0;
}

// VERIFY: 3003cf92 auto
// OFFSET: 0x0052B470
static int32_t Script_RegisterForSave(lua_State* L) {
    if (lua_tainted) {
        luaL_error(L, "RegisterForSave() is only available to Blizzard scripts");
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: RegisterForSave(\"variable\")");
        return 0;
    }

    CGGameUI::RegisterSavedVariable(lua_tolstring(L, 1, nullptr), 0);
    return 0;
}

// VERIFY: da056905 auto
// OFFSET: 0x0052B4E0
static int32_t Script_RegisterForSavePerCharacter(lua_State* L) {
    if (lua_tainted) {
        luaL_error(L, "RegisterForSavePerCharacter() is only available to Blizzard scripts");
        return 0;
    }

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: RegisterForSavePerCharacter(\"variable\")");
        return 0;
    }

    CGGameUI::RegisterSavedVariable(lua_tolstring(L, 1, nullptr), 1);
    return 0;
}

// VERIFY: de3879c5 manual
// OFFSET: 0x0051CC10
static int32_t Script_GetMirrorTimerInfo(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetMirrorTimerInfo(\"timer\")");
        return 0;
    }

    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (index > UNIT_MIRROR_TIMER_FEIGNDEATH) {
        luaL_error(L, "ERROR: Invalid timer (Ex: \"BREATH\"");
        return 0;
    }

    MirrorTimerInfo& timer = CGGameUI::m_mirrorTimerInfo[index];
    lua_pushstring(L, MirrorTimerTypeToken(timer.m_type));
    lua_pushnumber(L, timer.m_value);
    lua_pushnumber(L, timer.m_maxValue);
    lua_pushnumber(L, timer.m_scale);
    lua_pushnumber(L, timer.m_paused);
    lua_pushstring(L, GetMirrorTimerLabel(timer.m_type, timer.m_spellId));
    return 6;
}

// VERIFY: facf946d auto
// OFFSET: 0x00510470
static int32_t Script_GetFramerate(lua_State* L) {
    lua_pushnumber(L, World::GetFramerate());
    return 1;
}

// VERIFY: ee32b2a0 auto
// OFFSET: 0x005165E0
static int32_t Script_GetNumTitles(lua_State* L) {
    lua_pushnumber(L, g_charTitlesDB.m_maxID + 1);
    return 1;
}

// VERIFY: 289ea831 manual
// OFFSET: 0x0051B3B0
static int32_t Script_GetCurrentTitle(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        lua_pushnumber(L, static_cast<int32_t>(player->m_player->m_playerChosenTitle));
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: c9010307 manual
// OFFSET: 0x00510FC0
static int32_t Script_GetItemCooldown(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    uint32_t start = 0;
    uint32_t duration = 0;
    uint32_t enable = 0;

    if (itemId) {
        CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);

        if (item) {
            Spell_C_GetItemCooldown(item, &duration, &start, &enable);
        } else {
            Spell_C_GetItemCooldown(itemId, &duration, &start, &enable);
        }
    }

    lua_pushnumber(L, start * 0.001);
    lua_pushnumber(L, duration * 0.001);
    lua_pushnumber(L, enable);
    return 3;
}

// VERIFY: e11224b3 manual
// OFFSET: 0x00516C60
static int32_t Script_GetItemInfo(lua_State* L) {
    ItemLinkInfo linkInfo;
    linkInfo.Clear();
    int32_t itemId;

    if (lua_isnumber(L, 1)) {
        itemId = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetItemInfo(itemID|\"name\"|\"itemlink\")");
            return 0;
        }

        const char* text = lua_tolstring(L, 1, nullptr);

        if (SStrStr(text, "item:")) {
            itemId = linkInfo.GetItemIdFromLink(text);
        } else {
            itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
        }
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    char name[1024];
    CGItem_C::BuildItemName(name, sizeof(name), itemId, linkInfo.m_randomPropertiesId);
    lua_pushstring(L, name);
    lua_pushstring(L, CGTooltip::CreateItemLink(itemId, stats->m_quality, linkInfo.m_enchants[0], reinterpret_cast<const int32_t*>(&linkInfo.m_enchants[2]), linkInfo.m_randomPropertiesId, linkInfo.m_propertySeed));
    lua_pushnumber(L, static_cast<int32_t>(stats->m_quality));
    lua_pushnumber(L, static_cast<int32_t>(stats->m_itemLevel));
    lua_pushnumber(L, static_cast<int32_t>(stats->m_requiredLevel));
    ItemClassRec* itemClass = g_itemClassDB.GetRecord(stats->m_class);
    lua_pushstring(L, itemClass ? itemClass->m_className : "");
    const ItemSubClassRec* subClass = nullptr;

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        const ItemSubClassRec* rec = i >= 0 && i < g_itemSubClassDB.m_numRecords ? &g_itemSubClassDB.m_records[i] : nullptr;

        if (rec && rec->m_classID == static_cast<int32_t>(stats->m_class) && rec->m_subClassID == static_cast<int32_t>(stats->m_subClass)) {
            subClass = rec;
            break;
        }
    }

    if (subClass) {
        lua_pushstring(L, *subClass->m_verboseName ? subClass->m_verboseName : subClass->m_displayName);
    } else {
        lua_pushstring(L, "");
    }

    lua_pushnumber(L, stats->m_stackable);
    lua_pushstring(L, s_inventoryTypeTokens[stats->m_inventoryType]);
    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char* separator = *path ? "\\" : "";
    char texture[260];
    SStrPrintf(texture, sizeof(texture), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(stats->m_displayId));
    lua_pushstring(L, texture);
    lua_pushnumber(L, static_cast<int32_t>(stats->m_sellPrice));
    return 11;
}

// VERIFY: 75f296f7 auto
// OFFSET: 0x00517100
static int32_t Script_GetItemSpell(lua_State* L) {
    SpellRec spell;
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
    int32_t spellId = item ? item->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(itemId);

    if (g_spellDB.GetRecord(spellId, &spell)) {
        lua_pushstring(L, spell.m_name);
        lua_pushstring(L, spell.m_nameSubtext);
    } else {
        lua_pushnil(L);
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 025af269 manual
// OFFSET: 0x00517C60
static int32_t Script_GetTotemTimeLeft(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTotemTimeLeft(slot)");
        return 0;
    }

    uint32_t slot = static_cast<uint32_t>(lua_tonumber(L, 1)) - 1;

    if (slot > 3) {
        luaL_error(L, "Totem slot must be in range of 1 to %d", 4);
        return 0;
    }

    TotemInfo* totem = &CGGameUI::m_totems[slot];

    if (totem && totem->m_guid.m_guid) {
        lua_pushnumber(L, totem->GetSpawnTimeLeft() / 1000);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: 4b5496f9 auto
// OFFSET: 0x00524980
static int32_t Script_StopCinematic(lua_State* L) {
    SI2::StopSound(&CGGameUI::m_cinematic.m_cameraMusic, 1, -1.0f, 1);
    EnableFadingScreen(0.25f, &CGGameUI::StopCinematicInternal, nullptr);
    return 0;
}

// VERIFY: 8266a857 manual
// OFFSET: 0x00525A90
static int32_t Script_TargetNearest(lua_State* L) {
    if (CGUIMacros::IsActionAllowed(5)) {
        TargetNearestEnemy(StringToBOOL(L, 1, 0), 0);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: 54ce30e6 manual
// OFFSET: 0x00525AD0
static int32_t Script_TargetNearestEnemy(lua_State* L) {
    if (CGUIMacros::IsActionAllowed(5)) {
        TargetNearestEnemy(StringToBOOL(L, 1, 0), 1);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: 32d545fd manual
// OFFSET: 0x00525B10
static int32_t Script_TargetNearestEnemyPlayer(lua_State* L) {
    if (CGUIMacros::IsActionAllowed(5)) {
        TargetNearestEnemy(StringToBOOL(L, 1, 0), 2);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: bcf021cd manual
// OFFSET: 0x00525B50
static int32_t Script_TargetNearestFriend(lua_State* L) {
    if (CGUIMacros::IsActionAllowed(5)) {
        TargetNearestEnemy(StringToBOOL(L, 1, 0), 3);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: 04298fa1 manual
// OFFSET: 0x00525B90
static int32_t Script_TargetNearestFriendPlayer(lua_State* L) {
    if (CGUIMacros::IsActionAllowed(5)) {
        TargetNearestEnemy(StringToBOOL(L, 1, 0), 4);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: b149bc61 auto
// OFFSET: 0x00525BD0
static int32_t Script_TargetNearestPartyMember(lua_State* L) {
    TargetNearestEnemy(StringToBOOL(L, 1, 0), 5);
    return 0;
}

// VERIFY: 3be6e022 auto
// OFFSET: 0x00525C00
static int32_t Script_TargetNearestRaidMember(lua_State* L) {
    TargetNearestEnemy(StringToBOOL(L, 1, 0), 6);
    return 0;
}

// VERIFY: ea127e1f manual
// OFFSET: 0x00511FE0
static int32_t Script_GetThreatStatusColor(lua_State* L) {
    uint32_t color = *CGTooltip::GetThreatStateColor(static_cast<uint32_t>(static_cast<int64_t>(lua_tonumber(L, 1) + 1.0)));
    lua_pushnumber(L, ((color >> 16) & 0xFF) * 0.0039215689);
    lua_pushnumber(L, ((color >> 8) & 0xFF) * 0.0039215689);
    lua_pushnumber(L, (color & 0xFF) * 0.0039215689);
    return 3;
}

// VERIFY: 6ef43428 auto
// OFFSET: 0x00510920
static int32_t Script_ResetCursor(lua_State* L) {
    CursorResetMode();
    return 0;
}

// VERIFY: 10d25911 manual
// OFFSET: 0x00515200
static int32_t Script_GetCursorInfo(lua_State* L) {
    switch (CGGameUI::m_cursorItemType) {
        case UICURSOR_ITEM: {
            CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGGameUI::GetCursorItem(), TYPE_ITEM, __FILE__, __LINE__));

            if (!item) {
                return 0;
            }

            lua_pushstring(L, "item");
            lua_pushnumber(L, item->m_obj->m_entryID);
            lua_pushstring(L, CGTooltip::GetItemHyperlinkString(item));
            return 3;
        }

        case UICURSOR_MONEY:
            lua_pushstring(L, "money");
            lua_pushnumber(L, CGGameUI::m_cursorMoney);
            return 2;

        case UICURSOR_SPELL: {
            int32_t spellId = CGGameUI::m_cursorSpell;
            SpellRec spell;
            g_spellDB.GetRecord(spellId, &spell);

            if (SpellRec::WillSummonCritter(&spell)) {
                int32_t type = CGSpellBook::GetCompanionType(&spell);
                lua_pushstring(L, "companion");
                lua_pushnumber(L, CGSpellBook::FindCompanionSlotByID(spellId, type) + 1);

                if (type == 0) {
                    lua_pushstring(L, "CRITTER");
                } else {
                    if (type != 1) {
                        lua_pushstring(L, "UNKNOWN");
                        SErrDisplayAppFatalMessage("", "", 0);
                    }

                    lua_pushstring(L, "MOUNT");
                }
            } else {
                int32_t petSpell = 0;
                int32_t slot = GetSpellSlot(spellId, 0);

                if (slot < 0) {
                    slot = GetSpellSlot(spellId, 1);

                    if (slot >= 0) {
                        petSpell = 1;
                    }
                }

                lua_pushstring(L, "spell");
                lua_pushnumber(L, slot + 1);
                lua_pushstring(L, petSpell ? "pet" : "spell");
            }

            return 3;
        }

        case UICURSOR_MERCHANT:
            lua_pushstring(L, "merchant");
            lua_pushnumber(L, CGGameUI::m_cursorVirtualSlot + 1);
            return 2;

        case UICURSOR_ACTIONBAR:
        case UICURSOR_AMMO:
        case UICURSOR_GUILDBANK_ITEM: {
            int32_t itemId = CGGameUI::m_cursorVirtualID;
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

            if (!stats) {
                return 0;
            }

            lua_pushstring(L, "item");
            lua_pushnumber(L, itemId);
            lua_pushstring(L, CGTooltip::GetItemHyperlinkString(itemId, stats->m_quality, 0, 0, 0));
            return 3;
        }

        case UICURSOR_MACRO:
            lua_pushstring(L, "macro");
            lua_pushnumber(L, CGUIMacros::GetMacroIndexByID(CGGameUI::m_cursorMacro) + 1);
            return 2;

        case UICURSOR_GUILDBANK_MONEY:
            lua_pushstring(L, "guildbankmoney");
            lua_pushnumber(L, CGGameUI::m_cursorMoney);
            return 2;

        case UICURSOR_EQUIPMENT_SET: {
            const EQUIPMENT_SET* set = CGEquipmentManager::GetSetByID(CGGameUI::m_cursorEquipmentSet);

            if (!set) {
                return 0;
            }

            lua_pushstring(L, "equipmentset");
            lua_pushstring(L, set->m_name);
            return 2;
        }

        default:
            return 0;
    }
}

// VERIFY: dfb090c7 manual
// OFFSET: 0x0051A680
static int32_t Script_AutoEquipCursorItem(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->AutoEquipCursorItem(0);
    }

    return 0;
}

// VERIFY: c2688421 manual
// OFFSET: 0x0051AE80
static int32_t Script_HasKey(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player && player->m_bag.FindItemOfBagFamily(0x100, 79)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: bd9176fa manual
// OFFSET: 0x0051B410
static int32_t Script_IsTitleKnown(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: IsTitleKnown(titleMaskID)");
        return 0;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t mask = static_cast<uint32_t>(lua_tonumber(L, 1));

    if (player && mask && (player->m_player->m_playerFieldKnownTitles[mask >> 6] & (1ull << (mask & 63)))) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: a134affc manual
// OFFSET: 0x0051B4E0
static int32_t Script_GetTitleName(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetTitleName(titleMaskID)");
        return 0;
    }

    int32_t maskId = static_cast<int32_t>(lua_tonumber(L, 1));
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t gender = player ? player->m_player->m_playerBytes3[0] : 0;
    char title[256];
    title[0] = 0;
    CharTitlesRec* rec = nullptr;

    for (int32_t i = 0; i < g_charTitlesDB.m_numRecords; i++) {
        CharTitlesRec* candidate = g_charTitlesDB.GetRecordByIndex(i);

        if (candidate && candidate->m_maskID == maskId) {
            rec = candidate;
            break;
        }
    }

    if (!rec) {
        return 0;
    }

    if (gender == 1) {
        strcpy(title, strcmp(rec->m_name1, "") ? rec->m_name1 : rec->m_name);
    } else {
        strcpy(title, strcmp(rec->m_name, "") ? rec->m_name : rec->m_name1);
    }

    char stripped[256];
    int32_t length = 0;
    bool skipping = false;

    for (uint32_t i = 0; i < SStrLen(title); i++) {
        char c = title[i];

        if (!c) {
            break;
        }

        if (c == '%') {
            skipping = true;
        } else if (!skipping) {
            stripped[length++] = c;
        } else if (c == ' ') {
            skipping = false;
            stripped[length++] = c;
        }
    }

    stripped[length] = 0;
    lua_pushstring(L, stripped);
    return 1;
}

// VERIFY: 56c60ec4 manual
// OFFSET: 0x0051B1D0
static int32_t Script_SitStandOrDescendStart(lua_State* L) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return 0;
    }

    DescendStart();

    if (player->m_movement->m_flags & 0x2000000) {
        return 0;
    }

    if (!player->m_unit->m_unitFieldBytes1[0]) {
        player->ChangeStandState(UNIT_SITTING);
    } else {
        player->ChangeStandState(UNIT_STANDING);
    }

    return 0;
}

// VERIFY: f1cdd46e auto
// OFFSET: 0x005104A0
static int32_t Script_SetCursor(lua_State* L) {
    struct CursorName {
        CURSORMODE m_mode;
        const char* m_name;
    };

    CursorName cursors[52] = { { POINT_CURSOR, "POINT_CURSOR" }, { CAST_CURSOR, "CAST_CURSOR" }, { BUY_CURSOR, "BUY_CURSOR" }, { ATTACK_CURSOR, "ATTACK_CURSOR" }, { INTERACT_CURSOR, "INTERACT_CURSOR" }, { SPEAK_CURSOR, "SPEAK_CURSOR" }, { INSPECT_CURSOR, "INSPECT_CURSOR" }, { PICKUP_CURSOR, "PICKUP_CURSOR" }, { TAXI_CURSOR, "TAXI_CURSOR" }, { TRAINER_CURSOR, "TRAINER_CURSOR" }, { MINE_CURSOR, "MINE_CURSOR" }, { SKIN_CURSOR, "SKIN_CURSOR" }, { GATHER_CURSOR, "GATHER_CURSOR" }, { LOCK_CURSOR, "LOCK_CURSOR" }, { MAIL_CURSOR, "MAIL_CURSOR" }, { LOOT_ALL_CURSOR, "LOOT_ALL_CURSOR" }, { REPAIR_CURSOR, "REPAIR_CURSOR" }, { REPAIRNPC_CURSOR, "REPAIRNPC_CURSOR" }, { ITEM_CURSOR, "ITEM_CURSOR" }, { SKIN_HORDE_CURSOR, "SKIN_HORDE_CURSOR" }, { SKIN_ALLIANCE_CURSOR, "SKIN_ALLIANCE_CURSOR" }, { INNKEEPER_CURSOR, "INNKEEPER_CURSOR" }, { QUEST_CURSOR, "QUEST_CURSOR" }, { QUEST_REPEATABLE_CURSOR, "QUEST_REPEATABLE_CURSOR" }, { QUEST_TURNIN_CURSOR, "QUEST_TURNIN_CURSOR" }, { VEHICLE_CURSOR, "VEHICLE_CURSOR" }, { POINT_ERROR_CURSOR, "POINT_ERROR_CURSOR" }, { CAST_ERROR_CURSOR, "CAST_ERROR_CURSOR" }, { BUY_ERROR_CURSOR, "BUY_ERROR_CURSOR" }, { ATTACK_ERROR_CURSOR, "ATTACK_ERROR_CURSOR" }, { INTERACT_ERROR_CURSOR, "INTERACT_ERROR_CURSOR" }, { SPEAK_ERROR_CURSOR, "SPEAK_ERROR_CURSOR" }, { INSPECT_ERROR_CURSOR, "INSPECT_ERROR_CURSOR" }, { PICKUP_ERROR_CURSOR, "PICKUP_ERROR_CURSOR" }, { TAXI_ERROR_CURSOR, "TAXI_ERROR_CURSOR" }, { TRAINER_ERROR_CURSOR, "TRAINER_ERROR_CURSOR" }, { MINE_ERROR_CURSOR, "MINE_ERROR_CURSOR" }, { SKIN_ERROR_CURSOR, "SKIN_ERROR_CURSOR" }, { GATHER_ERROR_CURSOR, "GATHER_ERROR_CURSOR" }, { LOCK_ERROR_CURSOR, "LOCK_ERROR_CURSOR" }, { MAIL_ERROR_CURSOR, "MAIL_ERROR_CURSOR" }, { LOOT_ALL_ERROR_CURSOR, "LOOT_ALL_ERROR_CURSOR" }, { REPAIR_ERROR_CURSOR, "REPAIR_ERROR_CURSOR" }, { REPAIRNPC_ERROR_CURSOR, "REPAIRNPC_ERROR_CURSOR" }, { ITEM_ERROR_CURSOR, "ITEM_ERROR_CURSOR" }, { SKIN_HORDE_ERROR_CURSOR, "SKIN_HORDE_ERROR_CURSOR" }, { SKIN_ALLIANCE_ERROR_CURSOR, "SKIN_ALLIANCE_ERROR_CURSOR" }, { INNKEEPER_ERROR_CURSOR, "INNKEEPER_ERROR_CURSOR" }, { QUEST_ERROR_CURSOR, "QUEST_ERROR_CURSOR" }, { QUEST_REPEATABLE_ERROR_CURSOR, "QUEST_REPEATABLE_ERROR_CURSOR" }, { QUEST_TURNIN_ERROR_CURSOR, "QUEST_TURNIN_ERROR_CURSOR" }, { VEHICLE_ERROR_CURSOR, "VEHICLE_ERROR_CURSOR" } };

    if (lua_type(L, 1) == LUA_TNIL) {
        CursorResetMode();
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (!name) {
        luaL_error(L, "Usage: SetCursor(\"cursor\" or nil)");
        return 0;
    }

    for (uint32_t i = 0; i < 52; i++) {
        if (!SStrCmpI(cursors[i].m_name, name, STORM_MAX_STR)) {
            CursorSetMode(cursors[i].m_mode);
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    if (CursorSetFromName(name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 9895988e manual
// OFFSET: 0x00516970
static int32_t Script_SetPortraitToTexture(lua_State* L) {
    CSimpleTexture* texture;

    if (lua_isstring(L, 1)) {
        texture = static_cast<CSimpleTexture*>(CScriptObject::GetScriptObjectByName(lua_tolstring(L, 1, nullptr), CSimpleTexture::GetObjectType()));
    } else {
        if (lua_type(L, 1) != LUA_TTABLE) {
            luaL_error(L, "SetPortraitToTexture(): Couldn't find texture named '%s'", lua_tolstring(L, 1, nullptr));
        }

        lua_rawgeti(L, 1, 0);
        texture = static_cast<CSimpleTexture*>(lua_touserdata(L, -1));
        lua_settop(L, -2);

        if (!texture) {
            luaL_error(L, "SetPortraitToTexture(): Couldn't find 'this' in texture object");
        }

        if (!texture->IsA(CSimpleTexture::GetObjectType())) {
            luaL_error(L, "SetPortraitToTexture(): Wrong object type, expected texture");
        }
    }

    if (!texture) {
        luaL_error(L, "SetPortraitToTexture(): Couldn't find texture named '%s'", lua_tolstring(L, 1, nullptr));
    }

    if (lua_isstring(L, 2)) {
        SetPortraitTexture(texture, lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 13b09c22 auto
// OFFSET: 0x00510430
static int32_t Script_Logout(lua_State* L) {
    ClientServices::Connection()->CharacterLogout(false, false);
    return 0;
}

// VERIFY: e8e90aa3 manual
// OFFSET: 0x0050F890
static int32_t Script_GetBuildInfo(lua_State* L) {
    lua_pushstring(L, GAME_VERSION);
    lua_pushstring(L, GAME_BUILD_STRING);
    lua_pushstring(L, GAME_BUILD_DATE);
    lua_pushnumber(L, GetCodeInterfaceVersion());
    return 4;
}

// VERIFY: 91484fb6 manual
// OFFSET: 0x0050F990
static int32_t Script_SetConsoleKey(lua_State* L) {
    KEY key = KEY_LAST;

    if (!lua_isstring(L, 1)) {
        SetConsoleKey(key);
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (SStrLen(name) == 1) {
        char c = *name;

        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || c < 'a' || c > 'z') {
            SetConsoleKey(static_cast<KEY>(c));
        } else {
            SetConsoleKey(static_cast<KEY>(toupper(c)));
        }

        return 0;
    }

    if (toupper(*name) == 'F' && isdigit(name[1])) {
        SetConsoleKey(static_cast<KEY>(SStrToInt(name + 1) + KEY_F1 - 1));
        return 0;
    }

    if (!SStrCmpI(name, "NUMPAD", SStrLen("NUMPAD"))) {
        const char* rest = name + SStrLen("NUMPAD");

        while (*rest == ' ') {
            rest++;
        }

        if (isdigit(*rest)) {
            SetConsoleKey(static_cast<KEY>(*rest + KEY_NUMPAD0 - '0'));
            return 0;
        }

        if (!SStrCmpI(rest, "PLUS", STORM_MAX_STR) || *rest == '+') {
            SetConsoleKey(KEY_NUMPAD_PLUS);
            return 0;
        }

        if (!SStrCmpI(rest, "MINUS", STORM_MAX_STR) || *rest == '-') {
            SetConsoleKey(KEY_NUMPAD_MINUS);
            return 0;
        }

        if (!SStrCmpI(rest, "MULTIPLY", STORM_MAX_STR) || *rest == '*') {
            SetConsoleKey(KEY_NUMPAD_MULTIPLY);
            return 0;
        }

        if (!SStrCmpI(rest, "DIVIDE", STORM_MAX_STR) || *rest == '/') {
            SetConsoleKey(KEY_NUMPAD_DIVIDE);
            return 0;
        }

        if (!SStrCmpI(rest, "DECIMAL", STORM_MAX_STR) || *rest == '.') {
            SetConsoleKey(KEY_NUMPAD_DECIMAL);
            return 0;
        }

        if (!SStrCmpI(rest, "EQUALS", STORM_MAX_STR) || *rest == '=') {
            SetConsoleKey(KEY_NUMPAD_EQUALS);
            return 0;
        }
    } else {
        if (!SStrCmpI(name, "ESCAPE", STORM_MAX_STR)) {
            SetConsoleKey(KEY_ESCAPE);
            return 0;
        }

        if (!SStrCmpI(name, "ENTER", STORM_MAX_STR)) {
            SetConsoleKey(KEY_ENTER);
            return 0;
        }

        if (!SStrCmpI(name, "BACKSPACE", STORM_MAX_STR)) {
            SetConsoleKey(KEY_BACKSPACE);
            return 0;
        }

        if (!SStrCmpI(name, "TAB", STORM_MAX_STR)) {
            SetConsoleKey(KEY_TAB);
            return 0;
        }

        if (!SStrCmpI(name, "LEFT", STORM_MAX_STR)) {
            SetConsoleKey(KEY_LEFT);
            return 0;
        }

        if (!SStrCmpI(name, "UP", STORM_MAX_STR)) {
            SetConsoleKey(KEY_UP);
            return 0;
        }

        if (!SStrCmpI(name, "RIGHT", STORM_MAX_STR)) {
            SetConsoleKey(KEY_RIGHT);
            return 0;
        }

        if (!SStrCmpI(name, "DOWN", STORM_MAX_STR)) {
            SetConsoleKey(KEY_DOWN);
            return 0;
        }

        if (!SStrCmpI(name, "INSERT", STORM_MAX_STR)) {
            SetConsoleKey(KEY_INSERT);
            return 0;
        }

        if (!SStrCmpI(name, "DELETE", STORM_MAX_STR)) {
            SetConsoleKey(KEY_DELETE);
            return 0;
        }

        if (!SStrCmpI(name, "HOME", STORM_MAX_STR)) {
            SetConsoleKey(KEY_HOME);
            return 0;
        }

        if (!SStrCmpI(name, "END", STORM_MAX_STR)) {
            SetConsoleKey(KEY_END);
            return 0;
        }

        if (!SStrCmpI(name, "PAGEUP", STORM_MAX_STR)) {
            SetConsoleKey(KEY_PAGEUP);
            return 0;
        }

        if (!SStrCmpI(name, "PAGEDOWN", STORM_MAX_STR)) {
            SetConsoleKey(KEY_PAGEDOWN);
            return 0;
        }

        if (!SStrCmpI(name, "NUMLOCK", STORM_MAX_STR)) {
            SetConsoleKey(KEY_NUMLOCK);
            return 0;
        }

        if (!SStrCmpI(name, "PRINTSCREEN", STORM_MAX_STR)) {
            key = KEY_PRINTSCREEN;
        }
    }

    SetConsoleKey(key);
    return 0;
}

// VERIFY: 201e4d4d auto
// OFFSET: 0x0050FEA0
static int32_t RegisterCVar(lua_State* L, uint32_t flags) {
    if (!lua_isstring(L, 1)) {
        char function[1024];
        luaL_error(L, "Usage: %s(\"cvar\" [, default])", FrameScript_GetCurrentFunction(L, function, sizeof(function)));
    }

    const char* name = lua_tolstring(L, 1, nullptr);
    const char* value = lua_tolstring(L, 2, nullptr);

    if (!value) {
        value = "0";
    }

    CVar::Register(name, nullptr, flags, value, nullptr, 4, true, nullptr, true);
    return 0;
}

// VERIFY: e614a034 auto
// OFFSET: 0x0050FF30
static int32_t Script_RegisterCVar(lua_State* L) {
    return RegisterCVar(L, 0);
}

// VERIFY: b5349b75 auto
// OFFSET: 0x00510410
static int32_t Script_Stuck(lua_State* L) {
    Spell_C::CastSpell(CGSpellBook::m_stuckSpell, nullptr, WOWGUID(), false);
    return 0;
}

// VERIFY: cae879bb auto
// OFFSET: 0x00510930
static int32_t Script_ReportBug(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: ReportBug(\"description\")");
        return 0;
    }

    SendBugOpcode(0, lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: e60caf0c auto
// OFFSET: 0x00510980
static int32_t Script_ReportSuggestion(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: ReportSuggestion(\"description\")");
        return 0;
    }

    SendBugOpcode(1, lua_tolstring(L, 1, nullptr));
    return 0;
}

// VERIFY: de9c98bc auto
// OFFSET: 0x005109F0
static int32_t Script_ForceLogout(lua_State* L) {
    ClientServices::Connection()->CharacterForceLogout();
    return 0;
}

// VERIFY: 4f9f11a0 auto
// OFFSET: 0x00510B30
static int32_t Script_RunScript(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        return 0;
    }

    const char* script = lua_tolstring(L, 1, nullptr);

    if (script && *script) {
        FrameScript_Execute(script, script, g_macroTaint);
    }

    return 0;
}

// VERIFY: 2d6afd0c auto
// OFFSET: 0x00510B80
static int32_t Script_ReplaceTradeEnchant(lua_State* L) {
    Spell_C_TargetTradeItem();
    return 0;
}

// VERIFY: 077a385d auto
// OFFSET: 0x00510B90
static int32_t Script_IsLinuxClient(lua_State* L) {
    lua_pushnil(L);
    return 1;
}

// VERIFY: ec425fdd auto
// OFFSET: 0x00510BB0
static int32_t Script_GMRequestPlayerInfo(lua_State* L) {
    luaL_error(L, "Access Denied");
    return 0;
}

// VERIFY: 518f4cf8 auto
// OFFSET: 0x00510BD0
static int32_t Script_GetCoinIcon(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCoinIcon(amount)");
        return 0;
    }

    char icon[260];
    GetCurrencyIcon(static_cast<uint32_t>(lua_tonumber(L, 1)), icon, sizeof(icon));
    lua_pushstring(L, icon);
    return 1;
}

// VERIFY: aa5c7661 auto
// OFFSET: 0x00510C60
static int32_t Script_GetCoinText(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCoinText(amount [,separator])");
        return 0;
    }

    const char* separator = luaL_optlstring(L, 2, ", ", nullptr);
    char text[1024];
    GetCurrencyText(static_cast<uint32_t>(lua_tonumber(L, 1)), text, sizeof(text), separator);
    lua_pushstring(L, text);
    return 1;
}

// VERIFY: 53695d34 auto
// OFFSET: 0x00510D00
static int32_t Script_GetCoinTextureString(lua_State* L) {
    int32_t height = 14;

    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetCoinText(amount, fontHeight)");
        return 0;
    }

    if (lua_isnumber(L, 2)) {
        height = static_cast<int32_t>(lua_tonumber(L, 2));
    }

    char text[1024];
    GetCurrencyTextureString(static_cast<uint32_t>(lua_tonumber(L, 1)), text, sizeof(text), height);
    lua_pushstring(L, text);
    return 1;
}

// VERIFY: 6d0dd4dd auto
// OFFSET: 0x00510DB0
static int32_t Script_ConfirmBindOnUse(lua_State* L) {
    CGItem_C::ConfirmBindOnUse();
    return 0;
}

// VERIFY: f0fda45e manual
// OFFSET: 0x00510F20
static int32_t Script_GetItemFamily(lua_State* L) {
    WOWGUID none = WOWGUID();
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    lua_pushnumber(L, stats->m_bagFamily);
    return 1;
}

// VERIFY: 5af92263 auto
// OFFSET: 0x005110D0
static int32_t Script_IsCurrentItem(lua_State* L) {
    int32_t itemId;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else {
        if (!lua_isstring(L, 1)) {
            lua_pushnil(L);
            return 1;
        }

        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    if (itemId && Spell_C_IsCurrentItem(itemId)) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 0fed2788 auto
// OFFSET: 0x00511150
static int32_t Script_IsEquippableItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (stats && stats->m_inventoryType) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 5a794641 auto
// OFFSET: 0x00511240
static int32_t Script_GetAddOnInfo(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetAddOnInfo(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    lua_pushstring(L, name);
    const char* title = GetAddOnTitle(name);

    if (title) {
        lua_pushstring(L, title);
    } else {
        lua_pushnil(L);
    }

    const char* notes = GetAddOnNotes(name);

    if (notes) {
        lua_pushstring(L, notes);
    } else {
        lua_pushnil(L);
    }

    if (GetAddOnEnableState(name, ClientServices::GetCharacterName(), true)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (IsAddOnLoaded(name)) {
        lua_pushnumber(L, 1.0);
        lua_pushnil(L);
    } else {
        UIADDON_LOADABLE_REASON reason = UIADDON_LOADABLE;
        UIADDON_LOADABLE_REASON dependencyReason = UIADDON_LOADABLE;

        if (IsAddOnLoadable(name, true, reason, dependencyReason, ClientServices::GetCharacterName())) {
            lua_pushnumber(L, 1.0);
        } else {
            lua_pushnil(L);
        }

        if (reason || dependencyReason) {
            char token[128];
            lua_pushstring(L, AddOnUnloadableToken(reason, dependencyReason, token, sizeof(token)));
        } else {
            lua_pushnil(L);
        }
    }

    lua_pushstring(L, AddOnSecurityToken(GetAddOnSecurity(name)));
    return 7;
}

// VERIFY: 5be77f45 manual
// OFFSET: 0x00511430
static int32_t Script_GetAddOnMetadata(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
        }
    } else if (!lua_isstring(L, 1) || !(name = lua_tolstring(L, 1, nullptr))) {
        luaL_error(L, "Usage: GetAddOnMetadata(index or \"name\", \"variable\")");
    }

    if (!lua_isstring(L, 2)) {
        luaL_error(L, "Usage: GetAddOnMetadata(index or \"name\", \"variable\")");
    }

    lua_pushstring(L, GetAddOnMetadata(name, lua_tolstring(L, 2, nullptr)));
    return 1;
}

// VERIFY: f959985d manual
// OFFSET: 0x00511530
static int32_t Script_GetAddOnMemoryUsage(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetAddOnMemoryUsage(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    lua_pushnumber(L, lua_gcgetsize(L, GetAddOnName(name)) * 0.0009765625);
    return 1;
}

// VERIFY: b50f7eb0 auto
// OFFSET: 0x00511660
static int32_t Script_GetAddOnCPUUsage(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetAddOnCPUUsage(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    lua_pushnumber(L, lua_gcgettime(L, GetAddOnName(name)));
    return 1;
}

// VERIFY: 8458713d auto
// OFFSET: 0x00511840
static int32_t Script_EnableAddOn(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: EnableAddOn(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    SetAddOnEnableState(name, ClientServices::GetCharacterName(), 2);
    return 0;
}

// VERIFY: 4bf9ac48 auto
// OFFSET: 0x00511900
static int32_t Script_EnableAllAddOns(lua_State* L) {
    for (uint32_t i = 0; i < GetNumAddOns(); i++) {
        SetAddOnEnableState(GetAddOnByIndex(i), ClientServices::GetCharacterName(), 2);
    }

    return 0;
}

// VERIFY: 194605d2 auto
// OFFSET: 0x00511940
static int32_t Script_DisableAddOn(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: DisableAddOn(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    SetAddOnEnableState(name, ClientServices::GetCharacterName(), 0);
    return 0;
}

// VERIFY: 740cabb1 auto
// OFFSET: 0x00511A00
static int32_t Script_DisableAllAddOns(lua_State* L) {
    for (uint32_t i = 0; i < GetNumAddOns(); i++) {
        SetAddOnEnableState(GetAddOnByIndex(i), ClientServices::GetCharacterName(), 0);
    }

    return 0;
}

// VERIFY: 7468a523 auto
// OFFSET: 0x00511A60
static int32_t Script_IsAddOnLoadOnDemand(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: IsAddOnLoadOnDemand(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    if (IsAddOnLoadOnDemand(name)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// OFFSET: 0x005EEB70
static void VoicePushToTalkNoop() {
}

// VERIFY: 667732d4 auto
// OFFSET: 0x00511D20
static int32_t Script_VoicePushToTalkStart(lua_State* L) {
    VoicePushToTalkNoop();
    return 0;
}

// VERIFY: b5b5a6cb auto
// OFFSET: 0x00511D20
static int32_t Script_VoicePushToTalkStop(lua_State* L) {
    VoicePushToTalkNoop();
    return 0;
}

// VERIFY: abe09c5f auto
// OFFSET: 0x00511DD0
static int32_t Script_GetNumDeclensionSets(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetNumDeclensionSets(\"name\", gender)");
        return 0;
    }

    int32_t sex = 2;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (lua_isnumber(L, 2)) {
        int32_t value = lua_tointeger(L, 2);

        for (int32_t i = 0; i < 3; i++) {
            if (s_unitSexValues[i] == value) {
                sex = i;
                break;
            }
        }
    }

    lua_pushnumber(L, GetNumNameDeclensionSets(s_locale, name, sex));
    return 1;
}

// VERIFY: 379e1c6d auto
// OFFSET: 0x00511E80
static int32_t Script_DeclineName(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 3)) {
        luaL_error(L, "Usage: DeclineName(\"name\", gender, declensionSet)");
        return 0;
    }

    int32_t sex = 2;
    const char* name = lua_tolstring(L, 1, nullptr);

    if (lua_isnumber(L, 2)) {
        int32_t value = lua_tointeger(L, 2);

        for (int32_t i = 0; i < 3; i++) {
            if (s_unitSexValues[i] == value) {
                sex = i;
                break;
            }
        }
    }

    char declensions[5][1024];

    if (GetNameDeclensions(s_locale, declensions[0], name, sex, static_cast<int32_t>(lua_tonumber(L, 3)) - 1)) {
        for (uint32_t i = 0; i < 5; i++) {
            lua_pushstring(L, declensions[i]);
        }

        return 5;
    }

    for (uint32_t i = 0; i < 5; i++) {
        lua_pushnil(L);
    }

    return 5;
}

// VERIFY: 7075ea82 auto
// OFFSET: 0x00511F90
static int32_t Script_UploadSettings(lua_State* L) {
    UploadAccountData();
    return 0;
}

// VERIFY: bc7e64c4 auto
// OFFSET: 0x00511FA0
static int32_t Script_DownloadSettings(lua_State* L) {
    DownloadAccountData();
    return 0;
}

// VERIFY: 6d0a3829 manual
// OFFSET: 0x00514F60
static int32_t Script_GetCVarAbsoluteMin(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarAbsoluteMin(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    if (SStrCmpI(var->m_key.m_str, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: bb4233ac manual
// OFFSET: 0x00515010
static int32_t Script_GetCVarAbsoluteMax(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetCVarAbsoluteMax(\"cvar\")");
        return 0;
    }

    CVar* var = CVar::LookupRegistered(lua_tolstring(L, 1, nullptr));

    if (!var || (var->m_flags & 0x40)) {
        luaL_error(L, "Couldn't find CVar named '%s'", lua_tolstring(L, 1, nullptr));
        return 0;
    }

    if (SStrCmpI(var->m_key.m_str, "extShadowQuality", STORM_MAX_STR)) {
        lua_pushnil(L);
    } else {
        lua_pushnumber(L, CShadowCache::GetMaxShadowMode());
    }

    return 1;
}

// VERIFY: f1cea7a8 auto
// OFFSET: 0x00515900
static void ResurrectNameQueryCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    WOWGUID offerer = CGPlayer_C::s_resurrectOfferer;
    NameCache* name = g_nameDBCache.GetRecord(CGPlayer_C::s_resurrectOfferer.m_guid, offerer, nullptr, nullptr, false);

    if (name) {
        FrameScript_SignalEvent(SCRIPT_EVENT_RESURRECT_REQUEST, "%s", name->m_name);
    }
}

// VERIFY: ff819f0d auto
// OFFSET: 0x00515950
static int32_t Script_ResurrectGetOfferer(lua_State* L) {
    WOWGUID offerer = CGPlayer_C::s_resurrectOfferer;
    NameCache* name;

    if (CGPlayer_C::s_resurrectOfferer && (name = g_nameDBCache.GetRecord(CGPlayer_C::s_resurrectOfferer.m_guid, offerer, &ResurrectNameQueryCallback, nullptr, false))) {
        lua_pushstring(L, name->m_name);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 94ea0fc9 auto
// OFFSET: 0x005159C0
static int32_t Script_ResurrectHasSickness(lua_State* L) {
    if (CGPlayer_C::s_resurrectOfferSickness) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 89a5aed4 auto
// OFFSET: 0x00515A00
static int32_t Script_ResurrectHasTimer(lua_State* L) {
    if (CGBattlefieldInfo::m_instanceType == 4 || !CGPlayer_C::s_resurrectOfferUseTimer) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: a43735ec manual
// OFFSET: 0x00515A90
static int32_t Script_GuildSetMOTD(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GuildSetMOTD(\"message\")");
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GUILD_MOTD));
    const char* text = lua_tolstring(L, 1, nullptr);
    char motd[516];
    motd[0] = 0;

    if (text && *text) {
        ScrubString(text, motd, 129, sizeof(motd), 1);
    }

    msg.PutString(motd);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: 789a94a7 manual
// OFFSET: 0x00516340
static int32_t Script_GetInstanceLockTimeRemaining(lua_State* L) {
    int32_t remaining;
    uint32_t completedMask;
    uint32_t extending;

    if (CGGameUI::m_instanceLockTime && (remaining = CGGameUI::m_instanceLockTime - OsGetAsyncTimeMs()) > 0) {
        completedMask = CGGameUI::m_instanceLockCompletedMask;
        extending = CGGameUI::m_instanceLockExtending;
    } else {
        remaining = 0;
        completedMask = 0;
        extending = 0;
    }

    lua_pushnumber(L, static_cast<uint32_t>(remaining) / 1000);
    lua_pushboolean(L, extending);
    uint32_t total;
    uint32_t completed;
    DungeonEncounterCountMatchingRows(CGGameUI::m_iCurrentMapID, CGGameUI::m_instanceDifficulty, completedMask, &total, &completed);
    lua_pushnumber(L, total);
    lua_pushnumber(L, completed);
    return 4;
}

// VERIFY: 4d5a4279 auto
// OFFSET: 0x00516410
static int32_t Script_GetInstanceLockTimeRemainingEncounter(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: GetInstanceLockTimeRemainingEncounter(encounterIndex)");
        return 0;
    }

    double index = lua_tonumber(L, 1);
    uint32_t remaining;
    uint32_t completedMask;
    uint32_t extending;
    CGGameUI::GetInstanceLockInfo(&remaining, &completedMask, &extending);

    if (remaining) {
        return PushDungeonEncounterInfo(L, CGGameUI::m_iCurrentMapID, CGGameUI::m_instanceDifficulty, static_cast<int32_t>(index - 1.0), completedMask);
    }

    return PushDungeonEncounterInfo(L, 0, 0, 0, 0);
}

// VERIFY: 10a9e962 auto
// OFFSET: 0x005EEB70
static void SummonConfirmNameCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
}

// VERIFY: 61947521 manual
// OFFSET: 0x00516510
static int32_t Script_GetSummonConfirmSummoner(lua_State* L) {
    WOWGUID summoner = CGGameUI::m_summonConfirmGUID;
    NameCache* name = g_nameDBCache.GetRecord(CGGameUI::m_summonConfirmGUID.m_guid, summoner, &SummonConfirmNameCallback, nullptr, false);

    if (name) {
        lua_pushstring(L, name->m_name);
    } else {
        lua_pushstring(L, "");
    }

    return 1;
}

// VERIFY: cb70885d manual
// OFFSET: 0x005167A0
static int32_t Script_ReplaceEnchant(lua_State* L) {
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(CGGameUI::m_lastItemSpellTarget, TYPE_ITEM, __FILE__, __LINE__));

    if (item) {
        Spell_C_HandleSpriteClick(item, nullptr);
    }

    return 0;
}

// VERIFY: d4f1a121 manual
// OFFSET: 0x00516AA0
static int32_t Script_GetGMTicketCategories(lua_State* L) {
    int32_t count = g_gmTicketCategoryDB.m_numRecords;
    int32_t results = 2 * count;
    lua_checkstack(L, results);

    for (int32_t i = 0; i < count; i++) {
        GMTicketCategoryRec* category = g_gmTicketCategoryDB.GetRecordByIndex(i);
        lua_pushnumber(L, category->m_id);
        lua_pushstring(L, category->m_category);
    }

    return results;
}

// VERIFY: 4ac3e219 manual
// OFFSET: 0x00516ED0
static int32_t Script_GetItemUniqueness(lua_State* L) {
    ItemLinkInfo linkInfo;
    linkInfo.Clear();
    int32_t itemId;

    if (lua_isnumber(L, 1)) {
        itemId = static_cast<int32_t>(lua_tonumber(L, 1));
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetItemUniqueness(itemID|\"name\"|\"itemlink\")");
            return 0;
        }

        const char* text = lua_tolstring(L, 1, nullptr);

        if (SStrStr(text, "item:")) {
            itemId = linkInfo.GetItemIdFromLink(text);
        } else {
            itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
        }
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    if (stats->m_flags[0] & 0x80000) {
        lua_pushnumber(L, -1.0);
        lua_pushnumber(L, 1.0);
        return 2;
    }

    uint32_t limitCategory = stats->m_itemLimitCategory;

    if (!limitCategory) {
        return 0;
    }

    ItemLimitCategoryRec* category = g_itemLimitCategoryDB.GetRecord(limitCategory);

    if (!category) {
        return 0;
    }

    lua_pushnumber(L, limitCategory);
    lua_pushnumber(L, category->m_quantity);
    return 2;
}

// VERIFY: e6c83734 auto
// OFFSET: 0x00517020
static int32_t Script_GetItemIcon(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    ItemRec* item = g_itemDB.GetRecord(itemId);

    if (!item) {
        return 0;
    }

    const char* path = ClientDBStringLookup(SLOOKUP_INVENTORYICONPATH);
    const char* separator = *path ? "\\" : "";
    char icon[260];
    SStrPrintf(icon, sizeof(icon), "%s%s%s", path, separator, CGItem_C::GetInventoryArt(item->m_displayInfoID));
    lua_pushstring(L, icon);
    return 1;
}

// VERIFY: 3714ecc3 auto
// OFFSET: 0x005171F0
static int32_t Script_IsHelpfulItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
    int32_t spellId = item ? item->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(itemId);
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && SpellRec::IsHelpfulOrHarmful(&spell) == 1) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: c3430e02 auto
// OFFSET: 0x005172E0
static int32_t Script_IsHarmfulItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
    int32_t spellId = item ? item->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(itemId);
    SpellRec spell;

    if (g_spellDB.GetRecord(spellId, &spell) && SpellRec::IsHelpfulOrHarmful(&spell) == 2) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 2c64e850 auto
// OFFSET: 0x005173E0
static int32_t Script_IsConsumableItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        lua_pushnil(L);
        return 1;
    }

    if (stats->m_inventoryType != 24 && stats->m_inventoryType != 25) {
        uint32_t i;

        for (i = 0; i < 5; i++) {
            if (!stats->m_spellId[i] || stats->m_spellTrigger[i]) {
                continue;
            }

            SpellRec spell;

            if (!g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
                continue;
            }

            int32_t reagents = 0;

            for (uint32_t j = 0; j < 8; j++) {
                if (spell.m_reagent[j] && spell.m_reagentCount[j] > 0) {
                    reagents = 1;
                    break;
                }
            }

            if (stats->m_spellCharges[i] < 0 || reagents) {
                break;
            }
        }

        if (i >= 5) {
            lua_pushnil(L);
            return 1;
        }
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: 8716e6b2 auto
// OFFSET: 0x00510EF0
static int32_t GetItemSpell(int32_t itemId) {
    CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);

    if (item) {
        return item->GetEnchantmentSpell(0);
    }

    return CGItem_C::GetUseSpell(itemId);
}

// VERIFY: bc910484 auto
// OFFSET: 0x00517530
static int32_t Script_IsDressableItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (!stats) {
        lua_pushnil(L);
        return 1;
    }

    if (stats->m_inventoryType) {
        lua_pushnumber(L, 1.0);
        return 1;
    }

    for (uint32_t i = 0; i < 5; i++) {
        if (!stats->m_spellId[i]) {
            continue;
        }

        SpellRec spell;

        if (!g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
            continue;
        }

        SpellRec learned;

        if (spell.m_effect[0] != SPELL_EFFECT_LEARN_SPELL) {
            continue;
        }

        int32_t learnedId = spell.m_effectTriggerSpell[0];

        if (!learnedId) {
            for (uint32_t j = 0; j < 5; j++) {
                if (stats->m_spellTrigger[j] == 6) {
                    learnedId = stats->m_spellId[j];
                    break;
                }
            }
        }

        if (!g_spellDB.GetRecord(learnedId, &learned) || learned.m_effect[0] != SPELL_EFFECT_CREATE_ITEM) {
            continue;
        }

        WOWGUID created = WOWGUID();
        ItemStats_C* createdStats = g_itemDBCache.GetRecord(learned.m_effectItemType[0], created, nullptr, nullptr, false);

        if (createdStats && createdStats->m_inventoryType) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 39dd6870 auto
// OFFSET: 0x00517890
static int32_t Script_GetFrameCPUUsage(lua_State* L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        luaL_error(L, "Usage: GetFrameCPUUsage(frame[, includeChildren])");
        return 0;
    }

    lua_rawgeti(L, 1, 0);
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
    lua_settop(L, -2);

    if (!frame) {
        luaL_error(L, "GetFrameCPUUsage(): Couldn't find 'this' in frame object");
        return 0;
    }

    if (!frame->IsA(CSimpleFrame::GetObjectType())) {
        luaL_error(L, "GetFrameCPUUsage(): Wrong object type, expected frame");
        return 0;
    }

    double time = 0.0;
    uint32_t count = 0;
    frame->GetScriptTime(L, &time, &count, StringToBOOL(L, 2, 1));
    lua_pushnumber(L, time);
    lua_pushnumber(L, count);
    return 2;
}

// VERIFY: 19f55934 manual
// OFFSET: 0x005179B0
static int32_t Script_GetAddOnDependencies(lua_State* L) {
    const char* name;

    if (lua_isnumber(L, 1)) {
        name = GetAddOnByIndex(static_cast<uint32_t>(lua_tonumber(L, 1)) - 1);

        if (!name) {
            luaL_error(L, "AddOn index must be in the range of 1 to %d", GetNumAddOns());
            return 0;
        }
    } else {
        if (!lua_isstring(L, 1)) {
            luaL_error(L, "Usage: GetAddOnDependencies(index or \"name\")");
            return 0;
        }

        name = lua_tolstring(L, 1, nullptr);
    }

    TSGrowableArray<char*>* dependencies = GetAddonDeps(name);

    if (!dependencies) {
        return 0;
    }

    lua_checkstack(L, dependencies->m_count);

    for (uint32_t i = 0; i < dependencies->m_count; i++) {
        lua_pushstring(L, dependencies->m_data[i]);
    }

    return dependencies->m_count;
}

// VERIFY: a9ca381c auto
// OFFSET: 0x00517AA0
static int32_t Script_GetMirrorTimerProgress(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: GetMirrorTimerInfo(\"timer\")");
        return 0;
    }

    uint32_t index = CGGameUI::MirrorTimerToIndex(lua_tolstring(L, 1, nullptr));

    if (index > UNIT_MIRROR_TIMER_FEIGNDEATH) {
        luaL_error(L, "ERROR: Invalid timer (Ex: \"BREATH\"");
        return 0;
    }

    MirrorTimerInfo* timer = &CGGameUI::m_mirrorTimerInfo[index];

    if (!timer) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    lua_pushnumber(L, timer->m_value + timer->m_scale * static_cast<int32_t>(OsGetAsyncTimeMs() - timer->m_startTime));
    return 1;
}

// VERIFY: b065ae4b manual
// OFFSET: 0x0051A3D0
static int32_t Script_EquipCursorItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: EquipCursorItem(slot)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID item = CGGameUI::m_cursorItem;
    WOWGUID container = CGGameUI::m_cursorItemContainer;
    uint32_t sourceSlot = CGGameUI::m_cursorItemSlot;

    if (!item) {
        return 0;
    }

    int32_t slot = static_cast<int32_t>(lua_tonumber(L, 1));
    uint32_t index = slot - 1;

    if (slot != 256 && slot) {
        WOWGUID current;
        current.m_guid = player->m_bag.GetItemGuid(index);

        if (item == current) {
            CGGameUI::ClearCursor(1, 1);
            return 0;
        }

        if (player->ValidateSlot(index, item)) {
            player->SwapItems(item, container, sourceSlot, player->m_obj->m_guid, index, 0);
            CGGameUI::LockItem(current);
            return 0;
        }

        if (container == player->m_obj->m_guid && sourceSlot < 19) {
            CGGameUI::ClearCursor(1, 1);
            return 0;
        }
    }

    player->AutoEquipCursorItem(1);
    return 0;
}

// VERIFY: 313f3ea3 auto
// OFFSET: 0x0051A530
static int32_t Script_EquipPendingItem(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: EquipPendingItem(index)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->ClearPendingEquip(static_cast<uint32_t>(lua_tonumber(L, 1)), 1);
    }

    return 0;
}

// VERIFY: cd050a1b auto
// OFFSET: 0x0051A5C0
static int32_t Script_CancelPendingEquip(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: CancelPendingEquip(index)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->ClearPendingEquip(static_cast<uint32_t>(lua_tonumber(L, 1)), 0);
    }

    return 0;
}

// VERIFY: 3aaba36e manual
// OFFSET: 0x0051A650
static int32_t Script_AttackTarget(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->OnAttackIconPressed(WOWGUID(), 0);
    }

    return 0;
}

// VERIFY: 9a711012 manual
// OFFSET: 0x0051A6B0
static int32_t Script_ToggleSheath(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->ToggleSheathe();
    }

    return 0;
}

// VERIFY: 2d52f950 auto
// OFFSET: 0x0051A6E0
static int32_t Script_InviteUnit(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    const char* name = lua_tolstring(L, 1, nullptr);

    if (!name) {
        return 0;
    }

    if (SStrLen(name) > 48) {
        luaL_error(L, "Name too long");
        return 0;
    }

    uint32_t flags = 0;

    if (StringToBOOL(L, 2, 0)) {
        flags = 2;
    }

    if (StringToBOOL(L, 3, 0)) {
        flags |= 4;
    }

    if (StringToBOOL(L, 4, 0)) {
        flags |= 8;
    }

    player->GroupInvite(name, flags);
    return 0;
}

// VERIFY: 7cdc6cc7 manual
// OFFSET: 0x0051A7A0
static int32_t Script_UninviteUnit(lua_State* L) {
    if (!ClntObjMgrGetActivePlayerObj()) {
        return 0;
    }

    if (lua_tainted && CGGameUI::m_simpleTop && !CGGameUI::m_simpleTop->unused_1250) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 1);
        return 0;
    }

    const char* token = lua_tolstring(L, 1, nullptr);
    const char* reason = lua_tolstring(L, 2, nullptr);
    char scrubbed[256];
    scrubbed[0] = 0;

    if (reason && *reason) {
        ScrubString(reason, scrubbed, 64, sizeof(scrubbed), 1);
    }

    WOWGUID guid;

    if (Script_GetGUIDFromToken(token, guid, true)) {
        CGPlayer_C::Uninvite(guid, scrubbed);
    }

    return 0;
}

// VERIFY: dac75fc9 auto
// OFFSET: 0x0051A880
static int32_t Script_CanShowResetInstances(lua_State* L) {
    if (CGGameUI::CanResetInstance()) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 28670361 manual
// OFFSET: 0x0051ACD0
static int32_t Script_HasSoulstone(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || player->m_unit->m_unitFieldHealth > 0) {
        lua_pushnil(L);
        return 1;
    }

    if (!player->m_player->m_playerSelfResSpell) {
        CGItem_C* soulstone = player->GetSoulstone();

        if (soulstone) {
            char name[1024];
            soulstone->GetFullItemName(name, sizeof(name));
            lua_pushstring(L, name);
            return 1;
        }

        lua_pushnil(L);
        return 1;
    }

    SpellRec spell;
    lua_pushstring(L, g_spellDB.GetRecord(player->m_player->m_playerSelfResSpell, &spell) ? spell.m_name : "UNKNOWN");
    return 1;
}

// VERIFY: 85011863 manual
// OFFSET: 0x0051ADD0
static int32_t Script_UseSoulstone(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (player->m_player->m_playerSelfResSpell) {
        if (player->CheckSelfResurrect()) {
            WDataStore msg;
            msg.Put(static_cast<uint32_t>(CMSG_SELF_RES));
            msg.Finalize();
            ClientServices::Send2(&msg);
        }

        return 0;
    }

    CGItem_C* soulstone = player->GetSoulstone();

    if (soulstone) {
        Spell_C_CancelPlayerSpells();
        WOWGUID target = WOWGUID();
        soulstone->UseItem(&target, 0);
    }

    return 0;
}

// VERIFY: 419661cb manual
// OFFSET: 0x0051B050
static int32_t Script_ConfirmSummon(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player->IsDeadOrGhost()) {
        return 0;
    }

    uint32_t flags = player->m_unit->m_unitFieldFlags;

    if ((flags & 0x40000) || (flags & 0x800000) || (flags & 0x400000) || (flags & 0x80000)) {
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SUMMON_RESPONSE));
    msg.Put(CGGameUI::m_summonConfirmGUID.m_guid);
    msg.Put(static_cast<uint8_t>(1));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: a1b14380 manual
// OFFSET: 0x0051B240
static int32_t Script_CheckInteractDistance(lua_State* L) {
    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: CheckInteractDistance(\"unit\", distIndex)");
        return 0;
    }

    static float s_interactDistances[4] = { s_inspectDistSq, s_playerInteractDistSq, s_duelDistSq, s_followDistSq };
    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 1, nullptr));
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    uint32_t index = static_cast<uint32_t>(lua_tonumber(L, 2)) - 1;

    if (player && unit && index < 4) {
        C3Vector unitPosition = unit->GetPosition();
        C3Vector delta = player->GetPosition() - unitPosition;

        if (delta.y * delta.y + delta.x * delta.x + delta.z * delta.z < s_interactDistances[index]) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 07946c65 manual
// OFFSET: 0x0051B6A0
static int32_t Script_AcceptXPLoss(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->TalkToSpiritHealer(WOWGUID());
    }

    return 0;
}

// VERIFY: cab30f22 manual
// OFFSET: 0x0051BDD0
static int32_t Script_DropItemOnUnit(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: DropItemOnUnit(\"unit\")");
        return 0;
    }

    WOWGUID cursor = CGGameUI::m_cursorItemType == UICURSOR_ITEM ? CGGameUI::m_cursorItem : WOWGUID();
    CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(cursor, TYPE_ITEM, __FILE__, __LINE__));

    if (!item) {
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 1, nullptr));

    if (!unit || !(unit->m_obj->m_type & TYPE_UNIT)) {
        return 0;
    }

    if (unit->m_obj->m_type & TYPE_PLAYER) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (unit->m_obj->m_guid == ClntObjMgrGetActivePlayer()) {
            if (!player) {
                SErrDisplayAppFatalMessage("", "", 0);
            }

            player->AutoEquipCursorItem(0);
        } else if (player && player->CanCooperate(unit)) {
            if (unit->m_obj->m_guid == CGTradeInfo::m_tradingPlayer) {
                CGTradeInfo::AddCursorItem(item->IsBound());
            } else if (!CGTradeInfo::m_tradingPlayer) {
                CGTradeInfo::InitiateTrade(unit->m_obj->m_guid, 1);
            }
        }
    }

    if (unit->m_obj->m_guid == CGPetInfo::GetPet(0) && Spell_C_HandleItemDrop(item, unit)) {
        CGGameUI::ClearCursor(1, 1);
    }

    return 0;
}

// VERIFY: 4d251062 manual
// OFFSET: 0x0051BF50
static int32_t Script_ConfirmTalentWipe(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->TalkToTalentMaster(WOWGUID(), 0);
    }

    return 0;
}

// VERIFY: 0e25a655 manual
// OFFSET: 0x0051BF90
static int32_t Script_ConfirmBinder(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        player->TalkToBinder(WOWGUID());
    }

    return 0;
}

// VERIFY: 4cd0c782 manual
// OFFSET: 0x0051C0B0
static int32_t Script_ShowHelm(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (StringToBOOL(L, 1, 1)) {
        player->ShowHelm();
        return 0;
    }

    player->HideHelm();
    return 0;
}

// VERIFY: 35a04815 manual
// OFFSET: 0x0051C100
static int32_t Script_ShowCloak(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    if (StringToBOOL(L, 1, 1)) {
        player->ShowCloak();
        return 0;
    }

    player->HideCloak();
    return 0;
}

// VERIFY: 690dd681 manual
// OFFSET: 0x0051C150
static int32_t Script_GetItemGem(lua_State* L) {
    int32_t enchantId = 0;

    if (!lua_isstring(L, 1) || !lua_isnumber(L, 2)) {
        luaL_error(L, "Usage: GetItemGem(\"name\"|\"itemlink\", index)");
        return 0;
    }

    uint32_t index = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;

    if (index > 2) {
        return 0;
    }

    const char* text = lua_tolstring(L, 1, nullptr);

    if (SStrStr(text, "item:")) {
        ItemLinkInfo link;
        link.Clear();
        link.GetItemIdFromLink(text);
        enchantId = link.m_enchants[index + 2];
    } else {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (!player) {
            return 0;
        }

        WOWGUID container;
        uint32_t slot;
        CGItem_C* item = player->m_bag.GetItemIdByName(lua_tolstring(L, 1, nullptr), container, slot, 0);

        if (item) {
            enchantId = item->GetEnchantmentId(index + 2);
        }
    }

    int32_t gemId = 0;
    SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchantId);

    if (enchant) {
        gemId = enchant->m_srcItemID;
    }

    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(gemId, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    char name[1024];
    CGItem_C::BuildItemName(name, sizeof(name), gemId, 0);
    lua_pushstring(L, name);
    lua_pushstring(L, CGTooltip::GetItemHyperlinkString(gemId, stats->m_quality, 0, 0, 0));
    return 2;
}

// VERIFY: 4ed18199 manual
// OFFSET: 0x0051C2E0
static int32_t Script_GetItemCount(lua_State* L) {
    int32_t includeBank = StringToBOOL(L, 2, 0);
    int32_t includeCharges = StringToBOOL(L, 3, 0);
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    double count;

    if (player) {
        uint32_t flags = 0;

        if (includeBank) {
            flags = 8;
        }

        if (includeCharges) {
            flags |= 0x80;
        }

        int32_t total = player->m_bag.GetItemTypeCount(itemId, flags);

        if (total && includeCharges) {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

            if (stats && stats->m_stackable <= 1) {
                CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
                int32_t charges = item ? item->GetUsableSpellCharges() : CGItem_C::NumBaseCharges(itemId);

                if (charges < 0) {
                    total = player->GetBag()->GetItemTypeCharges(itemId, 0);
                }

                int32_t spellId = item ? item->GetEnchantmentSpell(0) : CGItem_C::GetUseSpell(itemId);
                int32_t uses = Spell_C_GetSpellReagentUses(spellId);

                if (uses) {
                    total = uses;
                }
            }
        }

        count = total;
    } else {
        count = 0.0;
    }

    lua_pushnumber(L, count);
    return 1;
}

// VERIFY: dab459ea manual
// OFFSET: 0x0051C450
static int32_t Script_IsUsableItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    bool usable = false;
    bool noMana = false;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && itemId && player->m_bag.GetItemTypeCount(itemId, 0) && !Spell_C_NeedsCooldownEvent(itemId)) {
        WOWGUID none = WOWGUID();
        ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);
        bool check = true;

        if (stats) {
            MapRec* map = g_mapDB.GetRecord(CGGameUI::m_iCurrentMapID);

            if (map && map->m_instanceType == 4 && !(stats->m_flags[0] & 0x200000)) {
                CGItem_C* item = CGActionBar::FindUseEnchantItemPtr(itemId);
                uint32_t cooldown;
                uint32_t categoryCooldown;
                uint32_t category;
                int32_t charges;

                if (item) {
                    charges = item->GetUsableSpellCharges();
                    item->GetSpellCooldownInfo(cooldown, categoryCooldown, category);
                } else {
                    charges = CGItem_C::NumBaseCharges(itemId);
                    CGItem_C::GetCooldowns(itemId, cooldown, categoryCooldown, category);
                }

                if (charges < 0 || static_cast<int32_t>(cooldown) > 900000 || static_cast<int32_t>(categoryCooldown) > 900000) {
                    check = false;
                }
            }
        }

        if (check) {
            SpellRec spell;

            if (g_spellDB.GetRecord(GetItemSpell(itemId), &spell)) {
                WOWGUID target = WOWGUID();

                if (Spell_C_IsUsableAction(player, &spell, target, stats, false)) {
                    usable = Spell_C_HaveSpellPower(player, &spell);
                    noMana = !usable;
                }
            } else {
                usable = true;
            }
        }
    }

    if (usable) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    if (noMana) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 2;
}

// VERIFY: 35e2d9c5 auto
// OFFSET: 0x005176F0
int32_t CanEquipInSlotCallback(const CGItem_C* item, void* param) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);
    return stats && static_cast<int32_t>(stats->m_inventoryType) == *static_cast<int32_t*>(param);
}

// VERIFY: 50448300 manual
// OFFSET: 0x00517740
int32_t FindItemClassTypeCallback(const CGItem_C* item, void* param) {
    const char* type = static_cast<const char*>(param);
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(item->m_obj->m_entryID, none, nullptr, nullptr, false);

    if (!stats) {
        return 0;
    }

    ItemClassRec* itemClass = g_itemClassDB.GetRecord(stats->m_class);

    if (itemClass && !SStrCmpUTF8I(itemClass->m_className, type, STORM_MAX_STR)) {
        return 1;
    }

    int32_t i = g_itemSubClassDB.m_numRecords;

    while (i) {
        i--;
        const ItemSubClassRec* rec = i >= 0 && i < g_itemSubClassDB.m_numRecords ? &g_itemSubClassDB.m_records[i] : nullptr;

        if (rec && rec->m_classID == static_cast<int32_t>(stats->m_class) && rec->m_subClassID == static_cast<int32_t>(stats->m_subClass)) {
            if (!SStrCmpUTF8I(rec->m_verboseName, type, STORM_MAX_STR) || !SStrCmpUTF8I(rec->m_displayName, type, STORM_MAX_STR)) {
                return 1;
            }

            break;
        }
    }

    i = g_itemSubClassMaskDB.m_numRecords;

    while (i) {
        i--;
        const ItemSubClassMaskRec* rec = i >= 0 && i < g_itemSubClassMaskDB.m_numRecords ? &g_itemSubClassMaskDB.m_records[i] : nullptr;

        if (rec->m_classID == static_cast<int32_t>(stats->m_class) && ((1 << stats->m_subClass) & rec->m_mask) && !SStrCmpUTF8I(rec->m_name, type, STORM_MAX_STR)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 6832b443 manual
// OFFSET: 0x0051C690
static int32_t Script_IsEquippedItem(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        CGItem_C* item;

        if (lua_isnumber(L, 1)) {
            item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), 1);
        } else {
            item = player->m_bag.FindItemByFullName(lua_tolstring(L, 1, nullptr), 1);
        }

        if (item) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 356b32c8 auto
// OFFSET: 0x0051C740
static int32_t Script_IsEquippedItemType(lua_State* L) {
    const char* type = lua_tolstring(L, 1, nullptr);

    if (!type || !*type) {
        luaL_error(L, "Usage: IsEquippedItemType(\"type\")");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        int32_t inventoryType = -1;

        for (int32_t i = 0; i < 29; i++) {
            if (!SStrCmpI(type, s_inventoryTypeTokens[i], STORM_MAX_STR) || !SStrCmpUTF8I(type, FrameScript_GetText(s_inventoryTypeTokens[i], -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR)) {
                inventoryType = i;
                break;
            }
        }

        WOWGUID container;
        uint32_t slot;
        CGItem_C* item;

        if (inventoryType > 0) {
            item = player->m_bag.FindItem(&CanEquipInSlotCallback, &inventoryType, container, slot, 1);
        } else {
            item = player->m_bag.FindItem(&FindItemClassTypeCallback, const_cast<char*>(type), container, slot, 1);
        }

        if (item) {
            lua_pushnumber(L, 1.0);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: aded2148 manual
// OFFSET: 0x0051C870
static int32_t Script_ItemHasRange(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(GetItemSpell(itemId), &spell)) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

        if (player) {
            float minRange;
            float maxRange;
            Spell_C_GetMinMaxRange(player, &spell, &minRange, &maxRange, nullptr);

            if (fnotequal(minRange, 0.0f) || fnotequal(maxRange, 0.0f)) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: e5d340e5 manual
// OFFSET: 0x0051C9C0
static int32_t Script_IsItemInRange(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(GetItemSpell(itemId), &spell)) {
        CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
        WOWGUID target;
        uint8_t inRange;

        if (player && Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true) && Spell_C_RangeCheckSelected(player, &spell, target, &inRange, false, nullptr)) {
            if (inRange) {
                lua_pushnumber(L, 1.0);
            } else {
                lua_pushnumber(L, 0.0);
            }

            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

// VERIFY: 6d6ce396 manual
// OFFSET: 0x0051CD00
static int32_t Script_UseItemByName(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        CGItem_C* item;

        if (lua_isnumber(L, 1)) {
            item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), 0);
        } else {
            item = player->m_bag.FindItemByFullName(lua_tolstring(L, 1, nullptr), 0);
        }

        if (item) {
            WOWGUID target;

            if (Script_GetGUIDFromToken(lua_tolstring(L, 2, nullptr), target, true)) {
                item->UseItem(&target, 0);
            }
        }
    }

    return 0;
}

// VERIFY: 01569489 manual
// OFFSET: 0x0051CDB0
static int32_t Script_EquipItemByName(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID container;
    uint32_t slot;
    CGItem_C* item;

    if (!lua_isnumber(L, 2) && !lua_isstring(L, 2)) {
        if (lua_isnumber(L, 1)) {
            item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), container, slot, 6);

            if (!container) {
                item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), container, slot, 0);
            }
        } else {
            item = player->m_bag.GetItemIdByName(lua_tolstring(L, 1, nullptr), container, slot, 6);

            if (!container) {
                item = player->m_bag.GetItemIdByName(lua_tolstring(L, 1, nullptr), container, slot, 0);
            }
        }

        if (!container) {
            return 0;
        }

        if (item && !(item->m_itemFlags & 0x1) && (player->m_bag.m_guid != container || slot > 18)) {
            player->AutoEquipItem(container, slot, 0);
        }

        return 0;
    }

    uint32_t dstSlot = 0;

    if (lua_isnumber(L, 2)) {
        dstSlot = static_cast<int32_t>(lua_tonumber(L, 2)) - 1;
    } else {
        int32_t found = 0;

        if (lua_isstring(L, 2)) {
            const char* name = lua_tolstring(L, 2, nullptr);

            for (int32_t i = 0; i < g_paperDollItemFrameDB.m_numRecords; i++) {
                const PaperDollItemFrameRec* rec = i >= 0 && i < g_paperDollItemFrameDB.m_numRecords ? &g_paperDollItemFrameDB.m_records[i] : nullptr;

                if (!SStrCmpI(rec->m_itemButtonName, name, STORM_MAX_STR)) {
                    dstSlot = rec->m_slotNumber;
                    found = 1;
                    break;
                }
            }
        }

        if (!found) {
            luaL_error(L, "EquipItemByName(): Invalid inventory dstSlot");
        }
    }

    if (dstSlot > 18) {
        luaL_error(L, "EquipItemByName(): Invalid inventory dstSlot");
    }

    if (lua_isnumber(L, 1)) {
        item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), container, slot, 6);

        if (!item) {
            item = player->m_bag.FindItemOfType(lua_tointeger(L, 1), container, slot, 0);
        }
    } else {
        item = player->m_bag.GetItemIdByName(lua_tolstring(L, 1, nullptr), container, slot, 6);

        if (!item) {
            item = player->m_bag.GetItemIdByName(lua_tolstring(L, 1, nullptr), container, slot, 0);
        }
    }

    if (!item) {
        return 0;
    }

    if ((item->m_itemFlags & 0x1) || (player->m_bag.m_guid == container && slot == dstSlot)) {
        return 0;
    }

    player->AutoEquipItemToSlot(item, dstSlot, 0);
    return 0;
}

// VERIFY: 27a04bf5 manual
// OFFSET: 0x0051D0B0
static int32_t Script_StopAttack(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && player->IsInMelee() && CGGameUI::m_hasControl) {
        player->SetCombatMode();
    }

    return 0;
}

// VERIFY: 0930b5b7 manual
// OFFSET: 0x0051D170
static int32_t Script_Dismount(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player && !player->IsDeadOrGhost()) {
        player->Dismount();
    }

    return 0;
}

// VERIFY: a24c6052 manual
// OFFSET: 0x0051D230
static int32_t Script_CanGrantLevel(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!player || player->CanGrantLevel(guid)) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: cbb4f9f3 manual
// OFFSET: 0x0051D2B0
static int32_t Script_CanSummonFriend(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!player || CGPlayer_C::CanSummonFriend(guid)) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushnumber(L, 1.0);
    return 1;
}

// VERIFY: e80fbb0d auto
// OFFSET: 0x0051D590
static int32_t Script_GameMovieFinished(lua_State* L) {
    CGGameUI::StopMovie();
    return 0;
}

// VERIFY: f280f677 auto
// OFFSET: 0x005222E0
static int32_t Script_ReloadUI(lua_State* L) {
    if (lua_tainted && CGGameUI::m_simpleTop && !CGGameUI::m_simpleTop->unused_1250) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 1);
        return 0;
    }

    CGGameUI::m_reloadUIRequested = 1;
    return 0;
}

// VERIFY: e3dc4253 manual
// OFFSET: 0x00522320
static int32_t Script_DeleteCursorItem(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 0;
    }

    WOWGUID item = CGGameUI::m_cursorItem;
    uint32_t slot = CGGameUI::m_cursorItemSlot;

    if (!item) {
        return 0;
    }

    uint8_t bag = player->FindSlotIndex(CGGameUI::m_cursorItemContainer);
    CGItem_C* object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (object && (object->GetItemStaticFlags(0) & 0x20)) {
        CGGameUI::DisplayError(ERR_ITEM_CANT_BE_DESTROYED);
        return 0;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_DESTROY_ITEM));
    msg.Put(bag);
    msg.Put(static_cast<uint8_t>(slot));
    msg.Put(static_cast<uint32_t>(CGGameUI::m_stackSplit));
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGameUI::ClearCursor(1, 1);

    object = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(item, TYPE_ITEM, __FILE__, __LINE__));

    if (object) {
        object->Lock();
    }

    CGActionBar::UpdateUsable();
    return 0;
}

// VERIFY: 1a059359 manual
// OFFSET: 0x005224C0
static int32_t Script_FollowUnit(lua_State* L) {
    const char* name = lua_tolstring(L, 1, nullptr);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_PLAYER, 3, StringToBOOL(L, 2, 0), FLT_MAX, 0);

    if (!guid) {
        guid = CGGameUI::ClosestObjectMatch(name, TYPE_UNIT, 0, StringToBOOL(L, 2, 0), FLT_MAX, 0);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (mover) {
            WOWGUID target = unit->m_obj->m_guid;
            mover->SetTrackingTarget(target, TRACKTYPE_FOLLOW);
        }

        return 0;
    }

    if (name && *name && SStrCmpI(name, "target", STORM_MAX_STR)) {
        CGGameUI::DisplayError(ERR_UNIT_NOT_FOUND);
        return 0;
    }

    CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
    return 0;
}

// VERIFY: 366c1156 auto
// OFFSET: 0x005225E0
static int32_t Script_ClearFocus(lua_State* L) {
    WOWGUID guid = WOWGUID();
    CGGameUI::SetFocusTarget(guid);
    return 0;
}

// VERIFY: a4660e2b auto
// OFFSET: 0x00522600
static int32_t Script_InitiateTrade(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: InitiateTrade(\"unit\")");
        return 0;
    }

    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));

    if (!guid) {
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CGPlayer_C* target = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(guid, TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !target) {
        return 0;
    }

    if (player->IsDeadOrGhost()) {
        CGGameUI::DisplayError(ERR_PLAYER_DEAD);
        return 0;
    }

    if (ClntObjMgrGetActivePlayer() == guid) {
        CGGameUI::DisplayError(ERR_TRADE_SELF);
        return 0;
    }

    if (player->CanCooperate(target)) {
        CGTradeInfo::InitiateTrade(guid, 0);
    } else {
        CGGameUI::DisplayError(ERR_PLAYER_WRONG_FACTION);
    }

    return 0;
}

// VERIFY: 945d5573 manual
// OFFSET: 0x00522710
static int32_t Script_CanInspect(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: CanInspect(unit [, showError])");
        return 0;
    }

    const char* token = lua_tolstring(L, 1, nullptr);
    CGUnit_C* unit = GetUnitFromName(token);

    if (StringToBOOL(L, 2, 0)) {
        if (unit) {
            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player && (unit->m_obj->m_type & TYPE_PLAYER) && !player->CanAttack(unit)) {
                C3Vector unitPosition = unit->GetPosition();
                C3Vector delta = player->GetPosition() - unitPosition;

                if (delta.y * delta.y + delta.z * delta.z + delta.x * delta.x > s_inspectDistSq) {
                    CGGameUI::DisplayError(ERR_OUT_OF_RANGE);
                }
            } else {
                CGGameUI::DisplayError(ERR_INVALID_INSPECT_TARGET);
            }
        } else if (token && *token && SStrCmpI(token, "target", STORM_MAX_STR)) {
            CGGameUI::DisplayError(ERR_UNIT_NOT_FOUND);
        } else {
            CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
        }
    }

    if (CGGameUI::CanInspect(unit)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 643ec510 manual
// OFFSET: 0x00522870
static int32_t Script_NotifyInspect(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: NotifyInspect(unit)");
        return 0;
    }

    CGUnit_C* unit = GetUnitFromName(lua_tolstring(L, 1, nullptr));

    if (CGGameUI::CanInspect(unit)) {
        CDataStore msg;
        msg.Put(static_cast<uint32_t>(CMSG_INSPECT));
        msg.Put(unit->m_obj->m_guid.m_guid);
        msg.Finalize();
        ClientServices::Send2(&msg);
        WOWGUID guid = unit->m_obj->m_guid;
        CGCharacterInfo::SetInspectTarget(guid);
    }

    return 0;
}

// VERIFY: d3b8f784 auto
// OFFSET: 0x00522980
static int32_t Script_PickupPlayerMoney(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: PickupPlayerMoney(amount)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (player) {
        uint32_t amount = static_cast<uint32_t>(lua_tonumber(L, 1));

        if (amount && amount <= player->GetCoinage()) {
            CGGameUI::SetCursorMoney(amount);
        }
    }

    return 0;
}

// STATIC: 0x00ACC600
static int32_t s_setTitleCount = 10;

// STATIC: 0x00BD0CB8
static float s_setTitleTime;

// VERIFY: 33bd568d manual
// OFFSET: 0x00522E30
static int32_t Script_SetCurrentTitle(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetCurrentTitle(titleMaskID)");
        return 0;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t title = static_cast<int32_t>(lua_tonumber(L, 1));

    if (!player) {
        return 0;
    }

    if (lua_tainted && CGGameUI::m_simpleTop && !CGGameUI::m_simpleTop->unused_1250) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 1);
        return 0;
    }

    if (++s_setTitleCount > 10) {
        float now = OsGetAsyncTimeSec();

        if (now - s_setTitleTime >= 60.0f) {
            s_setTitleTime = now;
            s_setTitleCount = 0;
        } else if (s_setTitleCount > 10) {
            CGGameUI::DisplayError(ERR_CLIENT_LOCKED_OUT);
            return 0;
        }
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_TITLE));
    msg.Put(static_cast<uint32_t>(title));
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: bfb4760c auto
// OFFSET: 0x00522F70
static int32_t Script_BindEnchant(lua_State* L) {
    CGGameUI::CastItemSpell(CGGameUI::m_lastItemSpellTarget, 1);
    return 0;
}

// VERIFY: d9d9fded auto
// OFFSET: 0x00522FA0
static int32_t Script_CancelAreaSpiritHeal(lua_State* L) {
    CGGameUI::CastSpiritHealSpell();
    return 0;
}

// VERIFY: a8811188 manual
// OFFSET: 0x00522FB0
static int32_t Script_PickupItem(lua_State* L) {
    int32_t itemId = 0;

    if (lua_isnumber(L, 1)) {
        itemId = lua_tointeger(L, 1);
    } else if (lua_isstring(L, 1)) {
        itemId = GetItemIDByName(lua_tolstring(L, 1, nullptr));
    }

    ItemRec* rec = g_itemDB.GetRecord(itemId);
    int32_t displayId = rec ? rec->m_displayInfoID : 0;
    CGGameUI::SetCursorVirtualItem(itemId, displayId, 0, UICURSOR_ACTIONBAR, 0);
    return 0;
}

// VERIFY: 530f95b1 manual
// OFFSET: 0x00523090
static int32_t Script_StartAttack(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    int32_t hasTarget = 0;

    if (!player) {
        return 0;
    }

    WOWGUID guid = WOWGUID();
    const char* token = lua_tolstring(L, 1, nullptr);

    if (token && *token) {
        if (!Script_GetGUIDFromToken(token, guid, false)) {
            guid = CGGameUI::ClosestObjectMatch(token, TYPE_UNIT, 1, StringToBOOL(L, 2, 0), FLT_MAX, 1);
        }

        hasTarget = 1;
    }

    if (!player->IsInMelee()) {
        player->OnAttackIconPressed(guid, hasTarget);
    }

    return 0;
}

// VERIFY: f467e068 manual
// OFFSET: 0x00523160
static int32_t Script_GrantLevel(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));
    int32_t error = player->CanGrantLevel(guid);

    if (!player || error) {
        BroadcastUIError(error, nullptr);
        return 0;
    }

    WDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GRANT_LEVEL));
    SmartGUID target;
    target.m_guid = guid.m_guid;
    msg << target;
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: bd8e1d89 auto
// OFFSET: 0x00523210
static void SummonFriendErrorCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (!success) {
        return;
    }

    WOWGUID none = WOWGUID();
    NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, nullptr, nullptr, false);

    if (name) {
        BroadcastUIError(static_cast<int32_t>(reinterpret_cast<intptr_t>(param)), name->m_name);
    }
}

// VERIFY: e5bdfb39 manual
// OFFSET: 0x00523260
static int32_t Script_SummonFriend(lua_State* L) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    WOWGUID guid = GetGuidByKeyword(lua_tolstring(L, 1, nullptr));
    int32_t error = CGPlayer_C::CanSummonFriend(guid);

    if (!player || error) {
        if (error != 9 && error != 13) {
            BroadcastUIError(error, nullptr);
            return 0;
        }

        WOWGUID none = WOWGUID();
        NameCache* name = g_nameDBCache.GetRecord(guid.m_guid, none, &SummonFriendErrorCallback, reinterpret_cast<void*>(static_cast<intptr_t>(error)), false);

        if (name) {
            BroadcastUIError(error, name->m_name);
        }

        return 0;
    }

    int32_t retarget = 0;

    if (CGGameUI::m_lockedTarget != guid) {
        retarget = 1;
        CGGameUI::SendTarget(guid);
    }

    Spell_C::CastSpell(CGSpellBook::m_summonFriendSpell, nullptr, guid, false);

    if (retarget) {
        CGGameUI::SendTarget(CGGameUI::m_lockedTarget);
    }

    return 0;
}

// VERIFY: 336995ce auto
// OFFSET: 0x00523360
static int32_t Script_AcceptLevelGrant(lua_State* L) {
    CGGameUI::AcceptLevelGrant();
    return 0;
}

// VERIFY: 81c40723 auto
// OFFSET: 0x00523370
static int32_t Script_EndRefund(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        return 0;
    }

    int32_t type = static_cast<int32_t>(lua_tonumber(L, 1));

    if (type == 1) {
        CGGameUI::CastItemSpell(CGGameUI::m_lastItemSpellTarget, 1);
        return 0;
    }

    if (type == 2) {
        CGItemSocketInfo::Accept();
    }

    return 0;
}

// VERIFY: 1fe4019a auto
// OFFSET: 0x005233D0
static int32_t Script_EndBoundTradeable(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: EndBoundTradeable(\"type\")");
        return 0;
    }

    const char* type = lua_tolstring(L, 1, nullptr);

    if (!SStrCmpI(type, "itemenchant", STORM_MAX_STR)) {
        CGGameUI::CastItemSpell(CGGameUI::m_lastItemSpellTarget, 1);
        return 0;
    }

    if (!SStrCmpI(type, "gem", STORM_MAX_STR)) {
        CGItemSocketInfo::Accept();
        return 0;
    }

    if (!SStrCmpI(type, "spellenchant", STORM_MAX_STR) && CGGameUI::m_lastEnchantSpellID) {
        Spell_C::CastSpell(CGGameUI::m_lastEnchantSpellID, nullptr, CGGameUI::m_lastItemSpellTarget, true);
        CGGameUI::m_lastEnchantSpellID = 0;
    }

    return 0;
}

// VERIFY: e1888672 manual
// OFFSET: 0x005259E0
static void TargetIfValid(const WOWGUID& guid) {
    if (ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__) || CGPartyInfo::IsMemberOrPet(guid) || CGRaidInfo::IsPlayerOrPetInRaid(guid)) {
        CGGameUI::Target(guid);
    }
}

// VERIFY: d99aad91 auto
// OFFSET: 0x00525A30
static int32_t Script_TargetUnit(lua_State* L) {
    const char* name = lua_tolstring(L, 1, nullptr);
    int32_t exact = StringToBOOL(L, 2, 0);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_UNIT, 0, exact, FLT_MAX, 1);

    if (guid) {
        TargetIfValid(guid);
    }

    return 0;
}

// VERIFY: a9d8ce70 manual
// OFFSET: 0x00525C30
static int32_t Script_TargetDirectionEnemy(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TargetDirectionEnemy(facing)");
        return 0;
    }

    if (CGUIMacros::IsActionAllowed(5)) {
        float facing = static_cast<float>(lua_tonumber(L, 1));
        float angle = static_cast<float>(lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 0.0);
        CGGameUI::TargetDirection(facing, angle, 1);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: 38ff157a manual
// OFFSET: 0x00525CD0
static int32_t Script_TargetDirectionFriend(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: TargetDirectionFriend(facing)");
        return 0;
    }

    if (CGUIMacros::IsActionAllowed(5)) {
        float facing = static_cast<float>(lua_tonumber(L, 1));
        float angle = static_cast<float>(lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 0.0);
        CGGameUI::TargetDirection(facing, angle, 3);
        CGUIMacros::m_state &= ~0x20u;
    }

    return 0;
}

// VERIFY: 32ad6284 manual
// OFFSET: 0x00525D70
static int32_t Script_TargetLastTarget(lua_State* L) {
    if (CGGameUI::m_lastTarget) {
        TargetIfValid(CGGameUI::m_lastTarget);
        return 0;
    }

    WOWGUID none = WOWGUID();
    CGGameUI::Target(none);
    return 0;
}

// VERIFY: 59fcfe50 manual
// OFFSET: 0x00525DF0
static int32_t Script_TargetLastEnemy(lua_State* L) {
    TargetIfValid(CGGameUI::m_lastEnemyTarget);
    return 0;
}

// VERIFY: a06467c2 manual
// OFFSET: 0x00525E50
static int32_t Script_TargetLastFriend(lua_State* L) {
    TargetIfValid(CGGameUI::m_lastFriendTarget);
    return 0;
}

// VERIFY: 1eba8f45 manual
// OFFSET: 0x00525EB0
static int32_t Script_AssistUnit(lua_State* L) {
    const char* name = lua_tolstring(L, 1, nullptr);
    int32_t exact = StringToBOOL(L, 2, 0);
    WOWGUID guid = CGGameUI::ClosestObjectMatch(name, TYPE_UNIT, 0, exact, FLT_MAX, 0);
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        WOWGUID target = unit->m_unit->m_unitFieldTarget;

        if (target) {
            TargetIfValid(target);

            if (g_assistAttackCVar->m_intValue) {
                CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

                if (player) {
                    player->CombatModeEnter(target, 0, 0);
                }
            }
        }

        return 0;
    }

    if (name && *name && SStrCmpI(name, "target", STORM_MAX_STR)) {
        CGGameUI::DisplayError(ERR_UNIT_NOT_FOUND);
        return 0;
    }

    CGGameUI::DisplayError(ERR_GENERIC_NO_TARGET);
    return 0;
}

// VERIFY: 818e9713 manual
// OFFSET: 0x00526050
static int32_t Script_SetDungeonDifficulty(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetDungeonDifficulty(difficulty)");
        return 0;
    }

    uint32_t difficulty = static_cast<uint32_t>(lua_tonumber(L, 1) - 1.0);

    if (difficulty > 1) {
        return 0;
    }

    if ((CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) && CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_NOT_LEADER);
        return 0;
    }

    uint32_t current = CGGameUI::GetCurrentDungeonDifficulty();
    CGGameUI::m_dungeonDifficulty = difficulty;
    CGPartyInfo::m_dungeonDifficulty = difficulty;

    if (current != difficulty) {
        CGGameUI::ShowDungeonDifficultyChanged();
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_SET_DUNGEON_DIFFICULTY));
    msg.Put(difficulty);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: c4c81cc7 manual
// OFFSET: 0x005261A0
static int32_t Script_SetRaidDifficulty(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetRaidDifficulty(difficulty)");
        return 0;
    }

    uint32_t difficulty = static_cast<uint32_t>(lua_tonumber(L, 1) - 1.0);

    if (difficulty > 3) {
        return 0;
    }

    if ((CGPartyInfo::m_members[0] || CGRaidInfo::m_numMembers) && CGPartyInfo::m_leader != ClntObjMgrGetActivePlayer()) {
        CGGameUI::DisplayError(ERR_NOT_LEADER);
        return 0;
    }

    CGGameUI::SetRaidDifficulty(difficulty, 1, 1);

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(MSG_SET_RAID_DIFFICULTY));
    msg.Put(difficulty);
    msg.Finalize();
    ClientServices::Send2(&msg);
    return 0;
}

// VERIFY: acea3d3f auto
// OFFSET: 0x005262D0
static int32_t Script_AcceptAreaSpiritHeal(lua_State* L) {
    CGGameUI::AcceptAreaSpiritHeal();
    return 0;
}

// VERIFY: 1959aa5f auto
// OFFSET: 0x00527F00
static int32_t Script_InteractUnit(lua_State* L) {
    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: InteractUnit(unitToken[, exactMatch])");
        return 0;
    }

    const char* token = lua_tolstring(L, 1, nullptr);
    WOWGUID guid;

    if (SStrCmpI(token, "mouseover", STORM_MAX_STR)) {
        guid = CGGameUI::ClosestObjectMatch(token, TYPE_OBJECT, 0, StringToBOOL(L, 2, 0), FLT_MAX, 0);
    } else {
        guid = CGGameUI::m_currentObjectTrack;
    }

    if (lua_tainted) {
        CGGameUI::ShowBlockedActionFeedback(nullptr, 0);
        return 0;
    }

    if (CGGameUI::OnSpriteRightClick(guid)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 18a1aa02 auto
// OFFSET: 0x008E5250
static int32_t Script_GetDebugStats(lua_State* L) {
    return 0;
}

// VERIFY: 2d906ed2 auto
// OFFSET: 0x008E5250
static int32_t Script_GetExtendedItemInfo(lua_State* L) {
    return 0;
}

// VERIFY: a2a81f16 auto
// OFFSET: 0x008E5250
static int32_t Script_ResetPerformanceValues(lua_State* L) {
    return 0;
}

// VERIFY: fd2073f6 auto
// OFFSET: 0x008E5250
static int32_t Script_ToggleCollision(lua_State* L) {
    return 0;
}

// VERIFY: 6257dd9e auto
// OFFSET: 0x008E5250
static int32_t Script_ToggleCollisionDisplay(lua_State* L) {
    return 0;
}

// VERIFY: b21b6e35 auto
// OFFSET: 0x008E5250
static int32_t Script_TogglePerformanceDisplay(lua_State* L) {
    return 0;
}

// VERIFY: 8b4eb514 auto
// OFFSET: 0x008E5250
static int32_t Script_TogglePerformancePause(lua_State* L) {
    return 0;
}

// VERIFY: cfd0fad1 auto
// OFFSET: 0x008E5250
static int32_t Script_TogglePerformanceValues(lua_State* L) {
    return 0;
}

// VERIFY: f89e457c auto
// OFFSET: 0x008E5250
static int32_t Script_TogglePlayerBounds(lua_State* L) {
    return 0;
}

// VERIFY: e0ce0afb auto
// OFFSET: 0x008E5250
static int32_t Script_TogglePortals(lua_State* L) {
    return 0;
}

// VERIFY: b6b7441a auto
// OFFSET: 0x008E5250
static int32_t Script_ToggleTris(lua_State* L) {
    return 0;
}

// STATIC: 0x00AC80B0
static FrameScript_Method s_ScriptFunctions[] = {
    { "FrameXML_Debug", &Script_FrameXML_Debug },
    { "GetBuildInfo", &Script_GetBuildInfo },
    { "ReloadUI", &Script_ReloadUI },
    { "RegisterForSave", &Script_RegisterForSave },
    { "RegisterForSavePerCharacter", &Script_RegisterForSavePerCharacter },
    { "SetLayoutMode", &Script_SetLayoutMode },
    { "IsModifierKeyDown", &Script_IsModifierKeyDown },
    { "IsLeftShiftKeyDown", &Script_IsLeftShiftKeyDown },
    { "IsRightShiftKeyDown", &Script_IsRightShiftKeyDown },
    { "IsShiftKeyDown", &Script_IsShiftKeyDown },
    { "IsLeftControlKeyDown", &Script_IsLeftControlKeyDown },
    { "IsRightControlKeyDown", &Script_IsRightControlKeyDown },
    { "IsControlKeyDown", &Script_IsControlKeyDown },
    { "IsLeftAltKeyDown", &Script_IsLeftAltKeyDown },
    { "IsRightAltKeyDown", &Script_IsRightAltKeyDown },
    { "IsAltKeyDown", &Script_IsAltKeyDown },
    { "IsMouseButtonDown", &Script_IsMouseButtonDown },
    { "GetMouseButtonName", &Script_GetMouseButtonName },
    { "GetMouseButtonClicked", &Script_GetMouseButtonClicked },
    { "SetConsoleKey", &Script_SetConsoleKey },
    { "Screenshot", &Script_Screenshot },
    { "GetFramerate", &Script_GetFramerate },
    { "TogglePerformanceDisplay", &Script_TogglePerformanceDisplay },
    { "TogglePerformancePause", &Script_TogglePerformancePause },
    { "TogglePerformanceValues", &Script_TogglePerformanceValues },
    { "ResetPerformanceValues", &Script_ResetPerformanceValues },
    { "GetDebugStats", &Script_GetDebugStats },
    { "IsDebugBuild", &Script_IsDebugBuild },
    { "RegisterCVar", &Script_RegisterCVar },
    { "GetCVarInfo", &Script_GetCVarInfo },
    { "SetCVar", &Script_SetCVar },
    { "GetCVar", &Script_GetCVar },
    { "GetCVarBool", &Script_GetCVarBool },
    { "GetCVarDefault", &Script_GetCVarDefault },
    { "GetCVarMin", &Script_GetCVarMin },
    { "GetCVarMax", &Script_GetCVarMax },
    { "GetCVarAbsoluteMin", &Script_GetCVarAbsoluteMin },
    { "GetCVarAbsoluteMax", &Script_GetCVarAbsoluteMax },
    { "GetWaterDetail", &Script_GetWaterDetail },
    { "SetWaterDetail", &Script_SetWaterDetail },
    { "GetFarclip", &Script_GetFarclip },
    { "SetFarclip", &Script_SetFarclip },
    { "GetTexLodBias", &Script_GetTexLodBias },
    { "SetTexLodBias", &Script_SetTexLodBias },
    { "SetBaseMip", &Script_SetBaseMip },
    { "GetBaseMip", &Script_GetBaseMip },
    { "ToggleTris", &Script_ToggleTris },
    { "TogglePortals", &Script_TogglePortals },
    { "ToggleCollision", &Script_ToggleCollision },
    { "ToggleCollisionDisplay", &Script_ToggleCollisionDisplay },
    { "TogglePlayerBounds", &Script_TogglePlayerBounds },
    { "Stuck", &Script_Stuck },
    { "Logout", &Script_Logout },
    { "Quit", &Script_Quit },
    { "SetCursor", &Script_SetCursor },
    { "ResetCursor", &Script_ResetCursor },
    { "ClearCursor", &Script_ClearCursor },
    { "CursorHasItem", &Script_CursorHasItem },
    { "CursorHasSpell", &Script_CursorHasSpell },
    { "CursorHasMacro", &Script_CursorHasMacro },
    { "CursorHasMoney", &Script_CursorHasMoney },
    { "GetCursorInfo", &Script_GetCursorInfo },
    { "EquipCursorItem", &Script_EquipCursorItem },
    { "DeleteCursorItem", &Script_DeleteCursorItem },
    { "EquipPendingItem", &Script_EquipPendingItem },
    { "CancelPendingEquip", &Script_CancelPendingEquip },
    { "TargetUnit", &Script_TargetUnit },
    { "TargetNearest", &Script_TargetNearest },
    { "TargetNearestEnemy", &Script_TargetNearestEnemy },
    { "TargetNearestEnemyPlayer", &Script_TargetNearestEnemyPlayer },
    { "TargetNearestFriend", &Script_TargetNearestFriend },
    { "TargetNearestFriendPlayer", &Script_TargetNearestFriendPlayer },
    { "TargetNearestPartyMember", &Script_TargetNearestPartyMember },
    { "TargetNearestRaidMember", &Script_TargetNearestRaidMember },
    { "TargetDirectionEnemy", &Script_TargetDirectionEnemy },
    { "TargetDirectionFriend", &Script_TargetDirectionFriend },
    { "TargetDirectionFinished", &Script_TargetDirectionFinished },
    { "TargetLastTarget", &Script_TargetLastTarget },
    { "TargetLastEnemy", &Script_TargetLastEnemy },
    { "TargetLastFriend", &Script_TargetLastFriend },
    { "AttackTarget", &Script_AttackTarget },
    { "AssistUnit", &Script_AssistUnit },
    { "FocusUnit", &Script_FocusUnit },
    { "FollowUnit", &Script_FollowUnit },
    { "InteractUnit", &Script_InteractUnit },
    { "ClearTarget", &Script_ClearTarget },
    { "ClearFocus", &Script_ClearFocus },
    { "AutoEquipCursorItem", &Script_AutoEquipCursorItem },
    { "ToggleSheath", &Script_ToggleSheath },
    { "GetZoneText", &Script_GetZoneText },
    { "GetRealZoneText", &Script_GetRealZoneText },
    { "GetSubZoneText", &Script_GetSubZoneText },
    { "GetMinimapZoneText", &Script_GetMinimapZoneText },
    { "InitiateTrade", &Script_InitiateTrade },
    { "CanInspect", &Script_CanInspect },
    { "NotifyInspect", &Script_NotifyInspect },
    { "InviteUnit", &Script_InviteUnit },
    { "UninviteUnit", &Script_UninviteUnit },
    { "RequestTimePlayed", &Script_RequestTimePlayed },
    { "RepopMe", &Script_RepopMe },
    { "AcceptResurrect", &Script_AcceptResurrect },
    { "DeclineResurrect", &Script_DeclineResurrect },
    { "ResurrectGetOfferer", &Script_ResurrectGetOfferer },
    { "ResurrectHasSickness", &Script_ResurrectHasSickness },
    { "ResurrectHasTimer", &Script_ResurrectHasTimer },
    { "BeginTrade", &Script_BeginTrade },
    { "CancelTrade", &Script_CancelTrade },
    { "AcceptGroup", &Script_AcceptGroup },
    { "DeclineGroup", &Script_DeclineGroup },
    { "AcceptGuild", &Script_AcceptGuild },
    { "DeclineGuild", &Script_DeclineGuild },
    { "AcceptArenaTeam", &Script_AcceptArenaTeam },
    { "DeclineArenaTeam", &Script_DeclineArenaTeam },
    { "CancelLogout", &Script_CancelLogout },
    { "ForceLogout", &Script_ForceLogout },
    { "ForceQuit", &Script_ForceQuit },
    { "GetCursorMoney", &Script_GetCursorMoney },
    { "DropCursorMoney", &Script_DropCursorMoney },
    { "PickupPlayerMoney", &Script_PickupPlayerMoney },
    { "HasSoulstone", &Script_HasSoulstone },
    { "UseSoulstone", &Script_UseSoulstone },
    { "HasKey", &Script_HasKey },
    { "GuildInvite", &Script_GuildInvite },
    { "GuildUninvite", &Script_GuildUninvite },
    { "GuildPromote", &Script_GuildPromote },
    { "GuildDemote", &Script_GuildDemote },
    { "GuildSetLeader", &Script_GuildSetLeader },
    { "GuildSetMOTD", &Script_GuildSetMOTD },
    { "GuildLeave", &Script_GuildLeave },
    { "GuildDisband", &Script_GuildDisband },
    { "GuildInfo", &Script_GuildInfo },
    { "ArenaTeamInviteByName", &Script_ArenaTeamInviteByName },
    { "ArenaTeamLeave", &Script_ArenaTeamLeave },
    { "ArenaTeamUninviteByName", &Script_ArenaTeamUninviteByName },
    { "ArenaTeamSetLeaderByName", &Script_ArenaTeamSetLeaderByName },
    { "ArenaTeamDisband", &Script_ArenaTeamDisband },
    { "GetScreenWidth", &Script_GetScreenWidth },
    { "GetScreenHeight", &Script_GetScreenHeight },
    { "GetDamageBonusStat", &Script_GetDamageBonusStat },
    { "GetReleaseTimeRemaining", &Script_GetReleaseTimeRemaining },
    { "GetCorpseRecoveryDelay", &Script_GetCorpseRecoveryDelay },
    { "GetInstanceBootTimeRemaining", &Script_GetInstanceBootTimeRemaining },
    { "GetInstanceLockTimeRemaining", &Script_GetInstanceLockTimeRemaining },
    { "GetInstanceLockTimeRemainingEncounter", &Script_GetInstanceLockTimeRemainingEncounter },
    { "GetSummonConfirmTimeLeft", &Script_GetSummonConfirmTimeLeft },
    { "GetSummonConfirmSummoner", &Script_GetSummonConfirmSummoner },
    { "GetSummonConfirmAreaName", &Script_GetSummonConfirmAreaName },
    { "ConfirmSummon", &Script_ConfirmSummon },
    { "CancelSummon", &Script_CancelSummon },
    { "GetCursorPosition", &Script_GetCursorPosition },
    { "GetNetStats", &Script_GetNetStats },
    { "SitStandOrDescendStart", &Script_SitStandOrDescendStart },
    { "StopCinematic", &Script_StopCinematic },
    { "RunScript", &Script_RunScript },
    { "CheckInteractDistance", &Script_CheckInteractDistance },
    { "RandomRoll", &Script_RandomRoll },
    { "OpeningCinematic", &Script_OpeningCinematic },
    { "InCinematic", &Script_InCinematic },
    { "IsWindowsClient", &Script_IsWindowsClient },
    { "IsMacClient", &Script_IsMacClient },
    { "IsLinuxClient", &Script_IsLinuxClient },
    { "AcceptXPLoss", &Script_AcceptXPLoss },
    { "CheckSpiritHealerDist", &Script_CheckSpiritHealerDist },
    { "CheckTalentMasterDist", &Script_CheckTalentMasterDist },
    { "CheckBinderDist", &Script_CheckBinderDist },
    { "RetrieveCorpse", &Script_RetrieveCorpse },
    { "BindEnchant", &Script_BindEnchant },
    { "ReplaceEnchant", &Script_ReplaceEnchant },
    { "ReplaceTradeEnchant", &Script_ReplaceTradeEnchant },
    { "NotWhileDeadError", &Script_NotWhileDeadError },
    { "GetRestState", &Script_GetRestState },
    { "GetXPExhaustion", &Script_GetXPExhaustion },
    { "GetTimeToWellRested", &Script_GetTimeToWellRested },
    { "GMRequestPlayerInfo", &Script_GMRequestPlayerInfo },
    { "GetCoinIcon", &Script_GetCoinIcon },
    { "GetCoinText", &Script_GetCoinText },
    { "GetCoinTextureString", &Script_GetCoinTextureString },
    { "IsSubZonePVPPOI", &Script_IsSubZonePVPPOI },
    { "GetZonePVPInfo", &Script_GetZonePVPInfo },
    { "TogglePVP", &Script_TogglePVP },
    { "SetPVP", &Script_SetPVP },
    { "GetPVPDesired", &Script_GetPVPDesired },
    { "GetPVPTimer", &Script_GetPVPTimer },
    { "IsPVPTimerRunning", &Script_IsPVPTimerRunning },
    { "ConfirmBindOnUse", &Script_ConfirmBindOnUse },
    { "SetPortraitToTexture", &Script_SetPortraitToTexture },
    { "GetLocale", &Script_GetLocale },
    { "GetGMTicketCategories", &Script_GetGMTicketCategories },
    { "DropItemOnUnit", &Script_DropItemOnUnit },
    { "RestartGx", &Script_RestartGx },
    { "RestoreVideoResolutionDefaults", &Script_RestoreVideoResolutionDefaults },
    { "RestoreVideoEffectsDefaults", &Script_RestoreVideoEffectsDefaults },
    { "RestoreVideoStereoDefaults", &Script_RestoreVideoStereoDefaults },
    { "GetBindLocation", &Script_GetBindLocation },
    { "ConfirmTalentWipe", &Script_ConfirmTalentWipe },
    { "ConfirmBinder", &Script_ConfirmBinder },
    { "ShowingHelm", &Script_ShowingHelm },
    { "ShowingCloak", &Script_ShowingCloak },
    { "ShowHelm", &Script_ShowHelm },
    { "ShowCloak", &Script_ShowCloak },
    { "SetEuropeanNumbers", &Script_SetEuropeanNumbers },
    { "GetAreaSpiritHealerTime", &Script_GetAreaSpiritHealerTime },
    { "AcceptAreaSpiritHeal", &Script_AcceptAreaSpiritHeal },
    { "CancelAreaSpiritHeal", &Script_CancelAreaSpiritHeal },
    { "GetMouseFocus", &Script_GetMouseFocus },
    { "GetRealmName", &Script_GetRealmName },
    { "GetItemQualityColor", &Script_GetItemQualityColor },
    { "GetItemInfo", &Script_GetItemInfo },
    { "GetItemGem", &Script_GetItemGem },
    { "GetExtendedItemInfo", &Script_GetExtendedItemInfo },
    { "GetItemIcon", &Script_GetItemIcon },
    { "GetItemFamily", &Script_GetItemFamily },
    { "GetItemCount", &Script_GetItemCount },
    { "GetItemSpell", &Script_GetItemSpell },
    { "GetItemCooldown", &Script_GetItemCooldown },
    { "PickupItem", &Script_PickupItem },
    { "IsCurrentItem", &Script_IsCurrentItem },
    { "IsUsableItem", &Script_IsUsableItem },
    { "IsHelpfulItem", &Script_IsHelpfulItem },
    { "IsHarmfulItem", &Script_IsHarmfulItem },
    { "IsConsumableItem", &Script_IsConsumableItem },
    { "IsEquippableItem", &Script_IsEquippableItem },
    { "IsEquippedItem", &Script_IsEquippedItem },
    { "IsEquippedItemType", &Script_IsEquippedItemType },
    { "IsDressableItem", &Script_IsDressableItem },
    { "ItemHasRange", &Script_ItemHasRange },
    { "IsItemInRange", &Script_IsItemInRange },
    { "GetNumAddOns", &Script_GetNumAddOns },
    { "GetAddOnInfo", &Script_GetAddOnInfo },
    { "GetAddOnMetadata", &Script_GetAddOnMetadata },
    { "UpdateAddOnMemoryUsage", &Script_UpdateAddOnMemoryUsage },
    { "GetAddOnMemoryUsage", &Script_GetAddOnMemoryUsage },
    { "GetScriptCPUUsage", &Script_GetScriptCPUUsage },
    { "UpdateAddOnCPUUsage", &Script_UpdateAddOnCPUUsage },
    { "GetAddOnCPUUsage", &Script_GetAddOnCPUUsage },
    { "GetFunctionCPUUsage", &Script_GetFunctionCPUUsage },
    { "GetFrameCPUUsage", &Script_GetFrameCPUUsage },
    { "GetEventCPUUsage", &Script_GetEventCPUUsage },
    { "ResetCPUUsage", &Script_ResetCPUUsage },
    { "GetAddOnDependencies", &Script_GetAddOnDependencies },
    { "EnableAddOn", &Script_EnableAddOn },
    { "EnableAllAddOns", &Script_EnableAllAddOns },
    { "DisableAddOn", &Script_DisableAddOn },
    { "DisableAllAddOns", &Script_DisableAllAddOns },
    { "ResetDisabledAddOns", &Script_ResetDisabledAddOns },
    { "IsAddOnLoadOnDemand", &Script_IsAddOnLoadOnDemand },
    { "IsAddOnLoaded", &Script_IsAddOnLoaded },
    { "LoadAddOn", &Script_LoadAddOn },
    { "PartialPlayTime", &Script_PartialPlayTime },
    { "NoPlayTime", &Script_NoPlayTime },
    { "GetBillingTimeRested", &Script_GetBillingTimeRested },
    { "CanShowResetInstances", &Script_CanShowResetInstances },
    { "ResetInstances", &Script_ResetInstances },
    { "IsInInstance", &Script_IsInInstance },
    { "GetInstanceDifficulty", &Script_GetInstanceDifficulty },
    { "GetInstanceInfo", &Script_GetInstanceInfo },
    { "GetDungeonDifficulty", &Script_GetDungeonDifficulty },
    { "SetDungeonDifficulty", &Script_SetDungeonDifficulty },
    { "GetRaidDifficulty", &Script_GetRaidDifficulty },
    { "SetRaidDifficulty", &Script_SetRaidDifficulty },
    { "ReportBug", &Script_ReportBug },
    { "ReportSuggestion", &Script_ReportSuggestion },
    { "GetMirrorTimerInfo", &Script_GetMirrorTimerInfo },
    { "GetMirrorTimerProgress", &Script_GetMirrorTimerProgress },
    { "GetNumTitles", &Script_GetNumTitles },
    { "GetCurrentTitle", &Script_GetCurrentTitle },
    { "SetCurrentTitle", &Script_SetCurrentTitle },
    { "IsTitleKnown", &Script_IsTitleKnown },
    { "GetTitleName", &Script_GetTitleName },
    { "UseItemByName", &Script_UseItemByName },
    { "EquipItemByName", &Script_EquipItemByName },
    { "GetExistingLocales", &Script_GetExistingLocales },
    { "InCombatLockdown", &Script_InCombatLockdown },
    { "StartAttack", &Script_StartAttack },
    { "StopAttack", &Script_StopAttack },
    { "SetTaxiBenchmarkMode", &Script_SetTaxiBenchmarkMode },
    { "GetTaxiBenchmarkMode", &Script_GetTaxiBenchmarkMode },
    { "Dismount", &Script_Dismount },
    { "VoicePushToTalkStart", &Script_VoicePushToTalkStart },
    { "VoicePushToTalkStop", &Script_VoicePushToTalkStop },
    { "SetUIVisibility", &Script_SetUIVisibility },
    { "IsReferAFriendLinked", &Script_IsReferAFriendLinked },
    { "CanGrantLevel", &Script_CanGrantLevel },
    { "GrantLevel", &Script_GrantLevel },
    { "CanSummonFriend", &Script_CanSummonFriend },
    { "SummonFriend", &Script_SummonFriend },
    { "GetSummonFriendCooldown", &Script_GetSummonFriendCooldown },
    { "GetTotemInfo", &Script_GetTotemInfo },
    { "GetTotemTimeLeft", &Script_GetTotemTimeLeft },
    { "TargetTotem", &Script_TargetTotem },
    { "DestroyTotem", &Script_DestroyTotem },
    { "GetNumDeclensionSets", &Script_GetNumDeclensionSets },
    { "DeclineName", &Script_DeclineName },
    { "AcceptLevelGrant", &Script_AcceptLevelGrant },
    { "DeclineLevelGrant", &Script_DeclineLevelGrant },
    { "UploadSettings", &Script_UploadSettings },
    { "DownloadSettings", &Script_DownloadSettings },
    { "GetMovieResolution", &Script_GetMovieResolution },
    { "GameMovieFinished", &Script_GameMovieFinished },
    { "IsDesaturateSupported", &Script_IsDesaturateSupported },
    { "GetThreatStatusColor", &Script_GetThreatStatusColor },
    { "IsThreatWarningEnabled", &Script_IsThreatWarningEnabled },
    { "ConsoleAddMessage", &Script_ConsoleAddMessage },
    { "GetItemUniqueness", &Script_GetItemUniqueness },
    { "EndRefund", &Script_EndRefund },
    { "EndBoundTradeable", &Script_EndBoundTradeable },
    { "CanMapChangeDifficulty", &Script_CanMapChangeDifficulty },
    { "GetExpansionLevel", &Script_GetAccountExpansionLevel },
    { "GetAllowLowLevelRaid", &Script_GetAllowLowLevelRaid },
    { "SetAllowLowLevelRaid", &Script_SetAllowLowLevelRaid },
};

// VERIFY: 2d4b8add manual
// OFFSET: 0x005120E0
void LoadScriptFunctions() {
    RegisterSimpleFrameScriptMethods();
    CGTooltip::CreateScriptMetaTable();
    CGCooldown::CreateScriptMetaTable();
    CGMinimapFrame::CreateScriptMetaTable();
    CGCharacterModelBase::CreateScriptMetaTable();
    CGDressUpModelFrame::CreateScriptMetaTable();
    CGTabardModelFrame::CreateScriptMetaTable();
    CGQuestPOIFrame::CreateScriptMetaTable();

    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }

    UIBindingsRegisterScriptFunctions();
    UIMacrosRegisterScriptFunctions();
    InputControlRegisterScriptFunctions();
    CameraRegisterScriptFunctions();
    SI2::RegisterScriptFunctions();
    SpellRegisterScriptFunctions();
    ScriptEventsRegisterFunctions();
    MinimapRegisterScriptFunctions();
    UnitCombatLogRegisterScriptFunctions();
    ActionBarRegisterScriptFunctions();
    CGPartyInfo::RegisterScriptFunctions();
    ChatRegisterScriptFunctions();
    AutoCompleteRegisterScriptFunctions();
    SpellBookRegisterScriptFunctions();
    CharacterInfoRegisterScriptFunctions();
    LootInfoRegisterScriptFunctions();
    ItemTextRegisterScriptFunctions();
    GossipInfoRegisterScriptFunctions();
    QuestInfoRegisterScriptFunctions();
    QuestLogRegisterScriptFunctions();
    CGTaxiMap::RegisterScriptFunctions();
    ClassTrainerRegisterScriptFunctions();
    TradeSkillRegisterScriptFunctions();
    MerchantRegisterScriptFunctions();
    TradeInfoRegisterScriptFunctions();
    ContainerRegisterScriptFunctions();
    BankRegisterScriptFunctions();
    FriendList::RegisterScriptFunctions();
    PetInfoRegisterScriptFunctions();
    WorldMapRegisterScriptFunctions();
    ReputationInfoRegisterScriptFunctions();
    TabardCreationRegisterScriptFunctions();
    GuildRegistrarRegisterScriptFunctions();
    DuelInfoRegisterScriptFunctions();
    TutorialRegisterScriptFunctions();
    PetitionInfoRegisterScriptFunctions();
    SkillInfoRegisterScriptFunctions();
    GuildInfoRegisterScriptFunctions();
    MailInfoRegisterScriptFunctions();
    BattlefieldInfoRegisterScriptFunctions();
    TalentInfoRegisterScriptFunctions();
    AuctionHouseRegisterScriptFunctions();
    StableInfoRegisterScriptFunctions();
    RaidInfoRegisterScriptFunctions();
    WorldStateInfoRegisterScriptFunctions();
    MinigameInfoRegisterScriptFunctions();
    KnowledgeBase_RegisterScriptFunctions();
    ItemSocketInfoRegisterScriptFunctions();
    PetitionVendorRegisterScriptFunctions();
    ArenaTeamInfoRegisterScriptFunctions();
    LookingForGroupRegisterScriptFunctions();
    VoiceRegisterScriptFunctions();
    CalendarRegisterScriptFunctions();
    CGCommentator::RegisterScriptFunctions();
    CGBarberShop::RegisterScriptFunctions();
    CGDanceStudio::RegisterScriptFunctions();
    GuildBankRegisterScriptFunctions();
    GlyphInfoRegisterScriptFunctions();
    AchievementInfoRegisterScriptFunctions();
    CurrencyTypesRegisterScriptFunctions();
    CGVideoOptions::RegisterScriptFunctions();
    EquipmentManagerRegisterScriptFunctions();
    GMTicketInfoRegisterScriptFunctions();
    BattlenetUI_RegisterScriptFunctions();
}

// VERIFY: ce499890 manual
// OFFSET: 0x00512280
void UnloadScriptFunctions() {
    UnregisterSimpleFrameScriptMethods();
    CGTooltip::DestroyScriptMetaTable();
    CGCooldown::DestroyScriptMetaTable();
    CGMinimapFrame::DestroyScriptMetaTable();
    CGCharacterModelBase::DestroyScriptMetaTable();
    CGDressUpModelFrame::DestroyScriptMetaTable();
    CGTabardModelFrame::DestroyScriptMetaTable();
    CGQuestPOIFrame::DestroyScriptMetaTable();

    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }

    UIBindingsUnegisterScriptFunctions();
    UIMacrosUnegisterScriptFunctions();
    InputControlUnregisterScriptFunctions();
    CameraUnregisterScriptFunctions();
    SI2::UnregisterScriptFunctions();
    SpellUnregisterScriptFunctions();
    MinimapUnregisterScriptFunctions();
    ScriptEventsUnregisterFunctions();
    UnitCombatLogUnregisterScriptFunctions();
    ActionBarUnregisterScriptFunctions();
    CGPartyInfo::UnregisterScriptFunctions();
    ChatUnregisterScriptFunctions();
    AutoCompleteUnregisterScriptFunctions();
    SpellBookUnregisterScriptFunctions();
    CharacterInfoUnregisterScriptFunctions();
    LootInfoUnregisterScriptFunctions();
    ItemTextUnregisterScriptFunctions();
    GossipInfoUnregisterScriptFunctions();
    QuestInfoUnregisterScriptFunctions();
    QuestLogUnregisterScriptFunctions();
    CGTaxiMap::UnregisterScriptFunctions();
    ClassTrainerUnregisterScriptFunctions();
    TradeSkillUnregisterScriptFunctions();
    MerchantUnregisterScriptFunctions();
    TradeInfoUnregisterScriptFunctions();
    ContainerUnregisterScriptFunctions();
    BankUnregisterScriptFunctions();
    FriendList::UnregisterScriptFunctions();
    PetInfoUnregisterScriptFunctions();
    WorldMapUnregisterScriptFunctions();
    ReputationInfoUnregisterScriptFunctions();
    TabardCreationUnregisterScriptFunctions();
    GuildRegistrarUnregisterScriptFunctions();
    DuelInfoUnregisterScriptFunctions();
    TutorialUnregisterScriptFunctions();
    PetitionInfoUnregisterScriptFunctions();
    SkillInfoUnregisterScriptFunctions();
    GuildInfoUnregisterScriptFunctions();
    MailInfoUnregisterScriptFunctions();
    BattlefieldInfoUnregisterScriptFunctions();
    TalentInfoUnregisterScriptFunctions();
    AuctionHouseUnregisterScriptFunctions();
    StableInfoUnregisterScriptFunctions();
    RaidInfoUnregisterScriptFunctions();
    WorldStateInfoUnregisterScriptFunctions();
    MinigameInfoUnregisterScriptFunctions();
    KnowledgeBase_UnregisterScriptFunctions();
    ItemSocketInfoUnregisterScriptFunctions();
    PetitionVendorUnregisterScriptFunctions();
    ArenaTeamInfoUnregisterScriptFunctions();
    LookingForGroupUnregisterScriptFunctions();
    VoiceUnregisterScriptFunctions();
    CalendarUnregisterScriptFunctions();
    CGCommentator::UnregisterScriptFunctions();
    CGBarberShop::UnregisterScriptFunctions();
    CGDanceStudio::UnregisterScriptFunctions();
    GuildBankUnregisterScriptFunctions();
    GlyphInfoUnregisterScriptFunctions();
    AchievementInfoUnregisterScriptFunctions();
    CurrencyTypesUnregisterScriptFunctions();
    CGVideoOptions::UnregisterScriptFunctions();
    EquipmentManagerUnregisterScriptFunctions();
    GMTicketInfoUnregisterScriptFunctions();
    BattlenetUI_UnregisterScriptFunctions();
}
