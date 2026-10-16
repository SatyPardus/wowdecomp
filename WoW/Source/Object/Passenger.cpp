#include "Passenger.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "WoW/Source/Object/ObjectClient/Movement_C.h"

// VERIFY: 185d3db3 auto
// OFFSET: 0x004F48E0
CPassenger::CPassenger(const WOWGUID& transportGuid, const C3Vector& position, const WOWGUID& ownerGuid) : m_transportGuid(transportGuid), m_position(position) {
    this->m_facing = 0.0f;
    this->m_pitch = 0.0f;
    this->m_ownerGuid = &ownerGuid;
    this->m_passengerFlags = 0;
}

// VERIFY: 3f84735d auto
// OFFSET: 0x004F4230
void CPassenger::PostInit(const C3Vector& position) {
    if (this->m_transportGuid) {
        if (!MovementAddToTransport(this, this->m_transportGuid, 1)) {
            this->m_transportGuid = WOWGUID();
            this->m_position = position;
        }
    }
}

// VERIFY: 871c8fad manual
// OFFSET: 0x004F4280
void CPassenger::Reenable() {
    WOWGUID transport = this->m_transportGuid;

    if (transport) {
        MovementAddToTransport(this, transport, 1);
    }
}

// VERIFY: 2668c115 auto
// OFFSET: 0x004F42A0
float CPassenger::GetFacing(float facing) const {
    if (!this->m_transportGuid) {
        return facing;
    }

    return CMath::normalizeangle0to2pi_(MovementGetTransportFacing(this->m_transportGuid) + facing);
}

// VERIFY: dcd90e3b auto
// OFFSET: 0x004F42E0
float CPassenger::GetRawFacing(float facing) const {
    if (!this->m_transportGuid) {
        return facing;
    }

    return CMath::normalizeangle0to2pi_(facing - MovementGetTransportFacing(this->m_transportGuid));
}

// VERIFY: eeaa5f5d auto
// OFFSET: 0x004F4460
C3Vector CPassenger::GetPosition(const C3Vector& position) const {
    WOWGUID transport = this->m_transportGuid;

    if (!transport) {
        return position;
    }

    C44Matrix matrix;
    MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);
    return position * matrix;
}

// VERIFY: d6a74e0d manual
// OFFSET: 0x004F4500
C3Vector CPassenger::GetRawPosition(const C3Vector& position) const {
    WOWGUID transport = this->m_transportGuid;

    if (!transport) {
        return position;
    }

    C44Matrix matrix;
    MovementGetTransportMtxX(transport, &matrix, *this->m_ownerGuid, __FILE__, __LINE__);
    return position * matrix.AffineInverse();
}

// VERIFY: 1a28fbc2 manual
// OFFSET: 0x004F45B0
C4Quaternion CPassenger::GetRotation() const {
    C4Quaternion rotation(*reinterpret_cast<const C4QuaternionCompressed*>(&this->m_facing));

    if (this->m_transportGuid.m_guid) {
        C4Quaternion transport;
        transport.x = 0.0f;
        transport.y = 0.0f;
        transport.z = 0.0f;
        transport.w = 1.0f;
        MovementGetTransportQuat(this->m_transportGuid, &transport);
        return rotation * transport;
    }

    return rotation;
}

// VERIFY: a4878ae8 manual
// OFFSET: 0x004F4630
float CPassenger::GetRawFacingFromRotation() const {
    C4Quaternion rotation = this->GetRotation();
    float x = 1.0f - (rotation.z * rotation.z + rotation.y * rotation.y) * 2.0f;
    float y = (rotation.w * rotation.z + rotation.y * rotation.x) * 2.0f;

    if (2.3841858e-7f <= fabsf(x)) {
        if (fabsf(y) < 2.3841858e-7f) {
            return 0.0f < x ? 0.0f : 3.1415927f;
        }

        return static_cast<float>(atan2(y, x));
    }

    return 0.0f <= y ? 0.5f * 3.1415927f : 1.5f * 3.1415927f;
}

// VERIFY: 954d19c3 manual
// OFFSET: 0x004F46E0
float CPassenger::RemoveFromTransportFinish(const C44Matrix& matrix) {
    float facing = MovementGetTransportFacing(this->m_transportGuid);
    this->m_position *= matrix;

    if (this->m_passengerFlags & 0x2) {
        C4Quaternion rotation(*reinterpret_cast<const C4QuaternionCompressed*>(&this->m_facing));
        C4Quaternion transport;
        transport.x = 0.0f;
        transport.y = 0.0f;
        transport.z = 0.0f;
        transport.w = 1.0f;
        MovementGetTransportQuat(this->m_transportGuid, &transport);
        rotation = rotation * transport;
        *reinterpret_cast<C4QuaternionCompressed*>(&this->m_facing) = rotation;
    } else {
        this->m_facing = CMath::normalizeangle0to2pi_(this->m_facing + facing);
    }

    return facing;
}

// VERIFY: 65f817a1 manual
// OFFSET: 0x004F4930
float CPassenger::RemoveFromTransport(C44Matrix* matrix) {
    MovementGetTransportMtxX(this->m_transportGuid, matrix, *this->m_ownerGuid, __FILE__, __LINE__);
    return this->RemoveFromTransportFinish(*matrix);
}

// VERIFY: 6997cb00 manual
// OFFSET: 0x004F47B0
float CPassenger::AddToTransport(WOWGUID transport, C44Matrix* inverse, C44Matrix* transportMatrix) {
    float facing = -MovementGetTransportFacing(transport);

    if (transportMatrix) {
        MovementGetTransportMtxX(transport, transportMatrix, *this->m_ownerGuid, __FILE__, __LINE__);
        *inverse = transportMatrix->AffineInverse();
    } else {
        MovementGetTransportMtxX(transport, inverse, *this->m_ownerGuid, __FILE__, __LINE__);
        *inverse = inverse->AffineInverse();
    }

    this->m_position *= *inverse;

    if (this->m_passengerFlags & 0x2) {
        C4Quaternion rotation;
        rotation.x = 0.0f;
        rotation.y = 0.0f;
        rotation.z = 0.0f;
        rotation.w = 0.0f;
        rotation.FromRotationMatrix(*inverse);
        C4Quaternion result = C4Quaternion(*reinterpret_cast<const C4QuaternionCompressed*>(&this->m_facing)) * rotation;
        *reinterpret_cast<C4QuaternionCompressed*>(&this->m_facing) = result;
    } else {
        this->m_facing = CMath::normalizeangle0to2pi_(this->m_facing + facing);
    }

    return facing;
}

// VERIFY: 346a66a5 auto
// OFFSET: 0x004F4970
void CPassenger::SetTransport(WOWGUID transport) {
    if (transport == this->m_transportGuid || (transport && !MovementObjIsTransport(transport))) {
        return;
    }

    C44Matrix matrix;

    if (this->m_transportGuid) {
        this->RemoveFromTransport(&matrix);
    }

    if (transport) {
        this->AddToTransport(transport, &matrix, nullptr);
    }

    this->m_transportGuid = transport;
}
