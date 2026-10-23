#include "GameUI.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Game/ViolenceLevels.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Ui/CGAutoCompleteName.h"
#include "WoW/Source/Ui/Calendar.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GuildInfo.h"
#include "WoW/Source/Ui/TalentInfo.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/WorldFrame.h"

// STATIC: 0x00BD0794
HSLOG s_taintLog;

// STATIC: 0x00BD08E4
CVar* g_taintLogCVar;

// STATIC: 0x00BD08E8
CVar* g_scriptErrorsCVar;

// STATIC: 0x00BD08EC
CVar* g_scriptProfileCVar;

// STATIC: 0x00BD08F0
CVar* g_deselectOnClickCVar;

// STATIC: 0x00BD08F4
CVar* g_cvAutoInteract;

// STATIC: 0x00BD08F8
CVar* g_autoStandCVar;

// STATIC: 0x00BD08FC
CVar* g_autoDismountCVar;

// STATIC: 0x00BD0900
CVar* g_autoDismountFlyingCVar;

// STATIC: 0x00BD0904
CVar* g_autoUnshiftCVar;

// STATIC: 0x00BD0908
CVar* g_autoClearAFKCVar;

// STATIC: 0x00BD090C
CVar* g_blockTradesCVar;

// STATIC: 0x00BD0910
CVar* g_lootUnderMouseCVar;

// STATIC: 0x00BD0914
CVar* g_autoLootDefaultCVar;

// STATIC: 0x00BD0918
CVar* g_assistAttackCVar;

// STATIC: 0x00BD091C
CVar* s_cvAutoRangedCombat;

// STATIC: 0x00BD0920
CVar* g_autoSelfCastCVar;

// STATIC: 0x00BD0924
CVar* g_stopAutoAttackOnTargetChangeCVar;

// STATIC: 0x00BD0928
CVar* g_showTargetOfTargetCVar;

// STATIC: 0x00BD092C
CVar* g_targetOfTargetModeCVar;

// STATIC: 0x00BD0930
CVar* g_showTargetCastbarCVar;

// STATIC: 0x00BD0934
CVar* g_showVKeyCastbarCVar;

// STATIC: 0x00BD0938
CVar* g_buffDurationsCVar;

// STATIC: 0x00BD093C
CVar* g_rotateMinimapCVar;

// STATIC: 0x00BD0940
CVar* g_minimapZoomCVar;

// STATIC: 0x00BD0944
CVar* g_minimapInsideZoomCVar;

// STATIC: 0x00BD0948
CVar* g_minimapPortalMaxCVar;

// STATIC: 0x00BD094C
CVar* g_screenEdgeFlashCVar;

// STATIC: 0x00BD0950
CVar* g_showLootSpamCVar;

// STATIC: 0x00BD0954
CVar* g_displayFreeBagSlotsCVar;

// STATIC: 0x00BD0958
CVar* g_displayWorldPVPObjectivesCVar;

// STATIC: 0x00BD095C
CVar* g_profanityFilterCVar;

// STATIC: 0x00BD0960
CVar* g_spamFilterCVar;

// STATIC: 0x00BD0964
CVar* g_chatBubblesCVar;

// STATIC: 0x00BD0968
CVar* g_chatBubblesPartyCVar;

// STATIC: 0x00BD096C
CVar* g_removeChatDelayCVar;

// STATIC: 0x00BD0970
CVar* g_guildShowOfflineCVar;

// STATIC: 0x00BD0974
CVar* g_guildMemberNotifyCVar;

// STATIC: 0x00BD0978
CVar* g_guildRecruitmentChannelCVar;

// STATIC: 0x00BD097C
CVar* g_secureAbilityToggleCVar;

// STATIC: 0x00BD0980
CVar* g_combatDamageCVar;

// STATIC: 0x00BD0984
CVar* s_cvCombatLogPeriodicSpells;

// STATIC: 0x00BD0988
CVar* g_petMeleeDamageCVar;

// STATIC: 0x00BD098C
CVar* g_petSpellDamageCVar;

// STATIC: 0x00BD0990
CVar* g_combatHealingCVar;

// STATIC: 0x00BD0998
CVar* s_cvCalendarShowWeeklyHolidays;

// STATIC: 0x00BD099C
CVar* s_cvCalendarShowDarkmoon;

