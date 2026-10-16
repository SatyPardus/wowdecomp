#if defined(__APPLE__)

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/System/Debug/Debug.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/TempBuffer.h"
#include "Engine/Source/BlizzardCore/Source/System/Process/Process.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"
#include "Engine/Source/Storm/H/storm.h"

#undef CreateDirectory
#undef GetFreeSpace
#undef RemoveDirectory

struct OVERLAPPED {
    uintptr_t Internal;
    uintptr_t InternalHigh;
    uint32_t Offset;
    uint32_t OffsetHigh;
    void* hEvent;
};

namespace {

constexpr int kInvalidFd = -1;

int HandleToFd(void* handle) {
    return handle ? static_cast<int>(reinterpret_cast<intptr_t>(handle)) : kInvalidFd;
}

void* FdToHandle(int fd) {
    return reinterpret_cast<void*>(static_cast<intptr_t>(fd));
}

std::string NormalizePath(const char* path) {
    std::string out = path ? path : "";
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

std::vector<std::string> SplitPath(const std::string& path) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        std::string part = path.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty()) {
            parts.push_back(part);
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    return parts;
}

bool ExistsNative(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool ResolveCaseInsensitive(const std::string& input, std::string& resolved) {
    if (input.empty()) {
        resolved.clear();
        return false;
    }

    if (ExistsNative(input)) {
        resolved = input;
        return true;
    }

    bool absolute = input[0] == '/';
    std::string current = absolute ? "/" : ".";
    std::string display = absolute ? "/" : "";

    for (const std::string& rawPart : SplitPath(input)) {
        if (rawPart == ".") {
            continue;
        }
        if (rawPart == "..") {
            if (display.empty()) {
                display = "..";
            } else if (display != "/") {
                display += "/..";
            }
            current = display.empty() ? "." : display;
            continue;
        }

        DIR* dir = opendir(current.c_str());
        if (!dir) {
            resolved = input;
            return false;
        }

        std::string match;
        while (dirent* entry = readdir(dir)) {
            if (strcasecmp(entry->d_name, rawPart.c_str()) == 0) {
                match = entry->d_name;
                break;
            }
        }
        closedir(dir);

        if (match.empty()) {
            resolved = input;
            return false;
        }

        if (display.empty()) {
            display = match;
        } else if (display == "/") {
            display += match;
        } else {
            display += "/" + match;
        }
        current = display.empty() ? "." : display;
    }

    resolved = display.empty() ? (absolute ? "/" : ".") : display;
    return ExistsNative(resolved);
}

std::string ResolveExistingPath(const char* path) {
    std::string normalized = NormalizePath(path);
    std::string resolved;
    return ResolveCaseInsensitive(normalized, resolved) ? resolved : normalized;
}

std::string ResolveParentForCreate(const char* path) {
    std::string normalized = NormalizePath(path);
    size_t slash = normalized.find_last_of('/');
    if (slash == std::string::npos) {
        return normalized;
    }
    std::string parent = slash == 0 ? "/" : normalized.substr(0, slash);
    std::string leaf = normalized.substr(slash + 1);
    std::string resolvedParent;
    if (ResolveCaseInsensitive(parent, resolvedParent)) {
        return (resolvedParent == "/" ? "/" : resolvedParent + "/") + leaf;
    }
    return normalized;
}

Blizzard::Time::Timestamp StatTimeToTimestamp(const timespec& ts) {
    return static_cast<Blizzard::Time::Timestamp>(ts.tv_sec) * 1000000000LL + ts.tv_nsec;
}

int32_t AttributesFromStat(const struct stat& st) {
    int32_t attrs = 0x20;
    if (S_ISDIR(st.st_mode)) {
        attrs |= 0x40;
        attrs &= ~0x20;
    }
    if (access(".", W_OK) != 0) {
        attrs |= 0x1;
    }
    return attrs;
}

void FillInfoFromStat(Blizzard::File::FileInfo* info, const char* path, const struct stat& st) {
    info->m_path = const_cast<char*>(path);
    info->m_size = S_ISDIR(st.st_mode) ? 0 : static_cast<int64_t>(st.st_size);
    info->m_attributes = AttributesFromStat(st);
    info->m_createTime = StatTimeToTimestamp(st.st_birthtimespec);
    info->m_modTime = StatTimeToTimestamp(st.st_mtimespec);
    info->m_accessTime = StatTimeToTimestamp(st.st_atimespec);
    info->m_existence = S_ISDIR(st.st_mode) ? 2 : 1;
    info->m_regularFile = S_ISREG(st.st_mode);
}

int Whence(int mode) {
    if (mode == 1) {
        return SEEK_CUR;
    }
    if (mode == 2) {
        return SEEK_END;
    }
    return SEEK_SET;
}

int OpenFlagsFromMode(uint32_t mode) {
    int access;
    if ((mode & 0x3) == 0x3) {
        access = O_RDWR;
    } else if (mode & 0x2) {
        access = O_WRONLY;
    } else {
        access = O_RDONLY;
    }

    int flags = access;
    if ((mode & 0x100) || (mode & 0x400)) {
        flags |= O_CREAT;
        if (mode & 0x800) {
            flags |= O_EXCL;
        } else {
            flags |= O_TRUNC;
        }
    } else if (mode & 0x800) {
        flags |= O_CREAT | O_EXCL;
    } else if (!(mode & 0x1000)) {
        flags |= O_CREAT;
    }
    return flags;
}

bool ReadFd(int fd, void* buffer, int32_t* count, off_t* offset) {
    if (fd == kInvalidFd) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    if (!count || *count <= 0) {
        if (count) *count = 0;
        return true;
    }
    ssize_t result;
    do {
        result = offset ? pread(fd, buffer, *count, *offset) : read(fd, buffer, *count);
    } while (result < 0 && errno == EINTR);
    if (result < 0) {
        *count = 0;
        Blizzard::File::SetLastError(5);
        return false;
    }
    *count = static_cast<int32_t>(result);
    return true;
}

bool WriteFd(int fd, const void* buffer, int32_t* count, off_t* offset) {
    if (fd == kInvalidFd) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    if (!count || *count <= 0) {
        if (count) *count = 0;
        return true;
    }
    ssize_t result;
    do {
        result = offset ? pwrite(fd, buffer, *count, *offset) : write(fd, buffer, *count);
    } while (result < 0 && errno == EINTR);
    if (result < 0) {
        *count = 0;
        Blizzard::File::SetLastError(6);
        return false;
    }
    *count = static_cast<int32_t>(result);
    return true;
}

} // namespace

System_File::IOFunc System_File::read_func = System_File::read_first;
System_File::IOFunc System_File::write_func = System_File::write_first;
bool System_File::s_read_init;
Blizzard::Lock::Atomic System_File::s_readInitCount;
bool System_File::s_write_init;
Blizzard::Lock::Atomic System_File::s_writeInitCount;
Blizzard::Lock::Mutex System_File::s_ioMutex;
Blizzard::Lock::Mutex* System_File::s_ioLock;

int32_t System_File::ToNativeAttributes(int32_t attributes) {
    return attributes;
}

int32_t System_File::FromNativeAttributes(int32_t attributes) {
    return attributes;
}

void System_File::FromNativeName(char* buffer, uint32_t buffersize, const char* str) {
    if (!buffer || !buffersize) {
        return;
    }
    Blizzard::String::Copy(buffer, str ? str : "", buffersize);
}

bool System_File::ReadP(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    off_t offset = static_cast<off_t>(parms->m_pos);
    return ReadFd(HandleToFd(parms->m_stream->m_fileHandle), parms->m_buffer, &parms->m_amount, &offset);
}

bool System_File::WriteP(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    off_t offset = static_cast<off_t>(parms->m_pos);
    parms->m_stream->m_infoValid = false;
    return WriteFd(HandleToFd(parms->m_stream->m_fileHandle), parms->m_buffer, &parms->m_amount, &offset);
}

bool System_File::Read(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    return ReadFd(HandleToFd(parms->m_stream->m_fileHandle), parms->m_buffer, &parms->m_amount, nullptr);
}

bool System_File::Write(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    parms->m_stream->m_infoValid = false;
    return WriteFd(HandleToFd(parms->m_stream->m_fileHandle), parms->m_buffer, &parms->m_amount, nullptr);
}

bool System_File::Close(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    Blizzard::File::StreamRecord* stream = parms->m_stream;
    int fd = HandleToFd(stream->m_fileHandle);
    if (fd != kInvalidFd) {
        close(fd);
    }
    SMemFree(stream, __FILE__, __LINE__, 0);
    return true;
}

bool System_File::Open(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    int flags = OpenFlagsFromMode(parms->m_mode);
    std::string path = (flags & O_CREAT) ? ResolveParentForCreate(parms->m_path) : ResolveExistingPath(parms->m_path);
    int fd;
    do {
        fd = open(path.c_str(), flags, 0666);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0 && !(flags & O_CREAT)) {
        std::string fallback = NormalizePath(parms->m_path);
        if (fallback != path) {
            do {
                fd = open(fallback.c_str(), flags, 0666);
            } while (fd < 0 && errno == EINTR);
            path = fallback;
        }
    }
    if (fd < 0) {
        Blizzard::File::SetLastError(4);
        Blizzard::File::AddToLastErrorStack(4, Blizzard::String::QuickFormat<1024>("POSIX Open - %s", path.c_str()).m_buffer, 1);
        return false;
    }

    Blizzard::File::StreamRecord* stream = parms->m_stream;
    if (!stream) {
        uint32_t length = static_cast<uint32_t>(path.size());
        stream = static_cast<Blizzard::File::StreamRecord*>(SMemAlloc(sizeof(Blizzard::File::StreamRecord) + length + 1, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));
        stream->m_pathname = reinterpret_cast<char*>(stream + 1);
        Blizzard::String::Copy(stream->m_pathname, path.c_str(), length + 1);
    }
    stream->m_fileHandle = FdToHandle(fd);
    stream->m_mode = parms->m_mode;
    stream->m_infoValid = false;
    parms->m_stream = stream;
    return true;
}

void System_File::GetFileInfoByFile(Blizzard::File::StreamRecord* file) {
    if (!file || file->m_infoValid) {
        return;
    }
    struct stat st;
    if (fstat(HandleToFd(file->m_fileHandle), &st) != 0) {
        return;
    }
    FillInfoFromStat(&file->m_info, file->m_pathname, st);
    file->m_infoValid = true;
}

bool System_File::GetFileInfo(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (parms->m_path) {
        std::string path = ResolveExistingPath(parms->m_path);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            Blizzard::File::SetLastError(8);
            return false;
        }
        FillInfoFromStat(parms->m_info, parms->m_path, st);
        return true;
    }
    if (!parms->m_stream) {
        return false;
    }
    GetFileInfoByFile(parms->m_stream);
    parms->m_info = &parms->m_stream->m_info;
    return parms->m_stream->m_infoValid;
}

