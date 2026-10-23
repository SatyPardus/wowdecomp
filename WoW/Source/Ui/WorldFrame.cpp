#include "WorldFrame.h"
#include "Engine/Source/Base/ConvertUTF.h"
#include <cstring>
#include <typeinfo>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/CKeyEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FFX/EffectGlow.h"
#include "Engine/Source/FFX/FFX.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxTexFlags.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Os/Time.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Common/TextureCache.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/MapRec.h"
#include "WoW/Source/DB/Rec/ScreenEffectRec.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Game/GameClient/FFXEffects.h"
#include "WoW/Source/Game/GameClient/PassFogSeed.h"
#include "WoW/Source/Game/GameClient/PlayerName.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/Game/GameClient/WorldText.h"
#include "WoW/Source/Game/GameTime.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Magic/MagicClient/SpellVisuals.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/ObjectClient/AuraData.h"
#include "WoW/Source/Object/ObjectClient/Corpse_C.h"
#include "WoW/Source/Object/ObjectClient/DynamicObject_C.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Missile_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/Cursor.h"
#include "WoW/Source/UIUtil/InputControl.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/UIUtil/SmartScreenRect.h"
#include "WoW/Source/UIUtil/Tooltip.h"
#include "WoW/Source/Ui/BattlefieldInfo.h"
#include "WoW/Source/Ui/CGCooldown.h"
#include "WoW/Source/Ui/CGQuestPOIFrame.h"
#include "WoW/Source/Ui/CGWorldStateInfo.h"
#include "WoW/Source/Ui/CharacterModelBase.h"
#include "WoW/Source/Ui/ChatBubbleFrame.h"
#include "WoW/Source/Ui/CommentatorFrame.h"
#include "WoW/Source/Ui/DressUpModelFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/MailInfo.h"
#include "WoW/Source/Ui/MinimapFrame.h"
#include "WoW/Source/Ui/NamePlateFrame.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/TabardModelFrame.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/UIMacros.h"
#include "WoW/Source/WorldClient/Barrier.h"
#include "WoW/Source/WorldClient/DayNight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapEntity.h"
#include "WoW/Source/WorldClient/MapWeather.h"
#include "WoW/Source/WorldClient/World.h"
#include <cfloat>
#include <cmath>
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x009F9864
static const CURSORMODE s_skinCursorModes[2] = { SKIN_ALLIANCE_CURSOR, SKIN_HORDE_CURSOR };

// STATIC: 0x009F986C
static const CURSORMODE s_skinCursorModesUnable[2] = { SKIN_ALLIANCE_ERROR_CURSOR, SKIN_HORDE_ERROR_CURSOR };

enum SPELLSHADOWSTYLE : int32_t {
    SPELL_GOOD = 0,
    SPELL_BAD = 1,
    NUM_SPELL_SHADOWS = 2,
    SPELL_NONE = 3,
};

// STATIC: 0x00AC799C
static const char* s_spellShadowTextureNames[NUM_SPELL_SHADOWS] = { "Interface\\SpellShadow\\Spell-Shadow-Acceptable.blp", "Interface\\SpellShadow\\Spell-Shadow-Unacceptable.blp" };

// STATIC: 0x00AC79A4
static SPELLSHADOWSTYLE s_spellShadowStyle = SPELL_NONE;

// STATIC: 0x00B74350
static CTexture* s_spellShadowTexture[NUM_SPELL_SHADOWS];

// STATIC: 0x00B74370
static float s_spellShadowSize;

// STATIC: 0x00B74380
static C3Vector s_spellShadowPos;

// STATIC: 0x00B74358
static CVar* s_listenerAtCharacterVar;

// STATIC: 0x00B7435C
FFX::Effect* CGWorldFrame::ffxEffectSwirlingFog;

// STATIC: 0x00B74360
FFX::Effect* CGWorldFrame::ffxEffectNetherWorld;

// STATIC: 0x00B74364
FFX::Effect* CGWorldFrame::ffxEffectGlow;

// STATIC: 0x00B74368
FFX::Effect* CGWorldFrame::ffxEffectDeath;

// STATIC: 0x00B7436C
CGWorldFrame* CGWorldFrame::s_currentWorldFrame;

// STATIC: 0x00B74394
CDataAllocator CGWorldFrame::s_worldFrameHeap(sizeof(CGWorldFrame), 1);

// STATIC: 0x00B743A8
static char s_keyNameBuffer[8];

// VERIFY: d8525c88 auto
// OFFSET: 0x004F5960
CGCamera* CGWorldFrame::GetActiveCamera() {
    if (CGWorldFrame::s_currentWorldFrame) {
        return CGWorldFrame::s_currentWorldFrame->m_camera;
    }

    return nullptr;
}

// VERIFY: 407a59b7 manual
// OFFSET: 0x004F59F0
void __stdcall CGWorldFrame::CursorTrackObject(CGGameObject_C* object) {
    int32_t mode = object->CanChangeCursor();

    if (!mode) {
        CursorResetMode();
    } else if (mode == NUM_CURSOR_MODES) {
        char file[260];
        object->GetCursorFile(file, sizeof(file));
        CursorSetFromName(file);
    } else {
        CursorSetMode(static_cast<CURSORMODE>(mode));
    }
}

// VERIFY: 65d31796 auto
// OFFSET: 0x004F5A60
int32_t CGWorldFrame::OnLayerTrackUpdate(const CMouseEvent& evt) {
    int32_t result = CSimpleFrame::OnLayerTrackUpdate(evt);

    if (!result) {
        return result;
    }

    CGInputControl::GetActive();

    return 1;
}

// VERIFY: dce627fd auto
// OFFSET: 0x004F5D20
void CGWorldFrame::OnMouseModeNormal() {
    this->m_worldFlags &= ~0x2;
}

// VERIFY: f3392eff auto
// OFFSET: 0x004F5D80
void CGWorldFrame::SetNamePlateUpdate() {
    this->m_namePlateFlags |= 0x1;
}

// VERIFY: c20f460f auto
// OFFSET: 0x004F58E0
int32_t CGWorldFrame::CanTargetCorpseSelection(const CGCorpse_C* corpse, uint32_t flags) {
    if (Spell_C_IsTargeting()) {
        return Spell_C_CanTargetCorpse(corpse);
    }

    return 1;
}

// VERIFY: 906e0891 auto
// OFFSET: 0x004F6270
static int32_t GetObjectHitTestType(CGObject_C* object) {
    if (!object) {
        return 0;
    }

    switch (object->m_obj->m_type) {
        case HEIR_TYPE_UNIT:
        case HEIR_TYPE_PLAYER: {
            CGUnit_C* unit = static_cast<CGUnit_C*>(object);

            if (unit->m_unit->m_unitFieldHealth > 0) {
                return 3;
            }

            return unit->CanBeLooted(OsGetAsyncTimeMs()) ? 2 : 0;
        }

        case HEIR_TYPE_GAMEOBJECT:
            return static_cast<CGGameObject_C*>(object)->CanUse() ? 1 : 0;

        case HEIR_TYPE_CORPSE:
            return static_cast<CGCorpse_C*>(object)->CanBeLooted() ? 2 : 0;

        default:
            return 0;
    }
}

// VERIFY: d69961a6 auto
// OFFSET: 0x004F6370
static void AddModelToHitTest(CM2Model* model, uint32_t group, void* owner, uint32_t mode) {
    if (model->m_fFlags & 0x1) {
        if (!model->m_hitTestPrev) {
            CM2Model** head = &model->m_scene->m_hitTestList;
            model->m_hitTestPrev = head;
            model->m_hitTestNext = *head;
            *head = model;

            if (model->m_hitTestNext) {
                model->m_hitTestNext->m_hitTestPrev = &model->m_hitTestNext;
            }
        }

        model->m_hitTestMode = mode;
        model->m_hitTestGroup = group;
        model->m_hitTestOwner = owner;
    }

    for (CM2Model* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
        if (!attached->m_hitTestPrev) {
            AddModelToHitTest(attached, group, reinterpret_cast<void*>(-1), mode);
        }
    }
}

// VERIFY: 7714957c manual
// OFFSET: 0x004F6400
void CGWorldFrame::AddObjectToHitTestList(CModelRecord* record, uint32_t flags) {
    uint32_t group;

    if (record->m_guid == this->m_hitGuid) {
        group = -1;
    } else {
        group = GetObjectHitTestType(record->m_object);
    }

    record->m_object = nullptr;
    AddModelToHitTest(record->m_model, group, record, (flags >> 25) & 0x4);
}

// VERIFY: 611dc17c manual
// OFFSET: 0x004F6450
int32_t CGWorldFrame::GetLineSegment(float mouseX, float mouseY, C3Vector* start, C3Vector* end) {
    float x = (mouseX - this->m_viewportDDC.minX) / (this->m_viewportDDC.maxX - this->m_viewportDDC.minX);
    float y = (mouseY - this->m_viewportDDC.minY) / (this->m_viewportDDC.maxY - this->m_viewportDDC.minY);

    if (x < 0.0f || y < 0.0f || x > 1.0f || y > 1.0f) {
        return 0;
    }

    CameraGetLineSegment(x, y, start, end);

    C3Vector position = this->m_camera->m_position;
    *start += position;
    *end += position;

    return 1;
}

// VERIFY: 0b3308f3 manual
// OFFSET: 0x004F6650
void CGWorldFrame::GetCameraPosition(C3Vector* position) {
    *position = CGWorldFrame::s_currentWorldFrame->m_camera->m_position;
}

// VERIFY: c8cc818e auto
// OFFSET: 0x004F66C0
void CGWorldFrame::OnLayerTrackTerrain(const HITTESTRESULT& result) {
    if (!Spell_C_IsTargeting() || !Spell_C_CanTargetTerrain()) {
        CursorResetMode();
        this->HandleSpriteTrack(WOWGUID());
        return;
    }

    s_spellShadowPos = result.m_point;
    CTerrainClickEvent evt;
    evt.m_point = s_spellShadowPos;
    evt.m_guid = WOWGUID();

    if ((result.m_guid.m_guid & 0xF000000000000000ull) == 0x1000000000000000ull && (result.m_guid.m_guid & 0x0FF0000000000000ull) == 0x0FC0000000000000ull) {
        CGObject_C* transport = ClntObjMgrObjectPtr(result.m_guid, TYPE_OBJECT, __FILE__, __LINE__);

        if (transport) {
            evt.m_guid = result.m_guid;
            s_spellShadowPos *= transport->GetMatrix();
        }
    }

    Spell_C_SetCursorWorldObjectPosition(s_spellShadowPos);

    switch (Spell_C_HandleTerrainRay(evt, true)) {
        case 0: {
            s_spellShadowStyle = SPELL_GOOD;
            float radius = Spell_C_GetSpellRadius();
            s_spellShadowSize = radius > 20.0f ? 20.0f : Spell_C_GetSpellRadius();
            CursorSetMode(CAST_CURSOR);
            break;
        }

        case 1:
            s_spellShadowStyle = SPELL_BAD;
            s_spellShadowSize = 0.0f;
            CursorSetMode(CAST_ERROR_CURSOR);
            break;

        case 2:
            s_spellShadowStyle = SPELL_NONE;
            s_spellShadowSize = 0.0f;
            CursorSetMode(CAST_ERROR_CURSOR);
            break;
    }

    CMapEntity* entity = Spell_C_GetSpellWorldObject();

    if (!entity) {
        return;
    }

    CM2Model* model = Spell_C_GetCursorWorldObjectModel();
    CAaBox box;
    box.b = C3Vector(0.0f, 0.0f, 0.0f);
    box.t = C3Vector(0.0f, 0.0f, 0.0f);
    CAaSphere sphere;
    C3Vector center = { 0.0f, 0.0f, 0.0f };

    if (model->IsLoaded(0, 0)) {
        box = model->GetBoundingBox();
        sphere = model->GetBoundingSphere();
        center = sphere.c;
    }

    C44Matrix matrix;
    const C3Vector& position = Spell_C_GetCursorWorldObjectPosition();
    matrix.d0 = position.x;
    matrix.d1 = position.y;
    matrix.d2 = position.z;
    matrix.RotateAroundZ(Spell_C_GetCursorWorldObjectFacing());
    CMap::ObjectUpdate(entity, matrix, box, sphere, center, false, 0xFFFFFFFF);
    World::ObjectSetHidden(entity, s_spellShadowStyle != SPELL_GOOD);
}

