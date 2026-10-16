#include "MapDoodadDef.h"
#include "Engine/Source/Model2/M2Data.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Source/WorldClient/CMapBaseObjLink.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldView.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "Engine/Source/Tempest/C3Segment.h"

// VERIFY: 4012bf37 manual
// OFFSET: 0x007B4FA0
void CMapDoodadDef::ExtendBounds(CMapDoodadDef* doodadDef, CMapBaseObj* parent) {
    if (parent->m_type & 0x4) {
        CMapChunk* chunk = static_cast<CMapChunk*>(parent);
        chunk->m_bbox.t.z = doodadDef->m_bbox.t.z < chunk->m_bbox.t.z ? chunk->m_bbox.t.z : doodadDef->m_bbox.t.z;
    } else if (parent->m_type & 0x10) {
        CMapObjDefGroup* group = static_cast<CMapObjDefGroup*>(parent);
        group->m_bbox |= doodadDef->m_bbox;

        CMapBaseObjLink* link = group->m_parentLinkList.Head();
        static_cast<CMapObjDef*>(link->m_ref)->m_bbox |= doodadDef->m_bbox;
    }
}

// VERIFY: 8dd6d44c manual
// OFFSET: 0x007B55E0
void CMapDoodadDef::ExtendChunkBounds() {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapDoodadDef::ExtendBounds(this, link->m_ref);
    }
}

// VERIFY: 2d4db79f manual
// OFFSET: 0x007BDB10
void CMapDoodadDef::UpdateBounds() {
    CAaBox box;
    CAaSphere sphere;
    sphere.r = 0.0f;
    CAaBox collisionBox;

    if (this->m_model) {
        box = this->m_model->GetBoundingBox();
        sphere = this->m_model->GetBoundingSphere();

        CM2Model* model = this->m_model;

        if (!(model->m_shared->m_flags & 0x1)) {
            model->WaitForLoad(nullptr);
        }

        collisionBox = model->m_shared->m_data->m_collisionBounds.m_extent;
    }

    this->m_sphere.c = sphere.c * this->m_mat;
    this->m_sphere.r = this->m_scale * sphere.r;

    if (box.b.x <= box.t.x || box.b.y <= box.t.y || box.b.z <= box.t.z) {
        CWorldMath::TransformAABox(this->m_mat, box, this->m_bbox);
    } else {
        this->m_bbox.b = this->m_position;
        this->m_bbox.t = this->m_position;
    }

    CWorldMath::TransformAABox(this->m_mat, collisionBox, this->m_bboxDoodadDef);

    C3Vector center;
    center.x = (collisionBox.t.x + collisionBox.b.x) * 0.5f;
    center.y = (collisionBox.t.y + collisionBox.b.y) * 0.5f;
    center.z = 0.5f * (collisionBox.t.z + collisionBox.b.z);
    this->m_vec2 = center * this->m_mat;

    float size = this->m_bbox.t.x - this->m_bbox.b.x;

    if (this->m_bbox.t.y - this->m_bbox.b.y >= size) {
        size = this->m_bbox.t.y - this->m_bbox.b.y;
    }

    if (this->m_bbox.t.z - this->m_bbox.b.z >= size) {
        size = this->m_bbox.t.z - this->m_bbox.b.z;
    }

    uint8_t fadeLevel;

    for (fadeLevel = 0; fadeLevel < 4; fadeLevel++) {
        if (size <= CWorldView::s_fadeSize[fadeLevel]) {
            break;
        }
    }

    this->m_fadeLevel = fadeLevel;

    if (!(CWorld::enables & Enable_NoAnimation) && fadeLevel < 3) {
        this->m_model->m_fFlags &= ~0x40u;
    }
}

// VERIFY: f560b093 manual
// OFFSET: 0x007C1150
void CMapDoodadDef::SelectLights(CM2Lighting* lighting) {
    if (this->m_flags & 0x2) {
        lighting->AddAmbient(C3Vector(this->m_m2AmbientColor));
        lighting->AddDiffuse(C3Vector(this->m_m2DiffuseColor), CMapStaticEntity::s_interiorSunDir);
    } else {
        CM2Light* light = &CMap::s_mapLight->m_light;
        lighting->AddAmbient(light->m_ambColor);

        float scale = this->m_diffuseLightScale;
        C3Vector dir = light->m_dir;
        lighting->AddDiffuse(C3Vector(light->m_dirColor.x * scale, light->m_dirColor.y * scale, scale * light->m_dirColor.z), dir);
    }

    DayNight::DNInfo* dayNight = DayNight::GetInfo();

    if (this->m_flags & 0x8000) {
        lighting->SetFog(C3Vector(dayNight->m_fogInterior.m_color), dayNight->m_fogInterior.m_start, dayNight->m_fogInterior.m_end, dayNight->m_fogInterior.m_density);
    } else {
        lighting->SetFog(C3Vector(dayNight->m_fog.m_color), dayNight->m_fog.m_start, dayNight->m_fog.m_end, dayNight->m_fog.m_density);
    }

    if (this->m_flags & 0x2) {
        lighting->m_flags |= 0x8;
    } else {
        lighting->m_flags &= ~0x8;
    }
}

