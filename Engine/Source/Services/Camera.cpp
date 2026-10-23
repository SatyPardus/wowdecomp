#include <cmath>
#include <cstddef>
#include <cstdint>
#include <new>
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CRect.h"

static const float DEFAULT_DIST = 100.0f;
static const float DEFAULT_FARZ = 5000.0f;
static const float DEFAULT_NEARZ = 8.0f;
static const float DEFAULT_FOV = 1.5707964f;
static const float DEFAULT_SCREEN_FRUSTUM_LENGTH = 500.0f;

// STATIC: 0x00B4A304
static float s_inv2Pi = 1.0f / 6.2831855f;

// INLINED
CBaseManaged::CBaseManaged() {
    this->m_dataTypeId = 0;
    this->m_flags = 0;
    this->m_updateFcn = nullptr;
    this->m_updateData = nullptr;
    this->m_updatePriority = 0.0f;
}

void CBaseManaged::Update(float time) {
}

void CBaseManaged::Validate(float time) {
}

// INLINED
template <class T>
TManaged<T>::TManaged(const T& value) {
    this->m_data = value;
}

// INLINED
CAngle::CAngle(float angle) : TManaged<float>(0.0f) {
    this->Set_(angle);
}

// VERIFY: 17bd52ca auto
// OFFSET: [0x004BEB90, 0x004BF580]
template <class T>
void TManaged<T>::Set_(const T& value) {
    if (value != this->m_data) {
        this->m_flags |= CBaseManaged::UPDATED;
        this->m_data = value;
    }
}

// VERIFY: a5190ff5 manual
// OFFSET: 0x004BEC90
void CAngle::Set_(const float& value) {
    TManaged<float>::Set_(this->ClampTo2Pi(value));
    this->m_cos = cos(this->m_data);
    this->m_sin = sin(this->m_data);
}

// VERIFY: 72a398ea auto
// OFFSET: 0x004BECF0
void CCamera::SetupWorldProjection(const CRect& rect, uint32_t flags) {
    C44Matrix projection;
    GxuXformCreateProjection_SG(this->m_fov.m_data, (rect.maxX - rect.minX) / (rect.maxY - rect.minY), this->m_zNear.m_data, this->m_zFar.m_data, projection);
    g_theGxDevicePtr->XformSetProjection(projection);

    C44Matrix view;
    C3Vector eye(0.0f, 0.0f, 0.0f);
    C3Vector target(this->m_target.m_data.x - this->m_position.m_data.x, this->m_target.m_data.y - this->m_position.m_data.y, this->m_target.m_data.z - this->m_position.m_data.z);
    C3Vector up(this->m_rotation.m_sin * this->m_roll.m_sin, -(this->m_rotation.m_cos * this->m_roll.m_sin), this->m_roll.m_cos);
    GxuXformCreateLookAtSgCompat(eye, target, up, view);
    g_theGxDevicePtr->XformSetView(view);

    CShaderEffect::UpdateProjMatrix();
}

// VERIFY: 5a7761be auto
// OFFSET: 0x004BEE60
void CameraSetupScreenProjection(const CRect& rect, const C2Vector& offset, float depth, int32_t forceOrtho) {
    float centerX = (rect.minX + rect.maxX) * 0.5f;
    float centerY = 0.5f * (rect.minY + rect.maxY);
    float left = rect.minX - centerX;
    float right = rect.maxX - centerX;
    float bottom = rect.minY + -centerY;
    float top = -centerY + rect.maxY;

    C44Matrix projection;

    if (forceOrtho || !g_theGxDevicePtr->StereoEnabled()) {
        GxuXformCreateOrtho(left, right, bottom, top, -DEFAULT_SCREEN_FRUSTUM_LENGTH, DEFAULT_SCREEN_FRUSTUM_LENGTH, projection);
        projection.Scale(C3Vector(1.0f, 1.0f, -1.0f));
    } else {
        GxuXformCreateOrthoDepth(left, right, bottom, top, -DEFAULT_SCREEN_FRUSTUM_LENGTH, DEFAULT_SCREEN_FRUSTUM_LENGTH, projection);
    }

    CGxCaps& caps = g_theGxDevicePtr->Caps();

    if (!caps.m_pixelCenterOnEdge && caps.m_texelCenterOnEdge) {
        CRect window = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_theGxDevicePtr->CapsWindowSizeInScreenCoords(window);
        projection.d0 = projection.d0 - (rect.maxX - rect.minX) / (window.maxX - window.minX) * 0.5f;
        projection.d1 = 0.5f * ((rect.maxY - rect.minY) / (window.maxY - window.minY)) + projection.d1;
    }

    GxXformSetProjection(projection);

    C44Matrix view;
    view.Translate(C3Vector(offset.x - centerX, offset.y - centerY, 0.0f));
    GxXformSetView(view);
}

