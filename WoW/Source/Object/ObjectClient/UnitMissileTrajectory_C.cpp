#include "UnitMissileTrajectory_C.h"
#include <new>
#include "Engine/Source/Base/ObjectAlloc.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/WorldClient/World.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellVisualRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/WorldClient/Map.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include <malloc.h>
#include <cmath>
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxMatrixStack.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Tempest/CRandom.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/DB/Rec/SpellMissileRec.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/ObjectClient/Missile_C.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/Net/NetClient/NetClient.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/Ui/PetInfo.h"
#include "Engine/Source/ShaderEffect/Types.h"

struct MissileUnitRank {
    CGUnit_C* m_unit;
    float m_rank;
};

// STATIC: 0x00CA0A98
CMapEntity* CGUnit_C::s_trajectoryWorldObjects[2];

// STATIC: 0x00CA0AA0
CM2Model* CGUnit_C::s_trajectoryModels[2];

// STATIC: 0x00CA0AA8
static WOWGUID s_trajectoryTargetGuid;

// STATIC: 0x00CA0AB8
static float s_trajectoryAlpha;

// STATIC: 0x00CA0ABC
static uint32_t s_trajectoryStartTime;

// STATIC: 0x00CA0AC0
static uint32_t s_trajectoryFadeStart;

// STATIC: 0x00CA0AC4
static int32_t s_trajectoryMode;

// STATIC: 0x00CA0AC8
C3Vector* CGUnit_C::s_trajectoryPoints;

// STATIC: 0x00CA0ACC
static uint32_t s_trajectoryPointCount;

// STATIC: 0x00CA0AD0
static CTexture* s_trajectoryTexture0;

// STATIC: 0x00CA0AD4
static CTexture* s_trajectoryTexture1;

// STATIC: 0x00CA0AEC
static C3Vector s_trajectoryLaunchPosition;

// STATIC: 0x00CA0698
static MissileUnitRank s_unitRanks[128];

// STATIC: 0x00CA0AE4
static int32_t s_unitRankCount;

// STATIC: 0x00CA0AE8
static CGUnit_C* s_unitRankExclude;

// STATIC: 0x00CA0AD8
CMissileCollisionHeap* CMissileCollision::s_pool;

// STATIC: 0x00CA0ADC
CMissileCollision* CMissileCollision::s_head;

// STATIC: 0x00CA0AE0
CMissileCollision* CMissileCollision::s_tail;

// INLINED
inline CMissileCollision::CMissileCollision() {
    this->m_start.x = 0.0f;
    this->m_start.y = 0.0f;
    this->m_end.x = 0.0f;
    this->m_end.y = 0.0f;
    this->m_dir.x = 0.0f;
    this->m_dir.y = 0.0f;
    this->m_perp.x = 0.0f;
    this->m_perp.y = 0.0f;
}

// VERIFY: dba3ae87 manual
// OFFSET: 0x006FC0F0
void CMissileCollision::RemoveUnitFromSystem(CGUnit_C* unit) {
    for (CMissileCollision* collision = CMissileCollision::s_head; collision; collision = collision->m_next) {
        uint32_t i = 0;

        while (collision->m_units[i]) {
            if (collision->m_units[i] == unit) {
                do {
                    collision->m_units[i] = collision->m_units[i + 1];
                    i++;
                } while (collision->m_units[i]);

                break;
            }

            i++;
        }
    }
}

// INLINED
inline void CMissileCollision::InitSystem() {
    delete CMissileCollision::s_pool;

    CMissileCollisionHeap* pool = static_cast<CMissileCollisionHeap*>(SMemAlloc(sizeof(CMissileCollisionHeap), __FILE__, __LINE__, 0));

    if (pool) {
        pool->m_heapId = ObjectAllocAddHeap(sizeof(CMissileCollision), 8, "Missile Collision", true);
        CMissileCollision::s_tail = nullptr;
        CMissileCollision::s_head = nullptr;
        CMissileCollision::s_pool = pool;
    } else {
        CMissileCollision::s_pool = nullptr;
        CMissileCollision::s_tail = nullptr;
        CMissileCollision::s_head = nullptr;
    }
}

// VERIFY: 9a79f0ab manual
// OFFSET: 0x006FC770
CMissileCollision* CMissileCollisionHeap::New() {
    uint32_t memHandle;
    void* memory = nullptr;

    if (!ObjectAlloc(this->m_heapId, &memHandle, &memory, false)) {
        return nullptr;
    }

    CMissileCollision* collision = memory ? new (memory) CMissileCollision() : nullptr;
    collision->m_memHandle = memHandle;

    return collision;
}

// VERIFY: 4b422f42 manual
// OFFSET: 0x006FC8E0
void CGUnit_C::InitMissileTrajectorySystem() {
    C3Vector* points = static_cast<C3Vector*>(SMemAlloc(2400, __FILE__, __LINE__, 0));
    CGUnit_C::s_trajectoryModels[0] = nullptr;
    CGUnit_C::s_trajectoryWorldObjects[0] = nullptr;
    CGUnit_C::s_trajectoryModels[1] = nullptr;
    CGUnit_C::s_trajectoryWorldObjects[1] = nullptr;
    CGUnit_C::s_trajectoryPoints = points;
    CMissileCollision::InitSystem();
}

// VERIFY: 23f03795 auto
// OFFSET: 0x006FC950
void CMissileCollision::Free() {
    if (this == CMissileCollision::s_head) {
        CMissileCollision::s_head = this->m_next;

        if (CMissileCollision::s_head) {
            CMissileCollision::s_head->m_prev = nullptr;
        }
    } else if (this == CMissileCollision::s_tail) {
        CMissileCollision::s_tail = this->m_prev;

        if (this->m_prev) {
            this->m_prev->m_next = nullptr;
        }
    } else {
        this->m_prev->m_next = this->m_next;
        this->m_next->m_prev = this->m_prev;
    }

    ObjectFree(CMissileCollision::s_pool->m_heapId, this->m_memHandle);
}

// VERIFY: 0abf6409 auto
// OFFSET: 0x006FC920
CMissileCollision* CMissileCollision::Alloc() {
    CMissileCollision* collision = CMissileCollision::s_pool->New();
    CMissileCollision* head = CMissileCollision::s_head;
    collision->m_prev = nullptr;
    CMissileCollision::s_head = collision;

    if (head) {
        collision->m_next = head;
        head->m_prev = collision;
    } else {
        collision->m_next = nullptr;
        CMissileCollision::s_tail = collision;
    }

    return collision;
}

