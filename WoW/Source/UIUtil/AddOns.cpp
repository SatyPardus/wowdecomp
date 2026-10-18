#include "AddOns.h"
#include <cstdlib>
#include <cstring>
#if defined(__APPLE__)
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/UIUtil/BannedAddOns.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "ThirdParty/zlib/zlib.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/Ui/UIBindings.h"

// STATIC: 0x00AD17B4
static const char* s_addOnLoadableTokens[12] = { "LOADABLE", "MISSING", "DISABLED", "BANNED", "CORRUPT", "INSECURE", "DEMAND_LOADED", "INTERFACE_VERSION", "INCOMPATIBLE", "SECURE", "INSECURE", "BANNED" };

// STATIC: 0x00AD17A8
static TSList<ADDONSTATELIST> s_addOnStates;

// STATIC: 0x00C2491C
static TSHashTable<UIADDON, HASHKEY_STRI> s_addons;

// STATIC: 0x00C24944
static TSGrowableArray<const char*> s_addOnInfo;

// STATIC: 0x00C24410
static uint32_t s_dataInterfaceVersion;

// STATIC: 0x00C24418
static char s_addOnAccountName[1280];

// STATIC: 0x00C24918
static bool s_addOnsInitialized;

// VERIFY: e4678852 auto
// OFFSET: 0x005F4910
static void ReadDataInterfaceVersion() {
    void* data;

    if (!SFile::ReadFile(nullptr, "Interface\\FrameXML\\FrameXML.toc", &data, nullptr, 1, 1, nullptr)) {
        return;
    }

    const char* text = static_cast<const char*>(data);

    if (static_cast<uint8_t>(text[0]) == 0xEF && static_cast<uint8_t>(text[1]) == 0xBB && static_cast<uint8_t>(text[2]) == 0xBF) {
        text += 3;
    }

    char line[1024];

    do {
        SStrTokenize(&text, line, sizeof(line), "\r\n", nullptr);

        if (!*text) {
            break;
        }

        if (line[0] == '#' && line[1] == '#') {
            const char* value = &line[2];

            while (*value == ' ') {
                value++;
            }

            if (!SStrCmpI(value, "Interface: ", SStrLen("Interface: "))) {
                while (*value != ' ') {
                    value++;
                }

                while (*value == ' ') {
                    value++;
                }

                s_dataInterfaceVersion = SStrToUnsigned(value);
                break;
            }
        } else if (!line[0]) {
            break;
        }
    } while (*text);

    SFile::Unload(data);
}

// VERIFY: 99fcd940 auto
// OFFSET: 0x005F4C10
uint32_t GetCodeInterfaceVersion() {
    return 30300;
}

// VERIFY: 75e0f3ca auto
// OFFSET: 0x005F4C20
uint32_t GetDataInterfaceVersion() {
    if (!s_dataInterfaceVersion) {
        ReadDataInterfaceVersion();
    }

    return s_dataInterfaceVersion;
}

// VERIFY: cf4c0a28 auto
// OFFSET: 0x005F4C40
char* AddOnUnloadableToken(UIADDON_LOADABLE_REASON reason, UIADDON_LOADABLE_REASON dependencyReason, char* buffer, uint32_t size) {
    if (reason) {
        SStrCopy(buffer, s_addOnLoadableTokens[reason], size);
    } else {
        SStrPrintf(buffer, size, "DEP_%s", s_addOnLoadableTokens[dependencyReason]);
    }

    return buffer;
}

// VERIFY: 200fce53 auto
// OFFSET: 0x005F4C90
const char* AddOnSecurityToken(int32_t security) {
    return s_addOnLoadableTokens[UIADDON_SECURE + security];
}

// VERIFY: 6c5d7816 auto
// OFFSET: 0x005F4CE0
static int32_t DeleteInterfaceAddOns(OS_FILE_DATA& data, void* param) {
    char path[260];
    SStrPrintf(path, sizeof(path), "%s%s\\%s.toc", "Interface\\AddOns\\", data.fileName, data.fileName);

    if (OsFileExists(path)) {
        SStrPrintf(path, sizeof(path), "%s%s", "Interface\\AddOns\\", data.fileName);

        char oldPath[260];
        SStrPrintf(oldPath, sizeof(oldPath), "%s.old", path);
        OsRemoveDirectoryRecurse(oldPath, 1);
        OsMoveFile(path, oldPath);
        *static_cast<bool*>(param) = true;
    }

    return 0;
}

// VERIFY: 012df8c8 auto
// OFFSET: 0x005F4D90
void DeleteInterfaceFiles() {
    bool deleted = false;
    const char* directories[] = { "Interface\\GlueXML", "Interface\\FrameXML" };

    for (uint32_t i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        const char* directory = directories[i];

        if (OsDirectoryExists(directory)) {
            char oldPath[260];
            SStrPrintf(oldPath, sizeof(oldPath), "%s.old", directory);
            OsRemoveDirectoryRecurse(oldPath, 1);
            OsMoveFile(directory, oldPath);
            deleted = true;
        }
    }

    OsFileList("Interface\\AddOns\\", "Blizzard*", DeleteInterfaceAddOns, &deleted, 0);

    if (deleted) {
        SFile::RebuildHash();
    }
}

// VERIFY: 0b8b0392 auto
// OFFSET: 0x005F4A30
static void ReadPublicKey(bool* hasKey, uint8_t* key, const char* path) {
    *hasKey = false;
    memset(key, 0, 256);

    void* data;
    uint32_t size;

    if (!SFile::ReadFile(nullptr, path, &data, &size, 0, 1, nullptr)) {
        return;
    }

    if (size == 257) {
        *hasKey = *static_cast<uint8_t*>(data);
        memcpy(key, static_cast<uint8_t*>(data) + 1, 256);
    }

    SFile::Unload(data);
}

// VERIFY: 1d813ba4 auto
// OFFSET: 0x005F4AA0
static void WritePublicKey(bool hasKey, const uint8_t* key, const char* path) {
    HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        return;
    }

    uint32_t bytes;
    OsWriteFile(file, &hasKey, 1, &bytes);
    OsWriteFile(file, key, 256, &bytes);
    OsCloseFile(file);
}

