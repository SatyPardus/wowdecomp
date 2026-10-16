#include "Liquid.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00D43B34
CDataAllocator Liquid::s_instanceAllocator(sizeof(Liquid::CInstance), 0x20);

// STATIC: 0x00B23F64
float Liquid::s_worldCoordScale = 1.0f;

// STATIC: 0x00B23F68
int32_t Liquid::s_specularEnabled = 1;

// STATIC: 0x00D439E8
IDatabase<LiquidMaterialRec>* Liquid::s_liquidMaterialDB;

// STATIC: 0x00D439EC
IDatabase<LiquidTypeRec>* Liquid::s_liquidTypeDB;

// STATIC: 0x00D439F0
char Liquid::s_shaderPath[256];

// STATIC: 0x00D43AF0
int32_t Liquid::s_shadersEnabled;

// STATIC: 0x00D43AF4
int32_t Liquid::s_useCameraRelativeCoords;

// STATIC: 0x00D43AF8
int32_t Liquid::s_settingsBankRefCount;

// STATIC: 0x00D43AFC
int32_t Liquid::s_materialBankRefCount;

// VERIFY: 96a38663 manual
// OFFSET: 0x008A32C0
Liquid::IEnvironment::~IEnvironment() {
}

// INLINED
Liquid::IGeomFactory::~IGeomFactory() {
}

// INLINED
Liquid::IWaveManager::~IWaveManager() {
}

// INLINED
Liquid::IMaterial::~IMaterial() {
}

// VERIFY: afda035c auto
// OFFSET: 0x008A1770
int32_t Liquid::Initialize(int32_t useCameraRelativeCoords, float worldCoordScale, int32_t shadersEnabled, const char* shaderPath, IDatabase<LiquidTypeRec>* liquidTypeDB, IDatabase<LiquidMaterialRec>* liquidMaterialDB) {
    SStrCopy(Liquid::s_shaderPath, shaderPath, sizeof(Liquid::s_shaderPath));
    Liquid::s_worldCoordScale = worldCoordScale;
    Liquid::s_settingsBankRefCount++;
    Liquid::s_materialBankRefCount++;
    Liquid::s_useCameraRelativeCoords = useCameraRelativeCoords;
    Liquid::s_shadersEnabled = shadersEnabled;
    Liquid::s_specularEnabled = 1;
    Liquid::s_liquidTypeDB = liquidTypeDB;
    Liquid::s_liquidMaterialDB = liquidMaterialDB;
    return 1;
}

// VERIFY: 6bde3ced manual
// OFFSET: 0x008A2400
void Liquid::Shutdown() {
    int32_t count = Liquid::s_materialBankRefCount--;
    Liquid::s_liquidTypeDB = nullptr;
    Liquid::s_liquidMaterialDB = nullptr;

    if (count == 1) {
        Liquid::CMaterialBank::Destroy();
        Liquid::s_shadersEnabled = 0;
    }

    Liquid::CSettingsBank::Destroy();
    Liquid::ClearMaterials();
    Liquid::s_instanceAllocator.Clear(".?AVCInstance@Liquid@@", SLOG_OBJECT, 1);
}

// VERIFY: 58e8bcb0 auto
// OFFSET: 0x008A1700
void Liquid::CInstance::AddRef() {
    this->m_refCount++;
}

// VERIFY: 01aeb3b0 manual
// OFFSET: 0x008A1710
void Liquid::CInstance::Purge() {
    this->m_geomFactory->Purge();
}

// VERIFY: 0c7c5584 auto
// OFFSET: 0x008A1720
void Liquid::SetShadersEnabled(int32_t enabled) {
    Liquid::s_shadersEnabled = enabled;
}

// VERIFY: 6952e6f0 auto
// OFFSET: 0x008A1730
void Liquid::SetSpecularEnabled(int32_t enabled) {
    Liquid::s_specularEnabled = enabled;
}

// VERIFY: e803d76a auto
// OFFSET: 0x008A1740
int32_t Liquid::UseCameraRelativeCoords() {
    return Liquid::s_useCameraRelativeCoords;
}

// VERIFY: d02f3d04 auto
// OFFSET: 0x008A1750
float Liquid::WorldCoordScale() {
    return Liquid::s_worldCoordScale;
}

// VERIFY: 2174e15a auto
// OFFSET: 0x008A1760
const char* Liquid::GetShaderSuffix() {
    return Liquid::s_shaderPath;
}

// VERIFY: 6413183d manual
// OFFSET: 0x008A1A40
Liquid::CInstance::CInstance() {
    this->m_material = nullptr;
    this->m_settings = nullptr;
    this->m_geomFactory = nullptr;
    this->m_waveManager = nullptr;
    this->m_environment = nullptr;
    this->m_bounds.r = 0.0f;
    this->m_queued = 0;
    this->m_refCount = 1;
}

// VERIFY: dfea0c3c manual
// OFFSET: 0x008A1AC0
void Liquid::CInstance::Release() {
    if (this->m_refCount-- == 1) {
        this->m_geomFactory->Release();
        this->m_environment->Release();

        if (this->m_waveManager) {
            this->m_waveManager->Release();
        }

        Liquid::s_instanceAllocator.PutData(this, nullptr, 0);
    }
}

// VERIFY: 43d6fc45 auto
// OFFSET: 0x008A1B00
Liquid::CInstance* Liquid::CInstance::Create() {
    void* mem = Liquid::s_instanceAllocator.GetData(0, ".?AVCInstance@Liquid@@", SLOG_OBJECT);
    return mem ? new (mem) Liquid::CInstance() : nullptr;
}
