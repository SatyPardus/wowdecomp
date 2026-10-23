#include "Liquid.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/Tempest/C33Matrix.h"

static const float WATER_SPEC_EXP = 6.0f;

// STATIC: 0x00D44CA8
Liquid::VSConstants Liquid::s_vsConstants;

// STATIC: 0x00B24120
Liquid::PSConstants Liquid::s_psConstants;

// VERIFY: ea58aa9f manual
// OFFSET: 0x008A32F0
void Liquid::SetShaderTransforms(const C3Vector& cameraPos, const C44Matrix& transform) {
    C44Matrix view;
    GxXformView(view);

    C44Matrix world = transform;

    if (Liquid::UseCameraRelativeCoords()) {
        world.d0 -= cameraPos.x;
        world.d1 -= cameraPos.y;
        world.d2 -= cameraPos.z;
    }

    C44Matrix modelView = world * view;
    Liquid::s_vsConstants.m_modelView = modelView;

    C44Matrix projection;
    projection = g_theGxDevicePtr->m_projNative;

    if (g_theGxDevicePtr->DeviceApi() == GxApi_OpenGl) {
        projection.c0 *= -1.0f;
        projection.c1 *= -1.0f;
        projection.c2 *= -1.0f;
        projection.c3 *= -1.0f;
    }

    Liquid::s_psConstants.m_modelViewProj = modelView * projection;
    Liquid::s_psConstants.m_cameraPos.x = cameraPos.x;
    Liquid::s_psConstants.m_cameraPos.y = cameraPos.y;
    Liquid::s_psConstants.m_cameraPos.z = cameraPos.z;
    Liquid::s_psConstants.m_cameraPos.w = 1.0f;
}

// VERIFY: ab5711b4 manual
// OFFSET: 0x008A34B0
C44Matrix* Liquid::GetTexScrollMtx(const C2Vector& speed) {
    float u = 0.0f;
    float v = 0.0f;
    uint32_t time = OsGetAsyncTimeMs();

    if (speed.x != 0.0f) {
        uint32_t period = static_cast<uint32_t>(1000.0f / speed.x);
        u = static_cast<float>(time % period) / static_cast<float>(period);
    }

    if (speed.y != 0.0f) {
        uint32_t period = static_cast<uint32_t>(1000.0f / speed.y);
        v = static_cast<float>(time % period) / static_cast<float>(period);
    }

    // STATIC: 0x00D450D8
    static C44Matrix s_matrix;

    s_matrix.d0 = u;
    s_matrix.d1 = v;
    s_matrix.d2 = 0.0f;

    return &s_matrix;
}

// VERIFY: 0ef940ce manual
// OFFSET: 0x008A38B0
void Liquid::SetShaderLighting(const CM2Lighting& lighting) {
    C44Matrix view;
    GxXformView(view);

    C3Vector sunDir = lighting.m_sunDir * C33Matrix(view);
    Liquid::s_vsConstants.m_sunDir.x = sunDir.x;
    Liquid::s_vsConstants.m_sunDir.y = sunDir.y;
    Liquid::s_vsConstants.m_sunDir.z = sunDir.z;
    Liquid::s_vsConstants.m_sunDir.w = 1.0f;

    Liquid::s_vsConstants.m_sunAmbient.x = lighting.m_sunAmbient.x;
    Liquid::s_vsConstants.m_sunAmbient.y = lighting.m_sunAmbient.y;
    Liquid::s_vsConstants.m_sunAmbient.z = lighting.m_sunAmbient.z;
    Liquid::s_vsConstants.m_sunAmbient.w = 1.0f;

    Liquid::s_vsConstants.m_sunDiffuse.x = lighting.m_sunDiffuse.x;
    Liquid::s_vsConstants.m_sunDiffuse.y = lighting.m_sunDiffuse.y;
    Liquid::s_vsConstants.m_sunDiffuse.z = lighting.m_sunDiffuse.z;
    Liquid::s_vsConstants.m_sunDiffuse.w = 1.0f;

    Liquid::s_vsConstants.m_sunSpecular.x = lighting.m_sunSpecular.x;
    Liquid::s_vsConstants.m_sunSpecular.y = lighting.m_sunSpecular.y;
    Liquid::s_vsConstants.m_sunSpecular.z = lighting.m_sunSpecular.z;
    Liquid::s_vsConstants.m_sunSpecular.w = WATER_SPEC_EXP;

    uint32_t lightCount = lighting.m_lightCount;

    for (uint32_t i = 0; i < lightCount; i++) {
        if (i >= sizeof(Liquid::s_vsConstants.m_lights) / sizeof(Liquid::s_vsConstants.m_lights[0])) {
            break;
        }

        const CM2Light* light = lighting.m_lights[i];
        C3Vector position = light->m_pos * C33Matrix(view);
        Liquid::VSLight& constants = Liquid::s_vsConstants.m_lights[i];

        constants.m_position.x = position.x;
        constants.m_position.y = position.y;
        constants.m_position.z = position.z;
        constants.m_position.w = 1.0f;

        constants.m_color.x = light->m_dirColor.x * (1.0f / 255.0f);
        constants.m_color.y = light->m_dirColor.y * (1.0f / 255.0f);
        constants.m_color.z = light->m_dirColor.z * (1.0f / 255.0f);
        constants.m_color.w = 1.0f;

        constants.m_attenuation.x = light->m_constantAttenuation;
        constants.m_attenuation.y = light->m_linearAttenuation;
        constants.m_attenuation.z = light->m_quadraticAttenuation;
    }

    if (g_theGxDevicePtr->MasterEnable(GxMasterEnable_Fog)) {
        float fogEnd = lighting.m_fogEnd;
        float fogRange = 1.0f / (fogEnd - lighting.m_fogStart);
        Liquid::s_vsConstants.m_fog.x = -(fogRange * CShaderEffect::s_fogMul);
        Liquid::s_vsConstants.m_fog.y = fogEnd * fogRange;
        Liquid::s_vsConstants.m_fog.z = lighting.m_fogDensity;
        Liquid::s_vsConstants.m_fog.w = 0.0f;
    } else {
        Liquid::s_vsConstants.m_fog.x = 0.0f;
        Liquid::s_vsConstants.m_fog.y = 1.0f;
        Liquid::s_vsConstants.m_fog.z = 1.0f;
        Liquid::s_vsConstants.m_fog.w = 0.0f;
    }

    Liquid::s_psConstants.m_fogColor.x = lighting.m_fogColor.x;
    Liquid::s_psConstants.m_fogColor.y = lighting.m_fogColor.y;
    Liquid::s_psConstants.m_fogColor.z = lighting.m_fogColor.z;
    Liquid::s_psConstants.m_fogColor.w = 1.0f;
}

// VERIFY: e12a4641 auto
// OFFSET: 0x008A3DA0
void Liquid::SetShaderConstants() {
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Vertex, 0, reinterpret_cast<const float*>(&Liquid::s_vsConstants), 46);
    g_theGxDevicePtr->ShaderConstantsSet(GxSh_Pixel, 0, reinterpret_cast<const float*>(&Liquid::s_psConstants), 6);
}