// VERIFY: 364a9bc8 auto
// OFFSET: 0x004F6680
void CGWorldFrame::GetCameraFacing(C3Vector* facing) {
    *facing = CGWorldFrame::s_currentWorldFrame->m_camera->Forward();
}

// VERIFY: 1f9c430b manual
// OFFSET: 0x004F6D20
bool CGWorldFrame::GetScreenCoordinates(const C3Vector& world, C3Vector* screen, uint32_t* result) {
    const C3Vector& camera = this->m_camera->m_position;

    C4Vector relative;
    relative.x = world.x - camera.x;
    relative.y = world.y - camera.y;
    relative.z = world.z - camera.z;
    relative.w = 0.0f;

    C4Vector projected = relative * this->m_viewProjection;

    if (projected.z < this->m_camera->m_nearZ) {
        return false;
    }

    screen->z = projected.z;

    float invW = 1.0f / projected.w;
    float ndcX = (projected.x * invW + 1.0f) * 0.5f;
    float ndcY = (projected.y * invW + 1.0f) * 0.5f;

    float ddcX;
    float ddcY;
    NDCToDDC(ndcX * (this->m_viewportNDC.maxX - this->m_viewportNDC.minX), ndcY * (this->m_viewportNDC.maxY - this->m_viewportNDC.minY), &ddcX, &ddcY);

    if (this->m_rect.minX < 0.0f) {
        ddcX -= this->m_rect.minX;
    }

    if (this->m_rect.minY < 0.0f) {
        ddcY -= this->m_rect.minY;
    }

    screen->x = ddcX;
    screen->y = ddcY;

    float width = this->m_rect.maxX - this->m_rect.minX;
    float height = this->m_rect.maxY - this->m_rect.minY;

    if (!result) {
        return ddcX >= 0.0f && width >= ddcX && ddcY >= 0.0f && height >= ddcY;
    }

    uint32_t flags = (ddcX >= 0.0f ? 0x1 : 0) | (ddcY >= 0.0f ? 0x2 : 0) | (width >= ddcX ? 0x4 : 0) | (height >= ddcY ? 0x8 : 0);
    *result = flags;

    return flags == 0xF;
}

// VERIFY: aaab1f42 manual
// OFFSET: 0x004F6F50
void CGWorldFrame::SetCameraTarget(CGObject_C* target, int32_t freeLook) {
    this->m_camera->SetTarget(target, freeLook);

    if (target && (target->m_obj->m_type & TYPE_UNIT)) {
        static_cast<CGUnit_C*>(target)->OnVehicleCameraPossiblyNeeded();
    }
}

// VERIFY: 412d637a manual
// OFFSET: 0x004F6F90
void CGWorldFrame::ClearTrackedObjects() {
    if (this->m_trackedEffectGuidA) {
        CGObject_C* object = ClntObjMgrObjectPtr(this->m_trackedEffectGuidA, TYPE_OBJECT, __FILE__, __LINE__);

        if (object) {
            object->RenderTargetSelection();
        }

        this->m_trackedEffectGuidA = WOWGUID();
    }

    if (this->m_trackedEffectGuidB) {
        CGObject_C* object = ClntObjMgrObjectPtr(this->m_trackedEffectGuidB, TYPE_OBJECT, __FILE__, __LINE__);

        if (object) {
            object->RenderPetTargetSelection();
        }

        this->m_trackedEffectGuidB = WOWGUID();
    }
}

// VERIFY: 8d2db281 manual
// OFFSET: 0x004F7350
int32_t CGWorldFrame::IsUnitLegalSelection(CGUnit_C* unit, uint32_t flags) {
    if (!(flags & 0x20)) {
        if (unit->IsActivePlayer()) {
            return 0;
        }

        WOWGUID activePlayer = ClntObjMgrGetActivePlayer();

        if (unit->IsPossessedBy(activePlayer)) {
            return 0;
        }
    }

    if (flags & 0xF0000) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
        uint32_t friends = flags & 0x40000;

        if (friends && !player->CanAssist(unit, false)) {
            return (flags & 0x800000) && unit->m_unit->m_unitFieldCharmedby && player->CanAttack(unit);
        }

        if (!((flags & 0x10000) && player->IsInMyPartyOrRaid(unit)) && !((flags & 0x20000) && player->IsInMyParty(unit)) && !(friends && player->CanAssist(unit, false)) && !((flags & 0x80000) && player->CanAttack(unit))) {
            return 0;
        }
    }

    if (flags & 0x300000) {
        if (unit->m_unit->m_unitFieldHealth > 0 ? !(flags & 0x100000) : !(flags & 0x200000)) {
            return 0;
        }
    }

    if ((flags & 0x400000) && unit->GetCreatureType() == 12) {
        return 1;
    }

    if (!Spell_C_IsTargeting()) {
        return 1;
    }

    bool restricted = (unit->m_unit->m_unitFieldFlags >> 8) & 1;

    if (restricted) {
        SpellRec spell;

        if (g_spellDB.GetRecord(Spell_C_GetTargetingSpell(), &spell) && ((spell.m_attributesExC & 0x10000000) || (spell.m_attributesExE & 0x200000))) {
            return 1;
        }
    }

    return !restricted;
}

// VERIFY: 120569f0 auto
// OFFSET: 0x004F7530
int32_t CGWorldFrame::IsLegalSelection(CGObject_C* object, uint32_t flags) {
    switch (object->m_obj->m_type) {
        case HEIR_TYPE_PLAYER:
            if (!(flags & 0x10)) {
                return 0;
            }

            return this->IsUnitLegalSelection(static_cast<CGUnit_C*>(object), flags);

        case HEIR_TYPE_UNIT:
            if (!(flags & 0x8)) {
                return 0;
            }

            return this->IsUnitLegalSelection(static_cast<CGUnit_C*>(object), flags);

        case HEIR_TYPE_GAMEOBJECT:
            if (!(flags & 0x4) || (Spell_C_IsTargeting() && !Spell_C_CanTargetObject(static_cast<CGGameObject_C*>(object)))) {
                return 0;
            }

            return 1;

        case HEIR_TYPE_CORPSE:
            if (!(flags & 0x40)) {
                return 0;
            }

            return this->CanTargetCorpseSelection(static_cast<CGCorpse_C*>(object), flags);

        default:
            return 0;
    }
}

// VERIFY: 4da5a71a manual
// OFFSET: 0x004F7650
uint32_t CGWorldFrame::GetHitTestFilterFlags(int32_t a1) {
    if (Spell_C_IsTargeting()) {
        uint32_t flags = 0;

        if (Spell_C_CanTargetTerrain()) {
            flags = 0x3;
        }

        if (Spell_C_CanTargetObjects()) {
            flags |= 0x4;
        }

        if (Spell_C_CanTargetUnits()) {
            flags |= 0x18;

            if (Spell_C_CanTargetMe()) {
                flags |= 0x20;
            }

            if (Spell_C_CanTargetParty()) {
                flags |= 0x10000;
            }

            if (Spell_C_CanTargetFriends()) {
                flags |= 0x40000;
            }

            if (Spell_C_CanTargetEnemies()) {
                flags |= 0x80000;
            }

            if (Spell_C_CanTargetRaid()) {
                flags |= 0x20000;
            }

            if (Spell_C_CanTargetDead()) {
                flags |= 0x200040;
            }

            if (Spell_C_CanTargetAlive()) {
                flags |= 0x100000;
            }

            if (Spell_C_CanTargetNonCombatPet()) {
                flags |= 0x400000;
            }

            if (Spell_C_CanTargetPossessedFriends()) {
                flags |= 0x800000;
            }
        }

        if (Spell_C_CanTargetFriendCorpses()) {
            flags |= 0x40040;
        }

        if (Spell_C_CanTargetEnemyCorpses()) {
            flags |= 0x80040;
        }

        return flags;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    uint32_t flags = 0;
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover && mover->CanAutoInteract() && !CGGameUI::m_cursorItemType) {
        uint32_t moveFlags = mover->m_movement->m_flags;
        flags = 1;

        if ((moveFlags & 0x10000000) && !(moveFlags & 0x200000)) {
            flags = 3;
        }
    }

    if (player) {
        flags |= 0x5C;
    }

    return flags;
}

