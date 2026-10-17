#include "BannedAddOns.h"
#include <cstring>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"

// STATIC: 0x00A1D648
static const uint8_t s_emptyMd5[16] = { 0xD4, 0x1D, 0x8C, 0xD9, 0x8F, 0x00, 0xB2, 0x04, 0xE9, 0x80, 0x09, 0x98, 0xEC, 0xF8, 0x42, 0x7E };

// STATIC: 0x00C243E4
static int32_t s_bannedAddOnCount;

// STATIC: 0x00C243E8
static TSHashTable<_BANNEDADDON, HASHKEY_NONE> s_bannedAddOns;

// VERIFY: a077583e manual
// OFFSET: 0x005F3410
static void CopyMd5(uint8_t* dest, const uint8_t* source) {
    for (int32_t i = 0; i < 16; i++) {
        dest[i] = source[i];
    }
}

// VERIFY: 4af0142f auto
// OFFSET: 0x005F34B0
BannedAddOnCache::BannedAddOnCache() {
    this->m_id = 0;
    this->m_lastModified = 0;
    memcpy(this->m_nameMd5, s_emptyMd5, sizeof(this->m_nameMd5));
    memcpy(this->m_versionMd5, s_emptyMd5, sizeof(this->m_versionMd5));
}

// VERIFY: 3f30b207 auto
// OFFSET: 0x005F3510
BannedAddOnCache& BannedAddOnCache::operator=(const BannedAddOnCache& source) {
    this->m_id = source.m_id;
    memcpy(this->m_nameMd5, source.m_nameMd5, sizeof(this->m_nameMd5));
    memcpy(this->m_versionMd5, source.m_versionMd5, sizeof(this->m_versionMd5));
    this->m_lastModified = source.m_lastModified;
    this->m_flags = source.m_flags;

    return *this;
}

// VERIFY: 7e868c2d auto
// OFFSET: 0x005F3560
static HOSFILE OpenCacheFile(const char* directory, const char* name, int32_t write) {
    if (!name) {
        return OS_INVALID_FILE;
    }

    char path[260];

    if (directory) {
        SStrCopy(path, directory, sizeof(path));

        if (write && !OsDirectoryExists(path)) {
            OsCreateDirectory(path, 1);
        }

        SStrPack(path, "\\", sizeof(path));
        SStrPack(path, name, sizeof(path));
    } else {
        SStrCopy(path, name, sizeof(path));
    }

    if (write) {
        return OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
    }

    return OsCreateFile(path, OS_GENERIC_READ, 0, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);
}

// VERIFY: 2a160efa auto
// OFFSET: 0x005F3630
static int32_t DeleteCacheFile(const char* directory, const char* name) {
    if (!name) {
        return 0;
    }

    char path[260];

    if (directory && OsDirectoryExists(directory)) {
        SStrCopy(path, directory, sizeof(path));
        SStrPack(path, "\\", sizeof(path));
        SStrPack(path, name, sizeof(path));
    } else {
        SStrCopy(path, name, sizeof(path));
    }

    return OsDeleteFile(path);
}

// VERIFY: f9298e8e auto
// OFFSET: 0x005F36D0
static bool ReadCacheRecord(HOSFILE file, BannedAddOnCache* record, uint32_t* bytes) {
    return file != OS_INVALID_FILE && OsReadFile(file, &record->m_id, sizeof(record->m_id), bytes) && OsReadFile(file, record->m_nameMd5, sizeof(record->m_nameMd5), bytes) && OsReadFile(file, record->m_versionMd5, sizeof(record->m_versionMd5), bytes) && OsReadFile(file, &record->m_lastModified, sizeof(record->m_lastModified), bytes) && OsReadFile(file, &record->m_flags, sizeof(record->m_flags), bytes);
}

