#include "Liquid.h"
#include "Engine/Source/Tempest/CMath.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00D4505C
CDataAllocator Liquid::s_procWaterFFPAllocator(sizeof(Liquid::CMaterialProcWaterFFP), 8);

// STATIC: 0x00D45048
CDataAllocator Liquid::s_procWaterAllocator(sizeof(Liquid::CMaterialProcWater), 8);

// STATIC: 0x00D44C34
int32_t Liquid::CMaterialProcWater::s_refCount;

// STATIC: 0x00D44C24
CGxShader* Liquid::CMaterialProcWater::s_vertexShaders[4];

// STATIC: 0x00D44C20
CGxShader* Liquid::CMaterialProcWater::s_pixelShader;

// STATIC: 0x00D44C48
Liquid::PSProcWaterConstants Liquid::s_psProcWaterConstants;

// STATIC: 0x00D44F88
Liquid::VSWaveConstants Liquid::s_vsWaveConstants;

// VERIFY: 241af1fe manual
// OFFSET: 0x008A3620
void Liquid::SetDirectionalWave(uint32_t index, const C2Vector& direction, float a3, float a4, float a5, float speed) {
    Liquid::s_vsWaveConstants.m_directionalPhase[index] = static_cast<float>(static_cast<uint32_t>(OsGetAsyncTimeMs() * speed) & 0x1FFF) / 1024.0f * TWO_PI;

    float clampedA3 = a3 < 0.001f ? 0.001f : a3;
    C4Vector& wave = Liquid::s_vsWaveConstants.m_directionalWaves[index];
    wave.x = direction.x;
    wave.y = direction.y;
    wave.z = 1.0f / clampedA3;
    wave.w = a5;

    Liquid::s_vsWaveConstants.unk_0050[index] = 1.0f / (a4 < 0.001f ? 0.001f : a4);
}

// VERIFY: ffbfd269 manual
// OFFSET: 0x008A3710
void Liquid::SetCircularWave(uint32_t index, const C2Vector& a2, const C2Vector& a3, float a4, float a5, float a6, float speed) {
    Liquid::s_vsWaveConstants.m_circularPhase[index] = static_cast<float>(static_cast<uint32_t>(OsGetAsyncTimeMs() * speed) & 0x1FFF) / 1024.0f * TWO_PI;

    float invA4 = 1.0f / (a4 < 0.001f ? 0.001f : a4);
    Liquid::s_vsWaveConstants.unk_0060[index] = 1.0f / (a5 < 0.001f ? 0.001f : a5);

    C4Vector& wave = Liquid::s_vsWaveConstants.m_circularWaves[index];
    wave.x = a2.x;
    wave.y = a2.y;
    wave.z = a3.x;
    wave.w = a3.y;

    Liquid::s_vsWaveConstants.unk_00A0[index] = invA4;
    Liquid::s_vsWaveConstants.unk_00B0[index] = a6;
}

// VERIFY: e47f29d4 manual
// OFFSET: 0x008A3810
void Liquid::SetProcWaterParams(float a1, float a2, float a3, float a4, float a5, float a6, float a7) {
    float* params = Liquid::s_psProcWaterConstants.m_params;
    params[0] = 1.0f / (a1 < 0.001f ? 0.001f : a1);
    params[1] = a2;
    params[2] = a3;
    params[3] = a4;
    params[4] = a7;
    params[5] = a5;
    params[6] = a6;
    params[7] = 0.0f;
}

