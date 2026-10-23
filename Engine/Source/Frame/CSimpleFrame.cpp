#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Event/CCharEvent.h"
#include "Engine/Source/Event/CKeyEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CBackdropGenerator.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CSimpleAlphaAnim.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Frame/CSimpleRender.h"
#include "Engine/Source/Frame/CSimpleScrollFrame.h"
#include "Engine/Source/Frame/CSimpleTexture.h"
#include "Engine/Source/Frame/CSimpleTitleRegion.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CRect.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"

// STATIC: 0x00AC154C
int32_t CSimpleFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00B49984
int32_t CSimpleFrame::s_objectType;

// STATIC: 0x00AC1260
const char* CSimpleFrame::s_objectTypeName = "Frame";

// STATIC: 0x00B49980
int32_t CSimpleFrame::s_frameCount;

// STATIC: 0x00B49988
CDataAllocator CSimpleFrame::s_allocator(sizeof(CSimpleFrame), 100);

// STATIC: 0x00AC1264
static float s_dragThreshold = 0.0001f;

// VERIFY: fc6506aa manual
// OFFSET: 0x0048EE40
static const char* KeyCodeToString(KEY key) {
    static char charBuf[8];

    if (key - 33 <= 0xDE) {
        sputu8(key, charBuf);
        return charBuf;
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
            SStrPrintf(charBuf, sizeof(charBuf), "NUMPAD%d", key - KEY_NUMPAD0);
            return charBuf;
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
        case KEY_NUMPAD_EQUALS:
            return "NUMPADEQUALS";
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
            SStrPrintf(charBuf, sizeof(charBuf), "F%d", key - KEY_F1 + 1);
            return charBuf;
        default:
            return "UNKNOWN";
    }
}

