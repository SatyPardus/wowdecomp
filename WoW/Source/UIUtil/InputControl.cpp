#include <cmath>
#include <cstdint>
#include <new>
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/CFocusEvent.h"
#include "Engine/Source/Event/CMouseEvent.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Os/Joystick.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/SteelseriesMouse/WowMouse.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include "WoW/Source/DB/Rec/VehicleSeatRec.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/Object/ObjectClient/VehicleCamera_C.h"
#include "WoW/Source/Object/ObjectClient/VehiclePassenger_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/CommentatorFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/UIUtil/InputControl.h"

static const float MOUSE_DRAG_THRESHOLD = 8.0f;

static const float DELTAX_PER_SECOND = 800.0f;
static const float DELTAY_PER_SECOND = 600.0f;

// STATIC: 0x00AD192C
uint32_t s_joystick = 0xFFFFFFFF;

// STATIC: 0x00AD1930
int32_t s_joystickMouseEnabled = 1;

// STATIC: 0x00C24954
CGInputControl* CGInputControl::s_inputControl;

// STATIC: 0x00C24958
float s_vehicleAimNormPower;

// VERIFY: 13a95e59 auto
// OFFSET: 0x005F95D0
CGInputControl* CGInputControl::GetActive() {
    return CGInputControl::s_inputControl;
}

// VERIFY: b2b6f3af auto
// OFFSET: 0x005F9670
void CGInputControl::UpdateJoystickMouseMode() {
    int32_t enable = s_joystickMouseEnabled && !s_cvCinematicJoystick->m_intValue && (this->m_joystickMouseFlags & 0x2) && (this->m_mouseModeFlags & 0x1);

    if (enable == (this->m_joystickMouseFlags & 0x1)) {
        return;
    }

    if (enable) {
        this->m_joystickMouseFlags |= 0x1;
        OsJoystickSetMouseMode(s_joystick, enable);
    } else {
        this->m_joystickMouseFlags &= ~0x1;
        OsJoystickSetMouseMode(s_joystick, 0);
    }
}

// VERIFY: faf10efa manual
// OFFSET: 0x005F96F0
void CGInputControl::SetMouseCVarHandler(bool enable) {
    if (!this->m_wowMouse) {
        if (enable) {
            this->m_wowMouse = new (SMemAlloc(sizeof(WowMouse), __FILE__, __LINE__, 0)) WowMouse();
        }
    } else if (!enable) {
        delete this->m_wowMouse;
        this->m_wowMouse = nullptr;
    }
}

// VERIFY: 36bc2098 auto
// OFFSET: 0x005FD1D0
CGInputControl::CGInputControl() : m_flags(INPUT_NONE), m_lookStartTime(0), m_pendingDefaultAction(INPUT_RELEASE_NONE), m_facingOverrideActive(0), m_facingOverride(0.0f), m_vehicleAimValid(0), m_vehicleAim(0.0f), m_mouseModeFlags(3), m_forceCursorOn(0), m_joystickMouseFlags(3), m_joystickLookX(0.0f), m_joystickLookY(0.0f), m_wowMouse(nullptr) {
    this->m_time = OsGetAsyncTimeMs();
}

// VERIFY: 67293d46 manual
// OFFSET: 0x005FD2C0
void InputControlInitialize() {
    CVar::Register("Joystick", "enable joystick control", 0, "0", JoystickCallback, 5, false, nullptr, false);
    s_cvCinematicJoystick = CVar::Register("CinematicJoystick", "enable cinematic joystick control", 0, "0", nullptr, 5, false, nullptr, false);
    CGInputControl::s_inputControl = new (SMemAlloc(sizeof(CGInputControl), __FILE__, __LINE__, 0)) CGInputControl();
    s_cvEnableWowMouse = CVar::Register("enableWowMouse", "Enable Steelseries World of Warcraft Mouse", 1, "0", WowMouseCVarCallback, 5, false, nullptr, false);
}

// VERIFY: f53f4378 auto
// OFFSET: 0x005FA890
void CGInputControl::UpdateMouseMode(int32_t force) {
    uint32_t flags = this->m_mouseModeFlags;
    uint32_t cursorOn;

    if (flags & 0x20) {
        cursorOn = 1;
    } else if (flags & 0x40) {
        cursorOn = 0;
    } else if (s_cvCinematicJoystick->m_intValue || (flags & 0x10) || this->m_forceCursorOn) {
        cursorOn = 1;
    } else {
        cursorOn = flags & 0x6;
    }

    if (force || cursorOn != (flags & 0x1)) {
        if (cursorOn) {
            this->m_mouseModeFlags = flags | 0x1;

            if (!(this->m_flags & INPUT_MOUSE_LOOK_MASK)) {
                g_theGxDevicePtr->CursorSetVisible(1);
            }
        } else {
            this->m_mouseModeFlags = flags & ~0x1;
            g_theGxDevicePtr->CursorSetVisible(0);
        }
    }

    this->UpdateJoystickMouseMode();
}

// INLINED
CGInputControl::~CGInputControl() {
    if (this->m_wowMouse) {
        delete this->m_wowMouse;
    }

    this->m_wowMouse = nullptr;
}

// VERIFY: e79550cf manual
// OFFSET: 0x005FD3D0
void CGInputControl::Destroy() {
    if (CGInputControl::s_inputControl) {
        delete CGInputControl::s_inputControl;
    }

    CGInputControl::s_inputControl = nullptr;
}

// VERIFY: 3a24bb1c auto
// OFFSET: 0x005FD430
void InputControlDestroy() {
    CGInputControl::Destroy();
}

// VERIFY: fae954c6 auto
// OFFSET: 0x005F95E0
void CGInputControl::UpdateHalted() {
    this->m_flags &= ~INPUT_HALT_SENT_MASK;
}

// VERIFY: a87f5e2f auto
// OFFSET: 0x005F95F0
void CGInputControl::UpdateMoveStopped() {
    this->m_flags &= ~INPUT_MOVE_STOPPED_MASK;
}

// VERIFY: 4f6c9ca3 manual
// OFFSET: 0x005F9600
int32_t CGInputControl::IsMouseDrag(uint32_t time) const {
    uint32_t elapsed = time - this->m_lookStartTime;

    if (static_cast<int32_t>(elapsed - 800) >= 0) {
        return 1;
    }

    if (this->m_dragAccumX >= MOUSE_DRAG_THRESHOLD || this->m_dragAccumY >= MOUSE_DRAG_THRESHOLD) {
        return static_cast<int32_t>(elapsed - 200) >= 0;
    }

    return 0;
}

