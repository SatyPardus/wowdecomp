#ifndef ENGINE_SOURCE_TEMPEST_C3SPLINE_H
#define ENGINE_SOURCE_TEMPEST_C3SPLINE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Storm/H/stpl.h"

class C44Matrix;

enum SPLINE_MODE : int32_t {
  MODE_LINEAR = 0x0,
  MODE_CATMULLROM = 0x1,
};

class C3Spline {
    public:
    enum EvalType {
        EVAL_PARAMETRIC = 0,
        EVAL_ARCLENGTH = 1,
        EVAL_COUNT = 0x2,
    };

    /* 0004 */ float cachedLength;
    /* 0008 */ C3Vector points [25];
    /* 0134 */ TSGrowableArray<C3Vector> m_extraPoints;
    /* 0144 */ uint32_t m_pointCount;
    /* 0148 */ float cachedSegLength [25];
    /* 01AC */ TSGrowableArray<float> m_extraSegLength;
    /* 01BC */ uint32_t m_segCount;

    C3Spline();
    void ArclengthSegT(float t, const C44Matrix& basis, uint32_t segCount, uint32_t& seg, float& segT);
    void Evaluate(uint32_t seg, float t, const C44Matrix& basis, C3Vector& pos);
    void EvaluateDer1(uint32_t seg, float t, const C44Matrix& basis, C3Vector& vel);
    void Frame(float t, C44Matrix& frame, EvalType type);
    float GetCachedSegLength(uint32_t index);
    void GetPoints(C3Vector* points, uint32_t count);
    float ILength(uint32_t segCount);
    float LengthBeforePoint(uint32_t index);
    C3Vector& Point(uint32_t index);
    void Pos(float t, C3Vector& pos, EvalType type);
    float SegLength(uint32_t seg, const C44Matrix& basis);
    void SetCachedSegLength(float length, uint32_t index);
    void SetPoints(const C3Vector* points, uint32_t count);
    void Vel(float t, C3Vector& vel, EvalType type);
    C3Spline& operator=(const C3Spline& source);

    virtual float ILength() = 0;
    virtual void IValidateCache() = 0;
    virtual void IPosArclength(float t, C3Vector& pos) = 0;
    virtual void IPosParametric(float t, C3Vector& pos) = 0;
    virtual void IVelArclength(float t, C3Vector& vel) = 0;
    virtual void IVelParametric(float t, C3Vector& vel) = 0;
    virtual void IFrameArclength(float t, C44Matrix& frame) = 0;
    virtual void ISetPoints(const C3Vector* points, uint32_t count);
    virtual uint32_t IGetPoints(float t, C3Vector* points, uint32_t count);
    virtual SPLINE_MODE GetSplineMode();
};

#endif
