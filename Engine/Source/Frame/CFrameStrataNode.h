#ifndef ENGINE_SOURCE_FRAME_CFRAMESTRATANODE_H
#define ENGINE_SOURCE_FRAME_CFRAMESTRATANODE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Storm/H/stpl.h"

class CFrameStrataNode {
    public:
    /* 0000 */ TSExplicitList<CSimpleFrame, offsetof(CSimpleFrame, m_strataLink)> m_pendingFrames;
    /* 000C */ TSExplicitList<CSimpleFrame, offsetof(CSimpleFrame, m_strataLink)> m_frames;
    /* 0018 */ CSimpleFrame* m_pendingFrame;
    /* 001C */ CRenderBatch m_batches [NUM_SIMPLEFRAME_DRAWLAYERS];
    /* 010C */ int32_t m_batchDirty;
    /* 0110 */ TSExplicitList<CRenderBatch, offsetof(CRenderBatch, m_renderLink)> m_renderList;

    CFrameStrataNode();
    ~CFrameStrataNode();
    int32_t BuildBatches();
    void OnLayerUpdate(float elapsedSec);
    int32_t RemoveFrame(CSimpleFrame* frame);
};

#endif
