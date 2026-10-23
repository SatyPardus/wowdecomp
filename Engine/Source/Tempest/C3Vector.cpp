#include <cmath>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C4Vector.h"
#include "Engine/Source/Tempest/CImVector.h"

// VERIFY: 237e1aa1 auto
// OFFSET: 0x00407990
C3Vector operator+(const C3Vector& l, const C3Vector& r) {
    C3Vector result;
    result.x = l.x + r.x;
    result.y = l.y + r.y;
    result.z = l.z + r.z;
    return result;
}

// VERIFY: 1e8771a4 manual
// OFFSET: 0x00408260
void C3Vector::Zero() {
    this->x = 0.0f;
    this->y = 0.0f;
    this->z = 0.0f;
}

// VERIFY: a755af8a auto
// OFFSET: 0x00482970
C3Vector& C3Vector::operator+=(const C3Vector& v) {
    this->x = v.x + this->x;
    this->y = v.y + this->y;
    this->z = v.z + this->z;
    return *this;
}

// VERIFY: a9b58487 auto
// OFFSET: 0x004829A0
C3Vector operator-(const C3Vector& l, const C3Vector& r) {
    C3Vector result;
    result.x = l.x - r.x;
    result.y = l.y - r.y;
    result.z = l.z - r.z;
    return result;
}

// VERIFY: 023d5589 manual
// OFFSET: 0x004BF500
bool operator==(const C3Vector& l, const C3Vector& r) {
    return r.x == l.x && r.y == l.y && r.z == l.z;
}

// VERIFY: 935caed1 manual
// OFFSET: 0x004BF540
bool operator!=(const C3Vector& l, const C3Vector& r) {
    return r.x != l.x || r.y != l.y || r.z != l.z;
}

// VERIFY: c262d6dc auto
// OFFSET: 0x004C1D20
C3Vector C3Vector::operator-() const {
    C3Vector result;
    result.x = -this->x;
    result.y = -this->y;
    result.z = -this->z;
    return result;
}

// VERIFY: 681282c6 manual
// OFFSET: 0x004C3420
void C3Vector::Normalize() {
    float scale = 1.0f / sqrt(this->z * this->z + this->y * this->y + this->x * this->x);
    this->x = this->x * scale;
    this->y = this->y * scale;
    this->z = scale * this->z;
}

// VERIFY: a70c3daa auto
// OFFSET: 0x004C35A0
C3Vector& C3Vector::operator*=(float s) {
    this->x = this->x * s;
    this->y = this->y * s;
    this->z = s * this->z;
    return *this;
}

// VERIFY: 2eb2846f auto
// OFFSET: 0x004C35D0
C3Vector& C3Vector::operator/=(float s) {
    float scale = 1.0f / s;
    this->x = this->x * scale;
    this->y = scale * this->y;
    this->z = scale * this->z;
    return *this;
}

// VERIFY: 460f2c58 manual
// OFFSET: 0x004C3600
void C3Vector::NormalizeSafe() {
    float length = this->z * this->z + this->y * this->y + this->x * this->x;

    if (length > 0.00000023841858f) {
        float scale = 1.0f / sqrt(length);
        this->x = this->x * scale;
        this->y = scale * this->y;
        this->z = scale * this->z;
    }
}

// VERIFY: 5d5f694d auto
// OFFSET: 0x005FEC70
C3Vector C3Vector::Cross(const C3Vector& a, const C3Vector& b) {
    C3Vector result;
    result.x = b.z * a.y - a.z * b.y;
    result.y = a.z * b.x - b.z * a.x;
    result.z = b.y * a.x - b.x * a.y;
    return result;
}

// VERIFY: 8d879f2a manual
// OFFSET: 0x00714D10
C3Vector C3Vector::Min(const C3Vector& a, const C3Vector& b) {
    C3Vector result;
    float z = b.z <= a.z ? b.z : a.z;
    float y = b.y <= a.y ? b.y : a.y;
    result.x = b.x <= a.x ? b.x : a.x;
    result.y = y;
    result.z = z;
    return result;
}

