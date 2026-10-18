#include "Tools/AppleBootstrap/TerrainLiquidPreview.h"
#include "Tools/AppleBootstrap/LiquidPreviewContext.h"
#include "WoW/Common/TerrainSurface.h"
#include "WoW/Source/LiquidSystem/Liquid.h"
#include "WoW/Source/WorldClient/CChunkLiquid.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/MapLight.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/LiquidGeomFactories.h"
#include "WoW/Source/WorldClient/LiquidWaveManager.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include <algorithm>
#include <map>
#include <tuple>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdexcept>

struct TerrainLiquidPreview::Impl {
    Liquid::CAreaData data{nullptr};
    std::unique_ptr<LiquidPreviewContext> context;
    std::vector<std::unique_ptr<CMapChunk>> owners;
    std::vector<std::unique_ptr<CChunkLiquid>> chunks;
    std::vector<Liquid::CInstance *> instances;
    Liquid::CRenderer renderer;
    CMapLight light;
    CMapLight *previousLight = CMap::s_mapLight;
    DNLightState previousDaylight = DayNight::GetInfo()->m_light1;
    DNFogInfo previousFog = DayNight::GetInfo()->m_fog;
    ~Impl() {
        for (auto *instance : instances)
            instance->Release();
        chunks.clear();
        owners.clear();
        if (context)
            CMap::FreeAllChunkLiquids();
        context.reset();
        CMap::s_mapLight = previousLight;
        DayNight::GetInfo()->m_light1 = previousDaylight;
        DayNight::GetInfo()->m_fog = previousFog;
    }
};
TerrainLiquidPreview::TerrainLiquidPreview(const std::vector<uint8_t> &adt, const Terrain::Surface &surface, uint32_t tileX, uint32_t tileY,
                                           float radius)
    : m(std::make_unique<Impl>()) {
    for (size_t at = 0; at + 8 <= adt.size();) {
        uint32_t token, size;
        std::memcpy(&token, adt.data() + at, 4);
        std::memcpy(&size, adt.data() + at + 4, 4);
        if (size > adt.size() - at - 8)
            throw std::runtime_error("Truncated terrain liquid container");
        if (token == 0x4D48324F && size) {
            if (m->data.HasData())
                throw std::runtime_error("Duplicate MH2O chunk");
            m->data.SetData(adt.data() + at + 8, size);
        }
        at += size_t(size) + 8;
    }
    if (!m->data.HasData())
        return;
    m->context = std::make_unique<LiquidPreviewContext>();
    // Same explicit atmosphere as TerrainPreview, supplied to original materials.
    auto *info = DayNight::GetInfo();
    info->m_light1.m_diffuse = CImVector{153, 166, 179, 255};
    info->m_light1.m_ambient = CImVector{89, 77, 77, 255};
    info->m_fog = {CImVector{46, 36, 31, 255}, 1.6f * radius, 5.6f * radius, 1};
    m->light.m_light.m_dir = C3Vector(-0.3f, -0.4f, -0.8660254f);
    m->light.m_light.m_specColor = C3Vector(0.35f, 0.35f, 0.3f);
    CMap::s_mapLight = &m->light;
    // Match CChunkLiquid::Batch's adjacent 2x2 chunks, grouped by liquid type.
    std::map<std::tuple<unsigned, unsigned, unsigned>, std::vector<CChunkLiquid *>> batches;
    for (const auto &source : surface.chunks) {
        const auto *descriptor = m->data.GetChunkDesc(source.x, source.y);
        if (!descriptor->m_layerCount)
            continue;
        auto owner = std::make_unique<CMapChunk>();
        owner->m_cOffset = C2iVector{int32_t(tileX * 16 + source.x), int32_t(tileY * 16 + source.y)};
        owner->m_aIndex = C2iVector{int32_t(source.x), int32_t(source.y)};
        std::array<CGxVertexPN, 145> vertices;
        const Terrain::GeometryView geometry{owner->m_cOffset,      source.origin.z, source.heights.data(),
                                             source.normals.data(), nullptr,         false};
        Terrain::CreateVerticesWorld(vertices.data(), geometry);
        owner->m_topLeftCoords = vertices[0].m_position;
        owner->m_topLeftCoords.z = source.origin.z;
        for (unsigned j = 0; j < descriptor->m_layerCount; ++j) {
            const auto *record = m->data.GetInstanceDesc(descriptor, j);
            const auto *type = g_liquidTypeDB.GetRecord(record->m_liquidType);
            if (!type || !g_liquidMaterialDB.GetRecord(type->m_materialID))
                throw std::runtime_error("Terrain liquid type/material is missing");
            auto chunk = std::make_unique<CChunkLiquid>();
            chunk->m_owner = owner.get();
            chunk->m_type = record->m_liquidType;
            chunk->m_vertexFormat = record->m_vertexFormat;
            chunk->m_topLeftCoords = owner->m_topLeftCoords;
            chunk->m_height = C2Vector(record->m_minHeight, record->m_maxHeight);
            chunk->m_tileBegin = C2iVector{record->m_yOffset, record->m_xOffset};
            chunk->m_tileEnd = C2iVector{record->m_yOffset + record->m_height, record->m_xOffset + record->m_width};
            chunk->m_vertexData = new Liquid::CInstanceVertexData(&m->data, record);
            chunk->m_tileBits.SetRawData(const_cast<char *>(reinterpret_cast<const char *>(m->data.GetExistsTable(record))),
                                         record->m_width * record->m_height, false);
            chunk->CreateVertXY();
            batches[{source.y / 2, source.x / 2, record->m_liquidType}].push_back(chunk.get());
            m->chunks.push_back(std::move(chunk));
        }
        m->owners.push_back(std::move(owner));
    }
    m->instances.reserve(batches.size());
    for (auto &entry : batches) {
        TSGrowableArray<CChunkLiquid *> list;
        C3Vector low(1e30f, 1e30f, 1e30f), high(-1e30f, -1e30f, -1e30f);
        for (auto *chunk : entry.second) {
            list.New(chunk);
            const auto count = (chunk->m_tileEnd.x - chunk->m_tileBegin.x + 1) * (chunk->m_tileEnd.y - chunk->m_tileBegin.y + 1);
            for (int i = 0; i < count; ++i) {
                const auto p = chunk->m_verts[i] + chunk->m_topLeftCoords;
                low.x = std::min(low.x, p.x);
                low.y = std::min(low.y, p.y);
                low.z = std::min(low.z, p.z);
                high.x = std::max(high.x, p.x);
                high.y = std::max(high.y, p.y);
                high.z = std::max(high.z, p.z);
            }
        }
        auto *settings = Liquid::CSettingsBank::GetSettings(list[0]->m_type);
        auto *material = Liquid::CMaterialBank::GetMaterial(list[0]->m_type);
        if (!settings || !material)
            throw std::runtime_error("Cannot create original terrain liquid material");
        auto factory = std::unique_ptr<Liquid::CChunkGeomFactory, void (*)(Liquid::CChunkGeomFactory *)>(
            Liquid::CChunkGeomFactory::Create(list), [](auto *p) { p->Release(); });
        auto environment = std::unique_ptr<Liquid::IEnvironment>(LiquidPreviewContext::CreateEnvironment(false));
        auto *instance = Liquid::CInstance::Create();
        instance->m_settings = settings;
        instance->m_material = material;
        instance->m_geomFactory = factory.release();
        instance->m_environment = environment.release();
        instance->m_transform.Translate(list[0]->m_topLeftCoords);
        instance->m_waveManager = CChunkLiquid::s_waveManager;
        instance->m_waveManager->AddRef();
        instance->m_bounds.c = (low + high) * 0.5f;
        instance->m_bounds.r = std::sqrt((high - instance->m_bounds.c).SquaredMag());
        m->instances.push_back(instance);
    }
    std::fprintf(stderr, "Original terrain MH2O: %zu layers, %zu adjacent-chunk liquid batches\n", m->chunks.size(), m->instances.size());
}
TerrainLiquidPreview::~TerrainLiquidPreview() = default;
bool TerrainLiquidPreview::Ready(bool wait) {
    bool ready = true;
    for (auto *instance : m->instances)
        ready &= LiquidPreviewContext::Ready(*instance, wait);
    return ready;
}
void TerrainLiquidPreview::Draw(const C3Vector &camera) {
    if (!m->context)
        return;
    Liquid::UpdateProceduralTextures();
    for (auto *instance : m->instances)
        m->renderer.Draw(instance);
    m->renderer.Flush(camera, 0);
    m->renderer.Flush(camera, 1);
}
size_t TerrainLiquidPreview::LayerCount() const { return m->chunks.size(); }
size_t TerrainLiquidPreview::BatchCount() const { return m->instances.size(); }
