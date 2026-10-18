#include <cstddef>
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleFont.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00B2D528
int32_t CSimpleButton::s_metatable = LUA_NOREF;

// STATIC: 0x00DCE650
int32_t CSimpleButton::s_objectType;

// STATIC: 0x00B2CD1C
const char* CSimpleButton::s_objectTypeName = "Button";

// STATIC: 0x00DCE654
CDataAllocator CSimpleButton::s_allocator(sizeof(CSimpleButton), 100);

// INLINED
void CSimpleButton::operator delete(void* ptr) {
    if (ptr) {
        CSimpleButton::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CSimpleButton::GetObjectType() {
    if (!CSimpleButton::s_objectType) {
        CSimpleButton::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleButton::s_objectType;
}

// VERIFY: cf256644 auto
// OFFSET: 0x009621E0
bool CSimpleButton::IsA(int32_t type) {
    return type == CSimpleButton::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: 296f9512 auto
// OFFSET: 0x00962220
bool CSimpleButton::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleButton::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
}

// VERIFY: 80dad073 auto
// OFFSET: 0x00962120
void CSimpleButton::LockHighlight(int32_t lock) {
    CSimpleFrame::LockHighlight(lock);
    this->UpdateTextState(this->m_state);
}

// VERIFY: ab04eabe manual
// OFFSET: 0x00962150
void CSimpleButton::SetMotionScriptsWhileDisabled(int32_t enabled) {
    this->m_motionScriptsWhileDisabled = enabled;
}

// VERIFY: 3131b961 auto
// OFFSET: 0x00962170
int32_t CSimpleButton::GetMotionScriptsWhileDisabled() {
    return this->m_motionScriptsWhileDisabled;
}

// VERIFY: a0ee9c2b auto
// OFFSET: 0x0096E9C0
uint64_t MouseButtonStringToBitmask(const char* string) {
    if (!string || !*string) {
        return 0;
    }

    if (!SStrCmpI(string, "LeftButtonDown", STORM_MAX_STR)) {
        return 0x1ULL;
    }

    if (!SStrCmpI(string, "LeftButtonUp", STORM_MAX_STR)) {
        return 0x80000000ULL;
    }

    if (!SStrCmpI(string, "MiddleButtonDown", STORM_MAX_STR)) {
        return 0x2ULL;
    }

    if (!SStrCmpI(string, "MiddleButtonUp", STORM_MAX_STR)) {
        return 0x100000000ULL;
    }

    if (!SStrCmpI(string, "RightButtonDown", STORM_MAX_STR)) {
        return 0x4ULL;
    }

    if (!SStrCmpI(string, "RightButtonUp", STORM_MAX_STR)) {
        return 0x200000000ULL;
    }

    if (!SStrCmpI(string, "Button4Down", STORM_MAX_STR)) {
        return 0x8ULL;
    }

    if (!SStrCmpI(string, "Button4Up", STORM_MAX_STR)) {
        return 0x400000000ULL;
    }

    if (!SStrCmpI(string, "Button5Down", STORM_MAX_STR)) {
        return 0x10ULL;
    }

    if (!SStrCmpI(string, "Button5Up", STORM_MAX_STR)) {
        return 0x800000000ULL;
    }

    if (!SStrCmpI(string, "Button6Down", STORM_MAX_STR)) {
        return 0x20ULL;
    }

    if (!SStrCmpI(string, "Button6Up", STORM_MAX_STR)) {
        return 0x1000000000ULL;
    }

    if (!SStrCmpI(string, "Button7Down", STORM_MAX_STR)) {
        return 0x40ULL;
    }

    if (!SStrCmpI(string, "Button7Up", STORM_MAX_STR)) {
        return 0x2000000000ULL;
    }

    if (!SStrCmpI(string, "Button8Down", STORM_MAX_STR)) {
        return 0x80ULL;
    }

    if (!SStrCmpI(string, "Button8Up", STORM_MAX_STR)) {
        return 0x4000000000ULL;
    }

    if (!SStrCmpI(string, "Button9Down", STORM_MAX_STR)) {
        return 0x100ULL;
    }

    if (!SStrCmpI(string, "Button9Up", STORM_MAX_STR)) {
        return 0x8000000000ULL;
    }

    if (!SStrCmpI(string, "Button10Down", STORM_MAX_STR)) {
        return 0x200ULL;
    }

    if (!SStrCmpI(string, "Button10Up", STORM_MAX_STR)) {
        return 0x10000000000ULL;
    }

    if (!SStrCmpI(string, "Button11Down", STORM_MAX_STR)) {
        return 0x400ULL;
    }

    if (!SStrCmpI(string, "Button11Up", STORM_MAX_STR)) {
        return 0x20000000000ULL;
    }

    if (!SStrCmpI(string, "Button12Up", STORM_MAX_STR)) {
        return 0x40000000000ULL;
    }

    if (!SStrCmpI(string, "Button12Down", STORM_MAX_STR)) {
        return 0x800ULL;
    }

    if (!SStrCmpI(string, "Button13Up", STORM_MAX_STR)) {
        return 0x80000000000ULL;
    }

    if (!SStrCmpI(string, "Button13Down", STORM_MAX_STR)) {
        return 0x1000ULL;
    }

    if (!SStrCmpI(string, "Button14Up", STORM_MAX_STR)) {
        return 0x100000000000ULL;
    }

    if (!SStrCmpI(string, "Button14Down", STORM_MAX_STR)) {
        return 0x2000ULL;
    }

    if (!SStrCmpI(string, "Button15Up", STORM_MAX_STR)) {
        return 0x200000000000ULL;
    }

    if (!SStrCmpI(string, "Button15Down", STORM_MAX_STR)) {
        return 0x4000ULL;
    }

    if (!SStrCmpI(string, "Button16Up", STORM_MAX_STR)) {
        return 0x400000000000ULL;
    }

    if (!SStrCmpI(string, "Button16Down", STORM_MAX_STR)) {
        return 0x8000ULL;
    }

    if (!SStrCmpI(string, "Button17Up", STORM_MAX_STR)) {
        return 0x800000000000ULL;
    }

    if (!SStrCmpI(string, "Button17Down", STORM_MAX_STR)) {
        return 0x10000ULL;
    }

    if (!SStrCmpI(string, "Button18Up", STORM_MAX_STR)) {
        return 0x1000000000000ULL;
    }

    if (!SStrCmpI(string, "Button18Down", STORM_MAX_STR)) {
        return 0x20000ULL;
    }

    if (!SStrCmpI(string, "Button19Up", STORM_MAX_STR)) {
        return 0x2000000000000ULL;
    }

    if (!SStrCmpI(string, "Button19Down", STORM_MAX_STR)) {
        return 0x40000ULL;
    }

    if (!SStrCmpI(string, "Button20Down", STORM_MAX_STR)) {
        return 0x80000ULL;
    }

    if (!SStrCmpI(string, "Button20Up", STORM_MAX_STR)) {
        return 0x4000000000000ULL;
    }

    if (!SStrCmpI(string, "Button21Down", STORM_MAX_STR)) {
        return 0x100000ULL;
    }

    if (!SStrCmpI(string, "Button21Up", STORM_MAX_STR)) {
        return 0x8000000000000ULL;
    }

    if (!SStrCmpI(string, "Button22Up", STORM_MAX_STR)) {
        return 0x10000000000000ULL;
    }

    if (!SStrCmpI(string, "Button22Down", STORM_MAX_STR)) {
        return 0x200000ULL;
    }

    if (!SStrCmpI(string, "Button23Up", STORM_MAX_STR)) {
        return 0x20000000000000ULL;
    }

    if (!SStrCmpI(string, "Button23Down", STORM_MAX_STR)) {
        return 0x400000ULL;
    }

    if (!SStrCmpI(string, "Button24Up", STORM_MAX_STR)) {
        return 0x40000000000000ULL;
    }

    if (!SStrCmpI(string, "Button24Down", STORM_MAX_STR)) {
        return 0x800000ULL;
    }

    if (!SStrCmpI(string, "Button25Up", STORM_MAX_STR)) {
        return 0x80000000000000ULL;
    }

    if (!SStrCmpI(string, "Button25Down", STORM_MAX_STR)) {
        return 0x1000000ULL;
    }

    if (!SStrCmpI(string, "Button26Up", STORM_MAX_STR)) {
        return 0x100000000000000ULL;
    }

    if (!SStrCmpI(string, "Button26Down", STORM_MAX_STR)) {
        return 0x2000000ULL;
    }

    if (!SStrCmpI(string, "Button27Up", STORM_MAX_STR)) {
        return 0x200000000000000ULL;
    }

    if (!SStrCmpI(string, "Button27Down", STORM_MAX_STR)) {
        return 0x4000000ULL;
    }

    if (!SStrCmpI(string, "Button28Up", STORM_MAX_STR)) {
        return 0x400000000000000ULL;
    }

    if (!SStrCmpI(string, "Button28Down", STORM_MAX_STR)) {
        return 0x8000000ULL;
    }

    if (!SStrCmpI(string, "Button29Up", STORM_MAX_STR)) {
        return 0x800000000000000ULL;
    }

    if (!SStrCmpI(string, "Button29Down", STORM_MAX_STR)) {
        return 0x10000000ULL;
    }

    if (!SStrCmpI(string, "Button30Down", STORM_MAX_STR)) {
        return 0x20000000ULL;
    }

    if (!SStrCmpI(string, "Button30Up", STORM_MAX_STR)) {
        return 0x1000000000000000ULL;
    }

    if (!SStrCmpI(string, "Button31Down", STORM_MAX_STR)) {
        return 0x40000000ULL;
    }

    if (!SStrCmpI(string, "Button31Up", STORM_MAX_STR)) {
        return 0x2000000000000000ULL;
    }

    if (!SStrCmpI(string, "AnyDown", STORM_MAX_STR)) {
        return 0x7FFFFFFFULL;
    }

    if (!SStrCmpI(string, "AnyUp", STORM_MAX_STR)) {
        return 0xFFFFFFFF80000000ULL;
    }

    return 0;
}

// VERIFY: 7ac18ca5 auto
// OFFSET: 0x0096F090
void CSimpleButton::RunOnClickScript(const char* btn, int32_t down) {
    if (this->m_onPreClick.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        lua_pushboolean(L, down);
        this->RunScript(this->m_onPreClick, 2, nullptr);
    }

    if (this->m_onClick.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        lua_pushboolean(L, down);
        this->RunScript(this->m_onClick, 2, nullptr);
    }

    if (this->m_onPostClick.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        lua_pushboolean(L, down);
        this->RunScript(this->m_onPostClick, 2, nullptr);
    }
}

// VERIFY: c1dafa11 auto
// OFFSET: 0x0096F150
void CSimpleButton::RunOnDoubleClickScript(const char* btn) {
    if (this->m_onDoubleClick.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        this->RunScript(this->m_onDoubleClick, 1, nullptr);
    }
}

// VERIFY: 1a2379ee auto
// OFFSET: [0x0096F190, 0x0096F520, 0x0096F740]
CSimpleButton::~CSimpleButton() {
}

// VERIFY: 6244a24f auto
// OFFSET: 0x0096F1E0
const char* CSimpleButton::GetObjectTypeName() {
    return CSimpleButton::s_objectTypeName;
}

// VERIFY: ea565c62 auto
// OFFSET: 0x0096F1F0
int32_t CSimpleButton::GetScriptMetaTable() {
    return CSimpleButton::s_metatable;
}

// VERIFY: d100e369 auto
// OFFSET: 0x0096F200
FrameScript_Object::ScriptIx* CSimpleButton::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "PreClick", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button,down) %s end";
        return &this->m_onPreClick;
    }

    if (!SStrCmpI(name, "OnClick", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button,down) %s end";
        return &this->m_onClick;
    }

    if (!SStrCmpI(name, "PostClick", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button,down) %s end";
        return &this->m_onPostClick;
    }

    if (!SStrCmpI(name, "OnDoubleClick", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button) %s end";
        return &this->m_onDoubleClick;
    }

    return nullptr;
}

