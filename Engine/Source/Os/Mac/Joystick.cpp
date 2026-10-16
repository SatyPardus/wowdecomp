#if defined(__APPLE__)

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>
#include <SDL.h>
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/Joystick.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"

struct MacJoystickStick {
    int32_t xAxis = -1;
    int32_t yAxis = -1;
};

struct MacJoystickHat {
    bool enabled = false;
    int32_t buttons[4] = {};
};

struct MacJoystickMouseMode {
    int32_t enabled = 1;
    int32_t stick = 0;
    float speed = 1.5f;
    int32_t leftButton = 0;
    int32_t rightButton = 1;
    uint32_t lastTime = 0;
    float accX = 0.0f;
    float accY = 0.0f;
};

struct MacJoystick {
    SDL_Joystick* joystick = nullptr;
    SDL_JoystickID instance = -1;
    char name[256] = {};
    int axes[8] = {};
    int prevAxes[8] = {};
    uint8_t buttons[32] = {};
    uint8_t prevButtons[32] = {};
    uint8_t hats[4] = {};
    uint8_t prevHats[4] = {};
    uint32_t axisMask = 0;
    uint32_t buttonCount = 0;
    uint32_t hatCount = 0;
    bool stickAxis[8] = {};
    bool sliderAxis[8] = {};
    bool hatButton[32] = {};
    MacJoystickStick sticks[4];
    MacJoystickHat mappedHats[4];
    MacJoystickMouseMode mouseMode;
};

static std::vector<MacJoystick> s_joysticks;
static bool s_enumerated;

static void EnumerateJoysticks() {
    if (s_enumerated) {
        return;
    }
    s_enumerated = true;
    int count = SDL_NumJoysticks();
    for (int i = 0; i < count; ++i) {
        MacJoystick joy;
        const char* name = SDL_JoystickNameForIndex(i);
        SStrCopy(joy.name, name ? name : "Joystick", sizeof(joy.name));
        joy.sticks[0].xAxis = 0;
        joy.sticks[0].yAxis = 1;
        joy.stickAxis[0] = true;
        joy.stickAxis[1] = true;
        s_joysticks.push_back(joy);
    }
}

uint32_t OsJoystickCount() {
    EnumerateJoysticks();
    return static_cast<uint32_t>(s_joysticks.size());
}

uint32_t OsJoystickOpen(uint32_t index) {
    if (index >= OsJoystickCount()) {
        return 0xFFFFFFFFu;
    }
    MacJoystick& joy = s_joysticks[index];
    if (!joy.joystick) {
        joy.joystick = SDL_JoystickOpen(static_cast<int>(index));
        if (!joy.joystick) {
            return 0xFFFFFFFFu;
        }
        joy.instance = SDL_JoystickInstanceID(joy.joystick);
        int axes = std::min(SDL_JoystickNumAxes(joy.joystick), 8);
        for (int i = 0; i < axes; ++i) {
            joy.axisMask |= 1u << i;
        }
        joy.buttonCount = static_cast<uint32_t>(std::min(SDL_JoystickNumButtons(joy.joystick), 32));
        joy.hatCount = static_cast<uint32_t>(std::min(SDL_JoystickNumHats(joy.joystick), 4));
    }
    return index;
}

void OsJoystickClose(uint32_t index) {
    if (index >= OsJoystickCount()) {
        return;
    }
    MacJoystick& joy = s_joysticks[index];
    if (joy.joystick) {
        SDL_JoystickClose(joy.joystick);
        joy.joystick = nullptr;
        joy.instance = -1;
    }
}

char* OsJoystickGetName(uint32_t index, char* buffer, uint32_t size) {
    if (index >= OsJoystickCount() || !buffer || !size) {
        return nullptr;
    }
    SStrCopy(buffer, s_joysticks[index].name, size);
    return buffer;
}

void OsJoystickMapStick(uint32_t index, int32_t stick, int32_t xAxis, int32_t yAxis) {
    if (index >= OsJoystickCount() || stick < 0 || stick >= 4 || xAxis < 0 || xAxis >= 8 || yAxis < 0 || yAxis >= 8) {
        return;
    }
    MacJoystick& joy = s_joysticks[index];
    joy.sticks[stick].xAxis = xAxis;
    joy.sticks[stick].yAxis = yAxis;
    joy.stickAxis[xAxis] = true;
    joy.stickAxis[yAxis] = true;
}

void OsJoystickMapSlider(uint32_t index, int32_t axis) {
    if (index < OsJoystickCount() && axis >= 0 && axis < 8) {
        s_joysticks[index].sliderAxis[axis] = true;
    }
}

