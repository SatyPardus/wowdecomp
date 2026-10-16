#include "CharacterSelection.h"
#include <cstring>
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/Event/Event.h"
#include "Engine/Source/FFX/FFX.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Model2/M2Light.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Model2/M2Shared.h"
#include "Engine/Source/Os/OsFile.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureFamilyRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/DB/Rec/SpellVisualKitRec.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Glue/CGlueLoading.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CSimpleModelFFX.h"
#include "WoW/Source/Glue/CharacterSelectionDisplay.h"
#include "WoW/Source/Glue/ComponentData.h"
#include "WoW/Source/Glue/RealmList.h"
#include "WoW/Source/LoadingScreen.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Ui/AddOns.h"
#include "WoW/Source/WorldClient/DayNight__DNInfo.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x009F3CD0
const int32_t g_scriptSexIndex[3] = { 2, 3, 1 };

// STATIC: 0x00AC441C
static float s_characterGlow = 0.4f;

// STATIC: 0x00AC4420
static float s_ghostGlow = 0.15f;

// STATIC: 0x00AC436C
int32_t CCharacterSelection::s_selectionIndex = -1;

// STATIC: 0x00B6B1FC
CSimpleModelFFX* CCharacterSelection::m_modelFrame;

// STATIC: 0x00B6B200
int32_t CCharacterSelection::s_characterCount;

// STATIC: 0x00B6B204
float CCharacterSelection::s_charFacing;

// STATIC: 0x00B6B208
uint32_t CCharacterSelection::s_raceRestrictions[10];

// STATIC: 0x00B6B238
TSGrowableArray<CharacterSelectionDisplay> CCharacterSelection::s_characterList;

// VERIFY: ef583a6e manual
// OFFSET: 0x005EEB70
void CCharacterSelection::Initialize() {
}

// VERIFY: cead22db auto
// OFFSET: 0x004E38F0
void CCharacterSelection::Shutdown() {
    LoadingScreenMiniDisable();

    if (CCharacterComponent::m_activePlayerComponent) {
        CCharacterComponent::FreeComponent(CCharacterComponent::m_activePlayerComponent);
        CCharacterComponent::m_activePlayerComponent = nullptr;
    }

    for (uint32_t i = 0; i < CCharacterSelection::s_characterList.Count(); i++) {
        if (!CCharacterSelection::s_characterList[i].m_component) {
            continue;
        }

        CM2Model* model = CCharacterSelection::s_characterList[i].m_component->m_data.m_model;

        if (!model || i != static_cast<uint32_t>(CGlueMgr::s_displayedCharacterIndex)) {
            continue;
        }

        if (CCharacterSelection::s_characterList[i].m_characterInfo.m_classID == 3) {
            CCharacterComponent::RemoveHandItem(model, INVSLOT_RANGED, SHEATHE_NONE, false);
            CCharacterSelection::s_characterList[i].m_component->RemoveItem(ITEMSLOT_11);
        }

        CCharacterComponent::RemoveVisuals(model);
        model->AttachToSceneRecursive(World::s_m2Scene);
        CCharacterComponent::m_activePlayerComponent = CCharacterSelection::s_characterList[i].m_component;
        CGPlayer_C::s_displayId = GetDisplayIdFromRaceAndGender(CCharacterSelection::s_characterList[i].m_characterInfo.m_raceID, CCharacterSelection::s_characterList[i].m_characterInfo.m_sexID);
        CCharacterSelection::s_characterList[i].m_component = nullptr;
    }

    CCharacterSelection::s_characterList.Clear();
    CharacterSelectionDisplay::SetSelectedCharacterInfo(nullptr);
}

// VERIFY: d8b19604 auto
// OFFSET: 0x004E2C30
void CGlueMgr::ProcessAttachment(CM2Model* model, int32_t effectID, uint32_t attachID) {
    if (!model->HasAttachment(attachID)) {
        return;
    }

    SpellVisualEffectNameRec* effectRec = g_spellVisualEffectNameDB.GetRecord(effectID);

    if (!effectRec) {
        return;
    }

    CM2Model* effect = CCharacterSelection::m_modelFrame->GetScene()->CreateModel(effectRec->m_fileName, 0);

    if (!effect) {
        return;
    }

    C44Matrix matrix;
    matrix.Scale(effectRec->m_scale);
    effect->m_fFlags |= 0x8000;
    effect->m_worldTransform = matrix;
    effect->AttachToParent(model, attachID, nullptr, 0);
    effect->Release();
}

