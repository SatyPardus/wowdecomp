#include "BarberShop.h"
#include "WoW/Source/Ui/ScriptEvents.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/Base/CDataStore.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Source/DB/Rec/BarberShopStyleRec.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/UIUtil/Camera.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Source/Net/Types.h"

// STATIC: 0x00BD19B8
uint8_t CGBarberShop::m_barberShopEnabled;

// STATIC: 0x00BD19B9
uint8_t CGBarberShop::m_helmHidden;

// STATIC: 0x00ACC784
int32_t CGBarberShop::m_prevView = 4;

// STATIC: 0x00ACC788
float CGBarberShop::m_minCameraDistance = 1.0f;

// STATIC: 0x00BD19BA
uint8_t CGBarberShop::m_initialized;

// STATIC: 0x00BD19BB
uint8_t CGBarberShop::m_resetPortrait;

// STATIC: 0x00BD19AC
STYLEREC* CGBarberShop::m_currentHairStyle;

// STATIC: 0x00BD19B0
STYLEREC* CGBarberShop::m_currentFacialHair;

// STATIC: 0x00BD19B4
STYLEREC* CGBarberShop::m_currentSkinColor;

// STATIC: 0x00BD19C4
TSHashTable<USEABLESTYLE, HASHKEY_NONE> CGBarberShop::m_useableStyles;

// VERIFY: 28b6e499 auto
// OFFSET: 0x0052E480
float CGBarberShop::GetMinCameraDistance() {
    return CGBarberShop::m_minCameraDistance;
}

// VERIFY: 8a411dd0 auto
// OFFSET: 0x0052E490
const BarberShopStyleRec* CGBarberShop::GetCurrentStyle(int32_t type) {
    switch (type) {
        case 0:
            return CGBarberShop::m_currentHairStyle->m_rec;
        case 2:
            return CGBarberShop::m_currentFacialHair->m_rec;
        case 3:
            return CGBarberShop::m_currentSkinColor->m_rec;
    }

    return nullptr;
}

// VERIFY: 6b04b74f auto
// OFFSET: 0x0052E4D0
void CGBarberShop::AppearanceApplied() {
    Script_SendUniqueSignal(SCRIPT_EVENT_BARBER_SHOP_APPEARANCE_APPLIED);
    CGBarberShop::m_resetPortrait = 1;
}

// VERIFY: 1f780087 auto
// OFFSET: 0x0052E5B0
int32_t Packet_SMSG_BARBER_SHOP_RESULT(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    uint32_t result = 0xFFFFFFFF;
    msg->Get(result);

    switch (result) {
        case 0:
            FrameScript_SignalEvent(SCRIPT_EVENT_BARBER_SHOP_SUCCESS, nullptr);
            break;
        case 1:
        case 3:
            CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
            break;
        case 2:
            CGGameUI::DisplayError(ERR_NOT_BARBER_SITTING);
            break;
        default:
            break;
    }

    return 1;
}

// VERIFY: 80924c98 manual
// OFFSET: 0x0052F7D0
void CGBarberShop::Reset() {
    if (!CGBarberShop::m_barberShopEnabled) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player || !player->m_characterComponent) {
        return;
    }

    HASHKEY_NONE key;

    while (player->m_player->m_playerBytes[2] != CGBarberShop::m_currentHairStyle->m_rec->m_data) {
        STYLEREC* style = CGBarberShop::m_currentHairStyle;
        USEABLESTYLE* useable = CGBarberShop::m_useableStyles.Ptr(style->m_rec->m_type, key);
        STYLEREC* next = style->Next();

        if (!next) {
            next = useable->m_styles.Head();
        }

        CGBarberShop::m_currentHairStyle = next;
    }

    while (player->m_player->m_playerBytes2[0] != CGBarberShop::m_currentFacialHair->m_rec->m_data) {
        STYLEREC* style = CGBarberShop::m_currentFacialHair;
        USEABLESTYLE* useable = CGBarberShop::m_useableStyles.Ptr(style->m_rec->m_type, key);
        STYLEREC* next = style->Next();

        if (!next) {
            next = useable->m_styles.Head();
        }

        CGBarberShop::m_currentFacialHair = next;
    }

    CGBarberShop::ResetAppearance();
}