// STATIC: 0x00BD09A0
CVar* s_cvCalendarShowBattlegrounds;

// STATIC: 0x00BD09A4
CVar* s_cvCalendarShowLockouts;

// STATIC: 0x00BD09A8
CVar* s_cvCalendarShowResets;

// STATIC: 0x00BD09AC
CVar* unk_BD09AC;

// STATIC: 0x00BD09B0
CVar* g_useUiScaleCVar;

// STATIC: 0x00BD09B4
CVar* g_uiScaleCVar;

// STATIC: 0x00BD09B8
CVar* g_nameplateShowEnemiesCVar;

// STATIC: 0x00BD09BC
CVar* g_nameplateShowEnemyPetsCVar;

// STATIC: 0x00BD09C0
CVar* g_nameplateShowEnemyGuardiansCVar;

// STATIC: 0x00BD09C4
CVar* g_nameplateShowEnemyTotemsCVar;

// STATIC: 0x00BD09C8
CVar* g_nameplateShowFriendsCVar;

// STATIC: 0x00BD09CC
CVar* g_nameplateShowFriendlyPetsCVar;

// STATIC: 0x00BD09D0
CVar* g_nameplateShowFriendlyGuardiansCVar;

// STATIC: 0x00BD09D4
CVar* g_nameplateShowFriendlyTotemsCVar;

// STATIC: 0x00BD09D8
CVar* s_cvNameplateAllowOverlap;

// STATIC: 0x00BD09DC
CVar* g_unitHighlightsCVar;

// STATIC: 0x00BD09E0
CVar* g_enablePVPNotifyAFKCVar;

// STATIC: 0x00BD09E4
CVar* g_serviceTypeFilterCVar;

// STATIC: 0x00BD09E8
CVar* g_autojoinPartyVoiceCVar;

// STATIC: 0x00BD09EC
CVar* g_autojoinBGVoiceCVar;

// STATIC: 0x00BD09F0
CVar* g_combatLogRetentionTimeCVar;

// STATIC: 0x00BD09F4
CVar* g_currencyTokensUnused1CVar;

// STATIC: 0x00BD09F8
CVar* g_currencyTokensUnused2CVar;

// STATIC: 0x00BD09FC
CVar* g_currencyTokensBackpack1CVar;

// STATIC: 0x00BD0A00
CVar* g_currencyTokensBackpack2CVar;

// STATIC: 0x00BD0A04
CVar* g_predictedHealthCVar;

// STATIC: 0x00BD0A08
CVar* g_predictedPowerCVar;

// STATIC: 0x00BD0A0C
CVar* g_threatWarningCVar;

// STATIC: 0x00BD0A10
CVar* g_threatWorldTextCVar;

// STATIC: 0x00BD0A14
CVar* s_cvThreatShowNumeric;

// STATIC: 0x00BD0A18
CVar* s_cvThreatPlaySounds;

// STATIC: 0x00BD0A1C
CVar* s_cvShowAllSpellRanks;

// STATIC: 0x00BD0A20
CVar* g_ShowClassColorInNameplateCVar;

// STATIC: 0x00BD0A24
CVar* g_previewTalentsCVar;

// STATIC: 0x00BD0A28
CVar* s_cvLfgSelectedRoles;

// STATIC: 0x00BD0A2C
CVar* s_cvLfdCollapsedHeaders;

// STATIC: 0x00BD0A30
CVar* s_cvLfdSelectedDungeons;

// STATIC: 0x00BD0A34
CVar* s_cvQuestLogCollapseFilter;

// STATIC: 0x00BD0A38
CVar* s_cvAutoFilledMultiCastSlots;

// STATIC: 0x00BD0A3C
CVar* g_questPOICVar;

// STATIC: 0x00BD0A40
CVar* g_trackerSortingCVar;

// STATIC: 0x00BD0A44
CVar* s_cvShowToastFriendRequest;

// STATIC: 0x00BD0A48
CVar* s_cvShowToastOnline;

// STATIC: 0x00BD0A4C
CVar* s_cvShowToastOffline;

// STATIC: 0x00BD0A50
CVar* s_cvShowToastBroadcast;

// STATIC: 0x00BD0A54
CVar* s_cvShowToastConversation;