// VERIFY: 01cf543c manual
// OFFSET: 0x004E2CE0
void CGlueMgr::AttachCharacterSelectVisuals(CM2Model* model) {
    SpellVisualKitRec* kitRec = g_spellVisualKitDB.GetRecord(989);

    if (!kitRec) {
        return;
    }

    CGlueMgr::ProcessAttachment(model, kitRec->m_headEffect, 20);
    CGlueMgr::ProcessAttachment(model, kitRec->m_chestEffect, 34);
    CGlueMgr::ProcessAttachment(model, kitRec->m_baseEffect, 19);
    CGlueMgr::ProcessAttachment(model, kitRec->m_leftHandEffect, 21);
    CGlueMgr::ProcessAttachment(model, kitRec->m_rightHandEffect, 22);
    CGlueMgr::ProcessAttachment(model, kitRec->m_breathEffect, 17);
    CGlueMgr::ProcessAttachment(model, kitRec->m_specialEffect[0], 23);
    CGlueMgr::ProcessAttachment(model, kitRec->m_specialEffect[1], 24);
    CGlueMgr::ProcessAttachment(model, kitRec->m_specialEffect[2], 25);

    for (uint32_t i = 0; i < 4; i++) {
        switch (kitRec->m_charProc[i]) {
            case 1: {
                CImVector color;
                *reinterpret_cast<uint32_t*>(&color) = static_cast<uint32_t>(kitRec->m_charParamZero[i]) | 0xFF000000;
                model->m_diffuseTintR = color.r * (1.0f / 255.0f);
                model->m_diffuseTintG = color.g * (1.0f / 255.0f);
                model->m_diffuseTintB = color.b * (1.0f / 255.0f);
                break;
            }

            case 14: {
                float alpha = kitRec->m_charParamZero[i];

                if (alpha < 0.0f) {
                    model->m_fadeAlpha = 0.0f;
                } else {
                    model->m_fadeAlpha = alpha > 1.0f ? 1.0f : alpha;
                }

                break;
            }

            default: {
                break;
            }
        }
    }
}

// VERIFY: 61c6f157 auto
// OFFSET: 0x004E0110
void CCharacterSelection::ModelLoadedCallback(CM2Model* model, void* arg) {
    if (model->HasEvent('TTW$') && model->HasEvent('BTW$')) {
        model->m_fFlags |= 0x20;
        model->m_drawCallback = &CDrawBowString::GlueDrawBowStringCB;
        model->m_drawArg = arg;
    }
}

