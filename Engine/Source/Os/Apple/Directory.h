#pragma once
#include <filesystem>
#include <system_error>

// Opening an external volume can be interrupted while macOS resolves its
// access request. Retry only the initial open: no entries have been delivered
// yet. Traversal failures and permanent open errors must remain visible.
template <class Iterator = std::filesystem::directory_iterator>
Iterator OsOpenDirectory(const std::filesystem::path& path) {
    constexpr unsigned maxAttempts = 4;
    for (unsigned attempt = 0; ; ++attempt) {
        std::error_code error;
        Iterator iterator(path, error);
        if (!error) return iterator;
        if (error != std::errc::interrupted || attempt + 1 == maxAttempts)
            throw std::filesystem::filesystem_error("Open directory", path, error);
    }
}