// VERIFY: 781d19ca auto
// OFFSET: 0x0096F2D0
void CSimpleButton::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onPreClick, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onClick, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onPostClick, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onDoubleClick, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: d62e2602 auto
// OFFSET: 0x0096F350
void CSimpleButton::UnregisterRegion(CSimpleRegion* region) {
    if (region == this->m_fontString) {
        this->m_fontString = nullptr;
    }

    if (region == this->m_stateTextures[BUTTONSTATE_DISABLED]) {
        this->m_stateTextures[BUTTONSTATE_DISABLED] = nullptr;
    }

    if (region == this->m_stateTextures[BUTTONSTATE_NORMAL]) {
        this->m_stateTextures[BUTTONSTATE_NORMAL] = nullptr;
    }

    if (region == this->m_stateTextures[BUTTONSTATE_PUSHED]) {
        this->m_stateTextures[BUTTONSTATE_PUSHED] = nullptr;
    }

    if (region == this->m_activeTexture) {
        this->m_activeTexture = nullptr;
    }

    if (region == this->m_highlightTexture) {
        this->m_highlightTexture = nullptr;
    }

    CSimpleFrame::UnregisterRegion(region);
}

// VERIFY: 3ee789b5 auto
// OFFSET: 0x0096F3C0
void CSimpleButton::SetHighlight(CSimpleTexture* texture, EGxBlend blend) {
    if (texture == this->m_highlightTexture) {
        return;
    }

    if (this->m_highlightTexture) {
        delete this->m_highlightTexture;
    }

    if (texture) {
        texture->SetFrame(this, 4, 1);
        texture->SetBlendMode(blend);
    }

    this->m_highlightTexture = texture;
}

