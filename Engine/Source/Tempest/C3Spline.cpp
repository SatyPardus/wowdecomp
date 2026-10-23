#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "Engine/Source/Tempest/C3Spline.h"
#include "Engine/Source/Tempest/C44Matrix.h"

// VERIFY: f58fcef3 manual
// OFFSET: 0x004096D0
C3Spline::C3Spline() {
    this->m_pointCount = 0;
    this->m_segCount = 0;
}

// INLINED
float C3Spline::GetCachedSegLength(uint32_t index) {
    if (index >= 25) {
        return this->m_extraSegLength[index - 25];
    }

    return this->cachedSegLength[index];
}

// INLINED
void C3Spline::SetCachedSegLength(float length, uint32_t index) {
    if (index >= 25) {
        this->m_extraSegLength[index - 25] = length;
    } else {
        this->cachedSegLength[index] = length;
    }
}

// VERIFY: a8388e46 manual
// OFFSET: 0x004C36F0
C3Vector& C3Spline::Point(uint32_t index) {
    if (index >= 25) {
        return this->m_extraPoints[index - 25];
    }

    return this->points[index];
}

// VERIFY: 6a282ff5 manual
// OFFSET: 0x004C3720
float C3Spline::LengthBeforePoint(uint32_t index) {
    float length = 0.0f;

    if (index) {
        for (uint32_t i = 0; i < index - 1; i++) {
            length += this->GetCachedSegLength(i);
        }
    }

    return length;
}

// VERIFY: 73880b5b auto
// OFFSET: 0x004C3830
void C3Spline::SetPoints(const C3Vector* points, uint32_t count) {
    this->ISetPoints(points, count);

    if (this->m_pointCount > 3) {
        this->IValidateCache();
        this->cachedLength = this->ILength();
    }
}

// VERIFY: 6f441caf manual
// OFFSET: 0x004C3870
void C3Spline::Pos(float t, C3Vector& pos, EvalType type) {
    if (t <= 0.0f) {
        pos = this->points[0];
        return;
    }

    if (t >= 1.0f) {
        pos = this->Point(this->m_pointCount - 1);
        return;
    }

    if (type == EVAL_PARAMETRIC) {
        this->IPosParametric(t, pos);
    } else if (type == EVAL_ARCLENGTH) {
        this->IPosArclength(t, pos);
    }
}

// VERIFY: 42c46af7 manual
// OFFSET: 0x004C3920
void C3Spline::Vel(float t, C3Vector& vel, EvalType type) {
    float clamped;

    if (t < 0.0f) {
        clamped = 0.0f;
    } else if (t < 1.0f) {
        clamped = t;
    } else {
        clamped = 1.0f;
    }

    if (type == EVAL_PARAMETRIC) {
        this->IVelParametric(clamped, vel);
    } else if (type == EVAL_ARCLENGTH) {
        this->IVelArclength(clamped, vel);
    }
}

// VERIFY: dbee021c manual
// OFFSET: 0x004C3980
void C3Spline::Frame(float t, C44Matrix& frame, EvalType type) {
    float clamped;

    if (t < 0.0f) {
        clamped = 0.0f;
    } else if (t < 1.0f) {
        clamped = t;
    } else {
        clamped = 1.0f;
    }

    if (type == EVAL_ARCLENGTH) {
        this->IFrameArclength(clamped, frame);
    }
}

// VERIFY: 551adffe manual
// OFFSET: 0x004C39D0
void C3Spline::Evaluate(uint32_t seg, float t, const C44Matrix& basis, C3Vector& pos) {
    const float* row = &basis.a0;

    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = 0.0f;

    for (uint32_t i = 0; i < 4; i++) {
        float weight = ((row[0] * t + row[1]) * t + row[2]) * t + row[3];
        const C3Vector& point = this->Point(seg + i);

        pos.x += weight * point.x;
        pos.y += point.y * weight;
        pos.z += weight * point.z;

        row += 4;
    }
}

