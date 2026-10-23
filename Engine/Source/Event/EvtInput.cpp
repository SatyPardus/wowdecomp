#include <cmath>
#include <cstdint>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Event/EvtContext.h"
#include "Engine/Source/Event/EvtInput.h"
#include "Engine/Source/Event/EvtQueue.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"

namespace Input {
    // STATIC: 0x00B41828
    MOUSEMODE s_mouseMode;

    // STATIC: 0x00B4182C
    uint32_t s_mouseHoldButton;

    // STATIC: 0x00B41830
    uint32_t s_metaKeyState;

    // STATIC: 0x00B41834
    uint32_t s_buttonState;

    // STATIC: 0x00B41840
    CRect s_boundingRect;
}

// STATIC: 0x00B41838
static int32_t (*s_confirmCloseCallback)(void*);

// STATIC: 0x00B4183C
static void* s_confirmCloseParam;

// VERIFY: 7f9ca0aa auto
// OFFSET: 0x0047F950
int32_t IEvtConfirmClose() {
    if (s_confirmCloseCallback) {
        return s_confirmCloseCallback(s_confirmCloseParam);
    }

    return 1;
}

// VERIFY: ee0e3e04 auto
// OFFSET: 0x0047F970
void PostString(EvtContext* context, const uint16_t* string, int32_t length) {
    EVENT_DATA_CHAR data;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_repeat = 1;

    for (int32_t i = 0; i < length; i++) {
        data.m_ch = string[i];
        IEvtQueueDispatch(context, EVENT_ID_CHAR, &data);
    }
}

// VERIFY: af7c407c auto
// OFFSET: 0x0047F9C0
void IEvtDispatchKeyDown(EvtContext* context, KEY key, uint32_t repeat, uint32_t time) {
    if (static_cast<int32_t>(key) <= KEY_LASTMETAKEY) {
        uint32_t bit = 1 << key;

        if (Input::s_metaKeyState & bit) {
            return;
        }

        Input::s_metaKeyState |= bit;
    }

    EVENT_DATA_KEY data;
    data.m_key = key;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_repeat = repeat;
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_KEYDOWN, &data);
}

// VERIFY: 685f8bc9 auto
// OFFSET: 0x0047FA10
void IEvtDispatchKeyUp(EvtContext* context, KEY key, uint32_t repeat, uint32_t time) {
    if (static_cast<int32_t>(key) <= KEY_LASTMETAKEY) {
        uint32_t bit = 1 << key;

        if (!(Input::s_metaKeyState & bit)) {
            return;
        }

        Input::s_metaKeyState &= ~bit;
    }

    EVENT_DATA_KEY data;
    data.m_key = key;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_repeat = repeat;
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_KEYUP, &data);
}

// VERIFY: 145efcf1 manual
// OFFSET: 0x0047FA60
void IEvtDispatchMouseMoveRelative(EvtContext* context, int32_t x, int32_t y, uint32_t time) {
    EVENT_DATA_MOUSE data;
    data.m_mode = Input::s_mouseMode;
    data.m_button = MOUSE_BUTTON_NONE;
    data.m_buttonState = Input::s_buttonState;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = Input::s_mouseMode == MOUSE_MODE_RELATIVE ? 2 : 0;
    data.m_x = static_cast<float>(x);
    data.m_y = static_cast<float>(y);
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_MOUSEMOVE_RELATIVE, &data);
}

// VERIFY: 9f76e0bb manual
// OFFSET: 0x0047FAC0
void PostMouseModeChanged(EvtContext* context, MOUSEMODE mode) {
    EVENT_DATA_MOUSE data = {};
    data.m_buttonState = Input::s_buttonState;

    Input::s_mouseMode = mode;

    data.m_mode = mode;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = mode == MOUSE_MODE_RELATIVE ? 2 : 0;

    IEvtQueueDispatch(context, EVENT_ID_MOUSEMODE_CHANGED, &data);
}

// VERIFY: 0ffd648b auto
// OFFSET: 0x0047FB30
void IEvtDispatchJoystickAxis(EvtContext* context, int32_t id, int32_t value, uint32_t time) {
    EVENT_DATA_JOYSTICK data;
    data.unused_0000 = 0;
    data.m_id = id;
    data.m_value = value < 0 ? -(value * -(1.0f / 32768.0f)) : value * (1.0f / 32767.0f);
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_19, &data);
}

