#ifndef WOW_SOURCE_UI_WORLDFRAME_H
#define WOW_SOURCE_UI_WORLDFRAME_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Event/Types.h"
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Tempest/CRect.h"

class CGGameObject_C;
class CGUnit_C;
class CGCorpse_C;
class CGCamera;
class CGObject_C;
class CKeyEvent;
class CMouseEvent;
class CRenderBatch;

namespace FFX {
    class Effect;
}

class CGWorldFrameUnkObjSomeElement {
    public:
    /* 0000 */ uint32_t unused_0000;
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint32_t unused_0008;
    /* 000C */ uint32_t unused_000C;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ float unused_0014;
    /* 0018 */ float unused_0018;
    /* 001C */ float unused_001C;
    /* 0020 */ float unused_0020;
    /* 0024 */ float unused_0024;
    /* 0028 */ float unused_0028;
    /* 002C */ float unused_002C;
    /* 0030 */ float unused_0030;
    /* 0034 */ float unused_0034;
    /* 0038 */ float unused_0038;
    /* 003C */ uint32_t unused_003C;
    /* 0040 */ uint32_t unused_0040;
    /* 0044 */ uint32_t unused_0044;
    /* 0048 */ uint32_t unused_0048;
    /* 004C */ uint32_t unused_004C;
    /* 0050 */ uint32_t unused_0050;
    /* 0054 */ uint32_t unused_0054;
    /* 0058 */ uint32_t unused_0058;
    /* 005C */ uint32_t unused_005C;
    /* 0060 */ uint32_t unused_0060;
    /* 0064 */ uint32_t unused_0064;
    /* 0068 */ uint32_t unused_0068;
};

class CGWorldFrameUnkObj {
    public:
    /* 0000 */ CGWorldFrameUnkObjSomeElement m_elements [16];
    /* 06C0 */ uint8_t unused_06C0 [140];
    /* 074C */ float m_tail [13];

    CGWorldFrameUnkObj();
};

class CModelRecord : public TSLinkedNode<CModelRecord> {
    public:
    /* 0008 */ CM2Model* m_model;
    /* 000C */ float m_distance;
    /* 0010 */ WOWGUID m_guid;
    /* 0018 */ CGObject_C* m_object;
    /* 001C */ uint32_t unused_001C;

    CModelRecord() {
        this->m_model = nullptr;
        this->m_distance = std::numeric_limits<float>::infinity();
        this->m_guid = WOWGUID();
        this->m_object = nullptr;
    }

    ~CModelRecord() {
        if (this->m_model) {
            this->m_model->Release();
        }
    }
};

struct HITTESTRESULT {
    /* 0000 */ WOWGUID m_guid;
    /* 0008 */ C3Vector m_point;
    /* 0014 */ float m_distance;
    /* 0018 */ C3Vector m_segStart;
    /* 0024 */ C3Vector m_segEnd;
};

struct KEYDOWNSTATE {
    /* 0000 */ char m_keyString [32];
    /* 0020 */ uint32_t m_modifiers;
};

class CGWorldFrame : public CSimpleFrame {
    public:
    int32_t SetupDefaultAction();
    static CDataAllocator s_worldFrameHeap;
    static FFX::Effect* ffxEffectSwirlingFog;
    static FFX::Effect* ffxEffectNetherWorld;
    static FFX::Effect* ffxEffectGlow;
    static FFX::Effect* ffxEffectDeath;
    static CGWorldFrame* s_currentWorldFrame;

