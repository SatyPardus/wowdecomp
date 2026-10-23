#include "MapEntity.h"
#include "WoW/Source/WorldClient/WorldLightingConstants.h"
#include "Engine/Source/Tempest/C3Segment.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/DB/Rec/WMOAreaTableRec.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include <cmath>
#include <cstring>
#include <typeinfo>
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/MapObjDef.h"
#include "WoW/Source/WorldClient/MapObjGroup.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/WorldClient/CWorldMath.h"

static const float dirLightScaleRate = 3.3333333f;

// STATIC: 0x00ADEEBC
float CMapStaticEntity::s_characterAmbient = 1.0f;

// STATIC: 0x00AEEDF0
C3Vector CMapStaticEntity::s_interiorSunDir = C3Vector(-0.30822f, -0.30822f, -0.9f);

// VERIFY: 4c01a51b manual
// OFFSET: 0x007A0B50
CMapStaticEntity::CMapStaticEntity() {
    this->m_sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_sphere.r = 0.0f;
    this->m_scale = 0.0f;
    this->m_staticFlags &= 0xFFFE4000;
    this->m_distanceToCamera = 0.0f;
    this->m_liquidHeight = 0.0f;
    this->m_model = nullptr;
    this->m_diffuseLightScale = 0.0f;
    this->m_unkCounter = 0;
    this->m_shadowStamps[0] = 0;
    this->m_shadowStamps[1] = 0;
    this->m_shadowStamps[2] = 0;
    this->unused_002B = 0;
    this->m_fadeLevel = 0;
    this->m_vec2 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_culled = 2;
}

// VERIFY: cd4cc195 auto
// OFFSET: 0x007A0C70
void FogQueue::Erase(uint32_t index, uint32_t count) {
    if (index >= this->m_count) {
        return;
    }

    if (index + count > this->m_count) {
        count = this->m_count - index;
    }

    uint32_t tail = this->m_count - index - count;

    if (tail) {
        memmove(this->m_data + sizeof(FogQ) * index, this->m_data + sizeof(FogQ) * (index + count), sizeof(FogQ) * tail);
    }

    this->m_count -= count;
}

// VERIFY: 67998dc9 manual
// OFFSET: 0x007A0CD0
static void LerpInteriorFog(SMOFog::Fog* dst, const SMOFog::Fog* src, float t) {
    for (int32_t i = 0; i < 2; i++) {
        dst[i].m_end = (src[i].m_end - dst[i].m_end) * t + dst[i].m_end;
        dst[i].m_startScalar = t * (src[i].m_startScalar - dst[i].m_startScalar) + dst[i].m_startScalar;
        dst[i].m_color = DayNight::InterpColor(dst[i].m_color, src[i].m_color, t);
    }
}

// VERIFY: a41b0f80 manual
// OFFSET: 0x007A0ED0
FogQueue::FogQueue(const uint32_t* params) : CMemBlock(sizeof(FogQ) * params[0], 0, typeid(FogQ).raw_name(), SLOG_OBJECT) {
    uint32_t count = params[0];
    uint32_t chunk = params[1];

    this->m_chunk = chunk;
    this->m_capacity = count;
    this->m_count = 0;

    if (count) {
        this->m_count++;
        return;
    }

    if (!chunk) {
        return;
    }

    uint32_t grow = chunk;

    if (grow <= 1 - count) {
        grow = 1 - count;
    }

    if (this->Resize(sizeof(FogQ) * (grow + count), true)) {
        this->m_capacity += grow;
        this->m_count++;
    }
}

