#if defined(__APPLE__)

#include <cstdint>
#include <SDL.h>
#include "Engine/Source/Os/Gui.h"

static SDL_Window* s_GxDevWindow;

void* OsGuiGetWindow(int32_t type) {
    switch (type) {
        case 0:
        case 1:
        case 2:
            return s_GxDevWindow;
        default:
            return nullptr;
    }
}

void OsGuiSetGxWindow(void* window) {
    s_GxDevWindow = static_cast<SDL_Window*>(window);
}

void OsGuiSetWindowTitle(void* window, const char* title) {
    SDL_SetWindowTitle(static_cast<SDL_Window*>(window ? window : static_cast<void*>(s_GxDevWindow)), title ? title : "");
}

int32_t OsGuiMessageBox(void* parentWindow, int32_t style, const char* message, const char* title) {
    const SDL_MessageBoxButtonData ok[] = {{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"}};
    const SDL_MessageBoxButtonData okCancel[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel"},
    };
    const SDL_MessageBoxButtonData yesNo[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "No"},
    };
    const SDL_MessageBoxButtonData yesNoCancel[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Yes"},
        {0, 1, "No"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 2, "Cancel"},
    };

    SDL_MessageBoxData data = {};
    data.flags = SDL_MESSAGEBOX_INFORMATION;
    data.window = static_cast<SDL_Window*>(parentWindow ? parentWindow : static_cast<void*>(s_GxDevWindow));
    data.title = title ? title : "";
    data.message = message ? message : "";

    switch (style) {
        case 1:
            data.numbuttons = 2;
            data.buttons = okCancel;
            break;
        case 2:
            data.numbuttons = 2;
            data.buttons = yesNo;
            break;
        case 3:
            data.numbuttons = 3;
            data.buttons = yesNoCancel;
            break;
        default:
            data.numbuttons = 1;
            data.buttons = ok;
            break;
    }

    int button = 2;
    if (SDL_ShowMessageBox(&data, &button) < 0) {
        return 2;
    }
    return button;
}

int32_t OsGuiIsModifierKeyDown(int32_t key) {
    SDL_Keymod mod = SDL_GetModState();
    switch (key) {
        case 0:
            return (mod & KMOD_CTRL) != 0;
        case 1:
            return (mod & KMOD_SHIFT) != 0;
        case 2:
            return (mod & KMOD_ALT) != 0;
        default:
            return 0;
    }
}

void OsGuiBeep() {
    SDL_Log("\a");
}

int32_t OsGuiProcessMessage(void* message) {
    return 0;
}

#endif
