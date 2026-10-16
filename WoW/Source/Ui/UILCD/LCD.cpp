#include "LCD.h"
#include <cmath>
#include <cstring>
#include <new>
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaPOIRec.h"
#include "WoW/Source/DB/Rec/BattlemasterListRec.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CWOWClientStatus.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/RaidInfo.h"
#include "WoW/Source/Ui/UILCD/Windows/EZ_LCD.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDBitmap.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDText.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDUtils.h"
#include "WoW/Source/Ui/WorldMap.h"
#include "WoW/Source/UIUtil/Tooltip.h"

// STATIC: 0x00B1C0BC
TSList<POIIcon, TSGetLink<POIIcon> > CGLCD::m_POIIcons;

// STATIC: 0x00B1C0C8
TSList<MAPDATA, TSGetLink<MAPDATA> > CGLCD::m_mapData;

// STATIC: 0x00D41314
CEzLcd* CGLCD::m_lcd;

// STATIC: 0x00D41330
int32_t CGLCD::m_chatAlertLines;

// STATIC: 0x00D41338
uint8_t CGLCD::initComplete;

// STATIC: 0x00D41348
TSHashTable<LCDHANDLE, HASHKEY_CONSTSTRI> CGLCD::m_handles;

// STATIC: 0x00D41318
CGPlayer_C* CGLCD::m_player;

// STATIC: 0x00D4131C
uint8_t CGLCD::m_inWorld;

// STATIC: 0x00D4131D
static uint8_t s_classFlag8;

// STATIC: 0x00D4131E
static uint8_t s_inBattlefield;

// STATIC: 0x00D412D0
static char s_battlefieldAlert[64];

// STATIC: 0x00D41320
static uint32_t s_battlefieldPortExpiration;

// STATIC: 0x00D4132C
static uint32_t s_battlefieldStartTime;

// STATIC: 0x00B1C098
static int32_t s_currentScreen = -1;

// STATIC: 0x00B1C09C
static int32_t s_previousScreen = -1;

// STATIC: 0x00B1C0A0
static int32_t s_subscreen;

// STATIC: 0x00B1C0A4
static int32_t s_alertDuration = 30000;

// STATIC: 0x00B1C0A8
static uint8_t s_alertExpired = 1;

// STATIC: 0x00D41270
static char s_idleLogoutText[32];

// STATIC: 0x00D41290
static char s_outbidText[32];

// STATIC: 0x00D412B0
static char s_whisperFromText[32];

// STATIC: 0x00D4131F
static uint8_t s_battlefieldAlertShown;

// STATIC: 0x00D41334
static uint32_t s_alertExpireTime;

// STATIC: 0x00D41339
static uint8_t s_loginPending;

// STATIC: 0x00D4133C
static TSFixedArray<CHATMESSAGE> s_chatMessages;

// STATIC: 0x00D41370
static HBITMAP s_lastBitmap;

// VERIFY: d134d241 auto
// OFFSET: 0x00848030
static bool IsLCDAvailable() {
    if (CGLCD::m_lcd->IsDeviceAvailable(1)) {
        return true;
    }

    switch (s_locale) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 6:
        case 7: {
            return true;
        }

        default: {
            return false;
        }
    }
}

// VERIFY: cf81c9f7 auto
// OFFSET: 0x00848140
void CGLCD::ShowBattlefieldScreen() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        s_inBattlefield = 0;
        CGLCD::SetScreen(1);
    }
}

// VERIFY: e986adf3 auto
// OFFSET: 0x00848180
void CGLCD::ClearWaitQueueScreen() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::SetScreen(0);
    }
}

// VERIFY: 6e32be08 manual
// OFFSET: 0x0084AED0
void CGLCD::Initialize() {
    CGLCD::m_lcd = new (SMemAlloc(sizeof(CEzLcd), __FILE__, __LINE__, 0)) CEzLcd();
    CGLCD::m_lcd->Initialize("World of Warcraft", 2, 0, 1, nullptr, nullptr);
    CGLCD::m_lcd->Update();

    if (!CGLCD::m_lcd->IsConnected()) {
        CGLCD::Shutdown();
        return;
    }

    EventRegister(EVENT_ID_IDLE, CGLCD::Update);
}

// VERIFY: 99ffe797 auto
// OFFSET: 0x0084A6E0
void CGLCD::OnBattlefieldActive() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        s_inBattlefield = 1;
        s_battlefieldStartTime = CGBattlefieldInfo::m_instanceStartTime;
        CGLCD::UpdateBattlefieldStats();
        CGLCD::SetScreen(3);
    }
}

// VERIFY: 20771c88 auto
// OFFSET: 0x0084AE50
void CGLCD::Shutdown() {
    CGLCD::m_handles.Clear();

    if (CGLCD::m_lcd) {
        CEzLcd* lcd = CGLCD::m_lcd;
        lcd->~CEzLcd();
        SMemFree(lcd, "delete", SLOG_FUNCTION, 0);
    }

    CGLCD::m_lcd = nullptr;
    EventUnregister(EVENT_ID_IDLE, CGLCD::Update);
    CGLCD::m_mapData.Clear();
    CGLCD::m_POIIcons.Clear();
    CGLCD::initComplete = 0;
    CLCDUtils::Shutdown();
    CGLCD::m_chatAlertLines = 0;
}

// VERIFY: b378b02a auto
// OFFSET: 0x0084A530
void CGLCD::OnWhisper(const char* name) {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::PushAlert(name, 1);
    }
}

// VERIFY: 0fd36640 auto
// OFFSET: 0x0084A5B0
void CGLCD::AuctionOutbid(const char* name) {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::PushAlert(name, 3);
    }
}

// VERIFY: 29915760 auto
// OFFSET: 0x0084A5F0
void CGLCD::AuctionExpired(const char* name) {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete && CGLCD::m_lcd->IsDeviceAvailable(1)) {
        CGLCD::PushAlert(name, 4);
    }
}

// VERIFY: ed6aa2a2 auto
// OFFSET: 0x0084A640
void CGLCD::AuctionSold(const char* name) {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete && CGLCD::m_lcd->IsDeviceAvailable(1)) {
        CGLCD::PushAlert(name, 6);
    }
}

// VERIFY: e4264b44 auto
// OFFSET: 0x0084A690
void CGLCD::AuctionWon(const char* name) {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete && CGLCD::m_lcd->IsDeviceAvailable(1)) {
        CGLCD::PushAlert(name, 5);
    }
}

// VERIFY: b3ae8be5 auto
// OFFSET: 0x00848380
void CGLCD::HideBattlefieldAlert() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete && s_currentScreen == 2) {
        CGLCD::SetScreen(s_previousScreen);
    }
}

// VERIFY: a406c5e1 auto
// OFFSET: 0x008483C0
void CGLCD::OnBattlefieldReady(const BattlefieldStatus* status) {
    if (!CGLCD::m_lcd || !CGLCD::m_lcd->IsConnected() || !IsLCDAvailable() || !CGLCD::initComplete || !status) {
        return;
    }

    if (status->m_bgTypeID == 32) {
        BattlemasterListRec* rec = g_battlemasterListDB.GetRecord(32);

        if (rec) {
            CGLCD::SetScreen(2);
            SStrPrintf(s_battlefieldAlert, sizeof(s_battlefieldAlert), "%s", rec->m_name);
            s_battlefieldPortExpiration = status->m_portExpiration;
        }
    } else {
        MapRec* map = g_mapDB.GetRecord(status->m_mapID);

        if (map) {
            CGLCD::SetScreen(2);
            SStrPrintf(s_battlefieldAlert, sizeof(s_battlefieldAlert), "%s %d", map->m_mapName, status->m_instanceID);
            s_battlefieldPortExpiration = status->m_portExpiration;
        }
    }
}

// VERIFY: 138d395a auto
// OFFSET: 0x00848490
void CGLCD::EnterWorld() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::m_inWorld = 1;
        CGLCD::m_player = ClntObjMgrGetActivePlayerObj();
    }
}