// VERIFY: 307e1c8c manual
// OFFSET: 0x007A0F50
void FogQueue::Push(float distance, uint32_t fogIndex) {
    if (this->m_count + 1 <= this->m_capacity) {
        this->m_count++;
    } else if (this->m_chunk) {
        uint32_t grow = this->m_chunk;

        if (grow <= this->m_count - this->m_capacity + 1) {
            grow = this->m_count - this->m_capacity + 1;
        }

        if (this->Resize(sizeof(FogQ) * (grow + this->m_capacity), true)) {
            this->m_capacity += grow;
            this->m_count++;
        }
    }

    FogQ* data = reinterpret_cast<FogQ*>(this->m_data);
    uint32_t index = this->m_count - 1;

    if (this->m_count != 1 && index != 1) {
        uint32_t parent;

        do {
            data = reinterpret_cast<FogQ*>(this->m_data);
            parent = index >> 1;

            if (distance < data[parent].m_distance) {
                break;
            }

            data[index] = data[parent];
            index >>= 1;
        } while (parent > 1);
    }

    data = reinterpret_cast<FogQ*>(this->m_data);
    data[index].m_distance = distance;
    data[index].m_fogIndex = fogIndex;
}

// VERIFY: eeb1ad46 manual
// OFFSET: 0x007A0FE0
CMapEntity::CMapEntity() {
    this->unused_00A8 = C3Vector(0.0f, 0.0f, 0.0f);
    this->m_func = nullptr;
    this->m_funcParam = nullptr;
    this->m_funcParam64 = 0;
    this->m_funcParam32 = 0;
    this->unused_00B4 = 0;
    this->m_groundType = 0;
    this->m_liquidType = 0;
    this->unused_00BE = 0;
    this->unk_00A4 = 0xFFFFFFFF;
    this->m_targetAmbientColor.b = 0xFF;
    this->m_targetAmbientColor.g = 0x00;
    this->m_targetAmbientColor.r = 0x00;
    this->m_targetAmbientColor.a = 0x00;
    this->m_targetDiffuseLightScale = 0.0f;
    this->m_type |= CMapBaseObj::Type_Entity;
}

// VERIFY: 515c7062 manual
// OFFSET: 0x007A1150
int32_t CMapEntity::QueryCameraFog(SMOFog* fog, CMapObjDef** mapObjDef, uint8_t* found, TSGrowableArray<uint32_t>** groups, float* distance) {
    if (!CWorldScene::s_viewerMapObjDef) {
        return 0;
    }

    CMapObj* owner = CWorldScene::s_viewerMapObjDef->m_owner;

    if (!owner || owner->m_fogsCount == 1) {
        return 0;
    }

    C3Vector localPos = CWorldScene::s_activeWorldView * CWorldScene::s_viewerMapObjDef->m_invMat;
    *found = 0;

    for (uint32_t i = 0; i < CWorldScene::s_viewerMapObjGroups.m_count; i++) {
        CMapObjGroup* group = owner->GetGroup(CWorldScene::s_viewerMapObjGroups.m_data[i], 0);

        if (group && !(group->m_flags & 0x48)) {
            *found = 1;
            float portalDistance = owner->DistFromClosestExtPortal(group, localPos, 1);

            if (portalDistance < *distance) {
                *distance = portalDistance;
            }
        }
    }

    if (*found) {
        *mapObjDef = CWorldScene::s_viewerMapObjDef;
        *groups = &CWorldScene::s_viewerMapObjGroups;
    } else {
        *distance = 0.0f;
    }

    *fog = owner->m_fogList[0];

    uint32_t params[2] = { 32, 32 };
    static FogQueue s_fogQueue(params);

    CMapObjGroup* group = owner->GetGroup(CWorldScene::s_viewerMapObjGroups.m_data[0], 0);

    if (group) {
        for (uint32_t i = 0; i < 4; i++) {
            uint8_t fogIndex = group->m_fogs[i];

            if (!fogIndex) {
                continue;
            }

            const SMOFog& groupFog = owner->m_fogList[fogIndex];
            float dx = groupFog.m_position.x - localPos.x;
            float dy = groupFog.m_position.y - localPos.y;
            float dz = groupFog.m_position.z - localPos.z;
            float fogDistance = sqrt(dx * dx + dy * dy + dz * dz);

            if (fogDistance < groupFog.m_largerRadius && !(groupFog.m_flags & 1)) {
                s_fogQueue.Push(fogDistance, fogIndex);
            }
        }
    }

    while (s_fogQueue.m_count > 1) {
        FogQ entry = s_fogQueue.Pop();
        const SMOFog& groupFog = owner->m_fogList[entry.m_fogIndex];
        float clamped = entry.m_distance;

        if (clamped < 0.0f) {
            clamped = 0.0f;
        } else if (groupFog.m_largerRadius <= clamped) {
            clamped = groupFog.m_largerRadius;
        }

        float weight;

        if (clamped >= groupFog.m_smallerRadius) {
            weight = 1.0f - (clamped - groupFog.m_smallerRadius) / (groupFog.m_largerRadius - groupFog.m_smallerRadius);
        } else {
            weight = 1.0f;
        }

        LerpInteriorFog(&fog->m_fog, &groupFog.m_fog, weight);

        if (s_fogQueue.m_count == 1) {
            fog->m_flags = groupFog.m_flags;
        }
    }

    return 1;
}

