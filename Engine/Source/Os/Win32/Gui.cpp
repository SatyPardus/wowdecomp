#if defined(WOWDECOMP_ARCH_32)

#include <cstdint>
#include <malloc.h>
#include <windows.h>
#include <commctrl.h>
#include "Engine/Source/Os/Gui.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D41620
static HWND s_GxDevWindow;

// STATIC: 0x00D4161C
static HINSTANCE s_appInstance;

// STATIC: 0x00D41624
static HWND s_globalTips;

// STATIC: 0x00D4163C
static TSGrowableArray<COsDialog*> s_dialogs;

// STATIC: 0x00D4164C
static TSGrowableArray<COsMenuBar*> s_menubars;

// STATIC: 0x00B1D34C
static int32_t s_menuHotkeysEnabled = 1;

// INLINED
int32_t COsDialog::OnEvent(int32_t itemId, int32_t notifyCode, int32_t code) {
    if (this->m_callback) {
        OsGuiCallbackParams params;
        params.type = itemId;
        params.subType = notifyCode;
        params.code = code;
        params.user = this->m_callbackParam;
        this->m_callback(params);
    }

    return 1;
}

// VERIFY: 8ebdb57b auto
// OFFSET: 0x0086C520
int32_t COsControl::SendEvent(int32_t event, int32_t code) {
    if (this->OnEvent(this->m_id, event, code)) {
        return 1;
    }

    if (this->m_dialog) {
        return this->m_dialog->OnEvent(this->m_id, event, code);
    }

    return 0;
}

// VERIFY: 7311c49a auto
// OFFSET: 0x0086C590
static int32_t sIsCharacterAllowed(char c, uint32_t filters) {
    uint8_t ch = static_cast<uint8_t>(c);

    if (ch <= 0x1F) {
        return 1;
    }

    if (filters & 0x1) {
        if (ch >= 'a' && ch <= 'z') {
            return 1;
        }

        if (ch >= 'A' && ch <= 'Z') {
            return 1;
        }
    }

    if ((filters & 0x2) && ch >= '0' && ch <= '9') {
        return 1;
    }

    if ((filters & 0x4) && (ch == '+' || ch == '-')) {
        return 1;
    }

    if ((filters & 0x8) && ch == '.') {
        return 1;
    }

    if ((filters & 0x10) && ch == ' ') {
        return 1;
    }

    if ((filters & 0x20) && ch == '_') {
        return 1;
    }

    return (filters & 0x40) && ch <= 0x7E;
}

// INLINED
OsGuiHandle COsTreeView::GetEditControl() {
    return reinterpret_cast<void*>(SendMessageA(static_cast<HWND>(this->m_handle), TVM_GETEDITCONTROL, 0, 0));
}

// VERIFY: da6670f7 manual
// OFFSET: 0x0086C600
int32_t COsTreeView::IsCharacterAllowed(char c) {
    if (!this->GetEditControl()) {
        return 1;
    }

    if (this->m_filtersEnabled) {
        return sIsCharacterAllowed(c, this->m_filters);
    }

    return 1;
}

// VERIFY: d132b7cd auto
// OFFSET: 0x0086C650
void OsGuiSetWindowTitle(OsGuiHandle window, const char* title) {
    uint32_t length = SUniConvertUTF8to16Len(title, 0x7FFFFFFF, nullptr);
    uint16_t* wideTitle = static_cast<uint16_t*>(_alloca(length * sizeof(uint16_t)));
    SUniConvertUTF8to16(wideTitle, length, title, 0x7FFFFFFF, nullptr, nullptr);

    SetWindowTextW(static_cast<HWND>(window), reinterpret_cast<const wchar_t*>(wideTitle));
}

// VERIFY: 7fcab45a auto
// OFFSET: 0x0086C6A0
OsGuiHandle OsGuiGetWindow(int32_t type) {
    switch (type) {
        case OSGUI_WINDOW_TYPE_GX:
            return s_GxDevWindow;

        case OSGUI_WINDOW_TYPE_ACTIVE:
            return GetActiveWindow();

        case OSGUI_WINDOW_TYPE_FRONT:
            return GetForegroundWindow();

        default:
            return nullptr;
    }
}

// VERIFY: 3cdad9fe auto
// OFFSET: 0x0086C6D0
void OsGuiSetGxWindow(OsGuiHandle window) {
    s_GxDevWindow = static_cast<HWND>(window);
}