// VERIFY: b0dd26d4 auto
// OFFSET: 0x005F96E0
float CGInputControl::GetUIVehiclePower() {
    return s_vehicleAimNormPower;
}

// VERIFY: c8871d3b auto
// OFFSET: 0x005F9750
int32_t CGInputControl::WowMouseEnabled() {
    return s_cvEnableWowMouse && s_cvEnableWowMouse->m_intValue;
}

// VERIFY: 21c02d86 auto
// OFFSET: 0x005F9770
int32_t CGInputControl::WowMouseConnected() {
    return this->m_wowMouse && this->m_wowMouse->IsConnected();
}

// VERIFY: 60b26448 auto
// OFFSET: 0x005F9790
bool CGInputControl::DetectWowMouse() {
    s_cvEnableWowMouse->Set("1", true, false, false, true);

    if (this->m_wowMouse) {
        bool connected = this->m_wowMouse->IsConnected();

        if (connected) {
            return connected;
        }
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_WOW_MOUSE_NOT_FOUND, nullptr);
    return s_cvEnableWowMouse->Set("0", true, false, false, true);
}

// VERIFY: 7c49134b auto
// OFFSET: 0x005F9830
int32_t CGInputControl::IsMouseDragging() const {
    uint32_t flags = this->m_flags;

    return (flags & INPUT_TURN_PLAYER_MASK) && !(flags & INPUT_PLAYER_LOOK_MASK);
}

// VERIFY: f9f95644 manual
// OFFSET: 0x005F9850
int32_t CGInputControl::IsIdle() const {
    uint32_t flags = this->m_flags;

    return !(flags & INPUT_MOVE_WITH_AUTORUN_MASK) && !(flags & INPUT_STRAFE_PLAYER_MASK) && (!(flags & INPUT_PLAYER_LOOK_MASK) || !(flags & INPUT_TURN_PLAYER_MASK)) && (!(flags & INPUT_TURN_PLAYER_MASK) || (flags & INPUT_PLAYER_LOOK_MASK)) && !(flags & INPUT_TRACKING_MASK);
}

// VERIFY: de4cd81f manual
// OFFSET: 0x005F9890
XMLTree* LoadJoystickConfig(XMLNode*& config) {
    config = nullptr;

    if (s_joystick == 0xFFFFFFFF) {
        return nullptr;
    }

    char joystickName[1024];
    OsJoystickGetName(s_joystick, joystickName, sizeof(joystickName));

    void* buffer;
    uint32_t bytes;

    if (!SFile::ReadFile(nullptr, "Interface\\Joystick\\JoystickConfig.xml", &buffer, &bytes, 0, 1, nullptr)) {
        return nullptr;
    }

    XMLTree* tree = XMLTree_Load(static_cast<const char*>(buffer), bytes);
    SMemFree(buffer, __FILE__, __LINE__, 0);

    if (!tree) {
        return nullptr;
    }

    for (XMLNode* joystick = XMLTree_GetRoot(tree)->m_child; joystick; joystick = joystick->m_next) {
        if (SStrCmpI(joystick->m_name.m_str, "Joystick", STORM_MAX_STR)) {
            continue;
        }

        const char* platform = joystick->GetAttributeByName("platform");

        if (platform && SStrCmpI(platform, "windows", STORM_MAX_STR)) {
            continue;
        }

        for (XMLNode* child = joystick->m_child; child; child = child->m_next) {
            if (SStrCmpI(child->m_name.m_str, "name", STORM_MAX_STR)) {
                continue;
            }

            const char* name = child->GetAttributeByName("name");

            if (name && !SStrCmpI(name, joystickName, STORM_MAX_STR)) {
                config = joystick;
                return tree;
            }
        }
    }

    XMLTree_Free(tree);

    return nullptr;
}

// VERIFY: 62cd78e5 manual
// OFFSET: 0x005F9D20
CGUnit_C* GetVehicleUnitPtr() {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return nullptr;
    }

    if (!mover->m_vehiclePassenger || mover->m_vehiclePassenger->m_state != 3) {
        return mover;
    }

    CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ mover->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

    if (!vehicle || !vehicle->m_vehicle) {
        return mover;
    }

    VehicleSeatRec* seat = vehicle->m_vehicle->GetVehicleSeatRec(mover->m_movementData.m_transportSeat);

    if (!seat || !(seat->m_flags & 0x800)) {
        return mover;
    }

    return vehicle;
}

// VERIFY: 1144f12f auto
// OFFSET: 0x005FA060
int32_t __stdcall CGInputControl::CanControl(CGUnit_C* unit) {
    if (!unit || unit->m_unit->m_unitFieldHealth <= 0) {
        return 0;
    }

    CMoveSpline* spline = unit->m_movement->m_spline;

    if (spline && !(spline->m_flags & 0x400)) {
        return 0;
    }

    return !unit->AnimSuppressesMovement() && (!unit->IsActivePlayer() || !(static_cast<CGPlayer_C*>(unit)->m_combatModeFlags & 0x1)) && !unit->IsAlteredFormTransitionPreventingMovement();
}

// VERIFY: 44c9ffc3 manual
// OFFSET: 0x005FA6B0
int32_t CGInputControl::CameraCanTurnPlayer() const {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!(((mover->m_obj->m_type >> 4) & 1) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena())) {
        if (mover->m_unit->m_unitFieldHealth <= 0) {
            return 0;
        }

        CMoveSpline* spline = mover->m_movement->m_spline;

        if (spline && !(spline->m_flags & 0x400)) {
            return 0;
        }

        if ((mover->m_unit->m_unitFieldFlags & 0x40000) || mover->IsAlteredFormTransitionPreventingMovement() || mover->GetClientStandState()) {
            return 0;
        }

        if ((camera->m_targetGUID) != mover->m_obj->m_guid) {
            return 0;
        }
    }

    if (!(camera->m_state & 0x1)) {
        return 0;
    }

    return (this->m_flags & INPUT_PLAYER_LOOK_MASK) != 0;
}

