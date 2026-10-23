#include "CGGameObject_C_Type_Text.h"
#include "WoW/Source/UIUtil/Types.h"
#include "WoW/Source/Ui/ItemTextFrame.h"

// VERIFY: ec789ebc auto
// OFFSET: 0x007126D0
CGGameObject_C_Type_Text::CGGameObject_C_Type_Text(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
    this->m_interactDistance = MAX_SHOP_DISTANCE;
}

// VERIFY: 6f615625 auto
// OFFSET: 0x0070B570
void CGGameObject_C_Type_Text::CloseInteraction() {
    this->UpdateAnimState(4);
}

// VERIFY: 47a10015 auto
// OFFSET: 0x0070B560
void CGGameObject_C_Type_Text::StartInteraction() {
    this->UpdateAnimState(2);
}

// VERIFY: 829ac8b8 auto
// OFFSET: 0x0070B540
int32_t CGGameObject_C_Type_Text::CanChangeCursor() const {
    return this->CanUseNow(nullptr, nullptr, nullptr) ? INSPECT_CURSOR : INSPECT_ERROR_CURSOR;
}

// VERIFY: 990b081a auto
// OFFSET: 0x0070C850
bool CGGameObject_C_Type_Text::Use() {
    WOWGUID guid = this->m_owner->m_obj->m_guid;
    CGItemText::SetItem(guid, 0);

    return true;
}

// VERIFY: 58b1a195 manual
// OFFSET: 0x0070C880
void CGGameObject_C_Type_Text::PostInitWithStats() {
    if (CGItemText::m_itemGUID == this->m_owner->m_obj->m_guid) {
        WOWGUID guid = this->m_owner->m_obj->m_guid;
        CGItemText::SetItem(guid, 1);
    }
}
