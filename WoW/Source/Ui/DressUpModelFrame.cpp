#include "DressUpModelFrame.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CScriptRegion.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "Engine/Source/Frame/CSimpleModel.h"
#include "Engine/Source/Model2/M2Scene.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/DB/DBClient/CreatureStats_C.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/Player.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/VisibleItem_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Game/GameClient/CDrawBowString.h"
#include "WoW/Source/Ui/GameUI.h"
#include <cstdlib>
#include <cstring>

// STATIC: 0x00ACF54C
const char* CGDressUpModelFrame::s_objectTypeName = "DressUpModel";

// STATIC: 0x00ACF568
int32_t CGDressUpModelFrame::s_metatable = LUA_NOREF;

// STATIC: 0x00C0E4F4
int32_t CGDressUpModelFrame::s_objectType;

// STATIC: 0x00C0E500
CDataAllocator CGDressUpModelFrame::s_dressUpModelHeap(sizeof(CGDressUpModelFrame), 5);

// INLINED
void CGDressUpModelFrame::operator delete(void* ptr) {
    if (ptr) {
        CGDressUpModelFrame::s_dressUpModelHeap.PutData(ptr, nullptr, 0);
    }
}

// INLINED
int32_t CGDressUpModelFrame::GetObjectType() {
    if (!CGDressUpModelFrame::s_objectType) {
        CGDressUpModelFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
    }

    return CGDressUpModelFrame::s_objectType;
}

// VERIFY: 3888121b auto
// OFFSET: 0x00514300
CSimpleFrame* CGDressUpModelFrame::Create(CSimpleFrame* parent) {
    if (CGDressUpModelFrame::s_dressUpModelHeap.m_bytesPerData < sizeof(CGDressUpModelFrame)) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    void* data = CGDressUpModelFrame::s_dressUpModelHeap.GetData(0, typeid(CGDressUpModelFrame).raw_name(), SLOG_OBJECT);
    return data ? new (data) CGDressUpModelFrame(parent) : nullptr;
}

// VERIFY: 62525612 auto
// OFFSET: 0x00597B50
CGDressUpModelFrame::CGDressUpModelFrame(CSimpleFrame* parent) : CGCharacterModelBase(parent) {
    this->m_component = nullptr;
}

// VERIFY: cc335fc2 auto
// OFFSET: 0x00597B80
const char* CGDressUpModelFrame::GetObjectTypeName() {
    return CGDressUpModelFrame::s_objectTypeName;
}

// VERIFY: 44b27d0d auto
// OFFSET: 0x00597B90
int32_t CGDressUpModelFrame::GetScriptMetaTable() {
    return CGDressUpModelFrame::s_metatable;
}

// VERIFY: 76453306 manual
// OFFSET: [0x00597D10, 0x00597DB0]
CGDressUpModelFrame::~CGDressUpModelFrame() {
    if (this->m_component) {
        CCharacterComponent::FreeComponent(this->m_component);
        this->m_component = nullptr;
    }
}

// OFFSET: 0x00597D20
bool CGDressUpModelFrame::IsA(int32_t type) {
    return type == CGDressUpModelFrame::s_objectType || type == CGCharacterModelBase::s_objectType || type == CSimpleModel::s_objectType || type == CSimpleFrame::s_objectType || type == CScriptRegion::s_objectType || type == CScriptObject::s_objectType;
}

// OFFSET: 0x00597D70
bool CGDressUpModelFrame::IsA(const char* typeName) {
    return !SStrCmpI(typeName, CGDressUpModelFrame::s_objectTypeName, STORM_MAX_STR) || CGCharacterModelBase::IsA(typeName);
}

// VERIFY: 79d529ac manual
// OFFSET: 0x00597F40
void CGDressUpModelFrame::UpdateModel() {
    CGCharacterModelBase::UpdateModel();

    if (!this->m_component) {
        return;
    }

    uint32_t ready = this->m_component->RenderPrep(0) & 1;
    CM2Model* model = this->m_component->m_data.m_model;

    if (model->m_attachParent) {
        model->m_fFlags = (model->m_fFlags & ~0x80u) | (ready << 7);
        model->m_fFlags = (model->m_fFlags & ~0x20000u) | (ready << 17);
    } else {
        model->m_fFlags = (model->m_fFlags & ~0x8u) | (ready << 3);
        model->m_fFlags = (model->m_fFlags & ~0x10000u) | (ready << 16);
    }
}

