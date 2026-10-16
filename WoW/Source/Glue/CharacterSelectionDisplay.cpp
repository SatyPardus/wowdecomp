#include "CharacterSelectionDisplay.h"
#include "Engine/Source/Model2/M2Model.h"
#include "WoW/Common/ComponentCore/CharacterComponent.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CharacterSelection.h"

// INLINED
CharacterSelectionDisplay::CharacterSelectionDisplay() {
    this->m_component = nullptr;
    this->m_petAlpha = 1.0f;
    this->m_petModel = nullptr;
}

// INLINED
CharacterSelectionDisplay::~CharacterSelectionDisplay() {
    if (this->m_petModel) {
        this->m_petModel->Release();
    }

    if (this->m_component) {
        CCharacterComponent::FreeComponent(this->m_component);
        this->m_component = nullptr;
    }
}

// VERIFY: 4e7bb4fe auto
// OFFSET: 0x004D9160
int32_t CharacterSelectionDisplay::SetSelectedCharacterInfo(CHARACTER_INFO* info) {
    CGlueMgr::m_characterInfo = info;

    if (!info) {
        return 0;
    }

    CGlueMgr::s_displayedCharacterIndex = CCharacterSelection::s_selectionIndex;
    return 1;
}

// VERIFY: 96adbffe manual
// OFFSET: 0x004E2E70
void CharacterSelectionDisplay::SetCharacterFacing(float rotation) {
    if (!CCharacterSelection::s_characterCount) {
        return;
    }

    CCharacterSelection::s_charFacing = rotation;

    if (CCharacterSelection::s_characterList.Count() <= 0) {
        return;
    }

    CharacterSelectionDisplay& display = CCharacterSelection::s_characterList[CCharacterSelection::s_selectionIndex];

    if (display.m_component->m_data.m_model) {
        C3Vector position(0.0f, 0.0f, 0.0f);
        display.m_component->m_data.m_model->SetWorldTransform(position, rotation, 1.0f);
    }
}
