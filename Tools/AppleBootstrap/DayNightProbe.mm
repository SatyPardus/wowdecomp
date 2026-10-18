#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/Map.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gxu/Gxu.h"
#include "Engine/Source/Services/AsyncFile.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {
void Require(bool value, const char* message) {
    if (!value)
        throw std::runtime_error(message);
}
std::vector<uint8_t> ReadTable(const char* name, uint32_t rowSize) {
    SFileImpl* file = nullptr;
    Require(SFile::OpenEx(nullptr, name, 0, &file), "Cannot open original lighting table");
    struct Close {
        SFileImpl* file;
        ~Close() { SFileCloseFile(file); }
    } close{file};
    uint32_t high = 0, size = SFileGetFileSize(file, &high);
    Require(!high && size >= 20 && size <= 16 * 1024 * 1024, "Unexpected lighting table size");
    std::vector<uint8_t> bytes(size);
    Require(SFileReadFile(file, bytes.data(), size, nullptr, nullptr, 0), "Cannot read lighting reference bytes");
    uint32_t header[5];
    std::memcpy(header, bytes.data(), sizeof(header));
    Require(header[0] == 0x43424457 && header[3] == rowSize && 20 + uint64_t(header[1]) * rowSize + header[4] == size,
            "Invalid lighting reference table");
    return bytes;
}
template <class T, class DB> void CheckRows(const DB& db, const std::vector<uint8_t>& bytes) {
    static_assert(sizeof(T) == T::s_rowSize);
    uint32_t count;
    std::memcpy(&count, bytes.data() + 4, 4);
    Require(db.m_loaded && db.m_numRecords == int32_t(count), "Lighting row count changed");
    for (uint32_t i = 0; i < count; ++i) {
        T record;
        Require(db.GetRecordByIndex(int32_t(i), &record) && !std::memcmp(&record, bytes.data() + 20 + size_t(i) * sizeof(T), sizeof(T)),
                "Lighting record differs from authored DBC bytes");
    }
}
template <class T> void CheckBandStorage() {
    const auto bytes = ReadTable(T::GetFilename(), T::s_rowSize);
    const auto previous = g_clientDbPerformCompress;
    struct Restore {
        int value;
        ~Restore() { g_clientDbPerformCompress = value; }
    } restore{previous};
    for (int compressed : {0, 1}) {
        g_clientDbPerformCompress = compressed;
        WowClientDB_RLE<T> database;
        database.Load(__FILE__, __LINE__);
        CheckRows<T>(database, bytes);
        for (int32_t i = 0; i < database.m_numRecords; ++i) {
            T byIndex, byId;
            Require(database.GetRecordByIndex(i, &byIndex) && database.GetRecord(byIndex.m_id, &byId) &&
                        !std::memcmp(&byIndex, &byId, sizeof(T)),
                    "Lighting compressed ID lookup changed");
            Require(byIndex.m_num >= 0 && byIndex.m_num <= 16, "Lighting band count is invalid");
        }
    }
}

void CheckSequenceClock(CM2Model& model) {
    Require(model.m_shared->m_data->m_bones.m_count, "Sky fixture has no animation clock");
    auto& sequence = model.m_bones[0].m_sequence;
    struct Restore {
        CM2Model& model;
        M2ModelBoneSeq sequence;
        uint32_t time;
        ~Restore() {
            model.m_bones[0].m_sequence = sequence;
            model.m_scene->m_time = time;
        }
    } restore{model, sequence, model.m_scene->m_time};
    struct Clock {
        uint32_t now, start, offset;
        float speed;
        uint32_t expected;
    };
    for (const auto c : {Clock{100, 101, 50, 1, 49}, Clock{1, 0xFFFFFFFFu, 10, 0.5f, 11}, Clock{0xFFFFFFFFu, 1, 10, 0.5f, 9},
                         Clock{31, 11, 50, -1.5f, 20}, Clock{16777217, 0, 0, 1, 16777217}, Clock{0x80000000u, 0, 0, 1, 0x80000000u}}) {
        model.m_scene->m_time = c.now;
        sequence.m_startTime = c.start;
        sequence.m_startOffset = c.offset;
        sequence.m_speed = c.speed;
        M2BoneSequenceInfo info{};
        Require(model.GetBoneSequenceInfo(0xFFFFFFFFu, info) && info.m_time == c.expected,
                "Original signed animation clock, fractional truncation or wrap changed");
    }
}