// VERIFY: da98c994 manual
// OFFSET: 0x005FA910
void CGInputControl::SendUIVehicleAngleUpdate(float angle) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    VehicleRec* rec = mover->m_vehicle ? mover->m_vehicle->m_vehicleRec : nullptr;

    if (!rec) {
        return;
    }

    float pitchMin;
    float pitchMax;

    if (rec->m_flags & 0x40) {
        pitchMin = rec->m_pitchMin;
        pitchMax = rec->m_pitchMax;
    } else {
        pitchMin = -1.5707964f;
        pitchMax = 1.5707964f;
    }

    float range = pitchMax - pitchMin;
    float normalized = fabsf(range) <= 0.000099999997f ? 0.0f : (angle - pitchMin) / range;

    FrameScript_SignalEvent(SCRIPT_EVENT_VEHICLE_ANGLE_UPDATE, "%f%f", static_cast<double>(normalized), static_cast<double>(angle));
}

// VERIFY: 6da9c237 manual
// OFFSET: 0x005FB560
int32_t CGInputControl::IsVehicleSeatFlagSet(uint32_t flags) const {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    CGUnit_C* player = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || !player->m_vehiclePassenger || player->m_vehiclePassenger->m_state != 3) {
        return 0;
    }

    if (player->GetTransportGUID() != mover->m_obj->m_guid.m_guid && player->m_obj->m_guid != mover->m_obj->m_guid) {
        return 0;
    }

    CGUnit_C* vehicle = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(WOWGUID{ player->GetTransportGUID() }, TYPE_UNIT, __FILE__, __LINE__));

    if (!vehicle) {
        return 0;
    }

    VehicleSeatRec* seat = vehicle->GetVehicleSeatRec(player->m_movementData.m_transportSeat);

    return seat && (seat->m_flags & flags);
}

// VERIFY: b6a28681 manual
// OFFSET: 0x005FD440
void CGInputControl::SetMouselookOverrideBinding(const char* key, const char* binding) {
    if (!binding) {
        MOUSELOOKBINDING* entry = this->m_mouseLookBindings.Ptr(key);

        if (entry) {
            this->m_mouseLookBindings.Delete(entry);
        }

        return;
    }

    MOUSELOOKBINDING* entry = this->m_mouseLookBindings.Ptr(key);

    if (entry) {
        if (SStrCmpI(entry->m_binding, binding, STORM_MAX_STR)) {
            SMemFree(entry->m_binding, __FILE__, __LINE__, 0);
            entry->m_binding = SStrDupA(binding, __FILE__, __LINE__);
        }
    } else {
        entry = this->m_mouseLookBindings.New(key, 0, 0);
        entry->m_binding = SStrDupA(binding, __FILE__, __LINE__);
    }
}

// VERIFY: 82de0c4b manual
// OFFSET: 0x005FA0D0
int32_t __stdcall CGInputControl::CanTurn(CGUnit_C* unit) {
    return CGInputControl::CanControl(unit) && !(unit->m_unit->m_unitFieldFlags & 0x40000) && !unit->IsVehiclePreventingTurning();
}

// VERIFY: feb80d65 manual
// OFFSET: 0x005FA110
int32_t CGInputControl::CanSyncFreeLookFacing(CGUnit_C* unit) const {
    return CGInputControl::CanControl(unit) && !(unit->m_unit->m_unitFieldFlags & 0x40000) && !unit->IsVehiclePreventingTurning() && (this->m_flags & INPUT_PLAYER_LOOK_MASK) && !unit->GetClientStandState();
}

// VERIFY: 83a6aa8d manual
// OFFSET: 0x005FA420
int32_t CGInputControl::IsMouseDragMoving() const {
    int32_t result = this->IsMouseDrag(OsGetAsyncTimeMs());

    if (result) {
        return g_theGxDevicePtr->m_frameCount - this->m_lastMouseMoveFrame < 2;
    }

    return result;
}

// VERIFY: 262a3d11 manual
// OFFSET: 0x005FA790
int32_t CGInputControl::CameraCanPitchPlayer() {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (((mover->m_obj->m_type >> 4) & 1) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena()) {
        return 1;
    }

    if (mover->m_unit->m_unitFieldHealth <= 0) {
        return 0;
    }

    CMoveSpline* spline = mover->m_movement->m_spline;

    if (spline && !(spline->m_flags & 0x400)) {
        return 0;
    }

    if (mover->IsAlteredFormTransitionPreventingMovement() || (mover->m_unit->m_unitFieldFlags & 0x40000) || mover->GetClientStandState()) {
        return 0;
    }

    CMovement_C* movement = mover->m_movement;

    if ((movement->m_flags2 & 0x20) && !(movement->m_flags & 0x2200000)) {
        VehicleRec* rec = mover->GetVehicleRec();

        if (!rec || !(rec->m_flags & 0x40000)) {
            return 0;
        }
    }

    if ((camera->m_targetGUID) != mover->m_obj->m_guid) {
        return 0;
    }

    return 1;
}

// VERIFY: ed61b82c auto
// OFFSET: 0x005FAC60
void CGInputControl::EnterWorld() {
    uint32_t time = OsGetAsyncTimeMs();
    this->UnsetControlBit(INPUT_ALL, time, 0);
    this->m_time = time;
    this->m_joystickLookX = 0.0f;
    this->m_joystickLookY = 0.0f;
}

// VERIFY: 988e7786 manual
// OFFSET: 0x005FAC90
int32_t __stdcall CGInputControl::CanMove(CGUnit_C* unit) {
    if (!CGInputControl::CanControl(unit)) {
        return 0;
    }

    return unit->m_unit->m_unitFieldHealth > 0 && !(unit->m_movement->m_flags & 0x100A00) && unit->m_unit->m_unitFieldBytes1[0] != UNIT_DEAD && !unit->HasVehicleTransport();
}

// VERIFY: d0f5f494 auto
// OFFSET: 0x00568560
void __stdcall ScaleMouseDeltaByFov(float* dx, float* dy) {
    if (!CGWorldFrame::s_currentWorldFrame) {
        return;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (!camera) {
        return;
    }

    float fovScale = camera->FOV() * 0.81851113f;
    float clamped = 1.0f;

    if (fovScale < 1.0f) {
        clamped = 0.050000001f;

        if (fovScale > 0.050000001f) {
            clamped = fovScale;
        }
    }

    float scale = 0.050000001f + clamped * 0.94999999f;
    *dx = *dx * scale;
    *dy = scale * *dy;
}

// VERIFY: c6ecfaf7 manual
// OFFSET: 0x005FB4B0
void CGInputControl::VehicleAimChange(uint32_t time, float delta) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    float aim = this->m_vehicleAimValid ? this->m_vehicleAim : mover->GetPitch();
    this->SetPlayerPitch(mover, time, aim + delta);
}