// VERIFY: d619b75b manual
// OFFSET: 0x004F7A50
void __stdcall CGWorldFrame::CursorTrackUnit(CGUnit_C* unit) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    if (((player->m_combatModeFlags & 1) && player->GetTransportGUID() != player->GetFarSightGuid()) || !player->VehicleAllowsInteraction()) {
        CGUnit_C* possessed = player->GetPossessedUnit();

        if (unit->m_unit->m_unitFieldHealth > 0 && possessed && possessed->CanAttackNow(unit)) {
            CursorSetMode(ATTACK_CURSOR);
        } else {
            CursorResetMode();
        }

        return;
    }

    float distance = player->GetDistanceToPos(unit->GetPosition());

    if (player->CanInteractNow(unit) && unit->m_unit->m_unitNpcFlags) {
        float reach = unit->m_unit->m_unitFieldCombatreach + 4.0f;
        int32_t unable = reach * reach < distance;
        int32_t mode = unit->GetCursorOverride(unable);

        if (mode) {
            if (mode == NUM_CURSOR_MODES) {
                char file[260];
                unit->GetCursorFile(unable, file, sizeof(file));
                CursorSetFromName(file);
            } else {
                CursorSetMode(static_cast<CURSORMODE>(mode));
            }

            return;
        }

        uint32_t flags = unit->m_unit->m_unitNpcFlags;

        if (flags & 0x1000) {
            CursorSetMode(unable ? REPAIRNPC_ERROR_CURSOR : REPAIRNPC_CURSOR);
        } else if (flags & 0x10000) {
            CursorSetMode(unable ? INNKEEPER_ERROR_CURSOR : INNKEEPER_CURSOR);
        } else if (flags & 0x2000) {
            CursorSetMode(unable ? TAXI_ERROR_CURSOR : TAXI_CURSOR);
        } else if (flags & 0x10) {
            CursorSetMode(unable ? TRAINER_ERROR_CURSOR : TRAINER_CURSOR);
        } else if ((flags & 0x4000) || (flags & 0x8000)) {
            CursorSetMode(unable ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
        } else if ((flags & 0x20000) || (flags & 0x800000)) {
            CursorSetMode(unable ? BUY_ERROR_CURSOR : BUY_CURSOR);
        } else if ((flags & 0x40000) || (flags & 0x80000) || unit->IsGuildMaster() || (flags & 0x100000)) {
            CursorSetMode(unable ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
        } else if (flags & 0x200000) {
            CursorSetMode(unable ? BUY_ERROR_CURSOR : BUY_CURSOR);
        } else if (flags & 0x400000) {
            CursorSetMode(unable ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
        } else if (flags & 0x80) {
            CursorSetMode(unable ? PICKUP_ERROR_CURSOR : PICKUP_CURSOR);
        } else if (flags & 0x1) {
            CursorSetMode(unable ? SPEAK_ERROR_CURSOR : SPEAK_CURSOR);
        } else if (flags & 0x1000000) {
            CursorSetMode(unable ? INTERACT_ERROR_CURSOR : INTERACT_CURSOR);
        } else if (unit->CanBeSpellClickedByLocalPlayer(player)) {
            CursorSetMode(unable ? VEHICLE_ERROR_CURSOR : VEHICLE_CURSOR);
        } else {
            CursorResetMode();
        }

        return;
    }

    if (unit->CanBeLooted(FrameTime::s_curTimeMs)) {
        if (player->CanLoot(unit) || player->m_lootTarget == unit->m_obj->m_guid) {
            CursorSetMode(CGGameUI::IsAutoLooting() ? LOOT_ALL_CURSOR : PICKUP_CURSOR);
        } else {
            CursorSetMode(CGGameUI::IsAutoLooting() ? LOOT_ALL_ERROR_CURSOR : PICKUP_ERROR_CURSOR);
        }

        return;
    }

    if ((unit->m_unit->m_unitFieldFlags & 0x4000000) && CGSpellBook::GetSkinningSpell(unit) && !((unit->m_obj->m_type & TYPE_PLAYER) && player->CanAssist(unit, false))) {
        float minRange;
        float maxRange;
        Spell_C::GetSpellRange(player, CGSpellBook::GetSkinningSpell(unit), &minRange, &maxRange, unit);

        if (unit->m_obj->m_type & TYPE_PLAYER) {
            if (maxRange * maxRange < distance) {
                CursorSetMode(s_skinCursorModesUnable[player->GetPVPFactionIndex()]);
            } else {
                CursorSetMode(s_skinCursorModes[player->GetPVPFactionIndex()]);
            }

            return;
        }

        switch (unit->GetSkinningType()) {
            case 0:
                CursorSetMode(maxRange * maxRange < distance ? SKIN_ERROR_CURSOR : SKIN_CURSOR);
                return;

            case 1:
                CursorSetMode(maxRange * maxRange < distance ? GATHER_ERROR_CURSOR : GATHER_CURSOR);
                return;

            case 2:
                CursorSetMode(maxRange * maxRange < distance ? MINE_ERROR_CURSOR : MINE_CURSOR);
                return;

            case 3:
                CursorSetMode(maxRange * maxRange < distance ? MINE_ERROR_CURSOR : MINE_CURSOR);
                return;

            default:
                CursorResetMode();
                return;
        }
    }

    if (!player->CanAttackNow(unit) || (player->m_unit->m_unitFieldFlags & 0x20000) || !CGGameUI::m_hasControl) {
        CursorResetMode();
        return;
    }

    float range = player->GetMeleeRange(unit);

    if (range * range < distance) {
        CursorSetMode(ATTACK_ERROR_CURSOR);
    } else {
        CursorSetMode(ATTACK_CURSOR);
    }
}

// VERIFY: 3106c989 manual
// OFFSET: 0x004F8000
void __stdcall CGWorldFrame::CursorTrackCorpse(CGCorpse_C* corpse) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player) {
        return;
    }

    C3Vector corpsePosition = corpse->GetPosition();
    C3Vector position = player->GetPosition();
    float dx = position.x - corpsePosition.x;
    float dy = position.y - corpsePosition.y;
    float dz = position.z - corpsePosition.z;
    float distance = dx * dx + dz * dz + dy * dy;

    if (corpse->CanBeLooted()) {
        if (player->CanLoot(corpse) || player->m_lootTarget == corpse->m_obj->m_guid) {
            CursorSetMode(CGGameUI::IsAutoLooting() ? LOOT_ALL_CURSOR : PICKUP_CURSOR);
        } else {
            CursorSetMode(CGGameUI::IsAutoLooting() ? LOOT_ALL_ERROR_CURSOR : PICKUP_ERROR_CURSOR);
        }

        return;
    }

    const CGUnit_C* asUnit = reinterpret_cast<const CGUnit_C*>(corpse);

    if ((corpse->m_corpse->m_corpseFieldFlags & 0x20) && CGSpellBook::GetSkinningSpell(asUnit) && !player->CanAssist(corpse)) {
        float minRange;
        float maxRange;
        Spell_C::GetSpellRange(player, CGSpellBook::GetSkinningSpell(asUnit), &minRange, &maxRange, nullptr);

        if (maxRange * maxRange < distance) {
            CursorSetMode(s_skinCursorModesUnable[player->GetPVPFactionIndex()]);
        } else {
            CursorSetMode(s_skinCursorModes[player->GetPVPFactionIndex()]);
        }

        return;
    }

    CursorResetMode();
}

// VERIFY: 92cbc82b manual
// OFFSET: 0x004F8190
void CGWorldFrame::OnLayerTrackObject(const HITTESTRESULT& result) {
    if (this->m_worldFlags & 0x2) {
        this->HandleSpriteTrack(WOWGUID());
        return;
    }

    if (Spell_C_IsTargeting()) {
        WOWGUID guid = result.m_guid;
        CursorSetMode(Spell_C::HandleSpriteRay(guid, true) ? CAST_CURSOR : CAST_ERROR_CURSOR);
        this->HandleSpriteTrack(result.m_guid);
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    CGObject_C* object = ClntObjMgrObjectPtr(result.m_guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!player || !object || !object->CanHighlight()) {
        CursorResetMode();
        this->HandleSpriteTrack(WOWGUID());
        return;
    }

    if (result.m_distance == 0.0f) {
        CursorResetMode();
        this->HandleSpriteTrack(result.m_guid);
        return;
    }

    switch (object->m_obj->m_type) {
        case HEIR_TYPE_ITEM:
            CursorSetMode(PICKUP_CURSOR);
            break;

        case HEIR_TYPE_UNIT:
        case HEIR_TYPE_PLAYER:
            CGWorldFrame::CursorTrackUnit(static_cast<CGUnit_C*>(object));
            break;

        case HEIR_TYPE_GAMEOBJECT:
            CGWorldFrame::CursorTrackObject(static_cast<CGGameObject_C*>(object));
            break;

        case HEIR_TYPE_CORPSE:
            CGWorldFrame::CursorTrackCorpse(static_cast<CGCorpse_C*>(object));
            break;

        default:
            CursorResetMode();
            break;
    }

    this->HandleSpriteTrack(result.m_guid);
}

// VERIFY: f0b2f3d4 manual
// OFFSET: 0x004F77C0
CGWorldFrameUnkObj::CGWorldFrameUnkObj() {
    for (int32_t i = 0; i < 16; i++) {
        CGWorldFrameUnkObjSomeElement& element = this->m_elements[i];
        element.unused_0014 = 0.0f;
        element.unused_0018 = 0.0f;
        element.unused_001C = 0.0f;
        element.unused_0020 = 0.0f;
        element.unused_0024 = 0.0f;
        element.unused_0028 = 0.0f;
        element.unused_002C = 0.0f;
        element.unused_0030 = 0.0f;
        element.unused_0034 = 0.0f;
        element.unused_0038 = 0.0f;
    }

    for (int32_t i = 0; i < 13; i++) {
        this->m_tail[i] = 0.0f;
    }
}

// VERIFY: 9d07413d manual
// OFFSET: 0x004F8660
void CGWorldFrame::SetPlayerFadeCameraValue(uint8_t value) {
    if (this->m_camera->m_targetGUID == ClntObjMgrGetActivePlayer()) {
        Player_C_SetPlayerRenderDisabled(value != 0);
    }

    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(this->m_camera->m_targetGUID, TYPE_UNIT, __FILE__, __LINE__));

    if (!unit) {
        return;
    }

    if ((unit->m_vehicle && unit->m_vehicle->m_vehicleRec) || (unit->m_vehiclePassenger && unit->m_vehiclePassenger->m_state == 3)) {
        unit = unit->m_vehiclePassenger ? unit->m_vehiclePassenger->GetStoredRootVehicleUnit() : unit->FindRootVehicle(nullptr);

        if (!unit) {
            return;
        }
    }

    unit->SetMaxAlpha(value * (1.0f / 255.0f));
}

// VERIFY: 4da3f8e7 manual
// OFFSET: 0x004F8770
void CGWorldFrame::UpdateScreenEffectParams() {
    if (FFX::s_effect != CGWorldFrame::ffxEffectGlow) {
        return;
    }

    float glow = DayNight::GetInfo()->m_bands.m_glow * 255.0f + 512.0f;
    uint8_t glowLevel = *reinterpret_cast<uint32_t*>(&glow) >> 14;
    int32_t underwater = 0;
    uint8_t minimum = 0;
    uint8_t level = minimum;

    if (ClntObjMgrGetCurrent()) {
        CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (player) {
            float drunk = player->GetDrunknessTurnValue() * 255.0f + 512.0f;
            uint8_t drunkLevel = *reinterpret_cast<uint32_t*>(&drunk) >> 14;

            if (World::SceneCamLiquidStatus(nullptr)) {
                minimum = 84;
                underwater = 1;
            }

            level = drunkLevel > minimum ? drunkLevel : minimum;
        }
    }

    int32_t params[3] = { underwater, glowLevel, level };
    CGWorldFrame::ffxEffectGlow->SetParameters(3, params);
}

// VERIFY: b33f52c2 manual
// OFFSET: 0x004F9310
void CGWorldFrame::MoveToFreeList(TSList<CModelRecord, TSGetLink<CModelRecord> >* list) {
    for (CModelRecord* record = list->Head(); record; record = list->Next(record)) {
        if (record->m_model) {
            record->m_model->Release();
            record->m_model = nullptr;
        }
    }

    this->m_freeModelList.Combine(list, 2, nullptr);
}