void OsJoystickMapHat(uint32_t index, int32_t hat, int32_t up, int32_t right, int32_t down, int32_t left) {
    if (index >= OsJoystickCount() || hat < 0 || hat >= 4 || up < 0 || up >= 32 || right < 0 || right >= 32 || down < 0 || down >= 32 || left < 0 || left >= 32) {
        return;
    }
    MacJoystick& joy = s_joysticks[index];
    joy.mappedHats[hat].enabled = true;
    joy.mappedHats[hat].buttons[0] = up;
    joy.mappedHats[hat].buttons[1] = right;
    joy.mappedHats[hat].buttons[2] = down;
    joy.mappedHats[hat].buttons[3] = left;
    joy.hatButton[up] = joy.hatButton[right] = joy.hatButton[down] = joy.hatButton[left] = true;
}

void OsJoystickSetMouseMode(uint32_t index, int32_t enable) {
    if (index >= OsJoystickCount()) {
        return;
    }
    MacJoystick& joy = s_joysticks[index];
    if (joy.mouseMode.enabled == enable) {
        return;
    }
    joy.mouseMode.enabled = enable;
    joy.mouseMode.lastTime = 0;
    joy.mouseMode.accX = 0.0f;
    joy.mouseMode.accY = 0.0f;
}

void OsJoystickConfigureMouseMode(uint32_t index, int32_t stick, int32_t leftButton, int32_t rightButton, float speed) {
    if (index >= OsJoystickCount() || stick < 0 || stick >= 4) {
        return;
    }
    MacJoystick& joy = s_joysticks[index];
    joy.mouseMode.stick = stick;
    joy.mouseMode.leftButton = leftButton;
    joy.mouseMode.rightButton = rightButton;
    joy.mouseMode.speed = speed;
}

static int NormalizeAxis(Sint16 value) {
    float f = value < 0 ? static_cast<float>(value) / 32768.0f : static_cast<float>(value) / 32767.0f;
    if (fabsf(f) <= 0.15f) {
        return 0;
    }
    float scaled = (fabsf(f) - 0.15f) / 0.85f;
    return static_cast<int>((f < 0.0f ? -scaled : scaled) * 32767.0f);
}

static uint8_t ButtonMaskToHat(uint32_t directions) {
    switch (directions) {
        case 1:
            return SDL_HAT_UP;
        case 2:
            return SDL_HAT_RIGHT;
        case 3:
            return SDL_HAT_RIGHTUP;
        case 4:
            return SDL_HAT_DOWN;
        case 6:
            return SDL_HAT_RIGHTDOWN;
        case 8:
            return SDL_HAT_LEFT;
        case 9:
            return SDL_HAT_LEFTUP;
        case 12:
            return SDL_HAT_LEFTDOWN;
        default:
            return SDL_HAT_CENTERED;
    }
}

static int32_t HatDirection(uint8_t hat) {
    switch (hat) {
        case SDL_HAT_UP:
            return 1;
        case SDL_HAT_RIGHTUP:
            return 2;
        case SDL_HAT_RIGHT:
            return 3;
        case SDL_HAT_RIGHTDOWN:
            return 4;
        case SDL_HAT_DOWN:
            return 5;
        case SDL_HAT_LEFTDOWN:
            return 6;
        case SDL_HAT_LEFT:
            return 7;
        case SDL_HAT_LEFTUP:
            return 8;
        default:
            return 0;
    }
}