// VERIFY: 297f9b99 auto
// OFFSET: 0x00848070
void CGLCD::LeaveWorld() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::m_inWorld = 0;
        CGLCD::m_player = nullptr;
    }
}

// VERIFY: 2bb3a983 auto
// OFFSET: 0x00848B70
void CGLCD::Login() {
    if (!CGLCD::m_lcd || !CGLCD::m_lcd->IsConnected() || !IsLCDAvailable() || !CGLCD::initComplete) {
        return;
    }

    CGLCD::m_player = ClntObjMgrGetActivePlayerObj();
    CGLCD::SetScreen(1);
    CGLCD::m_lcd->ShowSubscreen(0);
    CGPlayer_C* player = CGLCD::m_player;
    s_subscreen = 0;
    CGLCD::m_inWorld = 1;

    ChrClassesRec* classRec = g_chrClassesDB.GetRecord(player->m_unit->m_unitFieldBytes0[1]);

    if (classRec) {
        s_classFlag8 = (classRec->m_flags >> 3) & 1;
    }

    int32_t soulShard;

    switch (player->m_unit->m_unitFieldBytes0[1]) {
        case 1:
        case 3:
        case 4: {
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO"), 1);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO_FLD"), 1);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("SOUL_SHARD"), 0);
            soulShard = 0;
            break;
        }

        case 9: {
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO"), 0);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO_FLD"), 0);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("SOUL_SHARD"), 1);
            soulShard = 1;
            break;
        }

        default: {
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO"), 0);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AMMO_FLD"), 0);
            CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("SOUL_SHARD"), 0);
            soulShard = 0;
            break;
        }
    }

    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("SOUL_SHARD_FLD"), soulShard);
    const char* text = FrameScript_GetText("BF_NOT_IN", -1, GENDER_NOT_APPLICABLE);
    CGLCD::m_lcd->SetText(CGLCD::GetHandle("BF_NOT_IN"), text);
}

// VERIFY: 61997259 auto
// OFFSET: 0x008480B0
HRESULT CEzLcd::SetText(void* handle, const char* text) {
    uint16_t buffer[1024];
    ConvertUTF8toUTF16(buffer, 1024, text, 0x7FFFFFFF, nullptr, nullptr);
    return CGLCD::m_lcd->SetText(handle, reinterpret_cast<const WCHAR*>(buffer));
}

// VERIFY: 638d6a34 auto
// OFFSET: 0x00848100
void CGLCD::SetScreen(int32_t screen) {
    if (screen == s_currentScreen) {
        return;
    }

    s_previousScreen = s_currentScreen;
    s_currentScreen = screen;
    CGLCD::m_lcd->ShowPage(screen);
    CGLCD::m_lcd->HideGlobals();
}

// VERIFY: 994f51ce auto
// OFFSET: 0x008481F0
void CGLCD::Logout() {
    if (!CGLCD::m_lcd || !CGLCD::m_lcd->IsConnected() || !IsLCDAvailable()) {
        return;
    }

    if (CGLCD::initComplete) {
        CGLCD::m_inWorld = 0;
        CGLCD::SetScreen(0);
        s_inBattlefield = 0;
        CGLCD::m_player = nullptr;
        CGLCD::m_lcd->Update();
    }
}

// VERIFY: 6c3ba040 auto
// OFFSET: 0x00847E90
void LCDHANDLE::SetText(const char* text) {
    this->m_text.Copy(text);
}

// VERIFY: 6373bbc4 auto
// OFFSET: 0x00847EA0
int32_t CGLCD::GetScreenFromName(const char* name) {
    if (!SStrCmpI(name, "WoWTitleScreen", STORM_MAX_STR)) {
        return 0;
    }

    if (!SStrCmpI(name, "PlayerInfoScreen", STORM_MAX_STR)) {
        return 1;
    }

    if (!SStrCmpI(name, "BattlefieldAlertScreen", STORM_MAX_STR)) {
        return 2;
    }

    if (!SStrCmpI(name, "BattlefieldStatusScreen", STORM_MAX_STR)) {
        return 3;
    }

    if (!SStrCmpI(name, "NotInBattlefieldScreen", STORM_MAX_STR)) {
        return 4;
    }

    if (!SStrCmpI(name, "WaitQueueScreen", STORM_MAX_STR)) {
        return 5;
    }

    if (!SStrCmpI(name, "ChatAlertScreen", STORM_MAX_STR)) {
        return 6;
    }

    if (!SStrCmpI(name, "ArathiBasinMapScreen", STORM_MAX_STR)) {
        return 7;
    }

    if (!SStrCmpI(name, "WarsongMapScreen", STORM_MAX_STR)) {
        return 8;
    }

    if (!SStrCmpI(name, "EyeOfTheStormMapScreen", STORM_MAX_STR)) {
        return 9;
    }

    if (!SStrCmpI(name, "StrandOfTheAncientsMapScreen", STORM_MAX_STR)) {
        return 10;
    }

    if (!SStrCmpI(name, "AlteracValleyMapScreen", STORM_MAX_STR)) {
        return 11;
    }

    if (!SStrCmpI(name, "IsleOfConquestMapScreen", STORM_MAX_STR)) {
        return 12;
    }

    return -1;
}

// VERIFY: 81ad9697 auto
// OFFSET: 0x00848010
static bool IsAsianLocale() {
    return s_locale == 1 || (s_locale > 3 && s_locale <= 5);
}

// VERIFY: f6f78e1f auto
// OFFSET: 0x008481B0
static void SetSubscreen(int32_t subscreen) {
    if (subscreen == 3) {
        CGLCD::m_lcd->ShowSubscreen(0);
        s_subscreen = subscreen;
    } else {
        CGLCD::m_lcd->ShowSubscreen(subscreen);
        s_subscreen = subscreen;
    }
}

// VERIFY: 4fba18fe auto
// OFFSET: 0x00848320
static const char* GetAmmoToken() {
    if (!CGLCD::m_player) {
        return "";
    }

    switch (CGLCD::m_player->m_unit->m_unitFieldBytes0[1]) {
        case 1:
        case 3:
        case 4:
            return "AMMO";

        case 9:
            return "SOUL_SHARD";

        default:
            return "";
    }
}

// VERIFY: 1ed041a2 manual
// OFFSET: 0x008484D0
void CGLCD::CheckInput() {
    if (!CGLCD::m_inWorld) {
        return;
    }

    if (CGLCD::m_lcd->ButtonReleased(2)) {
        CGLCD::SetScreen(6);
    } else {
        if (CGLCD::m_lcd->ButtonReleased(1)) {
            if (s_inBattlefield) {
                CGLCD::SetScreen(3);
            } else if (s_battlefieldPortExpiration <= OsGetAsyncTimeMs()) {
                CGLCD::SetScreen(4);
            } else {
                CGLCD::SetScreen(2);
            }
        } else if (CGLCD::m_lcd->ButtonReleased(0)) {
            if ((!IsAsianLocale() || CGLCD::m_lcd->IsDeviceAvailable(1)) && s_currentScreen == 1) {
                s_subscreen = (s_subscreen + 1) % 4;
                SetSubscreen(s_subscreen);
            }

            CGLCD::SetScreen(1);
        } else {
            goto checkNavigation;
        }

        s_alertExpired = 1;
    }

checkNavigation:
    if (CGLCD::m_lcd->ButtonReleased(5)) {
        switch (s_currentScreen) {
            case 1:
                CGLCD::SetScreen(s_inBattlefield ? 3 : 4);
                break;

            case 2:
            case 3:
            case 4:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                CGLCD::SetScreen(6);
                break;

            case 6:
                CGLCD::SetScreen(1);
                break;
        }
    } else if (CGLCD::m_lcd->ButtonReleased(4)) {
        switch (s_currentScreen) {
            case 1:
                CGLCD::SetScreen(6);
                break;

            case 2:
            case 3:
            case 4:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                CGLCD::SetScreen(1);
                break;

            case 6:
                CGLCD::SetScreen(s_inBattlefield ? 3 : 4);
                break;
        }
    } else if (CGLCD::m_lcd->ButtonReleased(6)) {
        switch (s_currentScreen) {
            case 1:
                s_subscreen = (s_subscreen + 1) % 4;
                SetSubscreen(s_subscreen);
                break;

            case 3:
                switch (CGPlayer_C::GetNewContinentID()) {
                    case 30:
                        CGLCD::SetScreen(11);
                        break;

                    case 489:
                        CGLCD::SetScreen(8);
                        break;

                    case 529:
                        CGLCD::SetScreen(7);
                        break;

                    case 566:
                        CGLCD::SetScreen(9);
                        break;

                    case 607:
                        CGLCD::SetScreen(10);
                        break;

                    case 628:
                        CGLCD::SetScreen(12);
                        break;
                }

                CGWorldMap::SetMapToCurrentZone(0);
                break;

            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                CGLCD::SetScreen(3);
                break;
        }
    } else if (CGLCD::m_lcd->ButtonReleased(7)) {
        switch (s_currentScreen) {
            case 1:
                if (s_subscreen) {
                    s_subscreen = (s_subscreen - 1) % 4;
                    SetSubscreen(s_subscreen);
                } else {
                    s_subscreen = 3;
                    SetSubscreen(3);
                }

                break;

            case 3:
                switch (CGPlayer_C::GetNewContinentID()) {
                    case 30:
                        CGLCD::SetScreen(11);
                        break;

                    case 489:
                        CGLCD::SetScreen(8);
                        break;

                    case 529:
                        CGLCD::SetScreen(7);
                        break;

                    case 566:
                        CGLCD::SetScreen(9);
                        break;

                    case 607:
                        CGLCD::SetScreen(10);
                        break;

                    case 628:
                        CGLCD::SetScreen(12);
                        break;
                }

                CGWorldMap::SetMapToCurrentZone(0);
                break;

            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                CGLCD::SetScreen(3);
                break;
        }
    }
}

