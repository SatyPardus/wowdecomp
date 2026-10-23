#include "CGlueMgr.h"
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <new>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/IConsole.h"
#include "Engine/Source/Event/CSizeEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FFX/EffectGlow.h"
#include "Engine/Source/FFX/FFX.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/Frame/SimpleFrameScript.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Os/OsURLDownload.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CRect.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrClassesRec.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/GameTipsRec.h"
#include "WoW/Source/Game/GameClient/FFXEffects.h"
#include "WoW/Source/Game/ValidateName.h"
#include "WoW/Source/Glue/CharacterCreation.h"
#include "WoW/Source/Glue/CharacterSelection.h"
#include "WoW/Source/Glue/CSimpleModelFFX.h"
#include "WoW/Source/Glue/CWOWClientStatus.h"
#include "WoW/Source/Glue/GlueMgr.h"
#include "WoW/Source/Glue/RealmList.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/Ui/CAccountMsgHeaders.h"
#include "WoW/Source/Ui/CKBSystem.h"
#include "WoW/Source/Ui/KnowledgeBase.h"
#include "WoW/Source/Ui/Screen.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/UILCD/LCD.h"
#include "WoW/Source/Ui/VideoOptions.h"
#include "WoW/Source/UIUtil/AddOns.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/WowServices/Login.h"
#include "WoW/Source/WowServices/LoginResponse.h"
#include "WoW/Source/WowServices/RealmConnection.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ChecksumExecutables.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/FileDownload.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/Types.h"

// STATIC: 0x009E2C28
const uint8_t InterfaceKey[256] = {
    0xC3, 0x5B, 0x50, 0x84, 0xB9, 0x3E, 0x32, 0x42, 0x8C, 0xD0, 0xC7, 0x48, 0xFA, 0x0E, 0x5D, 0x54,
    0x5A, 0xA3, 0x0E, 0x14, 0xBA, 0x9E, 0x0D, 0xB9, 0x5D, 0x8B, 0xEE, 0xB6, 0x84, 0x93, 0x45, 0x75,
    0xFF, 0x31, 0xFE, 0x2F, 0x64, 0x3F, 0x3D, 0x6D, 0x07, 0xD9, 0x44, 0x9B, 0x40, 0x85, 0x59, 0x34,
    0x4E, 0x10, 0xE1, 0xE7, 0x43, 0x69, 0xEF, 0x7C, 0x16, 0xFC, 0xB4, 0xED, 0x1B, 0x95, 0x28, 0xA8,
    0x23, 0x76, 0x51, 0x31, 0x57, 0x30, 0x2B, 0x79, 0x08, 0x50, 0x10, 0x1C, 0x4A, 0x1A, 0x2C, 0xC8,
    0x8B, 0x8F, 0x05, 0x2D, 0x22, 0x3D, 0xDB, 0x5A, 0x24, 0x7A, 0x0F, 0x13, 0x50, 0x37, 0x8F, 0x5A,
    0xCC, 0x9E, 0x04, 0x44, 0x0E, 0x87, 0x01, 0xD4, 0xA3, 0x15, 0x94, 0x16, 0x34, 0xC6, 0xC2, 0xC3,
    0xFB, 0x49, 0xFE, 0xE1, 0xF9, 0xDA, 0x8C, 0x50, 0x3C, 0xBE, 0x2C, 0xBB, 0x57, 0xED, 0x46, 0xB9,
    0xAD, 0x8B, 0xC6, 0xDF, 0x0E, 0xD6, 0x0F, 0xBE, 0x80, 0xB3, 0x8B, 0x1E, 0x77, 0xCF, 0xAD, 0x22,
    0xCF, 0xB7, 0x4B, 0xCF, 0xFB, 0xF0, 0x6B, 0x11, 0x45, 0x2D, 0x7A, 0x81, 0x18, 0xF2, 0x92, 0x7E,
    0x98, 0x56, 0x5D, 0x5E, 0x69, 0x72, 0x0A, 0x0D, 0x03, 0x0A, 0x85, 0xA2, 0x85, 0x9C, 0xCB, 0xFB,
    0x56, 0x6E, 0x8F, 0x44, 0xBB, 0x8F, 0x02, 0x22, 0x68, 0x63, 0x97, 0xBC, 0x85, 0xBA, 0xA8, 0xF7,
    0xB5, 0x40, 0x68, 0x3C, 0x77, 0x86, 0x6F, 0x4B, 0xD7, 0x88, 0xCA, 0x8A, 0xD7, 0xCE, 0x36, 0xF0,
    0x45, 0x6E, 0xD5, 0x64, 0x79, 0x0F, 0x17, 0xFC, 0x64, 0xDD, 0x10, 0x6F, 0xF3, 0xF5, 0xE0, 0xA6,
    0xC3, 0xFB, 0x1B, 0x8C, 0x29, 0xEF, 0x8E, 0xE5, 0x34, 0xCB, 0xD1, 0x2A, 0xCE, 0x79, 0xC3, 0x9A,
    0x0D, 0x36, 0xEA, 0x01, 0xE0, 0xAA, 0x91, 0x20, 0x54, 0xF0, 0x72, 0xD8, 0x1E, 0xC7, 0x89, 0xD2,
};

// STATIC: 0x009F4448
static const LoginErrorURL s_loginErrorURLs[5] = {
    { AUTH_BANNED, "AUTH_BANNED_URL" },
    { AUTH_DB_BUSY, "AUTH_DB_BUSY_URL" },
    { AUTH_NO_TIME, "AUTH_NO_TIME_URL" },
    { AUTH_SUSPENDED, "AUTH_SUSPENDED_URL" },
    { AUTH_PARENTAL_CONTROL, "AUTH_PARENTAL_CONTROL_URL" },
};

// STATIC: 0x00AC3DA0
int32_t CGlueMgr::m_loginResult = -1;

// STATIC: 0x00AC3DA4
int32_t CGlueMgr::m_loginState = -1;

// STATIC: 0x00AC3DA8
int32_t CGlueMgr::m_lastLoginResult = -1;

// STATIC: 0x00AC3DAC
int32_t CGlueMgr::m_lastLoginState = -1;

// STATIC: 0x00AC3DB8
static const char* s_serverAlertTag = "SERVERALERT:";

// STATIC: 0x00AC3DBC
static const char* s_serverAlertTagBOM = "\xEF\xBB\xBFSERVERALERT:";

// STATIC: 0x00AC4370
int32_t CGlueMgr::s_displayedCharacterIndex = -1;

// STATIC: 0x00AC3DC0
static TSExplicitList<ConsoleString, offsetof(ConsoleString, m_link)> s_connectionStatus;

// STATIC: 0x00B6A180
uint8_t CGlueMgr::m_ffxActive;

// STATIC: 0x00B6A184
FFX::Effect* CGlueMgr::m_deathEffect;

// STATIC: 0x00B6A188
FFX::Effect* CGlueMgr::m_glowEffect;

// STATIC: 0x00B6A18C
static HSLOG s_connectionLog;

// STATIC: 0x00B6A190
char CGlueMgr::m_serverAlert[2048];

// STATIC: 0x00B6A99C
uint32_t CGlueMgr::m_queueTime[8];

// STATIC: 0x00B6A9BC
int32_t CGlueMgr::m_queuePosition[8];

// STATIC: 0x00B6A9DC
MipBits* CGlueMgr::m_cursorMipBits;

// STATIC: 0x00B6A9E0
char CGlueMgr::m_currentScreen[64];

// STATIC: 0x00B6AA20
CSimpleTop* CGlueMgr::m_simpleTop;

// STATIC: 0x00B6AA24
int32_t CGlueMgr::m_reload;

// STATIC: 0x00B6AA28
int32_t CGlueMgr::m_initialized;

// STATIC: 0x00B6AA2C
int32_t CGlueMgr::m_suspended;

// STATIC: 0x00B6AA30
int32_t CGlueMgr::m_disconnectPending;

// STATIC: 0x00B6AA34
int32_t CGlueMgr::m_reconnect;

// STATIC: 0x00B6AA38
GLUE_IDLE_STATE CGlueMgr::m_idleState;

// STATIC: 0x00B6AA3C
uint32_t CGlueMgr::m_matrixChallengeCount;

// STATIC: 0x00B6AA40
char CGlueMgr::m_accountName[1280];

// STATIC: 0x00B6AF40
uint32_t CGlueMgr::m_matrixRemaining;

// STATIC: 0x00B6AF44
int32_t CGlueMgr::m_estimatedWaitTime;

// STATIC: 0x00B6AF48
int32_t CGlueMgr::m_displayingQueueDialog;

// STATIC: 0x00B6AF4C
CHARACTER_INFO* CGlueMgr::m_characterInfo;

// STATIC: 0x00B6AF50
int32_t CGlueMgr::m_rereadTOS;

// STATIC: 0x00B6AF54
int32_t CGlueMgr::m_acceptedTOS;

// STATIC: 0x00B6AF58
int32_t CGlueMgr::m_rereadEULA;

// STATIC: 0x00B6AF5C
int32_t CGlueMgr::m_acceptedEULA;

// STATIC: 0x00B6AF60
int32_t CGlueMgr::m_rereadTerminationWithoutNotice;

// STATIC: 0x00B6AF64
int32_t CGlueMgr::m_acceptedTerminationWithoutNotice;

// STATIC: 0x00B6AF68
int32_t CGlueMgr::m_rereadScanning;

// STATIC: 0x00B6AF6C
int32_t CGlueMgr::m_acceptedScanning;

// STATIC: 0x00B6AF70
int32_t CGlueMgr::m_rereadContest;

// STATIC: 0x00B6AF74
int32_t CGlueMgr::m_acceptedContest;

// STATIC: 0x00B6AF78
uint8_t CGlueMgr::m_authenticated;

// STATIC: 0x00B6AF79
bool CGlueMgr::s_scriptFunctionsLoaded;

// STATIC: 0x00B6AF7C
int32_t CGlueMgr::m_screenWidth;

// STATIC: 0x00B6AF80
int32_t CGlueMgr::m_screenHeight;

// STATIC: 0x00B6AF84
float CGlueMgr::m_aspect;

// STATIC: 0x00B6AF88
int32_t CGlueMgr::m_pendingServerAlert;

// STATIC: 0x00B6AF8C
int32_t CGlueMgr::m_processServerAlert;

// STATIC: 0x00B6AF90
uint32_t CGlueMgr::m_pendingTimerAlert;

// STATIC: 0x00B6AF94
int32_t CGlueMgr::m_accountMsgAvailable;

// STATIC: 0x00B6AF98
static int32_t s_disconnectReason;

// STATIC: 0x00B6AF9C
uint32_t CGlueMgr::m_numChangedOptionWarnings;

// STATIC: 0x00B6AFA0
int32_t CGlueMgr::m_showedDisconnect;

// STATIC: 0x00B6AFA4
static CVar* s_curGameTip;

// STATIC: 0x00B6AFA8
static CVar* s_showGameTips;

// STATIC: 0x00B6AFAC
CGlueMgr::Agreements CGlueMgr::m_latestAgreements;

// STATIC: 0x00B6AFC0
TSGrowableArray<const char*> CGlueMgr::m_changedOptionWarnings;

// STATIC: 0x00B6AFD0
static SCritSect s_connectionStatusLock;

// STATIC: 0x00DD2010
static thread_local WOWGUID s_activeCharacterGuid;

// VERIFY: 81edbfd4 auto
// OFFSET: 0x004D7CD0
static void WriteConnectionLog(const char* text) {
    if (!s_connectionLog) {
        OsCreateDirectory("Logs", 0);
        SLogCreate("Logs\\connection.log", 0, &s_connectionLog);
    }

    SLogWrite(s_connectionLog, text);
    SLogFlush(s_connectionLog);
}