// VERIFY: 3780b5c1 auto
// OFFSET: 0x006FCA40
void CMissileCollision::MaybeAddUnitToSystem(CGUnit_C* unit) {
    for (CMissileCollision* collision = CMissileCollision::s_head; collision; collision = collision->m_next) {
        if (collision->ComputeUnitRank(unit) == -1.0f) {
            continue;
        }

        int32_t i = 0;

        while (collision->m_units[i]) {
            i++;
        }

        if (i < 15) {
            collision->m_units[i] = unit;
            collision->m_units[i + 1] = nullptr;
        }
    }
}

// VERIFY: d983a829 manual
// OFFSET: 0x006FBF00
void CGUnit_C::StopMissileTrajectoryTargeting() {
    if (s_trajectoryTexture0) {
        HandleClose(reinterpret_cast<HOBJECT>(s_trajectoryTexture0));
        s_trajectoryTexture0 = nullptr;
    }

    if (s_trajectoryTexture1) {
        HandleClose(reinterpret_cast<HOBJECT>(s_trajectoryTexture1));
        s_trajectoryTexture1 = nullptr;
    }

    for (uint32_t i = 0; i < 2; i++) {
        if (CGUnit_C::s_trajectoryModels[i]) {
            CGUnit_C::s_trajectoryModels[i]->Release();
            CGUnit_C::s_trajectoryModels[i] = nullptr;
        }

        if (CGUnit_C::s_trajectoryWorldObjects[i]) {
            World::ObjectDestroy(CGUnit_C::s_trajectoryWorldObjects[i]);
            CGUnit_C::s_trajectoryWorldObjects[i] = nullptr;
        }
    }

    s_trajectoryTargetGuid = WOWGUID();
}

// VERIFY: d51d83e9 auto
// OFFSET: 0x006FE7E0
void CGUnit_C::UpdateMissileTrajectory() {
    if (!this->m_missileTrajectoryCastId || !this->m_missileTrajectoryStart || this->m_missileTrajectoryReleased) {
        return;
    }

    int32_t spellId = this->m_missileTrajectorySpell;

    if (static_cast<int32_t>(FrameTime::s_curTimeMs - this->m_missileTrajectoryEnd - this->m_missileTrajectoryCastId) < 0) {
        return;
    }

    SpellRec spell;

    if (!g_spellDB.GetRecord(spellId, &spell)) {
        return;
    }

    SpellVisualRec visualRec;
    const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);
    SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual ? visual->m_castKit : 0);
    WOWGUID guid = this->m_obj->m_guid;
    PendingSpellCast* cast = GetPendingSpellCast(this->m_castCount, this->m_castSpellId, guid);

    if (cast && !cast->m_isTrajectoryCast && visual && visual->m_hasMissile && kit && kit->m_animID > 0 && !(visual->m_flags & 0x4)) {
        cast->m_data.m_targetFlags |= 0x40;
        cast->m_data.m_elevation = 0.0f;
        cast->m_data.m_destPos = C3Vector(0.0f, 0.0f, 0.0f);
        cast->m_data.m_missileSpeed = 1.0f;
        TSStackArray<CGUnit_C*> targets(static_cast<CGUnit_C**>(alloca(0)), 0, 0);
        Spell_C_PlayMissileTrajectoryCastVisuals(this, cast, &spell, visual, kit, &targets, 1.0f);
        this->m_missileTrajectoryReleased = cast->m_data.m_castCount;
        cast->m_isTrajectoryCast = 1;
    } else {
        this->SendFinalMissileTrajectoryUpdate();
    }
}

