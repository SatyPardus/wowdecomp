#if defined(__APPLE__)

#include <cstdint>
#include <SDL.h>
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Joystick.h"
#include "Engine/Source/Os/TimeManager.h"

void IOsIMEUpdateComposition(const char* text);
#include "Engine/Source/Storm/H/storm.h"

static OSEVENT s_queue[16];
static int32_t s_queueHead;
static int32_t s_queueTail;
static uint32_t s_osButtonState;
static OS_MOUSE_MODE s_osMouseMode;
static float s_mouseSpeed = 1.0f;
static int32_t s_appActive = 1;
static int32_t s_windowSize;
static tagRECT s_defaultWindowRect;
static void (*s_eventPollProc)();
static int32_t s_windowResizeLock;

void OsQueuePut(OSINPUT id, int32_t param0, int32_t param1, int32_t param2, int32_t param3) {
    (void)param3;
    int32_t next = s_queueTail == 15 ? 0 : s_queueTail + 1;
    if (next == s_queueHead) {
        s_queueHead = s_queueHead == 15 ? 0 : s_queueHead + 1;
    }
    OSEVENT& event = s_queue[s_queueTail];
    event.id = id;
    event.param[0] = param0;
    event.param[1] = param1;
    event.param[2] = param2;
    event.param[3] = OsGetAsyncTimeMs();
    s_queueTail = next;
}

int32_t OsQueueGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3) {
    if (s_queueHead == s_queueTail) {
        return 0;
    }
    OSEVENT& event = s_queue[s_queueHead];
    *id = event.id;
    *param0 = event.param[0];
    *param1 = event.param[1];
    *param2 = event.param[2];
    *param3 = event.param[3];
    s_queueHead = s_queueHead == 15 ? 0 : s_queueHead + 1;
    return 1;
}

