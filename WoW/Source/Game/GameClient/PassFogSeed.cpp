#include "PassFogSeed.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CImVector.h"

// STATIC: 0x00AF4938
static uint32_t s_deviceRestored;

// VERIFY: beff8e6d auto
// OFFSET: 0x007E7FE0
void __fastcall PassFogSeed::DeviceRestoredCallback() {
    s_deviceRestored = 1;
}

// INLINED
PassFogSeed::PassFogSeed(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
    this->m_row = 0;
    this->m_color = 0;
    s_deviceRestored = 1;
    g_theGxDevicePtr->AddDeviceRestoredCallback(PassFogSeed::DeviceRestoredCallback);
}

// VERIFY: bd3db29a manual
// OFFSET: 0x007E9260
PassFogSeed::~PassFogSeed() {
    g_theGxDevicePtr->RemoveDeviceRestoredCallback(PassFogSeed::DeviceRestoredCallback);
}

// VERIFY: da3c8c98 auto
// OFFSET: 0x008A1420
bool PassFogSeed::Valid() {
    return true;
}

// VERIFY: 0d91b22f manual
// OFFSET: 0x007E92A0
void PassFogSeed::Execute() {
    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 20, 4);
    float* vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(buf));
    vertices[0] = 0.0f;
    vertices[1] = 3.0f;
    vertices[2] = 0.0f;
    vertices[5] = 0.0f;
    vertices[6] = 4.0f;
    vertices[7] = 0.0f;
    vertices[10] = static_cast<float>(this->m_output->m_size.x);
    vertices[11] = 4.0f;
    vertices[12] = 0.0f;
    vertices[15] = static_cast<float>(this->m_output->m_size.x);
    vertices[16] = 3.0f;
    vertices[17] = 0.0f;

    FFX::Tex* input = this->m_inputs[0];
    float invWidth = 1.0f / input->m_texSize.x;
    float right = (input->m_size.x + 0.5f) * invWidth;
    float invHeight = 1.0f / input->m_texSize.y;
    float row = ((input->m_size.y + 0.5f) * invHeight - 0.5f * invHeight) / input->m_size.y * this->m_row + 0.5f * invHeight;
    float left = 0.5f * invWidth;
    vertices[3] = left;
    vertices[4] = row;
    vertices[8] = left;
    vertices[9] = row;
    vertices[14] = row;
    vertices[13] = right;
    vertices[18] = right;
    vertices[19] = row;
    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;

    FFX::SetupGx(this->m_output);

    if (s_deviceRestored) {
        GxSceneClear(1, CImVector(0, 0, 0, 0));
        s_deviceRestored = 0;
    }

    GxPrimVertexPtr(buf, GxVBF_PT);
    g_theGxDevicePtr->PrimVertexMask(0x41);
    FFX::SetupIB();
    GxRsSet(GxRs_MatDiffuse, static_cast<int32_t>(this->m_color));
    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));

    for (int32_t state = GxRs_Texture1; state <= GxRs_Texture15; state++) {
        GxRsSet(static_cast<EGxRenderState>(state), 0);
    }

    GxRsSet(GxRs_PixelShader, 0);
    GxRsSet(GxRs_BlendingMode, this->m_blend);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, 12, 4);
    vertices = reinterpret_cast<float*>(g_theGxDevicePtr->BufLock(buf));
    vertices[0] = 0.0f;
    vertices[1] = 0.0f;
    vertices[2] = 0.0f;
    vertices[3] = 0.0f;
    vertices[4] = 3.0f;
    vertices[5] = 0.0f;
    vertices[6] = static_cast<float>(this->m_output->m_size.x);
    vertices[7] = 3.0f;
    vertices[8] = 0.0f;
    vertices[9] = static_cast<float>(this->m_output->m_size.x);
    vertices[10] = 0.0f;
    vertices[11] = 0.0f;
    g_theGxDevicePtr->BufUnlock(buf, 0);
    buf->m_hasData = 1;

    GxPrimVertexPtr(buf, GxVBF_P);
    g_theGxDevicePtr->PrimVertexMask(0x1);
    GxRsSet(GxRs_Texture0, 0);
    FFX::SetupIB();
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);
    FFX::SetupReset();

    if (this->m_row < static_cast<uint32_t>(this->m_inputs[0]->m_texSize.y)) {
        this->m_row++;
    } else {
        this->m_row = 0;
    }
}
