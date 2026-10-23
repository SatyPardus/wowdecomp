#ifndef ENGINE_SOURCE_FRAME_CSIMPLEBUTTON_H
#define ENGINE_SOURCE_FRAME_CSIMPLEBUTTON_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Gx/Types.h"
#include "Engine/Source/Tempest/C2Vector.h"

class CImVector;
class CMouseEvent;
class CSimpleFont;
class CSimpleFontString;
class CSimpleRegion;
class CSimpleTexture;
class CStatus;
class XMLNode;
struct lua_State;

enum CLICK_ACTION_FLAGS : uint64_t {
    CLICK_ON_NONE = 0,
    CLICK_ON_LEFTDOWN = 0x0000000000000001ULL,
    CLICK_ON_LEFTUP = 0x0000000080000000ULL,
    CLICK_ON_MIDDLEDOWN = 0x0000000000000002ULL,
    CLICK_ON_MIDDLEUP = 0x0000000100000000ULL,
    CLICK_ON_RIGHTDOWN = 0x0000000000000004ULL,
    CLICK_ON_RIGHTUP = 0x0000000200000000ULL,
    CLICK_ON_X1DOWN = 0x0000000000000008ULL,
    CLICK_ON_X1UP = 0x0000000400000000ULL,
    CLICK_ON_X2DOWN = 0x0000000000000010ULL,
    CLICK_ON_X2UP = 0x0000000800000000ULL,
    CLICK_ON_BUTTON6DOWN = 0x0000000000000020ULL,
    CLICK_ON_BUTTON6UP = 0x0000001000000000ULL,
    CLICK_ON_BUTTON7DOWN = 0x0000000000000040ULL,
    CLICK_ON_BUTTON7UP = 0x0000002000000000ULL,
    CLICK_ON_BUTTON8DOWN = 0x0000000000000080ULL,
    CLICK_ON_BUTTON8UP = 0x0000004000000000ULL,
    CLICK_ON_BUTTON9DOWN = 0x0000000000000100ULL,
    CLICK_ON_BUTTON9UP = 0x0000008000000000ULL,
    CLICK_ON_BUTTON10DOWN = 0x0000000000000200ULL,
    CLICK_ON_BUTTON10UP = 0x0000010000000000ULL,
    CLICK_ON_BUTTON11DOWN = 0x0000000000000400ULL,
    CLICK_ON_BUTTON11UP = 0x0000020000000000ULL,
    CLICK_ON_BUTTON12UP = 0x0000040000000000ULL,
    CLICK_ON_BUTTON12DOWN = 0x0000000000000800ULL,
    CLICK_ON_BUTTON13UP = 0x0000080000000000ULL,
    CLICK_ON_BUTTON13DOWN = 0x0000000000001000ULL,
    CLICK_ON_BUTTON14UP = 0x0000100000000000ULL,
    CLICK_ON_BUTTON14DOWN = 0x0000000000002000ULL,
    CLICK_ON_BUTTON15UP = 0x0000200000000000ULL,
    CLICK_ON_BUTTON15DOWN = 0x0000000000004000ULL,
    CLICK_ON_BUTTON16UP = 0x0000400000000000ULL,
    CLICK_ON_BUTTON16DOWN = 0x0000000000008000ULL,
    CLICK_ON_BUTTON17UP = 0x0000800000000000ULL,
    CLICK_ON_BUTTON17DOWN = 0x0000000000010000ULL,
    CLICK_ON_BUTTON18UP = 0x0001000000000000ULL,
    CLICK_ON_BUTTON18DOWN = 0x0000000000020000ULL,
    CLICK_ON_BUTTON19UP = 0x0002000000000000ULL,
    CLICK_ON_BUTTON19DOWN = 0x0000000000040000ULL,
    CLICK_ON_BUTTON20DOWN = 0x0000000000080000ULL,
    CLICK_ON_BUTTON20UP = 0x0004000000000000ULL,
    CLICK_ON_BUTTON21DOWN = 0x0000000000100000ULL,
    CLICK_ON_BUTTON21UP = 0x0008000000000000ULL,
    CLICK_ON_BUTTON22UP = 0x0010000000000000ULL,
    CLICK_ON_BUTTON22DOWN = 0x0000000000200000ULL,
    CLICK_ON_BUTTON23UP = 0x0020000000000000ULL,
    CLICK_ON_BUTTON23DOWN = 0x0000000000400000ULL,
    CLICK_ON_BUTTON24UP = 0x0040000000000000ULL,
    CLICK_ON_BUTTON24DOWN = 0x0000000000800000ULL,
    CLICK_ON_BUTTON25UP = 0x0080000000000000ULL,
    CLICK_ON_BUTTON25DOWN = 0x0000000001000000ULL,
    CLICK_ON_BUTTON26UP = 0x0100000000000000ULL,
    CLICK_ON_BUTTON26DOWN = 0x0000000002000000ULL,
    CLICK_ON_BUTTON27UP = 0x0200000000000000ULL,
    CLICK_ON_BUTTON27DOWN = 0x0000000004000000ULL,
    CLICK_ON_BUTTON28UP = 0x0400000000000000ULL,
    CLICK_ON_BUTTON28DOWN = 0x0000000008000000ULL,
    CLICK_ON_BUTTON29UP = 0x0800000000000000ULL,
    CLICK_ON_BUTTON29DOWN = 0x0000000010000000ULL,
    CLICK_ON_BUTTON30DOWN = 0x0000000020000000ULL,
    CLICK_ON_BUTTON30UP = 0x1000000000000000ULL,
    CLICK_ON_BUTTON31DOWN = 0x0000000040000000ULL,
    CLICK_ON_BUTTON31UP = 0x2000000000000000ULL,
    CLICK_ON_ANYDOWN = 0x000000007FFFFFFFULL,
    CLICK_ON_ANYUP = 0xFFFFFFFF80000000ULL,
};