static KEY KeyFromScancode(SDL_Scancode scancode, SDL_Keycode sym) {
    if (scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z) {
        return static_cast<KEY>(KEY_A + (scancode - SDL_SCANCODE_A));
    }
    if (scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9) {
        return static_cast<KEY>(KEY_1 + (scancode - SDL_SCANCODE_1));
    }
    if (scancode == SDL_SCANCODE_0) {
        return KEY_0;
    }
    if (scancode >= SDL_SCANCODE_F1 && scancode <= SDL_SCANCODE_F12) {
        return static_cast<KEY>(KEY_F1 + (scancode - SDL_SCANCODE_F1));
    }
    if (scancode >= SDL_SCANCODE_F13 && scancode <= SDL_SCANCODE_F19) {
        return static_cast<KEY>(KEY_F13 + (scancode - SDL_SCANCODE_F13));
    }
    if (scancode >= SDL_SCANCODE_KP_1 && scancode <= SDL_SCANCODE_KP_9) {
        return static_cast<KEY>(KEY_NUMPAD1 + (scancode - SDL_SCANCODE_KP_1));
    }

    switch (scancode) {
        case SDL_SCANCODE_LSHIFT:
            return KEY_LSHIFT;
        case SDL_SCANCODE_RSHIFT:
            return KEY_RSHIFT;
        case SDL_SCANCODE_LCTRL:
            return KEY_LCONTROL;
        case SDL_SCANCODE_RCTRL:
            return KEY_RCONTROL;
        case SDL_SCANCODE_LALT:
            return KEY_LALT;
        case SDL_SCANCODE_RALT:
            return KEY_RALT;
        case SDL_SCANCODE_SPACE:
            return KEY_SPACE;
        case SDL_SCANCODE_KP_0:
            return KEY_NUMPAD0;
        case SDL_SCANCODE_KP_PLUS:
            return KEY_NUMPAD_PLUS;
        case SDL_SCANCODE_KP_MINUS:
            return KEY_NUMPAD_MINUS;
        case SDL_SCANCODE_KP_MULTIPLY:
            return KEY_NUMPAD_MULTIPLY;
        case SDL_SCANCODE_KP_DIVIDE:
            return KEY_NUMPAD_DIVIDE;
        case SDL_SCANCODE_KP_PERIOD:
            return KEY_NUMPAD_DECIMAL;
        case SDL_SCANCODE_KP_EQUALS:
            return KEY_NUMPAD_EQUALS;
        case SDL_SCANCODE_EQUALS:
            return KEY_PLUS;
        case SDL_SCANCODE_MINUS:
            return KEY_MINUS;
        case SDL_SCANCODE_LEFTBRACKET:
            return KEY_BRACKET_OPEN;
        case SDL_SCANCODE_RIGHTBRACKET:
            return KEY_BRACKET_CLOSE;
        case SDL_SCANCODE_SLASH:
            return KEY_SLASH;
        case SDL_SCANCODE_BACKSLASH:
            return KEY_BACKSLASH;
        case SDL_SCANCODE_SEMICOLON:
            return KEY_SEMICOLON;
        case SDL_SCANCODE_APOSTROPHE:
            return KEY_APOSTROPHE;
        case SDL_SCANCODE_COMMA:
            return KEY_COMMA;
        case SDL_SCANCODE_PERIOD:
            return KEY_PERIOD;
        case SDL_SCANCODE_GRAVE:
            return KEY_TILDE;
        case SDL_SCANCODE_ESCAPE:
            return KEY_ESCAPE;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            return KEY_ENTER;
        case SDL_SCANCODE_BACKSPACE:
            return KEY_BACKSPACE;
        case SDL_SCANCODE_TAB:
            return KEY_TAB;
        case SDL_SCANCODE_LEFT:
            return KEY_LEFT;
        case SDL_SCANCODE_UP:
            return KEY_UP;
        case SDL_SCANCODE_RIGHT:
            return KEY_RIGHT;
        case SDL_SCANCODE_DOWN:
            return KEY_DOWN;
        case SDL_SCANCODE_INSERT:
            return KEY_INSERT;
        case SDL_SCANCODE_DELETE:
            return KEY_DELETE;
        case SDL_SCANCODE_HOME:
            return KEY_HOME;
        case SDL_SCANCODE_END:
            return KEY_END;
        case SDL_SCANCODE_PAGEUP:
            return KEY_PAGEUP;
        case SDL_SCANCODE_PAGEDOWN:
            return KEY_PAGEDOWN;
        case SDL_SCANCODE_CAPSLOCK:
            return KEY_CAPSLOCK;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            return KEY_NUMLOCK;
        case SDL_SCANCODE_SCROLLLOCK:
            return KEY_SCROLLLOCK;
        case SDL_SCANCODE_PAUSE:
            return KEY_PAUSE;
        case SDL_SCANCODE_PRINTSCREEN:
            return KEY_PRINTSCREEN;
        default:
            break;
    }

    if (sym >= 0 && sym <= 0xFF) {
        return static_cast<KEY>(sym);
    }
    return KEY_NONE;
}