// VERIFY: 6364fe70 auto
// OFFSET: 0x0047FB80
void IEvtInputDestroy() {
    OsInputDestroy();
    Input::s_buttonState = 0;
    Input::s_metaKeyState = 0;
}

// VERIFY: 38e83e13 auto
// OFFSET: 0x0047FBA0
void IEvtInputInitialize() {
    OsInputInitialize();
}

// VERIFY: b0467071 manual
// OFFSET: 0x0047FBB0
void IEvtInputSetMouseMode(EvtContext* context, MOUSEMODE mode, uint32_t holdButton) {
    if (!context) {
        DebugValidate("context");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    if (holdButton != (Input::s_buttonState & holdButton)) {
        return;
    }

    Input::s_mouseHoldButton = holdButton;

    if (mode != Input::s_mouseMode) {
        OsInputSetMouseMode(mode == MOUSE_MODE_NORMAL ? OS_MOUSE_MODE_NORMAL : OS_MOUSE_MODE_RELATIVE);
        PostMouseModeChanged(context, mode);
    }
}

// VERIFY: 3f343c5b auto
// OFFSET: 0x0047FC20
void IEvtInputSetConfirmCloseCallback(int32_t (*callback)(void*), void* param) {
    s_confirmCloseCallback = callback;
    s_confirmCloseParam = param;
}

// VERIFY: b4a84231 manual
// OFFSET: 0x0047FC90
void ConvertPosition(int32_t x, int32_t y, float* outX, float* outY) {
    if (Input::s_boundingRect.maxX - Input::s_boundingRect.minX != 0.0f && Input::s_boundingRect.maxY - Input::s_boundingRect.minY != 0.0f) {
        C2Vector point;
        point.x = static_cast<float>(x);
        point.y = static_cast<float>(y);

        if (!Input::s_boundingRect.ContainsPoint(point)) {
            float minX = Input::s_boundingRect.minX + 1.0f;
            float maxX = Input::s_boundingRect.maxX - 1.0f;
            float clampX = point.x > minX ? point.x : minX;
            clampX = clampX < maxX ? clampX : maxX;

            float minY = Input::s_boundingRect.minY + 1.0f;
            float maxY = Input::s_boundingRect.maxY - 1.0f;
            float clampY = point.y > minY ? point.y : minY;
            clampY = clampY < maxY ? clampY : maxY;

            x = static_cast<int32_t>(clampX);
            y = static_cast<int32_t>(clampY);

            OsInputSetMousePosition(x, y);
        }
    }

    tagRECT windowRect;
    OsGetDefaultWindowRect(&windowRect);

    *outX = static_cast<float>(x) / static_cast<float>(windowRect.right - windowRect.left);
    *outY = 1.0f - static_cast<float>(y) / static_cast<float>(windowRect.bottom - windowRect.top);
}

// VERIFY: bc9e2143 auto
// OFFSET: 0x0047FDE0
void IEvtDispatchFocusChanged(EvtContext* context, int32_t focus) {
    Input::s_buttonState = 0;
    Input::s_metaKeyState = 0;

    EVENT_DATA_FOCUS data;
    data.m_focus = focus;

    IEvtQueueDispatch(context, EVENT_ID_FOCUS, &data);

    if (Input::s_mouseHoldButton && Input::s_mouseHoldButton != (Input::s_buttonState & Input::s_mouseHoldButton)) {
        EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
    }
}

// VERIFY: c5e66729 manual
// OFFSET: 0x0047FE30
void IEvtDispatchMouseDown(EvtContext* context, MOUSEBUTTON button, int32_t x, int32_t y, uint32_t time) {
    Input::s_buttonState |= button;

    EVENT_DATA_MOUSE data;
    data.m_mode = Input::s_mouseMode;
    data.m_button = button;
    data.m_buttonState = Input::s_buttonState;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = Input::s_mouseMode == MOUSE_MODE_RELATIVE ? 2 : 0;
    ConvertPosition(x, y, &data.m_x, &data.m_y);
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_MOUSEDOWN, &data);
}