// VERIFY: 384b5dd4 manual
// OFFSET: 0x005FB510
void CGInputControl::OnPitchToAngleStop() {
    this->m_vehicleAimValid = 0;

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover) {
        CGInputControl::SendUIVehicleAngleUpdate(mover->GetPitch());
    }
}

// VERIFY: b38d40e5 manual
// OFFSET: 0x005FAB70
void CGInputControl::OnUpdate(float elapsed) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    if (s_joystick != 0xFFFFFFFF && (this->m_flags & INPUT_JOYSTICK_LOOK_MASK)) {
        CGCamera* camera = CGWorldFrame::GetActiveCamera();

        if (camera) {
            // STATIC: 0x00C24964
            static C2Vector s_joystickLookScale = C2Vector(DELTAX_PER_SECOND, DELTAY_PER_SECOND);

            camera->UpdateFreeLookFacing(this->m_joystickLookX * s_joystickLookScale.x * elapsed, this->m_joystickLookY * s_joystickLookScale.y * elapsed, nullptr);

            if (this->CanSyncFreeLookFacing(mover)) {
                camera->SyncFreeLookFacing();
            }
        }
    }

    if (mover->m_movement->m_flags & 0xC0) {
        CGInputControl::SendUIVehicleAngleUpdate(mover->GetPitch());
    }
}