// STATIC: 0x00C0E4F8
static int32_t s_mainHandItemId;

// STATIC: 0x00C0E4FC
static int32_t s_offHandItemId;

// STATIC: 0x00ACF548
static int32_t s_nextHand = 15;

// VERIFY: 67467efb auto
// OFFSET: 0x00597BA0
void CGDressUpModelFrame::Undress() {
    if (!this->m_model) {
        return;
    }

    if (this->m_component) {
        for (uint32_t i = 0; i < 12; i++) {
            this->m_component->RemoveItem(static_cast<ITEM_SLOT>(i));
        }
    }

    if (this->m_mainHand.m_classID) {
        CCharacterComponent::RemoveHandItem(this->m_model, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(this->m_mainHand.m_sheatheType), this->m_mainHand.m_inventoryType == 14);
        this->m_mainHand.m_classID = 0;
        this->m_mainHand.m_inventoryType = 0;
    }

    if (this->m_offHand.m_classID) {
        CCharacterComponent::RemoveHandItem(this->m_model, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(this->m_offHand.m_sheatheType), this->m_offHand.m_inventoryType == 14);
        this->m_offHand.m_classID = 0;
        this->m_offHand.m_inventoryType = 0;
    }
}

// VERIFY: d67e4d73 auto
// OFFSET: 0x00597C50
static bool HandItemsCompatible(uint32_t mainHandType, uint32_t offHandType, bool dualWield) {
    if (mainHandType != 13 && mainHandType != 21) {
        return mainHandType == 17 && offHandType != 15 && CGItem_C::CanGoInSlot(s_mainHandItemId, 16, 0);
    }

    if (offHandType == 14 || offHandType == 23) {
        return true;
    }

    if (offHandType != 13 && offHandType != 22) {
        return (offHandType == 21 || offHandType == 17) && CGItem_C::CanGoInSlot(s_offHandItemId, 16, 0);
    }

    return dualWield;
}

// VERIFY: 1bee737b auto
// OFFSET: 0x00597E10
CM2Model* CGDressUpModelFrame::GetDuplicateUnitCharacterModel(CGUnit_C* unit) {
    if (!unit) {
        return nullptr;
    }

    CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(unit->m_unit->m_unitFieldNativedisplayid);

    if (!display) {
        return nullptr;
    }

    CreatureModelDataRec* modelData = g_creatureModelDataDB.GetRecord(display->m_modelID);

    if (!modelData) {
        return nullptr;
    }

    const char* modelName = modelData->m_modelName;

    if (!modelName || !*modelName) {
        return nullptr;
    }

    return this->GetScene()->CreateModel(modelName, 0);
}

// VERIFY: d215162c manual
// OFFSET: 0x00597FC0
void CGDressUpModelFrame::TryOn(int32_t itemId, const uint32_t* enchants, int32_t hand) {
    ItemVisualsRec* visuals = nullptr;
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, nullptr, nullptr, false);

    if (stats) {
        ItemDisplayInfoRec display;

        if (g_itemDisplayInfoDB.GetRecord(stats->m_displayId, &display)) {
            visuals = g_itemVisualsDB.GetRecord(display.m_itemVisual);
        }
    }

    int32_t visual = 0;

    if (enchants && !visuals) {
        for (uint32_t i = 0; i < 12; i++) {
            SpellItemEnchantmentRec* enchant = g_spellItemEnchantmentDB.GetRecord(enchants[i]);

            if (enchant && enchant->m_itemVisual) {
                visual = enchant->m_itemVisual;
                break;
            }
        }
    }

    this->TryOnWithVisual(itemId, visual, hand);
}

// VERIFY: 67beb92f auto
// OFFSET: 0x005980B0
static void DressUpItemCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success) {
        static_cast<CGDressUpModelFrame*>(param)->TryOn(id, nullptr, -1);
    }
}