// VERIFY: 8423ee34 manual
// OFFSET: 0x005F4B00
static void ReadURL(const char* path, char** url) {
    void* data;

    if (!SFile::ReadFile(nullptr, path, &data, nullptr, 1, 1, nullptr)) {
        return;
    }

    char* value = SStrChr(static_cast<char*>(data), '=');

    if (value) {
        *url = SStrDupA(value + 1, __FILE__, __LINE__);

        for (char* end = *url + SStrLen(*url) - 1; end >= *url; end--) {
            if (*end != '\r' && *end != '\n' && *end != ' ') {
                break;
            }

            *end = '\0';
        }
    }

    SFile::Unload(data);
}

// INLINED
UIADDON::METADATA::~METADATA() {
    if (this->m_value) {
        SMemFree(this->m_value, __FILE__, __LINE__, 0);
    }
}

// VERIFY: e761f62e auto
// OFFSET: 0x005F6FF0
ADDONSTATELIST::~ADDONSTATELIST() {
    if (this->m_name) {
        SMemFree(this->m_name, __FILE__, __LINE__, 0);
    }

    this->m_states.Clear();
}

// VERIFY: e9986d57 auto
// OFFSET: 0x005F4B80
static void WriteURL(const char* path, const char* url) {
    if (!url || !*url) {
        OsDeleteFile(path);
        return;
    }

    HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

    if (file == OS_INVALID_FILE) {
        return;
    }

    char text[1024];
    SStrPrintf(text, sizeof(text), "[InternetShortcut]\r\nURL=%s\r\n", url);

    uint32_t bytes;
    OsWriteFile(file, text, SStrLen(text), &bytes);
    OsCloseFile(file);
}

// VERIFY: 56328f5e auto
// OFFSET: 0x005F4EC0
void UIADDON::LoadPublicKey() {
    char path[260];
    SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\%s.pub", this->m_key.m_str, this->m_key.m_str);
    ReadPublicKey(&this->m_hasPublicKey, this->m_publicKey, path);
}

// VERIFY: 8cfec81c auto
// OFFSET: 0x005F4F10
void UIADDON::SavePublicKey() {
    const char* name = this->m_key.m_str;

    char path[260];
    SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s", name);

    if (!OsDirectoryExists(path)) {
        OsCreateDirectory(path, 1);
    }

    uint32_t length = SStrLen(path);
    SStrPrintf(&path[length], sizeof(path) - length, "\\%s.pub", name);
    WritePublicKey(this->m_hasPublicKey, this->m_publicKey, path);
}

// VERIFY: 36413c7e auto
// OFFSET: 0x005F4FB0
void UIADDON::LoadURL() {
    char path[260];
    SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\%s.url", this->m_key.m_str, this->m_key.m_str);
    ReadURL(path, &this->m_url);
}

// VERIFY: be16ab36 auto
// OFFSET: 0x005F4FF0
uint32_t GetNumAddOns() {
    return s_addOnInfo.Count();
}

// VERIFY: cdb080a3 manual
// OFFSET: 0x005F5000
const char* GetAddOnByIndex(uint32_t index) {
    if (index < s_addOnInfo.Count()) {
        return s_addOnInfo[index];
    }

    return nullptr;
}

// VERIFY: af71fe0c manual
// OFFSET: 0x005F5440
void GetAddOnInfoRequest(CDataStore* msg) {
    if (!s_addons.m_fulllist.Head()) {
        return;
    }

    LoadBannedAddOns();

    CDataStore info;
    uint32_t count = 0;
    info.Put(static_cast<uint32_t>(0));

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        if (!addon->m_secure) {
            continue;
        }

        info.PutString(addon->m_key.m_str);
        info.Put(static_cast<uint8_t>(addon->m_hasPublicKey));
        info.Put(addon->m_hasPublicKey ? SCrcCompute(addon->m_publicKey, sizeof(addon->m_publicKey), nullptr, 7) : 0u);
        info.Put(addon->m_url && *addon->m_url ? SCrcCompute(addon->m_url, SStrLen(addon->m_url), nullptr, 7) : 0u);
        count++;
    }

    info.Set(0, count);
    info.Put(static_cast<uint32_t>(GetLatestBannedAddOnTimestamp()));

    uint32_t size = info.m_size;
    info.Finalize();
    msg->Put(info.m_size);

    void* data;
    info.GetDataInSitu(data, size);

    uint32_t compressedSize = compressBound(size);
    uint8_t* allocated = nullptr;
    uint8_t* buffer;

    if (compressedSize <= 0x2000) {
        buffer = static_cast<uint8_t*>(alloca(compressedSize));
    } else {
        allocated = static_cast<uint8_t*>(SMemAlloc(compressedSize, __FILE__, __LINE__, 0));
        buffer = allocated;
    }

    zlib_compress(buffer, &compressedSize, static_cast<const uint8_t*>(data), size, -1);
    msg->PutData(buffer, compressedSize);

    if (allocated) {
        SMemFree(allocated, __FILE__, __LINE__, 0);
    }
}

// VERIFY: f6241796 manual
// OFFSET: 0x005F6C20
UIADDON::UIADDON() {
    this->m_loading = false;
    this->m_loaded = 0;
    this->m_interfaceVersion = 0;
    this->m_revision = 0;
    this->m_secure = false;
    this->m_blizzard = 0;
    this->m_signed = 0;
    this->m_loadOnDemand = false;
    this->m_checkingLoadable = 0;
    this->m_url = nullptr;
    this->m_numFiles = 0;
    this->m_status = 1;
    this->m_enabledByDefault = true;
    this->m_outOfDate = 0;
    this->m_hasPublicKey = false;
}

