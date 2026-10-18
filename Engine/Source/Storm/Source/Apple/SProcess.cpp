#if defined(__APPLE__)
#include "Engine/Source/Storm/H/storm.h"
#include <TargetConditionals.h>
#include <cerrno>
#if !TARGET_OS_IPHONE
#include "Engine/Source/Storm/Source/SThread.h"
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <crt_externs.h>

namespace {
// Keep the existing caller's command-line quoting. These are argv values;
// shell metacharacters, environment expansions and redirects are never run.
std::vector<std::string> Arguments(const char* commandLine) {
    std::vector<std::string> result;
    const char* p = commandLine;
    while (p && *p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        std::string argument;
        bool quoted = false;
        while (*p && (quoted || (*p != ' ' && *p != '\t'))) {
            size_t slashes = 0;
            while (*p == '\\') { ++slashes; ++p; }
            if (*p == '"') {
                argument.append(slashes / 2, '\\');
                if (slashes & 1) { argument += '"'; ++p; }
                else if (quoted && p[1] == '"') { argument += '"'; p += 2; }
                else { quoted = !quoted; ++p; }
            } else {
                argument.append(slashes, '\\');
                if (*p && (quoted || (*p != ' ' && *p != '\t'))) argument += *p++;
            }
        }
        result.push_back(std::move(argument));
    }
    return result;
}
struct Completion {
    std::mutex mutex;
    std::condition_variable changed;
    bool ready = false;
    pid_t process = -1;
    void (*callback)(void*);
    void* data;
};
uint32_t STORMAPI WaitForProcess(void* parameter) {
    std::unique_ptr<Completion> completion(static_cast<Completion*>(parameter));
    std::unique_lock<std::mutex> lock(completion->mutex);
    completion->changed.wait(lock, [&] { return completion->ready; });
    const pid_t process = completion->process;
    lock.unlock();
    if (process < 0) return 0;
    int status;
    pid_t result;
    do { result = waitpid(process, &status, 0); } while (result < 0 && errno == EINTR);
    if (result == process && completion->callback) completion->callback(completion->data);
    return 0;
}
struct SpawnAttributes {
    posix_spawnattr_t attributes;
    posix_spawn_file_actions_t actions;
    bool haveAttributes = false, haveActions = false;
    ~SpawnAttributes() {
        if (haveActions) posix_spawn_file_actions_destroy(&actions);
        if (haveAttributes) posix_spawnattr_destroy(&attributes);
    }
    int Initialize() {
        int error = posix_spawnattr_init(&attributes);
        if (error) return error;
        haveAttributes = true;
        error = posix_spawn_file_actions_init(&actions);
        if (error) return error;
        haveActions = true;
        error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_CLOEXEC_DEFAULT);
        // CreateProcess's non-inheritable handles must not leak archives or
        // sockets into helpers. Retain only existing standard descriptors.
        for (int fd = 0; !error && fd < 3; ++fd) {
            if (fcntl(fd, F_GETFD) >= 0) error = posix_spawn_file_actions_adddup2(&actions, fd, fd);
        }
        return error;
    }
};
}
#endif

int32_t STORMAPI SCreateProcess(const char* appName, char* commandLine,
    void (*callbackWhenProcessCompletes)(void*), void* callbackData) {
    // As with the native Storm synchronization layer, preserve POSIX errno;
    // do not publish it as an unrelated Windows/Storm error number.
    auto fail = [](int error) { errno = error; return 0; };
#if TARGET_OS_IPHONE
    // UIKit applications cannot start a helper process, including on Simulator.
    return fail(ENOTSUP);
#else
    if ((appName && (!*appName || strnlen(appName, 32768) == 32768)) ||
        (commandLine && strnlen(commandLine, 32768) == 32768)) return fail(EINVAL);
    try {
        auto arguments = Arguments(commandLine);
        if (arguments.empty()) {
            if (!appName) return fail(EINVAL);
            arguments.emplace_back(appName);
        }
        const char* executable = appName ? appName : arguments[0].c_str();
        if (!*executable) return fail(EINVAL);
        std::vector<std::string> candidates;
        if (std::strchr(executable, '/')) candidates.emplace_back(executable);
        else {
            const char* environment = std::getenv("PATH");
            const std::string search = environment ? environment : "/usr/bin:/bin";
            size_t begin = 0;
            do {
                const size_t end = search.find(':', begin);
                const auto directory = search.substr(begin, end == std::string::npos ? end : end - begin);
                candidates.push_back(directory.empty() ? executable : directory + '/' + executable);
                if (end == std::string::npos) break;
                begin = end + 1;
            } while (begin <= search.size());
        }
        std::vector<char*> argv;
        for (auto& argument : arguments) argv.push_back(argument.data());
        argv.push_back(nullptr);
        SpawnAttributes spawn;
        int error = spawn.Initialize();
        if (error) return fail(error);
        auto completion = std::make_unique<Completion>();
        completion->callback = callbackWhenProcessCompletes;
        completion->data = callbackData;
        SThread waiter;
        char name[] = "Process completion";
        // Create the reaper first, so thread creation failure cannot orphan a
        // child. Every successful spawn is reaped, even without a callback.
        if (!SThread::Create(WaitForProcess, completion.get(), waiter, name, 0)) return 0;
        auto* state = completion.release();
        std::lock_guard<std::mutex> lock(state->mutex);
        pid_t process;
        // Darwin posix_spawnp falls back to /bin/sh for ENOEXEC. Use native
        // spawn directly, including PATH candidates, so invalid/PE binaries
        // fail rather than being interpreted as command text.
        bool denied = false;
        for (const auto& candidate : candidates) {
            error = posix_spawn(&process, candidate.c_str(), &spawn.actions, &spawn.attributes, argv.data(), *_NSGetEnviron());
            if (error == EACCES) denied = true;
            else if (error != ENOENT && error != ENOTDIR) break;
        }
        if (denied && (error == ENOENT || error == ENOTDIR)) error = EACCES;
        if (!error) state->process = process;
        state->ready = true;
        state->changed.notify_one();
        // There is no Win32 GUI input-idle handshake on macOS. Success means
        // native exec succeeded; completion still runs after actual child exit.
        return error ? fail(error) : 1;
    } catch (const std::bad_alloc&) { return fail(ENOMEM); }
#endif
}
#endif
