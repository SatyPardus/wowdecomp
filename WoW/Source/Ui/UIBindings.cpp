#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <malloc.h>
#include <new>
#include <typeinfo>
#include "WoW/Source/Ui/UIBindings.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Event/CKeyEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Storm/Source/MD5.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/AccountData.h"

// STATIC: 0x00ACDF1C
static CGUIBindingsStatus s_bindingsStatus;

// STATIC: 0x00ACDF30
static const char* s_keyNames[31] = {
    "SPACE",
    "NUMPADPLUS",
    "NUMPADMINUS",
    "NUMPADMULTIPLY",
    "NUMPADDIVIDE",
    "NUMPADDECIMAL",
    "ESCAPE",
    "ENTER",
    "BACKSPACE",
    "TAB",
    "LEFT",
    "UP",
    "RIGHT",
    "DOWN",
    "INSERT",
    "DELETE",
    "HOME",
    "END",
    "PAGEUP",
    "PAGEDOWN",
    "NUMLOCK",
    "CAPSLOCK",
    "PRINTSCREEN",
    "NUMPADEQUALS",
    "MOUSEWHEELDOWN",
    "MOUSEWHEELUP",
    "JOYHAT",
    "JOYHATUP",
    "JOYHATRIGHT",
    "JOYHATDOWN",
    "JOYHATLEFT",
};

// STATIC: 0x00ACDFAC
static BINDINGPREFIX s_numberedKeys[6] = {
    { "F", 1 },
    { "NUMPAD", 6 },
    { "BUTTON", 6 },
    { "JOYSTICK", 8 },
    { "JOYAXIS", 7 },
    { "JOYBUTTON", 9 },
};

// STATIC: 0x00ACDFDC
static BINDINGPREFIX s_modifierPrefixes[3] = {
    { "SHIFT-", 6 },
    { "CTRL-", 5 },
    { "ALT-", 4 },
};

// STATIC: 0x00BEADD4
static uint32_t s_loadPendingMask;

// STATIC: 0x00BEADD8
CGUIBindings* CGUIBindings::s_bindings;

// STATIC: 0x00BEADDC
static uint16_t s_initRound;

// STATIC: 0x00BEADE8
static TSHashTable<CLICKFRAME, HASHKEY_STR> s_clickFrames;

// STATIC: 0x00BEAE10
static char s_keyName[8];

// STATIC: 0x00BEAE18
static char s_bindKey[2];

