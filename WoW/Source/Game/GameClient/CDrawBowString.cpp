#include "CDrawBowString.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/Object/ObjectClient/Unit_C.h"

// VERIFY: b6048102 manual
// OFFSET: 0x007EA9B0
void CDrawBowString::DrawBowString(CM2Model* model, const CM2Lighting* lighting, CGUnit_C* unit) {
    if (unit && !unit->m_worldModel) {
        return;
    }

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPC), 3);
    CGxVertexPC* vertices = reinterpret_cast<CGxVertexPC*>(g_theGxDevicePtr->BufLock(vertexBuf));
    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), 4);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));

    const C3Vector& ambient = lighting->m_sunAmbient;

    CImVector color;
    color.a = 0xFF;
    color.b = static_cast<uint8_t>(static_cast<int32_t>(ambient.z > 0.0f ? (ambient.z < 1.0f ? ambient.z * 255.0f + 0.5f : 255.0f) : 0.0f));
    color.g = static_cast<uint8_t>(static_cast<int32_t>(ambient.y > 0.0f ? (ambient.y < 1.0f ? ambient.y * 255.0f + 0.5f : 255.0f) : 0.0f));
    color.r = static_cast<uint8_t>(static_cast<int32_t>(ambient.x > 0.0f ? (ambient.x < 1.0f ? ambient.x * 255.0f + 0.5f : 255.0f) : 0.0f));

    if (g_theGxDevicePtr->Caps().m_colorFormat == GxCF_rgba) {
        CImVector swapped;
        swapped.a = 0xFF;
        swapped.r = color.b;
        swapped.g = color.g;
        swapped.b = color.r;
        color = swapped;
    }

    C3Vector top = model->GetEventPosition('TTW$');
    C3Vector bottom = model->GetEventPosition('BTW$');
    C3Vector hand(0.0f, 0.0f, 0.0f);

    if (unit && (unit->m_animationState & 0x4000)) {
        if (unit->m_worldModel->HasAttachment(35)) {
            hand = unit->m_worldModel->GetAttachmentPosition(35);
        } else if (unit->m_worldModel->HasAttachment(1)) {
            hand = unit->m_worldModel->GetAttachmentPosition(1);
        }
    } else {
        hand.x = (bottom.x + top.x) * 0.5f;
        hand.y = (bottom.y + top.y) * 0.5f;
        hand.z = (top.z + bottom.z) * 0.5f;
    }

    vertices[0].p = top;
    vertices[0].c = color;
    vertices[1].p = bottom;
    vertices[1].c = color;
    vertices[2].p = hand;
    vertices[2].c = color;

    indices[0] = 0;
    indices[1] = 2;
    indices[2] = 2;
    indices[3] = 1;

    GxBufUnlock(indexBuf, 0);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    GxBufUnlock(vertexBuf, 0);
    GxPrimVertexPtr(vertexBuf, GxVBF_PC);

    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Texture0, 0);

    CGxBatch batch;
    batch.m_primType = GxPrim_Lines;
    batch.m_start = 0;
    batch.m_count = 4;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 2;
    g_theGxDevicePtr->Draw(&batch, 1);
}

// VERIFY: 2e010a3c manual
// OFFSET: 0x007EAD80
void CDrawBowString::GlueDrawBowStringCB(CM2Model* model, const CM2Lighting* lighting, void* arg) {
    CDrawBowString::DrawBowString(model, lighting, nullptr);
}

// VERIFY: 76d7eb0b auto
// OFFSET: 0x007EAD70
void CDrawBowString::UnitDrawBowStringCB(CM2Model* model, const CM2Lighting* lighting, void* arg) {
    CDrawBowString::DrawBowString(model, lighting, static_cast<CGUnit_C*>(arg));
}

void CDrawBowString::ModelLoadedCallback(CM2Model* model, void* arg) {
    if (model->HasEvent('TTW$') && model->HasEvent('BTW$')) {
        model->m_fFlags |= 0x20;
        model->m_drawCallback = &CDrawBowString::GlueDrawBowStringCB;
        model->m_drawArg = arg;
    }
}
