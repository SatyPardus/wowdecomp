#include "Engine/Source/Frame/CSimpleTop.h"
#include <cmath>
#include <cstring>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/BLPFile/Types.h"
#include "Engine/Source/Event/CCharEvent.h"
#include "Engine/Source/Event/CFocusEvent.h"
#include "Engine/Source/Event/CImeEvent.h"
#include "Engine/Source/Event/CKeyEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Event/CSizeEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CFrameStrataNode.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Scrn/Scrn.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CRect.h"

// STATIC: 0x00B499A8
CSimpleTop* CSimpleTop::s_instance;

// STATIC: 0x00B499A4
uint32_t CSimpleTop::s_eventTime;

// VERIFY: 29e70871 auto
// OFFSET: 0x00495810
static void PaintScreen(void* param, const RECTF* rect, const RECTF* visible, float elapsedSec) {
    CSimpleTop* top = CSimpleTop::s_instance;
    top->OnLayerUpdate(elapsedSec);
    top->OnLayerRender();
}

// VERIFY: 3a1b6d83 auto
// OFFSET: 0x004883B0
CSimpleTop* CSimpleTop::GetInstance() {
    return CSimpleTop::s_instance;
}

// VERIFY: 4b605abd auto
// OFFSET: 0x004883C0
void CSimpleTop::GetCursorPosition(C2Vector& position) {
    NDCToDDC(this->m_mousePosition.m_x, this->m_mousePosition.m_y, &position.x, &position.y);
}

// VERIFY: 82aafc3e auto
// OFFSET: 0x00493A10
void CSimpleTop::StopMoveOrResizeFrame() {
    if (this->m_moveResizeFrame) {
        this->m_moveResizeFrame->UnflattenFrame(this);
    }

    this->m_moveResizeReason = MOVERESIZE_REASON_NONE;
    this->m_moveResizeFrame = nullptr;
}

// VERIFY: eccae11a auto
// OFFSET: 0x00493C10
void CSimpleTop::NotifyFrameLayerChanged(CSimpleFrame* frame, uint32_t drawLayer) {
    CFrameStrata* strata = this->m_strata[frame->m_strata];
    strata->m_levels[frame->m_level]->m_batchDirty |= 1 << drawLayer;
    strata->m_batchDirty = 1;
}

// VERIFY: 6bdea284 auto
// OFFSET: 0x00493C50
void CSimpleTop::NotifyFrameDelayedLoad(CSimpleFrame* frame) {
    CFrameStrata* strata = this->m_strata[frame->m_strata];
    strata->m_levels[frame->m_level]->m_batchDirty |= 0x20;
    strata->m_batchDirty = 1;
}

// VERIFY: 0f832a78 auto
// OFFSET: 0x00493C80
void CSimpleTop::SetCursor(MipBits* image) {
    if (image) {
        memcpy(g_theGxDevicePtr->CursorLock(), image->m_mip[0], sizeof(g_theGxDevicePtr->m_cursor));
        g_theGxDevicePtr->CursorUnlock(0, 0);
        g_theGxDevicePtr->CursorSetVisible(1);
    }
}

// VERIFY: 5547f677 auto
// OFFSET: 0x00493CE0
void CSimpleTop::MoveOrResizeFrame(const CMouseEvent& evt) {
    float offsetX = evt.m_data.m_mousePosition.x - this->m_moveResizeLast.x;
    float offsetY = evt.m_data.m_mousePosition.y - this->m_moveResizeLast.y;

    if (offsetX != 0.0f || offsetY != 0.0f) {
        this->m_moveResizeFrame->DragBy(offsetX, offsetY, this->m_moveResizePoint);
        this->m_moveResizeLast.x += offsetX;
        this->m_moveResizeLast.y += offsetY;
    }
}

// VERIFY: a9f9f36d auto
// OFFSET: 0x00493D70
float CSimpleTop::RoundToPixelWidth(float ddcWidth) {
    if (fabs(ddcWidth) < 0.00000023841858f) {
        return ddcWidth;
    }

    CRect windowRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSizeInScreenCoords(windowRect);

    float windowWidth = windowRect.maxX - windowRect.minX;

    if (fabs(windowWidth) < 0.00000023841858f) {
        return ddcWidth;
    }

    int32_t pixels = static_cast<int32_t>(CMath::fuint_pi(DDCToNDCWidth(ddcWidth) * windowWidth));

    return NDCToDDCWidth(pixels / windowWidth);
}

// VERIFY: c3d69b78 auto
// OFFSET: 0x00493E00
float CSimpleTop::RoundToPixelHeight(float ddcHeight) {
    if (fabs(ddcHeight) < 0.00000023841858f) {
        return ddcHeight;
    }

    CRect windowRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSizeInScreenCoords(windowRect);

    float windowHeight = windowRect.maxY - windowRect.minY;

    if (fabs(windowHeight) < 0.00000023841858f) {
        return ddcHeight;
    }

    int32_t pixels = static_cast<int32_t>(CMath::fuint_pi(DDCToNDCHeight(ddcHeight) * windowHeight));

    return NDCToDDCHeight(pixels / windowHeight);
}

