#include "Engine/Source/FFX/PassGlow.h"
#include "Engine/Source/Base/Handle.h"
#include "Engine/Source/FFX/PassGlowWave.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/CGxDevice/CGxShader.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/CMath.h"

// STATIC: 0x00AD88F0
static CGxVertexAttrib s_vertexAttribsPCT3[] = {
    { GxVA_Position, 4, 0, 40 },
    { GxVA_Color0, 0, 12, 40 },
    { GxVA_TexCoord0, 3, 16, 40 },
    { GxVA_TexCoord1, 3, 24, 40 },
    { GxVA_TexCoord2, 3, 32, 40 }
};

// STATIC: 0x00B24B20
static C2iVector s_wavePeriod = { 3174, 2805 };

// STATIC: 0x00B24B28
static float s_waveScaleX = 1.0f;

// STATIC: 0x00B24B2C
static float s_waveScaleY = 0.88f;

// STATIC: 0x00D45C68
static EGxTexFormat s_waveTexFormat;

// STATIC: 0x00D45C6C
static int32_t s_waveTexRefCount;

// STATIC: 0x00D45C70
FFX::Tex PassGlowWave::s_waveTex;

// STATIC: 0x00D45C8C
static void* s_waveTexData;

// VERIFY: 29c914f3 manual
// OFFSET: 0x008C2920
static void WaveTexCallback(EGxTexCommand command, uint32_t width, uint32_t height, uint32_t depth, uint32_t mipLevel, void* userArg, uint32_t& texelStrideInBytes, const void*& texels) {
    uint32_t bytesPerTexel = s_waveTexFormat == GxTex_Argb8888 ? 4 : 2;

    switch (command) {
        case GxTex_Lock: {
            s_waveTexData = SMemAlloc(bytesPerTexel * 128 * 128, __FILE__, __LINE__, 0);

            if (s_waveTexFormat == GxTex_Uv88) {
                uint16_t* row = static_cast<uint16_t*>(s_waveTexData);

                for (uint32_t y = 0; y < 128; y++) {
                    for (uint32_t x = 0; x < 128; x++) {
                        float u = CMath::sin_(x / 128.0f * 6.2831855f) * 128.0f;
                        u = u < -128.0f ? -128.0f : (u < 127.0f ? u : 127.0f);
                        int8_t du = static_cast<int8_t>(u);

                        float v = CMath::sin_(y / 128.0f * 6.2831855f) * 128.0f;
                        v = v < -128.0f ? -128.0f : (v < 127.0f ? v : 127.0f);
                        int8_t dv = static_cast<int8_t>(v);

                        row[x] = static_cast<uint8_t>(du) | (static_cast<uint8_t>(dv) << 8);
                    }

                    row += 128;
                }
            } else {
                CImVector* row = static_cast<CImVector*>(s_waveTexData);
                CImVector color;

                for (uint32_t y = 0; y < 128; y++) {
                    for (uint32_t x = 0; x < 128; x++) {
                        float u = (CMath::sin_(x / 128.0f * 6.2831855f) * 0.5f + 0.5f) * 255.0f;
                        u = u < 0.0f ? 0.0f : (u < 255.0f ? u : 255.0f);

                        float v = (CMath::sin_(y / 128.0f * 6.2831855f) * 0.5f + 0.5f) * 255.0f;
                        v = v < 0.0f ? 0.0f : (v < 255.0f ? v : 255.0f);

                        color.r = static_cast<uint8_t>(u);
                        color.g = static_cast<uint8_t>(v);
                        row[x] = color;
                    }

                    row += 128;
                }
            }

            break;
        }

        case GxTex_Latch:
            texels = s_waveTexData;
            texelStrideInBytes = bytesPerTexel * 128;
            break;

        case GxTex_Unlock:
            SMemFree(s_waveTexData, __FILE__, __LINE__, 0);
            s_waveTexData = nullptr;
            break;
    }
}