// VERIFY: 32517395 auto
// OFFSET: 0x00512700
bool AutoInteractStateChangedCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        camera->UpdateAutoInteractState(SStrToInt(value) != 0);
    }

    return true;
}

// VERIFY: 17fa790f auto
// OFFSET: 0x00512730
bool GuildShowOfflineCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGGuildInfo::SetShowOffline(StringToBOOL(value));
    return true;
}

// VERIFY: 436e46c1 auto
// OFFSET: 0x00512750
bool CVarHandler_guildRecruitmentChannel(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGChat::SetGuildRecruitmentMode(SStrToInt(value), 0);
    return true;
}

// VERIFY: 16ef0d16 auto
// OFFSET: 0x00512770
bool ShowClassColorInNameplateCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (!oldValue || SStrToInt(oldValue) != SStrToInt(value)) {
        CGUnit_C::NamePlateUpdateColor(true);
    }

    return true;
}

// VERIFY: b50a1249 auto
// OFFSET: 0x005127A0
bool SetCalendarFilterCVarCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (CGGameUI::m_initialized) {
        CGCalendar::SetFilter(var, SStrToInt(value));
    }

    return true;
}

// VERIFY: c4f24286 auto
// OFFSET: 0x005127D0
bool CVarHandler_previewTalents(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t spent = CGTalentInfo::SumPreviewPointsSpent(0);
    CGTalentInfo::RecalculatePreviewPoints(0);
    FrameScript_SignalEvent(SCRIPT_EVENT_PREVIEW_TALENT_POINTS_CHANGED, "%d%d%d%d", 0, 0, 0, -spent);

    int32_t petSpent = CGTalentInfo::SumPreviewPointsSpent(1);
    CGTalentInfo::RecalculatePreviewPoints(1);
    FrameScript_SignalEvent(SCRIPT_EVENT_PREVIEW_PET_TALENT_POINTS_CHANGED, "%d%d%d%d", 0, 0, 0, -petSpent);

    return true;
}

// VERIFY: 81db4ba2 auto
// OFFSET: 0x00512830
bool CVarHandler_autoCompleteResortNamesOnRecency(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGAutoCompleteName::s_resortNamesOnRecency = SStrToInt(value) != 0;
    CGAutoCompleteName::ResortAll();
    return true;
}

// VERIFY: e7b82005 auto
// OFFSET: 0x00512850
bool CVarHandler_autoCompleteWhenEditingFromCenter(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGAutoCompleteName::s_whenEditingFromCenter = SStrToInt(value) != 0;
    CGAutoCompleteName::ResortAll();
    return true;
}

// VERIFY: 501bae54 auto
// OFFSET: 0x00512870
bool CVarHandler_autoCompleteUseContext(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGAutoCompleteName::s_useContext = SStrToInt(value) != 0;
    CGAutoCompleteName::ResortAll();
    return true;
}

// VERIFY: e7ca0f30 auto
// OFFSET: 0x00518B60
bool CVarHandler_uiScale(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (g_useUiScaleCVar && g_useUiScaleCVar->m_intValue) {
        float scale = SStrToFloat(value);

        if (CGGameUI::m_aspect * 3.0f < 4.0f && CGGameUI::m_aspect * 0.75f <= scale) {
            scale = CGGameUI::m_aspect * 0.75f;
        }

        CGGameUI::SetUIScale(scale);
    }

    return true;
}

// VERIFY: 0f4b01e7 auto
// OFFSET: 0x00518BD0
bool CGGameUI::UnitHighlightsCVarCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (CGGameUI::m_initialized) {
        CGGameUI::UpdateUnitHighlights();
    }

    return true;
}

// VERIFY: f785f74f auto
// OFFSET: 0x005237E0
bool CVarHandler_useUiScale(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value)) {
        float scale = g_uiScaleCVar->m_floatValue;

        if (CGGameUI::m_aspect * 3.0f < 4.0f && CGGameUI::m_aspect * 0.75f <= scale) {
            scale = CGGameUI::m_aspect * 0.75f;
        }

        CGGameUI::SetUIScale(scale);
    } else {
        CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->CapsWindowSizeInScreenCoords(rect);
        CGGameUI::ScaleToScreenSize(static_cast<int32_t>(rect.maxX - rect.minX), static_cast<int32_t>(rect.maxY - rect.minY), 1);
    }

    return true;
}

