#if defined(__APPLE__)

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <libgen.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"

struct HOSFILE__ {
    int fd;
};

static std::string NormalizePath(const char* path) {
    std::string result = path ? path : "";
    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }
    return result;
}

static std::string Lower(std::string value) {
    for (char& c : value) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    return value;
}

static bool MatchPattern(const char* name, const char* pattern) {
    std::string n = Lower(name ? name : "");
    std::string p = Lower(pattern && *pattern ? pattern : "*");
    return fnmatch(p.c_str(), n.c_str(), 0) == 0;
}

static int32_t MakeDirectoryRecursive(const std::string& path) {
    if (path.empty()) {
        errno = EINVAL;
        return 0;
    }

    std::string current;
    size_t i = 0;
    if (path[0] == '/') {
        current = "/";
        i = 1;
    }

    while (i <= path.size()) {
        size_t next = path.find('/', i);
        std::string part = path.substr(i, next == std::string::npos ? std::string::npos : next - i);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '/') {
                current += '/';
            }
            current += part;
            if (mkdir(current.c_str(), 0777) != 0 && errno != EEXIST) {
                return 0;
            }
        }
        if (next == std::string::npos) {
            break;
        }
        i = next + 1;
    }

    return 1;
}

bool OsCloseFile(HOSFILE file) {
    if (!file || file == OS_INVALID_FILE) {
        errno = EINVAL;
        return false;
    }
    int result = close(file->fd);
    delete file;
    return result == 0;
}

int32_t OsFileExists(const char* fileName) {
    struct stat st;
    std::string path = NormalizePath(fileName);
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

int32_t OsDirectoryExists(const char* pathName) {
    struct stat st;
    std::string path = NormalizePath(pathName);
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

int32_t OsReadFile(HOSFILE file, void* buffer, uint32_t bytesToRead, uint32_t* bytesRead) {
    if (!file || file == OS_INVALID_FILE || !buffer || !bytesRead) {
        errno = EINVAL;
        return 0;
    }
    ssize_t result = read(file->fd, buffer, bytesToRead);
    if (result < 0) {
        *bytesRead = 0;
        return 0;
    }
    *bytesRead = static_cast<uint32_t>(result);
    return 1;
}

int32_t OsWriteFile(HOSFILE file, const void* buffer, uint32_t bytesToWrite, uint32_t* bytesWritten) {
    if (!file || file == OS_INVALID_FILE || !buffer || !bytesWritten) {
        errno = EINVAL;
        return 0;
    }
    ssize_t result = write(file->fd, buffer, bytesToWrite);
    if (result < 0) {
        *bytesWritten = 0;
        return 0;
    }
    *bytesWritten = static_cast<uint32_t>(result);
    return 1;
}

uint64_t OsGetFileSize(HOSFILE file) {
    if (!file || file == OS_INVALID_FILE) {
        errno = EINVAL;
        return 0xFFFFFFFFu;
    }
    struct stat st;
    return fstat(file->fd, &st) == 0 ? static_cast<uint64_t>(st.st_size) : 0xFFFFFFFFu;
}

uint32_t OsGetFileAttributes(const char* fileName) {
    if (!fileName) {
        errno = EINVAL;
        return 0;
    }
    std::string path = NormalizePath(fileName);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0xFFFFFFFFu;
    }
    uint32_t attr = 0;
    if (S_ISDIR(st.st_mode)) {
        attr |= OS_FILE_ATTRIBUTE_DIRECTORY;
    }
    if (!(st.st_mode & S_IWUSR)) {
        attr |= 0x1;
    }
    const char* base = strrchr(path.c_str(), '/');
    base = base ? base + 1 : path.c_str();
    if (base[0] == '.') {
        attr |= 0x2;
    }
    return attr ? attr : OS_FILE_ATTRIBUTE_NORMAL;
}

int32_t OsSetFileAttributes(const char* fileName, uint32_t attributes) {
    if (!fileName) {
        errno = EINVAL;
        return 0;
    }
    std::string path = NormalizePath(fileName);
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return 0;
    }
    mode_t mode = st.st_mode;
    if (attributes & 0x1) {
        mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
    } else {
        mode |= S_IWUSR;
    }
    return chmod(path.c_str(), mode) == 0;
}

int32_t OsMoveFile(const char* existingFileName, const char* newFileName) {
    if (!existingFileName || !newFileName) {
        errno = EINVAL;
        return 0;
    }
    std::string src = NormalizePath(existingFileName);
    std::string dst = NormalizePath(newFileName);
    return rename(src.c_str(), dst.c_str()) == 0;
}

int32_t OsCopyFile(const char* existingFileName, const char* newFileName, int32_t failIfExists) {
    if (!existingFileName || !newFileName) {
        errno = EINVAL;
        return 0;
    }
    std::string src = NormalizePath(existingFileName);
    std::string dst = NormalizePath(newFileName);
    int in = open(src.c_str(), O_RDONLY);
    if (in < 0) {
        return 0;
    }
    int flags = O_WRONLY | O_CREAT | (failIfExists ? O_EXCL : O_TRUNC);
    int out = open(dst.c_str(), flags, 0666);
    if (out < 0) {
        close(in);
        return 0;
    }
    char buffer[16384];
    int ok = 1;
    while (true) {
        ssize_t r = read(in, buffer, sizeof(buffer));
        if (r == 0) {
            break;
        }
        if (r < 0) {
            ok = 0;
            break;
        }
        char* p = buffer;
        while (r > 0) {
            ssize_t w = write(out, p, static_cast<size_t>(r));
            if (w < 0) {
                ok = 0;
                r = 0;
                break;
            }
            p += w;
            r -= w;
        }
    }
    close(out);
    close(in);
    return ok;
}