// VERIFY: 3f13756e auto
// OFFSET: 0x005F3750
static bool WriteCacheRecord(HOSFILE file, const BannedAddOnCache* record, uint32_t* bytes) {
    return file != OS_INVALID_FILE && OsWriteFile(file, &record->m_id, sizeof(record->m_id), bytes) && OsWriteFile(file, record->m_nameMd5, sizeof(record->m_nameMd5), bytes) && OsWriteFile(file, record->m_versionMd5, sizeof(record->m_versionMd5), bytes) && OsWriteFile(file, &record->m_lastModified, sizeof(record->m_lastModified), bytes) && OsWriteFile(file, &record->m_flags, sizeof(record->m_flags), bytes);
}

// VERIFY: 064159a5 manual
// OFFSET: 0x005F3820
static int32_t SaveCache() {
    char directory[260];

    if (CheckDataFilesExist()) {
        SStrPrintf(directory, sizeof(directory), "Cache/%s/%s", "WDB", s_localeCode);
    } else {
        SStrCopy(directory, "WDB", sizeof(directory));
    }

    HOSFILE file = OpenCacheFile(directory, "baddons.wcf", 1);

    if (file == OS_INVALID_FILE) {
        return 0;
    }

    uint32_t total = 0;

    for (_BANNEDADDON* entry = s_bannedAddOns.m_fulllist.Head(); entry; entry = s_bannedAddOns.m_fulllist.Next(entry)) {
        if (entry->m_fromDB) {
            continue;
        }

        uint32_t bytes;

        if (!WriteCacheRecord(file, &entry->m_cache, &bytes)) {
            OsCloseFile(file);
            DeleteCacheFile(directory, "baddons.wcf");
            return 0;
        }

        total += bytes;
    }

    OsCloseFile(file);

    if (!total) {
        return DeleteCacheFile(directory, "baddons.wcf");
    }

    return 1;
}

// VERIFY: 8b69e7ae auto
// OFFSET: 0x005F3940
int32_t SaveBannedAddOns() {
    if (!s_bannedAddOnCount) {
        return 0;
    }

    return SaveCache();
}

// VERIFY: a78036c1 manual
// OFFSET: 0x005F3960
int32_t GetLatestBannedAddOnTimestamp() {
    int32_t latest = 0;

    for (_BANNEDADDON* entry = s_bannedAddOns.m_fulllist.Head(); entry; entry = s_bannedAddOns.m_fulllist.Next(entry)) {
        if (latest < entry->m_cache.m_lastModified) {
            latest = entry->m_cache.m_lastModified;
        }
    }

    return latest;
}

// VERIFY: 36153782 manual
// OFFSET: 0x005F3B10
BannedAddOnCache* FindBannedAddOn(const char* name, const char* version) {
    if (!name) {
        name = "";
    }

    MD5_CTX context;
    uint8_t nameMd5[16];
    MD5Init(&context);
    MD5Update(&context, reinterpret_cast<const uint8_t*>(name), SStrLen(name));
    MD5Final(nameMd5, &context);

    if (!version) {
        version = "";
    }

    uint8_t versionMd5[16];
    MD5Init(&context);
    MD5Update(&context, reinterpret_cast<const uint8_t*>(version), SStrLen(version));
    MD5Final(versionMd5, &context);

    for (_BANNEDADDON* entry = s_bannedAddOns.m_fulllist.Head(); entry; entry = s_bannedAddOns.m_fulllist.Next(entry)) {
        BannedAddOnCache* cache = &entry->m_cache;

        if ((cache->m_flags & 1) && !memcmp(cache->m_nameMd5, nameMd5, sizeof(nameMd5)) && (!memcmp(cache->m_versionMd5, s_emptyMd5, sizeof(s_emptyMd5)) || !memcmp(cache->m_versionMd5, versionMd5, sizeof(versionMd5)))) {
            return cache;
        }
    }

    return nullptr;
}

// VERIFY: 362d92d8 manual
// OFFSET: 0x005F3D90
bool IsBannedAddOn(const char* name, const char* version) {
    return FindBannedAddOn(name, version) != nullptr;
}

// VERIFY: b9ccbec7 auto
// OFFSET: 0x005F3EB0
void DestroyBannedAddOns() {
    s_bannedAddOns.InternalClear(0);
    s_bannedAddOnCount = 0;
}