static SDL_Scancode ScancodeFromKey(KEY key) {
    if (key >= KEY_A && key <= KEY_Z) {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - KEY_A));
    }
    if (key >= KEY_1 && key <= KEY_9) {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_1 + (key - KEY_1));
    }
    if (key >= KEY_F1 && key <= KEY_F12) {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_F1 + (key - KEY_F1));
    }
    if (key >= KEY_F13 && key <= KEY_F19) {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_F13 + (key - KEY_F13));
    }
    if (key >= KEY_NUMPAD1 && key <= KEY_NUMPAD9) {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_KP_1 + (key - KEY_NUMPAD1));
    }

    switch (key) {
        case KEY_0:
            return SDL_SCANCODE_0;
        case KEY_LSHIFT:
            return SDL_SCANCODE_LSHIFT;
        case KEY_RSHIFT:
            return SDL_SCANCODE_RSHIFT;
        case KEY_LCONTROL:
            return SDL_SCANCODE_LCTRL;
        case KEY_RCONTROL:
            return SDL_SCANCODE_RCTRL;
        case KEY_LALT:
            return SDL_SCANCODE_LALT;
        case KEY_RALT:
            return SDL_SCANCODE_RALT;
        case KEY_SPACE:
            return SDL_SCANCODE_SPACE;
        case KEY_NUMPAD0:
            return SDL_SCANCODE_KP_0;
        case KEY_NUMPAD_PLUS:
            return SDL_SCANCODE_KP_PLUS;
        case KEY_NUMPAD_MINUS:
            return SDL_SCANCODE_KP_MINUS;
        case KEY_NUMPAD_MULTIPLY:
            return SDL_SCANCODE_KP_MULTIPLY;
        case KEY_NUMPAD_DIVIDE:
            return SDL_SCANCODE_KP_DIVIDE;
        case KEY_NUMPAD_DECIMAL:
            return SDL_SCANCODE_KP_PERIOD;
        case KEY_NUMPAD_EQUALS:
            return SDL_SCANCODE_KP_EQUALS;
        case KEY_PLUS:
            return SDL_SCANCODE_EQUALS;
        case KEY_MINUS:
            return SDL_SCANCODE_MINUS;
        case KEY_BRACKET_OPEN:
            return SDL_SCANCODE_LEFTBRACKET;
        case KEY_BRACKET_CLOSE:
            return SDL_SCANCODE_RIGHTBRACKET;
        case KEY_SLASH:
            return SDL_SCANCODE_SLASH;
        case KEY_BACKSLASH:
            return SDL_SCANCODE_BACKSLASH;
        case KEY_SEMICOLON:
            return SDL_SCANCODE_SEMICOLON;
        case KEY_APOSTROPHE:
            return SDL_SCANCODE_APOSTROPHE;
        case KEY_COMMA:
            return SDL_SCANCODE_COMMA;
        case KEY_PERIOD:
            return SDL_SCANCODE_PERIOD;
        case KEY_TILDE:
            return SDL_SCANCODE_GRAVE;
        case KEY_ESCAPE:
            return SDL_SCANCODE_ESCAPE;
        case KEY_ENTER:
            return SDL_SCANCODE_RETURN;
        case KEY_BACKSPACE:
            return SDL_SCANCODE_BACKSPACE;
        case KEY_TAB:
            return SDL_SCANCODE_TAB;
        case KEY_LEFT:
            return SDL_SCANCODE_LEFT;
        case KEY_UP:
            return SDL_SCANCODE_UP;
        case KEY_RIGHT:
            return SDL_SCANCODE_RIGHT;
        case KEY_DOWN:
            return SDL_SCANCODE_DOWN;
        case KEY_INSERT:
            return SDL_SCANCODE_INSERT;
        case KEY_DELETE:
            return SDL_SCANCODE_DELETE;
        case KEY_HOME:
            return SDL_SCANCODE_HOME;
        case KEY_END:
            return SDL_SCANCODE_END;
        case KEY_PAGEUP:
            return SDL_SCANCODE_PAGEUP;
        case KEY_PAGEDOWN:
            return SDL_SCANCODE_PAGEDOWN;
        case KEY_CAPSLOCK:
            return SDL_SCANCODE_CAPSLOCK;
        case KEY_NUMLOCK:
            return SDL_SCANCODE_NUMLOCKCLEAR;
        case KEY_SCROLLLOCK:
            return SDL_SCANCODE_SCROLLLOCK;
        case KEY_PAUSE:
            return SDL_SCANCODE_PAUSE;
        case KEY_PRINTSCREEN:
            return SDL_SCANCODE_PRINTSCREEN;
        default:
            return SDL_SCANCODE_UNKNOWN;
    }
}

static MOUSEBUTTON MouseButtonFromSDL(uint8_t button) {
    switch (button) {
        case SDL_BUTTON_LEFT:
            return MOUSE_BUTTON_LEFT;
        case SDL_BUTTON_MIDDLE:
            return MOUSE_BUTTON_MIDDLE;
        case SDL_BUTTON_RIGHT:
            return MOUSE_BUTTON_RIGHT;
        case SDL_BUTTON_X1:
            return MOUSE_BUTTON_XBUTTON1;
        case SDL_BUTTON_X2:
            return MOUSE_BUTTON_XBUTTON2;
        default:
            if (button >= 6 && button < 18) {
                return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON3 << (button - 6));
            }
            return MOUSE_BUTTON_NONE;
    }
}

