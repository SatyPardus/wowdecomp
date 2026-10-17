#if defined(__APPLE__)
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <mutex>
#include <string>

namespace {
namespace File = Blizzard::File;
bool Fail(int error = EINVAL) {
    File::SetLastError(8); File::AddToLastErrorStack(error, std::strerror(error), 1); errno = error;
    return false;
}
bool Valid(HOSFILE file) { return file && file != OS_INVALID_FILE; }
File::StreamRecord* Stream(HOSFILE file) { return reinterpret_cast<File::StreamRecord*>(file); }
uint32_t Attributes(int32_t internal) {
    // Blizzard's directory bit is 0x40; OsFile exposes the Win32-compatible
    // 0x10 consumed by the original CVar and AddOns directory callers.
    return uint32_t(internal & 3) | (internal & 0x40 ? OS_FILE_ATTRIBUTE_DIRECTORY : 0) |
        (internal & 0x20 ? OS_FILE_ATTRIBUTE_NORMAL : 0);
}
bool Transfer(HOSFILE file, void* data, uint32_t size, uint32_t* transferred, bool write) {
    if (transferred) { *transferred = 0; }
    if (!Valid(file) || !transferred || (!data && size)) { return Fail(); }
    auto* cursor = static_cast<uint8_t*>(data);
    while (*transferred < size) {
        int32_t amount = int32_t(std::min(size - *transferred, uint32_t(INT_MAX)));
        const int32_t requested = amount;
        const bool ok = write ? File::Write(Stream(file), cursor, &amount) : File::Read(Stream(file), cursor, &amount);
        if (!ok) { return false; }
        *transferred += uint32_t(amount); cursor += amount;
        if (amount < requested) { break; }
    }
    return true;
}
}