// VERIFY: 7ce09d4d manual
// OFFSET: 0x005F6CF0
UIADDON::~UIADDON() {
    if (this->m_url) {
        SMemFree(this->m_url, __FILE__, __LINE__, 0);
    }

    for (uint32_t i = 0; i < this->m_optionalDeps.Count(); i++) {
        SMemFree(this->m_optionalDeps[i], __FILE__, __LINE__, 0);
    }

    this->m_optionalDeps.Clear();

    for (uint32_t i = 0; i < this->m_requiredDeps.Count(); i++) {
        SMemFree(this->m_requiredDeps[i], __FILE__, __LINE__, 0);
    }

    this->m_requiredDeps.Clear();

    for (uint32_t i = 0; i < this->m_loadWith.Count(); i++) {
        SMemFree(this->m_loadWith[i], __FILE__, __LINE__, 0);
    }

    this->m_loadWith.Clear();

    for (uint32_t i = 0; i < this->m_loadManagers.Count(); i++) {
        SMemFree(this->m_loadManagers[i], __FILE__, __LINE__, 0);
    }

    this->m_loadManagers.Clear();

    for (uint32_t i = 0; i < this->m_savedVariables.Count(); i++) {
        SMemFree(this->m_savedVariables[i], __FILE__, __LINE__, 0);
    }

    this->m_savedVariables.Clear();

    for (uint32_t i = 0; i < this->m_savedVariablesPerCharacter.Count(); i++) {
        SMemFree(this->m_savedVariablesPerCharacter[i], __FILE__, __LINE__, 0);
    }

    this->m_savedVariablesPerCharacter.Clear();
    this->m_loadWithMe.Clear();
    this->m_metadata.Clear();
}

// VERIFY: c3d9c443 auto
// OFFSET: 0x005F7410
const char* GetAddOnName(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return addon->m_key.m_str;
    }

    return nullptr;
}

// VERIFY: d2ea37a9 auto
// OFFSET: 0x005F7430
const char* GetAddOnTitle(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        UIADDON::METADATA* metadata = addon->m_metadata.Ptr("Title");

        if (metadata) {
            return metadata->m_value;
        }
    }

    return nullptr;
}

// VERIFY: 3bd36ab0 auto
// OFFSET: 0x005F7470
const char* GetAddOnNotes(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        UIADDON::METADATA* metadata = addon->m_metadata.Ptr("Notes");

        if (metadata) {
            return metadata->m_value;
        }
    }

    return nullptr;
}

// VERIFY: 8330d5de auto
// OFFSET: 0x005F74E0
const char* GetAddOnMetadata(const char* name, const char* key) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        UIADDON::METADATA* metadata = addon->m_metadata.Ptr(key);

        if (metadata) {
            return metadata->m_value;
        }
    }

    return nullptr;
}

// VERIFY: 17c36db3 manual
// OFFSET: 0x005F7520
TSGrowableArray<char*>* GetAddonDeps(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return &addon->m_requiredDeps;
    }

    return nullptr;
}

// VERIFY: dad9fd3c manual
// OFFSET: 0x005F7570
static int32_t ComputeAddOnEnableState(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (!addon) {
        return 0;
    }

    uint32_t total = 0;
    uint32_t enabled = 0;

    for (ADDONSTATELIST* list = s_addOnStates.Head(); list; list = s_addOnStates.Next(list)) {
        ADDONSTATE* state = list->m_states.Ptr(name);

        if (state) {
            total++;

            if (state->m_enabled) {
                enabled++;
            }
        }
    }

    if (!total) {
        return addon->m_enabledByDefault ? 2 : 0;
    }

    if (!enabled) {
        return 0;
    }

    return 2 - (enabled < total);
}

// VERIFY: 8a5c7a5a auto
// OFFSET: 0x005F7610
bool IsAddOnLoadOnDemand(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return addon->m_loadOnDemand;
    }

    return false;
}

// VERIFY: 5af7c1e1 auto
// OFFSET: 0x005F7630
bool IsAddOnLoading(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return addon->m_loading;
    }

    return false;
}

// VERIFY: 159044be auto
// OFFSET: 0x005F7650
bool IsAddOnLoaded(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return addon->m_loaded;
    }

    return false;
}

// VERIFY: 348010f6 manual
// OFFSET: 0x005F7840
void ClearAddOnEnableState(const char* character) {
    ADDONSTATELIST* list = s_addOnStates.Head();

    while (!TSList<ADDONSTATELIST, TSGetLink<ADDONSTATELIST> >::IsEnd(list)) {
        ADDONSTATELIST* next = s_addOnStates.RawNext(list);

        if (!character || !SStrCmpI(character, list->m_name, STORM_MAX_STR)) {
            s_addOnStates.DeleteNode(list);

            if (character) {
                break;
            }
        }

        list = next;
    }
}

// VERIFY: 6dbd0218 manual
// OFFSET: 0x005F78B0
void ShutdownAddOns() {
    if (!s_addOnsInitialized) {
        return;
    }

    s_addons.InternalClear(0);
    s_addOnInfo.Clear();
    s_addOnStates.Clear();
    s_addOnsInitialized = false;
}

// VERIFY: 6a73058e auto
// OFFSET: 0x005F7910
static int32_t QSortAddOns(const void* a, const void* b) {
    const char* name1 = *static_cast<const char* const*>(a);
    const char* name2 = *static_cast<const char* const*>(b);

    const char* title1 = nullptr;
    UIADDON* addon1 = s_addons.Ptr(name1);

    if (addon1) {
        UIADDON::METADATA* title = addon1->m_metadata.Ptr("Title");
        title1 = title ? title->m_value : nullptr;
    }

    const char* title2 = nullptr;
    UIADDON* addon2 = s_addons.Ptr(name2);

    if (addon2) {
        UIADDON::METADATA* title = addon2->m_metadata.Ptr("Title");
        title2 = title ? title->m_value : nullptr;
    }

    if (!title1) {
        title1 = name1;
    }

    if (!title2) {
        title2 = name2;
    }

    return SStrCmpI(title1, title2, STORM_MAX_STR);
}

