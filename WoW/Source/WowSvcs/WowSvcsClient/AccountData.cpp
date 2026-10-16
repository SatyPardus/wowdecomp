#include "AccountData.h"
#include <cstring>
#include <malloc.h>
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "ThirdParty/zlib/zlib.h"
#include "WoW/Source/Client.h"
#include "Engine/Source/Event/Event.h"

// STATIC: 0x00AD94E0
static AccountDataInfo s_accountData[] = {
    { false, true, nullptr, "config-cache.wtf", "config-cache.old", "synchronizeConfig" },
    { false, false, nullptr, "config-cache.wtf", "config-cache.old", "synchronizeConfig" },
    { false, true, nullptr, "bindings-cache.wtf", "bindings-cache.old", "synchronizeBindings" },
    { false, false, nullptr, "bindings-cache.wtf", "bindings-cache.old", "synchronizeBindings" },
    { false, true, nullptr, "macros-cache.txt", "macros-cache.old", "synchronizeMacros" },
    { false, false, nullptr, "macros-cache.txt", "macros-cache.old", "synchronizeMacros" },
    { false, false, "layout-local.txt", nullptr, nullptr, nullptr, true },
    { false, false, nullptr, "chat-cache.txt", "chat-cache.old", nullptr }
};

// STATIC: 0x00C7BFF0
static uint32_t s_accountDataTime;

// STATIC: 0x00C7BFF8
static int32_t s_clearGlobalConfig;

// STATIC: 0x00C7BFFC
static int32_t s_clearCharacterConfig;

// VERIFY: 4934bdac auto
// OFFSET: 0x006B8AB0
static int32_t CopyAccountFile(bool global, const char* source, const char* dest) {
    char sourcePath[260];
    char destPath[260];

    const char* accountName = ClientServices::GetAccountName();

    if (!accountName || !source || !dest) {
        return 0;
    }

    if (global) {
        SStrPrintf(sourcePath, sizeof(sourcePath), "WTF\\Account\\%s\\%s", accountName, source);
        SStrPrintf(destPath, sizeof(destPath), "WTF\\Account\\%s\\%s", accountName, dest);
    } else {
        SStrPrintf(sourcePath, sizeof(sourcePath), "WTF\\Account\\%s\\%s\\%s\\%s", accountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName(), source);
        SStrPrintf(destPath, sizeof(destPath), "WTF\\Account\\%s\\%s\\%s\\%s", accountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName(), dest);
    }

    return OsCopyFile(sourcePath, destPath, 0);
}

// VERIFY: e89d2711 auto
// OFFSET: 0x006B8B90
static HOSFILE OpenAccountFile(bool global, const char* fileName, bool write) {
    char path[260];

    const char* accountName = ClientServices::GetAccountName();

    if (!accountName || !fileName) {
        return OS_INVALID_FILE;
    }

    if (global) {
        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s", accountName);
    } else {
        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s", accountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName());
    }

    if (write && !OsDirectoryExists(path)) {
        OsCreateDirectory(path, 1);
    }

    SStrPack(path, "\\", sizeof(path));
    SStrPack(path, fileName, sizeof(path));

    if (write) {
        return OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    }

    return OsCreateFile(path, OS_GENERIC_READ, 0, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
}

// VERIFY: e5cad552 auto
// OFFSET: 0x006B8CA0
static void MD5AccountFile(const char* fileName, uint8_t* digest, bool global) {
    memset(digest, 0, 16);

    HOSFILE file = OpenAccountFile(global, fileName, false);

    if (file == OS_INVALID_FILE) {
        return;
    }

    if (OsGetFileSize(file)) {
        MD5_CTX context;
        MD5Init(&context);

        uint8_t buffer[1024];
        uint32_t bytesRead;

        do {
            OsReadFile(file, buffer, sizeof(buffer), &bytesRead);
            MD5Update(&context, buffer, bytesRead);
        } while (bytesRead);

        MD5Final(digest, &context);
    }

    OsCloseFile(file);
}

// VERIFY: 862af76a manual
// OFFSET: 0x006B8D40
static void LoadCachedChecksums(bool global) {
    uint32_t count = 0;

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        if (s_accountData[i].m_global == global) {
            count++;
        }
    }

    HOSFILE file = OpenAccountFile(global, "cache.md5", false);

    if (file != OS_INVALID_FILE) {
        if (OsGetFileSize(file) == (sizeof(s_accountData[0].m_md5) + sizeof(s_accountData[0].m_time)) * count) {
            for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
                AccountDataInfo& info = s_accountData[i];

                if (info.m_global == global) {
                    uint32_t bytesRead;
                    OsReadFile(file, info.m_md5, sizeof(info.m_md5), &bytesRead);
                    OsReadFile(file, &info.m_time, sizeof(info.m_time), &bytesRead);
                }
            }
        }

        OsCloseFile(file);
    }

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];

        if (info.m_global == global) {
            uint8_t digest[16];
            MD5AccountFile(info.m_cacheFile, digest, info.m_global);
            info.m_changed = memcmp(digest, info.m_md5, sizeof(info.m_md5)) != 0;
        }
    }
}