// VERIFY: 054a94e9 manual
// OFFSET: 0x0052F9B0
int32_t Packet_SMSG_ENABLE_BARBER_SHOP(void* param, NETMESSAGE msgId, uint32_t time, CDataStore* msg) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return 1;
    }

    CCharacterComponent* component = player->m_characterComponent;

    if (!component) {
        return 1;
    }

    if (!(player->m_player->m_playerFlags & 0x400)) {
        if (player->CanEquipItemsInThisForm()) {
            component->RemoveItemBySlot(INVSLOT_HEAD);
        }

        CGBarberShop::m_helmHidden = 1;
    }

    component->RemoveItemBySlot(INVSLOT_SHOULDER);
    VisibleItem_C* offhand = const_cast<VisibleItem_C*>(player->GetVisibleItem(INVSLOT_OFFHAND));

    if (offhand->GetDisplayID() && offhand->GetInventoryType() == INVTYPE_SHIELD) {
        const CGUnitVirtualItem* ranged = player->GetVirtualItem(VIRTUAL_MONSTER_SLOT_RANGED, false);

        if ((!ranged || (ranged->m_inventoryType != 26 && ranged->m_inventoryType != 25)) && player->m_unit->m_unitFieldBytes2[0] == 2) {
            CM2Model* attached = nullptr;

            for (CM2Model* child = player->m_worldModel->m_attachList; child; child = child->m_attachNext) {
                if (child->m_attachmentId == 2) {
                    child->m_refCount++;
                    attached = child;
                    attached->DetachFromParent();
                    break;
                }
            }

            CCharacterComponent::RemoveHandItem(attached, INVSLOT_OFFHAND, static_cast<SHEATHE_TYPE>(offhand->GetSheatheType()), offhand->GetInventoryType() == INVTYPE_SHIELD);

            if (attached) {
                attached->AttachToParent(attached, 2, nullptr, 0);
                attached->Release();
            }
        } else {
            CM2Model* model = player->m_worldModel;
            CCharacterComponent::RemoveHandItem(model, INVSLOT_OFFHAND, static_cast<SHEATHE_TYPE>(offhand->GetSheatheType()), offhand->GetInventoryType() == INVTYPE_SHIELD);
        }
    }

    component->AddItemBySlot(INVSLOT_TABARD, 54559, 0);
    CGBarberShop::m_currentHairStyle = CGBarberShop::GetStyle(0, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], player->m_player->m_playerBytes[2]);
    CGBarberShop::m_currentFacialHair = CGBarberShop::GetStyle(2, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], player->m_player->m_playerBytes2[0]);
    CGBarberShop::m_currentSkinColor = CGBarberShop::GetStyle(3, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], component->m_data.m_preferences.m_skinID);
    CGBarberShop::m_barberShopEnabled = 1;
    FrameScript_SignalEvent(SCRIPT_EVENT_BARBER_SHOP_OPEN, nullptr);
    CGBarberShop::m_minCameraDistance = 1.0f;
    M2SequenceInfo info;
    info.m_bounds.b = C3Vector();
    info.m_bounds.t = C3Vector();
    info.m_center = C3Vector();
    info.m_radius = 0.0f;
    CM2Model* model = player->m_worldModel;

    if (model && model->IsLoaded(0, 0)) {
        model->GetSequenceInfo(player->GetCurrentTorsoAnimId(), 0, info);
        CGBarberShop::m_minCameraDistance = info.m_radius;
    }

    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        CGBarberShop::m_prevView = camera->m_viewIndex;
        float distance;

        if (player->m_unit->m_unitFieldBytes0[0] == 7) {
            distance = CGBarberShop::m_minCameraDistance * 3.0f;
        } else {
            distance = CGBarberShop::m_minCameraDistance + CGBarberShop::m_minCameraDistance;
        }

        camera->BarberShopSetView(distance, 3.1415927f, 0.0f);
    }

    return 1;
}