// VERIFY: 03c64564 auto
// OFFSET: 0x00493F30
void CSimpleTop::HideFrame(CSimpleFrame* frame, int32_t a3) {
    if (this->m_moveResizeFrame == frame && !a3) {
        this->StopMoveOrResizeFrame();
    }

    frame->UnregisterForEvents(a3);

    CFrameStrata* strata = this->m_strata[frame->m_strata];

    if (frame->m_level < strata->m_levels.Count()) {
        strata->m_batchDirty |= strata->m_levels[frame->m_level]->RemoveFrame(frame);
        strata->m_levelsDirty = 1;
    }
}

// VERIFY: 0240934e manual
// OFFSET: 0x00493FB0
void CSimpleTop::SetMouseFocus(CSimpleFrame* frame) {
    CSimpleFrame* oldFocus = this->m_mouseFocus;

    if (frame != oldFocus) {
        this->m_mouseFocus = frame;

        if (oldFocus) {
            oldFocus->OnLayerCursorExit(0, 0);
        }

        if (frame) {
            frame->OnLayerCursorEnter(0);
        }
    }

    if (this->m_mouseCapture && this->m_mouseCapture != frame) {
        CMouseEvent evt;
        evt = this->m_mousePosition;
        evt.m_data.m_mousePosition.x = 0.0f;
        evt.m_data.m_mousePosition.y = 0.0f;
        evt.m_id = 0x400500C9;
        this->m_mouseCapture->OnLayerMouseUp(evt, nullptr);
        this->m_mouseCapture = nullptr;
    }
}

// VERIFY: 4a164119 manual
// OFFSET: 0x00494060
int32_t CSimpleTop::StartMoveOrResizeFrame(CSimpleFrame* frame, MOVERESIZE_REASON reason, float x, float y, FRAMEPOINT point) {
    if (!frame->FlattenFrame(this, 0.0f, 0.0f, 0.0f, 0.0f)) {
        return 0;
    }

    frame->Raise();
    frame->SetFrameFlag(0x1000, true);

    this->m_moveResizeLast.x = x;
    this->m_moveResizeReason = reason;
    this->m_moveResizeLast.y = y;
    this->m_moveResizePoint = point;
    this->m_moveResizeFrame = frame;

    CMouseEvent evt;
    evt = this->m_mousePosition;
    this->MoveOrResizeFrame(evt);

    return 1;
}

// VERIFY: 01703242 manual
// OFFSET: 0x00494120
int32_t CSimpleTop::StartMoveOrResizeFrame(CSimpleFrame* frame, MOVERESIZE_REASON reason, float x, float y, int32_t resize) {
    FRAMEPOINT point = FRAMEPOINT_CENTER;

    if (resize) {
        if (frame->IsResizePending()) {
            frame->Resize(1);
        }

        CRect rect = { 0.0f, 0.0f, 0.0f, 0.0f };

        if (!frame->GetRect(&rect)) {
            return 0;
        }

        float offsetY = y - rect.minY;
        float quarterWidth = (rect.maxX - rect.minX) * 0.25f;
        float offsetX = x - rect.minX;
        float quarterHeight = (rect.maxY - rect.minY) * 0.25f;

        if (offsetX < quarterWidth) {
            if (offsetY < quarterHeight) {
                point = FRAMEPOINT_BOTTOMLEFT;
            } else if (quarterHeight * 3.0f <= offsetY) {
                point = FRAMEPOINT_TOPLEFT;
            } else {
                point = FRAMEPOINT_LEFT;
            }
        } else if (offsetX >= quarterWidth * 3.0f) {
            if (offsetY < quarterHeight) {
                point = FRAMEPOINT_BOTTOMRIGHT;
            } else if (3.0f * quarterHeight <= offsetY) {
                point = FRAMEPOINT_TOPRIGHT;
            } else {
                point = FRAMEPOINT_RIGHT;
            }
        } else if (offsetY < quarterHeight) {
            point = FRAMEPOINT_BOTTOM;
        } else if (3.0f * quarterHeight <= offsetY) {
            point = FRAMEPOINT_TOP;
        }
    }

    return this->StartMoveOrResizeFrame(frame, reason, x, y, point);
}

// VERIFY: d60ec4ff manual
// OFFSET: 0x00494280
int32_t CSimpleTop::OnChar(const void* data, void* param) {
    const EVENT_DATA_CHAR* charData = static_cast<const EVENT_DATA_CHAR*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t handled = 0;

    CCharEvent evt;
    evt.m_data = *charData;
    evt.m_id = 0x40060067;

    for (int32_t strata = FRAME_STRATA_TOOLTIP; strata >= 0; strata--) {
        if (handled) {
            break;
        }

        CSimpleSortedArray<FRAMEPRIORITY*>& queue = top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
        queue.m_iterator = 0;

        while (queue.m_iterator < queue.m_count) {
            FRAMEPRIORITY** priority = &queue.m_array[queue.m_iterator++];

            if (!priority || handled) {
                break;
            }

            handled = (*priority)->m_frame->OnLayerChar(evt);
        }
    }

    return handled == 0;
}

// VERIFY: f2b35fa9 manual
// OFFSET: 0x00494320
int32_t CSimpleTop::OnIme(const void* data, void* param) {
    const EVENT_DATA_IME* imeData = static_cast<const EVENT_DATA_IME*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t handled = 0;

    CImeEvent evt;
    evt.m_data = *imeData;
    evt.m_id = 0x40060068;

    for (int32_t strata = FRAME_STRATA_TOOLTIP; strata >= 0; strata--) {
        if (handled) {
            break;
        }

        CSimpleSortedArray<FRAMEPRIORITY*>& queue = top->m_eventqueue[SIMPLE_EVENT_CHAR][strata];
        queue.m_iterator = 0;

        while (queue.m_iterator < queue.m_count) {
            FRAMEPRIORITY** priority = &queue.m_array[queue.m_iterator++];

            if (!priority || handled) {
                break;
            }

            handled = (*priority)->m_frame->OnLayerIme(evt);
        }
    }

    return handled == 0;
}

