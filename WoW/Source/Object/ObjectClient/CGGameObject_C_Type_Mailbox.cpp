#include "CGGameObject_C_Type_Mailbox.h"
#include "WoW/Source/UIUtil/Types.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"

// VERIFY: c799badb auto
// OFFSET: 0x00712750
CGGameObject_C_Type_Mailbox::CGGameObject_C_Type_Mailbox(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
}

// VERIFY: a63603e7 manual
// OFFSET: 0x0070C9F0
void CGGameObject_C_Type_Mailbox::StartInteraction() {
    CGGameObject_C* owner = this->m_owner;

    if (owner->IsObjectModelLoaded()) {
        owner->m_typeData->ActivateOneShotAnim(8);
        owner->m_pendingOneShotAnim = 0;
    } else {
        owner->m_pendingOneShotAnim = 2;
    }
}

// VERIFY: a618df81 auto
// OFFSET: 0x0070B680
int32_t CGGameObject_C_Type_Mailbox::CanChangeCursor() const {
    return this->CanUseNow(nullptr, nullptr, nullptr) ? MAIL_CURSOR : MAIL_ERROR_CURSOR;
}

// VERIFY: 617bd209 auto
// OFFSET: 0x0070C9D0
bool CGGameObject_C_Type_Mailbox::Use() {
    CGPlayer_C::TalkToMailbox(this->m_owner->m_obj->m_guid);

    return true;
}