// INLINED
PassGlowBase::PassGlowBase(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : FFX::Pass(inputs, count, output, blend, unk) {
}

// VERIFY: 8a0b30cf auto
// OFFSET: 0x008C21E0
PassGlow::PassGlow(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : PassGlowBase(inputs, count, output, blend, unk) {
    this->m_shaderIndex = 0;

    if (FFX::s_useRECT) {
        this->m_shaderIndex = 1;
    }

    g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", "FFXGlow", sizeof(this->m_shaders) / sizeof(this->m_shaders[0]));
}

// VERIFY: 83e7a051 manual
// OFFSET: [0x008C2250, 0x008C2780]
PassGlow::~PassGlow() {
    for (uint32_t i = 0; i < sizeof(this->m_shaders) / sizeof(this->m_shaders[0]); i++) {
        if (this->m_shaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(this->m_shaders[i]);
        }
    }
}

// VERIFY: 2b9af23e manual
// OFFSET: 0x008C2290
bool PassGlow::Valid() {
    CGxShader* shader = this->m_shaders[this->m_shaderIndex];
    return (shader ? shader->Valid() : 0) != 0;
}

// VERIFY: b1fa5360 auto
// OFFSET: 0x008C27B0
void PassGlow::Execute() {
    C2Vector tex0[4];
    C2Vector tex1[4];

    FFX::SetupGx(this->m_output);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, tex0, this->m_flip);
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[1]->m_size, this->m_inputs[1]->m_texSize, FFX::s_gridPos, tex1, this->m_flip);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(this->m_inputs[1]->m_texture, 1, nullptr));
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shaders[this->m_shaderIndex]);

    GxPrimVertexPtr(4, FFX::s_gridPos, sizeof(C3Vector), nullptr, 0, &this->m_color, 0, tex0, sizeof(C2Vector), tex1, sizeof(C2Vector));
    GxPrimIndexPtr(4, FFX::s_quadIndices);
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}

// VERIFY: 752a0127 auto
// OFFSET: [0x008C22B0, 0x008C28F0]
PassGlowWave::~PassGlowWave() {
    for (uint32_t i = 0; i < sizeof(this->m_shaders) / sizeof(this->m_shaders[0]); i++) {
        if (this->m_shaders[i]) {
            g_theGxDevicePtr->ShaderDestroy(this->m_shaders[i]);
        }
    }

    if (--s_waveTexRefCount == 0) {
        if (PassGlowWave::s_waveTex.m_texture) {
            HandleClose(reinterpret_cast<HOBJECT>(PassGlowWave::s_waveTex.m_texture));
        }

        PassGlowWave::s_waveTex.m_texture = nullptr;
        PassGlowWave::s_waveTex.m_texSize.x = 0;
        PassGlowWave::s_waveTex.m_texSize.y = 0;
        PassGlowWave::s_waveTex.m_size.x = 0;
        PassGlowWave::s_waveTex.m_size.y = 0;
        PassGlowWave::s_waveTex.m_texelSize.x = 0.0f;
        PassGlowWave::s_waveTex.m_texelSize.y = 0.0f;
    }
}

bool PassGlowWave::Valid() {
    CGxShader* shader = this->m_shaders[this->m_shaderIndex];
    return (shader ? shader->Valid() : 0) != 0;
}

