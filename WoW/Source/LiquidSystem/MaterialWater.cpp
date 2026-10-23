#include "Liquid.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/ShaderEffect/Types.h"

// STATIC: 0x00D45098
CDataAllocator Liquid::s_waterFFPAllocator(sizeof(Liquid::CMaterialWaterFFP), 8);

// STATIC: 0x00D45084
CDataAllocator Liquid::s_waterNoSpecAllocator(sizeof(Liquid::CMaterialWaterNoSpec), 8);

// STATIC: 0x00D45070
CDataAllocator Liquid::s_waterAllocator(sizeof(Liquid::CMaterialWater), 8);

// STATIC: 0x00D44C38
int32_t Liquid::CMaterialWater::s_refCount;

// STATIC: 0x00D44C10
CGxShader* Liquid::CMaterialWater::s_vertexShaders[4];

// STATIC: 0x00D44C0C
CGxShader* Liquid::CMaterialWater::s_pixelShader;

// STATIC: 0x00D44C3C
int32_t Liquid::CMaterialWaterNoSpec::s_refCount;

// STATIC: 0x00D44BFC
CGxShader* Liquid::CMaterialWaterNoSpec::s_vertexShaders[4];

// STATIC: 0x00D44BF8
CGxShader* Liquid::CMaterialWaterNoSpec::s_pixelShader;

// VERIFY: 65c05f6d auto
// OFFSET: 0x008A3F50
void Liquid::CMaterialWater::Release() {
    this->~CMaterialWater();
    Liquid::s_waterAllocator.PutData(this, nullptr, 0);
}

// VERIFY: 05103905 auto
// OFFSET: 0x008A3F70
Liquid::CMaterialWater::CMaterialWater() {
    if (Liquid::CMaterialWater::s_refCount++ == 0) {
        g_theGxDevicePtr->ShaderCreate(Liquid::CMaterialWater::s_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", "vsLiquidWater", 4);
        g_theGxDevicePtr->ShaderCreate(&Liquid::CMaterialWater::s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "psLiquidWater", 1);
    }
}

// VERIFY: 181045ab manual
// OFFSET: [0x008A3FE0, 0x008A47C0]
Liquid::CMaterialWater::~CMaterialWater() {
    if (Liquid::CMaterialWater::s_refCount-- == 1) {
        for (uint32_t i = 0; i < 4; i++) {
            if (Liquid::CMaterialWater::s_vertexShaders[i]) {
                g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialWater::s_vertexShaders[i]);
            }
        }

        if (Liquid::CMaterialWater::s_pixelShader) {
            g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialWater::s_pixelShader);
        }
    }
}

// VERIFY: 894fb61f auto
// OFFSET: 0x008A4050
void Liquid::CMaterialWaterNoSpec::Release() {
    this->~CMaterialWaterNoSpec();
    Liquid::s_waterNoSpecAllocator.PutData(this, nullptr, 0);
}

// VERIFY: 4a9ff4b4 auto
// OFFSET: 0x008A4070
Liquid::CMaterialWaterNoSpec::CMaterialWaterNoSpec() {
    if (Liquid::CMaterialWaterNoSpec::s_refCount++ == 0) {
        g_theGxDevicePtr->ShaderCreate(Liquid::CMaterialWaterNoSpec::s_vertexShaders, GxSh_Vertex, "Shaders\\Vertex", "vsLiquidWaterNoSpec", 4);
        g_theGxDevicePtr->ShaderCreate(&Liquid::CMaterialWaterNoSpec::s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "psLiquidWaterNoSpec", 1);
    }
}

// VERIFY: eafe186b manual
// OFFSET: [0x008A40E0, 0x008A4820]
Liquid::CMaterialWaterNoSpec::~CMaterialWaterNoSpec() {
    if (Liquid::CMaterialWaterNoSpec::s_refCount-- == 1) {
        for (uint32_t i = 0; i < 4; i++) {
            if (Liquid::CMaterialWaterNoSpec::s_vertexShaders[i]) {
                g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialWaterNoSpec::s_vertexShaders[i]);
            }
        }

        if (Liquid::CMaterialWaterNoSpec::s_pixelShader) {
            g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialWaterNoSpec::s_pixelShader);
        }
    }
}

// VERIFY: 6511eac6 auto
// OFFSET: 0x008A4150
void Liquid::CMaterialWaterFFP::Release() {
    this->~CMaterialWaterFFP();
    Liquid::s_waterFFPAllocator.PutData(this, nullptr, 0);
}

