#ifndef ENGINE_SOURCE_TEMPEST_C3SPLINE_CATMULLROM_H
#define ENGINE_SOURCE_TEMPEST_C3SPLINE_CATMULLROM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Spline.h"

class CDataStore;

class C3Spline_CatmullRom : public C3Spline {
    public:

    /* 01C0 */ SPLINE_MODE splineMode;

    C3Spline_CatmullRom();
    void Evaluate(uint32_t seg, float t, C3Vector& pos);
    void ParametricSegT(float t, uint32_t& seg, float& segT);
    float SegLength(uint32_t seg);

    float ILength() override;
    void IValidateCache() override;
    void IPosArclength(float t, C3Vector& pos) override;
    void IPosParametric(float t, C3Vector& pos) override;
    void IVelArclength(float t, C3Vector& vel) override;
    void IVelParametric(float t, C3Vector& vel) override;
    void IFrameArclength(float t, C44Matrix& frame) override;
    void ISetPoints(const C3Vector* points, uint32_t count) override;
    uint32_t IGetPoints(float t, C3Vector* points, uint32_t count) override;
    SPLINE_MODE GetSplineMode() override;
};

CDataStore& operator>>(CDataStore& msg, C3Spline_CatmullRom& spline);

#endif