// VERIFY: 63f3a852 manual
// OFFSET: 0x005F79A0
void SetAddOnInfoRequest(CDataStore* msg) {
    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        if (!addon->m_secure) {
            continue;
        }

        uint8_t state;
        msg->Get(state);
        msg->Get(reinterpret_cast<uint8_t&>(addon->m_hasPublicKey));

        if (addon->m_hasPublicKey) {
            uint8_t hasKeyData;
            msg->Get(hasKeyData);

            if (hasKeyData) {
                msg->GetData(addon->m_publicKey, sizeof(addon->m_publicKey));
                addon->SavePublicKey();
            }

            uint32_t revision;
            msg->Get(revision);

            if (revision > addon->m_revision) {
                addon->m_outOfDate = true;
            }
        }

        uint8_t hasUrl;
        msg->Get(hasUrl);

        if (hasUrl) {
            char url[256];
            msg->GetString(url, sizeof(url));

            if (addon->m_url) {
                SMemFree(addon->m_url, __FILE__, __LINE__, 0);
            }

            addon->m_url = SStrDupA(url, __FILE__, __LINE__);

            char path[260];
            SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\%s.url", addon->m_key.m_str, addon->m_key.m_str);
            WriteURL(path, addon->m_url);
        }

        addon->m_status = 1;
        addon->m_signed = false;

        if (state == 2) {
            addon->m_blizzard = true;
        } else {
            addon->m_blizzard = false;

            if (!state) {
                addon->m_status = 2;
                continue;
            }
        }

        const char* name = addon->m_key.m_str;
        char toc[260];
        char bindings[260];
        SStrPrintf(toc, sizeof(toc), "Interface\\AddOns\\%s\\%s.toc", name, name);
        SStrPrintf(bindings, sizeof(bindings), "Interface\\AddOns\\%s\\Bindings.xml", name);

        int32_t result = FrameXML_CheckSignature(toc, bindings, addon->m_hasPublicKey ? addon->m_publicKey : nullptr, addon->m_digest);

        if (result == 2) {
            addon->m_signed = true;
        } else if (result == 3) {
            addon->m_status = 0;
        }
    }

    uint32_t count = 0;
    msg->Get(count);

    for (uint32_t i = 0; i < count; i++) {
        BannedAddOnCache cache;
        msg->Get(reinterpret_cast<uint32_t&>(cache.m_id));
        msg->GetData(cache.m_nameMd5, sizeof(cache.m_nameMd5));
        msg->GetData(cache.m_versionMd5, sizeof(cache.m_versionMd5));

        uint32_t lastModified = 0;
        msg->Get(lastModified);
        cache.m_lastModified = lastModified;
        msg->Get(reinterpret_cast<uint32_t&>(cache.m_flags));

        BannedAddOnCache* registered = RegisterBannedAddOn(cache.m_id, nullptr, nullptr);

        if (registered) {
            *registered = cache;
        }
    }

    SaveBannedAddOns();

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        const char* name = addon->m_key.m_str;
        UIADDON::METADATA* version = addon->m_metadata.Ptr("Version");

        if (IsBannedAddOn(name, version ? version->m_value : nullptr)) {
            addon->m_status = 2;
        }
    }

    DestroyBannedAddOns();

    s_addOnInfo.SetCount(0);

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        if (!addon->m_blizzard) {
            *s_addOnInfo.New() = addon->m_key.m_str;
        }
    }

    qsort(s_addOnInfo.m_data, s_addOnInfo.Count(), sizeof(const char*), QSortAddOns);
}

// VERIFY: 6c736269 manual
// OFFSET: 0x005F7D70
int32_t GetAddOnEnableState(const char* name, const char* character, bool compute) {
    UIADDON* addon = s_addons.Ptr(name);

    if (!addon) {
        return 0;
    }

    uint32_t total = 0;
    uint32_t enabled = 0;

    for (ADDONSTATELIST* list = s_addOnStates.Head(); list; list = s_addOnStates.Next(list)) {
        if (character && SStrCmpI(list->m_name, character, STORM_MAX_STR)) {
            continue;
        }

        bool state = false;
        ADDONSTATE* addonState = list->m_states.Ptr(name);

        if (addonState) {
            state = addonState->m_enabled;
        } else if (compute) {
            switch (ComputeAddOnEnableState(name)) {
                case 0:
                    state = false;
                    break;

                case 1:
                    state = addon->m_enabledByDefault;
                    break;

                case 2:
                    state = true;
                    break;
            }
        } else {
            state = addon->m_enabledByDefault;
        }

        total++;

        if (state) {
            enabled++;
        }

        if (character) {
            break;
        }
    }

    if (!total) {
        return addon->m_enabledByDefault ? 2 : 0;
    }

    if (!enabled) {
        return 0;
    }

    return 2 - (enabled < total);
}

// VERIFY: 34f50be6 manual
// OFFSET: 0x005F7E90
bool IsAddOnLoadable(const char* name, bool demandLoad, UIADDON_LOADABLE_REASON& reason, UIADDON_LOADABLE_REASON& dependencyReason, const char* character) {
    UIADDON* addon = s_addons.Ptr(name);

    if (!addon) {
        reason = UIADDON_MISSING;
        return false;
    }

    if (addon->m_checkingLoadable) {
        return true;
    }

    if (!GetAddOnEnableState(name, character, true)) {
        reason = UIADDON_DISABLED;
        return false;
    }

    if (addon->m_status == 2) {
        reason = UIADDON_BANNED;
        return false;
    }

    if (addon->m_signed) {
        reason = UIADDON_CORRUPT;
        return false;
    }

    if (addon->m_interfaceVersion < 20000) {
        reason = UIADDON_INCOMPATIBLE;
        return false;
    }

    if (addon->m_interfaceVersion != 30300) {
        reason = UIADDON_INTERFACE_VERSION;

        if (g_checkAddonVersionVar->m_intValue) {
            return false;
        }
    }

    reason = UIADDON_LOADABLE;

    bool loadable = true;
    addon->m_checkingLoadable = true;

    for (uint32_t i = 0; i < addon->m_requiredDeps.Count(); i++) {
        if (!IsAddOnLoading(addon->m_requiredDeps[i]) && !IsAddOnLoadable(addon->m_requiredDeps[i], true, dependencyReason, dependencyReason, character)) {
            loadable = false;
            break;
        }
    }

    addon->m_checkingLoadable = false;

    if (!loadable) {
        return false;
    }

    if (!demandLoad && addon->m_loadOnDemand) {
        reason = UIADDON_DEMAND_LOADED;
        return false;
    }

    return true;
}

