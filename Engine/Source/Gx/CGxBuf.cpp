#include <cstdint>
#include "Engine/Source/Gx/CGxBuf.h"

// INLINED
CGxBuf::CGxBuf(CGxPool* pool, uint32_t itemSize, uint32_t itemCount, uint32_t index) : m_pool(pool), m_itemSize(itemSize), m_itemCount(itemCount), m_size(itemSize * itemCount), m_index(index), m_hasData(0), m_valid(1), m_dirty(0), unk_001F(0) {
}
