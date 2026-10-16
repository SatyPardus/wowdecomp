#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOALIGNUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_IOUNIT_IOALIGNUNIT_H

#include <cstdint>
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"

class IOAlignUnit : public IOUnit {
    public:
    struct CacheBlock {
        /* 0000 */ int64_t m_offset;
        /* 0008 */ bool m_dirty;
        /* 0009 */ uint8_t unused_0009[3];
        /* 000C */ uint8_t* m_data;

        bool operator<(const CacheBlock& other) const;
    };

    /* 0010 */ int64_t m_base;
    /* 0018 */ int64_t m_size;
    /* 0020 */ int64_t m_writtenSize;
    /* 0028 */ uint32_t m_blockSize;
    /* 002C */ bool m_largeTransfers;
    /* 002D */ bool m_exactSize;
    /* 002E */ uint8_t unused_002E[2];
    /* 0030 */ uint32_t m_bufferAlign;
    /* 0034 */ bool m_skipCleanOnWrite;
    /* 0035 */ uint8_t unused_0035[3];
    /* 0038 */ uint32_t m_blockCount;
    /* 003C */ CacheBlock* m_blocks;
    /* 0040 */ CacheBlock m_block;
    /* 0050 */ uint8_t* m_alignedBuffer;
    /* 0054 */ void* m_buffer;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool Flush();
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual ~IOAlignUnit();
    virtual bool Close();

    static IOAlignUnit* Create(IOUnitContainer* container, int64_t base, uint32_t blockSize, uint32_t blockCount, bool largeTransfers, bool exactSize, uint32_t bufferAlign, bool skipCleanOnWrite, IOUnit* next);
    static IOAlignUnit* Create(IOUnitContainer* container, int64_t base, uint32_t blockSize, uint32_t blockCount, bool largeTransfers, bool exactSize, uint32_t bufferAlign, IOUnit* next);

    IOAlignUnit(int64_t base, uint32_t blockSize, uint32_t blockCount, bool largeTransfers, bool exactSize, uint32_t bufferAlign, bool skipCleanOnWrite, int64_t size, bool ownsMemory, IOUnit* next);
    void AllocateBuffer();
    bool CleanCache();
    bool CleanCacheEntry(uint32_t index);
    uint8_t* GetCachedBlock(int64_t offset, bool dirty);
    void MoveToBack(uint32_t index);
};

#endif