// VERIFY: c565c343 auto
// OFFSET: 0x004D8360
void CGlueMgr::QuitGameAndRunLauncher() {
    ClientPostClose(0);
    SCreateProcess("Launcher.exe", nullptr, nullptr, nullptr);
}

// VERIFY: f5c0bd29 auto
// OFFSET: 0x004D8380
void CGlueMgr::DownloadFeed(const uint8_t* data, uint32_t size) {
    if (CGlueMgr::m_patchDownload) {
        CGlueMgr::PatchDownloadFeed(data, size);
    } else if (CGlueMgr::m_surveyDownload) {
        CGlueMgr::SurveyDownloadFeed(data, size);
    }
}

// VERIFY: fa4475b8 auto
// OFFSET: 0x004D83B0
static int32_t CheckTrialAccountConversion() {
    CVar* converted = CVar::Lookup("converted");

    if (ClientServices::LoginConnection()->IsTrialAccount() || converted->m_intValue) {
        return 0;
    }

    char path[260];
    SStrPrintf(path, sizeof(path), "WTF/Account/%s", CGlueMgr::m_accountName);

    if (OsDirectoryExists(path)) {
        converted->Set("1", true, false, false, true);
        CVar::SaveAll();
        FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
        CGlueMgr::SetScreen("trialconvert");
        return 1;
    }

    FrameScript_SignalEvent(GLUE_EVENT_CLIENT_TRIAL, nullptr);
    CGlueMgr::m_disconnectPending = 1;
    ClientServices::Connection()->Disconnect();
    ClientServices::LoginConnection()->Logoff();

    return 1;
}

// VERIFY: e5e10296 auto
// OFFSET: 0x004D8A30
void CGlueMgr::LoginServerLogin(const char* username, char* password) {
    if (!CGlueMgr::m_scandllOkayToLogIn || !CGlueMgr::m_acceptedTOS || !CGlueMgr::m_acceptedEULA || CGlueMgr::m_idleState != IDLE_NONE) {
        return;
    }

    if (!username || !*username) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText("LOGIN_ENTER_NAME", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    if (!password || !*password) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText("LOGIN_ENTER_PASSWORD", -1, GENDER_NOT_APPLICABLE));
        return;
    }

    CGlueMgr::m_loginResult = -1;
    CGlueMgr::m_loginState = -1;
    CGlueMgr::m_lastLoginResult = -1;
    CGlueMgr::m_lastLoginState = -1;
    CGlueMgr::m_authenticated = 0;
    CGlueMgr::m_matrixRemaining = 0;
    CGlueMgr::m_idleState = IDLE_LOGIN_SERVER_LOGIN;
    CGlueMgr::m_showedDisconnect = 0;

    const char* src = username;
    char* dst = CGlueMgr::m_accountName;

    while (*src) {
        if (dst >= &CGlueMgr::m_accountName[sizeof(CGlueMgr::m_accountName) - 1]) {
            break;
        }

        if (*src == '|' && src[1] == '|') {
            src++;
        }

        *dst++ = *src++;
    }

    *dst = '\0';
    SStrUpper(CGlueMgr::m_accountName);

    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "CANCEL");
    ClientServices::Logon(CGlueMgr::m_accountName, password);
    memset(password, 0, strlen(password));
}

// VERIFY: 6c64b192 auto
// OFFSET: 0x004D8BA0
static void StartAccountLogin() {
    if (CGlueMgr::m_idleState != IDLE_NONE) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_ACCOUNT_LOGIN;
    CGlueMgr::m_showedDisconnect = 0;
    ClientServices::Connection()->Connect();
}

// VERIFY: 5586fc7c auto
// OFFSET: 0x004D9660
static bool CreateIntroScene(uint8_t classID, uint8_t raceID, C3Vector& position) {
    int32_t sequenceID;
    ChrClassesRec* classRec = g_chrClassesDB.GetRecord(classID);

    if (!classRec || !(sequenceID = classRec->m_cinematicSequenceID)) {
        ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(raceID);

        if (!raceRec || !(sequenceID = raceRec->m_cinematicSequenceID)) {
            return false;
        }
    }

    CinematicSequencesRec* sequenceRec = g_cinematicSequencesDB.GetRecord(sequenceID);

    if (!sequenceRec) {
        return false;
    }

    CinematicCameraRec* cameraRec = g_cinematicCameraDB.GetRecord(sequenceRec->m_camera[0]);

    if (!cameraRec) {
        return false;
    }

    CM2Scene* scene = M2CreateScene();
    CM2Model* model = scene->CreateModel(cameraRec->m_model, 0);

    if (model) {
        if (model->IsLoaded(1, 0)) {
            C44Matrix transform;
            transform.d0 = cameraRec->m_origin[0];
            transform.d1 = cameraRec->m_origin[1];
            transform.d2 = cameraRec->m_origin[2];
            transform.RotateAroundZ(cameraRec->m_originFacing);
            model->SetWorldTransform(transform);

            scene->SetTime(0);
            model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 0, 1);
            model->SetAnimating(1);

            C3Vector cameraPos;
            scene->Animate(cameraPos);

            void* camera = model->GetCameraByIndex(0);

            if (camera) {
                DataMgrGetCoord(static_cast<CDataMgr*>(camera), 7, &position);
                model->Release();
                scene->Release();
                return true;
            }
        }

        model->Release();
    }

    scene->Release();

    return false;
}

// VERIFY: 9fef272c auto
// OFFSET: 0x004DA090
int32_t CGlueMgr::OnCharRenameResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t result;
    WOWGUID guid = { 0 };
    char name[48];

    msg->Get(result);

    if (!msg->IsRead()) {
        msg->Get(guid.m_guid);
        msg->GetString(name, sizeof(name));
    }

    FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);

    if (result) {
        if (result == 50) {
            FrameScript_SignalEvent(GLUE_EVENT_FORCE_RENAME_CHARACTER, "%s", ClientServices::GetErrorToken(50));
        } else {
            FrameScript_SignalEvent(GLUE_EVENT_FORCE_RENAME_CHARACTER, "%s", FrameScript_GetText("CHAR_RENAME_FAILED", -1, GENDER_NOT_APPLICABLE));
        }

        return 1;
    }

    CCharacterSelection::UpdateCharacterName(guid, name);

    if (CCharacterSelection::s_selectionIndex >= 0 && CCharacterSelection::s_selectionIndex < static_cast<int32_t>(CCharacterSelection::s_characterList.m_count)) {
        CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList.m_data[CCharacterSelection::s_selectionIndex];

        if (display && display->m_characterInfo.m_guid == guid) {
            CGlueMgr::EnterWorld();
        }
    }

    return 1;
}

// VERIFY: 60c7823f auto
// OFFSET: 0x004DA190
int32_t CGlueMgr::OnCharDeclineResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid = { 0 };
    uint32_t result;

    FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
    msg->Get(result);

    if (result) {
        FrameScript_SignalEvent(GLUE_EVENT_FORCE_DECLINE_CHARACTER, "%s", "CHAR_DECLINE_FAILED");
        return 1;
    }

    msg->Get(guid.m_guid);
    CCharacterSelection::UpdateCharacterNameDeclension(guid);

    if (CCharacterSelection::s_selectionIndex >= 0 && CCharacterSelection::s_selectionIndex < static_cast<int32_t>(CCharacterSelection::s_characterList.m_count)) {
        CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList.m_data[CCharacterSelection::s_selectionIndex];

        if (display && display->m_characterInfo.m_guid == guid) {
            CGlueMgr::EnterWorld();
        }
    }

    return 1;
}

// VERIFY: a6b0e177 manual
// OFFSET: 0x004DA240
void CGlueMgr::AcceptChangedOptionWarnings() {
    CGlueMgr::m_changedOptionWarnings.Clear();
    CGlueMgr::m_numChangedOptionWarnings = 0;
}

// VERIFY: dccf23d8 manual
// OFFSET: 0x004DA360
static void RequestLatestAgreements() {
    CGlueMgr::m_latestAgreements.m_data.Clear();
    CGlueMgr::m_latestAgreements.m_complete = 0;

    if (!OsURLDownload(FrameScript_GetText("LATEST_AGREEMENTS_URL", -1, GENDER_NOT_APPLICABLE), CGlueMgr::AgreementsURLCallback, &CGlueMgr::m_latestAgreements)) {
        CGlueMgr::m_latestAgreements.m_complete = 1;
    }
}

// VERIFY: f4782413 auto
// OFFSET: 0x004D7880
bool CGlueMgr::AreScriptFunctionsLoaded() {
    return CGlueMgr::s_scriptFunctionsLoaded;
}

// VERIFY: 60565de3 auto
// OFFSET: 0x004D7890
bool CGlueMgr::ServerAlertURLCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    if (size) {
        char buffer[2048];
        SStrCopy(buffer, data, size >= sizeof(buffer) ? sizeof(buffer) : size);
        SStrPack(CGlueMgr::m_serverAlert, buffer, sizeof(CGlueMgr::m_serverAlert));
    }

    if (done) {
        if (CGlueMgr::m_serverAlert[0]) {
            if (!SStrCmpI(CGlueMgr::m_serverAlert, s_serverAlertTag, SStrLen(s_serverAlertTag)) || !SStrCmpI(CGlueMgr::m_serverAlert, s_serverAlertTagBOM, SStrLen(s_serverAlertTagBOM))) {
                CGlueMgr::m_processServerAlert = 1;
            }
        }

        CGlueMgr::m_pendingServerAlert = 0;
    }

    return true;
}

// VERIFY: 1ea10880 manual
// OFFSET: 0x004D7D20
void CGlueMgr::InitializeFFX() {
    if (CGlueMgr::m_ffxActive) {
        return;
    }

    CGlueMgr::m_ffxActive = 1;
    FFX::Init();
    CGlueMgr::m_glowEffect = new (SMemAlloc(sizeof(EffectGlow), __FILE__, __LINE__, 0)) EffectGlow();
    CGlueMgr::m_deathEffect = new (SMemAlloc(sizeof(EffectDeath), __FILE__, __LINE__, 0)) EffectDeath();
}

// VERIFY: 68028b82 auto
// OFFSET: 0x004D8650
void CGlueMgr::ProcessAgreements(CGlueMgr::Agreements* agreements) {
    if (!agreements) {
        return;
    }

    bool localized = CheckDataFilesExist();

    char path[260];
    SStrCopy(path, "data", STORM_MAX_STR);
    SStrPack(path, "\\", STORM_MAX_STR);

    if (localized) {
        SStrPack(path, s_localeCode, STORM_MAX_STR);
        SStrPack(path, "\\", STORM_MAX_STR);
    }

    SStrPack(path, "agreements.mpq", STORM_MAX_STR);

    HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        return;
    }

    uint32_t written = 0;

    if (!OsWriteFile(file, agreements->m_data.m_data, agreements->m_data.m_count, &written) || agreements->m_data.m_count != written) {
        OsCloseFile(file);
        return;
    }

    OsCloseFile(file);

    SArchive* archive;

    if (!SFile::OpenArchive(path, 0, 0x800, &archive)) {
        OsDeleteFile(path);
        return;
    }

    SFileImpl* agreement;

    if (SFile::OpenEx(archive, "tos.html", 0, &agreement)) {
        ProcessAgreement(agreement, "tos.html", g_readTOSVar, localized);
        SFileCloseFile(agreement);
    }

    if (SFile::OpenEx(archive, "eula.html", 0, &agreement)) {
        ProcessAgreement(agreement, "eula.html", g_readEULAVar, localized);
        SFileCloseFile(agreement);
    }

    if (SFile::OpenEx(archive, "termination.html", 0, &agreement)) {
        ProcessAgreement(agreement, "termination.html", g_readTerminationWithoutNoticeVar, localized);
        SFileCloseFile(agreement);
    }

    SFile::CloseArchive(archive);
    OsDeleteFile(path);
}