// VERIFY: 3d0d7fa9 manual
// OFFSET: 0x007C1730
void CMapStaticEntity::SelectLights(CM2Lighting* lighting) {
    if (CWorld::farFog > this->m_distanceToCamera) {
        if (this->m_flags & MAPOBJ_FLAG_UNPLACED) {
            this->m_flags = static_cast<MapObjFlag>(this->m_flags & ~MAPOBJ_FLAG_UNPLACED);
        }

        C3Vector diffuse = { 0.0f, 0.0f, 0.0f };
        C3Vector dir = { 0.0f, 0.0f, 0.0f };
        C3Vector ambient = { 0.0f, 0.0f, 0.0f };

        if ((this->m_flags & 4) != 0) {
            CM2Light* light = &CMap::s_mapLight->m_light;

            diffuse.x = light->m_dirColor.x * this->m_diffuseLightScale;
            diffuse.y = light->m_dirColor.y * this->m_diffuseLightScale;
            diffuse.z = light->m_dirColor.z * this->m_diffuseLightScale;

            dir = light->m_dir;

            ambient = C3Vector(this->m_m2AmbientColor);
        } else {
            diffuse = C3Vector(this->m_m2DiffuseColor);
            diffuse.x *= this->m_diffuseLightScale;
            diffuse.y *= this->m_diffuseLightScale;
            diffuse.z *= this->m_diffuseLightScale;

            ambient = C3Vector(this->m_m2AmbientColor);

            dir = CMapStaticEntity::s_interiorSunDir;

            if ((this->m_staticFlags & 0x1000) != 0) {
                DayNight::DNInfo* dayNight = DayNight::GetInfo();
                float blend = this->m_m2DiffuseColor.a / 255.0f;

                dir.x += (dayNight->m_light1.m_dir.x - dir.x) * blend;
                dir.y += (dayNight->m_light1.m_dir.y - dir.y) * blend;
                dir.z += (dayNight->m_light1.m_dir.z - dir.z) * blend;

                dir.Normalize();
            }
        }

        if ((this->m_staticFlags & 0x8000) != 0) {
            ambient.x *= CMapStaticEntity::s_characterAmbient;
            ambient.y *= CMapStaticEntity::s_characterAmbient;
            ambient.z *= CMapStaticEntity::s_characterAmbient;

            if (ambient.x >= 1.0f) {
                ambient.x = 1.0f;
            }

            if (ambient.y >= 1.0f) {
                ambient.y = 1.0f;
            }

            if (ambient.z >= 1.0f) {
                ambient.z = 1.0f;
            }
        }

        lighting->AddAmbient(ambient);
        lighting->AddDiffuse(diffuse, dir);
    }

    DayNight::DNInfo* dayNight = DayNight::GetInfo();

    if ((this->m_flags & 0x8000) != 0) {
        lighting->SetFog(C3Vector(dayNight->m_fogInterior.m_color), dayNight->m_fogInterior.m_start, dayNight->m_fogInterior.m_end, dayNight->m_fogInterior.m_density);
    } else {
        lighting->SetFog(C3Vector(dayNight->m_fog.m_color), dayNight->m_fog.m_start, dayNight->m_fog.m_end, dayNight->m_fog.m_density);
    }
}

