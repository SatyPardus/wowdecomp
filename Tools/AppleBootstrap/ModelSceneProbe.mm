#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Memory.h"
#include "Engine/Source/Base/CRTRandom.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/Services/ParticleSystem2.h"
#include "Engine/Source/Services/RibbonEmitter.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Services/Texture.h"
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
#include <array>
#include <algorithm>
#include <memory>
#include <stdexcept>

void CheckGameDetailDoodads(CM2Scene* scene, CGxDeviceMetal& device, MTLRenderPassDescriptor* pass);
void CheckGameCharacters(CGxDeviceMetal& device, MTLRenderPassDescriptor* pass);

namespace {
void CheckSequenceSpeed(CM2Model& model) {
    auto& sequence = model.m_bones[0].m_sequence;
    struct Restore {
        CM2Model& model; M2ModelBoneSeq sequence; uint32_t time;
        ~Restore() { model.m_bones[0].m_sequence = sequence; model.m_scene->m_time = time; }
    } restore{model, sequence, model.m_scene->m_time};
    struct Case { uint32_t now, start, offset; float previousSpeed, speed; uint32_t expectedStart, numerator, denominator; };
    // Expected clocks follow the two integer truncation points in Wow.exe
    // 0x008270BD..0x0082716C, including a sequence starting one tick in the future.
    for (const auto c : {
        Case{100,101,0,2,.5f,104,2,1}, Case{100,99,0,1.5f,.5f,98,2,1},
        Case{1,0xFFFFFFFFu,10,.5f,2,0xFFFFFFFCu,1,2}, Case{31,11,50,-1.5f,-2,21,1,2},
        Case{16777217,16777216,0,1,1,16777216,1,1},
        Case{0xFFFFFFF0u,0x10,0x80000010u,1,1,0x80000000u,1,1},
        Case{100,101,0,2,0,100,0,1}, Case{100,101,0,2,.000001f,100,0,1}}) {
        model.m_scene->m_time = c.now;
        sequence.m_sequenceIndex = 0; sequence.m_repeatCount = 1;
        sequence.m_startTime = c.start; sequence.m_startOffset = c.offset; sequence.m_speed = c.previousSpeed;
        model.SetBoneSequenceSpeed(0xFFFFFFFFu, c.speed);
        const uint32_t duration = model.m_shared->m_data->m_sequences[0].m_duration;
        if (sequence.m_startTime != c.expectedStart ||
            sequence.m_endTime != c.expectedStart + uint32_t(uint64_t(duration)*c.numerator/c.denominator) ||
            sequence.m_speed != c.speed || sequence.m_invSpeed != (c.numerator ? 1.0f/c.speed : 0.0f))
            throw std::runtime_error("M2 speed change lost signed/wrapping clocks or an original truncation boundary");
    }
    std::fputs("M2 speed changes: future start, clock wrap, fractional/reverse/zero speed and integer precision passed\n",stderr);
}

void CheckEmptySkeleton(CM2Scene* scene) {
    // Exercise a valid zero-bone instance with other arena-backed arrays. The
    // legacy release path used the absent bone view as its allocation owner.
    struct Data { M2Data header{}; M2Sequence sequences[2]{}; M2Loop loops[2]{}; };
    auto shared = std::make_unique<CM2Shared>(nullptr);
    auto* data = new (M2AllocData(sizeof(Data))) Data();
    shared->m_data = &data->header;
    shared->m_skinData = new (SIMD::AllocA16(sizeof(M2SkinProfile), __FILE__, __LINE__)) M2SkinProfile();
    data->header.m_sequences.m_count = 2;
    data->header.m_sequences.Set(data->sequences);
    for (unsigned i = 0; i < 2; ++i) {
        data->sequences[i].m_flags = 0x20;
        data->sequences[i].m_frequency = 16384;
        data->sequences[i].m_variationNext = i ? 0xFFFF : 1;
        data->sequences[i].m_duration = 100;
        data->sequences[i].m_replay.m_l = 2;
        data->sequences[i].m_replay.m_h = 6;
    }
    data->header.m_loops.m_count = 2;
    data->header.m_loops.Set(data->loops);
    shared->m_flags = 3;
    CM2Model model;
    if (!model.Initialize(scene, shared.get(), nullptr, 0) || !(model.m_fFlags & 1) || model.m_bones || !model.m_loops ||
        model.m_loops[0] || model.m_loops[1])
        throw std::runtime_error("Zero-bone instance did not initialize its independent arrays");
    ClientSrand(1);
    unsigned selected[2]{};
    for (unsigned i = 0; i < 4096; ++i) {
        uint16_t index = 0;
        uint32_t variation = 0;
        model.PickRandomSequenceVariation(&variation, &index);
        if (index > 1 || index != variation)
            throw std::runtime_error("M2 random variation escaped its authored sequence chain");
        ++selected[index];
        M2ModelBoneSeq clock{};
        model.SetupBoneSequence(index, {}, 0, 2.0f, &clock);
        if (clock.m_repeatCount < 2 || clock.m_repeatCount >= 6 ||
            clock.m_endTime - clock.m_startTime != clock.m_repeatCount * 50)
            throw std::runtime_error("M2 random replay escaped authored bounds or changed duration");
    }
    if (selected[0] < 1000 || selected[1] < 1000)
        throw std::runtime_error("M2 random variation did not select both authored alternatives");
    std::fprintf(stderr, "M2 original random variation/replay: %u / %u selections, all authored replay bounds preserved\n", selected[0], selected[1]);
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
    CheckSequenceSpeed(*model);
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
        struct DepthTarget { CTexture* owner = nullptr; CGxTex* texture = nullptr; ~DepthTarget() { if(owner) HandleClose(reinterpret_cast<HOBJECT>(owner)); } } shadowTarget, comparisonTarget;
        CGxTexFlags flags(GxTex_Nearest); flags.m_renderTarget = 1;
        shadowTarget.owner = TextureCreate(GxTex_2d,128,128,0,GxTex_R32F,GxTex_R32F,flags,nullptr,
            [](EGxTexCommand,uint32_t,uint32_t,uint32_t,uint32_t,void*,uint32_t&,const void*&) {},"Original wolf shadow depth",0);
        if(!shadowTarget.owner || !(shadowTarget.texture=TextureGetGxTex(shadowTarget.owner,1,nullptr)))
            throw std::runtime_error("Original wolf shadow target creation failed");
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
        struct RestoreShadows {
            CM2Model* model;
            decltype(CM2Model::m_lightingCallback) lighting;
            EShadowMode mode=CShadowCache::s_shadowMode;
            int32_t recreate=CShadowCache::s_recreateTextures;
            uint32_t pcf=CShaderEffect::s_usePcfFiltering;
            CShadowBand band=CShadowCache::s_extDynShadow;
            std::array<CShadowBand,3> distant;
            C4Vector cutoff=CShadowCache::s_cutoffPlane, direction=CShadowCache::s_sunDirViewSpace;
            C4Vector cutoffDefault=CShadowCache::s_cutoffPlaneDefault;
            std::array<C4Vector,12> matrices;
            std::array<C4Vector,8> offsets;
            RestoreShadows(CM2Model* m):model(m),lighting(m->m_lightingCallback) {
                std::copy_n(CShadowCache::s_shadowMatrices,12,matrices.begin());
                std::copy_n(CShadowCache::s_pcfOffsets,8,offsets.begin());
                std::copy_n(CShadowCache::s_extShadowBands,3,distant.begin());
            }
            ~RestoreShadows() {
                model->m_lightingCallback=lighting; CShadowCache::s_shadowMode=mode; CShadowCache::s_recreateTextures=recreate;
                CShaderEffect::s_usePcfFiltering=pcf; CShadowCache::s_extDynShadow=band;
                CShadowCache::s_cutoffPlane=cutoff; CShadowCache::s_sunDirViewSpace=direction;
                CShadowCache::s_cutoffPlaneDefault=cutoffDefault;
                std::copy(distant.begin(),distant.end(),CShadowCache::s_extShadowBands);
                std::copy(matrices.begin(),matrices.end(),CShadowCache::s_shadowMatrices);
                std::copy(offsets.begin(),offsets.end(),CShadowCache::s_pcfOffsets);
            }
        } restoreShadows{visible};
        CShadowCache::s_shadowMode=SHADOW_LOW; CShadowCache::s_recreateTextures=0; CShaderEffect::s_usePcfFiltering=0;
        CShadowCache::s_extDynShadow.m_textures[0]=shadowTarget.owner;
        auto vector=[](float x,float y,float z,float w) { C4Vector v; v.x=x; v.y=y; v.z=z; v.w=w; return v; };
        CShadowCache::s_cutoffPlane=vector(0,0,0,0); CShadowCache::s_sunDirViewSpace=vector(0,0,1,0);
        CShadowCache::s_shadowMatrices[0]=vector(1,0,0,0); CShadowCache::s_shadowMatrices[1]=vector(0,1,0,0);
        CShadowCache::s_shadowMatrices[2]=vector(0,0,.5f,.21f); // Same light transform, explicit 0.01 receiver offset.
        CShadowCache::s_cutoffPlaneDefault=vector(0,0,0,0);
        for(unsigned band=0;band<3;++band) {
            CShadowCache::s_extShadowBands[band].m_current=0;
            CShadowCache::s_extShadowBands[band].m_textures[0]=shadowTarget.owner;
            for(unsigned row=0;row<3;++row) CShadowCache::s_shadowMatrices[3+band*3+row]=CShadowCache::s_shadowMatrices[row];
        }
        const C2Vector offsets[]={{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1}};
        for(unsigned i=0;i<8;++i) CShadowCache::s_pcfOffsets[i]=vector(offsets[i].x/128,offsets[i].y/128,0,0);
        visible->m_lightingCallback=[](CM2Model*,CM2Lighting* lighting,void*) {
            lighting->AddAmbient(C3Vector(.4f,.4f,.4f)); lighting->AddDiffuse(C3Vector(.6f,.6f,.6f),C3Vector(0,0,-1));
            lighting->SetFog(C3Vector(.1f,.2f,.3f),1,20); lighting->m_flags|=0x10;
        };
        auto checkReceivers = [&](bool comparison) {
            CShaderEffect::s_usePcfFiltering=comparison;
            for (EShadowMode mode:{SHADOW_LOW,SHADOW_HIGH,SHADOW_VERY_HIGH}) {
                CShadowCache::s_shadowMode=mode;
                const uint32_t expectedShadow=CShadowCache::s_shadowValues[mode];
                device.BeginFrame(pass); visible->m_fFlags|=8; visible->SetAnimating(1); scene->Animate(C3Vector(0,0,0));
                unsigned receivers=0;
                for(unsigned i=0;i<scene->m_elements.Count();++i) {
                    const auto& element=scene->m_elements[i];
                    if(element.m_model==visible && element.m_shadowValue==expectedShadow &&
                        element.m_vertexPermute>=20+30*std::min(expectedShadow,2u) && (element.m_pixelPermute&7)==expectedShadow+4*comparison) ++receivers;
                }
                if(receivers!=2) throw std::runtime_error("Original wolf scene did not select both shadow receivers");
                scene->Draw(M2PASS_0); scene->Draw(M2PASS_1); scene->Draw(M2PASS_2);
                TSGrowableArray<CImVector> received; device.DeviceReadPixels(rect,received); unsigned darker=0;
                for(unsigned i=0;i<received.Count();++i) {
                    if(received[i].a!=animated[i].a) throw std::runtime_error("Shadow changed original wolf alpha coverage");
                    darker+=unsigned(received[i].r)+received[i].g+received[i].b+2 < unsigned(animated[i].r)+animated[i].g+animated[i].b;
                }
                if(darker<64) throw std::runtime_error("Generated shadow did not reach the original wolf material");
                std::fprintf(stderr,"Original wolf %s shadow receivers mode %u: %u batches, %u darker pixels, unchanged alpha coverage\n",comparison ? "PCF":"R32F",expectedShadow,receivers,darker);
                device.ScenePresent();
                // Exercise the original ground-decoration selector and cache
                // bindings with the same generated R32F map and all three modes.
                if(!comparison) CheckGameDetailDoodads(scene, device, pass);
            }
        };
        checkReceivers(false);
        CGxTexFlags comparisonFlags(GxTex_Linear); comparisonFlags.m_renderTarget=1;
        comparisonTarget.owner=TextureCreate(GxTex_2d,128,128,0,GxTex_D24X8,GxTex_D24X8,comparisonFlags,nullptr,
            [](EGxTexCommand,uint32_t,uint32_t,uint32_t,uint32_t,void*,uint32_t&,const void*&) {},"Original wolf PCF depth",0);
        if(!comparisonTarget.owner || !(comparisonTarget.texture=TextureGetGxTex(comparisonTarget.owner,1,nullptr)))
            throw std::runtime_error("Original wolf comparison target creation failed");
        device.BeginFrame(pass); device.DeviceSetRenderTarget(GxBuffers_Depth,comparisonTarget.texture,0);
        GxXformSetViewport(0,1,0,1,0,1); GxRsSet(GxRs_ColorWrite,0); GxRsSet(GxRs_Fog,0);
        device.SceneClear(2,CImVector(0,0,0,0));
        // The original projection API converts [-w,w] to [0,w]. This maps
        // light.z to light.z/2, matching the R32F map's explicit scale above.
        C44Matrix lightProjection(1); lightProjection.d2=-1;
        device.XformSetProjection(lightProjection);
        effect->SetCurrent(); CShaderEffect::SetTexMtx_Identity(0); CShaderEffect::UpdateProjMatrix();
        device.ShaderConstantsSet(GxSh_Vertex,14,lightFromView,3);
        device.ShaderConstantsSet(GxSh_Pixel,0,depthScale,1);
        CM2Model::RenderModelBatchesShadowMap(opaque,cutout);
        TSGrowableArray<float> comparisonDepth; device.DeviceReadDepths(rect,comparisonDepth);
        unsigned comparisonCovered=0;
        for(unsigned i=0;i<comparisonDepth.Count();++i) comparisonCovered+=comparisonDepth[i]>0 && comparisonDepth[i]<1;
        if(comparisonCovered<128) throw std::runtime_error("Original wolf batches did not write comparison depth");
        std::fprintf(stderr,"Original wolf native comparison depth: %u covered pixels\n",comparisonCovered);
        device.DeviceSetRenderTarget(GxBuffers_Depth,nullptr,0); device.ScenePresent();
        device.XformSetProjection(identity); GxRsSet(GxRs_ColorWrite,15);
        CShadowCache::s_extDynShadow.m_textures[1]=comparisonTarget.owner;
        for(unsigned band=0;band<3;++band) CShadowCache::s_extShadowBands[band].m_textures[0]=comparisonTarget.owner;
        checkReceivers(true);
        CShaderEffect::s_usePcfFiltering=0;
        for(unsigned band=0;band<3;++band) CShadowCache::s_extShadowBands[band].m_textures[0]=shadowTarget.owner;
        CShadowCache::s_shadowMode=SHADOW_LOWEST;
        CheckGameDetailDoodads(scene, device, pass);
        struct LayeredPose { uint32_t displayID, sequenceID, elapsed; };
        // The motorcycle's dual-UV effect is authored only in its death sequence:
        // color alpha stays zero through 500 ms, then rises to one at 1000 ms.
        for (const auto pose : {LayeredPose{110, 4, 137}, LayeredPose{25870, 4, 137}, LayeredPose{25870, 1, 1137}}) {
            CShadowCache::s_shadowMode=SHADOW_LOWEST;
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
            for(bool comparison:{false,true}) {
                CShaderEffect::s_usePcfFiltering=comparison;
                for(unsigned band=0;band<3;++band) CShadowCache::s_extShadowBands[band].m_textures[0]=comparison ? comparisonTarget.owner:shadowTarget.owner;
                for(EShadowMode mode:{SHADOW_LOWEST,SHADOW_LOW,SHADOW_HIGH,SHADOW_VERY_HIGH}) {
                    CShadowCache::s_shadowMode=mode;
                    const uint32_t expectedShadow=CShadowCache::s_shadowValues[mode];
                    device.BeginFrame(pass); layeredModel->m_fFlags|=0x10008; layeredModel->SetAnimating(1);
                    // Keep the original particle draw request and freeze elapsed
                    // time while only the shadow-quality selection changes.
                    layeredScene->AdvanceTime(0); layeredScene->Animate(C3Vector(0,0,0));
                    unsigned receivers=0,combinedReceivers=0;
                    for(unsigned i=0;i<layeredScene->m_elements.Count();++i) {
                        const auto& element=layeredScene->m_elements[i];
                        if(element.m_type>2 || element.m_model!=layeredModel || !element.m_batch || element.m_batch->m_textureCount!=2 || element.m_shadowValue!=expectedShadow) continue;
                        if((element.m_pixelPermute&7)!=expectedShadow+4*comparison) throw std::runtime_error("Layered model selected the wrong shadow sampling mode");
                        ++receivers; combinedReceivers+=(element.m_batch->m_shader&0x8000)!=0;
                    }
                    if(expectedShadow && (receivers!=3 || combinedReceivers!=(displayID==25870 ? 2u:0u)))
                        throw std::runtime_error("Original layered model did not select its shadow receivers");
                    layeredScene->Draw(M2PASS_0); layeredScene->Draw(M2PASS_1); layeredScene->Draw(M2PASS_2);
                    TSGrowableArray<CImVector> received; device.DeviceReadPixels(rect,received);
                    unsigned darker=0;
                    for(unsigned i=0;i<received.Count();++i) {
                        if(received[i].a!=image[i].a) throw std::runtime_error("Layered model alpha mismatch mode "+std::to_string(expectedShadow)+" pixel "+std::to_string(i)+" got "+std::to_string(received[i].a)+" expected "+std::to_string(image[i].a));
                        darker+=unsigned(received[i].r)+received[i].g+received[i].b+2 < unsigned(image[i].r)+image[i].g+image[i].b;
                    }
                    if(expectedShadow && !darker) throw std::runtime_error("Generated shadow did not reach the original layered materials");
                    std::fprintf(stderr,"Original layered model %u sequence %u %s shadow mode %u: %u dual-texture receivers (%u combined), %u darker pixels, unchanged alpha coverage\n",
                        displayID,pose.sequenceID,comparison ? "PCF":"R32F",expectedShadow,receivers,combinedReceivers,darker);
                    device.ScenePresent();
                }
            }
            CShaderEffect::s_usePcfFiltering=0;
            for(unsigned band=0;band<3;++band) CShadowCache::s_extShadowBands[band].m_textures[0]=shadowTarget.owner;
            if(displayID==110) {
                // Explicit model fade and liquid plane exercise the original M2
                // above/below-water selection. No batch or shader is forced.
                struct RestoreLiquid {
                    CM2Model* model; float alpha, baseAlpha; decltype(CM2Model::m_lightingCallback) lighting;
                    uint32_t cacheFlags=M2GetCacheFlags(); CGxDeviceMetal& device;
                    ~RestoreLiquid() { model->m_alpha=alpha; model->m_baseAlpha=baseAlpha; model->m_lightingCallback=lighting; M2SetCacheFlags(cacheFlags); device.XformSetProjection(C44Matrix(1)); }
                } restoreLiquid{layeredModel,layeredModel->m_alpha,layeredModel->m_baseAlpha,layeredModel->m_lightingCallback,M2GetCacheFlags(),device};
                CShadowCache::s_shadowMode=SHADOW_LOWEST;
                layeredModel->m_baseAlpha=.65f;
                C44Matrix projection(1); projection.a0=.875f; projection.b1=.75f;
                device.XformSetProjection(projection);
                // Original particles are assigned as whole billboards to a pass;
                // this comparison isolates the mesh that uses liquid clipping.
                auto prepare=[&] { layeredModel->m_fFlags|=8; layeredModel->SetAnimating(1); layeredScene->AdvanceTime(0); layeredScene->Animate(C3Vector(0,0,0)); };
                prepare(); device.BeginFrame(pass);
                layeredScene->Draw(M2PASS_0); layeredScene->Draw(M2PASS_1); layeredScene->Draw(M2PASS_2);
                TSGrowableArray<CImVector> baseline; device.DeviceReadPixels(rect,baseline); device.ScenePresent();
                prepare(); device.BeginFrame(pass);
                layeredScene->Draw(M2PASS_0); layeredScene->Draw(M2PASS_1); layeredScene->Draw(M2PASS_2);
                TSGrowableArray<CImVector> repeated; device.DeviceReadPixels(rect,repeated); device.ScenePresent();
                for(unsigned i=0;i<baseline.Count();++i) if(std::memcmp(&baseline[i],&repeated[i],sizeof(CImVector)))
                    throw std::runtime_error("Unclipped water mesh changed before plane selection at pixel "+std::to_string(i));
                M2SetCacheFlags(M2GetCacheFlags()|2);
                layeredModel->m_lightingCallback=[](CM2Model*,CM2Lighting* lighting,void*) {
                    lighting->AddAmbient(C3Vector(.4f,.4f,.4f)); lighting->AddDiffuse(C3Vector(.6f,.6f,.6f),C3Vector(0,0,-1));
                    lighting->SetFog(C3Vector(.1f,.2f,.3f),1,20);
                    lighting->m_flags|=0x60; lighting->m_liquidPlane.n={0,1,0}; lighting->m_liquidPlane.d=-.125f;
                };
                prepare(); unsigned clipped=0;
                for(unsigned i=0;i<layeredScene->m_elements.Count();++i) clipped+=(layeredScene->m_elements[i].m_flags&2)!=0;
                if(!clipped || !layeredScene->m_passElements[1].Count() || !layeredScene->m_passElements[2].Count())
                    throw std::runtime_error("Original water-crossing scene did not select both clipped passes");
                device.BeginFrame(pass); layeredScene->Draw(M2PASS_1);
                TSGrowableArray<CImVector> above,combined,below; device.DeviceReadPixels(rect,above);
                layeredScene->Draw(M2PASS_2); device.DeviceReadPixels(rect,combined); device.ScenePresent();
                device.BeginFrame(pass); layeredScene->Draw(M2PASS_2); device.DeviceReadPixels(rect,below); device.ScenePresent();
                unsigned upper=0,lower=0,rgbDifferences=0,maxRGBDifference=0;
                for(unsigned i=0;i<baseline.Count();++i) {
                    const bool a=above[i].a!=0,b=below[i].a!=0,full=baseline[i].a!=0;
                    upper+=a; lower+=b;
                    const float y=1-(float(i/128)+.5f)/64;
                    if((a && y<.09375f) || (b && y>.09375f) || (a && b) || (a||b)!=full)
                        throw std::runtime_error("Original liquid-plane split mismatch pixel "+std::to_string(i)+" y "+std::to_string(y)+" above/below/full "+std::to_string(above[i].a)+"/"+std::to_string(below[i].a)+"/"+std::to_string(baseline[i].a));
                    const uint8_t* wanted=reinterpret_cast<const uint8_t*>(&baseline[i]);
                    const uint8_t* actual=reinterpret_cast<const uint8_t*>(&combined[i]);
                    const uint8_t* separate=reinterpret_cast<const uint8_t*>(a ? &above[i]:&below[i]);
                    for(unsigned channel=0;channel<4;++channel)
                        if(actual[channel]!=separate[channel]) throw std::runtime_error("Water-crossing passes changed RGBA when composed in one frame");
                    if(combined[i].a!=baseline[i].a) throw std::runtime_error("Water-crossing passes changed model alpha");
                    // Clipping changes rasterization at the generated edges.
                    // Record the uncut RGB comparison separately: it is not a
                    // Windows reference or a promise of identical derivatives.
                    unsigned difference=0;
                    for(unsigned channel=0;channel<3;++channel) difference=std::max(difference,unsigned(std::abs(int(actual[channel])-int(wanted[channel]))));
                    rgbDifferences+=difference>2; maxRGBDifference=std::max(maxRGBDifference,difference);
                }
                if(!upper || !lower) throw std::runtime_error("Original liquid plane did not split visible model geometry");
                std::fprintf(stderr,"Original water-crossing M2 scene: %u clipped batches, %u above / %u below pixels, exact disjoint coverage/alpha and independent-pass RGBA; uncut reference RGB >2 at %u pixels, maximum %u/255\n",clipped,upper,lower,rgbDifferences,maxRGBDifference);
            }
        }
    }
    {
        // Self-contained original assets exercise the newly accepted coordinate
        // families through the original batch/effect selection and texture loader.
        struct CoordinateFixture { const char* path; GxMetal::ModelVertexEffect family; };
        for (const auto fixture : {CoordinateFixture{"Creature\\FrostLord\\FrostLord.m2",GxMetal::ModelVertexEffect::DiffuseSecondUV},
                 {"Creature\\CelestialHorse\\CelestialHorse.m2",GxMetal::ModelVertexEffect::DiffuseDualReflection},
                 {"World\\Goober\\G_ImpBottle.m2",GxMetal::ModelVertexEffect::DiffuseReflectionUV}}) {
            auto* coordinateScene=M2CreateScene();
            if(!coordinateScene) throw std::runtime_error("Coordinate model scene creation failed");
            Release releaseCoordinate{coordinateScene};
            auto* subject=coordinateScene->CreateModel(fixture.path,0);
            if(!subject || !subject->IsLoaded(1,0) || !subject->IsDrawable(1,0))
                throw std::runtime_error(std::string("Coordinate model resources failed to load: ")+fixture.path);
            CGxShader* expected[90]{};
            struct ReleaseShaders {
                CGxDeviceMetal& device; CGxShader** shaders;
                ~ReleaseShaders() { for(unsigned i=0;i<90;++i) device.ShaderDestroy(shaders[i]); }
            } releaseShaders{device,expected};
            device.ShaderCreate(expected,GxSh_Vertex,"Shaders\\Vertex",GxMetal::ModelVertexFamily(fixture.family),90);
            const auto& data=*subject->m_shared->m_data;
            const M2Bounds* bounds=&data.m_bounds;
            // Ahune's global radius includes distant effect envelopes; frame
            // this stand-pose fixture with its authored sequence bounds.
            for(unsigned i=0;i<data.m_sequences.m_count;++i)
                if(!data.m_sequences[i].m_id && !data.m_sequences[i].m_variationIndex) { bounds=&data.m_sequences[i].m_bounds; break; }
            C44Matrix world(1); world.RotateAroundX(-.65f); world.Scale(.8f/bounds->m_radius);
            world.Translate((bounds->m_extent.b+bounds->m_extent.t)*-.5f);
            subject->SetWorldTransform(world); subject->m_lightingCallback=visible->m_lightingCallback;
            subject->SetBoneSequence(0xFFFFFFFF,0,0xFFFFFFFF,0,1,0,1);
            subject->m_fFlags|=0x10008; subject->SetAnimating(1);
            device.XformSetView(C44Matrix(1)); device.XformSetProjection(C44Matrix(1));
            coordinateScene->AdvanceTime(137); coordinateScene->Animate(C3Vector(0,0,0));
            unsigned matching=0;
            for(unsigned i=0;i<coordinateScene->m_elements.Count();++i) {
                const auto& e=coordinateScene->m_elements[i];
                if(e.m_type<=2 && e.m_model==subject && e.m_effect && e.m_vertexPermute<90 &&
                    e.m_effect->m_vertexShaders[e.m_vertexPermute]==expected[e.m_vertexPermute]) ++matching;
            }
            if(!matching) throw std::runtime_error(std::string("Original model did not select coordinate family: ")+fixture.path);
            device.BeginFrame(pass);
            coordinateScene->Draw(M2PASS_0); coordinateScene->Draw(M2PASS_1); coordinateScene->Draw(M2PASS_2);
            TSGrowableArray<CImVector> image; device.DeviceReadPixels(rect,image);
            unsigned coverage=0;
            for(unsigned i=0;i<image.Count();++i) coverage+=image[i].r || image[i].g || image[i].b;
            device.ScenePresent();
            if(coverage<16) throw std::runtime_error(std::string("Coordinate model produced no visible geometry: ")+fixture.path);
            std::fprintf(stderr,"Original %s: %u %s batches, %u visible pixels through original scene/material/texture paths\n",fixture.path,matching,GxMetal::ModelVertexFamily(fixture.family),coverage);
        }
    }
    {
        // Exercise authored projected batches through CM2SceneRender. The
        // bounded planar receiver is an explicit fixture, not a world query.
        struct ShadowCleanup {
            ~ShadowCleanup() { ShadowDestroy(); }
        } shadowCleanup;
        ShadowInit();
        struct ProjectedFixture { const char* name; float zoom; };
        for (const auto fixture : {ProjectedFixture{"Spells\\AcidBurn.m2",1}, {"Spells\\Blizzard_Spawn.m2",1},
                                  {"Spells\\Consecration_Impact_Base.m2",1}, {"Spells\\Astral_Recall_Impact_Base.m2",8}}) {
            const char* name=fixture.name;
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
                    // Same normalized bias as World::ProjectTextureCallback
                    // and ProjectTex2dDrawShaderEffect; retain the planar fixture.
                    const float oldOffset=device.m_appRenderStates[GxRs_PolygonOffset].m_value.m_data.f[0];
                    GxRsSet(GxRs_PolygonOffset,(.4f+.4f)*3.0518044e-05f);
                    GxDraw(&batch, 0);
                    GxRsSet(GxRs_PolygonOffset,oldOffset);
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
            // Frame the small Hearthstone ground ring more closely without
            // changing the model, its animation or its clip-space depth.
            C44Matrix projection(1); projection.a0=projection.b1=fixture.zoom;
            device.XformSetProjection(projection);
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
            device.XformSetProjection(C44Matrix(1));
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