// VERIFY: e5255567 manual
// OFFSET: 0x006B8EE0
static void SaveCachedChecksums(bool global) {
    HOSFILE file = OpenAccountFile(global, "cache.md5", true);

    if (file == OS_INVALID_FILE) {
        return;
    }

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];

        if (info.m_global == global) {
            uint32_t bytesWritten;
            OsWriteFile(file, info.m_md5, sizeof(info.m_md5), &bytesWritten);
            OsWriteFile(file, &info.m_time, sizeof(info.m_time), &bytesWritten);
        }
    }

    OsCloseFile(file);
}

// VERIFY: f49c9f62 auto
// OFFSET: 0x006B8F50
static void UpdateCachedChecksum(int32_t type, int32_t time) {
    AccountDataInfo& info = s_accountData[type];

    uint8_t digest[16];
    MD5AccountFile(info.m_cacheFile, digest, info.m_global);
    memcpy(info.m_md5, digest, sizeof(info.m_md5));

    info.m_changed = false;
    info.m_time = time;
    info.m_serverTime = time;

    SaveCachedChecksums(info.m_global);
}

// VERIFY: 72a9a2be auto
// OFFSET: 0x006B8FC0
static int32_t Packet_SMSG_UPDATE_ACCOUNT_DATA_COMPLETE(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t type;
    msg->Get(type);

    uint32_t result;
    msg->Get(result);

    return 1;
}

// VERIFY: 682b8825 manual
// OFFSET: 0x006B9020
void AccountDataDestroy(bool global) {
    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        if (s_accountData[i].m_global == global) {
            s_accountData[i].m_active = false;
        }
    }
}

// VERIFY: 58c5c9b7 auto
// OFFSET: 0x006B9050
void LoadAccountData(int32_t type, ACCOUNTDATALOADFUNC func, void* param) {
    AccountDataInfo& info = s_accountData[type];

    if (info.m_downloading) {
        info.m_loadFunc = func;
        info.m_loadParam = param;
        return;
    }

    HOSFILE localFile = OpenAccountFile(info.m_global, info.m_localFile, false);
    HOSFILE cacheFile = OpenAccountFile(info.m_global, info.m_cacheFile, false);

    func(localFile, cacheFile, param);

    if (localFile != OS_INVALID_FILE) {
        OsCloseFile(localFile);
    }

    if (cacheFile != OS_INVALID_FILE) {
        OsCloseFile(cacheFile);
    }
}

// VERIFY: ac27b46e auto
// OFFSET: 0x006B90E0
void SetClearConfigData(int32_t clear) {
    s_clearGlobalConfig = clear;
    s_clearCharacterConfig = clear;
}