// VERIFY: ba836cb5 manual
// OFFSET: 0x0055DAB0
static int32_t IsValidKeyName(const char* key) {
    bool found;

    do {
        found = false;

        for (uint32_t i = 0; i < 3; i++) {
            if (!SStrCmp(s_modifierPrefixes[i].m_prefix, key, s_modifierPrefixes[i].m_length)) {
                key += s_modifierPrefixes[i].m_length;
                found = true;
            }
        }
    } while (found);

    int32_t chars = 0;
    sgetu8(reinterpret_cast<const uint8_t*>(key), &chars);

    if (!key[chars]) {
        return 1;
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (!SStrCmp(s_numberedKeys[i].m_prefix, key, s_numberedKeys[i].m_length) && key[s_numberedKeys[i].m_length] >= '0' && key[s_numberedKeys[i].m_length] <= '9') {
            const char* suffix = key + s_numberedKeys[i].m_length;

            while (*suffix >= '0' && *suffix <= '9') {
                suffix++;
            }

            if (!*suffix) {
                return 1;
            }

            if (!SStrCmp(s_numberedKeys[i].m_prefix, "JOYAXIS", STORM_MAX_STR) && (!SStrCmp(suffix, "POS", STORM_MAX_STR) || !SStrCmp(suffix, "NEG", STORM_MAX_STR))) {
                return 1;
            }

            return 0;
        }
    }

    for (uint32_t i = 0; i < 31; i++) {
        if (!SStrCmp(key, s_keyNames[i], STORM_MAX_STR)) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 532ce7dd manual
// OFFSET: 0x0055DD10
static const char* GetKeyName(KEY key) {
    if (key > KEY_SPACE && key < KEY_NUMPAD0) {
        sputu8(key, s_keyName);
        return s_keyName;
    }

    switch (key) {
        case KEY_NONE:
            return "NONE";
        case KEY_LSHIFT:
            return "LSHIFT";
        case KEY_RSHIFT:
            return "RSHIFT";
        case KEY_LCONTROL:
            return "LCTRL";
        case KEY_RCONTROL:
            return "RCTRL";
        case KEY_LALT:
            return "LALT";
        case KEY_RALT:
            return "RALT";
        case KEY_SPACE:
            return "SPACE";
        case KEY_NUMPAD0:
        case KEY_NUMPAD1:
        case KEY_NUMPAD2:
        case KEY_NUMPAD3:
        case KEY_NUMPAD4:
        case KEY_NUMPAD5:
        case KEY_NUMPAD6:
        case KEY_NUMPAD7:
        case KEY_NUMPAD8:
        case KEY_NUMPAD9:
            SStrPrintf(s_keyName, sizeof(s_keyName), "NUMPAD%d", key - KEY_NUMPAD0);
            return s_keyName;
        case KEY_NUMPAD_PLUS:
            return "NUMPADPLUS";
        case KEY_NUMPAD_MINUS:
            return "NUMPADMINUS";
        case KEY_NUMPAD_MULTIPLY:
            return "NUMPADMULTIPLY";
        case KEY_NUMPAD_DIVIDE:
            return "NUMPADDIVIDE";
        case KEY_NUMPAD_DECIMAL:
            return "NUMPADDECIMAL";
        case KEY_ESCAPE:
            return "ESCAPE";
        case KEY_ENTER:
            return "ENTER";
        case KEY_BACKSPACE:
            return "BACKSPACE";
        case KEY_TAB:
            return "TAB";
        case KEY_LEFT:
            return "LEFT";
        case KEY_UP:
            return "UP";
        case KEY_RIGHT:
            return "RIGHT";
        case KEY_DOWN:
            return "DOWN";
        case KEY_INSERT:
            return "INSERT";
        case KEY_DELETE:
            return "DELETE";
        case KEY_HOME:
            return "HOME";
        case KEY_END:
            return "END";
        case KEY_PAGEUP:
            return "PAGEUP";
        case KEY_PAGEDOWN:
            return "PAGEDOWN";
        case KEY_CAPSLOCK:
            return "CAPSLOCK";
        case KEY_NUMLOCK:
            return "NUMLOCK";
        case KEY_PRINTSCREEN:
            return "PRINTSCREEN";
        case KEY_NUMPAD_EQUALS:
            return "NUMPADEQUALS";
        case KEY_F1:
        case KEY_F2:
        case KEY_F3:
        case KEY_F4:
        case KEY_F5:
        case KEY_F6:
        case KEY_F7:
        case KEY_F8:
        case KEY_F9:
        case KEY_F10:
        case KEY_F11:
        case KEY_F12:
        case KEY_F14:
        case KEY_F15:
        case KEY_F16:
        case KEY_F17:
        case KEY_F18:
        case KEY_F19:
            SStrPrintf(s_keyName, sizeof(s_keyName), "F%d", key - KEY_F1 + 1);
            return s_keyName;
        default:
            return "UNKNOWN";
    }
}

// VERIFY: 54f763fe auto
// OFFSET: 0x00812F60
void CGUIBindingsStatus::Add(STATUS_TYPE severity, const char* format, ...) {
    char buffer[512];

    va_list args;
    va_start(args, format);
    SStrVPrintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    DebugLog(buffer);
}

// VERIFY: 5ba4a2e0 manual
// OFFSET: 0x00560300
OVERRIDEKEYBINDING::~OVERRIDEKEYBINDING() {
    for (int32_t i = 0; i < 2; i++) {
        this->m_lists[i].Clear();
    }

    this->m_freeList.Clear();
}

// VERIFY: 2b1c7395 manual
// OFFSET: 0x0055E5E0
KEYBINDING* OVERRIDEKEYBINDING::GetActiveBinding() {
    for (int32_t i = 1; i >= 0; i--) {
        OVERRIDEKEYBINDINGENTRY* entry = this->m_lists[i].Head();

        if (entry) {
            return &entry->m_binding;
        }
    }

    return nullptr;
}

// VERIFY: b85e16f3 manual
// OFFSET: 0x005603D0
void OVERRIDEKEYBINDING::SetOverrideBinding(void* owner, int32_t priority, const char* command) {
    for (int32_t i = 1; i >= 0; i--) {
        for (OVERRIDEKEYBINDINGENTRY* entry = this->m_lists[i].Head(); entry; entry = this->m_lists[i].Next(entry)) {
            if (entry->m_owner == owner) {
                this->m_freeList.LinkToTail(entry);
                break;
            }
        }
    }

    if (command && *command) {
        OVERRIDEKEYBINDINGENTRY* entry = this->m_freeList.Head();

        if (entry) {
            this->m_lists[priority].LinkToHead(entry);
        } else {
            entry = this->m_lists[priority].NewNode(1, 0, 0);
        }

        entry->m_owner = owner;
        entry->m_binding.m_data[0].m_command.Copy(command);
    }
}

// VERIFY: 124eeff1 manual
// OFFSET: 0x0055D990
int32_t CGUIBindings::AddMetaPrefix(uint32_t modifiers, char*& binding, int32_t& maxLength) {
    if ((modifiers & 0x30) == 0x30) {
        SStrCopy(binding, "ALT-", maxLength);
        binding += 4;
        maxLength -= 4;
    } else if (modifiers & 0x10) {
        SStrCopy(binding, "LALT-", maxLength);
        binding += 5;
        maxLength -= 5;
    } else if (modifiers & 0x20) {
        SStrCopy(binding, "RALT-", maxLength);
        binding += 5;
        maxLength -= 5;
    }

    if (maxLength < 0) {
        return 0;
    }

    if ((modifiers & 0xC) == 0xC) {
        SStrCopy(binding, "CTRL-", maxLength);
        binding += 5;
        maxLength -= 5;
    } else if (modifiers & 0x4) {
        SStrCopy(binding, "LCTRL-", maxLength);
        binding += 6;
        maxLength -= 6;
    } else if (modifiers & 0x8) {
        SStrCopy(binding, "RCTRL-", maxLength);
        binding += 6;
        maxLength -= 6;
    }

    if (maxLength < 0) {
        return 0;
    }

    if ((modifiers & 0x3) == 0x3) {
        SStrCopy(binding, "SHIFT-", maxLength);
        binding += 6;
        maxLength -= 6;
    } else if (modifiers & 0x1) {
        SStrCopy(binding, "LSHIFT-", maxLength);
        binding += 7;
        maxLength -= 7;
    } else if (modifiers & 0x2) {
        SStrCopy(binding, "RSHIFT-", maxLength);
        binding += 7;
        maxLength -= 7;
    }

    return maxLength >= 0;
}

// VERIFY: ab10c553 auto
// OFFSET: 0x0055D860
void CGUIBindings::AddModifiers(uint32_t& modifiers, const char*& binding) {
    while (*binding) {
        if (!SStrCmpI(binding, "LSHIFT", 6)) {
            modifiers |= 0x1;
            binding += 6;
        } else if (!SStrCmpI(binding, "RSHIFT", 6)) {
            modifiers |= 0x2;
            binding += 6;
        } else if (!SStrCmpI(binding, "SHIFT", 5)) {
            modifiers |= 0x3;
            binding += 5;
        } else if (!SStrCmpI(binding, "LCTRL", 5)) {
            modifiers |= 0x4;
            binding += 5;
        } else if (!SStrCmpI(binding, "RCTRL", 5)) {
            modifiers |= 0x8;
            binding += 5;
        } else if (!SStrCmpI(binding, "CTRL", 4)) {
            modifiers |= 0xC;
            binding += 4;
        } else if (!SStrCmpI(binding, "LALT", 4)) {
            modifiers |= 0x10;
            binding += 4;
        } else if (!SStrCmpI(binding, "RALT", 4)) {
            modifiers |= 0x20;
            binding += 4;
        } else if (!SStrCmpI(binding, "ALT", 3)) {
            modifiers |= 0x30;
            binding += 3;
        } else {
            return;
        }

        if (*binding == '-') {
            binding++;
        }
    }
}

// VERIFY: 47da3e12 manual
// OFFSET: 0x005620F0
void CGUIBindings::Initialize() {
    CGUIBindings::s_bindings = new (SMemAlloc(sizeof(CGUIBindings), __FILE__, __LINE__, 0)) CGUIBindings();

    do {
        s_initRound++;
    } while (!s_initRound);
}

// VERIFY: d92b1b48 auto
// OFFSET: 0x0055E2C0
char* CGUIBindings::KeyEventToString(const CKeyEvent& evt, char* binding, int32_t maxLength) {
    uint32_t modifiers = evt.m_data.m_metaKeyState;
    KEY key = evt.m_data.m_key;
    char* result = binding;

    if (evt.m_data.m_repeat > 1) {
        return nullptr;
    }

    if (static_cast<int32_t>(key) <= KEY_LASTMETAKEY) {
        modifiers &= ~(1 << key);
    }

    if (!CGUIBindings::AddMetaPrefix(modifiers, binding, maxLength)) {
        return nullptr;
    }

    const char* name = GetKeyName(key);

    if (!name || !*name || static_cast<int32_t>(SStrLen(name)) >= maxLength) {
        return nullptr;
    }

    SStrCopy(binding, name, STORM_MAX_STR);

    return result;
}

// VERIFY: a69aade2 auto
// OFFSET: 0x005643B0
void CGUIBindings::LoadBindings() {
    void* data;

    if (SFile::ReadFile(nullptr, "WTF\\DefaultBindings.wtf", &data, nullptr, 1, 1, nullptr)) {
        CGUIBindings::LoadBindings(BINDING_SET_DEFAULT, static_cast<const char*>(data));
        SFile::Unload(data);

        XMLNode* config;
        XMLTree* tree = LoadJoystickConfig(config);

        if (tree) {
            XMLNode* node = config->GetChildByName("DefaultBindings");

            if (node) {
                CGUIBindings::LoadBindings(BINDING_SET_DEFAULT, node->m_body);
            }

            XMLTree_Free(tree);
        }
    }

    s_loadPendingMask = 6;
    LoadAccountData(2, CGUIBindings::LoadBindingsCallback, reinterpret_cast<void*>(static_cast<uintptr_t>((s_initRound << 16) | BINDING_SET_ACCOUNT)));
    LoadAccountData(3, CGUIBindings::LoadBindingsCallback, reinterpret_cast<void*>(static_cast<uintptr_t>((s_initRound << 16) | BINDING_SET_CHARACTER)));
}

// VERIFY: f3c499e6 auto
// OFFSET: 0x005641C0
void CGUIBindings::LoadBindings(BINDING_SET set, const char* data) {
    CGUIBindings* bindings = CGUIBindings::s_bindings;

    if (!bindings) {
        return;
    }

    if (set) {
        bindings->CopyBindings(BINDING_SET_DEFAULT, set);
    }

    BINDING_MODE mode = BINDING_MODE_0;
    char line[1024];
    char key[32];

    while (data && *data) {
        SStrTokenize(&data, line, sizeof(line), "\r\n", nullptr);

        const char* text = line;

        while (*text == ' ' || *text == '\t') {
            text++;
        }

        if (!SStrCmpI(text, "BINDINGMODE ", 12)) {
            uint32_t value = SStrToInt(text + 12);

            if (value <= BINDING_MODE_3) {
                mode = static_cast<BINDING_MODE>(value);
            }
        } else if (!SStrCmpI(text, "bind ", 5)) {
            const char* command = text + 5;
            SStrTokenize(&command, key, sizeof(key), " ", nullptr);
            bindings->Bind(set, mode, key, command);
        } else if (!SStrCmpI(text, "modifiedclick ", 14)) {
            const char* action = text + 14;
            SStrTokenize(&action, key, sizeof(key), " ", nullptr);

            if (action) {
                MODIFIEDCLICK* click = bindings->m_modifiedClicks.Ptr(action);

                if (click) {
                    click->SetBinding(set, key);
                }
            }
        }
    }
}

// VERIFY: d999a639 manual
// OFFSET: 0x00564310
void CGUIBindings::LoadBindingsCallback(HOSFILE localFile, HOSFILE file, void* param) {
    uint32_t value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(param));

    if (static_cast<uint16_t>(value >> 16) != s_initRound) {
        return;
    }

    BINDING_SET set = static_cast<BINDING_SET>(value & 0xFFFF);

    if (file != OS_INVALID_FILE) {
        uint32_t size = static_cast<uint32_t>(OsGetFileSize(file));

        if (size) {
            char* buffer = static_cast<char*>(alloca(size + 1));
            uint32_t bytesRead;

            if (OsReadFile(file, buffer, size, &bytesRead)) {
                buffer[bytesRead] = '\0';
                CGUIBindings::LoadBindings(set, buffer);
            }
        }
    }

    s_loadPendingMask &= ~(1 << set);

    if (!s_loadPendingMask && CGUIBindings::s_bindings) {
        CGUIBindings::s_bindings->FinishLoadBindings();
    }
}

// VERIFY: 05b9e548 auto
// OFFSET: 0x0055E340
char* CGUIBindings::MouseEventToString(const CMouseEvent& evt, char* binding, int32_t maxLength) {
    char* result = binding;

    if (!CGUIBindings::AddMetaPrefix(evt.m_data.m_metaKeyState, binding, maxLength)) {
        return nullptr;
    }

    if (evt.m_id == 0x400500C8 || evt.m_id == 0x400500C9) {
        switch (evt.m_data.m_button) {
            case MOUSE_BUTTON_LEFT:
                SStrCopy(binding, "BUTTON1", maxLength);
                return result;

            case MOUSE_BUTTON_MIDDLE:
                SStrCopy(binding, "BUTTON3", maxLength);
                return result;

            case MOUSE_BUTTON_RIGHT:
                SStrCopy(binding, "BUTTON2", maxLength);
                return result;

            default:
                *binding = '\0';

                for (int32_t i = 4; i < 32; i++) {
                    if (evt.m_data.m_button == (1u << (i - 1))) {
                        SStrPrintf(binding, maxLength, "BUTTON%d", i);
                        break;
                    }
                }

                return result;
        }
    }

    if (evt.m_id == 0x400500CD) {
        if (static_cast<int32_t>(evt.m_data.m_wheelAmount) < 0) {
            SStrCopy(binding, "MOUSEWHEELDOWN", maxLength);
        } else {
            SStrCopy(binding, "MOUSEWHEELUP", maxLength);
        }

        return result;
    }

    return nullptr;
}

// VERIFY: c00232fd manual
// OFFSET: 0x0055F3D0
void CGUIBindings::SaveBindings() {
    if (s_loadPendingMask) {
        return;
    }

    if (CGUIBindings::s_bindings->m_dirtySets & (1 << BINDING_SET_ACCOUNT)) {
        SaveAccountData(2, CGUIBindings::SaveBindingsCallback, reinterpret_cast<void*>(BINDING_SET_ACCOUNT));
        SaveAccountData(3, CGUIBindings::SaveBindingsCallback, reinterpret_cast<void*>(BINDING_SET_CHARACTER));
    } else if (CGUIBindings::s_bindings->m_dirtySets & (1 << BINDING_SET_CHARACTER)) {
        SaveAccountData(3, CGUIBindings::SaveBindingsCallback, reinterpret_cast<void*>(BINDING_SET_CHARACTER));
    }
}

// VERIFY: 43abd8af auto
// OFFSET: 0x0055F3A0
void CGUIBindings::SaveBindingsCallback(HOSFILE localFile, HOSFILE file, void* param) {
    BINDING_SET set = static_cast<BINDING_SET>(reinterpret_cast<uintptr_t>(param));

    if (file != OS_INVALID_FILE && (set == BINDING_SET_ACCOUNT || set == CGUIBindings::s_bindings->m_saveSet)) {
        CGUIBindings::s_bindings->SaveBindings(set, file);
    }
}

// VERIFY: 1db2ae9b manual
// OFFSET: 0x00562820
void CGUIBindings::Shutdown() {
    if (CGUIBindings::s_bindings) {
        delete CGUIBindings::s_bindings;
        CGUIBindings::s_bindings = nullptr;
    }

    s_clickFrames.Clear();
    s_initRound++;
}

// VERIFY: 44863edd manual
// OFFSET: 0x0055D740
void CGUIBindings::StripModifiers(uint32_t& modifiers, const char* binding) {
    while (*binding) {
        if (!SStrCmpI(binding, "LSHIFT", 6)) {
            modifiers &= ~0x1u;
            binding += 6;
        } else if (!SStrCmpI(binding, "RSHIFT", 6)) {
            modifiers &= ~0x2u;
            binding += 6;
        } else if (!SStrCmpI(binding, "SHIFT", 5)) {
            modifiers &= ~0x3u;
            binding += 5;
        } else if (!SStrCmpI(binding, "LCTRL", 5)) {
            modifiers &= ~0x4u;
            binding += 5;
        } else if (!SStrCmpI(binding, "RCTRL", 5)) {
            modifiers &= ~0x8u;
            binding += 5;
        } else if (!SStrCmpI(binding, "CTRL", 4)) {
            modifiers &= ~0xCu;
            binding += 4;
        } else if (!SStrCmpI(binding, "LALT", 4)) {
            modifiers &= ~0x10u;
            binding += 4;
        } else if (!SStrCmpI(binding, "RALT", 4)) {
            modifiers &= ~0x20u;
            binding += 4;
        } else if (!SStrCmpI(binding, "ALT", 3)) {
            modifiers &= ~0x30u;
            binding += 3;
        } else {
            return;
        }

        if (*binding == '-') {
            binding++;
        }
    }
}

// VERIFY: 58990057 manual
// OFFSET: 0x00561B80
CGUIBindings::CGUIBindings() {
    this->m_numCommands = 0;
    this->m_numHiddenCommands = 0;
    this->m_dirtySets = 0;
    this->m_keyStateOverride = 0;
    this->m_heldModifiers = 0;
    this->m_bindingModeMask = 0;
    this->m_bindingModeToggles = 0;
}

// VERIFY: 1ca0f84e manual
// OFFSET: 0x00561C60
CGUIBindings::~CGUIBindings() {
    this->m_commands.Clear();

    for (int32_t i = 0; i < NUM_BINDING_SETS; i++) {
        this->m_bindings[i].Clear();
    }

    this->m_overrideBindings.Clear();
    this->m_modifiedClicks.Clear();
}

// VERIFY: 407933af manual
// OFFSET: 0x0055F430
void CGUIBindings::AdjustCommandKeyIndices(BINDING_SET set, BINDING_MODE mode, const char* command, int32_t index) {
    for (KEYBINDING* binding = this->m_bindings[set].m_fulllist.Head(); binding; binding = this->m_bindings[set].m_fulllist.Next(binding)) {
        if (this->GetBindingCommand(binding, mode) && !SStrCmpI(this->GetBindingCommand(binding, mode), command, STORM_MAX_STR) && this->GetBindingIndex(binding, mode) > index) {
            binding->m_data[mode].m_index--;
        }
    }
}

// VERIFY: db6813cd manual
// OFFSET: 0x00562ED0
int32_t CGUIBindings::Bind(BINDING_SET set, BINDING_MODE mode, const char* key, const char* command) {
    if (!CGGameUI::CanPerformAction(13)) {
        return 0;
    }

    if (!key) {
        return 0;
    }

    if (!command || !*command) {
        command = "NONE";
    }

    if (!SStrCmpI(key, "LEFTBRACKET", STORM_MAX_STR)) {
        s_bindKey[0] = '[';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "RIGHTBRACKET", STORM_MAX_STR)) {
        s_bindKey[0] = ']';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "SLASH", STORM_MAX_STR)) {
        s_bindKey[0] = '/';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "BACKSLASH", STORM_MAX_STR)) {
        s_bindKey[0] = '\\';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "SEMICOLON", STORM_MAX_STR)) {
        s_bindKey[0] = ';';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "APOSTROPHE", STORM_MAX_STR)) {
        s_bindKey[0] = '\'';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "COMMA", STORM_MAX_STR)) {
        s_bindKey[0] = ',';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "PERIOD", STORM_MAX_STR)) {
        s_bindKey[0] = '.';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "TILDE", STORM_MAX_STR)) {
        s_bindKey[0] = '`';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "PLUS", STORM_MAX_STR)) {
        s_bindKey[0] = '=';
        key = s_bindKey;
    } else if (!SStrCmpI(key, "MINUS", STORM_MAX_STR)) {
        s_bindKey[0] = '-';
        key = s_bindKey;
    }

    if (!IsValidKeyName(key)) {
        return 0;
    }

    KEYBINDING* binding = this->m_bindings[set].Ptr(key);

    if (!binding) {
        binding = this->m_bindings[set].New(key, 0, 0);
    }

    if (set) {
        binding->m_flags &= ~1u;
    } else {
        binding->m_flags |= 1u;
    }

    if (!this->GetBindingCommand(binding, mode) || !command || SStrCmp(this->GetBindingCommand(binding, mode), command, STORM_MAX_STR)) {
        if (this->GetBindingCommand(binding, mode)) {
            this->AdjustCommandKeyIndices(set, mode, this->GetBindingCommand(binding, mode), this->GetBindingIndex(binding, mode));
        }

        int32_t index = this->GetNumCommandKeys(set, mode, command);
        binding->m_data[mode].m_command.Copy(command);
        binding->m_data[mode].m_index = index;
    }

    return 1;
}