// VERIFY: f47c2198 auto
// OFFSET: 0x0049CAB0
int32_t CSimpleFrame::GetObjectType() {
    if (!CSimpleFrame::s_objectType) {
        CSimpleFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CSimpleFrame::s_objectType;
}

// INLINED
void CSimpleFrame::operator delete(void* ptr) {
    if (ptr) {
        CSimpleFrame::s_allocator.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 6649b89d manual
// OFFSET: 0x004922F0
CSimpleFrame::CSimpleFrame(CSimpleFrame* parent) {
    this->m_frameScale = 1.0f;
    this->m_animating = 0;
    this->m_animAlpha = 0;
    this->m_depth = 0.0f;
    this->m_parentDepth = 1.0f;
    this->m_hoverFrame = nullptr;
    this->m_titleRegion = nullptr;
    this->m_id = 0;
    this->m_frameFlags = 0;
    this->m_ignoreDepth = 0;
    this->m_strata = FRAME_STRATA_MEDIUM;
    this->m_level = 0;
    this->m_enabledEventsBitmask = 0;
    this->m_isVisible = 0;
    this->m_alpha = 0xFF;
    this->m_parentAlpha = 0xFF;
    this->m_animSavedAlpha = 0xFF;
    this->m_lifecycleState = 1;
    this->m_hitRect = { 0.0f, 0.0f, 0.0f, 0.0f };
    this->m_hitRectInsets = { 0.0f, 0.0f, 0.0f, 0.0f };
    this->m_clampRectInsets = { 0.0f, 0.0f, 0.0f, 0.0f };
    this->m_highlightLocked = 0;
    this->m_dragButtons = 0;
    this->m_isMouseDown = 0;
    this->m_isMouseDragging = 0;
    this->m_mousePosition.x = 0.0f;
    this->m_mousePosition.y = 0.0f;
    this->m_isLoading = 0;
    this->m_backdropGenerator = nullptr;
    this->m_batchDirty = 0;

    CSimpleFrame::s_frameCount++;

    this->m_simpleTop = CSimpleTop::s_instance;
    this->m_simpleTop->RegisterFrame(this);

    this->SetParent(parent);

    this->m_drawLayerEnabled[DRAWLAYER_BACKGROUND] = 1;
    this->m_drawLayerEnabled[DRAWLAYER_BACKGROUND_BORDER] = 1;
    this->m_drawLayerEnabled[DRAWLAYER_ARTWORK] = 1;
    this->m_drawLayerEnabled[DRAWLAYER_ARTWORK_OVERLAY] = 1;
    this->m_drawLayerEnabled[DRAWLAYER_HIGHLIGHT] = 0;
    this->m_renderBatch[DRAWLAYER_BACKGROUND] = nullptr;
    this->m_renderBatch[DRAWLAYER_BACKGROUND_BORDER] = nullptr;
    this->m_renderBatch[DRAWLAYER_ARTWORK] = nullptr;
    this->m_renderBatch[DRAWLAYER_ARTWORK_OVERLAY] = nullptr;
    this->m_renderBatch[DRAWLAYER_HIGHLIGHT] = nullptr;

    this->Show();
}

// VERIFY: e1a5b71b manual
// OFFSET: [0x00492770, 0x004926B0, 0x00492E40]
CSimpleFrame::~CSimpleFrame() {
    this->m_lifecycleState = 3;
    this->m_simpleTop->UnregisterFrame(this);
    this->m_simpleTop->ValidateDeletedFrame(this);
    this->m_simpleTop = nullptr;

    if (this->m_titleRegion) {
        delete this->m_titleRegion;
    }

    for (uint32_t i = 0; i < NUM_SIMPLEFRAME_DRAWLAYERS; i++) {
        this->m_drawLayers[i].UnlinkAll();

        if (this->m_renderBatch[i]) {
            delete this->m_renderBatch[i];
        }
    }

    CSimpleRegion* region = this->m_regions.Head();

    while (region) {
        CSimpleRegion* next = this->m_regions.Next(region);
        this->m_regions.UnlinkNode(region);
        delete region;
        region = next;
    }

    this->m_renderList.UnlinkAll();

    while (this->m_children.Head()) {
        CSimpleFrame* child = this->m_children.Head()->m_frame;

        if (child) {
            delete child;
        }
    }

    if (this->m_parent) {
        this->m_parent->UnparentFrame(this);
    }

    if (this->m_backdropGenerator) {
        delete this->m_backdropGenerator;
    }

    this->m_attributes.Clear();

    CSimpleFrame::s_frameCount--;
}

// VERIFY: bfa0f3ed auto
// OFFSET: 0x004926C0
bool CSimpleFrame::IsA(int32_t type) {
    return type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// VERIFY: bb492e3f auto
// OFFSET: 0x004926F0
bool CSimpleFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CSimpleFrame::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptRegion::s_objectTypeName, STORM_MAX_STR) || !SStrCmpI(typeName, CScriptObject::s_objectTypeName, STORM_MAX_STR);
}

// VERIFY: 734071a8 auto
// OFFSET: 0x00492750
const char* CSimpleFrame::GetObjectTypeName() {
    return CSimpleFrame::s_objectTypeName;
}

// VERIFY: 3fde1784 auto
// OFFSET: 0x00492760
int32_t CSimpleFrame::GetScriptMetaTable() {
    return CSimpleFrame::s_metatable;
}

// VERIFY: cb6c9f18 auto
// OFFSET: 0x0048E680
FrameScript_Object::ScriptIx* CSimpleFrame::GetScriptByName(const char* name, FrameScript_Object::ScriptData& data) {
    FrameScript_Object::ScriptIx* script = FrameScript_Object::GetScriptByName(name, data);

    if (script) {
        return script;
    }

    if (!SStrCmpI(name, "OnLoad", STORM_MAX_STR)) {
        return &this->m_onLoad;
    }

    if (!SStrCmpI(name, "OnSizeChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,w,h) %s end";
        return &this->m_onSizeChanged;
    }

    if (!SStrCmpI(name, "OnUpdate", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,elapsed) %s end";
        return &this->m_onUpdate;
    }

    if (!SStrCmpI(name, "OnShow", STORM_MAX_STR)) {
        return &this->m_onShow;
    }

    if (!SStrCmpI(name, "OnHide", STORM_MAX_STR)) {
        return &this->m_onHide;
    }

    if (!SStrCmpI(name, "OnEnter", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,motion) %s end";
        return &this->m_onCursorEnter;
    }

    if (!SStrCmpI(name, "OnLeave", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,motion) %s end";
        return &this->m_onCursorExit;
    }

    if (!SStrCmpI(name, "OnMouseDown", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button) %s end";
        return &this->m_onMouseDown;
    }

    if (!SStrCmpI(name, "OnMouseUp", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button) %s end";
        return &this->m_onMouseUp;
    }

    if (!SStrCmpI(name, "OnMouseWheel", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,delta) %s end";
        return &this->m_onMouseWheel;
    }

    if (!SStrCmpI(name, "OnDragStart", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,button) %s end";
        return &this->m_onDragStart;
    }

    if (!SStrCmpI(name, "OnDragStop", STORM_MAX_STR)) {
        return &this->m_onDragStop;
    }

    if (!SStrCmpI(name, "OnReceiveDrag", STORM_MAX_STR)) {
        return &this->m_onReceiveDrag;
    }

    if (!SStrCmpI(name, "OnChar", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,text) %s end";
        return &this->m_onChar;
    }

    if (!SStrCmpI(name, "OnKeyDown", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,key) %s end";
        return &this->m_onKeyDown;
    }

    if (!SStrCmpI(name, "OnKeyUp", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,key) %s end";
        return &this->m_onKeyUp;
    }

    if (!SStrCmpI(name, "OnAttributeChanged", STORM_MAX_STR)) {
        data.m_wrapper = "return function(self,name,value) %s end";
        return &this->m_onAttributeChanged;
    }

    if (!SStrCmpI(name, "OnEnable", STORM_MAX_STR)) {
        return &this->m_onEnable;
    }

    if (!SStrCmpI(name, "OnDisable", STORM_MAX_STR)) {
        return &this->m_onDisable;
    }

    return nullptr;
}

// VERIFY: d39dac25 manual
// OFFSET: 0x00482E60
uint8_t CSimpleFrame::GetEffectiveAlpha() {
    return this->m_alpha * this->m_parentAlpha / 255;
}

// VERIFY: 292e1728 auto
// OFFSET: 0x0048E970
void CSimpleFrame::SetFrameFlag(uint32_t flag, int32_t set) {
    if (set) {
        this->m_frameFlags |= flag;
    } else {
        this->m_frameFlags &= ~flag;
    }
}

// VERIFY: c5f65169 auto
// OFFSET: 0x0048E9A0
void CSimpleFrame::Raise() {
    this->m_simpleTop->RaiseFrame(this, 1);
}

// VERIFY: cd9f8146 auto
// OFFSET: 0x0048E9B0
int32_t CSimpleFrame::Lower() {
    return this->m_simpleTop->LowerFrame(this);
}

// VERIFY: 7a7cdf5e auto
// OFFSET: 0x0048E9C0
int32_t CSimpleFrame::DragBy(float offsetX, float offsetY, FRAMEPOINT point) {
    return CLayoutFrame::DragBy(this->m_simpleTop, offsetX, offsetY, point);
}

// VERIFY: d213d152 auto
// OFFSET: 0x0048E9F0
void CSimpleFrame::PropagateProtectFlagToParent(uint32_t flag) {
    if (this->m_parent) {
        this->m_parent->SetProtectFlag(flag);
    }
}

// VERIFY: 19a8f143 auto
// OFFSET: 0x0048EA10
void CSimpleFrame::SetFrameAlpha(uint8_t alpha) {
    if (alpha != this->m_alpha) {
        this->m_alpha = alpha;
        this->UpdateAlpha();
    }
}

// VERIFY: ee3b3c89 auto
// OFFSET: 0x0048EA40
void CSimpleFrame::RegisterForEvents(int32_t a2) {
    for (uint32_t i = 0; i < NUM_SIMPLE_EVENTS; i++) {
        if ((1 << i) & this->m_enabledEventsBitmask) {
            this->m_simpleTop->RegisterForEvent(this, static_cast<CSimpleEventType>(i), a2, -1);
        }
    }
}

// VERIFY: 68083be0 auto
// OFFSET: 0x0048EA80
void CSimpleFrame::UnregisterForEvents(int32_t a2) {
    for (uint32_t i = 0; i < NUM_SIMPLE_EVENTS; i++) {
        if ((1 << i) & this->m_enabledEventsBitmask) {
            this->m_simpleTop->UnregisterForEvent(this, static_cast<CSimpleEventType>(i), a2);
        }
    }
}

// VERIFY: b74e77a5 auto
// OFFSET: 0x0048EAC0
void CSimpleFrame::GetHitRectInsets(float& left, float& right, float& top, float& bottom) {
    left = this->m_hitRectInsets.minX;
    right = this->m_hitRectInsets.maxX;
    top = this->m_hitRectInsets.maxY;
    bottom = this->m_hitRectInsets.minY;
}

// VERIFY: 7a64124f auto
// OFFSET: 0x0048EB00
void CSimpleFrame::SetClampRectInsets(float left, float right, float top, float bottom) {
    this->m_clampRectInsets.minX = left;
    this->m_clampRectInsets.maxX = right;
    this->m_clampRectInsets.maxY = top;
    this->m_clampRectInsets.minY = bottom;
}

// VERIFY: 8374aca6 auto
// OFFSET: 0x0048EB30
void CSimpleFrame::GetClampRectInsets(float& left, float& right, float& top, float& bottom) {
    left = this->m_clampRectInsets.minX;
    right = this->m_clampRectInsets.maxX;
    top = this->m_clampRectInsets.maxY;
    bottom = this->m_clampRectInsets.minY;
}

// VERIFY: 31db8368 auto
// OFFSET: 0x0048EB70
bool CSimpleFrame::IsDragging() {
    return this->m_isMouseDragging || (this->m_parent && this->m_parent->IsDragging());
}

// VERIFY: 9375fd41 manual
// OFFSET: 0x0048EBA0
void CSimpleFrame::PreOnAnimUpdate() {
    if (this->m_animAlpha && this->m_animSavedAlpha != this->m_alpha) {
        this->m_alpha = this->m_animSavedAlpha;
        this->UpdateAlpha();
    }
}

// VERIFY: fb5aa7ed auto
// OFFSET: 0x0048EBD0
void CSimpleFrame::StoreAnimPosition() {
    this->Resize(1);
    this->m_animating = 1;
    this->m_animAlpha = 0;
}

// VERIFY: e9514e5e auto
// OFFSET: 0x0048EBF0
void CSimpleFrame::ResetAnimPosition() {
    this->m_animating = 0;
    this->m_animAlpha = 0;
    this->Resize(1);
}

// VERIFY: 4eef2a8c auto
// OFFSET: 0x0048EC10
void CSimpleFrame::RunOnAttributeChangedScript(const char* name, int32_t luaRef) {
    if (!this->m_onAttributeChanged.m_luaRef) {
        return;
    }

    lua_State* L = FrameScript_GetContext();
    const char* savedTaint = lua_tainted;

    if (++lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = nullptr;
    }

    char* lowerName = static_cast<char*>(alloca(SStrLen(name) + 1));
    SStrCopy(lowerName, name, STORM_MAX_STR);
    SStrLower(lowerName);

    lua_pushstring(L, lowerName);
    lua_rawgeti(L, LUA_REGISTRYINDEX, luaRef);
    this->RunScript(this->m_onAttributeChanged, 2, nullptr);

    if (lua_taintexpected && !lua_taintedclosure) {
        lua_tainted = savedTaint;
    }

    if (--lua_taintexpected <= 0) {
        lua_taintexpected = 0;
    }
}

// VERIFY: 0a1a8e02 manual
// OFFSET: 0x0048ECF0
void CSimpleFrame::AllowAttributeChanges(int32_t allow) {
    if (allow) {
        this->m_frameFlags |= 0x8000;
        this->m_simpleTop->RegisterForCleanup(this);
    } else {
        this->m_frameFlags &= ~0x8000u;
    }
}

// VERIFY: 0b620746 manual
// OFFSET: 0x0048ED30
int32_t CSimpleFrame::AttributeChangesAllowed() {
    for (CSimpleFrame* frame = this; frame; frame = frame->m_parent) {
        if (frame->m_frameFlags & 0x8000) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 0b2d4d3c auto
// OFFSET: 0x0048F070
void CSimpleFrame::RunOnSizeChangedScript(float width, float height) {
    if (this->m_onSizeChanged.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * width));
        lua_pushnumber(L, DDCToNDCWidth(CoordinateGetAspectCompensation() * 1024.0f * height));
        this->RunScript(this->m_onSizeChanged, 2, nullptr);
    }
}

// VERIFY: f5dfec34 auto
// OFFSET: 0x0048F0F0
void CSimpleFrame::RunOnEnterScript(int32_t motion) {
    if (this->m_onCursorEnter.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushboolean(L, motion);
        this->RunScript(this->m_onCursorEnter, 1, nullptr);
    }
}

// VERIFY: f2d2ba21 auto
// OFFSET: 0x0048F130
void CSimpleFrame::RunOnLeaveScript(int32_t motion) {
    if (this->m_onCursorExit.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushboolean(L, motion);
        this->RunScript(this->m_onCursorExit, 1, nullptr);
    }
}

// VERIFY: ab9ac4fe auto
// OFFSET: 0x0048F170
void CSimpleFrame::RunOnMouseWheelScript(int32_t delta) {
    if (this->m_onMouseWheel.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, delta);
        this->RunScript(this->m_onMouseWheel, 1, nullptr);
    }
}

// VERIFY: 4eba352f auto
// OFFSET: 0x0048F1B0
void CSimpleFrame::RunOnKeyUpScript(const char* key) {
    if (this->m_onKeyUp.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, key);
        this->RunScript(this->m_onKeyUp, 1, nullptr);
    }
}

// VERIFY: a4db5488 auto
// OFFSET: 0x0048F1F0
void CSimpleFrame::LoadXML_Layers(XMLNode* node, CStatus* status) {
    for (XMLNode* layerNode = node->m_child; layerNode; layerNode = layerNode->m_next) {
        if (SStrCmpI(layerNode->m_name.m_str, "Layer", STORM_MAX_STR)) {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown child node in %s element: %s", frameName ? frameName : "<unnamed>", node->m_name.m_str, layerNode->m_name.m_str);
            continue;
        }

        uint32_t drawLayer = DRAWLAYER_ARTWORK;
        const char* level = layerNode->GetAttributeByName("level");

        if (level && *level) {
            StringToDrawLayer(level, drawLayer);
        }

        for (XMLNode* child = layerNode->m_child; child; child = child->m_next) {
            if (!SStrCmpI(child->m_name.m_str, "Texture", STORM_MAX_STR)) {
                CSimpleTexture* texture = LoadXML_Texture(child, this, status);
                texture->SetFrame(this, drawLayer, texture->m_shown);
            } else if (!SStrCmpI(child->m_name.m_str, "FontString", STORM_MAX_STR)) {
                CSimpleFontString* string = LoadXML_String(child, this, status);
                string->SetFrame(this, drawLayer, string->m_shown);
            } else {
                const char* frameName = this->GetName();
                status->Add(STATUS_WARNING, "Frame %s: Unknown child node in %s element: %s", frameName ? frameName : "<unnamed>", layerNode->m_name.m_str, child->m_name.m_str);
            }
        }
    }
}

// INLINED
FrameScriptFunctionRef::FrameScriptFunctionRef(int32_t function) {
    this->m_function = function;
}

// VERIFY: e9b972f0 auto
// OFFSET: 0x0048F380
void FrameScriptFunctionRef::Release() {
    if (this->m_function > 0) {
        FrameScript_ReleaseFunction(this->m_function);
        this->m_function = 0;
    }

    delete this;
}

// VERIFY: 881c247c manual
// OFFSET: 0x0048F3B0
FrameScriptFunctionRef::~FrameScriptFunctionRef() {
    if (this->m_function > 0) {
        FrameScript_ReleaseFunction(this->m_function);
        this->m_function = 0;
    }
}

// VERIFY: 3c47d852 auto
// OFFSET: 0x0048F400
void CSimpleFrame::LoadXMLInheritance(XMLNode* node, CStatus* status) {
    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        char name[1024];

        do {
            SStrTokenize(&inherits, name, sizeof(name), " ,", nullptr);

            if (!name[0]) {
                break;
            }

            const char* taint;
            bool locked;
            XMLNode* inheritNode = FrameXML_AcquireHashNode(name, taint, locked);

            if (inheritNode && !locked) {
                FrameScript_GetContext();

                const char* savedTaint = lua_tainted;

                if (++lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = taint;
                }

                const char* savedTag = lua_gctag;
                lua_gctag = taint;

                this->LoadXMLInheritance(inheritNode, status);

                lua_gctag = savedTag;

                if (lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = savedTaint;
                }

                if (--lua_taintexpected <= 0) {
                    lua_taintexpected = 0;
                }

                FrameXML_ReleaseHashNode(name);
            }
        } while (name[0]);
    }

    XMLNode* frames = node->GetChildByName("Frames");

    if (frames) {
        for (XMLNode* child = frames->m_child; child; child = child->m_next) {
            FrameXML_CreateFrame(child, this, status);
        }
    }
}

