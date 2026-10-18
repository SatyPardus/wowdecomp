#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Memory.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxVertexPC.h"
#include "WoW/Common/TerrainGeometry.h"
#include "WoW/Source/WorldClient/shadow.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <memory>
#include <stdexcept>

void CheckGameDetailDoodads(CM2Scene* scene, CGxDeviceMetal& device, MTLRenderPassDescriptor* pass);
void CheckGameCharacters(CGxDeviceMetal& device, MTLRenderPassDescriptor* pass);

namespace {
void CheckEmptySkeleton(CM2Scene* scene) {
    // Exercise a valid zero-bone instance with other arena-backed arrays. The
    // legacy release path used the absent bone view as its allocation owner.
    struct Data { M2Data header{}; M2Sequence sequence{}; M2Loop loops[2]{}; };
    auto shared = std::make_unique<CM2Shared>(nullptr);
    auto* data = new (M2AllocData(sizeof(Data))) Data();
    shared->m_data = &data->header;
    shared->m_skinData = new (SIMD::AllocA16(sizeof(M2SkinProfile), __FILE__, __LINE__)) M2SkinProfile();
    data->header.m_sequences.m_count = 1;
    data->header.m_sequences.Set(&data->sequence);
    data->sequence.m_flags = 0x20;
    data->header.m_loops.m_count = 2;
    data->header.m_loops.Set(data->loops);
    shared->m_flags = 3;
    CM2Model model;
    if (!model.Initialize(scene, shared.get(), nullptr, 0) || !(model.m_fFlags & 1) || model.m_bones || !model.m_loops ||
        model.m_loops[0] || model.m_loops[1])
        throw std::runtime_error("Zero-bone instance did not initialize its independent arrays");
    model.FreeExternalResources();
    model.FreeInternalResources();
    if (model.m_internalMemory || model.m_loops || model.m_textures || model.m_ribbonEmitters || model.m_particleEmitters)
        throw std::runtime_error("Zero-bone instance retained its arena or dangling views");
    model.FreeInternalResources();
    std::fprintf(stderr, "M2 zero-bone instance: independent arrays initialized and released\n");
}
}

