#include "CSimpleModelFFX.h"
#include <cstdint>
#include <new>
#include <typeinfo>
#include "Engine/Source/FFX/FFX.h"
#include "Engine/Source/Frame/CRenderBatch.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CImVector.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/SoundInterface/SoundInterface2.h"

// STATIC: 0x00B6B48C
CDataAllocator CSimpleModelFFX::s_simpleModelFFXHeap(sizeof(CSimpleModelFFX), 5);

// VERIFY: 14316163 auto
// OFFSET: 0x004E5EA0
static void AnimEventCallback(CM2Model* model, uint32_t boneId, uint32_t eventId, uint32_t data, const C3Vector* position, uint32_t time, uint64_t param) {
    if (eventId == 0x4F534424) {
        SI2::PlaySoundKit(static_cast<int32_t>(data), nullptr, nullptr, nullptr, 0, nullptr, 1, 0);
    }
}

// VERIFY: 03559a15 auto
// OFFSET: 0x004E5ED0
void CSimpleModelFFX::SetModel(CM2Model* model) {
    this->CSimpleModel::SetModel(model);
    this->m_model->SetEventCallback(reinterpret_cast<void*>(AnimEventCallback), reinterpret_cast<uintptr_t>(this));
}

// VERIFY: 2698486b auto
// OFFSET: 0x004E5F00
void CSimpleModelFFX::LoadXML(XMLNode* node, CStatus* status) {
    this->CSimpleModel::LoadXML(node, status);

    const char* glow = node->GetAttributeByName("glow");

    if (glow && *glow) {
        this->SetGlow(SStrToFloat(glow));
    }
}

// VERIFY: 8359afe8 auto
// OFFSET: 0x004E5F50
void CSimpleModelFFX::OnLayerShow() {
    FFX::SetEffect(CGlueMgr::m_glowEffect);
    this->CSimpleFrame::OnLayerShow();
}

// INLINED
CM2LightArray::CM2LightArray() {
    this->m_lightCount = 0;
    this->m_hasCustomLight = 0;
    this->unk_01B5 = 0;
    this->m_modelFrame = nullptr;
    this->unk_01BC = 0;
}

// VERIFY: b99cd56f manual
// OFFSET: 0x004E5F70
CSimpleModelFFX::CSimpleModelFFX(CSimpleFrame* parent) : CSimpleModel(parent) {
    this->m_glow = 0.3f;
    this->m_lightArray1[0].m_modelFrame = this;
    this->m_lightArray1[1].m_modelFrame = this;
    this->m_lightArray2[0].m_modelFrame = this;
    this->m_lightArray2[1].m_modelFrame = this;
    this->m_lightArray3[0].m_modelFrame = this;
    this->m_lightArray3[1].m_modelFrame = this;
}

// VERIFY: 107d2001 auto
// OFFSET: 0x004E6160
void CSimpleModelFFX::SetGlow(float glow) {
    this->m_glow = glow;
}

// VERIFY: 47b33625 auto
// OFFSET: 0x004E6170
int32_t CSimpleModelFFX::GetScriptMetaTable() {
    return CSimpleModelFFX::s_metatable;
}

// VERIFY: 6b63ac80 manual
// OFFSET: 0x004E6190
void CSimpleModelFFX::Render(void* arg) {
    CSimpleModelFFX* model = static_cast<CSimpleModelFFX*>(arg);

    CImVector clearColor;
    clearColor.b = 0x00;
    clearColor.g = 0x00;
    clearColor.r = 0x00;
    clearColor.a = 0xFF;
    GxSceneClear(3, clearColor);

    int32_t params[3];
    params[0] = 0;
    params[1] = static_cast<uint8_t>(model->m_glow * 255.0f);
    params[2] = 0;
    CGlueMgr::m_glowEffect->SetParameters(3, params);

    FFX::Start();

    clearColor.b = 0x00;
    clearColor.g = 0x00;
    clearColor.r = 0x00;
    clearColor.a = 0xFF;
    GxSceneClear(3, clearColor);

    if (model->m_model) {
        CSimpleModel::RenderModel(model);
    }

    FFX::Finish();
}

// VERIFY: 68232335 manual
// OFFSET: [0x004E6230, 0x004E62D0]
CSimpleModelFFX::~CSimpleModelFFX() {
}

// INLINED
void CSimpleModelFFX::operator delete(void* ptr) {
    if (ptr) {
        CSimpleModelFFX::s_simpleModelFFXHeap.PutData(ptr, nullptr, 0);
    }
}

// VERIFY: 9f5b0582 auto
// OFFSET: 0x004E6300
CSimpleFrame* CSimpleModelFFX::Create(CSimpleFrame* parent) {
    void* data = CSimpleModelFFX::s_simpleModelFFXHeap.GetData(0, STORM_TYPE_NAME(CSimpleModelFFX), SLOG_OBJECT);
    return data ? new (data) CSimpleModelFFX(parent) : nullptr;
}

// VERIFY: e9124733 auto
// OFFSET: 0x004E6330
void CSimpleModelFFX::OnFrameRender(CRenderBatch* batch, uint32_t layer) {
    this->CSimpleFrame::OnFrameRender(batch, layer);

    if (layer == 2) {
        batch->QueueCallback(CSimpleModelFFX::Render, this);
    }
}
