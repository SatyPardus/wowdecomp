#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/SOption.h"
#include <TargetConditionals.h>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#if !TARGET_OS_IPHONE
#include <mach-o/dyld.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
extern char** environ;
#endif

namespace {
void Require(bool condition, const char* error) { if (!condition) throw std::runtime_error(error); }
std::atomic<unsigned> headers, firstCalls, secondCalls, unexpectedCalls;
std::atomic<bool> valid;
bool nested, expectPath;
void Observe(bool condition) { if (!condition) valid = false; }
int32_t STORMAPI Header(char* buffer, uint32_t size) {
    Observe(SErrIsDisplayingError() != 0);
    ++headers;
    std::snprintf(buffer, size, "%s", "client header: 100% %s %n native");
    SErrSetLogTitleString("next report title");
    return 1;
}
int32_t STORMAPI First(uint32_t error, const char*, const char*, int32_t, const char*) {
    ++firstCalls;
    char path[4096]{};
    const bool found = SErrGetLogLastPath(path, sizeof(path)) != 0;
    Observe(SErrIsDisplayingError() && found == expectPath);
    if (found) Observe(std::filesystem::is_regular_file(path) && std::filesystem::file_size(path) != 0);
    if (nested) Observe(SErrDisplayError(error, "nested.cpp", 9, "recursive handler", 1, 1, 0) == 0);
    return 1;
}
int32_t STORMAPI Second(uint32_t, const char*, const char*, int32_t, const char*) { ++secondCalls; return 0; }
int32_t STORMAPI Unexpected(uint32_t, const char*, const char*, int32_t, const char*) { ++unexpectedCalls; return 1; }
std::string Read(const std::filesystem::path& path) {
    std::ifstream input(path); Require(bool(input), "Cannot open native error report");
    return {std::istreambuf_iterator<char>(input), {}};
}
}

int RunDiagnosticsProbeChild(int argc, char** argv) {
#if !TARGET_OS_IPHONE
    if (argc != 4 || std::strcmp(argv[1], "--apple-error-child")) return -1;
    if (setenv("WOW_LOG_ROOT", argv[2], 1)) return 2;
    SErrInitialize(); SErrSetLogTitleString("native fatal child");
    if (!std::strcmp(argv[3], "direct")) {
        SErrDisplayError(87, "direct.cpp", 91, "direct fatal 100% %n", 0, 17, 0);
    } else {
        char file[] = "prepared.cpp"; SErrPrepareAppFatal(file, 73); std::memset(file, 'x', sizeof(file) - 1);
        SErrDisplayAppFatalCustom(0x12345678, "%s %d", "prepared fatal 100% %s", 12340);
    }
    return 3;
#else
    return -1;
#endif
}