// VERIFY: a2d24597 manual
// OFFSET: 0x005980D0
void CGDressUpModelFrame::TryOnWithVisual(int32_t itemId, int32_t visual, int32_t hand) {
    WOWGUID none = WOWGUID();
    ItemStats_C* stats = g_itemDBCache.GetRecord(itemId, none, DressUpItemCallback, this, false);

    if (!stats || !stats->m_displayId || !this->m_model) {
        return;
    }

    if (visual < 0) {
        visual = 0;
    }

    if (!stats->m_inventoryType) {
        for (uint32_t i = 0; i < 5; i++) {
            if (!stats->m_spellId[i]) {
                continue;
            }

            SpellRec spell;

            if (!g_spellDB.GetRecord(stats->m_spellId[i], &spell)) {
                continue;
            }

            SpellRec learned;

            if (spell.m_effect[0] != SPELL_EFFECT_LEARN_SPELL) {
                continue;
            }

            int32_t learnedId = spell.m_effectTriggerSpell[0];

            if (!learnedId) {
                for (uint32_t j = 0; j < 5; j++) {
                    if (stats->m_spellTrigger[j] == 6) {
                        learnedId = stats->m_spellId[j];
                        break;
                    }
                }
            }

            if (g_spellDB.GetRecord(learnedId, &learned) && learned.m_effect[0] == SPELL_EFFECT_CREATE_ITEM) {
                this->TryOn(learned.m_effectItemType[0], nullptr, -1);
                return;
            }
        }
    }

    ITEM_SLOT slot;

    switch (stats->m_inventoryType) {
        case 1:
            slot = static_cast<ITEM_SLOT>(0);
            break;

        case 2:
        case 11:
        case 12:
        case 28:
            return;

        case 3:
            slot = static_cast<ITEM_SLOT>(1);
            break;

        case 4:
            slot = static_cast<ITEM_SLOT>(2);
            break;

        case 5:
        case 20:
            slot = static_cast<ITEM_SLOT>(3);
            break;

        case 6:
            slot = static_cast<ITEM_SLOT>(4);
            break;

        case 7:
            slot = static_cast<ITEM_SLOT>(5);
            break;

        case 8:
            slot = static_cast<ITEM_SLOT>(6);
            break;

        case 9:
            slot = static_cast<ITEM_SLOT>(7);
            break;

        case 10:
            slot = static_cast<ITEM_SLOT>(8);
            break;

        case 16:
            slot = static_cast<ITEM_SLOT>(10);
            break;

        case 19: {
            if (!this->m_component) {
                return;
            }

            this->m_component->RemoveItem(static_cast<ITEM_SLOT>(9));
            this->m_component->AddItem(static_cast<ITEM_SLOT>(9), stats->m_displayId, visual);
            ItemDisplayInfoRec display;

            if (g_itemDisplayInfoDB.GetRecord(stats->m_displayId, &display) && (display.m_flags & 0x1)) {
                CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_unitGuid, TYPE_PLAYER, __FILE__, __LINE__));

                if (player) {
                    int32_t emblemStyle;
                    int32_t emblemColor;
                    int32_t borderStyle;
                    int32_t borderColor;
                    int32_t background;

                    if (GuildGetGuildTabard(player->m_obj->m_guid, player->m_player->m_playerGuildid, nullptr, nullptr, emblemStyle, emblemColor, borderStyle, borderColor, background)) {
                        this->m_component->ApplyGuildColor(emblemStyle, emblemColor, borderStyle, borderColor, background);
                    }
                }
            }

            return;
        }

        case 13:
        case 14:
        case 15:
        case 17:
        case 21:
        case 22:
        case 23:
        case 25:
        case 26: {
            CGPlayer_C* player = nullptr;

            if (this->m_unitGuid == ClntObjMgrGetActivePlayer()) {
                player = ClntObjMgrGetActivePlayerObj();
            }

            CGUnitVirtualItem* handItem;

            if (hand == -1) {
                switch (stats->m_inventoryType) {
                    case 13: {
                        uint8_t mainType;

                        if (!this->m_mainHand.m_classID || ((mainType = this->m_mainHand.m_inventoryType) != 13 && mainType != 21 && mainType != 17) || !player || !CGPlayer_C::CanDualWield()) {
                            hand = 15;
                            goto mainHand;
                        }

                        hand = s_nextHand;
                        s_nextHand = (s_nextHand == 15) + 15;
                        break;
                    }

                    case 17:
                    case 21: {
                        if (!CGItem_C::CanGoInSlot(itemId, 16, 0)) {
                            hand = 15;
                            s_nextHand = 15;
                            goto mainHand;
                        }

                        hand = s_nextHand;
                        s_nextHand = (s_nextHand == 15) + 15;
                        break;
                    }

                    case 25:
                    case 26: {
                        hand = 15;
                        s_nextHand = 15;
                        goto mainHand;
                    }

                    case 14:
                    case 15:
                    case 22:
                    case 23: {
                        hand = 16;
                        s_nextHand = 15;
                        goto offHand;
                    }

                    default:
                        return;
                }
            }

            if (hand == 15) {
mainHand:
                s_mainHandItemId = itemId;

                if (this->m_offHand.m_classID && !HandItemsCompatible(stats->m_inventoryType, this->m_offHand.m_inventoryType, player && CGPlayer_C::CanDualWield())) {
                    CCharacterComponent::RemoveHandItem(this->m_model, static_cast<INVENTORY_SLOTS>(16), static_cast<SHEATHE_TYPE>(this->m_offHand.m_sheatheType), this->m_offHand.m_inventoryType == 14);
                    s_offHandItemId = 0;
                }

                handItem = &this->m_mainHand;
            } else {
offHand:
                s_offHandItemId = itemId;

                if (this->m_mainHand.m_classID && !HandItemsCompatible(this->m_mainHand.m_inventoryType, stats->m_inventoryType, player && CGPlayer_C::CanDualWield())) {
                    CCharacterComponent::RemoveHandItem(this->m_model, static_cast<INVENTORY_SLOTS>(15), static_cast<SHEATHE_TYPE>(this->m_mainHand.m_sheatheType), this->m_mainHand.m_inventoryType == 14);
                    s_mainHandItemId = 0;
                }

                handItem = &this->m_offHand;
            }

            if (handItem->m_classID) {
                CCharacterComponent::RemoveHandItem(this->m_model, static_cast<INVENTORY_SLOTS>(hand), static_cast<SHEATHE_TYPE>(handItem->m_sheatheType), handItem->m_inventoryType == 14);
            }

            handItem->m_classID = static_cast<uint8_t>(stats->m_class);
            handItem->m_subclassID = static_cast<uint8_t>(stats->m_subClass);
            handItem->m_inventoryType = static_cast<uint8_t>(stats->m_inventoryType);
            handItem->m_sheatheType = static_cast<uint8_t>(stats->m_sheatheType);
            ItemDisplayInfoRec display;

            if (g_itemDisplayInfoDB.GetRecord(stats->m_displayId, &display)) {
                CCharacterComponent::AddHandItem(this->m_model, &display, static_cast<INVENTORY_SLOTS>(hand), static_cast<SHEATHE_TYPE>(stats->m_sheatheType), false, stats->m_inventoryType == 14, stats->m_inventoryType == 26, visual);
            }

            return;
        }

        default:
            CGGameUI::DisplayError(ERR_NOT_EQUIPPABLE);
            return;
    }

    if (this->m_component) {
        this->m_component->RemoveItem(slot);
        this->m_component->AddItem(slot, stats->m_displayId, visual);
    }
}