    static CSimpleFrame* Create(CSimpleFrame* parent);
    static CGCamera* GetActiveCamera();
    static void GetCameraFacing(C3Vector* facing);
    static void GetCameraPosition(C3Vector* position);
    static void RegisterFrames();
    static void SetScreenEffect(int32_t effect);
    static void UpdateScreenEffect();
    static void UpdateScreenEffectParams();
    static int32_t Update(const void* data, void* param);
    static void RenderWorld(void* param);
    static int32_t ObjectEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32);
    static int32_t MissileEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32);
    static int32_t SpellEffectEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32);

    /* 029C */ TSList<CModelRecord, TSGetLink<CModelRecord> > m_modelList;
    /* 02A8 */ TSList<CModelRecord, TSGetLink<CModelRecord> > m_hitModelList;
    /* 02B4 */ TSList<CModelRecord, TSGetLink<CModelRecord> > m_freeModelList;
    /* 02C0 */ uint32_t unused_02C0;
    /* 02C4 */ uint32_t unused_02C4;
    /* 02C8 */ WOWGUID m_trackedGuid;
    /* 02D0 */ WOWGUID m_hitGuid;
    /* 02D8 */ uint32_t m_defaultActionHitKind;
    /* 02DC */ uint32_t unused_02DC;
    /* 02E0 */ HITTESTRESULT m_defaultActionHit;
    /* 0310 */ C2Vector m_defaultActionPointDDC;
    /* 0318 */ uint32_t unused_0318;
    /* 031C */ uint32_t m_worldFlags;
    /* 0320 */ CRect m_viewportDDC;
    /* 0330 */ CRect m_viewportNDC;
    /* 0340 */ C44Matrix m_viewProjection;
    /* 0380 */ WOWGUID m_trackedEffectGuidA;
    /* 0388 */ WOWGUID m_trackedEffectGuidB;
    /* 0390 */ CGWorldFrameUnkObj unused_0390;
    /* 0B10 */ uint32_t m_namePlateFlags;
    /* 0B14 */ float m_elapsedSec;
    /* 0B18 */ KEYDOWNSTATE m_keyDown [787];
    /* 79C4 */ KEYDOWNSTATE m_mouseDown [31];
    /* 7E20 */ CGCamera* m_camera;
    /* 7E24 */ uint32_t unused_7E24;

    CGWorldFrame(CSimpleFrame* parent);
    ~CGWorldFrame() override;
    void OnLayerUpdate(float elapsedSec) override;
    int32_t OnLayerTrackUpdate(const CMouseEvent& evt) override;
    void OnFrameRender(CRenderBatch* batch, uint32_t layer) override;
    void OnFrameSizeChanged(const CRect& rect) override;
    void OnLayerCursorExit(int32_t motion, int32_t a3) override;
    int32_t OnLayerKeyDown(const CKeyEvent& evt) override;
    int32_t OnLayerKeyUp(const CKeyEvent& evt) override;
    int32_t OnLayerMouseDown(const CMouseEvent& evt, const char* btn) override;
    int32_t OnLayerMouseUp(const CMouseEvent& evt, const char* btn) override;
    int32_t OnLayerMouseWheel(const CMouseEvent& evt) override;
    void operator delete(void* ptr);
    void AddObjectToHitTestList(CModelRecord* record, uint32_t flags);
    void UpdateObject(CGObject_C* object, uint32_t flags);
    void ClearTrackedObjects();
    int32_t GetLineSegment(float mouseX, float mouseY, C3Vector* start, C3Vector* end);
    bool GetScreenCoordinates(const C3Vector& world, C3Vector* screen, uint32_t* result);
    bool HitTestModelList(const C3Vector& start, const C3Vector& end);
    void MoveToFreeList(TSList<CModelRecord, TSGetLink<CModelRecord> >* list);
    void OnMouseModeNormal();
    void OnMouseModeRelative();
    void PerformDefaultAction(MOUSEBUTTON button);
    void OnWorldRender();
    void OnWorldUpdate();
    void UpdateDayNight(float deltaSec);
    int32_t HandleSpriteTrack(WOWGUID guid);
    int32_t HitTestPoint(float x, float y, int32_t a3, HITTESTRESULT* result);
    int32_t CanTargetCorpseSelection(const CGCorpse_C* corpse, uint32_t flags);
    static void __stdcall CursorTrackObject(CGGameObject_C* object);
    static void __stdcall CursorTrackUnit(CGUnit_C* unit);
    static void __stdcall CursorTrackCorpse(CGCorpse_C* corpse);
    int32_t IsUnitLegalSelection(CGUnit_C* unit, uint32_t flags);
    int32_t IsLegalSelection(CGObject_C* object, uint32_t flags);
    uint32_t GetHitTestFilterFlags(int32_t a1);
    uint64_t FindClosestModel(const C3Vector& start, const C3Vector& end, uint32_t flags, float* distance);
    int32_t HitTest(const C3Vector& start, const C3Vector& end, uint32_t flags, HITTESTRESULT* result);
    void OnLayerTrackObject(const HITTESTRESULT& result);
    void OnLayerTrackTerrain(const HITTESTRESULT& result);
    void SetNamePlateUpdate();
    void SetPlayerFadeCameraValue(uint8_t value);
    void UnitClear();
    void SetCameraTarget(CGObject_C* target, int32_t freeLook);
};

class CWorldClickEvent {
    public:
    /* 0000 */ C3Vector m_segStart;
    /* 000C */ C3Vector m_segEnd;
    /* 0018 */ MOUSEBUTTON m_button;
};

#endif