// VERIFY: 8cb64958 manual
// OFFSET: 0x005F44E0
static _BANNEDADDON* GetBannedAddOn(int32_t id, const int32_t* lastModified, int32_t* updated) {
    if (!id) {
        if (updated) {
            *updated = 0;
        }

        return nullptr;
    }

    HASHKEY_NONE key;
    _BANNEDADDON* entry = s_bannedAddOns.Ptr(id, key);

    if (entry) {
        if (lastModified && updated) {
            *updated = entry->m_cache.m_lastModified <= *lastModified;
        }

        return entry;
    }

    entry = s_bannedAddOns.New(id, key, 0, 0);

    if (updated) {
        *updated = 1;
    }

    s_bannedAddOnCount++;

    return entry;
}

// VERIFY: 4426e36a auto
// OFFSET: 0x005F45B0
static void LoadDatabase() {
    for (int32_t i = 0; i < g_bannedAddOnsDB.m_numRecords; i++) {
        BannedAddOnsRec* record = &g_bannedAddOnsDB.m_records[i];

        if (!record) {
            continue;
        }

        int32_t lastModified = record->m_lastModified;
        int32_t updated;
        _BANNEDADDON* entry = GetBannedAddOn(record->m_id, &lastModified, &updated);

        if (entry && updated) {
            entry->m_fromDB = 1;
            entry->m_cache.m_id = record->m_id;
            CopyMd5(entry->m_cache.m_nameMd5, reinterpret_cast<const uint8_t*>(record->m_nameMd5));
            CopyMd5(entry->m_cache.m_versionMd5, reinterpret_cast<const uint8_t*>(record->m_versionMd5));
            entry->m_cache.m_lastModified = record->m_lastModified;
            entry->m_cache.m_flags = record->m_flags;
        }
    }
}

// VERIFY: f8889ed5 manual
// OFFSET: 0x005F4660
static int32_t LoadCache() {
    char directory[260];

    if (CheckDataFilesExist()) {
        SStrPrintf(directory, sizeof(directory), "Cache/%s/%s", "WDB", s_localeCode);
    } else {
        SStrCopy(directory, "WDB", sizeof(directory));
    }

    char path[260];
    SStrCopy(path, directory, sizeof(path));
    SStrPack(path, "\\", sizeof(path));
    SStrPack(path, "baddons.wcf", sizeof(path));

    HOSFILE file = OsCreateFile(path, OS_GENERIC_READ, 0, OS_OPEN_EXISTING, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        return 0;
    }

    TSList<_BANNEDADDONLOADNODE> records;
    uint32_t bytes;

    do {
        _BANNEDADDONLOADNODE* node = records.NewNode(2, 0, 8);

        if (!ReadCacheRecord(file, &node->m_cache, &bytes)) {
            records.Clear();
            OsCloseFile(file);
            return 0;
        }
    } while (bytes);

    OsCloseFile(file);

    for (_BANNEDADDONLOADNODE* node = records.Head(); node; node = records.Next(node)) {
        int32_t updated;
        _BANNEDADDON* entry = GetBannedAddOn(node->m_cache.m_id, &node->m_cache.m_lastModified, &updated);

        if (entry && updated) {
            entry->m_fromDB = 0;
            entry->m_cache = node->m_cache;
        }
    }

    records.Clear();

    return 1;
}

// VERIFY: 62709344 auto
// OFFSET: 0x005F48D0
int32_t LoadBannedAddOns() {
    LoadDatabase();

    return LoadCache();
}

// VERIFY: 66bc1b4c manual
// OFFSET: 0x005F48E0
BannedAddOnCache* RegisterBannedAddOn(int32_t id, const int32_t* lastModified, int32_t* updated) {
    _BANNEDADDON* entry = GetBannedAddOn(id, lastModified, updated);

    if (!entry) {
        return nullptr;
    }

    entry->m_fromDB = 0;

    return &entry->m_cache;
}
