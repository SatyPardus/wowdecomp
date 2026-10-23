#ifndef ENGINE_SOURCE_FRAME_TYPES_H
#define ENGINE_SOURCE_FRAME_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class CScriptRegionAnimGroupNode;
class CSimpleFrame;
struct SIMPLEFRAMENODE;

enum FRAMEPOINT {
    FRAMEPOINT_TOPLEFT = 0,
    FRAMEPOINT_TOP = 1,
    FRAMEPOINT_TOPRIGHT = 2,
    FRAMEPOINT_LEFT = 3,
    FRAMEPOINT_CENTER = 4,
    FRAMEPOINT_RIGHT = 5,
    FRAMEPOINT_BOTTOMLEFT = 6,
    FRAMEPOINT_BOTTOM = 7,
    FRAMEPOINT_BOTTOMRIGHT = 8,
    FRAMEPOINT_NUMPOINTS = 9,
};

enum FRAME_STRATA {
    FRAME_STRATA_WORLD = 0,
    FRAME_STRATA_BACKGROUND = 1,
    FRAME_STRATA_LOW = 2,
    FRAME_STRATA_MEDIUM = 3,
    FRAME_STRATA_HIGH = 4,
    FRAME_STRATA_DIALOG = 5,
    FRAME_STRATA_FULLSCREEN = 6,
    FRAME_STRATA_FULLSCREEN_DIALOG = 7,
    FRAME_STRATA_TOOLTIP = 8,
    NUM_FRAME_STRATA = 9,
};

enum : uint32_t {
    DRAWLAYER_BACKGROUND = 0,
    DRAWLAYER_BACKGROUND_BORDER = 1,
    DRAWLAYER_ARTWORK = 2,
    DRAWLAYER_ARTWORK_OVERLAY = 3,
    DRAWLAYER_HIGHLIGHT = 4,
    NUM_SIMPLEFRAME_DRAWLAYERS = 5,
};

enum CSimpleButtonState {
    BUTTONSTATE_DISABLED = 0,
    BUTTONSTATE_NORMAL = 1,
    BUTTONSTATE_PUSHED = 2,
    NUM_BUTTONSTATES = 3,
};

enum SimpleMessageFrameInsertMode : int32_t {
    INSERT_AT_TOP = 0,
    INSERT_AT_BOTTOM = 1,
};

enum TextureImageMode {
    ImageMode_Normal = 0,
    ImageMode_Desaturate = 1,
};

enum HTML_TEXT_TYPE {
    HTML_TEXT_NORMAL = 0,
    HTML_TEXT_HEADER1 = 1,
    HTML_TEXT_HEADER2 = 2,
    HTML_TEXT_HEADER3 = 3,
    NUM_HTML_TEXT_TYPES = 4,
};

enum ORIENTATION {
    ORIENTATION_HORIZONTAL = 0,
    ORIENTATION_VERTICAL = 1,
};

enum SimpleAnimCurveType {
    ANIM_CURVE_NONE = 0,
    ANIM_CURVE_SMOOTH = 1,
};

enum SimpleAnimLoopState {
    ANIM_LOOPSTATE_NONE = 0,
    ANIM_LOOPSTATE_FORWARD = 1,
    ANIM_LOOPSTATE_REVERSE = 2,
};

enum SimpleAnimLoopType {
    ANIM_LOOPTYPE_NONE = 0,
    ANIM_LOOPTYPE_REPEAT = 1,
    ANIM_LOOPTYPE_BOUNCE = 2,
};

class CColor {
    public:
    /* 0000 */ uint8_t m_b;
    /* 0001 */ uint8_t m_g;
    /* 0002 */ uint8_t m_r;
    /* 0003 */ uint8_t m_a;
};

class CScriptRegionAnimGroupNode {
    public:
    /* 0000 */ CScriptRegionAnimGroupNode* m_prev;
    /* 0004 */ CScriptRegionAnimGroupNode* m_next;
    /* 0008 */ void* m_group;
};

enum MOVERESIZE_REASON {
    MOVERESIZE_REASON_NONE = 0,
    MOVERESIZE_REASON_LAYOUT = 1,
    MOVERESIZE_REASON_TITLE = 2,
    MOVERESIZE_REASON_SCRIPT = 3,
};

enum CSimpleEventType {
    SIMPLE_EVENT_CHAR = 0,
    SIMPLE_EVENT_KEY = 1,
    SIMPLE_EVENT_MOUSE = 2,
    SIMPLE_EVENT_MOUSEWHEEL = 3,
    SIMPLE_EVENT_JOYSTICK = 4,
    NUM_SIMPLE_EVENTS = 5,
};

struct SIMPLEFRAMENODE : public TSLinkedNode<SIMPLEFRAMENODE> {
    /* 0008 */ CSimpleFrame* m_frame;
};

#endif