// VERIFY: a5cade8d auto
// OFFSET: 0x005F7FF0
uint32_t LoadAddOnFileCount() {
    uint32_t count = 0;

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        if (addon->m_loadOnDemand) {
            continue;
        }

        UIADDON_LOADABLE_REASON reason;
        UIADDON_LOADABLE_REASON dependencyReason;
        bool managed = false;

        for (uint32_t i = 0; i < addon->m_loadManagers.Count(); i++) {
            if (IsAddOnLoadable(addon->m_loadManagers[i], false, reason, dependencyReason, ClientServices::GetCharacterName())) {
                managed = true;
                break;
            }
        }

        if (!managed && IsAddOnLoadable(addon->m_key.m_str, false, reason, dependencyReason, ClientServices::GetCharacterName())) {
            count += addon->m_numFiles;
        }
    }

    return count;
}

// VERIFY: d96c4f04 manual
// OFFSET: 0x005F8530
void UIADDON::SetMetadata(const char* key, const char* value) {
    UIADDON::METADATA* metadata = this->m_metadata.Ptr(key);

    if (!metadata) {
        metadata = this->m_metadata.New(key, 0, 0);
    }

    if (metadata->m_value) {
        SMemFree(metadata->m_value, __FILE__, __LINE__, 0);
    }

    metadata->m_value = SStrDupA(value, __FILE__, __LINE__);
}

// VERIFY: 84480365 manual
// OFFSET: 0x005F85D0
static void LoadAddOnMetadata(UIADDON* addon, const char* line) {
    const char* end = line;
    const char* locale = nullptr;

    while (*end) {
        if (*end == '-') {
            locale = end + 1;
        } else if (*end == ':') {
            break;
        }

        end++;
    }

    const char* keyEnd = end;

    if (locale) {
        uint32_t i;

        for (i = 0; i < 9; i++) {
            if (!SStrCmpI(locale, s_iniLocaleNames[i], 4)) {
                break;
            }
        }

        if (i < 9) {
            if (i != s_locale) {
                return;
            }

            keyEnd = locale - 1;
        }
    }

    while (keyEnd > line && keyEnd[-1] == ' ') {
        keyEnd--;
    }

    while (*end == ':' || *end == ' ') {
        end++;
    }

    uint32_t keyLength = keyEnd - line;
    char* key = static_cast<char*>(alloca(keyLength + 1));
    memcpy(key, line, keyLength);
    key[keyLength] = '\0';

    addon->SetMetadata(key, end);
}

// VERIFY: 1e6f21da manual
// OFFSET: 0x005F80B0
bool LoadAddOn(const char* name, bool demandLoad, CStatus* status) {
    UIADDON* addon = s_addons.Ptr(name);

    if (!addon) {
        return false;
    }

    if (addon->m_loading) {
        return true;
    }

    const char* addonName = addon->m_key.m_str;
    UIADDON_LOADABLE_REASON reason;
    UIADDON_LOADABLE_REASON dependencyReason;

    if (!IsAddOnLoadable(addonName, demandLoad, reason, dependencyReason, ClientServices::GetCharacterName())) {
        return false;
    }

    addon->m_loading = true;

    for (uint32_t i = 0; i < addon->m_optionalDeps.Count(); i++) {
        LoadAddOn(addon->m_optionalDeps[i], true, status);
    }

    for (uint32_t i = 0; i < addon->m_requiredDeps.Count(); i++) {
        if (!LoadAddOn(addon->m_requiredDeps[i], true, status)) {
            addon->m_loading = false;
            return false;
        }
    }

    {
        FrameScript_GetContext();
        const char* savedTaint = lua_tainted;

        if (++lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = addon->m_status ? addonName : nullptr;
        }

        const char* savedTag = lua_gctag;
        lua_gctag = addon->m_status ? addonName : nullptr;

        CStatus addonStatus;
        MD5_CTX md5;
        MD5Init(&md5);

        char path[260];
        SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\%s.toc", addonName, addonName);
        lua_State* L = FrameScript_GetContext();
        lua_createtable(L, 0, 0);
        FrameXML_CreateFrames(path, addonName, &md5, &addonStatus);
        lua_settop(L, -2);

        SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\Bindings.xml", addonName);

        if (SFile::FileExistsEx(path, 1)) {
            CGUIBindings::s_bindings->Load(path, &md5, &addonStatus);
        }

        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\SavedVariables\\%s.lua", s_addOnAccountName, addonName);

        if (SFile::FileExistsEx(path, 1)) {
            FrameScript_LoadVariables(path, &addonStatus);
        }

        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s\\SavedVariables\\%s.lua", s_addOnAccountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName(), addonName);

        if (SFile::FileExistsEx(path, 1)) {
            FrameScript_LoadVariables(path, &addonStatus);
        }

        if (FrameXML_GetDebugLevel() > 0 || addonStatus.m_highestSeverity > STATUS_INFO) {
            addonStatus.Prepend(STATUS_INFO, "Loading add-on %s", addonName);
        }

        status->Add(addonStatus);

        uint8_t digest[16];
        MD5Final(digest, &md5);

#ifndef DISABLE_SIGNATURE_CHECKS
        if (!addon->m_status && memcmp(addon->m_digest, digest, sizeof(digest))) {
            ClientPostClose(10);
        }
#endif

        lua_gctag = savedTag;

        if (lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = savedTaint;
        }

        if (--lua_taintexpected <= 0) {
            lua_taintexpected = 0;
        }
    }

    addon->m_loaded = true;
    FrameScript_SignalEvent(SCRIPT_EVENT_ADDON_LOADED, "%s", addonName);

    for (uint32_t i = 0; i < addon->m_loadWithMe.Count(); i++) {
        LoadAddOn(addon->m_loadWithMe[i]->m_key.m_str, demandLoad, status);
    }

    return true;
}

// VERIFY: 96a89a20 auto
// OFFSET: 0x005F84A0
void LoadAddOns(CStatus* status) {
    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        for (uint32_t i = 0; i < addon->m_loadManagers.Count(); i++) {
            if (LoadAddOn(addon->m_loadManagers[i], false, status)) {
                addon->m_loadOnDemand = true;
                break;
            }
        }

        if (!addon->m_loadOnDemand) {
            LoadAddOn(addon->m_key.m_str, false, status);
        }
    }
}

