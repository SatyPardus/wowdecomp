#include "HttpDownload.h"
#include <cstdint>
#if defined(__APPLE__)
#include <algorithm>
#include <climits>
#endif
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"

// INLINED
DownloadFileCallback::DownloadFileCallback() {
    this->m_stop = false;
}

// VERIFY: 379d687c manual
// OFFSET: 0x008A1330
void DownloadFileCallback::Completed(Blizzard::URL::DownloadCallback::Status status) {
    if (this->m_state) {
#if defined(__APPLE__)
        std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
        if (m_state->m_nativeStatus != DOWNLOAD_FAILED) m_state->m_nativeStatus = status;
#endif
        this->m_state->m_done = true;
    }

#if !defined(__APPLE__)
    delete this;
#endif
}

// VERIFY: 18354f27 manual
// OFFSET: 0x008A1350
void DownloadFileCallback::GetContentLength(int64_t length) {
    if (this->m_state) {
#if defined(__APPLE__)
        std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
        // This original in-memory consumer has signed 32-bit storage/counts.
        // Do not narrow a larger response or allocate its advertised length.
        if (length > INT32_MAX) m_state->m_nativeStatus = DOWNLOAD_FAILED;
#endif
        this->m_state->m_size = length;
    }
}

// VERIFY: cc9642a9 manual
// OFFSET: 0x008A1370
bool HttpDownload::Done() {
#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
#endif
    return !this->m_requested || this->m_state->m_done;
}

// VERIFY: 5c3f7d88 manual
// OFFSET: 0x008A1390
bool HttpDownload::Cancel() {
#if defined(__APPLE__)
    if (Done()) return true;
    // Cancellation goes through the transport, so it also wakes idle requests.
    // The owning HttpDownload keeps the callback alive until teardown drains it.
    Blizzard::URL::CancelNativeDownloads(m_callback, false);
    return false;
#else
    if (!this->m_requested || this->m_state->m_done) {
        return true;
    }

    this->m_callback->m_stop = true;
    return false;
#endif
}

// VERIFY: 7e0e4c37 manual
// OFFSET: 0x008A13B0
uint8_t* HttpDownload::GetBytes() {
#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
    if (!m_state->m_done || m_state->m_nativeStatus != Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS) return nullptr;
#endif
    return this->m_state->m_data;
}

