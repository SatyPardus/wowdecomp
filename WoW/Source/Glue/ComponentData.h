#ifndef WOW_SOURCE_GLUE_COMPONENTDATA_H
#define WOW_SOURCE_GLUE_COMPONENTDATA_H

#include <cstddef>
#include <cstdint>
#include "WoW/Common/ComponentCore/Types.h"
#include "WoW/Source/Glue/Types.h"

class CM2Model;

class ComponentData {
    public:
    /* 0000 */ CharacterPreferences m_preferences;
    /* 0020 */ CM2Model* m_model;
    /* 0024 */ uint32_t m_flags;
    /* 0028 */ char m_npcSkinTexture [260];
    /* 012C */ uint32_t m_geosets [NUM_CHARGEOSETS];

    ComponentData();
    void SetPreferences(const CharacterPreferences* preferences);
};

#endif
