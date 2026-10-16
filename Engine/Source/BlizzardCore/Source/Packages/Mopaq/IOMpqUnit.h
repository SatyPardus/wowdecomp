#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_IOMPQUNIT_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_IOMPQUNIT_H

#include <cstdint>
#include <vector>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/Digest.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/TempBuffer.h"

class IOMpqUnit : public IOUnit {
    public:
    /* 0010 */ uint32_t m_position;
    /* 0014 */ System_SFile::ARCHIVEREC* m_archive;
    /* 0018 */ Blizzard::Memory::TempBuffer<char, 260> m_name;

    virtual uint32_t GetType();
    virtual ~IOMpqUnit();
    virtual bool Close();
    virtual System_SFile::ARCHIVEREC* GetArchive();
    virtual const char* GetName();
    virtual const char* GetPath() = 0;
    virtual bool GetCompressedSize(int64_t* size) = 0;
    virtual IOUnit* GetBaseUnit() = 0;
    virtual int64_t GetOffset() = 0;
    virtual uint64_t GetFileTime() = 0;
    virtual uint32_t GetCrc() = 0;
    virtual const Blizzard::Util::Digest<16>* GetMd5() = 0;
    virtual bool HasMd5Check() = 0;
    virtual void DisableMd5Check() = 0;
    virtual uint32_t GetFlags() = 0;
    virtual bool MarkDeleted() = 0;
    virtual bool IsEncrypted() = 0;

    IOMpqUnit(System_SFile::ARCHIVEREC* archive, const char* name, bool ownsMemory, IOUnit* next);
};

class IOMpqArchiveUnit : public IOMpqUnit {
    public:
    /* 0124 */ uint32_t m_blockIndex;
    /* 0128 */ System_SFile::BLOCKENTRY m_blockEntry;

    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual ~IOMpqArchiveUnit();
    virtual const char* GetPath();
    virtual bool GetCompressedSize(int64_t* size);
    virtual IOUnit* GetBaseUnit();
    virtual int64_t GetOffset();
    virtual uint64_t GetFileTime();
    virtual uint32_t GetCrc();
    virtual const Blizzard::Util::Digest<16>* GetMd5();
    virtual bool HasMd5Check();
    virtual void DisableMd5Check();
    virtual uint32_t GetFlags();
    virtual bool MarkDeleted();
    virtual bool IsEncrypted();

    IOMpqArchiveUnit(System_SFile::ARCHIVEREC* archive, const char* name, uint32_t blockIndex, bool ownsMemory, IOUnit* next);
};

class IOMpqArchiveReadUnit : public IOMpqArchiveUnit {
    public:
    static IOMpqArchiveReadUnit* Create(IOUnitContainer* container, System_SFile::ARCHIVEREC* archive, const char* name, uint32_t blockIndex, IOUnit* next);

    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);

    IOMpqArchiveReadUnit(System_SFile::ARCHIVEREC* archive, const char* name, uint32_t blockIndex, bool ownsMemory, IOUnit* next);
};

class IOMpqArchiveWriteUnit : public IOMpqArchiveUnit {
    public:
    static IOMpqArchiveWriteUnit* Create(IOUnitContainer* container, System_SFile::ARCHIVEREC* archive, const char* name, uint32_t blockIndex, IOUnit* next);

    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);

    IOMpqArchiveWriteUnit(System_SFile::ARCHIVEREC* archive, const char* name, uint32_t blockIndex, bool ownsMemory, IOUnit* next);
};

class IOMpqDiskUnit : public IOMpqUnit {
    public:
    static IOMpqDiskUnit* Create(IOUnitContainer* container, System_SFile::ARCHIVEREC* archive, const char* name, const char* path, IOUnit* next);

    /* 0124 */ Blizzard::Memory::TempBuffer<char, 260> m_path;

    virtual ~IOMpqDiskUnit();
    virtual const char* GetPath();
    virtual bool GetCompressedSize(int64_t* size);
    virtual IOUnit* GetBaseUnit();
    virtual int64_t GetOffset();
    virtual uint64_t GetFileTime();
    virtual uint32_t GetCrc();
    virtual const Blizzard::Util::Digest<16>* GetMd5();
    virtual bool HasMd5Check();
    virtual void DisableMd5Check();
    virtual uint32_t GetFlags();
    virtual bool MarkDeleted();
    virtual bool IsEncrypted();

    IOMpqDiskUnit(System_SFile::ARCHIVEREC* archive, const char* name, const char* path, bool ownsMemory, IOUnit* next);
};

class IOMpqReadUnit : public IOUnit {
    public:
    struct SectorJob {
        /* 0000 */ IOMpqReadUnit* m_unit;
        /* 0004 */ uint32_t m_first;
        /* 0008 */ uint32_t m_end;
        /* 000C */ uint8_t* m_scratch;
        /* 0010 */ uint32_t m_scratchSize;
        /* 0014 */ uint8_t* m_dest;
        /* 0018 */ uint32_t m_scratchLeft;
        /* 001C */ uint8_t* m_destCursor;
        /* 0020 */ bool m_success;
        /* 0021 */ uint8_t unused_0021[3];
    };

