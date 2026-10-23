#include "InputControl.h"
#include <cstdint>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameXML/LoadXML.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/UIBindings.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/Object/ObjectClient/Vehicle_C.h"
#include "WoW/Source/DB/Rec/VehicleRec.h"
#include <cmath>
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Ui/Types.h"
#include "WoW/Source/UIUtil/Types.h"

// VERIFY: 46af036c manual
// OFFSET: 0x005FBF80
static int32_t Script_JumpOrAscendStart(lua_State* L) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_ASCEND_PLAYER_KEY, 1, CSimpleTop::s_eventTime, 0);

    if (mover->m_movement->m_flags & 0x2200000) {
        return 0;
    }

    CheckToCancelCurrentChannelSpell();

    if (mover->m_unit->m_unitFieldHealth <= 0 || mover->IsOnSpline() || mover->IsAlteredFormTransitionPreventingMovement() || !mover->CanMove() || mover->AnimSuppressesMovement()) {
        return 0;
    }

    uint32_t time = CSimpleTop::s_eventTime;

    if (mover->GetClientStandState()) {
        mover->ChangeStandState(UNIT_STANDING);
    }

    if (ClntObjMgrGetActivePlayerObj()) {
        CGPlayer_C::ClearAFK(0);
    }

    if (mover->GetCanFly()) {
        mover->OnFlightLocal(time, 1);
    } else {
        mover->TryJumpOrAscend(time);
    }

    return 0;
}

// VERIFY: 1ad525fe manual
// OFFSET: 0x005FC0A0
static int32_t Script_AscendStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_ASCEND_PLAYER_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: d72c7867 manual
// OFFSET: 0x005FC140
static int32_t Script_DescendStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_DESCEND_PLAYER_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 8076179f manual
// OFFSET: 0x005FAAE0
static int32_t Script_ToggleRun(lua_State* L) {
    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    uint32_t time = CSimpleTop::s_eventTime;

    if (mover->m_unit->m_unitFieldHealth > 0 && !mover->m_movement->IsOnSpline() && !mover->IsAlteredFormTransitionPreventingMovement() && mover->CanMove()) {
        mover->ToggleRunModeLocal(time);
    }

    return 0;
}

// VERIFY: d7467083 manual
// OFFSET: 0x005FC190
static int32_t Script_ToggleAutoRun(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;
    int32_t enable = !(input->m_flags & INPUT_MOVE_PLAYER_AUTORUN);

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_MOVE_PLAYER_AUTORUN, enable, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 8852ef68 manual
// OFFSET: 0x005FC200
static int32_t Script_MoveForwardStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_MOVE_PLAYER_FORWARD_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 662e594c manual
// OFFSET: 0x005FC250
static int32_t Script_MoveForwardStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_MOVE_PLAYER_FORWARD_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: f8b5bc47 manual
// OFFSET: 0x005FC290
static int32_t Script_MoveBackwardStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_MOVE_PLAYER_BACKWARD_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 3c08cf9a manual
// OFFSET: 0x005FC2E0
static int32_t Script_MoveBackwardStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_MOVE_PLAYER_BACKWARD_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 4e295838 manual
// OFFSET: 0x005FC320
static int32_t Script_TurnLeftStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 9ce3a768 manual
// OFFSET: 0x005FC360
static int32_t Script_TurnLeftStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_TURN_PLAYER_LEFT_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 7e560f07 manual
// OFFSET: 0x005FC3B0
static int32_t Script_TurnRightStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 519204d4 manual
// OFFSET: 0x005FC3F0
static int32_t Script_TurnRightStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_TURN_PLAYER_RIGHT_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 0be9d4b2 manual
// OFFSET: 0x005FC440
static int32_t Script_StrafeLeftStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_STRAFE_PLAYER_LEFT_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 8954b43d manual
// OFFSET: 0x005FC490
static int32_t Script_StrafeLeftStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_STRAFE_PLAYER_LEFT_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 7ddc8426 manual
// OFFSET: 0x005FC4D0
static int32_t Script_StrafeRightStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_STRAFE_PLAYER_RIGHT_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 080d2052 manual
// OFFSET: 0x005FC520
static int32_t Script_StrafeRightStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_STRAFE_PLAYER_RIGHT_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: bc849210 manual
// OFFSET: 0x005FC570
static int32_t Script_VehicleAimUpStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_PITCH_PLAYER_UP_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 994e08c2 manual
// OFFSET: 0x005FC5C0
static int32_t Script_VehicleAimDownStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_PITCH_PLAYER_DOWN_KEY, 0, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 03a4e142 manual
// OFFSET: 0x005FC8E0
static int32_t Script_VehicleAimUpStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_PITCH_PLAYER_UP_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: ddbbfc22 manual
// OFFSET: 0x005FC920
static int32_t Script_VehicleAimDownStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    input->ToggleControlBit(INPUT_PITCH_PLAYER_DOWN_KEY, 1, CSimpleTop::s_eventTime, 0);
    return 0;
}

