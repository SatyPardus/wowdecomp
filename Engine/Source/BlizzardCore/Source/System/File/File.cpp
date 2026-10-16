#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/Debug/ErrorStack.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/ThreadLocalStorage.h"

// STATIC: 0x00AB935C
static System_File::UsingFile s_usingFile;

// STATIC: 0x00AB90B0
Blizzard::File::Filesystem System_File::Stacked::s_initstack = {
    &System_File::Stacked::s_initstack,
    nullptr,
    {
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init,
        System_File::Stacked::file_init
    }
};

// STATIC: 0x00AB90AC
Blizzard::File::Filesystem* System_File::Stacked::s_manager = &System_File::Stacked::s_initstack;

// STATIC: 0x00AB91C0
Blizzard::File::Filesystem System_File::Stacked::s_nullstack = {
    &System_File::Stacked::s_nullstack,
    nullptr,
    {
        System_File::Stacked::null_cd,
        System_File::Stacked::null_close,
        System_File::Stacked::null_create,
        System_File::Stacked::null_cwd,
        System_File::Stacked::null_dirwalk,
        System_File::Stacked::null_exists,
        System_File::Stacked::null_flush,
        System_File::Stacked::null_getfileinfo,
        System_File::Stacked::null_getfreespace,
        System_File::Stacked::null_getpos,
        System_File::Stacked::null_getrootchars,
        System_File::Stacked::null_isabspath,
        System_File::Stacked::null_isreadonly,
        System_File::Stacked::null_makeabspath,
        System_File::Stacked::null_mkdir,
        System_File::Stacked::null_move,
        System_File::Stacked::null_copy,
        System_File::Stacked::null_open,
        System_File::Stacked::null_read,
        System_File::Stacked::null_readp,
        System_File::Stacked::null_rmdir,
        System_File::Stacked::null_setcachemode,
        System_File::Stacked::null_seteof,
        System_File::Stacked::null_setfileinfo,
        System_File::Stacked::null_setpos,
        System_File::Stacked::null_unlink,
        System_File::Stacked::null_write,
        System_File::Stacked::null_writep,
        System_File::Stacked::null_shutdown
    }
};

// STATIC: 0x00AB92C0
Blizzard::File::Filesystem System_File::Stacked::s_basestack = {
    &System_File::Stacked::s_basestack,
    nullptr,
    {
        System_File::SetWorkingDirectory,
        System_File::Close,
        System_File::Create,
        System_File::GetWorkingDirectory,
        System_File::ProcessDirFast,
        System_File::Exists,
        System_File::Flush,
        System_File::GetFileInfo,
        System_File::GetFreeSpace,
        System_File::GetPos,
        System_File::GetRootChars,
        System_File::IsAbsolutePath,
        System_File::IsReadOnly,
        System_File::MakeAbsolutePath,
        System_File::CreateDirectory,
        System_File::Move,
        System_File::Copy,
        System_File::Open,
        System_File::Read,
        System_File::ReadP,
        System_File::RemoveDirectory,
        System_File::SetCacheMode,
        System_File::SetEOF,
        System_File::SetAttributes,
        System_File::SetPos,
        System_File::Delete,
        System_File::Write,
        System_File::WriteP,
        System_File::Shutdown
    }
};

// STATIC: 0x009E5630
const Blizzard::File::FileInfo System_File::s_noinfo = { nullptr, 0, -1 };

// STATIC: 0x00B38308
Blizzard::Thread::LocalObject<System_File::FileError> System_File::s_error;

// STATIC: 0x00AB9130
int64_t System_File::Stacked::s_throttleRate = 56888;

// STATIC: 0x00B332C8
int64_t System_File::Stacked::s_throttleDebt;

// STATIC: 0x00B38024
int32_t System_File::Stacked::s_throttleDelay;