// VERIFY: 3864be2e auto
// OFFSET: 0x00848780
static int32_t GetAmmoCount() {
    if (!CGLCD::m_player) {
        return 0;
    }

    CGPlayer_C* player = CGLCD::m_player;
    int32_t ammoId = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerAmmoId : 0;

    if (CGLCD::m_player->m_unit->m_unitFieldBytes0[1] == 9) {
        return CGLCD::m_player->m_bag.GetItemTypeCount(6265, 0);
    }

    if (ammoId) {
        return CGLCD::m_player->m_bag.GetItemTypeCount(ammoId, 0);
    }

    return 0;
}

// VERIFY: 5308707b auto
// OFFSET: 0x00848B50
void* CGLCD::GetHandle(const char* name) {
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);
    return handle ? handle->m_handle : nullptr;
}

// VERIFY: b364aa19 manual
// OFFSET: 0x00848D50
void CGLCD::UpdateMapPositions() {
    C3Vector position = { 0.0f, 0.0f, 0.0f };
    CGWorldMap::GetPlayerPosition(ClntObjMgrGetActivePlayer(), &position.x, &position.y);

    MAPDATA* map;

    for (map = CGLCD::m_mapData.Head(); map; map = CGLCD::m_mapData.Next(map)) {
        if (map->m_screen == s_currentScreen) {
            break;
        }
    }

    int32_t y = CMath::fint_(map->m_sizeY * position.y + map->m_offsetY - 8.0f);
    int32_t x = CMath::fint_(map->m_sizeX * position.x + map->m_offsetX - 8.0f);
    CGLCD::m_lcd->SetOrigin(CGLCD::GetHandle("PlayerIcon"), x, y);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("PlayerIcon"), 1);

    for (uint32_t i = 0; i < CGRaidInfo::m_numMembers; i++) {
        CGRaidMember* member = CGRaidInfo::m_members[i];

        if (member->m_guid != ClntObjMgrGetActivePlayer() && member->m_guid) {
            CGWorldMap::GetPlayerPosition(member->m_guid, &position.x, &position.y);
            char name[32];
            SStrPrintf(name, sizeof(name), "PartyIcon%d", i + 1);
            void* handle = CGLCD::GetHandle(name);
            CGLCD::m_lcd->SetVisible(handle, 1);
            int32_t memberY = CMath::fint_(map->m_sizeY * position.y + map->m_offsetY - 8.0f);
            int32_t memberX = CMath::fint_(map->m_sizeX * position.x + map->m_offsetX - 8.0f);
            CGLCD::m_lcd->SetOrigin(handle, memberX, memberY);
        }
    }

    CGBattlefieldInfo::RequestPlayerPositions();
    uint32_t numLandmarks = CGWorldMap::m_numLandmarks;

    for (POIIcon* icon = CGLCD::m_POIIcons.Head(); icon; icon = CGLCD::m_POIIcons.Next(icon)) {
        uint8_t visible = 0;

        for (uint32_t i = 0; i < numLandmarks; i++) {
            WorldMapLandmarkInfo* landmark = &CGWorldMap::m_landmarks.m_data[i];

            if (i >= CGWorldMap::m_numLandmarks || !landmark || landmark->m_id != icon->m_id) {
                continue;
            }

            AreaPOIRec* rec = g_areaPOIDB.GetRecord(landmark->m_id);

            if ((landmark->m_isGossip || CGWorldMap::m_currentZone == -1) && rec->m_flags < 0) {
                uint32_t iconIndex = 0;

                if (rec->m_worldStateID) {
                    iconIndex = GetWorldState(rec->m_worldStateID) - 1;

                    if (iconIndex >= 9) {
                        iconIndex = 0;
                    }
                }

                C2iVector size = { 0, 0 };
                HBITMAP bitmap = CLCDUtils::GetBitmapForIcon(rec->m_icon[iconIndex], &size);
                CGLCD::m_lcd->SetBitmap(icon->m_handle, bitmap);
                visible = 1;
                int32_t iconY = CMath::fint_(map->m_sizeY * landmark->m_y + map->m_offsetY - 8.0f);
                int32_t iconX = CMath::fint_(map->m_sizeX * landmark->m_x + map->m_offsetX - 8.0f);
                CGLCD::m_lcd->SetOrigin(icon->m_handle, iconX, iconY);
            }

            break;
        }

        CGLCD::m_lcd->SetVisible(icon->m_handle, visible);
    }

    uint8_t allianceVisible = 0;
    uint8_t hordeVisible = 0;

    for (uint32_t i = 0; i < CGBattlefieldInfo::GetNumFlagCarriers(); i++) {
        WOWGUID carrier = CGBattlefieldInfo::GetFlagCarrier(i);
        C3Vector flag = { 0.0f, 0.0f, 0.0f };

        if (!carrier || (CGWorldMap::GetPlayerPosition(carrier, &flag.x, &flag.y), flag.x == 0.0f && flag.y == 0.0f)) {
            if (CGBattlefieldInfo::GetFlagPosition(i, &flag)) {
                flag.z = 0.0f;
                float flagX;
                float flagY;
                CGWorldMap::GetWorldPosition(flag, CGBattlefieldInfo::m_activeMapID, &flagX, &flagY, nullptr, false, 0);
                flag.x = flagX;
                flag.y = flagY;
                flag.z = 0.0f;
            }
        }

        const char* token = CGBattlefieldInfo::GetFlagToken(i);

        if (!SStrCmpI(token, "AllianceFlag", STORM_MAX_STR)) {
            allianceVisible = 1;
            int32_t flagY = CMath::fint_(map->m_sizeY * flag.y + map->m_offsetY - 8.0f);
            int32_t flagX = CMath::fint_(map->m_sizeX * flag.x + map->m_offsetX - 8.0f);
            CGLCD::m_lcd->SetOrigin(CGLCD::GetHandle("AllianceFlag"), flagX, flagY);
        } else if (!SStrCmpI(token, "HordeFlag", STORM_MAX_STR)) {
            hordeVisible = 1;
            int32_t flagY = CMath::fint_(map->m_sizeY * flag.y + map->m_offsetY - 8.0f);
            int32_t flagX = CMath::fint_(map->m_sizeX * flag.x + map->m_offsetX - 8.0f);
            CGLCD::m_lcd->SetOrigin(CGLCD::GetHandle("HordeFlag"), flagX, flagY);
        }
    }

    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("AllianceFlag"), allianceVisible);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("HordeFlag"), hordeVisible);
}

