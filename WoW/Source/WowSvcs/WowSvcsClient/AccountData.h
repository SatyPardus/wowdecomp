#ifndef WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_ACCOUNTDATA_H
#define WOW_SOURCE_WOWSVCS_WOWSVCSCLIENT_ACCOUNTDATA_H

#include <cstdint>
#include "Engine/Source/Os/OsFile.h"
#include "WoW/Source/Net/Types.h"

class CDataStore;
struct CVar;

typedef void (*ACCOUNTDATALOADFUNC)(HOSFILE localFile, HOSFILE cacheFile, void* param);

struct AccountDataInfo {
    /* 0000 */ bool m_active;
    /* 0001 */ bool m_global;
    /* 0004 */ const char* m_localFile;
    /* 0008 */ const char* m_cacheFile;
    /* 000C */ const char* m_oldFile;
    /* 0010 */ const char* m_cvarName;
    /* 0014 */ bool m_localOnly;
    /* 0015 */ bool m_downloading;
    /* 0018 */ ACCOUNTDATALOADFUNC m_loadFunc;
    /* 001C */ void* m_loadParam;
    /* 0020 */ uint32_t m_saveTimer;
    /* 0024 */ uint8_t m_md5[16];
    /* 0034 */ bool m_changed;
    /* 0038 */ int32_t m_time;
    /* 003C */ int32_t m_serverTime;
};

extern CVar* s_cvSynchronizeSettings;

void AccountDataInitialize(bool global);
void AccountDataInitializeBasicSystem();
void AccountDataShutdownBasicSystem();
void DownloadAccountData(int32_t type);
void LoadAccountData(int32_t type, ACCOUNTDATALOADFUNC func, void* param);
void SaveAccountData(int32_t type, ACCOUNTDATALOADFUNC func, void* param);
void AccountDataFlush();
int32_t Packet_SMSG_CHECK_FOR_BOTS(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg);
void SetClearConfigData(int32_t clear);
void UploadAccountData(int32_t type);
void UploadAccountData();
void DownloadAccountData();

void AccountDataDestroy(bool global);

#endif