// VERIFY: 61aba3c9 manual
// OFFSET: 0x006B9100
void UploadAccountData(int32_t type) {
    HOSFILE file = OpenAccountFile(s_accountData[type].m_global, s_accountData[type].m_cacheFile, false);
    uint32_t size = file == OS_INVALID_FILE ? 0 : static_cast<uint32_t>(OsGetFileSize(file));

    uint32_t time = s_accountDataTime++;

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_UPDATE_ACCOUNT_DATA));
    msg.Put(static_cast<uint32_t>(type));
    msg.Put(time);
    msg.Put(size);

    if (size) {
        void* allocated = nullptr;
        uint8_t* data;

        if (size > 0x1000) {
            allocated = SMemAlloc(size, __FILE__, __LINE__, 0);
            data = static_cast<uint8_t*>(allocated);
        } else {
            data = static_cast<uint8_t*>(alloca(size));
        }

        uint32_t bytesRead;
        OsReadFile(file, data, size, &bytesRead);

        uint32_t compressedSize = compressBound(size);
        uint8_t* compressedAllocated = nullptr;
        uint8_t* compressed;

        if (compressedSize > 0x2000) {
            compressedAllocated = static_cast<uint8_t*>(SMemAlloc(compressedSize, __FILE__, __LINE__, 0));
            compressed = compressedAllocated;
        } else {
            compressed = static_cast<uint8_t*>(alloca(compressedSize));
        }

        zlib_compress(compressed, &compressedSize, data, size, 1);
        msg.PutData(compressed, compressedSize);

        if (allocated) {
            SMemFree(allocated, __FILE__, __LINE__, 0);
        }

        if (compressedAllocated) {
            SMemFree(compressedAllocated, __FILE__, __LINE__, 0);
        }
    }

    msg.Finalize();
    ClientServices::Send2(&msg);

    if (file != OS_INVALID_FILE) {
        OsCloseFile(file);
    }

    UpdateCachedChecksum(type, time);
}

// VERIFY: 7b7683e2 manual
// OFFSET: 0x006B92C0
void DownloadAccountData(int32_t type) {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_REQUEST_ACCOUNT_DATA));
    msg.Put(static_cast<uint32_t>(type));
    msg.Finalize();
    ClientServices::Send2(&msg);

    s_accountData[type].m_downloading = true;
}

// VERIFY: e1dd25e9 manual
// OFFSET: 0x006B9340
static int32_t SaveTimerHandler(const void* data, void* param) {
    int32_t type = static_cast<int32_t>(reinterpret_cast<intptr_t>(param));
    AccountDataInfo& info = s_accountData[type];
    uint8_t digest[16];
    MD5AccountFile(info.m_cacheFile, digest, info.m_global);

    if (!memcmp(digest, info.m_md5, sizeof(digest))) {
        info.m_changed = false;
        info.m_time = info.m_serverTime;
        SaveCachedChecksums(info.m_global);
    } else {
        UploadAccountData(type);
    }

    info.m_saveTimer = 0;
    return 1;
}

// VERIFY: c002fa95 auto
// OFFSET: 0x006B9440
static void ScheduleAccountDataSave(int32_t type) {
    AccountDataInfo& info = s_accountData[type];
    info.m_changed = true;

    if (!s_cvSynchronizeSettings->m_intValue) {
        return;
    }

    CVar* cvar = CVar::Lookup(info.m_cvarName);

    if ((cvar && !cvar->m_intValue) || info.m_localOnly) {
        return;
    }

    if (info.m_saveTimer) {
        ClientKillTimer(info.m_saveTimer, SaveTimerHandler, "SaveTimerHandler");
        info.m_saveTimer = 0;
    }

    info.m_saveTimer = ClientSetTimer(30000, SaveTimerHandler, reinterpret_cast<void*>(static_cast<intptr_t>(type)));
}