// VERIFY: 09bc12d2 manual
// OFFSET: 0x0052FC80
void CGBarberShop::SetNextBarberShopStyle(int32_t type, bool backward) {
    if (!CGBarberShop::m_barberShopEnabled) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CCharacterComponent* component = player->m_characterComponent;

    if (!component) {
        return;
    }

    switch (type) {
        case 0:
            if (!backward) {
                component->SetNextHairStyle(COMPONENT_CONTEXT_1);
            } else {
                component->SetPrevHairStyle(COMPONENT_CONTEXT_1);
            }

            CGBarberShop::m_currentHairStyle = CGBarberShop::GetStyle(0, player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[2], component->m_data.m_preferences.m_hairStyleId);
            break;
        case 1:
            if (!backward) {
                component->SetNextHairColor(COMPONENT_CONTEXT_1);
            } else {
                component->SetPrevHairColor(COMPONENT_CONTEXT_1);
            }

            break;
        case 2:
            if (!backward) {
                component->SetNextFacialFeature(COMPONENT_CONTEXT_1);
            } else {
                component->SetPrevFacialFeature(COMPONENT_CONTEXT_1);
            }

            CGBarberShop::m_currentFacialHair = CGBarberShop::GetStyle(2, player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[2], component->m_data.m_preferences.m_facialHairId);
            break;
        case 3:
            if (!backward) {
                component->SetNextSkin(COMPONENT_CONTEXT_1);
            } else {
                component->SetPrevSkin(COMPONENT_CONTEXT_1);
            }

            CGBarberShop::m_currentSkinColor = CGBarberShop::GetStyle(3, player->m_unit->m_unitFieldBytes0[0], player->m_unit->m_unitFieldBytes0[2], component->m_data.m_preferences.m_skinID);
            break;
        default:
            break;
    }

    component->m_data.m_model->IsDrawable(1, 1);
    component->RenderPrep(1);
}

// VERIFY: 1a363132 auto
// OFFSET: 0x0052FE60
void CGBarberShop::InitializeGame() {
    ClientServices::SetMessageHandler(SMSG_ENABLE_BARBER_SHOP, Packet_SMSG_ENABLE_BARBER_SHOP, nullptr);
    ClientServices::SetMessageHandler(SMSG_BARBER_SHOP_RESULT, Packet_SMSG_BARBER_SHOP_RESULT, nullptr);
    CGBarberShop::m_initialized = 0;
}

// VERIFY: d45ddf6d manual
// OFFSET: 0x0052F760
STYLEREC* CGBarberShop::GetStyle(int32_t type, int32_t race, int32_t sex, int32_t data) {
    HASHKEY_NONE key;
    USEABLESTYLE* useable = CGBarberShop::m_useableStyles.Ptr(type, key);

    if (!useable) {
        return nullptr;
    }

    for (STYLEREC* style = useable->m_styles.Head(); !useable->m_styles.IsEnd(style); style = useable->m_styles.RawNext(style)) {
        const BarberShopStyleRec* rec = style->m_rec;

        if (rec->m_race == race && (rec->m_sex == sex || rec->m_sex == -1) && rec->m_data == data) {
            return style;
        }
    }

    return nullptr;
}

// VERIFY: fdac6219 manual
// OFFSET: 0x0052FE90
void CGBarberShop::EnterWorld() {
    if (CGBarberShop::m_initialized) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(ClntObjMgrGetActivePlayer(), TYPE_PLAYER, __FILE__, __LINE__));
    int32_t race = player->m_unit->m_unitFieldBytes0[0];
    int32_t sex = player->m_player->m_playerBytes3[0];

    for (int32_t i = 0; i < g_barberShopStyleDB.m_numRecords; i++) {
        BarberShopStyleRec* rec = g_barberShopStyleDB.GetRecordByIndex(i);

        if (!rec || rec->m_race != race || (rec->m_sex != sex && rec->m_sex != -1)) {
            continue;
        }

        HASHKEY_NONE key;
        USEABLESTYLE* useable = CGBarberShop::m_useableStyles.Ptr(rec->m_type, key);

        if (!useable) {
            useable = CGBarberShop::m_useableStyles.New(rec->m_type, key, 0, 0);
        }

        STYLEREC* style = useable->m_styles.NewNode(2, 0, 0);
        style->m_rec = rec;
    }

    CGBarberShop::m_currentHairStyle = CGBarberShop::GetStyle(0, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], player->m_player->m_playerBytes[2]);
    CGBarberShop::m_currentFacialHair = CGBarberShop::GetStyle(2, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], player->m_player->m_playerBytes2[0]);
    CGBarberShop::m_currentSkinColor = CGBarberShop::GetStyle(3, player->m_unit->m_unitFieldBytes0[0], player->m_player->m_playerBytes3[0], player->m_player->m_playerBytes[0]);
}