// VERIFY: e534bcd9 auto
// OFFSET: 0x00598800
void CGDressUpModelFrame::TryOnItem(CGItem_C* item, int32_t hand) {
    if (item) {
        int32_t itemId = item->m_obj->m_entryID;
        this->TryOnWithVisual(itemId, item->GetItemEnchantmentVisualID(), hand);
    }
}

// VERIFY: 8eaa7ea4 auto
// OFFSET: 0x005989E0
void CGDressUpModelFrame::SetModel(CM2Model* model) {
    CSimpleModel::SetModel(model);

    if (this->m_component) {
        CCharacterComponent::FreeComponent(this->m_component);
        this->m_component = nullptr;
    }

    if (!model) {
        return;
    }

    CGPlayer_C* player = static_cast<CGPlayer_C*>(ClntObjMgrObjectPtr(this->m_unitGuid, TYPE_PLAYER, __FILE__, __LINE__));

    if (player) {
        this->m_component = CCharacterComponent::AllocComponent();
        model->m_refCount++;
        ComponentData data;
        data.m_preferences.m_raceID = player->m_unit->m_unitFieldBytes0[0];
        data.m_preferences.m_sexID = player->m_player->m_playerBytes3[0];
        data.m_preferences.m_classId = player->m_unit->m_unitFieldBytes0[1];
        data.m_preferences.m_skinID = player->m_player->m_playerBytes[0];
        data.m_preferences.m_faceId = player->m_player->m_playerBytes[1];
        data.m_preferences.m_hairStyleId = player->m_player->m_playerBytes[2];
        data.m_preferences.m_hairColorId = player->m_player->m_playerBytes[3];
        data.m_preferences.m_facialHairId = player->m_player->m_playerBytes2[0];
        data.m_model = model;
        data.m_flags ^= (data.m_flags ^ (static_cast<uint32_t>(player->m_obj->m_guid == ClntObjMgrGetActivePlayer()) << 1)) & 0x2;
        CCharacterComponent::ValidateComponentData(&data, COMPONENT_CONTEXT_2);
        this->m_component->Init(&data, nullptr);
        s_mainHandItemId = 0;
        s_offHandItemId = 0;
        s_nextHand = 15;

        if (player->GetBag()) {
            for (uint32_t slot = 0; slot <= 18; slot++) {
                const VisibleItem_C* item;
                int32_t hand;

                if (slot == 15) {
                    item = player->GetVisibleItem(15);

                    if (!item) {
                        continue;
                    }

                    hand = 15;
                } else if (slot == 16) {
                    item = player->GetVisibleItem(16);

                    if (!item) {
                        continue;
                    }

                    hand = 16;
                } else {
                    if (slot == 0 ? (player->m_player->m_playerFlags & 0x400) != 0 : slot == 14 ? (player->m_player->m_playerFlags & 0x800) != 0 : slot == 17) {
                        continue;
                    }

                    item = player->GetVisibleItem(slot);

                    if (!item) {
                        continue;
                    }

                    hand = -1;
                }

                int32_t itemId = std::abs(item->m_itemId);
                this->TryOnWithVisual(itemId, const_cast<VisibleItem_C*>(item)->GetItemEnchantmentVisualID(), hand);
            }
        }

        const CGUnitVirtualItem* mainHand = player->GetVirtualItem(VIRTUAL_MONSTER_SLOT_MAINHAND, true);

        if (mainHand) {
            this->m_mainHand = *mainHand;
        } else {
            memset(&this->m_mainHand, 0, sizeof(this->m_mainHand));
        }

        const CGUnitVirtualItem* offHand = player->GetVirtualItem(VIRTUAL_MONSTER_SLOT_OFFHAND, true);

        if (offHand) {
            this->m_offHand = *offHand;
        } else {
            memset(&this->m_offHand, 0, sizeof(this->m_offHand));
        }

        this->m_isNativeDisplay = player->m_unit->m_unitFieldNativedisplayid == player->m_unit->m_unitFieldDisplayid;
        return;
    }

    if (!this->m_creatureStats) {
        return;
    }

    CreatureDisplayInfoRec* display = g_creatureDisplayInfoDB.GetRecord(this->m_creatureStats->m_displayId[0]);

    if (!display) {
        return;
    }

    CreatureDisplayInfoExtraRec* extra = g_creatureDisplayInfoExtraDB.GetRecord(display->m_extendedDisplayInfoID);

    if (!extra) {
        return;
    }

    const char* bakeName = extra->m_bakeName;

    if (!bakeName || !*bakeName) {
        return;
    }

    this->m_component = CCharacterComponent::AllocComponent();
    ComponentData data;
    data.m_preferences.m_sexID = extra->m_displaySexID;
    data.m_preferences.m_faceId = extra->m_faceID;
    data.m_preferences.m_hairColorId = extra->m_hairColorID;
    data.m_preferences.m_raceID = extra->m_displayRaceID;
    data.m_flags |= 0x1;
    data.m_preferences.m_skinID = extra->m_skinID;
    data.m_preferences.m_classId = 1;
    data.m_preferences.m_hairStyleId = extra->m_hairStyleID;
    data.m_preferences.m_facialHairId = extra->m_facialHairID;
    data.m_model = model;
    SStrPrintf(data.m_npcSkinTexture, sizeof(data.m_npcSkinTexture), "%s%s", "Textures\\BakedNpcTextures\\", bakeName);
    data.m_model->m_refCount++;
    this->m_component->Init(&data, nullptr);

    for (uint32_t i = 0; i < 11; i++) {
        if (extra->m_npcitemDisplay[i]) {
            this->m_component->AddItem(static_cast<ITEM_SLOT>(i), extra->m_npcitemDisplay[i], 0);
        }
    }
}
