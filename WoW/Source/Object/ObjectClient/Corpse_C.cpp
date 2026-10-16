#include "Corpse_C.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"
#include "Engine/Source/Model2/M2Model.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "Engine/Source/Services/SysMessage.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/ChrRacesRec.h"
#include "WoW/Source/DB/Rec/CreatureDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/CreatureModelDataRec.h"
#include "WoW/Source/DB/Rec/ItemDisplayInfoRec.h"
#include "WoW/Source/DB/Rec/SpellVisualEffectNameRec.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Game/GameClient/ProjectTex.h"
#include "WoW/Source/Glue/ComponentData.h"
#include "WoW/Source/Object/ObjectClient/Effect_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/CClientObjCreate.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/UIUtil/PortraitButton.h"
#include "WoW/Source/WorldClient/World.h"

// VERIFY: 91aabd2d manual
// OFFSET: 0x00705950
static void GuildTabardCallback(int32_t guildID, const WOWGUID& guid, void* param, bool success) {
    WOWGUID* corpseGuid = static_cast<WOWGUID*>(param);
    CGCorpse_C* corpse = static_cast<CGCorpse_C*>(ClntObjMgrObjectPtr(*corpseGuid, TYPEMASK_CORPSE, __FILE__, __LINE__));

    if (corpse) {
        corpse->ApplyGuildColor(0);
    }

    delete corpseGuid;
}

// VERIFY: 9eb9d9be auto
// OFFSET: 0x0070CBA0
void CGCorpse_C::SetStorage(uint32_t* storage, uint32_t* saved) {
    this->CGObject_C::SetStorage(storage, saved);
    this->m_corpse = reinterpret_cast<CGCorpseData*>(storage + OBJECT_END);
    this->m_corpseMirror = saved + CGObject::TotalFieldsSaved();
}

// VERIFY: 9d2370ce manual
// OFFSET: 0x00706430
CGCorpse_C::CGCorpse_C(uint32_t time, const CClientObjCreate& create) : CGObject_C(time, create), m_passenger(create.m_move.m_status.m_transportGuid, create.m_move.m_status.m_transportPosition, this->m_obj->m_guid), m_component(nullptr), m_groundNormal(0.0f, 0.0f, 1.0f), m_corpseFlags(0), m_selectionRadius(0.0f), m_lootEffect(nullptr) {
    this->m_passenger.m_facing = create.m_move.m_status.m_transportFacing;
}

