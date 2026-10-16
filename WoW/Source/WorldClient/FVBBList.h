#ifndef WOW_SOURCE_WORLDCLIENT_FVBBLIST_H
#define WOW_SOURCE_WORLDCLIENT_FVBBLIST_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

struct CGxBuf;
struct CGxPool;

class FVBBList {
    public:
    struct Block {
        /* 0000 */ TSLink<Block> m_link;
        /* 0008 */ uint32_t m_lastUsedFrame;
        /* 000C */ CGxBuf* m_buf;
        /* 0010 */ Block** m_owner;
    };

    /* 0000 */ CGxPool* m_pool;
    /* 0004 */ uint32_t m_blockSize;
    /* 0008 */ TSExplicitList<Block, offsetof(Block, m_link)> m_blocks;
    /* 0014 */ TSExplicitList<Block, offsetof(Block, m_link)> m_usedBlocks;

    void Destroy();
    void Initialize(EGxPoolTarget target, EGxPoolUsage usage, uint32_t itemSize, uint32_t itemCount, uint32_t blockCount);
    void Release(Block* block);
    void AcquireBlock(Block** owner);
    Block* AllocBlock(Block** owner);
};

#endif