class CSimpleButton : public CSimpleFrame {
    public:
    static CDataAllocator s_allocator;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 029C */ uint32_t m_lastClickTime;
    /* 02A0 */ uint64_t m_clickAction;
    /* 02A8 */ CSimpleButtonState m_state : 4;
    /* 02A8 */ uint32_t m_inClick : 1;
    /* 02A8 */ uint32_t m_stateLocked : 1;
    /* 02A8 */ int32_t m_motionScriptsWhileDisabled : 1;
    /* 02AC */ CSimpleFontString* m_fontString;
    /* 02B0 */ CSimpleFont* m_normalFont;
    /* 02B4 */ CSimpleFont* m_highlightFont;
    /* 02B8 */ CSimpleFont* m_disabledFont;
    /* 02BC */ C2Vector m_pushedTextOffset;
    /* 02C4 */ CSimpleTexture* m_stateTextures[3];
    /* 02D0 */ CSimpleTexture* m_activeTexture;
    /* 02D4 */ CSimpleTexture* m_highlightTexture;
    /* 02D8 */ FrameScript_Object::ScriptIx m_onPreClick;
    /* 02E0 */ FrameScript_Object::ScriptIx m_onClick;
    /* 02E8 */ FrameScript_Object::ScriptIx m_onPostClick;
    /* 02F0 */ FrameScript_Object::ScriptIx m_onDoubleClick;

    virtual ~CSimpleButton();
    virtual int32_t GetScriptMetaTable();
    virtual FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data);
    virtual bool IsA(int32_t type);
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();
    virtual void GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse);
    virtual void UnregisterRegion(CSimpleRegion* region);
    virtual void OnLayerHide();
    virtual void OnLayerCursorEnter(int32_t motion);
    virtual void OnLayerCursorExit(int32_t motion, int32_t a3);
    virtual int32_t OnLayerMouseDown(const CMouseEvent& evt, const char* btn);
    virtual int32_t OnLayerMouseUp(const CMouseEvent& evt, const char* btn);
    virtual void OnDragStart(const CMouseEvent& evt);
    virtual void LockHighlight(int32_t lock);
    virtual void LoadXML(XMLNode* node, CStatus* status);
    virtual void Enable(int32_t enabled);
    virtual void OnClick(const char* btn, int32_t down);
    virtual void OnDoubleClick(const char* btn);
    virtual void SetButtonState(CSimpleButtonState state, int32_t stateLocked);
    virtual void SetMotionScriptsWhileDisabled(int32_t enabled);
    virtual int32_t GetMotionScriptsWhileDisabled();

    CSimpleButton(CSimpleFrame* parent);
    void RunOnClickScript(const char* btn, int32_t down);
    void RunOnDoubleClickScript(const char* btn);
    void SetClickAction(uint64_t action);
    void SetFontString(CSimpleFontString* fontString);
    int32_t SetHighlight(const char* fileName, EGxBlend blend);
    void SetHighlight(CSimpleTexture* texture, EGxBlend blend);
    void SetNormalFontColor(const CImVector& color);
    int32_t SetStateTexture(CSimpleButtonState state, const char* fileName);
    void SetStateTexture(CSimpleButtonState state, CSimpleTexture* texture);
    void SetText(const char* text);
    void UpdateTextState(CSimpleButtonState state);
};

const char* GetMouseButtonName(uint32_t button);
uint64_t MouseButtonStringToBitmask(const char* string);

#endif