// VERIFY: 1ec103a0 manual
// OFFSET: 0x005FBA60
void CGInputControl::OnMouseMoveRel(const CMouseEvent& evt) {
    if (!this->m_flags) {
        return;
    }

    uint32_t frame = g_theGxDevicePtr->m_frameCount;
    float x = evt.m_data.m_mousePosition.x;
    float y = evt.m_data.m_mousePosition.y;
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover && (mover->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena()) {
        ScaleMouseDeltaByFov(&x, &y);
    }

    this->m_lastMouseMoveFrame = frame;
    int32_t allowPitch = 1;
    this->m_dragAccumX = std::fabs(x) + this->m_dragAccumX;
    this->m_dragAccumY = std::fabs(y) + this->m_dragAccumY;
    int32_t canSync = this->CanSyncFreeLookFacing(mover);

    if (canSync && mover && mover->m_vehicle && mover->m_vehicle->m_vehicleRec && mover->m_vehicle->IsSuppressingCameraPitchWhileMouseAiming()) {
        allowPitch = 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (allowPitch) {
        camera->UpdateFreeLookFacing(x, y, nullptr);
    } else {
        float pitchDelta;
        camera->UpdateFreeLookFacing(x, y, &pitchDelta);

        if (CGInputControl::CameraCanPitchPlayer()) {
            float aim = this->m_vehicleAimValid ? this->m_vehicleAim : mover->GetPitch();
            this->SetPlayerPitch(mover, OsGetAsyncTimeMs(), aim - pitchDelta);
            this->m_flags &= ~INPUT_PITCH_PLAYER_SENT;
        }
    }

    if (canSync) {
        camera->SyncFreeLookFacing();
    }
}

// VERIFY: 69f1241e manual
// OFFSET: 0x005FB260
void CGInputControl::CameraTurnPlayer(uint32_t time, float facing) {
    if (!this->CameraCanTurnPlayer()) {
        return;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    if ((mover->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena()) {
        CGCommentator::s_Commentator.m_facing = facing;
        return;
    }

    if (mover->m_vehicleCamera) {
        mover->ConvertVehicleCameraRelativeFacingToTransportRelativeFacing(facing);
    }

    if (mover->m_movement->m_flags2 & 0x8) {
        if (!this->m_facingOverrideActive || this->m_facingOverride != facing) {
            mover->OnTurnToAngleLocal(time, facing);
            this->m_facingOverride = facing;
        }

        if (!this->m_facingOverrideActive) {
            CGWorldFrame::GetActiveCamera()->IncIgnoreFacing();
            this->m_flags &= ~INPUT_TURN_PLAYER_SENT;
            this->m_facingOverrideActive = 1;
            return;
        }
    } else {
        mover->OnSetRawFacingLocal(time, facing);
        CVehiclePassenger_C* passenger = mover->m_vehiclePassenger;

        if (passenger && mover->m_vehicleCamera && mover->m_vehicleCamera->IsTransitioningFacing()) {
            passenger->LockWorldFacingNextFrame(mover->CGUnit::GetFacing(facing));
        }
    }

    this->m_flags &= ~INPUT_TURN_PLAYER_SENT;
}

// VERIFY: 083fadd0 auto
// OFFSET: 0x005FB3A0
void CGInputControl::SetPlayerPitch(CGUnit_C* unit, uint32_t time, float pitch) {
    VehicleRec* rec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (rec) {
        float min;
        float max;

        if (rec->m_flags & 0x40) {
            min = rec->m_pitchMin;
            max = rec->m_pitchMax;
        } else {
            min = -1.5707964f;
            max = 1.5707964f;
        }

        if (pitch < min) {
            pitch = min;
        } else if (max <= pitch) {
            pitch = max;
        }
    }

    if (unit->m_movement->m_flags2 & 0x10) {
        if (!this->m_vehicleAimValid || this->m_vehicleAim != pitch) {
            unit->OnPitchToAngleLocal(time, pitch);
            this->m_vehicleAim = pitch;
        }

        this->m_vehicleAimValid = 1;
    } else {
        unit->OnSetPitchLocal(time, pitch);

        if (rec && (rec->m_flags & 0x400)) {
            CGInputControl::SendUIVehicleAngleUpdate(pitch);
        }
    }

    CGUnit_C::PitchMissileTrajectoryTargetingNextFrame();
}

// VERIFY: e939e05f manual
// OFFSET: 0x005FBE70
void CGInputControl::CameraPitchPlayer(uint32_t time, float pitch) {
    if (!CGInputControl::CameraCanPitchPlayer()) {
        return;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    if ((mover->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena()) {
        CGCommentator::s_Commentator.m_pitch = pitch;
        return;
    }

    float value = -pitch;

    if (mover->m_vehicle && mover->m_vehicle->m_vehicleRec) {
        value = mover->m_vehicle->m_vehicleRec->m_mouseLookOffsetPitch - pitch;
    }

    this->SetPlayerPitch(mover, time, value);
    this->m_flags &= ~INPUT_PITCH_PLAYER_SENT;
}

// VERIFY: 673b2c02 auto
// OFFSET: 0x005F9650
void CGInputControl::OnTurnToAngleStop() {
    if (this->m_facingOverrideActive) {
        this->m_facingOverrideActive = 0;
        CGWorldFrame::GetActiveCamera()->DecIgnoreFacing();
    }
}

// VERIFY: bc220db8 manual
// OFFSET: 0x005FAA40
void CheckToCancelCurrentChannelSpell() {
    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));

    if (!player || player->m_obj->m_guid != CGUnit_C::s_activeMover) {
        return;
    }

    Spell_C_CancelCastingSpellByMovement();
    int32_t channelSpell = player->m_unit->m_unitChannelSpell;

    if (!channelSpell) {
        return;
    }

    SpellRec spell;

    if (g_spellDB.GetRecord(channelSpell, &spell) && (spell.m_channelInterruptFlags & 0x8)) {
        Spell_C_CancelChannelSpell(channelSpell);
    }
}

// VERIFY: a56d8811 manual
// OFFSET: 0x005FA170
int32_t CGInputControl::SetControlBit(uint32_t bit, uint32_t time) {
    if (bit & this->m_flags) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    uint32_t flags = this->m_flags;
    int32_t wasIdle = !(flags & INPUT_MOVE_WITH_AUTORUN_MASK) && !(flags & INPUT_STRAFE_PLAYER_MASK) && (!(flags & INPUT_PLAYER_LOOK_MASK) || !(flags & INPUT_TURN_PLAYER_MASK)) && (!(flags & INPUT_TURN_PLAYER_MASK) || (flags & INPUT_PLAYER_LOOK_MASK)) && !(flags & INPUT_TRACKING_MASK);
    uint32_t wasLooking = flags & INPUT_FREE_LOOK_MASK;
    uint32_t wasMouseDown = flags & INPUT_MOUSE_LOOK_MASK;
    int32_t wasBothButtons = (flags & INPUT_TURN_PLAYER) && (flags & INPUT_TURN_CAMERA);
    uint32_t wasRightLook = flags & INPUT_CAMERA_LOOK_MASK;
    uint32_t wasLeftLook = flags & INPUT_PLAYER_LOOK_MASK;
    int32_t wasMouseTurn = (flags & INPUT_PLAYER_LOOK_MASK) && (flags & INPUT_TURN_PLAYER_MASK);

    this->m_flags = flags | bit;

    if (!wasLooking && (this->m_flags & INPUT_FREE_LOOK_MASK)) {
        this->m_dragAccumX = 0.0f;
        this->m_dragAccumY = 0.0f;
        this->m_lookStartTime = time;
        camera->EnableFreeLook();
    }

    if (!wasMouseDown && (this->m_flags & INPUT_MOUSE_LOOK_MASK)) {
        CGGameUI::OnMouseModeRelative();
    }

    if (!wasBothButtons && (this->m_flags & INPUT_TURN_PLAYER) && (this->m_flags & INPUT_TURN_CAMERA)) {
        CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

        if (this->CanSyncFreeLookFacing(mover)) {
            camera->SyncFreeLookFacing();
            this->m_freeLookFacingSynced = 1;
        }
    } else if (!(this->m_flags & INPUT_TURN_PLAYER) || !(this->m_flags & INPUT_TURN_CAMERA)) {
        this->m_freeLookFacingSynced = 0;
    }

    if (bit & INPUT_TRACKING_MASK) {
        camera->UpdateTrackingState(this->m_flags & INPUT_TRACKING_MASK);
    }

    if ((bit & INPUT_BOBBING_MASK) || (!wasBothButtons && (this->m_flags & INPUT_TURN_PLAYER) && (this->m_flags & INPUT_TURN_CAMERA)) || (!wasMouseTurn && (this->m_flags & INPUT_PLAYER_LOOK_MASK) && (this->m_flags & INPUT_TURN_PLAYER_MASK))) {
        uint32_t newFlags = this->m_flags;
        camera->UpdateBobbingState((newFlags & INPUT_BOBBING_MASK) || ((newFlags & INPUT_TURN_PLAYER) && (newFlags & INPUT_TURN_CAMERA)) || ((newFlags & INPUT_PLAYER_LOOK_MASK) && (newFlags & INPUT_TURN_PLAYER_MASK)));
    }

    if (this->m_facingOverrideActive && (bit & INPUT_TURN_PLAYER_MASK)) {
        this->OnTurnToAngleStop();
    }

    if ((bit & INPUT_MOVE_STRAFE_AND_TURN_MASK) || (!wasRightLook && (this->m_flags & INPUT_CAMERA_LOOK_MASK)) || (!wasLeftLook && (this->m_flags & INPUT_PLAYER_LOOK_MASK)) || (!wasMouseTurn && (this->m_flags & INPUT_PLAYER_LOOK_MASK) && (this->m_flags & INPUT_TURN_PLAYER_MASK))) {
        camera->SmoothFreeLook(this, !wasIdle && this->IsIdle());
    }

    if (bit & INPUT_MOVE_PLAYER_MASK) {
        this->m_flags &= ~INPUT_MOVE_PLAYER_AUTORUN;
    }

    if (!wasBothButtons && (this->m_flags & INPUT_TURN_PLAYER) && (this->m_flags & INPUT_TURN_CAMERA)) {
        this->m_flags &= ~INPUT_MOVE_PLAYER_AUTORUN;
    }

    if ((bit & INPUT_MOUSE_LOOK_MASK) && (this->m_flags & INPUT_MOUSE_LOOK_MASK) != bit) {
        this->m_pendingDefaultAction = INPUT_RELEASE_NONE;
    }

    return 1;
}

// VERIFY: 308e3e2e manual
// OFFSET: 0x005FA450
int32_t CGInputControl::UnsetControlBit(uint32_t bit, uint32_t time, int32_t keepSmoothing) {
    if (!(bit & this->m_flags)) {
        return 0;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    uint32_t flags = this->m_flags;
    uint32_t wasLooking = flags & INPUT_FREE_LOOK_MASK;
    uint32_t wasMouseDown = flags & INPUT_MOUSE_LOOK_MASK;
    int32_t wasIdle = !(flags & INPUT_MOVE_WITH_AUTORUN_MASK) && !(flags & INPUT_STRAFE_PLAYER_MASK) && (!(flags & INPUT_PLAYER_LOOK_MASK) || !(flags & INPUT_TURN_PLAYER_MASK)) && (!(flags & INPUT_TURN_PLAYER_MASK) || (flags & INPUT_PLAYER_LOOK_MASK)) && !(flags & INPUT_TRACKING_MASK);
    int32_t wasBothButtons = (flags & INPUT_TURN_PLAYER) && (flags & INPUT_TURN_CAMERA);
    uint32_t wasRightLook = flags & INPUT_CAMERA_LOOK_MASK;
    uint32_t wasLeftLook = flags & INPUT_PLAYER_LOOK_MASK;
    int32_t wasMouseTurn = (flags & INPUT_PLAYER_LOOK_MASK) && (flags & INPUT_TURN_PLAYER_MASK);

    this->m_flags = flags & ~bit;

    if (wasLooking && !(this->m_flags & INPUT_FREE_LOOK_MASK)) {
        camera->DisableFreeLook(keepSmoothing);

        if (this->m_facingOverrideActive && (this->m_flags & INPUT_TURN_PLAYER_MASK)) {
            this->OnTurnToAngleStop();
        }
    }

    if (wasMouseDown && !(this->m_flags & INPUT_MOUSE_LOOK_MASK)) {
        CGGameUI::OnMouseModeNormal();
    }

    if (bit & INPUT_TRACKING_MASK) {
        camera->UpdateTrackingState(this->m_flags & INPUT_TRACKING_MASK);
    }

    if ((bit & INPUT_BOBBING_MASK) || (wasBothButtons && (!(this->m_flags & INPUT_TURN_PLAYER) || !(this->m_flags & INPUT_TURN_CAMERA))) || (wasMouseTurn && (!(this->m_flags & INPUT_PLAYER_LOOK_MASK) || !(this->m_flags & INPUT_TURN_PLAYER_MASK)))) {
        uint32_t newFlags = this->m_flags;
        camera->UpdateBobbingState((newFlags & INPUT_BOBBING_MASK) || ((newFlags & INPUT_TURN_PLAYER) && (newFlags & INPUT_TURN_CAMERA)) || ((newFlags & INPUT_PLAYER_LOOK_MASK) && (newFlags & INPUT_TURN_PLAYER_MASK)));
    }

    if ((bit & INPUT_MOVE_STRAFE_AND_TURN_MASK) || (wasRightLook && !(this->m_flags & INPUT_CAMERA_LOOK_MASK)) || (wasLeftLook && !(this->m_flags & INPUT_PLAYER_LOOK_MASK)) || (wasMouseTurn && (!(this->m_flags & INPUT_PLAYER_LOOK_MASK) || !(this->m_flags & INPUT_TURN_PLAYER_MASK)))) {
        camera->SmoothFreeLook(this, !wasIdle && this->IsIdle());
    }

    if (wasMouseDown && !(this->m_flags & INPUT_MOUSE_LOOK_MASK)) {
        if (!this->IsMouseDrag(time)) {
            if (this->m_pendingDefaultAction == INPUT_RELEASE_SELECT) {
                CGWorldFrame::s_currentWorldFrame->PerformDefaultAction(MOUSE_BUTTON_LEFT);
            } else if (this->m_pendingDefaultAction == INPUT_RELEASE_ACTION) {
                CGWorldFrame::s_currentWorldFrame->PerformDefaultAction(MOUSE_BUTTON_RIGHT);
            }
        }

        this->m_pendingDefaultAction = INPUT_RELEASE_NONE;
    }

    return 1;
}

// VERIFY: b867df74 auto
// OFFSET: 0x005FAC60
void CGInputControl::Reset() {
    uint32_t time = OsGetAsyncTimeMs();
    this->UnsetControlBit(INPUT_ALL, time, 0);
    this->m_time = time;
    this->m_joystickLookX = 0.0f;
    this->m_joystickLookY = 0.0f;
}

// VERIFY: 67911eda manual
// OFFSET: 0x005FACE0
void CGInputControl::AscendDescendPlayer(uint32_t time, CGUnit_C* unit) {
    CMovement_C* movement = unit->m_movement;
    uint32_t moveFlags = movement->m_flags;

    if (!(moveFlags & 0x2200000)) {
        this->m_flags &= ~INPUT_ASCEND_DESCEND_PLAYER_SENT;
        return;
    }

    uint32_t flags = this->m_flags;
    int32_t direction = (flags & INPUT_ASCEND_PLAYER_KEY) != 0;

    if (flags & INPUT_DESCEND_PLAYER_KEY) {
        direction--;
    }

    if (movement->m_flags2 & 0x10) {
        if (direction > 0) {
            if (!(flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) || (moveFlags & 0x80)) {
                unit->OnPitchStartLocal(time, 1);
                this->m_flags |= INPUT_ASCEND_DESCEND_PLAYER_SENT;
            }
        } else if (direction < 0) {
            if (!(flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) || (moveFlags & 0x40)) {
                unit->OnPitchStartLocal(time, 0);
                this->m_flags |= INPUT_ASCEND_DESCEND_PLAYER_SENT;
            }
        } else if (flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) {
            unit->OnPitchStopLocal(time);
            this->m_flags &= ~INPUT_ASCEND_DESCEND_PLAYER_SENT;
            CGInputControl::SendUIVehicleAngleUpdate(unit->GetPitch());
        }

        return;
    }

    if (!direction) {
        if (flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) {
            unit->OnAscendDescendStopLocal(time);
            this->m_flags &= ~INPUT_ASCEND_DESCEND_PLAYER_SENT;
        }

        return;
    }

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::ClearAFK(0);
    }

    if (direction > 0) {
        if (!(this->m_flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) || (unit->m_movement->m_flags & 0x800000)) {
            unit->OnAscendDescendStartLocal(time, 1);
            this->m_flags |= INPUT_ASCEND_DESCEND_PLAYER_SENT;
        }
    } else {
        if (!(this->m_flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) || (unit->m_movement->m_flags & 0x400000)) {
            unit->OnAscendDescendStartLocal(time, 0);
            this->m_flags |= INPUT_ASCEND_DESCEND_PLAYER_SENT;
        }
    }
}

// VERIFY: af3364f8 manual
// OFFSET: 0x005FAE70
void CGInputControl::MovePlayer(uint32_t time, CGUnit_C* unit) {
    int32_t direction = 0;

    if ((unit->m_unit->m_unitFieldFlags2 & 0x40) || unit->IsVehicleCurrentlyUnstoppable()) {
        direction = 1;
    } else {
        uint32_t flags = this->m_flags;

        if (flags & INPUT_MOVE_PLAYER_AUTORUN) {
            direction = 1;
        }

        if (flags & INPUT_MOVE_PLAYER_FORWARD_KEY) {
            direction++;
        }

        if ((flags & INPUT_TURN_PLAYER) && (flags & INPUT_TURN_CAMERA)) {
            direction++;
        }

        if (flags & INPUT_MOVE_PLAYER_BACKWARD_KEY) {
            direction--;
        }

        if (!direction) {
            if (flags & INPUT_MOVE_PLAYER_SENT) {
                unit->OnMoveStopLocal(time);
                this->m_flags &= ~INPUT_MOVE_PLAYER_SENT;
            }

            return;
        }
    }

    if (unit->GetClientStandState()) {
        unit->ChangeStandState(UNIT_STANDING);
        CGCamera* camera = CGWorldFrame::GetActiveCamera();

        if (this->CanSyncFreeLookFacing(unit)) {
            camera->SyncFreeLookFacing();
        }
    }

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::ClearAFK(0);
    }

    if (direction > 0) {
        if (!(this->m_flags & INPUT_MOVE_PLAYER_SENT) || (unit->m_movement->m_flags & 0x2)) {
            unit->OnMoveStartLocal(time, 1);
            this->m_flags |= INPUT_MOVE_PLAYER_SENT;
        }
    } else {
        if (!(this->m_flags & INPUT_MOVE_PLAYER_SENT) || (unit->m_movement->m_flags & 0x1)) {
            unit->OnMoveStartLocal(time, 0);
            this->m_flags |= INPUT_MOVE_PLAYER_SENT;
        }
    }
}

// VERIFY: 02452f4c manual
// OFFSET: 0x005FAFB0
void CGInputControl::StrafePlayer(uint32_t time, CGUnit_C* unit) {
    uint32_t flags = this->m_flags;
    int32_t direction = (flags & INPUT_STRAFE_PLAYER_LEFT_KEY) != 0;

    if ((flags & INPUT_PLAYER_LOOK_MASK) && (flags & INPUT_TURN_PLAYER_LEFT_KEY)) {
        direction++;
    }

    if (flags & INPUT_STRAFE_PLAYER_RIGHT_KEY) {
        direction--;
    }

    if ((flags & INPUT_PLAYER_LOOK_MASK) && (flags & INPUT_TURN_PLAYER_RIGHT_KEY)) {
        direction--;
    }

    if (!direction) {
        if (flags & INPUT_STRAFE_PLAYER_SENT) {
            unit->OnStrafeStopLocal(time);
            this->m_flags &= ~INPUT_STRAFE_PLAYER_SENT;
        }

        return;
    }

    if (unit->GetClientStandState()) {
        unit->ChangeStandState(UNIT_STANDING);
    }

    if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
        CGPlayer_C::ClearAFK(0);
    }

    if (direction > 0) {
        if (!(this->m_flags & INPUT_STRAFE_PLAYER_SENT)) {
            unit->OnStrafeStartLocal(time, 1);
            this->m_flags |= INPUT_STRAFE_PLAYER_SENT;
        }
    } else {
        if (!(this->m_flags & INPUT_STRAFE_PLAYER_SENT)) {
            unit->OnStrafeStartLocal(time, 0);
            this->m_flags |= INPUT_STRAFE_PLAYER_SENT;
        }
    }
}

// VERIFY: f065952b manual
// OFFSET: 0x005FB0B0
void CGInputControl::TurnPlayer(uint32_t time, CGUnit_C* unit) {
    uint32_t flags = this->m_flags;
    int32_t direction = (flags & INPUT_TURN_PLAYER_LEFT_KEY) != 0;

    if (flags & INPUT_TURN_PLAYER_RIGHT_KEY) {
        direction--;
    }

    if ((!(flags & INPUT_PLAYER_LOOK_MASK) || unit->IsStrafingDisabled()) && direction) {
        if (unit->GetClientStandState()) {
            unit->ChangeStandState(UNIT_STANDING);
        }

        if (ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__)) {
            CGPlayer_C::ClearAFK(0);
        }

        if (direction > 0) {
            if (!(this->m_flags & INPUT_TURN_PLAYER_SENT)) {
                unit->OnTurnStartLocal(time, 1);
                this->m_flags |= INPUT_TURN_PLAYER_SENT;
            }
        } else {
            if (!(this->m_flags & INPUT_TURN_PLAYER_SENT)) {
                unit->OnTurnStartLocal(time, 0);
                this->m_flags |= INPUT_TURN_PLAYER_SENT;
            }
        }
    } else if (this->m_flags & INPUT_TURN_PLAYER_SENT) {
        unit->OnTurnStopLocal(time);
        this->m_flags &= ~INPUT_TURN_PLAYER_SENT;
    }
}

