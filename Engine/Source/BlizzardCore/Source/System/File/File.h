#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_FILE_FILE_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_SYSTEM_FILE_FILE_H

#include "Engine/Source/BlizzardCore/Source/System/Using/Using.h"
#include <cstdint>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "Engine/Source/BlizzardCore/Source/System/Debug/ErrorStack.h"
#include "Engine/Source/BlizzardCore/Source/System/File/Filestack.h"
#include "Engine/Source/BlizzardCore/Source/System/File/Types.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

#undef CreateDirectory
#undef GetFreeSpace
#undef RemoveDirectory

namespace Blizzard {
namespace File {

void AddToLastErrorStack(int32_t errCode, const char* errStr, int32_t category);
bool Close(StreamRecord* file);
bool Copy(const char* src, const char* dst, bool overwrite);
bool CreateDirectory(const char* path, bool recursive);
bool Delete(const char* path);
int32_t Exists(const char* path);
FileInfo* GetFileInfo(StreamRecord* file);
bool GetFileInfo(const char* path, FileInfo* info);
bool GetFileInfo(StreamRecord* file, FileInfo* info);
bool GetFileInfo(Filesystem* fs, const char* path, FileInfo* info);
Blizzard::Debug::ErrorStackRecord* GetLastErrorStack();
bool GetPos(StreamRecord* file, int64_t* offset);
int32_t GetRootChars(const char* path);
bool GetWorkingDirectory(char* buffer, int32_t buffersize);
bool IsAbsolutePath(const char* path);
bool IsDirectory(const char* path);
bool IsFile(const char* path);
bool MakeAbsolutePath(const char* rel, char* result, int32_t resultsize, bool canonicalize);
bool Move(const char* src, const char* dst);
bool Open(const char* filename, int32_t flags, StreamRecord*& file);
bool ProcessDirFast(const char* path, void* cookie, ProcessDirCallback callback, bool treatSymlinksAsFiles);
bool Read(StreamRecord* file, void* data, int32_t* count);
bool Read(StreamRecord* file, void* data, int64_t offset, int32_t* count);
bool Read(Filesystem* fs, StreamRecord* file, void* data, int64_t offset, int32_t* count);
bool RemoveDirectory(const char* path);
bool RemoveDirectoryAndContents(const char* path, bool removeReadOnly);
bool SetAttributes(const char* path, int32_t attributes);
bool SetEOF(StreamRecord* file, int64_t offset, int32_t whence);
void SetLastError(int32_t errorCode);
void SetLastErrorStack(Blizzard::Debug::ErrorStackRecord* stack);
bool SetPos(StreamRecord* file, int64_t offset, int32_t whence);
bool SetWorkingDirectory(const char* path);
bool SimpleGlob(const char* name, const char* pattern);
bool Write(StreamRecord* file, const void* data, int32_t* count);
bool Write(StreamRecord* file, const void* data, int64_t offset, int32_t* count);

}
}

namespace Blizzard {
namespace File {
namespace RemoveDirectoryAndContents_Internal {

struct State {
    /* 0000 */ bool m_success;
    /* 0001 */ bool m_removeReadOnly;

    bool Remove(const char* path, bool removeReadOnly);
};

bool Callback(const ProcessDirParms& parms);

}
}
}

namespace System_File {

class UsingFile : public Blizzard::Using::Use {
    public:
    UsingFile();
};

struct FileError {
    /* 0000 */ int32_t m_lastError;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ Blizzard::Debug::ErrorStackRecord* m_errorStack;

    FileError();
    ~FileError();
};

#if defined(_WIN32)
typedef bool (*IOFunc)(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif

extern Blizzard::Thread::LocalObject<FileError> s_error;
extern const Blizzard::File::FileInfo s_noinfo;
#if defined(_WIN32)
extern IOFunc read_func;
#endif
#if defined(_WIN32)
extern IOFunc write_func;
#endif
extern bool s_read_init;
extern bool s_write_init;
extern Blizzard::Lock::Atomic s_readInitCount;
extern Blizzard::Lock::Atomic s_writeInitCount;
extern Blizzard::Lock::Mutex s_ioMutex;
extern Blizzard::Lock::Mutex* s_ioLock;

bool Close(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Copy(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
void detect_io_mode();
bool Create(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool CreateDirectory(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Delete(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Exists(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Flush(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
int32_t FromNativeAttributes(int32_t attributes);
void FromNativeName(char* buffer, uint32_t buffersize, const char* str);
bool GetFileInfo(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
void GetFileInfoByFile(Blizzard::File::StreamRecord* file);
bool GetFreeSpace(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool GetPos(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool GetRootChars(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool GetWorkingDirectory(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool IsAbsolutePath(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool IsReadOnly(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool LoadFile(Blizzard::File::StreamRecord* file, void** data, uint32_t* bytes);
bool LoadFile(const char* filename, void** data, uint32_t* bytes, int32_t flags);
bool MakeAbsolutePath(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Move(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Open(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool ProcessDirFast(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Read(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
#if defined(_WIN32)
bool read_first(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
#if defined(_WIN32)
bool read_locked(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
#if defined(_WIN32)
bool read_overlapped(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
bool ReadP(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool RemoveDirectory(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool SetAttributes(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool SetCacheMode(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool SetEOF(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool SetPos(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool SetWorkingDirectory(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
#if defined(_WIN32)
bool ToCreateFlags(uint32_t flags, DWORD* sharemode, DWORD* desiredaccess, DWORD* creationdisposition, DWORD* flagsandattributes);
#endif
int32_t ToNativeAttributes(int32_t attributes);
void Shutdown();
bool Shutdown(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
bool Write(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);
#if defined(_WIN32)
bool write_first(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
#if defined(_WIN32)
bool write_locked(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
#if defined(_WIN32)
bool write_overlapped(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count);
#endif
bool WriteP(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms);

}

namespace System_File {
namespace Stacked {

extern Blizzard::File::Filesystem* s_manager;
extern Blizzard::File::Filesystem s_initstack;
extern Blizzard::File::Filesystem s_nullstack;
extern Blizzard::File::Filesystem s_basestack;
extern Blizzard::File::Filesystem s_throttlestack;
extern int64_t s_throttleRate;
extern int64_t s_throttleDebt;
extern int32_t s_throttleDelay;

bool file_init(Blizzard::File::Filesystem* fs, FileParms* parms);
void HoistAll();
bool PassThrough(Blizzard::File::Filesystem* fs, FileParms* parms);
bool stream_read(Blizzard::File::Filesystem* fs, FileParms* parms);
void Throttle(int64_t bytes, int64_t elapsed);
bool throttle_read(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_cd(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_close(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_copy(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_create(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_cwd(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_dirwalk(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_exists(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_flush(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_getfileinfo(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_getfreespace(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_getpos(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_getrootchars(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_isabspath(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_isreadonly(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_makeabspath(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_mkdir(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_move(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_open(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_read(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_readp(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_rmdir(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_setcachemode(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_seteof(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_setfileinfo(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_setpos(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_shutdown(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_unlink(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_write(Blizzard::File::Filesystem* fs, FileParms* parms);
bool null_writep(Blizzard::File::Filesystem* fs, FileParms* parms);

}
}

#endif