// VERIFY: b5322c54 manual
// OFFSET: 0x006FDA20
void CGUnit_C::UpdateMissileTrajectoryTargeting() {
    s_trajectoryAlpha = 0.0f;
    uint32_t pitchNextFrame = CGUnit_C::s_pitchMissileTrajectoryTargetingNextFrame;
    CGUnit_C::s_pitchMissileTrajectoryTargetingNextFrame = 0;

    if (!s_trajectoryTargetGuid) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_trajectoryTargetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        s_trajectoryTargetGuid = WOWGUID();
        return;
    }

    VehicleRec* vehicleRec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (!vehicleRec || !(vehicleRec->m_flags & 0xF800000)) {
        return;
    }

    int32_t spellId = 0;
    bool hasMissile = false;
    SpellRec spell;
    int32_t castSpell;

    if (unit->m_castSpellId) {
        castSpell = unit->m_castSpellId;
    } else if (Spell_C_GetDelayedTrajectoryCast()) {
        castSpell = Spell_C_GetDelayedTrajectoryCast()->m_data.m_spellId;
    } else {
        castSpell = 0;
    }

    if (g_spellDB.GetRecord(castSpell, &spell) && spell.m_spellMissileID && g_spellMissileDB.GetRecord(spell.m_spellMissileID)) {
        hasMissile = true;
        spellId = castSpell;
    }

    CGInputControl* input = CGInputControl::GetActive();

    if (!input) {
        return;
    }

    uint32_t flags = vehicleRec->m_flags;
    int32_t mode;
    bool active;

    if (flags & 0x800000) {
        mode = 0;
        active = true;
        s_trajectoryMode = 0;
    } else if ((flags & 0x1000000) && input->IsMouseDragging()) {
        mode = 1;
        active = true;
        s_trajectoryMode = 1;
    } else if ((flags & 0x2000000) && (pitchNextFrame || (input->m_flags & INPUT_PITCH_PLAYER_MASK))) {
        mode = 2;
        active = true;
        s_trajectoryMode = 2;
    } else if ((flags & 0x4000000) && input->CanSyncFreeLookFacing(unit)) {
        mode = 3;
        active = true;
        s_trajectoryMode = 3;
    } else if ((vehicleRec->m_flags & 0x8000000) && hasMissile) {
        mode = 0;
        active = true;
        s_trajectoryMode = 0;
    } else {
        mode = s_trajectoryMode;
        active = false;
    }

    bool fullAlpha = true;
    int32_t pitchArg = static_cast<int32_t>(pitchNextFrame);

    if (active) {
        s_trajectoryFadeStart = 0;
    } else {
        if (!mode) {
            s_trajectoryFadeStart = 0;
            s_trajectoryMode = 0;
            return;
        }

        if (!s_trajectoryFadeStart) {
            if (static_cast<int32_t>(CGUnit_C::s_missileTrajectoryTargetingSpell) <= 0) {
                s_trajectoryFadeStart = 0;
                s_trajectoryMode = 0;
                return;
            }

            s_trajectoryFadeStart = 1;

            if (FrameTime::s_curTimeMs) {
                s_trajectoryFadeStart = FrameTime::s_curTimeMs;
            }
        } else {
            float linger;

            if (mode == 1) {
                linger = vehicleRec->m_msslTrgtTurnLingering;
            } else if (mode == 2) {
                linger = vehicleRec->m_msslTrgtPitchLingering;
            } else {
                linger = vehicleRec->m_msslTrgtMouseLingering;
            }

            float fade = 0.5f * linger;

            if (fade > 0.3f) {
                fade = 0.3f;
            }

            int32_t hold = CMath::fint_((linger - fade) * 1000.0f);
            pitchArg = hold;
            uint32_t fadeMs = static_cast<uint32_t>(CMath::fint_(fade * 1000.0f));
            uint32_t elapsed = FrameTime::s_curTimeMs - (s_trajectoryFadeStart + hold);

            if (static_cast<int32_t>(elapsed) >= 0) {
                if (!fadeMs) {
                    s_trajectoryAlpha = 0.0f;
                    s_trajectoryFadeStart = 0;
                    s_trajectoryMode = 0;
                    return;
                }

                s_trajectoryAlpha = 1.0f - static_cast<float>(elapsed) / static_cast<float>(fadeMs);

                if (s_trajectoryAlpha <= 0.0f) {
                    s_trajectoryFadeStart = 0;
                    s_trajectoryMode = 0;
                    return;
                }

                fullAlpha = false;
            }
        }
    }

    if (fullAlpha) {
        s_trajectoryAlpha = 1.0f;
    }

    spell.m_id = 0;
    spell.m_spellMissileID = 0;

    if (!spellId) {
        spellId = CGUnit_C::s_missileTrajectoryTargetingSpell;
    }

    if (spellId <= 0 || (g_spellDB.GetRecord(spellId, &spell), !spell.m_id)) {
        for (uint32_t i = 0; i < 10; i++) {
            PetAction* action = GetPetAction(i);

            if (!action) {
                continue;
            }

            uint32_t type = (action->m_data >> 24) & 0x3F;

            if (type != 1 && (type <= 7 || type > 17)) {
                continue;
            }

            spellId = action->m_data & 0xFFFFFF;

            if (g_spellDB.GetRecord(spellId, &spell) && spell.m_spellMissileID) {
                break;
            }
        }

        if (!spell.m_id) {
            s_trajectoryAlpha = 0.0f;
            return;
        }
    }

    C3Vector position(0.0f, 0.0f, 0.0f);
    s_trajectoryPointCount = 0;
    float pitch;
    float speed;
    float facing;

    if (!unit->ComputeMissileTrajectory(&spell, &pitch, &speed, &s_trajectoryLaunchPosition, &position, &facing, CGUnit_C::s_trajectoryPoints, &s_trajectoryPointCount, 1)) {
        s_trajectoryAlpha = 0.0f;
        return;
    }

    facing = unit->GetFacing();

    for (uint32_t i = 0; i < 2; i++) {
        CM2Model* model = CGUnit_C::s_trajectoryModels[i];

        if (!model) {
            continue;
        }

        CAaSphere sphere = model->GetBoundingSphere();
        float scale = sphere.r <= 0.0f ? vehicleRec->m_msslTrgtImpactRadius[i] : vehicleRec->m_msslTrgtImpactRadius[i] / sphere.r;
        CGUnit_C::s_trajectoryModels[i]->SetWorldTransform(position, facing, scale);
        CGUnit_C::s_trajectoryModels[i]->m_fadeAlpha = s_trajectoryAlpha;
        CGUnit_C::s_trajectoryModels[i]->ForceAnimate();

        if (!CGUnit_C::s_trajectoryWorldObjects[i]) {
            C44Matrix transform = CGUnit_C::s_trajectoryModels[i]->m_worldTransform;
            CGUnit_C::s_trajectoryWorldObjects[i] = CMap::ObjectCreateStaticEx(CGUnit_C::s_trajectoryModels[i], transform, MissileTrajectoryObjectCallback, nullptr, 0, i, __FILE__, __LINE__);
        }

        if (CGUnit_C::s_trajectoryModels[i]->IsLoaded(0, 0)) {
            CAaBox box = CGUnit_C::s_trajectoryModels[i]->GetBoundingBox();
            C44Matrix transform = CGUnit_C::s_trajectoryModels[i]->m_worldTransform;
            CMap::ObjectUpdate(CGUnit_C::s_trajectoryWorldObjects[i], transform, box, sphere, sphere.c, false, 0xFFFFFFFF);
        }
    }

    CGUnit_C::s_missileTrajectoryTargetingSpell = spellId;
}

// OFFSET: 0x006FC190
void CGUnit_C::ShutdownMissileTrajectorySystem() {
    SMemFree(CGUnit_C::s_trajectoryPoints, __FILE__, __LINE__, 0);
    CGUnit_C::StopMissileTrajectoryTargeting();
    delete CMissileCollision::s_pool;
    CMissileCollision::s_pool = nullptr;
    CMissileCollision::s_tail = nullptr;
    CMissileCollision::s_head = nullptr;
}

