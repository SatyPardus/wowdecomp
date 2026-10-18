#include "Tools/AppleBootstrap/ModelPreview.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
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
#include <charconv>
#include <cstring>
#include <climits>

ModelPreview::ModelPreview(const char* name, uint32_t creatureDisplay, const char* character) : m_name(name ? name : "") {
    try {
        ComponentData characterData;
        uint32_t chestDisplay = 0;
        if (character) {
            uint32_t fields[8]{};
            unsigned count = 0;
            const char* end = character + std::strlen(character);
            while (character != end && count < 8) {
                auto parsed = std::from_chars(character, end, fields[count]);
                if (parsed.ec != std::errc() || fields[count] > INT_MAX || (parsed.ptr != end && *parsed.ptr != ':'))
                    throw std::runtime_error("WOW_CHARACTER requires race:sex[:skin:face:hairStyle:hairColor:facialHair:chestDisplay]");
                ++count; character = parsed.ptr;
                if (character != end && ++character == end) throw std::runtime_error("WOW_CHARACTER has an empty field");
            }
            if (character != end || count < 2 || fields[1] > 1)
                throw std::runtime_error("WOW_CHARACTER requires a race and sex 0 or 1, with at most eight fields");
            auto load = [](auto& db) { if (!db.m_loaded) db.Load(__FILE__, __LINE__); if (!db.m_loaded) throw std::runtime_error("Character appearance DBC failed to load"); };
            load(g_charHairGeosetsDB); load(g_charSectionsDB); load(g_characterFacialHairStylesDB);
            load(g_chrRacesDB); load(g_helmetGeosetVisDataDB); load(g_itemDisplayInfoDB);
            load(g_itemVisualEffectsDB); load(g_itemVisualsDB);
            const auto* race = g_chrRacesDB.GetRecord(fields[0]);
            if (!race) throw std::runtime_error("WOW_CHARACTER race is absent from ChrRaces.dbc");
            creatureDisplay = fields[1] ? race->m_femaleDisplayID : race->m_maleDisplayID;
            characterData.m_preferences.m_raceID = fields[0]; characterData.m_preferences.m_sexID = fields[1];
            characterData.m_preferences.m_classId = 1;
            characterData.m_preferences.m_skinID = fields[2]; characterData.m_preferences.m_faceId = fields[3];
            characterData.m_preferences.m_hairStyleId = fields[4]; characterData.m_preferences.m_hairColorId = fields[5];
            characterData.m_preferences.m_facialHairId = fields[6]; chestDisplay = fields[7];
        }
        if (!g_animationDataDB.m_loaded) g_animationDataDB.Load(__FILE__, __LINE__);
        CreatureDisplayInfoRec* display = nullptr;
        CreatureModelDataRec* modelData = nullptr;
        if (creatureDisplay) {
            if (!g_creatureDisplayInfoDB.m_loaded) g_creatureDisplayInfoDB.Load(__FILE__, __LINE__);
            if (!g_creatureModelDataDB.m_loaded) g_creatureModelDataDB.Load(__FILE__, __LINE__);
            if (!g_particleColorDB.m_loaded) g_particleColorDB.Load(__FILE__, __LINE__);
            display = g_creatureDisplayInfoDB.GetRecord(creatureDisplay);
            if (!display || !(modelData = g_creatureModelDataDB.GetRecord(display->m_modelID)))
                throw std::runtime_error("Creature display/model record is missing: " + std::to_string(creatureDisplay));
            if (display->m_extendedDisplayInfoID)
                throw std::runtime_error("Preview creature requires character texture composition, which is not integrated yet");
            m_name = modelData->m_modelName;
        }
        m_blobLoaded = ModelBlobLoad("World\\model.blob");
        if (!m_blobLoaded) throw std::runtime_error("Cannot load model bounds index");
        CShaderEffect::InitShaderSystem(1, 0); m_effectsInitialized = true;
        CShaderEffectManager::AddEffectFile("Model2.wfx");
        CShaderEffectManager::AddEffectFile("Particle.wfx");
        for (const char* effect : {"Particle", "Particle_Unlit", "Projected_ModMod", "Projected_ModAdd",
                                   "Projected_ModMod_Unlit", "Projected_ModAdd_Unlit"})
            if (!CShaderEffectManager::GetEffect(effect))
                throw std::runtime_error(std::string("Required original model effect is missing: ") + effect);
        M2Initialize(8, 16); m_initialized = true;
        m_scene = M2CreateScene();
        if (!m_scene) throw std::runtime_error("Cannot create preview model scene");
        m_model = m_scene->CreateModel(m_name.c_str(), 0);
        if (!m_model) throw std::runtime_error("Cannot load preview model: " + m_name);
        if (character) {
            if (!CCharacterComponent::Initialize()) throw std::runtime_error("Cannot initialize original character composition");
            m_componentsInitialized = true;
            CCharacterComponent::ValidateComponentData(&characterData, COMPONENT_CONTEXT_0);
            m_character = CCharacterComponent::AllocComponent();
            if (!m_character) throw std::runtime_error("Cannot allocate original character component");
            characterData.m_model = m_model;
            if (!m_character->Init(&characterData, nullptr)) throw std::runtime_error("Original character preferences are invalid");
            if (chestDisplay) {
                ItemDisplayInfoRec chest;
                if (!g_itemDisplayInfoDB.GetRecord(chestDisplay, &chest)) throw std::runtime_error("Character chest display is absent from ItemDisplayInfo.dbc");
                m_character->AddItem(ITEMSLOT_CHEST, &chest, 0);
            }
            m_character->RenderPrep(0);
            m_model->SetBoneSequence(0xFFFFFFFF, 4, 0xFFFFFFFF, 0, 1.0f, 0, 1);
        } else if (display) {
            CCharacterComponent::ApplyMonsterGeosets(m_model, display);
            CCharacterComponent::ReplaceMonsterSkin(m_model, display, modelData);
            m_model->SetBoneSequence(0xFFFFFFFF, 4, 0xFFFFFFFF, 0, 1.0f, 0, 1);
        }
        m_model->m_lightingCallback = [](CM2Model*, CM2Lighting* lighting, void*) {
            lighting->AddAmbient(C3Vector(0.3f, 0.3f, 0.35f));
            lighting->AddDiffuse(C3Vector(0.7f, 0.65f, 0.6f), C3Vector(-0.3f, -0.4f, -0.8f));
            lighting->SetFog(C3Vector(0.12f, 0.14f, 0.18f), 0.6f, 3.0f);
        };
    } catch (...) { Shutdown(); throw; }
}
ModelPreview::~ModelPreview() { Shutdown(); }
void ModelPreview::Shutdown() {
    if (m_character) { CCharacterComponent::FreeComponent(m_character); m_character = nullptr; m_model = nullptr; }
    if (m_componentsInitialized) { CCharacterComponent::Destroy(); m_componentsInitialized = false; }
    if (m_scene) { m_scene->Release(); m_scene = nullptr; m_model = nullptr; }
    if (m_initialized) { M2Destroy(); m_initialized = false; }
    if (m_effectsInitialized) { CShaderEffectManager::DestroyAllEffects(); m_effectsInitialized = false; }
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
    if (m_character && !m_character->RenderPrep(0)) return false;
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
    m_model->m_fFlags |= 0x10008; // World requests geometry and particles separately.
    m_model->SetAnimating(1); m_scene->Animate(C3Vector(0, 0, 0));
    m_scene->Draw(M2PASS_0); m_scene->Draw(M2PASS_1); m_scene->Draw(M2PASS_2);
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    return m_scene->m_elements.Count() != 0;
}
