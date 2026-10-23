#ifndef ENGINE_SOURCE_SERVICES_CAMERAINT_H
#define ENGINE_SOURCE_SERVICES_CAMERAINT_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Tempest/C3Vector.h"

class CRect;

class CBaseManaged {
    public:
    enum : uint8_t {
        ALWAYSUPDATE = 0x1,
        READONLY = 0x2,
        REQUIRESUPDATE = 0x4,
        UPDATED = 0x8,
    };

    /* 0004 */ TSLink<CBaseManaged> m_link;
    /* 000C */ uint8_t m_dataTypeId;
    /* 000D */ uint8_t m_flags;
    /* 0010 */ void (*m_updateFcn)(float, void*, void*);
    /* 0014 */ void* m_updateData;
    /* 0018 */ float m_updatePriority;

    CBaseManaged();
    virtual ~CBaseManaged();
    virtual void Update(float time);
    virtual void Validate(float time);
};

template <class T>
class TManaged : public CBaseManaged {
    public:
    /* 001C */ T m_data;

    TManaged(const T& value);
    virtual void Update(float time);
    virtual void Validate(float time);
    virtual void Set_(const T& value);
};

class CAngle : public TManaged<float> {
    public:
    /* 0020 */ float m_cos;
    /* 0024 */ float m_sin;

    CAngle(float angle);
    float ClampTo2Pi(float angle);
    virtual void Set_(const float& value);
};

class CDataMgr : public CHandleObject {
    public:
    /* 0008 */ TSFixedArray<CBaseManaged*> m_managed;
    /* 0014 */ TSExplicitList<CBaseManaged, offsetof(CBaseManaged, m_link)> m_updateList;

    CDataMgr(uint32_t count);
    virtual ~CDataMgr();
    void AddManaged(TManaged<C3Vector>* data, uint32_t index, uint8_t flags);
    void AddManaged(TManaged<float>* data, uint32_t index, uint8_t flags);
    void InsertByPriority(CBaseManaged* data);
};

class CCamera : public CDataMgr {
    public:
    /* 0020 */ TManaged<C3Vector> m_position;
    /* 0048 */ TManaged<C3Vector> m_target;
    /* 0070 */ TManaged<float> m_distance;
    /* 0090 */ TManaged<float> m_zFar;
    /* 00B0 */ TManaged<float> m_zNear;
    /* 00D0 */ CAngle m_aoa;
    /* 00F8 */ CAngle m_fov;
    /* 0120 */ CAngle m_roll;
    /* 0148 */ CAngle m_rotation;

    CCamera();
    virtual ~CCamera();
    void SetupWorldProjection(const CRect& rect, uint32_t flags);
};

#endif
