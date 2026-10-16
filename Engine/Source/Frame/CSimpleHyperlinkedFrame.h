#ifndef ENGINE_SOURCE_FRAME_CSIMPLEHYPERLINKEDFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEHYPERLINKEDFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Gxu/Types.h"
#include "Engine/Source/Storm/H/stpl.h"

class CSimpleFontString;
struct lua_State;

class CSimpleHyperlinkedFrame : public CSimpleFrame {
    public:
    /* 029C */ uint8_t m_inHyperlinkScript;
    /* 02A0 */ FrameScript_Object::ScriptIx m_onHyperlinkEnter;
    /* 02A8 */ FrameScript_Object::ScriptIx m_onHyperlinkLeave;
    /* 02B0 */ FrameScript_Object::ScriptIx m_onHyperlinkClick;

    virtual ~CSimpleHyperlinkedFrame();
    virtual FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data);
    virtual void GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse);
    virtual void OnHyperlinkEnter(const char* link, const char* text);
    virtual void OnHyperlinkLeave(const char* link, const char* text);
    virtual void OnHyperlinkClick(const char* link, const char* text, const char* btn);

    CSimpleHyperlinkedFrame(CSimpleFrame* parent);
    void RunOnHyperlinkClickScript(const char* link, const char* text, const char* btn);
    void RunOnHyperlinkEnterScript(const char* link, const char* text);
    void RunOnHyperlinkLeaveScript(const char* link, const char* text);
};

class CSimpleHyperlinkButton : public CSimpleButton {
    public:
    static CDataAllocator s_allocator;

    static void operator delete(void* ptr);

    /* 02F8 */ char* m_linkData;
    /* 02FC */ char* m_linkText;
    /* 0300 */ TSLink<CSimpleHyperlinkButton> m_hyperlinkLink;

    virtual ~CSimpleHyperlinkButton();
    virtual void OnLayerCursorEnter(int32_t motion);
    virtual void OnLayerCursorExit(int32_t motion, int32_t a3);
    virtual void OnClick(const char* btn, int32_t down);

    CSimpleHyperlinkButton(CSimpleHyperlinkedFrame* parent);
    void SetHyperlink(CSimpleFontString* fontString, const GXUFONTHYPERLINKINFO* info);
};

#endif