// VERIFY: 523f309e auto
// OFFSET: 0x004BF0C0
void CameraSetupWorldProjection(CCamera* camera, const CRect& rect, uint32_t flags) {
    if (!camera) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    camera->SetupWorldProjection(rect, flags);
}

// VERIFY: 6c58d581 manual
// OFFSET: 0x004BF0F0
void CameraGetLineSegment(float x, float y, C3Vector* start, C3Vector* end) {
    if (!start || !end || !(x >= 0.0f) || !(x <= 1.0f) || !(y >= 0.0f) || !(y <= 1.0f)) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    C3Vector corners[8];
    C44Matrix view;
    C44Matrix projection;
    CGxDevice* device = g_theGxDevicePtr;
    view = device->m_xforms[GxXform_View].m_mtx[device->m_xforms[GxXform_View].m_level];
    projection = device->m_projection;
    GxuXformCalcFrustumCorners(view, projection, corners);

    float nearLeftX = corners[0].x + (corners[1].x - corners[0].x) * y;
    float nearLeftY = corners[0].y + (corners[1].y - corners[0].y) * y;
    float nearLeftZ = corners[0].z + (corners[1].z - corners[0].z) * y;
    float nearRightX = corners[3].x + (corners[2].x - corners[3].x) * y;
    float nearRightY = corners[3].y + (corners[2].y - corners[3].y) * y;
    float nearRightZ = corners[3].z + (corners[2].z - corners[3].z) * y;

    start->x = nearLeftX + (nearRightX - nearLeftX) * x;
    start->y = nearLeftY + (nearRightY - nearLeftY) * x;
    start->z = nearLeftZ + (nearRightZ - nearLeftZ) * x;

    float farLeftX = corners[4].x + (corners[5].x - corners[4].x) * y;
    float farLeftY = corners[4].y + (corners[5].y - corners[4].y) * y;
    float farLeftZ = corners[4].z + (corners[5].z - corners[4].z) * y;
    float farRightX = corners[7].x + (corners[6].x - corners[7].x) * y;
    float farRightY = corners[7].y + (corners[6].y - corners[7].y) * y;
    float farRightZ = corners[7].z + (corners[6].z - corners[7].z) * y;

    end->x = farLeftX + (farRightX - farLeftX) * x;
    end->y = farLeftY + (farRightY - farLeftY) * x;
    end->z = farLeftZ + (farRightZ - farLeftZ) * x;
}

// VERIFY: ad64dcf0 auto
// OFFSET: [0x004BF410, 0x004BF4B0]
template <class T>
void TManaged<T>::Update(float time) {
    if (this->m_updateFcn) {
        T value = this->m_data;
        this->m_updateFcn(time, this->m_updateData, &value);
        this->Set_(value);
    }

    this->m_flags &= ~CBaseManaged::REQUIRESUPDATE;
}

// VERIFY: f9cfcece auto
// OFFSET: [0x004BF450, 0x004BF5D0]
template <class T>
void TManaged<T>::Validate(float time) {
    if (this->m_updateFcn) {
        T value = this->m_data;
        T original = this->m_data;
        this->m_updateFcn(time, this->m_updateData, &value);

        if (original != value) {
            SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
            return;
        }
    }

    this->m_flags &= ~CBaseManaged::REQUIRESUPDATE;
}

// VERIFY: a8b9e377 manual
// OFFSET: 0x004BF660
CBaseManaged::~CBaseManaged() {
}

// VERIFY: 11178118 manual
// OFFSET: [0x004BF6D0, 0x004BF740]
CDataMgr::~CDataMgr() {
}

// INLINED
CDataMgr::CDataMgr(uint32_t count) {
    this->m_managed.SetCount(count);
}

// VERIFY: a7d7446b manual
// OFFSET: 0x004BF770
CCamera::CCamera() : CDataMgr(9), m_position(C3Vector(DEFAULT_DIST, 0.0f, 0.0f)), m_target(C3Vector(0.0f, 0.0f, 0.0f)), m_distance(DEFAULT_DIST), m_zFar(DEFAULT_FARZ), m_zNear(DEFAULT_NEARZ), m_aoa(0.0f), m_fov(DEFAULT_FOV), m_roll(0.0f), m_rotation(0.0f) {
    this->AddManaged(&this->m_position, 7, 0);
    this->AddManaged(&this->m_target, 8, 0);
    this->AddManaged(&this->m_distance, 1, 0);
    this->AddManaged(&this->m_zFar, 2, 0);
    this->AddManaged(&this->m_zNear, 3, 0);
    this->AddManaged(&this->m_aoa, 0, 0);
    this->AddManaged(&this->m_fov, 4, 0);
    this->AddManaged(&this->m_roll, 5, 0);
    this->AddManaged(&this->m_rotation, 6, 0);
}