// VERIFY: ede45eb3 auto
// OFFSET: 0x005604F0
void CGUIBindings::ClearOverrideBindings(void* owner, bool checkPermission) {
    if (checkPermission && !CGGameUI::CanPerformAction(13)) {
        return;
    }

    for (OVERRIDEKEYBINDING* binding = this->m_overrideBindings.m_fulllist.Head(); binding; binding = this->m_overrideBindings.m_fulllist.Next(binding)) {
        binding->SetOverrideBinding(owner, 0, nullptr);
    }
}

// VERIFY: 62581540 manual
// OFFSET: 0x00562990
CSimpleFrame* GetClickFrame(lua_State* L, const char* name, uint32_t length) {
    char* buffer = static_cast<char*>(alloca(length + 1));
    memcpy(buffer, name, length);
    buffer[length] = '\0';
    CLICKFRAME* entry = s_clickFrames.Ptr(buffer);

    if (!entry) {
        lua_taintexpected++;
        CSimpleFrame* frame = nullptr;
        const char* savedTaint = lua_tainted;
        lua_pushstring(L, buffer);
        lua_rawget(L, LUA_GLOBALSINDEX);

        if (lua_type(L, -1) == LUA_TTABLE) {
            lua_rawgeti(L, -1, 0);
            frame = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
            lua_settop(L, -2);

            if (frame) {
                if (!frame->IsA(CSimpleFrame::GetObjectType())) {
                    frame = nullptr;
                } else {
                    const char* frameName = frame->GetName();

                    if (!frameName) {
                        frameName = "<unnamed>";
                    }

                    if (SStrCmp(frameName, buffer, STORM_MAX_STR)) {
                        frame = nullptr;
                    }
                }
            }
        }

        lua_settop(L, -2);

        if (lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = savedTaint;
        }

        if (--lua_taintexpected <= 0) {
            lua_taintexpected = 0;
        }

        if (!frame) {
            return nullptr;
        }

        entry = s_clickFrames.New(buffer, 0, 0);
        entry->m_frame = frame;
    }

    return entry->m_frame;
}