// VERIFY: feea4d8f manual
// OFFSET: 0x0097E080
FogQ FogQueue::Pop() {
    FogQ* data = reinterpret_cast<FogQ*>(this->m_data);
    FogQ top = data[1];
    FogQ last = data[this->m_count - 1];

    if (this->m_count) {
        this->Erase(this->m_count - 1, 1);
    }

    if (this->m_count < 2) {
        return top;
    }

    uint32_t limit = this->m_count - 1;
    uint32_t half = limit >> 1;
    uint32_t index = 1;

    if (half) {
        while (1) {
            uint32_t child = 2 * index;
            data = reinterpret_cast<FogQ*>(this->m_data);

            if (child < limit && data[child].m_distance <= data[child + 1].m_distance) {
                child++;
            }

            if (last.m_distance >= data[child].m_distance) {
                break;
            }

            data[index] = data[child];
            index = child;

            if (child > half) {
                break;
            }
        }
    }

    reinterpret_cast<FogQ*>(this->m_data)[index] = last;

    return top;
}

// VERIFY: 5f34315e manual
// OFFSET: 0x007A1B10
void CMapEntity::QueryLiquidSounds(TSGrowableArray<World::LiquidQueryResult>& results, uint32_t* distance) {
    if (!(this->m_staticFlags & 0x1)) {
        return;
    }

    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (!(ref->m_type & CMapBaseObj::Type_MapObjDefGroup)) {
            continue;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        C3Vector position = this->m_position * mapObjDef->m_invMat;
        mapObj->QueryLiquidSounds(mapObjDefGroup->m_groupNum, mapObjDefGroup->m_groupNum, 0, distance, position, results);
    }
}

// VERIFY: 254daf94 manual
// OFFSET: 0x007A1E90
void CMapEntity::Tick() {
    int32_t step = static_cast<int32_t>((FrameTime::s_tickTimeSec + FrameTime::s_tickTimeSec) * 255.0f);

    if (step < 1) {
        step = 1;
    }

    int32_t r = this->m_m2AmbientColor.r;
    int32_t g = this->m_m2AmbientColor.g;
    int32_t b = this->m_m2AmbientColor.b;
    int32_t targetR = this->m_targetAmbientColor.r;
    int32_t targetG = this->m_targetAmbientColor.g;
    int32_t targetB = this->m_targetAmbientColor.b;
    int32_t changed = 0;

    if (targetR - r) {
        changed = 1;

        if (targetR - r <= 0) {
            r -= step;

            if (r < targetR) {
                r = targetR;
            }
        } else {
            r += step;

            if (r > targetR) {
                r = targetR;
            }
        }
    }

    if (targetG - g) {
        changed = 1;

        if (targetG - g <= 0) {
            g -= step;

            if (g < targetG) {
                g = targetG;
            }
        } else {
            g += step;

            if (g > targetG) {
                g = targetG;
            }
        }
    }

    if (targetB - b) {
        changed = 1;

        if (targetB - b <= 0) {
            b -= step;

            if (b < targetB) {
                b = targetB;
            }
        } else {
            b += step;

            if (b > targetB) {
                b = targetB;
            }
        }
    }

    this->m_m2AmbientColor.r = static_cast<uint8_t>(r);
    this->m_m2AmbientColor.g = static_cast<uint8_t>(g);
    this->m_m2AmbientColor.b = static_cast<uint8_t>(b);

    if (!(this->m_staticFlags & 0x1) && !changed) {
        CImVector ambient = DayNight::GetInfo()->m_light1.m_ambient;
        this->m_m2AmbientColor = ambient;
        this->m_targetAmbientColor = ambient;
    }

    float rate = FrameTime::s_tickTimeSec * dirLightScaleRate;
    float delta = this->m_diffuseLightScale - this->m_targetDiffuseLightScale;

    if (0.0f != delta) {
        if (delta >= 0.0f) {
            this->m_diffuseLightScale = this->m_diffuseLightScale - rate;

            if (this->m_diffuseLightScale < this->m_targetDiffuseLightScale) {
                this->m_diffuseLightScale = this->m_targetDiffuseLightScale;
            }
        } else {
            this->m_diffuseLightScale = rate + this->m_diffuseLightScale;

            if (this->m_diffuseLightScale > this->m_targetDiffuseLightScale) {
                this->m_diffuseLightScale = this->m_targetDiffuseLightScale;
            }
        }
    }

    if (this->m_diffuseLightScale > 1.0f) {
        this->m_diffuseLightScale = 1.0f;
    }

    if (!this->m_parentLinkList.Head()) {
        CMap::UpdateEntity(this);
    }
}

