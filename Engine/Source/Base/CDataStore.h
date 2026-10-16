#ifndef ENGINE_SOURCE_BASE_CDATASTORE_H
#define ENGINE_SOURCE_BASE_CDATASTORE_H

#include <cstdint>
#include <cstring>
#include "Engine/Source/Storm/H/storm.h"

class CDataStore {
    public:
    static void* Alloc(uint32_t bytes, const char* fileName, int32_t lineNumber);
    static void* ReAlloc(void* ptr, uint32_t bytes, const char* fileName, int32_t lineNumber);

    /* 0004 */ uint8_t* m_data;
    /* 0008 */ uint32_t m_base;
    /* 000C */ uint32_t m_alloc;
    /* 0010 */ uint32_t m_size;
    /* 0014 */ uint32_t m_read;

    virtual void InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc);
    virtual void InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc);
    virtual int32_t InternalFetchRead(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc);
    virtual int32_t InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber);
    virtual ~CDataStore();
    virtual int32_t IsRead();
    virtual void Reset();
    virtual void Finalize();
    virtual void GetBufferParams(const void** data, uint32_t* size, uint32_t* alloc);
    virtual void DetachBuffer(void** data, uint32_t* size, uint32_t* alloc);
    virtual uint8_t* GetHeaderSpace(uint32_t bytes);

    CDataStore();
    CDataStore(uint8_t* data, uint32_t size);
    int32_t FetchRead(uint32_t pos, uint32_t bytes);
    int32_t FetchWrite(uint32_t pos, uint32_t bytes, const char* fileName, int32_t lineNumber);
    CDataStore& Get(uint8_t& val);
    CDataStore& Get(uint16_t& val);
    CDataStore& Get(uint32_t& val);
    CDataStore& Get(int32_t& val);
    CDataStore& Get(uint64_t& val);
    CDataStore& Get(float& val);
    uint8_t GetByte();
    CDataStore& GetInt8(uint32_t& val);
    CDataStore& GetArray(uint8_t* pval, uint32_t count);
    CDataStore& GetArray(uint32_t* pval, uint32_t count);
    CDataStore& GetData(void* data, uint32_t bytes);
    CDataStore& GetDataInSitu(void*& data, uint32_t bytes);
    CDataStore& GetString(char* string, uint32_t maxChars);
    CDataStore& Put(uint8_t val);
    CDataStore& Put(uint16_t val);
    CDataStore& Put(uint32_t val);
    CDataStore& Put(uint64_t val);
    CDataStore& Put(float val);
    CDataStore& PutArray(const uint8_t* pval, uint32_t count);
    CDataStore& PutData(const void* data, uint32_t bytes);
    CDataStore& PutString(const char* string);
    CDataStore& Set(uint32_t pos, uint8_t val);
    CDataStore& Set(uint32_t pos, uint16_t val);
    CDataStore& Set(uint32_t pos, uint32_t val);
};

template <uint32_t CacheSize>
class CDataStoreCache : public CDataStore {
    public:
    /* 0018 */ uint8_t m_cache[CacheSize];

    void InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc) override;
    void InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc) override;
    int32_t InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber) override;
    ~CDataStoreCache() override;
    void DetachBuffer(void** data, uint32_t* size, uint32_t* alloc) override;

    CDataStoreCache();
};

// VERIFY: 94af1ab9 manual
// OFFSET: [0x008CCEE0<1024>]
template <uint32_t CacheSize>
CDataStoreCache<CacheSize>::CDataStoreCache() {
    this->InternalInitialize(this->m_data, this->m_base, this->m_alloc);
}

// VERIFY: 929e0c1c manual
// OFFSET: [0x008CD040, 0x008CD160, 0x008CD1C0]
template <uint32_t CacheSize>
CDataStoreCache<CacheSize>::~CDataStoreCache() {
    if (this->m_alloc != -1) {
        this->InternalDestroy(this->m_data, this->m_base, this->m_alloc);
    }
}

// VERIFY: 3368e2bb auto
// OFFSET: [0x008CCF30, 0x008CD090]
template <uint32_t CacheSize>
void CDataStoreCache<CacheSize>::InternalInitialize(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    data = this->m_cache;
    alloc = CacheSize;
}

// VERIFY: 91276e33 manual
// OFFSET: 0x008CCF50
template <uint32_t CacheSize>
void CDataStoreCache<CacheSize>::InternalDestroy(uint8_t*& data, uint32_t& base, uint32_t& alloc) {
    if (data == this->m_cache) {
        data = nullptr;
        alloc = 0;
    } else {
        if (data) {
            SMemFree(data, __FILE__, __LINE__, 0);
        }

        data = nullptr;
        alloc = 0;
    }
}

// VERIFY: a0ace69c auto
// OFFSET: 0x008CD0B0
template <uint32_t CacheSize>
int32_t CDataStoreCache<CacheSize>::InternalFetchWrite(uint32_t pos, uint32_t bytes, uint8_t*& data, uint32_t& base, uint32_t& alloc, const char* fileName, int32_t lineNumber) {
    uint32_t newAlloc = pos + bytes;

    if (newAlloc <= alloc) {
        return 1;
    }

    if (data != this->m_cache) {
        data = static_cast<uint8_t*>(CDataStore::ReAlloc(data, newAlloc, fileName, lineNumber));
        alloc = newAlloc;
        return 1;
    }

    uint8_t* newData;

    if (newAlloc) {
        newData = static_cast<uint8_t*>(fileName ? SMemAlloc(newAlloc, fileName, lineNumber, 0) : SMemAlloc(newAlloc, __FILE__, __LINE__, 0));
    } else {
        newData = nullptr;
    }

    data = newData;
    memcpy(newData, this->m_cache, pos < alloc ? pos : alloc);
    alloc = newAlloc;

    return 1;
}

// VERIFY: 1f74376a auto
// OFFSET: 0x008CCFC0
template <uint32_t CacheSize>
void CDataStoreCache<CacheSize>::DetachBuffer(void** data, uint32_t* size_, uint32_t* alloc) {
    void* detachedData;
    uint32_t detachedSize;

    CDataStore::DetachBuffer(&detachedData, &detachedSize, alloc);

    if (detachedData == this->m_cache && detachedSize) {
        detachedData = SMemAlloc(detachedSize, __FILE__, __LINE__, 0);
        memcpy(detachedData, this->m_cache, detachedSize);

        if (alloc) {
            *alloc = detachedSize;
        }
    }

    if (data) {
        *data = detachedData;
    }

    if (size_) {
        *size_ = detachedSize;
    }
}

#endif