static uint32_t DecodeUtf8(const char*& p) {
    const unsigned char* s = reinterpret_cast<const unsigned char*>(p);
    if (s[0] < 0x80) {
        p += 1;
        return s[0];
    }
    if ((s[0] & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) {
        p += 2;
        return ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
    }
    if ((s[0] & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) {
        p += 3;
        return ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
    }
    if ((s[0] & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) {
        p += 4;
        return ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
    }
    p += 1;
    return 0xFFFD;
}

void OsInputSetWindowResizeLock(int32_t resizeLock) {
    s_windowResizeLock = resizeLock;
}

float OsInputGetMouseSpeed() {
    return s_mouseSpeed;
}

int32_t OsInputIsKeyDown(KEY key) {
    SDL_PumpEvents();
    SDL_Scancode scancode = ScancodeFromKey(key);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return 0;
    }
    const uint8_t* state = SDL_GetKeyboardState(nullptr);
    return state[scancode] != 0;
}

int32_t OsInputIsMouseButtonDown(MOUSEBUTTON button) {
    uint32_t state = SDL_GetMouseState(nullptr, nullptr);
    switch (button) {
        case MOUSE_BUTTON_LEFT:
            return (state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        case MOUSE_BUTTON_MIDDLE:
            return (state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
        case MOUSE_BUTTON_RIGHT:
            return (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        case MOUSE_BUTTON_XBUTTON1:
            return (state & SDL_BUTTON(SDL_BUTTON_X1)) != 0;
        case MOUSE_BUTTON_XBUTTON2:
            return (state & SDL_BUTTON(SDL_BUTTON_X2)) != 0;
        default:
            return (s_osButtonState & button) != 0;
    }
}

void OsInputInitialize() {
    s_queueHead = s_queueTail = 0;
    s_osButtonState = 0;
    s_osMouseMode = OS_MOUSE_MODE_NORMAL;
    s_appActive = 1;
    SDL_StartTextInput();
}

void OsInputDestroy() {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_StopTextInput();
}

void OsInputSetEventPollProc(void (*pollProc)()) {
    s_eventPollProc = pollProc;
}

static void QueueWindowSize(SDL_Window* window, int width, int height) {
    if (s_windowResizeLock) {
        return;
    }
    s_defaultWindowRect.left = 0;
    s_defaultWindowRect.top = 0;
    s_defaultWindowRect.right = width;
    s_defaultWindowRect.bottom = height;
    s_windowSize = 0;
    OsQueuePut(OS_INPUT_SIZE, width, height, 0, 0);
}

static void PumpEvent(const SDL_Event& event) {
    switch (event.type) {
        case SDL_QUIT:
            OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
            break;
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            KEY key = KeyFromScancode(event.key.keysym.scancode, event.key.keysym.sym);
            if (key != KEY_NONE) {
                OsQueuePut(event.type == SDL_KEYDOWN ? OS_INPUT_KEY_DOWN : OS_INPUT_KEY_UP, static_cast<int32_t>(key), event.key.repeat ? 1 : 0, 0, 0);
            }
            break;
        }
        case SDL_TEXTINPUT: {
            IOsIMEUpdateComposition("");
            const char* p = event.text.text;
            while (*p) {
                uint32_t ch = DecodeUtf8(p);
                if (ch >= 0x20) {
                    OsQueuePut(OS_INPUT_CHAR, static_cast<int32_t>(ch), 1, 0, 0);
                }
            }
            break;
        }
        case SDL_TEXTEDITING:
            IOsIMEUpdateComposition(event.edit.text);
            OsQueuePut(OS_INPUT_IME, 2, 0, 2, 0);
            break;
        case SDL_MOUSEMOTION:
            if (s_osMouseMode == OS_MOUSE_MODE_RELATIVE) {
                if (event.motion.xrel || event.motion.yrel) {
                    OsQueuePut(OS_INPUT_MOUSE_MOVE_RELATIVE, 0, event.motion.xrel, event.motion.yrel, 0);
                }
            } else {
                OsQueuePut(OS_INPUT_MOUSE_MOVE, 0, event.motion.x, event.motion.y, 0);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            MOUSEBUTTON button = MouseButtonFromSDL(event.button.button);
            if (button == MOUSE_BUTTON_NONE) {
                break;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                s_osButtonState |= button;
            } else {
                s_osButtonState &= ~button;
            }
            OsQueuePut(event.type == SDL_MOUSEBUTTONDOWN ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, static_cast<int32_t>(button), event.button.x, event.button.y, 0);
            break;
        }
        case SDL_MOUSEWHEEL: {
            int x = 0;
            int y = 0;
            SDL_GetMouseState(&x, &y);
            int amount = event.wheel.y * 120;
            if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                amount = -amount;
            }
            OsQueuePut(OS_INPUT_MOUSE_WHEEL, amount, x, y, 0);
            break;
        }
        case SDL_WINDOWEVENT:
            switch (event.window.event) {
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                case SDL_WINDOWEVENT_RESIZED:
                    QueueWindowSize(SDL_GetWindowFromID(event.window.windowID), event.window.data1, event.window.data2);
                    break;
                case SDL_WINDOWEVENT_CLOSE:
                    OsQueuePut(OS_INPUT_CLOSE, 0, 0, 0, 0);
                    break;
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    s_appActive = 1;
                    OsQueuePut(OS_INPUT_FOCUS, 1, 0, 0, 0);
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    s_appActive = 0;
                    s_osButtonState = 0;
                    OsQueuePut(OS_INPUT_FOCUS, 0, 0, 0, 0);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

int32_t OsInputGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3) {
    if (!id || !param0 || !param1 || !param2 || !param3) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    *id = static_cast<OSINPUT>(-1);

    // Unlike Win64, do not call s_eventPollProc here: Windows only runs it from a timer during
    // modal non-client loops. Calling it on every poll re-enters idle processing (including
    // PollNet) while the client is still loading, e.g. from UpdateProgressBar.
    if (s_queueHead == s_queueTail) {
        OsPollJoysticks(s_appActive);
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            PumpEvent(event);
            if (s_queueHead != s_queueTail) {
                break;
            }
        }
    }

    return OsQueueGet(id, param0, param1, param2, param3);
}

void OsInputSetMouseMode(OS_MOUSE_MODE mode) {
    if (mode >= OS_MOUSE_MODES) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }
    if (mode == s_osMouseMode) {
        return;
    }
    s_osMouseMode = mode;
    SDL_SetRelativeMouseMode(mode == OS_MOUSE_MODE_RELATIVE ? SDL_TRUE : SDL_FALSE);
}

void OsInputSetMouseSpeed(float speed) {
    if (speed < 0.1f) {
        speed = 0.1f;
    } else if (speed >= 2.0f) {
        speed = 2.0f;
    }
    s_mouseSpeed = speed;
}

void OsInputGetMousePosition(int32_t* x, int32_t* y) {
    int mx = 0;
    int my = 0;
    SDL_GetMouseState(&mx, &my);
    if (x) {
        *x = mx;
    }
    if (y) {
        *y = my;
    }
}

void OsInputSetMousePosition(int32_t x, int32_t y) {
    SDL_Window* window = static_cast<SDL_Window*>(OsGuiGetWindow(0));
    SDL_WarpMouseInWindow(window, x, y);
}

int32_t OsGetDefaultWindowRect(tagRECT* rect) {
    if (!rect) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!s_defaultWindowRect.right || !s_defaultWindowRect.bottom) {
        SDL_Window* window = static_cast<SDL_Window*>(OsGuiGetWindow(0));
        int width = 0;
        int height = 0;
        if (window) {
            SDL_GetWindowSize(window, &width, &height);
        }
        s_defaultWindowRect.left = 0;
        s_defaultWindowRect.top = 0;
        s_defaultWindowRect.right = width;
        s_defaultWindowRect.bottom = height;
    }
    *rect = s_defaultWindowRect;
    return 1;
}

intptr_t OsWindowProc(void* window, uint32_t message, uintptr_t wparam, intptr_t lparam) {
    (void)window;
    (void)message;
    (void)wparam;
    (void)lparam;
    return 0;
}

#endif
