#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_SFILEARCHIVES_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_SFILEARCHIVES_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/Digest.h"
#include "Engine/Source/BlizzardCore/Source/Packages/iounit/IOUnit.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/TempBuffer.h"

namespace Blizzard {
namespace Mopaq {
struct ArchiveOptions;
}
}

namespace System_SFile {

#pragma pack(push, 1)
struct BLOCKENTRY {
    /* 0000 */ int64_t m_offset;
    /* 0008 */ uint32_t m_compressedSize;
    /* 000C */ uint32_t m_fileSize;
    /* 0010 */ uint32_t m_flags;
    /* 0014 */ uint64_t m_fileTime;
    /* 001C */ uint32_t m_crc;
    /* 0020 */ Blizzard::Util::Digest<16> m_md5;
    /* 0030 */ bool m_md5Check;
};
#pragma pack(pop)

class BitPacked {
    public:
    /* 0000 */ uint8_t* m_data;
    /* 0004 */ uint32_t m_bitCount;

    BitPacked(uint32_t bitCount, uint8_t fill);
    ~BitPacked();
    void ReadBits(uint32_t start, uint32_t bits, uint8_t* out, uint32_t outSize) const;
    bool WriteBits(uint32_t start, uint32_t bits, const uint8_t* in, uint32_t inSize);
};

class BlockEntryTable {
    public:
    /* 0000 */ std::vector<uint32_t> m_flagTable;
    /* 0010 */ uint32_t m_entryBits;
    /* 0014 */ uint32_t m_offsetBit;
    /* 0018 */ uint32_t m_fileSizeBit;
    /* 001C */ uint32_t m_compressedSizeBit;
    /* 0020 */ uint32_t m_flagIndexBit;
    /* 0024 */ uint32_t m_md5Bit;
    /* 0028 */ uint32_t m_offsetBits;
    /* 002C */ uint32_t m_fileSizeBits;
    /* 0030 */ uint32_t m_compressedSizeBits;
    /* 0034 */ uint32_t m_flagIndexBits;
    /* 0038 */ uint32_t m_md5Bits;
    /* 003C */ uint32_t m_hashEntryBits;
    /* 0040 */ uint32_t m_hashExtraBits;
    /* 0044 */ int32_t m_hashBits;
    /* 0048 */ BitPacked* m_hashes;
    /* 004C */ BitPacked* m_entries;
    /* 0050 */ uint32_t m_count;
    /* 0054 */ std::vector<uint64_t> m_fileTimes;
    /* 0064 */ std::vector<uint32_t> m_crcs;
    /* 0074 */ std::vector<Blizzard::Util::Digest<16>> m_md5s;
    /* 0084 */ uint32_t m_tableFlags;

    BlockEntryTable(uint32_t count, uint32_t flags);
    ~BlockEntryTable();
    void AddFlags(uint32_t flags);
    BLOCKENTRY* GetMaxima(BLOCKENTRY* maxima, const std::vector<BLOCKENTRY>& entries);
    void InitializeHashCheckTable(int32_t bits);
    bool PackEntry(uint32_t index, const BLOCKENTRY* entry);
    void SetLayout(BLOCKENTRY& maxima);
    bool UnpackEntry(uint32_t index, BLOCKENTRY* entry);
    void UpdateLayout();
};

class HashEntryTable {
    public:
    static uint64_t CalculateHash(const char* name);

    /* 0000 */ uint32_t unk_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t m_indexBits;
    /* 000C */ uint8_t* m_tags;
    /* 0010 */ BitPacked* m_indices;
    /* 0014 */ uint32_t m_slotCount;
    /* 0018 */ int32_t m_hashBits;
    /* 001C */ uint8_t unused_001C[4];
    /* 0020 */ uint64_t m_hashMask;
    /* 0028 */ uint64_t m_hashTopBit;
    /* 0030 */ uint32_t m_entryBits;

    HashEntryTable(uint32_t count, int32_t hashBits);
    ~HashEntryTable();
    bool AddEntry(const char* name, uint32_t blockIndex, BlockEntryTable* blocks);
    uint32_t GetEntry(const uint64_t& hash, BlockEntryTable* blocks);
    void SetHashBits(int32_t bits);
};

struct HASHENTRY {
    /* 0000 */ uint32_t m_hashA;
    /* 0004 */ uint32_t m_hashB;
    /* 0008 */ uint16_t m_locale;
    /* 000A */ uint8_t m_platform;
    /* 000B */ uint8_t m_reserved;
    /* 000C */ uint32_t m_blockIndex;
};

#pragma pack(push, 4)
struct MPQHEADER {
    /* 0000 */ uint32_t m_magic;
    /* 0004 */ uint32_t m_headerSize;
    /* 0008 */ uint32_t m_archiveSize;
    /* 000C */ uint16_t m_formatVersion;
    /* 000E */ uint16_t m_sectorSizeShift;
    /* 0010 */ uint32_t m_hashTablePos;
    /* 0014 */ uint32_t m_blockTablePos;
    /* 0018 */ uint32_t m_hashTableSize;
    /* 001C */ uint32_t m_blockTableSize;
    /* 0020 */ uint64_t m_hiBlockTablePos;
    /* 0028 */ uint16_t m_hashTablePosHi;
    /* 002A */ uint16_t m_blockTablePosHi;
};
#pragma pack(pop)

struct ArchiveBuffers {
    /* 0000 */ Blizzard::Memory::TempBuffer<uint8_t, 4> unused_0000;
    /* 000C */ Blizzard::Memory::TempBuffer<uint8_t, 256> unused_000C;
    /* 0114 */ Blizzard::Memory::TempBuffer<uint8_t, 256> unused_0114;
    /* 021C */ std::string unused_021C;