// VERIFY: 2d5a7665 manual
// OFFSET: 0x006FDFB0
void CGUnit_C::RenderMissileTrajectoryTargetingTexture() {
    if (!(s_trajectoryAlpha > 0.0f) || !s_trajectoryTargetGuid) {
        return;
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(s_trajectoryTargetGuid, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        s_trajectoryTargetGuid = WOWGUID();
        return;
    }

    float facing = unit->GetFacing();
    VehicleRec* vehicle = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (!vehicle) {
        return;
    }

    CGxTex* arcTex;

    if (s_trajectoryTexture0 && (arcTex = TextureGetGxTex(s_trajectoryTexture0, 0, nullptr)) != nullptr) {
        g_theGxDevicePtr->RsPush();
        GxXformPush(GxXform_World);
        GxXformPush(GxXform_Tex0);
        GxXformTranslate(GxXform_World, -CGWorldFrame::GetActiveCamera()->m_position);
        GxRsSet(GxRs_Culling, 0);
        GxRsSet(GxRs_Fog, 0);
        GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
        GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
        GxRsSet(GxRs_ColorWrite, 15);
        GxRsSet(GxRs_Lighting, 0);
        GxRsSet(GxRs_DepthWrite, 0);
        GxRsSet(GxRs_BlendingMode, 3);
        int32_t vertexCount = 2 * s_trajectoryPointCount + 2;
        C3Vector* positions = static_cast<C3Vector*>(alloca(sizeof(C3Vector) * vertexCount));
        CImVector* colors = static_cast<CImVector*>(alloca(sizeof(CImVector) * vertexCount));
        C2Vector* texCoords = static_cast<C2Vector*>(alloca(sizeof(C2Vector) * vertexCount));
        uint16_t* indices = static_cast<uint16_t*>(alloca(sizeof(uint16_t) * vertexCount));

        for (int32_t i = 0; i < vertexCount; i++) {
            indices[i] = i;
        }

        float halfWidth = vehicle->m_msslTrgtArcWidth * 0.5f;
        C3Vector side;
        side.x = -(CMath::sin_(facing) * halfWidth);
        side.y = CMath::cos_(facing) * halfWidth;
        uint32_t elapsed = FrameTime::s_curTimeMs - s_trajectoryStartTime;
        float alpha = s_trajectoryAlpha * 255.0f;
        float alphaStep = (255.0f * (s_trajectoryAlpha * vehicle->m_msslTrgtEndOpacity) - alpha) * (1.0f / s_trajectoryPointCount);
        float texStart = 1.0f - fmodf(static_cast<float>(elapsed) * 0.001f * vehicle->m_msslTrgtArcSpeed, 1.0f);
        positions[0].x = s_trajectoryLaunchPosition.x + side.x;
        positions[0].y = s_trajectoryLaunchPosition.y + side.y;
        positions[0].z = s_trajectoryLaunchPosition.z;
        positions[1].x = s_trajectoryLaunchPosition.x - side.x;
        positions[1].y = s_trajectoryLaunchPosition.y - side.y;
        positions[1].z = s_trajectoryLaunchPosition.z;
        colors[0].a = 0;
        colors[0].r = 0xFF;
        colors[0].g = 0xFF;
        colors[0].b = 0xFF;
        colors[1] = colors[0];
        texCoords[0].x = texStart;
        texCoords[0].y = 0.0f;
        texCoords[1].x = texStart;
        texCoords[1].y = 1.0f;
        float pointAlpha = alphaStep + alpha;
        float texCoord = texStart + vehicle->m_msslTrgtArcRepeat;

        for (uint32_t i = 0; i < s_trajectoryPointCount; i++) {
            const C3Vector& point = CGUnit_C::s_trajectoryPoints[i];
            positions[2 * i + 2].x = point.x + side.x;
            positions[2 * i + 2].y = point.y + side.y;
            positions[2 * i + 2].z = point.z;
            positions[2 * i + 3].x = point.x - side.x;
            positions[2 * i + 3].y = point.y - side.y;
            positions[2 * i + 3].z = point.z;

            if (pointAlpha < 0.0f) {
                pointAlpha = 0.0f;
            }

            colors[2 * i + 2].b = 0xFF;
            colors[2 * i + 2].g = 0xFF;
            colors[2 * i + 2].r = 0xFF;
            colors[2 * i + 2].a = static_cast<uint8_t>(pointAlpha + 0.5f);
            colors[2 * i + 3] = colors[2 * i + 2];
            pointAlpha += alphaStep;
            texCoords[2 * i + 2].x = texCoord;
            texCoords[2 * i + 2].y = 0.0f;
            texCoords[2 * i + 3].x = texCoord;
            texCoords[2 * i + 3].y = 1.0f;
            texCoord += vehicle->m_msslTrgtArcRepeat;
        }

        CGxMatrixStack& texStack = g_theGxDevicePtr->m_xforms[GxXform_Tex0];
        texStack.m_dirty = 1;
        texStack.m_flags[texStack.m_level] &= ~CGxMatrixStack::F_Identity;
        texStack.m_mtx[texStack.m_level] = C44Matrix();
        g_theGxDevicePtr->RsSet(GxRs_Texture0, arcTex);
        C3Vector normal(0.0f, 0.0f, 1.0f);
        GxRsSet(GxRs_TextureShader0, 1);
        GxPrimLockVertexPtrs(vertexCount, positions, sizeof(C3Vector), &normal, 0, colors, sizeof(CImVector), nullptr, 0, texCoords, sizeof(C2Vector), nullptr, 0);
        GxDrawLockedElements(GxPrim_TriangleStrip, vertexCount, indices);
        GxPrimUnlockVertexPtrs();
        GxXformPop(GxXform_World);
        GxXformPop(GxXform_Tex0);
        g_theGxDevicePtr->RsPop();
    }

    if (!s_trajectoryTexture1) {
        return;
    }

    CGxTex* impactTex = TextureGetGxTex(s_trajectoryTexture1, 0, nullptr);

    if (!impactTex) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 3);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, impactTex);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, ProjectTex2dGetAddFadeTex());
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_ColorOp1, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp1, GxTexOp_Mod);
    const C3Vector& impact = CGUnit_C::s_trajectoryPoints[s_trajectoryPointCount - 1];
    float radius = vehicle->m_msslTrgtImpactTexRadius;
    CAaBox box;
    box.b.x = impact.x - radius;
    box.b.y = impact.y - radius;
    box.b.z = impact.z - radius;
    box.t.x = radius + impact.x;
    box.t.y = radius + impact.y;
    box.t.z = radius + impact.z;
    CImVector color;
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;
    color.a = static_cast<uint8_t>(s_trajectoryAlpha * 255.0f + 0.5f);
    C44Matrix rotation;
    rotation.RotateAroundZ(-facing);
    ProjectTex::Render2d(box, color, &rotation, 0.5f, 0x200122, 5, 0.039999999f);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: fe37f7cb auto
// OFFSET: 0x006FBFF0
void CGUnit_C::GetMissileCollisionSphere(CAaSphere* sphere, const C2Vector& direction) const {
    sphere->c = this->BuildSmoothPosition();
    CreatureModelDataRec* modelData = this->GetModelData();

    if (modelData) {
        float scale = this->GetTrueScale();

        if (0.0f < modelData->m_missileCollisionRadius) {
            sphere->r = scale * modelData->m_missileCollisionRadius;
        } else {
            sphere->r = modelData->m_collisionHeight * 0.5f;
        }

        float push = modelData->m_missileCollisionPush * scale;
        sphere->c.x = sphere->c.x - direction.x * push;
        sphere->c.y = sphere->c.y - push * direction.y;
        sphere->c.z = scale * modelData->m_missileCollisionRaise + sphere->c.z;
    } else {
        sphere->r = 1.0f;
    }

    sphere->c.z = sphere->r + sphere->c.z;
}