// VERIFY: 39d42e31 manual
// OFFSET: 0x0086C6E0
int32_t OsGuiMessageBox(OsGuiHandle parentWindow, int32_t style, const char* message, const char* title) {
    uint32_t messageLength = SUniConvertUTF8to16Len(message, 0x7FFFFFFF, nullptr);
    uint16_t* wideMessage = static_cast<uint16_t*>(_alloca(messageLength * sizeof(uint16_t)));
    SUniConvertUTF8to16(wideMessage, messageLength, message, 0x7FFFFFFF, nullptr, nullptr);

    if (!title) {
        title = "";
    }

    uint32_t titleLength = SUniConvertUTF8to16Len(title, 0x7FFFFFFF, nullptr);
    uint16_t* wideTitle = static_cast<uint16_t*>(_alloca(titleLength * sizeof(uint16_t)));
    SUniConvertUTF8to16(wideTitle, titleLength, title, 0x7FFFFFFF, nullptr, nullptr);

    UINT type = MB_OK;

    switch (style) {
        case OSGUI_MESSAGE_STYLE_OK:
            type = MB_OK;
            break;

        case OSGUI_MESSAGE_STYLE_OKCANCEL:
            type = MB_OKCANCEL;
            break;

        case OSGUI_MESSAGE_STYLE_YESNO:
            type = MB_YESNO;
            break;

        case OSGUI_MESSAGE_STYLE_YESNOCANCEL:
            type = MB_YESNOCANCEL;
            break;

        default:
            break;
    }

    switch (MessageBoxW(static_cast<HWND>(parentWindow), reinterpret_cast<const wchar_t*>(wideMessage), reinterpret_cast<const wchar_t*>(wideTitle), type)) {
        case IDOK:
        case IDYES:
            return OSGUI_MESSAGE_RESULT_YESOK;

        case IDNO:
            return OSGUI_MESSAGE_RESULT_NO;

        default:
            return OSGUI_MESSAGE_RESULT_CANCEL;
    }
}

// VERIFY: 93ff2d97 manual
// OFFSET: 0x0086C7D0
int32_t OsGuiIsModifierKeyDown(int32_t key) {
    switch (key) {
        case OSGUI_KEY_CONTROL:
            return (GetKeyState(VK_CONTROL) & 0xF000) != 0;

        case OSGUI_KEY_SHIFT:
            return (GetKeyState(VK_SHIFT) & 0xF000) != 0;

        case OSGUI_KEY_ALT:
            return (GetKeyState(VK_MENU) & 0xF000) != 0;

        default:
            return 0;
    }
}

// INLINED
void OsGuiBeep() {
    MessageBeep(0);
}

// INLINED
static COsDialog* sGetOsGuiPointer(void* window) {
    return static_cast<COsDialog*>(GetPropA(static_cast<HWND>(window), "OsGuiPointer"));
}

// INLINED
int32_t COsEditBox::IsCharacterAllowed(char c) {
    if (!this->m_filtersEnabled) {
        return 1;
    }

    return sIsCharacterAllowed(c, this->m_filters);
}

// INLINED
int32_t COsTabControl::GetNumItems() {
    return static_cast<int32_t>(SendMessageA(static_cast<HWND>(this->m_handle), TCM_GETITEMCOUNT, 0, 0));
}

// INLINED
OsGuiHandle COsDialog::GetParentWindow() {
    return GetParent(static_cast<HWND>(this->m_handle));
}

// INLINED
int32_t COsDialog::HasFlag(uint32_t flag) {
    return (this->m_flags & flag) != 0;
}

// INLINED
int32_t COsDialog::IsInFront() {
    return this->m_handle == OsGuiGetWindow(OSGUI_WINDOW_TYPE_FRONT);
}

// VERIFY: 6ad9fe33 auto
// OFFSET: 0x0086C840
COsControl* COsDialog::FindControl(OsGuiHandle handle) {
    for (uint32_t i = 0; i < this->m_controls.Count(); i++) {
        if (this->m_controls[i]->IsHandleFromControl(handle)) {
            return this->m_controls[i];
        }
    }

    return nullptr;
}

// VERIFY: 9f9f2a0b auto
// OFFSET: 0x0086C890
int32_t COsDialog::OnCancel() {
    if (!this->m_cancelButton || this->m_cancelButton->OnEvent(this->m_cancelButton->m_id, 0, 0)) {
        return 1;
    }

    return this->OnEvent(this->m_cancelButton->m_id, 0, 0);
}

