#ifndef WOW_SOURCE_CLIENT_H
#define WOW_SOURCE_CLIENT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/Source/SThread.h"
#include "WoW/Source/DB/Types.h"
#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/Startup_StringsRec.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Net/Types.h"

struct CVar;
class CDataStore;

class CAsyncObject;
class CObjectHeapList;

enum WOWCMDOPT {
    CMD_RES_800x600 = 16,
    CMD_RES_1024x768 = 17,
    CMD_RES_1280x960 = 18,
    CMD_RES_1280x1024 = 19,
    CMD_RES_1600x1200 = 20,
    CMD_UP_TO_DATE = 21,
    CMD_16_BIT = 22,
    CMD_NO_FIX_LAG = 24,
    CMD_NO_SOUND = 26,
    CMD_SOUND_CHAOS = 27,
    CMD_DEPTH_16 = 29,
    CMD_DEPTH_24 = 30,
    CMD_DEPTH_32 = 31,
    CMD_WINDOWED = 32,
    CMD_CONSOLE = 35,
    CMD_HW_DETECT = 36,
    CMD_GX_OVERRIDE = 39,
};

struct ARCHIVEINFO {
    /* 0000 */ const char* m_name;
    /* 0004 */ int32_t m_unk04;
    /* 0008 */ int32_t m_unk08;
    /* 000C */ bool m_opened;
};

extern CVar* g_accountListVar;
extern CVar* g_accountNameVar;
extern CVar* g_accountUsesTokenVar;
extern CVar* g_checkAddonVersionVar;
extern HEVENTCONTEXT g_clientEventContext;
extern bool g_hasIsoLocale[NUM_DB_LOCALES];
extern CVar* g_lastCharacterIndexVar;
extern CVar* g_readContestVar;
extern CVar* g_readEULAVar;
extern CVar* g_readScanningVar;
extern CVar* g_readTerminationWithoutNoticeVar;
extern CVar* g_readTOSVar;
extern WowClientDB<Startup_StringsRec> g_startup_StringsDB;
extern CVar* s_cvarUIFaster;
extern CVar* s_cvExpansionMovie;
extern CVar* s_cvMovie;
extern CVar* s_cvTextureCacheSize;
extern CVar* s_cvTextureFilteringMode;
extern CVar* s_desktopGammaCvar;
extern uint8_t s_expansionLevel;
extern CVar* s_gammaCvar;
extern int32_t s_locale;
extern const char* s_iniLocaleNames[9];
extern uint32_t s_localeIds[9];
extern char s_localeCode[5];
extern int32_t s_isPvPRealm;
extern int32_t s_region;

void AsyncFileReadRegisterCVars();
void BeginCloseGame();
int32_t CCommand_Perf(const char* command, const char* arguments);
int32_t CCommand_ReloadUI(const char* command, const char* arguments);
int32_t CCommand_TimingInfo(const char* command, const char* arguments);
bool CheckDataFilesExist();
void ClientDestroyGame(int32_t charSelect, int32_t resumeGlue, int32_t worldLogin);
void ClientInitializeGame(uint32_t mapId, C3Vector position);
void ClientPostClose(int32_t reason);
uint32_t ClientSetTimer(uint32_t delay, EVENTHANDLERFUNC handler, void* param);
uint32_t ClientSetTimer(uint32_t delay, EVENTHANDLERGUIDFUNC handler, uint64_t guid, void* param);
void ClientRegisterConsoleCommands();
int32_t CommonMain();
int32_t DestroyEngineCallback(const void* data, void* param);
uint8_t GetClientExpansionLevel();
const char* GetInstallLocationString();
uint32_t GetM2CvarFlags();
int32_t InitializeEngineCallback(const void* data, void* param);
bool InitializeGlobal();
bool LocaleCallback(CVar* var, const char* oldValue, const char* value, void* arg);
int main();
int32_t Packet_SMSG_NOTIFICATION(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_PLAYED_TIME(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_TRANSFER_ABORTED(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
int32_t Packet_SMSG_TRANSFER_PENDING(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
bool ProcessAffinityMaskCallback(CVar* var, const char* oldValue, const char* value, void* arg);
int32_t STORMAPI SendErrorLog(uint32_t errorcode, const char* description, const char* filename, int32_t linenumber, const char* errorstring);
void SetPaths();
void StartupReport(bool firstRun);
bool TextureCacheSizeCallback(CVar* var, const char* oldValue, const char* value, void* arg);
bool TimingMethodCallback(CVar* var, const char* oldValue, const char* value, void* arg);
bool UIFasterCallback(CVar* var, const char* oldValue, const char* value, void* arg);
int32_t STORMAPI WowLogHeader(char* buffer, uint32_t bufferchars);
void ZoneDebugDestroy();

struct ARCHIVELISTPARAMS {
    /* 0000 */ const char* m_prefix;
    /* 0004 */ uint32_t m_prefixLength;
    /* 0008 */ int32_t (*m_callback)(const char* fileName, void* param);
    /* 000C */ void* m_param;
};

struct AdditionalCheckInterface {
    /* 0000 */ void* (*m_memAlloc)(uint32_t bytes);
    /* 0004 */ void (*m_memFree)(void* ptr);
    /* 0008 */ void* (*m_loadLibrary)(const char* fileName);
    /* 000C */ void* (*m_getLibraryEntryPoint)(void* module, const char* name);
    /* 0010 */ void (*m_freeLibrary)(void* module);
};

extern uint8_t s_botCheckEnabled;
extern int32_t s_botCheckDelay;
extern volatile long s_additionalCheckArmed;

void CheckAndSendBotDetected();
void AdditionalCheck1();
uint8_t AdditionalCheck1Scan(const AdditionalCheckInterface* iface, int32_t unused_a2);
void InitTokenSeed(const uint8_t* key);
void* Client_MemAlloc(uint32_t bytes);
void Client_MemFree(void* ptr);
void SendBotDetected2(uint32_t msgId, uint32_t code);
int32_t GatherAdditionalChecks(uint8_t* check1, uint8_t* check2, uint8_t* check3);
void ClientUpdateTaxiBenchmarkMode();
void ClientKillTimer(uint32_t id, EVENTHANDLERFUNC handler, const char* name);
void ListInterfaceArchive(const char* prefix, int32_t (*callback)(const char* fileName, void* param), void* param);

#endif