void CheckNativeDiagnostics(const std::string& scratchDirectory) {
    std::string pattern = scratchDirectory + "/wow-errors-XXXXXX";
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create native error-report directory");
    const char* configured = std::getenv("WOW_LOG_ROOT");
    struct Cleanup {
        std::string directory, root;
        bool hadRoot;
        int32_t suppress = g_opt.serrsuppresslogs, silent = g_opt.serrleaksilentwarning;
        ~Cleanup() {
            SErrDestroy(); g_opt.serrsuppresslogs = suppress; g_opt.serrleaksilentwarning = silent;
            if (hadRoot) setenv("WOW_LOG_ROOT", root.c_str(), 1); else unsetenv("WOW_LOG_ROOT");
            std::error_code error; std::filesystem::remove_all(directory, error);
        }
    } cleanup{temporary.data(), configured ? configured : "", configured != nullptr};
    Require(!setenv("WOW_LOG_ROOT", temporary.data(), 1), "Cannot select writable error-report root");
    g_opt.serrsuppresslogs = 0;
    for (unsigned cycle = 0; cycle != 2; ++cycle) {
        headers = firstCalls = secondCalls = unexpectedCalls = 0; valid = true; nested = true; expectPath = true;
        SErrInitialize(); SErrSetLogTitleString("native title 100% %n"); SErrSetLogCallback(Header);
        SErrRegisterHandler(First); SErrRegisterHandler(Second); SErrRegisterHandler(Unexpected);
        Require(SErrDisplayErrorFmt(87, "source.cpp", 42, 1, 1, "%s %d", "description %s %n", 12340) == 1 &&
            SErrGetLastError() == 87 && !SErrIsDisplayingError(), "Recoverable native error changed state/return convention");
        char path[4096]{}; Require(SErrGetLogLastPath(path, sizeof(path)), "Native report path was not published");
        const std::string content = Read(path);
        Require(content.find("native title 100% %n") != std::string::npos && content.find("source.cpp:42") != std::string::npos &&
            content.find("description %s %n 12340") != std::string::npos && content.find("client header: 100% %s %n native") != std::string::npos,
            "Native error log lost source/header text or reinterpreted a format string");
        struct stat info{}; Require(!stat(path, &info) && (info.st_mode & 0777) == 0600, "Native error report permissions changed");
        char shortPath[4]{};
        Require(SErrGetLogLastPath(shortPath, sizeof(shortPath)) && !shortPath[3] && !std::memcmp(shortPath, path, 3) &&
            !SErrGetLogLastPath(nullptr, 1) && !SErrGetLogLastPath(path, 0), "Native last-error path boundary changed");
        nested = false;
        std::vector<std::thread> workers;
        for (unsigned i = 0; i != 4; ++i) workers.emplace_back([i] {
            SErrDisplayError(100 + i, "worker.cpp", int32_t(i), "worker report", 1, 1, 0);
            Observe(SErrGetLastError() == 100 + i);
        });
        for (auto& worker : workers) worker.join();
        Require(valid && headers == 5 && firstCalls == 5 && secondCalls == 5 && !unexpectedCalls,
            "Native error callback order, recursion, concurrency or TLS changed");
        g_opt.serrsuppresslogs = 1; expectPath = false;
        Require(SErrDisplayError(5, nullptr, 0, "suppressed report", 1, 1, 0) && !SErrGetLogLastPath(path, sizeof(path)) &&
            headers == 5 && firstCalls == 6 && secondCalls == 6, "Suppressed native report retained stale path or skipped handlers");
        g_opt.serrsuppresslogs = 0;
        SErrDestroy();
        Require(!SErrGetLogLastPath(path, sizeof(path)) && !SErrIsDisplayingError(), "Native diagnostic shutdown retained state");
    }
#if !TARGET_OS_IPHONE
    uint32_t size = 0; _NSGetExecutablePath(nullptr, &size); std::vector<char> executable(size);
    Require(!_NSGetExecutablePath(executable.data(), &size), "Cannot locate diagnostic child executable");
    for (const char* mode : {"direct", "prepared"}) {
        const std::string directory = std::string(temporary.data()) + '/' + mode;
        char* argv[] = {executable.data(), const_cast<char*>("--apple-error-child"), const_cast<char*>(directory.c_str()), const_cast<char*>(mode), nullptr};
        pid_t pid;
        Require(!posix_spawn(&pid, executable.data(), nullptr, nullptr, argv, environ), "Cannot spawn native fatal-error child");
        int status = 0;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        for (;;) {
            const auto result = waitpid(pid, &status, WNOHANG);
            if (result == pid) break;
            if (result < 0 && errno != EINTR) throw std::runtime_error("Cannot reap native fatal-error child");
            if (std::chrono::steady_clock::now() >= deadline) {
                kill(pid, SIGKILL); while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
                throw std::runtime_error("Native fatal-error child did not exit");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        const bool direct = !std::strcmp(mode, "direct");
        Require(WIFEXITED(status) && WEXITSTATUS(status) == (direct ? 17 : 1), "Native fatal-error exit code changed");
        unsigned reports = 0;
        for (const auto& entry : std::filesystem::directory_iterator(directory + "/Errors")) {
            const auto content = Read(entry.path()); ++reports;
            Require(content.find(direct ? "direct.cpp:91" : "prepared.cpp:73") != std::string::npos &&
                content.find(direct ? "direct fatal 100% %n" : "prepared fatal 100% %s 12340") != std::string::npos,
                "Native fatal report lost prepared source lifetime or literal message");
        }
        Require(reports == 1, "Native fatal child produced duplicate/missing reports");
    }
#endif
    std::fputs("Native diagnostics: writable flushed reports, literal text/source/header, callbacks, recursion/TLS/concurrency, suppression and recreation passed", stderr);
#if !TARGET_OS_IPHONE
    std::fputs("; actual fatal children preserved prepared source and exit codes", stderr);
#endif
    std::fputc('\n', stderr);
}
