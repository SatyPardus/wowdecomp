#include "Camera.h"
#include <cstdint>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Frame/CSimpleTop.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Ui/BarberShop.h"
#include "WoW/Source/Ui/WorldFrame.h"

// VERIFY: 60071b01 auto
// OFFSET: 0x006017E0
static int32_t Script_CameraZoomIn(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    double distance = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 1.0;

    if (CGBarberShop::m_barberShopEnabled) {
        distance *= 0.2f;
    }

    CGWorldFrame::GetActiveCamera()->ZoomIn(static_cast<float>(distance), time, 0.0f);
    return 0;
}

// VERIFY: 2a60fd0e auto
// OFFSET: 0x00601840
static int32_t Script_CameraZoomOut(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    double distance = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : 1.0;

    if (CGBarberShop::m_barberShopEnabled) {
        distance *= 0.2f;
    }

    CGWorldFrame::GetActiveCamera()->ZoomOut(static_cast<float>(distance), time, 0.0f);
    return 0;
}

// VERIFY: 1a486e5c manual
// OFFSET: 0x005FF000
static int32_t MoveViewStart(lua_State* L, CGCameraMotion motion) {
    uint32_t time = CSimpleTop::s_eventTime;
    float speed = 1.0f;

    if (lua_isnumber(L, 1)) {
        speed = static_cast<float>(lua_tonumber(L, 1));
    }

    CGWorldFrame::GetActiveCamera()->StartMotion(motion, time, 0, speed);
    return 0;
}

// VERIFY: 833c8351 auto
// OFFSET: 0x005FF080
static int32_t Script_MoveViewInStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_IN);
}

// VERIFY: 5d39c109 manual
// OFFSET: 0x005FF0A0
static int32_t Script_MoveViewInStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x1) {
        camera->m_motionStopTime[CAMERA_MOVE_IN] = time;
        camera->m_motionFlags |= 0x2;
    }

    return 0;
}

// VERIFY: 6a295394 auto
// OFFSET: 0x005FF0D0
static int32_t Script_MoveViewOutStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_OUT);
}

// VERIFY: b4961425 manual
// OFFSET: 0x005FF0F0
static int32_t Script_MoveViewOutStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x4) {
        camera->m_motionStopTime[CAMERA_MOVE_OUT] = time;
        camera->m_motionFlags |= 0x8;
    }

    return 0;
}

// VERIFY: 12f10ad4 auto
// OFFSET: 0x005FF170
static int32_t Script_MoveViewLeftStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_LEFT);
}

// VERIFY: 2c3e8bcb manual
// OFFSET: 0x005FF190
static int32_t Script_MoveViewLeftStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x40) {
        camera->m_motionStopTime[CAMERA_MOVE_LEFT] = time;
        camera->m_motionFlags |= 0x80;
    }

    return 0;
}

// VERIFY: 7e563174 auto
// OFFSET: 0x005FF120
static int32_t Script_MoveViewRightStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_RIGHT);
}

// VERIFY: 3f6208a0 manual
// OFFSET: 0x005FF140
static int32_t Script_MoveViewRightStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x10) {
        camera->m_motionStopTime[CAMERA_MOVE_RIGHT] = time;
        camera->m_motionFlags |= 0x20;
    }

    return 0;
}

// VERIFY: 57783860 auto
// OFFSET: 0x005FF1C0
static int32_t Script_MoveViewUpStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_UP);
}

// VERIFY: a013e6fb manual
// OFFSET: 0x005FF1E0
static int32_t Script_MoveViewUpStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x100) {
        camera->m_motionStopTime[CAMERA_MOVE_UP] = time;
        camera->m_motionFlags |= 0x200;
    }

    return 0;
}

// VERIFY: f815dfb9 auto
// OFFSET: 0x005FF210
static int32_t Script_MoveViewDownStart(lua_State* L) {
    return MoveViewStart(L, CAMERA_MOVE_DOWN);
}

// VERIFY: 8438c7a5 manual
// OFFSET: 0x005FF230
static int32_t Script_MoveViewDownStop(lua_State* L) {
    uint32_t time = CSimpleTop::s_eventTime;
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera->m_motionFlags & 0x400) {
        camera->m_motionStopTime[CAMERA_MOVE_DOWN] = time;
        camera->m_motionFlags |= 0x800;
    }

    return 0;
}

// VERIFY: 4cdf1c55 auto
// OFFSET: 0x006039B0
static int32_t Script_SetView(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SetView(viewModeIndex)");
        return 0;
    }

    int32_t view = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(view - 1) <= 4) {
        CGWorldFrame::GetActiveCamera()->SetView(view, s_cvCameraViewBlendStyle->m_intValue, 0);
    }

    return 0;
}