// VERIFY: 81128119 manual
// OFFSET: 0x004943C0
int32_t CSimpleTop::OnKeyDown(const void* data, void* param) {
    const EVENT_DATA_KEY* keyData = static_cast<const EVENT_DATA_KEY*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t handled = 0;

    CKeyEvent evt;
    evt.m_data = *keyData;
    CSimpleTop::s_eventTime = evt.m_data.m_time;
    evt.m_id = 0x40060064;

    if (top->m_keyDownCallback) {
        top->m_keyDownCallback(evt);
    }

    for (int32_t strata = FRAME_STRATA_TOOLTIP; strata >= 0; strata--) {
        if (handled) {
            break;
        }

        CSimpleSortedArray<FRAMEPRIORITY*>& queue = top->m_eventqueue[SIMPLE_EVENT_KEY][strata];
        queue.m_iterator = 0;

        while (queue.m_iterator < queue.m_count) {
            FRAMEPRIORITY** priority = &queue.m_array[queue.m_iterator++];

            if (!priority || handled) {
                break;
            }

            CSimpleFrame* frame = (*priority)->m_frame;
            handled = frame->OnLayerKeyDown(evt);

            if (handled) {
                top->m_keydownCapture[keyData->m_key] = frame;
            }
        }
    }

    return handled == 0;
}

// VERIFY: ab5a5ea6 manual
// OFFSET: 0x00494490
int32_t CSimpleTop::OnKeyUp(const void* data, void* param) {
    const EVENT_DATA_KEY* keyData = static_cast<const EVENT_DATA_KEY*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    CSimpleFrame* frame = top->m_keydownCapture[keyData->m_key];
    int32_t handled = 0;

    CSimpleTop::s_eventTime = keyData->m_time;

    if (frame) {
        CKeyEvent evt;
        evt.m_data = *keyData;
        evt.m_id = 0x40060066;
        frame->OnLayerKeyUp(evt);
        handled = 1;
    } else if (keyData->m_key == KEY_PRINTSCREEN) {
        handled = !CSimpleTop::OnKeyDown(data, param);
    }

    top->m_keydownCapture[keyData->m_key] = nullptr;

    return !handled;
}

// VERIFY: 060c6392 manual
// OFFSET: 0x00494530
int32_t CSimpleTop::OnKeyDownRepeat(const void* data, void* param) {
    const EVENT_DATA_KEY* keyData = static_cast<const EVENT_DATA_KEY*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    CSimpleFrame* frame = top->m_keydownCapture[keyData->m_key];
    int32_t handled = 0;

    if (frame) {
        CKeyEvent evt;
        evt.m_data = *keyData;
        evt.m_id = 0x40060065;
        frame->OnLayerKeyDownRepeat(evt);
        handled = 1;
    }

    return handled == 0;
}

// VERIFY: dbfbf7f9 manual
// OFFSET: 0x004945A0
int32_t CSimpleTop::OnMouseMove(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouseData = static_cast<const EVENT_DATA_MOUSE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t (*callback)(CMouseEvent*) = top->m_mouseMoveCallback;

    CMouseEvent evt;
    evt = *mouseData;

    CSimpleFrame* oldFocus = top->m_mouseFocus;
    evt.m_id = 0x400500CA;

    CSimpleFrame* newFocus = nullptr;
    int32_t motion = mouseData != &top->m_mousePosition;

    if (motion) {
        if (mouseData->m_x == top->m_mousePosition.m_x && mouseData->m_y == top->m_mousePosition.m_y) {
            return 0;
        }

        CSimpleTop::s_eventTime = mouseData->m_time;
    }

    memcpy(&top->m_mousePosition, mouseData, sizeof(top->m_mousePosition));

    if (top->m_moveResizeFrame) {
        CMouseEvent moveEvt;
        moveEvt = *mouseData;
        top->MoveOrResizeFrame(moveEvt);
        return 0;
    }

    if (callback && callback(&evt)) {
        return 0;
    }

    if (top->m_mouseCapture) {
        top->m_mouseCapture->OnLayerTrackUpdate(evt);
    }

    for (int32_t strata = FRAME_STRATA_TOOLTIP; strata >= 0; strata--) {
        if (newFocus) {
            break;
        }

        CSimpleSortedArray<FRAMEPRIORITY*>& queue = top->m_eventqueue[SIMPLE_EVENT_MOUSE][strata];
        queue.m_iterator = 0;

        while (queue.m_iterator < queue.m_count) {
            FRAMEPRIORITY** priority = &queue.m_array[queue.m_iterator++];

            if (!priority) {
                break;
            }

            CSimpleFrame* frame = (*priority)->m_frame;

            if (frame->OnLayerTrackUpdate(evt)) {
                newFocus = frame;
                break;
            }
        }
    }

    if (newFocus != oldFocus) {
        top->m_mouseFocus = newFocus;

        if (oldFocus) {
            oldFocus->OnLayerCursorExit(motion, 0);
        }

        if (newFocus) {
            newFocus->OnLayerCursorEnter(motion);
        }
    }

    return newFocus == nullptr;
}