// VERIFY: 08c5569f auto
// OFFSET: 0x0048F550
void CSimpleFrame::NotifyDelayedLoad() {
    if (this->m_frameFlags & 0x2000) {
        this->m_batchDirty |= 0x20;
    } else if (this->m_simpleTop && this->m_isVisible) {
        this->m_simpleTop->NotifyFrameDelayedLoad(this);
    }
}

// VERIFY: 4b524082 auto
// OFFSET: 0x0048F580
bool CSimpleFrame::SetFrameScale(float scale, bool force) {
    if ((!force && fabs(scale - this->m_frameScale) < 0.00000023841858f) || scale == 0.0f) {
        return false;
    }

    this->m_frameScale = scale;
    return this->UpdateScale(force);
}

// VERIFY: 00115b9e auto
// OFFSET: 0x0048F5D0
bool CSimpleFrame::SetFrameDepth(float depth, bool force) {
    if (!force && fabs(depth - this->m_depth) < 0.00000023841858f) {
        return false;
    }

    this->m_depth = depth;
    return this->UpdateDepth(force);
}

// VERIFY: 1e40a962 manual
// OFFSET: 0x0048F620
int32_t CSimpleFrame::Hide() {
    if (!this->ProtectedFunctionsAllowed()) {
        if (this->m_simpleTop->m_actionBlockedCallback) {
            this->m_simpleTop->m_actionBlockedCallback(nullptr);
        }

        return 0;
    }

    this->m_isShown = 0;
    return this->HideThis();
}

// VERIFY: aa204a9e manual
// OFFSET: 0x0048F660
int32_t CSimpleFrame::Show() {
    if (!this->ProtectedFunctionsAllowed()) {
        if (this->m_simpleTop->m_actionBlockedCallback) {
            this->m_simpleTop->m_actionBlockedCallback(nullptr);
        }

        return 0;
    }

    this->m_isShown = 1;
    return this->ShowThis();
}

// VERIFY: ec84450f auto
// OFFSET: 0x0048F6A0
void CSimpleFrame::EnableEvent(CSimpleEventType eventType, int32_t priority) {
    if ((1 << eventType) & this->m_enabledEventsBitmask) {
        return;
    }

    if (this->m_isVisible) {
        this->m_simpleTop->RegisterForEvent(this, eventType, 0, priority);
    }

    this->m_enabledEventsBitmask |= 1 << eventType;
}

// VERIFY: 219cc709 auto
// OFFSET: 0x0048F6F0
void CSimpleFrame::DisableEvent(CSimpleEventType eventType) {
    if (!((1 << eventType) & this->m_enabledEventsBitmask)) {
        return;
    }

    if (this->m_isVisible) {
        this->m_simpleTop->UnregisterForEvent(this, eventType, 0);
    }

    this->m_enabledEventsBitmask &= ~(1 << eventType);
}

// VERIFY: bbfb8171 manual
// OFFSET: 0x0048F730
int32_t CSimpleFrame::TestHitRect(const C2Vector& pt) {
    if (!(this->m_layoutFlags & 0x100)) {
        return 0;
    }

    if (!(this->m_frameFlags & 0x2000)) {
        return this->m_hitRect.IsPointInside(pt);
    }

    CSimpleFrame* scrollFrame = this->m_parent;

    if (!scrollFrame) {
        return 0;
    }

    while (scrollFrame->m_frameFlags & 0x2000) {
        scrollFrame = scrollFrame->m_parent;

        if (!scrollFrame) {
            return 0;
        }
    }

    CRect rect = scrollFrame->m_hitRect;
    CRect clip = rect &= this->m_hitRect;
    return clip.minX <= pt.x && clip.maxX >= pt.x && clip.minY <= pt.y && clip.maxY >= pt.y;
}

// VERIFY: 5bdccee1 auto
// OFFSET: 0x0048F820
void CSimpleFrame::SetHitRectInsets(float left, float right, float top, float bottom) {
    this->m_hitRectInsets.minX = left;
    this->m_hitRectInsets.maxX = right;
    this->m_hitRectInsets.maxY = top;
    this->m_hitRectInsets.minY = bottom;

    this->m_hitRect.minX = left * this->m_layoutScale + this->m_rect.minX;
    this->m_hitRect.maxX = this->m_rect.maxX - right * this->m_layoutScale;
    this->m_hitRect.minY = bottom * this->m_layoutScale + this->m_rect.minY;
    this->m_hitRect.maxY = this->m_rect.maxY - top * this->m_layoutScale;
}

// VERIFY: 8c2d9ec9 manual
// OFFSET: 0x0048F890
int32_t CSimpleFrame::GetHitRect(CRect& rect) {
    if (!(this->m_layoutFlags & 0x100)) {
        return 0;
    }

    rect = this->m_hitRect;
    return 1;
}

// VERIFY: a9240fc0 manual
// OFFSET: 0x0048F8E0
void CSimpleFrame::AddAnimAlpha(int16_t alpha) {
    int16_t value = alpha + static_cast<uint8_t>(this->m_alpha * this->m_parentAlpha / 255);
    uint8_t newAlpha;

    if (value > 255) {
        newAlpha = 255;
    } else if (value < 0) {
        newAlpha = 0;
    } else {
        newAlpha = static_cast<uint8_t>(value);
    }

    if (newAlpha != this->m_alpha) {
        this->m_alpha = newAlpha;
        this->UpdateAlpha();
    }
}