// VERIFY: e928fbf9 auto
// OFFSET: 0x008A3C90
void Liquid::SetProcWaterLighting(const CM2Lighting& lighting) {
    Liquid::s_psProcWaterConstants.m_sunDir.x = -lighting.m_sunDir.x;
    Liquid::s_psProcWaterConstants.m_sunDir.y = -lighting.m_sunDir.y;
    Liquid::s_psProcWaterConstants.m_sunDir.z = -lighting.m_sunDir.z;
    Liquid::s_psProcWaterConstants.m_sunDir.w = 0.0f;

    Liquid::s_psProcWaterConstants.m_sunAmbient.x = lighting.m_sunAmbient.x;
    Liquid::s_psProcWaterConstants.m_sunAmbient.y = lighting.m_sunAmbient.y;
    Liquid::s_psProcWaterConstants.m_sunAmbient.z = lighting.m_sunAmbient.z;
    Liquid::s_psProcWaterConstants.m_sunAmbient.w = 0.0f;

    Liquid::s_psProcWaterConstants.m_sunDiffuse.x = lighting.m_sunDiffuse.x;
    Liquid::s_psProcWaterConstants.m_sunDiffuse.y = lighting.m_sunDiffuse.y;
    Liquid::s_psProcWaterConstants.m_sunDiffuse.z = lighting.m_sunDiffuse.z;
    Liquid::s_psProcWaterConstants.m_sunDiffuse.w = 0.0f;

    Liquid::s_psProcWaterConstants.m_sunSpecular.x = lighting.m_sunSpecular.x;
    Liquid::s_psProcWaterConstants.m_sunSpecular.y = lighting.m_sunSpecular.y;
    Liquid::s_psProcWaterConstants.m_sunSpecular.z = lighting.m_sunSpecular.z;
    Liquid::s_psProcWaterConstants.m_sunSpecular.w = 50.0f;
}

// VERIFY: 4d2354fa auto
// OFFSET: 0x008A3DE0
void Liquid::CMaterialProcWater::Release() {
    this->~CMaterialProcWater();
    Liquid::s_procWaterAllocator.PutData(this, nullptr, 0);
}

// VERIFY: 624c4ea8 auto
// OFFSET: 0x008A3E00
Liquid::CMaterialProcWater::CMaterialProcWater() {
    if (Liquid::CMaterialProcWater::s_refCount++ == 0) {
        char name[256];
        SStrPrintf(name, sizeof(name), "vsLiquidProcWater%s", Liquid::GetShaderSuffix());
        g_theGxDevicePtr->ShaderCreate(Liquid::CMaterialProcWater::s_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", name, 4);
        SStrPrintf(name, sizeof(name), "psLiquidProcWater%s", Liquid::GetShaderSuffix());
        g_theGxDevicePtr->ShaderCreate(&Liquid::CMaterialProcWater::s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", name, 1);
    }
}

// VERIFY: 0b2449de manual
// OFFSET: [0x008A3EC0, 0x008A4740]
Liquid::CMaterialProcWater::~CMaterialProcWater() {
    if (Liquid::CMaterialProcWater::s_refCount-- == 1) {
        for (uint32_t i = 0; i < 4; i++) {
            if (Liquid::CMaterialProcWater::s_vertexShaders[i]) {
                g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialProcWater::s_vertexShaders[i]);
            }
        }

        if (Liquid::CMaterialProcWater::s_pixelShader) {
            g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialProcWater::s_pixelShader);
        }
    }
}

// VERIFY: c2386692 auto
// OFFSET: 0x008A3F30
void Liquid::CMaterialProcWaterFFP::Release() {
    this->~CMaterialProcWaterFFP();
    Liquid::s_procWaterFFPAllocator.PutData(this, nullptr, 0);
}