    ~ArchiveBuffers();
};

class ARCHIVEREC {
    public:
    static bool WriteArchiveChanges(ARCHIVEREC* archive);

    /* 0000 */ Blizzard::Lock::RMutex m_mutex;
    /* 0018 */ IOUnit* m_file;
    /* 001C */ int32_t m_priority;
    /* 0020 */ int32_t m_id;
    /* 0024 */ ARCHIVEREC* m_parent;
    /* 0028 */ MPQHEADER m_header;
    /* 0054 */ uint32_t unused_0054;
    /* 0058 */ int64_t m_archiveOffset;
    /* 0060 */ int64_t m_archiveEnd;
    /* 0068 */ int64_t m_userDataOffset;
    /* 0070 */ uint32_t m_userDataSize;
    /* 0074 */ uint32_t m_flags;
    /* 0078 */ uint32_t m_sectorSize;
    /* 007C */ uint32_t unk_007C;
    /* 0080 */ bool m_compressedTables;
    /* 0081 */ uint8_t unused_0081[3];
    /* 0084 */ char m_name[260];
    /* 0188 */ std::string m_directPath;
    /* 01A4 */ BlockEntryTable* m_blockEntryTable;
    /* 01A8 */ HashEntryTable* m_hashEntryTable;
    /* 01AC */ std::vector<BLOCKENTRY> m_blockTable;
    /* 01BC */ std::vector<HASHENTRY> m_hashTable;
    /* 01CC */ std::map<Blizzard::Util::Digest<16>, uint32_t> m_blockKeys;
    /* 01D8 */ bool m_blockKeysBuilt;
    /* 01D9 */ bool m_tablesInitialized;
    /* 01DA */ uint16_t m_locale;
    /* 01DC */ uint8_t m_platform;
    /* 01DD */ uint8_t unused_01DD[3];
    /* 01E0 */ uint32_t m_attributeFlags;
    /* 01E4 */ std::vector<Blizzard::Mopaq::CompressionAlgorithm> m_compression;
    /* 01F4 */ uint32_t unused_01F4;
    /* 01F8 */ ArchiveBuffers* m_buffers;
    /* 01FC */ bool (*m_closeCallback)(ARCHIVEREC* archive);
    /* 0200 */ uint32_t unk_0200;
    /* 0204 */ std::map<std::string, bool> m_listfile;
    /* 0210 */ bool m_listfileLoaded;
    /* 0211 */ uint8_t unused_0211[3];
    /* 0214 */ int32_t m_listfileRefcount;
    /* 0218 */ int64_t m_dataEnd;
    /* 0220 */ Blizzard::Lock::Atomic m_internalRefs;
    /* 0224 */ Blizzard::Lock::Atomic m_externalRefs;

    ARCHIVEREC();
    ~ARCHIVEREC();
    bool AcquireListfileUncompressedBlockHash();
    void BuildBlockKeys();
    void BuildBlockTable();
    void ConvertToNewHashTableFormat();
    bool FillNewHashTable(int32_t hashBits);
    BLOCKENTRY GetBlockEntry(uint32_t index);
    bool IncreaseHashBits(int32_t* hashBits);
    void InitializeTables();
    void PackBlockTable();
    bool ReadAdditionalAttributes();
    void ReleaseListfile();
    bool RemoveInternalArchiveRef();
    void SetBlockCrc(uint32_t index, uint32_t crc);
    void SetBlockEntry(uint32_t index, const BLOCKENTRY& entry);
    void SetBlockFileTime(uint32_t index, uint64_t fileTime);
    void SetBlockMd5(uint32_t index, const Blizzard::Util::Digest<16>& md5);
    void SetMD5Check(uint32_t index, bool enabled);
};

struct FileLookup {
    /* 0000 */ bool m_checkedDisk;
    /* 0001 */ uint8_t unused_0001[3];
    /* 0004 */ ARCHIVEREC* m_archive;
    /* 0008 */ uint32_t m_hashIndex;
    /* 000C */ uint32_t m_blockIndex;
    /* 0010 */ char* m_path;
};

class ArchivePtr {
    public:
    /* 0000 */ ARCHIVEREC* m_archive;
    /* 0004 */ bool m_locked;
    /* 0005 */ uint8_t unused_0005[3];

    ArchivePtr(ARCHIVEREC* archive);
    ~ArchivePtr();
    void Reset(ARCHIVEREC* handle);
};

template <class T>
class HeapArray {
    public:
    /* 0000 */ uint32_t m_count;
    /* 0004 */ T* m_data;