// VERIFY: 6acda3f1 manual
// OFFSET: 0x007A13E0
bool CMapStaticEntity::GetMapObjAndGroup(CMapObjDef** mapObjDef, CMapObj** mapObj, CMapObjDefGroup** mapObjDefGroup, CMapObjGroup** mapObjGroup, int32_t skipHidden) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (!(ref->m_type & CMapBaseObj::Type_MapObjDefGroup)) {
            continue;
        }

        *mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* def = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        *mapObjDef = def;

        if (skipHidden && (def->m_flags & 0x400)) {
            continue;
        }

        *mapObj = def->m_owner;
        *mapObjGroup = def->m_owner->GetGroup((*mapObjDefGroup)->m_groupNum, 0);

        if (*mapObjGroup) {
            return true;
        }
    }

    return false;
}

// VERIFY: d211d73c auto
// OFFSET: 0x007A1A30
bool CMapEntity::UpdateMapObjLiquid() {
    this->m_staticFlags &= ~0x60;
    CMapObjDef* mapObjDef;
    CMapObj* mapObj;
    CMapObjDefGroup* mapObjDefGroup;
    CMapObjGroup* mapObjGroup;

    if (!this->GetMapObjAndGroup(&mapObjDef, &mapObj, &mapObjDefGroup, &mapObjGroup, 0)) {
        return false;
    }

    C3Vector local = C3Vector(this->m_position.x, this->m_position.y, this->m_bbox.b.z) * mapObjDef->m_invMat;
    int32_t liquidType;
    float level;

    if (!mapObjGroup->QueryLiquid(local, &liquidType, &level)) {
        return false;
    }

    this->m_staticFlags |= 0x20;
    C3Vector world = C3Vector(local.x, local.y, level) * mapObjDef->m_mat;
    this->m_liquidHeight = world.z;

    if (world.z <= this->m_bbox.t.z) {
        this->m_staticFlags |= 0x40;
    } else {
        this->m_staticFlags &= ~0x40;
    }

    this->m_liquidType = static_cast<uint16_t>(liquidType);
    return true;
}