// VERIFY: d1b98282 manual
// OFFSET: 0x00562B80
void CGUIBindings::CopyBindings(BINDING_SET source, BINDING_SET dest) {
    if (!CGGameUI::CanPerformAction(13)) {
        return;
    }

    while (source > BINDING_SET_DEFAULT && !this->m_bindings[source].m_fulllist.Head()) {
        source = static_cast<BINDING_SET>(source - 1);
    }

    this->m_bindings[dest].Clear();

    for (KEYBINDING* binding = this->m_bindings[source].m_fulllist.Head(); binding; binding = this->m_bindings[source].m_fulllist.Next(binding)) {
        KEYBINDING* copy = this->m_bindings[dest].Ptr(binding->m_key.m_str);

        if (!copy) {
            copy = this->m_bindings[dest].New(SStrHashHT(binding->m_key.m_str), binding->m_key.m_str, 0, 0);
        }

        *copy = *binding;
    }

    for (MODIFIEDCLICK* click = this->m_modifiedClicks.m_fulllist.Head(); click; click = this->m_modifiedClicks.m_fulllist.Next(click)) {
        click->m_data[dest] = click->m_data[source];
    }

    this->m_dirtySets |= 1 << dest;

    if (dest == BINDING_SET_CURRENT) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
    }
}

