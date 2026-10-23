#ifndef ENGINE_SOURCE_TEMPEST_C3VECTOR_H
#define ENGINE_SOURCE_TEMPEST_C3VECTOR_H

#include <cstddef>
#include <cstdint>

class C2Vector;
class C4Vector;
class CImVector;

class C3Vector {
    public:
    enum EAxis {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2,
        C3AXIS_X = 0,
        C3AXIS_Y = 1,
        C3AXIS_Z = 2
    };

    static C3Vector Cross(const C3Vector& a, const C3Vector& b);
    static float Distance(const C3Vector& a, const C3Vector& b);
    static C3Vector Max(const C3Vector& a, const C3Vector& b);
    static C3Vector Min(const C3Vector& a, const C3Vector& b);

    /* 0000 */ float x;
    /* 0004 */ float y;
    /* 0008 */ float z;

    C3Vector();
    C3Vector(float x, float y, float z);
    C3Vector(const C4Vector& v);
    C3Vector(const CImVector& color);
    EAxis MajorAxis() const;
    EAxis MinorAxis() const;
    void Maximize(const C3Vector& v);
    void Minimize(const C3Vector& v);
    void Normalize();
    void NormalizeSafe();
    void FlushDenormals();
    float SquaredMag() const;
    void Zero();
    C3Vector operator-() const;
    C3Vector& operator+=(const C3Vector& v);
    C3Vector& operator-=(const C3Vector& v);
    C3Vector& operator*=(float s);
    C3Vector& operator/=(float s);
};

C3Vector operator+(const C3Vector& l, const C3Vector& r);
C3Vector operator-(const C3Vector& l, const C3Vector& r);
C3Vector operator*(const C3Vector& l, float r);
bool operator==(const C3Vector& l, const C3Vector& r);
bool operator!=(const C3Vector& l, const C3Vector& r);
C3Vector operator%(const C3Vector& l, const C2Vector& r);

// INLINED
inline C3Vector::C3Vector() {
    this->x = 0.0f;
    this->y = 0.0f;
    this->z = 0.0f;
}

// INLINED
inline C3Vector::C3Vector(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
}

#endif
