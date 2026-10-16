#include "Engine/Source/FFX/PassGauss4.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"

// VERIFY: f0206923 auto
// OFFSET: 0x008C1B40
PassGauss4::PassGauss4(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
    this->m_shaderIndex = 0;

    if (FFX::s_useRECT) {
        this->m_shaderIndex = 1;
    }

    g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", "FFXGauss4", sizeof(this->m_shaders) / sizeof(this->m_shaders[0]));
}

// VERIFY: 1148e3de manual
// OFFSET: [0x008C1BB0, 0x008C1BF0]
PassGauss4::~PassGauss4() {
    for (uint32_t i = 0; i < sizeof(this->m_shaders) / sizeof(this->m_shaders[0]); i++) {
        if (this->m_shaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(this->m_shaders[i]);
        }
    }
}

bool PassGauss4::Valid() {
    CGxShader* shader = this->m_shaders[this->m_shaderIndex];
    return (shader ? shader->Valid() : 0) != 0;
}

// VERIFY: e4ef82d7 manual
// OFFSET: 0x008C1C20
void PassGauss4::Execute() {
    C2Vector horizontal[4];
    horizontal[0].x = -2.5f;
    horizontal[0].y = 0.0f;
    horizontal[1].x = -0.5f;
    horizontal[1].y = 0.0f;
    horizontal[2].x = 0.5f;
    horizontal[2].y = 0.0f;
    horizontal[3].x = 2.5f;
    horizontal[3].y = 0.0f;

    C2Vector vertical[4];
    vertical[0].x = 0.0f;
    vertical[0].y = -2.5f;
    vertical[1].x = 0.0f;
    vertical[1].y = -0.5f;
    vertical[2].x = 0.0f;
    vertical[2].y = 0.5f;
    vertical[3].x = 0.0f;
    vertical[3].y = 2.5f;

    FFX::SetupGx(this->m_output);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, FFX::s_gridTex, this->m_flip);
    g_theGxDevicePtr->PrimVertexMask(0x3D1);
    FFX::SetupIB();
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shaders[this->m_shaderIndex]);

    CImVector white;
    white.b = 0xFF;
    white.g = 0xFF;
    white.r = 0xFF;
    white.a = 0xFF;
    FFX::SetupVB_PCT4(FFX::s_gridPos, white, FFX::s_gridTex, horizontal, this->m_inputs[0]->m_texelSize);

    GxRenderTargetSet(GxBuffers_Color, TextureGetGxTex(FFX::s_quart[1].m_texture, 1, nullptr), 0);
    GxXformSetViewport(0.0f, static_cast<float>(FFX::s_quart[1].m_size.x) / FFX::s_quart[1].m_texSize.x, 0.0f, static_cast<float>(FFX::s_quart[1].m_size.y) / FFX::s_quart[1].m_texSize.y, 0.0f, 1.0f);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture2, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture3, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupVB_PCT4(FFX::s_gridPos, white, FFX::s_gridTex, vertical, this->m_inputs[0]->m_texelSize);

    GxRenderTargetSet(GxBuffers_Color, TextureGetGxTex(this->m_output->m_texture, 1, nullptr), 0);
    GxXformSetViewport(0.0f, static_cast<float>(this->m_output->m_size.x) / this->m_output->m_texSize.x, 0.0f, static_cast<float>(this->m_output->m_size.y) / this->m_output->m_texSize.y, 0.0f, 1.0f);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(FFX::s_quart[1].m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(FFX::s_quart[1].m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture2, TextureGetGxTex(FFX::s_quart[1].m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture3, TextureGetGxTex(FFX::s_quart[1].m_texture, 1, nullptr));
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}
