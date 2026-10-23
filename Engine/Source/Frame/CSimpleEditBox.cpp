#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleEditBox.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Event/CCharEvent.h"
#include "Engine/Source/Event/CEvent.h"
#include "Engine/Source/Event/CImeEvent.h"
#include "Engine/Source/Event/CKeyEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleMessageFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gxu/GxuFont.h"
#include "Engine/Source/Os/Input.h"
#include "Engine/Source/Os/OsClipboard.h"
#include "Engine/Source/Os/OsIME.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00B2D3E0
int32_t CSimpleEditBox::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE474
CSimpleEditBox* CSimpleEditBox::s_currentFocus;

// STATIC: 0x00DCE470
int32_t CSimpleEditBox::s_objectType;

// STATIC: 0x00B2CBDC
const char* CSimpleEditBox::s_objectTypeName = "EditBox";

// STATIC: 0x00B2CBE0
const char* CSimpleEditBox::s_inputLanguageNames[4] = { "ROMAN", "KOREAN", "CHINESE", "JAPANESE" };

// STATIC: 0x00DCE478
CDataAllocator CSimpleEditBox::s_allocator(sizeof(CSimpleEditBox), 100);

// VERIFY: a90b971d auto
// OFFSET: 0x009634E0
static int32_t IsShiftDown() {
    return EventIsKeyDown(KEY_LSHIFT) || EventIsKeyDown(KEY_RSHIFT);
}

// VERIFY: 4d240939 auto
// OFFSET: 0x00963510
static int32_t IsControlDown() {
    return EventIsKeyDown(KEY_LCONTROL) || EventIsKeyDown(KEY_RCONTROL);
}

// INLINED
CSimpleEditBox::EditHistory::EditHistory() {
    this->m_text = nullptr;
    this->m_taint = nullptr;
}

// INLINED
CSimpleEditBox::EditHistory::EditHistory(const CSimpleEditBox::EditHistory& source) {
    this->m_text = source.m_text ? SStrDupA(source.m_text, __FILE__, __LINE__) : nullptr;
    this->m_taint = source.m_taint;
}

// INLINED
CSimpleEditBox::EditHistory::~EditHistory() {
    if (this->m_text) {
        SMemFree(this->m_text, __FILE__, __LINE__, 0);
    }
}

// INLINED
CSimpleFontedFrameFont::CSimpleFontedFrameFont(CSimpleFontedFrame* frame) {
    this->m_frame = frame;
}

// VERIFY: ab4308a9 auto
// OFFSET: 0x00963540
void CSimpleFontedFrameFont::UpdateObjects() {
    uint32_t flags = this->m_attributes.m_flags;

    CSimpleFont::UpdateObjects();

    if (this->m_frame) {
        this->m_frame->FontUpdated(this, flags);
    }
}

// INLINED
void CSimpleEditBox::operator delete(void* ptr) {
    if (ptr) {
        CSimpleEditBox::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 64369d87 manual
// OFFSET: 0x00965120
void CSimpleEditBox::SetKeyboardFocus(CSimpleEditBox* editBox) {
    if (!editBox->m_isVisible || editBox == CSimpleEditBox::s_currentFocus) {
        return;
    }

    if (CSimpleEditBox::s_currentFocus) {
        CSimpleEditBox::s_currentFocus->OnFocusChange(false, true);
    }

    CSimpleEditBox::s_currentFocus = editBox;
    editBox->OnFocusChange(true, true);
}

// VERIFY: ce72435c manual
// OFFSET: 0x009651F0
void CSimpleEditBox::ClearKeyboardFocus(CSimpleEditBox* editBox, bool runScript) {
    if (CSimpleEditBox::s_currentFocus != editBox) {
        return;
    }

    CSimpleEditBox::s_currentFocus = nullptr;
    editBox->OnFocusChange(false, runScript);
}

// INLINED
int32_t CSimpleEditBox::GetObjectType() {
    if (!CSimpleEditBox::s_objectType) {
        CSimpleEditBox::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleEditBox::s_objectType;
}

// VERIFY: 2a978a4a manual
// OFFSET: 0x00966B70
CSimpleEditBox::CSimpleEditBox(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_cursorBlinkSpeed = 0.5f;
    this->m_blinkElapsedTime = 0.0f;
    this->m_textTaint = nullptr;
    this->m_textHidden = nullptr;
    this->m_textLength = 0;
    this->m_textMaxLength = -1;
    this->m_textMaxLetters = 0;
    this->m_visiblePosition = 0;
    this->m_visibleLength = 0;
    this->m_highlightLeft = 0;
    this->m_highlightRight = 0;
    this->m_highlightDrag = 0;
    this->m_cursorTexture = nullptr;
    this->m_cursorPos = 0;
    this->m_editBoxFlags = 0x1;
    this->m_dirtyBits = 0x1;
    this->m_historyLines = 0;
    this->m_historyIndex = 0;
    this->m_imeComposition = 0;
    this->m_inputLanguage = 0;
    this->m_clauseHighlight = nullptr;
    this->m_candidatesFrame = nullptr;
    this->m_candidateHighlight = nullptr;
    this->m_textInsets.minY = 0.0f;
    this->m_textInsets.minX = 0.0f;
    this->m_textInsets.maxY = 0.0f;
    this->m_textInsets.maxX = 0.0f;

    this->m_textSize = 32;
    this->m_text = static_cast<char*>(SMemAlloc(this->m_textSize, __FILE__, __LINE__, 0));
    *this->m_text = 0;
    this->m_textInfo = static_cast<uint32_t*>(SMemAlloc(sizeof(uint32_t) * this->m_textSize, __FILE__, __LINE__, 0));
    memset(this->m_textInfo, 0, sizeof(uint32_t) * this->m_textSize);

    this->m_font = new (SMemAlloc(sizeof(CSimpleFontedFrameFont), __FILE__, __LINE__, 0)) CSimpleFontedFrameFont(this);
    this->m_font->m_attributes.SetJustifyH(0x1);

    void* data = CSimpleFontString::s_allocator.GetData(0, typeid(CSimpleFontString).raw_name(), SLOG_OBJECT);
    this->m_fontString = data ? new (data) CSimpleFontString(this, 2, true) : nullptr;
    this->m_fontString->SetFontObject(this->m_font);

    this->SetMultiLine(0);

    for (uint32_t i = 0; i < 3; i++) {
        data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
        this->m_highlightTextures[i] = data ? new (data) CSimpleTexture(this, 2, 0) : nullptr;
    }

    CImVector highlightColor;
    highlightColor.b = 0x60;
    highlightColor.g = 0x60;
    highlightColor.r = 0x60;
    highlightColor.a = 0xFF;

    for (uint32_t i = 0; i < 3; i++) {
        this->m_highlightTextures[i]->SetTexture(highlightColor);
    }

    data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    this->m_cursorTexture = data ? new (data) CSimpleTexture(this, 3, 1) : nullptr;
    this->m_cursorTexture->Hide();

    CImVector cursorColor;
    cursorColor.b = 0xFF;
    cursorColor.g = 0xFF;
    cursorColor.r = 0xFF;
    cursorColor.a = 0xFF;
    this->m_cursorTexture->SetTexture(cursorColor);

    this->m_enterPressedCallback.m_handler = nullptr;
    this->m_escapePressedCallback.m_handler = nullptr;
    this->m_spacePressedCallback.m_handler = nullptr;
    this->m_tabPressedCallback.m_handler = nullptr;
    this->m_textChangedCallback.m_handler = nullptr;
    this->m_textSetCallback.m_handler = nullptr;

    this->EnableEvent(SIMPLE_EVENT_CHAR, -1);
    this->EnableEvent(SIMPLE_EVENT_KEY, -1);
    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
}

// VERIFY: f72deacf auto
// OFFSET: [0x00966F40, 0x009670E0, 0x00967D80]
CSimpleEditBox::~CSimpleEditBox() {
    CSimpleEditBox::ClearKeyboardFocus(this, false);

    this->SetHistoryLines(0);

    if (this->m_font) {
        delete this->m_font;
    }

    SMemFree(this->m_text, __FILE__, __LINE__, 0);
    SMemFree(this->m_textInfo, __FILE__, __LINE__, 0);

    if (this->m_textHidden) {
        SMemFree(this->m_textHidden, __FILE__, __LINE__, 0);
    }

    if (this->m_cursorTexture) {
        delete this->m_cursorTexture;
    }

    if (this->m_candidatesFrame) {
        delete this->m_candidatesFrame;
    }
}

// INLINED
void CSimpleEditBox::OnFocusChange(bool focus, bool runScript) {
    if (focus) {
        if (this->m_editBoxFlags & 0x8) {
            OsInputEnableSecureMode(1);
        }

        if (!(this->m_editBoxFlags & 0xC)) {
            OsIMEEnable(1);
        }

        if (runScript && this->m_onEditFocusGained.m_luaRef) {
            this->RunScript(this->m_onEditFocusGained, 0, nullptr);
        }
    } else {
        if (this->m_editBoxFlags & 0x8) {
            OsInputEnableSecureMode(0);
        }

        if (!(this->m_editBoxFlags & 0xC)) {
            this->DisableIME();
        }

        if (runScript && this->m_onEditFocusLost.m_luaRef) {
            this->RunScript(this->m_onEditFocusLost, 0, nullptr);
        }
    }

    this->m_dirtyBits |= 0x4;
}

// VERIFY: 94275693 auto
// OFFSET: 0x009627E0
void CSimpleEditBox::RunOnCursorChangedScript(float x, float y, float width, float height) {
    if (!this->m_onCursorChanged.m_luaRef) {
        return;
    }

    lua_State* L = FrameScript_GetContext();
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * x));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * y));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * width));
    lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * height));
    this->RunScript(this->m_onCursorChanged, 4, nullptr);
}

// VERIFY: 66cf6270 auto
// OFFSET: 0x009628B0
void CSimpleEditBox::RunOnCharCompositionScript(const char* text) {
    if (!this->m_onCharComposition.m_luaRef) {
        return;
    }

    lua_State* L = FrameScript_GetContext();
    lua_pushstring(L, text);
    this->RunScript(this->m_onCharComposition, 1, nullptr);
}

