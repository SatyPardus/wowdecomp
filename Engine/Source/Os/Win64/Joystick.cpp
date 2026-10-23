#if defined(WOWDECOMP_ARCH_64)

#include <cmath>
#include <cstdint>
#include <cstring>
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include "Engine/Source/Base/Clamp.h"
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Joystick.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C2Vector.h"

struct W32JoystickRange {
    /* 0000 */ LONG m_min;
    /* 0004 */ LONG m_max;
};

struct W32JoystickStick {
    /* 0000 */ int32_t m_xAxis;
    /* 0004 */ int32_t m_yAxis;
};

struct W32JoystickHat {
    /* 0000 */ bool m_enabled;
    /* 0004 */ int32_t m_buttons[4];
};

struct W32JoystickMouseMode {
    /* 0000 */ int32_t m_enabled;
    /* 0004 */ int32_t m_stick;
    /* 0008 */ float m_speed;
    /* 000C */ int32_t m_leftButton;
    /* 0010 */ int32_t m_rightButton;
    /* 0014 */ uint32_t m_lastTime;
    /* 0018 */ C2Vector m_accumulated;
};

struct W32Joystick {
    /* 0000 */ DIDEVICEINSTANCEA m_instance;
    /* 0244 */ IDirectInputDevice8A* m_device;
    /* 0248 */ bool m_buffered;
    /* 0249 */ bool m_relative;
    /* 024C */ LONG m_lastAxes[8];
    /* 026C */ uint32_t m_axisMask;
    /* 0270 */ W32JoystickRange m_ranges[8];
    /* 02B0 */ uint32_t m_buttonCount;
    /* 02B4 */ uint32_t m_sliderCount;
    /* 02B8 */ uint32_t m_povCount;
    /* 02BC */ W32JoystickStick m_sticks[4];
    /* 02DC */ bool m_stickAxis[8];
    /* 02E4 */ bool m_sliderAxis[8];
    /* 02EC */ W32JoystickHat m_hats[4];
    /* 033C */ bool m_hatButton[32];
    /* 035C */ W32JoystickMouseMode m_mouseMode;
    /* 037C */ uint32_t m_stateIndex;
    /* 0380 */ DIJOYSTATE m_states[2];
    /* 0420 */ int32_t m_axes[2][8];
};

// STATIC: 0x00B1D4B4
static const char* s_relativeDevices[] = {
    "SpaceNavigator",
    "SpaceNavigator for Notebooks",
    "SpaceTraveler USB",
    "SpaceExplorer",
    "SpacePilot",
};

// STATIC: 0x00B1D4C8
static W32JoystickMouseMode s_defaultMouseMode = { 1, 0, 1.5f, 0, 1, 0 };

// STATIC: 0x00D41890
static IDirectInput8A* s_directInput;

// STATIC: 0x00D41894
static TSGrowableArray<W32Joystick> s_joysticks;

// OFFSET: 0x00870060
static void ResetState(W32Joystick* joystick, DIJOYSTATE* state) {
    LONG* axes = &state->lX;

    for (int32_t i = 0; i < 8; i++) {
        if (joystick->m_axisMask & (1 << i)) {
            axes[i] = joystick->m_ranges[i].m_min + (joystick->m_ranges[i].m_max - joystick->m_ranges[i].m_min) / 2;
        }
    }

    memset(state->rgbButtons, 0, sizeof(state->rgbButtons));

    for (int32_t i = 0; i < 4; i++) {
        state->rgdwPOV[i] = 0xFFFFFFFF;
    }
}

