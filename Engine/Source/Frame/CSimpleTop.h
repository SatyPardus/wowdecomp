#ifndef ENGINE_SOURCE_FRAME_CSIMPLETOP_H
#define ENGINE_SOURCE_FRAME_CSIMPLETOP_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Scrn/Types.h"
#include "Engine/Source/Event/Types.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Storm/H/stpl.h"

class CFrameStrataNode;
class CKeyEvent;
class CMouseEvent;
class CScriptRegion;
class CSimpleFrame;
class CSizeEvent;
class CFocusEvent;
struct MipBits;

class CFrameStrata {
    public:
    /* 0000 */ int32_t m_batchDirty;
    /* 0004 */ int32_t m_levelsDirty;
    /* 0008 */ uint32_t m_topLevel;
    /* 000C */ TSFixedArray<CFrameStrataNode*> m_levels;

    CFrameStrata();
    ~CFrameStrata();
    void AddFrame(CSimpleFrame* frame);
    int32_t BuildBatches(int32_t noMouseCapture);
    void CheckOcclusion();
    int32_t FrameOccluded(CSimpleFrame* frame);
    CSimpleFrame* GetHitFrame(const C2Vector& pt);
    void RenderBatches();
};

class FRAMEPRIORITY {
    public:
    /* 0000 */ CSimpleFrame* m_frame;
    /* 0004 */ uint32_t m_priority;
};

template <class T>
class CSimpleSortedArray {
    public:
    /* 0000 */ TSGrowableArray<T> m_array;
    /* 0010 */ uint32_t m_count;
    /* 0014 */ uint32_t m_maxcount;
    /* 0018 */ uint32_t m_iterator;

    CSimpleSortedArray();
    uint32_t Insert(T element);
    void Remove(uint32_t index);
};

class CSimpleTop : public CLayoutFrame {
    public:
    static CSimpleTop* s_instance;
    static uint32_t s_eventTime;

    static CSimpleTop* GetInstance();
    static int32_t OnChar(const void* data, void* param);
    static int32_t OnDisplaySizeChanged(const void* data, void* param);
    static int32_t OnFocusChanged(const void* data, void* param);
    static int32_t OnIme(const void* data, void* param);
    static int32_t OnKeyDown(const void* data, void* param);
    static int32_t OnKeyDownRepeat(const void* data, void* param);
    static int32_t OnKeyUp(const void* data, void* param);
    static int32_t OnMouseDown(const void* data, void* param);
    static int32_t OnMouseMove(const void* data, void* param);
    static int32_t OnMouseMoveRelative(const void* data, void* param);
    static int32_t OnMouseUp(const void* data, void* param);
    static int32_t OnMouseWheel(const void* data, void* param);
    static float RoundToPixelHeight(float ddcHeight);
    static float RoundToPixelWidth(float ddcWidth);

    /* 0074 */ HLAYER m_screenLayer;
    /* 0078 */ CSimpleFrame* m_mouseFocus;
    /* 007C */ CSimpleFrame* m_mouseCapture;
    /* 0080 */ CSimpleFrame* m_keydownCapture [787];
    /* 0CCC */ TSExplicitList<CSimpleFrame, offsetof(CSimpleFrame, m_framesLink)> m_frames;
    /* 0CD8 */ TSExplicitList<CSimpleFrame, offsetof(CSimpleFrame, m_destroyedLink)> m_destroyed;
    /* 0CE4 */ CFrameStrata* m_strata [NUM_FRAME_STRATA];
    /* 0D08 */ int32_t m_moveResizeEnabled;
    /* 0D0C */ CSimpleFrame* m_moveResizeFrame;
    /* 0D10 */ MOVERESIZE_REASON m_moveResizeReason;
    /* 0D14 */ FRAMEPOINT m_moveResizePoint;
    /* 0D18 */ C2Vector m_moveResizeLast;
    /* 0D20 */ CSimpleSortedArray<FRAMEPRIORITY*> m_eventqueue [NUM_SIMPLE_EVENTS][FRAME_STRATA_TOOLTIP + 1];
    /* 120C */ int32_t m_checkFocus;
    /* 1210 */ EVENT_DATA_MOUSE m_mousePosition;
    /* 1234 */ const char* m_mouseButtonName;
    /* 1238 */ int32_t (*m_mouseButtonCallback)(CMouseEvent* evt);
    /* 123C */ int32_t (*m_mouseMoveCallback)(CMouseEvent* evt);
    /* 1240 */ void (*m_displaySizeCallback)(const CSizeEvent& evt);
    /* 1244 */ void (*m_focusChangedCallback)(const CFocusEvent& evt);
    /* 1248 */ void (*m_keyDownCallback)(const CKeyEvent& evt);
    /* 124C */ int32_t m_allowProtectedActions;
    /* 1250 */ uint32_t unused_1250;
    /* 1254 */ void (*m_actionBlockedCallback)(CScriptRegion* region);

