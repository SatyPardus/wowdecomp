#include "Engine/Source/FFX/PassBox4.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"

// VERIFY: 41d87533 auto
// OFFSET: 0x008C1F70
PassBox4::PassBox4(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
    this->m_shaderIndex = 0;

    if (FFX::s_useRECT) {
        this->m_shaderIndex = 1;
    }

    g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", "FFXBox4", sizeof(this->m_shaders) / sizeof(this->m_shaders[0]));
}

// VERIFY: 787961fc manual
// OFFSET: [0x008C1FE0, 0x008C21B0]
PassBox4::~PassBox4() {
    for (uint32_t i = 0; i < sizeof(this->m_shaders) / sizeof(this->m_shaders[0]); i++) {
        if (this->m_shaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(this->m_shaders[i]);
        }
    }
}

// VERIFY: bf921836 manual
// OFFSET: 0x008C2020
bool PassBox4::Valid() {
    CGxShader* shader = this->m_shaders[this->m_shaderIndex];
    return (shader ? shader->Valid() : 0) != 0;
}

// VERIFY: 3361e6e6 manual
// OFFSET: 0x008C2040
void PassBox4::Execute() {
    static C2Vector s_offsets[4] = {
        C2Vector(-1.5f, -1.5f),
        C2Vector(0.5f, -1.5f),
        C2Vector(0.5f, 0.5f),
        C2Vector(-1.5f, 0.5f)
    };

    FFX::SetupGx(this->m_output);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, FFX::s_gridTex, this->m_flip);

    CImVector white;
    white.b = 0xFF;
    white.g = 0xFF;
    white.r = 0xFF;
    white.a = 0xFF;
    FFX::SetupVB_PCT4(FFX::s_gridPos, white, FFX::s_gridTex, s_offsets, this->m_inputs[0]->m_texelSize);

    g_theGxDevicePtr->PrimVertexMask(0x3D1);
    FFX::SetupIB();

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture2, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture3, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shaders[this->m_shaderIndex]);

    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}