// OFFSET: 0x00870240
static BOOL CALLBACK EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCEA object, LPVOID ref) {
    W32Joystick* joystick = static_cast<W32Joystick*>(ref);
    int32_t axis = -1;

    if (object->guidType == GUID_XAxis) {
        axis = 0;
    } else if (object->guidType == GUID_YAxis) {
        axis = 1;
    } else if (object->guidType == GUID_ZAxis) {
        axis = 2;
    } else if (object->guidType == GUID_RxAxis) {
        axis = 3;
    } else if (object->guidType == GUID_RyAxis) {
        axis = 4;
    } else if (object->guidType == GUID_RzAxis) {
        axis = 5;
    } else if (object->guidType == GUID_Slider) {
        if (joystick->m_sliderCount < 2) {
            axis = 6 + joystick->m_sliderCount++;
        }
    } else if (object->guidType == GUID_Button) {
        joystick->m_buttonCount++;
    } else if (object->guidType == GUID_POV) {
        joystick->m_povCount++;
    }

    if (axis < 0) {
        return DIENUM_CONTINUE;
    }

    DIPROPRANGE range;
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwObj = object->dwType;
    range.diph.dwHow = DIPH_BYID;

    if ((object->dwType & DIDFT_AXIS) && joystick->m_device->GetProperty(DIPROP_RANGE, &range.diph) >= 0 && range.lMax > range.lMin) {
        joystick->m_axisMask |= 1 << axis;

        if (joystick->m_relative) {
            joystick->m_ranges[axis].m_min = -300;
            joystick->m_ranges[axis].m_max = 300;
        } else {
            joystick->m_ranges[axis].m_min = range.lMin;
            joystick->m_ranges[axis].m_max = range.lMax;
        }
    }

    return DIENUM_CONTINUE;
}

// OFFSET: 0x00870420
void OsJoystickMapStick(OsJoystickID index, int32_t stick, int32_t xAxis, int32_t yAxis) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (joystick->m_device && stick >= 0 && stick < 4 && xAxis >= 0 && xAxis < 8 && (joystick->m_axisMask & (1 << xAxis)) && yAxis >= 0 && yAxis < 8 && (joystick->m_axisMask & (1 << yAxis))) {
        joystick->m_sticks[stick].m_xAxis = xAxis;
        joystick->m_sticks[stick].m_yAxis = yAxis;
        joystick->m_stickAxis[xAxis] = true;
        joystick->m_stickAxis[yAxis] = true;
    }
}

// OFFSET: 0x008705C0
static BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCEA instance, LPVOID ref) {
    W32Joystick* joystick = s_joysticks.New();
    memset(joystick, 0, sizeof(W32Joystick));
    joystick->m_instance = *instance;

    for (int32_t i = 0; i < 4; i++) {
        joystick->m_sticks[i].m_xAxis = -1;
        joystick->m_sticks[i].m_yAxis = -1;
    }

    for (uint32_t i = 0; i < sizeof(s_relativeDevices) / sizeof(s_relativeDevices[0]); i++) {
        if (!SStrCmpI(instance->tszInstanceName, s_relativeDevices[i], 0x7FFFFFFF)) {
            joystick->m_relative = true;
            break;
        }
    }

    joystick->m_mouseMode = s_defaultMouseMode;

    OsJoystickMapStick(s_joysticks.Count() - 1, 0, 0, 1);

    return DIENUM_CONTINUE;
}

// OFFSET: 0x008706E0
uint32_t OsJoystickCount() {
    if (!s_joysticks.Count()) {
        if (DirectInput8Create(GetModuleHandleA(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8A, reinterpret_cast<void**>(&s_directInput), nullptr) < 0) {
            return 0;
        }

        s_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, nullptr, DIEDFL_ATTACHEDONLY);
    }

    return s_joysticks.Count();
}

// OFFSET: 0x00870730
OsJoystickID OsJoystickOpen(OsJoystickID index) {
    if (index >= OsJoystickCount()) {
        return 0xFFFFFFFF;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (s_directInput->CreateDevice(joystick->m_instance.guidInstance, &joystick->m_device, nullptr) < 0) {
        return 0xFFFFFFFF;
    }

    if (joystick->m_device->SetDataFormat(&c_dfDIJoystick) >= 0 && joystick->m_device->SetCooperativeLevel(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), DISCL_EXCLUSIVE | DISCL_FOREGROUND) >= 0 && joystick->m_device->EnumObjects(EnumObjectsCallback, joystick, DIDFT_ALL) >= 0) {
        DIPROPDWORD bufferSize;
        bufferSize.diph.dwSize = sizeof(DIPROPDWORD);
        bufferSize.diph.dwHeaderSize = sizeof(DIPROPHEADER);
        bufferSize.diph.dwObj = 0;
        bufferSize.diph.dwHow = DIPH_DEVICE;
        bufferSize.dwData = 256;

        HRESULT result = joystick->m_device->SetProperty(DIPROP_BUFFERSIZE, &bufferSize.diph);

        if (result >= 0) {
            joystick->m_buffered = true;
        }

        if (result >= 0 || result == DI_POLLEDDEVICE) {
            ResetState(joystick, &joystick->m_states[0]);
            return index;
        }
    }

    joystick->m_device->Release();
    joystick->m_device = nullptr;

    return 0xFFFFFFFF;
}

