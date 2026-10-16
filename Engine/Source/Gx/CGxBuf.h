#ifndef ENGINE_SOURCE_GX_CGXBUF_H
#define ENGINE_SOURCE_GX_CGXBUF_H

#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

struct CGxPool;

struct CGxBuf : public TSLinkedNode<CGxBuf> {
    /* 0008 */ CGxPool* m_pool;
    /* 000C */ uint32_t m_itemSize;
    /* 0010 */ uint32_t m_itemCount;
    /* 0014 */ uint32_t m_size;
    /* 0018 */ uint32_t m_index;
    /* 001C */ uint8_t m_hasData;
    /* 001D */ uint8_t m_valid;
    /* 001E */ uint8_t m_dirty;
    /* 001F */ uint8_t unk_001F;

    CGxBuf(CGxPool* pool, uint32_t itemSize, uint32_t itemCount, uint32_t index);
};

#endif