// VERIFY: 065234f4 auto
// OFFSET: 0x008A4710
Liquid::CMaterialProcWater* Liquid::CMaterialProcWater::Create() {
    void* mem = Liquid::s_procWaterAllocator.GetData(0, ".?AVCMaterialProcWater@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialProcWater() : nullptr;
}

// VERIFY: 3eec8265 auto
// OFFSET: 0x008A4770
Liquid::CMaterialProcWaterFFP* Liquid::CMaterialProcWaterFFP::Create() {
    void* mem = Liquid::s_procWaterFFPAllocator.GetData(0, ".?AVCMaterialProcWaterFFP@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialProcWaterFFP() : nullptr;
}

// VERIFY: be2b8036 manual
// OFFSET: 0x008A48F0
void Liquid::CMaterialProcWater::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture0 = settings->GetTexture(0, 1250);

    if (!texture0) {
        return;
    }

    CGxTex* texture1 = settings->GetTexture(1, 1250);

    if (!texture1) {
        return;
    }

    CGxTex* animTextures[2];

    for (uint32_t i = 0; i < 2; i++) {
        animTextures[i] = settings->GetTexture(i + 2, settings->GetInt(i + 1));

        if (!animTextures[i]) {
            return;
        }
    }

    CGxTex* texture4 = settings->GetTexture(4, 1250);

    if (!texture4) {
        return;
    }

    CGxTex* texture5 = settings->GetTexture(5, settings->GetInt(3));

    if (!texture5) {
        return;
    }

    float scales[4];
    float angles[4];

    for (uint32_t i = 0; i < 4; i++) {
        scales[i] = settings->GetFloat(i) * Liquid::WorldCoordScale();
        angles[i] = settings->GetFloat(i + 4) * 57.29578f;
    }

    float scale4 = settings->GetFloat(8) * Liquid::WorldCoordScale();
    float scale5 = settings->GetFloat(9) * Liquid::WorldCoordScale();
    float angle5 = settings->GetFloat(10) * 57.29578f;

    g_theGxDevicePtr->RsPush();

    CGxBuf* vertexBuf = nullptr;
    CGxBuf* indexBuf = nullptr;
    CGxBatch batch;
    batch.m_primType = GxPrims_Last;
    batch.m_start = 0;
    batch.m_count = 0;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 0;

    bool identity = geomFactory->SetTransform(const_cast<C44Matrix*>(transform));
    geomFactory->GetBuffers(GxVBF_PT2, &vertexBuf, &indexBuf, &batch);
    GxPrimVertexPtr(vertexBuf, GxVBF_PT2);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_DepthWrite, 0);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    g_theGxDevicePtr->RsSet(GxRs_Fog, 1);

    Liquid::s_vsConstants.m_projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        Liquid::s_vsConstants.m_projection.c0 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c1 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c2 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c3 *= -1.0f;
    }

    C44Matrix matrix;
    Liquid::SetShaderTransforms(cameraPos, identity ? matrix : *transform);

    CM2Lighting lighting(*bounds);
    environment->SelectLights(&lighting);
    Liquid::SetShaderLighting(lighting);
    Liquid::SetProcWaterLighting(lighting);

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);
    g_theGxDevicePtr->RsSet(GxRs_Texture1, texture1);
    g_theGxDevicePtr->RsSet(GxRs_Texture2, texture4);
    g_theGxDevicePtr->RsSet(GxRs_Texture3, texture5);

    for (uint32_t i = 0; i < 2; i++) {
        g_theGxDevicePtr->RsSet(static_cast<EGxRenderState>(GxRs_Texture4 + i), animTextures[i]);
    }

    for (uint32_t i = 0; i < 4; i++) {
        matrix = C44Matrix::RotationAroundZ(angles[i]);
        matrix.Scale(scales[i]);
        Liquid::s_vsConstants.m_texTransforms[i] = matrix;
    }

    C44Matrix scaleMatrix;
    scaleMatrix.b1 = scale4;
    Liquid::s_vsConstants.m_texTransforms[4] = scaleMatrix;

    C44Matrix rotationMatrix = C44Matrix::RotationAroundZ(angle5);
    rotationMatrix.Scale(scale5);
    Liquid::s_vsConstants.m_texTransforms[5] = rotationMatrix;

    const float* waves = nullptr;
    const float* wavesEnd;

    if (waveManager) {
        waves = static_cast<const float*>(waveManager->GetData());
        wavesEnd = waves + waveManager->GetCount();
    } else {
        wavesEnd = waves;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (waves + 6 > wavesEnd) {
            C2Vector direction;
            Liquid::SetDirectionalWave(i, direction, 0.0f, 0.0f, 0.0f, 0.0f);
        } else {
            C2Vector direction(waves[0], waves[1]);
            Liquid::SetDirectionalWave(i, direction, waves[2], waves[3], waves[4], waves[5]);
        }

        waves += 6;
    }

    for (uint32_t i = 0; i < 3; i++) {
        if (waves + 8 > wavesEnd) {
            C2Vector a2;
            C2Vector a3;
            Liquid::SetCircularWave(i, a2, a3, 0.0f, 0.0f, 0.0f, 0.0f);
        } else {
            C2Vector a3(waves[2], waves[3]);
            C2Vector a2(waves[0], waves[1]);
            Liquid::SetCircularWave(i, a2, a3, waves[4], waves[5], waves[6], waves[7]);
        }

        waves += 8;
    }

    float param0 = settings->GetFloat(11) * Liquid::WorldCoordScale();
    float param1 = settings->GetFloat(12);
    float param2 = settings->GetFloat(13);
    float param3 = settings->GetFloat(14);
    float param4 = settings->GetFloat(15);
    float param5 = settings->GetFloat(16);
    float param6 = settings->GetFloat(17);
    Liquid::SetProcWaterParams(param0, param1, param2, param3, param4, param5, param6);

    Liquid::SetShaderConstants();
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 46, reinterpret_cast<const float*>(&Liquid::s_vsWaveConstants), 12);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 6, reinterpret_cast<const float*>(&Liquid::s_psProcWaterConstants), 6);

    uint32_t lightCount = lighting.m_lightCount;

    if (lightCount >= 3) {
        lightCount = 3;
    }

    g_theGxDevicePtr->RsSet(GxRs_VertexShader, Liquid::CMaterialProcWater::s_vertexShaders[lightCount]);
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, Liquid::CMaterialProcWater::s_pixelShader);
    g_theGxDevicePtr->Draw(&batch, 1);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 3685e63c manual
