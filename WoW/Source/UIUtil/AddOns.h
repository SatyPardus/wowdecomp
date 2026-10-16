#ifndef WOW_SOURCE_UIUTIL_ADDONS_H
#define WOW_SOURCE_UIUTIL_ADDONS_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class CDataStore;

struct UIADDON : public TSHashObject<UIADDON, HASHKEY_STRI> {
    struct METADATA : public TSHashObject<UIADDON::METADATA, HASHKEY_STRI> {
        /* 0018 */ char* m_value;

        ~METADATA();
    };

    /* 0018 */ bool m_loading;
    /* 0019 */ bool m_loaded;
    /* 001A */ uint8_t unused_001A[2];
    /* 001C */ uint32_t m_interfaceVersion;
    /* 0020 */ uint32_t m_revision;
    /* 0024 */ uint32_t m_status;
    /* 0028 */ bool m_secure;
    /* 0029 */ bool m_blizzard;
    /* 002A */ bool m_signed;
    /* 002B */ bool m_enabledByDefault;
    /* 002C */ bool m_loadOnDemand;
    /* 002D */ bool m_checkingLoadable;
    /* 002E */ uint8_t unused_002E[2];
    /* 0030 */ char* m_url;
    /* 0034 */ uint32_t m_numFiles;
    /* 0038 */ TSGrowableArray<char*> m_optionalDeps;
    /* 0048 */ TSGrowableArray<char*> m_requiredDeps;
    /* 0058 */ TSGrowableArray<char*> m_loadWith;
    /* 0068 */ TSGrowableArray<char*> m_loadManagers;
    /* 0078 */ TSGrowableArray<char*> m_savedVariables;
    /* 0088 */ TSGrowableArray<char*> m_savedVariablesPerCharacter;
    /* 0098 */ TSGrowableArray<UIADDON*> m_loadWithMe;
    /* 00A8 */ TSHashTable<UIADDON::METADATA, HASHKEY_STRI> m_metadata;
    /* 00D0 */ bool m_outOfDate;
    /* 00D1 */ bool m_hasPublicKey;
    /* 00D2 */ uint8_t m_publicKey [256];
    /* 01D2 */ uint8_t m_digest[18];

    UIADDON();
    ~UIADDON();
    void LoadPublicKey();
    void LoadURL();
    void SavePublicKey();
    void SetMetadata(const char* key, const char* value);
};

struct ADDONSTATE : public TSHashObject<ADDONSTATE, HASHKEY_STRI> {
    /* 0018 */ bool m_enabled;
};

struct ADDONSTATELIST : public TSLinkedNode<ADDONSTATELIST> {
    /* 0008 */ char* m_name;
    /* 000C */ bool m_dirty;
    /* 000D */ uint8_t unused_000D[3];
    /* 0010 */ TSHashTable<ADDONSTATE, HASHKEY_STRI> m_states;

    ~ADDONSTATELIST();
};

void DeleteInterfaceFiles();
void GetAddOnInfoRequest(CDataStore* msg);
void SetAddOnInfoRequest(CDataStore* msg);

#endif
