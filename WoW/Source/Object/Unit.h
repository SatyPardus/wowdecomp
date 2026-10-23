#ifndef WOW_SOURCE_OBJECT_UNIT_H
#define WOW_SOURCE_OBJECT_UNIT_H

#include <cstddef>
#include <cstdint>
#include "WoW/Source/Object/Types.h"
#include "WoW/Source/Object/WGUID.h"
#include "Engine/Source/Tempest/C3Vector.h"

class CGUnitData;
class CMovement_C;

class CGUnit {
    public:
    int32_t GetQuestGreenRange() const;
    static const uint32_t s_mirrorFieldIndices[123];

    static uint32_t MirrorIndexFromFieldIndex(uint32_t fieldIndex);
    static uint32_t DescriptorToMirrorOffset(uint32_t offset, uint32_t unused_a2, uint32_t unused_a3, uint32_t base);
    static int32_t TotalFields();
    static float __stdcall GetPowerRegenBase(POWER_TYPE powerType, uint8_t inCombat);

    /* 0000 */ CGUnitData* m_unit;
    /* 0004 */ void* m_unitMirror;
    /* 0008 */ CMovement_C* m_movement;

    C3Vector GetVelocity() const;

    bool CanSwim() const;
    int32_t CanPlayEmote(uint32_t emoteFlags, int32_t targeted, int32_t* standUp) const;
    void GetResistanceAndBuffs(int32_t school, int32_t* baseValue, int32_t* total, int32_t* effective, int32_t* positive, int32_t* negative) const;
    const WOWGUID* GetCharmerOrCreatorGuid() const;
    bool HasCreator() const;
    bool HasSummoner() const;
    bool IsPossessedBy(const WOWGUID& guid) const;
    bool IsCharmed() const;
    bool IsSitting() const;
    bool IsGuildMaster() const;
    int32_t GetSpellCostModifier(int32_t schoolMask) const;
    float GetSpellCostMultiplier(int32_t schoolMask) const;
    float GetMeleeRange(const CGUnit* target) const;
    int32_t GetEffectiveStat(int32_t stat) const;
    int32_t HasAuraState(int32_t state) const;
    int32_t IsOnFlyingSpline() const;
    int32_t IsOnSpline() const;
    int32_t IsJumping() const;
    int32_t CanMove() const;
    float GetFacing(float facing) const;
    float GetPowerRegen(POWER_TYPE powerType, bool inCombat) const;
};

class CGUnitData {
    public:
    /* 0000 */ WOWGUID m_unitFieldCharm;
    /* 0008 */ WOWGUID m_unitFieldSummon;
    /* 0010 */ WOWGUID m_unitFieldCritter;
    /* 0018 */ WOWGUID m_unitFieldCharmedby;
    /* 0020 */ WOWGUID m_unitFieldSummonedby;
    /* 0028 */ WOWGUID m_unitFieldCreatedby;
    /* 0030 */ WOWGUID m_unitFieldTarget;
    /* 0038 */ WOWGUID m_unitFieldChannelObject;
    /* 0040 */ uint32_t m_unitChannelSpell;
    /* 0044 */ uint8_t m_unitFieldBytes0 [4];
    /* 0048 */ int32_t m_unitFieldHealth;
    /* 004C */ uint32_t m_unitFieldPower [7];
    /* 0068 */ uint32_t m_unitFieldMaxhealth;
    /* 006C */ uint32_t m_unitFieldMaxpower [7];
    /* 0088 */ float m_unitFieldPowerRegenFlatModifier [7];
    /* 00A4 */ float m_unitFieldPowerRegenInterruptedFlatModifier [7];
    /* 00C0 */ uint32_t m_unitFieldLevel;
    /* 00C4 */ uint32_t m_unitFieldFactiontemplate;
    /* 00C8 */ uint32_t m_unitVirtualItemSlotId [NUM_VIRTUAL_MONSTER_SLOTS];
    /* 00D4 */ uint32_t m_unitFieldFlags;
    /* 00D8 */ uint32_t m_unitFieldFlags2;
    /* 00DC */ uint32_t m_unitFieldAurastate;
    /* 00E0 */ uint32_t m_unitFieldBaseattacktime [2];
    /* 00E8 */ uint32_t m_unitFieldRangedattacktime;
    /* 00EC */ float m_unitFieldBoundingradius;
    /* 00F0 */ float m_unitFieldCombatreach;
    /* 00F4 */ uint32_t m_unitFieldDisplayid;
    /* 00F8 */ uint32_t m_unitFieldNativedisplayid;
    /* 00FC */ uint32_t m_unitFieldMountdisplayid;
    /* 0100 */ float m_unitFieldMindamage;
    /* 0104 */ float m_unitFieldMaxdamage;
    /* 0108 */ float m_unitFieldMinoffhanddamage;
    /* 010C */ float m_unitFieldMaxoffhanddamage;
    /* 0110 */ uint8_t m_unitFieldBytes1 [4];
    /* 0114 */ uint32_t m_unitFieldPetnumber;
    /* 0118 */ uint32_t m_unitFieldPetNameTimestamp;
    /* 011C */ uint32_t m_unitFieldPetexperience;
    /* 0120 */ uint32_t m_unitFieldPetnextlevelexp;
    /* 0124 */ uint32_t m_unitDynamicFlags;
    /* 0128 */ float m_unitModCastSpeed;
    /* 012C */ uint32_t m_unitCreatedBySpell;
    /* 0130 */ uint32_t m_unitNpcFlags;
    /* 0134 */ uint32_t m_unitNpcEmotestate;
    /* 0138 */ int32_t m_unitFieldStat [5];
    /* 014C */ int32_t m_unitFieldPosstat [5];
    /* 0160 */ int32_t m_unitFieldNegstat [5];
    /* 0174 */ int32_t m_unitFieldResistances [7];
    /* 0190 */ int32_t m_unitFieldResistancebuffmodspositive [7];
    /* 01AC */ int32_t m_unitFieldResistancebuffmodsnegative [7];
    /* 01C8 */ uint32_t m_unitFieldBaseMana;
    /* 01CC */ uint32_t m_unitFieldBaseHealth;
    /* 01D0 */ uint8_t m_unitFieldBytes2 [4];
    /* 01D4 */ int32_t m_unitFieldAttackPower;
    /* 01D8 */ int16_t m_unitFieldAttackPowerMods [2];
    /* 01DC */ float m_unitFieldAttackPowerMultiplier;
    /* 01E0 */ int32_t m_unitFieldRangedAttackPower;
    /* 01E4 */ int16_t m_unitFieldRangedAttackPowerMods [2];
    /* 01E8 */ float m_unitFieldRangedAttackPowerMultiplier;
    /* 01EC */ float m_unitFieldMinrangeddamage;
    /* 01F0 */ float m_unitFieldMaxrangeddamage;
    /* 01F4 */ int32_t m_unitFieldPowerCostModifier [7];
    /* 0210 */ float m_unitFieldPowerCostMultiplier [7];
    /* 022C */ float m_unitFieldMaxhealthmodifier;
    /* 0230 */ float m_unitFieldHoverheight;
    /* 0234 */ uint32_t m_unitFieldPadding;
};

#endif
