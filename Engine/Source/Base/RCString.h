#ifndef ENGINE_SOURCE_BASE_RCSTRING_H
#define ENGINE_SOURCE_BASE_RCSTRING_H

#include <cstdint>
#include "Engine/Source/Base/RefCount.h"
#include "Engine/Source/Storm/H/stpl.h"

class RCString : public TRefCnt {
    public:
    /* 0008 */ const char* m_str;

    RCString();
    virtual ~RCString();
    void Copy(const RCString& source);
    void Copy(const char* str);
};

class CStringMemory : public TSGrowableArray<void*> {
    public:
    /* 0010 */ char* m_cursor;
    /* 0014 */ char* m_blockEnd;

    CStringMemory();
    ~CStringMemory();
    void AllocateBlock(uint32_t size);
    char* New(const char* str);
};

class CStringManager {
    public:
    /* 0000 */ char* m_buckets[521];
    /* 0824 */ CStringMemory m_memory;

    static CStringManager* s_stringManager;

    static void DestroyManager();
    static CStringManager* Get();

    CStringManager();
    const char* Add(const char* str);
};

#endif