// VERIFY: b3783823 manual
// OFFSET: 0x00849310
void CGLCD::SetStat(const char* name, int32_t value) {
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

    if (!handle) {
        return;
    }

    if (SStrCmpI(name, "SOUL_SHARD", STORM_MAX_STR)) {
        CEzLcd::SetText(handle->m_handle, FrameScript_GetText(handle->m_text.m_str, -1, GENDER_NOT_APPLICABLE));
    } else if (CGLCD::m_lcd->IsDeviceAvailable(1)) {
        ItemStats_C* stats = g_itemDBCache.GetRecord(6265, WOWGUID{ 0 }, nullptr, nullptr, false);

        if (stats) {
            CEzLcd::SetText(handle->m_handle, stats->GetName(1));
        } else {
            CEzLcd::SetText(handle->m_handle, "SOUL_SHARD");
        }
    } else {
        CEzLcd::SetText(handle->m_handle, FrameScript_GetText("SHARDS", -1, GENDER_NOT_APPLICABLE));
    }

    char text[32];
    SStrPrintf(text, sizeof(text), "%d", value);
    char fieldName[32];
    SStrPrintf(fieldName, sizeof(fieldName), "%s_FLD", name);
    LCDHANDLE* field = CGLCD::m_handles.Ptr(fieldName);

    if (field && field->m_handle) {
        CEzLcd::SetText(field->m_handle, text);
    }
}

// VERIFY: 7d51b4c3 auto
// OFFSET: 0x00849430
void CGLCD::SetStat(const char* name, float value, bool percent) {
    bool showPercent = CGLCD::m_lcd->IsDeviceAvailable(1) ? percent : false;
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

    if (!handle) {
        return;
    }

    CEzLcd::SetText(handle->m_handle, FrameScript_GetText(handle->m_text.m_str, -1, GENDER_NOT_APPLICABLE));
    char text[32];

    if (showPercent) {
        SStrPrintf(text, sizeof(text), "%.2f%%", value);
    } else {
        SStrPrintf(text, sizeof(text), "%.2f", value);
    }

    char fieldName[32];
    SStrPrintf(fieldName, sizeof(fieldName), "%s_FLD", name);
    LCDHANDLE* field = CGLCD::m_handles.Ptr(fieldName);

    if (field && field->m_handle) {
        CEzLcd::SetText(field->m_handle, text);
    }
}

// VERIFY: ca2718ad auto
// OFFSET: 0x00849520
void CGLCD::SetStat(const char* name, int32_t minValue, int32_t maxValue) {
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

    if (!handle) {
        return;
    }

    CEzLcd::SetText(handle->m_handle, FrameScript_GetText(handle->m_text.m_str, -1, GENDER_NOT_APPLICABLE));
    char text[32];
    SStrPrintf(text, sizeof(text), "%d-%d", minValue, maxValue);
    char fieldName[32];
    SStrPrintf(fieldName, sizeof(fieldName), "%s_FLD", name);
    LCDHANDLE* field = CGLCD::m_handles.Ptr(fieldName);

    if (field && field->m_handle) {
        CEzLcd::SetText(field->m_handle, text);
    }
}

// VERIFY: c37023b7 auto
// OFFSET: 0x008495E0
void CGLCD::SetStat(const char* name, const char* value) {
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

    if (!handle) {
        return;
    }

    CEzLcd::SetText(handle->m_handle, FrameScript_GetText(handle->m_text.m_str, -1, GENDER_NOT_APPLICABLE));
    char text[32];
    SStrPrintf(text, sizeof(text), "%s", value);
    char fieldName[32];
    SStrPrintf(fieldName, sizeof(fieldName), "%s_FLD", name);
    LCDHANDLE* field = CGLCD::m_handles.Ptr(fieldName);

    if (field && field->m_handle) {
        CEzLcd::SetText(field->m_handle, text);
    }
}

// VERIFY: 50ac7387 auto
// OFFSET: 0x008496A0
void CGLCD::SetHandleText(const char* name, const char* text) {
    LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

    if (handle) {
        CEzLcd::SetText(handle->m_handle, text);
    }
}

// VERIFY: 47b42533 manual
// OFFSET: 0x00849700
void CGLCD::DisplayMeleeStats() {
    float maxDamage = CGLCD::m_player->m_unit->m_unitFieldMaxdamage;
    int32_t minDamage = static_cast<int32_t>(floor(CGLCD::m_player->m_unit->m_unitFieldMindamage));

    if (minDamage <= 1) {
        minDamage = 1;
    }

    CGLCD::SetStat("DAMAGE", minDamage, ceil(maxDamage) <= 1 ? 1 : static_cast<int32_t>(ceil(maxDamage)));
    float speed = CGLCD::m_player->m_unit->m_unitFieldBaseattacktime[0] * 0.001f;
    CGItem_C* offhand = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(WOWGUID{ CGLCD::m_player->m_bag.GetItemGuid(16) }, TYPEMASK_ITEM, __FILE__, __LINE__));

    if (offhand) {
        offhand->GetClassID();
    }

    CGLCD::SetStat("SPEED", speed, false);
    int32_t power = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldAttackPower);
    int32_t positive = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldAttackPowerMods[0]);
    int32_t negative = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldAttackPowerMods[1]);
    CGLCD::SetStat("POWER", power + positive + negative);
    CGLCD::SetStat("HIT", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerFieldCombatRating1[5]));
    CGLCD::SetStat("CRIT", CGLCD::m_player->m_player->m_playerCritPercentage, true);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("EXPERTISE"), 1);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("EXPERTISE_FLD"), 1);
    CGLCD::SetStat("EXPERTISE", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerExpertise));
}

// VERIFY: a51aafb0 auto
// OFFSET: 0x00849960
void CGLCD::DisplayRangedStats() {
    const CGUnitVirtualItem* ranged = CGLCD::m_player->GetVirtualItem(2, false);
    float maxDamage = CGLCD::m_player->m_unit->m_unitFieldMaxrangeddamage;
    int32_t minDamage = static_cast<int32_t>(floor(CGLCD::m_player->m_unit->m_unitFieldMinrangeddamage));

    if (minDamage <= 1) {
        minDamage = 1;
    }

    int32_t maxValue = ceil(maxDamage) <= 1 ? 1 : static_cast<int32_t>(ceil(maxDamage));

    if (!ranged || s_classFlag8) {
        CGLCD::SetStat("DAMAGE", FrameScript_GetText("NOT_APPLICABLE", -1, GENDER_NOT_APPLICABLE));
    } else {
        CGLCD::SetStat("DAMAGE", minDamage, maxValue);
    }

    if (!ranged || s_classFlag8) {
        CGLCD::SetStat("SPEED", FrameScript_GetText("NOT_APPLICABLE", -1, GENDER_NOT_APPLICABLE));
    } else {
        CGLCD::SetStat("SPEED", CGLCD::m_player->m_unit->m_unitFieldRangedattacktime * 0.001f, false);
    }

    int32_t power = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldRangedAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldRangedAttackPower);
    int32_t positive = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldRangedAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldRangedAttackPowerMods[0]);
    int32_t negative = CMath::fint_((CGLCD::m_player->m_unit->m_unitFieldRangedAttackPowerMultiplier + 1.0f) * CGLCD::m_player->m_unit->m_unitFieldRangedAttackPowerMods[1]);
    CGLCD::SetStat("POWER", power + positive + negative);
    CGLCD::SetStat("HIT", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerFieldCombatRating1[6]));
    CGLCD::SetStat("CRIT", CGLCD::m_player->m_player->m_playerRangedCritPercentage, true);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("EXPERTISE"), 0);
    CGLCD::m_lcd->SetVisible(CGLCD::GetHandle("EXPERTISE_FLD"), 0);
}