// VERIFY: 1a4f0ed8 auto
// OFFSET: 0x005FC610
static int32_t Script_TurnOrActionStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (!(input->m_flags & INPUT_MOUSE_LOOK_MASK)) {
            input->m_pendingDefaultAction = INPUT_RELEASE_ACTION;
            CGWorldFrame::s_currentWorldFrame->SetupDefaultAction();
        }

        if (input->SetControlBit(INPUT_TURN_PLAYER, time)) {
            if ((input->m_flags & INPUT_TURN_PLAYER) && (input->m_flags & INPUT_TURN_CAMERA)) {
                CheckToCancelCurrentChannelSpell();
            }

            input->UpdatePlayer(time, 1);
        }
    }

    return 0;
}

// VERIFY: 984926de auto
// OFFSET: 0x005FC680
static int32_t Script_TurnOrActionStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (input->UnsetControlBit(INPUT_TURN_PLAYER, time, 0)) {
            input->UpdatePlayer(time, 1);
        }
    }

    return 0;
}

// VERIFY: a57bba4d auto
// OFFSET: 0x005FC6C0
static int32_t Script_CameraOrSelectOrMoveStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (!(input->m_flags & INPUT_MOUSE_LOOK_MASK)) {
            input->m_pendingDefaultAction = INPUT_RELEASE_SELECT;
            CGWorldFrame::s_currentWorldFrame->SetupDefaultAction();
        }

        if (input->SetControlBit(INPUT_TURN_CAMERA, time)) {
            if ((input->m_flags & INPUT_TURN_PLAYER) && (input->m_flags & INPUT_TURN_CAMERA)) {
                CheckToCancelCurrentChannelSpell();
            }

            input->UpdatePlayer(time, 1);
        }
    }

    return 0;
}

// VERIFY: 082fb5e1 auto
// OFFSET: 0x005FC730
static int32_t Script_CameraOrSelectOrMoveStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (input->UnsetControlBit(INPUT_TURN_CAMERA, time, StringToBOOL(L, 1, 0))) {
            input->UpdatePlayer(time, 1);
        }
    }

    return 0;
}

// VERIFY: c71b4971 auto
// OFFSET: 0x005FC890
static int32_t Script_MouselookStop(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;
    uint32_t time = CSimpleTop::s_eventTime;
    input->m_pendingDefaultAction = INPUT_RELEASE_NONE;

    if (input->UnsetControlBit(INPUT_TURN_PLAYER, time, 0)) {
        input->UpdatePlayer(time, 1);
    }

    CGUIBindings::s_bindings->ClearOverrideBindings(input, false);
    return 0;
}

// VERIFY: 0e9ff1d1 manual
// OFFSET: 0x005FCC10
static int32_t Script_MouselookStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;
    uint32_t time = CSimpleTop::s_eventTime;
    input->m_pendingDefaultAction = INPUT_RELEASE_NONE;

    if (input->SetControlBit(INPUT_TURN_PLAYER, time)) {
        if ((input->m_flags & INPUT_TURN_PLAYER) && (input->m_flags & INPUT_TURN_CAMERA)) {
            CheckToCancelCurrentChannelSpell();
        }

        input->UpdatePlayer(time, 1);
    }

    input->SetMouselookOverrideBindings();
    return 0;
}

// VERIFY: a6e9f837 auto
// OFFSET: 0x005F9550
static int32_t Script_VehicleAimGetNormPower(lua_State* L) {
    lua_pushnumber(L, s_vehicleAimNormPower);
    return 1;
}