bool OsCloseFile(HOSFILE file) { return Valid(file) ? File::Close(Stream(file)) : Fail(); }
int32_t OsReadFile(HOSFILE file, void* buffer, uint32_t size, uint32_t* read) { return Transfer(file, buffer, size, read, false); }
int32_t OsWriteFile(HOSFILE file, const void* buffer, uint32_t size, uint32_t* written) { return Transfer(file, const_cast<void*>(buffer), size, written, true); }
uint64_t OsGetFileSize(HOSFILE file) {
    if (!Valid(file)) { Fail(); return 0xFFFFFFFF; }
    auto* info = File::GetFileInfo(Stream(file));
    return info ? info->m_size : 0xFFFFFFFF;
}
int32_t OsFileExists(const char* path) { return path ? File::IsFile(path) : Fail(); }
int32_t OsDirectoryExists(const char* path) { return path ? File::IsDirectory(path) : Fail(); }
uint32_t OsGetFileAttributes(const char* path) {
    File::FileInfo info;
    if (!path) { Fail(); return 0xFFFFFFFF; }
    return File::GetFileInfo(path, &info) ? Attributes(info.m_attributes) : 0xFFFFFFFF;
}
int32_t OsSetFileAttributes(const char* path, uint32_t attributes) {
    if (!path) { return Fail(); }
    if (attributes & ~(3u | OS_FILE_ATTRIBUTE_DIRECTORY | OS_FILE_ATTRIBUTE_NORMAL)) { return Fail(ENOTSUP); }
    const uint32_t internal = (attributes & 3) | (attributes & OS_FILE_ATTRIBUTE_DIRECTORY ? 0x40 : 0) |
        (attributes & OS_FILE_ATTRIBUTE_NORMAL ? 0x20 : 0);
    return File::SetAttributes(path, internal);
}
int32_t OsMoveFile(const char* from, const char* to) { return from && to ? File::Move(from, to) : Fail(); }
int32_t OsCopyFile(const char* from, const char* to, int32_t failIfExists) { return from && to ? File::Copy(from, to, !failIfExists) : Fail(); }
int32_t OsDeleteFile(const char* path) { return path ? File::Delete(path) : Fail(); }
int32_t OsCreateDirectory(const char* path, int32_t recursive) { return path ? File::CreateDirectory(path, recursive != 0) : Fail(); }
int32_t OsRemoveDirectory(const char* path) { return path ? File::RemoveDirectory(path) : Fail(); }
int32_t OsRemoveDirectoryRecurse(const char* path, uint32_t flags) { return path ? File::RemoveDirectoryAndContents(path, flags & 1) : Fail(); }
bool OsSetCurrentDirectory(const char* path) { return path ? File::SetWorkingDirectory(path) : Fail(); }
int32_t OsGetCurrentDirectory(uint32_t size, char* buffer) {
    return buffer && size && size <= INT_MAX ? File::GetWorkingDirectory(buffer, int32_t(size)) : Fail();
}
char* OsFileGetDownloadFolder() {
    static char folder[PATH_MAX];
    static std::once_flag initialized;
    std::call_once(initialized, [] { File::GetWorkingDirectory(folder, sizeof(folder)); });
    return folder;
}
HOSFILE OsCreateFile(const char* path, uint32_t access, uint32_t share, uint32_t disposition, uint32_t attributes, uint32_t) {
    if (!path || !(access & (OS_GENERIC_READ | OS_GENERIC_WRITE)) ||
        (access & ~(OS_GENERIC_READ | OS_GENERIC_WRITE)) || (share & ~3u) || disposition < 1 || disposition > 5) {
        Fail(); return OS_INVALID_FILE;
    }
    if (attributes & ~(3u | OS_FILE_ATTRIBUTE_NORMAL)) { Fail(ENOTSUP); return OS_INVALID_FILE; }
    // Retain the original OsFile -> Blizzard mapping. Disposition 4 seeks to
    // EOF; disposition 5 uses the shared create/truncate mode, as on Windows.
    const uint32_t createMode[] = {0, 0xC00, 0x400, 0x1000, 0x200, 0x100};
    const uint32_t mode = ((access & OS_GENERIC_READ) ? 1 : 0) | ((access & OS_GENERIC_WRITE) ? 2 : 0) |
        (share << 2) | createMode[disposition];
    File::StreamRecord* file = nullptr;
    if (!File::Open(path, mode, file)) { return OS_INVALID_FILE; }
    if (!(attributes & OS_FILE_ATTRIBUTE_NORMAL) && !OsSetFileAttributes(path, attributes)) {
        File::Close(file); return OS_INVALID_FILE;
    }
    return reinterpret_cast<HOSFILE>(file);
}
bool OsFileList_Internal::Callback(const File::ProcessDirParms& parms) {
    auto& list = *static_cast<OsFileList_Internal*>(parms.m_cookie);
    if (!File::SimpleGlob(parms.m_filename, list.m_pattern)) { return true; }
    const std::string path = std::string(parms.m_pathRoot) + '/' + parms.m_filename;
    File::FileInfo info;
    if (!File::GetFileInfo(path.c_str(), &info)) { return true; }
    if (!list.m_includeHidden && (info.m_attributes & 2)) { return true; }
    OS_FILE_DATA data{};
    if (std::strlen(parms.m_filename) >= sizeof(data.fileName)) { return Fail(ENAMETOOLONG); }
    std::strcpy(data.fileName, parms.m_filename);
    data.size = uint32_t(info.m_size); data.flags = Attributes(info.m_attributes) & ~OS_FILE_ATTRIBUTE_NORMAL;
    return !list.m_callback(data, list.m_param);
}
int32_t OsFileList(const char* directory, const char* pattern, OsFileListCallback callback, void* param, int32_t hidden) {
    if (!directory || !pattern || !callback) { return Fail(); }
    // Original callers supply a trailing separator (or a filename suffix).
    std::string relative(directory);
    const auto separator = relative.find_last_of("/\\");
    if (separator != std::string::npos) { relative.resize(separator ? separator : 1); }
    char path[PATH_MAX];
    if (!File::MakeAbsolutePath(relative.c_str(), path, sizeof(path), false)) { return 0; }
    OsFileList_Internal list{callback, param, hidden, pattern};
    return File::ProcessDirFast(path, &list, OsFileList_Internal::Callback, false);
}
#endif