// VERIFY: 8c6874f5 auto
// OFFSET: 0x0096F410
void CSimpleButton::OnLayerHide() {
    if (this->m_state != BUTTONSTATE_DISABLED && !this->m_stateLocked) {
        this->SetButtonState(BUTTONSTATE_NORMAL, 0);
    }

    CSimpleFrame::OnLayerHide();
}

// VERIFY: 7324b618 auto
// OFFSET: 0x0096F440
void CSimpleButton::OnDragStart(const CMouseEvent& evt) {
    if (this->m_state != BUTTONSTATE_DISABLED && !this->m_stateLocked) {
        this->SetButtonState(BUTTONSTATE_NORMAL, 0);
    }

    if (this->m_state != BUTTONSTATE_DISABLED) {
        CSimpleFrame::OnDragStart(evt);
    }
}

// VERIFY: 78f1af9e auto
// OFFSET: 0x0096F480
void CSimpleButton::SetClickAction(uint64_t action) {
    this->m_clickAction = action;
}

// VERIFY: e5ddb2a8 auto
// OFFSET: 0x0096F4A0
void CSimpleButton::UpdateTextState(CSimpleButtonState state) {
    if (!this->m_fontString) {
        return;
    }

    if (state == BUTTONSTATE_PUSHED) {
        this->m_fontString->SetJustificationOffset(this->m_pushedTextOffset.x, this->m_pushedTextOffset.y);
    } else {
        this->m_fontString->SetJustificationOffset(0.0f, 0.0f);

        if (state == BUTTONSTATE_DISABLED) {
            this->m_fontString->SetFontObject(this->m_disabledFont);
            return;
        }
    }

    if (this->m_drawLayerEnabled[4]) {
        this->m_fontString->SetFontObject(this->m_highlightFont);
    } else {
        this->m_fontString->SetFontObject(this->m_normalFont);
    }
}