// VERIFY: d759f72d auto
// OFFSET: 0x004BFCA0
CCamera* CameraCreate() {
    void* mem = SMemAlloc(sizeof(CCamera), "HCAMERA", SLOG_OBJECT, 0);
    CCamera* camera = mem ? new (mem) CCamera : nullptr;

    if (!camera) {
        return nullptr;
    }

    return reinterpret_cast<CCamera*>(HandleCreate(camera, "HCAMERA"));
}

// VERIFY: 2e746ce9 manual
// OFFSET: 0x004BFCE0
CCamera* CameraDuplicate(CCamera* source) {
    if (!source) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return nullptr;
    }

    void* mem = SMemAlloc(sizeof(CCamera), "HCAMERA", SLOG_OBJECT, 0);
    CCamera* camera = mem ? new (mem) CCamera : nullptr;

    if (!camera) {
        return nullptr;
    }

    camera->m_position.m_updatePriority = 0.0f;
    camera->m_position.m_updateFcn = nullptr;
    camera->m_position.m_updateData = nullptr;
    camera->m_position.Set_(source->m_position.m_data);

    camera->m_target.m_updatePriority = 0.0f;
    camera->m_target.m_updateFcn = nullptr;
    camera->m_target.m_updateData = nullptr;
    camera->m_target.Set_(source->m_target.m_data);

    camera->m_distance.m_updatePriority = 0.0f;
    camera->m_distance.m_updateFcn = nullptr;
    camera->m_distance.m_updateData = nullptr;
    camera->m_distance.Set_(source->m_distance.m_data);

    camera->m_zNear.m_updatePriority = 0.0f;
    camera->m_zNear.m_updateFcn = nullptr;
    camera->m_zNear.m_updateData = nullptr;
    camera->m_zNear.Set_(source->m_zNear.m_data);

    camera->m_zFar.m_updatePriority = 0.0f;
    camera->m_zFar.m_updateFcn = nullptr;
    camera->m_zFar.m_updateData = nullptr;
    camera->m_zFar.Set_(source->m_zFar.m_data);

    camera->m_aoa.m_updatePriority = 0.0f;
    camera->m_aoa.m_updateFcn = nullptr;
    camera->m_aoa.m_updateData = nullptr;
    camera->m_aoa.Set_(source->m_aoa.m_data);

    camera->m_fov.m_updatePriority = 0.0f;
    camera->m_fov.m_updateFcn = nullptr;
    camera->m_fov.m_updateData = nullptr;
    camera->m_fov.Set_(source->m_fov.m_data);

    camera->m_roll.m_updatePriority = 0.0f;
    camera->m_roll.m_updateFcn = nullptr;
    camera->m_roll.m_updateData = nullptr;
    camera->m_roll.Set_(source->m_roll.m_data);

    camera->m_rotation.m_updatePriority = 0.0f;
    camera->m_rotation.m_updateFcn = nullptr;
    camera->m_rotation.m_updateData = nullptr;
    camera->m_rotation.Set_(source->m_rotation.m_data);

    return reinterpret_cast<CCamera*>(HandleCreate(camera, "HCAMERA"));
}

// VERIFY: 1384d21a auto
// OFFSET: 0x004C1140
float CAngle::ClampTo2Pi(float angle) {
    float turns = static_cast<float>(static_cast<int32_t>(angle * s_inv2Pi)) * 6.2831855f;

    if (angle < 0.0f) {
        turns -= 6.2831855f;
    }

    return angle - turns;
}

// VERIFY: d430685f manual
// OFFSET: 0x004C1180
static void GetManagedValue(CDataMgr* mgr, uint32_t index, float* value, uint8_t dataTypeId) {
    if (!value || !mgr || index >= mgr->m_managed.Count() || !mgr->m_managed[index] || mgr->m_managed[index]->m_dataTypeId != dataTypeId) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    CBaseManaged* data = mgr->m_managed[index];

    if (data->m_flags & CBaseManaged::REQUIRESUPDATE) {
        if (data->m_flags & CBaseManaged::READONLY) {
            data->Validate(0.0f);
        } else {
            data->Update(0.0f);
        }
    }

    *value = static_cast<TManaged<float>*>(data)->m_data;
}