int32_t OsDeleteFile(const char* fileName) {
    if (!fileName) {
        errno = EINVAL;
        return 0;
    }
    std::string path = NormalizePath(fileName);
    return unlink(path.c_str()) == 0;
}

int32_t OsCreateDirectory(const char* pathName, int32_t recursive) {
    if (!pathName) {
        errno = EINVAL;
        return 0;
    }
    std::string path = NormalizePath(pathName);
    if (recursive) {
        return MakeDirectoryRecursive(path);
    }
    return mkdir(path.c_str(), 0777) == 0 || errno == EEXIST;
}

int32_t OsRemoveDirectory(const char* pathName) {
    if (!pathName) {
        errno = EINVAL;
        return 0;
    }
    std::string path = NormalizePath(pathName);
    return rmdir(path.c_str()) == 0;
}

static int32_t RemoveRecursive(const std::string& path, uint32_t flags) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return 0;
    }
    dirent* entry;
    int32_t ok = 1;
    while ((entry = readdir(dir)) != nullptr) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        std::string child = path + "/" + entry->d_name;
        struct stat st;
        if (lstat(child.c_str(), &st) != 0) {
            ok = 0;
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            ok &= RemoveRecursive(child, flags);
            ok &= (rmdir(child.c_str()) == 0);
        } else if (!(flags & 0x1) || access(child.c_str(), W_OK) == 0) {
            ok &= (unlink(child.c_str()) == 0);
        }
    }
    closedir(dir);
    ok &= (rmdir(path.c_str()) == 0);
    return ok;
}

int32_t OsRemoveDirectoryRecurse(const char* pathName, uint32_t flags) {
    if (!pathName) {
        errno = EINVAL;
        return 0;
    }
    return RemoveRecursive(NormalizePath(pathName), flags);
}

bool OsSetCurrentDirectory(const char* path) {
    if (!path) {
        errno = EINVAL;
        return false;
    }
    std::string normalized = NormalizePath(path);
    return chdir(normalized.c_str()) == 0;
}

int32_t OsGetCurrentDirectory(uint32_t bufferSize, char* buffer) {
    if (!buffer) {
        errno = EINVAL;
        return 0;
    }
    return getcwd(buffer, bufferSize) != nullptr;
}

bool OsFileList_Internal::Callback(const Blizzard::File::ProcessDirParms& parms) {
    (void)parms;
    return true;
}

char* OsFileGetDownloadFolder() {
    static char s_downloadfolder[260];
    if (!s_downloadfolder[0]) {
        getcwd(s_downloadfolder, sizeof(s_downloadfolder));
    }
    return s_downloadfolder;
}

HOSFILE OsCreateFile(const char* fileName, uint32_t desiredAccess, uint32_t shareMode, uint32_t createDisposition, uint32_t flagsAndAttributes, uint32_t extendedFileType) {
    (void)shareMode;
    (void)flagsAndAttributes;
    (void)extendedFileType;
    if (!fileName || !desiredAccess) {
        errno = EINVAL;
        return OS_INVALID_FILE;
    }

    int access = 0;
    if ((desiredAccess & OS_GENERIC_READ) && (desiredAccess & OS_GENERIC_WRITE)) {
        access = O_RDWR;
    } else if (desiredAccess & OS_GENERIC_WRITE) {
        access = O_WRONLY;
    } else {
        access = O_RDONLY;
    }

    int flags = access;
    switch (createDisposition) {
        case 1:
            flags |= O_CREAT | O_EXCL;
            break;
        case OS_CREATE_ALWAYS:
            flags |= O_CREAT | O_TRUNC;
            break;
        case OS_OPEN_EXISTING:
            break;
        case 4:
            flags |= O_CREAT;
            break;
        case 5:
            flags |= O_TRUNC;
            break;
        default:
            errno = EINVAL;
            return OS_INVALID_FILE;
    }

    std::string path = NormalizePath(fileName);
    int fd = open(path.c_str(), flags, 0666);
    if (fd < 0) {
        return OS_INVALID_FILE;
    }
    HOSFILE file = new HOSFILE__;
    file->fd = fd;
    return file;
}

int32_t OsFileList(const char* dir, const char* pattern, OsFileListCallback callback, void* param, int32_t includeHidden) {
    if (!dir || !callback) {
        errno = EINVAL;
        return 0;
    }

    std::string path = NormalizePath(dir);
    while (path.size() > 1 && path.back() == '/') {
        path.pop_back();
    }

    DIR* directory = opendir(path.empty() ? "." : path.c_str());
    if (!directory) {
        return 0;
    }

    dirent* entry;
    while ((entry = readdir(directory)) != nullptr) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        if (!includeHidden && entry->d_name[0] == '.') {
            continue;
        }
        if (!MatchPattern(entry->d_name, pattern)) {
            continue;
        }

        std::string child = path + "/" + entry->d_name;
        struct stat st;
        if (stat(child.c_str(), &st) != 0) {
            continue;
        }

        OS_FILE_DATA data = {};
        data.size = static_cast<uint32_t>(st.st_size);
        if (S_ISDIR(st.st_mode)) {
            data.flags |= OS_FILE_ATTRIBUTE_DIRECTORY;
        }
        if (!(st.st_mode & S_IWUSR)) {
            data.flags |= 0x1;
        }
        if (entry->d_name[0] == '.') {
            data.flags |= 0x2;
        }
        SStrCopy(data.fileName, entry->d_name, sizeof(data.fileName));

        if (callback(data, param)) {
            break;
        }
    }

    closedir(directory);
    return 1;
}

#endif