// VERIFY: bb6eb71e manual
// OFFSET: 0x00494730
int32_t CSimpleTop::OnMouseMoveRelative(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouseData = static_cast<const EVENT_DATA_MOUSE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t (*callback)(CMouseEvent*) = top->m_mouseMoveCallback;

    CMouseEvent evt;
    evt = *mouseData;
    evt.m_id = 0x400500CB;
    CSimpleTop::s_eventTime = mouseData->m_time;

    return !callback || !callback(&evt);
}

// VERIFY: f7cc11f1 manual
// OFFSET: 0x004947A0
int32_t CSimpleTop::OnMouseUp(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouseData = static_cast<const EVENT_DATA_MOUSE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    CSimpleFrame* capture = top->m_mouseCapture;
    int32_t (*callback)(CMouseEvent*) = top->m_mouseButtonCallback;
    int32_t handled = 0;

    CMouseEvent evt;
    evt = *mouseData;
    CSimpleTop::s_eventTime = mouseData->m_time;
    evt.m_id = 0x400500C9;

    if (top->m_moveResizeReason == MOVERESIZE_REASON_LAYOUT || top->m_moveResizeReason == MOVERESIZE_REASON_TITLE) {
        top->StopMoveOrResizeFrame();
        return 0;
    }

    if (callback && !top->m_mouseCapture && callback(&evt)) {
        return 0;
    }

    if (capture) {
        capture->OnLayerMouseUp(evt, nullptr);
        handled = 1;

        if (!evt.m_data.m_buttonState) {
            top->m_mouseCapture = nullptr;
            return 0;
        }
    }

    return handled == 0;
}

// VERIFY: bb1ac078 manual
// OFFSET: 0x00494890
int32_t CSimpleTop::OnMouseWheel(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouseData = static_cast<const EVENT_DATA_MOUSE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t handled = 0;

    CMouseEvent evt;
    evt = *mouseData;

    C2Vector pt;
    CSimpleTop::s_eventTime = mouseData->m_time;
    evt.m_id = 0x400500CD;
    NDCToDDC(top->m_mousePosition.m_x, top->m_mousePosition.m_y, &pt.x, &pt.y);

    for (int32_t strata = FRAME_STRATA_TOOLTIP; strata >= 0; strata--) {
        if (handled) {
            break;
        }

        CSimpleSortedArray<FRAMEPRIORITY*>& queue = top->m_eventqueue[SIMPLE_EVENT_MOUSEWHEEL][strata];
        queue.m_iterator = 0;

        while (queue.m_iterator < queue.m_count) {
            FRAMEPRIORITY** priority = &queue.m_array[queue.m_iterator++];

            if (!priority || handled) {
                break;
            }

            CSimpleFrame* frame = (*priority)->m_frame;

            if (frame->TestHitRect(pt)) {
                handled = frame->OnLayerMouseWheel(evt);
            }
        }
    }

    return handled == 0;
}

// VERIFY: c36c518c manual
// OFFSET: 0x00494970
int32_t CSimpleTop::OnFocusChanged(const void* data, void* param) {
    const EVENT_DATA_FOCUS* focusData = static_cast<const EVENT_DATA_FOCUS*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    void (*callback)(const CFocusEvent&) = top->m_focusChangedCallback;

    OsIMESync();

    if (callback) {
        CFocusEvent evt;
        evt.m_data = *focusData;
        evt.m_id = 0x40040065;
        callback(evt);
    }

    return 1;
}

// VERIFY: 3330b0b2 manual
// OFFSET: 0x00494C80
CSimpleFrame* CFrameStrata::GetHitFrame(const C2Vector& pt) {
    for (uint32_t level = this->m_topLevel; level > 0;) {
        level--;
        CFrameStrataNode* node = this->m_levels[level];

        for (CSimpleFrame* frame = node->m_frames.Head(); frame; frame = node->m_frames.Next(frame)) {
            if (frame->TestHitRect(pt)) {
                while (!(frame->m_frameFlags & 1)) {
                    frame = frame->m_parent;

                    if (!frame) {
                        return nullptr;
                    }
                }

                return frame;
            }
        }
    }

    return nullptr;
}

// VERIFY: 314ab723 manual
// OFFSET: 0x00494D20
int32_t CFrameStrata::FrameOccluded(CSimpleFrame* frame) {
    CRect frameRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    CRect otherRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    for (uint32_t level = frame->m_level; level < this->m_topLevel; level++) {
        CFrameStrataNode* node = this->m_levels[level];

        for (CSimpleFrame* other = node->m_frames.Head(); other; other = node->m_frames.Next(other)) {
            if (other == frame) {
                continue;
            }

            CSimpleFrame* parent = other->m_parent;

            while (parent && parent != frame) {
                parent = parent->m_parent;
            }

            if (parent) {
                continue;
            }

            frame->GetRect(&frameRect);
            other->GetRect(&otherRect);
            frameRect = frameRect & otherRect;

            if (frameRect.maxY > frameRect.minY && frameRect.maxX > frameRect.minX) {
                return 1;
            }
        }
    }

    return 0;
}