// VERIFY: de9aeb94 auto
// OFFSET: 0x0048F950
int32_t CSimpleFrame::OnLayerTrackUpdate(const CMouseEvent& evt) {
    C2Vector pt = evt.m_data.m_mousePosition;

    if (this->m_isMouseDown && !this->m_isMouseDragging) {
        float dx = pt.x - this->m_mousePosition.x;
        float dy = pt.y - this->m_mousePosition.y;

        if (dx * dx + dy * dy >= s_dragThreshold) {
            this->m_isMouseDragging = 1;
            this->OnDragStart(evt);
            this->m_mousePosition.x = 0.0f;
            this->m_mousePosition.y = 0.0f;
        }
    }

    if (!this->TestHitRect(pt)) {
        return 0;
    }

    if (!this->m_ignoreDepth && g_theGxDevicePtr->StereoEnabled()) {
        g_theGxDevicePtr->CursorSetDepth(this->m_layoutDepth);
    }

    return 1;
}

// VERIFY: b9fc7020 manual
// OFFSET: 0x0048FA20
void CSimpleFrame::OnFrameSizeChanged(const CRect& rect) {
    CLayoutFrame::OnFrameSizeChanged(rect);

    this->m_hitRect.minX = this->m_hitRectInsets.minX * this->m_layoutScale + this->m_rect.minX;
    this->m_hitRect.maxX = this->m_rect.maxX - this->m_hitRectInsets.maxX * this->m_layoutScale;
    this->m_hitRect.minY = this->m_hitRectInsets.minY * this->m_layoutScale + this->m_rect.minY;
    this->m_hitRect.maxY = this->m_rect.maxY - this->m_hitRectInsets.maxY * this->m_layoutScale;

    if (fabs(rect.maxX - rect.minX - (this->m_rect.maxX - this->m_rect.minX)) >= 0.00000023841858f || fabs(rect.maxY - rect.minY - (this->m_rect.maxY - this->m_rect.minY)) >= 0.00000023841858f) {
        if (this->m_backdropGenerator) {
            this->m_backdropGenerator->Generate(&this->m_rect);
        }

        float scale = 1.0f / this->m_layoutScale;
        this->OnFrameSizeChanged((this->m_rect.maxX - this->m_rect.minX) * scale, (this->m_rect.maxY - this->m_rect.minY) * scale);
    }

    this->m_simpleTop->NotifyFrameMovedOrResized(this);
}

// VERIFY: f20c92f1 auto
// OFFSET: 0x004899F0
int32_t CSimpleFrame::OnLayerIme(const CImeEvent& evt) {
    return 0;
}

// VERIFY: c81f847b auto
// OFFSET: 0x004899F0
int32_t CSimpleFrame::OnLayerKeyDownRepeat(const CKeyEvent& evt) {
    return 0;
}

// VERIFY: 6103df2d auto
// OFFSET: 0x0048FB10
int32_t CSimpleFrame::OnLayerChar(const CCharEvent& evt) {
    if (!this->m_isVisible || !this->m_onChar.m_luaRef) {
        return 0;
    }

    char text[8];
    sputu8(evt.m_data.m_ch, text);

    if (this->m_onChar.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, text);
        this->RunScript(this->m_onChar, 1, nullptr);
    }

    return 1;
}

// VERIFY: 5ad01abe auto
// OFFSET: 0x0048FB80
int32_t CSimpleFrame::OnLayerKeyDown(const CKeyEvent& evt) {
    if (!this->m_isVisible || (!this->m_onKeyDown.m_luaRef && !this->m_onKeyUp.m_luaRef)) {
        return 0;
    }

    const char* key = KeyCodeToString(evt.m_data.m_key);

    if (this->m_onKeyDown.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, key);
        this->RunScript(this->m_onKeyDown, 1, nullptr);
    }

    return 1;
}

// VERIFY: b11bf9fd auto
// OFFSET: 0x0048FBF0
int32_t CSimpleFrame::OnLayerKeyUp(const CKeyEvent& evt) {
    if (!this->m_isVisible || !this->m_onKeyUp.m_luaRef) {
        return 0;
    }

    this->RunOnKeyUpScript(KeyCodeToString(evt.m_data.m_key));
    return 1;
}

// VERIFY: f962ad69 auto
// OFFSET: 0x0048FC30
int32_t CSimpleFrame::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    if (!btn) {
        if (this->m_dragButtons & evt.m_data.m_button) {
            this->m_isMouseDown = 1;
            this->m_isMouseDragging = 0;
            this->m_mouseDragButton = evt.m_data.m_button;
            this->m_mousePosition = evt.m_data.m_mousePosition;
        }

        btn = GetMouseButtonName(evt.m_data.m_button);
    }

    const char* savedButton = this->m_simpleTop->m_mouseButtonName;
    this->m_simpleTop->m_mouseButtonName = btn;

    if (this->m_onMouseDown.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        this->RunScript(this->m_onMouseDown, 1, nullptr);
    }

    this->m_simpleTop->m_mouseButtonName = savedButton;
    return 0;
}

// VERIFY: 9fd49d5d auto
// OFFSET: 0x0048FCE0
int32_t CSimpleFrame::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    if (!btn) {
        if (this->m_dragButtons & evt.m_data.m_button) {
            this->m_isMouseDown = 0;

            if (this->m_isMouseDragging) {
                this->OnDragStop(evt);

                CSimpleFrame* focus = this->m_simpleTop->m_mouseFocus;

                if (focus) {
                    focus->OnReceiveDrag(evt);
                }

                this->m_isMouseDragging = 0;
                return 1;
            }
        }

        btn = GetMouseButtonName(evt.m_data.m_button);
    }

    const char* savedButton = this->m_simpleTop->m_mouseButtonName;
    this->m_simpleTop->m_mouseButtonName = btn;

    if (this->m_onMouseUp.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, btn);
        this->RunScript(this->m_onMouseUp, 1, nullptr);
    }

    this->m_simpleTop->m_mouseButtonName = savedButton;
    return 0;
}

// VERIFY: 27bac257 manual
// OFFSET: 0x0048FDB0
int32_t CSimpleFrame::OnLayerMouseWheel(const CMouseEvent& evt) {
    if (!this->m_isVisible || !this->m_onMouseWheel.m_luaRef) {
        return 0;
    }

    this->RunOnMouseWheelScript(static_cast<int32_t>(evt.m_data.m_wheelAmount) >= 0 ? 1 : -1);
    return 1;
}

// VERIFY: 1d376a78 auto
// OFFSET: 0x0048FDF0
void CSimpleFrame::OnDragStart(const CMouseEvent& evt) {
    uint32_t button = 0;

    for (int32_t i = 0; i < 31; i++) {
        if (evt.m_data.m_buttonState & (1 << i)) {
            button = 1 << i;
            break;
        }
    }

    if (this->m_onDragStart.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushstring(L, GetMouseButtonName(button));
        this->RunScript(this->m_onDragStart, 1, nullptr);
    }
}

// VERIFY: 445a6cbe auto
// OFFSET: 0x0048FE60
void CSimpleFrame::OnDragStop(const CMouseEvent& evt) {
    if (this->m_onDragStop.m_luaRef) {
        this->RunScript(this->m_onDragStop, 0, nullptr);
    }
}

// VERIFY: 7c9a0de0 auto
// OFFSET: 0x0048FE80
void CSimpleFrame::OnReceiveDrag(const CMouseEvent& evt) {
    if (this->m_onReceiveDrag.m_luaRef) {
        this->RunScript(this->m_onReceiveDrag, 0, nullptr);
    }
}

// VERIFY: 703bbed0 auto
// OFFSET: 0x0048FEA0
void CSimpleFrame::NotifyScrollParent() {
    if (!(this->m_frameFlags & 0x2000) || !this->m_isVisible) {
        return;
    }

    CSimpleFrame* frame = this;

    while (!(frame->m_frameFlags & 0x4000)) {
        frame = frame->m_parent;
    }

    CSimpleScrollFrame* scrollFrame = static_cast<CSimpleScrollFrame*>(frame->m_parent);

    if (scrollFrame && scrollFrame->m_lifecycleState != 3) {
        scrollFrame->m_scrollChildUpdate = 1;
    }
}

