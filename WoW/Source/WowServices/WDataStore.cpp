#include "WDataStore.h"
#include <cstring>
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B38CF0
static WDataStoreHeap* s_smallBufferHeap;

// STATIC: 0x00B38CF4
static WDataStoreHeap* s_largeBufferHeap;

// STATIC: 0x00B38CF8
static WDataStoreHeap* s_superBufferHeap;

// STATIC: 0x00B38CFC
static bool s_initialized;

// VERIFY: 4532c277 manual
// OFFSET: 0x00403EC0
WDataStore::WDataStore() {
    this->InternalInitialize(this->m_data, this->m_base, this->m_alloc);
}

// VERIFY: 439c74d3 manual
// OFFSET: [0x00403F10, 0x00403F60]
WDataStore::~WDataStore() {
    if (this->m_alloc != -1) {
        this->InternalDestroy(this->m_data, this->m_base, this->m_alloc);
    }
}

// VERIFY: a4003730 auto
// OFFSET: 0x00465F50
void WDataStore::InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    this->m_bufferObj = nullptr;
    alloc = 0;
    data = nullptr;
    base = 0;
    this->m_chunkSize = 256;
}

// VERIFY: 192df1ac manual
// OFFSET: 0x00465F80
uint8_t* WDataStore::GetHeaderSpace(uint32_t bytes) {
    if (!this->m_bufferObj || bytes > sizeof(this->m_bufferObj->unk_0004)) {
        return nullptr;
    }

    return reinterpret_cast<uint8_t*>(this->m_bufferObj + 1) - bytes;
}

// VERIFY: 456591c0 auto
// OFFSET: 0x00465FB0
void WDataStoreHeap::Delete(WDATASTOREBUFFER* buffer) {
    ObjectFree(this->m_heapId, buffer->m_memHandle);
}

// OFFSET: 0x00466140
void WDataStore::StaticDestroy() {
#if !defined(__APPLE__)
    delete s_smallBufferHeap;
    delete s_largeBufferHeap;
    delete s_superBufferHeap;
    s_initialized = false;
#endif
}

// VERIFY: c0b008bd manual
// OFFSET: 0x00466050
void WDataStore::StaticInitialize() {
    if (s_initialized) {
        return;
    }

#if defined(__APPLE__)
    // ObjectAlloc heap identities live for the process. Keep these three
    // identities across network restarts, including buffers still owned by
    // a disconnected connection or message. Empty heap pages remain reclaimable.
    static WDataStoreHeap small{ObjectAllocAddHeap(800, 256, "WDataStoreSmallBuffer", true)};
    static WDataStoreHeap large{ObjectAllocAddHeap(0x4020, 16, "WDataStoreLargeBuffer", true)};
    static WDataStoreHeap super{ObjectAllocAddHeap(0x40060, 4, "WDataStoreSuperBuffer", true)};
    s_smallBufferHeap = &small; s_largeBufferHeap = &large; s_superBufferHeap = &super;
#else
    s_smallBufferHeap = new (__FILE__, __LINE__) WDataStoreHeap{ObjectAllocAddHeap(800, 256, "WDataStoreSmallBuffer", true)};
    s_largeBufferHeap = new (__FILE__, __LINE__) WDataStoreHeap{ObjectAllocAddHeap(0x4020, 16, "WDataStoreLargeBuffer", true)};
    s_superBufferHeap = new (__FILE__, __LINE__) WDataStoreHeap{ObjectAllocAddHeap(0x40060, 4, "WDataStoreSuperBuffer", true)};
#endif
    s_initialized = true;
}

// VERIFY: 6f638a32 manual
// OFFSET: 0x00466190
void WDataStore::InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    if (this->m_bufferObj) {
        if (alloc == 0x300) {
            ObjectFree(s_smallBufferHeap->m_heapId, this->m_bufferObj->m_memHandle);
            this->m_bufferObj = nullptr;
        } else if (alloc == 0x4000) {
            ObjectFree(s_largeBufferHeap->m_heapId, this->m_bufferObj->m_memHandle);
            this->m_bufferObj = nullptr;
        } else if (alloc == 0x40040) {
            ObjectFree(s_superBufferHeap->m_heapId, this->m_bufferObj->m_memHandle);
            this->m_bufferObj = nullptr;
        }
    } else if (data) {
        SMemFree(data, __FILE__, __LINE__, 0);
    }

    data = nullptr;
    base = 0;
    alloc = 0;
}