// VERIFY: fc0f43f4 manual
// OFFSET: 0x007C21E0
CMapDoodadDef::CMapDoodadDef() {
    this->m_type |= 0x40;
    this->m_model = nullptr;
    this->m_collisionGuid = WOWGUID();
    this->m_doodadSet = 0;
}

// VERIFY: cf40d12f manual
// OFFSET: 0x007C23F0
void CMapDoodadDef::SelectUnderwater(CM2Lighting* lighting) {
    if (!(this->m_staticFlags & 0x80)) {
        this->m_staticFlags = (this->m_staticFlags & ~0xE0u) | 0x80;

        for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
            CMapBaseObj* parent = link->m_ref;

            if (!(parent->m_type & 0x10)) {
                this->m_staticFlags |= 0x80;
                C3Vector pos(this->m_position.x, this->m_position.y, this->m_bbox.b.z);
                int32_t liquidType;

                if (CMap::QueryLiquidTerrain(&pos, &liquidType, &this->m_liquidHeight, 0)) {
                    this->m_diffuseLightScale = 1.0f;
                    this->m_staticFlags = (this->m_staticFlags & ~0x28u) | 0x20;

                    if (this->m_liquidHeight <= this->m_bbox.t.z) {
                        this->m_staticFlags |= 0x40;
                    } else {
                        this->m_staticFlags &= ~0x40u;
                    }
                }

                break;
            }

            CMapObjDefGroup* defGroup = static_cast<CMapObjDefGroup*>(parent);
            CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(defGroup->m_parentLinkList.Head()->m_ref);
            CMapObjGroup* group = mapObjDef->m_owner->GetGroup(defGroup->m_groupNum, 0);

            if (!group) {
                this->m_staticFlags &= ~0x80u;
                continue;
            }

            C3Vector localPos = C3Vector(this->m_position.x, this->m_position.y, this->m_bbox.b.z) * mapObjDef->m_invMat;
            int32_t liquidType;
            float level;

            if (group->QueryLiquid(localPos, &liquidType, &level)) {
                this->m_staticFlags |= 0x20;
                C3Vector surface = C3Vector(localPos.x, localPos.y, level) * mapObjDef->m_mat;
                this->m_liquidHeight = surface.z;

                if (this->m_liquidHeight <= this->m_bbox.t.z) {
                    this->m_staticFlags |= 0x40;
                } else {
                    this->m_staticFlags &= ~0x40u;
                }

                this->m_staticFlags |= 0x80;
                break;
            }
        }
    }

    this->CMapStaticEntity::SelectUnderwater(lighting);
}

// VERIFY: 5d30506f manual
// OFFSET: 0x007C3020
void CMapDoodadDef::Purge(CMapDoodadDef* doodadDef) {
    if (doodadDef->m_refCount) {
        return;
    }

    if (doodadDef->m_model) {
        doodadDef->m_model->SetSequenceCallback(nullptr, 0);
        doodadDef->m_model->SetEventCallback(nullptr, 0);
        doodadDef->m_model->m_lightingCallback = nullptr;
        doodadDef->m_model->m_lightingArg = nullptr;
        doodadDef->m_model->Release();
        doodadDef->m_model = nullptr;
    }

    doodadDef->m_doodadDefLink.Unlink();
    CMap::FreeDoodadDef(doodadDef);
}

