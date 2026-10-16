#ifndef WOW_SOURCE_OBJECT_PASSENGER_H
#define WOW_SOURCE_OBJECT_PASSENGER_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/Object/WGUID.h"

class C44Matrix;

#pragma pack(push, 4)
class CPassenger {
    public:
    /* 0000 */ TSLink<void> m_link;
    /* 0008 */ WOWGUID m_transportGuid;
    /* 0010 */ C3Vector m_position;
    /* 001C */ uint32_t unused_001C;
    /* 0020 */ float m_facing;
    /* 0024 */ float m_pitch;
    /* 0028 */ const WOWGUID* m_ownerGuid;
    /* 002C */ uint8_t m_passengerFlags;

    CPassenger(const WOWGUID& transportGuid, const C3Vector& position, const WOWGUID& ownerGuid);
    void PostInit(const C3Vector& position);
    void Reenable();
    float GetFacing(float facing) const;
    float GetRawFacing(float facing) const;
    C3Vector GetPosition(const C3Vector& position) const;
    C3Vector GetRawPosition(const C3Vector& position) const;
    C4Quaternion GetRotation() const;
    float GetRawFacingFromRotation() const;
    float RemoveFromTransportFinish(const C44Matrix& matrix);
    float AddToTransport(WOWGUID transport, C44Matrix* inverse, C44Matrix* transportMatrix);
    void SetTransport(WOWGUID transport);
    float RemoveFromTransport(C44Matrix* matrix);
};
#pragma pack(pop)

#endif