// VERIFY: 552b0392 manual
// OFFSET: 0x00714D70
C3Vector C3Vector::Max(const C3Vector& a, const C3Vector& b) {
    C3Vector result;
    float z = b.z >= a.z ? b.z : a.z;
    float y = b.y >= a.y ? b.y : a.y;
    result.x = b.x >= a.x ? b.x : a.x;
    result.y = y;
    result.z = z;
    return result;
}

// VERIFY: 0c1cd6c4 manual
// OFFSET: 0x00982950
C3Vector::C3Vector(const C4Vector& v) {
    this->x = v.x;
    this->y = v.y;
    this->z = v.z;
}

// VERIFY: 6a482f8d manual
// OFFSET: 0x00982970
C3Vector::C3Vector(const CImVector& color) {
    this->x = color.r * 0.0039215689f;
    this->y = color.g * 0.0039215689f;
    this->z = 0.0039215689f * color.b;
}

// VERIFY: 6cb56de3 manual
// OFFSET: 0x009829B0
C3Vector::EAxis C3Vector::MajorAxis() const {
    float y = fabs(this->y);
    float x = fabs(this->x);
    float z = fabs(this->z);

    if (x <= y) {
        if (y > z) {
            return C3AXIS_Y;
        }
    } else if (x > z) {
        return C3AXIS_X;
    }

    return C3AXIS_Z;
}

// VERIFY: 8564aa4a manual
// OFFSET: 0x009829F0
C3Vector::EAxis C3Vector::MinorAxis() const {
    float y = fabs(this->y);
    float x = fabs(this->x);
    float z = fabs(this->z);

    if (x >= y) {
        if (y <= z) {
            return C3AXIS_Y;
        }

        return C3AXIS_Z;
    }

    if (x >= z) {
        return C3AXIS_Z;
    }

    return C3AXIS_X;
}

// INLINED
float C3Vector::SquaredMag() const {
    return this->y * this->y + this->x * this->x + this->z * this->z;
}

// VERIFY: e6f533b8 auto
// OFFSET: 0x004C3650
C3Vector operator%(const C3Vector& l, const C2Vector& r) {
    C3Vector result;
    result.x = -(r.y * l.z);
    result.y = r.x * l.z;
    result.z = r.y * l.x - l.y * r.x;
    return result;
}

// VERIFY: cba1ea5b auto
// OFFSET: 0x005FECF0
float C3Vector::Distance(const C3Vector& a, const C3Vector& b) {
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    float dx = a.x - b.x;
    return sqrt(dx * dx + dz * dz + dy * dy);
}

// VERIFY: 9db61a8f auto
// OFFSET: 0x004BEC40
C3Vector operator*(const C3Vector& l, float r) {
    C3Vector result;
    result.x = l.x * r;
    result.y = l.y * r;
    result.z = r * l.z;
    return result;
}

// VERIFY: ac16577f auto
// OFFSET: 0x007E2CE0
void C3Vector::Minimize(const C3Vector& v) {
    *this = C3Vector::Min(*this, v);
}

// VERIFY: 1831a383 auto
// OFFSET: 0x007E2D20
void C3Vector::Maximize(const C3Vector& v) {
    *this = C3Vector::Max(*this, v);
}

// VERIFY: a2bee705 auto
// OFFSET: 0x00568680
C3Vector& C3Vector::operator-=(const C3Vector& v) {
    this->x = this->x - v.x;
    this->y = this->y - v.y;
    this->z = this->z - v.z;
    return *this;
}

// VERIFY: ae255bb1 auto
// OFFSET: 0x00978B70
void C3Vector::FlushDenormals() {
    if (0.0f != this->x && fabs(this->x) < 0.0000000099999999f) {
        this->x = 0.0f;
    }

    if (0.0f != this->y && fabs(this->y) < 0.0000000099999999f) {
        this->y = 0.0f;
    }

    if (0.0f != this->z && fabs(this->z) < 0.0000000099999999f) {
        this->z = 0.0f;
    }
}
