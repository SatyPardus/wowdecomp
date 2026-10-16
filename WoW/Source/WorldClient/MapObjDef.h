#ifndef WOW_SOURCE_WORLDCLIENT_MAPOBJDEF_H
#define WOW_SOURCE_WORLDCLIENT_MAPOBJDEF_H

#include "Engine/Source/Model2/M2Model.h"
#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CWFrustum.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WorldClient/MapBaseObj.h"

class CM2Lighting;

class CMapObj;
class CMapLight;
class CMapObjDefGroup;
namespace Liquid { class CInstance; }
class CRect;
class CMapObjDefMapObjDefGroupLink;

class CMapObjDefGroup : public CMapBaseObj {
    public:
    void Update(const C44Matrix& matrix);
    void SetSequence(uint32_t sequence, int32_t a3, uint16_t doodadSet);
    /* 0024 */ CAaBox m_bbox;
    /* 003C */ CAaSphere m_sphere;
    /* 004C */ float m_distanceToCamera;
    /* 0050 */ uint32_t m_groupNum;
    /* 0054 */ uint32_t m_unkFlags;
    /* 0058 */ int32_t unused_0058;
    /* 005C */ uint32_t m_ambientColor;
    /* 0060 */ const char* m_subzoneName;
    /* 0064 */ int32_t unused_0064;
    /* 0068 */ Liquid::CInstance* m_liquidInstance;
    /* 006C */ TSExplicitList<CWFrustum, offsetof(CWFrustum, m_link)> m_frustumList;
    /* 0078 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_doodadDefLinkList;
    /* 0084 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_entityLinkList;
    /* 0090 */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_lightLinkList;
    /* 009C */ TSList<CMapBaseObjLink, TSGetExplicitLink<CMapBaseObjLink> > m_unkLinkList;
    /* 00A8 */ TSLink<CMapObjDefGroup> m_sortEntryLink;
    /* 00B0 */ TSLink<CMapObjDefGroup> m_sortTableLink;
    /* 00B8 */ TSLink<CMapObjDefGroup> m_liquidLink;

    virtual void SelectLights(CM2Lighting* lighting);

    CMapObjDefGroup();
    ~CMapObjDefGroup() override;
    void UpdateLights();
    void SetSequenceFinishedCallback(M2SequenceCallback callback, uint64_t param, uint16_t doodadSet);
    bool IsAllLoaded();
    void SetEventCallback(M2EventCallback callback, uint64_t param, uint16_t doodadSet);
};

class CMapObjDef : public CMapBaseObj, public TSHashObject<CMapObjDef, HASHKEY_NONE> {
    public:
    static void UpdateMatrix(CMapObjDef* mapObjDef, const C44Matrix& matrix);
    void SetSequenceFinishedCallback(M2SequenceCallback callback, uint64_t param, uint16_t doodadSet);
    static void UpdateMoved(CMapObjDef* mapObjDef);
    static void UpdatePos(CMapObjDef* mapObjDef, const C3Vector* position, float facing, float a4, float a5);
    static void LinkDoodadDef(CMapDoodadDef* doodadDef, CMapObjDef* mapObjDef, int32_t testBounds);
    static int32_t MoveDoodadDefs(CMapObjDef* source, uint16_t doodadSet, CMapObjDef* target, uint16_t targetDoodadSet);
    static void SetDoodadEmittersIgnoresDistance(CMapObjDef* mapObjDef, int32_t ignore, uint16_t doodadSet);
    static void SetDoodadsEnabled(CMapObjDef* mapObjDef, int32_t enable, uint16_t doodadSet);
    void SetEventCallback(M2EventCallback callback, uint64_t param, uint16_t doodadSet);
    /* 003C */ C3Vector m_position;
    /* 0048 */ CAaBox m_bbox;
    /* 0060 */ CAaSphere m_sphere;
    /* 0070 */ C44Matrix m_mat;
    /* 00B0 */ C44Matrix m_invMat;
    /* 00F0 */ int32_t m_nameId;
    /* 00F4 */ CMapObj* m_owner;
    /* 00F8 */ int32_t unused_00F8;
    /* 00FC */ uint32_t m_unkFlags;
    /* 0100 */ int32_t m_doodadSet;
    /* 0104 */ int32_t m_nameSet;
    /* 0108 */ int32_t unused_0108;
    /* 010C */ TSLink<CMapObjDef> m_horizonLink;
    /* 0114 */ TSExplicitList<CMapBaseObjLink, offsetof(CMapBaseObjLink, m_refLink)> m_mapObjDefGroupLinkList;
    /* 0120 */ TSCGrowableArray<CMapObjDefGroup*, 4> m_defGroups;
    /* 0134 */ TSGrowableArray<CMapLight*> m_lightsArray;
    /* 0144 */ uint32_t m_argbColor;
    /* 0148 */ WOWGUID m_objectGuid;
    /* 0150 */ uint16_t m_doodadSetOverrides[3];
    /* 0156 */ uint8_t unused_0156[2];

    virtual ~CMapObjDef();

    CMapObjDef();
    bool TestAABox(C3Vector& start, C3Vector& end);
    bool IsAllDrawable();
    bool IsAllLoaded();
    int32_t GetGroundType(uint32_t groupNum, uint16_t faceIndex);
    void RenderWithCallback(TSGrowableArray<uint32_t>* groups);
    void CullMapObjDefGroupFromExterior(CMapObjDefGroup* mapObjDefGroup, CRect* rect, uint32_t skipOcclusion);
    void SetSequence(uint32_t sequence, int32_t a3, uint16_t doodadSet);
};

#endif