bool System_File::Exists(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    std::string path = ResolveExistingPath(parms->m_path);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        parms->m_info->m_existence = 0;
    } else {
        parms->m_info->m_existence = S_ISDIR(st.st_mode) ? 2 : 1;
    }
    return true;
}

bool System_File::GetPos(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    off_t pos = lseek(HandleToFd(parms->m_stream->m_fileHandle), 0, SEEK_CUR);
    if (pos < 0) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    parms->m_pos = pos;
    return true;
}

bool System_File::SetPos(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (lseek(HandleToFd(parms->m_stream->m_fileHandle), parms->m_pos, Whence(parms->m_posMode)) < 0) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    return true;
}

bool System_File::SetEOF(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    int fd = HandleToFd(parms->m_stream->m_fileHandle);
    off_t old = lseek(fd, 0, SEEK_CUR);
    off_t target = lseek(fd, parms->m_pos, Whence(parms->m_posMode));
    if (target < 0 || ftruncate(fd, target) != 0) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    if (old >= 0) {
        lseek(fd, old, SEEK_SET);
    }
    parms->m_stream->m_infoValid = false;
    return true;
}

bool System_File::Flush(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    return fsync(HandleToFd(parms->m_stream->m_fileHandle)) == 0;
}

bool System_File::Create(Blizzard::File::Filesystem*, System_File::Stacked::FileParms*) {
    Blizzard::File::SetLastError(9);
    return false;
}

