#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "Engine/Source/Base/Prop.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Services/Texture.h"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>

namespace {
void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
template<class T> void Load(T& db) { if (!db.m_loaded) db.Load(__FILE__, __LINE__); Require(db.m_loaded, "Character appearance DBC failed to load"); }
void DumpCompressionInputs(CCharacterComponent* component) {
    const char* path = std::getenv("WOW_CHARACTER_BLOCK_DUMP");
    if (!path) return;
    // Optional derived-data input for an independent original-x86 compressor
    // comparison. Normal application/probe runs do not write these images.
    component->RenderPrepAll();
    FILE* file = std::fopen(path, "ab");
    Require(file != nullptr, "Cannot open character compression input dump");
    bool success = true;
    for (unsigned level = 0, size = 1u << CCharacterComponent::s_mipLevels; size >= 4; ++level, size >>= 1) {
        const auto* source = reinterpret_cast<const uint8_t*>(CCharacterComponent::s_textureBuffer->m_mip[level]);
        for (unsigned y = 0; y < size; y += 4) for (unsigned x = 0; x < size; x += 4)
            for (unsigned row = 0; row < 4; ++row)
                success &= std::fwrite(source + ((y + row) * size + x) * 4, 1, 16, file) == 16;
    }
    success &= std::fclose(file) == 0;
    Require(success, "Cannot write character compression input dump");
}
}

