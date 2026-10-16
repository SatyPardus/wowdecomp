#ifndef WOW_SOURCE_UI_USEABLESTYLE_H
#define WOW_SOURCE_UI_USEABLESTYLE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Storm/H/stpl.h"

class BarberShopStyleRec;

struct STYLEREC : public TSLinkedNode<STYLEREC> {
    /* 0008 */ const BarberShopStyleRec* m_rec;
};

class USEABLESTYLE : public TSHashObject<USEABLESTYLE, HASHKEY_NONE> {
    public:
    /* 0018 */ TSList<STYLEREC, TSGetLink<STYLEREC>> m_styles;
    /* 0024 */ uint32_t unused_0024[3];

    ~USEABLESTYLE();
};

#endif