// VERIFY: 0759bbb3 manual
// OFFSET: 0x0047FEA0
void IEvtDispatchMouseMove(EvtContext* context, int32_t x, int32_t y, uint32_t time) {
    EVENT_DATA_MOUSE data;
    data.m_mode = Input::s_mouseMode;
    data.m_button = MOUSE_BUTTON_NONE;
    data.m_buttonState = Input::s_buttonState;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = Input::s_mouseMode == MOUSE_MODE_RELATIVE ? 2 : 0;
    data.m_time = time;
    ConvertPosition(x, y, &data.m_x, &data.m_y);

    IEvtQueueDispatch(context, EVENT_ID_MOUSEMOVE, &data);
}

// VERIFY: cd620aa8 manual
// OFFSET: 0x0047FF10
void IEvtDispatchMouseEvent(EvtContext* context, MOUSEBUTTON button, int32_t x, int32_t y, uint32_t flags, uint32_t time) {
    Input::s_buttonState &= ~button;

    EVENT_DATA_MOUSE data;
    data.m_mode = Input::s_mouseMode;
    data.m_button = button;
    data.m_buttonState = Input::s_buttonState;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = (Input::s_mouseMode == MOUSE_MODE_RELATIVE ? 2 : 0) | flags;
    ConvertPosition(x, y, &data.m_x, &data.m_y);
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_MOUSEUP, &data);

    if (Input::s_mouseHoldButton && Input::s_mouseHoldButton != (Input::s_buttonState & Input::s_mouseHoldButton)) {
        EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
    }
}

// VERIFY: 83c10de4 manual
// OFFSET: 0x0047FFB0
void IEvtDispatchMouseWheel(EvtContext* context, int32_t wheelDistance, int32_t x, int32_t y, uint32_t time) {
    EVENT_DATA_MOUSE data;
    data.m_mode = Input::s_mouseMode;
    data.m_button = MOUSE_BUTTON_NONE;
    data.m_buttonState = Input::s_buttonState;
    data.m_metaKeyState = Input::s_metaKeyState;
    data.m_flags = Input::s_mouseMode == MOUSE_MODE_RELATIVE ? 2 : 0;
    data.m_wheelDistance = wheelDistance;
    ConvertPosition(x, y, &data.m_x, &data.m_y);
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_MOUSEWHEEL, &data);
}

// VERIFY: 2eeedb0e auto
// OFFSET: 0x00480020
void IEvtDispatchJoystickStick(EvtContext* context, int32_t id, int32_t x, int32_t y, uint32_t time) {
    float stickX = x < 0 ? -(x * -(1.0f / 32768.0f)) : x * (1.0f / 32767.0f);
    float stickY = y < 0 ? -(y * -(1.0f / 32768.0f)) : y * (1.0f / 32767.0f);

    EVENT_DATA_JOYSTICK data;
    data.unused_0000 = 0;
    data.m_id = id;
    data.m_value = sqrtf(stickY * stickY + stickX * stickX);
    data.m_angle = 3.1415927f - atan2f(stickX, stickY);
    data.m_x = stickX;
    data.m_y = stickY;
    data.m_time = time;

    IEvtQueueDispatch(context, EVENT_ID_18, &data);
}

// VERIFY: 2eba42b1 auto
// OFFSET: 0x004800A0
void IEvtInputGetMousePosition(float* x, float* y) {
    int32_t mouseX;
    int32_t mouseY;
    OsInputGetMousePosition(&mouseX, &mouseY);

    float ndcX;
    float ndcY;
    ConvertPosition(mouseX, mouseY, &ndcX, &ndcY);

    NDCToDDC(ndcX, ndcY, x, y);
}

// VERIFY: 5707a931 auto
// OFFSET: 0x004800F0
void IEvtDispatchMouseEvents(EvtContext* context, int32_t x, int32_t y) {
    while (Input::s_buttonState) {
        uint32_t button = Input::s_buttonState & (Input::s_buttonState ^ (Input::s_buttonState - 1));
        IEvtDispatchMouseEvent(context, static_cast<MOUSEBUTTON>(button), x, y, 1, OsGetAsyncTimeMs());
    }
}