// VERIFY: ef98ea6d auto
// OFFSET: 0x0052A930
bool SetTaintLogCallback(CVar* var, const char* oldValue, const char* value, void* arg) {
    if (SStrToInt(value) > 0) {
        if (s_taintLog || SLogCreate("Logs\\taint.log", 0, &s_taintLog)) {
            lua_tainteventcallback = TaintEventCallback;
        }
    } else {
        lua_tainteventcallback = nullptr;
    }

    return true;
}

// VERIFY: acfaf284 auto
// OFFSET: 0x0051D990
bool CVarHandler_threatWarning(CVar* var, const char* oldValue, const char* value, void* arg) {
    CGGameUI::UpdateThreatWarningEnabled(SStrToInt(value));
    return true;
}

// VERIFY: 24146977 auto
// OFFSET: 0x0051D9B0
void CGGameUI::RegisterGameCVars() {
    g_deselectOnClickCVar = CVar::Register("deselectOnClick", "Clear the target when clicking on terrain", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_cvAutoInteract = CVar::Register("autoInteract", "Toggles auto-move to interact target", 0x10, s_locale == 1 ? "1" : "0", AutoInteractStateChangedCallback, CATEGORY_GAME, false, nullptr, false);
    g_autoStandCVar = CVar::Register("autoStand", "Automatically stand when needed", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoDismountCVar = CVar::Register("autoDismount", "Automatically dismount when needed", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoDismountFlyingCVar = CVar::Register("autoDismountFlying", "If enabled, your character will automatically dismount before casting while flying", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoUnshiftCVar = CVar::Register("autoUnshift", "Automatically leave shapeshift form when needed", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoClearAFKCVar = CVar::Register("autoClearAFK", "Automatically clear AFK when moving or chatting", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_blockTradesCVar = CVar::Register("blockTrades", "Whether to automatically block trade requests", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_lootUnderMouseCVar = CVar::Register("lootUnderMouse", "Whether the loot window should open under the mouse", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoLootDefaultCVar = CVar::Register("autoLootDefault", "Automatically loot items when the loot window opens", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("alwaysCompareItems", "Always show item comparison tooltips", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("equipmentManager", "Enables the equipment management UI", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_assistAttackCVar = CVar::Register("assistAttack", "Whether to start attacking after an assist", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvAutoRangedCombat = CVar::Register("autoRangedCombat", "Whether your character will automatically switch between auto attack and auto shot", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autoSelfCastCVar = CVar::Register("autoSelfCast", "Whether spells should automatically be cast on you if you don't have a valid target", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_stopAutoAttackOnTargetChangeCVar = CVar::Register("stopAutoAttackOnTargetChange", "Whether to stop attacking when changing targets", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_showTargetOfTargetCVar = CVar::Register("showTargetOfTarget", "Whether the target of target frame should be shown", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_targetOfTargetModeCVar = CVar::Register("targetOfTargetMode", "The conditions under which target of target should be shown", 0x10, "5", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_showTargetCastbarCVar = CVar::Register("showTargetCastbar", "Show the spell your current target is casting", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_showVKeyCastbarCVar = CVar::Register("showVKeyCastbar", "If the V key display is up for your current target, show the enemy cast bar with the target's health bar in the game field", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_rotateMinimapCVar = CVar::Register("rotateMinimap", "Whether to rotate the entire minimap instead of the player arrow", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_minimapZoomCVar = CVar::Register("minimapZoom", "The current outdoor minimap zoom level", 0x20, "3", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_minimapInsideZoomCVar = CVar::Register("minimapInsideZoom", "The current indoor minimap zoom level", 0x20, "3", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_minimapPortalMaxCVar = CVar::Register("minimapPortalMax", "Max Number of Portals to traverse for minimap", 0x20, "99", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_scriptErrorsCVar = CVar::Register("scriptErrors", "Whether or not the UI shows Lua errors", 0x10, "0", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    g_screenEdgeFlashCVar = CVar::Register("screenEdgeFlash", "Whether to show a red flash while you are in combat with the world map up", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_showLootSpamCVar = CVar::Register("showLootSpam", "Whether to show verbose loot rolls", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_displayFreeBagSlotsCVar = CVar::Register("displayFreeBagSlots", "Whether or not the backpack button should indicate how many inventory slots you've got free", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_displayWorldPVPObjectivesCVar = CVar::Register("displayWorldPVPObjectives", "Whether to show world PvP objectives", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showClock", "Whether to display the time manager's clock button", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("colorblindMode", "Enables colorblind accessibility features in the game", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("questFadingDisable", "Whether to disable quest text slowly fading in", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("autoQuestWatch", "Whether to automatically watch all quests when you obtain them", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("autoQuestProgress", "Whether to automatically watch all quests when they are updated", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showQuestTrackingTooltips", "Displays quest tracking information in unit and object tooltips", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("mapQuestDifficulty", "Whether to color quest titles by difficulty in the World Map", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvQuestLogCollapseFilter = CVar::Register("questLogCollapseFilter", "bit filed for saving off the state of the headers in Quest Log", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvQuestLogCollapseFilter->Set(-1, true, false, false, true);
    CVar::Register("advancedWatchFrame", "Enables advanced Objectives tracking features", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("watchFrameIgnoreCursor", "Disables Objectives frame mouseover and title dropdown.", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("watchFrameBaseAlpha", "Objectives frame opacity.", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("watchFrameState", "Stores Objectives frame locked and collapsed states", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showQuestObjectivesOnMap", "Shows quest POIs on the main map.", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("trackedQuests", "Internal cvar for saving tracked quests in order", 0x120, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("trackedAchievements", "Internal cvar for saving tracked achievements in order", 0x120, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("flaggedTutorials", "Internal cvar for saving compleated tutorials in order", 0x110, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("advancedWorldMap", "Enables advanced World Map features", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("worldMapOpacity", "Opacity for the world map when sized down", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("watchFrameWidth", "Controls objectives frame width", 0x0, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_trackerSortingCVar = CVar::Register("trackerSorting", "sorting option for the objectives tracker", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("trackerFilter", "filter option for the objectives tracker", 0x20, "7", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_profanityFilterCVar = CVar::Register("profanityFilter", "Whether to enable mature language filtering", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_spamFilterCVar = CVar::Register("spamFilter", "Whether to enable spam filtering", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_chatBubblesCVar = CVar::Register("chatBubbles", "Whether to show in-game chat bubbles", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_chatBubblesPartyCVar = CVar::Register("chatBubblesParty", "Whether to show in-game chat bubbles for party chat", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_removeChatDelayCVar = CVar::Register("removeChatDelay", "Remove Chat Hover Delay", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_guildShowOfflineCVar = CVar::Register("guildShowOffline", "Show offline guild members in the guild UI", 0x10, "1", GuildShowOfflineCallback, CATEGORY_GAME, false, nullptr, false);
    g_guildMemberNotifyCVar = CVar::Register("guildMemberNotify", "Receive notification when guild members log on/off", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_guildRecruitmentChannelCVar = CVar::Register("guildRecruitmentChannel", "Whether to automatically join the guild recruitment channel when not in a guild", 0x10, "1", CVarHandler_guildRecruitmentChannel, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("lfgAutoFill", "Whether to automatically add party members while looking for a group", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("lfgAutoJoin", "Whether to automatically join a party while looking for a group", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("friendsViewButtons", "Whether to show the friends list view buttons", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("friendsSmallView", "Whether to use smaller buttons in the friends list", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("chatStyle", "The style of Edit Boxes for the ChatFrame. Valid values: \"classic\", \"im\"", 0x10, "im", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("wholeChatWindowClickable", "Whether the user may click anywhere on a chat window to change EditBox focus (only works in IM style)", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("conversationMode", "The action new Real ID Conversations take by default: \"popout\", \"inline\"", 0x10, "popout", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showTimestamps", "The format of timestamps in chat or \"none\"", 0x10, "none", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("chatMouseScroll", "Whether the user can use the mouse wheel to scroll through chat", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("lockActionBars", "Whether the action bars should be locked, preventing changes", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("alwaysShowActionBars", "Whether to always show the action bar grid", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_secureAbilityToggleCVar = CVar::Register("secureAbilityToggle", "Whether you should be protected against accidentally double-clicking an aura", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    PlayerNameRegisterCVars();
    g_combatDamageCVar = CVar::Register("CombatDamage", "Display damage numbers over hostile creatures when damaged", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvCombatLogPeriodicSpells = CVar::Register("CombatLogPeriodicSpells", "Display damage caused by periodic effects", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_petMeleeDamageCVar = CVar::Register("PetMeleeDamage", "Display pet melee damage in the world", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_petSpellDamageCVar = CVar::Register("PetSpellDamage", "Display pet spell damage in the world", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_combatHealingCVar = CVar::Register("CombatHealing", "Display amount of healing you did to the target", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("enableCombatText", "Whether to show floating combat text", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("combatTextFloatMode", "The combat text float mode", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctCombatState", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctDodgeParryMiss", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctDamageReduction", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctRepChanges", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctReactives", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctFriendlyHealers", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctComboPoints", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctLowManaHealth", nullptr, 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctEnergyGains", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctPeriodicEnergyGains", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctHonorGains", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctAuras", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctAllSpellMechanics", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctSpellMechanics", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fctSpellMechanicsOther", nullptr, 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("xpBarText", "Whether the XP bar shows the numeric experience value", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("playerStatusText", "Whether the player portrait shows numeric health/mana values", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("petStatusText", "Whether the pet portrait shows numeric health/mana values", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("partyStatusText", "Whether the party portraits shows numeric health/mana values", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("targetStatusText", "Whether the target portrait shows numeric health/mana values", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("statusTextPercentage", "Whether numeric health/mana values are shown as raw values or percentages", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showPartyBackground", "Show a background behind party members", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("partyBackgroundOpacity", "The opacity of the party background", 0x10, "0.5", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("hidePartyInRaid", "Whether to hide the party UI while in a raid", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showPartyPets", "Whether to show pets in the party UI", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showRaidRange", "Show range indicator in raid UI", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showArenaEnemyFrames", "Show arena enemy frames while in an Arena", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showArenaEnemyCastbar", "Show the spell enemies are casting on the Arena Enemy frames", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showArenaEnemyPets", "Show the enemy team's pets on the ArenaEnemy frames", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("fullSizeFocusFrame", "Increases the size of the focus frame to that of the target frame", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_buffDurationsCVar = CVar::Register("buffDurations", "Whether to show buff durations", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showDispelDebuffs", "Show only Debuffs that the player can dispel.  Only applies to raids.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showCastableBuffs", "Show only Buffs the player can cast.  Only applies to raids.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("consolidateBuffs", "Consolidates buffs displayed for the player.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showCastableDebuffs", "Show only debuffs the player can apply.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvShowToastOnline = CVar::Register("showToastOnline", "Whether to show Battle.net message for friend coming online", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvShowToastOffline = CVar::Register("showToastOffline", "Whether to show Battle.net message for friend going offline", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvShowToastBroadcast = CVar::Register("showToastBroadcast", "Whether to show Battle.net message for broadcasts", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvShowToastFriendRequest = CVar::Register("showToastFriendRequest", "Whether to show Battle.net message for friend requests", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvShowToastConversation = CVar::Register("showToastConversation", "Whether to show Battle.net message for conversations", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showToastWindow", "Whether to show Battle.net system messages in a toast window", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("toastDuration", "How long to display Battle.net toast windows, in seconds", 0x10, "4", nullptr, CATEGORY_GAME, false, nullptr, false);
    CameraRegisterCVars();
    CVar::Register("showNewbieTips", "Show beginner tooltips", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("UberTooltips", "Show verbose tooltips", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showItemLevel", "Show item level in the tooltip", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showTutorials", "display tutorials", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvCalendarShowWeeklyHolidays = CVar::Register("calendarShowWeeklyHolidays", "Whether weekly holidays should appear in the calendar", 0x20, "1", SetCalendarFilterCVarCallback, CATEGORY_GAME, false, nullptr, false);
    s_cvCalendarShowDarkmoon = CVar::Register("calendarShowDarkmoon", "Whether Darkmoon Faire holidays should appear in the calendar", 0x20, "1", SetCalendarFilterCVarCallback, CATEGORY_GAME, false, nullptr, false);
    s_cvCalendarShowBattlegrounds = CVar::Register("calendarShowBattlegrounds", "Whether Battleground holidays should appear in the calendar", 0x20, "0", SetCalendarFilterCVarCallback, CATEGORY_GAME, false, nullptr, false);
    s_cvCalendarShowLockouts = CVar::Register("calendarShowLockouts", "Whether raid lockouts should appear in the calendar", 0x20, "1", SetCalendarFilterCVarCallback, CATEGORY_GAME, false, nullptr, false);
    s_cvCalendarShowResets = CVar::Register("calendarShowResets", "Whether raid resets should appear in the calendar", 0x20, "0", SetCalendarFilterCVarCallback, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowEnemiesCVar = CVar::Register("nameplateShowEnemies", nullptr, 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowEnemyPetsCVar = CVar::Register("nameplateShowEnemyPets", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowEnemyGuardiansCVar = CVar::Register("nameplateShowEnemyGuardians", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowEnemyTotemsCVar = CVar::Register("nameplateShowEnemyTotems", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowFriendsCVar = CVar::Register("nameplateShowFriends", nullptr, 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowFriendlyPetsCVar = CVar::Register("nameplateShowFriendlyPets", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowFriendlyGuardiansCVar = CVar::Register("nameplateShowFriendlyGuardians", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_nameplateShowFriendlyTotemsCVar = CVar::Register("nameplateShowFriendlyTotems", nullptr, 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvNameplateAllowOverlap = CVar::Register("nameplateAllowOverlap", "switches between overlapping nameplates or the (old) never overlapping version", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_unitHighlightsCVar = CVar::Register("unitHighlights", "Whether the highlight circle around units should be displayed", 0x10, "1", CGGameUI::UnitHighlightsCVarCallback, CATEGORY_GAME, false, nullptr, false);
    g_enablePVPNotifyAFKCVar = CVar::Register("enablePVPNotifyAFK", "The ability to shutdown the AFK notification system", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);

    char serviceTypeFilter[128];
    SStrPrintf(serviceTypeFilter, sizeof(serviceTypeFilter), "%d", 3);
    g_serviceTypeFilterCVar = CVar::Register("serviceTypeFilter", "Which trainer services to show", 0x10, serviceTypeFilter, nullptr, CATEGORY_GAME, false, nullptr, false);

    g_autojoinPartyVoiceCVar = CVar::Register("autojoinPartyVoice", "Automatically join the voice session in party/raid chat", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_autojoinBGVoiceCVar = CVar::Register("autojoinBGVoice", "Automatically join the voice session in battleground chat", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("PushToTalkSound", "Play a sound when voice recording activates and deactivates", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("combatLogOn", "Whether or not the combat log is shown", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showKeyring", "Whether or not the keyring is shown", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showBattlefieldMinimap", "Whether or not the battlefield minimap is shown", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("playerStatLeftDropdown", "The player stat selected in the left dropdown", 0x20, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("playerStatRightDropdown", "The player stat selected in the right dropdown", 0x20, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("talentFrameShown", "The talent UI has been shown", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("auctionDisplayOnCharacter", "Show auction items on the dress-up paperdoll", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("addFriendInfoShown", "The info for Add Friend has been shown", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("pendingInviteInfoShown", "The info for pending invites has been shown", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);

    const char* militaryTime = "1";

    switch (LocaleNameToIndex(s_localeCode)) {
        case DB_LOCALE_enUS:
        case DB_LOCALE_koKR:
        case DB_LOCALE_esMX: {
            militaryTime = "0";
            break;
        }
    }

    CVar::Register("timeMgrUseMilitaryTime", "Toggles the display of either 12 or 24 hour time", 0x10, militaryTime, nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("timeMgrUseLocalTime", "Toggles the use of either the realm time or your system time", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("timeMgrAlarmTime", "The time manager's alarm time in minutes", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("timeMgrAlarmMessage", "The time manager's alarm message", 0x10, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("timeMgrAlarmEnabled", "Toggles whether or not the time manager's alarm will go off", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_combatLogRetentionTimeCVar = CVar::Register("combatLogRetentionTime", "The maximum duration in seconds to retain combat log entries", 0x10, "300", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_currencyTokensUnused1CVar = CVar::Register("currencyTokensUnused1", "Currency token types marked as unused.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_currencyTokensUnused2CVar = CVar::Register("currencyTokensUnused2", "Currency token types marked as unused.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_currencyTokensBackpack1CVar = CVar::Register("currencyTokensBackpack1", "Currency token types shown on backpack.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_currencyTokensBackpack2CVar = CVar::Register("currencyTokensBackpack2", "Currency token types shown on backpack.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showTokenFrame", "The token UI has been shown", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("showTokenFrameHonor", "The token UI has shown Honor", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_predictedHealthCVar = CVar::Register("predictedHealth", "Whether or not to use predicted health values in the UI", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_predictedPowerCVar = CVar::Register("predictedPower", "Whether or not to use predicted power values in the UI", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_threatWarningCVar = CVar::Register("threatWarning", "Whether or not to show threat warning UI (0 = off, 1 = in dungeons, 2 = in party/raid, 3 = always)", 0x20, "3", CVarHandler_threatWarning, CATEGORY_GAME, false, nullptr, false);
    g_threatWorldTextCVar = CVar::Register("threatWorldText", "Whether or not to show threat floaters in combat", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvThreatShowNumeric = CVar::Register("threatShowNumeric", "Whether or not to show numeric threat on the target and focus frames", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvThreatPlaySounds = CVar::Register("threatPlaySounds", "Whether or not to sounds when certain threat transitions occur", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    ViolenceLevelsRegisterCVars();
    s_cvShowAllSpellRanks = CVar::Register("ShowAllSpellRanks", "show either all spell ranks, or only the highest rank", 0x10, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_ShowClassColorInNameplateCVar = CVar::Register("ShowClassColorInNameplate", "use this to display the class color in the nameplate health bar", 0x20, "0", ShowClassColorInNameplateCallback, CATEGORY_GAME, false, nullptr, false);
    g_previewTalentsCVar = CVar::Register("previewTalents", "Toggles the ability to preview talents before spending talent points.", 0x20, "0", CVarHandler_previewTalents, CATEGORY_GAME, false, nullptr, false);
    s_cvLfgSelectedRoles = CVar::Register("lfgSelectedRoles", "Stores what roles the player is willing to take on.", 0x120, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvLfdCollapsedHeaders = CVar::Register("lfdCollapsedHeaders", "Stores which LFD headers are collapsed.", 0x120, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvLfdSelectedDungeons = CVar::Register("lfdSelectedDungeons", "Stores which LFD dungeons are selected.", 0x120, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("lastTalkedToGM", "Stores the last GM someone was talking to in case they reload the UI while the GM chat window is open.", 0x10, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("autoCompleteResortNamesOnRecency", "Shows people you recently spoke with higher up on the AutoComplete list.", 0x10, "1", CVarHandler_autoCompleteResortNamesOnRecency, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("autoCompleteWhenEditingFromCenter", "If you edit a name by inserting characters into the center, a smarter auto-complete will occur.", 0x10, "1", CVarHandler_autoCompleteWhenEditingFromCenter, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("autoCompleteUseContext", "The system will, for example, only show people in your guild when you are typing /gpromote. Names will also never be removed.", 0x10, "1", CVarHandler_autoCompleteUseContext, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("colorChatNamesByClass", "If enabled, the name of a player speaking in chat will be colored according to his class.", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    s_cvAutoFilledMultiCastSlots = CVar::Register("autoFilledMultiCastSlots", "Bitfield that saves whether multi-cast slots have been automatically filled.", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("minimapTrackedInfo", "Stores the minimap tracking that was active last session.", 0x20, "", nullptr, CATEGORY_GAME, false, nullptr, false);
    g_questPOICVar = CVar::Register("questPOI", "If enabled, the quest POI system will be used.", 0x20, "1", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("miniWorldMap", "Whether or not the world map has been toggled to smaller size", 0x20, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
    CVar::Register("dontShowEquipmentSetsOnItems", "Don't show which equipment sets an item is associated with", 0x10, "0", nullptr, CATEGORY_GAME, false, nullptr, false);
}

// VERIFY: d0dc1b08 auto
// OFFSET: 0x005238A0
void CGGameUI::Initialize() {
    g_uiScaleCVar = CVar::Register("uiScale", "The current UI scale", 0, "1.0", CVarHandler_uiScale, 4, false, nullptr, false);
    g_useUiScaleCVar = CVar::Register("useUiScale", "Whether or not the UI scale should be used", 0, "0", CVarHandler_useUiScale, 4, false, nullptr, false);
}