// VERIFY: f2de4205 manual
// OFFSET: 0x0086C900
int32_t COsTabControl::OnControlTab() {
    int32_t shift = OsGuiIsModifierKeyDown(OSGUI_KEY_SHIFT);
    int32_t value = this->GetValue();
    int32_t count = this->GetNumItems();

    int32_t next = shift ? value - 1 : value + 1;

    if (next < 0) {
        next = count - 1;
    }

    if (next >= count) {
        next = 0;
    }

    this->SetValue(next);
    this->SendEvent(2, 0);

    return 1;
}

// VERIFY: 267c4173 manual
// OFFSET: 0x0086C980
int32_t COsDialog::OnControlTab() {
    int32_t count = this->m_controls.Count();

    for (int32_t i = 0; i < count; i++) {
        if (this->m_controls[i]->m_type == 15) {
            COsTabControl* tabControl = static_cast<COsTabControl*>(this->m_controls[i]);
            return tabControl ? tabControl->OnControlTab() : 0;
        }
    }

    return 0;
}

// VERIFY: c3267b06 manual
// OFFSET: 0x0086C9C0
int32_t COsDialog::ProcessMessage(void* message) {
    MSG* msg = static_cast<MSG*>(message);

    if (msg->wParam == VK_ESCAPE && msg->message == WM_KEYDOWN) {
        COsControl* control = this->FindControl(msg->hwnd);
        if (control && control->m_type == 4 && (GetWindowLongA(static_cast<HWND>(control->m_handle), GWL_STYLE) & ES_MULTILINE)) {
            return this->OnCancel();
        }
    }

    if (msg->message == WM_CHAR) {
        COsControl* control = this->FindControl(msg->hwnd);
        if (control) {
            char c = static_cast<char>(msg->wParam);
            int32_t allowed = 1;

            if (control->m_type == 4) {
                allowed = static_cast<COsEditBox*>(control)->IsCharacterAllowed(c);
            } else if (control->m_type == 10) {
                allowed = static_cast<COsTreeView*>(control)->IsCharacterAllowed(c);
            }

            if (!allowed) {
                OsGuiBeep();
                return 1;
            }
        }
    }

    if (msg->wParam == VK_RETURN && msg->message == WM_KEYDOWN) {
        COsControl* control = this->FindControl(msg->hwnd);
        if (control && (control->m_type == 6 || control->m_type == 10 || control->m_type == 16) && control->OnReturn()) {
            return 1;
        }
    }

    if (this->IsInFront() && msg->wParam == VK_TAB && msg->message == WM_KEYDOWN && OsGuiIsModifierKeyDown(OSGUI_KEY_CONTROL) && this->OnControlTab()) {
        return 1;
    }

    return IsDialogMessageA(static_cast<HWND>(this->m_handle), msg);
}

// VERIFY: 66cc3d60 manual
// OFFSET: 0x0086CB00
int32_t OsGuiProcessMessage(void* message) {
    MSG* msg = static_cast<MSG*>(message);

    if (!s_appInstance) {
        return 0;
    }

    int32_t handled = 0;

    if (s_menuHotkeysEnabled) {
        for (uint32_t i = 0; i < s_menubars.Count(); i++) {
            HACCEL accelerators = static_cast<HACCEL>(s_menubars[i]->m_accelerators);
            if (!accelerators) {
                continue;
            }

            HWND window = static_cast<HWND>(s_menubars[i]->m_windowHandle);
            void* activeWindow = OsGuiGetWindow(OSGUI_WINDOW_TYPE_ACTIVE);

            if (window != activeWindow) {
                COsDialog* dialog = sGetOsGuiPointer(activeWindow);
                if (!dialog || !dialog->HasFlag(0x10) || window != dialog->GetParentWindow()) {
                    continue;
                }
            }

            if (TranslateAcceleratorA(window, accelerators, msg)) {
                handled = 1;

                if (msg->message != WM_KEYDOWN || msg->wParam != VK_DELETE) {
                    return 1;
                }

                break;
            }
        }
    }

    for (uint32_t i = 0; i < s_dialogs.Count(); i++) {
        if (s_dialogs[i]->ProcessMessage(msg)) {
            handled = 1;
            break;
        }
    }

    if (s_globalTips) {
        SendMessageA(s_globalTips, TTM_RELAYEVENT, 0, reinterpret_cast<LPARAM>(msg));
    }

    return handled;
}

#endif