void CheckSky(DayNight::DNOverrideSky& sky, bool expectAnimation) {
    Require(sky.m_model->IsDrawable(1, 0), "Authored sky model did not become drawable");
    CheckSequenceClock(*sky.m_model);
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget;
    descriptor.storageMode = MTLStorageModePrivate;
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    pass.depthAttachment.texture = [device.GPU().device newTextureWithDescriptor:descriptor];
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.clearDepth = 1;
    device.RsPush();
    GxXformPush(GxXform_View);
    const auto previousProjection = device.m_projNative;
    struct Restore {
        CGxDeviceMetal& device;
        C44Matrix projection;
        ~Restore() {
            if (device.FrameCommand())
                device.ScenePresent();
            device.m_projNative = projection;
            GxXformPop(GxXform_View);
            device.RsPop();
        }
    } restore{device, previousProjection};
    C44Matrix projection;
    GxuXformCreateProjection_Exact(1.2f, 1, 0.1f, 4000, projection);
    device.XformSetProjection(projection);
    device.XformSetView(C44Matrix::RotationAroundX(-1.5707963f));
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_Culling, 0);
    GxRsSet(GxRs_DepthTest, 1);
    GxRsSet(GxRs_DepthWrite, 1);
    GxRsSet(GxRs_DepthFunc, 0);
    GxRsSet(GxRs_BlendingMode, GxBlend_Opaque);
    GxRsSet(GxRs_AlphaRef, 0);
    GxRsSet(GxRs_ColorWrite, 15);
    GxRsSet(GxRs_ScissorTest, 0);
    GxRsSet(GxRs_ClipPlaneMask, 0);
    GxRsSet(GxRs_PointSprite, 0);
    TSGrowableArray<CImVector> previous;
    unsigned frame = 0;
    for (int minutes : {360, 720, 720}) {
        if (frame == 1)
            sky.m_model->m_scene->AdvanceTime(1000);
        DayNight::SyncSkyAnimWithTime(&sky, minutes);
        device.BeginFrame(pass);
        const auto before = device.EncodingStats();
        DayNight::DrawSky(&sky, 1);
        const auto after = device.EncodingStats();
        TSGrowableArray<CImVector> pixels;
        CiRect rectangle(0, 0, 128, 128);
        device.DeviceReadPixels(rectangle, pixels);
        unsigned covered = 0, changed = 0;
        for (unsigned i = 0; i < pixels.Count(); ++i) {
            covered += pixels[i].r || pixels[i].g || pixels[i].b || pixels[i].a;
            if (frame)
                changed += std::memcmp(&pixels[i], &previous[i], sizeof(CImVector)) != 0;
        }
        Require(after.geometryDraws > before.geometryDraws && covered > 1024, "Original sky drawing produced no visible geometry");
        Require(frame != 1 || !expectAnimation || changed > 256, "Authored sky animation did not change visible pixels");
        Require(frame != 2 || !changed, "Stationary sky changed without advancing time");
        std::fprintf(stderr, "DayNight sky %s at %d minutes: %llu draws / %llu passes, %u covered / %u changed pixels\n",
                     sky.m_model->m_shared->m_fileName, minutes,
                     static_cast<unsigned long long>(after.geometryDraws - before.geometryDraws),
                     static_cast<unsigned long long>(after.renderPasses - before.renderPasses), covered, changed);
        previous.SetCount(pixels.Count());
        std::memcpy(previous.m_data, pixels.m_data, pixels.Count() * sizeof(CImVector));
        device.ScenePresent();
        ++frame;
    }
}
} // namespace

