#include "WoW/Source/Ui/Types.h"
#include "CGGameObject_C_Type_Button.h"

// VERIFY: e5e16a0a auto
// OFFSET: 0x00712560
CGGameObject_C_Type_Button::CGGameObject_C_Type_Button(CGGameObject_C* owner) : CGGameObject_C_TypeAnimated(owner) {
    this->m_useErrorType = ERR_BUTTON_LOCKED;
}