// VERIFY: c62441e5 manual
// OFFSET: 0x008A13C0
int64_t HttpDownload::Size() {
#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
    if (m_state->m_done && m_state->m_nativeStatus == Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS) {
#else
    if (this->m_state->m_done) {
#endif
        this->m_state->m_size = this->m_state->m_received;
    }

    if (this->m_state->m_size < this->m_state->m_received) {
        this->m_state->m_size = int64_t(2) * this->m_state->m_received;
    }

    return this->m_state->m_size;
}

// VERIFY: 66e5722c manual
// OFFSET: 0x008A1410
int64_t HttpDownload::Progress() {
#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
#endif
    return this->m_state->m_received;
}

// VERIFY: 5bc19210 auto
// OFFSET: 0x008A1420
bool DownloadFileCallback::WantData() {
#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
    return m_state->m_nativeStatus != DOWNLOAD_FAILED;
#else
    return true;
#endif
}

// VERIFY: 78dc384f auto
// OFFSET: 0x008A1430
bool DownloadFileCallback::Stop() {
    return this->m_stop;
}

// VERIFY: 3075fdd3 auto
// OFFSET: 0x00427A90
bool DownloadFileCallback::ShouldBlock() {
    return false;
}

// VERIFY: 7ed85eec manual
// OFFSET: 0x008A1440
bool DownloadFileCallback::ReceivedData(const void* data, int32_t size) {
    if (!this->m_state) {
        return true;
    }

#if defined(__APPLE__)
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
    const int64_t required = int64_t(m_state->m_received) + size;
    if (!data || size <= 0 || required > INT32_MAX || m_state->m_nativeStatus == DOWNLOAD_FAILED) {
        m_state->m_nativeStatus = DOWNLOAD_FAILED;
        return false;
    }
    if (required > m_state->m_capacity) {
        const int32_t capacity = int32_t(std::max(required, std::min<int64_t>(INT32_MAX,
            std::max<int64_t>(16384, int64_t(m_state->m_capacity) * 2))));
        auto* grown = static_cast<uint8_t*>(Blizzard::Memory::Allocate(capacity));
        if (!grown) { m_state->m_nativeStatus = DOWNLOAD_FAILED; return false; }
        if (m_state->m_received) Blizzard::String::MemCopy(grown, m_state->m_data, m_state->m_received);
        Blizzard::Memory::Free(m_state->m_data);
        m_state->m_data = grown; m_state->m_capacity = capacity;
    }
    Blizzard::String::MemCopy(m_state->m_data + m_state->m_received, data, size);
    m_state->m_received += size;
#else
    this->m_state->m_receiving = true;

    if (!this->m_state->m_data) {
        this->m_state->m_capacity = static_cast<int32_t>(this->m_state->m_size);

        if (this->m_state->m_capacity < 0) {
            this->m_state->m_capacity = size;
        }

        this->m_state->m_data = static_cast<uint8_t*>(Blizzard::Memory::Allocate(this->m_state->m_capacity));
    }

    if (this->m_state->m_received + size > this->m_state->m_capacity) {
        this->m_state->m_capacity *= 2;

        if (this->m_state->m_capacity < this->m_state->m_received + size) {
            this->m_state->m_capacity = this->m_state->m_received + size;
        }

        uint8_t* grown = static_cast<uint8_t*>(Blizzard::Memory::Allocate(this->m_state->m_capacity));
        Blizzard::String::MemCopy(grown, this->m_state->m_data, this->m_state->m_received);
        Blizzard::Memory::Free(this->m_state->m_data);
        this->m_state->m_data = grown;
    }

    Blizzard::String::MemCopy(this->m_state->m_data + this->m_state->m_received, data, size);
    this->m_state->m_received += size;
    this->m_state->m_receiving = false;
#endif

    return true;
}

// VERIFY: 5caeaac4 manual
// OFFSET: 0x008A1530
const char* HttpDownload::TargetName() {
    return this->m_state->m_targetName.c_str();
}

// VERIFY: 46de26a1 manual
// OFFSET: 0x008A1550
HttpDownload::~HttpDownload() {
#if defined(__APPLE__)
    if (m_requested) Blizzard::URL::CancelNativeDownloads(m_callback);
    delete m_callback;
#else
    if (!this->m_state->m_done) {
        while (this->m_state->m_receiving) {
            Blizzard::Thread::Sleep(0);
        }

        this->m_callback->m_state = nullptr;
    }
#endif

    Blizzard::Memory::Free(this->m_state->m_data);
    delete this->m_state;
}

// VERIFY: 0a70c43e manual
// OFFSET: 0x008A15E0
HttpDownload::HttpDownload(const char* url, const char* targetName) {
    this->m_state = new HttpDownloadState;
    this->m_state->m_receiving = false;
    this->m_state->m_done = false;
    this->m_state->m_size = -1;
    this->m_state->m_data = nullptr;
    this->m_state->m_received = 0;
#if defined(__APPLE__)
    this->m_state->m_capacity = 0;
#endif

    this->m_callback = new DownloadFileCallback;
    this->m_callback->m_state = this->m_state;
#if defined(__APPLE__)
    this->m_state->m_url = url ? url : "";
#else
    this->m_state->m_url = url;
#endif

    if (targetName) {
        this->m_state->m_targetName = targetName;
    }

    this->m_requested = Blizzard::URL::DownloadURL(url, this->m_callback);
#if defined(__APPLE__)
    if (!m_requested) {
        m_state->m_done = true;
        m_state->m_nativeStatus = Blizzard::URL::DownloadCallback::DOWNLOAD_FAILED;
        delete m_callback; m_callback = nullptr;
    }
#endif
}

#if defined(__APPLE__)
bool HttpDownload::Succeeded() {
    std::lock_guard<std::mutex> lock(m_state->m_nativeMutex);
    return m_state->m_done && m_state->m_nativeStatus == Blizzard::URL::DownloadCallback::DOWNLOAD_SUCCESS;
}
#endif