// VERIFY: 1ed8f03e auto
// OFFSET: 0x00849BE0
void CGLCD::DisplayDefenseStats() {
    int32_t base;
    int32_t total;
    int32_t effective;
    int32_t positive;
    int32_t negative;
    CGLCD::m_player->GetResistanceAndBuffs(GetPhysicalDamageClassID(), &base, &total, &effective, &positive, &negative);
    CGLCD::SetStat("ARMOR2", effective);

    int32_t rank;
    int32_t bonus;
    CGLCD::m_player->GetDefenseSkillRank(rank, bonus);
    CGLCD::SetStat("DEFENSE", rank + bonus);
    CGLCD::SetStat("DODGE", CGLCD::m_player->m_player->m_playerDodgePercentage, true);
    CGLCD::SetStat("PARRY", CGLCD::m_player->m_player->m_playerParryPercentage, true);
    CGLCD::SetStat("BLOCK", CGLCD::m_player->m_player->m_playerBlockPercentage, true);

    CGPlayerLocalData* data = CGLCD::m_player->m_player;
    int32_t melee = static_cast<int32_t>(data->m_playerFieldCombatRating1[14]);
    int32_t ranged = static_cast<int32_t>(data->m_playerFieldCombatRating1[15]);
    int32_t spell = static_cast<int32_t>(data->m_playerFieldCombatRating1[16]);
    CGLCD::SetStat("RESILIENCE", (melee < ranged ? melee : ranged) < spell ? (melee < ranged ? melee : ranged) : spell);
}

// VERIFY: 69f4f132 manual
// OFFSET: 0x00849D10
void CGLCD::DisplaySpellStats() {
    CGPlayer_C* player = CGLCD::m_player;
    int32_t spellDamage = player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? player->m_player->m_playerFieldModDamageDonePos[1] : 0;

    for (int32_t school = 2; school < 7; school++) {
        int32_t damage = CGLCD::m_player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? CGLCD::m_player->m_player->m_playerFieldModDamageDonePos[school] : 0;

        if (spellDamage >= damage) {
            spellDamage = CGLCD::m_player->m_obj->m_guid == ClntObjMgrGetActivePlayer() ? CGLCD::m_player->m_player->m_playerFieldModDamageDonePos[school] : 0;
        }
    }

    CGLCD::SetStat("SPELL_DAMAGE", spellDamage);
    CGLCD::SetStat("SPELL_HEALING", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerFieldModHealingDonePos));
    CGLCD::SetStat("SPELL_HIT", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerFieldCombatRating1[7]));

    float* crit = CGLCD::m_player->m_player->m_playerSpellCritPercentage1;
    float spellCrit = crit[1];

    for (int32_t school = 2; school < 7; school++) {
        if (spellCrit >= crit[school]) {
            spellCrit = crit[school];
        }
    }

    CGLCD::SetStat("SPELL_CRIT", spellCrit, true);
    CGLCD::SetStat("SPELL_HASTE", static_cast<int32_t>(CGLCD::m_player->m_player->m_playerFieldCombatRating1[19]));
    int32_t manaRegen = 0;

    if (!CGLCD::m_player->m_unit->m_unitFieldBytes0[3]) {
        float regen = CGLCD::m_player->GetPowerRegen(0, false);
        manaRegen = static_cast<int32_t>(floor((regen + 0.001f) * 5.0f));
    }

    CGLCD::SetStat("MANA_REGEN", manaRegen);
}

