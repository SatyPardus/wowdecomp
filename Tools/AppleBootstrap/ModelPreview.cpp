#include "Tools/AppleBootstrap/ModelPreview.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include <cmath>
#include <stdexcept>

ModelPreview::ModelPreview(const char* name) : m_name(name) {
    try {
        if (!g_animationDataDB.m_loaded) g_animationDataDB.Load(__FILE__, __LINE__);
        m_blobLoaded = ModelBlobLoad("World\\model.blob");
        if (!m_blobLoaded) throw std::runtime_error("Cannot load model bounds index");
        CShaderEffect::InitShaderSystem(1, 0);
        M2Initialize(8, 16); m_initialized = true;
        m_scene = M2CreateScene();
        if (!m_scene) throw std::runtime_error("Cannot create preview model scene");
        m_model = m_scene->CreateModel(m_name.c_str(), 0);
        if (!m_model) throw std::runtime_error("Cannot load preview model: " + m_name);
        m_model->m_lightingCallback = [](CM2Model*, CM2Lighting* lighting, void*) {
            lighting->AddAmbient(C3Vector(0.3f, 0.3f, 0.35f));
            lighting->AddDiffuse(C3Vector(0.7f, 0.65f, 0.6f), C3Vector(-0.3f, -0.4f, -0.8f));
            lighting->SetFog(C3Vector(0.12f, 0.14f, 0.18f), 0.6f, 3.0f);
        };
    } catch (...) { Shutdown(); throw; }
}
ModelPreview::~ModelPreview() { Shutdown(); }
void ModelPreview::Shutdown() {
    if (m_scene) { m_scene->Release(); m_scene = nullptr; m_model = nullptr; }
    if (m_initialized) { M2Destroy(); CShaderEffectManager::DestroyAllEffects(); m_initialized = false; }
    if (m_blobLoaded) { ModelBlobDestroy(); m_blobLoaded = false; }
}
bool ModelPreview::Draw(float elapsedSeconds, uint32_t width, uint32_t height) {
    m_fractionalMilliseconds += elapsedSeconds * 1000.0;
    const auto delta = uint32_t(m_fractionalMilliseconds);
    m_fractionalMilliseconds -= delta;
    m_scene->AdvanceTime(delta);
    if (!m_model->IsLoaded(0, 0)) {
        if (!m_model->m_shared->m_asyncObject) throw std::runtime_error("Preview model load did not complete: " + m_name);
        return false;
    }
    if (!m_model->IsDrawable(0, 0)) {
        for (uint32_t i = 0; i < m_model->m_shared->m_data->m_textures.m_count; ++i)
            if (m_model->m_textures[i] && !m_model->m_textures[i]->m_status.IsEmpty())
                throw std::runtime_error("Preview model texture failed: " + m_name);
        return false;
    }
    const auto& bounds = m_model->m_shared->m_data->m_bounds;
    if (!(bounds.m_radius > 0) || !std::isfinite(bounds.m_radius)) throw std::runtime_error("Preview model has invalid bounds: " + m_name);
    const auto center = (bounds.m_extent.b + bounds.m_extent.t) * 0.5f;
    m_rotation = std::fmod(m_rotation + elapsedSeconds * 0.35f, 6.2831853f);
    C44Matrix world(1); world.RotateAroundZ(m_rotation); world.RotateAroundX(-0.65f);
    world.Scale(0.8f / bounds.m_radius); world.Translate(center * -1.0f);
    m_model->SetWorldTransform(world);
    C44Matrix identity(1), projection(1);
    const float aspect = (0.48f * width) / (0.72f * height);
    if (aspect > 1) projection.a0 /= aspect; else projection.b1 *= aspect;
    g_theGxDevicePtr->XformSetView(identity); g_theGxDevicePtr->XformSetProjection(projection);
    GxXformSetViewport(0.5f, 0.98f, 0.2f, 0.92f, 0, 1);
    m_model->m_fFlags |= 8;
    m_model->SetAnimating(1); m_scene->Animate(C3Vector(0, 0, 0));
    m_scene->Draw(M2PASS_0); m_scene->Draw(M2PASS_1); m_scene->Draw(M2PASS_2);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    return m_scene->m_elements.Count() != 0;
}