// VERIFY: 3f50b660 manual
// OFFSET: 0x006FC0A0
void CMissileCollision::ComputeHitSpheres(CAaSphere* spheres) {
    uint32_t i = 0;

    for (CGUnit_C* unit = this->m_units[0]; unit; unit = this->m_units[i]) {
        unit->GetMissileCollisionSphere(&spheres[i], this->m_dir);
        spheres[i].r = this->m_missile->m_collisionRadius + spheres[i].r;
        i++;
    }

    spheres[i].r = 0.0f;
}

// VERIFY: fb1f96a2 auto
// OFFSET: 0x006FC210
void CGUnit_C::RecordMissileTrajectoryUpdate(uint8_t castId, const C3Vector* impact, float pitch, float speed, float time) {
    uint32_t now = FrameTime::s_curTimeMs;

    for (CMissile* missile = this->m_missiles; missile; missile = missile->m_next) {
        if (missile->m_castID == castId) {
            missile->m_castTime = now;
            missile->m_impactPos = *impact;
            missile->m_pitch = pitch;
            missile->m_trajectorySpeed = speed;
            missile->m_arrivalTime = now + static_cast<uint32_t>(time * 1000.0f);
        }
    }
}

// VERIFY: ca588b4f manual
// OFFSET: 0x006FC360
static float ComputeTrajectoryImpact(const C3Vector* start, const C3Vector* delta, const CAaSphere* spheres) {
    float x = delta->x;
    float y = delta->y;
    float z = delta->z;
    float length = sqrt(z * z + y * y + delta->x * delta->x);

    if (length < 0.0000099999997f) {
        return -1.0f;
    }

    float inv = 1.0f / length;
    float dirX = x * inv;
    float dirY = y * inv;
    float dirZ = inv * z;

    for (uint32_t i = 0; spheres[i].r > 0.0f; i++) {
        float dx = spheres[i].c.x - start->x;
        float dy = spheres[i].c.y - start->y;
        float dz = spheres[i].c.z - start->z;
        float projection = dz * dirZ + dx * dirX + dy * dirY;

        if (projection >= 0.0f && dy * dy + dx * dx + dz * dz - projection * projection <= spheres[i].r * spheres[i].r && length >= projection) {
            return projection * inv;
        }
    }

    return -1.0f;
}

// VERIFY: 6f3c88ff manual
// OFFSET: 0x006FC4A0
float CMissileCollision::ComputeUnitRank(CGUnit_C* unit) {
    if (unit->m_obj->m_type & TYPE_PLAYER) {
        int32_t mode = this->m_missile->m_flags & 0xC;

        if (!mode) {
            return -1.0f;
        }

        if (mode != 0xC) {
            CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

            if (!mover || !(mode & 0x4) != !mover->CanAttack(unit)) {
                return -1.0f;
            }
        }
    } else {
        int32_t mode = this->m_missile->m_flags & 0x30;

        if (!mode || !unit->m_creatureCacheEntry || !(unit->m_creatureCacheEntry->m_typeFlags & 0x80000)) {
            return -1.0f;
        }

        if (mode != 0x30) {
            CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

            if (!mover || !(mode & 0x10) != !mover->CanAttack(unit)) {
                return -1.0f;
            }
        }
    }

    float speed = unit->m_movement->m_runSpeed;

    if (unit->m_movement->m_swimSpeed > speed) {
        speed = unit->m_movement->m_swimSpeed;
    }

    if (unit->m_movement->m_flightSpeed > speed) {
        speed = unit->m_movement->m_flightSpeed;
    }

    CAaSphere sphere;
    sphere.c = C3Vector(0.0f, 0.0f, 0.0f);
    sphere.r = 0.0f;
    unit->GetMissileCollisionSphere(&sphere, this->m_dir);
    float radius = this->m_missile->m_collisionRadius + sphere.r;

    if (radius < 0.000099999997f) {
        return -1.0f;
    }

    C2Vector position(sphere.c);
    float dx = this->m_end.x - position.x;
    float dy = this->m_end.y - position.y;
    float rank;

    if (this->m_speed <= speed) {
        rank = -1.0f;
    } else {
        float distance = fabs(this->m_perp.y * dy + this->m_perp.x * dx);
        float along = (this->m_perp.x * distance + position.x - this->m_start.x) * this->m_dir.x + (this->m_perp.y * distance + position.y - this->m_start.y) * this->m_dir.y;

        if (along < 0.000099999997f) {
            float sx = this->m_start.x - position.x;
            float sy = this->m_start.y - position.y;
            rank = (radius * radius - (sx * sx + sy * sy)) * 10000.0f;
        } else if (this->m_length <= along) {
            rank = -1.0f;
        } else {
            float reach = along / this->m_length * this->m_duration * speed + radius;
            rank = (reach * reach - distance * distance) / along;
        }
    }

    float reach = this->m_duration * speed + radius;
    float endRank = (reach * reach - (dx * dx + dy * dy)) / this->m_length;

    if (endRank > 0.0f && endRank > rank) {
        return endRank;
    }

    if (0.0f >= rank) {
        return -1.0f;
    }

    return rank;
}

// VERIFY: 0d859195 manual
// OFFSET: 0x006FC7D0
static void SortUnitRanks(MissileUnitRank* ranks, int32_t count) {
    if (count < 2) {
        return;
    }

    int32_t node = count / 2 + 1;
    int32_t size = count;

    while (true) {
        if (node <= 1) {
            MissileUnitRank swap = ranks[0];
            ranks[0] = ranks[size - 1];
            ranks[size - 1] = swap;
            size--;

            if (size == 1) {
                return;
            }
        } else {
            node--;
        }

        int32_t parent = node;

        while (2 * parent <= size) {
            int32_t child = 2 * parent;

            if (child < size && ranks[child].m_rank - ranks[child - 1].m_rank < 0.0f) {
                child++;
            }

            if (ranks[child - 1].m_rank - ranks[parent - 1].m_rank >= 0.0f) {
                break;
            }

            MissileUnitRank swap = ranks[parent - 1];
            ranks[parent - 1] = ranks[child - 1];
            ranks[child - 1] = swap;
            parent = child;
        }
    }
}

