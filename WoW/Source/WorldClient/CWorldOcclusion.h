#ifndef WOW_SOURCE_WORLDCLIENT_CWORLDOCCLUSION_H
#define WOW_SOURCE_WORLDCLIENT_CWORLDOCCLUSION_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/C4Plane.h"
#include "Engine/Source/Storm/H/stpl.h"

class C3Vector;
class CAaBox;
class CAaSphere;

class CWorldOccluder {
    public:
    static void Initialize();
    static CWorldOccluder* Alloc();

    /* 0000 */ C3Vector m_points [2];
    /* 0018 */ TSLink<CWorldOccluder> m_link;

    void ClipBufferUpdate();
    void Recycle();
};

class CWorldAntiOccluder {
    public:
    /* 0000 */ uint8_t unused_0000[16];
    /* 0010 */ TSLink<CWorldAntiOccluder> m_link;
};

extern TSExplicitList<CWorldOccluder, offsetof(CWorldOccluder, m_link)> s_occluderFreeList;
extern TSExplicitList<CWorldAntiOccluder, offsetof(CWorldAntiOccluder, m_link)> s_antiOccluderFreeList;

class CClipVolume {
    public:
    /* 0000 */ uint32_t m_planeStart;
    /* 0004 */ uint32_t m_planeCount;
};

class CWorldOcclusion {
    public:
    static TSGrowableArray<C4Plane> planeArray;
    static TSGrowableArray<CClipVolume> clipVolumeArray;

    static void ClearVolumes();
    static uint32_t GetClipVolumeCount();
    static uint32_t QueryVolumes(CAaSphere* sphere);
    static uint32_t QueryVolumes(C3Vector* sphere, uint32_t count);
    static uint32_t QueryBuffer(CAaBox* box, uint8_t flags);
    static uint32_t QueryBuffer(CAaSphere* box, uint8_t flags);
    static uint32_t QueryBuffer(const C3Vector& center, float radius, uint8_t flags);
    static void UpdateBuffer(const C3Vector* points, int32_t count, uint32_t flags);
    static void UpdateBufferChunk(const C3Vector* verts, const float* heights, const uint32_t* indices, int32_t count, const C3Vector* offset, int32_t holes);
    static void AddVolume(const C3Vector* eye, const C3Vector* verts, int32_t count, int32_t clipToView);
};

#endif