// VERIFY: abfda7eb manual
// OFFSET: 0x004F9410
bool CGWorldFrame::HitTestModelList(const C3Vector& start, const C3Vector& end) {
    C44Matrix view;
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];

    C3Vector camera = this->m_camera->m_position;
    C3Vector viewStart = (start - camera) * view;
    C3Vector viewEnd = (end - camera) * view;

    World::s_m2Scene->BeginHitTest();

    for (CModelRecord* record = this->m_modelList.Head(); record; record = this->m_modelList.Next(record)) {
        AddModelToHitTest(record->m_model, 0, record, 0);
    }

    float t = 1.0f;

    return World::s_m2Scene->EndHitTest(viewStart, viewEnd, t, 0) != nullptr;
}

// VERIFY: 7c925d58 manual
// OFFSET: 0x004F9550
uint64_t CGWorldFrame::FindClosestModel(const C3Vector& start, const C3Vector& end, uint32_t flags, float* distance) {
    C44Matrix view;
    CGxMatrixStack& stack = g_theGxDevicePtr->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];

    C3Vector camera = this->m_camera->m_position;
    C3Vector viewStart = (start - camera) * view;
    C3Vector viewEnd = (end - camera) * view;

    TSList<CModelRecord, TSGetLink<CModelRecord> > lowPriority;
    uint32_t time = OsGetAsyncTimeMs();
    World::s_m2Scene->BeginHitTest();
    int32_t allowsTerrain = Spell_C_TargetingSpellAllowsTerrain();

    for (CModelRecord* record = this->m_modelList.Head(); record;) {
        CModelRecord* next = this->m_modelList.Next(record);
        CGObject_C* object = ClntObjMgrObjectPtr(record->m_guid, TYPE_OBJECT, __FILE__, __LINE__);

        if (object && this->IsLegalSelection(object, flags) && (object->CanHighlight() || allowsTerrain)) {
            record->m_object = object;

            if ((object->m_obj->m_type & TYPE_UNIT) && static_cast<CGUnit_C*>(object)->IsLowPrioritySelection(time)) {
                lowPriority.LinkToTail(record);
            } else {
                this->AddObjectToHitTestList(record, flags);
            }
        }

        record = next;
    }

    float t = 1.0f;
    CModelRecord* hit = static_cast<CModelRecord*>(World::s_m2Scene->EndHitTest(viewStart, viewEnd, t, 1));

    if (!hit) {
        World::s_m2Scene->BeginHitTest();

        for (CModelRecord* record = lowPriority.Head(); record; record = lowPriority.Next(record)) {
            this->AddObjectToHitTestList(record, flags);
        }

        t = 1.0f;
        hit = static_cast<CModelRecord*>(World::s_m2Scene->EndHitTest(viewStart, viewEnd, t, 1));
    }

    this->m_modelList.Combine(&lowPriority, 2, nullptr);

    if (!hit) {
        return 0;
    }

    C3Vector delta = viewEnd - viewStart;
    float hitDistance = sqrt(delta.x * delta.x + delta.y * delta.y + delta.z * delta.z) * t;
    hit->m_distance = hitDistance;
    *distance = hitDistance;
    return hit->m_guid.m_guid;
}

// VERIFY: d0d131d0 manual
// OFFSET: 0x004F9930
int32_t CGWorldFrame::HitTest(const C3Vector& start, const C3Vector& end, uint32_t flags, HITTESTRESULT* result) {
    C3Vector hitPoint = { 0.0f, 0.0f, 0.0f };
    float t = 1.0f;
    float objectDistance = 0.0f;
    WOWGUID collision = WOWGUID();
    uint32_t worldFlags = (flags & 0x2) ? 0x1020124 : 0x1000124;
    C3Vector point = end;
    C3Vector direction = point - start;
    float length = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;

    if (fabs(length) >= 0.00000023841858f) {
        length = sqrt(length);
        float inverse = 1.0f / length;
        direction.x *= inverse;
        direction.y *= inverse;
        direction.z *= inverse;
    }

    int32_t hitWorld = World::Intersect(start, point, &hitPoint, &t, worldFlags, nullptr) != 0;

    if (hitWorld) {
        if (t < -0.001f || t > 1.001f || !_finite(t)) {
            DebugLog("CWorld::Intersect() returned a bogus value for terrDist: %f", static_cast<double>(t));
        }

        length *= t;
        collision = s_lastCollisionGUID;
        point.x = direction.x * length + start.x;
        point.y = direction.y * length + start.y;
        point.z = direction.z * length + start.z;
        t = length;
    }

    WOWGUID model = WOWGUID();

    if (flags & 0x7C) {
        model.m_guid = this->FindClosestModel(start, point, flags, &objectDistance);

        if (model) {
            if (objectDistance < -0.001f || !_finite(objectDistance)) {
                DebugLog("CGWorldFrame::FindClosestModel() returned a bogus value for objDist: %f", static_cast<double>(objectDistance));
            }

            length = objectDistance;
            point.x = direction.x * objectDistance + start.x;
            point.y = direction.y * objectDistance + start.y;
            point.z = objectDistance * direction.z + start.z;
        }
    }

    uint32_t terrain = flags & 0x3;

    if (terrain) {
        t = 1.0f;

        if (World::Intersect(start, point, &hitPoint, &t, 0x100151, nullptr)) {
            if (t < -0.001f || t > 1.001f || !_finite(t)) {
                DebugLog("CWorld::Intersect() returned a bogus value for terrDist: %f", static_cast<double>(t));
            }

            length *= t;
            collision = s_lastCollisionGUID;
            hitWorld = 1;
            point.x = direction.x * length + start.x;
            point.y = direction.y * length + start.y;
            point.z = direction.z * length + start.z;
            t = length;
        }
    }

    C3Vector screen;
    screen.z = 0.0f;

    if (this->GetScreenCoordinates(point, &screen, nullptr)) {
        g_theGxDevicePtr->CursorSetDepth(screen.z);
    }

    if (model) {
        result->m_distance = objectDistance;
        result->m_point = point;
        result->m_guid = model;
        return HIT_OBJECT;
    }

    if (!hitWorld) {
        return HIT_NONE;
    }

    if (collision) {
        result->m_point = point;
        CGObject_C* object = ClntObjMgrObjectPtr(collision, TYPE_OBJECT, __FILE__, __LINE__);

        if (object) {
            result->m_guid = collision;

            if (object->IsTransport()) {
                C44Matrix inverse = object->GetMatrix().AffineInverse();
                result->m_point *= inverse;
            }
        } else {
            result->m_guid = WOWGUID();
        }

        result->m_distance = t;
        return HIT_WORLD_OBJECT;
    }

    result->m_point = point;
    result->m_distance = t;
    result->m_guid = WOWGUID();
    return terrain != 0;
}

// VERIFY: 53a93536 auto
// OFFSET: 0x004F9DA0
int32_t CGWorldFrame::HitTestPoint(float x, float y, int32_t a3, HITTESTRESULT* result) {
    if (!(CGInputControl::GetActive()->m_mouseModeFlags & 0x1)) {
        return HIT_NONE;
    }

    CGxDevice* device = g_theGxDevicePtr;
    C44Matrix projection;
    C44Matrix view;
    projection = device->m_projection;
    CGxMatrixStack& stack = device->m_xforms[GxXform_View];
    view = stack.m_mtx[stack.m_level];
    this->m_camera->SetupWorldProjection(this->m_viewportDDC);

    int32_t kind = HIT_NONE;
    uint32_t flags = this->GetHitTestFilterFlags(a3);

    if (flags) {
        C3Vector start = { 0.0f, 0.0f, 0.0f };
        C3Vector end = { 0.0f, 0.0f, 0.0f };

        if (this->GetLineSegment(x, y, &start, &end)) {
            result->m_segStart = start;
            result->m_segEnd = end;
            kind = this->HitTest(start, end, flags, result);

            if (kind >= HIT_OBJECT) {
                this->m_hitGuid = result->m_guid;
            } else {
                this->MoveToFreeList(&this->m_hitModelList);
                this->m_hitGuid = WOWGUID();
            }
        }
    }

    g_theGxDevicePtr->XformSetProjection(projection);
    g_theGxDevicePtr->XformSetView(view);
    return kind;
}

// VERIFY: 21cb1ac2 auto
// OFFSET: 0x004FA5D0
void CGWorldFrame::UnitClear() {
    this->MoveToFreeList(&this->m_modelList);
    this->MoveToFreeList(&this->m_hitModelList);
}

// VERIFY: 0cec28f7 manual
// OFFSET: 0x004FABD0
CGWorldFrame::CGWorldFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
    this->unused_02C0 = 1;
    this->unused_02C4 = 1;
    this->m_trackedGuid = WOWGUID();
    this->m_hitGuid = WOWGUID();
    this->m_defaultActionHit.m_guid = WOWGUID();
    this->m_defaultActionHit.m_point = { 0.0f, 0.0f, 0.0f };
    this->m_defaultActionHit.m_distance = 0.0f;
    this->m_defaultActionHit.m_segStart = { 0.0f, 0.0f, 0.0f };
    this->m_defaultActionHit.m_segEnd = { 0.0f, 0.0f, 0.0f };
    this->m_defaultActionPointDDC = { 0.0f, 0.0f };
    this->m_worldFlags = (this->m_worldFlags & ~0x3) | 0x1;
    this->m_viewportDDC = { 0.0f, 0.0f, 0.0f, 0.0f };
    this->m_viewportNDC = { 0.0f, 0.0f, 0.0f, 0.0f };
    this->m_namePlateFlags = 0;

    CGWorldFrame::s_currentWorldFrame = this;

    this->SetFrameStrata(FRAME_STRATA_WORLD);
    this->EnableEvent(SIMPLE_EVENT_KEY, -1);
    this->EnableEvent(SIMPLE_EVENT_MOUSE, -1);
    this->EnableEvent(SIMPLE_EVENT_MOUSEWHEEL, -1);

    memset(this->m_keyDown, 0, sizeof(this->m_keyDown));
    memset(this->m_mouseDown, 0, sizeof(this->m_mouseDown));

    this->m_camera = new (__FILE__, __LINE__) CGCamera();
    s_listenerAtCharacterVar = CVar::Lookup("Sound_ListenerAtCharacter");
    this->unused_02C0 = 5;

    CStatus status;

    for (uint32_t i = 0; i < sizeof(s_spellShadowTexture) / sizeof(s_spellShadowTexture[0]); i++) {
        s_spellShadowTexture[i] = TextureCreate(s_spellShadowTextureNames[i], CGxTexFlags(GxTex_Linear), &status, 1);
        SysMsgAdd(&status);
    }

    EventRegisterEx(EVENT_ID_IDLE, CGWorldFrame::Update, this, -1000.0f);
    FFX::Init();

    CGWorldFrame::ffxEffectGlow = new (__FILE__, __LINE__) EffectGlow();
    CGWorldFrame::ffxEffectDeath = new (__FILE__, __LINE__) EffectDeath();
    CGWorldFrame::ffxEffectNetherWorld = new (__FILE__, __LINE__) EffectNetherWorld();
    CGWorldFrame::ffxEffectSwirlingFog = new (__FILE__, __LINE__) EffectSwirlingFog();

    CGWorldFrame::UpdateScreenEffect();
}