bool System_File::Delete(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    return unlink(ResolveExistingPath(parms->m_path).c_str()) == 0;
}

bool System_File::Move(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    return rename(ResolveExistingPath(parms->m_path).c_str(), ResolveParentForCreate(parms->m_newpath).c_str()) == 0;
}

bool System_File::Copy(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (!parms->m_overwrite && Blizzard::File::Exists(parms->m_newpath)) {
        return false;
    }
    Blizzard::File::StreamRecord* input;
    if (!Blizzard::File::Open(parms->m_path, 0x1005, input)) {
        return false;
    }
    Blizzard::File::StreamRecord* output;
    if (!Blizzard::File::Open(parms->m_newpath, (parms->m_overwrite ? 0x400 : 0xC00) | 0x2, output)) {
        Blizzard::File::Close(input);
        return false;
    }
    char buffer[64 * 1024];
    bool ok = true;
    for (;;) {
        int32_t got = sizeof(buffer);
        if (!Blizzard::File::Read(input, buffer, &got)) {
            ok = false;
            break;
        }
        if (!got) {
            break;
        }
        int32_t put = got;
        if (!Blizzard::File::Write(output, buffer, &put) || put != got) {
            ok = false;
            break;
        }
    }
    Blizzard::File::Close(input);
    Blizzard::File::Close(output);
    if (!ok) {
        Blizzard::File::Delete(parms->m_newpath);
    }
    return ok;
}