// VERIFY: c7e90e1f auto
// OFFSET: 0x004C11F0
float CameraGetFloat(CCamera* camera, uint32_t index) {
    float value = 0.0f;
    GetManagedValue(camera, index, &value, 6);

    return value;
}

// VERIFY: ecbbb5cc manual
// OFFSET: 0x004C1220
static void GetManagedValue(CDataMgr* mgr, uint32_t index, C3Vector* value, uint8_t dataTypeId) {
    if (!value || !mgr || index >= mgr->m_managed.Count() || !mgr->m_managed[index] || mgr->m_managed[index]->m_dataTypeId != dataTypeId) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    CBaseManaged* data = mgr->m_managed[index];

    if (data->m_flags & CBaseManaged::REQUIRESUPDATE) {
        if (data->m_flags & CBaseManaged::READONLY) {
            data->Validate(0.0f);
        } else {
            data->Update(0.0f);
        }
    }

    *value = static_cast<TManaged<C3Vector>*>(data)->m_data;
}

// VERIFY: 15f73a5e auto
// OFFSET: 0x004C1290
void DataMgrGetCoord(CDataMgr* mgr, uint32_t index, C3Vector* value) {
    GetManagedValue(mgr, index, value, 3);
}

// VERIFY: 6e02d0ab manual
// OFFSET: 0x004C12B0
void CameraSetCoord(CCamera* camera, uint32_t index, const C3Vector& value, uint32_t keepMask) {
    C3Vector current(0.0f, 0.0f, 0.0f);
    GetManagedValue(camera, index, &current, 3);

    C3Vector newValue((keepMask & 0x1) ? current.x : value.x, (keepMask & 0x2) ? current.y : value.y, (keepMask & 0x4) ? current.z : value.z);

    if (!camera || index >= camera->m_managed.Count() || !camera->m_managed[index] || camera->m_managed[index]->m_dataTypeId != 3 || (camera->m_managed[index]->m_flags & CBaseManaged::READONLY)) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    TManaged<C3Vector>* data = static_cast<TManaged<C3Vector>*>(camera->m_managed[index]);
    data->m_updateFcn = nullptr;
    data->m_updateData = nullptr;
    data->m_updatePriority = 0.0f;
    data->Set_(newValue);
}

// VERIFY: ef5a2ea9 manual
// OFFSET: 0x004C1360
void CameraSetFloat(CCamera* camera, uint32_t index, float value) {
    if (!camera || index >= camera->m_managed.Count() || !camera->m_managed[index] || camera->m_managed[index]->m_dataTypeId != 6 || (camera->m_managed[index]->m_flags & CBaseManaged::READONLY)) {
        SErrSetLastError(STORM_ERROR_INVALID_PARAMETER);
        return;
    }

    TManaged<float>* data = static_cast<TManaged<float>*>(camera->m_managed[index]);
    data->m_updateFcn = nullptr;
    data->m_updateData = nullptr;
    data->m_updatePriority = 0.0f;
    data->Set_(value);
}

// VERIFY: 8e9f0604 manual
// OFFSET: 0x004C13B0
void CDataMgr::InsertByPriority(CBaseManaged* data) {
    for (CBaseManaged* node = this->m_updateList.Head(); node; node = this->m_updateList.Next(node)) {
        if (node->m_updatePriority <= data->m_updatePriority) {
            this->m_updateList.LinkNode(data, 2, node);
            return;
        }
    }

    this->m_updateList.LinkToTail(data);
}

// VERIFY: e2e5042b auto
// OFFSET: 0x004C14B0
void CDataMgr::AddManaged(TManaged<C3Vector>* data, uint32_t index, uint8_t flags) {
    data->m_flags = flags;
    data->m_dataTypeId = 3;
    this->m_managed[index] = data;

    if (flags & CBaseManaged::ALWAYSUPDATE) {
        this->InsertByPriority(data);
    }
}

// VERIFY: 0e9ed7c7 auto
// OFFSET: 0x004C14E0
void CDataMgr::AddManaged(TManaged<float>* data, uint32_t index, uint8_t flags) {
    data->m_flags = flags;
    data->m_dataTypeId = 6;
    this->m_managed[index] = data;

    if (flags & CBaseManaged::ALWAYSUPDATE) {
        this->InsertByPriority(data);
    }
}

// VERIFY: e9e02e6c manual
// OFFSET: [0x004BFA50, 0x004BFC70]
CCamera::~CCamera() {
}