// VERIFY: b8ed8d06 manual
// OFFSET: 0x005F86A0
static void LoadAddOnInfo(const char* name) {
    if (s_addons.Ptr(name)) {
        return;
    }

    char path[260];
    SStrPrintf(path, sizeof(path), "Interface\\AddOns\\%s\\%s.toc", name, name);

    void* data;

    if (!SFile::ReadFile(nullptr, path, &data, nullptr, 1, 1, nullptr)) {
        return;
    }

    const char* toc = static_cast<const char*>(data);

    if (static_cast<uint8_t>(toc[0]) == 0xEF && static_cast<uint8_t>(toc[1]) == 0xBB && static_cast<uint8_t>(toc[2]) == 0xBF) {
        toc += 3;
    }

    UIADDON* addon = s_addons.New(name, 0, 0);
    addon->SetMetadata("Title", name);
    addon->m_numFiles = FrameXML_GuessNumFiles(toc);

    char line[1024];

    while (true) {
        SStrTokenize(&toc, line, sizeof(line), "\r\n", nullptr);

        if (line[0] == '#' && line[1] == '#') {
            char* p = &line[2];

            while (*p == ' ') {
                p++;
            }

            if (!SStrCmpI(p, "Interface:", SStrLen("Interface:"))) {
                if (*p != ':') {
                    do {
                        p++;
                    } while (*p != ':');

                    p++;
                }

                while (*p == ':' || *p == ' ') {
                    p++;
                }

                addon->m_interfaceVersion = SStrToUnsigned(p);
            } else if (!SStrCmpI(p, "Title", 5) || !SStrCmpI(p, "Notes", 5) || !SStrCmpI(p, "Author", 6) || !SStrCmpI(p, "Version", 7) || !SStrCmpI(p, "X-", 2)) {
                LoadAddOnMetadata(addon, p);
            } else if (!SStrCmpI(p, "Revision:", SStrLen("Revision:"))) {
                if (*p != ':') {
                    do {
                        p++;
                    } while (*p != ':');

                    p++;
                }

                while (*p == ':' || *p == ' ') {
                    p++;
                }

                addon->m_revision = SStrToUnsigned(p);
            } else if (!SStrCmpI(p, "OptionalDep", SStrLen("OptionalDep"))) {
                while (*p && *p != ':') {
                    p++;
                }

                if (*p == ':') {
                    const char* list = p + 1;
                    char token[260];

                    do {
                        SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                        if (!token[0]) {
                            break;
                        }

                        *addon->m_optionalDeps.New() = SStrDupA(token, __FILE__, __LINE__);
                    } while (*list);
                }
            } else if (!SStrCmpI(p, "RequiredDep", SStrLen("RequiredDep")) || !SStrCmpI(p, "Dep", SStrLen("Dep"))) {
                while (*p && *p != ':') {
                    p++;
                }

                if (*p == ':') {
                    const char* list = p + 1;
                    char token[260];

                    do {
                        SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                        if (!token[0]) {
                            break;
                        }

                        *addon->m_requiredDeps.New() = SStrDupA(token, __FILE__, __LINE__);
                    } while (*list);
                }
            } else if (!SStrCmpI(p, "LoadWith:", SStrLen("LoadWith:"))) {
                while (*p && *p != ':') {
                    p++;
                }

                if (*p == ':') {
                    const char* list = p + 1;
                    char token[260];

                    do {
                        SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                        if (!token[0]) {
                            break;
                        }

                        *addon->m_loadWith.New() = SStrDupA(token, __FILE__, __LINE__);
                    } while (*list);
                }
            } else if (!SStrCmpI(p, "LoadManagers:", SStrLen("LoadManagers:"))) {
                while (*p && *p != ':') {
                    p++;
                }

                if (*p == ':') {
                    const char* list = p + 1;
                    char token[260];

                    do {
                        SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                        if (!token[0]) {
                            break;
                        }

                        *addon->m_loadManagers.New() = SStrDupA(token, __FILE__, __LINE__);
                    } while (*list);
                }
            } else if (!SStrCmpI(p, "SavedVariables:", SStrLen("SavedVariables:"))) {
                while (*p != ':') {
                    p++;
                }

                const char* list = p + 1;
                char token[260];

                do {
                    SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                    if (!token[0]) {
                        break;
                    }

                    *addon->m_savedVariables.New() = SStrDupA(token, __FILE__, __LINE__);
                } while (*list);
            } else if (!SStrCmpI(p, "SavedVariablesPerCharacter:", SStrLen("SavedVariablesPerCharacter:"))) {
                while (*p != ':') {
                    p++;
                }

                const char* list = p + 1;
                char token[260];

                do {
                    SStrTokenize(&list, token, sizeof(token), " ,", nullptr);

                    if (!token[0]) {
                        break;
                    }

                    *addon->m_savedVariablesPerCharacter.New() = SStrDupA(token, __FILE__, __LINE__);
                } while (*list);
            } else if (!SStrCmpI(p, "DefaultState:", SStrLen("DefaultState:"))) {
                if (*p != ':') {
                    do {
                        p++;
                    } while (*p != ':');

                    p++;
                }

                while (*p == ':' || *p == ' ') {
                    p++;
                }

                if (!SStrCmpI(p, "enabled", STORM_MAX_STR)) {
                    addon->m_enabledByDefault = true;
                }

                if (!SStrCmpI(p, "disabled", STORM_MAX_STR)) {
                    addon->m_enabledByDefault = false;
                }
            } else if (!SStrCmpI(p, "LoadOnDemand:", SStrLen("LoadOnDemand:"))) {
                if (*p != ':') {
                    do {
                        p++;
                    } while (*p != ':');

                    p++;
                }

                while (*p == ':' || *p == ' ') {
                    p++;
                }

                addon->m_loadOnDemand = StringToBOOL(p) != 0;
            } else if (!SStrCmpI(p, "Secure:", SStrLen("Secure:"))) {
                if (*p != ':') {
                    do {
                        p++;
                    } while (*p != ':');

                    p++;
                }

                while (*p == ':' || *p == ' ') {
                    p++;
                }

                addon->m_secure = StringToBOOL(p) != 0;
            }
        }

        if (!line[0] || !*toc) {
            break;
        }
    }

    SFile::Unload(data);

    if (addon->m_secure) {
        addon->LoadPublicKey();
        addon->LoadURL();
    }
}