// VERIFY: 02b25264 manual
// OFFSET: 0x007059A0
void CGCorpse_C::ApplyGuildColor(int32_t request) {
    if (!this->m_component) {
        return;
    }

    int32_t emblemStyle;
    int32_t emblemColor;
    int32_t borderStyle;
    int32_t borderColor;
    int32_t background;

    if (GuildGetGuildTabard(this->m_corpse->m_corpseFieldOwner, this->m_corpse->m_corpseFieldGuild, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
        this->m_component->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
    } else if (request) {
        WOWGUID* corpseGuid = new (SMemAlloc(sizeof(WOWGUID), __FILE__, __LINE__, 0)) WOWGUID(this->m_obj->m_guid);
        GuildGetGuildTabard(this->m_corpse->m_corpseFieldOwner, this->m_corpse->m_corpseFieldGuild, &GuildTabardCallback, corpseGuid, emblemStyle, emblemColor, borderStyle, borderColor, background);
    }
}

// VERIFY: 54fbf329 manual
// OFFSET: 0x00705B20
void CGCorpse_C::PostInit(uint32_t time, const CClientObjCreate& create, bool a3) {
    this->m_passenger.PostInit(create.m_move.m_status.m_position);

    if (!this->GetTransportGUID()) {
        this->m_passenger.m_facing = create.m_move.m_status.m_facing;
    }

    this->CGObject_C::PostInit(time, create, a3);

    CM2Model* model = this->m_worldModel;
    int32_t useComponent = 1;
    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(this->m_corpse->m_corpseFieldDisplayId);
    CreatureModelDataRec* modelRec = nullptr;

    if (displayRec) {
        modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

        if (modelRec && !(modelRec->m_flags & 0x4)) {
            useComponent = 0;
        }
    }

    if (!(this->m_corpse->m_corpseFieldFlags & 0x1)) {
        if (useComponent) {
            this->m_component = CCharacterComponent::AllocComponent();

            ComponentData data;
            data.m_preferences.m_raceID = this->m_corpse->m_corpseFieldBytes1[1];
            data.m_preferences.m_sexID = this->m_corpse->m_corpseFieldBytes1[2];
            data.m_preferences.m_skinID = this->m_corpse->m_corpseFieldBytes1[3];
            data.m_preferences.m_faceId = this->m_corpse->m_corpseFieldBytes2[0];
            data.m_preferences.m_hairStyleId = this->m_corpse->m_corpseFieldBytes2[1];
            data.m_preferences.m_hairColorId = this->m_corpse->m_corpseFieldBytes2[2];
            data.m_preferences.m_facialHairId = this->m_corpse->m_corpseFieldBytes2[3];
            data.m_model = model;

            uint32_t activePlayer = this->m_corpse->m_corpseFieldOwner == ClntObjMgrGetActivePlayer();
            model->m_refCount++;
            data.m_flags ^= (data.m_flags ^ (activePlayer << 1)) & 0x2;

            this->m_component->Init(&data, nullptr);
        } else {
            CCharacterComponent::ReplaceMonsterSkin(model, displayRec, modelRec);
        }
    }

    CMapEntity* entity = this->m_worldObject;
    C3Vector position = this->GetPosition();
    int32_t liquidType;
    float height;
    int32_t liquidFlags;

    if (World::QueryObjectLiquid(entity, &liquidType, &height, &liquidFlags) && height - position.z > 0.6666667f) {
        this->m_corpseFlags |= 0x2;
        model->SetBoneSequence(0xFFFFFFFF, 132, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        this->m_corpseFlags &= ~0x2;
        model->SetBoneSequence(0xFFFFFFFF, 6, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }

    if (this->m_component) {
        for (uint32_t slot = 0; slot < 19; slot++) {
            ItemDisplayInfoRec rec;

            if (!g_itemDisplayInfoDB.GetRecord(this->m_corpse->m_corpseFieldItem[slot] & 0xFFFFFF, &rec) || slot == 17) {
                continue;
            }

            if (slot == 15 || slot == 16) {
                CGItem_C* item = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(WOWGUID{ this->m_corpse->m_corpseFieldItem[slot] }, TYPEMASK_ITEM, __FILE__, __LINE__));

                if (item) {
                    CM2Model* worldModel = this->m_worldModel;
                    bool shield = item->GetInventoryType() == 14;
                    CCharacterComponent::AddHandItem(worldModel, &rec, static_cast<INVENTORY_SLOTS>(slot), static_cast<SHEATHE_TYPE>(item->GetSheatheType()), false, shield, false, 0);
                }

                continue;
            }

            if (slot == 0) {
                if (this->m_corpse->m_corpseFieldFlags & 0x8) {
                    continue;
                }
            } else if (slot == 14 && (this->m_corpse->m_corpseFieldFlags & 0x10)) {
                continue;
            }

            this->m_component->AddItemBySlot(static_cast<INVENTORY_SLOTS>(slot), rec.m_id, 0);

            if (slot == 18 && (rec.m_flags & 0x1)) {
                this->ApplyGuildColor(1);
            }
        }
    }

    if ((this->m_corpse->m_corpseFieldDynamicFlags & 0x1) && !this->m_lootEffect) {
        const SpellVisualEffectNameRec* effect = GetSpecialSpellVisualEffectNameRec(4);
        this->m_lootEffect = this->m_worldModel->m_scene->CreateModel(effect->m_fileName, 0);

        if (this->m_lootEffect) {
            this->m_lootEffect->AttachToParent(this->m_worldModel, 19, nullptr, 0);
        }
    }

    if (!(this->m_corpse->m_corpseFieldFlags & 0x1) && this->m_corpse->m_corpseFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGGameUI::SetActiveCorpse(this->m_obj->m_guid);
    }
}

// VERIFY: 638a971b manual
// OFFSET: [0x007065B0, 0x007068A0]
CGCorpse_C::~CGCorpse_C() {
    if (this->m_component) {
        CCharacterComponent::FreeComponent(this->m_component);
    }

    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }
}

// VERIFY: e8b3bd0d auto
// OFFSET: 0x007057A0
const char* CGCorpse_C::GetPortraitTexture() const {
    uint32_t displayId = this->m_corpse->m_corpseFieldDisplayId;

    if (!displayId) {
        return nullptr;
    }

    CreatureDisplayInfoRec* rec = g_creatureDisplayInfoDB.GetRecord(displayId);

    return rec ? rec->m_portraitTextureName : nullptr;
}

// VERIFY: e4a2c418 auto
// OFFSET: 0x007058B0
int32_t CGCorpse_C::ShouldFadeout() {
    return !this->m_component || (TextureIsReadyNow(this->m_component->m_baseSkinTexture) && this->m_component->RenderPrep(0));
}

// VERIFY: 761d4ea2 auto
// OFFSET: 0x007058F0
int32_t CGCorpse_C::CanBeLooted() const {
    return this->m_corpse->m_corpseFieldDynamicFlags & 0x1;
}

// VERIFY: 78f995ee auto
// OFFSET: 0x00705A80
uint32_t CGCorpse_C::GetFactionTemplate() const {
    ChrRacesRec* rec = g_chrRacesDB.GetRecord(this->m_corpse->m_corpseFieldBytes1[1]);

    return rec ? rec->m_factionID : 0;
}

// VERIFY: 1cfc7c8d auto
// OFFSET: 0x00705AC0
float CGCorpse_C::GetDisplayScale() const {
    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(this->m_corpse->m_corpseFieldDisplayId);

    if (displayRec) {
        CreatureModelDataRec* modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

        if (modelRec) {
            return modelRec->m_modelScale * displayRec->m_creatureModelScale;
        }
    }

    return 1.0f;
}

// VERIFY: 242bdafd auto
// OFFSET: 0x00706570
float CGCorpse_C::GetTrueScale() const {
    return this->GetDisplayScale() * this->m_scaleModifier * this->m_scale;
}

// VERIFY: e46b7590 manual
// OFFSET: 0x00706590
int32_t CGCorpse_C::CanHighlight() const {
    return !(this->m_corpse->m_corpseFieldFlags & 0x1) || (this->m_corpse->m_corpseFieldDynamicFlags & 0x1);
}

// VERIFY: 5ae4217a auto
// OFFSET: 0x007062C0
void CGCorpse_C::Initialize() {
    ClntObjMgrSetTypeMirrorHandler(ID_CORPSE, offsetof(CGCorpseData, m_corpseFieldFlags), sizeof(uint32_t), CorpseFlagUpdateHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
    ClntObjMgrSetTypeMirrorHandler(ID_CORPSE, offsetof(CGCorpseData, m_corpseFieldDynamicFlags), sizeof(uint32_t), CorpseDynamicFlagUpdateHandler, nullptr, static_cast<HANDLER_PRIORITY>(0), 0);
}

// VERIFY: b1c4d0d1 auto
// OFFSET: 0x00705900
void CGCorpse_C::ShowLootEffect() {
    if (this->m_lootEffect) {
        return;
    }

    const SpellVisualEffectNameRec* effect = GetSpecialSpellVisualEffectNameRec(4);
    this->m_lootEffect = this->m_worldModel->m_scene->CreateModel(effect->m_fileName, 0);

    if (this->m_lootEffect) {
        this->m_lootEffect->AttachToParent(this->m_worldModel, 19, nullptr, 0);
    }
}

// VERIFY: 865254c2 auto
// OFFSET: 0x00706120
void CGCorpse_C::OnFlagChanged(uint32_t oldFlags) {
    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }

    this->Fadeout();

    if (this->m_component) {
        CCharacterComponent::FreeComponent(this->m_component);
        this->m_component = nullptr;
    }

    this->m_corpseFlags &= ~0x1;
    const char* fileName;

    if (!this->GetModelFileName(&fileName)) {
        return;
    }

    CM2Model* model = World::s_m2Scene->CreateModel(fileName, 0);
    this->SetModel(model);

    if (this->m_corpse->m_corpseFieldDynamicFlags & 0x1) {
        this->ShowLootEffect();
    }

    if (model) {
        model->Release();
    }

    this->AddWorldObject();
}

// VERIFY: b8476d46 auto
// OFFSET: 0x007061E0
void CGCorpse_C::OnDynamicFlagChanged(uint32_t oldFlags) {
    if (!((static_cast<uint8_t>(oldFlags) ^ this->m_corpse->m_corpseFieldDynamicFlags) & 0x1)) {
        return;
    }

    if (this->m_corpse->m_corpseFieldDynamicFlags & 0x1) {
        this->ShowLootEffect();
        return;
    }

    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }
}