// VERIFY: f5952a56 auto
// OFFSET: 0x0096F530
const char* GetMouseButtonName(uint32_t button) {
    switch (button) {
        case 0x1:
            return "LeftButton";
        case 0x2:
            return "MiddleButton";
        case 0x4:
            return "RightButton";
        case 0x8:
            return "Button4";
        case 0x10:
            return "Button5";
        case 0x20:
            return "Button6";
        case 0x40:
            return "Button7";
        case 0x80:
            return "Button8";
        case 0x100:
            return "Button9";
        case 0x200:
            return "Button10";
        case 0x400:
            return "Button11";
        case 0x800:
            return "Button12";
        case 0x1000:
            return "Button13";
        case 0x2000:
            return "Button14";
        case 0x4000:
            return "Button15";
        case 0x8000:
            return "Button16";
        case 0x10000:
            return "Button17";
        case 0x20000:
            return "Button18";
        case 0x40000:
            return "Button19";
        case 0x80000:
            return "Button20";
        case 0x100000:
            return "Button21";
        case 0x200000:
            return "Button22";
        case 0x400000:
            return "Button23";
        case 0x800000:
            return "Button24";
        case 0x1000000:
            return "Button25";
        case 0x2000000:
            return "Button26";
        case 0x4000000:
            return "Button27";
        case 0x8000000:
            return "Button28";
        case 0x10000000:
            return "Button29";
        case 0x20000000:
            return "Button30";
        case 0x40000000:
            return "Button31";
        default:
            return "UNKNOWN";
    }
}