// VERIFY: 391fcf83 manual
// OFFSET: 0x005FB1A0
void CGInputControl::PitchPlayer(uint32_t time, CGUnit_C* unit) {
    uint32_t flags = this->m_flags;

    if (flags & INPUT_PLAYER_LOOK_MASK) {
        return;
    }

    int32_t direction = (flags & INPUT_PITCH_PLAYER_UP_KEY) != 0;

    if (flags & INPUT_PITCH_PLAYER_DOWN_KEY) {
        direction--;
    }

    if (direction > 0) {
        if (!(flags & INPUT_PITCH_PLAYER_SENT)) {
            unit->OnPitchStartLocal(time, 1);
            this->m_flags |= INPUT_PITCH_PLAYER_SENT;
        }
    } else if (direction < 0) {
        if (!(flags & INPUT_PITCH_PLAYER_SENT)) {
            unit->OnPitchStartLocal(time, 0);
            this->m_flags |= INPUT_PITCH_PLAYER_SENT;
        }
    } else if (flags & INPUT_PITCH_PLAYER_SENT) {
        unit->OnPitchStopLocal(time);
        this->m_flags &= ~INPUT_PITCH_PLAYER_SENT;
        CGInputControl::SendUIVehicleAngleUpdate(unit->GetPitch());
    }
}