void CheckGameModels() {
    if (!g_animationDataDB.m_loaded) g_animationDataDB.Load(__FILE__, __LINE__);
    if (!ModelBlobLoad("World\\model.blob")) throw std::runtime_error("Original model bounds index failed to load");
    struct BlobCleanup { ~BlobCleanup() { ModelBlobDestroy(); } } blobCleanup;
    CShaderEffect::InitShaderSystem(1, 0);
    struct Shutdown { bool cache = false; ~Shutdown() { if (cache) M2Destroy(); CShaderEffectManager::DestroyAllEffects(); } } shutdown;
    CShaderEffectManager::AddEffectFile("Model2.wfx");
    CShaderEffectManager::AddEffectFile("ShadowMap.wfx");
    CShaderEffectManager::AddEffectFile("Particle.wfx");
    for (const char* effect : {"Particle", "Particle_Unlit", "Projected_ModMod", "Projected_ModAdd",
                               "Projected_ModMod_Unlit", "Projected_ModAdd_Unlit"})
        if (!CShaderEffectManager::GetEffect(effect))
            throw std::runtime_error(std::string("Required original model effect is missing: ") + effect);
    M2Initialize(8, 16); shutdown.cache = true;
    auto* scene = M2CreateScene();
    if (!scene) throw std::runtime_error("M2 scene creation failed");
    struct Release { CM2Scene* scene; ~Release() { scene->Release(); } } release{scene};
    CheckEmptySkeleton(scene);
    auto* model = scene->CreateModel("Cameras\\FlyByDwarf.m2", 0);
    if (!model || !model->IsLoaded(1, 0)) throw std::runtime_error("M2 camera instance failed to load");
    if (!model->m_bones || model->m_shared->m_data->m_cameras.m_count != 1)
        throw std::runtime_error("M2 camera instance is incomplete");
    auto* arena = model->m_internalMemory;
    auto* camera = model->m_cameras[0].m_camera;
    if (!model->InitializeLoaded() || model->m_internalMemory != arena || model->m_cameras[0].m_camera != camera)
        throw std::runtime_error("Repeated M2 initialization replaced live instance resources");
    C3Vector first, last;
    for (uint32_t frame = 0; frame < 4; ++frame) {
        scene->AdvanceTime(frame ? 1000 : 0);
        model->SetAnimating(1);
        scene->Animate(C3Vector(0, 0, 0));
        auto* camera = static_cast<CCamera*>(model->GetCameraByIndex(0));
        if (!camera) throw std::runtime_error("M2 animated camera is missing");
        DataMgrGetCoord(camera, 7, &last);
        if (!std::isfinite(last.x) || !std::isfinite(last.y) || !std::isfinite(last.z))
            throw std::runtime_error("M2 camera animation is non-finite");
        if (!frame) first = last;
    }
    if (first.x == last.x && first.y == last.y && first.z == last.z)
        throw std::runtime_error("M2 camera animation did not advance");
    if (!g_creatureDisplayInfoDB.m_loaded) g_creatureDisplayInfoDB.Load(__FILE__, __LINE__);
    if (!g_creatureModelDataDB.m_loaded) g_creatureModelDataDB.Load(__FILE__, __LINE__);
    if (!g_particleColorDB.m_loaded) g_particleColorDB.Load(__FILE__, __LINE__);
    const auto* display = g_creatureDisplayInfoDB.GetRecord(165);
    const auto* modelData = display ? g_creatureModelDataDB.GetRecord(display->m_modelID) : nullptr;
    if (!modelData) throw std::runtime_error("Original wolf appearance records are missing");
    auto* visible = scene->CreateModel(modelData->m_modelName, 0);
    CCharacterComponent::ApplyMonsterGeosets(visible, display);
    CCharacterComponent::ReplaceMonsterSkin(visible, display, modelData);
    if (visible) visible->SetBoneSequence(0xFFFFFFFF, 4, 0xFFFFFFFF, 0, 1.0f, 0, 1);
    if (!visible || !visible->IsLoaded(1, 0) || !visible->IsDrawable(1, 0))
        throw std::runtime_error("M2 visible model or its textures failed to load");
    auto& device = *static_cast<CGxDeviceMetal*>(g_theGxDevicePtr);
    auto descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:128 height:128 mipmapped:NO];
    descriptor.usage = MTLTextureUsageRenderTarget; descriptor.storageMode = MTLStorageModePrivate;
    id<MTLTexture> target = [device.GPU().device newTextureWithDescriptor:descriptor];
    descriptor.pixelFormat = MTLPixelFormatDepth32Float;
    id<MTLTexture> depth = [device.GPU().device newTextureWithDescriptor:descriptor];
    auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = target; pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 0);
    pass.depthAttachment.texture = depth; pass.depthAttachment.loadAction = MTLLoadActionClear; pass.depthAttachment.clearDepth = 1;
    CheckGameCharacters(device, pass);
    device.BeginFrame(pass);
    struct EndFrame { CGxDeviceMetal& device; ~EndFrame() { if (device.FrameCommand()) device.ScenePresent(); } } endFrame{device};
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    C44Matrix identity(1.0f); device.XformSetView(identity);
    device.XformSetProjection(identity);
    const auto& bounds = visible->m_shared->m_data->m_bounds;
    const auto center = (bounds.m_extent.b + bounds.m_extent.t) * 0.5f;
    const float scale = 0.8f / bounds.m_radius;
    C44Matrix world(1); world.RotateAroundX(-0.65f); world.Scale(scale); world.Translate(center * -1.0f);
    visible->SetWorldTransform(world);
    visible->m_lightingCallback = [](CM2Model*, CM2Lighting* lighting, void*) {
        lighting->AddAmbient(C3Vector(0.4f, 0.4f, 0.4f));
        lighting->AddDiffuse(C3Vector(0.6f, 0.6f, 0.6f), C3Vector(0, 0, -1));
        lighting->SetFog(C3Vector(0.1f, 0.2f, 0.3f), 1, 20);
    };
    visible->m_fFlags |= 8; // Same draw request as World::RenderModels.
    visible->SetAnimating(1); scene->AdvanceTime(16); scene->Animate(C3Vector(0, 0, 0));
    if (!scene->m_elements.Count()) throw std::runtime_error("M2 scene produced no render elements");
    unsigned materials = 0;
    for (unsigned i = 0; i < scene->m_elements.Count(); ++i) {
        const auto& e = scene->m_elements[i];
        if (e.m_model != visible || e.m_vertexPermute < 20) throw std::runtime_error("Wolf did not select weighted native skinning");
        materials |= 1u << visible->m_shared->m_data->m_materials[e.m_batch->m_materialIndex].m_blendMode;
        std::fprintf(stderr, "M2 native draw: vertex %u pixel %u, %u indices\n", e.m_vertexPermute, e.m_pixelPermute, e.m_skinSection->m_indexCount);
    }
    if (materials != 3) throw std::runtime_error("Wolf opaque and alpha-cutout materials were not both selected");
    scene->Draw(M2PASS_0); scene->Draw(M2PASS_1); scene->Draw(M2PASS_2);
    CiRect rect(0, 0, 128, 128); TSGrowableArray<CImVector> pixels;
    device.DeviceReadPixels(rect, pixels);
    unsigned covered = 0;
    for (uint32_t i = 0; i < pixels.Count(); ++i) if (pixels[i].a) ++covered;
    if (covered < 256) throw std::runtime_error("M2 scene did not produce visible GPU pixels");
    std::vector<C44Matrix> firstPose(visible->m_boneMatrices, visible->m_boneMatrices + visible->m_shared->m_data->m_bones.m_count);
    device.ScenePresent();
    device.BeginFrame(pass);
    visible->m_fFlags |= 8;
    visible->SetAnimating(1); scene->AdvanceTime(137); scene->Animate(C3Vector(0, 0, 0));
    bool changed = false;
    for (size_t i = 0; i < firstPose.size(); ++i) {
        float matrix[16]; std::memcpy(matrix, &visible->m_boneMatrices[i], sizeof(matrix));
        for (float value : matrix) if (!std::isfinite(value)) throw std::runtime_error("Wolf pose is non-finite");
        changed |= std::memcmp(&firstPose[i], &visible->m_boneMatrices[i], sizeof(C44Matrix)) != 0;
    }
    if (!changed) throw std::runtime_error("Original wolf walk sequence did not advance its bone matrices");
    scene->Draw(M2PASS_0); scene->Draw(M2PASS_1); scene->Draw(M2PASS_2);
    TSGrowableArray<CImVector> animated; device.DeviceReadPixels(rect, animated);
    unsigned changedPixels = 0;
    for (uint32_t i = 0; i < pixels.Count(); ++i)
        changedPixels += std::memcmp(&pixels[i], &animated[i], sizeof(CImVector)) != 0;
    if (changedPixels < 16) throw std::runtime_error("Animated wolf pose did not reach Metal drawing");
    std::fprintf(stderr, "M2 native wolf scene: %u covered pixels, %u changed by original walk animation\n", covered, changedPixels);
    device.ScenePresent();
    {
        auto* effect = CShaderEffectManager::GetEffect("ShadowMapRenderSL");
        if (!effect) throw std::runtime_error("Original shadow-map effect is missing");
        const unsigned capacity = visible->m_shared->m_skinData->m_batches.m_count;
        std::vector<CModelBatch> opaqueStorage(capacity), cutoutStorage(capacity);
        CModelBatchList opaque{opaqueStorage.data(), 0, capacity}, cutout{cutoutStorage.data(), 0, capacity};
        visible->GatherModelBatchesShadowMap(opaque, cutout);
        if (!opaque.m_count || !cutout.m_count) throw std::runtime_error("Wolf shadow gather omitted opaque or cutout batches");
        device.RsPush();
        struct RestoreState { CGxDeviceMetal& device; ~RestoreState() { device.RsPop(); } } restoreState{device};
        struct DepthTarget { CGxDeviceMetal& device; CGxTex* texture = nullptr; ~DepthTarget() { device.TexDestroy(texture); } } shadowTarget{device};
        CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
        device.TexCreate(GxTex_2d, 128, 128, 0, GxTex_R32F, GxTex_R32F, flags, nullptr, nullptr, "Original wolf shadow depth", shadowTarget.texture);
        device.BeginFrame(pass); device.DeviceSetRenderTarget(GxBuffers_Color, shadowTarget.texture, 0);
        device.SceneClear(3, CImVector(255,255,255,255));
        effect->SetCurrent(); CShaderEffect::SetTexMtx_Identity(0); CShaderEffect::UpdateProjMatrix();
        const float lightFromView[] = {1,0,0,0, 0,1,0,0, 0,0,1,0.4f};
        const float depthScale[] = {0,0,0,0.5f};
        device.ShaderConstantsSet(GxSh_Vertex, 14, lightFromView, 3);
        device.ShaderConstantsSet(GxSh_Pixel, 0, depthScale, 1);
        const auto before = device.EncodingStats();
        CM2Model::RenderModelBatchesShadowMap(opaque, cutout);
        TSGrowableArray<float> shadow; device.ReadFloatColor(rect, shadow);
        unsigned covered = 0, positive = 0;
        for (unsigned i = 0; i < shadow.Count(); ++i) { covered += shadow[i] != 1.0f; positive += shadow[i] > 0 && shadow[i] < 1.0f; }
        const auto draws = device.EncodingStats().geometryDraws - before.geometryDraws;
        std::fprintf(stderr, "Original wolf R32F shadow generator: %u opaque + %u cutout batches, %llu draws, %u covered / %u positive-depth pixels\n",
            opaque.m_count, cutout.m_count, static_cast<unsigned long long>(draws), covered, positive);
        if (draws != opaque.m_count + cutout.m_count || covered < 256 || positive < 128)
            throw std::runtime_error("Original shadow batches did not generate a visible depth map");
        device.DeviceSetRenderTarget(GxBuffers_Color, nullptr, 0);
        device.ScenePresent();
    }
    CheckGameDetailDoodads(scene, device, pass);
    struct LayeredPose { uint32_t displayID, sequenceID, elapsed; };
    // The motorcycle's dual-UV effect is authored only in its death sequence:
    // color alpha stays zero through 500 ms, then rises to one at 1000 ms.
    for (const auto pose : {LayeredPose{110, 4, 137}, LayeredPose{25870, 4, 137}, LayeredPose{25870, 1, 1137}}) {
        const auto displayID = pose.displayID;
        auto* layeredScene = M2CreateScene();
        if (!layeredScene) throw std::runtime_error("Layered model scene creation failed");
        Release releaseLayered{layeredScene};
        auto* appearance = g_creatureDisplayInfoDB.GetRecord(displayID);
        auto* appearanceModel = appearance ? g_creatureModelDataDB.GetRecord(appearance->m_modelID) : nullptr;
        if (!appearanceModel) throw std::runtime_error("Layered model appearance is missing");
        auto* layeredModel = layeredScene->CreateModel(appearanceModel->m_modelName, 0);
        if (!layeredModel) throw std::runtime_error("Layered model creation failed");
        CCharacterComponent::ApplyMonsterGeosets(layeredModel, appearance);
        CCharacterComponent::ReplaceMonsterSkin(layeredModel, appearance, appearanceModel);
        layeredModel->SetBoneSequence(0xFFFFFFFF, pose.sequenceID, 0xFFFFFFFF, 0, 1.0f, 0, 1);
        if (!layeredModel->IsLoaded(1, 0) || !layeredModel->IsDrawable(1, 0)) throw std::runtime_error("Layered model resources did not load");
        const auto& bounds = layeredModel->m_shared->m_data->m_bounds;
        C44Matrix world(1); world.RotateAroundX(-0.65f); world.Scale(0.8f / bounds.m_radius);
        world.Translate((bounds.m_extent.b + bounds.m_extent.t) * -0.5f);
        layeredModel->SetWorldTransform(world); layeredModel->m_lightingCallback = visible->m_lightingCallback;
        layeredModel->m_fFlags |= 0x10008; layeredModel->SetAnimating(1);
        layeredScene->AdvanceTime(pose.elapsed); layeredScene->Animate(C3Vector(0, 0, 0));
        unsigned reflectedBatches = 0, reflectedMaterials = 0, dualUVBatches = 0, combinedBatches = 0;
        for (unsigned i = 0; i < layeredScene->m_elements.Count(); ++i) {
            const auto& element = layeredScene->m_elements[i];
            if (element.m_type > 2 || !element.m_batch || element.m_batch->m_textureCount != 2) continue;
            ++reflectedBatches;
            dualUVBatches += !(element.m_batch->m_shader & 0x8000) && !(element.m_batch->m_shader & 8);
            combinedBatches += (element.m_batch->m_shader & 0x8000) != 0;
            reflectedMaterials |= 1u << layeredModel->m_shared->m_data->m_materials[element.m_batch->m_materialIndex].m_blendMode;
        }
        device.BeginFrame(pass);
        layeredScene->Draw(M2PASS_0); layeredScene->Draw(M2PASS_1); layeredScene->Draw(M2PASS_2);
        TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect, image);
        unsigned pixels = 0;
        for (unsigned i = 0; i < image.Count(); ++i) pixels += image[i].r || image[i].g || image[i].b;
        std::fprintf(stderr, "M2 display %u sequence %u: %u dual-texture batches (%u authored UV pairs, %u combined layers), %u visible pixels\n",
            displayID, pose.sequenceID, reflectedBatches, dualUVBatches, combinedBatches, pixels);
        if (pixels < (pose.sequenceID == 1 ? 16u : 256u) || (displayID == 110 ? reflectedBatches != 3 || reflectedMaterials != 3
            : dualUVBatches != (pose.sequenceID == 1 ? 1u : 0u) || combinedBatches != 2))
            throw std::runtime_error("Original layered model materials did not reach Metal drawing");
        device.ScenePresent();
    }
    {
        // Exercise authored projected batches through CM2SceneRender. The
        // bounded planar receiver is an explicit fixture, not a world query.
        struct ShadowCleanup {
            ~ShadowCleanup() { ShadowDestroy(); }
        } shadowCleanup;
        ShadowInit();
        for (const char *name : {"Spells\\AcidBurn.m2", "Spells\\Blizzard_Spawn.m2", "Spells\\Consecration_Impact_Base.m2"}) {
            auto *projectedScene = M2CreateScene();
            if (!projectedScene)
                throw std::runtime_error("Projected spell scene creation failed");
            Release releaseProjected{projectedScene};
            struct Receiver {
                unsigned draws = 0;
            } receiver;
            projectedScene->SetProjectTextureCallback(
                [](const CAaBox &box, const CImVector &color, int32_t lit, void *arg, int32_t) {
                    auto &receiver = *static_cast<Receiver *>(arg);
                    const float z = (box.b.z + box.t.z) * 0.5f;
                    const C3Vector corners[] = {{box.b.x, box.b.y, z}, {box.t.x, box.b.y, z}, {box.b.x, box.t.y, z},
                                                {box.b.x, box.t.y, z}, {box.t.x, box.b.y, z}, {box.t.x, box.t.y, z}};
                    auto &device = *g_theGxDevicePtr;
                    const bool mod = device.m_appRenderStates[GxRs_BlendingMode].m_value.m_data.i[0] == GxBlend_Mod;
                    device.RsSet(GxRs_Texture1, mod ? ProjectTex2dGetModFadeTex() : ProjectTex2dGetAddFadeTex());
                    C44Matrix identity(1);
                    GxXformSet(GxXform_World, identity);
                    CShaderEffect::UpdateWorldViewMatrix();
                    const auto format = lit ? GxVBF_PN : GxVBF_PC;
                    auto *buffer = GxBufStream(GxPoolTarget_Vertex, GxVertexSize(format), 6);
                    auto *data = device.BufLock(buffer);
                    if (lit) {
                        CShaderEffect::SetDiffuse(C4Vector(color));
                        auto *vertices = reinterpret_cast<CGxVertexPN *>(data);
                        for (unsigned i = 0; i < 6; ++i) {
                            vertices[i].m_position = corners[i];
                            vertices[i].m_normal = C3Vector(0, 0, 1);
                        }
                    } else {
                        CImVector packed = color;
                        GxFormatColor(packed);
                        auto *vertices = reinterpret_cast<CGxVertexPC *>(data);
                        for (unsigned i = 0; i < 6; ++i) {
                            vertices[i].p = corners[i];
                            vertices[i].c = packed;
                        }
                    }
                    device.BufUnlock(buffer, 0);
                    GxPrimVertexPtr(buffer, format);
                    CShaderEffect::SetDefaultShaders(0);
                    CGxBatch batch{GxPrim_Triangles, 0, 6, 0, 5};
                    GxDraw(&batch, 0);
                    ++receiver.draws;
                },
                &receiver);
            auto *spell = projectedScene->CreateModel(name, 0);
            if (!spell || !spell->IsLoaded(1, 0) || !spell->IsDrawable(1, 0))
                throw std::runtime_error("Projected spell resources did not load");
            const auto &bounds = spell->m_shared->m_data->m_bounds;
            C44Matrix world(1);
            world.Translate(C3Vector(0, 0, 0.4f));
            world.Scale(0.65f / bounds.m_radius);
            world.Translate((bounds.m_extent.b + bounds.m_extent.t) * -0.5f);
            spell->SetWorldTransform(world);
            spell->m_lightingCallback = visible->m_lightingCallback;
            for (unsigned i = 0; i < 32; ++i) {
                spell->m_fFlags |= 8;
                spell->SetAnimating(1);
                projectedScene->AdvanceTime(16);
                projectedScene->Animate(C3Vector(0, 0, 0));
            }
            unsigned batches = 0;
            for (unsigned i = 0; i < projectedScene->m_elements.Count(); ++i)
                batches += projectedScene->m_elements[i].m_type == 1;
            device.BeginFrame(pass);
            projectedScene->Draw(M2PASS_0);
            projectedScene->Draw(M2PASS_1);
            projectedScene->Draw(M2PASS_2);
            TSGrowableArray<CImVector> image;
            device.DeviceReadPixels(rect, image);
            unsigned changed = 0;
            for (unsigned i = 0; i < image.Count(); ++i)
                changed += image[i].r || image[i].g || image[i].b;
            std::fprintf(stderr, "Projected spell %s: %u authored batches, %u receiver draws, %u visible pixels\n", name, batches,
                         receiver.draws, changed);
            if (!batches || receiver.draws != batches || changed < 16)
                throw std::runtime_error("Authored projected spell did not reach its receiver");
            device.ScenePresent();
        }
    }
    for (const bool ribbons : {false, true}) {
        auto *spellScene = M2CreateScene();
        if (!spellScene)
            throw std::runtime_error("Spell scene creation failed");
        Release releaseSpell{spellScene};
        const char* name = ribbons ? "Spells\\Rejuvenation_Impact_Base.m2" : "Spells\\Fireball_Missile_Low.m2";
        auto* spell = spellScene->CreateModel(name, 0);
        if (!spell || !spell->IsLoaded(1, 0) || !spell->IsDrawable(1, 0))
            throw std::runtime_error(std::string("Original spell resources failed to load: ") + name);
        const auto& spellBounds = spell->m_shared->m_data->m_bounds;
        C44Matrix spellWorld(1); spellWorld.Scale(0.8f / spellBounds.m_radius);
        spellWorld.Translate((spellBounds.m_extent.b + spellBounds.m_extent.t) * -0.5f);
        spell->m_lightingCallback = visible->m_lightingCallback;
        // Rejuvenation has no mesh: first draw only its original ribbons, then
        // enable its sphere/plane emitters to exercise the combined effect.
        if (ribbons) spell->SetEmittersEnabled(0);
        for (unsigned frame = 0; frame < (ribbons ? 64u : 32u); ++frame) {
            if (ribbons && frame == 32) spell->SetEmittersEnabled(1);
            spell->SetWorldTransform(spellWorld);
            spell->m_fFlags |= 0x10008; spell->SetAnimating(1);
            spellScene->AdvanceTime(16); spellScene->Animate(C3Vector(0, 0, 0));
            if (frame != 31 && frame != 63) continue;
            unsigned particleDraws = 0, ribbonDraws = 0, rendered = 0, sphereParticles = 0, primitives = 0;
            for (unsigned i = 0; i < spellScene->m_elements.Count(); ++i) {
                particleDraws += spellScene->m_elements[i].m_type == 4;
                ribbonDraws += spellScene->m_elements[i].m_type == 3;
            }
            device.BeginFrame(pass);
            const auto beforeEncoding = device.EncodingStats();
            spellScene->Draw(M2PASS_0); spellScene->Draw(M2PASS_1); spellScene->Draw(M2PASS_2);
            const auto afterEncoding = device.EncodingStats();
            for (unsigned i = 0; i < spell->m_shared->m_data->m_particles.m_count; ++i) {
                rendered += spell->m_particleEmitters[i]->m_renderedParticles;
                if (spell->m_shared->m_data->m_particles[i].m_emitterType == 2)
                    sphereParticles += spell->m_particleEmitters[i]->m_renderedParticles;
            }
            for (unsigned i = 0; i < spell->m_shared->m_data->m_ribbons.m_count; ++i)
                primitives += spell->m_ribbonEmitters[i]->GetNumPrimitives();
            TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect, image);
            unsigned visiblePixels = 0;
            for (unsigned i = 0; i < image.Count(); ++i) visiblePixels += image[i].r || image[i].g || image[i].b;
            std::fprintf(stderr, "M2 %s frame %u: %u particle batches, %u particles, %u ribbon batches, %u ribbon triangles, %u visible pixels\n",
                name, frame, particleDraws, rendered, ribbonDraws, primitives, visiblePixels);
            std::fprintf(stderr, "M2 encoding: %llu geometry draws, %llu render passes, %llu buffer upload passes\n",
                (unsigned long long)(afterEncoding.geometryDraws - beforeEncoding.geometryDraws),
                (unsigned long long)(afterEncoding.renderPasses - beforeEncoding.renderPasses),
                (unsigned long long)(afterEncoding.bufferUploadPasses - beforeEncoding.bufferUploadPasses));
            if (visiblePixels < 16 || (ribbons && (!ribbonDraws || !primitives)) ||
                ((!ribbons || frame == 63) && (!particleDraws || !rendered)) ||
                (ribbons && (frame == 31 ? particleDraws != 0 : sphereParticles == 0)))
                throw std::runtime_error(std::string("Original spell particles/ribbons did not reach Metal drawing: ") + name);
            device.ScenePresent();
        }
    }
}