// VERIFY: cf79e310 auto
// OFFSET: 0x009628F0
FrameScript_Object::ScriptIx* CSimpleEditBox::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnEnterPressed", STORM_MAX_STR)) {
        return &this->m_onEnterPressed;
    }

    if (!SStrCmpI(name, "OnEscapePressed", STORM_MAX_STR)) {
        return &this->m_onEscapePressed;
    }

    if (!SStrCmpI(name, "OnSpacePressed", STORM_MAX_STR)) {
        return &this->m_onSpacePressed;
    }

    if (!SStrCmpI(name, "OnTabPressed", STORM_MAX_STR)) {
        return &this->m_onTabPressed;
    }

    if (!SStrCmpI(name, "OnTextChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self, userInput) %s end";
        return &this->m_onTextChanged;
    }

    if (!SStrCmpI(name, "OnTextSet", STORM_MAX_STR)) {
        return &this->m_onTextSet;
    }

    if (!SStrCmpI(name, "OnCursorChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,x,y,w,h) %s end";
        return &this->m_onCursorChanged;
    }

    if (!SStrCmpI(name, "OnInputLanguageChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,language) %s end";
        return &this->m_onInputLanguageChanged;
    }

    if (!SStrCmpI(name, "OnEditFocusGained", STORM_MAX_STR)) {
        return &this->m_onEditFocusGained;
    }

    if (!SStrCmpI(name, "OnEditFocusLost", STORM_MAX_STR)) {
        return &this->m_onEditFocusLost;
    }

    if (!SStrCmpI(name, "OnCharComposition", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,text) %s end";
        return &this->m_onCharComposition;
    }

    return nullptr;
}

// VERIFY: 0691a8cf auto
// OFFSET: 0x00962AA0
void CSimpleEditBox::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onEnterPressed, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onEscapePressed, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onSpacePressed, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTabPressed, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTextChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onTextSet, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onCursorChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onInputLanguageChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onEditFocusGained, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onEditFocusLost, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onCharComposition, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: c0664a7f auto
// OFFSET: 0x00962BB0
void CSimpleEditBox::SetAutoFocus(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x1;
    } else {
        this->m_editBoxFlags &= ~0x1u;
    }
}

// VERIFY: 22756da1 auto
// OFFSET: 0x00962BD0
void CSimpleEditBox::SetCountInvisibleLetters(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x20;
    } else {
        this->m_editBoxFlags &= ~0x20u;
    }
}

// VERIFY: b113f5ff auto
// OFFSET: 0x00962BF0
void CSimpleEditBox::SetNumeric(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x4;
    } else {
        this->m_editBoxFlags &= ~0x4u;
    }
}

// VERIFY: 253df0e6 auto
// OFFSET: 0x00962C10
void CSimpleEditBox::SetPassword(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x8;
    } else {
        this->m_editBoxFlags &= ~0x8u;
    }
}

// VERIFY: 237bc021 auto
// OFFSET: 0x00962C30
void CSimpleEditBox::SetAltArrowKeyMode(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x10;
    } else {
        this->m_editBoxFlags &= ~0x10u;
    }
}

// VERIFY: c78586fe auto
// OFFSET: 0x00962C50
const char* CSimpleEditBox::GetInputLanguageString() {
    return CSimpleEditBox::s_inputLanguageNames[this->m_inputLanguage];
}

// VERIFY: e187e845 auto
// OFFSET: 0x00962C60
void CSimpleEditBox::ToggleInputLanguage() {
    OsIMESetLanguageMode(this->m_inputLanguage == 0);
}

// VERIFY: 266e6b18 auto
// OFFSET: 0x00962C80
int32_t CSimpleEditBox::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    int32_t result = CSimpleFrame::OnLayerMouseUp(evt, btn);

    if (!result && this->m_highlightDrag) {
        this->m_highlightDrag = 0;
        return 1;
    }

    return result;
}

// VERIFY: 71f28044 manual
// OFFSET: 0x00962CC0
void CSimpleEditBox::UpdateTextInfo() {
    const char* text = this->m_text;
    int32_t offset = 0;
    uint32_t hyperlink = 0;

    memset(this->m_textInfo, 0, sizeof(uint32_t) * this->m_textSize);

    while (text && *text) {
        int32_t advance;
        uint32_t wide;
        QUOTEDCODE code = GxuDetermineQuotedCode(text, advance, nullptr, 0, wide);

        if (code == CODE_HYPERLINKSTART) {
            hyperlink |= 0x80000000;
        }

        this->m_textInfo[offset] = hyperlink | (code << 16) | advance;
        offset += advance;
        text += advance;

        if (code == CODE_HYPERLINKSTOP) {
            hyperlink &= ~0x80000000;
        }
    }
}

// VERIFY: c6c2720f manual
// OFFSET: 0x00962D50
int32_t CSimpleEditBox::GetNumToLen(int32_t offset, int32_t count, bool skipHyperlinks) {
    uint32_t* info = &this->m_textInfo[offset];
    int32_t length = 0;

    if (count > 0) {
        do {
            count--;

            if (!*info) {
                break;
            }

            uint32_t code;
            uint32_t hyperlink;

            do {
                code = (*info >> 16) & 0xFF;
                uint32_t size = *info & 0xFFFF;
                hyperlink = *info & 0xFF000000;
                length += size;
                info += size;
            } while (code == CODE_COLORON || code == CODE_HYPERLINKSTART || code == CODE_TEXTURESTART || (skipHyperlinks && static_cast<int32_t>(hyperlink) < 0 && code != CODE_HYPERLINKSTOP));

            while (true) {
                code = (*info >> 16) & 0xFF;
                uint32_t size = *info & 0xFFFF;

                if (code != CODE_COLORRESTORE && code != CODE_HYPERLINKSTOP && code != CODE_TEXTURESTOP) {
                    break;
                }

                length += size;
                info += size;
            }
        } while (count > 0);

        return length;
    }

    count = -count;

    while (count > 0) {
        count--;

        if (info <= this->m_textInfo) {
            break;
        }

        uint32_t code;

        do {
            do {
                info--;
            } while (info > this->m_textInfo && !*info);

            code = (*info >> 16) & 0xFF;
            length += *info & 0xFFFF;
        } while (code == CODE_COLORRESTORE || code == CODE_HYPERLINKSTOP || code == CODE_TEXTURESTOP || (skipHyperlinks && static_cast<int32_t>(*info) < 0 && code != CODE_HYPERLINKSTART));

        while (info > this->m_textInfo) {
            do {
                info--;
            } while (info > this->m_textInfo && !*info);

            code = (*info >> 16) & 0xFF;
            uint32_t size = *info & 0xFFFF;

            if (code != CODE_COLORON && code != CODE_HYPERLINKSTART && code != CODE_TEXTURESTART) {
                info += size;
                break;
            }

            length += size;
        }
    }

    return length;
}

// VERIFY: e45533bc manual
// OFFSET: 0x00962EA0
int32_t CSimpleEditBox::GetLenToNum(int32_t offset, int32_t length) {
    uint32_t* info = &this->m_textInfo[offset];
    int32_t count = 0;

    if (length > 0) {
        do {
            length--;

            if (*info) {
                uint32_t code = (*info >> 16) & 0xFF;

                if (code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) {
                    count++;
                }
            }

            info++;
        } while (length > 0);

        return count;
    }

    length = -length;

    while (length > 0) {
        length--;

        if (info <= this->m_textInfo) {
            break;
        }

        info--;

        if (*info) {
            uint32_t code = (*info >> 16) & 0xFF;

            if (code == CODE_NEWLINE || code == CODE_PIPE || code == CODE_INVALIDCODE) {
                count++;
            }
        }
    }

    return count;
}

// VERIFY: 7d55a2ac manual
// OFFSET: 0x00962F20
void CSimpleEditBox::GrowText(int32_t length) {
    if (static_cast<uint32_t>(length + 1) <= this->m_textSize) {
        return;
    }

    this->m_textSize = (length + 32) & ~0x1Fu;
    this->m_text = static_cast<char*>(SMemReAlloc(this->m_text, this->m_textSize, __FILE__, __LINE__, 0));
    this->m_textInfo = static_cast<uint32_t*>(SMemReAlloc(this->m_textInfo, sizeof(uint32_t) * this->m_textSize, __FILE__, __LINE__, 0));
}

// VERIFY: 2029d7dc manual
// OFFSET: 0x00962F90
void CSimpleEditBox::DeleteSubstring(int32_t left, int32_t right, int32_t script) {
    uint32_t* info = this->m_textInfo;

    if (info[left] & 0x80000000) {
        int32_t start = left;

        while (left >= 0) {
            uint32_t code = (info[left] >> 16) & 0xFF;

            if (code == CODE_COLORON || code == CODE_HYPERLINKSTART || code == CODE_TEXTURESTART) {
                break;
            }

            start = left;

            do {
                left--;
            } while (left >= 0 && !info[left]);
        }

        while (left >= 0) {
            uint32_t code = (info[left] >> 16) & 0xFF;

            if (code != CODE_COLORON && code != CODE_HYPERLINKSTART && code != CODE_TEXTURESTART) {
                break;
            }

            start = left;

            do {
                left--;
            } while (left >= 0 && !info[left]);
        }

        left = start;
    }

    if ((info[right] & 0x80000000) && right < this->m_textLength) {
        while (right < this->m_textLength) {
            uint32_t code = (info[right] >> 16) & 0xFF;

            if (code == CODE_COLORRESTORE || code == CODE_HYPERLINKSTOP || code == CODE_TEXTURESTOP) {
                break;
            }

            right += info[right] & 0xFFFF;
        }

        while (right < this->m_textLength) {
            uint32_t code = (info[right] >> 16) & 0xFF;

            if (code != CODE_COLORRESTORE && code != CODE_HYPERLINKSTOP && code != CODE_TEXTURESTOP) {
                break;
            }

            right += info[right] & 0xFFFF;
        }
    }

    int32_t count = right - left;
    char* text = &this->m_text[left];
    int32_t length = this->m_textLength - count;
    this->m_cursorPos = left;
    uint32_t* textInfo = &info[left];

    if (length < 0) {
        return;
    }

    this->m_textLength = length;
    memcpy(text, &text[count], length - left);
    memcpy(textInfo, &textInfo[count], sizeof(uint32_t) * (this->m_textLength - this->m_cursorPos));
    this->m_text[this->m_textLength] = 0;
    this->m_textInfo[this->m_textLength] = 0;

    if (this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }

    this->m_dirtyBits |= 0x5;

    if (!script) {
        this->m_dirtyBits |= 0x8;
    }

    if (!this->m_textLength) {
        this->m_textTaint = nullptr;
    }
}

// VERIFY: 902d762f manual
// OFFSET: 0x00963140
void CSimpleEditBox::HighlightText(int32_t left, int32_t right) {
    int32_t length = this->m_textLength;
    int32_t start = left <= 0 ? 0 : left;

    if (start >= length) {
        start = length;
    }

    int32_t end = right <= -1 ? -1 : right;

    if (end >= length) {
        end = length;
    }

    if (end < start) {
        end = length;
    }

    this->m_dirtyBits |= 0x2;
    this->m_highlightLeft = start;
    this->m_highlightRight = end;
}

// VERIFY: cb64bdd1 manual
// OFFSET: 0x009631A0
void CSimpleEditBox::ExtendHighlight(int32_t count) {
    if (!count) {
        return;
    }

    int32_t position = this->m_cursorPos + count;

    if (count >= 0 ? position > this->m_highlightRight : position >= this->m_highlightLeft) {
        int32_t right = position >= this->m_textLength ? this->m_textLength : position;
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = right > 0 ? right : 0;
    } else {
        int32_t left = position <= 0 ? 0 : position;
        this->m_dirtyBits |= 0x2;
        this->m_highlightLeft = left >= this->m_textLength ? this->m_textLength : left;
    }
}