// VERIFY: 791c078d manual
// OFFSET: 0x004C3A70
void C3Spline::EvaluateDer1(uint32_t seg, float t, const C44Matrix& basis, C3Vector& vel) {
    const float* row = &basis.a0;

    vel.x = 0.0f;
    vel.y = 0.0f;
    vel.z = 0.0f;

    for (uint32_t i = 0; i < 4; i++) {
        float weight = (row[0] * t + row[1]) * t + row[2];
        const C3Vector& point = this->Point(seg + i);

        vel.x += weight * point.x;
        vel.y += point.y * weight;
        vel.z += weight * point.z;

        row += 4;
    }
}

// VERIFY: a6dddf58 auto
// OFFSET: 0x004C3B10
float C3Spline::SegLength(uint32_t seg, const C44Matrix& basis) {
    float step = 0.05f;
    float length = 0.0f;
    C3Vector prev;
    C3Vector cur;

    this->Evaluate(seg, 0.0f, basis, prev);

    for (int32_t i = 20; i; i--) {
        this->Evaluate(seg, step, basis, cur);

        float dx = cur.x - prev.x;
        float dy = cur.y - prev.y;
        float dz = cur.z - prev.z;
        prev = cur;

        length = sqrt(dx * dx + dy * dy + dz * dz) + length;
        step = step + 0.05f;
    }

    return length;
}

// VERIFY: c8ceaa76 manual
// OFFSET: 0x004C3BD0
void C3Spline::ArclengthSegT(float t, const C44Matrix& basis, uint32_t segCount, uint32_t& seg, float& segT) {
    if (segCount <= 1) {
        seg = 0;
        segT = t;
        return;
    }

    float target = this->cachedLength * t;
    float before = 0.0f;
    seg = 0;

    do {
        float after = this->GetCachedSegLength(seg) + before;

        if (after > target) {
            break;
        }

        before = after;
        seg++;
    } while (seg < segCount - 1);

    segT = (target - before) / this->GetCachedSegLength(seg);
}

// VERIFY: 735c6cce manual
// OFFSET: 0x004C3C80
float C3Spline::ILength(uint32_t segCount) {
    float length = 0.0f;

    for (uint32_t i = 0; i < segCount; i++) {
        length += this->GetCachedSegLength(i);
    }

    return length;
}

// VERIFY: 982a9f86 manual
// OFFSET: 0x004C3D80
void C3Spline::GetPoints(C3Vector* points, uint32_t count) {
    uint32_t n = count >= this->m_pointCount ? this->m_pointCount : count;

    for (uint32_t i = 0; i < n; i++) {
        if (i >= 25) {
            break;
        }

        points[i] = this->points[i];
    }

    if (n > 25) {
        memcpy(&points[25], this->m_extraPoints.m_data, sizeof(C3Vector) * (n - 25));
    }
}

// VERIFY: 1e5ef38f manual
// OFFSET: 0x004C4CD0
void C3Spline::ISetPoints(const C3Vector* points, uint32_t count) {
    this->m_pointCount = count;

    for (uint32_t i = 0; i < count; i++) {
        if (i >= 25) {
            break;
        }

        this->points[i] = points[i];
    }

    if (count <= 25) {
        this->m_extraPoints.SetCount(0);
    } else {
        this->m_extraPoints.Set(count - 25, &points[25]);
    }
}

// VERIFY: 9a5cc932 manual
// OFFSET: 0x006EBE80
C3Spline& C3Spline::operator=(const C3Spline& source) {
    this->cachedLength = source.cachedLength;

    for (uint32_t i = 0; i < 25; i++) {
        this->points[i] = source.points[i];
    }

    this->m_extraPoints = source.m_extraPoints;
    this->m_pointCount = source.m_pointCount;

    for (uint32_t i = 0; i < 25; i++) {
        this->cachedSegLength[i] = source.cachedSegLength[i];
    }

    this->m_extraSegLength = source.m_extraSegLength;
    this->m_segCount = source.m_segCount;

    return *this;
}

// VERIFY: 2d50a3f0 auto
// OFFSET: 0x008E5250
SPLINE_MODE C3Spline::GetSplineMode() {
    return MODE_LINEAR;
}

// VERIFY: aa7f98b5 auto
// OFFSET: 0x00946080
uint32_t C3Spline::IGetPoints(float t, C3Vector* points, uint32_t count) {
    return 0;
}