// VERIFY: 786cc374 manual
// OFFSET: 0x005F8E80
static int32_t EnumAddOnsArchive(const char* fileName, void* param) {
    const char* extension = SStrChrR(const_cast<char*>(fileName), '.');

    if (!extension || SStrCmpI(extension, ".toc", STORM_MAX_STR)) {
        return 1;
    }

    char name[260];
    SStrCopy(name, fileName + SStrLen("Interface\\AddOns\\"), sizeof(name));

    for (char* p = name; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = '\0';
            break;
        }
    }

    LoadAddOnInfo(name);

    return 1;
}

// VERIFY: 32ba47b7 auto
// OFFSET: 0x005F8F30
static int32_t EnumAddOnsDisk(OS_FILE_DATA& data, void* param) {
    if ((data.flags & 0x10) && data.fileName[0] != '.') {
        LoadAddOnInfo(data.fileName);
    }

    return 0;
}

// VERIFY: 605b03d2 manual
// OFFSET: 0x005F8F50
void InitializeAddOns() {
    ListInterfaceArchive("Interface\\AddOns\\", EnumAddOnsArchive, nullptr);
    OsFileList("Interface\\AddOns\\", "*", EnumAddOnsDisk, nullptr, 0);

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.m_fulllist.Next(addon)) {
        for (uint32_t i = 0; i < addon->m_loadWith.Count(); i++) {
            UIADDON* target = s_addons.Ptr(addon->m_loadWith[i]);

            if (target) {
                *target->m_loadWithMe.New() = addon;
            }
        }
    }
}

// VERIFY: 0f76c1b3 auto
// OFFSET: 0x005F9080
void InitializeAddOns(const char* accountName) {
    ShutdownAddOns();
    SStrCopy(s_addOnAccountName, accountName, sizeof(s_addOnAccountName));
    InitializeAddOns();
    s_addOnsInitialized = true;
}

// VERIFY: ee1f2414 manual
// OFFSET: 0x005F90B0
void SetAddOnEnableState(const char* name, const char* character, int32_t state) {
    bool enabled = state != 0;

    for (ADDONSTATELIST* list = s_addOnStates.Head(); list; list = s_addOnStates.Next(list)) {
        if (character && SStrCmpI(list->m_name, character, STORM_MAX_STR)) {
            continue;
        }

        ADDONSTATE* addonState = list->m_states.Ptr(name);

        if (addonState) {
            if (enabled != addonState->m_enabled) {
                addonState->m_enabled = enabled;
                list->m_dirty = true;
            }
        } else {
            addonState = list->m_states.New(name, 0, 0);
            addonState->m_enabled = enabled;
            list->m_dirty = true;
        }

        if (character) {
            break;
        }
    }
}

// VERIFY: c1cbd847 manual
// OFFSET: 0x005F91E0
void LoadAddOnEnableState(const char* character) {
    if (!character) {
        for (ADDONSTATELIST* list = s_addOnStates.Head(); !TSList<ADDONSTATELIST, TSGetLink<ADDONSTATELIST> >::IsEnd(list); list = s_addOnStates.RawNext(list)) {
            LoadAddOnEnableState(list->m_name);
        }

        return;
    }

    ADDONSTATELIST* list = s_addOnStates.Head();

    while (!TSList<ADDONSTATELIST, TSGetLink<ADDONSTATELIST> >::IsEnd(list)) {
        if (!SStrCmpI(character, list->m_name, STORM_MAX_STR)) {
            list->m_dirty = false;
            list->m_states.InternalClear(0);
            break;
        }

        list = s_addOnStates.RawNext(list);
    }

    if (TSList<ADDONSTATELIST, TSGetLink<ADDONSTATELIST> >::IsEnd(list)) {
        list = s_addOnStates.NewNode(2, 0, 8);
        list->m_name = SStrDupA(character, __FILE__, __LINE__);
    }

    char path[260];
    SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s\\AddOns.txt", s_addOnAccountName, ClientServices::GetSelectedRealmName(), character);

    void* data;

    if (!SFile::ReadFile(nullptr, path, &data, nullptr, 1, 1, nullptr)) {
        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\AddOns.txt", s_addOnAccountName);

        if (!SFile::ReadFile(nullptr, path, &data, nullptr, 1, 1, nullptr)) {
            return;
        }

        list->m_dirty = true;
    }

    const char* cursor = static_cast<const char*>(data);
    char line[1024];

    while (true) {
        SStrTokenize(&cursor, line, sizeof(line), "\r\n\"", nullptr);

        if (!*cursor) {
            break;
        }

        char* separator = line;

        while (*separator && *separator != ':') {
            separator++;
        }

        if (*separator == ':') {
            *separator = '\0';

            const char* value = separator + 1;

            while (*value == ' ') {
                value++;
            }

            if (*value) {
                bool enabled = SStrCmpI(value, "disabled", STORM_MAX_STR) != 0;
                ADDONSTATE* state = list->m_states.Ptr(line);

                if (!state) {
                    state = list->m_states.New(line, 0, 0);
                }

                state->m_enabled = enabled;
            }
        }

        if (!line[0] || !*cursor) {
            break;
        }
    }

    SFile::Unload(data);
}