// VERIFY: fbc4fd55 auto
// OFFSET: 0x004D8860
void CGlueMgr::HandleDisplaySizeChanged(const CSizeEvent& event) {
    if (CGlueMgr::m_screenWidth > 0 && CGlueMgr::m_screenWidth == event.m_data.m_w && CGlueMgr::m_screenHeight > 0 && CGlueMgr::m_screenHeight == event.m_data.m_h) {
        return;
    }

    CGlueMgr::m_screenWidth = event.m_data.m_w;
    CGlueMgr::m_screenHeight = event.m_data.m_h;

    float aspect = Screen::GetAspectRatio();
    CSimpleFrame* glueParent = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("GlueParent", CSimpleFrame::GetObjectType()));

    if (glueParent) {
        glueParent->SetFrameScale(1.0f, true);
        CLayoutFrame::ResizePending();
    }

    if (fabs(CGlueMgr::m_aspect - aspect) < 0.01f) {
        return;
    }

    CGlueMgr::m_aspect = aspect;
    CGlueMgr::m_reload = 1;
}

// VERIFY: e486096e manual
// OFFSET: 0x004DA5F0
void CGlueMgr::Resume() {
    CGlueMgr::m_disconnectPending = 0;
    CGlueMgr::m_reconnect = 0;
    CGlueMgr::m_idleState = IDLE_NONE;
    CGlueMgr::m_showedDisconnect = 0;
    CGlueMgr::m_characterInfo = nullptr;
    CGlueMgr::m_suspended = 0;
    CGlueMgr::m_reload = 0;

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSizeInScreenCoords(rect);
    CGlueMgr::m_screenWidth = static_cast<int32_t>(rect.maxX - rect.minX);
    CGlueMgr::m_screenHeight = static_cast<int32_t>(rect.maxY - rect.minY);
    CGlueMgr::m_aspect = Screen::GetAspectRatio();
    CoordinateSetAspectRatio(CGlueMgr::m_aspect);

    CGlueMgr::m_simpleTop = new (SMemAlloc(sizeof(CSimpleTop), __FILE__, __LINE__, 0)) CSimpleTop();
    CGlueMgr::m_simpleTop->m_displaySizeCallback = CGlueMgr::HandleDisplaySizeChanged;

    uint32_t width = 0;
    uint32_t height = 0;
    PIXEL_FORMAT format = PIXEL_ARGB8888;
    CGlueMgr::m_cursorMipBits = TextureLoadImage("Interface\\Cursor\\Point.blp", &width, &height, &format, nullptr, nullptr, nullptr, 0);
    CGlueMgr::m_simpleTop->SetCursor(CGlueMgr::m_cursorMipBits);
    CGlueMgr::m_simpleTop->SetCursor(CGlueMgr::m_cursorMipBits);

    CGInputControl* control = CGInputControl::GetActive();
    control->m_mouseModeFlags = 3;
    control->m_forceCursorOn = 0;
    control->UpdateMouseMode(1);

    FrameScript_Flush();
    SystemRegisterFunctions();
    RegisterSimpleFrameScriptMethods();
    GlueScriptEventsRegisterFunctions();
    CharSelectRegisterScriptFunctions();
    CharacterCreateRegisterScriptFunctions();
    RealmListRegisterScriptFunctions();
    SI2::RegisterScriptFunctions();
    AccountMsg_RegisterScriptFunctions();
    CGVideoOptions::RegisterScriptFunctions();
    CGlueMgr::s_scriptFunctionsLoaded = 1;
    FrameScript_CreateEvents(g_glueScriptEvents, sizeof(g_glueScriptEvents) / sizeof(g_glueScriptEvents[0]));

    CCharacterSelection::Initialize();
    CCharacterCreation::Initialize();

    OsCreateDirectory("Logs", 0);
    CWOWClientStatus status("Logs\\GlueXML.log");

    DeleteInterfaceFiles();

    MD5_CTX md5;
    MD5Init(&md5);

    uint8_t expectedDigest[16];

    switch (FrameXML_CheckSignature("Interface\\GlueXML\\GlueXML.toc", nullptr, InterfaceKey, expectedDigest)) {
        case 0: {
            status.Add(STATUS_WARNING, "GlueXML missing signature");
            break;
        }

        case 1: {
            status.Add(STATUS_WARNING, "GlueXML has corrupt signature");
            break;
        }

        case 2: {
            status.Add(STATUS_WARNING, "GlueXML is modified or corrupt");
            break;
        }

        case 3: {
            FrameXML_FreeHashNodes();
            FrameXML_CreateFrames("Interface\\GlueXML\\GlueXML.toc", nullptr, &md5, &status);

            uint8_t digest[16];
            MD5Final(digest, &md5);

#ifndef DISABLE_SIGNATURE_CHECKS
            if (memcmp(expectedDigest, digest, sizeof(digest))) {
                break;
            }
#endif

            FrameScript_SignalEvent(GLUE_EVENT_FRAMES_LOADED, nullptr);
            CGlueMgr::InitializeFFX();

            ClientServices::SetMessageHandler(SMSG_CHARACTER_RENAME_RESULT, CGlueMgr::OnCharRenameResult, nullptr);
            ClientServices::SetMessageHandler(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT, CGlueMgr::OnCharDeclineResult, nullptr);
            ClientServices::SetMessageHandler(SMSG_CHAR_CUSTOMIZE, CGlueMgr::OnCharCustomizeResult, nullptr);
            ClientServices::SetMessageHandler(SMSG_REALM_SPLIT, CGlueMgr::OnRealmSplitMsg, nullptr);
            ClientServices::SetMessageHandler(SMSG_KICK_REASON, CGlueMgr::OnKickReasonMsg, nullptr);
            ClientServices::SetMessageHandler(SMSG_CHAR_FACTION_CHANGE_RESULT, CGlueMgr::OnCharFactionChangeResult, nullptr);

            CGlueMgr::m_pendingServerAlert = 1;
            CGlueMgr::m_processServerAlert = 0;
            CGlueMgr::m_serverAlert[0] = '\0';

            if (!OsURLDownload(FrameScript_GetText(ClientServices::GetServerAlertURL(), -1, GENDER_NOT_APPLICABLE), CGlueMgr::ServerAlertURLCallback, nullptr)) {
                CGlueMgr::m_pendingServerAlert = 0;
            }

            return;
        }

        default: {
            break;
        }
    }

    ClientPostClose(9);
}

// VERIFY: fc9c4123 auto
// OFFSET: 0x004D8200
int32_t CGlueMgr::OnRealmSplitMsg(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t realmId = 0;
    uint32_t state = 0;
    char date[64];
    date[0] = '\0';

    if (!msg->IsRead()) {
        msg->Get(realmId);
        msg->Get(state);
        msg->GetString(date, sizeof(date));
    }

    FrameScript_SignalEvent(GLUE_EVENT_SERVER_SPLIT_NOTICE, "%d%d%s", realmId, state, date);

    return 1;
}

// VERIFY: 4b2e7146 manual
// OFFSET: 0x004D8270
void CGlueMgr::RealmListDialogCancelled() {
    if (!SStrCmpI(CGlueMgr::m_currentScreen, "login", STORM_MAX_STR)) {
        ClientServices::LoginConnection()->Logoff();
    }
}

// VERIFY: 227c1fe1 auto
// OFFSET: 0x004D82A0
void CGlueMgr::SetScreen(const char* screen) {
    FrameScript_SignalEvent(GLUE_EVENT_SET_GLUE_SCREEN, "%s", screen);
}

// VERIFY: d3e9c694 manual
// OFFSET: 0x004D82C0
void CGlueMgr::UpdateCurrentScreen(const char* screen) {
    if (!SStrCmpI(CGlueMgr::m_currentScreen, "charselect", STORM_MAX_STR)) {
        CCharacterSelection::OnHide();
    } else if (!SStrCmpI(CGlueMgr::m_currentScreen, "charcreate", STORM_MAX_STR)) {
        CCharacterCreation::OnHide();
    }

    SStrCopy(CGlueMgr::m_currentScreen, screen, sizeof(CGlueMgr::m_currentScreen));

    if (!SStrCmpI(CGlueMgr::m_currentScreen, "charselect", STORM_MAX_STR)) {
        EventRegisterEx(EVENT_ID_IDLE, CCharacterSelection::Idle, nullptr, EVENT_PRIORITY_NORMAL);
    } else if (!SStrCmpI(CGlueMgr::m_currentScreen, "charcreate", STORM_MAX_STR)) {
        EventRegisterEx(EVENT_ID_IDLE, CCharacterCreation::Idle, nullptr, EVENT_PRIORITY_NORMAL);
    }
}

// VERIFY: 7d4f3c79 manual
// OFFSET: 0x004D8540
void CGlueMgr::HandleScreenshot(int32_t success) {
    if (success) {
        FrameScript_SignalEvent(GLUE_EVENT_GLUE_SCREENSHOT_SUCCEEDED, nullptr);
    } else {
        FrameScript_SignalEvent(GLUE_EVENT_GLUE_SCREENSHOT_FAILED, nullptr);
    }
}

// VERIFY: b2ebc87e manual
// OFFSET: 0x004D8570
int32_t CGlueMgr::ShowChangedOptionWarnings() {
    return CGlueMgr::m_numChangedOptionWarnings != 0;
}

// VERIFY: 253dd4f3 auto
// OFFSET: 0x004D84A0
int32_t CGlueMgr::HandleBattlenetDisconnect() {
    if (CGlueMgr::m_loginState != LOGIN_STATE_FAILED || CGlueMgr::m_loginResult != LOGIN_DISCONNECTED || CGlueMgr::m_showedDisconnect) {
        return 0;
    }

    if (!ClientServices::LoginConnection() || ClientServices::LoginConnection()->GetLoginServerType() != 1) {
        return 0;
    }

    CGlueMgr::m_disconnectPending = 1;
    ClientServices::Connection()->Disconnect();
    ClientServices::LoginConnection()->Logoff();
    FrameScript_SignalEvent(GLUE_EVENT_DISCONNECTED_FROM_SERVER, "%d", 0);
    CGlueMgr::m_showedDisconnect = 1;
    return 1;
}

// VERIFY: 0a9c147f auto
// OFFSET: 0x004D9F50
void CGlueMgr::AddChangedOptionWarning(const char* warning) {
    if (!warning) {
        return;
    }

    const char* text = FrameScript_GetText(warning, -1, GENDER_NOT_APPLICABLE);

    if (!text) {
        return;
    }

    if (CGlueMgr::m_changedOptionWarnings.Count() <= CGlueMgr::m_numChangedOptionWarnings) {
        CGlueMgr::m_changedOptionWarnings.SetCount(2 * CGlueMgr::m_changedOptionWarnings.Count());
    }

    CGlueMgr::m_changedOptionWarnings[CGlueMgr::m_numChangedOptionWarnings++] = text;
}

// VERIFY: 8c4362f4 auto
// OFFSET: 0x004DA040
bool CGlueMgr::AgreementsURLCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    CGlueMgr::Agreements* agreements = static_cast<CGlueMgr::Agreements*>(param);

    if (!agreements) {
        return true;
    }

    if (size) {
        agreements->m_data.Add(size, data);
    }

    if (done) {
        agreements->m_complete = 1;

        if (agreements->m_data.Count()) {
            CGlueMgr::ProcessAgreements(agreements);
            agreements->m_data.Clear();
        }
    }

    return true;
}