// VERIFY: 9bd1ba20 manual
// OFFSET: 0x007BDA70
void CMapDoodadDef::CreateModel(const char* fileName, CMapDoodadDef* doodadDef, int32_t waitForLoad, int32_t a4) {
    CM2Model* model = World::s_m2Scene->CreateModel(fileName, a4 ? 0x20 : 0);
    doodadDef->m_model = model;

    if (!model) {
        return;
    }

    model->m_fFlags |= 0x8000;
    model->m_worldTransform = doodadDef->m_mat;
    doodadDef->m_model->SetEventCallback(reinterpret_cast<void*>(CMapStaticEntity::ModelEventCallback), reinterpret_cast<uintptr_t>(doodadDef));
    doodadDef->m_model->m_lightingCallback = CMapStaticEntity::ModelLightingCallback;
    doodadDef->m_model->m_lightingArg = doodadDef;
    doodadDef->m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);

    if (waitForLoad) {
        doodadDef->m_model->IsLoaded(1, 0);
    }
}

// VERIFY: 5cc39abd manual
// OFFSET: 0x007C1380
void CMapDoodadDef::UpdateMatrixOnly(const C44Matrix& matrix) {
    if (!this->m_model) {
        return;
    }

    this->m_flags = static_cast<MapObjFlag>(this->m_flags | MAPOBJ_FLAG_UNPLACED);
    this->m_position.x = this->m_identity.d0;
    this->m_position.y = this->m_identity.d1;
    this->m_position.z = this->m_identity.d2;
    this->m_position *= matrix;
    this->m_mat = this->m_identity * matrix;
    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);
    CAaBox collisionBox;
    collisionBox.b = C3Vector(0.0f, 0.0f, 0.0f);
    collisionBox.t = C3Vector(0.0f, 0.0f, 0.0f);
    CAaSphere sphere;
    sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    sphere.r = 0.0f;

    if (this->m_model->IsLoaded(0, 0)) {
        box = this->m_model->GetBoundingBox();
        CM2Model* model = this->m_model;

        if (!(model->m_shared->m_flags & 0x1)) {
            model->WaitForLoad(nullptr);
        }

        collisionBox = model->m_shared->m_data->m_collisionBounds.m_extent;
        sphere = this->m_model->GetBoundingSphere();
    } else {
        collisionBox = this->m_model->m_shared->m_boundingBox;
    }

    this->m_sphere.c = sphere.c * this->m_mat;
    this->m_sphere.r = this->m_scale * sphere.r;
    CWorldMath::TransformAABox(this->m_mat, box, this->m_bbox);
    CWorldMath::TransformAABox(this->m_mat, collisionBox, this->m_bboxDoodadDef);
    CM2Model* model = this->m_model;
    model->m_fFlags |= 0x8000;
    model->m_worldTransform = this->m_mat;
    SI2::Set3DPosition(&this->m_soundKitObject, &this->m_position);
}

// VERIFY: 33a4955f manual
// OFFSET: 0x007C1C40
bool CMapDoodadDef::QueryInteriorLighting(CMapObjDef* mapObjDef, int32_t groupNum, uint16_t* faceIndex, C3Vector* pos) {
    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0;
    uint8_t flag;
    bool result;

    if (faceIndex) {
        C3Vector local = *pos * mapObjDef->m_invMat;
        result = mapObjDef->m_owner->QueryLighting(groupNum, local, *faceIndex, &color, &flag);
    } else {
        C3Vector top(this->m_position.x, this->m_position.y, this->m_position.z + 1.5f);
        C3Vector start = top * mapObjDef->m_invMat;
        C3Vector bottom(this->m_position.x, this->m_position.y, this->m_position.z - 1760.0f);
        C3Vector end = bottom * mapObjDef->m_invMat;
        C3Segment seg;
        seg.b = start;
        seg.t = end;
        result = mapObjDef->m_owner->QueryLighting(groupNum, seg, &color, &flag);
    }

    if (!result) {
        return result;
    }

    CImVector limit;
    *reinterpret_cast<uint32_t*>(&limit) = 0xA8;
    this->AdjustLightmap(&color, &this->m_m2DiffuseColor, limit, &this->m_m2AmbientColor, 96);

    if (!flag) {
        this->m_staticFlags &= ~0x1000;
        return result;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    uint8_t alpha = color.a;

    if (alpha) {
        this->m_m2DiffuseColor.InterpColor(alpha, &info->m_light1.m_diffuse);
        this->m_m2AmbientColor.InterpColor(alpha, &info->m_light1.m_ambient);
    }

    this->m_staticFlags |= 0x1000;
    this->m_m2DiffuseColor.a = color.a;
    return result;
}

// VERIFY: aa96dd41 manual
// OFFSET: [0x007C2350, 0x007C2E40]
CMapDoodadDef::~CMapDoodadDef() {
}
