#ifndef ENGINE_SOURCE_OS_INPUT_H
#define ENGINE_SOURCE_OS_INPUT_H

#include <cstdint>
#include "Engine/Source/Event/Types.h"

struct tagRECT;

enum OSINPUT {
    OS_INPUT_CAPTURE_CHANGED = 0,
    OS_INPUT_CHAR = 1,
    OS_INPUT_STRING = 2,
    OS_INPUT_IME = 3,
    OS_INPUT_SIZE = 4,
    OS_INPUT_CLOSE = 5,
    OS_INPUT_FOCUS = 6,
    OS_INPUT_KEY_DOWN = 7,
    OS_INPUT_KEY_UP = 8,
    OS_INPUT_MOUSE_DOWN = 9,
    OS_INPUT_MOUSE_MOVE = 10,
    OS_INPUT_MOUSE_WHEEL = 11,
    OS_INPUT_MOUSE_MOVE_RELATIVE = 12,
    OS_INPUT_MOUSE_UP = 13,
    OS_INPUT_JOYSTICK_STICK = 14,
    OS_INPUT_JOYSTICK_AXIS = 15,
    OS_INPUT_JOYSTICK_BUTTON_DOWN = 16,
    OS_INPUT_JOYSTICK_BUTTON_UP = 17,
    OS_INPUT_JOYSTICK_HAT = 18,
    OS_INPUT_SHUTDOWN = 19,
};

enum : int32_t {
    OS_INPUT_IME_NOTIFYLANGUAGE = 0,
    OS_INPUT_IME_STARTCOMPOSITION = 1,
    OS_INPUT_IME_COMPOSITION = 2,
    OS_INPUT_IME_OPENCANDIDATES = 3,
    OS_INPUT_IME_CHANGECANDIDATES = 4,
    OS_INPUT_IME_CLOSECANDIDATES = 5,
    OS_INPUT_IME_ENDCOMPOSITION = 6,
};

enum OS_MOUSE_MODE {
    OS_MOUSE_MODE_NORMAL = 0,
    OS_MOUSE_MODE_RELATIVE = 1,
    OS_MOUSE_MODES = 2,
};

struct OSEVENT {
    /* 0000 */ OSINPUT id;
    /* 0004 */ int32_t param[4];
};

int32_t OsQueueGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3);
void OsQueuePut(OSINPUT id, int32_t param0, int32_t param1, int32_t param2, int32_t param3);
int32_t OsInputIsKeyDown(KEY key);
int32_t OsInputIsMouseButtonDown(MOUSEBUTTON button);
void OsInputSetWindowResizeLock(int32_t resizeLock);
float OsInputGetMouseSpeed();
void OsInputInitialize();
void OsInputDestroy();
void OsInputEnableSecureMode(int32_t enable);
void OsInputSetEventPollProc(void (*pollProc)());
int32_t OsInputGet(OSINPUT* id, int32_t* param0, int32_t* param1, int32_t* param2, int32_t* param3);
void OsInputSetMouseMode(OS_MOUSE_MODE mode);
void OsInputSetMouseSpeed(float speed);
void OsInputGetMousePosition(int32_t* x, int32_t* y);
void OsInputSetMousePosition(int32_t x, int32_t y);
int32_t OsGetDefaultWindowRect(tagRECT* rect);
intptr_t OsWindowProc(void* window, uint32_t message, uintptr_t wparam, intptr_t lparam);

#endif
