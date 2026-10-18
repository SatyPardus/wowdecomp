#ifndef WOW_SOURCE_GLUE_CGLUEMGR_H
#define WOW_SOURCE_GLUE_CGLUEMGR_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Os/OsURLDownload.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowServices/Types.h"

struct CVar;
class SFileImpl;
struct SArchive;
class CDataStore;
struct CHARACTER_CREATE_INFO;
class CHARACTER_INFO;
class CM2Model;
class CharacterSelectionDisplay;
class CSimpleTop;
class SThread;

namespace FFX {
    class Effect;
}
struct MipBits;
class CSizeEvent;
class FileDownload;
class NetClient;
struct CVar;

struct ConsoleString {
    /* 0000 */ TSLink<ConsoleString> m_link;
    /* 0008 */ char m_text[256];
};

struct LoginErrorURL {
    /* 0000 */ int32_t m_errorCode;
    /* 0004 */ char m_url[32];
};

class CGlueMgr {
    public:
    static void HandleScreenshot(int32_t success);
    struct Agreements {
        /* 0000 */ TSGrowableArray<char> m_data;
        /* 0010 */ int32_t m_complete;
    };

    static char m_serverAlert[2048];
    static uint32_t m_queueTime[8];
    static int32_t m_queuePosition[8];
    static MipBits* m_cursorMipBits;
    static char m_currentScreen[64];
    static CSimpleTop* m_simpleTop;
    static int32_t m_reload;
    static int32_t m_initialized;
    static int32_t m_suspended;
    static int32_t m_disconnectPending;
    static int32_t m_reconnect;
    static GLUE_IDLE_STATE m_idleState;
    static uint32_t m_matrixChallengeCount;
    static char m_accountName[1280];
    static int32_t m_loginResult;
    static int32_t m_loginState;
    static int32_t m_lastLoginResult;
    static int32_t m_lastLoginState;
    static uint32_t m_matrixRemaining;
    static int32_t m_estimatedWaitTime;
    static int32_t m_displayingQueueDialog;
    static CHARACTER_INFO* m_characterInfo;
    static int32_t s_displayedCharacterIndex;
    static int32_t m_rereadTOS;
    static int32_t m_acceptedTOS;
    static int32_t m_rereadEULA;
    static int32_t m_acceptedEULA;
    static int32_t m_rereadTerminationWithoutNotice;
    static int32_t m_acceptedTerminationWithoutNotice;
    static int32_t m_rereadScanning;
    static int32_t m_acceptedScanning;
    static int32_t m_rereadContest;
    static int32_t m_acceptedContest;
    static uint8_t m_authenticated;
    static bool s_scriptFunctionsLoaded;
    static int32_t m_screenWidth;
    static int32_t m_screenHeight;
    static float m_aspect;
    static int32_t m_pendingServerAlert;
    static int32_t m_processServerAlert;
    static uint32_t m_pendingTimerAlert;
    static int32_t m_accountMsgAvailable;
    static uint32_t m_numChangedOptionWarnings;
    static int32_t m_showedDisconnect;
    static Agreements m_latestAgreements;
    static TSGrowableArray<const char*> m_changedOptionWarnings;
    static uint32_t m_surveyTimer;
    static uint8_t m_executedSurvey;
    static FileDownload* m_surveyDownload;
    static FileDownload* m_patchDownload;
    static float m_patchDownloadProgress;
    static uint32_t m_patchLastUpdate;
    static uint32_t m_patchStartTime;
    static uint8_t* m_surveyData;
    static uint16_t m_surveyDataSize;
    static bool m_deleteLocalPatch;
    static uint8_t m_ffxActive;
    static FFX::Effect* m_deathEffect;
    static FFX::Effect* m_glowEffect;
    static char m_scandllHackName[512];
    static uint8_t m_scandllOkayToLogIn;
    static const char* m_scandllResult;
    static uint8_t m_scandllShouldBlockPlaying;
    static GlueScanStatus m_scandllStatus;
    static SThread m_scandllThread;