// VERIFY: 13bd75d7 manual
// OFFSET: 0x006FC9B0
float CMissileCollision::ComputeSegmentImpactTime(const C3Vector* start, const C3Vector* end) {
    CAaSphere spheres[16];

    for (uint32_t i = 0; i < 16; i++) {
        spheres[i].c = C3Vector(0.0f, 0.0f, 0.0f);
        spheres[i].r = 0.0f;
    }

    this->ComputeHitSpheres(spheres);
    C3Vector delta;
    delta.x = end->x - start->x;
    delta.y = end->y - start->y;
    delta.z = end->z - start->z;
    return ComputeTrajectoryImpact(start, &delta, spheres);
}

// VERIFY: 1d5508fa manual
// OFFSET: 0x006FCAA0
int32_t CMissileCollision::MaybeAddUnitToWorkingList(WOWGUID guid, void* param) {
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__));

    if (!unit || !(unit->m_obj->m_type & TYPE_UNIT) || unit == s_unitRankExclude) {
        return 1;
    }

    float rank = static_cast<CMissileCollision*>(param)->ComputeUnitRank(unit);

    if (-1.0f == rank) {
        return 1;
    }

    CGUnit_C* root = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->GetStoredRootVehicleUnit() : unit->FindRootVehicle(nullptr);

    if (root == s_unitRankExclude) {
        return 1;
    }

    int32_t index = s_unitRankCount;
    s_unitRanks[index].m_unit = unit;
    s_unitRanks[index].m_rank = rank;
    s_unitRankCount = index + 1;
    return index + 1 < 128;
}

// VERIFY: 8c3f617d manual
// OFFSET: 0x006FCB60
int32_t CMissileCollision::Init(const C3Vector* start, const C3Vector* end, float pitch, float speed, const SpellMissileRec* missile, CGUnit_C* caster) {
    this->m_start.x = start->x;
    this->m_start.y = start->y;
    this->m_end.x = end->x;
    this->m_end.y = end->y;
    float dirX = this->m_end.x - this->m_start.x;
    float dirY = this->m_end.y - this->m_start.y;
    this->m_dir.x = dirX;
    this->m_dir.y = dirY;

    if (fabs(this->m_dir.x) >= 0.001f || fabs(this->m_dir.y) >= 0.001f) {
        this->m_length = sqrt(this->m_dir.y * this->m_dir.y + this->m_dir.x * this->m_dir.x);
        float inv = 1.0f / sqrt(this->m_dir.x * this->m_dir.x + this->m_dir.y * this->m_dir.y);
        this->m_dir.x = this->m_dir.x * inv;
        this->m_dir.y = inv * this->m_dir.y;

        float frac;
        int32_t whole;
        CMath::split_(pitch * 0.31830987f, &frac, &whole);
        float cosine = 1.0f - frac * ((6.0f - 4.0f * frac) * frac);

        if (whole & 0x1) {
            cosine = -cosine;
        }

        this->m_speed = cosine * speed;

        if (this->m_speed < 0.000099999997f) {
            this->m_speed = 0.000099999997f;
        }
    } else {
        float time;

        if (speed <= 0.000099999997f) {
            time = 1.0f;
        } else {
            float dz = end->z - start->z;
            float dy = end->y - start->y;
            float dx = end->x - start->x;
            time = sqrt(dx * dx + (dy * dy + dz * dz)) / speed;
        }

        this->m_speed = 0.0099999998f / time;
        this->m_start.x = this->m_start.x - 0.0099999998f;
        this->m_dir.x = 1.0f;
        this->m_dir.y = 0.0f;
        this->m_length = 1.0f;
    }

    this->m_perp.x = -this->m_dir.y;
    s_unitRankCount = 0;
    this->m_perp.y = this->m_dir.x;
    this->m_missile = missile;
    s_unitRankExclude = caster;
    this->m_duration = this->m_length / this->m_speed;
    ClntObjMgrEnumVisibleObjects(CMissileCollision::MaybeAddUnitToWorkingList, this);

    int32_t count = s_unitRankCount;

    if (count > 15) {
        SortUnitRanks(s_unitRanks, s_unitRankCount);
        count = 15;
        s_unitRankCount = 15;
    }

    int32_t i = 0;

    for (; i < count; i++) {
        this->m_units[i] = s_unitRanks[i].m_unit;
    }

    this->m_units[i] = nullptr;
    return i;
}

