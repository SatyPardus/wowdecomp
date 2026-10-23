#ifndef ENGINE_SOURCE_SERVICES_CAMERA_H
#define ENGINE_SOURCE_SERVICES_CAMERA_H

#include <cstddef>
#include <cstdint>

// Object-camera IDs accepted by simple models; M2 camera array counts are separate.
enum : uint32_t {
    CAMERA_PORTRAIT = 0,
    CAMERA_PAPERDOLL = 1,
    NUM_OBJECT_CAMERAS = 2,
};

class C2Vector;
class C3Vector;
class CCamera;
class CDataMgr;
class CRect;

CCamera* CameraCreate();
CCamera* CameraDuplicate(CCamera* camera);
void CameraGetLineSegment(float x, float y, C3Vector* start, C3Vector* end);
float CameraGetFloat(CCamera* camera, uint32_t index);
void CameraSetCoord(CCamera* camera, uint32_t index, const C3Vector& value, uint32_t keepMask = 0);
void CameraSetFloat(CCamera* camera, uint32_t index, float value);
void CameraSetupScreenProjection(const CRect& rect, const C2Vector& offset, float depth, int32_t forceOrtho);
void CameraSetupWorldProjection(CCamera* camera, const CRect& rect, uint32_t flags);
void DataMgrGetCoord(CDataMgr* mgr, uint32_t index, C3Vector* value);

#endif