// VERIFY: 8e94ced8 manual
// OFFSET: 0x00494E60
void CFrameStrata::CheckOcclusion() {
    for (uint32_t level = 0; level < this->m_topLevel; level++) {
        CFrameStrataNode* node = this->m_levels[level];

        for (CSimpleFrame* frame = node->m_frames.Head(); frame; frame = node->m_frames.Next(frame)) {
            if (frame->m_frameFlags & 1) {
                frame->SetFrameFlag(0x10, this->FrameOccluded(frame));
            }
        }
    }
}

// VERIFY: 2b665fc9 manual
// OFFSET: 0x00494EE0
int32_t CFrameStrata::BuildBatches(int32_t noMouseCapture) {
    if (this->m_levelsDirty) {
        this->CheckOcclusion();
        this->m_levelsDirty = 0;
    }

    if (!this->m_batchDirty) {
        return this->m_batchDirty;
    }

    this->m_batchDirty = 0;

    for (uint32_t level = 0; level < this->m_topLevel; level++) {
        if (this->m_levels[level]->BuildBatches()) {
            this->m_batchDirty = 1;
        }
    }

    return this->m_batchDirty;
}

// VERIFY: 62b40657 manual
// OFFSET: 0x00494F30
void CFrameStrata::RenderBatches() {
    for (uint32_t level = 0; level < this->m_topLevel; level++) {
        CFrameStrataNode* node = this->m_levels[level];

        for (CRenderBatch* batch = node->m_renderList.Head(); batch; batch = node->m_renderList.Next(batch)) {
            CSimpleRender::DrawBatch(batch);
        }
    }
}

// VERIFY: df78660e auto
// OFFSET: 0x00494F90
void CSimpleTop::RegisterFrame(CSimpleFrame* frame) {
    this->m_frames.LinkToTail(frame);
}

// VERIFY: ee8d9d78 auto
// OFFSET: 0x00494FA0
void CSimpleTop::UnregisterFrame(CSimpleFrame* frame) {
    if (this->m_moveResizeFrame == frame) {
        this->StopMoveOrResizeFrame();
    }

    frame->UnregisterForEvents(0);

    CFrameStrata* strata = this->m_strata[frame->m_strata];

    if (frame->m_level < strata->m_levels.Count()) {
        strata->m_batchDirty |= strata->m_levels[frame->m_level]->RemoveFrame(frame);
        strata->m_levelsDirty = 1;
    }

    this->m_frames.UnlinkNode(frame);
}

// VERIFY: b22dcea3 auto
// OFFSET: 0x00495050
void CSimpleTop::RegisterForCleanup(CSimpleFrame* frame) {
    this->m_destroyed.LinkToTail(frame);
}

// VERIFY: cfde847a manual
// OFFSET: 0x00495060
void CSimpleTop::CompressStrata(int32_t strata) {
    if (strata < FRAME_STRATA_WORLD || strata > FRAME_STRATA_TOOLTIP) {
        return;
    }

    uint32_t topLevel = this->m_strata[strata]->m_topLevel;
    int32_t emptyLevel = -1;
    uint32_t level = 0;

    while (level < topLevel) {
        int32_t used = this->m_strata[strata]->m_levels[level]->m_frames.Head() != nullptr;

        if (!used) {
            for (CSimpleFrame* frame = this->m_frames.Head(); frame; frame = this->m_frames.Next(frame)) {
                if (frame->m_strata == strata && frame->m_level == level) {
                    used = 1;
                    break;
                }
            }
        }

        if (!used) {
            if (emptyLevel == -1) {
                emptyLevel = level;
            }

            level++;
            continue;
        }

        if (emptyLevel == -1) {
            level++;
            continue;
        }

        uint32_t gap = level - emptyLevel;

        for (uint32_t moveLevel = level; moveLevel < topLevel; moveLevel++) {
            for (CSimpleFrame* frame = this->m_frames.Head(); frame; frame = this->m_frames.Next(frame)) {
                if (frame->m_strata == strata && frame->m_level == moveLevel) {
                    frame->SetFrameLevel(moveLevel - gap, 0);
                }
            }
        }

        topLevel -= gap;
        emptyLevel = -1;
    }

    if (emptyLevel != -1) {
        topLevel = emptyLevel;
    }

    this->m_strata[strata]->m_topLevel = topLevel;
}

// VERIFY: ca612c34 manual
// OFFSET: 0x004951A0
int32_t CSimpleTop::RaiseFrame(CSimpleFrame* frame, int32_t checkOcclusion) {
    while (!(frame->m_frameFlags & 1)) {
        frame = frame->m_parent;

        if (!frame) {
            return 0;
        }
    }

    if (checkOcclusion) {
        if (!(frame->m_layoutFlags & 0x100) || frame->IsResizePending()) {
            frame->Resize(1);
        }

        frame->SetFrameFlag(0x10, this->m_strata[frame->m_strata]->FrameOccluded(frame));
    }

    if (frame->m_frameFlags & 0x10) {
        this->CompressStrata(frame->m_strata);
        frame->SetFrameLevel(this->m_strata[frame->m_strata]->m_topLevel, 1);
        this->m_checkFocus = 1;
    }

    return 1;
}