// VERIFY: 52e62ff8 auto
// OFFSET: 0x00849F20
void CGLCD::SetAlertText() {
    for (int32_t i = 0; i < CGLCD::m_chatAlertLines;) {
        CHATMESSAGE& message = s_chatMessages[i];
        char format[64];

        switch (message.m_type) {
            case 1:
                SStrCopy(format, s_whisperFromText, STORM_MAX_STR);
                break;

            case 2:
                SStrCopy(format, s_idleLogoutText, STORM_MAX_STR);
                break;

            case 3:
                SStrCopy(format, s_outbidText, STORM_MAX_STR);
                break;

            case 4:
                SStrCopy(format, FrameScript_GetText("AUCTION_EXPIRED_MAIL_SUBJECT", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
                break;

            case 5:
                SStrCopy(format, FrameScript_GetText("AUCTION_WON_MAIL_SUBJECT", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
                break;

            case 6:
                SStrCopy(format, FrameScript_GetText("AUCTION_ITEM_SOLD", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
                break;

            default:
                SStrCopy(format, "", STORM_MAX_STR);
                break;
        }

        char text[64];
        SStrPrintf(text, sizeof(text), format, message.m_text);
        char name[32];
        SStrPrintf(name, sizeof(name), "CHAT_ALERT_%d", ++i);
        LCDHANDLE* handle = CGLCD::m_handles.Ptr(name);

        if (handle) {
            CEzLcd::SetText(handle->m_handle, text);
        }
    }
}

// VERIFY: ff5a2080 auto
// OFFSET: 0x0084A2E0
void CGLCD::UpdateBattlefieldStats() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPEMASK_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    int32_t numScores = CGBattlefieldInfo::m_numScores;
    int32_t alliance = 0;
    int32_t horde = 0;

    for (uint32_t i = 0; i < static_cast<uint32_t>(numScores); i++) {
        BattlefieldScore* score = i < CGBattlefieldInfo::m_numScoreEntries ? CGBattlefieldInfo::m_scores[i] : nullptr;

        if (!score) {
            continue;
        }

        if (score->m_guid == player->m_obj->m_guid) {
            CGLCD::SetStat("BF_KILLBLOWS", static_cast<int32_t>(score->m_killingBlows));
            CGLCD::SetStat("BF_DEATHS", static_cast<int32_t>(score->m_deaths));
            CGLCD::SetStat("BF_HONOR_KILLS", static_cast<int32_t>(score->m_honorableKills));
            CGLCD::SetStat("BF_DAMAGE", static_cast<int32_t>(score->m_damageDone));
            CGLCD::SetStat("BF_HEALING", static_cast<int32_t>(score->m_healingDone));
        }

        if (score->m_faction) {
            alliance++;
        } else {
            horde++;
        }
    }

    if (!numScores) {
        CGLCD::SetStat("BF_KILLBLOWS", 0);
        CGLCD::SetStat("BF_DEATHS", 0);
        CGLCD::SetStat("BF_HONOR_KILLS", 0);
        CGLCD::SetStat("BF_DAMAGE", 0);
        CGLCD::SetStat("BF_HEALING", 0);
    }

    CGLCD::SetStat("BF_ALLIANCE", alliance);
    CGLCD::SetStat("BF_HORDE", horde);
    CGLCD::SetStat("BF_TIME", static_cast<int32_t>((OsGetAsyncTimeMs() - s_battlefieldStartTime) / 60000));
    CGBattlefieldInfo::RequestScoreData();
}

// VERIFY: 3476656c manual
// OFFSET: 0x0084A450
void CGLCD::PushAlert(const char* text, int32_t type) {
    if (!CGLCD::m_lcd || !CGLCD::m_lcd->IsConnected() || !IsLCDAvailable() || !CGLCD::initComplete) {
        return;
    }

    CGLCD::SetScreen(6);
    s_alertExpireTime = s_alertDuration + OsGetAsyncTimeMs();
    s_alertExpired = 0;
    int32_t i = CGLCD::m_chatAlertLines - 1;

    for (; i > 0; i--) {
        SStrCopy(s_chatMessages[i].m_text, s_chatMessages[i - 1].m_text, STORM_MAX_STR);
        s_chatMessages[i].m_type = s_chatMessages[i - 1].m_type;
    }

    SStrCopy(s_chatMessages[i].m_text, text, sizeof(s_chatMessages[i].m_text));
    s_chatMessages[i].m_type = type;
    CGLCD::SetAlertText();
}

// VERIFY: ad610a9d auto
// OFFSET: 0x0084A570
void CGLCD::IdleDisconnectWarning() {
    if (CGLCD::m_lcd && CGLCD::m_lcd->IsConnected() && IsLCDAvailable() && CGLCD::initComplete) {
        CGLCD::PushAlert("", 2);
    }
}

// VERIFY: eac13edc manual
// OFFSET: 0x0084A790
void CGLCD::PostInit() {
    if (!CGLCD::LoadLayout()) {
        return;
    }

    SStrCopy(s_whisperFromText, FrameScript_GetText("WHISPER_FROM", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
    SStrCopy(s_outbidText, FrameScript_GetText("OUTBID_ON_SHORT", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
    SStrCopy(s_idleLogoutText, FrameScript_GetText("IDLE_LOGOUT_WARNING", -1, GENDER_NOT_APPLICABLE), STORM_MAX_STR);
    s_chatMessages.SetCount(CGLCD::m_chatAlertLines);

    for (int32_t i = 0; i < CGLCD::m_chatAlertLines; i++) {
        memset(s_chatMessages[i].m_text, 0, sizeof(s_chatMessages[i].m_text));
        s_chatMessages[i].m_type = 0;
        char name[64];
        SStrPrintf(name, sizeof(name), "CHAT_ALERT_%d", i + 1);
        s_chatMessages[i].m_handle = CGLCD::GetHandle(name);
    }

    const char* title = FrameScript_GetText("WORLD_OF_WARCRAFT", -1, GENDER_NOT_APPLICABLE);
    CGLCD::SetHandleText("WOWTITLE", title);
    CGLCD::SetScreen(0);
    CGLCD::m_lcd->Update();
    CGLCD::initComplete = 1;

    if (s_loginPending) {
        CGLCD::Login();
        s_loginPending = 0;
    }
}

// VERIFY: 0a9d100f auto
// OFFSET: 0x0084A940
int32_t CGLCD::Update(const void* data, void* param) {
    if (CGLCD::m_lcd && !CGLCD::initComplete) {
        CGLCD::m_lcd->Update();

        if (CGLCD::m_lcd->IsDeviceAvailable(1) || CGLCD::m_lcd->IsDeviceAvailable(0)) {
            CGLCD::PostInit();
        }
    }

    if (!CGLCD::m_lcd || !CGLCD::m_lcd->IsConnected() || !IsLCDAvailable()) {
        return 1;
    }

    if (CGLCD::m_inWorld && CGLCD::m_player) {
        CGLCD::CheckInput();

        if (s_currentScreen == 1) {
            CGLCD::SetStat("STRENGTH", CGLCD::m_player->m_unit->m_unitFieldStat[0]);
            CGLCD::SetStat("AGILITY", CGLCD::m_player->m_unit->m_unitFieldStat[1]);
            CGLCD::SetStat("STAMINA", CGLCD::m_player->m_unit->m_unitFieldStat[2]);
            CGLCD::SetStat("INTELLECT", CGLCD::m_player->m_unit->m_unitFieldStat[3]);
            CGLCD::SetStat("SPIRIT", CGLCD::m_player->m_unit->m_unitFieldStat[4]);

            if (CGLCD::m_lcd->IsDeviceAvailable(1)) {
                int32_t base;
                int32_t total;
                int32_t effective;
                int32_t positive;
                int32_t negative;
                CGLCD::m_player->GetResistanceAndBuffs(GetPhysicalDamageClassID(), &base, &total, &effective, &positive, &negative);
                CGLCD::SetStat("Armor", effective);
            } else if (IsAsianLocale() && !CGLCD::m_lcd->IsDeviceAvailable(1)) {
                float minDamage = CGLCD::m_player->m_unit->m_unitFieldMindamage;
                float maxDamage = CGLCD::m_player->m_unit->m_unitFieldMaxdamage;
                float percent = CGLCD::m_player->GetModDamageDonePct(0);
                int32_t positive = CGLCD::m_player->GetModDamageDonePos(0);
                int32_t negative = CGLCD::m_player->GetModDamageDoneNeg(0);
                float damage = (negative + (1.0f / percent * maxDamage - positive - negative + 1.0f / percent * minDamage - positive - negative) * 0.5f + positive) * percent;
                float average = 1.0f;

                if (damage > 1.0f) {
                    average = damage;
                }

                char text[32];
                SStrPrintf(text, sizeof(text), "%.1f", average / (CGLCD::m_player->m_unit->m_unitFieldBaseattacktime[0] * 0.001f));
                CEzLcd::SetText(CGLCD::GetHandle("DPS_FLD"), text);
            }

            switch (s_subscreen) {
                case 0:
                    CGLCD::DisplayMeleeStats();
                    break;

                case 1:
                    CGLCD::DisplayDefenseStats();
                    break;

                case 2:
                    CGLCD::DisplaySpellStats();
                    break;

                case 3:
                    CGLCD::DisplayRangedStats();
                    break;
            }

            CGLCD::SetStat(GetAmmoToken(), GetAmmoCount());
            CGLCD::SetStat("BAG", CGLCD::m_player->m_bag.GetNumFreeInvSlots());
            CGLCD::SetStat("DURABILITY", static_cast<int32_t>(CGLCD::m_player->m_bag.GetAvgEquippedDurability()));
        }

        uint32_t now = OsGetAsyncTimeMs();

        if (static_cast<int32_t>(now - s_alertExpireTime) >= 0 && !s_alertExpired) {
            s_alertExpired = 1;
            CGLCD::SetScreen(s_previousScreen);
        }

        if (s_battlefieldPortExpiration) {
            if (static_cast<int32_t>(now - s_battlefieldPortExpiration) >= 0) {
                if (s_battlefieldAlertShown) {
                    s_battlefieldPortExpiration = 0;
                    s_battlefieldAlertShown = 0;

                    if (s_currentScreen == 2) {
                        CGLCD::SetScreen(s_inBattlefield ? 3 : 1);
                    }
                }
            } else {
                char duration[32];
                CGTooltip::GetDurationString(duration, sizeof(duration), s_battlefieldPortExpiration - OsGetAsyncTimeMs(), "SPELL_TIME_REMAINING", nullptr, 1, 0);
                const char* token = CGLCD::m_handles.Ptr("BATTLEFIELD_ALERT")->m_text.m_str;
                const char* remaining = LanguageProcess(duration);
                char text[128];
                SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), s_battlefieldAlert, remaining);
                CGLCD::SetHandleText("BATTLEFIELD_ALERT", text);
                s_battlefieldAlertShown = 1;
            }
        }

        if (s_inBattlefield) {
            CGLCD::UpdateBattlefieldStats();
        }

        if (static_cast<uint32_t>(s_currentScreen - 7) <= 5) {
            CGLCD::UpdateMapPositions();
        }
    } else if (CGlueMgr::m_queuePosition[0]) {
        int32_t waitTime = CGlueMgr::m_estimatedWaitTime;
        int32_t position = CGlueMgr::m_queuePosition[0];
        char token[32];

        if (waitTime <= 0) {
            waitTime = 0;
        }

        if (waitTime > 60000) {
            SStrCopy(token, "QUEUE_TIME_LEFT", STORM_MAX_STR);
        } else if (waitTime) {
            SStrCopy(token, "QUEUE_TIME_LEFT_SECONDS", STORM_MAX_STR);
        } else {
            SStrCopy(token, "QUEUE_TIME_LEFT_UNKNOWN", STORM_MAX_STR);
        }

        char text[128];
        SStrPrintf(text, sizeof(text), FrameScript_GetText(token, -1, GENDER_NOT_APPLICABLE), position, waitTime / 60000);
        CGLCD::SetHandleText("WAIT_QUEUE", text);
        CGLCD::SetScreen(5);
    }

    CGLCD::m_lcd->Update();
    return 1;
}

// VERIFY: bc2eb3a6 manual
// OFFSET: 0x0084B410
bool CGLCD::LoadLCDMap(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    MAPDATA* map = CGLCD::m_mapData.NewNode(2, 0, 0);
    map->m_screen = screen;

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Size", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            map->m_sizeX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            map->m_sizeY = y && *y ? SStrToInt(y) : 0;
        } else if (!SStrCmpI(child->m_name.m_str, "Offset", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            map->m_offsetX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            map->m_offsetY = y && *y ? SStrToInt(y) : 0;
        }
    }

    return true;
}

// VERIFY: c850fcee manual
// OFFSET: 0x0084C100
bool CGLCD::LoadLCDText(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    int32_t size = 7;
    int32_t alignment = 0;
    int32_t maxWidth = 0;
    int32_t originX = 0;
    int32_t originY = 0;
    int32_t sizeX = 0;
    int32_t sizeY = 0;
    bool wrap = false;
    const char* name = node->GetAttributeByName("name");

    if (!name || !*name) {
        status->Add(STATUS_WARNING, "Text element has unknown name: %s", name);
        return false;
    }

    if (CGLCD::GetHandle(name)) {
        status->Add(STATUS_FATAL, "Attempted to create an LCD entitiy %s with duplicate key %d", name, 0);
        return false;
    }

    char text[1024];
    const char* textAttr = node->GetAttributeByName("text");

    if (textAttr && *textAttr) {
        SStrCopy(text, textAttr, STORM_MAX_STR);
    } else {
        SStrCopy(text, "", STORM_MAX_STR);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Font", STORM_MAX_STR)) {
            const char* fontSize = child->GetAttributeByName("Size");

            if (!fontSize || !*fontSize) {
                status->Add(STATUS_WARNING, "LCDText element %s has no or unknown font size");
                return false;
            }

            if (!SStrCmpI(fontSize, "LG_BIG", STORM_MAX_STR)) {
                size = 12;
            } else if (!SStrCmpI(fontSize, "LG_MEDIUM", STORM_MAX_STR)) {
                size = 8;
            } else if (!SStrCmpI(fontSize, "LG_SMALL", STORM_MAX_STR)) {
                size = 7;
            } else {
                size = SStrToInt(fontSize);
            }

            const char* align = child->GetAttributeByName("Alignment");

            if (!align || !*align || !SStrCmpI(align, "LG_LEFT", STORM_MAX_STR)) {
                alignment = 0;
            } else if (!SStrCmpI(align, "LG_CENTER", STORM_MAX_STR)) {
                alignment = 1;
            } else {
                alignment = SStrCmpI(align, "LG_RIGHT", STORM_MAX_STR) ? 0 : 2;
            }

            const char* maxWidthAttr = child->GetAttributeByName("MaxWidth");

            if (!maxWidthAttr || !*maxWidthAttr) {
                status->Add(STATUS_WARNING, "LCDText element %s has MaxWidth defined");
                return false;
            }

            maxWidth = SStrToInt(maxWidthAttr);
            const char* wrapAttr = child->GetAttributeByName("Wrap");

            if (wrapAttr && *wrapAttr && (*wrapAttr == 't' || *wrapAttr == 'T')) {
                wrap = true;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "RelOrigin", STORM_MAX_STR)) {
            const char* relTo = child->GetAttributeByName("RelTo");
            LCDHANDLE* relative = CGLCD::m_handles.Ptr(relTo);

            if (!relative) {
                status->Add(STATUS_WARNING, "LCDText %s relative to unknown LCDElement %s", name, relTo);
                return false;
            }

            const char* x = child->GetAttributeByName("x");
            int32_t offsetX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            int32_t offsetY = y && *y ? SStrToInt(y) : 0;
            originX = relative->m_x + offsetX;
            originY = relative->m_y + offsetY;
        } else if (!SStrCmpI(child->m_name.m_str, "Origin", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            originX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            originY = y && *y ? SStrToInt(y) : 0;
        } else if (!SStrCmpI(child->m_name.m_str, "Size", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            sizeX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            sizeY = y && *y ? SStrToInt(y) : 0;
        } else {
            status->Add(STATUS_WARNING, "Unknown attribute type of LCDText: %s", child->m_name.m_str);
        }
    }

    CGLCD::m_lcd->ModifyControlsOnPage(screen);
    CLCDText* lcdText = CGLCD::m_lcd->AddText(1, size, alignment, maxWidth, subscreen, 1, 0);
    CGLCD::m_lcd->SetOrigin(lcdText, originX, originY);
    CEzLcd::SetText(lcdText, text);
    CGLCD::m_lcd->SetTextWordWrap(lcdText, wrap);

    if (sizeX) {
        CGLCD::m_lcd->SetSize(lcdText, sizeX, sizeY);
    }

    LCDHANDLE* handle = CGLCD::m_handles.New(name, 0, 0);
    handle->m_handle = lcdText;
    handle->SetText(text);
    handle->m_x = originX;
    handle->m_y = originY;

    if (SStrStr(name, "CHAT_ALERT_")) {
        CGLCD::m_chatAlertLines++;
    }

    return true;
}

// VERIFY: 78e17fe9 manual
// OFFSET: 0x0084C660
bool CGLCD::LoadLCDBitmap(int32_t screen, XMLNode* node, CStatus* status) {
    int32_t originX = 0;
    int32_t originY = 0;
    int32_t sizeX = 0;
    int32_t sizeY = 0;
    int32_t mipLevel = 0;
    const char* name = node->GetAttributeByName("name");
    int32_t texTop = 0;
    int32_t texLeft = 0;
    int32_t texBottom = 0;
    int32_t texRight = 0;

    if (!name || !*name) {
        status->Add(STATUS_WARNING, "Text element has unknown name: %s", name);
        return false;
    }

    if (CGLCD::GetHandle(name)) {
        status->Add(STATUS_FATAL, "Attempted to create an LCD entity %s with duplicate key %s", name, name);
        return false;
    }

    int32_t type = 0;
    char fileName[256];
    const char* typeAttr = node->GetAttributeByName("type");

    if (typeAttr && *typeAttr) {
        if (!SStrCmpI("texture", typeAttr, STORM_MAX_STR)) {
            type = 2;
            const char* data = node->GetAttributeByName("data");

            if (!data || !*data) {
                status->Add(STATUS_WARNING, "LCD Bitmap (name: %S) has no data", name);
                return false;
            }

            SStrCopy(fileName, data, STORM_MAX_STR);
        } else if (!SStrCmpI("prevTexture", typeAttr, STORM_MAX_STR)) {
            type = 3;
        }
    }

    const char* mipLevelAttr = node->GetAttributeByName("miplevel");

    if (mipLevelAttr && *mipLevelAttr) {
        mipLevel = SStrToInt(mipLevelAttr);
    }

    int32_t alpha = 1;
    const char* alphaAttr = node->GetAttributeByName("alpha");

    if (alphaAttr && *alphaAttr) {
        alpha = SStrToInt(alphaAttr);
    }

    int32_t icon = -1;
    const char* iconAttr = node->GetAttributeByName("icon");

    if (iconAttr && *iconAttr) {
        icon = SStrToInt(iconAttr);

        if (icon != -1) {
            type = 4;
        }
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Origin", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            originX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            originY = y && *y ? SStrToInt(y) : 0;
        } else if (!SStrCmpI(child->m_name.m_str, "RelOrigin", STORM_MAX_STR)) {
            const char* relTo = child->GetAttributeByName("RelTo");
            LCDHANDLE* relative = CGLCD::m_handles.Ptr(relTo);

            if (!relative) {
                status->Add(STATUS_WARNING, "LCDBitmap %s relative to unknown LCDElement %s", name, relTo);
                return false;
            }

            const char* x = child->GetAttributeByName("x");
            int32_t offsetX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            int32_t offsetY = y && *y ? SStrToInt(y) : 0;
            originX = relative->m_x + offsetX;
            originY = relative->m_y + offsetY;
        } else if (!SStrCmpI(child->m_name.m_str, "Size", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");
            sizeX = x && *x ? SStrToInt(x) : 0;
            const char* y = child->GetAttributeByName("y");
            sizeY = y && *y ? SStrToInt(y) : 0;
        } else if (!SStrCmpI(child->m_name.m_str, "TexCoord", STORM_MAX_STR)) {
            const char* x = child->GetAttributeByName("x");

            if (x && *x) {
                texLeft = SStrToInt(x);
            }

            const char* y = child->GetAttributeByName("y");

            if (y && *y) {
                texTop = SStrToInt(y);
            }

            const char* width = child->GetAttributeByName("width");

            if (width && *width) {
                texRight = texLeft + SStrToInt(width);
            }

            const char* height = child->GetAttributeByName("height");

            if (height && *height) {
                texBottom = texTop + SStrToInt(height);
            }
        } else {
            status->Add(STATUS_WARNING, "LCD Bitmap (name: %s) has unknown child element", name);
        }
    }

    HBITMAP bitmap = nullptr;
    C2iVector bitmapSize = { 0, 0 };

    switch (type) {
        case 1:
            bitmap = LoadBitmapA(GetModuleHandleA(nullptr), nullptr);
            break;

        case 2:
            bitmap = CLCDUtils::CreateBitmapFromBLPTex(fileName, mipLevel, &bitmapSize, texTop, texLeft, texBottom, texRight);
            break;

        case 3:
            bitmap = s_lastBitmap;
            break;

        case 4:
            bitmap = CLCDUtils::GetBitmapForIcon(icon, &bitmapSize);
            break;
    }

    CGLCD::m_lcd->ModifyControlsOnPage(screen);
    CLCDBitmap* lcdBitmap = CGLCD::m_lcd->AddBitmap(sizeX, sizeY, bitmapSize.x, bitmapSize.y);
    CGLCD::m_lcd->SetBitmap(lcdBitmap, bitmap);
    s_lastBitmap = bitmap;
    CGLCD::m_lcd->SetOrigin(lcdBitmap, originX, originY);
    CGLCD::m_lcd->SetAlpha(lcdBitmap, alpha);

    LCDHANDLE* handle = CGLCD::m_handles.New(name, 0, 0);
    handle->m_y = originY;
    handle->m_handle = lcdBitmap;
    handle->m_x = originX;
    return true;
}

// VERIFY: 7a2c9599 auto
// OFFSET: 0x0084CC00
bool CGLCD::LoadLCDElement(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    const char* name = node->m_name.m_str;

    if (!name || !*name) {
        return false;
    }

    if (!SStrCmpI(name, "LCDText", STORM_MAX_STR)) {
        return CGLCD::LoadLCDText(screen, node, status, subscreen);
    }

    if (!SStrCmpI(name, "LCDBitmap", STORM_MAX_STR)) {
        return CGLCD::LoadLCDBitmap(screen, node, status);
    }

    if (!SStrCmpI(name, "LCDMap", STORM_MAX_STR)) {
        return CGLCD::LoadLCDMap(screen, node, status, -1);
    }

    status->Add(STATUS_WARNING, "Unknown LCD Element: %s", name);
    return false;
}

// VERIFY: e9a1626b auto
// OFFSET: 0x0084CCC0
bool CGLCD::LoadGlobals(XMLNode* node, CStatus* status) {
    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        CGLCD::LoadLCDElement(-1, child, status, -1);
    }

    return true;
}

// VERIFY: 9385021f manual
// OFFSET: 0x0084CCF0
bool CGLCD::GeneratePOIs(XMLNode* node) {
    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        int32_t screen = CGLCD::GetScreenFromName(child->GetAttributeByName("Screen"));

        if (screen == -1) {
            continue;
        }

        CGLCD::m_lcd->ModifyControlsOnPage(screen);
        int32_t continentID = SStrToInt(child->GetAttributeByName("id"));
        int32_t x = 0;

        for (int32_t i = 0; i < g_areaPOIDB.m_numRecords; i++) {
            AreaPOIRec* rec = i >= 0 && i < g_areaPOIDB.m_numRecords ? &g_areaPOIDB.m_records[i] : nullptr;

            if (!rec || rec->m_continentID != continentID) {
                continue;
            }

            C2iVector size = { 0, 0 };
            HBITMAP bitmap = CLCDUtils::GetBitmapForIcon(rec->m_icon[0], &size);
            CLCDBitmap* lcdBitmap = CGLCD::m_lcd->AddBitmap(size.x, size.y, size.x, size.y);
            CGLCD::m_lcd->SetBitmap(lcdBitmap, bitmap);
            float originX = 0.0f;
            float originY = 0.0f;
            CGLCD::m_lcd->SetOrigin(lcdBitmap, static_cast<int32_t>(static_cast<float>(x)), static_cast<int32_t>(originY));

            char name[64];
            SStrPrintf(name, sizeof(name), "%s_%s", rec->m_name, rec->m_description);
            LCDHANDLE* handle = CGLCD::m_handles.New(name, 0, 0);
            handle->m_handle = handle;
            handle->m_x = static_cast<int32_t>(originX);
            handle->m_y = static_cast<int32_t>(originY);

            POIIcon* icon = CGLCD::m_POIIcons.NewNode(2, 0, 0);
            icon->m_handle = lcdBitmap;
            icon->m_id = rec->m_id;
            SStrCopy(icon->m_name, name, STORM_MAX_STR);
            x += 16;
        }
    }

    return true;
}

// VERIFY: 85785302 auto
// OFFSET: 0x0084CFF0
bool CGLCD::LoadScreen(XMLNode* node, CStatus* status) {
    const char* name = node->GetAttributeByName("name");

    if (!name || !*name) {
        status->Add(STATUS_WARNING, "Screen declared with no name");
        return false;
    }

    int32_t screen = CGLCD::GetScreenFromName(name);
    CGLCD::m_lcd->AddNewPage(screen);
    CGLCD::m_lcd->ModifyControlsOnPage(screen);
    CGLCD::m_lcd->ShowPage(screen);

    const char* color = node->GetAttributeByName("BGColor");

    if (color && *color) {
        char red[8];
        char green[8];
        char blue[8];
        SStrTokenize(&color, red, sizeof(red), ",", nullptr);
        SStrTokenize(&color, green, sizeof(green), ",", nullptr);
        SStrTokenize(&color, blue, sizeof(blue), ",", nullptr);
        int32_t value = ((SStrToInt(blue) << 8) | SStrToInt(green)) << 8;
        CGLCD::m_lcd->SetBackground(value | static_cast<uint8_t>(SStrToInt(red)));
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (SStrCmpI(child->m_name.m_str, "LCDSubscreen", STORM_MAX_STR)) {
            if (!CGLCD::LoadLCDElement(screen, child, status, -1)) {
                return false;
            }

            continue;
        }

        const char* id = child->GetAttributeByName("id");

        if (!id || !*id) {
            continue;
        }

        int32_t subscreen = SStrToInt(id);

        for (XMLNode* element = child->m_child; element; element = element->m_next) {
            if (!CGLCD::LoadLCDElement(screen, element, status, subscreen)) {
                return false;
            }
        }
    }

    return true;
}

// VERIFY: 6e364b01 manual
// OFFSET: 0x0084D1A0
bool CGLCD::LoadLayout() {
    char fileName[260];

    if (CGLCD::m_lcd->IsDeviceAvailable(1)) {
        CGLCD::m_lcd->ModifyDisplay(1);
        SStrCopy(fileName, "Interface\\LCDXML\\LCDColorLayout.xml", STORM_MAX_STR);
    } else if (CGLCD::m_lcd->IsDeviceAvailable(0)) {
        CGLCD::m_lcd->ModifyDisplay(0);
        SStrCopy(fileName, "Interface\\LCDXML\\LCDLayout.xml", STORM_MAX_STR);
    } else {
        CGLCD::Shutdown();
        return false;
    }

    MD5_CTX md5;
    MD5Init(&md5);
    CWOWClientStatus status("Logs\\LCDXML.log");
    XMLTree* tree = FrameXML_LoadXML(fileName, &md5, &status);

    if (!tree) {
        CGLCD::Shutdown();
        return false;
    }

    for (XMLNode* child = XMLTree_GetRoot(tree)->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "Screen", STORM_MAX_STR)) {
            if (!CGLCD::LoadScreen(child, &status)) {
                CGLCD::Shutdown();
                XMLTree_Free(tree);
                return false;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "Globals", STORM_MAX_STR)) {
            CGLCD::LoadGlobals(child, &status);
        } else if (!SStrCmpI(child->m_name.m_str, "POI", STORM_MAX_STR)) {
            CGLCD::GeneratePOIs(child);
        } else {
            status.Add(STATUS_ERROR, "Unknown element: %s");
        }
    }

    XMLTree_Free(tree);
    return true;
}