// VERIFY: de66a4ac manual
// OFFSET: 0x0048FEF0
void CSimpleFrame::LoadXML_Scripts(XMLNode* node, CStatus* status) {
    lua_State* L = FrameScript_GetContext();

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        const char* body = child->m_body;
        const char* scriptName = child->m_name.m_str;

        FrameScript_Object::ScriptData data;
        data.m_wrapper = "return function(self) %s end";

        FrameScript_Object::ScriptIx* script = this->GetScriptByName(scriptName, data);

        if (!script) {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown script element %s", frameName ? frameName : "<unnamed>", scriptName);
            continue;
        }

        if (script->m_luaRef) {
            luaL_unref(L, LUA_REGISTRYINDEX, script->m_luaRef);
        }

        if (!child->m_userData && body && *body) {
            char name[1024];
            SStrPrintf(name, sizeof(name), "*:%s", scriptName);

            void* mem = SMemAlloc(sizeof(FrameScriptFunctionRef), __FILE__, __LINE__, 0);
            child->m_userData = mem ? new (mem) FrameScriptFunctionRef(FrameScript_CompileFunction(name, data.m_wrapper, body, status)) : nullptr;
        }

        const char* function = child->GetAttributeByName("function");

        if (function && *function) {
            lua_pushstring(L, function);
            lua_rawget(L, LUA_GLOBALSINDEX);
            int32_t ref = luaL_ref(L, LUA_REGISTRYINDEX);

            if (ref == -1) {
                const char* frameName = this->GetName();
                status->Add(STATUS_WARNING, "Frame %s: Unknown function %s in element %s", frameName ? frameName : "<unnamed>", function, scriptName);
                script->m_luaRef = 0;
            } else {
                script->m_luaRef = ref;
            }
        } else if (child->m_userData) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, static_cast<FrameScriptFunctionRef*>(child->m_userData)->m_function);
            script->m_luaRef = luaL_ref(L, LUA_REGISTRYINDEX);
        } else {
            script->m_luaRef = 0;
        }

        script->m_tainted = nullptr;

        if (!SStrCmpI(child->m_name.m_str, "OnChar", STORM_MAX_STR) && !(this->m_enabledEventsBitmask & (1 << SIMPLE_EVENT_CHAR))) {
            if (this->m_isVisible) {
                this->m_simpleTop->RegisterForEvent(this, SIMPLE_EVENT_CHAR, 0, -1);
            }

            this->m_enabledEventsBitmask |= 1 << SIMPLE_EVENT_CHAR;
        }

        if ((!SStrCmpI(child->m_name.m_str, "OnKeyDown", STORM_MAX_STR) || !SStrCmpI(child->m_name.m_str, "OnKeyUp", STORM_MAX_STR)) && !(this->m_enabledEventsBitmask & (1 << SIMPLE_EVENT_KEY))) {
            if (this->m_isVisible) {
                this->m_simpleTop->RegisterForEvent(this, SIMPLE_EVENT_KEY, 0, -1);
            }

            this->m_enabledEventsBitmask |= 1 << SIMPLE_EVENT_KEY;
        }

        if ((!SStrCmpI(child->m_name.m_str, "OnEnter", STORM_MAX_STR) || !SStrCmpI(child->m_name.m_str, "OnLeave", STORM_MAX_STR) || !SStrCmpI(child->m_name.m_str, "OnMouseDown", STORM_MAX_STR) || !SStrCmpI(child->m_name.m_str, "OnMouseUp", STORM_MAX_STR) || !SStrCmpI(child->m_name.m_str, "OnDragStart", STORM_MAX_STR)) && !(this->m_enabledEventsBitmask & (1 << SIMPLE_EVENT_MOUSE))) {
            if (this->m_isVisible) {
                this->m_simpleTop->RegisterForEvent(this, SIMPLE_EVENT_MOUSE, 0, -1);
            }

            this->m_enabledEventsBitmask |= 1 << SIMPLE_EVENT_MOUSE;
        }

        if (!SStrCmpI(child->m_name.m_str, "OnMouseWheel", STORM_MAX_STR) && !(this->m_enabledEventsBitmask & (1 << SIMPLE_EVENT_MOUSEWHEEL))) {
            if (this->m_isVisible) {
                this->m_simpleTop->RegisterForEvent(this, SIMPLE_EVENT_MOUSEWHEEL, 0, -1);
            }

            this->m_enabledEventsBitmask |= 1 << SIMPLE_EVENT_MOUSEWHEEL;
        }
    }
}

// VERIFY: 784118ad auto
// OFFSET: 0x00490270
void CSimpleFrame::NotifyDrawLayerChanged(uint32_t drawLayer) {
    if (this->m_frameFlags & 0x2000) {
        this->m_batchDirty |= 1 << drawLayer;
    } else if (this->m_simpleTop && this->m_isVisible) {
        this->m_simpleTop->NotifyFrameLayerChanged(this, drawLayer);
    }

    this->NotifyScrollParent();
}

// VERIFY: 5d917b41 auto
// OFFSET: 0x004902D0
void CSimpleFrame::OnLayerShow() {
    if (this->m_onShow.m_luaRef && !this->m_isLoading) {
        this->RunScript(this->m_onShow, 0, nullptr);
    }

    this->NotifyScrollParent();
}

// VERIFY: 68810318 auto
// OFFSET: 0x00490300
void CSimpleFrame::OnLayerHide() {
    if (this->m_onHide.m_luaRef && !this->m_isLoading) {
        this->RunScript(this->m_onHide, 0, nullptr);
    }

    this->NotifyScrollParent();
}

// VERIFY: 328166ac auto
// OFFSET: 0x00490330
void CSimpleFrame::OnFrameSizeChanged(float width, float height) {
    this->RunOnSizeChangedScript(width, height);
    this->NotifyScrollParent();
}

// VERIFY: a64e9507 auto
// OFFSET: 0x00490360
void CSimpleFrame::PreLoadXML(XMLNode* node, CStatus* status) {
    const char* name = node->GetAttributeByName("name");

    if (name && *name) {
        this->SetName(name);
    }

    const char* id = node->GetAttributeByName("id");

    if (id && *id) {
        int32_t value = SStrToInt(id);

        if (value >= 0) {
            this->m_id = value;
        }
    }

    this->m_isLoading = 1;
    this->SetDeferredResize(1);

    if (this->m_titleRegion) {
        this->m_titleRegion->SetDeferredResize(1);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->SetDeferredResize(1);
    }
}

// VERIFY: b8ec6f00 manual
// OFFSET: 0x00490410
void CSimpleFrame::PostLoadXML(XMLNode* node, CStatus* status) {
    this->m_isLoading = 0;

    if (this->m_isVisible) {
        this->SetDeferredResize(0);

        if (this->m_titleRegion) {
            this->m_titleRegion->SetDeferredResize(0);
        }

        for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
            if (region->m_visible) {
                region->SetDeferredResize(0);
            }
        }
    }

    this->LoadXMLInheritance(node, status);

    int32_t active;
    int32_t activeAlpha;
    this->GetActiveAnimInfo(CSimpleAlphaAnim::GetObjectType(), active, activeAlpha);

    if (active) {
        this->StoreAnimPosition();
        this->m_animAlpha = activeAlpha;
        this->m_animSavedAlpha = this->m_alpha * this->m_parentAlpha / 255;

        for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
            region->AnimActivated(nullptr, 1, activeAlpha);
        }
    }

    if (this->m_onLoad.m_luaRef) {
        this->RunScript(this->m_onLoad, 0, nullptr);
    }

    if (this->m_isVisible && this->m_onShow.m_luaRef && !this->m_isLoading) {
        this->RunScript(this->m_onShow, 0, nullptr);
    }
}

// VERIFY: d6b8f34e manual
// OFFSET: 0x004905A0
void CSimpleFrame::SetBackdrop(CBackdropGenerator* backdrop) {
    if (this->m_backdropGenerator) {
        this->m_backdropGenerator->DestroyTextures();
        delete this->m_backdropGenerator;
    }

    if (backdrop) {
        backdrop->SetOutput(this);
    }

    this->m_backdropGenerator = backdrop;
}

// VERIFY: 8faa6b6f manual
// OFFSET: 0x00490600
void CSimpleFrame::EnableDrawLayer(uint32_t drawLayer) {
    this->m_drawLayerEnabled[drawLayer] = 1;
    this->NotifyDrawLayerChanged(drawLayer);
}

// VERIFY: 65896940 manual
// OFFSET: 0x00490620
void CSimpleFrame::DisableDrawLayer(uint32_t drawLayer) {
    this->m_drawLayerEnabled[drawLayer] = 0;
    this->NotifyDrawLayerChanged(drawLayer);
}

// VERIFY: 0670b415 manual
// OFFSET: 0x00490640
void CSimpleFrame::RegisterRegion(CSimpleRegion* region) {
    if (!region) {
        DebugValidate("region");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    this->m_regions.LinkToTail(region);
}

// VERIFY: a3acb818 manual
// OFFSET: 0x00490680
void CSimpleFrame::UnregisterRegion(CSimpleRegion* region) {
    if (!region) {
        DebugValidate("region");
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    this->m_regions.UnlinkNode(region);
}

// VERIFY: 0dfedb0a manual
// OFFSET: 0x004906C0
void CSimpleFrame::AddFrameRegion(CSimpleRegion* region, uint32_t drawLayer) {
    region->SetLayoutScale(this->m_layoutScale, false);
    this->m_drawLayers[drawLayer].LinkToTail(region);
    this->NotifyDrawLayerChanged(drawLayer);
}

// VERIFY: 3faa2d29 manual
// OFFSET: 0x00490700
void CSimpleFrame::RemoveFrameRegion(CSimpleRegion* region, uint32_t drawLayer) {
    this->m_drawLayers[drawLayer].UnlinkNode(region);
    this->NotifyDrawLayerChanged(drawLayer);
}

// VERIFY: 787b525f auto
// OFFSET: 0x00490770
void CSimpleFrame::OnLayerUpdate(float elapsedSec) {
    if (this->m_onUpdate.m_luaRef) {
        lua_State* L = FrameScript_GetContext();
        lua_pushnumber(L, elapsedSec);
        this->RunScript(this->m_onUpdate, 1, nullptr);
    }

    this->PreOnAnimUpdate();

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->PreOnAnimUpdate();
    }

    this->OnAnimUpdate(elapsedSec);

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->OnAnimUpdate(elapsedSec);
    }
}

