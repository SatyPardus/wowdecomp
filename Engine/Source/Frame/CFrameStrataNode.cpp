#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CFrameStrataNode.h"

// VERIFY: 7f1b23bd manual
// OFFSET: 0x00495BD0
CFrameStrataNode::CFrameStrataNode() {
    this->m_pendingFrame = nullptr;
    this->m_batchDirty = 0;
}

// VERIFY: 8460ba54 auto
// OFFSET: 0x00493E90
int32_t CFrameStrataNode::RemoveFrame(CSimpleFrame* frame) {
    if (!frame->m_strataLink.IsLinked()) {
        return 0;
    }

    if (frame == this->m_pendingFrame) {
        this->m_pendingFrame = frame->m_strataLink.Next();
    }

    frame->m_strataLink.Unlink();

    if (!(frame->m_frameFlags & 0x2000)) {
        this->m_batchDirty = -1;
    }

    return this->m_batchDirty != 0;
}

// VERIFY: b23367ca manual
// OFFSET: 0x00494A10
void CFrameStrataNode::OnLayerUpdate(float elapsedSec) {
    for (CSimpleFrame* frame = this->m_frames.Head(); frame; frame = this->m_pendingFrame) {
        this->m_pendingFrame = frame->m_strataLink.Next();
        frame->OnLayerUpdate(elapsedSec);
    }

    this->m_frames.Combine(&this->m_pendingFrames, 2, nullptr);
}

// VERIFY: 2564cd95 auto
// OFFSET: 0x00494AF0
int32_t CFrameStrataNode::BuildBatches() {
    int32_t batchDirty = this->m_batchDirty;
    this->m_batchDirty = 0;

    if (!batchDirty) {
        return 0;
    }

    for (uint32_t layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; layer++) {
        CRenderBatch* batch = &this->m_batches[layer];

        if (this->m_renderList.IsLinked(batch)) {
            this->m_renderList.UnlinkNode(batch);
        }

        if (batchDirty & (1 << layer)) {
            batch->Clear();

            for (CSimpleFrame* frame = this->m_frames.Head(); frame; frame = this->m_frames.Next(frame)) {
                if (!(frame->m_frameFlags & 0x2000)) {
                    frame->OnFrameRender(batch, layer);
                }
            }

            batch->Finish();
        }

        if (batch->m_count) {
            this->m_renderList.LinkToTail(batch);
        }
    }

    if (this->m_batchDirty & 0x20) {
        this->m_batchDirty |= 0x1F;
    }

    return 0;
}

// VERIFY: e04671c7 manual
// OFFSET: 0x00495C50
CFrameStrataNode::~CFrameStrataNode() {
    this->m_frames.UnlinkAll();
    this->m_renderList.UnlinkAll();
}
