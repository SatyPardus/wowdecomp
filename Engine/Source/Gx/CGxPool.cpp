#include <cstdint>
#include "Engine/Source/Gx/CGxPool.h"

// INLINED
CGxPool::CGxPool(EGxPoolTarget target, EGxPoolUsage usage, int32_t size, EGxPoolHintBits hint, const char* name) : m_target(target), m_usage(usage), m_size(size), m_apiSpecific(nullptr), m_offset(0), m_hint(hint), m_name(name) {
}

// VERIFY: 434bd8af manual
// OFFSET: 0x00687740
CGxPool::~CGxPool() {
}

// VERIFY: 3a3418b6 manual
// OFFSET: 0x00688230
void CGxPool::Invalidate() {
    for (CGxBuf* buf = this->m_bufList.Head(); buf; buf = this->m_bufList.Next(buf)) {
        buf->m_hasData = 0;
    }
}

// VERIFY: 9189f656 manual
// OFFSET: 0x00688260
void CGxPool::Discard() {
    for (CGxBuf* buf = this->m_bufList.Head(); buf; buf = this->m_bufList.Next(buf)) {
        buf->m_hasData = 0;
    }

    this->m_offset = 0;
}

// VERIFY: 56739d8d auto
// OFFSET: 0x00685C00
void CGxPool::UnlinkBuf(CGxBuf* buf) {
    this->m_bufList.UnlinkNode(buf);
    buf->m_pool = nullptr;
}
