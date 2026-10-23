#include "Liquid.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Services/SysMessage.h"
#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"

// STATIC: 0x00D43B24
TSGrowableArray<Liquid::IMaterial*> Liquid::CMaterialBank::s_materials;

// VERIFY: 8e2d4258 manual
// OFFSET: 0x008A1F50
void Liquid::CMaterialBank::Destroy() {
    for (uint32_t i = 0; i < Liquid::CMaterialBank::s_materials.Count(); i++) {
        if (Liquid::CMaterialBank::s_materials[i]) {
            Liquid::CMaterialBank::s_materials[i]->Release();
        }
    }

    Liquid::CMaterialBank::s_materials.SetCount(0);
}

// VERIFY: 5f91fa25 auto
// OFFSET: 0x008A4280
void Liquid::ClearMaterials() {
    Liquid::s_procWaterAllocator.Clear(".?AVCMaterialProcWater@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_procWaterFFPAllocator.Clear(".?AVCMaterialProcWaterFFP@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_waterAllocator.Clear(".?AVCMaterialWater@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_waterNoSpecAllocator.Clear(".?AVCMaterialWaterNoSpec@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_waterFFPAllocator.Clear(".?AVCMaterialWaterFFP@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_magmaAllocator.Clear(".?AVCMaterialMagma@Liquid@@", SLOG_OBJECT, 1);
    Liquid::s_magmaFFPAllocator.Clear(".?AVCMaterialMagmaFFP@Liquid@@", SLOG_OBJECT, 1);
}

// VERIFY: 85245160 auto
// OFFSET: 0x008A1FA0
Liquid::IMaterial* Liquid::CMaterialBank::GetMaterial(uint32_t liquidType) {
    LiquidTypeRec* typeRec;

    while (true) {
        if (!Liquid::s_materialBankRefCount) {
            return nullptr;
        }

        typeRec = Liquid::s_liquidTypeDB->GetRecord(liquidType);

        if (typeRec) {
            break;
        }

        SysMsgPrintf(SYSMSG_ERROR, 2, "Material Bank: Liquid type [%d] not found, defaulting to water!", liquidType);
        liquidType = 1;
    }

    uint32_t materialId = typeRec->m_materialID;

    if (materialId < Liquid::CMaterialBank::s_materials.Count() && Liquid::CMaterialBank::s_materials.m_data[materialId]) {
        return Liquid::CMaterialBank::s_materials.m_data[materialId];
    }

    bool vertexShaders = g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Vertex] >= 1;
    bool shaders = Liquid::s_shadersEnabled && vertexShaders && g_theGxDevicePtr->Caps().m_shaderTargets[GxSh_Pixel] >= 3;

    Liquid::IMaterial* material = nullptr;

    switch (materialId) {
        case 1:
            if (shaders) {
                if (Liquid::s_specularEnabled) {
                    material = Liquid::CMaterialWater::Create();
                } else {
                    material = Liquid::CMaterialWaterNoSpec::Create();
                }
            } else {
                material = Liquid::CMaterialWaterFFP::Create();
            }
            break;

        case 2:
            if (shaders) {
                material = Liquid::CMaterialMagma::Create();
            } else {
                material = Liquid::CMaterialMagmaFFP::Create();
            }
            break;

        case 3:
            if (shaders) {
                material = Liquid::CMaterialProcWater::Create();
            } else {
                material = Liquid::CMaterialProcWaterFFP::Create();
            }
            break;
    }

    Liquid::CMaterialBank::s_materials.GrowToFit(materialId, 1);
    Liquid::CMaterialBank::s_materials.m_data[materialId] = material;

    return material;
}