// VERIFY: 9e9f4da1 auto
// OFFSET: 0x005FF260
static int32_t Script_SaveView(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: SaveView(viewModeIndex)");
        return 0;
    }

    int32_t view = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(view - 1) <= 4 && view) {
        CGWorldFrame::GetActiveCamera()->CreateViewFromCamera(view);
    }

    return 0;
}

// VERIFY: 2c84a3e5 auto
// OFFSET: 0x00604C80
static int32_t Script_ResetView(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: ResetView(viewModeIndex)");
        return 0;
    }

    int32_t view = static_cast<int32_t>(lua_tonumber(L, 1));

    if (static_cast<uint32_t>(view - 1) <= 4) {
        CGWorldFrame::GetActiveCamera()->ResetView(view);
    }

    return 0;
}

// VERIFY: 62f6a98c auto
// OFFSET: 0x00604CE0
static int32_t Script_NextView(lua_State* L) {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    int32_t view = camera->m_viewIndex;

    if (view < 5) {
        camera->SetView(view + 1, s_cvCameraViewBlendStyle->m_intValue, 0);
    }

    return 0;
}

// VERIFY: 7e911a30 manual
// OFFSET: 0x00604D10
static int32_t Script_PrevView(lua_State* L) {
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    int32_t view = camera->m_viewIndex;

    if (view > 1) {
        camera->SetView(view - 1, s_cvCameraViewBlendStyle->m_intValue, 0);
    }

    return 0;
}

// VERIFY: 9859ab0a auto
// OFFSET: 0x005FF2C0
static int32_t Script_FlipCameraYaw(lua_State* L) {
    if (!lua_isnumber(L, 1)) {
        luaL_error(L, "Usage: FlipCameraYaw(degrees)");
        return 0;
    }

    float degrees = static_cast<float>(lua_tonumber(L, 1));
    CGCamera* camera = CGWorldFrame::GetActiveCamera();
    camera->m_yawOffset = degrees * 0.017453292f + camera->m_yawOffset;
    return 0;
}

// VERIFY: a2394a6a auto
// OFFSET: 0x006018A0
static int32_t Script_VehicleCameraZoomIn(lua_State* L) {
    return Script_CameraZoomIn(L);
}

// VERIFY: e94d57e0 auto
// OFFSET: 0x006018B0
static int32_t Script_VehicleCameraZoomOut(lua_State* L) {
    return Script_CameraZoomOut(L);
}

// STATIC: 0x00AD2060
static FrameScript_Method s_ScriptFunctions[] = {
    { "CameraZoomIn", &Script_CameraZoomIn },
    { "CameraZoomOut", &Script_CameraZoomOut },
    { "MoveViewInStart", &Script_MoveViewInStart },
    { "MoveViewInStop", &Script_MoveViewInStop },
    { "MoveViewOutStart", &Script_MoveViewOutStart },
    { "MoveViewOutStop", &Script_MoveViewOutStop },
    { "MoveViewLeftStart", &Script_MoveViewLeftStart },
    { "MoveViewLeftStop", &Script_MoveViewLeftStop },
    { "MoveViewRightStart", &Script_MoveViewRightStart },
    { "MoveViewRightStop", &Script_MoveViewRightStop },
    { "MoveViewUpStart", &Script_MoveViewUpStart },
    { "MoveViewUpStop", &Script_MoveViewUpStop },
    { "MoveViewDownStart", &Script_MoveViewDownStart },
    { "MoveViewDownStop", &Script_MoveViewDownStop },
    { "SetView", &Script_SetView },
    { "SaveView", &Script_SaveView },
    { "ResetView", &Script_ResetView },
    { "NextView", &Script_NextView },
    { "PrevView", &Script_PrevView },
    { "FlipCameraYaw", &Script_FlipCameraYaw },
    { "VehicleCameraZoomIn", &Script_VehicleCameraZoomIn },
    { "VehicleCameraZoomOut", &Script_VehicleCameraZoomOut },
};

// VERIFY: bb9ee072 auto
// OFFSET: 0x005FE2C0
void CameraRegisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_RegisterFunction(s_ScriptFunctions[i].m_name, s_ScriptFunctions[i].m_method);
    }
}

// VERIFY: cfd72b5f auto
// OFFSET: 0x005FE2F0
void CameraUnregisterScriptFunctions() {
    for (uint32_t i = 0; i < sizeof(s_ScriptFunctions) / sizeof(s_ScriptFunctions[0]); i++) {
        FrameScript_UnregisterFunction(s_ScriptFunctions[i].m_name);
    }
}
