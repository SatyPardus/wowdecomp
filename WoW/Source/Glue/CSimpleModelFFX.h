#ifndef WOW_SOURCE_GLUE_CSIMPLEMODELFFX_H
#define WOW_SOURCE_GLUE_CSIMPLEMODELFFX_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/Model2/M2Light.h"

class CM2Model;
class CRenderBatch;
class CSimpleModelFFX;
class CStatus;
class XMLNode;

class CM2LightArray {
    public:
    /* 0000 */ uint32_t m_lightCount;
    /* 0004 */ CM2Light m_lights [4];
    /* 01B4 */ uint8_t m_hasCustomLight;
    /* 01B5 */ uint8_t unk_01B5;
    /* 01B6 */ uint8_t unused_01B6;
    /* 01B7 */ uint8_t unused_01B7;
    /* 01B8 */ CSimpleModelFFX* m_modelFrame;
    /* 01BC */ uint8_t unk_01BC;
    /* 01BD */ uint8_t unused_01BD;
    /* 01BE */ uint8_t unused_01BE;
    /* 01BF */ uint8_t unused_01BF;

    CM2LightArray();
};

class CSimpleModelFFX : public CSimpleModel {
    public:
    /* 0368 */ CM2LightArray m_lightArray1 [2];
    /* 06E8 */ CM2LightArray m_lightArray2 [2];
    /* 0A68 */ CM2LightArray m_lightArray3 [2];
    /* 0DE8 */ float m_glow;

    static CDataAllocator s_simpleModelFFXHeap;
    static int32_t s_metatable;

    static void operator delete(void* ptr);
    static CSimpleFrame* Create(CSimpleFrame* parent);
    static void CreateScriptMetaTable();
    static void RegisterScriptMethods(lua_State* L);
    static void Render(void* arg);

    using CSimpleModel::SetModel;

    virtual ~CSimpleModelFFX();
    virtual int32_t GetScriptMetaTable();
    virtual void OnLayerShow();
    virtual void OnFrameRender(CRenderBatch* batch, uint32_t layer);
    virtual void SetModel(CM2Model* model);
    virtual void SetGlow(float glow);
    virtual void LoadXML(XMLNode* node, CStatus* status);

    CSimpleModelFFX(CSimpleFrame* parent);
};

#endif
