#ifndef WOW_SOURCE_WORLDCLIENT_MAPDOODADDEF_H
#define WOW_SOURCE_WORLDCLIENT_MAPDOODADDEF_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/WorldClient/MapLoad.h"
#include "WoW/Source/Object/WGUID.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

class CM2Lighting;
class CMapBaseObj;
class CMapObjDef;

class CMapDoodadDef : public CMapStaticEntity, public TSHashObject<CMapDoodadDef, HASHKEY_DWORD> {
    public:
    ~CMapDoodadDef() override;
    static void CreateModel(const char* fileName, CMapDoodadDef* doodadDef, int32_t waitForLoad, int32_t a4);
    static void ExtendBounds(CMapDoodadDef* doodadDef, CMapBaseObj* parent);
    static void Purge(CMapDoodadDef* doodadDef);

    /* 00A8 */ TSLink<CMapDoodadDef> m_doodadDefLink;
    /* 00B0 */ int32_t m_cullPassStamp;
    /* 00B4 */ int32_t unused_00B4;
    /* 00B8 */ WOWGUID m_collisionGuid;
    /* 00C0 */ CAaBox m_bboxDoodadDef;
    /* 00D8 */ C44Matrix m_mat;
    /* 0118 */ C44Matrix m_identity;
    /* 0158 */ SE2SoundKitObject m_soundKitObject;
    /* 016C */ uint16_t m_doodadSet;
    /* 016E */ uint8_t unused_016E[2];

    virtual void SelectLights(CM2Lighting* lighting);
    virtual void SelectUnderwater(CM2Lighting* lighting);
    virtual bool QueryInteriorLighting(CMapObjDef* mapObjDef, int32_t groupNum, uint16_t* faceIndex, C3Vector* pos);

    CMapDoodadDef();
    void UpdateBounds();
    void ExtendChunkBounds();
    void UpdateMatrixOnly(const C44Matrix& matrix);
};

#endif
