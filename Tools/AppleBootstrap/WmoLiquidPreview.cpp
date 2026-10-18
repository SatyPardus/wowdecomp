#include "Tools/AppleBootstrap/WmoLiquidPreview.h"
#include "Tools/AppleBootstrap/LiquidPreviewContext.h"
#include "WoW/Source/WorldClient/MapObj.h"
#include "WoW/Source/WorldClient/LiquidGeomFactories.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include <cstdio>
#include <cmath>
#include <stdexcept>

struct WmoLiquidPreview::Impl {
    LiquidPreviewContext context;
    Liquid::CInstance *instance = nullptr;
    Liquid::CRenderer renderer;
    ~Impl() {
        if (instance)
            instance->Release();
    }
};
WmoLiquidPreview::WmoLiquidPreview(CMapObj &object, CMapObjGroup &group) : m(std::make_unique<Impl>()) {
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
    auto environment = std::unique_ptr<Liquid::IEnvironment>(LiquidPreviewContext::CreateEnvironment(!exterior));
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
bool WmoLiquidPreview::Ready(bool wait) { return LiquidPreviewContext::Ready(*m->instance, wait); }
void WmoLiquidPreview::Prepare() {
    m->renderer.Draw(m->instance);
    m->renderer.Prepare();
}
void WmoLiquidPreview::Draw(const C3Vector &camera) {
    Liquid::UpdateProceduralTextures();
    m->renderer.Draw(m->instance);
    m->renderer.Flush(camera, 0);
    m->renderer.Flush(camera, 1);
}