// VERIFY: 791e4917 auto
// OFFSET: 0x0096F7B0
void CSimpleButton::SetFontString(CSimpleFontString* fontString) {
    if (fontString == this->m_fontString) {
        return;
    }

    if (this->m_fontString) {
        delete this->m_fontString;
    }

    if (fontString) {
        fontString->SetFrame(this, 2, 1);

        if (!fontString->HasAnyFramepoints()) {
            uint32_t styleFlags = this->m_normalFont ? this->m_normalFont->m_attributes.m_styleFlags : 0;

            if (styleFlags & 0x1) {
                fontString->SetPoint(FRAMEPOINT_LEFT, this, FRAMEPOINT_LEFT, 0.0f, 0.0f, 1);
            } else if (styleFlags & 0x4) {
                fontString->SetPoint(FRAMEPOINT_RIGHT, this, FRAMEPOINT_RIGHT, 0.0f, 0.0f, 1);
            } else {
                fontString->SetPoint(FRAMEPOINT_CENTER, this, FRAMEPOINT_CENTER, 0.0f, 0.0f, 1);
            }
        }
    }

    this->m_fontString = fontString;
    this->UpdateTextState(this->m_state);
}

// VERIFY: 9b5e101b manual
// OFFSET: 0x0096F860
void CSimpleButton::SetText(const char* text) {
    if ((!text || !*text) && !this->m_fontString) {
        return;
    }

    if (!this->m_fontString) {
        void* data = CSimpleFontString::s_allocator.GetData(0, StormTypeName(typeid(CSimpleFontString)), SLOG_OBJECT);
        this->SetFontString(data ? new (data) CSimpleFontString(this, 2, true) : nullptr);
    }

    this->m_fontString->SetText(text, 1);
}

// VERIFY: 0a129d6e manual
// OFFSET: 0x0096F8D0
void CSimpleButton::SetNormalFontColor(const CImVector& color) {
    CSimpleFont* font = this->m_normalFont;

    if (!font) {
        return;
    }

    if (*reinterpret_cast<const uint32_t*>(&color) != *reinterpret_cast<uint32_t*>(&font->m_attributes.m_color) || !(font->m_attributes.m_flags & 0x400)) {
        font->m_attributes.m_color = color;
        font->m_attributes.m_flags |= 0x404;
    }

    this->m_normalFont->m_fontableFlags &= ~0x4u;
    this->m_normalFont->UpdateObjects();
}

// VERIFY: 5e94ba11 auto
// OFFSET: 0x0096F920
void CSimpleButton::SetStateTexture(CSimpleButtonState state, CSimpleTexture* texture) {
    if (texture == this->m_stateTextures[state]) {
        return;
    }

    if (this->m_activeTexture == this->m_stateTextures[state]) {
        this->m_activeTexture = nullptr;
    }

    if (this->m_stateTextures[state]) {
        delete this->m_stateTextures[state];
    }

    if (texture) {
        texture->SetFrame(this, 2, 0);
    }

    this->m_stateTextures[state] = texture;

    if (texture && state == this->m_state) {
        this->m_activeTexture = texture;
        texture->m_shown = 1;
        texture->ShowThis();
    }
}

// VERIFY: 8dd71c35 manual
// OFFSET: 0x0096F9B0
int32_t CSimpleButton::SetHighlight(const char* fileName, EGxBlend blend) {
    CSimpleTexture* texture = this->m_highlightTexture;

    if (texture) {
        texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        texture->SetBlendMode(blend);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    texture = data ? new (data) CSimpleTexture(this, 4, 1) : nullptr;

    if (!texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        if (texture) {
            delete texture;
        }

        return 0;
    }

    texture->SetAllPoints(this, 1);
    texture->SetBlendMode(blend);
    this->m_highlightTexture = texture;
    return 1;
}

// VERIFY: 093d46bd auto
// OFFSET: 0x0096FA80
int32_t CSimpleButton::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    int32_t result = CSimpleFrame::OnLayerMouseDown(evt, btn);

    if (result) {
        return result;
    }

    if (!btn) {
        C2Vector pt;
        pt.x = evt.m_data.m_mousePosition.x;
        pt.y = evt.m_data.m_mousePosition.y;

        if (!this->TestHitRect(pt)) {
            return 0;
        }
    }

    if (this->m_state == BUTTONSTATE_DISABLED) {
        return 1;
    }

    uint32_t button = btn ? 0x7FFFFFFF : evt.m_data.m_button;

    if (this->m_clickAction & button) {
        if (!btn) {
            btn = GetMouseButtonName(evt.m_data.m_button);
        }

        this->m_simpleTop->unused_1250 = 1;
        this->OnClick(btn, 1);
        this->m_simpleTop->unused_1250 = 0;
    }

    if (!this->m_stateLocked && this->m_state != BUTTONSTATE_DISABLED) {
        this->SetButtonState(BUTTONSTATE_PUSHED, 0);
    }

    return 1;
}