static void PollJoystick(uint32_t index, int32_t active) {
    MacJoystick& joy = s_joysticks[index];
    if (!joy.joystick) {
        return;
    }

    memcpy(joy.prevAxes, joy.axes, sizeof(joy.axes));
    memcpy(joy.prevButtons, joy.buttons, sizeof(joy.buttons));
    memcpy(joy.prevHats, joy.hats, sizeof(joy.hats));

    SDL_JoystickUpdate();
    if (!active) {
        memset(joy.axes, 0, sizeof(joy.axes));
        memset(joy.buttons, 0, sizeof(joy.buttons));
        memset(joy.hats, 0, sizeof(joy.hats));
    } else {
        for (int i = 0; i < 8; ++i) {
            joy.axes[i] = (joy.axisMask & (1u << i)) ? NormalizeAxis(SDL_JoystickGetAxis(joy.joystick, i)) : 0;
        }
        for (uint32_t i = 0; i < joy.buttonCount; ++i) {
            joy.buttons[i] = SDL_JoystickGetButton(joy.joystick, i) != 0;
        }
        for (uint32_t i = 0; i < joy.hatCount; ++i) {
            joy.hats[i] = SDL_JoystickGetHat(joy.joystick, i);
        }
    }

    for (int i = 0; i < 4; ++i) {
        MacJoystickHat& hat = joy.mappedHats[i];
        if (!hat.enabled) {
            continue;
        }
        uint32_t directions = 0;
        if (joy.buttons[hat.buttons[0]]) directions |= 1;
        if (joy.buttons[hat.buttons[1]]) directions |= 2;
        if (joy.buttons[hat.buttons[2]]) directions |= 4;
        if (joy.buttons[hat.buttons[3]]) directions |= 8;
        joy.hats[i] = ButtonMaskToHat(directions);
    }

    MacJoystickMouseMode& mouse = joy.mouseMode;
    if (mouse.enabled && mouse.stick >= 0 && mouse.stick < 4) {
        MacJoystickStick& stick = joy.sticks[mouse.stick];
        if (stick.xAxis >= 0 && stick.yAxis >= 0) {
            uint32_t now = OsGetAsyncTimeMs();
            if (mouse.lastTime) {
                float x = static_cast<float>(joy.axes[stick.xAxis]) / 32767.0f;
                float y = static_cast<float>(joy.axes[stick.yAxis]) / 32767.0f;
                if (x || y) {
                    tagRECT rect;
                    OsGetDefaultWindowRect(&rect);
                    float distance = static_cast<float>(now - mouse.lastTime) * 0.001f * static_cast<float>(rect.right - rect.left) * mouse.speed;
                    mouse.accX += x * fabsf(x) * fabsf(x) * distance;
                    mouse.accY += y * fabsf(y) * fabsf(y) * distance;
                    int moveX = static_cast<int>(mouse.accX);
                    int moveY = static_cast<int>(mouse.accY);
                    if (moveX || moveY) {
                        mouse.accX -= moveX;
                        mouse.accY -= moveY;
                        int mx;
                        int my;
                        OsInputGetMousePosition(&mx, &my);
                        OsInputSetMousePosition(mx + moveX, my + moveY);
                    }
                }
            }
            mouse.lastTime = now;
        }
    }

    for (int i = 0; i < 4; ++i) {
        MacJoystickStick& stick = joy.sticks[i];
        if (stick.xAxis >= 0 && stick.yAxis >= 0 && (!mouse.enabled || i != mouse.stick)) {
            if (joy.prevAxes[stick.xAxis] != joy.axes[stick.xAxis] || joy.prevAxes[stick.yAxis] != joy.axes[stick.yAxis]) {
                OsQueuePut(OS_INPUT_14, i, joy.axes[stick.xAxis], joy.axes[stick.yAxis], 0);
            }
        }
    }

    for (int axis = 0; axis < 8; ++axis) {
        if ((joy.axisMask & (1u << axis)) && !joy.stickAxis[axis]) {
            if (joy.prevAxes[axis] != joy.axes[axis]) {
                OsQueuePut(OS_INPUT_15, axis, joy.axes[axis], 0, 0);
            }
        }
    }

    for (uint32_t button = 0; button < joy.buttonCount; ++button) {
        if (joy.hatButton[button] || joy.prevButtons[button] == joy.buttons[button]) {
            continue;
        }
        if (mouse.enabled && (static_cast<int32_t>(button) == mouse.leftButton || static_cast<int32_t>(button) == mouse.rightButton)) {
            int x;
            int y;
            OsInputGetMousePosition(&x, &y);
            MOUSEBUTTON mouseButton = static_cast<int32_t>(button) == mouse.leftButton ? MOUSE_BUTTON_LEFT : MOUSE_BUTTON_RIGHT;
            OsQueuePut(joy.buttons[button] ? OS_INPUT_MOUSE_DOWN : OS_INPUT_MOUSE_UP, mouseButton, x, y, 0);
        } else {
            OsQueuePut(joy.buttons[button] ? OS_INPUT_16 : OS_INPUT_17, button, 0, 0, 0);
        }
    }

    for (uint32_t i = 0; i < joy.hatCount; ++i) {
        if (joy.prevHats[i] != joy.hats[i]) {
            OsQueuePut(OS_INPUT_18, i, HatDirection(joy.hats[i]), 0, 0);
        }
    }
}

void OsPollJoysticks(int32_t active) {
    for (uint32_t i = 0; i < OsJoystickCount(); ++i) {
        PollJoystick(i, active);
    }
}

#endif