// VERIFY: 644e39b6 manual
// OFFSET: 0x004D7940
void ProcessAgreement(SFileImpl* file, const char* name, CVar* cvar, bool localized) {
    char path[264];
    SStrCopy(path, "data", STORM_MAX_STR);
    SStrPack(path, "\\", STORM_MAX_STR);

    if (localized) {
        SStrPack(path, s_localeCode, STORM_MAX_STR);
        SStrPack(path, "\\", STORM_MAX_STR);
    }

    SStrPack(path, name, STORM_MAX_STR);

    void* existing = nullptr;
    uint32_t existingSize = 0;

    if (OsFileExists(path)) {
        HOSFILE input = OsCreateFile(path, OS_GENERIC_READ, 3, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

        if (input != OS_INVALID_FILE) {
            uint32_t size = static_cast<uint32_t>(OsGetFileSize(input));
            existing = SMemAlloc(size, __FILE__, __LINE__, 0);

            if (existing) {
                OsReadFile(input, existing, size, &existingSize);
            }

            OsCloseFile(input);
        }
    }

    uint32_t size = SFileGetFileSize(file, nullptr);
    void* data = SMemAlloc(size, __FILE__, __LINE__, 0);

    if (!data) {
        if (existing) {
            SMemFree(existing, __FILE__, __LINE__, 0);
        }

        return;
    }

    uint32_t read = 0;

    if (!SFileReadFile(file, data, size, &read, nullptr, 0)) {
        SMemFree(data, __FILE__, __LINE__, 0);

        if (existing) {
            SMemFree(existing, __FILE__, __LINE__, 0);
        }

        return;
    }

    if (existingSize == read && !memcmp(existing, data, existingSize)) {
        SMemFree(data, __FILE__, __LINE__, 0);

        if (existing) {
            SMemFree(existing, __FILE__, __LINE__, 0);
        }

        return;
    }

    if (existing) {
        SMemFree(existing, __FILE__, __LINE__, 0);
    }

    if (OsFileExists(path) && !OsDeleteFile(path)) {
        SMemFree(data, __FILE__, __LINE__, 0);
        return;
    }

    HOSFILE output = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (output == OS_INVALID_FILE) {
        SMemFree(data, __FILE__, __LINE__, 0);
        return;
    }

    uint32_t written = 0;

    if (!OsWriteFile(output, data, read, &written) || read != written) {
        SMemFree(data, __FILE__, __LINE__, 0);
        OsCloseFile(output);
        return;
    }

    SMemFree(data, __FILE__, __LINE__, 0);
    OsCloseFile(output);

    if (cvar->m_intValue >= 1) {
        cvar->Set("-1", true, false, false, true);
    } else {
        cvar->Set("-2", true, false, false, true);
    }
}

// VERIFY: 42fb8ddc manual
// OFFSET: 0x004DB9F0
void CGlueMgr::Initialize() {
    CGlueMgr::m_initialized = 1;

    int32_t readTOS = g_readTOSVar->m_intValue;
    CGlueMgr::m_acceptedTOS = readTOS == 1;

    if (readTOS < 0) {
        CGlueMgr::m_rereadTOS = 1;
    }

    int32_t readEULA = g_readEULAVar->m_intValue;
    CGlueMgr::m_acceptedEULA = readEULA == 1;

    if (readEULA < 0) {
        CGlueMgr::m_rereadEULA = 1;
    }

    int32_t readTerminationWithoutNotice = g_readTerminationWithoutNoticeVar->m_intValue;
    CGlueMgr::m_acceptedTerminationWithoutNotice = readTerminationWithoutNotice == 1;

    if (readTerminationWithoutNotice < 0) {
        CGlueMgr::m_rereadTerminationWithoutNotice = 1;
    }

    int32_t readScanning = g_readScanningVar->m_intValue;
    CGlueMgr::m_acceptedScanning = readScanning == 1;

    if (readScanning < 0) {
        CGlueMgr::m_rereadScanning = 1;
    }

    int32_t readContest = g_readContestVar->m_intValue;
    CGlueMgr::m_acceptedContest = readContest == 1;

    if (readContest < 0) {
        CGlueMgr::m_rereadContest = 1;
    }

    CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSize(rect);

    CSizeEvent event;
    event.m_data.m_w = static_cast<int32_t>(rect.maxX - rect.minX);
    event.m_data.m_h = static_cast<int32_t>(rect.maxY - rect.minY);
    CGlueMgr::HandleDisplaySizeChanged(event);

    FrameXML_RegisterFactory("ModelFFX", CSimpleModelFFX::Create, false);
    CGlueMgr::Resume();
    CRealmList::Initialize();
    EventRegisterEx(EVENT_ID_IDLE, CGlueMgr::Idle, nullptr, EVENT_PRIORITY_NORMAL);
    CGLCD::Initialize();
    CKBPage::Initialize();
    CKBSystem::Initialize();

    CGlueMgr::m_changedOptionWarnings.Clear();
    CGlueMgr::m_numChangedOptionWarnings = 0;
    CGlueMgr::m_changedOptionWarnings.SetCount(16);

    RequestLatestAgreements();
    AccountDataInitializeBasicSystem();
}

// VERIFY: 5e7db3b8 manual
// OFFSET: 0x004DAB40
int32_t CGlueMgr::Idle(const void* data, void* param) {
    if (g_theGxDevicePtr->StereoEnabled()) {
        CGlueMgr::SetUIDepth(g_theGxDevicePtr->StereoGetConvergence());
    }

    if (ClientServices::LoginConnection()) {
        ClientServices::LoginConnection()->Update();
    }

    if (CGlueMgr::m_processServerAlert) {
        FrameScript_SignalEvent(GLUE_EVENT_SHOW_SERVER_ALERT, "%s", &CGlueMgr::m_serverAlert[SStrLen(s_serverAlertTag) + (CGlueMgr::m_serverAlert[0] == '\xEF' ? 3 : 0)]);
        CGlueMgr::m_processServerAlert = 0;
    }

    if (CGlueMgr::m_pendingTimerAlert) {
        FrameScript_SignalEvent(GLUE_EVENT_TIMER_ALERT, "%d", CGlueMgr::m_pendingTimerAlert);
        CGlueMgr::m_pendingTimerAlert = 0;
    }

    CKBPage::UpdateLoadingQueue();

    if (CGlueMgr::m_idleState == IDLE_NONE) {
        if (CGlueMgr::m_reload) {
            if (!CGlueMgr::m_suspended) {
                CGlueMgr::Suspend();
                CGlueMgr::Resume();
                RequestLatestAgreements();
                CGlueMgr::SetScreen(CGlueMgr::m_currentScreen);
            }

            CGlueMgr::m_reload = 0;
        }

        if (CGlueMgr::m_accountMsgAvailable) {
            FrameScript_SignalEvent(GLUE_EVENT_ACCOUNT_MESSAGES_AVAILABLE, nullptr);
            CGlueMgr::m_accountMsgAvailable = 0;
        }

        CGlueMgr::HandleBattlenetDisconnect();

        return 1;
    }

    s_connectionStatusLock.Enter();

    if (s_connectionStatus.Head()) {
        for (ConsoleString* string = s_connectionStatus.Head(); string; string = s_connectionStatus.Next(string)) {
            ConsoleWrite(string->m_text, DEFAULT_COLOR);
        }

        s_connectionStatus.Clear();
    }

    s_connectionStatusLock.Leave();

    const char* statusText = nullptr;
    ClientConnection* connection = ClientServices::Connection();
    WOWCS_OPS op;
    int32_t result;
    int32_t errorCode;
    int32_t complete = connection->PollStatus(op, &statusText, result, errorCode);

    if (CGlueMgr::m_idleState == IDLE_LOGIN_SERVER_LOGIN) {
        if (CGlueMgr::m_loginState != LOGIN_STATE_PIN_WAIT) {
            CGlueMgr::DisplayLoginStatus();
        }

        if (CGlueMgr::m_authenticated) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            StartAccountLogin();
            CGlueMgr::m_authenticated = 0;
            return 1;
        }

        switch (CGlueMgr::m_loginState) {
            case LOGIN_STATE_FAILED: {
                ClientServices::LoginConnection()->Logoff();
                CGlueMgr::m_idleState = IDLE_NONE;
                CGlueMgr::m_showedDisconnect = 0;
                return 1;
            }

            case LOGIN_STATE_DOWNLOADFILE: {
                const char* fileName = ClientServices::LoginConnection()->m_fileName;

                if (!SStrCmpI(fileName, "Patch", STORM_MAX_STR)) {
                    CGlueMgr::PatchDownloadStart();
                    return 1;
                }

                if (!SStrCmpI(fileName, "Survey", STORM_MAX_STR)) {
                    CGlueMgr::SurveyDownloadStart();
                }

                return 1;
            }

            case LOGIN_STATE_PIN: {
                FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
                const uint8_t* keyMap = ClientServices::LoginConnection()->GetKeyMap();
                FrameScript_SignalEvent(GLUE_EVENT_PLAYER_ENTER_PIN, "%d%d%d%d%d%d%d%d%d%d", keyMap[0], keyMap[1], keyMap[2], keyMap[3], keyMap[4], keyMap[5], keyMap[6], keyMap[7], keyMap[8], keyMap[9]);
                CGlueMgr::m_loginState = LOGIN_STATE_PIN_WAIT;
                return 1;
            }

            case LOGIN_STATE_MATRIX: {
                uint32_t width;
                uint32_t height;
                uint32_t minDigits;
                uint32_t maxDigits;
                bool flipCoords;
                uint32_t challengeCount;
                ClientServices::LoginConnection()->GetMatrixInfo(&width, &height, &minDigits, &maxDigits, &flipCoords, &challengeCount);

                CGlueMgr::m_matrixChallengeCount = challengeCount;
                CGlueMgr::m_matrixRemaining = challengeCount;
                CGlueMgr::m_loginState = LOGIN_STATE_MATRIX_WAIT;

                FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
                FrameScript_SignalEvent(GLUE_EVENT_PLAYER_ENTER_MATRIX, "%d%d%d%d%b%d", width, height, minDigits, maxDigits, flipCoords, challengeCount);
                return 1;
            }

            case LOGIN_STATE_TOKEN: {
                CGlueMgr::m_loginState = LOGIN_STATE_TOKEN_WAIT;
                FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
                FrameScript_SignalEvent(GLUE_EVENT_PLAYER_ENTER_TOKEN, nullptr);
                return 1;
            }

            case LOGIN_STATE_CHECKINGVERSIONS: {
                uint8_t versionChecksum[20];
                ChecksumExecutables(ClientServices::LoginConnection()->GetVersionChallenge(), 16, versionChecksum);
                ClientServices::LoginConnection()->ProveVersion(versionChecksum);
                return 1;
            }

            default: {
                return 1;
            }
        }
    }

    GLUE_IDLE_STATE idleState = CGlueMgr::m_idleState;

    if (idleState == IDLE_12) {
        if (CGlueMgr::m_patchDownload) {
            CGlueMgr::PatchDownloadIdle();
        } else {
            if (!CGlueMgr::m_surveyDownload) {
                return 1;
            }

            CGlueMgr::SurveyDownloadIdle();
        }

        idleState = CGlueMgr::m_idleState;
    }

    if (idleState == IDLE_13) {
        if (CGlueMgr::m_surveyDownload && CGlueMgr::m_surveyDownload->GetStatus() == 2) {
            if (CGlueMgr::m_executedSurvey) {
                if (OsGetAsyncTimeMs() - CGlueMgr::m_surveyTimer > 1500) {
                    CGlueMgr::SurveyDownloadEnd();
                }
            } else {
                CGlueMgr::m_executedSurvey = 1;

                if (CGlueMgr::SurveyExecute()) {
                    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("LOGIN_STATE_SURVEY", -1, GENDER_NOT_APPLICABLE));
                    CGlueMgr::SurveySendResults();
                    CGlueMgr::m_surveyTimer = OsGetAsyncTimeMs();
                } else {
                    CGlueMgr::SurveyDownloadEnd();
                }
            }
        } else {
            CGlueMgr::SurveyDownloadEnd();
        }

        idleState = CGlueMgr::m_idleState;
    }

    if (idleState == IDLE_ACCOUNT_LOGIN) {
        if (ClientServices::LoginConnection()->GetLoginServerType() == 1 && CGlueMgr::m_loginState == LOGIN_STATE_FAILED) {
            CGlueMgr::DisplayLoginStatus();
            ClientServices::Connection()->Cancel(2);
            ClientServices::LoginConnection()->Logoff();
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            return 1;
        }

        if (errorCode == AUTH_WAIT_QUEUE) {
            const char* realmName = ClientServices::GetSelectedRealmName();
            bool hasRealmName = realmName && *realmName;

            if (connection->m_freeCharacterMigration) {
                CGlueMgr::m_displayingQueueDialog = 1;
                FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "QUEUED_WITH_FCM");
            }

            if (CGlueMgr::m_estimatedWaitTime > 0) {
                char timeLeft[256];
                int32_t remaining;

                if (CGlueMgr::m_estimatedWaitTime + CGlueMgr::m_queueTime[0] - OsGetAsyncTimeMs() == 0 || (remaining = CGlueMgr::m_estimatedWaitTime + CGlueMgr::m_queueTime[0] - OsGetAsyncTimeMs()) < 60000) {
                    if (hasRealmName) {
                        SStrPrintf(timeLeft, sizeof(timeLeft), FrameScript_GetText("QUEUE_NAME_TIME_LEFT_SECONDS", -1, GENDER_NOT_APPLICABLE), realmName, CGlueMgr::m_queuePosition[0]);
                    } else {
                        SStrPrintf(timeLeft, sizeof(timeLeft), FrameScript_GetText("QUEUE_TIME_LEFT_SECONDS", -1, GENDER_NOT_APPLICABLE), CGlueMgr::m_queuePosition[0]);
                    }
                } else {
                    int32_t minutes = remaining / 60000;

                    if (hasRealmName) {
                        SStrPrintf(timeLeft, sizeof(timeLeft), FrameScript_GetText("QUEUE_NAME_TIME_LEFT", -1, GENDER_NOT_APPLICABLE), realmName, CGlueMgr::m_queuePosition[0], minutes);
                    } else {
                        SStrPrintf(timeLeft, sizeof(timeLeft), FrameScript_GetText("QUEUE_TIME_LEFT", -1, GENDER_NOT_APPLICABLE), CGlueMgr::m_queuePosition[0], minutes);
                    }
                }

                char queueText[512];
                SStrPrintf(queueText, sizeof(queueText), "%s\n%s", statusText, timeLeft);

                if (connection->m_freeCharacterMigration) {
                    SStrPack(queueText, "\n\n", sizeof(queueText));
                    SStrPack(queueText, FrameScript_GetText("QUEUE_FCM", -1, GENDER_NOT_APPLICABLE), sizeof(queueText));
                }

                FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s%s", queueText, FrameScript_GetText("CHANGE_REALM", -1, GENDER_NOT_APPLICABLE));
            } else {
                char queueText[512];

                if (hasRealmName) {
                    SStrPrintf(queueText, sizeof(queueText), FrameScript_GetText("QUEUE_NAME_TIME_LEFT_UNKNOWN", -1, GENDER_NOT_APPLICABLE), realmName, CGlueMgr::m_queuePosition[0]);
                } else {
                    SStrPrintf(queueText, sizeof(queueText), FrameScript_GetText("QUEUE_TIME_LEFT_UNKNOWN", -1, GENDER_NOT_APPLICABLE), CGlueMgr::m_queuePosition[0]);
                }

                if (connection->m_freeCharacterMigration) {
                    SStrPack(queueText, "\n\n", sizeof(queueText));
                    SStrPack(queueText, FrameScript_GetText("QUEUE_FCM", -1, GENDER_NOT_APPLICABLE), sizeof(queueText));
                }

                FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s%s", queueText, FrameScript_GetText("CHANGE_REALM", -1, GENDER_NOT_APPLICABLE));
            }
        } else {
            if (CGlueMgr::m_displayingQueueDialog) {
                FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "CANCEL");
                CGlueMgr::m_displayingQueueDialog = 0;
            }

            FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s", statusText);
        }

        if (!complete) {
            return 1;
        }

        if (!result) {
            if (errorCode != RESPONSE_CANCELLED) {
                uint32_t i;

                for (i = 0; i < sizeof(s_loginErrorURLs) / sizeof(s_loginErrorURLs[0]); i++) {
                    if (errorCode == s_loginErrorURLs[i].m_errorCode) {
                        break;
                    }
                }

                if (i < sizeof(s_loginErrorURLs) / sizeof(s_loginErrorURLs[0])) {
                    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s%s", "OKAY_WITH_URL", statusText, s_loginErrorURLs[i].m_url);
                } else {
                    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
                }
            }

            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;

            if (errorCode == RESPONSE_CANCELLED) {
                CGlueMgr::m_disconnectPending = 1;
                ClientServices::Connection()->Disconnect();
            }

            if (errorCode != AUTH_FAILED) {
                CCharacterSelection::ClearCharacterList();

                if (ClientServices::CreateLoginResponse()->m_realmList.m_count) {
                    FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
                    CRealmList::UpdateList();
                } else {
                    CGlueMgr::GetRealmList(1);
                }

                return 1;
            }

            if (!SStrCmpI(CGlueMgr::m_currentScreen, "charselect", STORM_MAX_STR)) {
                CGlueMgr::SetScreen("login");
            }

            return 1;
        }

        if (op == COP_CONNECT) {
            ClientServices::Connection()->AccountLogin(CGlueMgr::m_accountName, "", s_region, s_locale);
            return 1;
        }

        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
        CCharacterSelection::ClearCharacterList();
        CGlueMgr::m_queuePosition[0] = 0;
        CGLCD::ClearWaitQueueScreen();

        CVar* accountType = CVar::Lookup("accounttype");

        if (ClientServices::LoginConnection()->IsTrialAccount()) {
            if (SFile::IsTrial()) {
                accountType->Set("ST", true, false, false, true);
            } else {
                accountType->Set("RT", true, false, false, true);
            }
        } else {
            switch (ClientServices::Connection()->m_accountExpansion) {
                case 0: {
                    accountType->Set("CL", true, false, false, true);
                    break;
                }

                case 1: {
                    accountType->Set("BC", true, false, false, true);
                    break;
                }

                case 2: {
                    accountType->Set("LK", true, false, false, true);
                    break;
                }

                default: {
                    break;
                }
            }
        }

        if (SFile::IsTrial() && CheckTrialAccountConversion()) {
            return 1;
        }

        if (ClientServices::Connection()->m_accountExpansion > GetClientExpansionLevel()) {
            FrameScript_SignalEvent(GLUE_EVENT_CLIENT_ACCOUNT_MISMATCH, "%d%d", ClientServices::Connection()->m_accountExpansion, GetClientExpansionLevel());
            CGlueMgr::m_disconnectPending = 1;
            ClientServices::Connection()->Disconnect();
            ClientServices::LoginConnection()->Logoff();
            return 1;
        }

        Cfg_CategoriesRec* category = g_cfg_CategoriesDB.GetRecord(ClientServices::GetSelectedRealm()->m_category);

        if (category) {
            ValidateNameInitialize(category->m_localeMask, category->m_createCharsetMask);
        } else {
            ValidateNameInitialize(1 << s_locale, 0);
        }

        CGlueMgr::SetScreen("charselect");

        return 1;
    }

    if (idleState == IDLE_CHARACTER_LIST) {
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s", statusText);

        if (CGlueMgr::HandleBattlenetDisconnect()) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
        }

        if (!complete) {
            return 1;
        }

        if (result) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
            CCharacterSelection::UpdateCharacterList();

            if (CGlueMgr::m_accountMsgAvailable) {
                FrameScript_SignalEvent(GLUE_EVENT_ACCOUNT_MESSAGES_AVAILABLE, nullptr);
                CGlueMgr::m_accountMsgAvailable = 0;
            }

            return 1;
        }

        if (errorCode == RESPONSE_CANCELLED) {
            CCharacterSelection::ClearCharacterList();
            CGlueMgr::GetRealmList(1);
            return 1;
        }

        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;

        return 1;
    }

    if (idleState == IDLE_REALM_LIST) {
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s", statusText);

        if (CGlueMgr::HandleBattlenetDisconnect()) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
        }

        if (!complete) {
            return 1;
        }

        if (!result) {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            return 1;
        }

        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
        FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
        CRealmList::UpdateList();

        if (CGlueMgr::m_accountMsgAvailable) {
            FrameScript_SignalEvent(GLUE_EVENT_ACCOUNT_MESSAGES_AVAILABLE, nullptr);
            CGlueMgr::m_accountMsgAvailable = 0;
        }

        return 1;
    }

    if (idleState == IDLE_CREATE_CHARACTER) {
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s", statusText);

        if (CGlueMgr::HandleBattlenetDisconnect()) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
        }

        if (!complete) {
            return 1;
        }

        if (!result) {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            return 1;
        }

        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
        FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
        FrameScript_SignalEvent(GLUE_EVENT_SELECT_LAST_CHARACTER, nullptr);
        CGlueMgr::SetScreen("charselect");

        return 1;
    }

    if (idleState == IDLE_DELETE_CHARACTER) {
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_STATUS_DIALOG, "%s", statusText);

        if (CGlueMgr::HandleBattlenetDisconnect()) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
        }

        if (!complete) {
            return 1;
        }

        if (!result) {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            return 1;
        }

        FrameScript_SignalEvent(GLUE_EVENT_SELECT_FIRST_CHARACTER, nullptr);
        CGlueMgr::GetCharacterList();

        return 1;
    }

    if (idleState == IDLE_ENTER_WORLD) {
        if (!LoadingScreenDrawing()) {
            return 1;
        }

        if (CGlueMgr::m_suspended) {
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            SI2::StopGlueMusic(3.0f);
            SI2::StopGlueAmbience(-1.0f);
            ClientServices::Connection()->CharacterLogin(s_activeCharacterGuid, 0);
            return 1;
        }

        CHARACTER_INFO* characterInfo = CGlueMgr::m_characterInfo;
        uint32_t mapID = characterInfo->m_mapID;
        s_activeCharacterGuid = characterInfo->m_guid;
        C3Vector position = characterInfo->m_position;

        if (characterInfo->m_firstLogin) {
            CreateIntroScene(characterInfo->m_classID, characterInfo->m_raceID, position);
        }

        CGlueMgr::Suspend();
        ClientInitializeGame(mapID, position);

        return 1;
    }

    if (idleState == IDLE_WORLD_LOGIN) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", statusText);
        return 1;
    }

    if (idleState == IDLE_14 && (CGlueMgr::m_scandllStatus == 3 || CGlueMgr::m_scandllStatus == 2)) {
        CGlueMgr::ScanDLLFinished();
    }

    return 1;
}

