#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/Gx/Gx.h"
#include <cmath>
#include <cstdio>
#include <stdexcept>

void CheckGameModels() {
    if (!g_animationDataDB.m_loaded) g_animationDataDB.Load(__FILE__, __LINE__);
    if (!ModelBlobLoad("World\\model.blob")) throw std::runtime_error("Original model bounds index failed to load");
    struct BlobCleanup { ~BlobCleanup() { ModelBlobDestroy(); } } blobCleanup;
    CShaderEffect::InitShaderSystem(1, 0);
    M2Initialize(8, 16);
    struct Shutdown { ~Shutdown() { M2Destroy(); CShaderEffectManager::DestroyAllEffects(); } } shutdown;
    auto* scene = M2CreateScene();
    if (!scene) throw std::runtime_error("M2 scene creation failed");
    struct Release { CM2Scene* scene; ~Release() { scene->Release(); } } release{scene};
    auto* model = scene->CreateModel("Cameras\\FlyByDwarf.m2", 0);
    if (!model || !model->IsLoaded(1, 0)) throw std::runtime_error("M2 camera instance failed to load");
    if (!model->m_bones || model->m_shared->m_data->m_cameras.m_count != 1)
        throw std::runtime_error("M2 camera instance is incomplete");
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
    auto* visible = scene->CreateModel("Spells\\ErrorCube.m2", 0);
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
    device.BeginFrame(pass);
    struct EndFrame { CGxDeviceMetal& device; ~EndFrame() { if (device.FrameCommand()) device.ScenePresent(); } } endFrame{device};
    GxXformSetViewport(0, 1, 0, 1, 0, 1);
    C44Matrix identity(1.0f); device.XformSetView(identity);
    device.XformSetProjection(identity);
    const auto& bounds = visible->m_shared->m_data->m_bounds;
    const auto center = (bounds.m_extent.b + bounds.m_extent.t) * 0.5f;
    const float scale = 0.8f / bounds.m_radius;
    C44Matrix world(1); world.Scale(scale); world.Translate(center * -scale);
    visible->SetWorldTransform(world);
    visible->m_lightingCallback = [](CM2Model*, CM2Lighting* lighting, void*) {
        lighting->AddAmbient(C3Vector(0.4f, 0.4f, 0.4f));
        lighting->AddDiffuse(C3Vector(0.6f, 0.6f, 0.6f), C3Vector(0, 0, -1));
        lighting->SetFog(C3Vector(0.1f, 0.2f, 0.3f), 1, 20);
    };
    visible->m_fFlags |= 8; // Same draw request as World::RenderModels.
    visible->SetAnimating(1); scene->AdvanceTime(16); scene->Animate(C3Vector(0, 0, 0));
    if (!scene->m_elements.Count()) throw std::runtime_error("M2 scene produced no render elements");
    for (unsigned i = 0; i < scene->m_elements.Count(); ++i) {
        const auto& e = scene->m_elements[i];
        std::fprintf(stderr, "M2 native draw: vertex %u pixel %u, %u indices\n", e.m_vertexPermute, e.m_pixelPermute, e.m_skinSection->m_indexCount);
    }
    scene->Draw(M2PASS_0); scene->Draw(M2PASS_1); scene->Draw(M2PASS_2);
    CiRect rect(0, 0, 128, 128); TSGrowableArray<CImVector> pixels;
    device.DeviceReadPixels(rect, pixels);
    unsigned covered = 0;
    for (uint32_t i = 0; i < pixels.Count(); ++i) if (pixels[i].a) ++covered;
    if (covered < 256) throw std::runtime_error("M2 scene did not produce visible GPU pixels");
    std::fprintf(stderr, "M2 native scene: %u covered pixels\n", covered);
}