// OFFSET: 0x00870840
void OsJoystickClose(OsJoystickID index) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (joystick->m_device) {
        joystick->m_device->Release();
        joystick->m_device = nullptr;
    }
}

// OFFSET: 0x00870890
char* OsJoystickGetName(OsJoystickID index, char* buffer, uint32_t size) {
    if (index >= OsJoystickCount()) {
        return nullptr;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (!joystick->m_device) {
        return nullptr;
    }

    SStrCopy(buffer, joystick->m_instance.tszInstanceName, size);

    return buffer;
}

// OFFSET: 0x008708D0
void OsJoystickMapSlider(OsJoystickID index, int32_t axis) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (joystick->m_device && axis >= 0 && axis < 8) {
        joystick->m_sliderAxis[axis] = true;
    }
}

// OFFSET: 0x00870910
void OsJoystickMapHat(OsJoystickID index, int32_t hat, int32_t up, int32_t right, int32_t down, int32_t left) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (joystick->m_device && hat >= 0 && hat < 4 && up >= 0 && up < 32 && right >= 0 && right < 32 && down >= 0 && down < 32 && left >= 0 && left < 32) {
        joystick->m_hats[hat].m_buttons[0] = up;
        joystick->m_hats[hat].m_buttons[1] = right;
        joystick->m_hats[hat].m_buttons[3] = left;
        joystick->m_hats[hat].m_enabled = true;
        joystick->m_hats[hat].m_buttons[2] = down;
        joystick->m_hatButton[up] = true;
        joystick->m_hatButton[right] = true;
        joystick->m_hatButton[down] = true;
        joystick->m_hatButton[left] = true;
    }
}

// OFFSET: 0x008709E0
void OsJoystickSetMouseMode(OsJoystickID index, int32_t enable) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (!joystick->m_device) {
        return;
    }

    W32JoystickMouseMode* mouseMode = &joystick->m_mouseMode;

    if (enable == mouseMode->m_enabled) {
        return;
    }

    mouseMode->m_enabled = enable;
    mouseMode->m_lastTime = 0;
    mouseMode->m_accumulated.x = 0.0f;
    mouseMode->m_accumulated.y = 0.0f;

    if (!enable) {
        return;
    }

    DIJOYSTATE* state = &joystick->m_states[joystick->m_stateIndex];
    int32_t* axes = joystick->m_axes[joystick->m_stateIndex];
    W32JoystickStick* stick = &joystick->m_sticks[mouseMode->m_stick];

    if (axes[stick->m_xAxis] || axes[stick->m_yAxis]) {
        OsQueuePut(OS_INPUT_JOYSTICK_STICK, mouseMode->m_stick, 0, 0, 0);
    }

    int32_t x;
    int32_t y;

    if (state->rgbButtons[mouseMode->m_leftButton]) {
        OsInputGetMousePosition(&x, &y);
        OsQueuePut(OS_INPUT_MOUSE_UP, MOUSE_BUTTON_LEFT, x, y, 0);
    }

    if (state->rgbButtons[mouseMode->m_rightButton]) {
        OsInputGetMousePosition(&x, &y);
        OsQueuePut(OS_INPUT_MOUSE_UP, MOUSE_BUTTON_RIGHT, x, y, 0);
    }
}