// VERIFY: fe34ba05 manual
// OFFSET: 0x006B94C0
static int32_t Packet_SMSG_ACCOUNT_DATA_TIMES(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t serverTime = 0;
    msg->Get(serverTime);
    s_accountDataTime = serverTime;

    uint8_t unk;
    msg->Get(unk);

    uint32_t mask;
    msg->Get(mask);

    uint32_t count = 0;

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        if (mask & (1 << i)) {
            count++;
        }
    }

    if (msg->m_size - msg->m_read != count * sizeof(uint32_t)) {
        return 1;
    }

    for (int32_t type = 0; type < static_cast<int32_t>(sizeof(s_accountData) / sizeof(s_accountData[0])); type++) {
        AccountDataInfo& info = s_accountData[type];

        if (info.m_localOnly) {
            if (mask & (1 << type)) {
                uint32_t discard = 0;
                msg->Get(discard);
            }
        } else {
            if (!(mask & (1 << type))) {
                continue;
            }

            uint32_t typeTime = 0;
            msg->Get(typeTime);
            info.m_serverTime = typeTime;
        }

        if (!info.m_active) {
            continue;
        }

        if (!unk && info.m_global && info.m_serverTime < info.m_time) {
            info.m_serverTime = info.m_time;
        }

        info.m_downloading = false;

        if (s_cvSynchronizeSettings->m_intValue) {
            CVar* var = CVar::Lookup(info.m_cvarName);

            if ((!var || var->m_intValue) && !info.m_localOnly) {
                if (info.m_changed || info.m_time > info.m_serverTime) {
                    UploadAccountData(type);
                } else if (info.m_time < info.m_serverTime) {
                    DownloadAccountData(type);
                }
            }
        }

        if (!info.m_downloading && info.m_loadFunc) {
            LoadAccountData(type, info.m_loadFunc, info.m_loadParam);
        }
    }

    return 1;
}

// VERIFY: d5e85999 manual
// OFFSET: 0x006B9730
static int32_t Packet_SMSG_UPDATE_ACCOUNT_DATA(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    WOWGUID guid;
    msg->Get(guid.m_guid);

    uint32_t type;
    msg->Get(type);

    AccountDataInfo& info = s_accountData[type];

    if ((guid != CGlueMgr::GetActive() && !info.m_global) || !info.m_downloading) {
        msg->m_read = msg->m_size;
        return 1;
    }

    uint32_t serverTime = 0;
    msg->Get(serverTime);

    if (info.m_changed || info.m_time < static_cast<int32_t>(serverTime)) {
        HOSFILE file = OpenAccountFile(info.m_global, info.m_cacheFile, true);

        if (file != OS_INVALID_FILE) {
            uint32_t size;
            msg->Get(size);

            if (size) {
                uint32_t compressedSize = msg->m_size - msg->m_read;
                void* compressed;
                msg->GetDataInSitu(compressed, compressedSize);

                uint8_t* buffer;
                bool stackBuffer;

                if (size >= 0x2000) {
                    buffer = static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0));
                    stackBuffer = false;
                } else {
                    buffer = static_cast<uint8_t*>(alloca(size));
                    stackBuffer = true;
                }

                uint32_t uncompressedSize = size;

                if (!zlib_uncompress(buffer, &uncompressedSize, static_cast<const uint8_t*>(compressed), compressedSize)) {
                    uint32_t bytesWritten;
                    OsWriteFile(file, buffer, uncompressedSize, &bytesWritten);
                }

                if (!stackBuffer) {
                    SMemFree(buffer, __FILE__, __LINE__, 0);
                }
            }

            OsCloseFile(file);
            UpdateCachedChecksum(type, serverTime);
        }
    } else {
        msg->m_read = msg->m_size;
    }

    info.m_downloading = false;

    if (info.m_loadFunc) {
        LoadAccountData(type, info.m_loadFunc, info.m_loadParam);
    }

    return 1;
}

// VERIFY: d11d72c6 auto
// OFFSET: 0x006B9900
void AccountDataInitializeBasicSystem() {
    s_cvSynchronizeSettings = CVar::Register("synchronizeSettings", "Whether client settings should be stored on the server", 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    ClientServices::SetMessageHandler(SMSG_ACCOUNT_DATA_TIMES, Packet_SMSG_ACCOUNT_DATA_TIMES, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_ACCOUNT_DATA, Packet_SMSG_UPDATE_ACCOUNT_DATA, nullptr);
    ClientServices::SetMessageHandler(SMSG_UPDATE_ACCOUNT_DATA_COMPLETE, Packet_SMSG_UPDATE_ACCOUNT_DATA_COMPLETE, nullptr);
    ClientServices::SetMessageHandler(SMSG_CHECK_FOR_BOTS, Packet_SMSG_CHECK_FOR_BOTS, nullptr);
}

// VERIFY: 04a1fda3 auto
// OFFSET: 0x006B8FF0
void AccountDataShutdownBasicSystem() {
    ClientServices::ClearMessageHandler(SMSG_ACCOUNT_DATA_TIMES);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_ACCOUNT_DATA);
    ClientServices::ClearMessageHandler(SMSG_UPDATE_ACCOUNT_DATA_COMPLETE);
    ClientServices::ClearMessageHandler(SMSG_CHECK_FOR_BOTS);
}

