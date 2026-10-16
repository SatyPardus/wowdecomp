#ifndef WOW_SOURCE_GLUE_CHARACTERSELECTIONDISPLAY_H
#define WOW_SOURCE_GLUE_CHARACTERSELECTIONDISPLAY_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/WowServices/RealmConnection.h"

class CCharacterComponent;
class CM2Model;

class CharacterSelectionDisplay {
    public:
    /* 0000 */ CHARACTER_INFO m_characterInfo;
    /* 0188 */ CCharacterComponent* m_component;
    /* 018C */ CM2Model* m_petModel;
    /* 0190 */ float m_petAlpha;
    /* 0194 */ uint32_t unused_0194;

    static void SetCharacterFacing(float rotation);
    static int32_t SetSelectedCharacterInfo(CHARACTER_INFO* info);

    CharacterSelectionDisplay();
    ~CharacterSelectionDisplay();
};

#endif