// VERIFY: fb6c1e22 manual
// OFFSET: 0x007A0D60
bool CMapEntity::QueryInteriorLighting(CMapObjDef* mapObjDef, int32_t groupNum, uint16_t* faceIndex, C3Vector* pos) {
    CImVector color;
    *reinterpret_cast<uint32_t*>(&color) = 0;
    uint8_t flag;
    bool result;

    if (faceIndex) {
        C3Vector local = *pos * mapObjDef->m_invMat;
        result = mapObjDef->m_owner->QueryLighting(groupNum, local, *faceIndex, &color, &flag);
    } else {
        C3Vector local = this->m_position * mapObjDef->m_invMat;
        C3Segment seg;
        seg.b = C3Vector(local.x, local.y, local.z + 1.0f);
        seg.t = C3Vector(local.x, local.y, local.z - 12.0f);
        result = mapObjDef->m_owner->QueryLighting(groupNum, seg, &color, &flag);
    }

    if (!result) {
        return result;
    }

    CImVector limit;
    *reinterpret_cast<uint32_t*>(&limit) = MIN_INTERIOR_DIRCOLOR;
    this->AdjustLightmap(&color, &this->m_m2DiffuseColor, limit, &this->m_targetAmbientColor, MAX_AMBIENT);

    if (!flag) {
        this->m_staticFlags &= ~0x1000;
        return result;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();
    uint8_t alpha = color.a;

    if (alpha) {
        this->m_m2DiffuseColor.InterpColor(alpha, &info->m_light1.m_diffuse);
        this->m_targetAmbientColor.InterpColor(alpha, &info->m_light1.m_ambient);
    }

    this->m_staticFlags |= 0x1000;
    this->m_m2DiffuseColor.a = color.a;
    return result;
}

// VERIFY: 8dca5fb5 auto
// OFFSET: 0x007A1500
int32_t CMapEntity::QueryMapObjZoneName(const char** zoneName) {
    CMapObjDef* mapObjDef;
    CMapObj* mapObj;
    CMapObjDefGroup* mapObjDefGroup;
    CMapObjGroup* mapObjGroup;

    if (!this->GetMapObjAndGroup(&mapObjDef, &mapObj, &mapObjDefGroup, &mapObjGroup, 1)) {
        return 0;
    }

    *zoneName = "";
    WMOAreaTableRec* rec = GetWMOAreaRec(mapObjDef->m_owner->m_header->m_wmoID, mapObjDef->m_nameSet, -1);

    if (rec) {
        const char* name = rec->m_areaName;

        if (!*name) {
            int32_t areaId = rec->m_areaTableID;

            if (!areaId) {
                areaId = CMap::QueryAreaIdTerrain(&this->m_position);

                if (!areaId) {
                    return 1;
                }
            }

            AreaTableRec* area = g_areaTableDB.GetRecord(areaId);

            if (!area) {
                return 1;
            }

            name = area->m_areaName;
        }

        *zoneName = name;
    }

    return 1;
}

// VERIFY: 384f6f8b auto
// OFFSET: 0x007A15B0
int32_t CMapEntity::QueryMapObjSubzoneName(const char** subzoneName, int32_t* areaId) {
    CMapObjDef* mapObjDef;
    CMapObj* mapObj;
    CMapObjDefGroup* mapObjDefGroup;
    CMapObjGroup* mapObjGroup;

    if (!this->GetMapObjAndGroup(&mapObjDef, &mapObj, &mapObjDefGroup, &mapObjGroup, 1)) {
        return 0;
    }

    if (!mapObjDefGroup->m_subzoneName) {
        WMOAreaTableRec* rec = GetWMOAreaRec(mapObj->m_header->m_wmoID, mapObjDef->m_nameSet, mapObjGroup->m_wmoGroupId);
        mapObjDefGroup->m_subzoneName = rec ? rec->m_areaName : "";
    }

    *subzoneName = mapObjDefGroup->m_subzoneName;

    if (mapObjDefGroup->m_groupNum) {
        *areaId = mapObjDefGroup->m_groupNum;
    }

    return 1;
}

// VERIFY: feac96db manual
// OFFSET: 0x007A1640
bool CMapEntity::QueryMapObjAreaTable(WMOAreaTableRec** groupRec, WMOAreaTableRec** rootRec, int32_t* wmoGroupId) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            break;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        CMapObjGroup* group = mapObj->GetGroup(mapObjDefGroup->m_groupNum, 0);

        if (group && (!(group->m_flags & 0x8) || !(mapObjDef->m_flags & 0x400))) {
            *wmoGroupId = group->m_wmoGroupId;
            *groupRec = GetWMOAreaRec(mapObj->m_header->m_wmoID, mapObjDef->m_nameSet, group->m_wmoGroupId);
            WMOAreaTableRec* rec = GetWMOAreaRec(mapObj->m_header->m_wmoID, mapObjDef->m_nameSet, -1);
            *rootRec = rec;
            return *groupRec && rec;
        }
    }

    return false;
}