// STATIC: 0x00AB9138
Blizzard::File::Filesystem System_File::Stacked::s_throttlestack = {
    &System_File::Stacked::s_throttlestack,
    nullptr,
    {
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::throttle_read,
        System_File::Stacked::throttle_read,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::PassThrough,
        System_File::Stacked::null_shutdown
    }
};

bool System_File::Stacked::null_cd(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_close(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_create(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_cwd(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_dirwalk(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_exists(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_flush(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_getfileinfo(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_getfreespace(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_getpos(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_getrootchars(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_isabspath(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_isreadonly(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_makeabspath(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_mkdir(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_move(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_copy(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_open(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_read(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_readp(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_rmdir(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_setcachemode(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_seteof(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_setfileinfo(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_setpos(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_unlink(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_write(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_writep(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return false;
}

bool System_File::Stacked::null_shutdown(Blizzard::File::Filesystem* fs, FileParms* parms) {
    return true;
}

bool System_File::Shutdown(Blizzard::File::Filesystem* fs, System_File::Stacked::FileParms* parms) {
    return true;
}

// INLINED
System_File::FileError::FileError() {
    this->m_errorStack = nullptr;
    this->m_lastError = 0;
    this->unk_0004 = 0;
}

// INLINED
System_File::FileError::~FileError() {
    Blizzard::Debug::FreeErrorStack(this->m_errorStack);
    this->m_errorStack = nullptr;
}

// VERIFY: 664d977c manual
// OFFSET: 0x004270F0
bool Blizzard::File::Close(StreamRecord* file) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_close);
    parms.m_stream = file;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_close(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 16e511f0 manual
// OFFSET: 0x00427130
bool Blizzard::File::Delete(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_unlink);
    parms.m_path = const_cast<char*>(path);

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_unlink(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 80eb3fe1 auto
// OFFSET: 0x00427170
int32_t Blizzard::File::Exists(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_exists);
    parms.m_path = const_cast<char*>(path);
    parms.m_info = &parms.m_info2;

    if (path && System_File::Stacked::s_manager && System_File::Stacked::s_manager->m_verbs.m_exists(System_File::Stacked::s_manager, &parms)) {
        return parms.m_info->m_existence;
    }

    return 0;
}

// VERIFY: 9dc696d9 manual
// OFFSET: 0x004271D0
bool Blizzard::File::GetFileInfo(const char* path, FileInfo* info) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getfileinfo);
    parms.m_path = const_cast<char*>(path);
    parms.m_stream = nullptr;
    parms.m_has.m_all = 0;
    parms.m_want.m_all = 0xFFFFFFFF;
    parms.m_info = &parms.m_info2;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_getfileinfo(System_File::Stacked::s_manager, &parms);

    if (result) {
        *info = *parms.m_info;
    }

    return result;
}

// VERIFY: f950e621 auto
// OFFSET: 0x00427250
Blizzard::File::FileInfo* Blizzard::File::GetFileInfo(StreamRecord* file) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getfileinfo);
    parms.m_path = nullptr;
    parms.m_stream = file;
    parms.m_has.m_all = 0;
    parms.m_want.m_all = 0xFFFFFFFF;
    parms.m_info = nullptr;

    if (System_File::Stacked::s_manager && System_File::Stacked::s_manager->m_verbs.m_getfileinfo(System_File::Stacked::s_manager, &parms)) {
        return parms.m_info;
    }

    return const_cast<FileInfo*>(&System_File::s_noinfo);
}

// VERIFY: c3f16d01 manual
// OFFSET: 0x004272C0
bool Blizzard::File::GetFileInfo(StreamRecord* file, FileInfo* info) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getfileinfo);
    parms.m_path = nullptr;
    parms.m_stream = file;
    parms.m_info = nullptr;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_getfileinfo(System_File::Stacked::s_manager, &parms);

    if (result) {
        *info = *parms.m_info;
    }

    return result;
}

// VERIFY: 187269ed manual
// OFFSET: 0x00427330
bool Blizzard::File::GetFileInfo(Filesystem* fs, const char* path, FileInfo* info) {
    System_File::Stacked::FileParms parms;
    parms.m_path = const_cast<char*>(path);
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getfileinfo);
    parms.m_stream = nullptr;
    parms.m_has.m_all = 0;
    parms.m_want.m_all = 0xFFFFFFFF;
    parms.m_info = &parms.m_info2;

    if (!fs) {
        return false;
    }

    bool result = fs->m_verbs.m_getfileinfo(fs, &parms);

    if (result) {
        *info = *parms.m_info;
    }

    return result;
}

// VERIFY: 377f7ed4 manual
// OFFSET: 0x004273A0
bool Blizzard::File::GetPos(StreamRecord* file, int64_t* offset) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getpos);
    parms.m_stream = file;
    parms.m_pos = 0;
    parms.m_posMode = 1;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_getpos(System_File::Stacked::s_manager, &parms);

    if (result) {
        *offset = parms.m_pos;
    }

    return result;
}

// VERIFY: 0dfe2edf manual
// OFFSET: 0x00427410
int32_t Blizzard::File::GetRootChars(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_getrootchars);
    parms.m_path = const_cast<char*>(path);
    parms.m_pos = 0;

    bool result = System_File::Stacked::s_manager ? System_File::Stacked::s_manager->m_verbs.m_getrootchars(System_File::Stacked::s_manager, &parms) : false;

    return result ? static_cast<int32_t>(parms.m_pos) : 0;
}

// VERIFY: 38a9778a manual
// OFFSET: 0x00427470
bool Blizzard::File::GetWorkingDirectory(char* buffer, int32_t buffersize) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_cwd);
    parms.m_cwdbuf = buffer;
    parms.m_cwdbuflen = buffersize;
    *buffer = '\0';

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_cwd(System_File::Stacked::s_manager, &parms);
}

// VERIFY: c1354c78 manual
// OFFSET: 0x004274C0
bool Blizzard::File::IsAbsolutePath(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_isabspath);
    parms.m_path = const_cast<char*>(path);

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_isabspath(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 2ab3cec6 manual
// OFFSET: 0x00427500
bool Blizzard::File::MakeAbsolutePath(const char* rel, char* result, int32_t resultsize, bool canonicalize) {
    System_File::Stacked::FileParms parms;
    parms.m_path = const_cast<char*>(rel);
    parms.m_cwdbuf = result;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_makeabspath);
    parms.m_cwdbuflen = resultsize;
    parms.m_makeCanonical = canonicalize;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_makeabspath(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 7f9fd317 manual
// OFFSET: 0x00427560
bool Blizzard::File::Move(const char* src, const char* dst) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_move);
    parms.m_path = const_cast<char*>(src);
    parms.m_newpath = const_cast<char*>(dst);

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_move(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 6f79e9e7 manual
// OFFSET: 0x004275B0
bool Blizzard::File::Copy(const char* src, const char* dst, bool overwrite) {
    System_File::Stacked::FileParms parms;
    parms.m_path = const_cast<char*>(src);
    parms.m_verb = offsetof(Filesystem, m_verbs.m_copy);
    parms.m_newpath = const_cast<char*>(dst);
    parms.m_overwrite = overwrite;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_copy(System_File::Stacked::s_manager, &parms);
}

// VERIFY: f96ef994 manual
// OFFSET: 0x00427600
bool Blizzard::File::Open(const char* filename, int32_t flags, StreamRecord*& file) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_open);
    parms.m_stream = nullptr;
    parms.m_path = const_cast<char*>(filename);
    parms.m_mode = flags;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_open(System_File::Stacked::s_manager, &parms);

    if (result) {
        file = parms.m_stream;
    }

    return result;
}

// VERIFY: 21074bbc manual
// OFFSET: 0x00427660
bool Blizzard::File::ProcessDirFast(const char* path, void* cookie, ProcessDirCallback callback, bool treatSymlinksAsFiles) {
    System_File::Stacked::FileParms parms;
    parms.m_cookie = cookie;
    parms.m_path = const_cast<char*>(path);
    parms.m_verb = offsetof(Filesystem, m_verbs.m_dirwalk);
    parms.m_dircallback = callback;
    parms.m_sorted = false;
    parms.m_mode = treatSymlinksAsFiles;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_dirwalk(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 87e421fd manual
// OFFSET: 0x004276C0
bool Blizzard::File::Read(StreamRecord* file, void* data, int32_t* count) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_read);
    parms.m_buffer = data;
    parms.m_amount = *count;
    *count = 0;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_read(System_File::Stacked::s_manager, &parms);

    if (result) {
        *count = parms.m_amount;
    }

    return result;
}

// VERIFY: eaa6a867 manual
// OFFSET: 0x00427720
bool Blizzard::File::Read(StreamRecord* file, void* data, int64_t offset, int32_t* count) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_buffer = data;
    parms.m_pos = offset;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_readp);
    parms.m_amount = *count;
    *count = 0;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_readp(System_File::Stacked::s_manager, &parms);

    if (result) {
        *count = parms.m_amount;
    }

    return result;
}

// VERIFY: 80fc4844 manual
// OFFSET: 0x00427790
bool Blizzard::File::Read(Filesystem* fs, StreamRecord* file, void* data, int64_t offset, int32_t* count) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_buffer = data;
    parms.m_pos = offset;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_readp);
    parms.m_amount = *count;

    if (!fs) {
        return false;
    }

    bool result = fs->m_verbs.m_readp(fs, &parms);

    if (result) {
        *count = parms.m_amount;
    }

    return result;
}

// VERIFY: dd4c961c manual
// OFFSET: 0x00427800
bool Blizzard::File::RemoveDirectory(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_rmdir);
    parms.m_path = const_cast<char*>(path);
    parms.m_recursive = false;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_rmdir(System_File::Stacked::s_manager, &parms);
}

// VERIFY: cac3f529 manual
// OFFSET: 0x00427850
bool Blizzard::File::SetAttributes(const char* path, int32_t attributes) {
    System_File::Stacked::FileParms parms;
    parms.m_path = const_cast<char*>(path);
    parms.m_verb = offsetof(Filesystem, m_verbs.m_setfileinfo);
    parms.m_has.m_all = 4;
    parms.m_info2.m_attributes = attributes;
    parms.m_info = &parms.m_info2;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_setfileinfo(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 3cb8465c manual
// OFFSET: 0x004278B0
bool Blizzard::File::SetEOF(StreamRecord* file, int64_t offset, int32_t whence) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_pos = offset;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_seteof);
    parms.m_posMode = whence;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_seteof(System_File::Stacked::s_manager, &parms);
}

// VERIFY: c9a231e7 manual
// OFFSET: 0x00427910
bool Blizzard::File::SetPos(StreamRecord* file, int64_t offset, int32_t whence) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_pos = offset;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_setpos);
    parms.m_posMode = whence;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_setpos(System_File::Stacked::s_manager, &parms);
}

// VERIFY: 61995703 manual
// OFFSET: 0x00427970
bool Blizzard::File::SetWorkingDirectory(const char* path) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_cd);
    parms.m_path = const_cast<char*>(path);

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_cd(System_File::Stacked::s_manager, &parms);
}

// VERIFY: e4c5427a manual
// OFFSET: 0x004279B0
bool Blizzard::File::Write(StreamRecord* file, const void* data, int32_t* count) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_amount = *count;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_write);
    parms.m_buffer = const_cast<void*>(data);
    parms.m_pos = -1;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_write(System_File::Stacked::s_manager, &parms);

    if (result) {
        *count = parms.m_amount;
    }

    return result;
}

// VERIFY: dc122629 manual
// OFFSET: 0x00427A20
bool Blizzard::File::Write(StreamRecord* file, const void* data, int64_t offset, int32_t* count) {
    System_File::Stacked::FileParms parms;
    parms.m_stream = file;
    parms.m_buffer = const_cast<void*>(data);
    parms.m_pos = offset;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_writep);
    parms.m_amount = *count;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    bool result = System_File::Stacked::s_manager->m_verbs.m_writep(System_File::Stacked::s_manager, &parms);

    if (result) {
        *count = parms.m_amount;
    }

    return result;
}

// VERIFY: d3b229dc auto
// OFFSET: 0x004281D0
bool Blizzard::File::IsDirectory(const char* path) {
    return Exists(path) == 2;
}

// VERIFY: 11722698 manual
// OFFSET: 0x004281F0
bool Blizzard::File::IsFile(const char* path) {
    return Exists(path) == 1;
}

// VERIFY: 897a55e1 manual
// OFFSET: 0x00428210
bool Blizzard::File::SimpleGlob(const char* name, const char* pattern) {
    char p = *pattern;

    if (!p) {
        return !*name;
    }

    for (;;) {
        char c = *name;

        if (!c && p != '*') {
            return false;
        }

        char pc = p;
        pattern++;

        if (p == '*') {
            break;
        }

        if (p == '?') {
            if (!c) {
                return false;
            }
        } else {
            if (p == '\\' && *pattern) {
                pc = *pattern++;
            }

            if (pc != c && tolower(c) != tolower(pc)) {
                return false;
            }
        }

        p = *pattern;
        name++;

        if (!p) {
            return !*name;
        }
    }

    while (*pattern == '*') {
        pattern++;
    }

    char next = *pattern;

    if (!next) {
        return true;
    }

    if (next != '?' && next != '\\') {
        if (!*name) {
            return false;
        }

        while (next != *name) {
            if (!*++name) {
                return false;
            }
        }
    }

    if (!*name) {
        return false;
    }

    for (;;) {
        if (SimpleGlob(name, pattern)) {
            return true;
        }

        if (!*++name) {
            return false;
        }
    }
}

// VERIFY: 7278620e manual
// OFFSET: 0x0042CEA0
bool Blizzard::File::CreateDirectory(const char* path, bool recursive) {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Filesystem, m_verbs.m_mkdir);
    parms.m_path = const_cast<char*>(path);
    parms.m_recursive = recursive;
    parms.m_grantEveryone = false;

    if (!System_File::Stacked::s_manager) {
        return false;
    }

    return System_File::Stacked::s_manager->m_verbs.m_mkdir(System_File::Stacked::s_manager, &parms);
}

// VERIFY: d4d54f3b manual
// OFFSET: 0x0042CF50
void System_File::Shutdown() {
    System_File::Stacked::FileParms parms;
    parms.m_verb = offsetof(Blizzard::File::Filesystem, m_verbs.m_shutdown);

    if (!System_File::Stacked::s_manager) {
        return;
    }

    System_File::Stacked::s_manager->m_verbs.m_shutdown(System_File::Stacked::s_manager, &parms);

    while (System_File::Stacked::s_manager) {
        Blizzard::File::Filesystem* next = System_File::Stacked::s_manager->m_next;
        free(System_File::Stacked::s_manager);
        System_File::Stacked::s_manager = next;
    }
}

#define HOIST(verb) \
    if (!s_manager->m_verbs.verb) { \
        Blizzard::File::Filesystem* fs = s_manager; \
        while (fs->m_next) { \
            fs->m_verbs.verb = fs->m_next->m_verbs.verb; \
            fs = fs->m_next; \
        } \
        fs->m_verbs.verb = nullptr; \
    }

// VERIFY: 538ad083 manual
// OFFSET: 0x0042CFB0
void System_File::Stacked::HoistAll() {
    HOIST(m_cd);
    HOIST(m_close);
    HOIST(m_create);
    HOIST(m_cwd);
    HOIST(m_dirwalk);
    HOIST(m_exists);
    HOIST(m_flush);
    HOIST(m_getfileinfo);
    HOIST(m_getfreespace);
    HOIST(m_getpos);
    HOIST(m_getrootchars);
    HOIST(m_isabspath);
    HOIST(m_isreadonly);
    HOIST(m_makeabspath);
    HOIST(m_mkdir);
    HOIST(m_move);
    HOIST(m_copy);
    HOIST(m_open);
    HOIST(m_read);
    HOIST(m_readp);
    HOIST(m_rmdir);
    HOIST(m_setcachemode);
    HOIST(m_seteof);
    HOIST(m_setfileinfo);
    HOIST(m_setpos);
    HOIST(m_unlink);
    HOIST(m_write);
    HOIST(m_writep);
    HOIST(m_shutdown);
}

#undef HOIST

// VERIFY: ad3fbaa4 manual
// OFFSET: 0x0042D440
bool System_File::Stacked::file_init(Blizzard::File::Filesystem* fs, FileParms* parms) {
    s_manager = static_cast<Blizzard::File::Filesystem*>(malloc(sizeof(Blizzard::File::Filesystem)));
    Blizzard::String::MemCopy(s_manager, &s_nullstack, sizeof(Blizzard::File::Filesystem));

    Blizzard::File::Filesystem* base = static_cast<Blizzard::File::Filesystem*>(malloc(sizeof(Blizzard::File::Filesystem)));
    Blizzard::String::MemCopy(base, &s_basestack, sizeof(Blizzard::File::Filesystem));
    base->m_next = s_manager;
    s_manager = base;
    HoistAll();

    if (!s_manager) {
        return false;
    }

    return (*reinterpret_cast<FileFunc*>(reinterpret_cast<uintptr_t>(s_manager) + parms->m_verb))(s_manager, parms);
}

// VERIFY: 56577400 auto
// OFFSET: 0x00454210
Blizzard::Debug::ErrorStackRecord* Blizzard::File::GetLastErrorStack() {
    System_File::FileError& error = System_File::s_error;

    return error.m_errorStack;
}

// VERIFY: 53fd6945 auto
// OFFSET: 0x00454220
void Blizzard::File::SetLastErrorStack(Blizzard::Debug::ErrorStackRecord* stack) {
    System_File::FileError& error = System_File::s_error;

    if (error.m_errorStack != stack) {
        Blizzard::Debug::FreeErrorStack(error.m_errorStack);
    }

    error.m_errorStack = stack;
}

// VERIFY: 95676968 auto
// OFFSET: 0x00454250
void Blizzard::File::AddToLastErrorStack(int32_t errCode, const char* errStr, int32_t category) {
    System_File::FileError& error = System_File::s_error;
    Blizzard::Debug::ErrorStackRecord* stack = Blizzard::Debug::AddErrorToStack(error.m_errorStack, errCode, errStr ? errStr : "", category);

    if (stack) {
        SetLastErrorStack(stack);
    }
}

// VERIFY: c5aaecb9 auto
// OFFSET: 0x004541F0
void Blizzard::File::SetLastError(int32_t errorCode) {
    System_File::FileError& error = System_File::s_error;
    error.m_lastError = errorCode;
}

// VERIFY: fa086660 manual
// OFFSET: 0x00432CF0
bool System_File::LoadFile(Blizzard::File::StreamRecord* file, void** data, uint32_t* bytes) {
    Blizzard::File::FileInfo info;

    if (!Blizzard::File::GetFileInfo(file, &info)) {
        return false;
    }

    if (info.m_size < 0) {
        return false;
    }

    uint32_t size = static_cast<uint32_t>(info.m_size);

    if (info.m_size <= 0) {
        return false;
    }

    *data = Blizzard::Memory::Allocate(size + 1);
    int32_t count = size;

    if (Blizzard::File::Read(file, *data, 0, &count) && count == size) {
        static_cast<char*>(*data)[size] = '\0';

        if (bytes) {
            *bytes = size;
        }

        return true;
    }

    Blizzard::Memory::Free(*data);

    return false;
}

// VERIFY: d195c793 manual
// OFFSET: 0x00435B30
bool System_File::LoadFile(const char* filename, void** data, uint32_t* bytes, int32_t flags) {
    Blizzard::File::StreamRecord* file;

    if (!Blizzard::File::Open(filename, flags | 0x1001, file)) {
        return false;
    }

    bool result = LoadFile(file, data, bytes);
    Blizzard::File::Close(file);

    return result;
}

// VERIFY: 4035c774 manual
// OFFSET: 0x00427AA0
bool System_File::Stacked::PassThrough(Blizzard::File::Filesystem* fs, FileParms* parms) {
    if (!fs->m_next) {
        return false;
    }

    return (*reinterpret_cast<FileFunc*>(reinterpret_cast<uintptr_t>(fs->m_next) + parms->m_verb))(fs->m_next, parms);
}

// VERIFY: 8a8e85a7 manual
// OFFSET: 0x004378C0
bool System_File::Stacked::throttle_read(Blizzard::File::Filesystem* fs, FileParms* parms) {
    Blizzard::Time::Timestamp start = Blizzard::Time::GetTimestamp();
    bool result = fs->m_next ? (*reinterpret_cast<FileFunc*>(reinterpret_cast<uintptr_t>(fs->m_next) + parms->m_verb))(fs->m_next, parms) : false;
    Blizzard::Time::Timestamp end = Blizzard::Time::GetTimestamp();
    Throttle(parms->m_amount, end - start);

    return result;
}

// VERIFY: 7905c4fd manual
// OFFSET: 0x0044FA10
bool Blizzard::File::RemoveDirectoryAndContents_Internal::Callback(const ProcessDirParms& parms) {
    State* state = static_cast<State*>(parms.m_cookie);
    char path[1024];

    if (parms.m_isDir) {
        Blizzard::String::Format(path, sizeof(path), "%s%s/", parms.m_pathRoot, parms.m_filename);
        Blizzard::File::ProcessDirFast(path, state, Callback, true);

        if (!Blizzard::File::RemoveDirectory(path)) {
            state->m_success = false;
        }

        return true;
    }

    Blizzard::String::Format(path, sizeof(path), "%s%s", parms.m_pathRoot, parms.m_filename);

    if (state->m_removeReadOnly) {
        Blizzard::File::SetAttributes(path, 0x20);
    }

    if (Blizzard::Streaming::IsStreamedFile(path)) {
        return true;
    }

    if (!Blizzard::File::Delete(path)) {
        state->m_success = false;
    }

    return true;
}

// VERIFY: 03f16ff6 manual
// OFFSET: 0x00450970
bool Blizzard::File::RemoveDirectoryAndContents_Internal::State::Remove(const char* path, bool removeReadOnly) {
    this->m_removeReadOnly = removeReadOnly;
    this->m_success = true;

    return Blizzard::File::ProcessDirFast(path, this, Callback, true) && this->m_success;
}

// VERIFY: 3b3eafc9 manual
// OFFSET: 0x004518C0
bool Blizzard::File::RemoveDirectoryAndContents(const char* path, bool removeReadOnly) {
    if (Exists(path) != 2) {
        return false;
    }

    char buffer[1024];
    Blizzard::String::Copy(buffer, path, sizeof(buffer));
    Blizzard::String::ForceTrailingSeparator(buffer, sizeof(buffer), Blizzard::String::FirstPathSeparator(buffer));

    RemoveDirectoryAndContents_Internal::State state = {};

    if (!state.Remove(buffer, removeReadOnly)) {
        return false;
    }

    return RemoveDirectory(buffer);
}

// INLINED
System_File::UsingFile::UsingFile() : Blizzard::Using::Use("Blizzard::File") {
    System_Using::Intern(this->m_name)->shutdownproc = System_File::Shutdown;
    Blizzard::Using::DependsOn("Blizzard::File", "Blizzard::Memory");
    Blizzard::Using::DependsOn("Blizzard::File", "Blizzard::ThreadLocalStorage");
}
