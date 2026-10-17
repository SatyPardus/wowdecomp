#if defined(__APPLE__)
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"
#include "Engine/Source/Storm/H/storm.h"

namespace {
using Blizzard::File::StreamRecord;
using Blizzard::File::FileInfo;
using System_File::Stacked::FileParms;
struct NativeFile {
    int descriptor;
    std::mutex mutex;
    explicit NativeFile(int fd) : descriptor(fd) {}
};
NativeFile& Native(StreamRecord* stream) { return *static_cast<NativeFile*>(stream->m_fileHandle); }
std::string Path(const char* name) {
    std::string path = name ? name : "";
    std::replace(path.begin(), path.end(), '\\', '/');
    return path;
}
bool Failure(int code = 8, int nativeError = errno) {
    Blizzard::File::SetLastError(code);
    Blizzard::File::AddToLastErrorStack(nativeError, std::strerror(nativeError), 1);
    errno = nativeError;
    return false;
}
bool Result(int result, int code = 8) { return result == 0 || Failure(code); }
bool Result(const std::error_code& error) { return !error || Failure(8, error.value()); }
int64_t Timestamp(const timespec& time) {
    const __int128 value = (__int128(time.tv_sec) - 946684800) * 1000000000 + time.tv_nsec;
    return value < INT64_MIN ? INT64_MIN : value > INT64_MAX ? INT64_MAX : int64_t(value);
}
void Info(const struct stat& status, FileInfo& info, char* path) {
    info = {};
    info.m_path = path;
    info.m_size = status.st_size;
    info.m_regularFile = S_ISREG(status.st_mode);
    info.m_existence = S_ISDIR(status.st_mode) ? 2 : 1;
    info.m_attributes = (S_ISDIR(status.st_mode) ? 0x40 : 0x20) |
        ((status.st_mode & 0222) ? 0 : 1) | ((status.st_flags & UF_HIDDEN) ? 2 : 0);
    info.m_createTime = Timestamp(status.st_birthtimespec);
    info.m_modTime = Timestamp(status.st_mtimespec);
    info.m_accessTime = Timestamp(status.st_atimespec);
}
bool PathInfo(const char* path, FileInfo& info) {
    struct stat status;
    if (stat(Path(path).c_str(), &status)) { return Failure(); }
    Info(status, info, nullptr);
    return true;
}
bool CopyPath(const std::string& path, FileParms* parms) {
    if (!parms->m_cwdbuf || parms->m_cwdbuflen <= 0) { return Failure(8, EINVAL); }
    if (path.size() >= size_t(parms->m_cwdbuflen)) {
        parms->m_cwdbuf[0] = 0;
        return Failure(8, ENAMETOOLONG);
    }
    std::memcpy(parms->m_cwdbuf, path.c_str(), path.size() + 1);
    return true;
}
bool Transfer(FileParms* parms, bool write, bool positional) {
    const int32_t count = parms->m_amount;
    parms->m_amount = 0;
    if (!parms->m_stream || count < 0 || (!parms->m_buffer && count) ||
        (positional && (parms->m_pos < 0 || parms->m_pos > INT64_MAX - count))) {
        return Failure(8, EINVAL);
    }
    auto* stream = parms->m_stream;
    if (!(stream->m_mode & (write ? 2 : 1))) { return Failure(write ? 6 : 5, EBADF); }
    auto& native = Native(stream);
    // Positional reads need no shared cursor and may proceed concurrently.
    std::unique_lock<std::mutex> lock(native.mutex, std::defer_lock);
    if (!positional || write) { lock.lock(); }
    auto* buffer = static_cast<uint8_t*>(parms->m_buffer);
    while (parms->m_amount < count) {
        const auto done = parms->m_amount;
        const auto remaining = size_t(count - done);
        ssize_t bytes;
        if (positional) {
            bytes = write ? pwrite(native.descriptor, buffer + done, remaining, parms->m_pos + done)
                          : pread(native.descriptor, buffer + done, remaining, parms->m_pos + done);
        } else {
            bytes = write ? ::write(native.descriptor, buffer + done, remaining)
                          : ::read(native.descriptor, buffer + done, remaining);
        }
        if (bytes < 0) {
            if (errno == EINTR) { continue; }
            return Failure(write ? 6 : 5);
        }
        if (!bytes) { return !write || Failure(6, EIO); }
        parms->m_amount += int32_t(bytes);
        if (write) { stream->m_infoValid = false; }
    }
    if (write && (stream->m_mode & 0x40) && fsync(native.descriptor)) { return Failure(6); }
    return true;
}
}