// VERIFY: 066af30d manual
// OFFSET: 0x0052E630
void CGBarberShop::ResetAppearance() {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CCharacterComponent* component = player->m_characterComponent;

    if (!component) {
        return;
    }

    component->SetHairStyle(player->m_player->m_playerBytes[2], nullptr);
    component->SetHairColor(player->m_player->m_playerBytes[3], true, nullptr);
    component->SetBeardStyle(player->m_player->m_playerBytes2[0], true, nullptr);
    component->SetSkinColor(player->m_player->m_playerBytes[0], true, true, nullptr);
    component->m_data.m_model->IsDrawable(1, 1);
}

// VERIFY: fb38ad28 manual
// OFFSET: 0x0052E790
bool CGBarberShop::IsCurrentStyle(int32_t type) {
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();
    CCharacterComponent* component = player->m_characterComponent;

    if (!component) {
        return true;
    }

    switch (type) {
        case 0:
            return player->m_player->m_playerBytes[2] == CGBarberShop::m_currentHairStyle->m_rec->m_data;
        case 1:
            return player->m_player->m_playerBytes[3] == component->m_data.m_preferences.m_hairColorId;
        case 2:
            return player->m_player->m_playerBytes2[0] == CGBarberShop::m_currentFacialHair->m_rec->m_data;
        case 3:
            return player->m_player->m_playerBytes[0] == component->m_data.m_preferences.m_skinID;
        default:
            return false;
    }
}

// VERIFY: 436b6807 manual
// OFFSET: 0x0052E9F0
void CGBarberShop::DisableBarberShop() {
    if (!CGBarberShop::m_barberShopEnabled) {
        return;
    }

    CGBarberShop::m_barberShopEnabled = 0;
    FrameScript_SignalEvent(SCRIPT_EVENT_BARBER_SHOP_CLOSE, nullptr);
    CGCamera* camera = CGWorldFrame::GetActiveCamera();

    if (camera) {
        camera->SetView(CGBarberShop::m_prevView, 1, 0);
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    player->ChangeStandState(UNIT_STANDING);
    CCharacterComponent* component = player->m_characterComponent;

    if (!component || player->m_unit->m_unitFieldDisplayid != player->m_unit->m_unitFieldNativedisplayid) {
        return;
    }

    CGBarberShop::ResetAppearance();

    if (CGBarberShop::m_helmHidden) {
        VisibleItem_C* head = const_cast<VisibleItem_C*>(player->GetVisibleItem(INVSLOT_HEAD));

        if (head && head->GetDisplayID() && player->CanEquipItemsInThisForm()) {
            component->AddItemBySlot(INVSLOT_HEAD, head->GetDisplayID(), 0);
        }

        CGBarberShop::m_helmHidden = 0;
    }

    VisibleItem_C* shoulder = const_cast<VisibleItem_C*>(player->GetVisibleItem(INVSLOT_SHOULDER));

    if (shoulder->GetDisplayID()) {
        component->AddItemBySlot(INVSLOT_SHOULDER, shoulder->GetDisplayID(), 0);
    }

    VisibleItem_C* offhand = const_cast<VisibleItem_C*>(player->GetVisibleItem(INVSLOT_OFFHAND));

    if (offhand->GetDisplayID() && offhand->GetInventoryType() == INVTYPE_SHIELD) {
        player->EquipVisibleItem(offhand, INVSLOT_OFFHAND);
    }

    VisibleItem_C* tabard = const_cast<VisibleItem_C*>(player->GetVisibleItem(INVSLOT_TABARD));

    if (tabard && tabard->GetDisplayID()) {
        player->LoadGuildTextures(tabard->GetDisplayID());
    } else {
        component->RemoveItemBySlot(INVSLOT_TABARD);
    }

    if (CGBarberShop::m_resetPortrait) {
        WOWGUID guid = ClntObjMgrGetActivePlayer();
        CGGameUI::UnitModelUpdate(guid, 3);
        CGBarberShop::m_resetPortrait = 0;
    }
}

// VERIFY: ba4d94bb manual
// OFFSET: 0x0052EBA0
int32_t CGBarberShop::GetTotalCost() {
    if (!CGBarberShop::m_barberShopEnabled) {
        return 0;
    }

    int32_t cost = 0;
    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player->m_characterComponent) {
        return 0;
    }

    float base = GetGameTableValue(0, player->m_unit->m_unitFieldLevel - 1, 0);
    int32_t baseCost = static_cast<int32_t>(base);

    if (!CGBarberShop::IsCurrentStyle(0)) {
        float value = baseCost * CGBarberShop::m_currentHairStyle->m_rec->m_costModifier;
        cost = static_cast<int32_t>(value);
    } else if (!CGBarberShop::IsCurrentStyle(1)) {
        float value = baseCost * 0.5f;
        cost = static_cast<int32_t>(value);
    }

    if (!CGBarberShop::IsCurrentStyle(2)) {
        float value = baseCost * CGBarberShop::m_currentFacialHair->m_rec->m_costModifier;
        cost += static_cast<int32_t>(value);
    }

    if (!CGBarberShop::IsCurrentStyle(3)) {
        float value = baseCost * CGBarberShop::m_currentSkinColor->m_rec->m_costModifier;
        cost += static_cast<int32_t>(value);
    }

    return cost;
}

