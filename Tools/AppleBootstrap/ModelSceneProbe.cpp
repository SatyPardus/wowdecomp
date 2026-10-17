#include "Engine/Source/Model2/M2Cache.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Services/Camera.h"
#include "Engine/Source/Services/CameraInt.h"
#include "Engine/Source/Services/ModelBlob.h"
#include "Engine/Source/ShaderEffect/ShaderEffect.h"
#include "Engine/Source/ShaderEffect/ShaderEffectManager.h"
#include <cmath>
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
}