// VERIFY: 0b2f6b2e manual
// OFFSET: 0x007A18D0
bool CMapEntity::QueryMapObjIDs(int32_t* wmoId, int32_t* wmoHash, int32_t* groupNum) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            break;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        uint8_t groupFlags = static_cast<uint8_t>(mapObj->GetGroupFlags(mapObjDefGroup->m_groupNum));

        if (!(mapObjDef->m_flags & 0x400) || !(groupFlags & 0x8)) {
            *wmoId = mapObj->m_header->m_wmoID;
            *wmoHash = mapObjDef->m_owner->m_hashval;
            *groupNum = mapObjDefGroup->m_groupNum;
            return true;
        }
    }

    return false;
}

// VERIFY: 2b02ba04 manual
// OFFSET: 0x007A1480
int32_t CMapEntity::QueryInterior() {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            break;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObjGroup* group = mapObjDef->m_owner->GetGroup(mapObjDefGroup->m_groupNum, 0);

        if (group) {
            if (!(group->m_flags & 0x8)) {
                return 1;
            }

            if (!(mapObjDef->m_flags & 0x400)) {
                break;
            }
        }
    }

    return 0;
}

// VERIFY: 4d2de931 manual
// OFFSET: 0x007A1730
int32_t CMapEntity::QueryMapObjFileName(const char** fileName) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            *fileName = nullptr;
            return 0;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        CMapObjGroup* group = mapObj->GetGroup(mapObjDefGroup->m_groupNum, 0);

        if (group && (!(group->m_flags & 0x8) || !(mapObjDef->m_flags & 0x400))) {
            *fileName = mapObj->m_wmoName;
            return 1;
        }
    }

    return 0;
}

// VERIFY: c065df6f manual
// OFFSET: 0x007A17E0
bool CMapEntity::QueryMapObjMinimap(CAaBox* box, TSStackArray<MINIMAPQUADINFO>* quads, uint32_t maxQuads, int32_t inside, uint8_t indoorArea) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            return false;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);
        CMapObj* mapObj = mapObjDef->m_owner;
        uint8_t groupFlags = static_cast<uint8_t>(mapObj->GetGroupFlags(mapObjDefGroup->m_groupNum));

        if (!(mapObjDef->m_flags & 0x400) || !(groupFlags & 0x8)) {
            if (!(groupFlags & 0x48)) {
                indoorArea = 0;
            }

            CAaBox local;
            local.b = C3Vector(0.0f, 0.0f, 0.0f);
            local.t = C3Vector(0.0f, 0.0f, 0.0f);
            CWorldMath::TransformAABox(mapObjDef->m_invMat, *box, local);
            return mapObj->QueryMapObjMinimap(mapObjDefGroup->m_groupNum, &local, quads, maxQuads, inside, indoorArea);
        }
    }

    return false;
}

// VERIFY: b0f36f16 manual
// OFFSET: 0x007A1980
bool CMapEntity::QueryMapObjMatrix(C44Matrix* matrix, C44Matrix* invMatrix) {
    for (CMapBaseObjLink* link = this->m_parentLinkList.Head(); link; link = this->m_parentLinkList.Next(link)) {
        CMapBaseObj* ref = link->m_ref;

        if (ref->m_type & CMapBaseObj::Type_Chunk) {
            return false;
        }

        CMapObjDefGroup* mapObjDefGroup = static_cast<CMapObjDefGroup*>(ref);
        CMapObjDef* mapObjDef = static_cast<CMapObjDef*>(ref->m_parentLinkList.Head()->m_ref);

        if (!(mapObjDef->m_flags & 0x400)) {
            if (matrix) {
                *matrix = mapObjDef->m_mat;
            }

            if (invMatrix) {
                *invMatrix = mapObjDef->m_invMat;
            }

            return true;
        }

        CMapObjGroup* group = mapObjDef->m_owner->GetGroup(mapObjDefGroup->m_groupNum, 0);

        if (group && !(group->m_flags & 0x8)) {
            if (matrix) {
                *matrix = mapObjDef->m_mat;
            }

            if (invMatrix) {
                *invMatrix = mapObjDef->m_invMat;
            }

            return true;
        }
    }

    return false;
}

// VERIFY: 0284b305 manual
// OFFSET: [0x007A10A0, 0x007A1120]
CMapEntity::~CMapEntity() {
}
