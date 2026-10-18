#include "Tools/AppleBootstrap/WmoLiquidPreview.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/LiquidGeomFactories.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Model2/M2Light.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>

namespace {
class Environment final : public Liquid::IEnvironment {
    bool m_interior;

  public:
    explicit Environment(bool interior) : m_interior(interior) {}
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
struct WmoLiquidPreview::Impl {
    DayNight::CurrentLight previousBands = DayNight::GetInfo()->m_bands;
    Liquid::CInstance *instance = nullptr;
    Liquid::CRenderer renderer;
    bool initialized = false, textures = false;
    ~Impl() {
        if (instance)
            instance->Release();
        if (initialized)
            Liquid::Shutdown();
        if (textures)
            Liquid::DestroyProceduralTextures();
        DayNight::GetInfo()->m_bands = previousBands;
    }
};
WmoLiquidPreview::WmoLiquidPreview(CMapObj &object, CMapObjGroup &group) : m(std::make_unique<Impl>()) {
    if (Liquid::s_settingsBankRefCount || Liquid::s_materialBankRefCount || Liquid::s_wmoWaterTex)
        throw std::runtime_error("WMO liquid preview requires an exclusive liquid context");
    g_liquidTypeDB.Load(__FILE__, __LINE__);
    g_liquidMaterialDB.Load(__FILE__, __LINE__);
    if (!g_liquidTypeDB.m_loaded || !g_liquidMaterialDB.m_loaded)
        throw std::runtime_error("Cannot load original liquid databases");
    auto type = uint32_t(group.GetLiquidType());
    auto *record = g_liquidTypeDB.GetRecord(type);
    if (!record)
        throw std::runtime_error("WMO liquid type missing from database");
    const bool exterior = (group.m_flags & 0x48) || (record->m_flags & 0x200);
    if (!exterior && type < 21 && type && !((type - 1) & 3)) {
        type = 17;
        record = g_liquidTypeDB.GetRecord(type);
    }
    auto *material = record ? g_liquidMaterialDB.GetRecord(record->m_materialID) : nullptr;
    if (!material)
        throw std::runtime_error("WMO liquid material missing from database");
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
    auto *settings = Liquid::CSettingsBank::GetSettings(type);
    auto *shader = Liquid::CMaterialBank::GetMaterial(type);
    if (!settings || !shader)
        throw std::runtime_error("Cannot create original WMO liquid material");
    auto factory = std::unique_ptr<Liquid::CMeshGeomFactory, void (*)(Liquid::CMeshGeomFactory *)>(
        Liquid::CMeshGeomFactory::Create(&object, &group), [](auto *p) { p->Release(); });
    factory->SetMagma(material->m_lvf == 1);
    factory->SetTexCoord1ValueX(exterior ? 0 : 1);
    CImVector color{255, 255, 255, 255};
    const auto materialIndex = uint16_t(group.m_liquidMaterialId);
    if (materialIndex >= object.m_materialsCount)
        throw std::runtime_error("Invalid WMO liquid material index");
    if (!exterior)
        color = object.m_materialList[materialIndex].m_diffColor;
    factory->SetDiffuseColor(color);
    auto environment = std::make_unique<Environment>(!exterior);
    m->instance = Liquid::CInstance::Create();
    m->instance->m_material = shader;
    m->instance->m_settings = settings;
    m->instance->m_geomFactory = factory.release();
    m->instance->m_environment = environment.release();
    m->instance->m_transform = C44Matrix(1);
    m->instance->m_bounds.c = (group.m_bbox.b + group.m_bbox.t) * 0.5f;
    m->instance->m_bounds.r = std::sqrt((group.m_bbox.t - m->instance->m_bounds.c).SquaredMag());
    std::fprintf(stderr, "Original WMO liquid: type %u, material %d, %dx%d authored tiles, %s lighting\n", type, record->m_materialID,
                 group.m_liquidTiles.x, group.m_liquidTiles.y, exterior ? "exterior" : "interior");
}
WmoLiquidPreview::~WmoLiquidPreview() = default;
bool WmoLiquidPreview::Ready(bool wait) {
    bool ready = true;
    for (auto &frames : m->instance->m_settings->m_textures)
        for (unsigned i = 0; i < frames.Count(); ++i) {
            auto *texture = frames[i];
            CStatus status;
            ready &= TextureGetGxTex(texture, wait, &status) != nullptr;
            if (!status.IsEmpty() || !texture->m_status.IsEmpty())
                throw std::runtime_error("WMO liquid texture failed to load");
        }
    return ready;
}
void WmoLiquidPreview::Draw(const C3Vector &camera) {
    Liquid::UpdateProceduralTextures();
    m->renderer.Draw(m->instance);
    m->renderer.Flush(camera, 0);
    m->renderer.Flush(camera, 1);
}
