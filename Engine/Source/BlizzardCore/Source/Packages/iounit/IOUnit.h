#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOUNIT_H

#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"

class IOUnitContainer;

class IOUnit {
    public:
    /* 0004 */ IOUnit* m_next;
    /* 0008 */ bool m_ownsMemory;
    /* 0009 */ uint8_t unused_0009[3];
    /* 000C */ Blizzard::Lock::Atomic m_refCount;

    virtual uint32_t GetType() = 0;
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool Flush();
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual void* Win32Handle();
    virtual ~IOUnit();
    virtual bool Close();

    IOUnit(IOUnit* next, bool ownsMemory);
    bool Release();
};

class IORangeUnit : public IOUnit {
    public:
    static IORangeUnit* Create(IOUnitContainer* container, int64_t start, int64_t end, IOUnit* next);

    /* 0010 */ int64_t m_start;
    /* 0018 */ int64_t m_end;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);

    IORangeUnit(int64_t start, int64_t end, bool ownsMemory, IOUnit* next);
    bool Clamp(int64_t* offset, uint32_t* count, uint32_t* bytesRead);
};

class IOLockUnit : public IOUnit {
    public:
    static IOLockUnit* Create(IOUnitContainer* container, uint32_t flags, IOUnit* next);
    static IOLockUnit* Create(IOUnitContainer* container, IOUnit* next);
    static bool ReplaceNext(IOUnit* lock, IOUnit* next);

    /* 0010 */ Blizzard::Lock::Mutex* m_mutex;
    /* 0014 */ Blizzard::Lock::Mutex m_ownMutex;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool Flush();
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual void* Win32Handle();
    virtual ~IOLockUnit();

    IOLockUnit(IOUnit* next, bool ownsMemory);
};

#endif