// VERIFY: f8937da0 manual
// OFFSET: 0x006FCD60
int32_t CGUnit_C::ComputeMissileTrajectory(const SpellRec* spell, float* pitch, float* speed, C3Vector* launch, C3Vector* impact, float* time, C3Vector* points, uint32_t* count, int32_t preview) {
    if (!spell) {
        return 0;
    }

    SpellMissileRec* missile = g_spellMissileDB.GetRecord(spell->m_spellMissileID);

    if (!missile) {
        return 0;
    }

    VehicleRec* vehicleRec = this->m_vehicle ? this->m_vehicle->m_vehicleRec : nullptr;

    if (vehicleRec && (vehicleRec->m_flags & 0x10)) {
        *pitch = this->GetPitch();

        if (vehicleRec->m_flags & 0x40) {
            ClampInPlace(*pitch, vehicleRec->m_pitchMin, vehicleRec->m_pitchMax);
        }
    } else {
        *pitch = (missile->m_defaultPitchMax + missile->m_defaultPitchMin) * 0.5f;
    }

    if (vehicleRec && (vehicleRec->m_flags & 0x800)) {
        float power = CGInputControl::GetUIVehiclePower();
        *speed = power;
        *speed = power * (missile->m_defaultSpeedMax - missile->m_defaultSpeedMin);
        *speed = *speed + missile->m_defaultSpeedMin;
    } else {
        *speed = (missile->m_defaultSpeedMax + missile->m_defaultSpeedMin) * 0.5f;
    }

    CM2Model* model = this->GetObjectModel();

    if (model && model->IsLoaded(0, 0) && (model->m_fFlags & 0x8000) && model->HasEvent('MIA$')) {
        *launch = model->GetEventPosition('MIA$');
    } else {
        *launch = this->GetMissileFirePosition();
        this->OverrideMissileFirePos(launch, spell);
    }

    float facing = this->m_vehiclePassenger ? this->m_vehiclePassenger->GetLockedWorldFacing() : this->GetFacing();
    float yaw;
    uint32_t bits;

    if (preview) {
        *pitch = (missile->m_randomizePitchMax + missile->m_randomizePitchMin) * 0.5f + *pitch;
        *speed = (missile->m_randomizeSpeedMax + missile->m_randomizeSpeedMin) * 0.5f + *speed;
        yaw = facing + 0.5f * (missile->m_randomizeFacingMax + missile->m_randomizeFacingMin);
    } else {
        *pitch = *pitch + missile->m_randomizePitchMin;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        *pitch = (missile->m_randomizePitchMax - missile->m_randomizePitchMin) * (*reinterpret_cast<float*>(&bits) - 1.0f) + *pitch;
        *speed = missile->m_randomizeSpeedMin + *speed;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        *speed = (missile->m_randomizeSpeedMax - missile->m_randomizeSpeedMin) * (*reinterpret_cast<float*>(&bits) - 1.0f) + *speed;
        float base = missile->m_randomizeFacingMin + facing;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        yaw = (missile->m_randomizeFacingMax - missile->m_randomizeFacingMin) * (*reinterpret_cast<float*>(&bits) - 1.0f) + base;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        *pitch = (*reinterpret_cast<float*>(&bits) - 1.0f) * 0.02f + *pitch - 0.0099999998f;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        *speed = (*reinterpret_cast<float*>(&bits) - 1.0f) * 0.02f + *speed - 0.0099999998f;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        launch->x = (*reinterpret_cast<float*>(&bits) - 1.0f) * 0.2f + launch->x - 0.1f;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        launch->y = (*reinterpret_cast<float*>(&bits) - 1.0f) * 0.2f + launch->y - 0.1f;
        bits = (CRandom::uint32_(g_rndSeed) & 0x7FFFFF) | 0x3F800000;
        launch->z = (*reinterpret_cast<float*>(&bits) - 1.0f) * 0.02f + launch->z - 0.0099999998f;
    }

    float cosPitch = cos(*pitch);
    C3Vector velocity;
    velocity.x = cos(yaw) * cosPitch;
    velocity.y = cosPitch * sin(yaw);
    velocity.z = sin(*pitch);

    CGUnit_C* root = this->m_vehiclePassenger ? this->m_vehiclePassenger->GetStoredRootVehicleUnit() : this->FindRootVehicle(nullptr);

    if (!root) {
        root = this;
    }

    if (!(this->m_movement->m_flags & 0x2200000) && model && (model->m_fFlags & 0x8000)) {
        C44Matrix transform = model->GetAnimatedWorldTransform();

        if (transform.c2 < 1.0f) {
            transform.Scale(1.0f / sqrt(transform.a2 * transform.a2 + transform.a0 * transform.a0 + transform.a1 * transform.a1));
            transform.d0 = 0.0f;
            transform.d1 = 0.0f;
            transform.d2 = 0.0f;
            transform.RotateAroundZ(-yaw);
            velocity = TransformDirection(velocity, transform);
            *pitch = asin(velocity.z);
        }
    }

    float scale = *speed * 0.1f;
    velocity.x = velocity.x * scale;
    velocity.y = velocity.y * scale;
    float gravity = missile->m_gravity * 0.010000001f;
    velocity.z = scale * velocity.z - gravity * 0.5f;

    int32_t steps;

    if (missile->m_maxDuration <= 0.000099999997f || (steps = static_cast<int32_t>(missile->m_maxDuration * 10.0f)) > 200) {
        steps = 200;
    }

    int32_t maxSteps = steps;
    CM2Model* rootModel = root->GetObjectModel();
    C3Vector center;

    if (rootModel && rootModel->IsLoaded(0, 0)) {
        if (!(rootModel->m_shared->m_flags & 0x1)) {
            rootModel->WaitForLoad(nullptr);
        }

        center = rootModel->m_shared->m_data->m_collisionBounds.m_extent.Center();
    } else {
        center.x = 0.0f;
        center.y = 0.0f;
        center.z = 1.0f;
    }

    C3Vector normal = root->m_movement->GetGroundNormal();
    C3Vector up;
    up.x = normal.x * center.z;
    up.y = normal.y * center.z;
    up.z = center.z * normal.z;
    C3Vector rootPosition = root->GetPosition();
    C3Vector start;
    start.x = rootPosition.x + up.x;
    start.y = rootPosition.y + up.y;
    start.z = rootPosition.z + up.z;
    C3Vector hit(0.0f, 0.0f, 0.0f);
    float t = 1.0f;

    if (World::Intersect(start, *launch, &hit, &t, 0x100111, nullptr)) {
        *impact = *launch;
        *time = 0.0f;

        if (points) {
            points[*count] = *impact;
            ++*count;
        }

        return 1;
    }

    CAaSphere spheres[16];

    for (uint32_t i = 0; i < 16; i++) {
        spheres[i].c = C3Vector(0.0f, 0.0f, 0.0f);
        spheres[i].r = 0.0f;
    }

    if (preview && (missile->m_flags & 0x3C)) {
        CMissileCollision collision;
        C3Vector travel;
        travel.x = velocity.x * steps;
        travel.y = velocity.y * steps;
        travel.z = steps * velocity.z;
        C3Vector end = *launch + travel;
        collision.Init(launch, &end, *pitch, *speed, missile, root);
        collision.ComputeHitSpheres(spheres);
    } else {
        spheres[0].r = 0.0f;
    }

    C3Vector from = *launch;
    C3Vector to;
    to.x = from.x + velocity.x;
    to.y = from.y + velocity.y;
    to.z = from.z + velocity.z;
    t = 1.0f;
    float traveled;

    if (World::Intersect(from, to, &hit, &t, 0x100111, nullptr)) {
        *impact = hit;
        traveled = (maxSteps - steps) + t;
    } else {
        while (true) {
            if (spheres[0].r > 0.0f) {
                float fraction = ComputeTrajectoryImpact(&from, &velocity, spheres);

                if (-1.0f != fraction) {
                    impact->x = velocity.x * fraction + from.x;
                    impact->y = velocity.y * fraction + from.y;
                    impact->z = fraction * velocity.z + from.z;
                    traveled = fraction + (maxSteps - steps);
                    break;
                }
            }

            if (--steps <= 0) {
                *impact = to;
                traveled = static_cast<float>(maxSteps);
                break;
            }

            if (points) {
                points[(*count)++] = to;
            }

            velocity.z = velocity.z - gravity;
            from = to;
            to.x = from.x + velocity.x;
            to.y = from.y + velocity.y;
            to.z = velocity.z + from.z;
            t = 1.0f;

            if (World::Intersect(from, to, &hit, &t, 0x100111, nullptr)) {
                *impact = hit;
                traveled = (maxSteps - steps) + t;
                break;
            }
        }
    }

    *time = traveled * 0.1f;

    if (points) {
        points[*count] = *impact;
        ++*count;
    }

    return 1;
}