// VERIFY: c0d2e3b9 manual
// OFFSET: 0x005FBBC0
void CGInputControl::UpdatePlayer(uint32_t time, int32_t a2) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return;
    }

    if ((mover->m_obj->m_type & TYPE_PLAYER) && static_cast<CGPlayer_C*>(mover)->IsCommentatorUberOrInArena()) {
        CGCommentator::s_Commentator.UpdateCameraVelocity(this->m_flags, a2);
        return;
    }

    uint32_t lastUpdateTime;

    if (MovementGetLastUpdateTime(&lastUpdateTime) && static_cast<int32_t>(time - lastUpdateTime) < 0) {
        time = lastUpdateTime;
    }

    bool canMove = a2 && CGInputControl::CanMove(mover);
    bool canTurn = a2 && CGInputControl::CanTurn(mover);

    if (canMove) {
        this->MovePlayer(time, mover);
        this->StrafePlayer(time, mover);
        this->AscendDescendPlayer(time, mover);
    } else {
        if (!canTurn) {
            mover->ClearTrackingTarget(false, true);
        }

        if (this->m_flags & INPUT_MOVE_PLAYER_SENT) {
            if (!mover->m_movement->IsOnSpline()) {
                mover->OnMoveStopLocal(time);
            }

            this->m_flags &= ~INPUT_MOVE_PLAYER_SENT;
        }

        if (this->m_flags & INPUT_STRAFE_PLAYER_SENT) {
            if (!mover->m_movement->IsOnSpline()) {
                mover->OnStrafeStopLocal(time);
            }

            this->m_flags &= ~INPUT_STRAFE_PLAYER_SENT;
        }

        if (this->m_flags & INPUT_MOVE_PLAYER_AUTORUN) {
            this->m_flags &= ~INPUT_MOVE_PLAYER_AUTORUN;
        }

        if (this->m_flags & INPUT_ASCEND_DESCEND_PLAYER_SENT) {
            if (!mover->m_movement->IsOnSpline()) {
                mover->OnAscendDescendStopLocal(time);
            }

            this->m_flags &= ~INPUT_ASCEND_DESCEND_PLAYER_SENT;
        }
    }

    if (canTurn) {
        this->TurnPlayer(time, mover);

        if ((mover->m_movement->m_flags & 0x2200000) || (mover->m_movement->m_flags2 & 0x20)) {
            this->PitchPlayer(time, mover);
        }
    } else {
        if (this->m_flags & INPUT_TURN_PLAYER_SENT) {
            if (!mover->m_movement->IsOnSpline()) {
                mover->OnTurnStopLocal(time);
            }

            this->m_flags &= ~INPUT_TURN_PLAYER_SENT;
        }

        if (this->m_flags & INPUT_PITCH_PLAYER_SENT) {
            if (!mover->m_movement->IsOnSpline() && (mover->m_movement->m_flags & 0x2200000)) {
                mover->OnPitchStopLocal(time);
            }

            this->m_flags &= ~INPUT_PITCH_PLAYER_SENT;
        }
    }
}

