#ifndef ENGINE_SOURCE_GX_CGXPOOL_H
#define ENGINE_SOURCE_GX_CGXPOOL_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/Types.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

struct CGxBuf;
struct CGxPool;

struct CGxPool : public TSLinkedNode<CGxPool> {
    /* 0008 */ EGxPoolTarget m_target;
    /* 000C */ EGxPoolUsage m_usage;
    /* 0010 */ int32_t m_size;
    /* 0014 */ void* m_apiSpecific;
    /* 0018 */ void* m_mem;
    /* 001C */ int32_t m_offset;
    /* 0020 */ TSList<CGxBuf> m_bufList;
    /* 002C */ EGxPoolHintBits m_hint;
    /* 0030 */ const char* m_name;

    CGxPool(EGxPoolTarget target, EGxPoolUsage usage, int32_t size, EGxPoolHintBits hint, const char* name);
    ~CGxPool();
    void Discard();
    void Invalidate();
    void UnlinkBuf(CGxBuf* buf);
};

#endif
