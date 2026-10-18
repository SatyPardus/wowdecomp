#ifndef WOW_COMMON_COMPONENTCORE_TYPES_H
#define WOW_COMMON_COMPONENTCORE_TYPES_H

#include <cstddef>
#include <cstdint>

struct CharColorList;
class CharSectionsRec;

enum COMPONENT_SECTIONS {
    SECTION_ARM_UPPER = 0,
    SECTION_ARM_LOWER = 1,
    SECTION_HAND = 2,
    SECTION_TORSO_UPPER = 3,
    SECTION_TORSO_LOWER = 4,
    SECTION_LEG_UPPER = 5,
    SECTION_LEG_LOWER = 6,
    SECTION_FOOT = 7,
    SECTION_HEAD_UPPER = 8,
    SECTION_HEAD_LOWER = 9,
    NUM_COMPONENT_SECTIONS = 10,
};

enum COMP_HAND_SLOT {
    COMP_HAND_RIGHT = 0,
    COMP_HAND_LEFT = 1,
};

enum GEOCOMPONENTLINKS {
    ATTACH_NONE = -1,
    ATTACH_SHIELD = 0,
    ATTACH_HANDRIGHT = 1,
    ATTACH_HANDLEFT = 2,
    ATTACH_HELM = 11,
    ATTACH_SHEATH_MAINHAND = 26,
    ATTACH_SHEATH_OFFHAND = 27,
    ATTACH_SHEATH_SHIELD = 28,
    ATTACH_BACK = 30,
    ATTACH_BACK_OFFHAND = 31,
    ATTACH_HIP = 32,
    ATTACH_HIP_OFFHAND = 33,
    NUM_ATTACH_LINKS = 50,
};

enum INVENTORY_SLOTS {
    INVSLOT_HEAD = 0,
    INVSLOT_NECK = 1,
    INVSLOT_SHOULDER = 2,
    INVSLOT_BODY = 3,
    INVSLOT_CHEST = 4,
    INVSLOT_WAIST = 5,
    INVSLOT_LEGS = 6,
    INVSLOT_FEET = 7,
    INVSLOT_WRIST = 8,
    INVSLOT_HAND = 9,
    INVSLOT_FINGER1 = 10,
    INVSLOT_FINGER2 = 11,
    INVSLOT_TRINKET1 = 12,
    INVSLOT_TRINKET2 = 13,
    INVSLOT_BACK = 14,
    INVSLOT_MAINHAND = 15,
    INVSLOT_OFFHAND = 16,
    INVSLOT_RANGED = 17,
    INVSLOT_TABARD = 18,
    INVSLOT_NONE = -1,
};

enum INVENTORY_TYPES {
    INVTYPE_NON_EQUIP = 0,
    INVTYPE_HEAD = 1,
    INVTYPE_NECK = 2,
    INVTYPE_SHOULDERS = 3,
    INVTYPE_BODY = 4,
    INVTYPE_CHEST = 5,
    INVTYPE_WAIST = 6,
    INVTYPE_LEGS = 7,
    INVTYPE_FEET = 8,
    INVTYPE_WRISTS = 9,
    INVTYPE_HANDS = 10,
    INVTYPE_FINGER = 11,
    INVTYPE_TRINKET = 12,
    INVTYPE_WEAPON = 13,
    INVTYPE_SHIELD = 14,
    INVTYPE_RANGED = 15,
    INVTYPE_CLOAK = 16,
    INVTYPE_2HWEAPON = 17,
    INVTYPE_BAG = 18,
    INVTYPE_TABARD = 19,
    INVTYPE_ROBE = 20,
    INVTYPE_WEAPONMAINHAND = 21,
    INVTYPE_WEAPONOFFHAND = 22,
    INVTYPE_HOLDABLE = 23,
    INVTYPE_AMMO = 24,
    INVTYPE_THROWN = 25,
    INVTYPE_RANGEDRIGHT = 26,
    INVTYPE_QUIVER = 27,
    INVTYPE_RELIC = 28,
};

// Hand attachments use -1 to mean no character texture/component slot.
enum ITEM_SLOT : int32_t {
    ITEMSLOT_HEAD = 0,
    ITEMSLOT_SHOULDER = 1,
    ITEMSLOT_SHIRT = 2,
    ITEMSLOT_CHEST = 3,
    ITEMSLOT_BELT = 4,
    ITEMSLOT_PANTS = 5,
    ITEMSLOT_BOOTS = 6,
    ITEMSLOT_BRACERS = 7,
    ITEMSLOT_GLOVES = 8,
    ITEMSLOT_TABARD = 9,
    ITEMSLOT_CAPE = 10,
    ITEMSLOT_11 = 11,
    NUM_ITEM_SLOTS = 12,
};

enum SHEATHE_TYPE {
    SHEATHE_NONE = 0,
    SHEATHE_MAINHAND = 1,
    SHEATHE_BACK = 2,
    SHEATHE_HIP = 3,
    SHEATHE_SHIELD = 4,
};

enum COMPONENT_CONTEXT {
    COMPONENT_CONTEXT_0 = 0,
    COMPONENT_CONTEXT_1 = 1,
    COMPONENT_CONTEXT_2 = 2,
    COMPONENT_CONTEXT_3 = 3,
};

enum COMPONENT_SELECTION {
    COMPONENT_SELECTION_0 = 0,
    COMPONENT_SELECTION_1 = 1,
    COMPONENT_SELECTION_2 = 2,
    COMPONENT_SELECTION_3 = 3,
    COMPONENT_SELECTION_4 = 4,
    COMPONENT_SELECTION_5 = 5,
    COMPONENT_SELECTION_6 = 6,
};

enum COMPONENT_VARIATIONS {
    VARIATION_SKIN = 0,
    VARIATION_FACE = 1,
    VARIATION_FACIAL_HAIR = 2,
    VARIATION_HAIR = 3,
    VARIATION_UNDERWEAR = 4,
    NUM_COMPONENT_VARIATIONS = 5,
};

struct CharColorList {
    /* 0000 */ int32_t m_numColors;
    /* 0004 */ CharSectionsRec** m_color;

    CharColorList();
};

struct CharVariationList {
    /* 0000 */ int32_t m_numVariations;
    /* 0004 */ CharColorList* m_variation;

    CharVariationList();
};

struct TCTEXTUREINFO {
    /* 0000 */ uint16_t m_width;
    /* 0002 */ uint16_t m_height;
    /* 0004 */ uint32_t m_mipCount : 8;
    /* 0004 */ uint32_t m_alphaSize : 8;
    /* 0004 */ uint32_t m_opaque : 1;
    /* 0004 */ uint32_t m_unused : 15;
};

// INLINED
inline CharColorList::CharColorList() {
    this->m_numColors = 0;
    this->m_color = nullptr;
}

// INLINED
inline CharVariationList::CharVariationList() {
    this->m_numVariations = 0;
    this->m_variation = nullptr;
}

#endif
