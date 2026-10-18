#include "Engine/Source/Storm/H/storm.h"
#include <TargetConditionals.h>
#include <cerrno>
#include <cstdio>
#include <stdexcept>
#include <string>
#if !TARGET_OS_IPHONE
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <fcntl.h>
#include <iomanip>
#include <mach-o/dyld.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#endif

namespace {
void Require(bool condition, const char* error) { if (!condition) throw std::runtime_error(error); }
#if !TARGET_OS_IPHONE
std::string Quote(const std::string& value) {
    std::string result = "\"";
    size_t slashes = 0;
    for (char c : value) {
        if (c == '\\') { ++slashes; continue; }
        result.append(slashes * (c == '"' ? 2 : 1), '\\'); slashes = 0;
        if (c == '"') result += '\\';
        result += c;
    }
    result.append(slashes * 2, '\\');
    return result + '"';
}
struct Callback {
    std::atomic<unsigned> calls{0};
    std::atomic<uint32_t> thread{0};
    static void Done(void* data) {
        auto& self = *static_cast<Callback*>(data);
        self.thread = SGetCurrentThreadId();
        self.calls.fetch_add(1);
    }
};
template <class T> void Wait(T predicate, const char* error) {
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!predicate()) {
        Require(std::chrono::steady_clock::now() < end, error);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}
#endif
}

// Used only by the Mac core executable when its process check starts a native
// copy of itself. No shell, external app, network or original client is run.
int RunProcessProbeChild(int argc, char** argv) {
#if !TARGET_OS_IPHONE
    if (argc < 4 || std::strcmp(argv[1], "--apple-process-child")) return -1;
    const auto pending = std::string(argv[2]) + ".pending";
    std::ofstream file(pending);
    if (!file) return 2;
    const int descriptor = std::stoi(argv[3]);
    file << getpid() << ' ' << (fcntl(descriptor, F_GETFD) >= 0) << ' ' << argc << '\n';
    for (int i = 0; i < argc; ++i) file << std::quoted(std::string(argv[i])) << '\n';
    file.close();
    return file && rename(pending.c_str(), argv[2]) == 0 ? 0 : 3;
#else
    return -1;
#endif
}

void CheckNativeProcesses(const std::string& scratchDirectory) {
#if TARGET_OS_IPHONE
    Require(!SCreateProcess("native-helper", nullptr, nullptr, nullptr) && errno == ENOTSUP,
        "iOS incorrectly reported helper-process support");
    std::fputs("Native process: UIKit helper launch explicitly unsupported\n", stderr);
#else
    uint32_t size = 0; _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    Require(_NSGetExecutablePath(buffer.data(), &size) == 0, "Cannot resolve native process-check executable");
    const auto executable = std::filesystem::canonical(buffer.data()).string();
    const auto pattern = (std::filesystem::path(scratchDirectory) / "wow-process-XXXXXX").string();
    std::vector<char> temporary(pattern.begin(), pattern.end()); temporary.push_back(0);
    Require(mkdtemp(temporary.data()), "Cannot create process-check directory");
    struct Cleanup {
        const char* directory; int descriptor = -1;
        ~Cleanup() { if (descriptor >= 0) close(descriptor); std::error_code error; std::filesystem::remove_all(directory, error); }
    } cleanup{temporary.data()};
    int file = open((std::string(temporary.data()) + "/not-inherited").c_str(), O_CREAT | O_RDWR, 0600);
    Require(file >= 0, "Cannot open process inheritance check");
    cleanup.descriptor = fcntl(file, F_DUPFD, 200); close(file);
    Require(cleanup.descriptor >= 200 && !(fcntl(cleanup.descriptor, F_GETFD) & FD_CLOEXEC), "Cannot create inheritable descriptor");
    const auto callbackThread = SGetCurrentThreadId();
    for (unsigned mode = 0; mode < 3; ++mode) {
        const std::string output = std::string(temporary.data()) + "/result " + std::to_string(mode);
        std::vector<std::string> expected{
            mode == 1 ? "custom argv zero" : executable, "--apple-process-child", output,
            std::to_string(cleanup.descriptor), u8"space 中文 value", "", "backslash\\", "escaped\\\"quote",
            "$(touch should-not-exist); * > file", "doubled\"quote"
        };
        std::string line;
        for (size_t i = 0; i + 1 < expected.size(); ++i) line += Quote(expected[i]) + ' ';
        line += "\"doubled\"\"quote\"";
        Callback callback;
        Require(SCreateProcess(mode == 0 ? nullptr : executable.c_str(), line.data(),
            mode == 2 ? nullptr : Callback::Done, &callback), "Cannot launch native child process");
        if (mode != 2) Wait([&] { return callback.calls.load() == 1; }, "Process completion callback did not arrive");
        else Wait([&] { return std::filesystem::exists(output) && std::filesystem::file_size(output) > 0; }, "Child without callback did not run");
        std::ifstream result(output);
        pid_t pid = -1; int inherited = -1; size_t count = 0;
        result >> pid >> inherited >> count;
        Require(result && pid > 0 && inherited == 0 && count == expected.size(), "Child inherited file handles or lost argv");
        for (const auto& value : expected) {
            std::string actual; result >> std::quoted(actual);
            Require(result && actual == value, "Native child command-line quoting changed an argument");
        }
        // Reaper must collect children even when callers request no callback.
        Wait([&] { return kill(pid, 0) < 0 && errno == ESRCH; }, "Native child was not reaped");
        int status;
        Require(waitpid(pid, &status, WNOHANG) == -1 && errno == ECHILD, "Native child left a zombie");
        Require(mode == 2 ? callback.calls == 0 : callback.calls == 1 && callback.thread != callbackThread,
            "Process completion callback thread/count changed");
    }
    Callback failed;
    Callback noArguments;
    Require(SCreateProcess("true", nullptr, Callback::Done, &noArguments), "Cannot launch PATH native helper without command line");
    Wait([&] { return noArguments.calls.load() == 1; }, "No-argument native helper did not complete");
    Require(!SCreateProcess("/wow-no-such-native-executable", nullptr, Callback::Done, &failed) && errno == ENOENT,
        "Missing native executable was reported as launched");
    const auto invalid = std::string(temporary.data()) + "/not-an-executable";
    { std::ofstream contents(invalid); contents << "MZ invalid executable"; }
    chmod(invalid.c_str(), 0700);
    Require(!SCreateProcess(invalid.c_str(), nullptr, Callback::Done, &failed) && errno == ENOEXEC,
        "Invalid executable was run through a shell fallback");
    std::string oversized(32768, 'x');
    Require(!SCreateProcess(nullptr, nullptr, Callback::Done, &failed) && errno == EINVAL &&
        !SCreateProcess("", nullptr, Callback::Done, &failed) && errno == EINVAL &&
        !SCreateProcess(executable.c_str(), oversized.data(), Callback::Done, &failed) && errno == EINVAL,
        "Invalid/oversized process request was accepted");
    Require(failed.calls == 0, "Failed process launch called completion");
    std::fputs("Native process: actual child argv/Unicode/quoting, no shell expansion, descriptor isolation, worker completion, reaping without callback and failure checks passed\n", stderr);
#endif
}