// VERIFY: 36f05eee manual
// OFFSET: 0x00495270
int32_t CSimpleTop::StartMoveOrResizeFrame(const CMouseEvent& evt, MOVERESIZE_REASON reason, int32_t resize) {
    CSimpleFrame* frame = nullptr;

    for (int32_t strata = FRAME_STRATA_FULLSCREEN_DIALOG; strata >= 0; strata--) {
        if (frame) {
            break;
        }

        C2Vector pt;
        pt.x = evt.m_data.m_mousePosition.x;
        pt.y = evt.m_data.m_mousePosition.y;
        frame = this->m_strata[strata]->GetHitFrame(pt);
    }

    if (!frame) {
        return 0;
    }

    if (resize) {
        if (!(frame->m_frameFlags & 0x200)) {
            return 0;
        }
    } else if (!(frame->m_frameFlags & 0x100)) {
        return 0;
    }

    return this->StartMoveOrResizeFrame(frame, reason, evt.m_data.m_mousePosition.x, evt.m_data.m_mousePosition.y, resize);
}

// VERIFY: 09bbf45a auto
// OFFSET: 0x00495320
void CSimpleTop::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame* frame;

    while ((frame = this->m_destroyed.Head()) != nullptr) {
        this->m_destroyed.UnlinkNode(frame);
        frame->AllowAttributeChanges(0);
    }

    CLayoutFrame::ResizePending();

    for (int32_t strata = 0; strata < FRAME_STRATA_TOOLTIP + 1; strata++) {
        CFrameStrata* frameStrata = this->m_strata[strata];

        for (uint32_t level = 0; level < frameStrata->m_topLevel; level++) {
            frameStrata->m_levels[level]->OnLayerUpdate(elapsedSec);
        }
    }

    CLayoutFrame::ResizePending();

    if (this->m_checkFocus) {
        this->m_checkFocus = 0;
        CSimpleTop::OnMouseMove(&this->m_mousePosition, this);
    }
}

// VERIFY: 587fb7f2 auto
// OFFSET: 0x00495410
void CSimpleTop::OnLayerRender() {
    C2Vector offset;
    CameraSetupScreenProjection(this->m_rect, offset, 0.0f, 0);

    for (int32_t strata = 0; strata < FRAME_STRATA_TOOLTIP + 1; strata++) {
        this->m_strata[strata]->BuildBatches(this->m_mouseCapture == nullptr);
        this->m_strata[strata]->RenderBatches();
    }
}

// VERIFY: 6721d10a manual
// OFFSET: 0x00495470
int32_t CSimpleTop::OnMouseDown(const void* data, void* param) {
    const EVENT_DATA_MOUSE* mouseData = static_cast<const EVENT_DATA_MOUSE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    int32_t (*callback)(CMouseEvent*) = top->m_mouseButtonCallback;

    CMouseEvent evt;
    evt = *mouseData;
    CSimpleTop::s_eventTime = mouseData->m_time;
    evt.m_id = 0x400500C8;
    memcpy(&top->m_mousePosition, mouseData, sizeof(top->m_mousePosition));

    if (top->m_moveResizeEnabled && (EventIsKeyDown(KEY_LCONTROL) || EventIsKeyDown(KEY_LALT))) {
        top->StartMoveOrResizeFrame(evt, MOVERESIZE_REASON_LAYOUT, EventIsKeyDown(KEY_LCONTROL));
        return 0;
    }

    if (callback && !top->m_mouseCapture && callback(&evt)) {
        return 0;
    }

    CSimpleFrame* frame = top->m_mouseCapture;

    if (!frame) {
        frame = top->m_mouseFocus;

        if (!frame) {
            return 1;
        }
    }

    frame->Raise();

    if (frame->m_titleRegion) {
        C2Vector pt;
        pt.x = evt.m_data.m_mousePosition.x;
        pt.y = evt.m_data.m_mousePosition.y;

        if (frame->m_titleRegion->PtInFrameRect(pt)) {
            top->StartMoveOrResizeFrame(frame, MOVERESIZE_REASON_TITLE, evt.m_data.m_mousePosition.x, evt.m_data.m_mousePosition.y, FRAMEPOINT_CENTER);
            return 0;
        }
    }

    top->m_mouseCapture = frame;
    frame->OnLayerMouseDown(evt, nullptr);

    return 0;
}

// VERIFY: c200f27f manual
// OFFSET: 0x004955E0
int32_t CSimpleTop::OnDisplaySizeChanged(const void* data, void* param) {
    const EVENT_DATA_SIZE* sizeData = static_cast<const EVENT_DATA_SIZE*>(data);
    CSimpleTop* top = static_cast<CSimpleTop*>(param);
    void (*callback)(const CSizeEvent&) = top->m_displaySizeCallback;

    GxuFontWindowSizeChanged();

    for (CSimpleFrame* frame = top->m_frames.Head(); frame; frame = top->m_frames.Next(frame)) {
        frame->OnScreenSizeChanged();
    }

    if (callback) {
        CSizeEvent evt;
        evt.m_data = *sizeData;
        evt.m_id = 0x40040064;
        callback(evt);
    }

    return 1;
}

// VERIFY: 524e2c20 auto
// OFFSET: 0x00495840
void CSimpleTop::NotifyFrameMovedOrResized(CSimpleFrame* frame) {
    this->m_strata[frame->m_strata]->m_levelsDirty = 1;

    if (this->m_moveResizeFrame) {
        this->RaiseFrame(this->m_moveResizeFrame, 0);
    }

    this->m_checkFocus = 1;
}