// VERIFY: b0a0345a manual
// OFFSET: 0x00490840
void CSimpleFrame::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    if (!this->m_drawLayerEnabled[layer]) {
        return;
    }

    for (CSimpleRegion* region = this->m_drawLayers[layer].Head(); region; region = this->m_drawLayers[layer].Next(region)) {
        region->Draw(batch);
    }
}

// VERIFY: 25e7b1ac manual
// OFFSET: 0x004908A0
void CSimpleFrame::OnScreenSizeChanged() {
    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->OnScreenSizeChanged();
    }
}

// VERIFY: ee342124 manual
// OFFSET: 0x004908E0
void CSimpleFrame::OnLayerCursorEnter(int32_t motion) {
    if (this->m_hoverFrame) {
        this->m_hoverFrame->Show();
    }

    if (!this->m_highlightLocked) {
        this->m_drawLayerEnabled[DRAWLAYER_HIGHLIGHT] = 1;
        this->NotifyDrawLayerChanged(DRAWLAYER_HIGHLIGHT);
    }

    this->RunOnEnterScript(motion);
}

// VERIFY: ebe4e67e manual
// OFFSET: 0x00490980
void CSimpleFrame::OnLayerCursorExit(int32_t motion, int32_t a3) {
    if (this->m_hoverFrame) {
        this->m_hoverFrame->Hide();
    }

    if (!this->m_highlightLocked) {
        this->m_drawLayerEnabled[DRAWLAYER_HIGHLIGHT] = 0;
        this->NotifyDrawLayerChanged(DRAWLAYER_HIGHLIGHT);
    }

    if (a3) {
        this->m_isMouseDown = 0;
        this->m_isMouseDragging = 0;
    }

    this->RunOnLeaveScript(motion);
}

// VERIFY: 05878e28 auto
// OFFSET: 0x00490A30
void CSimpleFrame::LockHighlight(int32_t lock) {
    if (lock == this->m_highlightLocked) {
        return;
    }

    this->m_highlightLocked = lock;

    if (lock) {
        this->m_drawLayerEnabled[DRAWLAYER_HIGHLIGHT] = 1;
        this->NotifyDrawLayerChanged(DRAWLAYER_HIGHLIGHT);
    } else if (this->m_simpleTop->m_mouseFocus != this) {
        this->m_drawLayerEnabled[DRAWLAYER_HIGHLIGHT] = 0;
        this->NotifyDrawLayerChanged(DRAWLAYER_HIGHLIGHT);
    }
}

// VERIFY: 95d4bac7 manual
// OFFSET: 0x00490C70
void CSimpleFrame::ParentFrame(CSimpleFrame* child) {
    SIMPLEFRAMENODE* node = this->m_children.NewNode(2, 0, 0);
    node->m_frame = child;
}

// VERIFY: c03a9cc6 auto
// OFFSET: 0x00490D80
void CSimpleFrame::GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse) {
    FrameScript_Object::GetScriptTime(L, this->m_onEvent, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onLoad, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onSizeChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onShow, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onHide, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onCursorEnter, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onCursorExit, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMouseDown, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMouseUp, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onMouseWheel, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onDragStart, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onDragStop, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onReceiveDrag, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onChar, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onKeyDown, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onKeyUp, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onAttributeChanged, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onEnable, time, count);
    FrameScript_Object::GetScriptTime(L, this->m_onDisable, time, count);

    if (recurse) {
        for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
            node->m_frame->GetScriptTime(L, time, count, 1);
        }
    }
}

// VERIFY: c1ad0fe4 manual
// OFFSET: 0x00490F60
void CSimpleFrame::SetBeingScrolled(int32_t scrolled, int32_t a3) {
    uint32_t visible = this->m_isVisible;

    if (visible) {
        this->HideThis();
    }

    if (scrolled) {
        this->m_frameFlags |= 0x2000;
    } else {
        this->m_frameFlags &= ~0x2000u;
    }

    if (a3 >= 0) {
        if (a3) {
            this->m_frameFlags |= 0x4000;
        } else {
            this->m_frameFlags &= ~0x4000u;
        }
    }

    if (scrolled) {
        this->m_batchDirty |= 0x1F;
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        if (!(node->m_frame->m_frameFlags & 0x4000)) {
            node->m_frame->SetBeingScrolled(scrolled, -1);
        }
    }

    if (visible) {
        this->ShowThis();
    }
}

// VERIFY: bb8a4b54 auto
// OFFSET: 0x00491020
void CSimpleFrame::SetFrameStrata(FRAME_STRATA strata) {
    if (strata == this->m_strata) {
        return;
    }

    if (this->m_isVisible) {
        this->m_simpleTop->HideFrame(this, 1);
    }

    this->m_strata = strata;

    if (this->m_isVisible) {
        this->m_simpleTop->ShowFrame(this, 1);
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->SetFrameStrata(strata);
    }
}

// VERIFY: fcfe87c9 manual
// OFFSET: 0x004910A0
void CSimpleFrame::SetFrameLevel(int32_t level, int32_t shiftChildren) {
    if (level < 0) {
        level = 0;
    }

    if (level == this->m_level) {
        return;
    }

    int32_t delta = level - this->m_level;

    if (this->m_isVisible) {
        this->m_simpleTop->HideFrame(this, 1);
    }

    if (delta > 128) {
        delta = 128;
    }

    this->m_level += delta;

    if (this->m_isVisible) {
        this->m_simpleTop->ShowFrame(this, 1);
    }

    if (shiftChildren) {
        for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
            CSimpleFrame* child = node->m_frame;

            if (child->m_strata == this->m_strata) {
                child->SetFrameLevel(delta + child->m_level, 1);
            }
        }
    }
}

// VERIFY: b8653e4f manual
// OFFSET: 0x00491160
void CSimpleFrame::UnparentFrame(CSimpleFrame* child) {
    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        if (node->m_frame == child) {
            this->m_children.DeleteNode(node);
            break;
        }
    }
}

// VERIFY: 53398f96 manual
// OFFSET: 0x004911B0
void CSimpleFrame::SetParent(CSimpleFrame* parent) {
    if (parent == this->m_parent) {
        return;
    }

    uint32_t visible = this->m_isVisible;
    uint32_t shown = this->m_isShown;

    if (this->m_parent) {
        this->OnProtectedDetach(this->m_parent);
        this->m_parent->UnparentFrame(this);
    }

    if (visible) {
        this->HideThis();
    }

    this->m_parent = parent;

    if (parent) {
        this->SetFrameStrata(static_cast<FRAME_STRATA>(parent->m_strata));
        this->SetFrameLevel(this->m_parent->m_level + 1, 1);
        this->UpdateScale(false);

        uint8_t alpha = this->m_parent->m_alpha * this->m_parent->m_parentAlpha / 255;

        if (alpha != this->m_parentAlpha) {
            this->m_parentAlpha = alpha;
            this->UpdateAlpha();
        }

        this->SetBeingScrolled((this->m_parent->m_frameFlags >> 13) & 1, -1);
    } else {
        this->SetFrameStrata(FRAME_STRATA_MEDIUM);
        this->SetFrameLevel(0, 1);
        this->UpdateScale(false);

        if (this->m_parentAlpha != 0xFF) {
            this->m_parentAlpha = 0xFF;
            this->UpdateAlpha();
        }

        this->SetBeingScrolled(0, -1);
    }

    if (this->m_parent) {
        this->OnProtectedAttach(this->m_parent);
        this->m_parent->ParentFrame(this);
    }

    if (shown && (!this->m_parent || this->m_parent->m_isVisible)) {
        this->ShowThis();
    }
}

// VERIFY: b2a7bb2c manual
// OFFSET: 0x00491350
int32_t CSimpleFrame::AreChildrenProtected(int32_t& deferred) {
    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        if (node->m_frame->m_frameFlags & 0x800) {
            deferred = 1;
        } else if (node->m_frame->IsProtected()) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 53570259 manual
// OFFSET: 0x004913C0
bool CSimpleFrame::GetBoundsRect(CRect& rect) {
    CRect frameRect = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (this->IsResizePending()) {
        this->Resize(1);
    }

    if (this->GetRect(&frameRect)) {
        rect.minX = frameRect.minX >= rect.minX ? rect.minX : frameRect.minX;
        rect.maxX = frameRect.maxX <= rect.maxX ? rect.maxX : frameRect.maxX;
        rect.minY = frameRect.minY >= rect.minY ? rect.minY : frameRect.minY;
        rect.maxY = frameRect.maxY <= rect.maxY ? rect.maxY : frameRect.maxY;
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        if (!region->m_shown) {
            continue;
        }

        if (region->IsResizePending()) {
            region->Resize(1);
        }

        if (region->GetRect(&frameRect)) {
            rect.minX = frameRect.minX >= rect.minX ? rect.minX : frameRect.minX;
            rect.maxX = frameRect.maxX <= rect.maxX ? rect.maxX : frameRect.maxX;
            rect.minY = frameRect.minY >= rect.minY ? rect.minY : frameRect.minY;
            rect.maxY = frameRect.maxY <= rect.maxY ? rect.maxY : frameRect.maxY;
        }
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        if (node->m_frame->m_isShown) {
            node->m_frame->GetBoundsRect(rect);
        }
    }

    return rect.maxY > rect.minY && rect.maxX > rect.minX;
}

// VERIFY: f8739e8e auto
// OFFSET: 0x004915A0
bool CSimpleFrame::UpdateScale(bool force) {
    float scale = this->m_frameScale;

    if (this->m_parent) {
        scale *= this->m_parent->m_layoutScale;
    }

    if ((!force && fabs(scale - this->m_layoutScale) < 0.00000023841858f) || scale == 0.0f) {
        return false;
    }

    this->CLayoutFrame::SetLayoutScale(scale, force);

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->SetLayoutScale(scale, force);
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->UpdateScale(false);
    }

    return true;
}

