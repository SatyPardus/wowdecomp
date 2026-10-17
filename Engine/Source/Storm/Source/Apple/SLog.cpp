#if defined(__APPLE__)
#include <algorithm>
#include <array>
#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unistd.h>
#include "Engine/Source/BlizzardCore/Source/System/Log/Log.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SOption.h"

namespace {
struct Log {
    std::filesystem::path path;
    uint32_t flags = 0;
    FILE* file = nullptr;
    size_t pending = 0;
    std::array<char, 65536> buffer;
    ~Log() { if (file) { fclose(file); } }
};
struct Registry {
    std::mutex mutex;
    uintptr_t sequence = 0;
    std::unordered_map<HSLOG, std::unique_ptr<Log>> logs;
};
Registry& Logs() { static Registry logs; return logs; }

bool Open(Log& log) {
    if (log.file) { return true; }
    if (log.flags & SLOG_FLAG_NOFILE) { return false; }
    std::error_code error;
    std::filesystem::create_directories(log.path.parent_path(), error);
    if (!error) { log.file = fopen(log.path.c_str(), log.flags & SLOG_FLAG_APPEND ? "ab" : "wb"); }
    if (!log.file) {
        SErrSetLastError(5);
        fprintf(stderr, "Cannot open Storm log %s: %s\n", log.path.c_str(),
            error ? error.message().c_str() : strerror(errno));
        return false;
    }
    return true;
}
void Flush(Log& log) {
    if (log.file && log.pending) {
        if (fflush(log.file) || fsync(fileno(log.file))) { SErrSetLastError(29); }
        else { log.pending = 0; }
    }
}
}

void STORMAPI SLogInitialize() { (void)Logs(); }

int32_t STORMAPI SLogCreate(const char* filename, uint32_t flags, HSLOG* result) {
    if (!filename || !*filename || !result) { SErrSetLastError(87); return 0; }
    *result = nullptr;
    auto log = std::make_unique<Log>();
    std::string normalized = filename;
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    log->path = normalized;
    if (log->path.is_relative()) {
        // All relative game logs, including Logs\\gx.log, use a writable root.
        // Never resolve them against the application bundle or original Data.
        char root[4096];
        System_Log::GetDefaultDirectory(root, sizeof(root));
        log->path = std::filesystem::path(root) / log->path;
    }
    log->flags = flags;
    if ((flags & SLOG_FLAG_OPENFILE) && !(flags & SLOG_FLAG_NOFILE) && !Open(*log)) { return 0; }
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    auto handle = reinterpret_cast<HSLOG>(++registry.sequence);
    registry.logs.emplace(handle, std::move(log));
    *result = handle;
    return 1;
}

void STORMAPI SLogClose(HSLOG handle) {
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    const auto entry = registry.logs.find(handle);
    if (entry != registry.logs.end()) { Flush(*entry->second); registry.logs.erase(entry); }
}
void STORMAPI SLogFlush(HSLOG handle) {
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    const auto entry = registry.logs.find(handle);
    if (entry != registry.logs.end()) { Flush(*entry->second); }
}
void STORMAPI SLogFlushAll() {
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    for (const auto& entry : registry.logs) { Flush(*entry.second); }
}
void STORMAPI SLogDestroy() {
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    for (const auto& entry : registry.logs) { Flush(*entry.second); }
    registry.logs.clear();
}

void STORMAPI SLogVWrite(HSLOG handle, const char* format, va_list args) {
    if (!format) { return; }
    auto& registry = Logs();
    std::lock_guard<std::mutex> lock(registry.mutex);
    const auto entry = registry.logs.find(handle);
    if (entry == registry.logs.end() || !Open(*entry->second)) { return; }
    auto& log = *entry->second;
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    const time_t seconds = std::chrono::system_clock::to_time_t(now);
    tm date{};
    localtime_r(&seconds, &date);
    char* buffer = log.buffer.data();
    const int prefix = snprintf(buffer, 64, "%u/%u %02u:%02u:%02u.%03u  ", date.tm_mon + 1,
        date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, unsigned(milliseconds % 1000));
    vsnoprintf(buffer + prefix, log.buffer.size() - prefix - 2, format, args);
    const size_t length = strlen(buffer);
    buffer[length] = '\r'; buffer[length + 1] = '\n';
    if (fwrite(buffer, 1, length + 2, log.file) != length + 2) { SErrSetLastError(29); }
    log.pending += length + 2;
    if (g_opt.echotooutputdebugstring) { fwrite(buffer, 1, length + 2, stderr); }
    if (log.pending >= 0xC000) { Flush(log); }
}
void SLogWrite(HSLOG log, const char* format, ...) {
    va_list args;
    va_start(args, format);
    SLogVWrite(log, format, args);
    va_end(args);
}
#endif
