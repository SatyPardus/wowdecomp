#ifndef ENGINE_SOURCE_FRAME_CSIMPLEFRAME_H
#define ENGINE_SOURCE_FRAME_CSIMPLEFRAME_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CSimpleRegion.h"
#include "Engine/Source/Frame/CSimpleAnim.h"
#include "Engine/Source/Frame/Types.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/CRect.h"

class CBackdropGenerator;
class CCharEvent;
class CImeEvent;
class CKeyEvent;
class CMouseEvent;
class CSimpleFrame;
class CStatus;
class CSimpleTop;
struct SIMPLEFRAMENODE;
struct lua_State;

struct FRAMEATTR : public TSHashObject<FRAMEATTR, HASHKEY_STRI> {
    /* 0018 */ int32_t m_luaRef;
};

class FrameScriptFunctionRef : public XMLNodeData {
    public:
    /* 0004 */ int32_t m_function;

    virtual ~FrameScriptFunctionRef();
    virtual void Release();

    FrameScriptFunctionRef(int32_t function);
};

class CSimpleFrame : public CScriptRegion {
    public:
    static CDataAllocator s_allocator;
    static int32_t s_frameCount;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr);
    static void CreateScriptMetaTable();
    static void DestroyScriptMetaTable();
    static int32_t GetObjectType();
    static void RegisterScriptMethods(lua_State* L);

    /* 00A0 */ CSimpleTop* m_simpleTop;
    /* 00A4 */ CSimpleFrame* m_hoverFrame;
    /* 00A8 */ CScriptRegion* m_titleRegion;
    /* 00AC */ uint32_t m_lifecycleState;
    /* 00B0 */ int32_t m_id;
    /* 00B4 */ uint32_t m_frameFlags;
    /* 00B8 */ float m_frameScale;
    /* 00BC */ uint8_t m_alpha;
    /* 00BD */ uint8_t m_parentAlpha;
    /* 00BE */ uint8_t m_animSavedAlpha;
    /* 00BF */ uint8_t m_animating : 1;
    /* 00BF */ uint8_t m_animAlpha : 1;
    /* 00C0 */ uint32_t unused_00C0;
    /* 00C4 */ float m_depth;
    /* 00C8 */ float m_parentDepth;
    /* 00CC */ uint32_t m_ignoreDepth;
    /* 00D0 */ uint32_t m_strata;
    /* 00D4 */ int32_t m_level;
    /* 00D8 */ uint32_t m_enabledEventsBitmask;
    /* 00DC */ uint32_t m_isShown;
    /* 00E0 */ uint32_t m_isVisible;
    /* 00E4 */ CRect m_hitRect;
    /* 00F4 */ CRect m_hitRectInsets;
    /* 0104 */ CRect m_clampRectInsets;
    /* 0114 */ uint32_t m_highlightLocked;
    /* 0118 */ uint32_t m_dragButtons;
    /* 011C */ uint32_t m_isMouseDown;
    /* 0120 */ uint32_t m_isMouseDragging;
    /* 0124 */ uint32_t m_mouseDragButton;
    /* 0128 */ C2Vector m_mousePosition;
    /* 0130 */ uint32_t m_isLoading;
    /* 0134 */ FrameScript_Object::ScriptIx m_onLoad;
    /* 013C */ FrameScript_Object::ScriptIx m_onSizeChanged;
    /* 0144 */ FrameScript_Object::ScriptIx m_onUpdate;
    /* 014C */ FrameScript_Object::ScriptIx m_onShow;
    /* 0154 */ FrameScript_Object::ScriptIx m_onHide;
    /* 015C */ FrameScript_Object::ScriptIx m_onCursorEnter;
    /* 0164 */ FrameScript_Object::ScriptIx m_onCursorExit;
    /* 016C */ FrameScript_Object::ScriptIx m_onMouseDown;
    /* 0174 */ FrameScript_Object::ScriptIx m_onMouseUp;
    /* 017C */ FrameScript_Object::ScriptIx m_onMouseWheel;
    /* 0184 */ FrameScript_Object::ScriptIx m_onDragStart;
    /* 018C */ FrameScript_Object::ScriptIx m_onDragStop;
    /* 0194 */ FrameScript_Object::ScriptIx m_onReceiveDrag;
    /* 019C */ FrameScript_Object::ScriptIx m_onChar;
    /* 01A4 */ FrameScript_Object::ScriptIx m_onKeyDown;
    /* 01AC */ FrameScript_Object::ScriptIx m_onKeyUp;
    /* 01B4 */ FrameScript_Object::ScriptIx m_onAttributeChanged;
    /* 01BC */ FrameScript_Object::ScriptIx m_onEnable;
    /* 01C4 */ FrameScript_Object::ScriptIx m_onDisable;
    /* 01CC */ TSHashTable<FRAMEATTR, HASHKEY_STRI> m_attributes;
    /* 01F4 */ uint32_t m_drawLayerEnabled [NUM_SIMPLEFRAME_DRAWLAYERS];
    /* 0208 */ CBackdropGenerator* m_backdropGenerator;
    /* 020C */ TSExplicitList<CSimpleRegion, offsetof(CSimpleRegion, m_regionLink)> m_regions;
    /* 0218 */ TSExplicitList<CSimpleRegion, offsetof(CSimpleRegion, m_layerLink)> m_drawLayers [NUM_SIMPLEFRAME_DRAWLAYERS];
    /* 0254 */ uint32_t m_batchDirty;
    /* 0258 */ CRenderBatch* m_renderBatch [NUM_SIMPLEFRAME_DRAWLAYERS];
    /* 026C */ TSExplicitList<CRenderBatch, offsetof(CRenderBatch, m_renderLink)> m_renderList;
    /* 0278 */ TSList<SIMPLEFRAMENODE> m_children;
    /* 0284 */ TSLink<CSimpleFrame> m_framesLink;
    /* 028C */ TSLink<CSimpleFrame> m_destroyedLink;
    /* 0294 */ TSLink<CSimpleFrame> m_strataLink;

    virtual ~CSimpleFrame();
    virtual int32_t GetScriptMetaTable();
    virtual FrameScript_Object::ScriptIx* GetScriptByName(const char* name, FrameScript_Object::ScriptData& data);
    virtual bool IsA(int32_t type);
    virtual bool IsA(const char* typeName);
    virtual const char* GetObjectTypeName();
    virtual void SetParent(CSimpleFrame* parent);
    virtual bool IsDragging();
    virtual void PreOnAnimUpdate();
    virtual void StopAnimating();
    virtual void AnimActivated(CSimpleAnimGroup* group, int32_t first, int32_t hasAlpha);
    virtual void AnimDeactivated(CSimpleAnimGroup* group, int32_t last);
    virtual void AddAnimTranslation(const C2Vector& translation);
    virtual void AddAnimRotation(FRAMEPOINT origin, const C2Vector& offset, float radians);
    virtual void AddAnimScale(FRAMEPOINT origin, const C2Vector& offset, const C2Vector& scale);
    virtual void AddAnimAlpha(int16_t alpha);
    virtual void PreLoadXML(XMLNode* node, CStatus* status);
    virtual void GetScriptTime(lua_State* L, double* time, uint32_t* count, int32_t recurse);
    virtual void PostLoadXML(XMLNode* node, CStatus* status);
    virtual void UnregisterRegion(CSimpleRegion* region);
    virtual bool GetBoundsRect(CRect& rect);
    virtual void StoreAnimPosition();
    virtual void ResetAnimPosition();
    virtual void OnLayerShow();
    virtual void OnLayerHide();
    virtual void OnLayerUpdate(float elapsedSec);
    virtual int32_t OnLayerTrackUpdate(const CMouseEvent& evt);
    virtual void OnFrameRender(CRenderBatch* batch, uint32_t layer);
    virtual void OnFrameRender();
    virtual void OnScreenSizeChanged();
    virtual void OnFrameSizeChanged(float width, float height);
    virtual void OnLayerCursorEnter(int32_t motion);
    virtual void OnLayerCursorExit(int32_t motion, int32_t a3);
    virtual int32_t OnLayerIme(const CImeEvent& evt);
    virtual int32_t OnLayerKeyDownRepeat(const CKeyEvent& evt);
    virtual int32_t OnLayerChar(const CCharEvent& evt);
    virtual int32_t OnLayerKeyDown(const CKeyEvent& evt);
    virtual int32_t OnLayerKeyUp(const CKeyEvent& evt);
    virtual int32_t OnLayerMouseDown(const CMouseEvent& evt, const char* btn);
    virtual int32_t OnLayerMouseUp(const CMouseEvent& evt, const char* btn);
    virtual int32_t OnLayerMouseWheel(const CMouseEvent& evt);
    virtual void OnDragStart(const CMouseEvent& evt);
    virtual void OnDragStop(const CMouseEvent& evt);
    virtual void OnReceiveDrag(const CMouseEvent& evt);
    virtual void LockHighlight(int32_t lock);
    virtual int32_t HideThis();
    virtual int32_t ShowThis();
    virtual bool UpdateScale(bool force);
    virtual void UpdateAlpha();
    virtual bool UpdateDepth(bool force);
    virtual void ParentFrame(CSimpleFrame* child);
    virtual void UnparentFrame(CSimpleFrame* child);
    virtual void LoadXML(XMLNode* node, CStatus* status);
    virtual void PropagateProtectFlagToParent(uint32_t flag);
    virtual int32_t AreChildrenProtected(int32_t& deferred);
    virtual void GetClampRectInsets(float& left, float& right, float& top, float& bottom);
    virtual void OnFrameSizeChanged(const CRect& rect);

    CSimpleFrame(CSimpleFrame* parent);
    void AddFrameRegion(CSimpleRegion* region, uint32_t drawLayer);
    void AllowAttributeChanges(int32_t allow);
    int32_t AttributeChangesAllowed();
    void DisableDrawLayer(uint32_t drawLayer);
    void DisableEvent(CSimpleEventType eventType);
    int32_t DragBy(float offsetX, float offsetY, FRAMEPOINT point);
    void EnableDrawLayer(uint32_t drawLayer);
    void EnableEvent(CSimpleEventType eventType, int32_t priority);
    bool GetAttribute(const char* name, int32_t& luaRef);
    uint8_t GetEffectiveAlpha();
    int32_t GetHitRect(CRect& rect);
    void GetHitRectInsets(float& left, float& right, float& top, float& bottom);
    int32_t Hide();
    void LoadXML_Attributes(XMLNode* node, CStatus* status);
    void LoadXML_Layers(XMLNode* node, CStatus* status);
    void LoadXML_Scripts(XMLNode* node, CStatus* status);
    void LoadXMLInheritance(XMLNode* node, CStatus* status);
    int32_t Lower();
    void NotifyDelayedLoad();
    void NotifyDrawLayerChanged(uint32_t drawLayer);
    void NotifyScrollParent();
    void Raise();
    void RegisterForEvents(int32_t a2);
    void RegisterRegion(CSimpleRegion* region);
    void RemoveFrameRegion(CSimpleRegion* region, uint32_t drawLayer);
    void RunOnAttributeChangedScript(const char* name, int32_t luaRef);
    void RunOnEnterScript(int32_t motion);
    void RunOnKeyUpScript(const char* key);
    void RunOnLeaveScript(int32_t motion);
    void RunOnMouseWheelScript(int32_t delta);
    void RunOnSizeChangedScript(float width, float height);
    void SetAttribute(const char* name, int32_t luaRef);
    void SetBackdrop(CBackdropGenerator* backdrop);
    void SetBeingScrolled(int32_t scrolled, int32_t a3);
    void SetClampRectInsets(float left, float right, float top, float bottom);
    void SetFrameAlpha(uint8_t alpha);
    bool SetFrameDepth(float depth, bool force);
    void SetFrameFlag(uint32_t flag, int32_t set);
    void SetFrameLevel(int32_t level, int32_t shiftChildren);
    bool SetFrameScale(float scale, bool force);
    void SetFrameStrata(FRAME_STRATA strata);
    void SetHitRectInsets(float left, float right, float top, float bottom);
    int32_t Show();
    int32_t TestHitRect(const C2Vector& pt);
    void UnregisterForEvents(int32_t a2);
};

#endif