bool System_File::CreateDirectory(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (!parms->m_path) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    std::string path = NormalizePath(parms->m_path);
    if (!parms->m_recursive) {
        return mkdir(ResolveParentForCreate(path.c_str()).c_str(), 0777) == 0 || errno == EEXIST;
    }
    std::string current;
    if (!path.empty() && path[0] == '/') {
        current = "/";
    }
    for (const std::string& part : SplitPath(path)) {
        if (part == ".") {
            continue;
        }
        if (current.empty() || current == "/") {
            current += part;
        } else {
            current += "/" + part;
        }
        if (mkdir(current.c_str(), 0777) != 0 && errno != EEXIST) {
            return false;
        }
    }
    return true;
}

bool System_File::RemoveDirectory(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    return rmdir(ResolveExistingPath(parms->m_path).c_str()) == 0;
}

bool System_File::ProcessDirFast(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    std::string path = ResolveExistingPath(parms->m_path);
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        Blizzard::Debug::Printf(Blizzard::Debug::StderrOutput, "failed to open %s\n", parms->m_path);
        Blizzard::File::SetLastError(8);
        return false;
    }
    Blizzard::File::ProcessDirParms dirParms;
    dirParms.m_pathRoot = parms->m_path;
    dirParms.m_cookie = parms->m_cookie;
    while (dirent* entry = readdir(dir)) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        std::string full = path;
        if (!full.empty() && full.back() != '/') {
            full += '/';
        }
        full += entry->d_name;
        struct stat st;
        bool isDir = false;
        if ((parms->m_mode ? lstat(full.c_str(), &st) : stat(full.c_str(), &st)) == 0) {
            isDir = S_ISDIR(st.st_mode);
        }
        dirParms.m_filename = entry->d_name;
        dirParms.m_isDir = isDir;
        if (!parms->m_dircallback(dirParms)) {
            closedir(dir);
            return false;
        }
    }
    closedir(dir);
    return true;
}

bool System_File::SetAttributes(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (parms->m_has.m_all & 0x10) {
        int fd = parms->m_stream ? HandleToFd(parms->m_stream->m_fileHandle) : kInvalidFd;
        timeval times[2];
        Blizzard::File::FileInfo info;
        if (parms->m_stream && Blizzard::File::GetFileInfo(parms->m_stream, &info)) {
            times[0].tv_sec = info.m_accessTime / 1000000000LL;
            times[0].tv_usec = (info.m_accessTime % 1000000000LL) / 1000;
        } else {
            gettimeofday(&times[0], nullptr);
        }
        times[1].tv_sec = parms->m_info->m_modTime / 1000000000LL;
        times[1].tv_usec = (parms->m_info->m_modTime % 1000000000LL) / 1000;
        if (fd != kInvalidFd ? futimes(fd, times) != 0 : utimes(ResolveExistingPath(parms->m_path).c_str(), times) != 0) {
            return false;
        }
        parms->m_has.m_all &= ~0x10u;
    }
    if (parms->m_has.m_all & 0x4) {
        std::string path = ResolveExistingPath(parms->m_path);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            return false;
        }
        mode_t mode = st.st_mode;
        if (parms->m_info->m_attributes & 0x1) {
            mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
        } else {
            mode |= S_IWUSR;
        }
        if (chmod(path.c_str(), mode) != 0) {
            return false;
        }
        parms->m_has.m_all &= ~0x4u;
    }
    return parms->m_has.m_all == 0;
}

bool System_File::IsReadOnly(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    std::string path = ResolveExistingPath(parms->m_path);
    return access(path.c_str(), W_OK) != 0;
}

bool System_File::GetRootChars(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    const char* path = parms->m_path;
    parms->m_pos = (path && (path[0] == '/' || path[0] == '\\')) ? 1 : 0;
    return true;
}

bool System_File::IsAbsolutePath(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    const char* path = parms->m_path;
    if (path && (path[0] == '/' || path[0] == '\\')) {
        return true;
    }
    Blizzard::File::SetLastError(8);
    return false;
}

