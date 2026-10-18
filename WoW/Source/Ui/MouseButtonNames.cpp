#include "WoW/Source/Ui/UIBindings.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 47894222 manual
// OFFSET: 0x007DC010
MOUSEBUTTON StringToMouseButton(const char* string) {
    if (!string || !*string) {
        return MOUSE_BUTTON_NONE;
    }

    if (!SStrCmpI(string, "LeftButton", STORM_MAX_STR)) {
        return MOUSE_BUTTON_LEFT;
    }

    if (!SStrCmpI(string, "RightButton", STORM_MAX_STR)) {
        return MOUSE_BUTTON_RIGHT;
    }

    if (!SStrCmpI(string, "MiddleButton", STORM_MAX_STR)) {
        return MOUSE_BUTTON_MIDDLE;
    }

    if (!SStrCmpI(string, "Button4", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON1;
    }

    if (!SStrCmpI(string, "Button5", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON2;
    }

    if (!SStrCmpI(string, "Button6", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON3;
    }

    if (!SStrCmpI(string, "Button7", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON4;
    }

    if (!SStrCmpI(string, "Button8", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON5;
    }

    if (!SStrCmpI(string, "Button9", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON6;
    }

    if (!SStrCmpI(string, "Button10", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON7;
    }

    if (!SStrCmpI(string, "Button11", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON8;
    }

    if (!SStrCmpI(string, "Button12", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON9;
    }

    if (!SStrCmpI(string, "Button13", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON10;
    }

    if (!SStrCmpI(string, "Button14", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON11;
    }

    if (!SStrCmpI(string, "Button15", STORM_MAX_STR)) {
        return MOUSE_BUTTON_XBUTTON12;
    }

    if (!SStrCmpI(string, "Button16", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x8000);
    }

    if (!SStrCmpI(string, "Button17", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x10000);
    }

    if (!SStrCmpI(string, "Button18", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x20000);
    }

    if (!SStrCmpI(string, "Button19", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x40000);
    }

    if (!SStrCmpI(string, "Button20", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x80000);
    }

    if (!SStrCmpI(string, "Button21", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x100000);
    }

    if (!SStrCmpI(string, "Button22", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x200000);
    }

    if (!SStrCmpI(string, "Button23", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x400000);
    }

    if (!SStrCmpI(string, "Button24", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x800000);
    }

    if (!SStrCmpI(string, "Button25", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x1000000);
    }

    if (!SStrCmpI(string, "Button26", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x2000000);
    }

    if (!SStrCmpI(string, "Button27", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x4000000);
    }

    if (!SStrCmpI(string, "Button28", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x8000000);
    }

    if (!SStrCmpI(string, "Button29", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x10000000);
    }

    if (!SStrCmpI(string, "Button30", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x20000000);
    }

    if (!SStrCmpI(string, "Button31", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(0x40000000);
    }

    return MOUSE_BUTTON_NONE;
}