void CheckDayNightData() {
    CheckBandStorage<LightIntBandRec>();
    CheckBandStorage<LightFloatBandRec>();
    if (!g_lightIntBandDB.m_loaded)
        g_lightIntBandDB.Load(__FILE__, __LINE__);
    if (!g_lightFloatBandDB.m_loaded)
        g_lightFloatBandDB.Load(__FILE__, __LINE__);
    if (!g_lightParamsDB.m_loaded)
        g_lightParamsDB.Load(__FILE__, __LINE__);
    if (!g_lightSkyboxDB.m_loaded)
        g_lightSkyboxDB.Load(__FILE__, __LINE__);
    CheckRows<LightParamsRec>(g_lightParamsDB, ReadTable(LightParamsRec::GetFilename(), LightParamsRec::s_rowSize));
    const auto skyboxBytes = ReadTable(LightSkyboxRec::GetFilename(), LightSkyboxRec::s_rowSize);
    const size_t strings = 20 + size_t(g_lightSkyboxDB.m_numRecords) * LightSkyboxRec::s_rowSize;
    for (int32_t i = 0; i < g_lightSkyboxDB.m_numRecords; ++i) {
        uint32_t row[3];
        std::memcpy(row, skyboxBytes.data() + 20 + size_t(i) * sizeof(row), sizeof(row));
        Require(strings + row[1] < skyboxBytes.size() &&
                    std::memchr(skyboxBytes.data() + strings + row[1], 0, skyboxBytes.size() - strings - row[1]),
                "Skybox reference string exceeds table");
        const auto* sky = g_lightSkyboxDB.GetRecordByIndex(i);
        Require(sky && sky->m_id == int32_t(row[0]) && sky->m_flags == int32_t(row[2]) &&
                    !std::strcmp(sky->m_name, reinterpret_cast<const char*>(skyboxBytes.data() + strings + row[1])),
                "Skybox ID, flags or resource name changed on arm64");
    }
    if (!g_lightDB.m_loaded) {
        g_lightDB.Load(__FILE__, __LINE__);
        CheckRows<LightRec>(g_lightDB, ReadTable(LightRec::GetFilename(), LightRec::s_rowSize));
        // ClientDBInitialize applies this once to all records before map load.
        Require(!CMap::bDungeon, "Lighting startup unexpectedly has an active dungeon");
        InitLights(true);
    }
    const auto lights = ReadTable(LightRec::GetFilename(), LightRec::s_rowSize);
    for (int32_t i = 0; i < g_lightDB.m_numRecords; ++i) {
        LightRec reference;
        std::memcpy(&reference, lights.data() + 20 + size_t(i) * sizeof(reference), sizeof(reference));
        if (reference.m_gameCoords[0] || reference.m_gameCoords[1] || reference.m_gameCoords[2]) {
            const float x = reference.m_gameCoords[0] / 36, y = reference.m_gameCoords[1] / 36, z = reference.m_gameCoords[2] / 36;
            reference.m_gameCoords[0] = 51200.0f / 3 - z;
            reference.m_gameCoords[1] = 51200.0f / 3 - x;
            reference.m_gameCoords[2] = y;
            reference.m_gameFalloffStart /= 36;
            reference.m_gameFalloffEnd /= 36;
        }
        const auto* actual = g_lightDB.GetRecordByIndex(i);
        for (unsigned j = 0; j < 3; ++j)
            Require(std::abs(actual->m_gameCoords[j] - reference.m_gameCoords[j]) < 0.004f, "Lighting map axes or scale changed");
        Require(std::abs(actual->m_gameFalloffStart - reference.m_gameFalloffStart) < 0.001f &&
                    std::abs(actual->m_gameFalloffEnd - reference.m_gameFalloffEnd) < 0.001f,
                "Lighting falloff scale changed");
    }
    // Select authored knots across every parameter set, including both ends of
    // the day. Strictly ordered knots preserve their exact authored colors.
    unsigned checks = 0, repeatedTimes = 0;
    for (int32_t i = 0; i < g_lightParamsDB.m_numRecords; ++i) {
        const auto* params = g_lightParamsDB.GetRecordByIndex(i);
        for (unsigned band = 0; band < 18; ++band) {
            LightIntBandRec record;
            if (!g_lightIntBandDB.GetRecord(18 * params->m_id - 17 + int(band), &record))
                continue;
            bool ordered = true;
            for (int32_t knot = 1; knot < record.m_num; ++knot)
                ordered &= record.m_time[knot] > record.m_time[knot - 1];
            // Some shipped bands repeat timestamps. The reconstructed selector
            // treats a non-increasing interval as the midnight span, including
            // an equal pair, matching the local Wow.exe at 0x007EAE70;
            // it does not promise to visit later authored knots.
            if (!ordered) {
                ++repeatedTimes;
                continue;
            }
            for (int32_t knot = 0; knot < record.m_num; ++knot) {
                const auto actual = DayNight::LoadLightIntBand(record.m_time[knot], params, int(band));
                uint32_t color;
                std::memcpy(&color, &actual, 4);
                Require(color == (record.m_data[knot] | 0xFF000000u), "DayNight ordered color knot changed");
                ++checks;
            }
        }
    }
    const auto* repeated = g_lightParamsDB.GetRecord(20);
    Require(repeated, "Repeated-time lighting reference is missing");
    for (int time : {0, 720, 2879}) {
        const auto color = DayNight::LoadLightIntBand(time, repeated, 17);
        Require(color.r == 0x5C && color.g == 0x5C && color.b == 0x5C && color.a == 255, "Original repeated-time color selection changed");
    }
    LightFloatBandRec wrap{};
    wrap.m_num = 2;
    wrap.m_time[0] = 60;
    wrap.m_time[1] = 2820;
    wrap.m_data[0] = 36;
    wrap.m_data[1] = 108;
    Require(DayNight::InterpFloatBand(&wrap, 0, 0) == 2 && DayNight::InterpFloatBand(&wrap, 2880, 0) == 2 &&
                DayNight::InterpFloatBand(&wrap, 0, 1) == 72,
            "DayNight midnight wrap or distance conversion changed");
    std::fprintf(stderr,
                 "DayNight data: %d lights, %d parameter sets, %d skyboxes, %d color / %d float bands; both RLE modes and %u ordered "
                 "knots passed (%u bands retain repeated-time behavior)\n",
                 g_lightDB.m_numRecords, g_lightParamsDB.m_numRecords, g_lightSkyboxDB.m_numRecords, g_lightIntBandDB.m_numRecords,
                 g_lightFloatBandDB.m_numRecords, checks, repeatedTimes);
}

