#if defined(__APPLE__)
#include "WoW/Source/Ui/KnowledgeBase.h"
#include "Engine/Source/Base/DynamicString.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"

namespace {
struct Lock {
    SCritSect& lock;
    explicit Lock(SCritSect& value) : lock(value) { lock.Enter(); }
    ~Lock() { lock.Leave(); }
};
void FreeData(CDynamicString*& data) {
    if (data) { data->~CDynamicString(); SMemFree(data, __FILE__, __LINE__, 0); data = nullptr; }
}
void Unlink(CKBPage* page) {
    auto** link = &CKBPage::s_pLoadingQueue_Head;
    while (*link && *link != page) link = &(*link)->m_loadingNext;
    if (*link) *link = page->m_loadingNext;
    page->m_loadingNext = nullptr; page->m_inQueue = 0;
}
}

void CKBPage::_ResetNativeDownload() {
    Lock lock(m_lock);
    FreeData(m_data); m_nativeResult = 0; m_nativeFinished = false;
}

bool CKBPage::_StartNativeDownload(const char* url) {
    if (m_state == 1 || m_inQueue) return false;
    // Publish the receiving state before NSURLSession may call back.
    _BeginLoading();
    if (OsURLDownload(url, LoadingCallback, this)) return true;
    m_state = 3; Unlink(this); return false;
}

void CKBPage::_CancelNativeDownload() {
    if (!m_inQueue) return;
    OsURLDownloadCancel(LoadingCallback, this);
    Unlink(this);
}

bool CKBPage::NativeLoadingCallback(void* argument, const char* bytes, uint32_t size, bool done, HTTPError::ErrorCode error) {
    auto* page = static_cast<CKBPage*>(argument);
    if (!page) return false;
    Lock lock(page->m_lock);
    if (page->m_nativeFinished) return false;
    if (!page->m_nativeResult && error)
        page->m_nativeResult = error == HTTPError::Timeout ? 5 : error == HTTPError::NotFound ? 6 : 4;
    if (!page->m_nativeResult && size) {
        constexpr uint32_t budget = 8 * 1024 * 1024;
        const uint32_t used = page->m_data ? uint32_t(page->m_data->m_length) : 0;
        if (!bytes || size > budget - used) page->m_nativeResult = 7;
        else {
            if (!page->m_data) page->m_data = new (SMemAlloc(sizeof(CDynamicString), __FILE__, __LINE__, 0)) CDynamicString();
            page->m_data->Append(bytes, size);
        }
    }
    // A rejected chunk still receives a final callback. Wait for it before
    // publishing completion, so an event handler can safely delete the page.
    if (done) {
        if (!page->m_nativeResult) page->m_nativeResult = 2;
        page->m_nativeFinished = true;
    }
    return !page->m_nativeResult || page->m_nativeResult == 2;
}

bool CKBPage::_FinishNativeDownload() {
    Lock lock(m_lock);
    if (!m_nativeFinished) return false;
    uint32_t result = m_nativeResult;
    if (result == 2) {
        if (m_tree) XMLTree_Free(m_tree);
        // XMLTree's shared parser and free-node pool are also used by FrameXML.
        // Only the game thread parses and publishes downloaded page trees.
        m_tree = XMLTree_Load(m_data ? m_data->m_buffer : "", m_data ? m_data->m_length : 0);
        if (m_tree) _ComputeCachedData();
        else result = 8;
    }
    FreeData(m_data);
    m_state = result;
    return true;
}

void CKBPage::UpdateNativeLoadingQueue() {
    for (;;) {
        CKBPage* completed = nullptr;
        for (auto* page = s_pLoadingQueue_Head; page; page = page->m_loadingNext) {
            if (page->_FinishNativeDownload()) { completed = page; break; }
        }
        if (!completed) return;
        Unlink(completed);
        // Detach before invoking Lua/client callbacks. They can destroy, reload
        // or remove another page; restart traversal after each notification.
        if (completed->m_state == 2) completed->_SendSuccessEvent();
        else completed->_SendFailureEvent();
    }
}
#endif