// VERIFY: 8c56ba82 manual
// OFFSET: 0x00963260
void CSimpleEditBox::DeleteHighlight(int32_t script) {
    int32_t length = this->m_textLength;
    int32_t left = this->m_highlightLeft <= 0 ? 0 : this->m_highlightLeft;

    if (left >= length) {
        left = length;
    }

    this->m_highlightLeft = left;

    int32_t right = this->m_highlightRight >= length ? length : this->m_highlightRight;

    if (right <= 0) {
        right = 0;
    }

    this->m_highlightRight = right;
    this->DeleteSubstring(left, right, script);
}

// VERIFY: 4adcb896 auto
// OFFSET: 0x009632D0
void CSimpleEditBox::CopyToClipboard() {
    int32_t left = this->m_highlightLeft;
    int32_t right = this->m_highlightRight;

    if (left == right) {
        return;
    }

    if (this->m_editBoxFlags & 0x8) {
        OsClipboardPutString("");
        return;
    }

    uint32_t length = right - left;
    char* buffer = static_cast<char*>(alloca(length + 1));
    GxuFontStripEscapeCodes(&this->m_text[left], length, 0x1500, buffer, length + 1);
    OsClipboardPutString(buffer);
}

// VERIFY: 5501d9d4 manual
// OFFSET: 0x00963350
void CSimpleEditBox::SetCursorPosition(int32_t position) {
    if (position < 0) {
        this->m_dirtyBits |= 0x4;
        this->m_cursorPos = 0;
        return;
    }

    this->m_cursorPos = position > this->m_textLength ? this->m_textLength : position;
    this->m_dirtyBits |= 0x4;
}

// VERIFY: 04d3c307 auto
// OFFSET: 0x00963390
void CSimpleEditBox::DisableIME() {
    if (this->m_candidatesFrame) {
        this->m_candidatesFrame->Hide();
    }

    this->m_imeComposition = 0;
    OsIMEEnable(0);
}

// VERIFY: c0e09945 manual
// OFFSET: 0x00963570
void CSimpleEditBox::CreateClauseHighlight() {
    if (this->m_clauseHighlight) {
        return;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    this->m_clauseHighlight = data ? new (data) CSimpleTexture(this, 3, 1) : nullptr;

    CImVector color;
    color.b = 0x10;
    color.g = 0x20;
    color.r = 0x20;
    color.a = 0xFF;
    this->m_clauseHighlight->SetTexture(color);
    this->m_clauseHighlight->SetBlendMode(GxBlend_Add);
    this->m_clauseHighlight->SetHeight(this->m_fontString->GetFontHeight(true));
}

// VERIFY: 90ed488c auto
// OFFSET: 0x00963610
void CSimpleEditBox::UpdateLanguageIndicator() {
    int32_t language = this->m_inputLanguage;

    switch (OsIMEGetLanguageMode()) {
        case OS_IME_MODE_ROMAN:
            language = 0;
            break;

        case OS_IME_MODE_JAPANESE:
            language = 3;
            break;

        case OS_IME_MODE_KOREAN:
            language = 1;
            break;

        case OS_IME_MODE_CHINESE:
            language = 2;
            break;
    }

    if (language == this->m_inputLanguage) {
        return;
    }

    this->m_inputLanguage = language;

    if (this->m_onInputLanguageChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, CSimpleEditBox::s_inputLanguageNames[this->m_inputLanguage]);
        this->RunScript(this->m_onInputLanguageChanged, 1, nullptr);
    }
}

// VERIFY: 6b630cf6 manual
// OFFSET: 0x009636A0
void CSimpleEditBox::UpdateClauseInfo() {
    uint32_t clauseStart;
    uint32_t clauseEnd;
    uint32_t cursor;

    if (!OsIMEGetClauseInfo(clauseStart, clauseEnd, cursor)) {
        this->m_clauseEnd = this->m_cursorPos;
        this->m_clauseStart = this->m_cursorPos;
        return;
    }

    this->CreateClauseHighlight();

    int32_t left = this->m_highlightLeft;
    this->m_clauseStart = left + this->GetNumToLen(left, clauseStart, false);
    this->m_clauseEnd = left + this->GetNumToLen(left, clauseEnd, false);
    int32_t position = left + this->GetNumToLen(left, cursor, false);
    this->m_dirtyBits |= 0x6;
    this->m_cursorPos = position;
}

// VERIFY: 002cdee4 auto
// OFFSET: 0x00963740
void CSimpleEditBox::UpdateSizes() {
    if (!(this->m_layoutFlags & 0x100)) {
        return;
    }

    this->m_fontString->MarkFramePointsUnused();
    this->m_fontString->SetWidth((this->m_rect.maxX - this->m_rect.minX) / this->m_layoutScale - (this->m_textInsets.maxX + this->m_textInsets.minX));
    this->m_fontString->SetHeight(this->m_editBoxFlags & 0x2 ? 0.0f : (this->m_rect.maxY - this->m_rect.minY) / this->m_layoutScale - (this->m_textInsets.maxY + this->m_textInsets.minY));
    this->m_fontString->SetPoint(FRAMEPOINT_TOPLEFT, this, FRAMEPOINT_TOPLEFT, this->m_textInsets.minX, -this->m_textInsets.maxY, 1);

    float fontHeight = this->m_fontString->GetFontHeight(true);

    for (uint32_t i = 0; i < 3; i++) {
        this->m_highlightTextures[i]->MarkFramePointsUnused();

        if (i == 1) {
            this->m_highlightTextures[1]->SetPoint(FRAMEPOINT_TOPRIGHT, this->m_highlightTextures[0], FRAMEPOINT_BOTTOMRIGHT, 0.0f, 0.0f, 1);
            this->m_highlightTextures[1]->SetPoint(FRAMEPOINT_BOTTOMLEFT, this->m_highlightTextures[2], FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
            this->m_highlightTextures[1]->Resize(0);
        } else {
            this->m_highlightTextures[i]->SetHeight(fontHeight);
        }
    }

    if (this->m_clauseHighlight) {
        this->m_clauseHighlight->SetHeight(fontHeight);
    }

    float cursorWidth = NDCToDDCWidth(4.0f / (CoordinateGetAspectCompensation() * 1024.0f));
    this->m_cursorTexture->MarkFramePointsUnused();
    this->m_cursorTexture->SetWidth(cursorWidth);
    this->m_cursorTexture->SetHeight(fontHeight);

    this->m_dirtyBits |= 0x7;
}

// VERIFY: 491a1b6d auto
// OFFSET: 0x00963950
void CSimpleEditBox::Delete(int32_t count, int32_t script) {
    if (!count) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    int32_t length = this->GetNumToLen(this->m_cursorPos, count, true);
    int32_t cursor = this->m_cursorPos;

    if (count >= 0) {
        this->DeleteSubstring(cursor, cursor + length, script);
    } else {
        this->DeleteSubstring(cursor - length, cursor, script);
    }
}

// VERIFY: 2599b4be auto
// OFFSET: 0x009639B0
void CSimpleEditBox::DeleteForward(int32_t script) {
    if (this->m_highlightLeft != this->m_highlightRight) {
        this->DeleteHighlight(script);
        return;
    }

    if (this->m_cursorPos < this->m_textLength) {
        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor, cursor + this->GetNumToLen(cursor, 1, true), script);
    }
}

// VERIFY: 40feaa84 auto
// OFFSET: 0x00963A10
void CSimpleEditBox::DeleteForwardWord(int32_t script) {
    if (this->m_highlightLeft != this->m_highlightRight) {
        this->DeleteHighlight(script);
        return;
    }

    int32_t chars;

    while (this->m_cursorPos < this->m_textLength) {
        if (!iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor, cursor + this->GetNumToLen(cursor, 1, true), script);
    }

    while (this->m_cursorPos < this->m_textLength) {
        if (iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor, cursor + this->GetNumToLen(cursor, 1, true), script);
    }
}

// VERIFY: 9aa8f16a auto
// OFFSET: 0x00963B10
void CSimpleEditBox::DeleteBackward(int32_t script) {
    if (this->m_highlightLeft != this->m_highlightRight) {
        this->DeleteHighlight(script);
        return;
    }

    if (this->m_cursorPos > 0) {
        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor - this->GetNumToLen(cursor, -1, true), cursor, script);
    }
}