bool System_File::MakeAbsolutePath(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    std::string path = NormalizePath(parms->m_path);
    char buffer[PATH_MAX];
    const char* result = realpath(path.c_str(), buffer);
    if (!result) {
        if (!path.empty() && path[0] == '/') {
            Blizzard::String::Copy(buffer, path.c_str(), sizeof(buffer));
        } else {
            char cwd[PATH_MAX];
            getcwd(cwd, sizeof(cwd));
            Blizzard::String::Format(buffer, sizeof(buffer), "%s/%s", cwd, path.c_str());
        }
    }
    Blizzard::String::Copy(parms->m_cwdbuf, buffer, parms->m_cwdbuflen);
    return true;
}

bool System_File::GetWorkingDirectory(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (!parms->m_cwdbuf || parms->m_cwdbuflen <= 0) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    return getcwd(parms->m_cwdbuf, parms->m_cwdbuflen) != nullptr;
}

bool System_File::SetWorkingDirectory(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    if (!parms->m_path) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    return chdir(ResolveExistingPath(parms->m_path).c_str()) == 0;
}

bool System_File::GetFreeSpace(Blizzard::File::Filesystem*, System_File::Stacked::FileParms* parms) {
    std::string path = parms->m_path ? ResolveExistingPath(parms->m_path) : ".";
    struct statfs fs;
    if (statfs(path.c_str(), &fs) != 0) {
        Blizzard::File::SetLastError(8);
        return false;
    }
    parms->m_pos = static_cast<int64_t>(fs.f_bavail) * static_cast<int64_t>(fs.f_bsize);
    return true;
}

bool System_File::SetCacheMode(Blizzard::File::Filesystem*, System_File::Stacked::FileParms*) {
    return true;
}

bool System_File::ToCreateFlags(uint32_t, DWORD* sharemode, DWORD* desiredaccess, DWORD* creationdisposition, DWORD* flagsandattributes) {
    if (sharemode) *sharemode = 0;
    if (desiredaccess) *desiredaccess = 0;
    if (creationdisposition) *creationdisposition = 0;
    if (flagsandattributes) *flagsandattributes = 0;
    return true;
}

bool System_File::read_overlapped(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    off_t offsetValue;
    off_t* offset = nullptr;
    if (overlapped) {
        offsetValue = (static_cast<off_t>(overlapped->OffsetHigh) << 32) | overlapped->Offset;
        offset = &offsetValue;
    }
    return ReadFd(HandleToFd(file->m_fileHandle), buffer, count, offset);
}

bool System_File::write_overlapped(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    off_t offsetValue;
    off_t* offset = nullptr;
    if (overlapped) {
        offsetValue = (static_cast<off_t>(overlapped->OffsetHigh) << 32) | overlapped->Offset;
        offset = &offsetValue;
    }
    file->m_infoValid = false;
    return WriteFd(HandleToFd(file->m_fileHandle), buffer, count, offset);
}

bool System_File::read_locked(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    return read_overlapped(file, buffer, overlapped, count);
}

bool System_File::write_locked(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    return write_overlapped(file, buffer, overlapped, count);
}

void System_File::detect_io_mode() {
    read_func = read_overlapped;
    write_func = write_overlapped;
}

bool System_File::read_first(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    if (!s_read_init) {
        detect_io_mode();
        s_read_init = true;
    }
    return read_func(file, buffer, overlapped, count);
}

bool System_File::write_first(Blizzard::File::StreamRecord* file, void* buffer, OVERLAPPED* overlapped, int32_t* count) {
    if (!s_write_init) {
        detect_io_mode();
        s_write_init = true;
    }
    return write_func(file, buffer, overlapped, count);
}

void System_File::Stacked::Throttle(int64_t bytes, int64_t elapsed) {
    if (s_throttleDelay > 0) {
        Blizzard::Process::Sleep(s_throttleDelay);
    }
    s_throttleDebt += static_cast<int64_t>(static_cast<double>(bytes) / static_cast<double>(s_throttleRate) * 1000000000.0) - elapsed;
    if (s_throttleDebt < -1000000000LL) {
        s_throttleDebt = -1000000000LL;
    } else if (s_throttleDebt > 10000000LL) {
        int32_t milliseconds = static_cast<int32_t>(s_throttleDebt / 1000000LL);
        s_throttleDebt -= static_cast<int64_t>(milliseconds) * 1000000LL;
        Blizzard::Process::Sleep(milliseconds);
    }
}

#endif