// OFFSET: 0x00870B00
void OsJoystickConfigureMouseMode(OsJoystickID index, int32_t stick, int32_t leftButton, int32_t rightButton, float speed) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (joystick->m_device && stick >= 0 && stick < 4 && joystick->m_sticks[stick].m_xAxis >= 0 && joystick->m_sticks[stick].m_yAxis >= 0) {
        joystick->m_mouseMode.m_speed = speed;
        joystick->m_mouseMode.m_stick = stick;
        joystick->m_mouseMode.m_leftButton = leftButton;
        joystick->m_mouseMode.m_rightButton = rightButton;
    }
}

// OFFSET: 0x00870B80
static void PollJoystick(uint32_t index, int32_t active) {
    if (index >= OsJoystickCount()) {
        return;
    }

    W32Joystick* joystick = &s_joysticks[index];

    if (!joystick->m_device) {
        return;
    }

    uint32_t previous = joystick->m_stateIndex;
    uint32_t current = previous == 0;
    joystick->m_stateIndex = current;

    DIJOYSTATE* previousState = &joystick->m_states[previous];
    DIJOYSTATE* state = &joystick->m_states[current];
    int32_t* previousAxes = joystick->m_axes[previous];
    int32_t* axes = joystick->m_axes[current];
    uint32_t bufferedButtons = 0;
    HRESULT result = -1;

    if (active) {
        bool polled = joystick->m_device->Poll() >= 0;

        if (!polled) {
            HRESULT acquired = joystick->m_device->Acquire();

            if (acquired == DIERR_OTHERAPPHASPRIO) {
                acquired = joystick->m_device->SetCooperativeLevel(static_cast<HWND>(OsGuiGetWindow(OSGUI_WINDOW_TYPE_GX)), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
            }

            polled = acquired >= 0 && joystick->m_device->Poll() >= 0;
        }

        if (polled) {
            if (joystick->m_buffered) {
                DIDEVICEOBJECTDATA data[256];
                DWORD count = 256;
                result = joystick->m_device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), data, &count, 0);

                if (result >= 0) {
                    *state = *previousState;

                    for (DWORD i = 0; i < count; i++) {
                        DWORD offset = data[i].dwOfs;

                        if (offset >= DIJOFS_POV(0)) {
                            if (offset >= DIJOFS_BUTTON0) {
                                reinterpret_cast<BYTE*>(state)[offset] = static_cast<BYTE>(data[i].dwData);
                                bufferedButtons |= 1 << (offset - DIJOFS_BUTTON0);
                            } else {
                                state->rgdwPOV[(offset - DIJOFS_POV(0)) >> 2] = data[i].dwData;
                            }
                        } else {
                            uint32_t axis = offset >> 2;
                            LONG value = data[i].dwData;

                            if (joystick->m_relative) {
                                LONG delta = value - joystick->m_lastAxes[axis];
                                joystick->m_lastAxes[axis] = value;
                                (&state->lX)[axis] = Clamp(delta, joystick->m_ranges[axis].m_min, joystick->m_ranges[axis].m_max);
                            } else {
                                (&state->lX)[axis] = value;
                            }
                        }
                    }
                }
            } else {
                result = joystick->m_device->GetDeviceState(sizeof(DIJOYSTATE), state);

                if (joystick->m_relative && result >= 0) {
                    for (int32_t axis = 0; axis < 8; axis++) {
                        if (joystick->m_axisMask & (1 << axis)) {
                            LONG value = (&state->lX)[axis];
                            LONG delta = value - joystick->m_lastAxes[axis];
                            joystick->m_lastAxes[axis] = value;
                            (&state->lX)[axis] = Clamp(delta, joystick->m_ranges[axis].m_min, joystick->m_ranges[axis].m_max);
                        }
                    }
                }
            }
        }
    }

    if (result < 0) {
        ResetState(joystick, state);
    }

    const LONG* values = &state->lX;

    for (int32_t i = 0; i < 4; i++) {
        int32_t xAxis = joystick->m_sticks[i].m_xAxis;
        int32_t yAxis = joystick->m_sticks[i].m_yAxis;

        if (xAxis >= 0 && yAxis >= 0) {
            float x = ((static_cast<float>(values[xAxis]) - joystick->m_ranges[xAxis].m_min) / (joystick->m_ranges[xAxis].m_max - joystick->m_ranges[xAxis].m_min) - 0.5f) * 2.0f;
            float y = ((static_cast<float>(values[yAxis]) - joystick->m_ranges[yAxis].m_min) / (joystick->m_ranges[yAxis].m_max - joystick->m_ranges[yAxis].m_min) - 0.5f) * 2.0f;
            float length = sqrtf(y * y + x * x);

            if (length <= 0.15f) {
                axes[xAxis] = 0;
                axes[yAxis] = 0;
            } else {
                float scale = (length - 0.15f) / (length - length * 0.15f);
                axes[xAxis] = static_cast<int32_t>(x * scale * 32767.0f);
                axes[yAxis] = static_cast<int32_t>(scale * y * 32767.0f);
            }
        }
    }

    for (int32_t axis = 0; axis < 8; axis++) {
        if ((joystick->m_axisMask & (1 << axis)) && !joystick->m_stickAxis[axis]) {
            float value = (static_cast<float>(values[axis]) - joystick->m_ranges[axis].m_min) / (joystick->m_ranges[axis].m_max - joystick->m_ranges[axis].m_min);

            if (!joystick->m_sliderAxis[axis]) {
                value = (value - 0.5f) * 2.0f;
            }

            if (value > 0.15f) {
                axes[axis] = static_cast<int32_t>((value - 0.15f) * 1.1764705f * 32767.0f);
            } else if (value >= -0.15f) {
                axes[axis] = 0;
            } else {
                axes[axis] = static_cast<int32_t>((value + 0.15f) * 1.1764705f * 32767.0f);
            }
        }
    }

    for (int32_t i = 0; i < 4; i++) {
        W32JoystickHat* hat = &joystick->m_hats[i];

        if (hat->m_enabled) {
            uint32_t directions = state->rgbButtons[hat->m_buttons[0]] != 0;

            if (state->rgbButtons[hat->m_buttons[1]]) {
                directions |= 2;
            }

            if (state->rgbButtons[hat->m_buttons[2]]) {
                directions |= 4;
            }

            if (state->rgbButtons[hat->m_buttons[3]]) {
                directions |= 8;
            }

            switch (directions) {
                case 1:
                    state->rgdwPOV[i] = 0;
                    break;

                case 2:
                    state->rgdwPOV[i] = 9000;
                    break;

                case 3:
                    state->rgdwPOV[i] = 4500;
                    break;

                case 4:
                    state->rgdwPOV[i] = 18000;
                    break;

                case 6:
                    state->rgdwPOV[i] = 13500;
                    break;

                case 8:
                    state->rgdwPOV[i] = 27000;
                    break;

                case 9:
                    state->rgdwPOV[i] = 31500;
                    break;

                case 12:
                    state->rgdwPOV[i] = 22500;
                    break;

                default:
                    state->rgdwPOV[i] = 0xFFFFFFFF;
                    break;
            }
        }
    }

    W32JoystickMouseMode* mouseMode = &joystick->m_mouseMode;

    if (mouseMode->m_enabled) {
        uint32_t now = OsGetAsyncTimeMs();

        if (mouseMode->m_lastTime) {
            float x = axes[joystick->m_sticks[0].m_xAxis] * (1.0f / 32767.0f);
            float y = (1.0f / 32767.0f) * axes[joystick->m_sticks[0].m_yAxis];
            float y2 = y * y;
            float x2 = x * x;

            if (y2 + x2 > 0.0f) {
                RECT rect;
                OsGetDefaultWindowRect(&rect);

                float distance = static_cast<int32_t>(now - mouseMode->m_lastTime) * 0.001f * (rect.right - rect.left) * mouseMode->m_speed;

                float dx = x * (x2 * x) * distance;
                mouseMode->m_accumulated.x = x < 0.0f ? mouseMode->m_accumulated.x - dx : dx + mouseMode->m_accumulated.x;

                float dy = distance * (y * (y2 * y));
                mouseMode->m_accumulated.y = y < 0.0f ? mouseMode->m_accumulated.y - dy : dy + mouseMode->m_accumulated.y;

                int32_t moveX = static_cast<int32_t>(mouseMode->m_accumulated.x);
                int32_t moveY = static_cast<int32_t>(mouseMode->m_accumulated.y);

                if (moveX || moveY) {
                    mouseMode->m_accumulated.x -= moveX;
                    mouseMode->m_accumulated.y -= moveY;

                    int32_t mouseX;
                    int32_t mouseY;
                    OsInputGetMousePosition(&mouseX, &mouseY);
                    OsInputSetMousePosition(moveX + mouseX, moveY + mouseY);
                }
            }
        }

        mouseMode->m_lastTime = now;
    }

    for (int32_t i = 0; i < 4; i++) {
        int32_t xAxis = joystick->m_sticks[i].m_xAxis;
        int32_t yAxis = joystick->m_sticks[i].m_yAxis;

        if (xAxis >= 0 && yAxis >= 0 && (!mouseMode->m_enabled || i != mouseMode->m_stick)) {
            if (previousAxes[xAxis] != axes[xAxis] || previousAxes[yAxis] != axes[yAxis]) {
                OsQueuePut(OS_INPUT_JOYSTICK_STICK, i, axes[xAxis], axes[yAxis], 0);
            }
        }
    }

    for (int32_t axis = 0; axis < 8; axis++) {
        if ((joystick->m_axisMask & (1 << axis)) && !joystick->m_stickAxis[axis]) {
            if (!mouseMode->m_enabled || (axis != joystick->m_sticks[mouseMode->m_stick].m_xAxis && axis != joystick->m_sticks[mouseMode->m_stick].m_yAxis)) {
                if (previousAxes[axis] != axes[axis]) {
                    OsQueuePut(OS_INPUT_JOYSTICK_AXIS, axis, axes[axis], 0, 0);
                }
            }
        }
    }

    for (uint32_t button = 0; button < 32; button++) {
        if (joystick->m_hatButton[button]) {
            continue;
        }

        BYTE down = state->rgbButtons[button];
        BYTE wasDown = previousState->rgbButtons[button];

        if (wasDown == down && !(bufferedButtons & (1 << button))) {
            continue;
        }

        if (mouseMode->m_enabled && (button == mouseMode->m_leftButton || button == mouseMode->m_rightButton)) {
            int32_t x;
            int32_t y;
            OsInputGetMousePosition(&x, &y);

            MOUSEBUTTON mouseButton = button != mouseMode->m_leftButton ? MOUSE_BUTTON_RIGHT : MOUSE_BUTTON_LEFT;

            if (previousState->rgbButtons[button] == state->rgbButtons[button]) {
                OsQueuePut(OS_INPUT_MOUSE_DOWN, mouseButton, x, y, 0);
                OsQueuePut(OS_INPUT_MOUSE_UP, mouseButton, x, y, 0);
            } else {
                OsQueuePut(state->rgbButtons[button] ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, mouseButton, x, y, 0);
            }
        } else if (wasDown == down) {
            OsQueuePut(OS_INPUT_JOYSTICK_BUTTON_DOWN, button, 0, 0, 0);
            OsQueuePut(OS_INPUT_JOYSTICK_BUTTON_UP, button, 0, 0, 0);
        } else {
            OsQueuePut(down ? OS_INPUT_JOYSTICK_BUTTON_DOWN : OS_INPUT_JOYSTICK_BUTTON_UP, button, 0, 0, 0);
        }
    }

    for (uint32_t i = 0; i < 4; i++) {
        DWORD pov = state->rgdwPOV[i];

        if (previousState->rgdwPOV[i] != pov) {
            int32_t direction = 0;

            if (pov != 0xFFFFFFFF) {
                direction = 8 * pov / 36000 + 1;
            }

            OsQueuePut(OS_INPUT_JOYSTICK_HAT, i, direction, 0, 0);
        }
    }
}

// OFFSET: 0x008714B0
void OsPollJoysticks(int32_t active) {
    for (uint32_t i = 0; i < s_joysticks.Count(); i++) {
        PollJoystick(i, active);
    }
}

#endif
