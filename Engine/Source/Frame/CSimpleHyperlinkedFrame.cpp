#include <cstddef>
#include <cstdint>
#include "Engine/Source/Frame/CSimpleHyperlinkedFrame.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00DCE668
CDataAllocator CSimpleHyperlinkButton::s_allocator(sizeof(CSimpleHyperlinkButton), 10);

// INLINED
void CSimpleHyperlinkButton::operator delete(void* ptr) {
    if (ptr) {
        CSimpleHyperlinkButton::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 5a02c721 auto
// OFFSET: 0x00978540
void CSimpleHyperlinkButton::OnLayerCursorEnter(int32_t motion) {
    static_cast<CSimpleHyperlinkedFrame*>(this->m_parent)->OnHyperlinkEnter(this->m_linkData, this->m_linkText);
}

// VERIFY: e8af4768 auto
// OFFSET: 0x00978570
void CSimpleHyperlinkButton::OnLayerCursorExit(int32_t motion, int32_t a3) {
    static_cast<CSimpleHyperlinkedFrame*>(this->m_parent)->OnHyperlinkLeave(this->m_linkData, this->m_linkText);
}

// VERIFY: 4678b058 auto
// OFFSET: 0x009785A0
void CSimpleHyperlinkButton::OnClick(const char* btn, int32_t down) {
    static_cast<CSimpleHyperlinkedFrame*>(this->m_parent)->OnHyperlinkClick(this->m_linkData, this->m_linkText, btn);
}

// VERIFY: 33c565d4 manual
// OFFSET: 0x00978760
void CSimpleHyperlinkButton::SetHyperlink(CSimpleFontString* fontString, const GXUFONTHYPERLINKINFO* info) {
    if (!fontString || !info) {
        this->Hide();
        return;
    }

    this->m_linkData = static_cast<char*>(SMemReAlloc(this->m_linkData, info->m_linkDataLength + 1, __FILE__, __LINE__, 0));
    SStrCopy(this->m_linkData, info->m_linkData, info->m_linkDataLength + 1);

    this->m_linkText = static_cast<char*>(SMemReAlloc(this->m_linkText, info->m_linkTextLength + 1, __FILE__, __LINE__, 0));
    SStrCopy(this->m_linkText, info->m_linkText, info->m_linkTextLength + 1);

    float bottom = 0.0f;
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    NDCToDDC(info->m_indentation, info->m_baseLineHeight, &left, &bottom);
    NDCToDDC(info->m_right, info->m_lineHeight, &right, &top);

    float x = 0.0f;
    FRAMEPOINT point = FRAMEPOINT_TOPLEFT;

    switch (fontString->m_styleFlags & 0x7) {
        case 1:
            x = left;
            break;
        case 2:
            point = FRAMEPOINT_TOP;
            x = left + (right - left) * 0.5f;
            break;
        case 4:
            point = FRAMEPOINT_TOPRIGHT;
            x = right;
            break;
    }

    this->MarkFramePointsUnused();

    float scale = 1.0f / this->m_layoutScale;
    this->SetPoint(point, fontString, point, x * scale, top * scale, 1);
    this->SetWidth((right - left) / this->m_layoutScale);
    this->SetHeight((top - bottom) / this->m_layoutScale);
    this->Show();
}

// VERIFY: c24df541 auto
// OFFSET: 0x009789B0
CSimpleHyperlinkButton::CSimpleHyperlinkButton(CSimpleHyperlinkedFrame* parent) : CSimpleButton(parent) {
    this->SetClickAction(0xFFFFFFFF80000000ULL);
    this->m_linkData = nullptr;
    this->m_linkText = nullptr;
}

// VERIFY: e4fd8db4 auto
// OFFSET: [0x00978A00, 0x00978A10, 0x00978AA0]
CSimpleHyperlinkButton::~CSimpleHyperlinkButton() {
    if (this->m_linkData) {
        SMemFree(this->m_linkData, __FILE__, __LINE__, 0);
    }

    if (this->m_linkText) {
        SMemFree(this->m_linkText, __FILE__, __LINE__, 0);
    }
}

// VERIFY: b8408a8d auto
// OFFSET: 0x00978420
void CSimpleHyperlinkedFrame::RunOnHyperlinkEnterScript(const char* link, const char* text) {
    this->m_inHyperlinkScript = 1;

    if (this->m_onHyperlinkEnter.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, link);
        lua_pushstring(L, text);
        this->RunScript(this->m_onHyperlinkEnter, 2, nullptr);
    }

    this->m_inHyperlinkScript = 0;
}

// VERIFY: 2fce4294 auto
// OFFSET: 0x00978480
void CSimpleHyperlinkedFrame::RunOnHyperlinkLeaveScript(const char* link, const char* text) {
    this->m_inHyperlinkScript = 1;

    if (this->m_onHyperlinkLeave.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, link);
        lua_pushstring(L, text);
        this->RunScript(this->m_onHyperlinkLeave, 2, nullptr);
    }

    this->m_inHyperlinkScript = 0;
}

// VERIFY: bcc9be72 auto
// OFFSET: 0x009784E0
void CSimpleHyperlinkedFrame::RunOnHyperlinkClickScript(const char* link, const char* text, const char* btn) {
    this->m_inHyperlinkScript = 1;

    if (this->m_onHyperlinkClick.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, link);
        lua_pushstring(L, text);
        lua_pushstring(L, btn);
        this->RunScript(this->m_onHyperlinkClick, 3, nullptr);
    }

    this->m_inHyperlinkScript = 0;
}

// VERIFY: 0524d7db auto
// OFFSET: [0x009785D0, 0x00978750, 0x00978950]
CSimpleHyperlinkedFrame::~CSimpleHyperlinkedFrame() {
}

// VERIFY: ef46b435 auto
// OFFSET: 0x00978610
FrameScript_Object::ScriptIx* CSimpleHyperlinkedFrame::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = CSimpleFrame::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnHyperlinkEnter", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,link,text) %s end";
        return &this->m_onHyperlinkEnter;
    }

    if (!SStrCmpI(name, "OnHyperlinkLeave", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,link,text) %s end";
        return &this->m_onHyperlinkLeave;
    }

    if (!SStrCmpI(name, "OnHyperlinkClick", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,link,text,button) %s end";
        return &this->m_onHyperlinkClick;
    }

    return nullptr;
}

// VERIFY: 9a0495a8 auto
// OFFSET: 0x009786B0
void CSimpleHyperlinkedFrame::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onHyperlinkEnter, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onHyperlinkLeave, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onHyperlinkClick, time, count);
    CSimpleFrame::GetScriptTime(L, time, count, recurse);
}

// VERIFY: badc060d auto
// OFFSET: 0x00978720
void CSimpleHyperlinkedFrame::OnHyperlinkEnter(const char* link, const char* text) {
    this->RunOnHyperlinkEnterScript(link, text);
}

// VERIFY: 3cb33adf auto
// OFFSET: 0x00978730
void CSimpleHyperlinkedFrame::OnHyperlinkLeave(const char* link, const char* text) {
    this->RunOnHyperlinkLeaveScript(link, text);
}

// VERIFY: a2bf881e auto
// OFFSET: 0x00978740
void CSimpleHyperlinkedFrame::OnHyperlinkClick(const char* link, const char* text, const char* btn) {
    this->RunOnHyperlinkClickScript(link, text, btn);
}

// VERIFY: 279ba90e auto
// OFFSET: 0x00978900
CSimpleHyperlinkedFrame::CSimpleHyperlinkedFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->m_inHyperlinkScript = 0;
}