// OFFSET: 0x008A5170
void Liquid::CMaterialProcWaterFFP::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture0 = settings->GetTexture(4, 1250);

    if (!texture0) {
        return;
    }

    CGxTex* texture1 = settings->GetTexture(5, settings->GetInt(2));

    if (!texture1) {
        return;
    }

    float scale0 = settings->GetFloat(8) * Liquid::WorldCoordScale();
    float scale1 = settings->GetFloat(9) * Liquid::WorldCoordScale();
    float angle1 = settings->GetFloat(10) * 57.29578f;

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthWrite, g_theGxDevicePtr->Caps().m_maxClipPlanes == 0);

    C44Matrix world = *transform;

    if (Liquid::UseCameraRelativeCoords()) {
        world.d0 -= cameraPos.x;
        world.d1 -= cameraPos.y;
        world.d2 -= cameraPos.z;
    }

    GxXformSet(GxXform_World, world);

    CM2Lighting lighting(*bounds);
    environment->SelectLights(&lighting);
    lighting.SetupGxLights(&cameraPos);
    lighting.SetupGxFog();

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);
    g_theGxDevicePtr->RsSet(GxRs_TextureCoord0, 1);

    C44Matrix texMatrix0;
    texMatrix0.b1 = scale0;
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex0, texMatrix0);

    g_theGxDevicePtr->RsSet(GxRs_Texture1, texture1);
    g_theGxDevicePtr->RsSet(GxRs_TextureCoord1, 0);
    g_theGxDevicePtr->RsSet(GxRs_TextureShader1, 1);
    g_theGxDevicePtr->RsSet(GxRs_ColorOp1, GxTexOp_Add);
    g_theGxDevicePtr->RsSet(GxRs_AlphaOp1, GxTexOp_PassThru);

    C44Matrix texMatrix1 = C44Matrix::RotationAroundZ(angle1);
    texMatrix1.Scale(scale1);
    g_theGxDevicePtr->XformPushAndSet(GxXform_Tex1, texMatrix1);

    CGxBuf* vertexBuf = nullptr;
    CGxBuf* indexBuf = nullptr;
    CGxBatch batch;
    batch.m_primType = GxPrims_Last;
    batch.m_start = 0;
    batch.m_count = 0;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 0;
    geomFactory->GetBuffers(GxVBF_PNCT2, &vertexBuf, &indexBuf, &batch);
    GxPrimVertexPtr(vertexBuf, GxVBF_PNCT2);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    g_theGxDevicePtr->Draw(&batch, 1);

    GxXformPop(GxXform_Tex1);
    GxXformPop(GxXform_Tex0);
    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}
