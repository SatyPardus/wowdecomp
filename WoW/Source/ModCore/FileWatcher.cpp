#include "FileWatcher.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iostream>

namespace ModCore
{
    FileWatcher& FileWatcher::Get()
    {
        static FileWatcher instance;
        return instance;
    }

    FileWatcher::~FileWatcher()
    {
        Stop();
    }

    void FileWatcher::Start(const std::filesystem::path& watchDir)
    {
        if (m_running)
            return;

        m_watchDir = watchDir;
        if (!std::filesystem::exists(m_watchDir))
            return;

        m_running = true;
        m_thread = std::thread(&FileWatcher::WatchLoop, this);
    }

    void FileWatcher::Stop()
    {
        if (!m_running)
            return;

        m_running = false;
        if (m_hDir && m_hDir != INVALID_HANDLE_VALUE)
        {
            CancelIoEx(static_cast<HANDLE>(m_hDir), nullptr);
            CloseHandle(static_cast<HANDLE>(m_hDir));
            m_hDir = nullptr;
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    std::vector<std::filesystem::path> FileWatcher::PopModifiedFiles()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<std::filesystem::path> result = std::move(m_modifiedFiles);
        m_modifiedFiles.clear();
        return result;
    }

    void FileWatcher::WatchLoop()
    {
        HANDLE hDir = CreateFileW(
            m_watchDir.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
        );

        if (hDir == INVALID_HANDLE_VALUE)
        {
            m_running = false;
            return;
        }
        m_hDir = hDir;

        BYTE buffer[4096];
        DWORD bytesReturned = 0;

        while (m_running)
        {
            if (ReadDirectoryChangesW(
                    hDir,
                    buffer,
                    sizeof(buffer),
                    TRUE, // Recursive watch
                    FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME,
                    &bytesReturned,
                    nullptr,
                    nullptr))
            {
                if (bytesReturned == 0) continue;

                BYTE* cursor = buffer;
                while (cursor)
                {
                    FILE_NOTIFY_INFORMATION* fni = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(cursor);
                    std::wstring fileName(fni->FileName, fni->FileNameLength / sizeof(wchar_t));

                    // Filter only .lua files
                    if (fileName.length() >= 4 && fileName.substr(fileName.length() - 4) == L".lua")
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_modifiedFiles.push_back(m_watchDir / fileName);
                    }

                    if (fni->NextEntryOffset == 0)
                        break;
                    cursor += fni->NextEntryOffset;
                }
            }
            else
            {
                break;
            }
        }

        if (m_hDir == hDir)
        {
            CloseHandle(hDir);
            m_hDir = nullptr;
        }
    }
}