    virtual ~CSimpleTop();

    CSimpleTop();
    void CompressStrata(int32_t strata);
    void DisableEvents();
    void EnableEvents();
    void GetCursorPosition(C2Vector& position);
    void HideFrame(CSimpleFrame* frame, int32_t a3);
    int32_t LowerFrame(CSimpleFrame* frame);
    void MoveOrResizeFrame(const CMouseEvent& evt);
    void NotifyFrameDelayedLoad(CSimpleFrame* frame);
    void NotifyFrameLayerChanged(CSimpleFrame* frame, uint32_t drawLayer);
    void NotifyFrameMovedOrResized(CSimpleFrame* frame);
    void OnLayerRender();
    void OnLayerUpdate(float elapsedSec);
    int32_t RaiseFrame(CSimpleFrame* frame, int32_t checkOcclusion);
    void RegisterForCleanup(CSimpleFrame* frame);
    void RegisterForEvent(CSimpleFrame* frame, CSimpleEventType event, int32_t a4, int32_t priority);
    void RegisterFrame(CSimpleFrame* frame);
    void SetCursor(MipBits* image);
    void SetMouseFocus(CSimpleFrame* frame);
    void ShowFrame(CSimpleFrame* frame, int32_t a3);
    int32_t StartMoveOrResizeFrame(const CMouseEvent& evt, MOVERESIZE_REASON reason, int32_t resize);
    int32_t StartMoveOrResizeFrame(CSimpleFrame* frame, MOVERESIZE_REASON reason, float x, float y, FRAMEPOINT point);
    int32_t StartMoveOrResizeFrame(CSimpleFrame* frame, MOVERESIZE_REASON reason, float x, float y, int32_t resize);
    void StopMoveOrResizeFrame();
    void UnregisterForEvent(CSimpleFrame* frame, CSimpleEventType event, int32_t a4);
    void UnregisterFrame(CSimpleFrame* frame);
    void ValidateDeletedFrame(CSimpleFrame* frame);
};

// INLINED
template <class T>
CSimpleSortedArray<T>::CSimpleSortedArray() {
    this->m_count = 0;
    this->m_maxcount = 0;
    this->m_iterator = 0;
}

// VERIFY: 057eea5d manual
// OFFSET: 0x00495670
template <class T>
uint32_t CSimpleSortedArray<T>::Insert(T element) {
    uint32_t priority = element->m_priority;

    if (this->m_count == this->m_maxcount) {
        this->m_maxcount += 8;
        this->m_array.SetCount(this->m_maxcount);
    }

    uint32_t index;

    for (index = 0; index < this->m_count; index++) {
        if (priority > this->m_array[index]->m_priority) {
            for (uint32_t i = this->m_count; i > index; i--) {
                this->m_array[i] = this->m_array[i - 1];
            }

            this->m_array[index] = element;
            break;
        }
    }

    if (index == this->m_count) {
        this->m_array[index] = element;
    }

    if (index < this->m_iterator) {
        this->m_iterator++;
    }

    this->m_count++;

    return index;
}

// VERIFY: d5e547e6 manual
// OFFSET: 0x00495710
template <class T>
void CSimpleSortedArray<T>::Remove(uint32_t index) {
    for (uint32_t i = index; i < this->m_count - 1; i++) {
        this->m_array[i] = this->m_array[i + 1];
    }

    if (index < this->m_iterator) {
        this->m_iterator--;
    }

    this->m_count--;

    if (this->m_maxcount - this->m_count > 8) {
        this->m_maxcount -= 8;
        this->m_array.SetCount(this->m_maxcount);
    }
}

#endif