void CheckDayNightMap(unsigned cycle) {
    for (int map : {0, 530, 571}) {
        DayNight::LoadMap(map);
        struct Unload {
            ~Unload() { DayNight::UnloadMap(); }
        } unload;
        auto& info = *DayNight::GetInfo();
        info.m_farClip = 600;
        info.m_cameraDir = C3Vector(1, 0, 0);
        info.m_lightRefPos = map == 0 ? C3Vector(-9000, 0, 80) : map == 530 ? C3Vector(-2000, 5000, 0) : C3Vector(5400, -250, 200);
        int32_t skyLight = 0;
        for (uint32_t i = 1; i < DayNight::g_areaLights.Count(); ++i) {
            const auto* light = DayNight::g_areaLights[i];
            const auto* params = g_lightParamsDB.GetRecord(light->m_lightParamsID[0]);
            if (params && params->m_lightSkyboxID && light->m_gameFalloffEnd >= 3) {
                info.m_lightRefPos = C3Vector(light->m_gameCoords[0], light->m_gameCoords[1], light->m_gameCoords[2]);
                skyLight = light->m_id;
                break;
            }
        }
        for (float day : {0.0f, 0.25f, 0.5f, 0.75f, 0.999f}) {
            info.m_dayProgression = day;
            DayNight::UpdateLighting();
            Require(std::isfinite(info.m_bands.m_fogEnd) && info.m_bands.m_fogEnd >= 10 && std::isfinite(info.m_light1.m_dir.z) &&
                        info.m_light1.m_diffuse.a == 255,
                    "Original DayNight light output is invalid");
        }
        AsyncFileReadWaitAll();
        unsigned skies = 0;
        for (auto* sky = DayNight::g_overrideSky.m_fulllist.Head(); sky; sky = DayNight::g_overrideSky.m_fulllist.Next(sky)) {
            Require(sky->m_model && sky->m_model->IsLoaded(1, 0), "DayNight sky model did not load");
            CheckSky(*sky, map == 571);
            ++skies;
        }
        Require(!skyLight || skies, "Authored sky light did not select a sky model");
        std::fprintf(stderr, "DayNight map %d cycle %u: %u area lights, five time samples at light %d, %u authored sky models loaded\n",
                     map, cycle, DayNight::g_areaLights.Count(), skyLight, skies);
    }
}
