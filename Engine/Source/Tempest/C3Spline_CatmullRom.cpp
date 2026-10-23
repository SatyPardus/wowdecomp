#include <cmath>
#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Spline_CatmullRom.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/Tempest/C34Matrix.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CMath.h"

// STATIC: 0x00AC37B8
static const C44Matrix s_catmullRomBasis(-0.5f, 1.0f, -0.5f, 0.0f, 1.5f, -2.5f, 0.0f, 1.0f, -1.5f, 2.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f);

// STATIC: 0x00AC37F8
static const C34Matrix s_catmullRomDer1 = { -1.5f, 2.0f, -0.5f, 4.5f, -5.0f, 0.0f, -4.5f, 4.0f, 0.5f, 1.5f, -1.0f, 0.0f };

// VERIFY: e9bcea7f auto
// OFFSET: 0x00409740
C3Spline_CatmullRom::C3Spline_CatmullRom() {
    this->splineMode = MODE_CATMULLROM;
}

// VERIFY: 0afb68ad auto
// OFFSET: 0x00409760
SPLINE_MODE C3Spline_CatmullRom::GetSplineMode() {
    return this->splineMode;
}

// VERIFY: f3bb76de manual
// OFFSET: 0x004C3FD0
void C3Spline_CatmullRom::Evaluate(uint32_t seg, float t, C3Vector& pos) {
    if (this->splineMode != MODE_LINEAR) {
        C3Spline::Evaluate(seg, t, s_catmullRomBasis, pos);
        return;
    }

    const C3Vector& a = this->Point(seg + 1);
    const C3Vector& b = this->Point(seg + 2);
    const C3Vector& base = this->Point(seg + 1);

    C3Vector result((b.x - a.x) * t + base.x, (b.y - a.y) * t + base.y, (b.z - a.z) * t + base.z);
    pos = result;
}

// VERIFY: f0c48125 manual
// OFFSET: 0x004C40B0
float C3Spline_CatmullRom::SegLength(uint32_t seg) {
    if (this->splineMode != MODE_LINEAR) {
        return C3Spline::SegLength(seg, s_catmullRomBasis);
    }

    const C3Vector& a = this->Point(seg + 1);
    const C3Vector& b = this->Point(seg + 2);

    float dy = b.y - a.y;
    float dz = b.z - a.z;
    float dx = b.x - a.x;

    return sqrt(dx * dx + dz * dz + dy * dy);
}

// VERIFY: 6f5dd97e manual
// OFFSET: 0x004C4140
void C3Spline_CatmullRom::ParametricSegT(float t, uint32_t& seg, float& segT) {
    float segCount = static_cast<float>(this->m_pointCount - 3);

    seg = CMath::fint_(segCount * t - 0.5f);
    segT = (t - 1.0f / segCount * seg) * segCount;
}

// VERIFY: 90d58011 auto
// OFFSET: 0x004C41B0
float C3Spline_CatmullRom::ILength() {
    return C3Spline::ILength(this->m_pointCount - 3);
}

// VERIFY: a9242506 manual
// OFFSET: 0x004C41C0
void C3Spline_CatmullRom::IValidateCache() {
    uint32_t segCount = this->m_pointCount - 3;

    for (uint32_t i = 0; i < segCount; i++) {
        this->SetCachedSegLength(this->SegLength(i), i);
    }
}

// VERIFY: 093a92f1 auto
// OFFSET: 0x004C4230
void C3Spline_CatmullRom::IPosArclength(float t, C3Vector& pos) {
    uint32_t seg;
    float segT;

    this->ArclengthSegT(t, s_catmullRomBasis, this->m_pointCount - 3, seg, segT);
    this->Evaluate(seg, segT, pos);
}

// VERIFY: 20c0de0e auto
// OFFSET: 0x004C4280
void C3Spline_CatmullRom::IPosParametric(float t, C3Vector& pos) {
    uint32_t seg;
    float segT;

    this->ParametricSegT(t, seg, segT);
    this->Evaluate(seg, segT, pos);
}

// VERIFY: 7e41051b auto
// OFFSET: 0x004C42C0
void C3Spline_CatmullRom::IVelArclength(float t, C3Vector& vel) {
    float clamped;

    if (t < 0.0f) {
        clamped = 0.0f;
    } else if (t < 1.0f) {
        clamped = t;
    } else {
        clamped = 1.0f;
    }

    uint32_t seg;
    float segT;

    this->ArclengthSegT(clamped, s_catmullRomBasis, this->m_pointCount - 3, seg, segT);
    C3Spline::EvaluateDer1(seg, segT, C44Matrix(s_catmullRomDer1), vel);
}