// VERIFY: 74e81b7f auto
// OFFSET: 0x0050F3F0
CSimpleFrame* CGWorldFrame::Create(CSimpleFrame* parent) {
    void* data = CGWorldFrame::s_worldFrameHeap.GetData(0, typeid(CGWorldFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGWorldFrame(parent) : nullptr;
}

// INLINED
void CGWorldFrame::operator delete(void* ptr) {
    if (ptr) {
        CGWorldFrame::s_worldFrameHeap.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: ee139ec8 auto
// OFFSET: 0x00521040
void CGWorldFrame::RegisterFrames() {
    FrameXML_RegisterFactory("WorldFrame", CGWorldFrame::Create, true);
    FrameXML_RegisterFactory("GameTooltip", CGTooltip::Create, false);
    FrameXML_RegisterFactory("Cooldown", CGCooldown::Create, false);
    FrameXML_RegisterFactory("Minimap", CGMinimapFrame::Create, false);
    FrameXML_RegisterFactory("PlayerModel", CGCharacterModelBase::Create, false);
    FrameXML_RegisterFactory("DressUpModel", CGDressUpModelFrame::Create, false);
    FrameXML_RegisterFactory("TabardModel", CGTabardModelFrame::Create, false);
    FrameXML_RegisterFactory("QuestPOIFrame", CGQuestPOIFrame::Create, false);
}

// VERIFY: d1740d14 manual
// OFFSET: 0x004F7020
void CGWorldFrame::SetScreenEffect(int32_t effect) {
    ScreenEffectRec* rec = g_screenEffectDB.GetRecord(effect);

    if (!rec) {
        DayNight::ClearOverrideFog();
        FFX::SetEffect(CGWorldFrame::ffxEffectGlow);
        DayNight::ClearOverrideLightParamsID();
        SI2::SetOverrideZoneSounds(0, 0);
        return;
    }

    switch (rec->m_effect) {
        case 0:
            DayNight::ClearOverrideFog();
            FFX::SetEffect(CGWorldFrame::ffxEffectGlow);
            break;

        case 1:
            DayNight::ClearOverrideFog();
            FFX::SetEffect(CGWorldFrame::ffxEffectDeath);
            break;

        case 2: {
            CImVector color;

            if (FFX::s_cvarFFX->m_intValue) {
                color.b = 0xFF;
                color.g = 0xFF;
                color.r = 0xFF;
                color.a = 0xFF;
            } else {
                color.b = 0x63;
                color.g = 0x4C;
                color.r = 0x4C;
                color.a = 0xFF;
            }

            DayNight::SetOverrideFog(0.7f, 150.0f, color, 0);
            FFX::SetEffect(CGWorldFrame::ffxEffectNetherWorld);
            break;
        }

        case 3:
            if (g_theGxDevicePtr->DeviceApi()) {
                DayNight::ClearOverrideFog();
                PassFogSeed::DeviceRestoredCallback();
                CGWorldFrame::ffxEffectSwirlingFog->SetParameters(3, rec->m_param);
                FFX::SetEffect(CGWorldFrame::ffxEffectSwirlingFog);
            } else {
                DayNight::ClearOverrideFog();
                FFX::SetEffect(CGWorldFrame::ffxEffectGlow);
            }

            break;

        default:
            break;
    }

    DayNight::SetOverrideLightParamsID(rec->m_lightParamsID);
    SI2::SetOverrideZoneSounds(rec->m_soundAmbienceID, rec->m_zoneMusicID);
}

// VERIFY: 01c7ec1b manual
// OFFSET: 0x004F8730
int32_t CGWorldFrame::Update(const void* data, void* param) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        player->UpdateActiveMoverDisplay(OsGetAsyncTimeMs());
    }

    return 1;
}

// VERIFY: 256f3626 manual
// OFFSET: 0x004F88B0
void CGWorldFrame::UpdateScreenEffect() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        SpellRec spell;

        for (uint32_t i = player->GetAuraCount(); i > 0; i--) {
            AuraData* aura = player->GetAura(i - 1);

            if (!aura->m_spellId || !g_spellDB.GetRecord(aura->m_spellId, &spell)) {
                continue;
            }

            for (uint32_t j = 0; j < 3; j++) {
                if (spell.m_effectAura[j] == SPELL_AURA_SCREEN_EFFECT) {
                    CGWorldFrame::SetScreenEffect(spell.m_effectMiscValue[j]);
                    return;
                }
            }
        }

        if ((player->m_player->m_playerFlags & 0x10) && CGBattlefieldInfo::m_instanceType != 4) {
            CGWorldFrame::SetScreenEffect(1);
            return;
        }

        if (player->m_player->m_playerFieldBytes2 & 0x40000000) {
            CGWorldFrame::SetScreenEffect(81);
            return;
        }
    }

    CGWorldFrame::SetScreenEffect(0);
}

// VERIFY: 8df0efa3 manual
// OFFSET: 0x004F5A90
void CGWorldFrame::OnFrameSizeChanged(const CRect& rect) {
    this->CSimpleFrame::OnFrameSizeChanged(rect);

    this->m_viewportDDC = this->m_rect;
    this->m_viewportDDC.minX = this->m_viewportDDC.minX > 0.0f ? this->m_viewportDDC.minX : 0.0f;
    this->m_viewportDDC.minY = this->m_viewportDDC.minY > 0.0f ? this->m_viewportDDC.minY : 0.0f;
    this->m_viewportDDC.maxX = NDCToDDCWidth(1.0f) > this->m_viewportDDC.maxX ? this->m_viewportDDC.maxX : NDCToDDCWidth(1.0f);
    this->m_viewportDDC.maxY = NDCToDDCHeight(1.0f) > this->m_viewportDDC.maxY ? this->m_viewportDDC.maxY : NDCToDDCHeight(1.0f);

    if (this->m_camera) {
        this->m_camera->SetScreenAspect(this->m_viewportDDC);
    }

    DDCToNDC(this->m_rect.minX, this->m_rect.minY, &this->m_viewportNDC.minX, &this->m_viewportNDC.minY);
    DDCToNDC(this->m_rect.maxX, this->m_rect.maxY, &this->m_viewportNDC.maxX, &this->m_viewportNDC.maxY);
    this->m_viewportNDC.minX = this->m_viewportNDC.minX > 0.0f ? this->m_viewportNDC.minX : 0.0f;
    this->m_viewportNDC.minY = this->m_viewportNDC.minY > 0.0f ? this->m_viewportNDC.minY : 0.0f;
    this->m_viewportNDC.maxX = this->m_viewportNDC.maxX < 1.0f ? this->m_viewportNDC.maxX : 1.0f;
    this->m_viewportNDC.maxY = this->m_viewportNDC.maxY < 1.0f ? this->m_viewportNDC.maxY : 1.0f;
}

// VERIFY: 0cb92e2e manual
// OFFSET: 0x004F6560
static int32_t ObjectCollisionProc(WOWGUID* guid, uintptr_t param, World::ObjectCollisionData* data, uint32_t flags) {
    CGObject_C* object = ClntObjMgrObjectPtr(*guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object || !object->IsCollidable(flags) || !(object->m_obj->m_type & TYPE_GAMEOBJECT)) {
        return 0;
    }

    data->m_model = object->GetObjectModel();
    data->m_bounds = static_cast<CGGameObject_C*>(object)->GetWorldBounds();
    data->m_scale = object->GetTrueScale();
    data->m_mat = object->GetMatrix();

    uint64_t transport = object->GetTransportGUID();

    if (transport) {
        guid->m_guid = transport;
    }

    return 1;
}

// VERIFY: bed13d63 manual
// OFFSET: 0x004F6970
static int32_t UnitUpdateProc(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return 1;
    }

    switch (object->m_obj->m_type) {
        case HEIR_TYPE_UNIT:
        case HEIR_TYPE_PLAYER:
            static_cast<CGUnit_C*>(object)->UpdateDisplay(FrameTime::s_curTimeMs);
            break;

        case HEIR_TYPE_GAMEOBJECT:
            static_cast<CGGameObject_C*>(object)->UpdateDisplay(FrameTime::s_curTimeMs);
            break;

        case HEIR_TYPE_DYNAMICOBJECT:
            static_cast<CGDynamicObject_C*>(object)->UpdateDisplay(FrameTime::s_curTimeMs);
            break;

        default:
            break;
    }

    return 1;
}

// VERIFY: bee9c240 manual
// OFFSET: 0x004F6A40
static int32_t UnitPostAnimateProc(WOWGUID guid, void* param) {
    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (object && (object->m_obj->m_type & TYPE_UNIT)) {
        static_cast<CGUnit_C*>(object)->PostAnimate(static_cast<CGWorldFrame*>(param));
    }

    return 1;
}

// VERIFY: f55fe239 manual
// OFFSET: 0x004F6A90
static C3Vector GetFacingVector(CGObject_C* object) {
    float facing = object->GetFacing();
    return C3Vector(cosf(facing), sinf(facing), 0.0f);
}

// VERIFY: 5b70d219 manual
// OFFSET: 0x004F8A40
static void DrawSpellCursorShadow(uint32_t a1) {
    if (!s_spellShadowTexture[s_spellShadowStyle]) {
        return;
    }

    CGxTex* texture = TextureGetGxTex(s_spellShadowTexture[s_spellShadowStyle], 0, nullptr);

    if (!texture) {
        return;
    }

    if (Spell_C_GetSpellWorldObject() && !s_spellShadowStyle) {
        return;
    }

    float size = s_spellShadowSize;

    if (size == 0.0f) {
        size = 1.3888888f;
    }

    CAaBox box;
    box.t = C3Vector(s_spellShadowPos.x + size, s_spellShadowPos.y + size, s_spellShadowPos.z + 2.0f);
    box.b = C3Vector(s_spellShadowPos.x - size, s_spellShadowPos.y - size, s_spellShadowPos.z - 2.0f);

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, ProjectTex2dGetAddFadeTex());
    GxRsSet(GxRs_ColorOp0, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp0, GxTexOp_Mod);
    GxRsSet(GxRs_ColorOp1, GxTexOp_Mod);
    GxRsSet(GxRs_AlphaOp1, GxTexOp_Mod);

    CImVector color;
    color.b = 0xFF;
    color.g = 0xFF;
    color.r = 0xFF;
    color.a = 0xFF;
    ProjectTex::Render2d(box, color, nullptr, 0.5f, a1, 5, 0.4f);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: b5c2c6d4 manual