// VERIFY: 91e60469 auto
// OFFSET: 0x004E27C0
void CCharacterSelection::GuildCallback(int32_t guildID, const WOWGUID& guid, void* param, bool success) {
    if (!guildID || !success) {
        return;
    }

    for (int32_t i = 0; i < static_cast<int32_t>(CCharacterSelection::s_characterList.Count()); i++) {
        if (CCharacterSelection::s_characterList[i].m_characterInfo.m_guildID != static_cast<uint32_t>(guildID)) {
            continue;
        }

        int32_t emblemStyle;
        int32_t emblemColor;
        int32_t borderStyle;
        int32_t borderColor;
        int32_t background;

        if (GuildGetGuildTabard(WOWGUID(), guildID, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
            CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
        }
    }
}

// VERIFY: 9fe08e4d auto
// OFFSET: 0x004E2870
void CCharacterSelection::UpdateCharacterName(WOWGUID guid, const char* name) {
    uint32_t index;

    for (index = 0; index < CCharacterSelection::s_characterList.Count(); index++) {
        if (CCharacterSelection::s_characterList[index].m_characterInfo.m_guid == guid) {
            break;
        }
    }

    if (index >= CCharacterSelection::s_characterList.Count()) {
        return;
    }

    char oldPath[260];
    char newPath[260];
    SStrPrintf(oldPath, sizeof(oldPath), "WTF\\Account\\%s\\%s\\%s", CGlueMgr::m_accountName, ClientServices::GetSelectedRealmName(), CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
    SStrPrintf(newPath, sizeof(newPath), "WTF\\Account\\%s\\%s\\%s", CGlueMgr::m_accountName, ClientServices::GetSelectedRealmName(), name);
    OsMoveFile(oldPath, newPath);
    SStrPrintf(oldPath, sizeof(oldPath), "WTF\\Account\\%s\\%s", CGlueMgr::m_accountName, CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
    SStrPrintf(newPath, sizeof(newPath), "WTF\\Account\\%s\\%s", CGlueMgr::m_accountName, name);
    OsMoveFile(oldPath, newPath);
    ClearAddOnEnableState(CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
    LoadAddOnEnableState(name);
    SStrCopy(CCharacterSelection::s_characterList[index].m_characterInfo.m_name, name, sizeof(CCharacterSelection::s_characterList[index].m_characterInfo.m_name));
    CCharacterSelection::s_characterList[index].m_characterInfo.m_flags &= ~0x4000;
    CCharacterSelection::s_characterList[index].m_characterInfo.m_flags &= ~0x2000000;
    FrameScript_SignalEvent(GLUE_EVENT_CHARACTER_LIST_UPDATE, nullptr);
}

// VERIFY: 945f4dbd auto
// OFFSET: 0x004E29E0
void CCharacterSelection::UpdateCharacterInfo(WOWGUID guid, const char* name, uint8_t sexID, uint8_t skinColorID, uint8_t faceID, uint8_t hairStyleID, uint8_t hairColorID, uint8_t facialHairStyleID, uint8_t raceID) {
    uint32_t index;

    for (index = 0; index < CCharacterSelection::s_characterList.Count(); index++) {
        if (CCharacterSelection::s_characterList[index].m_characterInfo.m_guid == guid) {
            break;
        }
    }

    if (index >= CCharacterSelection::s_characterList.Count()) {
        return;
    }

    if (SStrCmpI(name, CCharacterSelection::s_characterList[index].m_characterInfo.m_name, STORM_MAX_STR)) {
        char oldPath[260];
        char newPath[260];
        SStrPrintf(oldPath, sizeof(oldPath), "WTF\\Account\\%s\\%s\\%s", CGlueMgr::m_accountName, ClientServices::GetSelectedRealmName(), CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
        SStrPrintf(newPath, sizeof(newPath), "WTF\\Account\\%s\\%s\\%s", CGlueMgr::m_accountName, ClientServices::GetSelectedRealmName(), name);
        OsMoveFile(oldPath, newPath);
        SStrPrintf(oldPath, sizeof(oldPath), "WTF\\Account\\%s\\%s", CGlueMgr::m_accountName, CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
        SStrPrintf(newPath, sizeof(newPath), "WTF\\Account\\%s\\%s", CGlueMgr::m_accountName, name);
        OsMoveFile(oldPath, newPath);
        ClearAddOnEnableState(CCharacterSelection::s_characterList[index].m_characterInfo.m_name);
        LoadAddOnEnableState(name);
        SStrCopy(CCharacterSelection::s_characterList[index].m_characterInfo.m_name, name, sizeof(CCharacterSelection::s_characterList[index].m_characterInfo.m_name));
    }

    CHARACTER_INFO& info = CCharacterSelection::s_characterList[index].m_characterInfo;
    info.m_sexID = sexID;
    info.m_skinColorID = skinColorID;
    info.m_faceID = faceID;
    info.m_hairStyleID = hairStyleID;
    info.m_hairColorID = hairColorID;
    info.m_facialHairStyleID = facialHairStyleID;

    if (raceID) {
        info.m_raceID = raceID;
    }

    info.m_customizeFlags &= 0xFFEEFFFE;
    FrameScript_SignalEvent(GLUE_EVENT_CHARACTER_LIST_UPDATE, nullptr);
}

// VERIFY: db6735fe manual
// OFFSET: 0x004E2BD0
void CCharacterSelection::UpdateCharacterNameDeclension(WOWGUID guid) {
    for (uint32_t index = 0; index < CCharacterSelection::s_characterList.Count(); index++) {
        if (CCharacterSelection::s_characterList[index].m_characterInfo.m_guid == guid) {
            CCharacterSelection::s_characterList[index].m_characterInfo.m_flags |= 0x2000000;
            return;
        }
    }
}

// VERIFY: 805a43b3 auto
// OFFSET: 0x004E2EF0
CharacterSelectionDisplay* CCharacterSelection::GetCharacterDisplay(uint32_t index) {
    if (index < CCharacterSelection::s_characterList.Count()) {
        return &CCharacterSelection::s_characterList[index];
    }

    return nullptr;
}

// VERIFY: 68f0d05a auto
// OFFSET: 0x004E2F10
int32_t CCharacterSelection::Idle(const void* data, void* param) {
    if (CCharacterSelection::s_selectionIndex < 0 || CCharacterSelection::s_selectionIndex >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        CGlueLoading::Idle(nullptr);
        return 1;
    }

    CGlueLoading::Idle(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component);
    return 1;
}

// VERIFY: f62a960c auto
// OFFSET: 0x004E3120
void CCharacterSelection::OnHide() {
    EventUnregister(EVENT_ID_IDLE, &CCharacterSelection::Idle);
    LoadingScreenMiniDisable();
}

// VERIFY: 61faf5ca auto
// OFFSET: 0x004E3A20
void CCharacterSelection::GenericLightingCallback(CM2Model* model, CM2Lighting* lighting, void* arg) {
    CM2LightArray* lights = static_cast<CM2LightArray*>(arg);
    CharacterSelectionDisplay* display = CCharacterSelection::s_selectionIndex < 0 || CCharacterSelection::s_selectionIndex >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count()) ? nullptr : &CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];

    if (!lights->unk_01BC && display && (display->m_characterInfo.m_flags & 0x2000)) {
        lights = &lights[1];

        if (!lights->m_hasCustomLight) {
            CAaSphere sphere;
            sphere.c = model->GetWorldPosition();
            sphere.r = 0.0f;
            lighting->Initialize(model->m_scene, sphere);

            LightParamsRec* paramsRec = g_lightParamsDB.GetRecord(3);
            CImVector direct = DayNight::LoadLightIntBand(0, paramsRec, 0);
            CImVector ambient = DayNight::LoadLightIntBand(0, paramsRec, 1);

            CM2Light light;
            light.SetLightType(M2LIGHT_DIRECT);
            light.m_ambColor = C3Vector(ambient);
            light.m_dirColor = C3Vector(direct);
            light.SetDirection(C3Vector(0.0f, 0.0f, -1.0f));
            light.SetVisible(1);
            lighting->AddLight(&light);

            return;
        }
    }

    if (lights->m_hasCustomLight) {
        lighting->Reset();

        for (uint32_t i = 0; i < lights->m_lightCount; i++) {
            lighting->AddLight(&lights->m_lights[i]);
        }
    }

    if (lights->unk_01B5) {
        CSimpleModelFFX* frame = lights->m_modelFrame;

        if (frame->m_flags & 0x1) {
            lighting->SetFog(C3Vector(frame->m_fogColor), frame->m_fogNear, frame->m_fogFar);
        }
    }
}

// VERIFY: ca485bb6 auto
// OFFSET: 0x004E3C10
void CCharacterSelection::SetBackgroundModel(const char* modelName) {
    if (!CCharacterSelection::m_modelFrame || !modelName || !*modelName) {
        return;
    }

    if (CCharacterSelection::m_modelFrame->m_model && !SStrCmpI(modelName, CCharacterSelection::m_modelFrame->m_model->m_shared->m_fileName, STORM_MAX_STR)) {
        return;
    }

    CCharacterSelection::m_modelFrame->SetModel(modelName);
    CCharacterSelection::m_modelFrame->m_lightArray1[0].unk_01B5 = 1;

    CM2Model* model = CCharacterSelection::m_modelFrame->m_model;

    if (model) {
        model->m_lightingCallback = &CCharacterSelection::GenericLightingCallback;
        model->m_lightingArg = CCharacterSelection::m_modelFrame->m_lightArray1;
        model->IsDrawable(1, 1);
    }
}

// VERIFY: fcc3bd5f auto
// OFFSET: 0x004E3CA0
void CCharacterSelection::EnumerateCharactersCallback(CHARACTER_INFO& info, void* param) {
    memcpy(&CCharacterSelection::s_characterList.New()->m_characterInfo, &info, sizeof(info));
    LoadAddOnEnableState(info.m_name);
}

// VERIFY: 3fe94d69 manual
// OFFSET: 0x004E3CD0
void CCharacterSelection::ShowCharacter() {
    if (CCharacterSelection::s_selectionIndex < 0 || CCharacterSelection::s_selectionIndex >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
        return;
    }

    CreatureDisplayInfoRec* displayRec = nullptr;
    CreatureModelDataRec* modelRec = nullptr;
    ComponentData data;

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(0);
    }

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(1);
    }

    CCharacterSelection::s_charFacing = 0.0f;

    int32_t newComponent = 0;

    if (!CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component) {
        newComponent = 1;

        CreatureModelDataRec* playerRec = GetModelDataRec(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_raceID, CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_sexID);

        if (!playerRec || !playerRec->m_modelName) {
            return;
        }

        CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component = CCharacterComponent::AllocComponent();

        CHARACTER_INFO& info = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo;
        data.m_preferences.m_raceID = info.m_raceID;
        data.m_preferences.m_sexID = info.m_sexID;
        data.m_preferences.m_classId = info.m_classID;
        data.m_preferences.m_skinID = info.m_skinColorID;
        data.m_preferences.m_faceId = info.m_faceID;
        data.m_preferences.m_hairStyleId = info.m_hairStyleID;
        data.m_preferences.m_hairColorId = info.m_hairColorID;
        data.m_preferences.m_facialHairId = info.m_facialHairStyleID;
        data.m_model = CCharacterSelection::m_modelFrame->GetScene()->CreateModel(playerRec->m_modelName, 0);
        data.m_flags |= 0x2;

        CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->Init(&data, nullptr);

        CM2Model* characterModel = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->m_data.m_model;
        characterModel->m_lightingCallback = &CCharacterSelection::GenericLightingCallback;
        characterModel->m_lightingArg = CCharacterSelection::m_modelFrame->m_lightArray2;

        CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->m_data.m_model->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);

        for (uint32_t slot = 0; slot < 23; slot++) {
            int32_t displayID = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_inventoryItemDisplayID[slot];
            int32_t itemVisual = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_inventoryItemVisualID[slot];

            if (!displayID) {
                continue;
            }

            bool hunter = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_classID == 3;

            if (hunter) {
                if (slot == INVSLOT_MAINHAND || slot == INVSLOT_OFFHAND) {
                    continue;
                }
            } else if (slot == INVSLOT_RANGED) {
                continue;
            }

            uint32_t flags = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_flags;

            if ((flags & 0x400) && slot == INVSLOT_HEAD) {
                continue;
            }

            if ((flags & 0x800) && slot == INVSLOT_BACK) {
                continue;
            }

            ItemDisplayInfoRec itemRec;
            int32_t found = g_itemDisplayInfoDB.GetRecord(displayID, &itemRec);
            int32_t visual = found && g_itemVisualsDB.GetRecord(itemRec.m_itemVisual) ? 0 : itemVisual;

            if (slot == INVSLOT_MAINHAND || slot == INVSLOT_OFFHAND || (slot == INVSLOT_RANGED && hunter)) {
                if (found) {
                    GEOCOMPONENTLINKS link = CCharacterComponent::AddHandItem(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->m_data.m_model, &itemRec, static_cast<INVENTORY_SLOTS>(slot), SHEATHE_NONE, false, CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_inventoryItemType[slot] == INVTYPE_SHIELD, false, visual);

                    if (slot == INVSLOT_RANGED) {
                        if (CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_inventoryItemType[INVSLOT_RANGED] == INVTYPE_RANGED) {
                            for (CM2Model* attached = data.m_model->m_attachList; attached; attached = attached->m_attachNext) {
                                if (attached->m_attachmentId == static_cast<uint32_t>(link)) {
                                    attached->SetLoadedCallback(&CCharacterSelection::ModelLoadedCallback, nullptr);
                                    break;
                                }
                            }
                        }

                        continue;
                    }
                }
            } else if (static_cast<int32_t>(slot) >= 19 && static_cast<int32_t>(slot) <= 22) {
                if (*itemRec.m_modelName[0]) {
                    CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->AddItem(ITEMSLOT_11, &itemRec, 0);
                }
            } else {
                CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->AddItemBySlot(static_cast<INVENTORY_SLOTS>(slot), displayID, visual);
            }

            if (slot == INVSLOT_TABARD && found && (itemRec.m_flags & 0x1)) {
                int32_t emblemStyle;
                int32_t emblemColor;
                int32_t borderStyle;
                int32_t borderColor;
                int32_t background;
                CHARACTER_INFO& tabardInfo = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo;

                if (GuildGetGuildTabard(tabardInfo.m_guid, tabardInfo.m_guildID, &CCharacterSelection::GuildCallback, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
                    CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
                }
            }
        }

        if (CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_flags & 0x2000) {
            CGlueMgr::AttachCharacterSelectVisuals(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->m_data.m_model);
        }

        CharacterSelectionDisplay* display = &CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];

        if (display->m_characterInfo.m_petDisplayInfoID) {
            displayRec = g_creatureDisplayInfoDB.GetRecord(display->m_characterInfo.m_petDisplayInfoID);

            if (displayRec) {
                modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

                if (modelRec) {
                    if (display->m_petModel) {
                        display->m_petModel->Release();
                    }

                    CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel = CCharacterSelection::m_modelFrame->GetScene()->CreateModel(modelRec->m_modelName, 0);
                }
            }
        }

        CharacterSelectionDisplay* current = &CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];
        CM2Model* petModel = current->m_petModel;

        if (petModel) {
            float scale = modelRec->m_modelScale * displayRec->m_creatureModelScale;
            CreatureFamilyRec* familyRec = g_creatureFamilyDB.GetRecord(current->m_characterInfo.m_petCreatureFamilyID);

            if (familyRec) {
                int32_t petLevel = static_cast<int32_t>(current->m_characterInfo.m_petExperienceLevel);
                int32_t level = petLevel < familyRec->m_minScaleLevel ? 0 : petLevel - familyRec->m_minScaleLevel;
                int32_t range = familyRec->m_maxScaleLevel - familyRec->m_minScaleLevel;

                if (level > range) {
                    level = range;
                }

                scale = static_cast<float>(level) / range * (familyRec->m_maxScale - familyRec->m_minScale) + familyRec->m_minScale;
            }

            petModel->m_lightingCallback = &CCharacterSelection::GenericLightingCallback;
            petModel->m_lightingArg = CCharacterSelection::m_modelFrame->m_lightArray3;

            CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel->SetBoneSequence(0xFFFFFFFF, 0, 0xFFFFFFFF, 0, 1.0f, 1, 1);
            CCharacterComponent::ReplaceMonsterSkin(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel, displayRec, modelRec);
            C44Matrix matrix(scale);
            CM2Model* scaledModel = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel;
            scaledModel->m_fFlags |= 0x8000;
            scaledModel->m_worldTransform = matrix;
            CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petAlpha = displayRec->m_creatureModelAlpha * (1.0f / 255.0f);
            CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel->m_fadeAlpha = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petAlpha;
        }

        CCharacterSelection::s_characterCount++;
    }

    DayNight::DNInfo* info = DayNight::GetInfo();

    if (CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_characterInfo.m_flags & 0x2000) {
        FFX::SetEffect(CGlueMgr::m_deathEffect);
        info->m_bands.m_glow = s_ghostGlow;
    } else {
        FFX::SetEffect(CGlueMgr::m_glowEffect);
        info->m_bands.m_glow = s_characterGlow;
    }

    if (CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component->m_data.m_model->AttachToParent(CCharacterSelection::m_modelFrame->m_model, 0, nullptr, 0);
        CharacterSelectionDisplay::SetCharacterFacing(0.0f);

        if (CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel) {
            CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_petModel->AttachToParent(CCharacterSelection::m_modelFrame->m_model, 1, nullptr, 0);
        }
    }

    CGlueLoading::StartLoad(CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex].m_component, newComponent);
}