// VERIFY: bab5ab7f manual
// OFFSET: 0x00706240
int32_t CorpseFlagUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGCorpse_C* corpse = static_cast<CGCorpse_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_CORPSE, __FILE__, __LINE__));

    if (corpse) {
        corpse->OnFlagChanged(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: c5f353fa manual
// OFFSET: 0x00706280
int32_t CorpseDynamicFlagUpdateHandler(WOWGUID guid, uint32_t fieldOffset, uint32_t fieldSize, const void* oldValue, void* param) {
    CGCorpse_C* corpse = static_cast<CGCorpse_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_CORPSE, __FILE__, __LINE__));

    if (corpse) {
        corpse->OnDynamicFlagChanged(*static_cast<const uint32_t*>(oldValue));
    }

    return 1;
}

// VERIFY: 12175e66 auto
// OFFSET: 0x00706530
float CGCorpse_C::GetFacing() const {
    return this->m_passenger.GetFacing(this->m_passenger.m_facing);
}

// VERIFY: 63ae9671 manual
// OFFSET: 0x00705850
void CGCorpse_C::ModelLoaded(CM2Model* model) {
    this->CGObject_C::ModelLoaded(model);

    if (this->m_corpseFlags & 0x2) {
        model->SetBoneSequence(0xFFFFFFFF, 132, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    } else {
        model->SetBoneSequence(0xFFFFFFFF, 6, 0xFFFFFFFF, 0, 1.0f, 1, 1);
    }

    this->m_selectionRadius = model->GetBoundingSphere().r * 0.5f;
}

// VERIFY: af73bdb1 auto
// OFFSET: 0x00705610
void CGCorpse_C::PostReenable() {
    this->m_passenger.Reenable();
    CGObject_C::PostReenable();
}

// VERIFY: 342b4610 manual
// OFFSET: 0x00705670
int32_t CGCorpse_C::GetModelFileName(const char** name) const {
    static char s_modelName[260];

    if (this->m_corpse->m_corpseFieldFlags & 0x1) {
        ChrRacesRec* race = g_chrRacesDB.GetRecord(this->m_corpse->m_corpseFieldBytes1[1]);

        if (!race) {
            return 0;
        }

        SStrPrintf(s_modelName, sizeof(s_modelName), "World\\Generic\\PassiveDoodads\\DeathSkeletons\\%s%sDeathSkeleton.mdx", race->m_clientFileString, s_sexNames[this->m_corpse->m_corpseFieldBytes1[2]]);
        *name = s_modelName;
        return 1;
    }

    uint32_t displayId = this->m_corpse->m_corpseFieldDisplayId;
    CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(displayId);

    if (!display) {
        SysMsgPrintf(STATUS_WARNING, 16, "INVALIDPLAYERDISPLAYID|%d|%d|%d", displayId, this->m_corpse->m_corpseFieldBytes1[1], this->m_corpse->m_corpseFieldBytes1[2]);
        return 0;
    }

    CreatureModelDataRec* model = g_creatureModelDataDB.GetRecord(display->m_modelID);

    if (!model) {
        SysMsgPrintf(STATUS_WARNING, 16, "INVALIDPLAYERMODELRECORD|%d|%d|%d", display->m_modelID, this->m_corpse->m_corpseFieldBytes1[1], this->m_corpse->m_corpseFieldBytes1[2]);
        return 0;
    }

    *name = model->m_modelName;
    return *name != nullptr;
}

// VERIFY: abe6008c auto
// OFFSET: 0x007057E0
void CGCorpse_C::ShouldRender(uint32_t flags, int32_t& render, int32_t& hidden) {
    CGObject_C::ShouldRender(flags, render, hidden);

    if (render || hidden) {
        if (this->m_component) {
            this->m_component->Prep();
        }
    } else if (((this->m_corpseFlags & 0x2) && (this->m_corpse->m_corpseFieldFlags & 0x1)) || (this->m_component && !this->m_component->RenderPrep(0))) {
        hidden = 1;
    }
}

// VERIFY: e0970af6 auto
// OFFSET: 0x00705F30
void CGCorpse_C::Disable() {
    CGObject_C::Disable();
    this->m_passenger.m_link.Unlink();

    if (!(this->m_corpse->m_corpseFieldFlags & 0x1) && this->m_corpse->m_corpseFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGGameUI::SetActiveCorpse(WOWGUID());
    }

    if (this->m_lootEffect) {
        if (this->m_lootEffect->m_attachParent) {
            this->m_lootEffect->DetachFromParent();
        }

        this->m_lootEffect->Release();
        this->m_lootEffect = nullptr;
    }
}

// VERIFY: f555246e manual
// OFFSET: 0x00705FA0
void CGCorpse_C::Reenable() {
    CGObject_C::Reenable();
    this->m_corpseFlags &= ~0x1;

    if (this->m_corpse->m_corpseFieldDynamicFlags & 0x1) {
        this->ShowLootEffect();
    }

    if (this->m_worldModel) {
        this->AddWorldObject();
    }

    if (!(this->m_corpse->m_corpseFieldFlags & 0x1) && this->m_corpse->m_corpseFieldOwner == ClntObjMgrGetActivePlayer()) {
        CGGameUI::SetActiveCorpse(this->m_obj->m_guid);
    }
}

// VERIFY: 99261d4d manual
// OFFSET: 0x00706010
void CGCorpse_C::OnRightClick() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    if ((player->m_mountDisplayId <= 0 || (player->m_clientFlags & 0x10000000)) && (this->m_corpse->m_corpseFieldDynamicFlags & 0x1)) {
        if (player->m_obj->m_guid != CGUnit_C::s_activeMover) {
            return;
        }

        if (player->GetClientStandState()) {
            if (!player->CanAutoStand()) {
                CGGameUI::DisplayError(ERR_LOOT_NOTSTANDING);
                return;
            }

            player->ChangeStandState(0);
        }

        player->SetAutoLoot(CGGameUI::IsAutoLooting());
        player->LootCorpse(this, player->CanAutoInteract());
    } else if ((this->m_corpse->m_corpseFieldFlags & 0x20) && CGSpellBook::GetSkinningSpell(this) && !player->CanAssist(this)) {
        player->HandleSkinning(this, player->CanAutoInteract(), true);
    }
}