// VERIFY: 9ac6f481 auto
// OFFSET: 0x0055E550
void CGUIBindings::EnableBindingMode(BINDING_MODE mode, int32_t enable) {
    if (!mode) {
        return;
    }

    uint32_t bit = 1 << mode;

    if (enable == static_cast<int32_t>(bit & this->m_bindingModeMask)) {
        return;
    }

    if (!CGGameUI::CanPerformAction(13)) {
        return;
    }

    uint32_t active = bit & (this->m_bindingModeMask ^ this->m_bindingModeToggles);

    if (enable) {
        this->m_bindingModeMask |= bit;
    } else {
        this->m_bindingModeMask &= ~bit;
    }

    this->m_bindingModeToggles &= ~bit;

    if (active != (bit & (this->m_bindingModeToggles ^ this->m_bindingModeMask))) {
        FrameScript_SignalEvent(SCRIPT_EVENT_UPDATE_BINDINGS, nullptr);
    }
}

// VERIFY: 3bf1ffd2 manual
// OFFSET: 0x0055F860
int32_t CGUIBindings::ExecCommand(const char* command, int32_t isDown, float pressure, int32_t allowPressure, int32_t isPressure, int32_t isAngle, float angle, float precision, const char* taint) {
    if (!command || !*command) {
        return 0;
    }

    KEYCOMMAND* keyCommand = this->m_commands.Ptr(command);

    if (!keyCommand) {
        return 0;
    }

    if (!isDown && !keyCommand->m_runOnUp) {
        return 0;
    }

    if (keyCommand->m_pressure) {
        if (!allowPressure) {
            return 0;
        }
    } else if (isPressure) {
        return 0;
    }

    if (keyCommand->m_angle != isAngle) {
        return 0;
    }

    if (keyCommand->m_function != -1) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, isDown ? "down" : "up");
        lua_pushnumber(L, pressure);
        lua_pushnumber(L, angle);
        lua_pushnumber(L, precision);
        FrameScript_Execute(keyCommand->m_function, nullptr, 4, taint, nullptr);
    }

    return 1;
}

// VERIFY: 9242eb88 manual
// OFFSET: 0x00563150
int32_t CGUIBindings::ExecKey(uint32_t modifiers, const char* key, int32_t isDown, int32_t allowPressure, BINDING_MODE mode) {
    char reducedKey[128];
    KEYBINDING* binding = this->GetReducedKeyBinding(mode, key, reducedKey, sizeof(reducedKey));

    if (!binding) {
        return 0;
    }

    this->m_keyStateOverride = 1;
    this->m_heldModifiers = modifiers;
    CGUIBindings::StripModifiers(this->m_heldModifiers, reducedKey);

    CSimpleTop* top = CSimpleTop::s_instance;
    top->unused_1250 = 1;

    int32_t result = 0;
    const char* command = this->GetBindingCommand(binding, mode);

    if (!SStrCmpI(command, "SPELL ", 6)) {
        if (!isDown) {
            int32_t spell = IsValidSpell(command + 6, &isDown);

            if (spell >= 0) {
                WOWGUID target = {};

                if (this->IsModifiedClick("SELFCAST")) {
                    target = ClntObjMgrGetActivePlayer();
                } else if (this->IsModifiedClick("FOCUSCAST")) {
                    target = CGGameUI::m_focusTarget;
                }

                CGSpellBook::CastSpell(spell, isDown, target);
            }
        }

        result = 1;
    } else if (!SStrCmpI(command, "ITEM ", 5)) {
        CGPlayer_C* player;
        CGItem_C* item;

        if (!isDown && (player = ClntObjMgrGetActivePlayerObj()) != nullptr && (item = player->m_bag.FindItemByName(command + 5, 0)) != nullptr) {
            WOWGUID target = {};

            if (this->IsModifiedClick("SELFCAST")) {
                target = ClntObjMgrGetActivePlayer();
            } else if (this->IsModifiedClick("FOCUSCAST")) {
                target = CGGameUI::m_focusTarget;
            }

            item->UseItem(&target, 0);
        }

        result = 1;
    } else if (!SStrCmpI(command, "MACRO ", 6)) {
        const char* name = command + 6;

        if (!isDown) {
            const char* digit = name;

            while (*digit >= '0' && *digit <= '9') {
                digit++;
            }

            MACRONODE* macro = *name && !*digit ? CGUIMacros::GetMacroByIndex(SStrToInt(name) - 1) : CGUIMacros::FindMacro(name);

            if (macro) {
                CGUIMacros::RunMacro(macro, "LeftButton");
            }
        }

        result = 1;
    } else if (!SStrCmpI(command, "CLICK ", 6)) {
        const char* name = command + 6;
        const char* separator = SStrChr(name, ':');

        if (name && separator) {
            CSimpleFrame* frame = GetClickFrame(FrameScript_GetContext(), name, separator - name);

            if (frame && frame != CGWorldFrame::s_currentWorldFrame) {
                const char* button = separator + 1;
                const char* macroRunning = CGUIMacros::m_macroRunning;

                CMouseEvent evt;
                C2Vector position;
                frame->m_simpleTop->GetCursorPosition(position);
                evt.m_data.m_mousePosition = position;
                evt.m_data.m_button = StringToMouseButton(button);

                CGUIMacros::m_macroRunning = button;

                if (isDown) {
                    frame->OnLayerMouseDown(evt, button);
                } else {
                    frame->OnLayerMouseUp(evt, button);
                }

                CGUIMacros::m_macroRunning = macroRunning;
                result = 1;
            }
        }
    } else {
        result = this->ExecCommand(command, isDown, isDown ? 1.0f : 0.0f, allowPressure, 0, 0, -1.0f, 0.0f, nullptr);
    }

    top->unused_1250 = 0;
    this->m_keyStateOverride = 0;

    return result;
}

// VERIFY: 6088c7f1 manual
// OFFSET: 0x00562D60
void CGUIBindings::FinishLoadBindings() {
    this->m_dirtySets = 0;
    this->m_saveSet = this->m_bindings[BINDING_SET_CHARACTER].m_fulllist.Head() ? BINDING_SET_CHARACTER : BINDING_SET_ACCOUNT;
    this->CopyBindings(static_cast<BINDING_SET>(this->m_saveSet), BINDING_SET_CURRENT);

    if (this->GetCommandKey(BINDING_MODE_0, "TOGGLEMOUSE", 0)) {
        CGInputControl* inputControl = CGInputControl::GetActive();
        inputControl->m_mouseModeFlags &= ~0x2u;
        inputControl->UpdateMouseMode(0);
    }
}

// VERIFY: 43101d83 manual
// OFFSET: 0x0055E470
const char* CGUIBindings::GetBindingCommand(KEYBINDING* binding, BINDING_MODE mode) {
    if (mode != BINDING_MODE_ACTIVE) {
        return binding->m_data[mode].m_command.m_str;
    }

    for (int32_t i = NUM_BINDING_MODES - 1; i >= 0; i--) {
        if ((!i || ((1 << i) & (this->m_bindingModeMask ^ this->m_bindingModeToggles))) && binding->m_data[i].m_command.m_str) {
            return binding->m_data[i].m_command.m_str;
        }
    }

    return nullptr;
}