// OFFSET: 0x004F8410
void CGWorldFrame::UpdateDayNight(float deltaSec) {
    DayNight::DNInfo* info = DayNight::GetInfo();
    info->m_farClip = this->m_camera->m_farZ;
    info->m_deltaSec = deltaSec;
    info->m_timeSec = OsGetAsyncTimeMs() * 0.001f;
    info->m_cameraPos = this->m_camera->m_position;
    info->m_cameraDir = this->m_camera->Forward();

    float scale = 1.0f / sqrtf(info->m_cameraDir.z * info->m_cameraDir.z + info->m_cameraDir.y * info->m_cameraDir.y + info->m_cameraDir.x * info->m_cameraDir.x);
    info->m_cameraDir.x = info->m_cameraDir.x * scale;
    info->m_cameraDir.y = info->m_cameraDir.y * scale;
    info->m_cameraDir.z = scale * info->m_cameraDir.z;

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (target) {
        info->m_lightRefPos = target->GetPosition();
    } else {
        info->m_lightRefPos = info->m_cameraPos;
    }

    WOWGUID activePlayer = ClntObjMgrGetActivePlayer();
    CGPlayer_C* player = activePlayer ? static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(activePlayer, TYPE_PLAYER, __FILE__, __LINE__)) : nullptr;

    if (player) {
        info->m_playerPos = player->GetPosition();
    }

    if (player && (player->m_unit->m_unitFieldFlags & 0x100000) && (player->m_player->m_playerFlags & 0x20000)) {
        info->m_timeOfDay = 0;
        info->m_dayProgression = 0.5f;
        info->m_day = 0.0f;
    } else {
        info->m_timeOfDay = g_clientGameTime.GetHourAndMinutes();
        info->m_dayProgression = g_clientGameTime.GameTimeGetDayProgression();
        info->m_day = static_cast<float>(g_clientGameTime.GetDaysSinceEpoch());
    }

    MapRec* map = g_mapDB.GetRecord(World::QueryMapId());

    if (map->m_timeOfDayOverride != -1) {
        info->m_timeOfDay = 0;
        info->m_dayProgression = map->m_timeOfDayOverride * (1.0f / 1440.0f);
        info->m_day = 0.0f;

        if (info->m_dayProgression < 0.0f || info->m_dayProgression > 1.0f) {
            SysMsgPrintf(SYSMSG_ERROR, "Invalid time of day override in Map ID %d. TOD = %d.", World::QueryMapId(), map->m_timeOfDayOverride);
            info->m_dayProgression = 0.5f;
        }
    }

    info->m_weatherIntensity = World::s_weather->m_densityBlend * World::s_weather->m_fogIntensity;
}

// VERIFY: 1381402d manual
// OFFSET: 0x004F8D10
void CGWorldFrame::UpdateObject(CGObject_C* object, uint32_t flags) {
    CM2Model* model = object->GetObjectModel();

    if (!model) {
        return;
    }

    int32_t render = 0;
    int32_t a3 = 0;
    object->ShouldRender(flags, render, a3);
    int32_t visible = render || a3 ? 0 : 1;

    object->PreAnimate(this);

    if (!object->Animate(this->m_elapsedSec)) {
        return;
    }

    if ((object->m_obj->m_type & (TYPE_UNIT | TYPE_GAMEOBJECT | TYPE_CORPSE)) && visible && !(object->m_modelFlags & 0x100000)) {
        CModelRecord* record;

        if (this->m_freeModelList.IsEmpty()) {
            record = this->m_modelList.NewNode(2, 0, 0);
        } else {
            record = this->m_freeModelList.Head();
            this->m_freeModelList.UnlinkNode(record);
            this->m_modelList.LinkToTail(record);
        }

        CM2Model* recordModel = object->GetObjectModel();
        record->m_model = recordModel;
        recordModel->m_refCount++;
        record->m_guid = object->m_obj->m_guid;
    }

    uint32_t bit = visible & 1;

    if (model->m_attachParent) {
        model->m_fFlags = (model->m_fFlags & ~0x80) | (bit << 7);
    } else {
        model->m_fFlags = (model->m_fFlags & ~0x8) | (bit << 3);
    }

    if (model->m_attachParent) {
        model->m_fFlags = (model->m_fFlags & ~0x20000) | (bit << 17);
    } else {
        model->m_fFlags = (model->m_fFlags & ~0x10000) | (bit << 16);
    }
}

// VERIFY: 2ea40d3b manual
// OFFSET: 0x004F8EA0
void CGWorldFrame::OnWorldRender() {
    CRect window = { 0.0f, 0.0f, 0.0f, 0.0f };
    g_theGxDevicePtr->CapsWindowSize(window);

    if (0.0f == window.maxY - window.minY || window.maxX - window.minX == 0.0f) {
        return;
    }

    if (!(this->m_viewportNDC.maxY > this->m_viewportNDC.minY && this->m_viewportNDC.maxX > this->m_viewportNDC.minX)) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_Multisample, 1);
    CGWorldFrame::UpdateScreenEffectParams();
    FFX::Start();

    CRect fullViewport = { 0.0f, 0.0f, 1.0f, 1.0f };

    if (this->m_viewportNDC != fullViewport) {
        CImVector black;
        black.b = 0x00;
        black.g = 0x00;
        black.r = 0x00;
        black.a = 0xFF;
        GxSceneClear(GxBufferBit_Color | GxBufferBit_Depth, black);
    }

    if (CWorld::enables & 0x20000000) {
        g_theGxDevicePtr->MasterEnableSet(GxMasterEnable_PolygonFill, 0);
    }

    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;
    GxXformViewport(minX, maxX, minY, maxY, minZ, maxZ);

    CGxTex* renderTarget;
    g_theGxDevicePtr->DeviceGetRenderTarget(GxBuffers_Color, renderTarget);

    float width = maxX - minX;
    float height = maxY - minY;
    float top;
    float bottom;

    if (renderTarget) {
        top = 1.0f - this->m_viewportNDC.minY;
        bottom = 1.0f - this->m_viewportNDC.maxY;
    } else {
        top = this->m_viewportNDC.maxY;
        bottom = this->m_viewportNDC.minY;
    }

    GxXformSetViewport(width * this->m_viewportNDC.minX + minX, this->m_viewportNDC.maxX * width + minX, height * bottom + minY, top * height + minY, minZ, CWorld::s_unk_ADEEE4);
    TextureCacheResetLoadCount();
    PlayerNameUpdateEarly();

    if (this->m_namePlateFlags & 0x2) {
        CGUnit_C::NamePlateSetResortAll(true);
        this->m_namePlateFlags &= ~0x2;
    }

    CShaderEffect::UpdateProjMatrix();
    CWorld::Render(this->m_camera->m_position, this->m_elapsedSec);
    CMap::RenderFootprints();
    ClntObjMgrEnumVisibleObjects(UnitPostAnimateProc, this);
    CGUnit_C::NamePlateSetResortAll(false);
    CGUnit_C::NamePlateUpdateLevels(false);
    CGUnit_C::NamePlateUpdateColor(false);

    if (this->m_namePlateFlags & 0x1) {
        SmartScreenRectClearGrid(SRECTGRID_NAMEPLATES);
        CGUnit_C::UnitNameplatesReposition(this);
        this->m_namePlateFlags &= ~0x1;
    }

    CM2Scene* scene = World::s_m2Scene;

    if (!scene) {
        return;
    }

    scene->SetFloat18(-CWorld::s_unk_CD7758);
    scene->m_passMask = CWorld::m_M2PassMask;
    scene->Draw(M2PASS_0);

    if (s_spellShadowStyle < NUM_SPELL_SHADOWS) {
        DrawSpellCursorShadow(0x200122);
    }

    GxuSortDrawList(GxuCat_0, GxuSort_VtxStream, *World::GetCamPos());
    GxuFlushDrawList(GxuCat_0, *World::GetCamPos());
    CWorldScene::RenderDetailDoodads();

    if (World::SceneCamLiquidStatus(nullptr)) {
        CWorldScene::RenderBarriers(this->m_elapsedSec);
        scene->Draw(M2PASS_1);
        CWorldScene::RenderWeather();
        CWorldScene::RenderLiquidPass();
        scene->Draw(M2PASS_2);
    } else {
        scene->Draw(M2PASS_2);
        CWorldScene::RenderLiquidPass();
        CWorldScene::RenderWeather();
        CWorldScene::RenderBarriers(this->m_elapsedSec);
        scene->Draw(M2PASS_1);
    }

    if (s_spellShadowStyle < NUM_SPELL_SHADOWS) {
        DrawSpellCursorShadow(0x20000);
    }

    SpellVisualsTick();
    SpellVisualsRender();
    CGUnit_C::RenderMissileTrajectoryTargetingTexture();
    this->ClearTrackedObjects();
    CWorldScene::RenderParticulate();
    GxuSortDrawList(GxuCat_1, GxuSort_B2F, *World::GetCamPos());
    GxuFlushDrawList(GxuCat_1, *World::GetCamPos());
    DayNight::RenderGlares();
    PlayerNameUpdateLate();

    if (CWorld::enables & 0x20000000) {
        GxMasterEnableSet(GxMasterEnable_PolygonFill, 1);
    }

    g_theGxDevicePtr->RsPop();
    SmartScreenRectClearGrid(SRECTGRID_WORLDTEXT);
    CGChatBubbleFrame::OnWorldRender(this);
    GxXformSetViewport(minX, maxX, minY, maxY, minZ, maxZ);
    FFX::Finish();
    CVehiclePassenger_C::PostRenderUpdateAll();
}

// VERIFY: 79b17fb9 manual
// OFFSET: 0x004F9F70
int32_t CGWorldFrame::ObjectEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32) {
    CGWorldFrame* frame = CGWorldFrame::s_currentWorldFrame;
    CGObject_C* object = ClntObjMgrObjectPtr(WOWGUID{ guid }, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return 1;
    }

    if (flags & 4) {
        object->m_modelFlags |= 0x800000;
    } else {
        object->m_modelFlags &= ~0x800000;
    }

    if (!(object->m_modelFlags & 0x10000) && object->IsReadyToDraw()) {
        frame->UpdateObject(object, flags);
        return 1;
    }

    if (object->GetObjectModel()) {
        CM2Model* model = object->GetObjectModel();

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x80;
        } else {
            model->m_fFlags &= ~0x8;
        }

        if (model->m_attachParent) {
            model->m_fFlags &= ~0x20000;
        } else {
            model->m_fFlags &= ~0x10000;
        }
    }

    return 1;
}

