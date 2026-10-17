#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <thread>
#include <atomic>
#include <mutex>

namespace ModCore
{
    class FileWatcher
    {
    public:
        static FileWatcher& Get();

        void Start(const std::filesystem::path& watchDir);
        void Stop();

        // Fetches and clears modified files since last call (thread-safe)
        std::vector<std::filesystem::path> PopModifiedFiles();

    private:
        FileWatcher() = default;
        ~FileWatcher();

        void WatchLoop();

        std::filesystem::path m_watchDir;
        std::atomic<bool> m_running{false};
        std::thread m_thread;
        std::mutex m_mutex;
        std::vector<std::filesystem::path> m_modifiedFiles;
        void* m_hDir = nullptr;
    };
}