// VERIFY: 3bde2f06 manual
// OFFSET: 0x005F5620
void UnloadAddOns() {
    int32_t changed = 0;
    char path[260];
    SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\SavedVariables", s_addOnAccountName);

    if (!OsDirectoryExists(path)) {
        OsCreateDirectory(path, 1);
    }

    SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s\\SavedVariables", s_addOnAccountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName());

    if (!OsDirectoryExists(path)) {
        OsCreateDirectory(path, 1);
    }

    for (UIADDON* addon = s_addons.m_fulllist.Head(); addon; addon = s_addons.Next(addon)) {
        if (!addon->m_loading) {
            continue;
        }

        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\SavedVariables\\%s.lua", s_addOnAccountName, addon->m_key.m_str);

        if (addon->m_savedVariables.Count()) {
            if (OsFileExists(path)) {
                char backup[260];
                SStrCopy(backup, path, sizeof(backup));
                SStrPack(backup, ".bak", sizeof(backup));
                OsMoveFile(path, backup);
            } else {
                changed = 1;
            }

            HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

            if (file != OS_INVALID_FILE) {
                FrameScript_GetContext();
                const char* savedTaint = lua_tainted;

                if (++lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = nullptr;
                }

                for (uint32_t i = 0; i < addon->m_savedVariables.Count(); i++) {
                    const char* name = addon->m_savedVariables.m_data[i];

                    if (!name || !*name) {
                        continue;
                    }

                    const char* c = name;

                    while (*c < 0 || (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') {
                        if (!*++c) {
                            FrameScript_PrintVariable(file, name);
                            break;
                        }
                    }
                }

                uint32_t written;
                OsWriteFile(file, "\r\n", 2, &written);
                OsCloseFile(file);

                if (lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = savedTaint;
                }

                if (--lua_taintexpected <= 0) {
                    lua_taintexpected = 0;
                }
            }
        } else if (OsFileExists(path)) {
            OsDeleteFile(path);
            changed = 1;
        }

        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s\\SavedVariables\\%s.lua", s_addOnAccountName, ClientServices::GetSelectedRealmName(), ClientServices::GetCharacterName(), addon->m_key.m_str);

        if (addon->m_savedVariablesPerCharacter.Count()) {
            if (OsFileExists(path)) {
                char backup[260];
                SStrCopy(backup, path, sizeof(backup));
                SStrPack(backup, ".bak", sizeof(backup));
                OsMoveFile(path, backup);
            } else {
                changed = 1;
            }

            HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

            if (file != OS_INVALID_FILE) {
                FrameScript_GetContext();
                const char* savedTaint = lua_tainted;

                if (++lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = nullptr;
                }

                for (uint32_t i = 0; i < addon->m_savedVariablesPerCharacter.Count(); i++) {
                    const char* name = addon->m_savedVariablesPerCharacter.m_data[i];

                    if (!name || !*name) {
                        continue;
                    }

                    const char* c = name;

                    while (*c < 0 || (*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || *c == '_') {
                        if (!*++c) {
                            FrameScript_PrintVariable(file, name);
                            break;
                        }
                    }
                }

                uint32_t written;
                OsWriteFile(file, "\r\n", 2, &written);
                OsCloseFile(file);

                if (lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = savedTaint;
                }

                if (--lua_taintexpected <= 0) {
                    lua_taintexpected = 0;
                }
            }
        } else {
            if (OsFileExists(path)) {
                OsDeleteFile(path);
                changed = 1;
            }

            SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\SavedVariables\\%s.lua", s_addOnAccountName, ClientServices::GetCharacterName(), addon->m_key.m_str);

            if (OsFileExists(path)) {
                OsDeleteFile(path);
                changed = 1;
            }
        }

        addon->m_loading = false;
        addon->m_loaded = false;
    }

    if (changed) {
        SFile::RebuildHash();
    }
}

// VERIFY: 1697dec4 auto
// OFFSET: 0x005F7670
int32_t GetAddOnSecurity(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (addon) {
        return addon->m_status;
    }

    return 1;
}

// VERIFY: 6b68ae99 manual
// OFFSET: 0x005F76A0
void SaveAddOnEnableState() {
    int32_t created = 0;

    for (ADDONSTATELIST* list = s_addOnStates.Head(); list; list = s_addOnStates.Next(list)) {
        if (!list->m_dirty) {
            continue;
        }

        list->m_dirty = false;
        char path[260];
        SStrPrintf(path, sizeof(path), "WTF\\Account\\%s\\%s\\%s", s_addOnAccountName, ClientServices::GetSelectedRealmName(), list->m_name);

        if (!OsDirectoryExists(path)) {
            OsCreateDirectory(path, 1);
        }

        SStrPack(path, "\\AddOns.txt", sizeof(path));

        if (!created) {
            created = !OsFileExists(path);
        }

        HOSFILE file = OsCreateFile(path, OS_GENERIC_WRITE, 0, OS_CREATE_ALWAYS, OS_FILE_ATTRIBUTE_NORMAL, OS_FILE_TYPE_ANY);

        if (file == OS_INVALID_FILE) {
            continue;
        }

        for (ADDONSTATE* state = list->m_states.m_fulllist.Head(); state; state = list->m_states.Next(state)) {
            char line[1024];
            SStrPrintf(line, sizeof(line), "%s: %s\r\n", state->m_key.m_str, state->m_enabled ? "enabled" : "disabled");
            uint32_t written;
            OsWriteFile(file, line, SStrLen(line), &written);
        }

        OsCloseFile(file);
    }

    if (created) {
        SFile::RebuildHash();
    }
}

// VERIFY: 631a1581 manual
// OFFSET: 0x005F4CA0
bool IsAddonVersionCheckEnabled() {
    return g_checkAddonVersionVar->m_intValue != 0;
}

// VERIFY: e8b33bcf auto
// OFFSET: 0x005F4CB0
void SetAddonVersionCheck(int32_t enable) {
    if (enable) {
        g_checkAddonVersionVar->Set("1", true, false, false, true);
    } else {
        g_checkAddonVersionVar->Set("0", true, false, false, true);
    }
}

// VERIFY: caf3378a manual
// OFFSET: 0x005F74B0
const char* GetAddOnURL(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);

    if (!addon) {
        return nullptr;
    }

    const char* url = addon->m_url;

    if (!url || !*url || addon->m_status == 2) {
        return nullptr;
    }

    return url;
}

// VERIFY: 0ce75a0e auto
// OFFSET: 0x005F7540
bool IsAddOnUpdateAvailable(const char* name) {
    UIADDON* addon = s_addons.Ptr(name);
    return addon ? addon->m_outOfDate : false;
}
