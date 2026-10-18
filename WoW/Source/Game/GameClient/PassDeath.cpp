#include "WoW/Source/Game/GameClient/PassDeath.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"

// VERIFY: 874a5f41 manual
// OFFSET: 0x007E86A0
PassDeath::PassDeath(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
    this->m_useRect = 0;
    this->m_shaders[0] = nullptr;
    this->m_shaders[1] = nullptr;

    if (g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 8 || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 9 || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 10) {
        g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", FFX::s_useRECT ? "FFXDeath_nvts_rect" : "FFXDeath_nvts_2d", 1);
        return;
    }

    if (FFX::s_useRECT) {
        this->m_useRect = 1;
    }

    g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", "FFXDeath", 2);
}

// VERIFY: 5970ea32 manual
// OFFSET: [0x007E8770, 0x007E9920]
PassDeath::~PassDeath() {
    for (uint32_t i = 0; i < sizeof(this->m_shaders) / sizeof(this->m_shaders[0]); i++) {
        if (this->m_shaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(this->m_shaders[i]);
        }
    }
}

// VERIFY: e2aee2fe auto
// OFFSET: 0x007E8920
PassDeathNVRC::PassDeathNVRC(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
    g_theGxDevicePtr->ShaderCreate(&this->m_shader, GxSh_Pixel, "Shaders\\Pixel", "FFXDeath_nvrc", 1);
}

// VERIFY: b3213b19 manual
// OFFSET: 0x007E9950
PassDeathNVRC::~PassDeathNVRC() {
    if (this->m_shader) {
        g_theGxDevicePtr->ShaderDestroy(this->m_shader);
    }
}

// VERIFY: 7ab3fdf0 manual
// OFFSET: 0x007E8970
bool PassDeathNVRC::Valid() {
    return this->m_shader && this->m_shader->Valid();
}

// VERIFY: 45689fc4 manual
// OFFSET: 0x007E99A0
void PassDeathNVRC::Execute() {
    FFX::SetupGx(this->m_output);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, FFX::s_gridTex, this->m_flip);

    CImVector color;
    color.b = 0xA8;
    color.g = 0x93;
    color.r = 0x53;
    color.a = 0xFF;
    GxPrimVertexPtr(4, FFX::s_gridPos, sizeof(C3Vector), nullptr, 0, &color, 0, FFX::s_gridTex, sizeof(C2Vector), nullptr, 0);
    GxPrimIndexPtr(4, FFX::s_quadIndices);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shader);
    GxRsSet(GxRs_BlendingMode, this->m_blend);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}

// VERIFY: d096b4ea manual
// OFFSET: 0x008C2020
bool PassDeath::Valid() {
    CGxShader* shader = this->m_shaders[this->m_useRect];
    return (shader ? shader->Valid() : 0) != 0;
}

// VERIFY: 883cadff manual
// OFFSET: 0x007E87B0
void PassDeath::Execute() {
    C2Vector texCoords0[4];
    C2Vector texCoords1[4];

    FFX::SetupGx(this->m_output);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, texCoords0, this->m_flip);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[1]->m_size, this->m_inputs[1]->m_texSize, FFX::s_gridPos, texCoords1, this->m_flip);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(this->m_inputs[1]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shaders[this->m_useRect]);

    CImVector color;
    color.b = 0xA8;
    color.g = 0x93;
    color.r = 0x53;
    color.a = static_cast<uint8_t>(CMath::fint_(DayNight::GetInfo()->m_bands.m_glow * 255.0f));

    GxPrimVertexPtr(4, FFX::s_gridPos, sizeof(C3Vector), nullptr, 0, &color, 0, texCoords0, sizeof(C2Vector), texCoords1, sizeof(C2Vector));
    GxPrimIndexPtr(4, FFX::s_quadIndices);
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}
