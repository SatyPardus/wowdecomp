#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOFILEUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOFILEUNIT_H

#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"
#include "Engine/Source/BlizzardCore/Source/System/File/Types.h"

class IOFileUnit : public IOUnit {
    public:
    static IOFileUnit* Create(IOUnitContainer* container, const char* path, int32_t flags);

    /* 0010 */ Blizzard::File::StreamRecord* m_stream;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool Flush();
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual void* Win32Handle();
    virtual bool Close();

    IOFileUnit(Blizzard::File::StreamRecord* stream, bool ownsMemory);
#if defined(__APPLE__)
    bool ReadNative(int64_t offset, const char* path, void* buffer, uint32_t count, uint32_t* bytesRead);
#else
    bool ReadNative(uint32_t offset, const char* path, void* buffer, uint32_t count, uint32_t* bytesRead);
#endif
};

class IOUnbufferedFileUnit : public IOFileUnit {
    public:
    /* 0014 */ void* m_buffer;
    /* 0018 */ void* m_alignedBuffer;
    /* 001C */ uint32_t m_alignMask;
    /* 0020 */ int64_t m_size;

    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual bool Close();

    IOUnbufferedFileUnit(Blizzard::File::StreamRecord* stream, bool ownsMemory);
};

namespace System_Mopaq {

IOUnbufferedFileUnit* OpenDiskFile(IOUnitContainer* container, const char* path, int32_t flags);

}

#endif