    static IOUnit* Create(IOUnitContainer* container, uint32_t fileSize, uint32_t compressedSize, uint32_t sectorSize, uint32_t flags, uint32_t key, IOUnit* next);
    static IOUnit* CreateSingle(IOUnitContainer* container, uint32_t fileSize, uint32_t compressedSize, uint32_t flags, uint32_t key, IOUnit* next);
    static bool DecompressBlocks(IOMpqReadUnit* unit, uint32_t offset, uint32_t size, uint8_t* source, uint32_t first, uint32_t count, uint8_t* dest);
    static void DecompressSectors(void* param);
    static void ReadSectors(SectorJob* job);
    static bool VerifyBlocks(IOMpqReadUnit* unit, uint32_t offset, uint32_t size, uint8_t* source, uint32_t first, uint32_t count, uint8_t* dest);

    /* 0010 */ Blizzard::Memory::TempBuffer<uint32_t, 10> m_sectorOffsets;
    /* 0040 */ Blizzard::Memory::TempBuffer<uint32_t, 8> m_checksums;
    /* 0068 */ uint32_t m_sectorSize;
    /* 006C */ uint32_t m_lastSectorSize;
    /* 0070 */ uint32_t m_flags;
    /* 0074 */ uint32_t m_key;
    /* 0078 */ uint32_t m_fileSize;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool Flush();
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual ~IOMpqReadUnit();
    virtual bool Close();

    IOMpqReadUnit(uint32_t fileSize, uint32_t compressedSize, uint32_t sectorCount, uint32_t sectorSize, uint32_t flags, uint32_t key, bool ownsMemory, IOUnit* next);
    IOMpqReadUnit(uint32_t fileSize, uint32_t compressedSize, uint32_t flags, uint32_t key, bool ownsMemory, IOUnit* next);
    uint32_t CountCompressed(uint32_t first, uint32_t end);
    uint32_t CountUncompressed(uint32_t first, uint32_t end);
    bool DecompressRun(uint32_t first, uint32_t count, uint8_t** dest, uint8_t** scratch, uint32_t* scratchLeft);
    bool LoadSectorTable();
    bool ReadRun(uint32_t first, uint32_t count, uint8_t** dest, uint8_t** scratch, uint32_t* scratchLeft);
    bool VerifyRun(uint32_t first, uint32_t count, uint8_t** dest, uint8_t** scratch, uint32_t* scratchLeft);
};

class IOMpqWriteUnit : public IOUnit {
    public:
    typedef void (*Callback)(void* context, uint32_t size);

    struct CompressJob {
        /* 0000 */ IOMpqWriteUnit* m_unit;
        /* 0004 */ const uint8_t* m_src;
        /* 0008 */ uint8_t* m_dst;
        /* 000C */ uint32_t m_sectorSize;
        /* 0010 */ uint32_t m_size;
        /* 0014 */ uint32_t* m_sizes;
        /* 0018 */ uint32_t* m_checksums;
        /* 001C */ bool m_success;
        /* 001D */ uint8_t unused_001D[3];
    };

    static void CompressBlock(void* param);
    static IOUnit* Create(IOUnitContainer* container, const std::vector<Blizzard::Mopaq::CompressionAlgorithm>& compression, uint32_t fileSize, uint32_t sectorSize, Callback callback, IOMpqWriteUnit** self, IOUnit* next);
    static IOUnit* CreateSingle(IOUnitContainer* container, const std::vector<Blizzard::Mopaq::CompressionAlgorithm>& compression, uint32_t fileSize, Callback callback, IOMpqWriteUnit** self, IOUnit* next);

    /* 0010 */ uint32_t m_fileSize;
    /* 0014 */ Blizzard::Memory::TempBuffer<uint32_t, 8> m_checksums;
    /* 003C */ bool m_hasChecksums;
    /* 003D */ uint8_t unused_003D[3];
    /* 0040 */ Blizzard::Memory::TempBuffer<uint32_t, 10> m_sectorOffsets;
    /* 0070 */ uint32_t m_sectorSize;
    /* 0074 */ uint32_t m_sectorIndex;
    /* 0078 */ std::vector<Blizzard::Mopaq::CompressionAlgorithm> m_compression;
    /* 0088 */ Callback m_callback;
    /* 008C */ void* m_context;

    virtual uint32_t GetType();
    virtual bool Read(void* buffer, int64_t offset, uint32_t count, uint32_t* bytesRead);
    virtual bool Write(const void* buffer, int64_t offset, uint32_t count);
    virtual bool GetFileSize(int64_t* size);
    virtual bool SetFileSize(int64_t size);
    virtual ~IOMpqWriteUnit();
    virtual bool Close();
    virtual void SetContext(void* context);

    IOMpqWriteUnit(const std::vector<Blizzard::Mopaq::CompressionAlgorithm>& compression, uint32_t fileSize, uint32_t sectorCount, uint32_t sectorSize, Callback callback, IOMpqWriteUnit** self, bool ownsMemory, IOUnit* next);
    IOMpqWriteUnit(const std::vector<Blizzard::Mopaq::CompressionAlgorithm>& compression, uint32_t fileSize, Callback callback, IOMpqWriteUnit** self, bool ownsMemory, IOUnit* next);
};

#endif