// VERIFY: 3e18137e auto
// OFFSET: 0x0096FB60
int32_t CSimpleButton::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    int32_t result = CSimpleFrame::OnLayerMouseUp(evt, btn);

    if (result) {
        return result;
    }

    if (this->m_state != BUTTONSTATE_PUSHED) {
        return 1;
    }

    uint64_t button = 0;

    if (btn) {
        button = 0xFFFFFFFF80000000ULL;
    } else {
        C2Vector pt;
        pt.x = evt.m_data.m_mousePosition.x;
        pt.y = evt.m_data.m_mousePosition.y;

        if (this->TestHitRect(pt)) {
            button = static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(evt.m_data.m_button)) << 31);
        }
    }

    if (this->m_clickAction & button) {
        if (!btn) {
            btn = GetMouseButtonName(evt.m_data.m_button);
        }

        this->m_simpleTop->unused_1250 = 1;
        uint32_t time = OsGetAsyncTimeMs();

        if (this->m_onDoubleClick.m_luaRef && this->m_lastClickTime && time - this->m_lastClickTime <= 300) {
            this->OnDoubleClick(btn);
            this->m_lastClickTime = 0;
        } else {
            this->OnClick(btn, 0);
            this->m_lastClickTime = time;
        }

        this->m_simpleTop->unused_1250 = 0;
    }

    if (!this->m_stateLocked && this->m_state != BUTTONSTATE_DISABLED) {
        this->SetButtonState(BUTTONSTATE_NORMAL, 0);
    }

    return 1;
}

// VERIFY: a6303e9e auto
// OFFSET: 0x0096FCA0
void CSimpleButton::OnLayerCursorEnter(int32_t motion) {
    if (this->m_state != BUTTONSTATE_DISABLED) {
        CSimpleFrame::OnLayerCursorEnter(motion);
        this->UpdateTextState(this->m_state);
        return;
    }

    if (this->GetMotionScriptsWhileDisabled()) {
        if (this->m_hoverFrame) {
            this->m_hoverFrame->Show();
        }

        this->RunOnEnterScript(motion);
    }
}

// VERIFY: f683541a auto
// OFFSET: 0x0096FD00
void CSimpleButton::OnLayerCursorExit(int32_t motion, int32_t a3) {
    if (this->m_state != BUTTONSTATE_DISABLED) {
        CSimpleFrame::OnLayerCursorExit(motion, a3);
        this->UpdateTextState(this->m_state);
        return;
    }

    if (this->GetMotionScriptsWhileDisabled()) {
        if (this->m_hoverFrame) {
            this->m_hoverFrame->Hide();
        }

        this->RunOnLeaveScript(motion);
    }
}

// VERIFY: cddc7e35 auto
// OFFSET: 0x0096FD70
void CSimpleButton::OnClick(const char* btn, int32_t down) {
    if (this->m_inClick || this->m_state == BUTTONSTATE_DISABLED) {
        return;
    }

    const char* savedButtonName = this->m_simpleTop->m_mouseButtonName;
    this->m_simpleTop->m_mouseButtonName = btn;
    this->m_inClick = 1;
    this->RunOnClickScript(btn, down);
    this->m_inClick = 0;
    this->m_simpleTop->m_mouseButtonName = savedButtonName;
}

