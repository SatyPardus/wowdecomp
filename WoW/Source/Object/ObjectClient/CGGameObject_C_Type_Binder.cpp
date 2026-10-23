#include "CGGameObject_C_Type_Binder.h"

// VERIFY: 3617d329 manual
// OFFSET: 0x0070C4E0
CGGameObject_C_Type_Binder::CGGameObject_C_Type_Binder(CGGameObject_C* owner) : CGGameObject_C_TypeBase(owner) {
    this->m_interactDistance = MAX_BIND_DISTANCE;
}
