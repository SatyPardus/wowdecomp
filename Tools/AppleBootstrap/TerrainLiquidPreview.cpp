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
                                           float radius, const std::string &mapName)
    : m(std::make_unique<Impl>()) {
    std::array<std::pair<size_t, size_t>, 256> chunks{};
    std::array<bool, 256> legacy{};
    bool hasLegacy = false;
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
        if (token == 0x4D434E4B) {
            if (size < 128)
                throw std::runtime_error("Truncated MCNK liquid header");
            const auto *header = adt.data() + at + 8;
            auto word = [&](size_t offset) {
                uint32_t value;
                std::memcpy(&value, header + offset, 4);
                return value;
            };
            const auto x = word(4), y = word(8), offset = word(0x60), length = word(0x64);
            if (x >= 16 || y >= 16)
                throw std::runtime_error("Invalid MCNK liquid coordinates");
            chunks[y*16+x] = {at, size_t(size)+8};
            if (length > 8) {
                if (offset > size_t(size) + 8 || length > size_t(size) + 8 - offset)
                    throw std::runtime_error("MCLQ extent exceeds MCNK");
                uint32_t tag;
                std::memcpy(&tag, adt.data() + at + offset, 4);
                if (tag != 0x4D434C51)
                    throw std::runtime_error("Missing MCLQ at MCNK liquid offset");
                legacy[y*16+x] = true;
                hasLegacy = true;
            }
        }
        at += size_t(size) + 8;
    }
    if (!m->data.HasData() && !hasLegacy)
        return;
    m->context = std::make_unique<LiquidPreviewContext>();
    int32_t mapId = -1;
    g_mapDB.Load(__FILE__, __LINE__);
    if (!g_mapDB.m_loaded)
        throw std::runtime_error("Cannot load map database for liquid selection");
    for (int32_t i = 0; i < g_mapDB.m_numRecords; ++i) {
        const auto *record = g_mapDB.GetRecordByIndex(i);
        if (!SStrCmpI(record->m_directory, mapName.c_str(), STORM_MAX_STR)) {
            mapId = record->m_id;
            break;
        }
    }
    if (mapId < 0)
        throw std::runtime_error("Terrain map is absent from original Map database");
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
    size_t legacyLayers = 0;
    for (const auto &source : surface.chunks) {
        const auto *descriptor = m->data.HasData() ? m->data.GetChunkDesc(source.x, source.y) : nullptr;
        const auto index = source.y*16+source.x;
        if ((!descriptor || !descriptor->m_layerCount) && !legacy[index])
            continue;
        auto owner = std::make_unique<CMapChunk>();
        owner->LoadChunkData(adt.data()+chunks[index].first, chunks[index].second, surface.bigAlpha, surface.alphaMapShift);
        owner->m_cOffset = C2iVector{int32_t(tileX * 16 + source.x), int32_t(tileY * 16 + source.y)};
        owner->m_aIndex = C2iVector{int32_t(source.x), int32_t(source.y)};
        std::array<CGxVertexPN, 145> vertices;
        const Terrain::GeometryView geometry{owner->m_cOffset,      source.origin.z, source.heights.data(),
                                             source.normals.data(), nullptr,         false};
        Terrain::CreateVerticesWorld(vertices.data(), geometry);
        owner->m_topLeftCoords = vertices[0].m_position;
        owner->m_topLeftCoords.z = source.origin.z;
        unsigned legacyIndex = 0;
        for (unsigned i = 0; i < 4 && owner->m_liquid; ++i)
            if (source.flags & (4 << i)) {
                const auto typeId = (mapId == 530 && i == 2) ? 15 : i + 1;
                const auto *type = g_liquidTypeDB.GetRecord(typeId);
                const auto *material = type ? g_liquidMaterialDB.GetRecord(type->m_materialID) : nullptr;
                if (!material)
                    throw std::runtime_error("Legacy liquid type/material is missing");
                auto chunk = std::make_unique<CChunkLiquid>();
                chunk->Initialize(*owner, typeId, material->m_lvf, owner->m_liquid[legacyIndex++]);
                ++legacyLayers;
                batches[{source.y / 2, source.x / 2, typeId}].push_back(chunk.get());
                m->chunks.push_back(std::move(chunk));
            }
        for (unsigned j = 0; descriptor && j < descriptor->m_layerCount; ++j) {
            const auto *record = m->data.GetInstanceDesc(descriptor, j);
            const auto *type = g_liquidTypeDB.GetRecord(record->m_liquidType);
            if (!type || !g_liquidMaterialDB.GetRecord(type->m_materialID))
                throw std::runtime_error("Terrain liquid type/material is missing");
            auto chunk = std::make_unique<CChunkLiquid>();
            chunk->Initialize(*owner, m->data, *record);
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
    std::fprintf(stderr, "Original terrain liquids: %zu MH2O / %zu MCLQ layers, %zu adjacent-chunk liquid batches\n",
                 m->chunks.size() - legacyLayers, legacyLayers, m->instances.size());
}
TerrainLiquidPreview::~TerrainLiquidPreview() = default;
void TerrainLiquidPreview::UseDayNightLighting() {
    const auto& info = *DayNight::GetInfo();
    m->light.m_light.SetDirection(info.m_light1.m_dir);
    m->light.m_light.m_specColor = C3Vector(info.m_bands.m_sunCloudColors[0]);
}
bool TerrainLiquidPreview::Ready(bool wait) {
    bool ready = true;
    for (auto *instance : m->instances)
        ready &= LiquidPreviewContext::Ready(*instance, wait);
    return ready;
}
void TerrainLiquidPreview::Prepare() {
    if (!m->context) return;
    for (auto* instance:m->instances) m->renderer.Draw(instance);
    m->renderer.Prepare();
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
