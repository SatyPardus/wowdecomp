#include "Tools/AppleBootstrap/LiquidPreviewContext.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Model2/M2Light.h"
#include <stdexcept>

namespace {
class PreviewEnvironment final : public Liquid::IEnvironment {
    bool m_interior;

  public:
    explicit PreviewEnvironment(bool interior) : m_interior(interior) {}
    void Release() override { delete this; }
    void SetInteriorFog(int32_t interior) override { m_interior = interior != 0; }
    void SelectLights(CM2Lighting *lighting) override {
        const auto *info = DayNight::GetInfo();
        const auto &fog = m_interior ? info->m_fogInterior : info->m_fog;
        lighting->SetFog(C3Vector(fog.m_color), fog.m_start, fog.m_end, fog.m_density);
        if (m_interior) {
            lighting->AddDiffuse(C3Vector(1, 1, 1), C3Vector(0, 0, -1));
        } else {
            lighting->AddAmbient(C3Vector(info->m_light1.m_ambient));
            lighting->AddDiffuse(C3Vector(info->m_light1.m_diffuse), CMap::s_mapLight->m_light.m_dir);
            lighting->m_sunSpecular = CMap::s_mapLight->m_light.m_specColor;
        }
    }
};
} // namespace
struct LiquidPreviewContext::Impl {
    DayNight::CurrentLight previousBands = DayNight::GetInfo()->m_bands;
    bool initialized = false, textures = false;
    ~Impl() {
        if (initialized)
            Liquid::Shutdown();
        if (textures)
            Liquid::DestroyProceduralTextures();
        DayNight::GetInfo()->m_bands = previousBands;
    }
};
LiquidPreviewContext::LiquidPreviewContext() : m(std::make_unique<Impl>()) {
    if (Liquid::s_settingsBankRefCount || Liquid::s_materialBankRefCount || Liquid::s_wmoWaterTex)
        throw std::runtime_error("Liquid preview requires an exclusive liquid context");
    g_liquidTypeDB.Load(__FILE__, __LINE__);
    g_liquidMaterialDB.Load(__FILE__, __LINE__);
    if (!g_liquidTypeDB.m_loaded || !g_liquidMaterialDB.m_loaded)
        throw std::runtime_error("Cannot load original liquid databases");
    auto &bands = DayNight::GetInfo()->m_bands;
    bands.m_waterColors[0] = CImVector{80, 56, 30, 255};
    bands.m_waterColors[1] = CImVector{48, 24, 10, 255};
    bands.m_waterColors[2] = CImVector{72, 64, 40, 255};
    bands.m_waterColors[3] = CImVector{40, 32, 16, 255};
    bands.m_waterShallowAlpha = bands.m_oceanShallowAlpha = 0.25f;
    bands.m_waterDeepAlpha = bands.m_oceanDeepAlpha = 0.85f;
    Liquid::Initialize(0, 1, 1, "", &g_liquidTypeDB, &g_liquidMaterialDB);
    m->initialized = true;
    CMap::InitializeLiquidDepthTables();
    m->textures = true;
    Liquid::CreateProceduralTextures();
}
LiquidPreviewContext::~LiquidPreviewContext() = default;
Liquid::IEnvironment *LiquidPreviewContext::CreateEnvironment(bool interior) { return new PreviewEnvironment(interior); }
bool LiquidPreviewContext::Ready(Liquid::CInstance &instance, bool wait) {
    bool ready = true;
    for (auto &frames : instance.m_settings->m_textures)
        for (unsigned i = 0; i < frames.Count(); ++i) {
            auto *texture = frames[i];
            CStatus status;
            ready &= TextureGetGxTex(texture, wait, &status) != nullptr;
            if (!status.IsEmpty() || !texture->m_status.IsEmpty())
                throw std::runtime_error("Liquid texture failed to load");
        }
    return ready;
}
