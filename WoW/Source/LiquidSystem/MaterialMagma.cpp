#include "Liquid.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D450C0
CDataAllocator Liquid::s_magmaFFPAllocator(sizeof(Liquid::CMaterialMagmaFFP), 8);

// STATIC: 0x00D450AC
CDataAllocator Liquid::s_magmaAllocator(sizeof(Liquid::CMaterialMagma), 8);

// STATIC: 0x00D44C40
int32_t Liquid::CMaterialMagma::s_refCount;

// STATIC: 0x00D44BF4
CGxShader* Liquid::CMaterialMagma::s_vertexShader;

// STATIC: 0x00D44BF0
CGxShader* Liquid::CMaterialMagma::s_pixelShader;

// VERIFY: c1ce3d88 auto
// OFFSET: 0x008A4170
void Liquid::CMaterialMagma::Release() {
    this->~CMaterialMagma();
    Liquid::s_magmaAllocator.PutData(this, nullptr, 0);
}

// VERIFY: a8322b3f auto
// OFFSET: 0x008A4190
Liquid::CMaterialMagma::CMaterialMagma() {
    if (Liquid::CMaterialMagma::s_refCount++ == 0) {
        g_theGxDevicePtr->ShaderCreate(&Liquid::CMaterialMagma::s_vertexShader, GxSh_Vertex, "Shaders\\Vertex", "vsLiquidMagma", 1);
        g_theGxDevicePtr->ShaderCreate(&Liquid::CMaterialMagma::s_pixelShader, GxSh_Pixel, "Shaders\\Pixel", "psLiquidMagma", 1);
    }
}

// VERIFY: 90d48f71 auto
// OFFSET: [0x008A4200, 0x008A48A0]
Liquid::CMaterialMagma::~CMaterialMagma() {
    if (Liquid::CMaterialMagma::s_refCount-- == 1) {
        if (Liquid::CMaterialMagma::s_vertexShader) {
            g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialMagma::s_vertexShader);
        }

        if (Liquid::CMaterialMagma::s_pixelShader) {
            g_theGxDevicePtr->ShaderDestroy(Liquid::CMaterialMagma::s_pixelShader);
        }
    }
}

// VERIFY: 98a0bedc auto
// OFFSET: 0x008A4260
void Liquid::CMaterialMagmaFFP::Release() {
    this->~CMaterialMagmaFFP();
    Liquid::s_magmaFFPAllocator.PutData(this, nullptr, 0);
}

// VERIFY: 7e8b3bec auto
// OFFSET: 0x008A4870
Liquid::CMaterialMagma* Liquid::CMaterialMagma::Create() {
    void* mem = Liquid::s_magmaAllocator.GetData(0, ".?AVCMaterialMagma@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialMagma() : nullptr;
}

// VERIFY: db3efd66 auto
// OFFSET: 0x008A48D0
Liquid::CMaterialMagmaFFP* Liquid::CMaterialMagmaFFP::Create() {
    void* mem = Liquid::s_magmaFFPAllocator.GetData(0, ".?AVCMaterialMagmaFFP@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CMaterialMagmaFFP() : nullptr;
}

// VERIFY: b5c1ce43 manual
// OFFSET: 0x008A6090
void Liquid::CMaterialMagma::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture0 = settings->GetTexture(0, 1250);

    if (!texture0) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);
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

    C2Vector speed;
    speed.x = settings->GetFloat(0);
    speed.y = settings->GetFloat(1);
    C44Matrix texMatrix = *Liquid::GetTexScrollMtx(speed);
    texMatrix.Scale(Liquid::WorldCoordScale());
    Liquid::s_vsConstants.m_texTransforms[0] = texMatrix;

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);
    Liquid::SetShaderConstants();
    g_theGxDevicePtr->RsSet(GxRs_VertexShader, Liquid::CMaterialMagma::s_vertexShader);
    g_theGxDevicePtr->RsSet(GxRs_PixelShader, Liquid::CMaterialMagma::s_pixelShader);

    CGxBuf* vertexBuf = nullptr;
    CGxBuf* indexBuf = nullptr;
    CGxBatch batch;
    batch.m_primType = GxPrims_Last;
    batch.m_start = 0;
    batch.m_count = 0;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 0;
    geomFactory->GetBuffers(GxVBF_PCT, &vertexBuf, &indexBuf, &batch);
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    g_theGxDevicePtr->Draw(&batch, 1);
    g_theGxDevicePtr->RsPop();
}

// VERIFY: 35b378c5 manual
// OFFSET: 0x008A6350
void Liquid::CMaterialMagmaFFP::Render(Liquid::IEnvironment* environment, Liquid::IGeomFactory* geomFactory, Liquid::IWaveManager* waveManager, const C3Vector& cameraPos, const C44Matrix* transform, const CAaSphere* bounds, Liquid::CMaterialSettings* settings) {
    CGxTex* texture0 = settings->GetTexture(0, 1250);

    if (!texture0) {
        return;
    }

    g_theGxDevicePtr->RsPush();
    g_theGxDevicePtr->XformPush(GxXform_World);
    g_theGxDevicePtr->XformPush(GxXform_Tex0);
    g_theGxDevicePtr->RsSet(GxRs_Culling, 0);

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
    g_theGxDevicePtr->RsSet(GxRs_Lighting, 0);
    lighting.SetupGxFog();

    g_theGxDevicePtr->RsSet(GxRs_Texture0, texture0);

    C2Vector speed;
    speed.x = settings->GetFloat(0);
    speed.y = settings->GetFloat(1);
    GxXformSet(GxXform_Tex0, *Liquid::GetTexScrollMtx(speed));
    g_theGxDevicePtr->RsSet(GxRs_TextureShader0, 1);

    CGxBuf* vertexBuf = nullptr;
    CGxBuf* indexBuf = nullptr;
    CGxBatch batch;
    batch.m_primType = GxPrims_Last;
    batch.m_start = 0;
    batch.m_count = 0;
    batch.m_minIndex = 0;
    batch.m_maxIndex = 0;
    geomFactory->GetBuffers(GxVBF_PCT, &vertexBuf, &indexBuf, &batch);
    GxPrimVertexPtr(vertexBuf, GxVBF_PCT);
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);
    g_theGxDevicePtr->Draw(&batch, 1);

    GxXformPop(GxXform_Tex0);
    GxXformPop(GxXform_World);
    g_theGxDevicePtr->RsPop();
}

bool Liquid::CMaterialMagma::PrepareGeometry(IGeomFactory *geometry, C44Matrix *transform, CMaterialSettings *settings, CGxBuf **vertices,
                                             CGxBuf **indices) {
    if (!settings->GetTexture(0, 1250))
        return false;
    geometry->SetTransform(transform);
    CGxBatch batch{};
    geometry->GetBuffers(GxVBF_PCT, vertices, indices, &batch);
    return batch.m_count != 0;
}
