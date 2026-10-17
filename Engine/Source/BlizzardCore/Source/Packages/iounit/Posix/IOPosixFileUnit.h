#ifndef ENGINE_SOURCE_BLIZZARDCORE_IOPOSIXFILEUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_IOPOSIXFILEUNIT_H

#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"

// A native leaf for the existing range/lock/MPQ IOUnit chains. OpenRead never
// creates or modifies data. Writable caches must explicitly use CreateWritable.
class IOPosixFileUnit final : public IOUnit {
public:
    static IOPosixFileUnit* OpenRead(const char* path);
    static IOPosixFileUnit* CreateWritable(const char* path);

    uint32_t GetType() override;
    bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead) override;
    bool Write(const void* buffer, int64_t offset, uint32_t count) override;
    bool Flush() override;
    bool GetFileSize(int64_t* size) override;
    bool SetFileSize(int64_t size) override;
    void* Win32Handle() override;
    bool Close() override;
    ~IOPosixFileUnit() override;

private:
    explicit IOPosixFileUnit(int descriptor);
    static IOPosixFileUnit* Open(const char* path, int flags);
    int m_descriptor;
};

#endif