// VERIFY: 5ab100e2 manual
// OFFSET: 0x004FA040
void CGWorldFrame::OnLayerUpdate(float elapsedSec) {
    this->CSimpleFrame::OnLayerUpdate(elapsedSec);
    CGChatBubbleFrame::OnWorldLayerUpdate();

    CSimpleTop* top = this->m_simpleTop;
    HITTESTRESULT hit;
    hit.m_point = C3Vector(0.0f, 0.0f, 0.0f);
    hit.m_guid = WOWGUID();
    hit.m_distance = 0.0f;
    hit.m_segStart = C3Vector(0.0f, 0.0f, 0.0f);
    hit.m_segEnd = C3Vector(0.0f, 0.0f, 0.0f);
    int32_t kind = -1;
    CSimpleFrame* focus = top->m_mouseFocus;

    if (focus == this) {
        C2Vector ddc = { 0.0f, 0.0f };
        NDCToDDC(top->m_mousePosition.m_x, top->m_mousePosition.m_y, &ddc.x, &ddc.y);

        if (Spell_C_IsTargeting() && Spell_C_CanTargetTerrain() && !Spell_C_CanTargetUnits()) {
            CGUnit_C::ClearNamePlateFocus();
        } else {
            CGUnit_C::SetNamePlateFocus(ddc);
        }

        if (CGNamePlateFrame::GetNamePlateFocus()) {
            kind = HIT_OBJECT;
            hit.m_guid = CGNamePlateFrame::GetNamePlateFocus()->m_guid;
        } else {
            kind = this->HitTestPoint(ddc.x, ddc.y, 1, &hit);
        }
    } else if (focus && (focus->m_layoutFlags & 0x10000)) {
        lua_State* L = FrameScript_GetContext();
        const char* savedTaint = lua_tainted;

        if (++lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = nullptr;
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, FrameScript_GetErrorHandlerReference());
        lua_pushstring(L, "SecureButton_GetModifiedUnit");
        lua_rawget(L, LUA_GLOBALSINDEX);

        if (!focus->m_luaRegistered) {
            focus->RegisterScriptObject(nullptr);
        }

        lua_rawgeti(L, LUA_REGISTRYINDEX, focus->m_luaObjectRef);
        lua_pushstring(L, CGUIMacros::m_macroRunning);

        if (!lua_pcall(L, 2, 1, -4)) {
            const char* token = lua_tolstring(L, -1, nullptr);

            if (token && !lua_tainted && Script_GetGUIDFromToken(token, hit.m_guid, false)) {
                kind = HIT_OBJECT;
            }
        }

        lua_settop(L, -3);

        if (lua_taintexpected && !lua_taintedclosure) {
            lua_tainted = savedTaint;
        }

        if (--lua_taintexpected <= 0) {
            lua_taintexpected = 0;
        }
    }

    s_spellShadowStyle = SPELL_NONE;

    switch (kind) {
        case HIT_NONE:
            if (Spell_C_IsTargeting()) {
                CursorSetMode(CAST_ERROR_CURSOR);
            } else {
                CursorResetMode();
            }

            this->HandleSpriteTrack(WOWGUID());
            break;

        case HIT_GROUND:
            this->OnLayerTrackTerrain(hit);
            break;

        case HIT_OBJECT:
            this->OnLayerTrackObject(hit);
            break;

        case HIT_WORLD_OBJECT: {
            CGGameObject_C* gameObject = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(hit.m_guid, TYPE_GAMEOBJECT, __FILE__, __LINE__));

            if (!gameObject || gameObject->m_gameObject->m_typeID != 33 || Spell_C_CanTargetTerrain()) {
                this->OnLayerTrackTerrain(hit);
                break;
            }

            CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

            if (player && gameObject->m_obj->m_guid.m_guid == World::QueryObjectParentParam64(player->m_worldObject)) {
                this->OnLayerTrackTerrain(hit);
                break;
            }

            this->OnLayerTrackObject(hit);
            break;
        }

        default:
            break;
    }

    CGInputControl::GetActive()->OnUpdate(elapsedSec);
    this->m_elapsedSec = elapsedSec;
    CGGameUI::UpdateInteractTarget();
    CGGameUI::UpdateCorpseDistance();
    CGGameUI::UpdateAreaSpiritHealerDistance();
    CGMailInfo::UpdatePendingMail(elapsedSec);
}

// VERIFY: b7190175 manual
// OFFSET: 0x004FA5F0
void CGWorldFrame::OnWorldUpdate() {
    float elapsedSec = this->m_elapsedSec;
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (CGUnit_C::s_activeMover) {
        CGObject_C* mover = ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__);

        if (mover) {
            CBarrier::SetViewerPos(mover->GetPosition());
        }
    }

    CGObject_C* target = ClntObjMgrObjectPtr(this->m_camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__);

    if (player && player->IsCommentatorUberOrInArena()) {
        if (this->m_camera->m_targetGUID) {
            this->m_camera->SetTarget(nullptr, 0);
        }

        CGCommentator::s_Commentator.OnUpdate(this->m_elapsedSec);
    } else if (!ClntObjMgrObjectPtr(this->m_camera->m_targetGUID, TYPE_OBJECT, __FILE__, __LINE__) && !ClntObjMgrObjectPtr(this->m_camera->m_guid0090, TYPE_OBJECT, __FILE__, __LINE__)) {
        CGPlayer_C* activePlayer = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

        if (activePlayer) {
            if (activePlayer->m_combatModeFlags & 0x1) {
                activePlayer->ToggleFarSight(nullptr);
            }

            this->m_camera->SetTarget(activePlayer, 0);
            activePlayer->OnVehicleCameraPossiblyNeeded();
        }
    }

    if (target && target->m_worldObject) {
        CWorldScene::SetCameraTarget(target->m_worldObject, WOWGUID{ target->GetTransportGUID() });
    }

    CVehiclePassenger_C::UpdateAll(FrameTime::s_curTimeMs);
    CGCamera::UpdateCallback(nullptr, this->m_camera);

    CGCamera* camera = this->m_camera;
    C3Vector camPos = camera->m_position;
    C3Vector forward = camera->Forward();
    C3Vector camTarget = C3Vector(camera->m_position.x + forward.x, camera->m_position.y + forward.y, camera->m_position.z + forward.z);
    this->m_camera->SetupWorldProjection(this->m_viewportDDC);
    this->UpdateDayNight(elapsedSec);

    if (!this->m_camera->HasModelCamera() && s_listenerAtCharacterVar && s_listenerAtCharacterVar->m_intValue) {
        CGObject_C* focus = CGObject_C::GetFocusObject();

        if (focus) {
            // STATIC: 0x00B743B8
            static C3Vector s_listenerUp = C3Vector(0.0f, 0.0f, 1.0f);

            C3Vector facing = (focus->m_obj->m_type & TYPE_UNIT) ? GetFacingVector(focus) : this->m_camera->Forward();

            // STATIC: 0x00B743B4
            static CVar* s_listenerBackDist = CVar::Lookup("Sound_ListenerBackDist");
            // STATIC: 0x00B743B0
            static CVar* s_listenerUpDist = CVar::Lookup("Sound_ListenerUpDist");

            float backDist = 2.0f;
            float upDist = 4.0f;

            if (s_listenerBackDist) {
                backDist = s_listenerBackDist->m_floatValue;
            }

            if (s_listenerUpDist) {
                upDist = s_listenerUpDist->m_floatValue;
            }

            C3Vector offset = C3Vector(facing.x * backDist, facing.y * backDist, backDist * facing.z);
            C3Vector listener;

            if (focus->m_worldModel && (focus->m_worldModel->m_fFlags & 0x8000)) {
                listener = focus->m_worldModel->GetWorldPosition() - offset;
                listener.z = listener.z + upDist;
            } else {
                C3Vector position = focus->GetPosition();
                listener = C3Vector(position.x - offset.x, position.y - offset.y, position.z - offset.z + upDist);
            }

            SI2::SetListenerAttributes(&listener, &facing, &s_listenerUp, nullptr);
        } else {
            C3Vector up = this->m_camera->Up();
            C3Vector cameraForward = this->m_camera->Forward();
            SI2::SetListenerAttributes(&camPos, &cameraForward, &up, nullptr);
        }
    } else {
        C3Vector up = this->m_camera->Up();
        C3Vector cameraForward = this->m_camera->Forward();
        SI2::SetListenerAttributes(&camPos, &cameraForward, &up, nullptr);
    }

    CGObject_C::UpdateAllObjectEffectManagers();
    UpdatePendingPortraits();
    this->MoveToFreeList(&this->m_modelList);
    this->MoveToFreeList(&this->m_hitModelList);
    ClntObjMgrEnumVisibleObjects(UnitUpdateProc, nullptr);
    World::SetObjectCollisionCallback(ObjectCollisionProc);
    CEffect::CleanUpStaticLists();
    CMissile::UpdateStaticLists();
    Spell_C_ExpireDestLocSpellCasts(FrameTime::s_curTimeMs);
    Spell_C_UpdateInTransitList();

    C3Vector position = (this->m_camera->m_model || !target) ? camPos : target->GetPosition();
    CWorld::Update(&camPos, &camTarget, &position);

    C44Matrix viewProjection;
    GxXformViewProj(viewProjection);

    if (viewProjection != this->m_viewProjection) {
        this->m_namePlateFlags |= 0x2;
        this->m_viewProjection = viewProjection;
    }

    ClientUpdateTaxiBenchmarkMode();
    SI2::UpdateTaxiSounds(elapsedSec);
    SI2::DSP_MakeDSPsExpire(elapsedSec);

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover) {
        mover->UpdateMissileTrajectory();
    }

    CGUnit_C::UpdateAllSmoothFacing();
    CheckEventWorldStateUITimerUpdate();
}

// VERIFY: 09c5961f manual
// OFFSET: 0x004FAF90
void CGWorldFrame::RenderWorld(void* param) {
    CGWorldFrame* worldFrame = static_cast<CGWorldFrame*>(param);
    C44Matrix projection;
    C44Matrix view;
    projection = g_theGxDevicePtr->m_projection;
    GxXformView(view);

    worldFrame->OnWorldUpdate();
    PlayerNameUpdateWorldText();
    worldFrame->OnWorldRender();
    WorldTextRenderBatch();

    g_theGxDevicePtr->XformSetProjection(projection);
    g_theGxDevicePtr->XformSetView(view);
    CShaderEffect::UpdateProjMatrix();
}

// VERIFY: 3e6634ba auto
// OFFSET: 0x004FB080
void CGWorldFrame::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    this->CSimpleFrame::OnFrameRender(batch, layer);

    if (!layer) {
        batch->QueueCallback(CGWorldFrame::RenderWorld, this);
    }
}

// VERIFY: 1cd14340 auto
// OFFSET: 0x004F5D30
void CGWorldFrame::OnMouseModeRelative() {
    WOWGUID tracked = this->m_trackedGuid;
    this->m_worldFlags |= 0x2;

    if (tracked) {
        CObjectTrackEvent evt;
        evt.m_guid = WOWGUID();
        evt.m_prevGuid = tracked;
        CGGameUI::HandleSpriteTrack(evt);
        this->m_trackedGuid = WOWGUID();
    }
}

