#include <cmath>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Tempest/C33Matrix.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/C4Quaternion.h"
#include "Engine/Source/Tempest/CMath.h"

// STATIC: 0x00AA2E4C
static const uint32_t s_nextAxis[3] = { 1, 2, 0 };

// VERIFY: f3474eed manual
// OFFSET: [0x009826A0, 0x009827B0]
template <uint32_t N>
static void FromRotationMatrixImpl(float trace, C4Quaternion& q, const float* m) {
    float* out = &q.x;

    if (trace > 0.0f) {
        float s = sqrt(trace + 1.0f);
        q.w = s * 0.5f;
        s = 0.5f / s;
        q.x = (m[2 * N + 1] - m[1 * N + 2]) * s;
        q.y = (m[0 * N + 2] - m[2 * N + 0]) * s;
        q.z = (m[1 * N + 0] - m[0 * N + 1]) * s;
        return;
    }

    uint32_t i = m[0] < m[N + 1] ? 1 : 0;

    if (m[i * N + i] < m[2 * N + 2]) {
        i = 2;
    }

    uint32_t j = s_nextAxis[i];
    uint32_t k = s_nextAxis[j];

    float s = sqrt(m[i * N + i] - m[j * N + j] - m[k * N + k] + 1.0f);
    out[i] = s * 0.5f;
    s = 0.5f / s;
    q.w = (m[k * N + j] - m[j * N + k]) * s;
    out[j] = (m[i * N + j] + m[j * N + i]) * s;
    out[k] = (m[i * N + k] + m[k * N + i]) * s;
}

// VERIFY: fc606713 manual
// OFFSET: 0x00982340
C4Quaternion::C4Quaternion(const C4QuaternionCompressed& compressed) {
    this->x = 0.0f;
    this->y = 0.0f;
    this->z = 0.0f;
    this->w = 0.0f;

    const uint64_t packed = static_cast<uint64_t>(compressed.m_packed);
    // Extract the original signed 22/21/21-bit fields without shifting a
    // negative signed integer or overflowing its representation.
    const auto signedField = [](uint32_t value, uint32_t signBit) {
        return static_cast<int32_t>(value ^ signBit) - static_cast<int32_t>(signBit);
    };
    this->x = signedField(static_cast<uint32_t>(packed >> 42), 0x200000) * 0.00000047683716f;
    this->y = signedField(static_cast<uint32_t>(packed >> 21) & 0x1FFFFF, 0x100000) * 0.00000095367432f;
    this->z = signedField(static_cast<uint32_t>(packed) & 0x1FFFFF, 0x100000) * 0.00000095367432f;

    float length = this->z * this->z + this->x * this->x + this->y * this->y;

    if (fabs(length - 1.0f) >= 0.00000095367432f) {
        this->w = sqrt(1.0f - length);
    } else {
        this->w = 0.0f;
    }
}

// VERIFY: 94bc14f8 manual
// OFFSET: 0x00982400
void C4Quaternion::FromAngleAxis(float angle, const C3Vector& axis) {
    float half = angle * 0.5f;
    float c = cos(half);
    float s = sin(half);
    this->w = c;
    this->x = axis.x * s;
    this->y = axis.y * s;
    this->z = s * axis.z;
}

// VERIFY: 53458598 manual
// OFFSET: 0x00982460
C4Quaternion C4Quaternion::Slerp(float t, const C4Quaternion& a, const C4Quaternion& b) {
    float cosine = a.z * b.z + a.y * b.y + a.x * b.x + b.w * a.w;
    float sign = 1.0f;

    if (cosine < 0.0f) {
        sign = -1.0f;
        cosine = -cosine;
    }

    float sine = sqrt(fabs(1.0f - cosine * cosine));

    C4Quaternion result;

    if (fabs(sine) < 0.00000047683716f) {
        result.x = a.x;
        result.y = a.y;
        result.z = a.z;
        result.w = a.w;
        return result;
    }

    float angle = atan2(sine, cosine);
    float scale = 1.0f / sine;
    float wa = sin((1.0f - t) * angle) * scale;
    float wb = scale * sin(t * angle) * sign;

    result.x = b.x * wb + a.x * wa;
    result.y = wb * b.y + a.y * wa;
    result.z = a.z * wa + wb * b.z;
    result.w = wb * b.w + wa * a.w;
    return result;
}

// VERIFY: 1afec70e manual
// OFFSET: 0x00982570
void C4Quaternion::NormalizeApprox() {
    float length = this->w * this->w + this->z * this->z + this->y * this->y + this->x * this->x;
    float scale = 1.021435f - (length - 0.95906597f) * 0.532516f;

    if (length <= 0.91521198f) {
        scale = scale * (1.021435f - (scale * scale * length - 0.95906597f) * 0.532516f);

        if (length <= 0.6521197f) {
            scale = (1.021435f - (length * (scale * scale) - 0.95906597f) * 0.532516f) * scale;
        }
    }

    this->x = this->x * scale;
    this->y = this->y * scale;
    this->z = scale * this->z;
    this->w = scale * this->w;
}

// VERIFY: 7bdd0dd3 manual
// OFFSET: 0x00982630
C4Quaternion C4Quaternion::Nlerp(float t, const C4Quaternion& a, const C4Quaternion& b) {
    C4Quaternion result;
    result.x = (b.x - a.x) * t + a.x;
    result.y = (b.y - a.y) * t + a.y;
    result.z = (b.z - a.z) * t + a.z;
    result.w = (b.w - a.w) * t + a.w;
    result.NormalizeApprox();
    return result;
}

// VERIFY: df043799 manual
// OFFSET: 0x009828B0
void C4Quaternion::FromRotationMatrix(const C33Matrix& m) {
    FromRotationMatrixImpl<3>(m.a0 + m.b1 + m.c2, *this, &m.a0);
}

// VERIFY: e4334e9b manual
// OFFSET: 0x00982910
void C4Quaternion::FromRotationMatrix(const C44Matrix& m) {
    C44Matrix transposed = m.Transpose();
    FromRotationMatrixImpl<4>(transposed.c2 + transposed.b1 + transposed.a0, *this, &transposed.a0);
}

// VERIFY: 326feb29 auto
// OFFSET: 0x004F4320
C4Quaternion operator*(const C4Quaternion& l, const C4Quaternion& r) {
    C4Quaternion result;
    result.x = l.w * r.x + r.z * l.y + r.w * l.x - l.z * r.y;
    result.y = r.w * l.y + l.w * r.y + l.z * r.x - r.z * l.x;
    result.z = r.z * l.w + r.w * l.z + l.x * r.y - r.x * l.y;
    result.w = r.w * l.w - l.x * r.x - l.y * r.y - r.z * l.z;
    return result;
}

// VERIFY: 97be132a auto
// OFFSET: 0x004F43B0
C4QuaternionCompressed& C4QuaternionCompressed::operator=(const C4Quaternion& q) {
    const int64_t sign = q.w < 0.0f ? -1 : 1;
    const uint64_t x = static_cast<uint64_t>(sign * FloatToInt64(q.x * 2097152.0f)) & 0x3FFFFF;
    const uint64_t y = static_cast<uint64_t>(sign * FloatToInt64(q.y * 1048576.0f)) & 0x1FFFFF;
    const uint64_t z = static_cast<uint64_t>(sign * FloatToInt64(q.z * 1048576.0f)) & 0x1FFFFF;
    const uint64_t packed = (x << 42) | (y << 21) | z;
    std::memcpy(&this->m_packed, &packed, sizeof(packed));
    return *this;
}
