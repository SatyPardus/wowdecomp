#include "CharacterComponent.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Model2/M2Model.h"

// Original character-creation outfit selection from CCharacterCreation::Dress.
void CCharacterComponent::DressStartingOutfit(int32_t classID, void (*modelLoaded)(CM2Model*, void*)) {
    for (uint32_t i = 0; i < NUM_ITEM_SLOTS; i++) {
        this->RemoveItem(static_cast<ITEM_SLOT>(i));
    }

    CCharacterComponent::RemoveHandItem(this->m_data.m_model, INVSLOT_MAINHAND, SHEATHE_NONE, false);
    CCharacterComponent::RemoveHandItem(this->m_data.m_model, INVSLOT_OFFHAND, SHEATHE_NONE, false);
    CCharacterComponent::RemoveHandItem(this->m_data.m_model, INVSLOT_OFFHAND, SHEATHE_NONE, true);
    CCharacterComponent::RemoveHandItem(this->m_data.m_model, INVSLOT_RANGED, SHEATHE_NONE, false);

    for (int32_t i = 0; i < g_charStartOutfitDB.m_numRecords; i++) {
        CharStartOutfitRec* outfit = i >= 0 && i < g_charStartOutfitDB.m_numRecords ? &g_charStartOutfitDB.m_records[i] : nullptr;

        if (static_cast<uint8_t>(outfit->m_raceID) != this->m_data.m_preferences.m_raceID || static_cast<uint8_t>(outfit->m_classID) != classID || static_cast<uint8_t>(outfit->m_sexID) != this->m_data.m_preferences.m_sexID) {
            continue;
        }

        for (int32_t j = 0; j < 24; j++) {
            int32_t displayID = outfit->m_displayItemID[j];
            int32_t type = outfit->m_inventoryType[j];

            if (displayID <= 0 || type == INVTYPE_HEAD) {
                continue;
            }

            if (classID == 3) {
                switch (type) {
                    case INVTYPE_WEAPON:
                    case INVTYPE_2HWEAPON:
                    case INVTYPE_WEAPONMAINHAND:
                    case INVTYPE_WEAPONOFFHAND: {
                        continue;
                    }

                    case INVTYPE_RANGED:
                    case INVTYPE_RANGEDRIGHT: {
                        bool mainHand = type == INVTYPE_RANGEDRIGHT || type == INVTYPE_THROWN;
                        ItemDisplayInfoRec itemRec;

                        if (g_itemDisplayInfoDB.GetRecord(displayID, &itemRec)) {
                            GEOCOMPONENTLINKS link = CCharacterComponent::AddHandItem(this->m_data.m_model, &itemRec, INVSLOT_RANGED, SHEATHE_NONE, false, false, mainHand, 0);

                            if (type == INVTYPE_RANGED) {
                                for (CM2Model* attached = this->m_data.m_model->m_attachList; attached; attached = attached->m_attachNext) {
                                    if (attached->m_attachmentId == static_cast<uint32_t>(link)) {
                                        attached->SetLoadedCallback(modelLoaded, nullptr);
                                        break;
                                    }
                                }
                            }
                        }

                        break;
                    }

                    case INVTYPE_BAG: {
                        ItemDisplayInfoRec itemRec;

                        if (g_itemDisplayInfoDB.GetRecord(displayID, &itemRec) && *itemRec.m_modelName[0]) {
                            this->AddItem(ITEMSLOT_11, &itemRec, 0);
                        }

                        break;
                    }

                    default: {
                        break;
                    }
                }
            }

            this->AddItemByType(static_cast<INVENTORY_TYPES>(type), displayID);
        }

        return;
    }
}
