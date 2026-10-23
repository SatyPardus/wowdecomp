#ifndef ENGINE_SOURCE_EVENT_TYPES_H
#define ENGINE_SOURCE_EVENT_TYPES_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C2Vector.h"

enum KEY : uint32_t {
    KEY_NONE = 0xFFFFFFFF,
    KEY_LSHIFT = 0,
    KEY_RSHIFT = 1,
    KEY_LCONTROL = 2,
    KEY_RCONTROL = 3,
    KEY_LALT = 4,
    KEY_RALT = 5,
    KEY_LASTMETAKEY = 5,
    KEY_SPACE = 32,
    KEY_0 = 48,
    KEY_1 = 49,
    KEY_2 = 50,
    KEY_3 = 51,
    KEY_4 = 52,
    KEY_5 = 53,
    KEY_6 = 54,
    KEY_7 = 55,
    KEY_8 = 56,
    KEY_9 = 57,
    KEY_A = 65,
    KEY_B = 66,
    KEY_C = 67,
    KEY_D = 68,
    KEY_E = 69,
    KEY_F = 70,
    KEY_G = 71,
    KEY_H = 72,
    KEY_I = 73,
    KEY_J = 74,
    KEY_K = 75,
    KEY_L = 76,
    KEY_M = 77,
    KEY_N = 78,
    KEY_O = 79,
    KEY_P = 80,
    KEY_Q = 81,
    KEY_R = 82,
    KEY_S = 83,
    KEY_T = 84,
    KEY_U = 85,
    KEY_V = 86,
    KEY_W = 87,
    KEY_X = 88,
    KEY_Y = 89,
    KEY_Z = 90,
    KEY_TILDE = 96,
    KEY_NUMPAD0 = 0x100,
    KEY_NUMPAD1 = 0x101,
    KEY_NUMPAD2 = 0x102,
    KEY_NUMPAD3 = 0x103,
    KEY_NUMPAD4 = 0x104,
    KEY_NUMPAD5 = 0x105,
    KEY_NUMPAD6 = 0x106,
    KEY_NUMPAD7 = 0x107,
    KEY_NUMPAD8 = 0x108,
    KEY_NUMPAD9 = 0x109,
    KEY_NUMPAD_PLUS = 0x10A,
    KEY_NUMPAD_MINUS = 0x10B,
    KEY_NUMPAD_MULTIPLY = 0x10C,
    KEY_NUMPAD_DIVIDE = 0x10D,
    KEY_NUMPAD_DECIMAL = 0x10E,
    KEY_NUMPAD_EQUALS = 0x30C,
    KEY_PLUS = 61,
    KEY_MINUS = 45,
    KEY_BRACKET_OPEN = 91,
    KEY_BRACKET_CLOSE = 93,
    KEY_SLASH = 47,
    KEY_BACKSLASH = 92,
    KEY_SEMICOLON = 59,
    KEY_APOSTROPHE = 39,
    KEY_COMMA = 44,
    KEY_PERIOD = 46,
    KEY_ESCAPE = 0x200,
    KEY_ENTER = 0x201,
    KEY_BACKSPACE = 0x202,
    KEY_TAB = 0x203,
    KEY_LEFT = 0x204,
    KEY_UP = 0x205,
    KEY_RIGHT = 0x206,
    KEY_DOWN = 0x207,
    KEY_INSERT = 0x208,
    KEY_DELETE = 0x209,
    KEY_HOME = 0x20A,
    KEY_END = 0x20B,
    KEY_PAGEUP = 0x20C,
    KEY_PAGEDOWN = 0x20D,
    KEY_CAPSLOCK = 0x20E,
    KEY_NUMLOCK = 0x20F,
    KEY_SCROLLLOCK = 0x210,
    KEY_PAUSE = 0x211,
    KEY_PRINTSCREEN = 0x212,
    KEY_F1 = 0x300,
    KEY_F2 = 0x301,
    KEY_F3 = 0x302,
    KEY_F4 = 0x303,
    KEY_F5 = 0x304,
    KEY_F6 = 0x305,
    KEY_F7 = 0x306,
    KEY_F8 = 0x307,
    KEY_F9 = 0x308,
    KEY_F10 = 0x309,
    KEY_F11 = 0x30A,
    KEY_F12 = 0x30B,
    KEY_F13 = 0x212,
    KEY_F14 = 0x30D,
    KEY_F15 = 0x30E,
    KEY_F16 = 0x30F,
    KEY_F17 = 0x310,
    KEY_F18 = 0x311,
    KEY_F19 = 0x312,
    KEY_LAST = 0x313,
};