    static void AcceptChangedOptionWarnings();
    static void AcceptContest();
    static void AcceptEULA();
    static void AcceptScanning();
    static void AcceptTerminationWithoutNotice();
    static void AcceptTOS();
    static void AddChangedOptionWarning(const char* warning);
    static void AttachCharacterSelectVisuals(CM2Model* model);
    static void ChangeRealm(REALM_INFO* realm);
    static void CreateCharacter(const CHARACTER_CREATE_INFO* info);
    static void CustomizeCharacter(WOWGUID guid, const CHARACTER_CREATE_INFO* info);
    static bool AreScriptFunctionsLoaded();
    static void CancelLogin();
    static void DownloadFeed(const uint8_t* data, uint32_t size);
    static size_t GetPatchFolder(char* buffer, uint32_t size);
    static void LoginCommitMatrix();
    static void LoginEnterMatrix(uint8_t value);
    static void LoginEnterPIN(int32_t count, uint8_t* digits);
    static bool LoginGetMatrixCoordinate(uint32_t* x, uint32_t* y);
    static void LoginRevertMatrix();
    static void PatchDownloadApply();
    static void PatchDownloadCancel();
    static bool PatchDownloadDeleteCommand(const char* path);
    static bool PatchDownloadExecuteCommand(const char* path, const char* arguments);
    static bool PatchDownloadExecutePrepatch(SArchive* archive);
    static bool PatchDownloadExtractCommand(SArchive* archive, const char* name);
    static void PatchDownloadFeed(const uint8_t* data, uint32_t size);
    static void PatchFailed(int32_t reason, uint32_t data);
    static void QuitGameAndRunLauncher();
    static void SurveyDownloadFeed(const uint8_t* data, uint32_t size);
    static bool DeclineCharacter(WOWGUID guid, const char** names);
    static void DeleteCharacter(WOWGUID guid);
    static void EnterWorld();
    static bool AgreementsURLCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error);
    static void DisplayLoginStatus();
    static void FactionChange(WOWGUID guid, const CHARACTER_CREATE_INFO* info);
    static WOWGUID GetActive();
    static void GetCharacterList();
    static void GetRealmList(int32_t showDialog);
    static int32_t HandleBattlenetDisconnect();
    static int32_t NetDisconnectHandler(NetClient* client);
    static void HandleDisplaySizeChanged(const CSizeEvent& event);
    static int32_t Idle(const void* data, void* param);
    static void Initialize();
    static void SurveyDownloadCancel();
    static void StatusDialogClick();
    static void QuitGame();
    static void RaceChange(WOWGUID guid, const CHARACTER_CREATE_INFO* info);
    static bool RenameCharacter(WOWGUID guid, const char* name);
    static void RealmListDialogCancelled();
    static void InitializeFFX();
    static bool IsScanDLLFinished();
    static int32_t OnCharCustomizeResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnCharDeclineResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnCharFactionChangeResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnCharRenameResult(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnKickReasonMsg(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static int32_t OnRealmSplitMsg(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
    static void PatchDownloadIdle();
    static void ProcessAttachment(CM2Model* model, int32_t effectID, uint32_t attachID);
    static void ProcessAgreements(CGlueMgr::Agreements* agreements);
    static void PatchDownloadStart();
    static void LoginServerLogin(const char* username, char* password);
    static void LoginEnterToken(const char* token);
    static void LogConnectionStatus(const char* format, ...);
    static void ResetQueueState();
    static void SetCurrentAccount(const char* accountName);
    static void SetLoginStateAndResult(LOGIN_STATE state, LOGIN_RESULT result, const char* addressString, const char* stateString, const char* resultString, uint16_t flags);
    static void Resume();
    static void Shutdown();
    static void ScanDLLContinueAnyway();
    static void ScanDLLFinished();
    static void ScanDLLStart(const char* versionURL, const char* dllURL);
    static bool ServerAlertURLCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error);
    static void SetScreen(const char* screen);
    static void SetUIDepth(float depth);
    static int32_t ShowChangedOptionWarnings();
    static void Suspend();
    static void SurveyDownloadEnd();
    static void SurveyDownloadIdle();
    static void SurveyDownloadStart();
    static bool SurveyExecute();
    static void SurveySendResults();
    static void UpdateCurrentScreen(const char* screen);
    static void UpdateWaitQueue(int32_t position);
};

extern const uint8_t InterfaceKey[256];

void CancelRealmListQuery();
CharacterSelectionDisplay* GetCharacterInfo(int32_t index);
void ProcessAgreement(SFileImpl* file, const char* name, CVar* cvar, bool localized);

#endif