// VERIFY: 24578191 manual
// OFFSET: 0x00491680
void CSimpleFrame::UpdateAlpha() {
    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->OnColorChanged(false);
    }

    uint8_t alpha = this->m_alpha * this->m_parentAlpha / 255;

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        CSimpleFrame* child = node->m_frame;

        if (alpha != child->m_parentAlpha) {
            child->m_parentAlpha = alpha;
            child->UpdateAlpha();
        }
    }
}

// VERIFY: c06c770d manual
// OFFSET: 0x00491740
bool CSimpleFrame::UpdateDepth(bool force) {
    float depth = this->m_parentDepth + this->m_depth;

    if (!force && fabs(depth - this->m_layoutDepth) < 0.00000023841858f) {
        return false;
    }

    this->CLayoutFrame::SetLayoutDepth(depth, force);

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->SetLayoutDepth(depth, force);
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        CSimpleFrame* child = node->m_frame;

        if (force || fabs(depth - child->m_parentDepth) >= 0.00000023841858f) {
            child->m_parentDepth = depth;
            child->UpdateDepth(force);
        }
    }

    return true;
}

// VERIFY: 9b62d0cd auto
// OFFSET: 0x00491830
int32_t CSimpleFrame::HideThis() {
    if (!this->m_isVisible) {
        return 1;
    }

    if (!this->m_isLoading) {
        this->SetDeferredResize(1);

        if (this->m_titleRegion) {
            this->m_titleRegion->SetDeferredResize(1);
        }
    }

    this->m_isVisible = 0;
    this->m_simpleTop->HideFrame(this, 0);

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->HideThis();
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->HideThis();
    }

    this->OnLayerHide();
    return 1;
}

// VERIFY: 7d89da2c manual
// OFFSET: 0x004918F0
int32_t CSimpleFrame::ShowThis() {
    if (!this->m_isShown || (this->m_parent && !this->m_parent->m_isVisible)) {
        return 0;
    }

    if (this->m_isVisible) {
        return 1;
    }

    if (!this->m_isLoading) {
        this->SetDeferredResize(0);

        if (this->m_titleRegion) {
            this->m_titleRegion->SetDeferredResize(0);
        }
    }

    this->m_isVisible = 1;
    this->m_simpleTop->ShowFrame(this, 0);

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->ShowThis();
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->ShowThis();
    }

    if (this->m_frameFlags & 0x1) {
        this->m_simpleTop->RaiseFrame(this, 1);
    }

    this->OnLayerShow();
    return 1;
}

// VERIFY: dddda613 manual
// OFFSET: 0x004919F0
void CSimpleFrame::StopAnimating() {
    CScriptRegion::StopAnimating();

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->StopAnimating();
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->StopAnimating();
    }
}

// VERIFY: 8b0118ef manual
// OFFSET: 0x00491A70
void CSimpleFrame::AnimActivated(CSimpleAnimGroup* group, int32_t first, int32_t hasAlpha) {
    if (first && (!this->m_playingAnimGroups || !this->m_playingAnimGroups->Head())) {
        this->StoreAnimPosition();
    }

    if (!this->m_animAlpha && hasAlpha) {
        this->m_animAlpha = 1;
        this->m_animSavedAlpha = this->m_alpha * this->m_parentAlpha / 255;
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->AnimActivated(group, first, hasAlpha);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->AnimActivated(group, first, hasAlpha);
    }
}

// VERIFY: 567bbc12 manual
// OFFSET: 0x00491B60
void CSimpleFrame::AnimDeactivated(CSimpleAnimGroup* group, int32_t last) {
    if (this->m_animAlpha) {
        this->m_animAlpha = this->HasActiveAnimOfType(CSimpleAlphaAnim::GetObjectType());

        if (!this->m_animAlpha && this->m_animSavedAlpha != this->m_alpha) {
            this->m_alpha = this->m_animSavedAlpha;
            this->UpdateAlpha();
        }
    }

    if (last && (!this->m_playingAnimGroups || !this->m_playingAnimGroups->Head())) {
        this->ResetAnimPosition();
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->AnimDeactivated(group, last);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->AnimDeactivated(group, last);
    }
}

// VERIFY: eba06fbd manual
// OFFSET: 0x00491C60
void CSimpleFrame::AddAnimTranslation(const C2Vector& translation) {
    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->AddAnimTranslation(translation);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->AddAnimTranslation(translation);
    }
}

// VERIFY: 61d08bd1 manual
// OFFSET: 0x00491CE0
void CSimpleFrame::AddAnimRotation(FRAMEPOINT origin, const C2Vector& offset, float radians) {
    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->AddAnimRotation(origin, offset, radians);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->AddAnimRotation(origin, offset, radians);
    }
}

// VERIFY: 2e5a88bd manual
// OFFSET: 0x00491D70
void CSimpleFrame::AddAnimScale(FRAMEPOINT origin, const C2Vector& offset, const C2Vector& scale) {
    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        node->m_frame->AddAnimScale(origin, offset, scale);
    }

    for (CSimpleRegion* region = this->m_regions.Head(); region; region = this->m_regions.Next(region)) {
        region->AddAnimScale(origin, offset, scale);
    }
}

// VERIFY: 1582c675 manual
// OFFSET: 0x00491E00
void CSimpleFrame::OnFrameRender() {
    uint32_t dirty = this->m_batchDirty;
    this->m_batchDirty = 0;

    if (dirty) {
        for (uint32_t layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; layer++) {
            if (!this->m_renderBatch[layer]) {
                void* mem = SMemAlloc(sizeof(CRenderBatch), __FILE__, __LINE__, 0);
                this->m_renderBatch[layer] = mem ? new (mem) CRenderBatch() : nullptr;
            }

            CRenderBatch* batch = this->m_renderBatch[layer];

            if (this->m_renderList.IsLinked(batch)) {
                this->m_renderList.UnlinkNode(batch);
            }

            if ((1 << layer) & dirty) {
                batch->Clear();
                this->OnFrameRender(batch, layer);
                batch->Finish();
            }

            if (batch->m_count) {
                this->m_renderList.LinkToTail(batch);
            }
        }

        if (this->m_batchDirty & 0x20) {
            for (uint32_t layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; layer++) {
                this->m_batchDirty |= 1 << layer;
            }
        }
    }

    for (uint32_t layer = 0; layer < NUM_SIMPLEFRAME_DRAWLAYERS; layer++) {
        if (this->m_renderBatch[layer]) {
            CSimpleRender::DrawBatch(this->m_renderBatch[layer]);
        }
    }

    for (SIMPLEFRAMENODE* node = this->m_children.Head(); node; node = this->m_children.Next(node)) {
        CSimpleFrame* child = node->m_frame;

        if (child->m_isVisible && !(child->m_frameFlags & 0x4000)) {
            child->OnFrameRender();
        }
    }
}

// VERIFY: 00fe9b58 auto
// OFFSET: 0x00492BE0
bool CSimpleFrame::GetAttribute(const char* name, int32_t& luaRef) {
    FRAMEATTR* attr = this->m_attributes.Ptr(name);

    if (!attr || attr->m_luaRef == -1) {
        return false;
    }

    luaRef = attr->m_luaRef;
    return true;
}