// VERIFY: 1a473fbd manual
// OFFSET: 0x0055E4E0
int32_t CGUIBindings::GetBindingIndex(KEYBINDING* binding, BINDING_MODE mode) {
    if (mode != BINDING_MODE_ACTIVE) {
        return binding->m_data[mode].m_index;
    }

    for (int32_t i = NUM_BINDING_MODES - 1; i >= 0; i--) {
        if ((!i || ((1 << i) & (this->m_bindingModeMask ^ this->m_bindingModeToggles))) && binding->m_data[i].m_command.m_str) {
            return binding->m_data[i].m_index;
        }
    }

    return -1;
}

// VERIFY: c27df5c9 manual
// OFFSET: 0x0055E700
void CGUIBindings::GetCommand(int32_t index, const char** command) {
    for (KEYCOMMAND* keyCommand = this->m_commands.m_fulllist.Head(); keyCommand; keyCommand = this->m_commands.m_fulllist.Next(keyCommand)) {
        if (keyCommand->m_index == index) {
            *command = keyCommand->m_key.m_str;
            return;
        }
    }
}

// VERIFY: 1f7084ce auto
// OFFSET: 0x00562490
const char* CGUIBindings::GetCommandAction(BINDING_MODE mode, const char* key, int32_t checkOverride) {
    if (!key || !*key) {
        return nullptr;
    }

    KEYBINDING* binding = nullptr;

    if (checkOverride) {
        OVERRIDEKEYBINDING* overrideBinding = this->m_overrideBindings.Ptr(key);

        if (overrideBinding) {
            binding = overrideBinding->GetActiveBinding();
        }
    }

    if (!binding) {
        binding = this->m_bindings[BINDING_SET_CURRENT].Ptr(key);

        if (!binding) {
            return nullptr;
        }
    }

    const char* command = this->GetBindingCommand(binding, mode);

    if (!command) {
        return nullptr;
    }

    if (!SStrCmpI(command, "SPELL ", 6) || !SStrCmpI(command, "ITEM ", 5) || !SStrCmpI(command, "MACRO ", 6) || !SStrCmpI(command, "CLICK ", 6) || this->m_commands.Ptr(command)) {
        return command;
    }

    return nullptr;
}

// VERIFY: 56d73fa6 auto
// OFFSET: 0x0055E750
const char* CGUIBindings::GetCommandKey(BINDING_MODE mode, const char* command, int32_t index) {
    for (KEYBINDING* binding = this->m_bindings[BINDING_SET_CURRENT].m_fulllist.Head(); binding; binding = this->m_bindings[BINDING_SET_CURRENT].m_fulllist.Next(binding)) {
        if (this->GetBindingCommand(binding, mode) && !SStrCmpI(this->GetBindingCommand(binding, mode), command, STORM_MAX_STR) && this->GetBindingIndex(binding, mode) == index) {
            return binding->m_key.m_str;
        }
    }

    return nullptr;
}

// VERIFY: 78943d44 auto
// OFFSET: 0x00562140
KEYBINDING* CGUIBindings::GetKeyBinding(BINDING_MODE mode, const char* key) {
    if (!key || !*key) {
        return nullptr;
    }

    KEYBINDING* binding = nullptr;
    OVERRIDEKEYBINDING* overrideBinding = this->m_overrideBindings.Ptr(key);

    if (overrideBinding) {
        binding = overrideBinding->GetActiveBinding();
    }

    if (!binding) {
        binding = this->m_bindings[BINDING_SET_CURRENT].Ptr(key);

        if (!binding) {
            if (!SStrChr(key, '-')) {
                return nullptr;
            }

            uint32_t modifiers = 0;
            CGUIBindings::AddModifiers(modifiers, key);

            if (modifiers & 0x3) {
                modifiers |= 0x3;
            }

            if (modifiers & 0xC) {
                modifiers |= 0xC;
            }

            if (modifiers & 0x30) {
                modifiers |= 0x30;
            }

            char genericKey[128];
            char* buffer = genericKey;
            int32_t maxLength = sizeof(genericKey);
            CGUIBindings::AddMetaPrefix(modifiers, buffer, maxLength);
            SStrCopy(buffer, key, maxLength);

            overrideBinding = this->m_overrideBindings.Ptr(genericKey);

            if (overrideBinding) {
                binding = overrideBinding->GetActiveBinding();
            }

            if (!binding) {
                binding = this->m_bindings[BINDING_SET_CURRENT].Ptr(genericKey);

                if (!binding) {
                    return nullptr;
                }
            }
        }
    }

    const char* command = this->GetBindingCommand(binding, mode);

    if (!command) {
        return nullptr;
    }

    if (SStrCmpI(command, "SPELL ", 6) && SStrCmpI(command, "ITEM ", 5) && SStrCmpI(command, "MACRO ", 6) && SStrCmpI(command, "CLICK ", 6) && !this->m_commands.Ptr(command)) {
        return nullptr;
    }

    return binding;
}

// VERIFY: e14318ed manual
// OFFSET: 0x0055E870
const char* CGUIBindings::GetModifiedClick(int32_t index) {
    int32_t i = 0;

    for (MODIFIEDCLICK* click = this->m_modifiedClicks.m_fulllist.Head(); click; click = this->m_modifiedClicks.m_fulllist.Next(click), i++) {
        if (i == index) {
            return click->m_key.m_str;
        }
    }

    return nullptr;
}