enum MOUSEBUTTON : uint32_t {
    MOUSE_BUTTON_NONE = 0,
    MOUSE_BUTTON_LEFT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_RIGHT = 4,
    MOUSE_BUTTON_XBUTTON1 = 8,
    MOUSE_BUTTON_XBUTTON2 = 16,
    MOUSE_BUTTON_XBUTTON3 = 32,
    MOUSE_BUTTON_XBUTTON4 = 64,
    MOUSE_BUTTON_XBUTTON5 = 128,
    MOUSE_BUTTON_XBUTTON6 = 0x100,
    MOUSE_BUTTON_XBUTTON7 = 0x200,
    MOUSE_BUTTON_XBUTTON8 = 0x400,
    MOUSE_BUTTON_XBUTTON9 = 0x800,
    MOUSE_BUTTON_XBUTTON10 = 0x1000,
    MOUSE_BUTTON_XBUTTON11 = 0x2000,
    MOUSE_BUTTON_XBUTTON12 = 0x4000,
    MOUSE_BUTTON_XBUTTON13 = 0x8000,
    MOUSE_BUTTON_XBUTTON14 = 0x10000,
    MOUSE_BUTTON_XBUTTON15 = 0x20000,
    MOUSE_BUTTON_XBUTTON16 = 0x40000,
    MOUSE_BUTTON_XBUTTON17 = 0x80000,
    MOUSE_BUTTON_XBUTTON18 = 0x100000,
    MOUSE_BUTTON_XBUTTON19 = 0x200000,
    MOUSE_BUTTON_XBUTTON20 = 0x400000,
    MOUSE_BUTTON_XBUTTON21 = 0x800000,
    MOUSE_BUTTON_XBUTTON22 = 0x1000000,
    MOUSE_BUTTON_XBUTTON23 = 0x2000000,
    MOUSE_BUTTON_XBUTTON24 = 0x4000000,
    MOUSE_BUTTON_XBUTTON25 = 0x8000000,
    MOUSE_BUTTON_XBUTTON26 = 0x10000000,
    MOUSE_BUTTON_XBUTTON27 = 0x20000000,
    MOUSE_BUTTON_XBUTTON28 = 0x40000000,
    MOUSE_BUTTON_ALL = 0xFFFFFFFF,
};

enum MOUSEMODE {
    MOUSE_MODE_NORMAL = 0,
    MOUSE_MODE_RELATIVE = 1,
    MOUSE_MODES = 2,
};

enum SCHEDSTATE : uint32_t {
    SCHEDSTATE_ACTIVE = 0,
    SCHEDSTATE_CLOSED = 1,
    SCHEDSTATE_DESTROYED = 2,
    _UNIQUE_SYMBOL_SCHEDSTATE_96 = 0xFFFFFFFFFFFFFFFF,
};

struct MouseEventData {
    /* 0000 */ uint32_t m_mode;
    /* 0004 */ uint32_t m_button;
    /* 0008 */ uint32_t m_buttonState;
    /* 000C */ uint32_t m_metaKeyState;
    /* 0010 */ uint32_t m_flags;
    /* 0014 */ C2Vector m_mousePosition;
    /* 001C */ uint32_t m_wheelAmount;
    /* 0020 */ uint32_t m_eventTime;
};