// VERIFY: 60de2945 auto
// OFFSET: 0x00963B60
void CSimpleEditBox::DeleteBackwardWord(int32_t script) {
    if (this->m_highlightLeft != this->m_highlightRight) {
        this->DeleteHighlight(script);
        return;
    }

    int32_t chars;

    while (this->m_cursorPos > 0) {
        int32_t position = this->m_cursorPos;

        do {
            position--;
        } while (position >= 0 && !this->m_textInfo[position]);

        if (!iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[position]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor - this->GetNumToLen(cursor, -1, true), cursor, script);
    }

    while (this->m_cursorPos > 0) {
        int32_t position = this->m_cursorPos;

        do {
            position--;
        } while (position >= 0 && !this->m_textInfo[position]);

        if (iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[position]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        this->DeleteSubstring(cursor - this->GetNumToLen(cursor, -1, true), cursor, script);
    }
}

// VERIFY: ca58031d auto
// OFFSET: 0x00963C60
void CSimpleEditBox::DeleteToStart(int32_t script) {
    int32_t count = this->GetLenToNum(0, this->m_cursorPos);

    if (count) {
        this->Delete(-count, script);
    }
}

// VERIFY: 5ad8daca auto
// OFFSET: 0x00963C90
void CSimpleEditBox::DeleteToEnd(int32_t script) {
    int32_t count = this->GetLenToNum(this->m_cursorPos, this->m_textLength - this->m_cursorPos);

    if (count) {
        this->Delete(count, script);
    }
}

// VERIFY: f479e151 manual
// OFFSET: 0x00963CD0
void CSimpleEditBox::Move(int32_t count, int32_t highlight) {
    if (!count) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    int32_t cursor = this->m_cursorPos;
    int32_t length = this->GetNumToLen(cursor, count, true);

    if (count < 0) {
        length = -length;
    }

    if (highlight) {
        if (this->m_highlightLeft == this->m_highlightRight) {
            this->m_highlightRight = cursor;
            this->m_highlightLeft = cursor;
        }

        this->ExtendHighlight(length);
    } else if (this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }

    this->m_cursorPos += length;
    this->m_dirtyBits |= 0x4;
}

// VERIFY: eb995a11 manual
// OFFSET: 0x00963D80
void CSimpleEditBox::MoveForward(int32_t highlight) {
    int32_t cursor = this->m_cursorPos;

    if (cursor < this->m_textLength) {
        int32_t length = this->GetNumToLen(cursor, 1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 75d5e794 auto
// OFFSET: 0x00963E40
void CSimpleEditBox::MoveForwardWord(int32_t highlight) {
    int32_t chars;

    while (this->m_cursorPos < this->m_textLength) {
        if (iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        int32_t length = this->GetNumToLen(cursor, 1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    while (this->m_cursorPos < this->m_textLength) {
        if (!iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        int32_t length = this->GetNumToLen(cursor, 1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: f6fc10fc auto
// OFFSET: 0x00964000
void CSimpleEditBox::MoveBackward(int32_t highlight) {
    if (this->m_cursorPos > 0) {
        this->Move(-1, highlight);
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 31888679 manual
// OFFSET: 0x00964050
void CSimpleEditBox::MoveBackwardWord(int32_t highlight) {
    int32_t chars;

    while (this->m_cursorPos > 0) {
        if (!iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos - 1]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        int32_t length = -this->GetNumToLen(cursor, -1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    while (this->m_cursorPos > 0) {
        if (iswspace(static_cast<wint_t>(sgetu8(reinterpret_cast<const uint8_t*>(&this->m_text[this->m_cursorPos - 1]), &chars)))) {
            break;
        }

        int32_t cursor = this->m_cursorPos;
        int32_t length = -this->GetNumToLen(cursor, -1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: cc77d6fa manual
// OFFSET: 0x00964200
void CSimpleEditBox::MoveToLineStart(int32_t highlight) {
    while (this->m_cursorPos > 0) {
        int32_t cursor = this->m_cursorPos;

        if (this->m_text[cursor - 1] == '\n') {
            break;
        }

        int32_t length = -this->GetNumToLen(cursor, -1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 9be53a50 auto
// OFFSET: 0x009642E0
void CSimpleEditBox::MoveToLineEnd(int32_t highlight) {
    while (this->m_cursorPos < this->m_textLength) {
        int32_t cursor = this->m_cursorPos;

        if (this->m_text[cursor] == '\n') {
            break;
        }

        int32_t length = this->GetNumToLen(cursor, 1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 9118eb7e manual
// OFFSET: 0x009643C0
void CSimpleEditBox::MoveToStart(int32_t highlight) {
    while (this->m_cursorPos > 0) {
        int32_t cursor = this->m_cursorPos;
        int32_t length = -this->GetNumToLen(cursor, -1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 37089728 manual
// OFFSET: 0x00964480
void CSimpleEditBox::MoveToEnd(int32_t highlight) {
    while (this->m_cursorPos < this->m_textLength) {
        int32_t cursor = this->m_cursorPos;
        int32_t length = this->GetNumToLen(cursor, 1, true);

        if (highlight) {
            if (this->m_highlightLeft == this->m_highlightRight) {
                this->m_highlightRight = cursor;
                this->m_highlightLeft = cursor;
            }

            this->ExtendHighlight(length);
        } else if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->m_cursorPos += length;
        this->m_dirtyBits |= 0x4;
    }

    if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }
}

// VERIFY: 6a8a7228 auto
// OFFSET: 0x00964550
void CSimpleEditBox::MoveLine(int32_t count, int32_t highlight) {
    uint32_t* lines = this->m_visibleLines.m_data;
    int32_t lastLine = this->m_visibleLines.m_count - 2;
    uint32_t cursor = this->m_cursorPos;
    int32_t line = 0;

    while (cursor >= lines[line + 1]) {
        if (line >= lastLine) {
            break;
        }

        line++;
    }

    int32_t target = line + count;

    if (target > lastLine) {
        target = lastLine;
    } else if (target < 0) {
        target = 0;
    }

    if (target == line) {
        if (!highlight && this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        return;
    }

    int32_t num = this->GetLenToNum(lines[line], cursor - lines[line]);
    uint32_t position = lines[target] + this->GetNumToLen(lines[target], num, false);
    uint32_t start = lines[target];
    uint32_t end = lines[target + 1];

    if (end > start && position >= end) {
        position = end - this->GetNumToLen(end, -1, false);
    }

    int32_t current = this->m_cursorPos;
    int32_t length = position - current;

    if (highlight) {
        if (this->m_highlightLeft == this->m_highlightRight) {
            this->m_highlightRight = current;
            this->m_highlightLeft = current;
        }

        this->ExtendHighlight(length);
    } else if (this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }

    this->m_cursorPos += length;
    this->m_dirtyBits |= 0x4;
}

// VERIFY: e5117211 manual
// OFFSET: 0x00964690
void CSimpleEditBox::AddHistoryLine(const char* text, const char* taint) {
    if (!this->m_historyLines) {
        return;
    }

    CSimpleEditBox::EditHistory& history = this->m_history.m_data[this->m_historyIndex];
    history.m_text = static_cast<char*>(SMemReAlloc(history.m_text, SStrLen(text) + 1, __FILE__, __LINE__, 0));
    this->m_history.m_data[this->m_historyIndex].m_taint = taint;
    SStrCopy(this->m_history.m_data[this->m_historyIndex].m_text, text, STORM_MAX_STR);
    this->m_historyIndex = (this->m_historyIndex + 1) % this->m_historyLines;
}

// VERIFY: f789062e manual
// OFFSET: 0x00964720
void CSimpleEditBox::ClearHistory() {
    this->m_historyIndex = 0;

    for (int32_t i = 0; i < this->m_historyLines; i++) {
        if (this->m_history.m_data[i].m_text) {
            SMemFree(this->m_history.m_data[i].m_text, __FILE__, __LINE__, 0);
        }

        this->m_history.m_data[i].m_text = nullptr;
        this->m_history.m_data[i].m_taint = nullptr;
    }
}

// VERIFY: e661ecd2 manual
// OFFSET: 0x00964790
int32_t CSimpleEditBox::ConvertCoordinateToIndex(float x, float y, int32_t& index) {
    CRect rect;
    rect.minY = 0.0f;
    rect.minX = 0.0f;
    rect.maxY = 0.0f;
    rect.maxX = 0.0f;

    int32_t result = this->m_fontString->GetRect(&rect);

    if (!result) {
        return result;
    }

    const char* text = this->m_editBoxFlags & 0x8 ? this->m_textHidden : this->m_text;
    float spacing = this->m_fontString->m_spacing;
    float fontHeight = CSimpleTop::RoundToPixelHeight(this->m_fontString->GetFontHeight(true) * this->m_layoutScale);
    float lineHeight = CSimpleTop::RoundToPixelHeight(this->m_layoutScale * spacing) + fontHeight;
    uint32_t line = 0;

    if (this->m_editBoxFlags & 0x2) {
        for (float offset = rect.maxY - rect.minY - (y - rect.minY); offset > lineHeight; line++) {
            if (line >= this->m_visibleLines.m_count - 2) {
                break;
            }

            offset -= lineHeight;
        }
    }

    float left;
    float right;

    if ((this->m_fontString->m_styleFlags & 0x7) == 2) {
        float textWidth = this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], this->m_visibleLines.m_data[line + 1] - this->m_visibleLines.m_data[line]);
        float width = rect.maxX - rect.minX;
        left = rect.minX + (width - textWidth) * 0.5f;
        rect.minX = left;
        right = rect.maxX - (width - textWidth) * 0.5f;
        rect.maxX = right;
    } else {
        left = rect.minX;
        right = rect.maxX;
    }

    float offset = x - left;

    if (offset < 0.0f) {
        index = 0;
        return 1;
    }

    if (offset > right) {
        offset = right;
    }

    uint32_t justify = this->m_fontString->m_styleFlags & 0x7;

    if (!justify) {
        return 1;
    }

    if (justify <= 2) {
        int32_t count = this->m_fontString->GetNumCharsWithinWidth(&text[this->m_visibleLines.m_data[line]], this->m_visibleLines.m_data[line + 1] - this->m_visibleLines.m_data[line], offset / this->m_fontString->m_layoutScale);

        if (text == this->m_text) {
            count = this->GetNumToLen(this->m_visibleLines.m_data[line], count, false);
        }

        index = count + this->m_visibleLines.m_data[line];
        return 1;
    }

    if (justify != 4) {
        return 1;
    }

    int32_t count = this->m_fontString->GetNumCharsWithinWidthFromEnd(&text[this->m_visibleLines.m_data[line]], this->m_visibleLines.m_data[line + 1] - this->m_visibleLines.m_data[line], (right - left - offset) / this->m_fontString->m_layoutScale);

    if (text == this->m_text) {
        count = this->GetNumToLen(this->m_visibleLines.m_data[line], count, false);
    }

    index = this->m_visibleLines.m_data[line + 1] - count;
    return 1;
}

// VERIFY: 1fd324ce auto
// OFFSET: 0x009649D0
void CSimpleEditBox::MakeTextVisible(int32_t position, float offset, float width) {
    if (this->m_editBoxFlags & 0x2) {
        return;
    }

    uint32_t count = this->m_fontString->GetNumCharsWithinWidthFromEnd(this->m_text, position, offset);
    this->m_visiblePosition = position - this->GetNumToLen(position, -static_cast<int32_t>(count), false);

    if (this->m_visiblePosition < 0) {
        this->m_visiblePosition = 0;
    }

    this->m_visibleLength = this->m_fontString->GetNumCharsWithinWidth(&this->m_text[this->m_visiblePosition], 0, width);
    int32_t visiblePosition = this->m_visiblePosition;
    this->m_visibleLength = this->GetNumToLen(visiblePosition, this->m_visibleLength, false);
    this->m_visibleLines.m_data[0] = visiblePosition;
    this->m_visibleLines.m_data[1] = this->m_visiblePosition + this->m_visibleLength;
}

// VERIFY: 6383fa0d manual
// OFFSET: 0x00964A90
void CSimpleEditBox::UpdateVisibleCursor() {
    int32_t cursor = this->m_cursorPos;

    if (cursor < this->m_visiblePosition || cursor > this->m_visiblePosition + this->m_visibleLength) {
        this->m_cursorTexture->Hide();
        return;
    }

    const char* text = this->m_editBoxFlags & 0x8 ? this->m_textHidden : this->m_text;
    float x = 0.0f;
    float y = 0.0f;
    float spacing = this->m_fontString->m_spacing;
    float fontHeight = CSimpleTop::RoundToPixelHeight(this->m_fontString->GetFontHeight(true) * this->m_layoutScale);
    float lineHeight = (CSimpleTop::RoundToPixelHeight(this->m_layoutScale * spacing) + fontHeight) / this->m_layoutScale;
    uint32_t* lines = this->m_visibleLines.m_data;
    uint32_t line = 0;

    while (static_cast<uint32_t>(cursor) >= lines[line + 1]) {
        if (line >= this->m_visibleLines.m_count - 2) {
            break;
        }

        line++;
        y -= lineHeight;
    }

    this->m_cursorTexture->MarkFramePointsUnused();

    switch (this->m_fontString->m_styleFlags & 0x7) {
        case 1:
            x = static_cast<uint32_t>(cursor) == this->m_visibleLines.m_data[line] ? 0.0f : this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], cursor - this->m_visibleLines.m_data[line]);

            if (this->m_editBoxFlags & 0x2) {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOPLEFT, x, y, 1);
            } else {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_LEFT, x, y, 1);
            }

            break;

        case 2:
            x = static_cast<uint32_t>(cursor) == this->m_visibleLines.m_data[line] ? 0.0f : this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], cursor - this->m_visibleLines.m_data[line]);
            x -= this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], this->m_visibleLines.m_data[line + 1] - this->m_visibleLines.m_data[line]) * 0.5f;

            if (this->m_editBoxFlags & 0x2) {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOP, x, y, 1);
            } else {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_CENTER, x, y, 1);
            }

            break;

        case 4:
            x = static_cast<uint32_t>(cursor) == this->m_visibleLines.m_data[line + 1] ? 0.0f : -this->m_fontString->GetTextWidth(&text[cursor], this->m_visibleLines.m_data[line + 1] - cursor);

            if (this->m_editBoxFlags & 0x2) {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOPRIGHT, x, y, 1);
            } else {
                this->m_cursorTexture->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_RIGHT, x, y, 1);
            }

            break;
    }

    this->m_blinkElapsedTime = 0.0f;

    float height = this->m_cursorTexture->GetHeight();
    float width = this->m_cursorTexture->GetWidth();
    this->RunOnCursorChangedScript(x, y, width, height);

    if (this == CSimpleEditBox::s_currentFocus) {
        this->m_cursorTexture->m_shown = 1;
        this->m_cursorTexture->ShowThis();
        return;
    }

    this->m_cursorTexture->Hide();
}

// VERIFY: f8f4d424 manual
// OFFSET: 0x00964DB0
void CSimpleEditBox::UpdateHighlightArea(CSimpleRegion* region, int32_t left, int32_t right) {
    const char* text = this->m_editBoxFlags & 0x8 ? this->m_textHidden : this->m_text;
    float y = 0.0f;
    float spacing = this->m_fontString->m_spacing;
    float fontHeight = CSimpleTop::RoundToPixelHeight(this->m_fontString->GetFontHeight(true) * this->m_layoutScale);
    float lineHeight = (CSimpleTop::RoundToPixelHeight(this->m_layoutScale * spacing) + fontHeight) / this->m_layoutScale;
    uint32_t* lines = this->m_visibleLines.m_data;
    uint32_t line = 0;
    uint32_t lastLine = this->m_visibleLines.m_count - 2;

    while (static_cast<uint32_t>(left) >= lines[line + 1]) {
        if (line >= lastLine) {
            break;
        }

        line++;
        y -= lineHeight;
    }

    uint32_t start = lines[line];

    if (start <= static_cast<uint32_t>(left)) {
        start = left;
    } else {
        left = lines[line];
    }

    uint32_t end = lines[line + 1];

    if (static_cast<uint32_t>(right) >= end) {
        right = end;
    }

    float width = line < lastLine && static_cast<uint32_t>(right) == end ? this->m_fontString->GetStringWidth() : this->m_fontString->GetTextWidth(&text[start], right - start);
    float x;

    region->MarkFramePointsUnused();

    switch (this->m_fontString->m_styleFlags & 0x7) {
        case 1:
            x = static_cast<uint32_t>(left) == this->m_visibleLines.m_data[line] ? 0.0f : this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], left - this->m_visibleLines.m_data[line]);

            if (this->m_editBoxFlags & 0x2) {
                region->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOPLEFT, x, y, 0);
            } else {
                region->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_LEFT, x, y, 0);
            }

            break;

        case 2:
            x = static_cast<uint32_t>(left) == this->m_visibleLines.m_data[line] ? 0.0f : this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], left - this->m_visibleLines.m_data[line]);
            x -= this->m_fontString->GetTextWidth(&text[this->m_visibleLines.m_data[line]], this->m_visibleLines.m_data[line + 1] - this->m_visibleLines.m_data[line]) * 0.5f;

            if (this->m_editBoxFlags & 0x2) {
                region->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOP, x, y, 0);
            } else {
                region->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_CENTER, x, y, 0);
            }

            break;

        case 4:
            x = static_cast<uint32_t>(left) == this->m_visibleLines.m_data[line + 1] ? 0.0f : -this->m_fontString->GetTextWidth(&text[left], this->m_visibleLines.m_data[line + 1] - left);

            if (this->m_editBoxFlags & 0x2) {
                region->SetPoint(FRAMEPOINT_TOPLEFT, this->m_fontString, FRAMEPOINT_TOPRIGHT, x, y, 0);
            } else {
                region->SetPoint(FRAMEPOINT_LEFT, this->m_fontString, FRAMEPOINT_RIGHT, x, y, 0);
            }

            break;
    }

    region->SetWidth(width);
    region->Resize(1);

    if (static_cast<uint32_t>(left) >= static_cast<uint32_t>(right) || fabs(width) < 0.00000023841858f) {
        region->Hide();
    } else {
        region->m_shown = 1;
        region->ShowThis();
    }
}

// VERIFY: c4d888b7 auto
// OFFSET: 0x009650E0
void CSimpleEditBox::FontUpdated(CSimpleFontedFrameFont* font, int32_t flags) {
    if (flags & 0x11) {
        this->UpdateSizes();
    }

    if (flags & 0x4) {
        this->m_cursorTexture->SetTexture(this->m_font->m_attributes.m_color);
    }
}

// VERIFY: 4593dc8a auto
// OFFSET: 0x009652E0
void CSimpleEditBox::RunOnEnterPressedScript() {
    if (this->m_onEnterPressed.m_luaRef) {
        this->RunScript(this->m_onEnterPressed, 0, nullptr);
    }

    if (this->m_enterPressedCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_enterPressedCallback.m_id;
        evt.m_param = this;
        this->m_enterPressedCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: 87b262d1 auto
// OFFSET: 0x00965340
void CSimpleEditBox::RunOnEscapePressedScript() {
    if (this->m_onEscapePressed.m_luaRef) {
        this->RunScript(this->m_onEscapePressed, 0, nullptr);
    }

    if (this->m_escapePressedCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_escapePressedCallback.m_id;
        evt.m_param = this;
        this->m_escapePressedCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: e94ed763 auto
// OFFSET: 0x009653A0
void CSimpleEditBox::RunOnSpacePressedScript() {
    if (this->m_onSpacePressed.m_luaRef) {
        this->RunScript(this->m_onSpacePressed, 0, nullptr);
    }

    if (this->m_spacePressedCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_spacePressedCallback.m_id;
        evt.m_param = this;
        this->m_spacePressedCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: 687c6a99 auto
// OFFSET: 0x00965400
void CSimpleEditBox::RunOnTabPressedScript() {
    if (this->m_onTabPressed.m_luaRef) {
        this->RunScript(this->m_onTabPressed, 0, nullptr);
    }

    if (this->m_tabPressedCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_tabPressedCallback.m_id;
        evt.m_param = this;
        this->m_tabPressedCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: 567f97b2 auto
// OFFSET: 0x00965460
void CSimpleEditBox::RunOnTextChangedScript() {
    if (this->m_onTextChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushboolean(L, (this->m_dirtyBits >> 3) & 0x1);
        this->RunScript(this->m_onTextChanged, 1, nullptr);
    }

    if (this->m_textChangedCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_textChangedCallback.m_id;
        evt.m_param = this;
        this->m_textChangedCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: a523ec28 auto
// OFFSET: 0x009654E0
void CSimpleEditBox::RunOnTextSetScript(const char* taint) {
    if (this->m_onTextSet.m_luaRef) {
        this->RunScript(this->m_onTextSet, 0, taint);
    }

    if (this->m_textSetCallback.m_handler) {
        CEvent evt;
        evt.m_id = this->m_textSetCallback.m_id;
        evt.m_param = this;
        this->m_textSetCallback.m_handler->OnEvent(evt);
    }
}

// VERIFY: 2550800e auto
// OFFSET: 0x00965550
void CSimpleEditBox::SetEditTextInsets(float left, float right, float top, float bottom) {
    this->m_textInsets.minX = left;
    this->m_textInsets.maxX = right;
    this->m_textInsets.maxY = top;
    this->m_textInsets.minY = bottom;
    this->UpdateSizes();
}

// VERIFY: e7e61940 auto
// OFFSET: 0x00965580
void CSimpleEditBox::GetEditTextInsets(float& left, float& right, float& top, float& bottom) {
    left = this->m_textInsets.minX;
    right = this->m_textInsets.maxX;
    top = this->m_textInsets.maxY;
    bottom = this->m_textInsets.minY;
    this->UpdateSizes();
}

// VERIFY: 1e831d32 auto
// OFFSET: 0x009655C0
void CSimpleEditBox::OnLayerShow() {
    CSimpleFrame::OnLayerShow();

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
    }
}

// VERIFY: 411486df manual
// OFFSET: 0x009655F0
void CSimpleEditBox::OnLayerHide() {
    CSimpleFrame::OnLayerHide();

    if (this == CSimpleEditBox::s_currentFocus) {
        CSimpleEditBox::s_currentFocus = nullptr;
        this->OnFocusChange(false, true);
    }
}

// VERIFY: c10b6d1d auto
// OFFSET: 0x00965660
int32_t CSimpleEditBox::OnLayerTrackUpdate(const CMouseEvent& evt) {
    if (!CSimpleFrame::OnLayerTrackUpdate(evt)) {
        return 0;
    }

    if (this->m_highlightDrag) {
        int32_t index;

        if (this->ConvertCoordinateToIndex(evt.m_data.m_mousePosition.x, evt.m_data.m_mousePosition.y, index)) {
            this->ExtendHighlight(index - this->m_cursorPos);
            this->SetCursorPosition(index);
        }
    }

    return 1;
}

// VERIFY: a53a098a auto
// OFFSET: 0x009656D0
void CSimpleEditBox::OnFrameSizeChanged(float width, float height) {
    CSimpleFrame::OnFrameSizeChanged(width, height);
    this->UpdateSizes();
}

// VERIFY: d28e9628 manual
// OFFSET: 0x00965700
void CSimpleEditBox::CreateCandidatesFrame() {
    if (this->m_candidatesFrame) {
        return;
    }

    float fontHeight = this->m_fontString->GetFontHeight(true);

    void* data = CSimpleMessageFrame::s_allocator.GetData(0, typeid(CSimpleMessageFrame).raw_name(), SLOG_OBJECT);
    this->m_candidatesFrame = data ? new (data) CSimpleMessageFrame(nullptr) : nullptr;
    this->m_candidatesFrame->SetFrameStrata(FRAME_STRATA_TOOLTIP);
    this->m_candidatesFrame->m_font->SetFontObject(this->m_font);
    this->m_candidatesFrame->m_font->m_attributes.SetJustifyH(0x1);
    this->m_candidatesFrame->SetWidth(fontHeight * 10.0f);

    CLayoutFrame* relative = this->m_clauseHighlight ? static_cast<CLayoutFrame*>(this->m_clauseHighlight) : static_cast<CLayoutFrame*>(this->m_highlightTextures[0]);
    this->m_candidatesFrame->SetPoint(FRAMEPOINT_BOTTOMLEFT, relative, FRAMEPOINT_TOPLEFT, 0.0f, 0.0f, 1);
    this->m_candidatesFrame->m_fading = 0;
    this->m_candidatesFrame->SetInsertMode(INSERT_AT_TOP);

    CSimpleTexture* background = nullptr;
    data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);

    if (data) {
        background = new (data) CSimpleTexture(this->m_candidatesFrame, 0, 1);
    }

    CImVector backgroundColor;
    backgroundColor.b = 0x60;
    backgroundColor.g = 0x60;
    backgroundColor.r = 0x60;
    backgroundColor.a = 0xFF;
    background->SetTexture(backgroundColor);
    background->SetAllPoints(this->m_candidatesFrame, 1);

    data = CSimpleTexture::s_allocator.GetData(0, typeid(CSimpleTexture).raw_name(), SLOG_OBJECT);
    this->m_candidateHighlight = data ? new (data) CSimpleTexture(this->m_candidatesFrame, 2, 1) : nullptr;

    CImVector highlightColor;
    highlightColor.b = 0x80;
    highlightColor.g = 0x80;
    highlightColor.r = 0x80;
    highlightColor.a = 0xFF;
    this->m_candidateHighlight->SetTexture(highlightColor);
}

// VERIFY: ad41eed4 manual
// OFFSET: 0x009658D0
int32_t CSimpleEditBox::OnLayerKeyDownRepeat(const CKeyEvent& evt) {
    if (!this->m_isVisible) {
        return 0;
    }

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
    } else if (this != CSimpleEditBox::s_currentFocus) {
        return 0;
    }

    return this->OnLayerKeyDown(evt);
}

// VERIFY: ce97a3ef auto
// OFFSET: 0x00965920
int32_t CSimpleEditBox::OnLayerKeyUp(const CKeyEvent& evt) {
    if (!this->m_isVisible) {
        return 0;
    }

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
        return 1;
    }

    return this == CSimpleEditBox::s_currentFocus;
}

// VERIFY: b386b85b manual
// OFFSET: 0x00965960
void CSimpleEditBox::Insert(const char* text, const char* taint, int32_t runScripts, int32_t highlight, int32_t script) {
    int32_t position = this->m_cursorPos;
    uint32_t* info = this->m_textInfo;

    if ((info[position] & 0x80000000) && position > 0) {
        do {
            position--;
        } while (position >= 0 && !info[position]);

        if (info[position] & 0x80000000) {
            return;
        }
    }

    if (this->m_highlightLeft != this->m_highlightRight) {
        this->DeleteHighlight(script);
    }

    if (!text) {
        text = "";
    }

    int32_t length = SStrLen(text);
    int32_t chars;

    if ((this->m_editBoxFlags & 0x4) && !highlight && *text) {
        for (const char* digit = text; *digit; digit += chars) {
            if (sgetu8(reinterpret_cast<const uint8_t*>(digit), &chars) - '0' > 9) {
                return;
            }
        }
    }

    this->GrowText(length + this->m_textLength);

    char* dest = &this->m_text[this->m_cursorPos];

    if (this->m_cursorPos < this->m_textLength) {
        memmove(&dest[length], dest, this->m_textLength - this->m_cursorPos);
    }

    memcpy(dest, text, length);
    this->m_textLength += length;
    this->m_text[this->m_textLength] = 0;

    int32_t end = this->m_cursorPos + length;
    int32_t start = this->m_cursorPos;
    this->m_cursorPos = end;

    this->UpdateTextInfo();

    this->m_dirtyBits |= 0x5;

    if (script) {
        this->m_dirtyBits &= ~0x8u;
    } else {
        this->m_dirtyBits |= 0x8;
    }

    if (this->m_textMaxLength >= 0 && this->m_textLength > this->m_textMaxLength) {
        int32_t cursor = this->m_cursorPos;
        this->SetCursorPosition(this->m_textLength);

        while (this->m_textLength > this->m_textMaxLength) {
            int32_t current = this->m_cursorPos;
            this->DeleteSubstring(current - this->GetNumToLen(current, -1, true), current, script);
        }

        this->SetCursorPosition(cursor);
    }

    if (this->m_textMaxLetters) {
        int32_t letters = this->m_editBoxFlags & 0x20 ? SStrLenUTF8(this->m_text) : this->GetLenToNum(0, this->m_textLength);

        if (letters > this->m_textMaxLetters) {
            int32_t cursor = this->m_cursorPos;
            this->SetCursorPosition(this->m_textLength);

            letters = this->m_editBoxFlags & 0x20 ? SStrLenUTF8(this->m_text) : this->GetLenToNum(0, this->m_textLength);

            while (letters > this->m_textMaxLetters) {
                this->Delete(this->m_textMaxLetters - letters, script);
                letters = this->m_editBoxFlags & 0x20 ? SStrLenUTF8(this->m_text) : this->GetLenToNum(0, this->m_textLength);
            }

            this->SetCursorPosition(cursor);
        }
    }

    if (highlight) {
        this->m_highlightLeft = start;
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = end < this->m_textLength ? end : this->m_textLength;
    }

    if (!this->m_textTaint && this->m_textLength > 0) {
        this->m_textTaint = taint;
    }

    if (!runScripts) {
        return;
    }

    if (this->m_onChar.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, text);
        this->RunScript(this->m_onChar, 1, nullptr);
    }

    for (const char* c = text;; c += chars) {
        uint32_t ch = sgetu8(reinterpret_cast<const uint8_t*>(c), &chars);

        if (ch == 0xFFFFFFFF) {
            return;
        }

        if (ch == ' ') {
            break;
        }
    }

    this->RunOnSpacePressedScript();
}

// VERIFY: f9405edc auto
// OFFSET: 0x00965CE0
void CSimpleEditBox::Insert(uint32_t ch) {
    if (ch < 0x20 || ch == 0x7F) {
        if (ch == '\t') {
            this->Insert("    ", nullptr, 1, 0, 0);
            return;
        }

        if (!(this->m_editBoxFlags & 0x2) || ch != '\n') {
            return;
        }
    }

    if (ch == '|') {
        this->Insert("||", nullptr, 1, 0, 0);
        return;
    }

    char buffer[8];
    sputu8(ch, buffer);
    this->Insert(buffer, nullptr, 1, 0, 0);
}

// VERIFY: fbe65e47 manual
// OFFSET: 0x00965D70
void CSimpleEditBox::UpdateVisibleHighlight() {
    if (this->m_clauseHighlight) {
        if (this->m_imeComposition) {
            this->UpdateHighlightArea(this->m_clauseHighlight, this->m_clauseStart, this->m_clauseEnd);
        } else {
            this->m_clauseHighlight->Hide();
        }
    }

    uint32_t* lines = this->m_visibleLines.m_data;
    int32_t lastLine = this->m_visibleLines.m_count - 2;
    int32_t startLine = 0;

    while (static_cast<uint32_t>(this->m_highlightLeft) >= lines[startLine + 1]) {
        if (startLine >= lastLine) {
            break;
        }

        startLine++;
    }

    int32_t endLine = 0;

    while (static_cast<uint32_t>(this->m_highlightRight) >= lines[endLine + 1]) {
        if (endLine >= lastLine) {
            break;
        }

        endLine++;
    }

    if (endLine == startLine) {
        this->UpdateHighlightArea(this->m_highlightTextures[0], this->m_highlightLeft, this->m_highlightRight);
        this->m_highlightTextures[1]->Hide();
        this->m_highlightTextures[2]->Hide();
        return;
    }

    this->UpdateHighlightArea(this->m_highlightTextures[0], this->m_highlightLeft, this->m_visibleLines.m_data[startLine + 1]);

    if (endLine - startLine + 1 == 2) {
        this->m_highlightTextures[1]->Hide();
    } else {
        this->m_highlightTextures[1]->m_shown = 1;
        this->m_highlightTextures[1]->ShowThis();
    }

    this->UpdateHighlightArea(this->m_highlightTextures[2], this->m_visibleLines.m_data[endLine], this->m_highlightRight);
}

// VERIFY: 9b220353 auto
// OFFSET: 0x00965ED0
void CSimpleEditBox::PasteFromClipboard() {
    char* string = OsClipboardGetString();

    if (!string) {
        return;
    }

    int32_t chars;

    for (const char* c = string; *c; c += chars) {
        this->Insert(sgetu8(reinterpret_cast<const uint8_t*>(c), &chars));
    }

    OsClipboardFreeString(string);
}

// VERIFY: 3333ed53 manual
// OFFSET: 0x00965F20
void CSimpleEditBox::SetMultiLine(int32_t enable) {
    if (enable) {
        this->m_editBoxFlags |= 0x2;
    } else {
        this->m_editBoxFlags &= ~0x2u;
    }

    this->m_visibleLines.SetCount(2);

    CSimpleFontString* fontString = this->m_fontString;

    if (this->m_editBoxFlags & 0x2) {
        fontString->SetJustifyV(0x8);

        fontString->m_settableStyleFlags &= ~0x4000u;
        uint32_t styleFlags = fontString->m_styleFlags & ~0x4000u;

        if (styleFlags != fontString->m_styleFlags) {
            fontString->m_styleFlags = styleFlags;

            if (fontString->m_string) {
                fontString->UpdateString();
            }
        }

        this->m_fontString->SetNonWordWrap(0);
    } else {
        fontString->SetJustifyV(0x10);

        fontString->m_settableStyleFlags &= ~0x4000u;
        uint32_t styleFlags = fontString->m_styleFlags | 0x4000;

        if (styleFlags != fontString->m_styleFlags) {
            fontString->m_styleFlags = styleFlags;

            if (fontString->m_string) {
                fontString->UpdateString();
            }
        }

        this->m_fontString->SetNonWordWrap(1);
    }

    this->m_fontString->SetNonSpaceWrap(1);
    this->UpdateSizes();
}

// VERIFY: fe353c96 auto
// OFFSET: 0x009660E0
int32_t CSimpleEditBox::OnLayerChar(const CCharEvent& evt) {
    if (!this->m_isVisible) {
        return 0;
    }

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
    } else if (this != CSimpleEditBox::s_currentFocus) {
        return 0;
    }

    this->m_imeComposition = 0;
    this->Insert(static_cast<uint32_t>(evt.m_data.m_ch));
    return 1;
}

// VERIFY: feaa14c1 manual
// OFFSET: 0x00966140
int32_t CSimpleEditBox::PopulateCandidates(uint32_t listMask) {
    TSGrowableArray<OsIMECandidate> candidates;
    uint32_t pageStart;
    uint32_t pageSize;
    uint32_t count;
    uint32_t selection;

    if (!OsIMEGetCandidates(listMask, pageStart, pageSize, count, selection, candidates)) {
        return 0;
    }

    this->CreateCandidatesFrame();

    float fontHeight = this->m_fontString->GetFontHeight(true);
    this->m_candidatesFrame->Clear();
    this->m_candidatesFrame->SetHeight((pageSize + 1) * fontHeight + fontHeight * 0.1f);

    this->m_candidateHighlight->MarkFramePointsUnused();
    this->m_candidateHighlight->SetPoint(FRAMEPOINT_TOPLEFT, this->m_candidatesFrame, FRAMEPOINT_TOPLEFT, 0.0f, -((selection % pageSize) * fontHeight), 0);
    this->m_candidateHighlight->SetPoint(FRAMEPOINT_BOTTOMRIGHT, this->m_candidatesFrame, FRAMEPOINT_TOPRIGHT, 0.0f, -((selection % pageSize + 1) * fontHeight), 0);
    this->m_candidateHighlight->Resize(0);

    char buffer[1024];
    CImVector color;

    SStrPrintf(buffer, sizeof(buffer), "> %d/%d", selection + 1, count);
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;
    color.a = 0xFF;
    this->m_candidatesFrame->AddMessage(buffer, color);

    for (uint32_t i = pageSize; i; i--) {
        const char* candidate = candidates.m_data[i - 1].m_text;

        if (*candidate) {
            SStrPrintf(buffer, sizeof(buffer), "%d: %s", i, candidate);
        } else {
            SStrCopy(buffer, " ", sizeof(buffer));
        }

        color.b = 0xFF;
        color.g = 0xFF;
        color.r = 0xFF;
        color.a = 0xFF;
        this->m_candidatesFrame->AddMessage(buffer, color);
    }

    this->m_candidatesFrame->Resize(1);
    return 1;
}

// VERIFY: e7688b1b manual
// OFFSET: 0x009663C0
int32_t CSimpleEditBox::OnLayerIme(const CImeEvent& evt) {
    if (!this->m_isVisible) {
        return 0;
    }

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
    } else if (this != CSimpleEditBox::s_currentFocus) {
        return 0;
    }

    if (this->m_editBoxFlags & 0x8) {
        return 1;
    }

    switch (evt.m_data.m_action) {
        case OS_INPUT_IME_NOTIFYLANGUAGE:
            this->UpdateLanguageIndicator();
            return 1;

        case OS_INPUT_IME_STARTCOMPOSITION:
            this->m_highlightDrag = 0;
            this->m_imeComposition = 1;
            return 1;

        case OS_INPUT_IME_COMPOSITION: {
            char buffer[512];

            if (evt.m_data.m_param & 0x4) {
                OsIMEGetCompositionResult(buffer, sizeof(buffer));

                int32_t chars;

                for (const char* c = buffer; *c; c += chars) {
                    this->Insert(sgetu8(reinterpret_cast<const uint8_t*>(c), &chars));
                }
            }

            if (evt.m_data.m_param & 0x2) {
                OsIMEGetCompositionString(buffer, sizeof(buffer));
                this->Insert(buffer, nullptr, 0, 1, 0);
                this->RunOnCharCompositionScript(buffer);
            }

            if (evt.m_data.m_param & 0x1) {
                this->UpdateClauseInfo();
            }

            return 1;
        }

        case OS_INPUT_IME_OPENCANDIDATES:
        case OS_INPUT_IME_CHANGECANDIDATES:
            if (this->PopulateCandidates(evt.m_data.m_param)) {
                if (this->m_candidatesFrame) {
                    this->m_candidatesFrame->Show();
                }
            } else if (this->m_candidatesFrame) {
                this->m_candidatesFrame->Hide();
            }

            return 1;

        case OS_INPUT_IME_CLOSECANDIDATES:
        case OS_INPUT_IME_ENDCOMPOSITION:
            if (this->m_candidatesFrame) {
                this->m_candidatesFrame->Hide();
            }

            this->m_imeComposition = 0;
            return 1;
    }

    return 1;
}

// VERIFY: 2adb42fa auto
// OFFSET: 0x00966580
void CSimpleEditBox::SetText(const char* text, const char* taint) {
    if (this->m_highlightLeft != this->m_highlightRight) {
        this->m_dirtyBits |= 0x2;
        this->m_highlightRight = 0;
        this->m_highlightLeft = 0;
    }

    if (!SStrCmp(text, this->m_text, STORM_MAX_STR)) {
        return;
    }

    this->DeleteSubstring(0, this->m_textLength, 1);
    this->m_visiblePosition = 0;
    this->Insert(text, taint, 0, 0, 1);
    this->RunOnTextSetScript(taint);
}

// VERIFY: 0e6aee1d auto
// OFFSET: 0x00966600
void CSimpleEditBox::BackwardHistory() {
    int32_t lines = this->m_historyLines;

    for (int32_t count = 1; count <= lines; count++) {
        int32_t index = (this->m_historyIndex + count) % lines;

        if (this->m_history.m_data[index].m_text) {
            this->m_historyIndex = index;
            this->SetText(this->m_history.m_data[index].m_text, this->m_history.m_data[index].m_taint);
            return;
        }
    }
}

// VERIFY: b6e843c9 manual
// OFFSET: 0x00966670
void CSimpleEditBox::ForwardHistory() {
    for (int32_t count = 1; count <= this->m_historyLines; count++) {
        int32_t index = this->m_historyIndex - count;

        if (index < 0) {
            index += this->m_historyLines;
        }

        if (this->m_history.m_data[index].m_text) {
            this->m_historyIndex = index;
            this->SetText(this->m_history.m_data[index].m_text, this->m_history.m_data[index].m_taint);
            return;
        }
    }
}

// VERIFY: 675a5d35 manual
// OFFSET: 0x00966710
void CSimpleEditBox::UpdateVisibleText() {
    float width = this->m_fontString->GetWidth();

    if (this->m_editBoxFlags & 0x8) {
        uint32_t length = SStrLen(this->m_text);
        this->m_textHidden = static_cast<char*>(SMemReAlloc(this->m_textHidden, length + 1, __FILE__, __LINE__, 0));
        memset(this->m_textHidden, '*', length);
        this->m_textHidden[length] = 0;
    }

    char* text = this->m_editBoxFlags & 0x8 ? this->m_textHidden : this->m_text;

    if (this->m_editBoxFlags & 0x2) {
        uint32_t lines = this->m_fontString->WrapText(&text[this->m_visiblePosition], width, this->m_visibleLines.m_data, this->m_visibleLines.m_count);

        if (!lines) {
            this->m_visibleLines.SetCount(2);
            this->m_visibleLines.m_data[0] = 0;
            lines = 1;
        } else if (lines < this->m_visibleLines.m_count) {
            this->m_visibleLines.SetCount(lines + 1);
        } else {
            this->m_visibleLines.SetCount(lines + 1);
            this->m_fontString->WrapText(&text[this->m_visiblePosition], width, this->m_visibleLines.m_data, this->m_visibleLines.m_count);
        }

        if (this->m_visiblePosition > 0) {
            for (uint32_t i = 0; i < lines; i++) {
                this->m_visibleLines.m_data[i] += this->m_visiblePosition;
            }
        }

        this->m_visibleLines.m_data[lines] = SStrLen(text);
        this->m_visibleLength = this->m_visibleLines.m_data[lines];
    } else {
        float textWidth = width - NDCToDDCWidth(4.0f / (CoordinateGetAspectCompensation() * 1024.0f));

        if (textWidth <= 0.0f) {
            textWidth = width;
        }

        this->m_visibleLength = this->m_fontString->GetNumCharsWithinWidth(&text[this->m_visiblePosition], 0, textWidth);

        if (text == this->m_text) {
            this->m_visibleLength = this->GetNumToLen(this->m_visiblePosition, this->m_visibleLength, false);
        }

        this->m_visibleLines.m_data[0] = this->m_visiblePosition;
        this->m_visibleLines.m_data[1] = this->m_visibleLength + this->m_visiblePosition;
    }

    if (!(this->m_editBoxFlags & 0x8)) {
        if (this->m_imeComposition && this->m_clauseHighlight && this->m_clauseEnd > this->m_visiblePosition + this->m_visibleLength) {
            this->MakeTextVisible(this->m_clauseEnd, width, width);
        }

        int32_t cursor = this->m_cursorPos;

        if (cursor < this->m_visiblePosition || cursor > this->m_visiblePosition + this->m_visibleLength) {
            this->MakeTextVisible(cursor, (cursor < this->m_visiblePosition ? 0.25f : 0.75f) * width, width);
        }
    }

    const char* colorCode = nullptr;

    for (const char* scan = &text[this->m_visiblePosition]; *scan; scan++) {
        if (*scan != '|') {
            continue;
        }

        if (scan[1] == '|') {
            scan++;
            continue;
        }

        if (scan[1] == 'c') {
            break;
        }

        if (scan[1] == 'r') {
            const char* back = scan;

            while (back >= text) {
                if (*back == '|') {
                    if (back > text && back[-1] == '|') {
                        back--;
                        continue;
                    }

                    if (back[1] == 'c') {
                        colorCode = back;
                        break;
                    }
                }

                back--;
            }

            break;
        }
    }

    if (colorCode) {
        char* buffer = static_cast<char*>(alloca(this->m_visibleLength + 11));
        memcpy(buffer, colorCode, 10);
        memcpy(&buffer[10], &text[this->m_visiblePosition], this->m_visibleLength);
        buffer[this->m_visibleLength + 10] = 0;
        this->m_fontString->SetText(buffer, 0);
    } else {
        int32_t end = this->m_visiblePosition + this->m_visibleLength;
        char saved = text[end];
        text[end] = 0;
        this->m_fontString->SetText(&text[this->m_visiblePosition], 0);
        text[end] = saved;
    }

    if (this->m_editBoxFlags & 0x2) {
        float height = this->m_fontString->GetStringHeight();

        if (height == 0.0f) {
            height = this->m_fontString->GetFontHeight(true);
        }

        this->SetHeight(height + (this->m_textInsets.maxY + this->m_textInsets.minY));
    }
}

// VERIFY: 47818eff auto
// OFFSET: 0x00966F20
const char* CSimpleEditBox::GetObjectTypeName() {
    return CSimpleEditBox::s_objectTypeName;
}

// VERIFY: b50cdb9f auto
// OFFSET: 0x00966F30
int32_t CSimpleEditBox::GetScriptMetaTable() {
    return CSimpleEditBox::s_metatable;
}

// VERIFY: 5d8f0f26 auto
// OFFSET: 0x00966F50
bool CSimpleEditBox::IsA(int32_t type) {
    return type == CSimpleEditBox::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 9f181853 auto
// OFFSET: 0x00966F90
bool CSimpleEditBox::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleEditBox::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 59f6a6f7 manual
// OFFSET: 0x00966FD0
void CSimpleEditBox::SetHistoryLines(int32_t lines) {
    this->m_history.SetCount(lines);
    this->m_historyLines = lines;

    if (this->m_historyIndex >= lines) {
        this->m_historyIndex = lines ? lines - 1 : 0;
    }
}

// VERIFY: 12e6f1a3 auto
// OFFSET: 0x00967020
void CSimpleEditBox::UpdateDirtyBits() {
    uint32_t textChanged = this->m_dirtyBits & 0x1;

    if (this->m_dirtyBits & 0x4) {
        int32_t cursor = this->m_cursorPos;
        int32_t visibleEnd = this->m_visibleLength + this->m_visiblePosition;

        if (cursor < this->m_visiblePosition || cursor > visibleEnd || (this->m_imeComposition && this->m_clauseHighlight && this->m_clauseEnd > visibleEnd)) {
            this->m_dirtyBits |= 0x1;
        }
    }

    if (this->m_dirtyBits & 0x1) {
        this->m_dirtyBits &= ~0x1u;
        this->UpdateVisibleText();
    }

    if (this->m_dirtyBits & 0x2) {
        this->m_dirtyBits &= ~0x2u;
        this->UpdateVisibleHighlight();
    }

    if (this->m_dirtyBits & 0x4) {
        this->m_dirtyBits &= ~0x4u;
        this->UpdateVisibleCursor();
    }

    if (textChanged) {
        this->RunOnTextChangedScript();
        this->m_dirtyBits &= ~0x8u;
    }
}

// VERIFY: 03deb178 manual
// OFFSET: 0x00967290
void CSimpleEditBox::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    const char* fontName = node->GetAttributeByName("font");

    if (fontName && *fontName) {
        CSimpleFont* font = CSimpleFont::GetFont(fontName, 0);

        if (font) {
            this->m_font->SetFontObject(font);
        } else {
            const char* name = this->GetName();
            status->Add(STATUS_WARNING, "%s %s: Couldn't find font object named %s", this->GetObjectTypeName(), name ? name : "<unnamed>", fontName);
        }
    }

    const char* letters = node->GetAttributeByName("letters");

    if (letters && *letters) {
        this->m_textMaxLetters = SStrToInt(letters);
    }

    const char* countInvisibleLetters = node->GetAttributeByName("countInvisibleLetters");

    if (countInvisibleLetters && *countInvisibleLetters) {
        this->SetCountInvisibleLetters(StringToBOOL(countInvisibleLetters));
    }

    const char* blinkSpeed = node->GetAttributeByName("blinkSpeed");

    if (blinkSpeed && *blinkSpeed) {
        this->m_cursorBlinkSpeed = SStrToFloat(blinkSpeed) > 0.0f ? SStrToFloat(blinkSpeed) : 0.0f;
    }

    const char* autoFocus = node->GetAttributeByName("autoFocus");

    if (autoFocus && *autoFocus) {
        this->SetAutoFocus(StringToBOOL(autoFocus));
    }

    const char* multiLine = node->GetAttributeByName("multiLine");

    if (multiLine && *multiLine) {
        this->SetMultiLine(StringToBOOL(multiLine));
    }

    const char* numeric = node->GetAttributeByName("numeric");

    if (numeric && *numeric) {
        this->SetNumeric(StringToBOOL(numeric));
    }

    const char* password = node->GetAttributeByName("password");

    if (password && *password) {
        this->SetPassword(StringToBOOL(password));
    }

    const char* ignoreArrows = node->GetAttributeByName("ignoreArrows");

    if (ignoreArrows && *ignoreArrows) {
        this->SetAltArrowKeyMode(StringToBOOL(ignoreArrows));
    }

    const char* historyLines = node->GetAttributeByName("historyLines");

    if (historyLines && *historyLines) {
        this->SetHistoryLines(SStrToInt(historyLines) <= 0 ? 0 : SStrToInt(historyLines));
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "FontString", STORM_MAX_STR)) {
            if (this->m_font->m_fontObject) {
                CSimpleFont* fontObject = this->m_font->m_fontObject;
                const char* name = this->GetName();
                status->Add(STATUS_WARNING, "%s: FontString element overriding font %s", name ? name : "<unnamed>", fontObject->GetName());
            }

            this->m_font->LoadXML(child, status);

            const char* bytes = child->GetAttributeByName("bytes");

            if (bytes && *bytes) {
                int32_t maxBytes = SStrToInt(bytes);
                this->m_textMaxLength = maxBytes <= 0 ? -1 : maxBytes - 1;
            }
        } else if (!SStrCmpI(child->m_name.m_str, "HighlightColor", STORM_MAX_STR)) {
            CImVector color;

            if (LoadXML_Color(child, color, status)) {
                for (uint32_t i = 0; i < 3; i++) {
                    this->m_highlightTextures[i]->SetTexture(color);
                }
            }
        } else if (!SStrCmpI(child->m_name.m_str, "TextInsets", STORM_MAX_STR)) {
            float left;
            float right;
            float top;
            float bottom;

            if (LoadXML_Insets(child, left, right, top, bottom, status)) {
                this->SetEditTextInsets(left, right, top, bottom);
            }
        }
    }
}

// VERIFY: f5607a28 manual
// OFFSET: 0x00967670
void CSimpleEditBox::OnLayerUpdate(float elapsedSec) {
    CSimpleFrame::OnLayerUpdate(elapsedSec);

    this->UpdateDirtyBits();

    if (!g_theGxDevicePtr->CapsWindowHasFocus(-1)) {
        if (this->m_cursorTexture->m_shown) {
            this->m_cursorTexture->Hide();
        }

        return;
    }

    if (this->m_cursorBlinkSpeed == 0.0f) {
        return;
    }

    int32_t cursor = this->m_cursorPos;

    if (cursor < this->m_visiblePosition || cursor > this->m_visiblePosition + this->m_visibleLength) {
        return;
    }

    this->m_blinkElapsedTime += elapsedSec;

    if (this == CSimpleEditBox::s_currentFocus && this->m_blinkElapsedTime > this->m_cursorBlinkSpeed) {
        if (this->m_cursorTexture->m_shown) {
            this->m_cursorTexture->Hide();
        } else {
            this->m_cursorTexture->m_shown = 1;
            this->m_cursorTexture->ShowThis();
        }

        this->m_blinkElapsedTime = 0.0f;
    }
}

// VERIFY: 7fac4541 manual
// OFFSET: 0x00967760
int32_t CSimpleEditBox::OnLayerKeyDown(const CKeyEvent& evt) {
    if (!this->m_isVisible) {
        return 0;
    }

    KEY key = evt.m_data.m_key;

    if (key == KEY_PRINTSCREEN || ((this->m_editBoxFlags & 0x10) && (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN) && !EventIsKeyDown(KEY_LALT) && !EventIsKeyDown(KEY_RALT))) {
        return 0;
    }

    if (!CSimpleEditBox::s_currentFocus && (this->m_editBoxFlags & 0x1)) {
        CSimpleEditBox::SetKeyboardFocus(this);
    } else if (this != CSimpleEditBox::s_currentFocus) {
        return 0;
    }

    this->UpdateDirtyBits();

    switch (evt.m_data.m_key) {
        case KEY_ESCAPE:
            this->RunOnEscapePressedScript();
            break;

        case KEY_ENTER:
            if (!(this->m_editBoxFlags & 0x2) || this->m_onEnterPressed.m_luaRef) {
                this->m_simpleTop->unused_1250 = 1;
                this->RunOnEnterPressedScript();
                this->m_simpleTop->unused_1250 = 0;
            } else {
                this->Insert("\n", nullptr, 1, 0, 0);
            }

            break;

        case KEY_BACKSPACE:
            if (IsControlDown()) {
                this->DeleteBackwardWord(0);
            } else {
                this->DeleteBackward(0);
            }

            break;

        case KEY_TAB:
            this->RunOnTabPressedScript();
            break;

        case KEY_LEFT:
            if (IsControlDown()) {
                this->MoveBackwardWord(IsShiftDown());
            } else {
                this->MoveBackward(IsShiftDown());
            }

            break;

        case KEY_UP:
            if (this->m_editBoxFlags & 0x2) {
                this->MoveLine(-1, IsShiftDown());
            } else {
                this->ForwardHistory();
            }

            break;

        case KEY_RIGHT:
            if (IsControlDown()) {
                this->MoveForwardWord(IsShiftDown());
            } else {
                this->MoveForward(IsShiftDown());
            }

            break;

        case KEY_DOWN:
            if (this->m_editBoxFlags & 0x2) {
                this->MoveLine(1, IsShiftDown());
            } else {
                this->BackwardHistory();
            }

            break;

        case KEY_INSERT:
            if (IsControlDown()) {
                if (this->m_highlightLeft != this->m_highlightRight) {
                    this->CopyToClipboard();
                }
            } else if (IsShiftDown()) {
                this->PasteFromClipboard();
            }

            break;

        case KEY_DELETE:
            if (IsShiftDown()) {
                if (this->m_highlightLeft != this->m_highlightRight) {
                    this->CopyToClipboard();
                    this->DeleteHighlight(0);
                }
            } else if (IsControlDown()) {
                this->DeleteForwardWord(0);
            } else {
                this->DeleteForward(0);
            }

            break;

        case KEY_HOME:
            if (IsControlDown()) {
                this->MoveToStart(IsShiftDown());
            } else {
                this->MoveToLineStart(IsShiftDown());
            }

            break;

        case KEY_END:
            if (IsControlDown()) {
                this->MoveToEnd(IsShiftDown());
            } else {
                this->MoveToLineEnd(IsShiftDown());
            }

            break;

        case KEY_A:
        case 'a':
            if (IsControlDown()) {
                this->HighlightText(0, -1);
            }

            break;

        case KEY_B:
        case 'b':
            if (IsControlDown()) {
                this->MoveBackward(IsShiftDown());
            }

            break;

        case KEY_C:
        case KEY_X:
        case 'c':
        case 'x':
            if (IsControlDown() && this->m_highlightLeft != this->m_highlightRight) {
                this->CopyToClipboard();

                if (evt.m_data.m_key == KEY_X || evt.m_data.m_key == 'x') {
                    this->DeleteHighlight(0);
                }
            }

            break;

        case KEY_D:
        case 'd':
            if (IsControlDown()) {
                this->DeleteForward(0);
            }

            break;

        case KEY_F:
        case 'f':
            if (IsControlDown()) {
                this->MoveForward(IsShiftDown());
            }

            break;

        case KEY_K:
        case 'k':
            if (IsControlDown()) {
                this->DeleteToEnd(0);
            }

            break;

        case KEY_N:
        case 'n':
            if (IsControlDown()) {
                this->BackwardHistory();
            }

            break;

        case KEY_P:
        case 'p':
            if (IsControlDown()) {
                this->ForwardHistory();
            }

            break;

        case KEY_U:
        case 'u':
            if (IsControlDown()) {
                this->DeleteToStart(0);
            }

            break;

        case KEY_V:
        case 'v':
            if (IsControlDown()) {
                this->PasteFromClipboard();
            }

            break;

        case KEY_W:
        case 'w':
            if (IsControlDown()) {
                this->DeleteBackwardWord(0);
            }

            break;
    }

    return 1;
}

// VERIFY: b2eac8d7 auto
// OFFSET: 0x00967CD0
int32_t CSimpleEditBox::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    int32_t result = CSimpleFrame::OnLayerMouseDown(evt, btn);

    if (result) {
        return result;
    }

    this->UpdateDirtyBits();

    int32_t index;

    if (!this->m_imeComposition && this->ConvertCoordinateToIndex(evt.m_data.m_mousePosition.x, evt.m_data.m_mousePosition.y, index)) {
        if (this->m_highlightLeft != this->m_highlightRight) {
            this->m_dirtyBits |= 0x2;
            this->m_highlightRight = 0;
            this->m_highlightLeft = 0;
        }

        this->SetCursorPosition(index);
        result = 1;
        this->m_highlightRight = this->m_cursorPos;
        this->m_highlightLeft = this->m_cursorPos;
        this->m_highlightDrag = 1;
    }

    CSimpleEditBox::SetKeyboardFocus(this);
    return result;
}
