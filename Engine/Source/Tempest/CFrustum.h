#ifndef ENGINE_SOURCE_TEMPEST_CFRUSTUM_H
#define ENGINE_SOURCE_TEMPEST_CFRUSTUM_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Plane.h"

class C44Matrix;
class CAaBox;
class CAaSphere;

enum FRUSTUM_CORNER : uint32_t {
    NEAR_LL = 0,
    NEAR_UL = 1,
    NEAR_UR = 2,
    NEAR_LR = 3,
    FAR_LL = 4,
    FAR_UL = 5,
    FAR_UR = 6,
    FAR_LR = 7,
    NUM_CORNERS = 8,
};

enum FRUSTUM_PLANE : uint32_t {
    P_TOP = 0,
    P_BOTTOM = 1,
    P_LEFT = 2,
    P_RIGHT = 3,
    P_FAR = 4,
    P_NEAR = 5,
    NUM_PLANES = 6,
};

enum WorldCullStatus : uint32_t {
    WorldCull_outside = 0,
    WorldCull_notOutside = 3,

    // Unused 0.5.3 definitions; current 3.3.5 status paths still emit only 0 or 3.
    // WorldCull_inside = 1,
    // WorldCull_intersect = 2,
    // WorldCull_count = 4,
};

class CFrustum {
    public:
    /* 0000 */ C4Plane planes[NUM_PLANES];
    /* 0060 */ C3Vector corners[NUM_CORNERS];
    /* 00C0 */ C3Vector lookPos;
    /* 00CC */ C3Vector lookAt;
    /* 00D8 */ C3Vector lookUp;
    /* 00E4 */ float fovy;
    /* 00E8 */ float aspect;
    /* 00EC */ float minz;
    /* 00F0 */ float maxz;

    CFrustum();
    CFrustum(const C3Vector* corners);
    void CalcPlanesFromCorners();
    void CalcPlanesFromCorners(const C3Vector* corners);
    WorldCullStatus Cull(const CAaBox& box);
    WorldCullStatus Cull(const CAaSphere& sphere);
    void Cull(const C3Vector& point, uint8_t& outside) const;
    WorldCullStatus Cull(const C3Vector& center, float radius);
    WorldCullStatus CullInv(const CAaBox& box);
    void Extents(CAaBox& box);
    void InvertPlanes();
    void SwapCorners();
    void Transform(const C44Matrix& m);
    void Translate(const C3Vector& v);
    CFrustum& operator=(const CFrustum& source);
};

#endif