    HeapArray(uint32_t count);
    ~HeapArray();
};

extern Blizzard::Util::Digest<16> s_emptyMd5;
extern Blizzard::Lock::Atomic s_nextArchiveId;
extern uint32_t s_lastError;

char* BackslashPath(const char* path, char* buffer, uint32_t size);
bool CheckFileExistsOnDisk(const char* name, uint32_t flags, char* result);
void ClearAllFileFlags();
void ConvertRelativePathName(const char* path, char* result);
bool CreateArchiveView(IOUnit* file, const char* name, int32_t priority, uint32_t userDataSize, ARCHIVEREC** result, const Blizzard::Mopaq::ArchiveOptions* options);
uint32_t FindBlockEntry(ARCHIVEREC* archive, const Blizzard::Util::Digest<16>& md5, bool* deleted);
void FinishFileWrite(void* context, uint32_t size);
ARCHIVEREC* GetArchivePtr(ARCHIVEREC* handle, bool addRef);
int64_t GetDataEnd(ARCHIVEREC* archive);
bool InitEmptyTables(ARCHIVEREC* archive, uint32_t hashCount);
bool OpenDirectPathAsArchive(const char* path, int32_t priority, uint32_t flags, ARCHIVEREC** result);
bool PrepareArchiveForWriting(ARCHIVEREC* archive);
bool RebuildHashTable(ARCHIVEREC* archive, uint32_t hashSize);
bool s_OpenArchive(ARCHIVEREC* archive, uint32_t flags, ARCHIVEREC** result);
uint32_t SearchHashTable(ARCHIVEREC* archive, const char* name, bool matchNeutral, bool* deleted);
bool SetFileCrc(void* context, uint32_t crc);
bool SetFileMd5(void* context, const uint8_t* md5);

}

int32_t FindFileInOneArchive(System_SFile::ARCHIVEREC* archive, const char* name, const uint64_t& hash, const Blizzard::Util::Digest<16>& md5, uint32_t flags, System_SFile::FileLookup* lookup);
int32_t GetFileBlockEntry(System_SFile::ARCHIVEREC* handle, const char* name, const Blizzard::Util::Digest<16>& md5, uint32_t flags, System_SFile::ARCHIVEREC** archiveOut, System_SFile::ARCHIVEREC** recordOut, uint32_t* blockIndex, uint32_t* hashIndex, char* path);

namespace Blizzard {
namespace Mopaq {

bool SFileAuthenticateArchiveEx(System_SFile::ARCHIVEREC* handle, AuthResult* result, const uint8_t* modulus, uint32_t modulusSize, const uint8_t* exponent, uint32_t exponentSize, const char* salt);
struct ArchiveOptions {
    /* 0000 */ uint32_t m_sectorSize;
    /* 0004 */ uint32_t unk_0004;
    /* 0008 */ bool m_compressedTables;
    /* 0009 */ uint8_t unused_0009[3];
};

bool SFileCloseArchive(System_SFile::ARCHIVEREC* handle);
bool SFileCreateArchive(const char* path, int32_t priority, uint32_t userDataSize, System_SFile::ARCHIVEREC** result, const ArchiveOptions* options);
bool SFileEnableDirectAccess(uint32_t enable);
int32_t SFileFileExists(System_SFile::ARCHIVEREC* handle, const char* name, uint32_t flags, System_SFile::ARCHIVEREC** archiveOut);
bool SFileGetFileDetails(System_SFile::ARCHIVEREC* handle, const char* name, System_SFile::ARCHIVEREC** archiveOut, int64_t* offset, uint32_t* size);
int32_t SFileFileIsLocal(const char* name, int32_t flags);
bool SFileGetIsLocalAmount(const char* name, int64_t& local, int64_t& total);
bool SFileMakeFileLocal(const char* name, int32_t priority, bool wait);
uint32_t SFileGetLastError();
bool SFileOpenArchive(const char* path, int32_t priority, uint32_t flags, System_SFile::ARCHIVEREC** result);
bool SFileOpenArchive(IOUnit* file, const char* name, int32_t priority, uint32_t flags, System_SFile::ARCHIVEREC** result);
bool SFileOpenArchiveView(System_SFile::ARCHIVEREC* handle, const char* name, int32_t priority, System_SFile::ARCHIVEREC** result);
bool SFileSetBasePath(const char* path);
bool SetFileSignature(const char* name, const Blizzard::Util::Digest<16>& md5);
void SetMd5Callback(bool (*callback)(void* context, const uint8_t* md5));
void SetMd5Mode(uint32_t mode);

}
}

// VERIFY: 6d32e555 manual
// OFFSET: [0x0042EA10, 0x0042ED30]
template <class T>
System_SFile::HeapArray<T>::HeapArray(uint32_t count) {
    this->m_count = count;
    this->m_data = count ? new T[count] : nullptr;
}

// INLINED
template <class T>
System_SFile::HeapArray<T>::~HeapArray() {
    delete[] this->m_data;
}

#endif