// VERIFY: 07912041 auto
// OFFSET: 0x005F9DD0
static int32_t Script_IsMouselooking(lua_State* L) {
    if (CGInputControl::s_inputControl->m_flags & INPUT_PLAYER_LOOK_MASK) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 1c122e10 manual
// OFFSET: 0x005F9E10
static int32_t Script_VehicleAimGetAngle(lua_State* L) {
    CGUnit_C* vehicle = GetVehicleUnitPtr();

    if (vehicle) {
        lua_pushnumber(L, vehicle->GetPitch());
    } else {
        lua_pushnumber(L, 0.0);
    }

    return 1;
}

// VERIFY: b1728266 manual
// OFFSET: 0x005FB970
static int32_t Script_IsUsingVehicleControls(lua_State* L) {
    if (CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x800000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 5b29d740 auto
// OFFSET: 0x005FB9C0
static int32_t Script_CanExitVehicle(lua_State* L) {
    if (CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x2000000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 19ba3dd9 auto
// OFFSET: 0x005FBA10
static int32_t Script_CanSwitchVehicleSeats(lua_State* L) {
    if (CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x4000000)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 3a809a8e auto
// OFFSET: 0x005F9F10
static int32_t Script_VehicleAimSetNormPower(lua_State* L) {
    if (!GetVehicleUnitPtr() || !lua_isnumber(L, 1)) {
        return 0;
    }

    double power = lua_tonumber(L, 1);
    s_vehicleAimNormPower = static_cast<float>(power);

    if (power < 0.0) {
        s_vehicleAimNormPower = 0.0f;
    } else if (power >= 1.0) {
        s_vehicleAimNormPower = 1.0f;
    }

    return 0;
}

// VERIFY: 2026e785 auto
// OFFSET: 0x005FC830
static int32_t Script_MoveAndSteerStop(lua_State* L) {
    CGInputControl* inputControl = CGInputControl::s_inputControl;

    if (CGGameUI::CanPerformAction(0)) {
        uint32_t time = CSimpleTop::s_eventTime;

        if (inputControl->UnsetControlBit(INPUT_TURN_CAMERA, time, 0)) {
            inputControl->UpdatePlayer(time, 1);
        }

        if (inputControl->UnsetControlBit(INPUT_TURN_PLAYER, time, 0)) {
            inputControl->UpdatePlayer(time, 1);
        }
    }

    return 0;
}

// VERIFY: f929ce28 auto
// OFFSET: 0x005F9F70
static int32_t Script_IsVehicleAimAngleAdjustable(lua_State* L) {
    CGUnit_C* unit = GetVehicleUnitPtr();
    VehicleRec* rec = unit && unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (rec && (rec->m_flags & 0x400)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: c2775bae auto
// OFFSET: 0x005F9FE0
static int32_t Script_IsVehicleAimPowerAdjustable(lua_State* L) {
    CGUnit_C* unit = GetVehicleUnitPtr();
    VehicleRec* rec = unit && unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (rec && (rec->m_flags & 0x800)) {
        lua_pushnumber(L, 1.0);
    } else {
        lua_pushnil(L);
    }

    return 1;
}

// VERIFY: 2474fdc2 auto
// OFFSET: 0x005FD550
static int32_t Script_SetMouselookOverrideBinding(lua_State* L) {
    CGInputControl* inputControl = CGInputControl::s_inputControl;

    if (!lua_isstring(L, 1)) {
        luaL_error(L, "Usage: SetMouselookOverrideBinding(\"key\", \"binding\" or nil)");
        return 0;
    }

    if (CGGameUI::CanPerformAction(13)) {
        inputControl->SetMouselookOverrideBinding(lua_tolstring(L, 1, nullptr), lua_tolstring(L, 2, nullptr));
    }

    return 0;
}

// VERIFY: 1724ff8a auto
// OFFSET: 0x005F9E60
static int32_t Script_VehicleAimGetNormAngle(lua_State* L) {
    CGUnit_C* unit = GetVehicleUnitPtr();

    if (!unit) {
        lua_pushnumber(L, 0.0);
        return 1;
    }

    VehicleRec* rec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;
    float pitchMin;
    float pitchMax;

    if (rec && (rec->m_flags & 0x40)) {
        pitchMin = rec->m_pitchMin;
        pitchMax = rec->m_pitchMax;
    } else {
        pitchMin = -1.5707964f;
        pitchMax = 1.5707964f;
    }

    float range = pitchMax - pitchMin;
    float angle = fabs(range) <= 0.000099999997f ? 0.0f : (unit->GetPitch() - pitchMin) / range;
    lua_pushnumber(L, angle);
    return 1;
}

// VERIFY: 821e98a1 manual
// OFFSET: 0x005FB820
static int32_t Script_VehicleAimRequestAngle(lua_State* L) {
    CGInputControl* inputControl = CGInputControl::s_inputControl;
    uint32_t time = CSimpleTop::s_eventTime;
    CGUnit_C* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (unit) {
        VehicleRec* rec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

        if (rec && (rec->m_flags & 0x400) && lua_isnumber(L, 1)) {
            inputControl->SetPlayerPitch(unit, time, static_cast<float>(lua_tonumber(L, 1)));
        }
    }

    return 0;
}

// VERIFY: 6a8a12ad auto
// OFFSET: 0x005FB8C0
static int32_t Script_VehicleAimRequestNormAngle(lua_State* L) {
    CGInputControl* inputControl = CGInputControl::s_inputControl;
    uint32_t time = CSimpleTop::s_eventTime;
    CGUnit_C* unit = GetVehicleUnitPtr();

    if (!unit) {
        return 0;
    }

    VehicleRec* rec = unit->m_vehicle ? unit->m_vehicle->m_vehicleRec : nullptr;

    if (!rec || !(rec->m_flags & 0x400)) {
        return 0;
    }

    if (lua_isnumber(L, 1)) {
        double value = lua_tonumber(L, 1);
        float pitchMin;
        float pitchMax;

        if (rec->m_flags & 0x40) {
            pitchMin = rec->m_pitchMin;
            pitchMax = rec->m_pitchMax;
        } else {
            pitchMin = -1.5707964f;
            pitchMax = 1.5707964f;
        }

        inputControl->SetPlayerPitch(unit, time, static_cast<float>(value * (pitchMax - pitchMin) + pitchMin));
    }

    return 0;
}

// VERIFY: 2a01745d auto
// OFFSET: 0x005FA050
static int32_t Script_DetectWowMouse(lua_State* L) {
    CGInputControl::s_inputControl->DetectWowMouse();
    return 0;
}

// VERIFY: 67de4681 manual
// OFFSET: 0x005FB660
static int32_t Script_VehicleExit(lua_State* L) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (!mover) {
        return 0;
    }

    if (!CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x2000000)) {
        CGGameUI::DisplayError(ERR_SPELL_FAILED_S, FrameScript_GetText("SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW", -1, GENDER_NOT_APPLICABLE));
        return 0;
    }

    mover->VehicleExit();
    return 0;
}

// VERIFY: aedf0962 manual
// OFFSET: 0x005FB6D0
static int32_t Script_VehiclePrevSeat(lua_State* L) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover && CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x4000000)) {
        mover->VehiclePrevSeat();
    }

    return 0;
}

// VERIFY: 5481f00f manual
// OFFSET: 0x005FB720
static int32_t Script_VehicleNextSeat(lua_State* L) {
    CGUnit_C* mover = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(CGUnit_C::s_activeMover, TYPE_UNIT, __FILE__, __LINE__));

    if (mover && CGInputControl::s_inputControl->IsVehicleSeatFlagSet(0x4000000)) {
        mover->VehicleNextSeat();
    }

    return 0;
}

// VERIFY: f9b60516 auto
// OFFSET: 0x005FB770
static int32_t Script_VehicleAimIncrement(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGInputControl* input = CGInputControl::s_inputControl;

    if (lua_isnumber(L, 1)) {
        input->VehicleAimChange(time, static_cast<float>(lua_tonumber(L, 1)));
    } else {
        input->VehicleAimChange(time, 0.1f);
    }

    return 0;
}

// VERIFY: 40c0ff35 auto
// OFFSET: 0x005FB7D0
static int32_t Script_VehicleAimDecrement(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGInputControl* input = CGInputControl::s_inputControl;
    double delta = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 0.1;
    input->VehicleAimChange(time, static_cast<float>(-delta));
    return 0;
}

// VERIFY: 6579b2cf auto
// OFFSET: 0x005FC780
static int32_t Script_MoveAndSteerStart(lua_State* L) {
    CGInputControl* input = CGInputControl::s_inputControl;

    if (!CGGameUI::CanPerformAction(0)) {
        return 0;
    }

    uint32_t time = CSimpleTop::s_eventTime;

    if (!(input->m_flags & INPUT_MOUSE_LOOK_MASK)) {
        input->m_pendingDefaultAction = INPUT_RELEASE_SELECT;
        CGWorldFrame::s_currentWorldFrame->SetupDefaultAction();
    }

    if (input->SetControlBit(INPUT_TURN_CAMERA, time)) {
        if ((input->m_flags & INPUT_TURN_PLAYER) && (input->m_flags & INPUT_TURN_CAMERA)) {
            CheckToCancelCurrentChannelSpell();
        }

        input->UpdatePlayer(time, 1);
    }

    if (!(input->m_flags & INPUT_MOUSE_LOOK_MASK)) {
        input->m_pendingDefaultAction = INPUT_RELEASE_ACTION;
        CGWorldFrame::s_currentWorldFrame->SetupDefaultAction();
    }

    if (input->SetControlBit(INPUT_TURN_PLAYER, time)) {
        if ((input->m_flags & INPUT_TURN_PLAYER) && (input->m_flags & INPUT_TURN_CAMERA)) {
            CheckToCancelCurrentChannelSpell();
        }

        input->UpdatePlayer(time, 1);
    }

    return 0;
}

// STATIC: 0x00AD1938
static FrameScript_Method s_ScriptFunctions[] = {
    { "JumpOrAscendStart", &Script_JumpOrAscendStart },
    { "AscendStop", &Script_AscendStop },
    { "DescendStop", &Script_DescendStop },
    { "ToggleRun", &Script_ToggleRun },
    { "ToggleAutoRun", &Script_ToggleAutoRun },
    { "MoveForwardStart", &Script_MoveForwardStart },
    { "MoveForwardStop", &Script_MoveForwardStop },
    { "MoveBackwardStart", &Script_MoveBackwardStart },
    { "MoveBackwardStop", &Script_MoveBackwardStop },
    { "TurnLeftStart", &Script_TurnLeftStart },
    { "TurnLeftStop", &Script_TurnLeftStop },
    { "TurnRightStart", &Script_TurnRightStart },
    { "TurnRightStop", &Script_TurnRightStop },
    { "StrafeLeftStart", &Script_StrafeLeftStart },
    { "StrafeLeftStop", &Script_StrafeLeftStop },
    { "StrafeRightStart", &Script_StrafeRightStart },
    { "StrafeRightStop", &Script_StrafeRightStop },
    { "PitchUpStart", &Script_VehicleAimUpStart },
    { "PitchUpStop", &Script_VehicleAimUpStop },
    { "PitchDownStart", &Script_VehicleAimDownStart },
    { "PitchDownStop", &Script_VehicleAimDownStop },
    { "TurnOrActionStart", &Script_TurnOrActionStart },
    { "TurnOrActionStop", &Script_TurnOrActionStop },
    { "CameraOrSelectOrMoveStart", &Script_CameraOrSelectOrMoveStart },
    { "CameraOrSelectOrMoveStop", &Script_CameraOrSelectOrMoveStop },
    { "MoveAndSteerStart", &Script_MoveAndSteerStart },
    { "MoveAndSteerStop", &Script_MoveAndSteerStop },
    { "SetMouselookOverrideBinding", &Script_SetMouselookOverrideBinding },
    { "MouselookStart", &Script_MouselookStart },
    { "MouselookStop", &Script_MouselookStop },
    { "IsMouselooking", &Script_IsMouselooking },
    { "VehicleExit", &Script_VehicleExit },
    { "VehiclePrevSeat", &Script_VehiclePrevSeat },
    { "VehicleNextSeat", &Script_VehicleNextSeat },
    { "VehicleAimUpStart", &Script_VehicleAimUpStart },
    { "VehicleAimUpStop", &Script_VehicleAimUpStop },
    { "VehicleAimDownStart", &Script_VehicleAimDownStart },
    { "VehicleAimDownStop", &Script_VehicleAimDownStop },
    { "VehicleAimIncrement", &Script_VehicleAimIncrement },
    { "VehicleAimDecrement", &Script_VehicleAimDecrement },
    { "VehicleAimRequestAngle", &Script_VehicleAimRequestAngle },
    { "VehicleAimGetAngle", &Script_VehicleAimGetAngle },
    { "VehicleAimRequestNormAngle", &Script_VehicleAimRequestNormAngle },
    { "VehicleAimGetNormAngle", &Script_VehicleAimGetNormAngle },
    { "VehicleAimSetNormPower", &Script_VehicleAimSetNormPower },
    { "VehicleAimGetNormPower", &Script_VehicleAimGetNormPower },
    { "IsUsingVehicleControls", &Script_IsUsingVehicleControls },
    { "CanExitVehicle", &Script_CanExitVehicle },
    { "CanSwitchVehicleSeats", &Script_CanSwitchVehicleSeats },
    { "IsVehicleAimAngleAdjustable", &Script_IsVehicleAimAngleAdjustable },
    { "IsVehicleAimPowerAdjustable", &Script_IsVehicleAimPowerAdjustable },
    { "DetectWowMouse", &Script_DetectWowMouse },
};

// VERIFY: ee8cb67b auto
// OFFSET: 0x005F9580
void InputControlRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: b76a41b1 auto
// OFFSET: 0x005F95B0
void InputControlUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
