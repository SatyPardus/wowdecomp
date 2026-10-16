#ifndef WOW_SOURCE_WOWSERVICES_WDATASTORE_H
#define WOW_SOURCE_WOWSERVICES_WDATASTORE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataStore.h"

struct WDATASTOREBUFFER {
    /* 0000 */ uint32_t m_memHandle;
    /* 0004 */ uint8_t unk_0004[28];
};

class WDataStoreHeap {
    public:
    /* 0000 */ uint32_t m_heapId;

    void Delete(WDATASTOREBUFFER* buffer);
    WDATASTOREBUFFER* New();
};

class WDataStore : public CDataStore {
    public:
    static void* AllocBuffer(uint32_t size);
    static void FreeBuffer(void* buffer, uint32_t size);
    static void StaticDestroy();
    static void StaticInitialize();

    /* 0018 */ WDATASTOREBUFFER* m_bufferObj;
    /* 001C */ uint32_t unused_001C;
    /* 0020 */ uint32_t m_chunkSize;

    void InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc) override;
    void InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc) override;
    int32_t InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber) override;
    ~WDataStore() override;
    uint8_t* GetHeaderSpace(uint32_t bytes) override;

    WDataStore();
};

#endif
