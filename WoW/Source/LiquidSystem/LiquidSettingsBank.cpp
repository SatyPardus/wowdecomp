#include "Liquid.h"
#include "Engine/Source/Services/SysMessage.h"
#include "WoW/Source/DB/WowClientDB.h"
#include "WoW/Source/DB/Rec/LiquidTypeRec.h"
#include "WoW/Source/DB/Rec/LiquidMaterialRec.h"

// STATIC: 0x00D43B00
CDataAllocator Liquid::s_settingsAllocator(sizeof(Liquid::CMaterialSettings), 0x20);

// STATIC: 0x00D43B14
TSGrowableArray<Liquid::CMaterialSettings*> Liquid::CSettingsBank::s_settings;

// VERIFY: 1d70e7aa manual
// OFFSET: 0x008A2380
void Liquid::CSettingsBank::Destroy() {
    if (!--Liquid::s_settingsBankRefCount) {
        for (uint32_t i = 0; i < Liquid::CSettingsBank::s_settings.Count(); i++) {
            if (Liquid::CSettingsBank::s_settings[i]) {
                Liquid::CSettingsBank::s_settings[i]->Destroy();

                Liquid::CMaterialSettings* settings = Liquid::CSettingsBank::s_settings[i];
                settings->~CMaterialSettings();
                Liquid::s_settingsAllocator.PutData(settings, nullptr, 0);
            }
        }

        Liquid::CSettingsBank::s_settings.SetCount(0);
    }

    Liquid::s_settingsAllocator.Clear(".?AVCMaterialSettings@Liquid@@", SLOG_OBJECT, 1);
}

// VERIFY: 2a7eaf1c manual
// OFFSET: 0x008A2780
void Liquid::CSettingsBank::ResetTextures() {
    for (uint32_t i = 0; i < Liquid::CSettingsBank::s_settings.Count(); i++) {
        if (Liquid::CSettingsBank::s_settings[i]) {
            Liquid::CSettingsBank::s_settings[i]->Destroy();
            Liquid::CSettingsBank::s_settings[i]->InitTexture();
        }
    }
}

// VERIFY: b281ff61 manual
// OFFSET: 0x008A27C0
bool Liquid::CSettingsBank::UpdateRecord(Liquid::CMaterialSettings* settings, uint32_t liquidType) {
    settings->Destroy();

    LiquidTypeRec* typeRec = Liquid::s_liquidTypeDB->GetRecord(liquidType);

    if (!typeRec) {
        return false;
    }

    LiquidMaterialRec* materialRec = Liquid::s_liquidMaterialDB->GetRecord(typeRec->m_materialID);

    if (!materialRec) {
        return false;
    }

    for (uint32_t i = 0; i < 6; i++) {
        const char* src = typeRec->m_texture[i];
        char* dst = settings->m_textureNames[i];

        while ((*dst++ = *src++) != 0) {
        }
    }

    settings->m_colors[0] = *reinterpret_cast<CImVector*>(&typeRec->m_color[0]);
    settings->m_colors[1] = *reinterpret_cast<CImVector*>(&typeRec->m_color[1]);
    settings->m_ints[0] = typeRec->m_int[0];
    settings->m_ints[1] = typeRec->m_int[1];
    settings->m_ints[2] = typeRec->m_int[2];
    settings->m_ints[3] = typeRec->m_int[3];

    for (uint32_t i = 0; i < 18; i++) {
        settings->m_floats[i] = typeRec->m_float[i];
    }

    settings->m_transparent = materialRec->m_flags & 0x1;
    settings->InitTexture();

    return true;
}

// VERIFY: f18ac9d9 auto
// OFFSET: 0x008A28F0
Liquid::CMaterialSettings* Liquid::CSettingsBank::GetSettings(uint32_t liquidType) {
    Liquid::CMaterialSettings* settings;

    while (true) {
        if (liquidType < Liquid::CSettingsBank::s_settings.Count() && Liquid::CSettingsBank::s_settings.m_data[liquidType]) {
            return Liquid::CSettingsBank::s_settings.m_data[liquidType];
        }

        void* mem = Liquid::s_settingsAllocator.GetData(0, ".?AVCMaterialSettings@Liquid@@", SLOG_OBJECT);
        settings = mem ? new (mem) Liquid::CMaterialSettings() : nullptr;

        if (Liquid::CSettingsBank::UpdateRecord(settings, liquidType)) {
            break;
        }

        SysMsgPrintf(STATUS_ERROR, 2, "Settings Bank: Liquid type [%d] not found, defaulting to water!", liquidType);
        liquidType = 1;
    }

    Liquid::CSettingsBank::s_settings.GrowToFit(liquidType, 1);
    Liquid::CSettingsBank::s_settings.m_data[liquidType] = settings;

    return settings;
}