void CheckGameCharacters(CGxDeviceMetal& device, MTLRenderPassDescriptor* pass) {
    Load(g_charHairGeosetsDB); Load(g_charSectionsDB); Load(g_characterFacialHairStylesDB);
    Load(g_chrRacesDB); Load(g_helmetGeosetVisDataDB); Load(g_itemDisplayInfoDB);
    Load(g_itemVisualEffectsDB); Load(g_itemVisualsDB);
    Load(g_charBaseInfoDB); Load(g_charStartOutfitDB); Load(g_chrClassesDB);
    unsigned identities = 0;
    for (int i = 0; i < g_chrRacesDB.m_numRecords; ++i) {
        const auto& race = g_chrRacesDB.m_records[i];
        for (int sex = 0; sex < 2; ++sex) {
            const auto id = GetDisplayIdFromRaceAndGender(race.m_id, sex);
            Require(id == (sex ? race.m_femaleDisplayID : race.m_maleDisplayID), "Original character display lookup changed");
            int displaySex = -1;
            const char* name = CGUnit_C::GetDisplayRaceNameFromRecord(&race, sex, &displaySex);
            Require(name && *name && displaySex >= 0 && displaySex <= 1, "Original localized race name is missing");
            if (id) Require(GetModelDataRec(race.m_id, sex) != nullptr, "Original character model lookup failed");
            ++identities;
        }
    }
    for (int i = 0; i < g_chrClassesDB.m_numRecords; ++i) for (int sex = 0; sex < 2; ++sex) {
        const char* name = CGUnit_C::GetDisplayClassNameFromRecord(&g_chrClassesDB.m_records[i], sex, nullptr);
        Require(name && *name, "Original localized class name is missing");
    }
    Require(!GetDisplayIdFromRaceAndGender(0, 0) && !GetModelDataRec(0, 0) && !GetModelDataRec(1, 2), "Original missing/neutral character identity changed");
    std::fprintf(stderr, "Original character identity: %u race/sex entries, %d classes, %d starting outfits\n", identities, g_chrClassesDB.m_numRecords, g_charStartOutfitDB.m_numRecords);
    const auto context = PropGetSelectedContext();
    Require(context != nullptr, "Character composition requires the original Event context");
    Require(CCharacterComponent::Initialize(GxTex_Rgb565, 8, 1, 1), "Character component initialization failed");
    struct Shutdown { ~Shutdown() { CCharacterComponent::Destroy(); } } shutdown;
    auto* scene = M2CreateScene();
    struct Scene { CM2Scene* value; ~Scene() { value->Release(); } } releaseScene{scene};
    ItemDisplayInfoRec chest;
    for (int i = 0; i < g_itemDisplayInfoDB.m_numRecords; ++i) {
        Require(g_itemDisplayInfoDB.GetRecordByIndex(i, &chest), "Item appearance record failed to decode");
        if (*chest.m_texture[SECTION_TORSO_UPPER] && *chest.m_texture[SECTION_TORSO_LOWER]) break;
    }
    Require(*chest.m_texture[SECTION_TORSO_UPPER], "No original chest texture layers found");
    const unsigned cases[][3] = {{1, 0, 1}, {1, 1, 1}, {4, 0, 3}, {6, 1, 7}};
    for (const auto& entry : cases) {
        const unsigned raceID = entry[0], sex = entry[1], classID = entry[2];
        const auto* modelRec = GetModelDataRec(raceID, sex);
        Require(modelRec != nullptr, "Original human character model record is missing");
        ComponentData data;
        data.m_flags = sex ? 2 : 0; // Original selection high quality and world compressed paths.
        data.m_preferences.m_raceID = raceID; data.m_preferences.m_sexID = sex; data.m_preferences.m_classId = classID;
        CCharacterComponent::ValidateComponentData(&data, COMPONENT_CONTEXT_0);
        data.m_model = scene->CreateModel(modelRec->m_modelName, 0);
        Require(data.m_model && data.m_model->IsLoaded(1, 0), "Original character model failed to load");
        auto* component = CCharacterComponent::AllocComponent();
        Require(component != nullptr, "Character component allocation failed");
        struct Component { CCharacterComponent* value; ~Component() { CCharacterComponent::FreeComponent(value); } } releaseComponent{component};
        Require(component->Init(&data, nullptr), "Character appearance rejected original preferences");
        auto compose = [&] {
            component->RenderPrep(0);
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            while (component->m_dirtySections || (component->m_flags & 1)) {
                EventProcessPending(); PropSelectContext(context);
                Require(std::chrono::steady_clock::now() < deadline, "Original threaded character composition timed out");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        };
        compose();
        Require(component->m_baseSkinTexture && component->m_gxTexFormat == (sex ? GxTex_Argb8888 : GxTex_Dxt1), "Character composition changed the requested format");
        auto* model = component->m_data.m_model;
        Require(model->IsDrawable(1, 0), "Composed character textures did not become drawable");
        model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1, 0, 1);
        const auto& bounds = model->m_shared->m_data->m_bounds;
        C44Matrix world(1); world.RotateAroundX(-1.1f); world.Scale(0.8f / bounds.m_radius);
        world.Translate((bounds.m_extent.b + bounds.m_extent.t) * -0.5f);
        model->SetWorldTransform(world);
        model->m_lightingCallback = [](CM2Model*, CM2Lighting* light, void*) {
            light->AddAmbient(C3Vector(0.6f, 0.6f, 0.6f));
            light->AddDiffuse(C3Vector(0.4f, 0.4f, 0.4f), C3Vector(0, 0, -1));
            light->SetFog(C3Vector(0, 0, 0), 10, 20);
        };
        auto draw = [&] {
            const auto before = device.EncodingStats();
            device.BeginFrame(pass);
            struct Frame { CGxDeviceMetal& device; ~Frame() { if (device.FrameCommand()) device.ScenePresent(); } } frame{device};
            GxXformSetViewport(0, 1, 0, 1, 0, 1);
            device.XformSetView(C44Matrix(1)); device.XformSetProjection(C44Matrix(1));
            model->m_fFlags |= 8; model->SetAnimating(1); scene->AdvanceTime(0); scene->Animate(C3Vector(0, 0, 0));
            Require(scene->m_elements.Count() != 0, "Original character produced no material batches");
            scene->Draw(M2PASS_0); scene->Draw(M2PASS_1); scene->Draw(M2PASS_2);
            CiRect rect(0, 0, 128, 128);
            TSGrowableArray<CImVector> pixels; device.DeviceReadPixels(rect, pixels);
            const auto after = device.EncodingStats();
            unsigned covered = 0; for (unsigned i = 0; i < pixels.Count(); ++i) covered += pixels[i].a != 0;
            std::fprintf(stderr, "Original character sex %u: threaded format %u, %u batches, %u visible pixels\n", sex, component->m_gxTexFormat, scene->m_elements.Count(), covered);
            Require(covered > 32, "Original composed character did not reach Metal output");
            Require(after.renderPasses - before.renderPasses == 1, "Character geometry uploads interrupted the scene pass");
            Require(after.bufferUploadPasses - before.bufferUploadPasses <= 1, "Character geometry uploads did not batch together");
            return std::vector<CImVector>(pixels.Ptr(), pixels.Ptr() + pixels.Count());
        };
        const auto bare = draw();
        DumpCompressionInputs(component);
        component->AddItem(ITEMSLOT_CHEST, &chest, 0); compose();
        const auto equipped = draw();
        DumpCompressionInputs(component);
        unsigned changed = 0;
        for (size_t i = 0; i < bare.size(); ++i) changed += std::memcmp(&bare[i], &equipped[i], sizeof(CImVector)) != 0;
        Require(changed > 4, "Original equipment layers did not alter the character output");
        component->RemoveItem(ITEMSLOT_CHEST); compose();
        const auto restored = draw();
        Require(std::memcmp(bare.data(), restored.data(), bare.size() * sizeof(CImVector)) == 0, "Removing equipment did not restore the original character pixels");
        component->SetTexturesDirty(); component->RenderPrep(0);
        EventProcessPending(); PropSelectContext(context);
        // Cancel a submitted request, replace it, then destroy another queued
        // request on scope exit. The worker must retain its source ownership.
        component->SetTexturesDirty(); compose();
        const auto recomposed = draw();
        Require(std::memcmp(bare.data(), recomposed.data(), bare.size() * sizeof(CImVector)) == 0, "Canceled character recomposition changed the original pixels");
        std::fprintf(stderr, "Original character equipment %d: %u changed pixels; removal and canceled recomposition restored exact output\n", chest.m_id, changed);
        component->DressStartingOutfit(classID, &CDrawBowString::ModelLoadedCallback); compose();
        unsigned attachments = 0, bows = 0, shields = 0;
        for (auto* attached = model->m_attachList; attached; attached = attached->m_attachNext) {
            Require(attached->IsDrawable(1, 0), "Original starting equipment did not load");
            Require(model->HasAttachment(attached->m_attachmentId), "Equipment attached to a missing character bone");
            ++attachments;
            bows += attached->m_drawCallback == &CDrawBowString::GlueDrawBowStringCB;
            shields += attached->m_attachmentId == ATTACH_SHIELD;
            std::fprintf(stderr, "Starting attachment %u: %s, bow %d\n", attached->m_attachmentId, attached->m_shared->m_fileNameWithoutPath, attached->m_drawCallback != nullptr);
        }
        Require(attachments > 0 && (classID != 3 || bows > 0), "Original starting weapon/bow attachment is missing");
        Require(classID != 7 || shields == 1, "Original shaman starting shield is missing");
        const auto outfit = draw();
        unsigned outfitChanged = 0;
        for (size_t i = 0; i < bare.size(); ++i) outfitChanged += std::memcmp(&bare[i], &outfit[i], sizeof(CImVector)) != 0;
        Require(outfitChanged > 4, "Original starting outfit did not change the rendered character");
        const auto steady = draw();
        Require(std::memcmp(outfit.data(), steady.data(), outfit.size() * sizeof(CImVector)) == 0, "Unchanged starting outfit is not stable");
        component->DressStartingOutfit(classID, &CDrawBowString::ModelLoadedCallback); compose();
        for (auto* attached = model->m_attachList; attached; attached = attached->m_attachNext)
            Require(attached->IsDrawable(1, 0), "Replaced starting equipment did not load");
        const auto replaced = draw();
        Require(std::memcmp(outfit.data(), replaced.data(), outfit.size() * sizeof(CImVector)) == 0, "Replacing the same starting outfit changed its output");
        std::fprintf(stderr, "Original starting outfit race %u sex %u class %u: %u attachments, %u bows, %u changed pixels; replacement exact\n", raceID, sex, classID, attachments, bows, outfitChanged);
        component->SetTexturesDirty(); component->RenderPrep(0);
        EventProcessPending(); PropSelectContext(context);
    }
}
