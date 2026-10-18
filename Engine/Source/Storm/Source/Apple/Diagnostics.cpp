#if defined(__APPLE__)
#include <cstdio>
#include <cstdlib>
#include <array>
#include <atomic>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SOption.h"
#include "Engine/Source/BlizzardCore/Source/System/Log/Log.h"

static thread_local uint32_t s_lastError;
namespace {
using Handler = int32_t (STORMAPI*)(uint32_t, const char*, const char*, int32_t, const char*);
using Header = int32_t (STORMAPI*)(char*, uint32_t);
struct Diagnostics {
    std::mutex mutex;
    std::recursive_mutex reportMutex;
    std::array<char, 128> title{};
    Header header = nullptr;
    std::vector<Handler> handlers;
    std::string lastPath;
    std::atomic<unsigned> displaying{0};
};
Diagnostics& State() { static Diagnostics state; return state; }
thread_local bool reporting;
struct Origin { std::array<char, 1024> file{}; int32_t line = 0; };
thread_local Origin prepared;

void WriteReport(const char* title, Header header, uint32_t error, const char* errorText,
    const char* file, int32_t line, const char* description) {
    auto& state = State();
    { std::lock_guard<std::mutex> lock(state.mutex); state.lastPath.clear(); }
    if (g_opt.serrsuppresslogs) return;
    char root[4096]{}; System_Log::GetDefaultDirectory(root, sizeof(root));
    const auto directory = std::filesystem::path(root) / "Errors";
    std::filesystem::create_directories(directory);
    const std::string pattern = (directory / "error-XXXXXX.txt").string();
    std::vector<char> name(pattern.begin(), pattern.end()); name.push_back(0);
    const int descriptor = mkstemps(name.data(), 4);
    if (descriptor < 0) return;
    fcntl(descriptor, F_SETFD, FD_CLOEXEC);
    std::unique_ptr<FILE, decltype(&fclose)> output(fdopen(descriptor, "w"), fclose);
    if (!output) { close(descriptor); unlink(name.data()); return; }
    const time_t now = time(nullptr); tm utc{}; gmtime_r(&now, &utc);
    std::fprintf(output.get(), "%s\n%04d-%02d-%02d %02d:%02d:%02d UTC; process %ld\nStorm error 0x%08x: %s\n%s:%d\n%s\n",
        title, utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min, utc.tm_sec,
        long(getpid()), error, errorText, file ? file : "?", line, description ? description : "");
    if (header) {
        char text[5200]{};
        try { header(text, sizeof(text)); }
        catch (...) { std::snprintf(text, sizeof(text), "Client error-log callback failed"); }
        text[sizeof(text) - 1] = 0;
        std::fprintf(output.get(), "\n%s\n", text);
    }
    if (std::ferror(output.get()) || std::fflush(output.get()) || fsync(descriptor)) return;
    output.reset();
    std::lock_guard<std::mutex> lock(state.mutex); state.lastPath = name.data();
}

void Report(uint32_t error, const char* file, int32_t line, const char* description) {
    auto& state = State();
    std::array<char, 128> title;
    Header header;
    std::vector<Handler> handlers;
    { std::lock_guard<std::mutex> lock(state.mutex);
      title = state.title; header = state.header; handlers = state.handlers;
    }
    char errorText[256]; SErrGetErrorStr(error, errorText, sizeof(errorText));
    try { SLogFlushAll(); WriteReport(title.data(), header, error, errorText, file, line, description); }
    catch (...) { std::fputs("Cannot write native Storm error report\n", stderr); }
    for (const auto handler : handlers) {
        try { if (!handler(error, errorText, file, line, description ? description : "")) break; }
        catch (...) { std::fputs("Client error handler failed\n", stderr); }
    }
}
}

// Error callbacks and text reports are native. Modal error UI, minidumps and
// interception of OS faults are separate from this ordinary error path.
int32_t STORMAPI SErrIsDisplayingError() { return State().displaying.load() != 0; }
void SErrInitialize() { g_opt.serrleaksilentwarning = 1; (void)State(); }
int32_t STORMAPI SErrDestroy() {
    auto& state = State(); std::lock_guard<std::recursive_mutex> reportingLock(state.reportMutex);
    std::lock_guard<std::mutex> lock(state.mutex);
    state.header = nullptr; state.handlers.clear(); state.title.fill(0); state.lastPath.clear(); prepared = {};
    return 1;
}
void STORMAPI SErrPrepareAppFatal(const char* file, int32_t line) {
    std::snprintf(prepared.file.data(), prepared.file.size(), "%s", file ? file : ""); prepared.line = line;
}
void STORMAPI SErrSetLogTitleString(const char* title) {
    auto& state = State(); std::lock_guard<std::mutex> lock(state.mutex);
    std::snprintf(state.title.data(), state.title.size(), "%s", title ? title : "");
}
void STORMAPI SErrSetLogCallback(Header callback) {
    auto& state = State(); std::lock_guard<std::mutex> lock(state.mutex); state.header = callback;
}
void STORMAPI SErrRegisterHandler(Handler handler) {
    if (!handler) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return; }
    auto& state = State(); std::lock_guard<std::mutex> lock(state.mutex); state.handlers.push_back(handler);
}
int32_t STORMAPI SErrGetLogLastPath(char* buffer, uint32_t size) {
    if (!buffer || !size) { SErrSetLastError(STORM_ERROR_INVALID_PARAMETER); return 0; }
    auto& state = State(); std::lock_guard<std::mutex> lock(state.mutex);
    if (state.lastPath.empty()) return 0;
    std::snprintf(buffer, size, "%s", state.lastPath.c_str());
    return 1;
}