// VERIFY: e3307852 manual
// OFFSET: 0x00495880
void CSimpleTop::RegisterForEvent(CSimpleFrame* frame, CSimpleEventType event, int32_t a4, int32_t priority) {
    CSimpleSortedArray<FRAMEPRIORITY*>& queue = this->m_eventqueue[event][frame->m_strata];

    if (priority == -1) {
        priority = frame->m_level;
    }

    FRAMEPRIORITY* framePriority = static_cast<FRAMEPRIORITY*>(SMemAlloc(sizeof(FRAMEPRIORITY), __FILE__, __LINE__, 0));
    framePriority->m_frame = frame;
    framePriority->m_priority = priority;
    queue.Insert(framePriority);

    if (!a4 && event == SIMPLE_EVENT_MOUSE) {
        this->m_checkFocus = 1;
    }
}

// VERIFY: 4939f5d1 manual
// OFFSET: 0x00495900
void CSimpleTop::UnregisterForEvent(CSimpleFrame* frame, CSimpleEventType event, int32_t a4) {
    CSimpleSortedArray<FRAMEPRIORITY*>& queue = this->m_eventqueue[event][frame->m_strata];

    for (uint32_t i = 0; i < queue.m_count; i++) {
        FRAMEPRIORITY* framePriority = queue.m_array[i];

        if (framePriority->m_frame != frame) {
            continue;
        }

        queue.Remove(i);
        delete framePriority;

        if (a4) {
            return;
        }

        if (event == SIMPLE_EVENT_KEY) {
            for (uint32_t key = 0; key < sizeof(this->m_keydownCapture) / sizeof(this->m_keydownCapture[0]); key++) {
                if (this->m_keydownCapture[key] == frame) {
                    this->m_keydownCapture[key] = nullptr;
                }
            }
        } else if (event == SIMPLE_EVENT_MOUSE) {
            if (frame == this->m_mouseCapture) {
                this->m_mouseCapture = nullptr;
            }

            if (frame == this->m_mouseFocus) {
                this->m_mouseFocus = nullptr;
                this->m_checkFocus = 1;

                if (frame->m_lifecycleState != 3) {
                    frame->OnLayerCursorExit(0, 1);
                }
            }
        }

        return;
    }
}

// VERIFY: f41311c7 auto
// OFFSET: 0x004959F0
void CSimpleTop::EnableEvents() {
    EventRegisterEx(EVENT_ID_CHAR, &CSimpleTop::OnChar, this, 1.0f);
    EventRegisterEx(EVENT_ID_IME, &CSimpleTop::OnIme, this, 1.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN, &CSimpleTop::OnKeyDown, this, 1.0f);
    EventRegisterEx(EVENT_ID_KEYUP, &CSimpleTop::OnKeyUp, this, 1.0f);
    EventRegisterEx(EVENT_ID_KEYDOWN_REPEATING, &CSimpleTop::OnKeyDownRepeat, this, 1.0f);
    EventRegisterEx(EVENT_ID_MOUSEMOVE, &CSimpleTop::OnMouseMove, this, 1.0f);
    EventRegisterEx(EVENT_ID_MOUSEMOVE_RELATIVE, &CSimpleTop::OnMouseMoveRelative, this, 1.0f);
    EventRegisterEx(EVENT_ID_MOUSEDOWN, &CSimpleTop::OnMouseDown, this, 1.0f);
    EventRegisterEx(EVENT_ID_MOUSEUP, &CSimpleTop::OnMouseUp, this, 1.0f);
    EventRegisterEx(EVENT_ID_MOUSEWHEEL, &CSimpleTop::OnMouseWheel, this, 1.0f);
    EventRegisterEx(EVENT_ID_SIZE, &CSimpleTop::OnDisplaySizeChanged, this, 1.0f);
    EventRegisterEx(EVENT_ID_FOCUS, &CSimpleTop::OnFocusChanged, this, 1.0f);
}

// VERIFY: 8b4e6642 auto
// OFFSET: 0x00495B00
void CSimpleTop::DisableEvents() {
    EventUnregisterEx(EVENT_ID_CHAR, &CSimpleTop::OnChar, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_IME, &CSimpleTop::OnIme, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_KEYDOWN, &CSimpleTop::OnKeyDown, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_KEYUP, &CSimpleTop::OnKeyUp, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_KEYDOWN_REPEATING, &CSimpleTop::OnKeyDownRepeat, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_MOUSEMOVE, &CSimpleTop::OnMouseMove, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_MOUSEMOVE_RELATIVE, &CSimpleTop::OnMouseMoveRelative, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_MOUSEDOWN, &CSimpleTop::OnMouseDown, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_MOUSEUP, &CSimpleTop::OnMouseUp, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_MOUSEWHEEL, &CSimpleTop::OnMouseWheel, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_SIZE, &CSimpleTop::OnDisplaySizeChanged, this, 0xFFFFFFFF);
    EventUnregisterEx(EVENT_ID_FOCUS, &CSimpleTop::OnFocusChanged, this, 0xFFFFFFFF);
}

// INLINED
CFrameStrata::CFrameStrata() {
    this->m_batchDirty = 0;
    this->m_levelsDirty = 0;
    this->m_topLevel = 0;
}