// VERIFY: 030a5e4d auto
// OFFSET: 0x004D8BD0
void CGlueMgr::ChangeRealm(REALM_INFO* realm) {
    if (!realm) {
        return;
    }

    ClientServices::SelectRealm(realm->m_realmName);

    if (ClientServices::Connection()->m_connected) {
        CGlueMgr::m_disconnectPending = 1;
        CGlueMgr::m_reconnect = 1;
        ClientServices::Connection()->Disconnect();
    } else {
        CGlueMgr::m_idleState = IDLE_ACCOUNT_LOGIN;
        CGlueMgr::m_showedDisconnect = 0;
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("GAME_SERVER_LOGIN", -1, GENDER_NOT_APPLICABLE));
        ClientServices::Connection()->Connect();
    }
}

// VERIFY: 50af9eaf auto
// OFFSET: 0x004D8930
void CGlueMgr::Suspend() {
    CGlueMgr::m_suspended = 1;

    if (CGlueMgr::m_ffxActive) {
        CGlueMgr::m_ffxActive = 0;

        if (CGlueMgr::m_glowEffect) {
            delete CGlueMgr::m_glowEffect;
        }

        if (CGlueMgr::m_deathEffect) {
            delete CGlueMgr::m_deathEffect;
        }

        FFX::Destroy();
    }

    CCharacterSelection::Shutdown();
    CCharacterCreation::Shutdown();

    if (CGlueMgr::m_simpleTop) {
        delete CGlueMgr::m_simpleTop;
        CGlueMgr::m_simpleTop = nullptr;
    }

    if (CGlueMgr::m_cursorMipBits) {
        TextureFreeMippedImg(CGlueMgr::m_cursorMipBits, PIXEL_ARGB8888, 32, 32);
        CGlueMgr::m_cursorMipBits = nullptr;
    }

    SystemUnregisterFunctions();
    UnregisterSimpleFrameScriptMethods();
    GlueScriptEventsUnregisterFunctions();
    CharSelectUnregisterScriptFunctions();
    CharacterCreateUnregisterScriptFunctions();
    RealmListUnregisterScriptFunctions();
    SI2::UnregisterScriptFunctions();
    AccountMsg_UnregisterScriptFunctions();
    CGVideoOptions::UnregisterScriptFunctions();
    CGlueMgr::s_scriptFunctionsLoaded = 0;
    FrameXML_FreeHashNodes();
    ClientServices::ClearMessageHandler(SMSG_CHARACTER_RENAME_RESULT);
    ClientServices::ClearMessageHandler(SMSG_SET_PLAYER_DECLINED_NAMES_RESULT);
    ClientServices::ClearMessageHandler(SMSG_CHAR_CUSTOMIZE);
    ClientServices::ClearMessageHandler(SMSG_REALM_SPLIT);
    ClientServices::ClearMessageHandler(SMSG_KICK_REASON);
    ClientServices::ClearMessageHandler(SMSG_CHAR_FACTION_CHANGE_RESULT);
}

