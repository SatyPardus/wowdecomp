#ifndef ENGINE_SOURCE_BASE_TEXTRAINSTANCERECYCLER_H
#define ENGINE_SOURCE_BASE_TEXTRAINSTANCERECYCLER_H

#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/CDataRecycler.h"
#include "Engine/Source/Storm/H/stpl.h"

struct TExtraInstanceRecyclable {
    /* 0000 */ uint32_t m_recycleBytes;
};

template <class T>
class TExtraInstanceRecycler : public CDataRecycler {
    public:
    /* 0018 */ uint32_t m_recycleBytes;

    TExtraInstanceRecycler(uint32_t nodesPerBlock, int32_t maxNodes, uint32_t recycleBytes);
    T* New(uint32_t extraBytes);
    void Delete(T* ptr);
};

// INLINED
template <class T>
TExtraInstanceRecycler<T>::TExtraInstanceRecycler(uint32_t nodesPerBlock, int32_t maxNodes, uint32_t recycleBytes) : CDataRecycler(nodesPerBlock, maxNodes), m_recycleBytes(recycleBytes) {
}

// VERIFY: c8037aea manual
// OFFSET: 0x00480600
template <class T>
T* TExtraInstanceRecycler<T>::New(uint32_t extraBytes) {
    uint32_t bytes = sizeof(T) + extraBytes;
    void* data;
    uint32_t allocated;

    if (bytes > this->m_recycleBytes) {
        data = this->AllocData(bytes, &allocated, StormTypeName(typeid(T)), SLOG_OBJECT);
    } else {
        this->GetData(data, allocated, StormTypeName(typeid(T)), SLOG_OBJECT);

        if (allocated < bytes) {
            data = this->ReallocData(data, bytes, &allocated, StormTypeName(typeid(T)), SLOG_OBJECT);
        }
    }

    T* ptr = new (data) T;
    ptr->m_recycleBytes = allocated;

    return ptr;
}

// VERIFY: 626225ab auto
// OFFSET: 0x00480820
template <class T>
void TExtraInstanceRecycler<T>::Delete(T* ptr) {
    uint32_t bytes = ptr->m_recycleBytes;
    ptr->~T();

    if (bytes > this->m_recycleBytes) {
        this->FreeData(ptr, StormTypeName(typeid(T)), SLOG_OBJECT);
    } else {
        this->PutData(ptr, bytes, StormTypeName(typeid(T)), SLOG_OBJECT);
    }
}

#endif