enum EVENTID {
    EVENT_ID_0 = 0,
    EVENT_ID_CHAR = 1,
    EVENT_ID_FOCUS = 2,
    EVENT_ID_3 = 3,
    EVENT_ID_DESTROY = 4,
    EVENT_ID_UPDATE = 5,
    EVENT_ID_IDLE = 6,
    EVENT_ID_POLL = 7,
    EVENT_ID_INITIALIZE = 8,
    EVENT_ID_KEYDOWN = 9,
    EVENT_ID_KEYUP = 10,
    EVENT_ID_KEYDOWN_REPEATING = 11,
    EVENT_ID_MOUSEDOWN = 12,
    EVENT_ID_MOUSEMOVE = 13,
    EVENT_ID_MOUSEMOVE_RELATIVE = 14,
    EVENT_ID_MOUSEUP = 15,
    EVENT_ID_MOUSEMODE_CHANGED = 16,
    EVENT_ID_MOUSEWHEEL = 17,
    EVENT_ID_18 = 18,
    EVENT_ID_19 = 19,
    EVENT_ID_20 = 20,
    EVENT_ID_21 = 21,
    EVENT_ID_22 = 22,
    EVENT_ID_PAINT = 23,
    EVENT_ID_NET_DATA = 24,
    EVENT_ID_NET_CONNECT = 25,
    EVENT_ID_NET_DISCONNECT = 26,
    EVENT_ID_NET_CANTCONNECT = 27,
    EVENT_ID_NET_DESTROY = 28,
    EVENT_ID_NET_AUTH_CHALLENGE = 29,
    EVENT_ID_30 = 30,
    EVENT_ID_31 = 31,
    EVENT_ID_32 = 32,
    EVENT_ID_33 = 33,
    EVENT_ID_IME = 34,
    EVENT_ID_SIZE = 35,
    EVENTIDS = 36,

    // Unused 0.5.3 names/values; not mapped to the current 3.3.5 placeholder IDs.
    // EVENT_ID_CAPTURECHANGED = 0,
    // EVENT_ID_CLOSE = 3,
    // EVENT_ID_CONSOLE_INPUT = 23,
    // EVENT_ID_ENGINENET = 24,
    // EVENT_ID_BATTLENET = 25,
    // EVENT_ID_WOW_Q_IDLE = 26,
};

typedef int32_t (*EVENTHANDLERFUNC)(const void* data, void* param);
typedef int32_t (*EVENTHANDLERGUIDFUNC)(const void* data, uint64_t guid, void* param);

struct HEVENTCONTEXT__ {
    /* 0000 */ int32_t m_unused;
};

typedef HEVENTCONTEXT__* HEVENTCONTEXT;

struct EVENT_DATA_CHAR {
    /* 0000 */ int32_t m_ch;
    /* 0004 */ uint32_t m_metaKeyState;
    /* 0008 */ uint32_t m_repeat;
};

struct EVENT_DATA_FOCUS {
    /* 0000 */ int32_t m_focus;
};

struct EVENT_DATA_IDLE {
    /* 0000 */ float m_elapsedSec;
    /* 0004 */ uint32_t m_time;
};

struct EVENT_DATA_UPDATE {
    /* 0000 */ float m_elapsedSec;
    /* 0004 */ uint32_t m_time;
};

struct EVENT_DATA_TIMER {
    /* 0000 */ float m_timeout;
    /* 0004 */ uint32_t m_time;
};

struct EVENT_DATA_IME {
    /* 0000 */ int32_t m_action;
    /* 0004 */ int32_t unused_0004;
    /* 0008 */ int32_t m_param;
    /* 000C */ uint32_t m_codePage;
};

struct EVENT_DATA_JOYSTICK {
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ int32_t m_id;
    /* 0008 */ int32_t m_state;
    /* 000C */ float m_value;
    /* 0010 */ float m_angle;
    /* 0014 */ float m_x;
    /* 0018 */ float m_y;
    /* 001C */ uint32_t m_time;
};

struct EVENT_DATA_KEY {
    /* 0000 */ KEY m_key;
    /* 0004 */ uint32_t m_metaKeyState;
    /* 0008 */ uint32_t m_repeat;
    /* 000C */ uint32_t m_time;
};

struct EVENT_DATA_MOUSE {
    /* 0000 */ MOUSEMODE m_mode;
    /* 0004 */ MOUSEBUTTON m_button;
    /* 0008 */ uint32_t m_buttonState;
    /* 000C */ uint32_t m_metaKeyState;
    /* 0010 */ uint32_t m_flags;
    /* 0014 */ float m_x;
    /* 0018 */ float m_y;
    /* 001C */ int32_t m_wheelDistance;
    /* 0020 */ uint32_t m_time;
};

struct EVENT_DATA_SIZE {
    /* 0000 */ int32_t m_w;
    /* 0004 */ int32_t m_h;
};

#endif