// VERIFY: 39c0975e manual
// OFFSET: 0x004DBBC0
void CGlueMgr::Shutdown() {
    while (CGlueMgr::m_pendingServerAlert || CGlueMgr::m_latestAgreements.m_data.Count()) {
        if (CGlueMgr::m_latestAgreements.m_data.Count()) {
            CGlueMgr::m_latestAgreements.m_data.Clear();
        }

        OsSleep(1);
    }

    AccountDataShutdownBasicSystem();
    CKBPage::Shutdown();
    CKBSystem::Shutdown();
    CRealmList::Shutdown();
    CGlueMgr::Suspend();
    CGlueMgr::m_initialized = 0;
    EventUnregister(EVENT_ID_IDLE, CGlueMgr::Idle);
    s_connectionStatus.Clear();
    CGLCD::Shutdown();

    if (s_connectionLog) {
        SLogClose(s_connectionLog);
    }

    ClientServices::ClearMessageHandler(CMSG_GM_CHARACTER_RESTORE);
}

// VERIFY: 5beb532c auto
// OFFSET: 0x004D7E40
void CGlueMgr::AcceptTOS() {
    int32_t value = g_readTOSVar->m_intValue;
    CGlueMgr::m_acceptedTOS = 1;
    CGlueMgr::m_rereadTOS = 0;
    if (value == -2) {
        g_readTOSVar->Set("-1", true, false, false, true);
    } else {
        g_readTOSVar->Set("1", true, false, false, true);
    }
}

// VERIFY: 2a47844a auto
// OFFSET: 0x004D7E80
void CGlueMgr::AcceptEULA() {
    int32_t value = g_readEULAVar->m_intValue;
    CGlueMgr::m_acceptedEULA = 1;
    CGlueMgr::m_rereadEULA = 0;
    if (value == -2) {
        g_readEULAVar->Set("-1", true, false, false, true);
    } else {
        g_readEULAVar->Set("1", true, false, false, true);
    }
}

// VERIFY: 22ea06ff auto
// OFFSET: 0x004D7EC0
void CGlueMgr::AcceptTerminationWithoutNotice() {
    int32_t value = g_readTerminationWithoutNoticeVar->m_intValue;
    CGlueMgr::m_acceptedTerminationWithoutNotice = 1;
    CGlueMgr::m_rereadTerminationWithoutNotice = 0;
    if (value == -2) {
        g_readTerminationWithoutNoticeVar->Set("-1", true, false, false, true);
    } else {
        g_readTerminationWithoutNoticeVar->Set("1", true, false, false, true);
    }
}

// VERIFY: fe1e0509 auto
// OFFSET: 0x004D7F00
void CGlueMgr::AcceptScanning() {
    CGlueMgr::m_acceptedScanning = 1;
    CGlueMgr::m_rereadScanning = 0;
    g_readScanningVar->Set("1", true, false, false, true);
}

// VERIFY: c831d965 auto
// OFFSET: 0x004D7F30
void CGlueMgr::AcceptContest() {
    CGlueMgr::m_acceptedContest = 1;
    CGlueMgr::m_rereadContest = 0;
    g_readContestVar->Set("1", true, false, false, true);
}

// VERIFY: 71a7a4ee auto
// OFFSET: 0x004DA3D0
void CGlueMgr::LogConnectionStatus(const char* format, ...) {
    if (!CGlueMgr::m_initialized || !format || !*format) {
        return;
    }

    s_connectionStatusLock.Enter();

    ConsoleString* string = s_connectionStatus.NewNode(2, 0, 0);

    va_list args;
    va_start(args, format);
    SStrVPrintf(string->m_text, sizeof(string->m_text), format, args);
    va_end(args);

    WriteConnectionLog(string->m_text);

    s_connectionStatusLock.Leave();
}

// VERIFY: 66eda7a5 auto
// OFFSET: 0x004DA4B0
void CGlueMgr::SetLoginStateAndResult(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, const char* stateString, const char* resultString, uint16_t flags) {
    CGlueMgr::LogConnectionStatus("GRUNT: state: %s result: %s %s", stateString, resultString, addressString ? addressString : "");

    if (flags & 0x1) {
        CGlueMgr::m_accountMsgAvailable = 1;
    }

    CGlueMgr::m_loginState = state;

    if (state == LOGIN_STATE_AUTHENTICATED) {
        CGlueMgr::m_authenticated = 1;
    }

    CGlueMgr::m_loginResult = result;
}

// VERIFY: dd34ad8d auto
// OFFSET: 0x004D7F60
void CGlueMgr::SetCurrentAccount(const char* accountName) {
    SStrCopy(CGlueMgr::m_accountName, accountName, sizeof(CGlueMgr::m_accountName));
    SStrUpper(CGlueMgr::m_accountName);
}

// VERIFY: 3a1bab91 auto
// OFFSET: 0x004D9590
void CancelRealmListQuery() {
    if (CGlueMgr::m_idleState == IDLE_REALM_LIST) {
        ClientServices::Connection()->Cancel(2);
        CGlueMgr::m_idleState = IDLE_NONE;
        CGlueMgr::m_showedDisconnect = 0;
    }
}

// VERIFY: 25496e10 auto
// OFFSET: 0x004D8C60
void CGlueMgr::CreateCharacter(const CHARACTER_CREATE_INFO* info) {
    if (!info) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_CREATE_CHARACTER;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText(ClientServices::GetErrorToken(CHAR_CREATE_IN_PROGRESS), -1, GENDER_NOT_APPLICABLE));
    ClientServices::Connection()->RequestCharacterCreate(*info);
}

// VERIFY: 8d26ef59 auto
// OFFSET: 0x004D8CC0
void CGlueMgr::DeleteCharacter(WOWGUID guid) {
    if (!guid) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_DELETE_CHARACTER;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText(ClientServices::GetErrorToken(CHAR_DELETE_IN_PROGRESS), -1, GENDER_NOT_APPLICABLE));
    ClientServices::Connection()->RequestCharacterDelete(guid);
}

// VERIFY: bc95747b manual
// OFFSET: 0x004D8D20
bool CGlueMgr::RenameCharacter(WOWGUID guid, const char* name) {
    if (!guid || !name || !*name) {
        return true;
    }

    int32_t result = ClientServices::CharacterValidateName(name);

    if (result != CHAR_NAME_SUCCESS) {
        FrameScript_SignalEvent(GLUE_EVENT_FORCE_RENAME_CHARACTER, "%s", ClientServices::GetErrorToken(result));
        return false;
    }

    CGlueMgr::m_idleState = IDLE_7;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("CHAR_RENAME_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHARACTER_RENAME_REQUEST));
    msg.Put(guid.m_guid);
    msg.PutString(name);
    msg.Finalize();
    ClientServices::Send2(&msg);

    return true;
}

// VERIFY: 7551aebf manual
// OFFSET: 0x004D9810
void CGlueMgr::SetUIDepth(float depth) {
    CSimpleFrame* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("GlueParent", CSimpleFrame::GetObjectType()));

    if (frame) {
        frame->SetFrameDepth(depth - frame->m_parentDepth, true);
    }

    frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("OptionsSelectFrame", CSimpleFrame::GetObjectType()));

    if (frame) {
        frame->SetFrameDepth(depth - frame->m_parentDepth, true);
    }
}

// VERIFY: f70ffd2d auto
// OFFSET: 0x004D98A0
CharacterSelectionDisplay* GetCharacterInfo(int32_t index) {
    if (index < 0 || index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        return nullptr;
    }

    return &CCharacterSelection::s_characterList[index];
}

// VERIFY: 86906951 manual
// OFFSET: 0x004D99E0
void CGlueMgr::CancelLogin() {
    if (CGlueMgr::m_idleState == IDLE_LOGIN_SERVER_LOGIN) {
        CGlueMgr::StatusDialogClick();
    }
}

// VERIFY: 2b1dadd9 manual
// OFFSET: 0x004D9A40
bool CGlueMgr::DeclineCharacter(WOWGUID guid, const char** names) {
    if (CCharacterSelection::s_selectionIndex < 0 || CCharacterSelection::s_selectionIndex >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        return true;
    }

    CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];

    if (!display || display->m_characterInfo.m_guid == guid) {
        return true;
    }

    for (int32_t i = 0; i < 5; i++) {
        if (!names[i] || !*names[i]) {
            return true;
        }

        int32_t result = ClientServices::CharacterValidateDeclension(display->m_characterInfo.m_name, names[i]);

        if (result != CHAR_NAME_SUCCESS) {
            FrameScript_SignalEvent(GLUE_EVENT_FORCE_DECLINE_CHARACTER, "%s", ClientServices::GetErrorToken(result));
            return false;
        }
    }

    CGlueMgr::m_idleState = IDLE_8;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("CHAR_DECLINE_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_SET_PLAYER_DECLINED_NAMES));
    msg.Put(guid.m_guid);
    msg.PutString(display->m_characterInfo.m_name);

    for (int32_t i = 0; i < 5; i++) {
        msg.PutString(names[i]);
    }

    msg.Finalize();
    ClientServices::Send2(&msg);

    return true;
}