// VERIFY: 033fdb6c manual
// OFFSET: 0x008C2350
void PassGlowWave::Execute() {
    C2Vector tex1[4];
    C2Vector tex2[4];
    C2Vector tex0[4];

    FFX::SetupGx(this->m_output);
    FFX::SetupIB();
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[0]->m_size, this->m_inputs[0]->m_texSize, FFX::s_gridPos, tex0, this->m_flip);
    g_theGxDevicePtr->RsSet(GxRs_Texture0, TextureGetGxTex(this->m_inputs[0]->m_texture, 1, nullptr));

    C44Matrix waveMtx;
    uint32_t time = OsGetAsyncTimeMs();
    C3Vector move;
    move.x = static_cast<float>(time % s_wavePeriod.x) / s_wavePeriod.x;
    move.y = static_cast<float>(time % s_wavePeriod.y) / s_wavePeriod.y;
    move.z = 0.0f;
    waveMtx.Translate(move);

    C3Vector scale;
    scale.x = this->m_output->m_size.x * s_waveScaleX / 128.0f;
    scale.y = this->m_output->m_size.y * s_waveScaleY / 128.0f;
    scale.z = 1.0f;
    waveMtx.Scale(scale);
    waveMtx.RotateAroundZ(0.17453294f);

    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[1]->m_size, this->m_inputs[1]->m_texSize, FFX::s_gridPos, tex1, this->m_flip);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, TextureGetGxTex(this->m_inputs[1]->m_texture, 1, nullptr));
    FFX::SetupGeo_Quad(this->m_output->m_size, this->m_inputs[2]->m_size, this->m_inputs[2]->m_texSize, FFX::s_gridPos, tex2, this->m_flip);
    g_theGxDevicePtr->RsSet(GxRs_Texture2, TextureGetGxTex(this->m_inputs[2]->m_texture, 1, nullptr));

    CGxBuf* buf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(FFX::VertexPCT3), 4);
    FFX::VertexPCT3* vertex = reinterpret_cast<FFX::VertexPCT3*>(GxBufLock(buf));

    for (uint32_t i = 0; i < 4; i++) {
        vertex->p = FFX::s_gridPos[i];
        vertex->c = this->m_color;

        C3Vector wave;
        wave.x = tex0[i].x;
        wave.y = tex0[i].y;
        wave.z = 0.0f;
        vertex->tc[0] = C2Vector(wave * waveMtx);
        vertex->tc[1] = tex1[i];
        vertex->tc[2] = tex2[i];

        vertex++;
    }

    GxBufUnlock(buf, 0);
    g_theGxDevicePtr->PrimVertexFormat(buf, s_vertexAttribsPCT3, sizeof(s_vertexAttribsPCT3) / sizeof(s_vertexAttribsPCT3[0]));
    g_theGxDevicePtr->PrimVertexMask(0x1D1);
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, this->m_shaders[this->m_shaderIndex]);

    float scale0[4];
    float scale1[4];

    if (FFX::s_useRECT) {
        scale0[0] = 3.0f;
        scale0[1] = 0.0f;
        scale0[2] = 0.0f;
        scale0[3] = 3.0f;
        scale1[0] = 0.75f;
        scale1[1] = 0.0f;
        scale1[2] = 0.0f;
        scale1[3] = 0.75f;
    } else {
        scale0[0] = 3.0f * this->m_inputs[1]->m_texelSize.x;
        scale0[1] = 0.0f;
        scale0[2] = 0.0f;
        scale0[3] = 3.0f * this->m_inputs[1]->m_texelSize.y;
        scale1[0] = 0.75f * this->m_inputs[2]->m_texelSize.x;
        scale1[1] = 0.0f;
        scale1[2] = 0.0f;
        scale1[3] = 0.75f * this->m_inputs[2]->m_texelSize.y;
    }

    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 0, scale0, 1);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 1, scale1, 1);
    g_theGxDevicePtr->Draw(&FFX::s_quadBatch, 1);

    FFX::SetupReset();
}

// VERIFY: c26e8e83 auto
// OFFSET: 0x008C2CE0
PassGlowWave::PassGlowWave(FFX::Tex** inputs, uint32_t count, FFX::Tex* output, EGxBlend blend, bool unk) : PassGlowBase(inputs, count, output, blend, unk) {
    if (!s_waveTexRefCount) {
        PassGlowWave::s_waveTex.m_texture = nullptr;
        PassGlowWave::s_waveTex.m_texSize.x = 128;
        PassGlowWave::s_waveTex.m_texSize.y = 128;
        PassGlowWave::s_waveTex.m_size.x = 128;
        PassGlowWave::s_waveTex.m_size.y = 128;
        PassGlowWave::s_waveTex.m_texelSize.x = 1.0f / 128.0f;
        PassGlowWave::s_waveTex.m_texelSize.y = 1.0f / 128.0f;

        CGxTexFlags flags(GxTex_Linear, 1, 1, 0, 0, 0, 1, 0, 0, 0);

        s_waveTexFormat = GxTex_Uv88;

        if (g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 12
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 11
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 2
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 3
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 4
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 5
            || g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] == 6) {
            s_waveTexFormat = GxTex_Argb8888;
        }

        PassGlowWave::s_waveTex.m_texture = TextureCreate(128, 128, s_waveTexFormat, s_waveTexFormat, flags, nullptr, WaveTexCallback, "PassGlowWave", 0);
    }

    s_waveTexRefCount++;

    this->m_shaderIndex = 0;

    if (FFX::s_useRECT) {
        this->m_shaderIndex = 1;
    }

    if (!g_theGxDevicePtr->Caps().m_texFmt[GxTex_Uv88]) {
        this->m_shaderIndex += 2;
    }

    g_theGxDevicePtr->ShaderCreate(this->m_shaders, GxSh_Pixel, "Shaders\\Pixel", "FFXGlowWave", sizeof(this->m_shaders) / sizeof(this->m_shaders[0]));
}