// VERIFY: eb81c4d7 auto
// OFFSET: 0x008A4790
Liquid::CMaterialWater* Liquid::CMaterialWater::Create() {
    void* mem = Liquid::s_waterAllocator.GetData(0, ".?AVCMaterialWater@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialWater() : nullptr;
}

// VERIFY: c1099507 auto
// OFFSET: 0x008A47F0
Liquid::CMaterialWaterNoSpec* Liquid::CMaterialWaterNoSpec::Create() {
    void* mem = Liquid::s_waterNoSpecAllocator.GetData(0, ".?AVCMaterialWaterNoSpec@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialWaterNoSpec() : nullptr;
}

// VERIFY: f4a31f53 auto
// OFFSET: 0x008A4850
Liquid::CMaterialWaterFFP* Liquid::CMaterialWaterFFP::Create() {
    void* mem = Liquid::s_waterFFPAllocator.GetData(0, ".?AVCMaterialWaterFFP@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialWaterFFP() : nullptr;
}

// VERIFY: a882be97 manual
// OFFSET: 0x008A5590
void Liquid::CMaterialWater::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture1 = settings->GetTexture(0, settings->GetInt(1));

    if (!texture1) {
        return;
    }

    CGxTex* texture0 = settings->GetTexture(1, 1250);

    if (!texture0) {
        return;
    }

    float scale1 = settings->GetFloat(0);
    float angle1 = settings->GetFloat(1) * 57.29578f;
    float scale0 = settings->GetFloat(2);

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthWrite, g_theGxDevicePtr->Caps().m_maxClipPlanes == 0);
    g_theGxDevicePtr->RsSet(GxRs_Fog, 1);

    Liquid::s_vsConstants.m_projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        Liquid::s_vsConstants.m_projection.c0 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c1 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c2 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c3 *= -1.0f;
    }

    C44Matrix matrix;
    Liquid::SetShaderTransforms(cameraPos, geomFactory->SetTransform(const_cast<C44Matrix*>(transform)) ? matrix : *transform);

    CM2Lighting lighting(*bounds);
    environment->SelectLights(&lighting);
    Liquid::SetShaderLighting(lighting);

    C44Matrix texMatrix1;
    texMatrix1.b1 = scale0;
    Liquid::s_vsConstants.m_texTransforms[1] = texMatrix1;

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);

    C44Matrix texMatrix0 = C44Matrix::RotationAroundZ(angle1);
    texMatrix0.Scale(scale1);
    Liquid::s_vsConstants.m_texTransforms[0] = texMatrix0;

    g_theGxDevicePtr->RsSet(GxRs_Texture1, texture1);
    Liquid::SetShaderConstants();

    uint32_t lightCount = lighting.m_lightCount;

    if (lightCount >= 3) {
        lightCount = 3;
    }

    g_theGxDevicePtr->RsSet(GxRs_VertexShader, Liquid::CMaterialWater::s_vertexShaders[lightCount]);
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, Liquid::CMaterialWater::s_pixelShader);

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
    g_theGxDevicePtr->RsPop();
}

// VERIFY: b899ce66 manual
// OFFSET: 0x008A5900
void Liquid::CMaterialWaterNoSpec::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture1 = settings->GetTexture(0, settings->GetInt(1));

    if (!texture1) {
        return;
    }

    CGxTex* texture0 = settings->GetTexture(1, 1250);

    if (!texture0) {
        return;
    }

    float scale1 = settings->GetFloat(0);
    float angle1 = settings->GetFloat(1) * 57.29578f;
    float scale0 = settings->GetFloat(2);

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
    g_theGxDevicePtr->RsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthWrite, g_theGxDevicePtr->Caps().m_maxClipPlanes == 0);
    g_theGxDevicePtr->RsSet(GxRs_Fog, 1);

    Liquid::s_vsConstants.m_projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        Liquid::s_vsConstants.m_projection.c0 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c1 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c2 *= -1.0f;
        Liquid::s_vsConstants.m_projection.c3 *= -1.0f;
    }

    C44Matrix matrix;
    Liquid::SetShaderTransforms(cameraPos, geomFactory->SetTransform(const_cast<C44Matrix*>(transform)) ? matrix : *transform);

    CM2Lighting lighting(*bounds);
    environment->SelectLights(&lighting);
    Liquid::SetShaderLighting(lighting);

    C44Matrix texMatrix1;
    texMatrix1.b1 = scale0;
    Liquid::s_vsConstants.m_texTransforms[1] = texMatrix1;

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);

    C44Matrix texMatrix0 = C44Matrix::RotationAroundZ(angle1);
    texMatrix0.Scale(scale1);
    Liquid::s_vsConstants.m_texTransforms[0] = texMatrix0;

    g_theGxDevicePtr->RsSet(GxRs_Texture1, texture1);
    Liquid::SetShaderConstants();

    uint32_t lightCount = lighting.m_lightCount;

    if (lightCount >= 3) {
        lightCount = 3;
    }

    g_theGxDevicePtr->RsSet(GxRs_VertexShader, Liquid::CMaterialWaterNoSpec::s_vertexShaders[lightCount]);
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, Liquid::CMaterialWaterNoSpec::s_pixelShader);

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
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 556f1caa manual
// OFFSET: 0x008A5C70
void Liquid::CMaterialWaterFFP::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture1 = settings->GetTexture(0, settings->GetInt(1));

    if (!texture1) {
        return;
    }

    CGxTex* texture0 = settings->GetTexture(1, 1250);

    if (!texture0) {
        return;
    }

    float scale1 = settings->GetFloat(0) * Liquid::WorldCoordScale();
    float angle1 = settings->GetFloat(1) * 57.29578f;
    float scale0 = settings->GetFloat(2) * Liquid::WorldCoordScale();

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
