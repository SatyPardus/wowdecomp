#include <cstdint>
#include <cstring>
#include <new>
#include "Engine/Source/Base/RCString.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00B4166C
CStringManager* CStringManager::s_stringManager;

// INLINED
RCString::RCString() {
    this->Copy(nullptr);
}

// INLINED
RCString::~RCString() {
}

// VERIFY: e6cc38e8 auto
// OFFSET: 0x0047CCB0
void RCString::Copy(const RCString& source) {
    this->m_str = source.m_str;
}

// INLINED
CStringMemory::CStringMemory() {
    this->AllocateBlock(0);
}

// VERIFY: 73173e19 manual
// OFFSET: 0x0047CD50
CStringMemory::~CStringMemory() {
    for (uint32_t i = 0; i < this->Count(); i++) {
        SMemFree(this->m_data[i], __FILE__, __LINE__, 0);
    }
}

// VERIFY: 2806e6af manual
// OFFSET: 0x0047CDA0
void CStringMemory::AllocateBlock(uint32_t size) {
    if (size < 0x10000) {
        size = 0x10000;
    }

    uint32_t index = this->Count();
    this->SetCount(index + 1);
    (*this)[index] = SMemAlloc(size, __FILE__, __LINE__, 0);
    this->m_cursor = static_cast<char*>((*this)[index]);
    this->m_blockEnd = this->m_cursor + size;
}

// VERIFY: dc39ef0f auto
// OFFSET: 0x0047CE40
char* CStringMemory::New(const char* str) {
    uint32_t bytes = (SStrLen(str) + sizeof(char*) + 4) & ~3;

    if (this->m_cursor + bytes > this->m_blockEnd) {
        this->AllocateBlock(bytes);
    }

    char* entry = this->m_cursor;
    SStrCopy(entry + sizeof(char*), str, STORM_MAX_STR);
    this->m_cursor += bytes;

    return entry;
}

// VERIFY: b5fb4508 manual
// OFFSET: 0x0047CE90
const char* CStringManager::Add(const char* str) {
    uint32_t index = SStrHashHT(str) % 521;

    for (char* entry = this->m_buckets[index]; entry; entry = *reinterpret_cast<char**>(entry)) {
        if (!SStrCmp(str, entry + sizeof(char*), STORM_MAX_STR)) {
            return entry + sizeof(char*);
        }
    }

    char* entry = this->m_memory.New(str);
    *reinterpret_cast<char**>(entry) = this->m_buckets[index];
    this->m_buckets[index] = entry;

    return entry + sizeof(char*);
}

// VERIFY: 0fc843bc manual
// OFFSET: 0x0047CF00
CStringManager::CStringManager() {
    memset(this->m_buckets, 0, sizeof(this->m_buckets));
}

// VERIFY: f5732b49 manual
// OFFSET: 0x0047CF40
void CStringManager::DestroyManager() {
    delete CStringManager::s_stringManager;
    CStringManager::s_stringManager = nullptr;
}

// INLINED
CStringManager* CStringManager::Get() {
    if (!CStringManager::s_stringManager) {
        void* mem = SMemAlloc(sizeof(CStringManager), __FILE__, __LINE__, 0);
        CStringManager::s_stringManager = mem ? new (mem) CStringManager() : nullptr;
    }

    return CStringManager::s_stringManager;
}

// VERIFY: 881a5cd6 manual
// OFFSET: 0x0047CF80
void RCString::Copy(const char* str) {
    if (!str) {
        this->m_str = nullptr;
        return;
    }

    CStringManager* pManager = CStringManager::Get();

    if (!pManager) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    this->m_str = pManager->Add(str);
}
