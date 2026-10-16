#ifndef ENGINE_SOURCE_BLIZZARDCORE_SFILE2_CORE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SFILE2_CORE_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include "Engine/Source/BlizzardCore/Source/System/File/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/TempBuffer.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"
#include "Engine/Source/Storm/H/Types.h"

class IOMpqUnit;
class SEvent;

namespace System_SFile {

class ARCHIVEREC;

}

struct SOVERLAPPED {
    /* 0000 */ uint32_t Offset;
    /* 0004 */ SEvent* hEvent;
};

struct SArchive {
    /* 0000 */ uint32_t m_type;
    /* 0004 */ System_SFile::ARCHIVEREC* m_archive;
};

struct SFileLock {
    /* 0000 */ Blizzard::Lock::Mutex m_mutex;
    /* 0018 */ uint32_t unused_0018;
    /* 001C */ uint8_t unused_001C[16];

    SFileLock();
    ~SFileLock();
};

struct SFileStreamingStatus {
    /* 0000 */ bool m_enabled;
    /* 0001 */ bool m_initialized;
    /* 0002 */ bool m_flag2;
    /* 0003 */ bool m_flag3;
    /* 0004 */ uint32_t m_build;
};

class SFileImpl {
    public:
    /* 0000 */ uint32_t m_type;
    /* 0004 */ Blizzard::File::StreamRecord* m_stream;
    /* 0008 */ SArchive* m_archive;
    /* 000C */ char* m_name;
    /* 0010 */ void* m_actualName;
    /* 0014 */ uint32_t m_size;
    /* 0018 */ uint8_t* m_data;
    /* 001C */ uint32_t m_position;
    /* 0020 */ IOMpqUnit* m_file;
    /* 0024 */ uint32_t unused_0024;
    /* 0028 */ uint32_t unused_0028;
    /* 002C */ SFileLock* m_lock;

    SFileImpl(uint32_t type);
    ~SFileImpl();
};

class SFile {
    public:
    static uint32_t s_locale;

    static char m_basePath[260];

    static int32_t STORMAPI CloseArchive(SArchive* archive);
    static void STORMAPI CreateOverlapped(SOVERLAPPED* overlapped);
    static void Destroy();
    static void STORMAPI DestroyOverlapped(SOVERLAPPED* overlapped);
    static void DisableSFileCheckDisk();
    static int32_t STORMAPI EnableDirectAccess(uint32_t flags);
    static int32_t STORMAPI FileExists(const char* name);
    static int32_t STORMAPI FileExistsEx(const char* name, uint32_t flags);
    static int32_t STORMAPI FileExistsInArchives(const char* name);
    static bool STORMAPI FileGetIsLocalAmount(const char* name, int64_t& local, int64_t& total);
    static int32_t STORMAPI FileIsLocal(const char* name, int32_t flags);
    static int32_t STORMAPI FileIsLocal(SFileImpl* file, int32_t flags);
    static int32_t STORMAPI GetActualFileName(SFileImpl* file, char* buffer, uint32_t size);
    static int32_t STORMAPI GetBasePath(char* path, uint32_t size);
    static const char* GetDataPath();
    static const char* GetErrorDetails();
    static int32_t Initialize();
    static int32_t IsStreamingMode();
    static int32_t STORMAPI InitializeStreaming(const char* mfilPath);
    static int32_t IsTrial();
    static void SetStreamingStatus(bool enabled, bool initialized, bool flag2, bool flag3, uint32_t build);
    static int32_t STORMAPI List(SArchive* archive, int32_t (*callback)(const char* fileName, void* param), void* param);
    static int32_t STORMAPI LoadFile(const char* name, void** data, uint32_t* size, uint32_t extra, SOVERLAPPED* overlapped);
    static int32_t STORMAPI MakeFileLocal(const char* name, int32_t priority, bool wait);
    static int32_t STORMAPI MakeFileLocal(SFileImpl* file, int32_t priority, bool wait);
    static int32_t STORMAPI OpenArchive(const char* path, int32_t priority, uint32_t flags, SArchive** archive);
    static int32_t STORMAPI OpenEx(SArchive* archive, const char* name, uint32_t flags, SFileImpl** result);
    static int32_t STORMAPI OpenPathAsArchive(SArchive* archive, const char* path, int32_t priority, uint32_t flags, SArchive** result);
    static int32_t STORMAPI ReadFile(SArchive* archive, const char* name, void** data, uint32_t* size, uint32_t extra, uint32_t flags, SOVERLAPPED* overlapped);
    static void RebuildHash();
    static int32_t STORMAPI SetBasePath(const char* path);
    static int32_t STORMAPI SetDataPath(const char* path);
    static int32_t STORMAPI SetDataPathAlternate(const char* path);
    static int32_t STORMAPI SetFilePointer(SFileImpl* file, int32_t distance, int32_t* distanceHigh, uint32_t method);
    static void STORMAPI SetFilePriority(SFileImpl* file, int32_t urgent, int32_t unused);
    static void STORMAPI SetReadFatal(bool fatal);
    static int32_t STORMAPI Unload(void* data);