// VERIFY: eafa3254 manual
// OFFSET: 0x004E4610
void CCharacterSelection::UpdateCharacterList() {
    ClearAddOnEnableState(nullptr);
    CCharacterSelection::s_characterList.SetCount(0);

    CCharacterSelection::s_raceRestrictions[0] = 0;
    CCharacterSelection::s_raceRestrictions[1] = 0;
    CCharacterSelection::s_raceRestrictions[2] = 0;
    CCharacterSelection::s_raceRestrictions[3] = 0;
    CCharacterSelection::s_raceRestrictions[4] = 0;
    CCharacterSelection::s_raceRestrictions[5] = 0;
    CCharacterSelection::s_raceRestrictions[6] = 0;
    CCharacterSelection::s_raceRestrictions[7] = 0;
    CCharacterSelection::s_raceRestrictions[8] = 0;
    CCharacterSelection::s_raceRestrictions[9] = 0;

    ClientServices::Connection()->EnumerateCharacters(&CCharacterSelection::EnumerateCharactersCallback, nullptr);

    if (!CCharacterSelection::s_characterList.Count()) {
        CCharacterSelection::s_selectionIndex = 0;
        CCharacterSelection::ShowCharacter();
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_SELECTED_CHARACTER, "%d", CCharacterSelection::s_selectionIndex + 1);

        if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
            CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(0);
        }

        if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
            CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(1);
        }
    } else {
        CCharacterSelection::s_raceRestrictions[0] = ClientServices::Connection()->m_restrictHuman;
        CCharacterSelection::s_raceRestrictions[1] = ClientServices::Connection()->m_restrictDwarf;
        CCharacterSelection::s_raceRestrictions[2] = ClientServices::Connection()->m_restrictGnome;
        CCharacterSelection::s_raceRestrictions[3] = ClientServices::Connection()->m_restrictNightElf;
        CCharacterSelection::s_raceRestrictions[4] = ClientServices::Connection()->m_restrictDraenei;
        CCharacterSelection::s_raceRestrictions[5] = ClientServices::Connection()->m_restrictOrc;
        CCharacterSelection::s_raceRestrictions[6] = ClientServices::Connection()->m_restrictTroll;
        CCharacterSelection::s_raceRestrictions[7] = ClientServices::Connection()->m_restrictTauren;
        CCharacterSelection::s_raceRestrictions[8] = ClientServices::Connection()->m_restrictUndead;
        CCharacterSelection::s_raceRestrictions[9] = ClientServices::Connection()->m_restrictBloodElf;

        CRealmList::m_preferredCategory = 0;

        int32_t index = g_lastCharacterIndexVar->m_intValue < 0 ? 0 : g_lastCharacterIndexVar->m_intValue;

        if (index >= static_cast<int32_t>(CCharacterSelection::s_characterList.Count())) {
            index = 0;
        }

        CCharacterSelection::s_selectionIndex = index;
        CCharacterSelection::ShowCharacter();
        FrameScript_SignalEvent(GLUE_EVENT_UPDATE_SELECTED_CHARACTER, "%d", CCharacterSelection::s_selectionIndex + 1);
    }

    FrameScript_SignalEvent(GLUE_EVENT_CHARACTER_LIST_UPDATE, nullptr);
}

// VERIFY: 83731d6e auto
// OFFSET: 0x004E47F0
void CCharacterSelection::ClearCharacterList() {
    CCharacterSelection::s_characterList.SetCount(0);

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(0);
    }

    if (CCharacterSelection::m_modelFrame && CCharacterSelection::m_modelFrame->m_model) {
        CCharacterSelection::m_modelFrame->m_model->DetachAllChildrenById(1);
    }

    CCharacterSelection::s_selectionIndex = 0;
    CCharacterSelection::ShowCharacter();
    FrameScript_SignalEvent(GLUE_EVENT_UPDATE_SELECTED_CHARACTER, "%d", CCharacterSelection::s_selectionIndex + 1);

    if (CCharacterSelection::m_modelFrame) {
        CCharacterSelection::m_modelFrame->SetCameraByIndex(0);
    }

    FrameScript_SignalEvent(GLUE_EVENT_CHARACTER_LIST_UPDATE, nullptr);
}