// VERIFY: ad249167 manual
// OFFSET: 0x005FBE10
void CGInputControl::ToggleControlBit(uint32_t bit, int32_t enable, uint32_t time, int32_t keepSmoothing) {
    int32_t changed = enable ? this->SetControlBit(bit, time) : this->UnsetControlBit(bit, time, keepSmoothing);

    if (!changed) {
        return;
    }

    if (enable && ((bit & INPUT_MOVE_AND_STRAFE_MASK) || ((bit & INPUT_MOUSE_LOOK_MASK) && (this->m_flags & INPUT_TURN_PLAYER) && (this->m_flags & INPUT_TURN_CAMERA)))) {
        CheckToCancelCurrentChannelSpell();
    }

    this->UpdatePlayer(time, 1);
}

// VERIFY: 2645b833 manual
// OFFSET: 0x005FBF10
void CGInputControl::StopMovementAndTurning(uint32_t time) {
    uint32_t bits = this->m_flags & INPUT_MOVE_AND_TURN_MASK;
    this->ToggleControlBit(bits, 0, time, 0);
    this->UpdatePlayer(time, 1);
    this->ToggleControlBit(bits, 1, time, 0);
}

// VERIFY: 2d993d52 auto
// OFFSET: 0x005F9540
uint32_t GetJoystick() {
    return s_joystick;
}

// VERIFY: 321bafa5 auto
// OFFSET: 0x005FC0F0
void DescendStart() {
    CGInputControl* inputControl = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (inputControl->SetControlBit(INPUT_DESCEND_PLAYER_KEY, time)) {
            inputControl->UpdatePlayer(time, 1);
        }
    }

    CheckToCancelCurrentChannelSpell();
}

// VERIFY: c131355d auto
// OFFSET: 0x005FC960
void CGInputControl::OnFocusChange(const CFocusEvent& event) {
    if (!event.m_data.m_focus) {
        this->m_flags &= ~INPUT_DIRECTION_KEYS_MASK;
        this->UpdatePlayer(OsGetAsyncTimeMs(), 1);
    }

    if (event.m_data.m_focus) {
        this->m_mouseModeFlags &= ~0x20u;
    } else {
        this->m_mouseModeFlags |= 0x20u;
    }

    this->UpdateMouseMode(0);
}

// VERIFY: 0826014e manual
// OFFSET: 0x005FCB20
void CGInputControl::SetMouselookOverrideBindings() {
    CGUIBindings* bindings = CGUIBindings::s_bindings;

    for (MOUSELOOKBINDING* entry = this->m_mouseLookBindings.m_fulllist.Head(); entry; entry = this->m_mouseLookBindings.Next(entry)) {
        bindings->SetOverrideBinding(this, 1, entry->m_key.m_str, entry->m_binding, false);
    }
}