// VERIFY: f98a7b11 auto
// OFFSET: 0x004C4340
void C3Spline_CatmullRom::IVelParametric(float t, C3Vector& vel) {
    float clamped;

    if (t < 0.0f) {
        clamped = 0.0f;
    } else if (t < 1.0f) {
        clamped = t;
    } else {
        clamped = 1.0f;
    }

    uint32_t seg;
    float segT;

    this->ParametricSegT(clamped, seg, segT);
    C3Spline::EvaluateDer1(seg, segT, C44Matrix(s_catmullRomDer1), vel);
}

// VERIFY: 9b45b540 manual
// OFFSET: 0x004C43B0
void C3Spline_CatmullRom::IFrameArclength(float t, C44Matrix& frame) {
    uint32_t seg;
    float segT;

    this->ArclengthSegT(t, s_catmullRomBasis, this->m_pointCount - 3, seg, segT);
    this->Evaluate(seg, segT, *reinterpret_cast<C3Vector*>(&frame.d0));

    const C3Vector& a = this->Point(seg + 1);
    const C3Vector& b = this->Point(seg + 2);

    C3Vector dir(b.x - a.x, b.y - a.y, b.z - a.z);
    float dirSq = dir.z * dir.z + dir.y * dir.y + dir.x * dir.x;

    if (dirSq > 0.00000023841858f) {
        float inv = 1.0f / sqrt(dirSq);
        dir.x = dir.x * inv;
        dir.y = dir.y * inv;
        dir.z = inv * dir.z;
    }

    if (this->splineMode != MODE_LINEAR) {
        C3Vector vel(0.0f, 0.0f, 0.0f);
        C3Spline::EvaluateDer1(seg, segT, C44Matrix(s_catmullRomDer1), vel);

        float velSq = vel.z * vel.z + vel.y * vel.y + vel.x * vel.x;

        if (velSq > 0.000099999997f) {
            float inv = 1.0f / sqrt(velSq);
            vel.x = vel.x * inv;
            vel.y = vel.y * inv;
            vel.z = inv * vel.z;

            if (vel.z * dir.z + vel.y * dir.y + vel.x * dir.x < 0.5f) {
                frame.a0 = dir.x;
                frame.a1 = dir.y;
                frame.a2 = dir.z;
            } else {
                frame.a0 = vel.x;
                frame.a1 = vel.y;
                frame.a2 = vel.z;
            }
        }
    } else {
        frame.a0 = dir.x;
        frame.a1 = dir.y;
        frame.a2 = dir.z;
    }

    frame.b0 = -frame.a1;
    frame.b1 = frame.a0;
    frame.b2 = 0.0f;

    float sideSq = frame.b1 * frame.b1 + frame.b0 * frame.b0;

    if (sideSq > 0.00000023841858f) {
        float inv = 1.0f / sqrt(sideSq);
        frame.b0 = inv * frame.b0;
        frame.b1 = inv * frame.b1;
    }

    frame.c0 = -(frame.a2 * frame.b1);
    frame.c1 = frame.a2 * frame.b0;
    frame.c2 = frame.a0 * frame.b1 - frame.a1 * frame.b0;
}

// VERIFY: 936d407a manual
// OFFSET: 0x004C4600
uint32_t C3Spline_CatmullRom::IGetPoints(float t, C3Vector* points, uint32_t count) {
    uint32_t pointCount = this->m_pointCount;
    uint32_t seg;
    float segT;

    this->ArclengthSegT(t, s_catmullRomBasis, pointCount - 3, seg, segT);

    uint32_t remaining = pointCount - seg - 3;

    if (!points) {
        return remaining;
    }

    if (remaining >= count) {
        remaining = count;
    }

    for (uint32_t i = 0; i < remaining; i++) {
        points[i] = this->Point(seg + 2 + i);
    }

    return remaining;
}

// VERIFY: ad1247b6 auto
// OFFSET: 0x004C4DA0
void C3Spline_CatmullRom::ISetPoints(const C3Vector* points, uint32_t count) {
    if (count) {
        uint32_t segCount = count - 3;
        this->m_segCount = segCount;

        if (segCount > 25) {
            this->m_extraSegLength.SetCount(segCount - 25);
        }
    } else {
        this->m_segCount = 0;
        this->m_extraSegLength.SetCount(0);
    }

    C3Spline::ISetPoints(points, count);
}

// VERIFY: 4e0fa75c manual
// OFFSET: 0x004F4AE0
CDataStore& operator>>(CDataStore& msg, C3Spline_CatmullRom& spline) {
    uint32_t count = 0;
    msg.Get(count);

    void* points;
    msg.GetDataInSitu(points, sizeof(C3Vector) * count);

    uint8_t mode;
    msg.Get(mode);
    spline.splineMode = static_cast<SPLINE_MODE>(mode);

    if (count && msg.m_read <= msg.m_size) {
        spline.SetPoints(static_cast<const C3Vector*>(points), count);
    }

    return msg;
}