// VERIFY: bef9479f manual
// OFFSET: 0x004F7880
void CGWorldFrame::PerformDefaultAction(MOUSEBUTTON button) {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || player->m_unit->m_unitFieldCharmedby) {
        CWorldClickEvent evt;
        evt.m_segStart = C3Vector(0.0f, 0.0f, 0.0f);
        evt.m_button = button;
        evt.m_segEnd = C3Vector(0.0f, 0.0f, 0.0f);
        CGGameUI::HandleWorldClick(evt);
        return;
    }

    CGNamePlateFrame* namePlate = CGNamePlateFrame::GetNamePlateFocus();

    if (namePlate) {
        if (button == 1) {
            CGGameUI::OnSpriteLeftClick(namePlate->m_guid);
        } else {
            CGGameUI::OnSpriteRightClick(namePlate->m_guid);
        }

        return;
    }

    switch (this->m_defaultActionHitKind) {
        case HIT_NONE: {
            CWorldClickEvent evt;
            evt.m_segStart = this->m_defaultActionHit.m_segStart;
            evt.m_segEnd = this->m_defaultActionHit.m_segEnd;
            evt.m_button = button;
            CGGameUI::HandleWorldClick(evt);
            break;
        }

        case HIT_GROUND:
        case HIT_WORLD_OBJECT: {
            CTerrainClickEvent evt;
            uint32_t high = static_cast<uint32_t>(this->m_defaultActionHit.m_guid.m_guid >> 32);

            if ((high & 0xF0000000) == 0x10000000 && (high & 0xFF00000) == 0xFC00000) {
                evt.m_guid = this->m_defaultActionHit.m_guid;
            } else {
                evt.m_guid = WOWGUID();
            }

            evt.m_point = this->m_defaultActionHit.m_point;
            evt.m_button = button;
            CGGameUI::HandleTerrainClick(evt);
            break;
        }

        case HIT_OBJECT: {
            CSpriteClickEvent evt;
            evt.m_guid = this->m_defaultActionHit.m_guid;
            evt.m_button = button;
            CGGameUI::HandleSpriteClick(evt);
            break;
        }

        default:
            break;
    }
}

// VERIFY: 46a37201 auto
// OFFSET: 0x004F5980
int32_t CGWorldFrame::HandleSpriteTrack(WOWGUID guid) {
    WOWGUID tracked = this->m_trackedGuid;

    if (guid == tracked) {
        return 0;
    }

    CObjectTrackEvent evt;
    evt.m_guid = guid;
    evt.m_prevGuid = tracked;
    CGGameUI::HandleSpriteTrack(evt);
    this->m_trackedGuid = guid;
    return 1;
}

// VERIFY: 15b5df38 manual
// OFFSET: 0x004F5FB0
static const char* KeyCodeToString(KEY key) {
    if (key - 33 <= 0xDE) {
        sputu8(key, s_keyNameBuffer);
        return s_keyNameBuffer;
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
            SStrPrintf(s_keyNameBuffer, sizeof(s_keyNameBuffer), "NUMPAD%d", key - KEY_NUMPAD0);
            return s_keyNameBuffer;
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
            SStrPrintf(s_keyNameBuffer, sizeof(s_keyNameBuffer), "F%d", key - KEY_F1 + 1);
            return s_keyNameBuffer;
        default:
            return "UNKNOWN";
    }
}

// VERIFY: 00570459 auto
// OFFSET: 0x004F5C10
void CGWorldFrame::OnLayerCursorExit(int32_t motion, int32_t a3) {
    this->CSimpleFrame::OnLayerCursorExit(motion, a3);
    WOWGUID tracked = this->m_trackedGuid;

    if (tracked) {
        CObjectTrackEvent evt;
        evt.m_guid = WOWGUID();
        evt.m_prevGuid = tracked;
        CGGameUI::HandleSpriteTrack(evt);
        this->m_trackedGuid = WOWGUID();
    }

    CursorResetMode();
    s_spellShadowStyle = SPELL_NONE;
}

// VERIFY: f81c29ba auto
// OFFSET: 0x004F6AE0
int32_t CGWorldFrame::OnLayerKeyDown(const CKeyEvent& evt) {
    if (this->CSimpleFrame::OnLayerKeyDown(evt)) {
        return 1;
    }

    KEY key = evt.m_data.m_key;

    if (key >= KEY_LAST) {
        return 0;
    }

    if (key <= KEY_RALT) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MODIFIER_STATE_CHANGED, "%s%d", KeyCodeToString(key), 1);
        return 1;
    }

    KEYDOWNSTATE& state = this->m_keyDown[key];

    if (!CGUIBindings::KeyEventToString(evt, state.m_keyString, sizeof(state.m_keyString))) {
        return 0;
    }

    state.m_modifiers = evt.m_data.m_metaKeyState;
    return CGUIBindings::s_bindings->ExecKey(state.m_modifiers, state.m_keyString, 1, 1, static_cast<BINDING_MODE>(4));
}

// VERIFY: e0914772 manual
// OFFSET: 0x004F6B70
int32_t CGWorldFrame::OnLayerKeyUp(const CKeyEvent& evt) {
    if (this->CSimpleFrame::OnLayerKeyUp(evt)) {
        return 1;
    }

    KEY key = evt.m_data.m_key;

    if (key >= KEY_LAST) {
        return 0;
    }

    if (key <= KEY_RALT) {
        FrameScript_SignalEvent(SCRIPT_EVENT_MODIFIER_STATE_CHANGED, "%s%d", KeyCodeToString(key), 0);
        return 1;
    }

    KEYDOWNSTATE& state = this->m_keyDown[key];
    int32_t result = 0;

    if (!state.m_keyString[0]) {
        state.m_modifiers = 0;
        CGUIBindings::KeyEventToString(evt, state.m_keyString, sizeof(state.m_keyString));
    }

    if (state.m_keyString[0]) {
        state.m_modifiers |= evt.m_data.m_metaKeyState;
        result = CGUIBindings::s_bindings->ExecKey(state.m_modifiers, state.m_keyString, 0, 1, static_cast<BINDING_MODE>(4));
        state.m_keyString[0] = 0;
    }

    return result;
}

// VERIFY: 0965c168 auto
// OFFSET: 0x004F6C10
int32_t CGWorldFrame::OnLayerMouseDown(const CMouseEvent& evt, const char* btn) {
    if (this->CSimpleFrame::OnLayerMouseDown(evt, btn) || CGGameUI::HandleMouseDown(evt)) {
        return 1;
    }

    KEYDOWNSTATE& state = this->m_mouseDown[MouseButtonToIndex(evt.m_data.m_button)];

    if (!CGUIBindings::MouseEventToString(evt, state.m_keyString, sizeof(state.m_keyString))) {
        return 0;
    }

    state.m_modifiers = evt.m_data.m_metaKeyState;
    return CGUIBindings::s_bindings->ExecKey(state.m_modifiers, state.m_keyString, 1, 1, static_cast<BINDING_MODE>(4));
}

// VERIFY: 513360ba manual
// OFFSET: 0x004F6C90
int32_t CGWorldFrame::OnLayerMouseUp(const CMouseEvent& evt, const char* btn) {
    if (this->CSimpleFrame::OnLayerMouseUp(evt, btn)) {
        return 1;
    }

    KEYDOWNSTATE& state = this->m_mouseDown[MouseButtonToIndex(evt.m_data.m_button)];
    int32_t result = 0;

    if (!state.m_keyString[0]) {
        state.m_modifiers = 0;
        CGUIBindings::MouseEventToString(evt, state.m_keyString, sizeof(state.m_keyString));
    }

    if (state.m_keyString[0]) {
        state.m_modifiers |= evt.m_data.m_metaKeyState;
        result = CGUIBindings::s_bindings->ExecKey(state.m_modifiers, state.m_keyString, 0, 1, static_cast<BINDING_MODE>(4));
        state.m_keyString[0] = 0;
    }

    return result;
}

// VERIFY: 14a0e78a manual
// OFFSET: 0x004FA570
int32_t CGWorldFrame::SetupDefaultAction() {
    NDCToDDC(this->m_simpleTop->m_mousePosition.m_x, this->m_simpleTop->m_mousePosition.m_y, &this->m_defaultActionPointDDC.x, &this->m_defaultActionPointDDC.y);
    this->m_defaultActionHitKind = this->HitTestPoint(this->m_defaultActionPointDDC.x, this->m_defaultActionPointDDC.y, 0, &this->m_defaultActionHit);
    return this->m_defaultActionHitKind;
}

// VERIFY: 4cfa29e1 manual
// OFFSET: 0x004F5C80
int32_t CGWorldFrame::OnLayerMouseWheel(const CMouseEvent& evt) {
    if (this->CSimpleFrame::OnLayerMouseWheel(evt) || !evt.m_data.m_wheelAmount) {
        return 1;
    }

    char binding[32];

    if (!CGUIBindings::MouseEventToString(evt, binding, sizeof(binding))) {
        return 0;
    }

    int32_t down = CGUIBindings::s_bindings->ExecKey(evt.m_data.m_metaKeyState, binding, 1, 1, static_cast<BINDING_MODE>(4));
    return CGUIBindings::s_bindings->ExecKey(evt.m_data.m_metaKeyState, binding, 0, 1, static_cast<BINDING_MODE>(4)) + down;
}

// VERIFY: 548e1cb9 manual
// OFFSET: 0x004F5910
int32_t CGWorldFrame::MissileEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32) {
    CMissile* missile = reinterpret_cast<CMissile*>(param32);
    return !missile || missile->m_mapEntity != nullptr;
}

// VERIFY: c40ba1e3 auto
// OFFSET: 0x004F5930
int32_t CGWorldFrame::SpellEffectEnumProc(void* param, uint32_t flags, uint64_t guid, uintptr_t param32) {
    CEffect* effect = reinterpret_cast<CEffect*>(param32);

    if (effect) {
        effect->UpdateWorldObject(CGWorldFrame::s_currentWorldFrame->m_elapsedSec, flags);
    }

    return 1;
}

// VERIFY: 57ad68af manual
// OFFSET: [0x004FA390, 0x004FAF60]
CGWorldFrame::~CGWorldFrame() {
    EventUnregisterEx(EVENT_ID_IDLE, CGWorldFrame::Update, this, 0xFFFFFFFF);
    EventSetMouseMode(MOUSE_MODE_NORMAL, 0);
    CGWorldFrame::s_currentWorldFrame = nullptr;

    for (uint32_t i = 0; i < sizeof(s_spellShadowTexture) / sizeof(s_spellShadowTexture[0]); i++) {
        if (s_spellShadowTexture[i]) {
            HandleClose(reinterpret_cast<HOBJECT>(s_spellShadowTexture[i]));
        }

        s_spellShadowTexture[i] = nullptr;
    }

    SmartScreenRectShutdown();
    Spell_C_ClearInTransitList();
    this->m_modelList.Clear();
    this->m_hitModelList.Clear();
    this->m_freeModelList.Clear();
    delete this->m_camera;
    this->m_camera = nullptr;

    if (CGWorldFrame::ffxEffectGlow) {
        delete CGWorldFrame::ffxEffectGlow;
    }

    if (CGWorldFrame::ffxEffectDeath) {
        delete CGWorldFrame::ffxEffectDeath;
    }

    if (CGWorldFrame::ffxEffectNetherWorld) {
        delete CGWorldFrame::ffxEffectNetherWorld;
    }

    if (CGWorldFrame::ffxEffectSwirlingFog) {
        delete CGWorldFrame::ffxEffectSwirlingFog;
    }

    FFX::Destroy();
}