// VERIFY: 46f7c2a5 manual
// OFFSET: 0x00495DE0
void CFrameStrata::AddFrame(CSimpleFrame* frame) {
    uint32_t level = frame->m_level;

    if (level >= this->m_levels.Count()) {
        uint32_t count = this->m_levels.Count();
        this->m_levels.SetCount(level + 1);

        for (; count <= level; count++) {
            this->m_levels[count] = new (SMemAlloc(sizeof(CFrameStrataNode), __FILE__, __LINE__, 0)) CFrameStrataNode;
        }
    }

    if (level >= this->m_topLevel) {
        this->m_topLevel = level + 1;
    }

    CFrameStrataNode* node = this->m_levels[level];

    if (!frame->m_strataLink.IsLinked()) {
        TSExplicitList<CSimpleFrame, offsetof(CSimpleFrame, m_strataLink)>* list = node->m_pendingFrame ? &node->m_pendingFrames : &node->m_frames;
        list->LinkToTail(frame);

        if (!(frame->m_frameFlags & 0x2000)) {
            node->m_batchDirty = -1;
        }
    }

    this->m_levelsDirty = 1;
    this->m_batchDirty |= node->m_batchDirty != 0;
}

// VERIFY: a790458c manual
// OFFSET: 0x00495EB0
CSimpleTop::CSimpleTop() {
    this->m_mouseFocus = nullptr;
    this->m_mouseCapture = nullptr;
    this->m_checkFocus = 1;
    this->m_mouseButtonName = nullptr;
    this->m_mouseButtonCallback = nullptr;
    this->m_mouseMoveCallback = nullptr;
    this->m_displaySizeCallback = nullptr;
    this->m_focusChangedCallback = nullptr;
    this->m_keyDownCallback = nullptr;
    this->m_allowProtectedActions = 1;
    this->unused_1250 = 0;
    this->m_actionBlockedCallback = nullptr;

    CSimpleTop::s_instance = this;
    ScrnLayerCreate(nullptr, 1.0f, 4, nullptr, &PaintScreen, &this->m_screenLayer);
    this->EnableEvents();

    memset(this->m_keydownCapture, 0, sizeof(this->m_keydownCapture));

    NDCToDDC(1.0f, 1.0f, &this->m_rect.maxX, &this->m_rect.maxY);
    this->m_layoutFlags |= 0x100;

    this->m_moveResizeEnabled = 0;
    this->m_moveResizeFrame = nullptr;
    this->m_moveResizeReason = MOVERESIZE_REASON_NONE;
    this->m_moveResizePoint = FRAMEPOINT_TOPLEFT;
    this->m_moveResizeLast.x = 0.0f;
    this->m_moveResizeLast.y = 0.0f;

    for (int32_t strata = 0; strata < FRAME_STRATA_TOOLTIP + 1; strata++) {
        this->m_strata[strata] = new (SMemAlloc(sizeof(CFrameStrata), __FILE__, __LINE__, 0)) CFrameStrata;
    }

    this->m_mousePosition.m_mode = MOUSE_MODE_NORMAL;
    this->m_mousePosition.m_button = static_cast<MOUSEBUTTON>(0);
    this->m_mousePosition.m_buttonState = 0;
    this->m_mousePosition.m_metaKeyState = 0;
    this->m_mousePosition.m_flags = 0;
    this->m_mousePosition.m_x = 0.0f;
    this->m_mousePosition.m_y = 0.0f;
    this->m_mousePosition.m_wheelDistance = 0;
    this->m_mousePosition.m_time = 0;

    float x;
    float y;
    EventInputGetMousePosition(&x, &y);
    DDCToNDC(x, y, &this->m_mousePosition.m_x, &this->m_mousePosition.m_y);

    CSimpleRender::Init();
    CSimpleTexture::Init();
}

// VERIFY: 4c3b2719 auto
// OFFSET: 0x004960D0
void CSimpleTop::ShowFrame(CSimpleFrame* frame, int32_t a3) {
    this->m_strata[frame->m_strata]->AddFrame(frame);
    frame->RegisterForEvents(a3);
}

// VERIFY: 9bb3362f manual
// OFFSET: 0x00496100
CFrameStrata::~CFrameStrata() {
    for (uint32_t level = 0; level < this->m_levels.Count(); level++) {
        delete this->m_levels[level];
        this->m_levels[level] = nullptr;
    }
}

// VERIFY: caeade74 manual
// OFFSET: [0x00496170, 0x00496360]
CSimpleTop::~CSimpleTop() {
    CSimpleFrame* frame;

    while ((frame = this->m_destroyed.Head()) != nullptr) {
        this->m_destroyed.UnlinkNode(frame);
    }

    while ((frame = this->m_frames.Head()) != nullptr) {
        this->m_frames.UnlinkNode(frame);
        delete frame;
    }

    for (int32_t strata = 0; strata < FRAME_STRATA_TOOLTIP + 1; strata++) {
        delete this->m_strata[strata];
        this->m_strata[strata] = nullptr;
    }

    CSimpleFontString::FreeEmbededTextureStorage();
    CSimpleFont::Destroy();
    CSimpleTexture::Destroy();
    CSimpleRender::Destroy();

    this->DisableEvents();
    HandleClose(reinterpret_cast<HOBJECT>(this->m_screenLayer));
    CSimpleTop::s_instance = nullptr;
}

// VERIFY: a98d5c82 auto
// OFFSET: 0x004899F0
int32_t CSimpleTop::LowerFrame(CSimpleFrame* frame) {
    return 0;
}

// VERIFY: 0cc3f3ae auto
// OFFSET: 0x00632050
void CSimpleTop::ValidateDeletedFrame(CSimpleFrame* frame) {
}