// VERIFY: f5f6a1bc auto
// OFFSET: 0x00480130
void IEvtDispatchInput(EvtContext* context, OSINPUT id, int32_t* param, int32_t* shutdown) {
    if (!context) {
        DebugValidate("context");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    switch (id) {
        case OS_INPUT_CAPTURE_CHANGED:
            IEvtDispatchMouseEvents(context, param[1], param[2]);
            break;

        case OS_INPUT_CHAR: {
            EVENT_DATA_CHAR data;
            data.m_ch = param[0];
            data.m_metaKeyState = Input::s_metaKeyState;
            data.m_repeat = param[1];

            IEvtQueueDispatch(context, EVENT_ID_CHAR, &data);
            break;
        }

        case OS_INPUT_STRING:
            PostString(context, reinterpret_cast<const uint16_t*>(static_cast<intptr_t>(param[0])), param[1]);
            break;

        case OS_INPUT_IME: {
            EVENT_DATA_IME data;
            data.m_action = param[0];
            data.unused_0004 = param[1];
            data.m_param = param[2];
            data.m_codePage = GetACP();

            IEvtQueueDispatch(context, EVENT_ID_IME, &data);
            break;
        }

        case OS_INPUT_SIZE: {
            EVENT_DATA_SIZE data;
            data.m_w = param[0];
            data.m_h = param[1];

            IEvtQueueDispatch(context, EVENT_ID_SIZE, &data);
            break;
        }

        case OS_INPUT_CLOSE:
            if (IEvtConfirmClose()) {
                EventInitiateShutdown();
                *shutdown = 1;
            }

            break;

        case OS_INPUT_FOCUS:
            IEvtDispatchFocusChanged(context, param[0]);
            break;

        case OS_INPUT_KEY_DOWN:
            IEvtDispatchKeyDown(context, static_cast<KEY>(param[0]), param[1], param[3]);
            break;

        case OS_INPUT_KEY_UP:
            IEvtDispatchKeyUp(context, static_cast<KEY>(param[0]), param[1], param[3]);
            break;

        case OS_INPUT_MOUSE_DOWN:
            IEvtDispatchMouseDown(context, static_cast<MOUSEBUTTON>(param[0]), param[1], param[2], param[3]);
            break;

        case OS_INPUT_MOUSE_MOVE:
            IEvtDispatchMouseMove(context, param[1], param[2], param[3]);
            break;

        case OS_INPUT_MOUSE_WHEEL:
            IEvtDispatchMouseWheel(context, param[0], param[1], param[2], param[3]);
            break;

        case OS_INPUT_MOUSE_MOVE_RELATIVE:
            IEvtDispatchMouseMoveRelative(context, param[1], param[2], param[3]);
            break;

        case OS_INPUT_MOUSE_UP:
            IEvtDispatchMouseEvent(context, static_cast<MOUSEBUTTON>(param[0]), param[1], param[2], 0, param[3]);
            break;

        case OS_INPUT_JOYSTICK_STICK:
            IEvtDispatchJoystickStick(context, param[0], param[1], param[2], param[3]);
            break;

        case OS_INPUT_JOYSTICK_AXIS:
            IEvtDispatchJoystickAxis(context, param[0], param[1], param[3]);
            break;

        case OS_INPUT_JOYSTICK_BUTTON_DOWN: {
            EVENT_DATA_JOYSTICK data;
            data.unused_0000 = 0;
            data.m_id = param[0];
            data.m_state = 1;
            data.m_time = param[3];

            IEvtQueueDispatch(context, EVENT_ID_20, &data);
            break;
        }

        case OS_INPUT_JOYSTICK_BUTTON_UP: {
            EVENT_DATA_JOYSTICK data;
            data.unused_0000 = 0;
            data.m_id = param[0];
            data.m_state = 0;
            data.m_time = param[3];

            IEvtQueueDispatch(context, EVENT_ID_21, &data);
            break;
        }

        case OS_INPUT_JOYSTICK_HAT: {
            EVENT_DATA_JOYSTICK data;
            data.unused_0000 = 0;
            data.m_id = param[0];
            data.m_state = param[1];
            data.m_time = param[3];

            IEvtQueueDispatch(context, EVENT_ID_22, &data);
            break;
        }

        case OS_INPUT_SHUTDOWN:
            if (IEvtConfirmClose()) {
                *shutdown = 1;
            }

            break;
    }
}

// VERIFY: 389086bc auto
// OFFSET: 0x00480410
int32_t IEvtInputProcess(EvtContext* context, int32_t* shutdown) {
    if (!context) {
        DebugValidate("context");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return 0;
    }

    int32_t result = 0;
    OSINPUT id;
    int32_t param[4];

    while (OsInputGet(&id, &param[0], &param[1], &param[2], &param[3])) {
        result = 1;
        IEvtDispatchInput(context, id, param, shutdown);
    }

    return result;
}