// VERIFY: bc2420a0 manual
// OFFSET: 0x006B9970
void AccountDataInitialize(bool global) {
    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        CVar::Register(s_accountData[i].m_cvarName, nullptr, 0, "1", nullptr, CATEGORY_DEFAULT, false, nullptr, false);
    }

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];

        if (info.m_global == global) {
            info.m_downloading = true;
            memset(info.m_md5, 0, sizeof(info.m_md5));
            info.m_time = 0;
            info.m_serverTime = 0;
            info.m_active = true;
        }
    }

    LoadCachedChecksums(global);

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];

        if (info.m_global != global) {
            continue;
        }

        HOSFILE file = OpenAccountFile(info.m_global, info.m_oldFile, false);

        if (file == OS_INVALID_FILE) {
            CopyAccountFile(info.m_global, info.m_cacheFile, info.m_oldFile);
        } else {
            OsCloseFile(file);
        }
    }

    uint32_t bytesWritten;

    if (global) {
        if (s_clearGlobalConfig) {
            HOSFILE file = OpenAccountFile(s_accountData[0].m_global, s_accountData[0].m_cacheFile, true);

            if (file != OS_INVALID_FILE) {
                OsWriteFile(file, "", 0, &bytesWritten);
                OsCloseFile(file);
            }

            s_accountData[0].m_changed = true;
            s_accountData[0].m_downloading = false;
        }
    } else if (s_clearCharacterConfig) {
        HOSFILE file = OpenAccountFile(s_accountData[1].m_global, s_accountData[1].m_cacheFile, true);

        if (file != OS_INVALID_FILE) {
            OsWriteFile(file, "", 0, &bytesWritten);
            OsCloseFile(file);
        }

        s_accountData[1].m_downloading = false;
        s_accountData[1].m_changed = true;
    }
}

// VERIFY: 8182067a auto
// OFFSET: 0x006B9B90
void AccountDataFlush() {
    for (int32_t i = 0; i < static_cast<int32_t>(sizeof(s_accountData) / sizeof(s_accountData[0])); i++) {
        if (s_accountData[i].m_saveTimer) {
            ClientKillTimer(s_accountData[i].m_saveTimer, SaveTimerHandler, "SaveTimerHandler");
            SaveTimerHandler(nullptr, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
        }
    }
}

// VERIFY: aad4b10d auto
// OFFSET: 0x006B9BE0
void SaveAccountData(int32_t type, ACCOUNTDATALOADFUNC func, void* param) {
    AccountDataInfo& info = s_accountData[type];

    if (info.m_downloading) {
        return;
    }

    HOSFILE localFile = OpenAccountFile(info.m_global, info.m_localFile, true);
    HOSFILE cacheFile = OpenAccountFile(info.m_global, info.m_cacheFile, true);
    func(localFile, cacheFile, param);

    if (localFile != OS_INVALID_FILE) {
        OsCloseFile(localFile);
    }

    if (cacheFile != OS_INVALID_FILE) {
        OsCloseFile(cacheFile);
    }

    ScheduleAccountDataSave(type);
}

// VERIFY: 7c0bceb5 manual
// OFFSET: 0x006B9C60
void UploadAccountData() {
    AccountDataFlush();

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];
        info.m_downloading = false;

        if (info.m_changed || info.m_time != info.m_serverTime) {
            UploadAccountData(i);
        }
    }
}

// VERIFY: 5a1491a5 manual
// OFFSET: 0x006B9CA0
void DownloadAccountData() {
    AccountDataFlush();

    for (uint32_t i = 0; i < sizeof(s_accountData) / sizeof(s_accountData[0]); i++) {
        AccountDataInfo& info = s_accountData[i];

        if ((info.m_changed || info.m_time != info.m_serverTime) && !info.m_downloading) {
            DownloadAccountData(i);
        }
    }
}