uint32_t STORMAPI SErrGetLastError() { return s_lastError; }
void STORMAPI SErrSetLastError(uint32_t error) { s_lastError = error; }

int32_t STORMAPI SErrGetErrorStr(uint32_t error, char* buffer, uint32_t size) {
    if (!buffer || !size) { SErrSetLastError(87); return 0; }
    // Storm retains Windows error numbers. Do not interpret them as POSIX errno.
    const char* message = nullptr;
    switch (error) {
        case 0: message = "Success"; break;
        case 2: message = "File not found"; break;
        case 5: message = "Access denied"; break;
        case 38: message = "End of file"; break;
        case 87: message = "Invalid parameter"; break;
        case 108: message = "Invalid or corrupt archive"; break;
        case 120: message = "Function not implemented"; break;
    }
    if (message) { std::snprintf(buffer, size, "%s", message); }
    else { std::snprintf(buffer, size, "Storm error 0x%08x", error); }
    return 1;
}

void SErrDisplayAppFatal(const char* format, ...) {
    va_list args;
    va_start(args, format);
    SErrDisplayAppFatalCustomV(STORM_ERROR_FATAL_CONDITION, format, args);
    va_end(args);
    StormExitProcess(1);
}

void SErrDisplayAppFatalMessage(const char* message, const char* file, int32_t line) {
    SErrDisplayError(STORM_ERROR_FATAL_CONDITION, file, line, message, 0, 1, 0);
    StormExitProcess(1);
}

void SErrDisplayAppFatalCustomV(uint32_t error, const char* format, va_list args) {
    char message[2048]; std::vsnprintf(message, sizeof(message), format ? format : "", args);
    const Origin origin = prepared; prepared = {};
    SErrDisplayError(error, origin.file[0] ? origin.file.data() : nullptr, origin.line, message, 0, 1, 0);
    StormExitProcess(1);
}

void SErrDisplayAppFatalCustom(uint32_t error, const char* format, ...) {
    va_list args;
    va_start(args, format);
    SErrDisplayAppFatalCustomV(error, format, args);
    va_end(args);
}

int32_t STORMAPI SErrDisplayError(uint32_t error, const char* file, int32_t line,
    const char* description, int32_t recoverable, uint32_t exitCode, uint32_t) {
    SErrSetLastError(error);
    fprintf(stderr, "Storm error 0x%08x at %s:%d: %s\n", error,
        file ? file : "?", line, description ? description : "");
    if (reporting) {
        if (!recoverable) StormExitProcess(static_cast<int32_t>(exitCode));
        return 0;
    }
    {
        auto& state = State(); std::lock_guard<std::recursive_mutex> lock(state.reportMutex);
        struct Active {
            Diagnostics& state;
            Active(Diagnostics& value) : state(value) { reporting = true; ++state.displaying; }
            ~Active() { --state.displaying; reporting = false; }
        } active(state);
        try { Report(error, file, line, description); }
        catch (...) { std::fputs("Native Storm error reporting failed\n", stderr); }
    }
    SErrSetLastError(error);
    if (!recoverable) {
        StormExitProcess(static_cast<int32_t>(exitCode));
    }
    return 1;
}

[[noreturn]] void StormExitProcess(int32_t code) { std::exit(code); }

int32_t SErrDisplayErrorFmt(uint32_t error, const char* file, int32_t line, int32_t recoverable,
    uint32_t exitCode, const char* format, ...) {
    char description[2048];
    va_list args; va_start(args, format);
    std::vsnprintf(description, sizeof(description), format, args); va_end(args);
    return SErrDisplayError(error, file, line, description, recoverable, exitCode, 0);
}

#ifndef NDEBUG
void DebugLog(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void DebugLog(int32_t, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

void DebugValidate(const char* expression) {
    DebugLog("Validation failed: %s\n", expression);
}
#endif
#endif