// VERIFY: 1b4de750 auto
// OFFSET: 0x007062F0
void CGCorpse_C::RenderTargetSelection() const {
    C3Vector position = this->GetPosition();
    float size = this->GetTrueScale() * this->m_selectionRadius;

    if (size > 10.0f) {
        size = 10.0f;
    }

    CAaBox box;
    box.t.x = position.x + size;
    box.t.y = position.y + size;
    box.t.z = size + size + position.z;
    box.b.x = position.x - size;
    box.b.y = position.y - size;
    box.b.z = position.z - (size + size);
    C44Matrix matrix;
    C3Vector center = this->GetPosition();
    CGObject_C::BuildSelectionRotMatrix(&matrix, &center);
    g_theGxDevicePtr->RsPush();

    if (CGObject_C::SetCircleRenderStates()) {
        ProjectTex::Render2d(box, s_deadSelectionColor, &matrix, 0.5f, 0x200122, 0, 0.4f);
    }

    g_theGxDevicePtr->RsPop();
}

// VERIFY: f37ff84d manual
// OFFSET: 0x007066B0
C3Vector CGCorpse_C::GetGroundNormal() {
    if (!(this->m_corpseFlags & 0x1) && this->GetObjectModel() && this->GetObjectModel()->IsLoaded(0, 0)) {
        static World::FacetData s_facets;
        CAaBox box = this->GetObjectModel()->GetBoundingBox();
        float scale = this->GetScale();
        float height = (box.t.z - box.b.z) * scale;
        box *= scale;
        box += this->GetPosition();
        box.b.z -= height * 0.5f;

        if (World::GetFacets(&box, &s_facets, 0x100111, nullptr) && s_facets.facets.Count()) {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            uint32_t count = 0;

            for (uint32_t i = 0; i < s_facets.facets.Count(); i++) {
                const C3Vector& normal = s_facets.facets[i].plane.n;

                if (normal.z > 0.0f) {
                    count++;
                    x += normal.x;
                    y += normal.y;
                    z += normal.z;
                }
            }

            if (count) {
                float inverse = 1.0f / count;
                this->m_groundNormal = C3Vector(x * inverse, y * inverse, z * inverse);
            }
        }

        this->m_corpseFlags |= 0x1;
    }

    return this->m_groundNormal;
}

// VERIFY: da463ffb auto
// OFFSET: 0x007068D0
int32_t CGCorpse_C::Animate(float elapsed) {
    for (CEffect* effect = this->m_effectList; effect; ) {
        CEffect* next = effect->m_next;
        effect->EffectUpdate();
        effect = next;
    }

    CM2Model* model = this->GetObjectModel();

    if (model) {
        C3Vector up = this->GetGroundNormal();
        model->SetWorldTransform(this->GetPosition(), this->GetRenderFacing(), this->GetTrueScale(), up, 0xFFFFFFFF);
    }

    return 1;
}
