#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOCHECKSUMUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOCHECKSUMUNIT_H

#include <cstdint>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/MD5.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"

class IOChecksumUnit : public IOUnit {
    public:
    struct Job {
        /* 0000 */ IOChecksumUnit* m_unit;
        /* 0004 */ const uint8_t* m_data;
        /* 0008 */ uint32_t m_count;
    };

    static void ChecksumJob(void* param);

    /* 0010 */ int64_t m_position;
    /* 0018 */ int64_t m_size;
    /* 0020 */ void* m_context;
    /* 0024 */ bool m_readToEnd;
    /* 0025 */ uint8_t unused_0025[3];

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool SetFileSize(int64_t size);
    virtual ~IOChecksumUnit();
    virtual bool Close();
    virtual void SetContext(void* context);
    virtual void Checksum(const uint8_t* data, uint32_t count) = 0;
    virtual bool Finish() = 0;

    IOChecksumUnit(int64_t size, bool readToEnd, IOChecksumUnit** self, bool ownsMemory, IOUnit* next);
    bool Clamp(int64_t offset, uint32_t* count, uint32_t* bytesRead);
    bool GetRange(const uint8_t* data, int64_t offset, uint32_t count, const uint8_t** rangeData, uint32_t* rangeCount);
    bool Update(const uint8_t* data, int64_t offset, uint32_t count);
};

class IOCrcUnit : public IOChecksumUnit {
    public:
    typedef bool (*Callback)(void* context, uint32_t crc);

    static IOUnit* Create(IOUnitContainer* container, int64_t size, Callback callback, bool readToEnd, IOChecksumUnit** self, IOUnit* next);

    /* 0028 */ Callback m_callback;
    /* 002C */ uint32_t m_crc;

    virtual void Checksum(const uint8_t* data, uint32_t count);
    virtual bool Finish();

    IOCrcUnit(int64_t size, Callback callback, bool readToEnd, IOChecksumUnit** self, bool ownsMemory, IOUnit* next);
};

class IOMd5Unit : public IOChecksumUnit {
    public:
    typedef bool (*Callback)(void* context, const uint8_t* md5);

    static IOUnit* Create(int64_t size, Callback callback, bool readToEnd, IOChecksumUnit** self, IOUnit* next);
    static IOUnit* Create(IOUnitContainer* container, int64_t size, Callback callback, bool readToEnd, IOChecksumUnit** self, IOUnit* next);

    /* 0028 */ Callback m_callback;
    /* 002C */ Blizzard::Util::MD5 m_md5;

    virtual void Checksum(const uint8_t* data, uint32_t count);
    virtual bool Finish();

    IOMd5Unit(int64_t size, Callback callback, bool readToEnd, IOChecksumUnit** self, bool ownsMemory, IOUnit* next);
};

#endif