// VERIFY: dc978c8a manual
// OFFSET: 0x0055E7E0
int32_t CGUIBindings::GetNumCommandKeys(BINDING_SET set, BINDING_MODE mode, const char* command) {
    int32_t count = 0;

    for (KEYBINDING* binding = this->m_bindings[set].m_fulllist.Head(); binding; binding = this->m_bindings[set].m_fulllist.Next(binding)) {
        if (this->GetBindingCommand(binding, mode) && !SStrCmpI(this->GetBindingCommand(binding, mode), command, STORM_MAX_STR)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 59504db4 auto
// OFFSET: 0x005622E0
KEYBINDING* CGUIBindings::GetReducedKeyBinding(BINDING_MODE mode, const char* key, char* reducedKey, uint32_t maxLength) {
    char modifiers[3][32];
    uint32_t numModifiers = 0;

    while (numModifiers < 3) {
        const char* separator = SStrChr(key, '-');

        if (!separator || separator == key) {
            break;
        }

        SStrCopy(modifiers[numModifiers], key, sizeof(modifiers[numModifiers]));
        modifiers[numModifiers][separator - key + 1] = '\0';
        numModifiers++;
        key = separator + 1;
    }

    uint32_t combinations[4][9] = {
        { 1, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 2, 1, 0, 0, 0, 0, 0, 0, 0 },
        { 4, 3, 2, 1, 0, 0, 0, 0, 0 },
        { 8, 7, 6, 5, 3, 4, 2, 1, 0 },
    };

    uint32_t numCombinations = combinations[numModifiers][0];

    for (uint32_t i = 0; i < numCombinations; i++) {
        *reducedKey = '\0';

        for (uint32_t j = 0; j < numModifiers; j++) {
            if ((1 << j) & combinations[numModifiers][i + 1]) {
                SStrPack(reducedKey, modifiers[j], maxLength);
            }
        }

        SStrPack(reducedKey, key, maxLength);

        KEYBINDING* binding = this->GetKeyBinding(mode, reducedKey);

        if (binding) {
            return binding;
        }
    }

    return nullptr;
}

// VERIFY: ced91a43 manual
// OFFSET: 0x0055F940
int32_t CGUIBindings::IsModifiedClick(const char* action) {
    int32_t result = 0;

    if (!action) {
        for (int32_t key = KEY_LSHIFT; key <= KEY_RALT; key++) {
            if (CGGameUI::IsKeyDown(static_cast<KEY>(key))) {
                return 1;
            }
        }

        for (MODIFIEDCLICK* click = this->m_modifiedClicks.m_fulllist.Head(); click; click = this->m_modifiedClicks.m_fulllist.Next(click)) {
            if (click->IsActive(BINDING_SET_CURRENT)) {
                return 1;
            }
        }

        return result;
    }

    uint32_t modifiers = 0;
    CGUIBindings::AddModifiers(modifiers, action);

    if (!modifiers) {
        MODIFIEDCLICK* click = this->m_modifiedClicks.Ptr(action);

        if (click) {
            result = click->IsActive(BINDING_SET_CURRENT);
        }

        return result;
    }

    result = 1;

    if ((modifiers & 0x3) == 0x1) {
        if (!CGGameUI::IsKeyDown(KEY_LSHIFT)) {
            result = 0;
        }
    } else if ((modifiers & 0x3) == 0x2 || ((modifiers & 0x3) == 0x3 && !CGGameUI::IsKeyDown(KEY_LSHIFT))) {
        if (!CGGameUI::IsKeyDown(KEY_RSHIFT)) {
            result = 0;
        }
    }

    if ((modifiers & 0xC) == 0x4) {
        if (!CGGameUI::IsKeyDown(KEY_LCONTROL)) {
            result = 0;
        }
    } else if ((modifiers & 0xC) == 0x8 || ((modifiers & 0xC) == 0xC && !CGGameUI::IsKeyDown(KEY_LCONTROL))) {
        if (!CGGameUI::IsKeyDown(KEY_RCONTROL)) {
            result = 0;
        }
    }

    if ((modifiers & 0x30) == 0x10) {
        if (!CGGameUI::IsKeyDown(KEY_LALT)) {
            result = 0;
        }
    } else if ((modifiers & 0x30) == 0x20 || ((modifiers & 0x30) == 0x30 && !CGGameUI::IsKeyDown(KEY_LALT))) {
        if (!CGGameUI::IsKeyDown(KEY_RALT)) {
            result = 0;
        }
    }

    return result;
}

// VERIFY: ff16c8cb manual
// OFFSET: 0x00564760
int32_t CGUIBindings::Load(const char* file, MD5_CTX* md5, CStatus* status) {
    if (!status) {
        status = &s_bindingsStatus;
    }

    void* data;
    uint32_t size;

    if (!SFile::ReadFile(nullptr, file, &data, &size, 0, 1, nullptr)) {
        status->Add(STATUS_ERROR, "Couldn't open %s", file);
        return 0;
    }

    MD5Update(md5, static_cast<const uint8_t*>(data), size);
    XMLTree* tree = XMLTree_Load(static_cast<const char*>(data), size);
    SMemFree(data, __FILE__, __LINE__, 0);

    if (!tree) {
        status->Add(STATUS_ERROR, "Couldn't parse XML in %s", file);
        return 0;
    }

    for (XMLNode* node = XMLTree_GetRoot(tree)->m_child; node; node = node->m_next) {
        if (!SStrCmpI(node->m_name.m_str, "Binding", STORM_MAX_STR)) {
            this->LoadBinding(file, node, status);
        } else if (!SStrCmpI(node->m_name.m_str, "ModifiedClick", STORM_MAX_STR)) {
            this->LoadModifiedClick(file, node, status);
        } else {
            status->Add(STATUS_WARNING, "Unknown node type %s in %s", node->m_name.m_str, file);
        }
    }

    XMLTree_Free(tree);

    return 1;
}

// VERIFY: d9323b50 manual
// OFFSET: 0x00564470
void CGUIBindings::LoadBinding(const char* file, XMLNode* node, CStatus* status) {
    const char* name = node->GetAttributeByName("name");
    const char* body = node->m_body;

    if (!name || !*name) {
        status->Add(STATUS_WARNING, "Found binding with no name in %s", file);
        return;
    }

    const char* debug = node->GetAttributeByName("debug");

    if (debug && StringToBOOL(debug)) {
        return;
    }

    const char* platform = node->GetAttributeByName("platform");

    if (platform && SStrCmpI(platform, "windows", STORM_MAX_STR)) {
        return;
    }

    if (this->m_commands.Ptr(name)) {
        status->Add(STATUS_WARNING, "Binding %s is defined more than once in %s", name, file);
        return;
    }

    const char* header = node->GetAttributeByName("header");

    if (header && *header) {
        char headerName[1024];
        SStrPrintf(headerName, sizeof(headerName), "HEADER_%s", header);

        if (this->m_commands.Ptr(headerName)) {
            status->Add(STATUS_WARNING, "Binding header %s is defined more than once in %s", header, file);
        } else {
            KEYCOMMAND* headerCommand = this->m_commands.New(headerName, 0, 0);
            headerCommand->m_index = this->m_numCommands++;
            headerCommand->m_function = -1;
        }
    }

    KEYCOMMAND* command = this->m_commands.New(SStrHashHT(name), name, 0, 0);

    const char* hidden = node->GetAttributeByName("hidden");
    const char* joystick = node->GetAttributeByName("joystick");

    if ((hidden && StringToBOOL(hidden)) || (joystick && StringToBOOL(joystick) && GetJoystick() == 0xFFFFFFFF)) {
        this->m_numHiddenCommands++;
        command->m_index = -this->m_numHiddenCommands;
    } else {
        command->m_index = this->m_numCommands++;
    }

    if (body && *body) {
        command->m_function = FrameScript_CompileFunction(name, "return function(keystate, pressure, angle, precision) %s end", body, status);
    } else {
        command->m_function = -1;
    }

    const char* runOnUp = node->GetAttributeByName("runOnUp");
    command->m_runOnUp = runOnUp ? StringToBOOL(runOnUp) : 0;

    const char* pressure = node->GetAttributeByName("pressure");
    command->m_pressure = pressure ? StringToBOOL(pressure) : 0;

    const char* angle = node->GetAttributeByName("angle");
    command->m_angle = angle ? StringToBOOL(angle) : 0;

    const char* key = node->GetAttributeByName("default");

    if (key && *key && !this->m_bindings[BINDING_SET_DEFAULT].Ptr(key)) {
        this->Bind(BINDING_SET_DEFAULT, BINDING_MODE_0, key, name);
    }
}

// VERIFY: fc4c9587 manual
// OFFSET: 0x00562DD0
void CGUIBindings::LoadModifiedClick(const char* file, XMLNode* node, CStatus* status) {
    const char* action = node->GetAttributeByName("action");

    if (!action || !*action) {
        status->Add(STATUS_WARNING, "Found modified click with no action in %s", file);
        return;
    }

    if (this->m_modifiedClicks.Ptr(action)) {
        status->Add(STATUS_WARNING, "Modified click %s is defined more than once in %s", action, file);
        return;
    }

    MODIFIEDCLICK* click = this->m_modifiedClicks.New(SStrHashHT(action), action, 0, 0);
    this->m_numModifiedClicks++;

    const char* binding = node->GetAttributeByName("default");

    if (binding && *binding) {
        click->SetBinding(BINDING_SET_DEFAULT, binding);
    }
}

// VERIFY: df565680 manual
// OFFSET: 0x0055F1B0
void CGUIBindings::SaveBindings(BINDING_SET set, HOSFILE file) {
    char buffer[1024];
    uint32_t written;

    for (int32_t mode = 0; mode < NUM_BINDING_MODES; mode++) {
        int32_t first = 1;

        for (KEYBINDING* binding = this->m_bindings[set].m_fulllist.Head(); binding; binding = this->m_bindings[set].m_fulllist.Next(binding)) {
            if (binding->m_flags & 1) {
                continue;
            }

            const char* command = binding->m_data[mode].m_command.m_str;

            if (!command) {
                continue;
            }

            if (first) {
                SStrPrintf(buffer, sizeof(buffer), "BINDINGMODE %d\r\n", mode);
                OsWriteFile(file, buffer, SStrLen(buffer), &written);
                first = 0;
            }

            SStrPrintf(buffer, sizeof(buffer), "bind %s %s\r\n", binding->m_key.m_str, command);
            OsWriteFile(file, buffer, SStrLen(buffer), &written);
        }
    }

    for (MODIFIEDCLICK* click = this->m_modifiedClicks.m_fulllist.Head(); click; click = this->m_modifiedClicks.m_fulllist.Next(click)) {
        char binding[128];
        uint32_t flags;
        click->GetBinding(set, binding, sizeof(binding), &flags);

        if (!(flags & 1)) {
            SStrPrintf(buffer, sizeof(buffer), "modifiedclick %s %s\r\n", binding, click->m_key.m_str);
            OsWriteFile(file, buffer, SStrLen(buffer), &written);
        }
    }
}

// VERIFY: 39c35a34 manual
// OFFSET: 0x00563480
void CGUIBindings::SetOverrideBinding(void* owner, int32_t priority, const char* key, const char* command, bool checkPermission) {
    if (checkPermission && !CGGameUI::CanPerformAction(13)) {
        return;
    }

    OVERRIDEKEYBINDING* binding = this->m_overrideBindings.Ptr(key);

    if (!binding) {
        binding = this->m_overrideBindings.New(SStrHashHT(key), key, 0, 0);
    }

    binding->SetOverrideBinding(owner, priority, command);
}

// VERIFY: 5d989985 manual
// OFFSET: 0x0055DF30
int32_t MouseButtonToIndex(uint32_t button) {
    switch (button) {
        case 0x1:
            return 1;
        case 0x2:
            return 3;
        case 0x4:
            return 2;
        case 0x8:
            return 4;
        case 0x10:
            return 5;
        case 0x20:
            return 6;
        case 0x40:
            return 7;
        case 0x80:
            return 8;
        case 0x100:
            return 9;
        case 0x200:
            return 10;
        case 0x400:
            return 11;
        case 0x800:
            return 12;
        case 0x1000:
            return 13;
        case 0x2000:
            return 14;
        case 0x4000:
            return 15;
        case MOUSE_BUTTON_XBUTTON13:
            return 16;
        case MOUSE_BUTTON_XBUTTON14:
            return 17;
        case MOUSE_BUTTON_XBUTTON15:
            return 18;
        case MOUSE_BUTTON_XBUTTON16:
            return 19;
        case MOUSE_BUTTON_XBUTTON17:
            return 20;
        case MOUSE_BUTTON_XBUTTON18:
            return 21;
        case MOUSE_BUTTON_XBUTTON19:
            return 22;
        case MOUSE_BUTTON_XBUTTON20:
            return 23;
        case MOUSE_BUTTON_XBUTTON21:
            return 24;
        case MOUSE_BUTTON_XBUTTON22:
            return 25;
        case MOUSE_BUTTON_XBUTTON23:
            return 26;
        case MOUSE_BUTTON_XBUTTON24:
            return 27;
        case MOUSE_BUTTON_XBUTTON25:
            return 28;
        case MOUSE_BUTTON_XBUTTON26:
            return 29;
        case MOUSE_BUTTON_XBUTTON27:
            return 30;
        case MOUSE_BUTTON_XBUTTON28:
            return 31;
        default:
            return 0;
    }
}

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
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON13);
    }

    if (!SStrCmpI(string, "Button17", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON14);
    }

    if (!SStrCmpI(string, "Button18", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON15);
    }

    if (!SStrCmpI(string, "Button19", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON16);
    }

    if (!SStrCmpI(string, "Button20", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON17);
    }

    if (!SStrCmpI(string, "Button21", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON18);
    }

    if (!SStrCmpI(string, "Button22", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON19);
    }

    if (!SStrCmpI(string, "Button23", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON20);
    }

    if (!SStrCmpI(string, "Button24", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON21);
    }

    if (!SStrCmpI(string, "Button25", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON22);
    }

    if (!SStrCmpI(string, "Button26", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON23);
    }

    if (!SStrCmpI(string, "Button27", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON24);
    }

    if (!SStrCmpI(string, "Button28", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON25);
    }

    if (!SStrCmpI(string, "Button29", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON26);
    }

    if (!SStrCmpI(string, "Button30", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON27);
    }

    if (!SStrCmpI(string, "Button31", STORM_MAX_STR)) {
        return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON28);
    }

    return MOUSE_BUTTON_NONE;
}