// VERIFY: 7a969f93 auto
// OFFSET: 0x0096FDD0
void CSimpleButton::OnDoubleClick(const char* btn) {
    if (this->m_inClick || this->m_state == BUTTONSTATE_DISABLED) {
        return;
    }

    const char* savedButtonName = this->m_simpleTop->m_mouseButtonName;
    this->m_simpleTop->m_mouseButtonName = btn;
    this->m_inClick = 1;
    this->RunOnDoubleClickScript(btn);
    this->m_inClick = 0;
    this->m_simpleTop->m_mouseButtonName = savedButtonName;
}

// VERIFY: d16dc184 manual
// OFFSET: 0x0096FE30
void CSimpleButton::SetButtonState(CSimpleButtonState state, int32_t stateLocked) {
    this->m_stateLocked = stateLocked;

    if (state == this->m_state) {
        return;
    }

    if (this->m_activeTexture && (this->m_stateTextures[state] || state == BUTTONSTATE_NORMAL)) {
        this->m_activeTexture->m_shown = 0;
        this->m_activeTexture->HideThis();
        this->m_activeTexture = nullptr;
    }

    if (this->m_stateTextures[state]) {
        this->m_activeTexture = this->m_stateTextures[state];
        this->m_activeTexture->m_shown = 1;
        this->m_activeTexture->ShowThis();
    }

    this->UpdateTextState(state);
    this->m_state = state;
}

// VERIFY: 45a8641e auto
// OFFSET: 0x0096FED0
void CSimpleButton::LoadXML(XMLNode* node, CStatus* status) {
    CSimpleFrame::LoadXML(node, status);

    CSimpleFontString* fontString = nullptr;

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "NormalTexture", STORM_MAX_STR)) {
            this->SetStateTexture(BUTTONSTATE_NORMAL, LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "PushedTexture", STORM_MAX_STR)) {
            this->SetStateTexture(BUTTONSTATE_PUSHED, LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "DisabledTexture", STORM_MAX_STR)) {
            this->SetStateTexture(BUTTONSTATE_DISABLED, LoadXML_Texture(child, this, status));
        } else if (!SStrCmpI(child->m_name.m_str, "HighlightTexture", STORM_MAX_STR)) {
            CSimpleTexture* texture = LoadXML_Texture(child, this, status);
            this->SetHighlight(texture, texture->m_alphaMode);
        } else if (!SStrCmpI(child->m_name.m_str, "ButtonText", STORM_MAX_STR)) {
            fontString = LoadXML_String(child, this, status);
        } else if (!SStrCmpI(child->m_name.m_str, "NormalFont", STORM_MAX_STR)) {
            const char* style = child->GetAttributeByName("style");

            if (style && *style) {
                this->m_normalFont = CSimpleFont::GetFont(style, 0);

                if (!this->m_normalFont) {
                    status->Add(STATUS_WARNING, "Couldn't find NORMAL font: %s", style);
                }
            }
        } else if (!SStrCmpI(child->m_name.m_str, "DisabledFont", STORM_MAX_STR)) {
            const char* style = child->GetAttributeByName("style");

            if (style && *style) {
                this->m_disabledFont = CSimpleFont::GetFont(style, 0);

                if (!this->m_disabledFont) {
                    status->Add(STATUS_WARNING, "Couldn't find DISABLED font: %s", style);
                }
            }
        } else if (!SStrCmpI(child->m_name.m_str, "HighlightFont", STORM_MAX_STR)) {
            const char* style = child->GetAttributeByName("style");

            if (style && *style) {
                this->m_highlightFont = CSimpleFont::GetFont(style, 0);

                if (!this->m_highlightFont) {
                    status->Add(STATUS_WARNING, "Couldn't find HIGHLIGHT font: %s", style);
                }
            }
        } else if (!SStrCmpI(child->m_name.m_str, "NormalColor", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetNormalFontColor(color);
        } else if (!SStrCmpI(child->m_name.m_str, "DisabledColor", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetNormalFontColor(color);
        } else if (!SStrCmpI(child->m_name.m_str, "HighlightColor", STORM_MAX_STR)) {
            CImVector color;
            LoadXML_Color(child, color, status);
            this->SetNormalFontColor(color);
        } else if (!SStrCmpI(child->m_name.m_str, "PushedTextOffset", STORM_MAX_STR)) {
            float x;
            float y;

            if (LoadXML_Dimensions(child, x, y, status)) {
                this->m_pushedTextOffset.x = x;
                this->m_pushedTextOffset.y = y;
            }
        }
    }

    if (fontString) {
        this->SetFontString(fontString);
    } else {
        this->UpdateTextState(this->m_state);
    }

    const char* text = node->GetAttributeByName("text");

    if (text && *text) {
        const char* localized = FrameScript_GetText(text, -1, GENDER_NOT_APPLICABLE);

        if (!localized || !*localized) {
            localized = text;
        }

        this->SetText(localized);
    }

    const char* registerForClicks = node->GetAttributeByName("registerForClicks");

    if (registerForClicks && *registerForClicks) {
        uint32_t clickAction = 0;
        char token[1024];

        SStrTokenize(&registerForClicks, token, sizeof(token), " ,", nullptr);

        while (token[0]) {
            clickAction |= static_cast<uint32_t>(MouseButtonStringToBitmask(token));
            SStrTokenize(&registerForClicks, token, sizeof(token), " ,", nullptr);
        }

        this->m_clickAction = clickAction;
    }

    const char* motionScriptsWhileDisabled = node->GetAttributeByName("motionScriptsWhileDisabled");

    if (motionScriptsWhileDisabled) {
        this->SetMotionScriptsWhileDisabled(StringToBOOL(motionScriptsWhileDisabled));
    }
}