// VERIFY: fbc07a26 manual
// OFFSET: 0x00466230
void WDataStore::FreeBuffer(void* buffer, uint32_t size) {
    WDATASTOREBUFFER* header = static_cast<WDATASTOREBUFFER*>(buffer) - 1;

    if (size <= 0x300) {
        ObjectFree(s_smallBufferHeap->m_heapId, header->m_memHandle);
    } else if (size <= 0x4000) {
        ObjectFree(s_largeBufferHeap->m_heapId, header->m_memHandle);
    } else if (size <= 0x40040) {
        ObjectFree(s_superBufferHeap->m_heapId, header->m_memHandle);
    } else {
        SMemFree(buffer, __FILE__, __LINE__, 0);
    }
}

// VERIFY: a9ab9cff manual
// OFFSET: 0x004662B0
WDATASTOREBUFFER* WDataStoreHeap::New() {
    uint32_t memHandle;
    WDATASTOREBUFFER* buffer = nullptr;

    if (ObjectAlloc(this->m_heapId, &memHandle, reinterpret_cast<void**>(&buffer), false)) {
        buffer->m_memHandle = memHandle;
        return buffer;
    }

    return nullptr;
}

// VERIFY: f375d837 manual
// OFFSET: 0x004662F0
int32_t WDataStore::InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber) {
    if (!s_initialized) {
        WDataStore::StaticInitialize();
    }

    uint32_t size = pos + bytes;
    uint32_t oldAlloc = alloc;

    if (size < oldAlloc) {
        return 1;
    }

    WDATASTOREBUFFER* oldBuffer = this->m_bufferObj;

    if (size <= 0x300) {
        WDATASTOREBUFFER* buffer = s_smallBufferHeap->New();

        if (data) {
            memcpy(buffer + 1, data, alloc);
        }

        data = reinterpret_cast<uint8_t*>(buffer + 1);
        alloc = 0x300;
        this->m_bufferObj = buffer;
    } else if (size <= 0x4000) {
        WDATASTOREBUFFER* buffer = s_largeBufferHeap->New();

        if (data) {
            memcpy(buffer + 1, data, alloc);
        }

        data = reinterpret_cast<uint8_t*>(buffer + 1);
        alloc = 0x4000;
        this->m_bufferObj = buffer;
    } else if (size <= 0x40040) {
        WDATASTOREBUFFER* buffer = s_superBufferHeap->New();

        if (data) {
            memcpy(buffer + 1, data, alloc);
        }

        data = reinterpret_cast<uint8_t*>(buffer + 1);
        alloc = 0x40040;
        this->m_bufferObj = buffer;
    } else {
        uint32_t newAlloc = ~(this->m_chunkSize - 1) & (this->m_chunkSize + size - 1);

        if (oldBuffer) {
            uint8_t* newData = static_cast<uint8_t*>(CDataStore::Alloc(newAlloc, fileName ? fileName : __FILE__, lineNumber ? lineNumber : __LINE__));
            memcpy(newData, data, alloc);
            data = newData;
            this->m_bufferObj = nullptr;
        } else {
            data = static_cast<uint8_t*>(CDataStore::ReAlloc(data, newAlloc, fileName ? fileName : __FILE__, lineNumber ? lineNumber : __LINE__));
        }

        alloc = newAlloc;
    }

    if (oldBuffer) {
        switch (oldAlloc) {
            case 0x300:
                ObjectFree(s_smallBufferHeap->m_heapId, oldBuffer->m_memHandle);
                return 1;

            case 0x4000:
                ObjectFree(s_largeBufferHeap->m_heapId, oldBuffer->m_memHandle);
                return 1;

            case 0x40040:
                s_superBufferHeap->Delete(oldBuffer);
                break;
        }
    }

    return 1;
}

// VERIFY: 507026fb manual
// OFFSET: 0x00466510
void* WDataStore::AllocBuffer(uint32_t size) {
    if (size <= 0x300) {
        return s_smallBufferHeap->New() + 1;
    }

    if (size <= 0x4000) {
        return s_largeBufferHeap->New() + 1;
    }

    if (size <= 0x40040) {
        return s_superBufferHeap->New() + 1;
    }

    return SMemAlloc(size, __FILE__, __LINE__, 0);
}