// VERIFY: d3396ade manual
// OFFSET: 0x005EEC20
MOUSEBUTTON IndexToMouseButton(uint32_t index) {
    switch (index) {
        case 1:
            return static_cast<MOUSEBUTTON>(0x1);

        case 2:
            return static_cast<MOUSEBUTTON>(0x4);

        case 3:
            return static_cast<MOUSEBUTTON>(0x2);

        case 4:
            return static_cast<MOUSEBUTTON>(0x8);

        case 5:
            return static_cast<MOUSEBUTTON>(0x10);

        case 6:
            return static_cast<MOUSEBUTTON>(0x20);

        case 7:
            return static_cast<MOUSEBUTTON>(0x40);

        case 8:
            return static_cast<MOUSEBUTTON>(0x80);

        case 9:
            return static_cast<MOUSEBUTTON>(0x100);

        case 10:
            return static_cast<MOUSEBUTTON>(0x200);

        case 11:
            return static_cast<MOUSEBUTTON>(0x400);

        case 12:
            return static_cast<MOUSEBUTTON>(0x800);

        case 13:
            return static_cast<MOUSEBUTTON>(0x1000);

        case 14:
            return static_cast<MOUSEBUTTON>(0x2000);

        case 15:
            return static_cast<MOUSEBUTTON>(0x4000);

        case 16:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON13);

        case 17:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON14);

        case 18:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON15);

        case 19:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON16);

        case 20:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON17);

        case 21:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON18);

        case 22:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON19);

        case 23:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON20);

        case 24:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON21);

        case 25:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON22);

        case 26:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON23);

        case 27:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON24);

        case 28:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON25);

        case 29:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON26);

        case 30:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON27);

        case 31:
            return static_cast<MOUSEBUTTON>(MOUSE_BUTTON_XBUTTON28);

        default:
            return MOUSE_BUTTON_NONE;
    }
}

