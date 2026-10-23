#include "WorldView.h"
#include "WoW/Source/WorldClient/WorldConstants.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/WorldClient/CWorldOcclusion.h"
#include "WoW/Source/WorldClient/SortTable.h"
#include "WoW/Source/WorldClient/World.h"

// STATIC: 0x00ADF378
float CWorldView::s_fadeSize[5] = { 1.0f, 4.0f, 15.0f, 100.0f, 100000.0f };

// STATIC: 0x00ADF3B4
float CWorldView::s_fadeDistSqr[5] = { 900.0f, 10000.0f, 40000.0f, 562500.0f, 1562500.0f };

// VERIFY: 54af8fd2 manual
// OFFSET: 0x0078FB60
int16_t CWorldView::GetFadeLevelForDistance(float distance) {
    if (distance < 0.0f) {
        return 0;
    }

    float distanceSqr = distance * distance;

    if (CWorldView::s_fadeDistSqr[0] > distanceSqr) {
        return 0;
    }

    if (CWorldView::s_fadeDistSqr[1] > distanceSqr) {
        return 1;
    }

    if (CWorldView::s_fadeDistSqr[2] > distanceSqr) {
        return 2;
    }

    if (distanceSqr >= CWorldView::s_fadeDistSqr[3]) {
        return 4;
    }

    return 3;
}

// VERIFY: ee050906 auto
// OFFSET: 0x007927E0
void CWorldView::AddOccluder(const C3Vector& a, const C3Vector& b) {
    if (CWorldScene::camVec.z < -0.89999998f || CWorldScene::camVec.z > 0.89999998f) {
        return;
    }

    const C4Plane& plane = CWorldScene::camPlaneXY;
    float distA = a.x * plane.n.x + a.z * plane.n.z + a.y * plane.n.y + plane.d;
    float distB = plane.d + plane.n.x * b.x + plane.n.z * b.z + plane.n.y * b.y;

    if (distA < 0.1f && distB < 0.1f) {
        return;
    }

    int32_t bucketA = CMath::fint_(distA * CBDIVISOR - 0.5f);
    int32_t bucketB = CMath::fint_(distB * CBDIVISOR - 0.5f);

    if (bucketA >= 64 && bucketB >= 64) {
        return;
    }

    CWorldOccluder* occluder = CWorldOccluder::Alloc();
    occluder->m_points[0] = a;
    occluder->m_points[1] = b;
    int32_t bucket = bucketA;

    if (bucketA != bucketB) {
        C3Vector nearPoint;
        C3Vector farPoint;
        int32_t first;
        int32_t last;

        if (bucketA <= bucketB) {
            nearPoint = occluder->m_points[0];
            farPoint = b;
            first = bucketA;
            last = bucketB;
        } else {
            nearPoint = b;
            farPoint = occluder->m_points[0];
            first = bucketB;
            last = bucketA;
        }

        if (first < 0) {
            first = 0;
        }

        if (last >= 64) {
            last = 63;
        }

        if (first < last) {
            for (int32_t i = first; i < last; i++) {
                const C4Vector& split = CWorldScene::sortTable.m_bucketPlanes[i + 1];
                float farDist = split.y * farPoint.y + split.z * farPoint.z + split.x * farPoint.x + split.w;
                float delta = farDist - (split.x * nearPoint.x + split.y * nearPoint.y + split.z * nearPoint.z + split.w);
                C3Vector point;

                if (delta <= 0.0000099999997f) {
                    point = farPoint;
                } else {
                    float t = farDist / delta;
                    point.x = farPoint.x + (nearPoint.x - farPoint.x) * t;
                    point.y = farPoint.y + (nearPoint.y - farPoint.y) * t;
                    point.z = farPoint.z + t * (nearPoint.z - farPoint.z);
                }

                CWorldOccluder* piece = CWorldOccluder::Alloc();
                piece->m_points[0] = nearPoint;
                piece->m_points[1] = point;
                CWorldScene::sortTable.m_table[i].m_occluderList.LinkToTail(piece);
                nearPoint = point;
            }

            first = last;
        }

        occluder->m_points[0] = nearPoint;
        occluder->m_points[1] = farPoint;
        bucket = first;
    }

    CWorldScene::sortTable.m_table[bucket].m_occluderList.LinkToTail(occluder);
}