// VERIFY: 117cc753 auto
// OFFSET: 0x004D9BD0
void CGlueMgr::EnterWorld() {
    if (!ClientServices::GetSelectedRealm()) {
        return;
    }

    CharacterSelectionDisplay* display = CCharacterSelection::s_selectionIndex < 0 || CCharacterSelection::s_selectionIndex >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count()) ? nullptr : &CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];
    CGlueMgr::m_characterInfo = &display->m_characterInfo;

    if (!display) {
        return;
    }

    CGlueMgr::s_displayedCharacterIndex = CCharacterSelection::s_selectionIndex;

    if (!ClientServices::Connection()->m_connected) {
        return;
    }

    uint32_t flags = CGlueMgr::m_characterInfo->m_flags;

    if (flags & 0x4) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText(ClientServices::GetErrorToken(CHAR_LOGIN_LOCKED_FOR_TRANSFER), -1, GENDER_NOT_APPLICABLE));
        return;
    }

    if (flags & 0x1000000) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText(ClientServices::GetErrorToken(CHAR_LOGIN_LOCKED_BY_BILLING), -1, GENDER_NOT_APPLICABLE));
        return;
    }

    if (flags & 0x4000) {
        FrameScript_SignalEvent(GLUE_EVENT_FORCE_RENAME_CHARACTER, "%s", "CHAR_RENAME_DESCRIPTION");
        return;
    }

    if (!(flags & 0x2000000) && NameNeedsDeclensions(s_locale, CGlueMgr::m_characterInfo->m_name)) {
        FrameScript_SignalEvent(GLUE_EVENT_FORCE_DECLINE_CHARACTER, nullptr);
        return;
    }

    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(CGlueMgr::m_characterInfo->m_raceID);
    ChrClassesRec* classRec;

    if (!raceRec || raceRec->m_requiredExpansion > ClientServices::Connection()->m_accountExpansion || !(classRec = g_chrClassesDB.GetRecord(CGlueMgr::m_characterInfo->m_classID)) || classRec->m_requiredExpansion > ClientServices::Connection()->m_accountExpansion) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", FrameScript_GetText(ClientServices::GetErrorToken(CHAR_LOGIN_DISABLED), -1, GENDER_NOT_APPLICABLE));
        return;
    }

    char indexString[32];
    SStrPrintf(indexString, sizeof(indexString), "%d", CCharacterSelection::s_selectionIndex);
    g_lastCharacterIndexVar->Set(indexString, true, false, false, true);

    ClientServices::SetAccountName(CGlueMgr::m_accountName);
    ClientServices::SetCharacterInfo(CGlueMgr::m_characterInfo);

    if (!s_curGameTip) {
        s_curGameTip = CVar::Register("gameTip", nullptr, 0, "0", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    }

    if (!s_showGameTips) {
        s_showGameTips = CVar::Register("showGameTips", nullptr, 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    }

    if (!CGlueMgr::m_characterInfo->m_firstLogin && s_showGameTips->m_intValue) {
        int32_t tip = s_curGameTip->m_intValue;

        if (tip < 0 || tip >= g_gameTipsDB.m_numRecords) {
            tip = 0;
        }

        if (g_gameTipsDB.m_numRecords > 0) {
            GameTipsRec* tipRec = tip >= 0 && tip < g_gameTipsDB.m_numRecords ? &g_gameTipsDB.m_records[tip] : nullptr;
            LoadingScreenSetTip(tipRec->m_text);

            char tipString[40];
            SStrPrintf(tipString, sizeof(tipString), "%d", tip + 1);
            s_curGameTip->Set(tipString, true, false, false, true);
        }
    }

    if (SFile::IsTrial()) {
        bool ready = StreamingIsRaceDataReady(CGlueMgr::m_characterInfo->m_raceID);
        StreamingSetRace(CGlueMgr::m_characterInfo->m_raceID);
        StreamingSetState(2);
        LoadingScreenSetScreenTipForRaceData(!ready);
    }

    LoadingScreenEnable(CGlueMgr::m_characterInfo->m_mapID, true);
    CVar::SaveAll();
    TextureGxCacheFlush();

    if (!ClientServices::LoginConnection()->GetLoginServerType()) {
        ClientServices::LoginConnection()->Logoff();
    }

    CGlueMgr::m_idleState = IDLE_ENTER_WORLD;
    CGlueMgr::m_showedDisconnect = 0;
}

// VERIFY: 0e9fb28c auto
// OFFSET: 0x004D95C0
void CGlueMgr::GetCharacterList() {
    if (CGlueMgr::m_idleState == IDLE_WORLD_LOGIN) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_CHARACTER_LIST;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("CHAR_LIST_RETRIEVING", -1, GENDER_NOT_APPLICABLE));
    ClientServices::Connection()->GetCharacterList();
}

// VERIFY: 616c4f30 auto
// OFFSET: 0x004D9610
void CGlueMgr::GetRealmList(int32_t showDialog) {
    CGlueMgr::m_idleState = IDLE_REALM_LIST;
    CGlueMgr::m_showedDisconnect = 0;

    if (showDialog) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("REALM_LIST_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));
    }

    ClientServices::GetRealmList();
}

// VERIFY: 0a5da6c1 auto
// OFFSET: 0x006B0130
void CGlueMgr::ResetQueueState() {
    memset(CGlueMgr::m_queuePosition, 0, sizeof(CGlueMgr::m_queuePosition));
    memset(CGlueMgr::m_queueTime, 0, sizeof(CGlueMgr::m_queueTime));
    CGlueMgr::m_estimatedWaitTime = 0;
}

// VERIFY: e6792714 manual
// OFFSET: 0x004D80A0
WOWGUID CGlueMgr::GetActive() {
    return s_activeCharacterGuid;
}

// VERIFY: db8aba56 manual
// OFFSET: 0x004D80C0
void CGlueMgr::DisplayLoginStatus() {
    if (CGlueMgr::m_lastLoginState == CGlueMgr::m_loginState && CGlueMgr::m_lastLoginResult == CGlueMgr::m_loginResult) {
        return;
    }

    CGlueMgr::m_lastLoginState = CGlueMgr::m_loginState;
    CGlueMgr::m_lastLoginResult = CGlueMgr::m_loginResult;

    if (CGlueMgr::m_loginState == LOGIN_STATE_FAILED) {
        if (CGlueMgr::m_loginResult < 0 || CGlueMgr::m_loginResult >= LOGIN_RESULT_MAX) {
            return;
        }

        if (CGlueMgr::m_loginResult == LOGIN_PARENTALCONTROL) {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s%s", "PARENTAL_CONTROL", FrameScript_GetText(g_LoginResultStringNames[LOGIN_PARENTALCONTROL], -1, GENDER_NOT_APPLICABLE), "AUTH_PARENTAL_CONTROL_URL");
            return;
        }

        const char* text = FrameScript_GetText(g_LoginResultStringNames[CGlueMgr::m_loginResult], -1, GENDER_NOT_APPLICABLE);
        const char* button;

        if (SStrStrUTF8I(text, "<HTML>")) {
            button = "OKAY_HTML";

            if (CGlueMgr::m_loginResult == LOGIN_FAILED) {
                FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CONNECTION_HELP_HTML", text);
                return;
            }
        } else {
            button = CGlueMgr::m_loginResult == LOGIN_FAILED ? "CONNECTION_HELP" : "OKAY";
        }

        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", button, text);
        return;
    }

    int32_t state = CGlueMgr::m_loginState;
    bool show = state >= 0 && state < LOGIN_STATE_MAX;

    if (state == LOGIN_STATE_DISCONNECTED || state == LOGIN_STATE_SURVEY) {
        return;
    }

    if (state == LOGIN_STATE_MATRIX || state == LOGIN_STATE_MATRIX_WAIT) {
        show = false;
    }

    if (state == LOGIN_STATE_TOKEN || state == LOGIN_STATE_TOKEN_WAIT) {
        return;
    }

    if (show) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText(g_LoginStateStringNames[state], -1, GENDER_NOT_APPLICABLE));
    }
}

// VERIFY: 6397c8fa auto
// OFFSET: 0x004D8350
void CGlueMgr::QuitGame() {
    ClientPostClose(0);
}

// VERIFY: 36e76190 manual
// OFFSET: 0x004D98D0
void CGlueMgr::StatusDialogClick() {
    if (!SStrCmpI(CGlueMgr::m_currentScreen, "patchdownload", STORM_MAX_STR)) {
        FrameScript_SignalEvent(GLUE_EVENT_SET_GLUE_SCREEN, "%s", "login");
    }

    switch (CGlueMgr::m_idleState) {
        case IDLE_NONE:
            ClientServices::Connection()->Cleanup();
            break;

        case IDLE_LOGIN_SERVER_LOGIN:
            ClientServices::LoginConnection()->Logoff();
            CGlueMgr::m_showedDisconnect = 0;
            CGlueMgr::m_idleState = IDLE_NONE;
            break;

        case IDLE_ACCOUNT_LOGIN:
        case IDLE_CHARACTER_LIST:
            ClientServices::Connection()->Cancel(2);
            CCharacterSelection::ClearCharacterList();
            break;

        case IDLE_REALM_LIST:
        case IDLE_CREATE_CHARACTER:
        case IDLE_DELETE_CHARACTER:
        case IDLE_ENTER_WORLD:
            ClientServices::Connection()->Cancel(2);
            CGlueMgr::m_showedDisconnect = 0;
            CGlueMgr::m_idleState = IDLE_NONE;
            break;

        case IDLE_7:
        case IDLE_8:
        case IDLE_9:
            CGlueMgr::m_showedDisconnect = 0;
            CGlueMgr::m_idleState = IDLE_NONE;
            break;

        case IDLE_WORLD_LOGIN:
            CGlueMgr::m_idleState = IDLE_NONE;
            CGlueMgr::m_showedDisconnect = 0;
            CGlueMgr::GetCharacterList();
            break;

        case IDLE_12:
        case IDLE_13:
            if (CGlueMgr::m_surveyDownload) {
                CGlueMgr::SurveyDownloadCancel();
                CGlueMgr::m_showedDisconnect = 0;
                CGlueMgr::m_idleState = IDLE_NONE;
            }

            break;

        default:
            break;
    }
}