namespace System_File {
using Blizzard::File::Filesystem;
bool Create(Filesystem*, FileParms*) { return Failure(9, ENOTSUP); }
bool Open(Filesystem*, FileParms* parms) {
    const std::string path = Path(parms->m_path);
    if (path.empty()) { return Failure(4, EINVAL); }
    const int mode = parms->m_mode;
    int flags = (mode & 2) ? ((mode & 1) ? O_RDWR : O_WRONLY) : O_RDONLY;
    if (mode & 0x800) { flags |= O_CREAT | O_EXCL; }
    else if (mode & 0x500) { flags |= O_CREAT | O_TRUNC; }
    else if (!(mode & 0x1000)) { flags |= O_CREAT; }
    if (mode & 0x40) { flags |= O_SYNC; }
    int descriptor;
    do { descriptor = ::open(path.c_str(), flags | O_CLOEXEC, 0666); } while (descriptor < 0 && errno == EINTR);
    if (descriptor < 0) { return Failure(4); }
    if ((mode & 0x40) && fcntl(descriptor, F_NOCACHE, 1)) {
        const int error = errno; ::close(descriptor); return Failure(4, error);
    }
    struct stat status;
    if (fstat(descriptor, &status)) {
        const int error = errno; ::close(descriptor); return Failure(4, error);
    }
    if (!S_ISREG(status.st_mode)) { ::close(descriptor); return Failure(4, EISDIR); }
    if ((mode & 0x200) && lseek(descriptor, 0, SEEK_END) < 0) {
        const int error = errno; ::close(descriptor); return Failure(4, error);
    }
    auto* native = new (std::nothrow) NativeFile(descriptor);
    if (!native) { ::close(descriptor); return Failure(4, ENOMEM); }
    auto* stream = parms->m_stream;
    if (!stream) {
        stream = static_cast<StreamRecord*>(SMemAlloc(sizeof(StreamRecord) + path.size() + 1, __FILE__, __LINE__, SMEM_FLAG_ZEROMEMORY));
        stream->m_pathname = reinterpret_cast<char*>(stream + 1);
        std::memcpy(stream->m_pathname, path.c_str(), path.size() + 1);
    }
    stream->m_fileHandle = native;
    stream->m_mode = mode;
    Info(status, stream->m_info, stream->m_pathname);
    stream->m_infoValid = true;
    parms->m_stream = stream;
    return true;
}
bool Close(Filesystem*, FileParms* parms) {
    if (!parms->m_stream) { return Failure(8, EINVAL); }
    auto* stream = parms->m_stream;
    auto* native = &Native(stream);
    const int result = ::close(native->descriptor);
    const int error = errno;
    delete native;
    SMemFree(stream, __FILE__, __LINE__, 0);
    return result == 0 || Failure(8, error);
}
bool Read(Filesystem*, FileParms* p) { return Transfer(p, false, false); }
bool ReadP(Filesystem*, FileParms* p) { return Transfer(p, false, true); }
bool Write(Filesystem*, FileParms* p) { return Transfer(p, true, false); }
bool WriteP(Filesystem*, FileParms* p) { return Transfer(p, true, true); }
bool Flush(Filesystem*, FileParms* p) { return Result(fsync(Native(p->m_stream).descriptor)); }
bool Exists(Filesystem*, FileParms* p) { return PathInfo(p->m_path, *p->m_info); }
void GetFileInfoByFile(StreamRecord* stream) {
    auto& native = Native(stream);
    std::lock_guard<std::mutex> lock(native.mutex);
    if (stream->m_infoValid) { return; }
    struct stat status;
    if (fstat(native.descriptor, &status)) { Failure(); return; }
    Info(status, stream->m_info, stream->m_pathname);
    stream->m_infoValid = true;
}
bool GetFileInfo(Filesystem*, FileParms* p) {
    if (p->m_path) { return PathInfo(p->m_path, *p->m_info); }
    if (!p->m_stream) { return Failure(8, EINVAL); }
    GetFileInfoByFile(p->m_stream);
    p->m_info = &p->m_stream->m_info;
    return p->m_stream->m_infoValid;
}
bool GetPos(Filesystem*, FileParms* p) {
    auto& native = Native(p->m_stream);
    std::lock_guard<std::mutex> lock(native.mutex);
    p->m_pos = lseek(native.descriptor, 0, SEEK_CUR);
    return p->m_pos >= 0 || Failure();
}
bool SetPos(Filesystem*, FileParms* p) {
    auto& native = Native(p->m_stream);
    std::lock_guard<std::mutex> lock(native.mutex);
    const int whence = p->m_posMode == 0 ? SEEK_SET : p->m_posMode == 1 ? SEEK_CUR : SEEK_END;
    return lseek(native.descriptor, p->m_pos, whence) >= 0 || Failure();
}
bool SetEOF(Filesystem*, FileParms* p) {
    auto& native = Native(p->m_stream);
    std::lock_guard<std::mutex> lock(native.mutex);
    int64_t base = 0;
    if (p->m_posMode == 1) {
        base = lseek(native.descriptor, 0, SEEK_CUR);
        if (base < 0) { return Failure(); }
    } else if (p->m_posMode) {
        struct stat status;
        if (fstat(native.descriptor, &status)) { return Failure(); }
        base = status.st_size;
    }
    int64_t size;
    if (__builtin_add_overflow(base, p->m_pos, &size) || size < 0) { return Failure(8, EINVAL); }
    if (ftruncate(native.descriptor, size)) { return Failure(); }
    p->m_stream->m_infoValid = false;
    return true;
}
bool SetCacheMode(Filesystem*, FileParms* p) {
    auto& native = Native(p->m_stream);
    std::lock_guard<std::mutex> lock(native.mutex);
    if (fcntl(native.descriptor, F_NOCACHE, (p->m_mode & 0x40) != 0)) { return Failure(); }
    p->m_stream->m_mode = (p->m_stream->m_mode & ~0x40) | (p->m_mode & 0x40);
    return true;
}
bool GetRootChars(Filesystem*, FileParms* p) { p->m_pos = (p->m_path && (p->m_path[0] == '/' || p->m_path[0] == '\\')); return true; }
bool IsAbsolutePath(Filesystem*, FileParms* p) { return (p->m_path && (p->m_path[0] == '/' || p->m_path[0] == '\\')); }
bool MakeAbsolutePath(Filesystem*, FileParms* p) {
    std::error_code error;
    auto path = std::filesystem::absolute(Path(p->m_path), error);
    if (!Result(error)) { return false; }
    if (p->m_makeCanonical) { path = path.lexically_normal(); }
    return CopyPath(path.string(), p);
}
bool GetWorkingDirectory(Filesystem*, FileParms* p) {
    std::error_code error;
    auto path = std::filesystem::current_path(error);
    return Result(error) && CopyPath(path.string(), p);
}
bool SetWorkingDirectory(Filesystem*, FileParms* p) { return Result(chdir(Path(p->m_path).c_str())); }
bool CreateDirectory(Filesystem*, FileParms* p) {
    std::error_code error;
    const auto path = Path(p->m_path);
    if (p->m_recursive) { std::filesystem::create_directories(path, error); }
    else { std::filesystem::create_directory(path, error); }
    return Result(error);
}
bool RemoveDirectory(Filesystem*, FileParms* p) { return Result(rmdir(Path(p->m_path).c_str())); }
bool Delete(Filesystem*, FileParms* p) { return Result(unlink(Path(p->m_path).c_str())); }
bool Move(Filesystem*, FileParms* p) {
    return Result(renamex_np(Path(p->m_path).c_str(), Path(p->m_newpath).c_str(), RENAME_EXCL));
}
bool Copy(Filesystem*, FileParms* p) {
    std::error_code error;
    std::filesystem::copy_file(Path(p->m_path), Path(p->m_newpath), p->m_overwrite ?
        std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none, error);
    return Result(error);
}
bool IsReadOnly(Filesystem*, FileParms* p) {
    struct stat status;
    if (stat(Path(p->m_path).c_str(), &status)) { return Failure(); }
    return !(status.st_mode & 0222);
}
bool GetFreeSpace(Filesystem*, FileParms* p) {
    struct statvfs status;
    auto path = std::filesystem::path(Path(p->m_path));
    if (statvfs(path.c_str(), &status)) {
        if (statvfs(path.parent_path().c_str(), &status)) { return Failure(); }
    }
    p->m_pos = int64_t(status.f_bavail) * status.f_frsize;
    return true;
}
bool SetAttributes(Filesystem*, FileParms* p) {
    if (p->m_has.m_all & 0x10) {
        const int64_t timestamp = p->m_info->m_modTime;
        timespec times[2]{{0, UTIME_OMIT}, {timestamp / 1000000000 + 946684800, timestamp % 1000000000}};
        if (times[1].tv_nsec < 0) { --times[1].tv_sec; times[1].tv_nsec += 1000000000; }
        if (futimens(Native(p->m_stream).descriptor, times)) { return Failure(); }
        p->m_stream->m_infoValid = false;
        p->m_has.m_all &= ~0x10u;
    }
    if (p->m_has.m_all & 4) {
        const auto path = Path(p->m_path);
        struct stat status;
        if (stat(path.c_str(), &status)) { return Failure(); }
        const auto attributes = p->m_info->m_attributes;
        if (attributes & ~(1 | 2 | 0x20 | 0x40)) { return Failure(9, ENOTSUP); }
        const auto mode = (attributes & 1) ? (status.st_mode & ~0222) : (status.st_mode | S_IWUSR);
        if (chmod(path.c_str(), mode)) { return Failure(); }
        const auto flags = (attributes & 2) ? status.st_flags | UF_HIDDEN : status.st_flags & ~UF_HIDDEN;
        if (flags != status.st_flags && chflags(path.c_str(), flags)) { return Failure(); }
        p->m_has.m_all &= ~4u;
    }
    return p->m_has.m_all == 0 || Failure(9, ENOTSUP);
}
bool ProcessDirFast(Filesystem*, FileParms* p) {
    std::error_code error;
    const std::string root = Path(p->m_path);
    std::filesystem::directory_iterator iterator(root, error), end;
    if (!Result(error)) { return false; }
    for (; iterator != end; iterator.increment(error)) {
        if (!Result(error)) { return false; }
        const auto& entry = *iterator;
        const auto status = p->m_mode ? entry.symlink_status(error) : entry.status(error);
        if (!Result(error)) { return false; }
        const std::string filename = entry.path().filename().string();
        Blizzard::File::ProcessDirParms callback{};
        callback.m_pathRoot = p->m_path;
        callback.m_filename = filename.c_str();
        callback.m_cookie = p->m_cookie;
        callback.m_isDir = std::filesystem::is_directory(status);
        if (!p->m_dircallback(callback)) { return false; }
    }
    return Result(error);
}
namespace Stacked {
void Throttle(int64_t bytes, int64_t elapsed) {
    if (s_throttleDelay > 0) { Blizzard::Thread::Sleep(s_throttleDelay); }
    if (s_throttleRate <= 0) { return; }
    s_throttleDebt += int64_t(double(bytes) / double(s_throttleRate) * 1000000000.0) - elapsed;
    s_throttleDebt = std::max(s_throttleDebt, int64_t(-1000000000));
    if (s_throttleDebt > 10000000) {
        const uint32_t milliseconds = uint32_t(std::min(s_throttleDebt / 1000000, int64_t(UINT32_MAX)));
        s_throttleDebt -= int64_t(milliseconds) * 1000000;
        Blizzard::Thread::Sleep(milliseconds);
    }
}
}
}
#endif