// VERIFY: 1c20fe76 manual
// OFFSET: 0x0052EDA0
void CGBarberShop::ApplyAppearanceChange() {
    if (!CGBarberShop::m_barberShopEnabled) {
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CCharacterComponent* component = player->m_characterComponent;

    if (!component) {
        return;
    }

    if (player->m_player->m_playerBytes[3] == component->m_data.m_preferences.m_hairColorId && player->m_player->m_playerBytes[2] == CGBarberShop::m_currentHairStyle->m_rec->m_data && player->m_player->m_playerBytes2[0] == CGBarberShop::m_currentFacialHair->m_rec->m_data && (!CGBarberShop::m_currentSkinColor || player->m_player->m_playerBytes[0] == CGBarberShop::m_currentSkinColor->m_rec->m_data)) {
        return;
    }

    uint32_t coinage = player->GetCoinage();

    if (static_cast<uint32_t>(CGBarberShop::GetTotalCost()) > coinage) {
        CGGameUI::DisplayError(ERR_NOT_ENOUGH_MONEY);
        return;
    }

    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_ALTER_APPEARANCE));
    msg.Put(static_cast<uint32_t>(CGBarberShop::m_currentHairStyle->m_rec->m_id));
    msg.Put(static_cast<uint32_t>(component->m_data.m_preferences.m_hairColorId));
    msg.Put(static_cast<uint32_t>(CGBarberShop::m_currentFacialHair->m_rec->m_id));

    if (CGBarberShop::m_currentSkinColor) {
        msg.Put(static_cast<uint32_t>(CGBarberShop::m_currentSkinColor->m_rec->m_id));
    } else {
        msg.Put(static_cast<uint32_t>(0));
    }

    msg.Finalize();
    ClientServices::SendPacket(&msg);
}

// VERIFY: 51a462fe auto
// OFFSET: 0x0052F060
void CGBarberShop::ShutdownGame() {
    CGBarberShop::m_initialized = 0;
    CGBarberShop::m_useableStyles.InternalClear(0);
    ClientServices::ClearMessageHandler(SMSG_ENABLE_BARBER_SHOP);
    ClientServices::ClearMessageHandler(SMSG_BARBER_SHOP_RESULT);
}

// VERIFY: a8bfef3b manual
// OFFSET: 0x0052F190
USEABLESTYLE::~USEABLESTYLE() {
    this->m_styles.Clear();
}