// VERIFY: 2fb23803 manual
// OFFSET: 0x006FD6B0
void CGUnit_C::SendFinalMissileTrajectoryUpdate() {
    SpellRec spell;

    if (!g_spellDB.GetRecord(this->m_missileTrajectorySpell, &spell)) {
        return;
    }

    C3Vector launch(0.0f, 0.0f, 0.0f);
    C3Vector impact(0.0f, 0.0f, 0.0f);
    float pitch;
    float speed;
    float time;

    if (!this->ComputeMissileTrajectory(&spell, &pitch, &speed, &launch, &impact, &time, nullptr, nullptr, 0)) {
        return;
    }

    Spell_C_EncodeSequenceIntoMissileTrajectory(reinterpret_cast<uint32_t*>(&pitch), reinterpret_cast<uint32_t*>(&speed), reinterpret_cast<uint32_t*>(&launch), reinterpret_cast<uint32_t*>(&impact));

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_UPDATE_MISSILE_TRAJECTORY));
    msg.Put(this->m_obj->m_guid.m_guid);
    msg.Put(static_cast<uint32_t>(spell.m_id));
    msg.Put(pitch);
    msg.Put(speed);
    msg.Put(launch.x);
    msg.Put(launch.y);
    msg.Put(launch.z);
    msg.Put(impact.x);
    msg.Put(impact.y);
    msg.Put(impact.z);
    WriteMovementPacket(this, MSG_MOVE_STOP, &msg, 1);
    msg.Finalize();
    ClientServices::Send2(&msg);

    float bytes;
    uint32_t latency;
    ClientServices::Connection()->GetNetStats(bytes, bytes, latency);
    float arrival = time + static_cast<int32_t>(latency) * 0.001f * 0.5f + 0.2f;
    float limit = time * 1.3f;

    if (limit < arrival) {
        arrival = limit;
    }

    WOWGUID guid = this->m_obj->m_guid;
    PendingSpellCast* cast = GetPendingSpellCast(this->m_castCount, this->m_castSpellId, guid);

    if (cast) {
        cast->m_data.m_targetFlags |= 0x40;
        cast->m_data.m_destPos = impact;
        cast->m_isTrajectoryCast = 1;

        if (this->m_missileTrajectoryReleased) {
            this->RecordMissileTrajectoryUpdate(cast->m_data.m_castCount, &impact, pitch, speed, arrival);
        } else {
            cast->m_data.m_elevation = pitch;
            cast->m_data.m_missileSpeed = speed;
            SpellVisualRec visualRec;
            const SpellVisualRec* visual = this->GetAppropriateSpellVisual(&spell, visualRec, 0, 0);
            SpellVisualKitRec* kit = g_spellVisualKitDB.GetRecord(visual ? visual->m_castKit : 0);
            TSStackArray<CGUnit_C*> targets(static_cast<CGUnit_C**>(alloca(0)), 0, 0);
            Spell_C_PlayMissileTrajectoryCastVisuals(this, cast, &spell, visual, kit, &targets, arrival);
            this->ReleaseMissiles();
        }
    }

    this->StopUpdatingMissileTrajectory(spell.m_id);
}

// VERIFY: 8309b5b1 manual
// OFFSET: 0x006FE9B0
void CGUnit_C::StartMissileTrajectoryTargeting(CGUnit_C* unit, const VehicleRec* vehicleRec) {
    CGUnit_C::StopMissileTrajectoryTargeting();

    if (!(vehicleRec->m_flags & 0xF800000)) {
        return;
    }

    CStatus status;

    if (*vehicleRec->m_msslTrgtArcTexture) {
        s_trajectoryTexture0 = TextureCreate(vehicleRec->m_msslTrgtArcTexture, CGxTexFlags(GxTex_Linear, 1, 1, 0, 0, 0, 1, 0, 0, 0), &status, 0);
    } else {
        s_trajectoryTexture0 = nullptr;
    }

    if (*vehicleRec->m_msslTrgtImpactTexture) {
        s_trajectoryTexture1 = TextureCreate(vehicleRec->m_msslTrgtImpactTexture, CGxTexFlags(GxTex_Linear, 0, 0, 0, 0, 0, 1, 0, 0, 0), &status, 0);
    } else {
        s_trajectoryTexture1 = nullptr;
    }

    for (uint32_t i = 0; i < 2; i++) {
        const char* name = vehicleRec->m_msslTrgtImpactModel[i];

        if (name && *name) {
            CGUnit_C::s_trajectoryModels[i] = World::s_m2Scene->CreateModel(name, 0);
        }
    }

    s_trajectoryTargetGuid = unit->m_obj->m_guid;
    CGUnit_C::s_missileTrajectoryTargetingSpell = 0;
    CGUnit_C::s_pitchMissileTrajectoryTargetingNextFrame = 0;
    s_trajectoryFadeStart = 0;
    s_trajectoryStartTime = FrameTime::s_curTimeMs;
}

// VERIFY: 2a978a8c auto
// OFFSET: 0x006FBFA0
int32_t MissileTrajectoryObjectCallback(void* param, uint32_t flags, uint64_t param64, uintptr_t param32) {
    if (s_trajectoryAlpha <= 0.0f && CGUnit_C::s_trajectoryWorldObjects[param32]) {
        World::ObjectDestroy(CGUnit_C::s_trajectoryWorldObjects[param32]);
        CGUnit_C::s_trajectoryWorldObjects[param32] = nullptr;
    }

    return CGUnit_C::s_trajectoryWorldObjects[param32] != nullptr;
}

// VERIFY: 156171df auto
// OFFSET: 0x006FBF80
void CGUnit_C::PitchMissileTrajectoryTargetingNextFrame() {
    s_pitchMissileTrajectoryTargetingNextFrame = 1;
}

// VERIFY: 3eb050df auto
// OFFSET: 0x006FBF90
void CGUnit_C::SetMissileTrajectoryTargetingSpell(uint32_t spellId) {
    s_missileTrajectoryTargetingSpell = spellId;
}

// VERIFY: 9082d6a5 auto
// OFFSET: 0x006FC290
void CGUnit_C::OnRemoteMissileTrajectoryCollision(MissileCollisionData* data) {
    for (CMissile* missile = this->m_missiles; missile; missile = missile->m_next) {
        if (missile->m_casterGuid == data->m_casterGuid && missile->m_castID == data->m_castID) {
            missile->SetRemoteCollisionPos(data->m_position);
        }
    }
}