// VERIFY: b8ad7d54 manual
// OFFSET: 0x00970370
int32_t CSimpleButton::SetStateTexture(CSimpleButtonState state, const char* fileName) {
    if (this->m_stateTextures[state]) {
        this->m_stateTextures[state]->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal);
        return 1;
    }

    void* data = CSimpleTexture::s_allocator.GetData(0, StormTypeName(typeid(CSimpleTexture)), SLOG_OBJECT);
    CSimpleTexture* texture = data ? new (data) CSimpleTexture(nullptr, 2, 1) : nullptr;

    if (!texture->SetTexture(fileName, 0, 0, CSimpleTexture::s_textureFilterMode, ImageMode_Normal)) {
        if (texture) {
            delete texture;
        }

        return 0;
    }

    texture->SetAllPoints(this, 1);
    this->SetStateTexture(state, texture);
    return 1;
}

// VERIFY: 3f2ae434 manual
// OFFSET: 0x00970440
void CSimpleButton::Enable(int32_t enabled) {
    if (enabled) {
        if (this->m_state != BUTTONSTATE_DISABLED) {
            return;
        }

        this->SetButtonState(BUTTONSTATE_NORMAL, 0);

        if (this == this->m_simpleTop->m_mouseFocus) {
            this->OnLayerCursorEnter(0);
        }

        if (this->m_highlightLocked) {
            this->EnableDrawLayer(4);
        }
    } else {
        if (this->m_state == BUTTONSTATE_DISABLED) {
            return;
        }

        if (this == this->m_simpleTop->m_mouseFocus) {
            this->OnLayerCursorExit(0, 1);
        }

        this->DisableDrawLayer(4);
        this->SetButtonState(BUTTONSTATE_DISABLED, 0);
    }

    int32_t disabled = enabled == 0;
    this->SetFrameFlag(0x400, disabled);

    FrameScript_Object::ScriptIx& script = disabled ? this->m_onDisable : this->m_onEnable;

    if (script.m_luaRef && !this->m_isLoading) {
        this->RunScript(script, 0, nullptr);
    }
}

// VERIFY: 7a7f3ab5 auto
// OFFSET: 0x00970520
CSimpleButton::CSimpleButton(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_state = BUTTONSTATE_DISABLED;
    this->m_inClick = 0;
    this->m_stateLocked = 0;
    this->m_motionScriptsWhileDisabled = 0;
    this->m_lastClickTime = 0;
    this->m_clickAction = 0x80000000;
    this->m_fontString = nullptr;
    this->m_normalFont = nullptr;
    this->m_highlightFont = nullptr;
    this->m_disabledFont = nullptr;
    this->m_activeTexture = nullptr;
    this->m_highlightTexture = nullptr;
    this->m_pushedTextOffset.x = 0.001f;
    this->m_pushedTextOffset.y = -0.001f;

    for (uint32_t i = 0; i < NUM_BUTTONSTATES; i++) {
        this->m_stateTextures[i] = nullptr;
    }

    this->Enable(1);
    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
    this->SetFrameFlag(0x10000, 1);
}