// VERIFY: b799b38a manual
// OFFSET: 0x00492F70
void CSimpleFrame::LoadXML_Attributes(XMLNode* node, CStatus* status) {
    lua_State* L = FrameScript_GetContext();

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (SStrCmpI(child->m_name.m_str, "Attribute", STORM_MAX_STR)) {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown attributes element %s", frameName ? frameName : "<unnamed>", child->m_name.m_str);
            continue;
        }

        const char* name = child->GetAttributeByName("name");

        if (!name) {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: unnamed attribute element", frameName ? frameName : "<unnamed>");
            continue;
        }

        const char* type = child->GetAttributeByName("type");

        if (!type) {
            type = "string";
        }

        const char* value = child->GetAttributeByName("value");

        if (!value && SStrCmpI(type, "nil", STORM_MAX_STR)) {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: attribute element named %s missing value", frameName ? frameName : "<unnamed>", name);
            continue;
        }

        if (!SStrCmpI(type, "nil", STORM_MAX_STR)) {
            lua_pushnil(L);
        } else if (!SStrCmpI(type, "boolean", STORM_MAX_STR)) {
            lua_pushboolean(L, StringToBOOL(value));
        } else if (!SStrCmpI(type, "number", STORM_MAX_STR)) {
            lua_pushnumber(L, SStrToFloat(value));
        } else {
            lua_pushstring(L, value);
        }

        FRAMEATTR* attr = this->m_attributes.Ptr(name);

        if (attr) {
            luaL_unref(L, LUA_REGISTRYINDEX, attr->m_luaRef);
        } else {
            attr = this->m_attributes.New(name, 0, 0);
        }

        const char* savedTaint = lua_tainted;

        if (++lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = nullptr;
        }

        lua_settaint(L, -1, nullptr);
        attr->m_luaRef = luaL_ref(L, LUA_REGISTRYINDEX);

        if (lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = savedTaint;
        }

        if (--lua_taintexpected <= 0) {
            lua_taintexpected = 0;
        }
    }
}

// VERIFY: 1a129ce3 manual
// OFFSET: 0x00493230
void CSimpleFrame::SetAttribute(const char* name, int32_t luaRef) {
    FRAMEATTR* attr = this->m_attributes.Ptr(name);

    if (!attr) {
        attr = this->m_attributes.New(name, 0, 0);
    }

    attr->m_luaRef = luaRef;
    this->RunOnAttributeChangedScript(name, luaRef);
}

// VERIFY: 545a4263 manual
// OFFSET: 0x004932C0
void CSimpleFrame::LoadXML(XMLNode* node, CStatus* status) {
    const char* inherits = node->GetAttributeByName("inherits");

    if (inherits && *inherits) {
        char name[1024];

        do {
            SStrTokenize(&inherits, name, sizeof(name), " ,", nullptr);

            if (!name[0]) {
                break;
            }

            const char* taint;
            bool locked;
            XMLNode* inheritNode = FrameXML_AcquireHashNode(name, taint, locked);

            if (!inheritNode) {
                status->Add(STATUS_WARNING, "Couldn't find inherited node: %s", name);
            } else if (locked) {
                status->Add(STATUS_WARNING, "Recursively inherited node: %s", name);
            } else {
                FrameScript_GetContext();

                const char* savedTaint = lua_tainted;

                if (++lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = taint;
                }

                const char* savedTag = lua_gctag;
                lua_gctag = taint;

                this->LoadXML(inheritNode, status);

                lua_gctag = savedTag;

                if (lua_taintexpected && !lua_taintedclosure) {
                    lua_tainted = savedTaint;
                }

                if (--lua_taintexpected <= 0) {
                    lua_taintexpected = 0;
                }

                FrameXML_ReleaseHashNode(name);
            }
        } while (name[0]);
    }

    CScriptRegion::LoadXML(node, status);

    const char* hidden = node->GetAttributeByName("hidden");

    if (hidden && *hidden) {
        if (StringToBOOL(hidden)) {
            this->Hide();
        } else {
            this->Show();
        }
    }

    const char* toplevel = node->GetAttributeByName("toplevel");

    if (toplevel && *toplevel) {
        this->SetFrameFlag(0x1, StringToBOOL(toplevel));
    }

    const char* movable = node->GetAttributeByName("movable");

    if (movable && *movable) {
        this->SetFrameFlag(0x100, StringToBOOL(movable));
    }

    const char* dontSavePosition = node->GetAttributeByName("dontSavePosition");

    if (dontSavePosition && *dontSavePosition) {
        this->SetFrameFlag(0x80000, StringToBOOL(dontSavePosition));
    }

    const char* resizable = node->GetAttributeByName("resizable");

    if (resizable && *resizable) {
        this->SetFrameFlag(0x200, StringToBOOL(resizable));
    }

    const char* frameStrata = node->GetAttributeByName("frameStrata");

    if (frameStrata && *frameStrata) {
        FRAME_STRATA strata;

        if (StringToFrameStrata(frameStrata, strata)) {
            this->SetFrameStrata(strata);
        } else {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown frame strata: %s", frameName ? frameName : "<unnamed>", frameStrata);
        }
    }

    const char* frameLevel = node->GetAttributeByName("frameLevel");

    if (frameLevel && *frameLevel) {
        int32_t level = SStrToInt(frameLevel);

        if (level > 0) {
            this->SetFrameLevel(level, 1);
        } else {
            const char* frameName = this->GetName();
            status->Add(STATUS_WARNING, "Frame %s: Unknown frame level: %s", frameName ? frameName : "<unnamed>", frameLevel);
        }
    }

    const char* alpha = node->GetAttributeByName("alpha");

    if (alpha && *alpha) {
        float value;

        if (SStrToFloat(alpha) < 1.0f && !(SStrToFloat(alpha) > 0.0f)) {
            value = 0.0f;
        } else if (SStrToFloat(alpha) >= 1.0f) {
            value = 1.0f;
        } else {
            value = SStrToFloat(alpha);
        }

        this->SetFrameAlpha(static_cast<uint8_t>(value * 255.0f));
    }

    const char* enableMouse = node->GetAttributeByName("enableMouse");

    if (enableMouse && *enableMouse && StringToBOOL(enableMouse)) {
        this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
    }

    const char* enableKeyboard = node->GetAttributeByName("enableKeyboard");

    if (enableKeyboard && *enableKeyboard && StringToBOOL(enableKeyboard)) {
        this->EnableEvent(SIMPLE_EVENT_KEY, -1);
        this->EnableEvent(SIMPLE_EVENT_CHAR, -1);
    }

    const char* clampedToScreen = node->GetAttributeByName("clampedToScreen");

    if (clampedToScreen && *clampedToScreen) {
        this->SetClampedToScreen(StringToBOOL(clampedToScreen));
    }

    const char* protectedAttr = node->GetAttributeByName("protected");

    if (protectedAttr && *protectedAttr && StringToBOOL(protectedAttr)) {
        this->SetProtected();
    }

    const char* depth = node->GetAttributeByName("depth");

    if (depth && *depth) {
        this->SetFrameDepth(SStrToFloat(depth), false);
    }

    for (XMLNode* child = node->m_child; child; child = child->m_next) {
        if (!SStrCmpI(child->m_name.m_str, "TitleRegion", STORM_MAX_STR)) {
            void* mem = SMemAlloc(sizeof(CSimpleTitleRegion), __FILE__, __LINE__, 0);
            CSimpleTitleRegion* titleRegion = mem ? new (mem) CSimpleTitleRegion : nullptr;

            titleRegion->SetParent(this);
            titleRegion->LoadXML(child, status);
            this->m_titleRegion = titleRegion;
            titleRegion->SetDeferredResize((this->m_layoutFlags >> 8) & 2);
        } else if (!SStrCmpI(child->m_name.m_str, "ResizeBounds", STORM_MAX_STR)) {
            float minWidth = 0.0f;
            float minHeight = 0.0f;
            float maxWidth = 0.0f;
            float maxHeight = 0.0f;

            XMLNode* minResize = child->GetChildByName("minResize");

            if (minResize) {
                LoadXML_Dimensions(minResize, minWidth, minHeight, status);
            }

            XMLNode* maxResize = child->GetChildByName("maxResize");

            if (maxResize) {
                LoadXML_Dimensions(maxResize, maxWidth, maxHeight, status);
            }

            this->m_resizeRect.minY = minHeight;
            this->m_resizeRect.minX = minWidth;
            this->m_resizeRect.maxY = maxHeight;
            this->m_resizeRect.maxX = maxWidth;
        } else if (!SStrCmpI(child->m_name.m_str, "Backdrop", STORM_MAX_STR)) {
            void* mem = SMemAlloc(sizeof(CBackdropGenerator), __FILE__, __LINE__, 0);
            CBackdropGenerator* backdrop = mem ? new (mem) CBackdropGenerator() : nullptr;

            backdrop->LoadXML(child, status);
            this->SetBackdrop(backdrop);
        } else if (!SStrCmpI(child->m_name.m_str, "HitRectInsets", STORM_MAX_STR)) {
            float left;
            float right;
            float top;
            float bottom;

            if (LoadXML_Insets(child, left, right, top, bottom, status)) {
                this->SetHitRectInsets(left, right, top, bottom);
            }
        } else if (!SStrCmpI(child->m_name.m_str, "Layers", STORM_MAX_STR)) {
            this->LoadXML_Layers(child, status);
        } else if (!SStrCmpI(child->m_name.m_str, "Attributes", STORM_MAX_STR)) {
            this->LoadXML_Attributes(child, status);
        } else if (!SStrCmpI(child->m_name.m_str, "Scripts", STORM_MAX_STR)) {
            this->LoadXML_Scripts(child, status);
        }
    }
}