// VERIFY: 4eb1c510 auto
// OFFSET: 0x004DA9D0
int32_t CGlueMgr::NetDisconnectHandler(NetClient* client) {
    bool displayScript = CGlueMgr::m_idleState != IDLE_ACCOUNT_LOGIN;
    CGlueMgr::m_idleState = IDLE_NONE;
    CGlueMgr::m_showedDisconnect = 0;

    if (CGlueMgr::m_disconnectPending) {
        ConsolePrintf("CGlueMgr::NetDisconnectHandler: Disconnect pending");
        CGlueMgr::m_disconnectPending = 0;

        if (CGlueMgr::m_reconnect) {
            CGlueMgr::m_reconnect = 0;
            CGlueMgr::m_idleState = IDLE_ACCOUNT_LOGIN;
            CGlueMgr::m_showedDisconnect = 0;
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("GAME_SERVER_LOGIN", -1, GENDER_NOT_APPLICABLE));
            ClientServices::Connection()->Connect();
        }

        return 1;
    }

    if (!ClientServices::ValidDisconnect(client)) {
        ConsolePrintf("CGlueMgr::NetDisconnectHandler: Invalid disconnect");
        return 1;
    }

    ClientDestroyGame(0, 1, 0);
    EventSetMouseMode(MOUSE_MODE_NORMAL, 0);

    if (CGlueMgr::m_suspended) {
        CGlueMgr::Resume();
    }

    if (displayScript) {
        ConsolePrintf("CGlueMgr::NetDisconnectHandler: Displaying script");
        FrameScript_SignalEvent(GLUE_EVENT_DISCONNECTED_FROM_SERVER, "%d", s_disconnectReason);
    } else {
        ConsolePrintf("CGlueMgr::NetDisconnectHandler: NOT displaying script");

        WOWCS_OPS op;
        const char* text = nullptr;
        int32_t result;
        int32_t errorCode;

        if (!ClientServices::Connection()->PollStatus(op, &text, result, errorCode) || result) {
            ClientServices::SelectRealm("");
            FrameScript_SignalEvent(GLUE_EVENT_DISCONNECTED_FROM_SERVER, "%d", s_disconnectReason);
        } else {
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", text);
        }
    }

    ClientServices::LoginConnection()->Logoff();

    return 1;
}

// VERIFY: 390f3100 manual
// OFFSET: 0x004D7DA0
void CGlueMgr::UpdateWaitQueue(int32_t position) {
    if (position != CGlueMgr::m_queuePosition[0] || !CGlueMgr::m_queueTime[7]) {
        for (int32_t i = 7; i; i--) {
            CGlueMgr::m_queuePosition[i] = CGlueMgr::m_queuePosition[i - 1];
            CGlueMgr::m_queueTime[i] = CGlueMgr::m_queueTime[i - 1];
        }

        CGlueMgr::m_queuePosition[0] = position;
    }

    uint32_t now = OsGetAsyncTimeMs();
    CGlueMgr::m_queueTime[0] = now;

    if (CGlueMgr::m_queueTime[7] && CGlueMgr::m_queuePosition[0] - CGlueMgr::m_queuePosition[7] < 0) {
        CGlueMgr::m_estimatedWaitTime = -(position * (static_cast<int32_t>(now - CGlueMgr::m_queueTime[7]) / (CGlueMgr::m_queuePosition[0] - CGlueMgr::m_queuePosition[7])));
    }
}

// VERIFY: 13f80fa2 manual
// OFFSET: 0x004D8E10
void CGlueMgr::CustomizeCharacter(WOWGUID guid, const CHARACTER_CREATE_INFO* info) {
    if (!guid || !info || !info->m_name[0]) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_9;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("CHAR_CUSTOMIZE_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAR_CUSTOMIZE));
    msg.Put(guid.m_guid);
    msg.PutString(info->m_name);
    msg.Put(info->m_sexID);
    msg.Put(info->m_skinID);
    msg.Put(info->m_hairColorID);
    msg.Put(info->m_hairStyleID);
    msg.Put(info->m_facialHairStyleID);
    msg.Put(info->m_faceID);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: ac651ceb manual
// OFFSET: 0x004D8F20
void CGlueMgr::FactionChange(WOWGUID guid, const CHARACTER_CREATE_INFO* info) {
    if (!guid || !info || !info->m_name[0]) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_9;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("FACTION_CHANGE_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAR_FACTION_CHANGE));
    msg.Put(guid.m_guid);
    msg.PutString(info->m_name);
    msg.Put(info->m_sexID);
    msg.Put(info->m_skinID);
    msg.Put(info->m_hairColorID);
    msg.Put(info->m_hairStyleID);
    msg.Put(info->m_facialHairStyleID);
    msg.Put(info->m_faceID);
    msg.Put(info->m_raceID);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 72aae471 manual
// OFFSET: 0x004D9040
void CGlueMgr::RaceChange(WOWGUID guid, const CHARACTER_CREATE_INFO* info) {
    if (!guid || !info || !info->m_name[0]) {
        return;
    }

    CGlueMgr::m_idleState = IDLE_9;
    CGlueMgr::m_showedDisconnect = 0;
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "CANCEL", FrameScript_GetText("RACE_CHANGE_IN_PROGRESS", -1, GENDER_NOT_APPLICABLE));
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_CHAR_RACE_CHANGE));
    msg.Put(guid.m_guid);
    msg.PutString(info->m_name);
    msg.Put(info->m_sexID);
    msg.Put(info->m_skinID);
    msg.Put(info->m_hairColorID);
    msg.Put(info->m_hairStyleID);
    msg.Put(info->m_facialHairStyleID);
    msg.Put(info->m_faceID);
    msg.Put(info->m_raceID);
    msg.Finalize();
    ClientServices::Send2(&msg);
}

// VERIFY: 1b14a261 auto
// OFFSET: 0x004D7F90
void CGlueMgr::LoginEnterPIN(int32_t count, uint8_t* digits) {
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "CANCEL");
    ClientServices::LoginEnterPIN(count, digits);
}

// VERIFY: 89a8abe0 auto
// OFFSET: 0x004D7FB0
void CGlueMgr::LoginEnterMatrix(uint8_t value) {
    if (CGlueMgr::m_loginState == LOGIN_STATE_MATRIX_WAIT && CGlueMgr::m_matrixRemaining) {
        ClientServices::LoginEnterMatrix(value);
    }
}

// VERIFY: 327a11cd manual
// OFFSET: 0x004D7FD0
void CGlueMgr::LoginRevertMatrix() {
    if (CGlueMgr::m_loginState == LOGIN_STATE_MATRIX_WAIT && CGlueMgr::m_matrixRemaining) {
        ClientServices::LoginRevertMatrix();
    }
}

// VERIFY: 697e070f auto
// OFFSET: 0x004D7FF0
void CGlueMgr::LoginCommitMatrix() {
    if (CGlueMgr::m_loginState == LOGIN_STATE_MATRIX_WAIT && CGlueMgr::m_matrixRemaining) {
        ClientServices::LoginCommitMatrix();

        if (!--CGlueMgr::m_matrixRemaining) {
            ClientServices::LoginFinalizeMatrix();
            FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "CANCEL");
        }
    }
}

// VERIFY: 4fc0fd1e auto
// OFFSET: 0x004D8030
bool CGlueMgr::LoginGetMatrixCoordinate(uint32_t* x, uint32_t* y) {
    if (CGlueMgr::m_loginState != LOGIN_STATE_MATRIX_WAIT || !CGlueMgr::m_matrixRemaining) {
        return false;
    }

    return ClientServices::LoginConnection()->GetMatrixCoordinates(CGlueMgr::m_matrixChallengeCount - CGlueMgr::m_matrixRemaining, x, y);
}

// VERIFY: 24156984 auto
// OFFSET: 0x004D8080
void CGlueMgr::LoginEnterToken(const char* token) {
    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s", "CANCEL");
    ClientServices::LoginEnterToken(token);
}

// VERIFY: 90e67519 auto
// OFFSET: 0x004D9190
int32_t CGlueMgr::OnCharCustomizeResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid = { 0 };
    char name[48];
    uint8_t sexID = 0;
    uint8_t skinID = 0;
    uint8_t faceID = 0;
    uint8_t hairStyleID = 0;
    uint8_t hairColorID = 0;
    uint8_t facialHairStyleID = 0;
    uint8_t result;

    CGlueMgr::m_idleState = IDLE_NONE;
    CGlueMgr::m_showedDisconnect = 0;
    msg->Get(result);

    if (!msg->IsRead()) {
        msg->Get(guid.m_guid);
        msg->GetString(name, sizeof(name));
        msg->Get(sexID);
        msg->Get(skinID);
        msg->Get(faceID);
        msg->Get(hairStyleID);
        msg->Get(hairColorID);
        msg->Get(facialHairStyleID);
    }

    if (result) {
        const char* text = FrameScript_GetText(result == 50 ? ClientServices::GetErrorToken(50) : "CHAR_CUSTOMIZE_FAILED", -1, GENDER_NOT_APPLICABLE);
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", text);
        return 1;
    }

    CCharacterSelection::UpdateCharacterInfo(guid, name, sexID, skinID, faceID, hairStyleID, hairColorID, facialHairStyleID, 0);
    FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
    FrameScript_SignalEvent(GLUE_EVENT_SET_GLUE_SCREEN, "%s", "charselect");
    return 1;
}

// VERIFY: 894a77a0 auto
// OFFSET: 0x004D92D0
int32_t CGlueMgr::OnCharFactionChangeResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid = { 0 };
    char name[48];
    uint8_t sexID = 0;
    uint8_t skinID = 0;
    uint8_t faceID = 0;
    uint8_t hairStyleID = 0;
    uint8_t hairColorID = 0;
    uint8_t facialHairStyleID = 0;
    uint8_t raceID = 0;
    uint8_t result;

    CGlueMgr::m_idleState = IDLE_NONE;
    CGlueMgr::m_showedDisconnect = 0;
    msg->Get(result);

    if (!msg->IsRead()) {
        msg->Get(guid.m_guid);
        msg->GetString(name, sizeof(name));
        msg->Get(sexID);
        msg->Get(skinID);
        msg->Get(faceID);
        msg->Get(hairStyleID);
        msg->Get(hairColorID);
        msg->Get(facialHairStyleID);
        msg->Get(raceID);
    }

    const char* text;

    switch (result) {
        case 0:
            CCharacterSelection::UpdateCharacterInfo(guid, name, sexID, skinID, faceID, hairStyleID, hairColorID, facialHairStyleID, raceID);
            FrameScript_SignalEvent(GLUE_EVENT_CLOSE_STATUS_DIALOG, nullptr);
            CGlueMgr::SetScreen("charselect");
            return 1;

        case 50:
            text = FrameScript_GetText(ClientServices::GetErrorToken(50), -1, GENDER_NOT_APPLICABLE);
            break;

        case 61:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_STILL_IN_GUILD", -1, GENDER_NOT_APPLICABLE);
            break;

        case 62:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_RACECLASS_RESTRICTED", -1, GENDER_NOT_APPLICABLE);
            break;

        case 63:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_CHOOSE_RACE", -1, GENDER_NOT_APPLICABLE);
            break;

        case 64:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_ARENA_LEADER", -1, GENDER_NOT_APPLICABLE);
            break;

        case 65:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_DELETE_MAIL", -1, GENDER_NOT_APPLICABLE);
            break;

        case 66:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_SWAP_FACTION", -1, GENDER_NOT_APPLICABLE);
            break;

        case 67:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_RACE_ONLY", -1, GENDER_NOT_APPLICABLE);
            break;

        case 68:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_GOLD_LIMIT", -1, GENDER_NOT_APPLICABLE);
            break;

        case 69:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_FORCE_LOGIN", -1, GENDER_NOT_APPLICABLE);
            break;

        default:
            text = FrameScript_GetText("CHAR_FACTION_CHANGE_FAILED", -1, GENDER_NOT_APPLICABLE);
            break;
    }

    FrameScript_SignalEvent(GLUE_EVENT_OPEN_STATUS_DIALOG, "%s%s", "OKAY", text);
    return 1;
}

// VERIFY: 2e472e07 auto
// OFFSET: 0x004D9500
int32_t CGlueMgr::OnKickReasonMsg(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint8_t reason = 0;
    uint8_t seed[16];

    if (!msg->IsRead()) {
        msg->Get(reason);

        if (!msg->IsRead() && CGlueMgr::m_initialized && !CGlueMgr::m_suspended) {
            msg->GetArray(seed, sizeof(seed));

            if (msg->m_read <= msg->m_size) {
                InitTokenSeed(seed);
                return 1;
            }
        }
    }

    s_disconnectReason = reason;
    return 1;
}
