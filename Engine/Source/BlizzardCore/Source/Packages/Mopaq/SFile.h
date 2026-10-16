#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_SFILE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_MOPAQ_SFILE_H

#include <cstdint>
#include "Engine/Source/BlizzardCore/Include/Blizzard/Util/Digest.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

class IOMpqUnit;
class IOUnit;
class IOUnitContainer;

namespace System_SFile {

class ARCHIVEREC;

struct RawReadInfo {
    /* 0000 */ bool m_retryNative;

    RawReadInfo();
};

struct ListfileParser {
    /* 0000 */ char* m_data;
    /* 0004 */ uint32_t m_pos;
    /* 0008 */ uint32_t m_size;
    /* 000C */ uint32_t m_special;
    /* 0010 */ char unused_0010[260];
    /* 0114 */ const char* m_token;
    /* 0118 */ bool m_isSpecial;
    /* 0119 */ uint8_t unused_0119[3];

    ~ListfileParser();
    bool Next();
};

extern Blizzard::Thread::LocalObject<RawReadInfo> s_rawReadInfo;
extern uint32_t s_threadingMode;

bool CheckCrc(void* context, uint32_t crc);
IOUnit* CreateRangeUnit(IOUnitContainer* container, int64_t start, int64_t end, IOUnit* next);
IOUnit* GetLockedUnit(IOUnit* file);
bool InsertLockedUnit(IOUnit* file, IOUnit* unit);
IOUnit* OpenFileUnit(IOUnitContainer* container, const char* path, int32_t mode);

}

namespace System_Mopaq {

bool CreateDiskFile(const char* name, uint32_t flags, const char* path, System_SFile::ARCHIVEREC* archive, IOMpqUnit** result);

}

namespace Blizzard {
namespace Mopaq {

bool SFileCloseFile(IOMpqUnit* file);
bool SFileCreateFile(System_SFile::ARCHIVEREC* handle, const char* name, uint32_t size, uint32_t flags, IOMpqUnit** result);
int32_t SFileFileIsLocal(IOMpqUnit* file, int32_t flags);
bool SFileGetArchiveName(System_SFile::ARCHIVEREC* handle, char* name, uint32_t size);
uint32_t SFileGetFileSize(IOMpqUnit* file, uint32_t* sizeHigh);
bool SFileLoadFile(System_SFile::ARCHIVEREC* handle, const char* name, void** data, uint32_t* size, uint32_t extra, uint32_t flags);
int32_t SFileOpenFileEx(System_SFile::ARCHIVEREC* handle, const char* name, const Blizzard::Util::Digest<16>& md5, uint32_t flags, IOMpqUnit** result);
int32_t SFileOpenFileEx(System_SFile::ARCHIVEREC* handle, const char* name, uint32_t flags, IOMpqUnit** result);
bool SFileReadFile(IOMpqUnit* file, void* buffer, uint32_t count, uint32_t* bytesRead);
bool SFileUnloadFile(void* data);
int32_t SFileSetFilePointer(IOMpqUnit* file, int32_t distance, int32_t* distanceHigh, uint32_t method);
void SFileSetThreadingMode(uint32_t mode);
bool SFileWriteFile(IOMpqUnit* file, const void* data, uint32_t size);

}
}

#endif