    /* 0000 */ uint8_t unused_0000[1];
};

namespace System_SFile2 {

struct SFileFatalInfo {
    /* 0000 */ bool m_fatal;

    SFileFatalInfo();
};

struct FileHashEntry {
    /* 0000 */ std::string m_path;
    /* 001C */ int32_t m_type;
};

struct FileHashCompare {
    size_t operator()(const std::string& key) const;
    bool operator()(const std::string& key1, const std::string& key2) const;
};

typedef std::map<std::string, FileHashEntry, FileHashCompare> FileHash;

typedef bool (*SOUNDCACHECALLBACK)(const void* data, uint32_t size, Blizzard::Memory::TempBuffer<uint8_t, 0x400000>* buffer);

struct ScanDirCookie {
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ const char* m_root;
    /* 0008 */ uint32_t m_rootLength;
    /* 000C */ const char* m_prefix;
};

extern char s_dataPath[260];
extern char s_dataPath2[260];
extern bool s_checkDisk;
extern uint32_t s_directAccess;
extern uint32_t s_useOldFindFile;
extern char s_hashedBasePath[260];
extern Blizzard::Lock::Event s_asyncOpenEvent;
extern Blizzard::Lock::Mutex s_soundCacheLock;
extern Blizzard::Lock::Mutex s_asyncOpenLock;
extern System_SFile::ARCHIVEREC* s_soundCacheArchive;
extern bool s_fileHashBuilt;
extern std::deque<std::string> s_asyncOpenQueue;
extern Blizzard::Lock::RMutex s_fileHashLock;
extern FileHash s_fileHash;
extern Blizzard::Thread::LocalObject<SFileFatalInfo> s_fatalInfo;
extern Blizzard::Memory::TempBuffer<uint8_t, 0x400000>* s_soundCacheBuffer;
extern volatile int32_t s_soundCacheStop;
extern volatile int32_t s_soundCacheStopped;
extern SOUNDCACHECALLBACK s_soundCacheCallback;

void BuildFileSystemHash();
bool InitSoundCache(SOUNDCACHECALLBACK callback);
bool ShutdownSoundCache();
uint32_t SoundCacheThread(void* param);
bool VerifyAndCacheFile(const char* name);
bool WriteSoundCacheFile(const char* name, const void* data, uint32_t size);
bool FindFile(const char* name, char* path, uint32_t size, uint32_t flags, uint32_t* type, System_SFile::ARCHIVEREC** archive);
bool OldFindFile(const char* name, char* path, uint32_t size, uint32_t flags, uint32_t* type);
bool QueueAsyncOpen(const char* name);
bool ScanDirForFileSystemHash(const char* directory);

}

int32_t STORMAPI SFileCloseFile(SFileImpl* file);
uint32_t STORMAPI SFileGetFileSize(SFileImpl* file, uint32_t* sizeHigh);
int32_t STORMAPI SFileAuthenticateArchiveEx(SArchive* archive, uint32_t* result, const uint8_t* modulus, uint32_t modulusSize, const uint8_t* exponent, uint32_t exponentSize);
int32_t STORMAPI SFileCloseArchive(SArchive* archive);
int32_t STORMAPI SFileLoadFileEx(SArchive* archive, const char* name, void** data, uint32_t* size, uint32_t extra, uint32_t flags, SOVERLAPPED* overlapped);
int32_t STORMAPI SFileOpenArchive(const char* path, int32_t priority, uint32_t flags, SArchive** archive);
int32_t STORMAPI SFileUnloadFile(void* data);
int32_t STORMAPI SFileOpenFile(const char* name, SFileImpl** result);
int32_t STORMAPI SFileReadFile(SFileImpl* file, void* buffer, uint32_t count, uint32_t* bytesRead, SOVERLAPPED* overlapped, uint32_t unused);

class MD5;

void SFileClearAllFileFlags();
int32_t STORMAPI SFileSetFileBlockEntryMD5(const char* name, const MD5& md5);
void STORMAPI SFileSetSignatureCallbacks(int32_t enable);

#endif
