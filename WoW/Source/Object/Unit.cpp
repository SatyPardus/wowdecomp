#include "Unit.h"
#include "WoW/Source/Object/CMoveSpline.h"
#include "WoW/Source/Object/Movement.h"
#include "WoW/Source/Object/Object.h"

// STATIC: 0x00AC49D8
const uint32_t CGUnit::s_mirrorFieldIndices[123] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31,
    0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3D, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x5D, 0x5E, 0x5F, 0x60,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
    0x71, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82,
    0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8E,
};

// VERIFY: af22e65c auto
// OFFSET: 0x004F52D0
uint32_t CGUnit::MirrorIndexFromFieldIndex(uint32_t fieldIndex) {
    for (uint32_t i = 0; i < sizeof(s_mirrorFieldIndices) / sizeof(s_mirrorFieldIndices[0]); i++) {
        if (s_mirrorFieldIndices[i] == fieldIndex) {
            return i;
        }
    }

    return 142;
}

// VERIFY: 2461627f manual
// OFFSET: 0x004F53D0
bool CGUnit::CanSwim() const {
    uint32_t flags = this->m_unit->m_unitFieldFlags;

    if (flags & 0x4000) {
        return false;
    }

    if ((flags & 0x8) || (flags & 0x800) || (flags & 0x10)) {
        return true;
    }

    return (flags >> 15) & 1;
}

// VERIFY: a616de1f manual
// OFFSET: 0x004F5410
int32_t CGUnit::CanPlayEmote(uint32_t emoteFlags, int32_t targeted, int32_t* standUp) const {
    *standUp = 0;

    if (targeted && (emoteFlags & 0x400)) {
        return 0;
    }

    if ((emoteFlags & 0x1) && this->m_unit->m_unitFieldBytes1[0]) {
        return 0;
    }

    if ((emoteFlags & 0x80) && (this->m_movement->m_flags & 0x200000)) {
        return 0;
    }

    if (emoteFlags & 0x8000) {
        CMoveSpline* spline = this->m_movement->m_spline;

        if (spline && !(spline->m_flags & 0x400) && (spline->m_flags & 0x2000)) {
            return 0;
        }

        if (this->m_movement->m_flags & 0x2000000) {
            return 0;
        }
    }

    if (!(emoteFlags & 0x200)) {
        uint8_t standState = this->m_unit->m_unitFieldBytes1[0];

        if (standState == 3 || standState == 7) {
            return 0;
        }
    }

    if ((emoteFlags & 0x4000) && (this->m_movement->m_flags & 0xC010FF)) {
        *standUp = 1;
    }

    return 1;
}

// VERIFY: 4f508d75 auto
// OFFSET: 0x004F54D0
void CGUnit::GetResistanceAndBuffs(int32_t school, int32_t* baseValue, int32_t* total, int32_t* effective, int32_t* positive, int32_t* negative) const {
    int32_t resistance = this->m_unit->m_unitFieldResistances[school];
    *effective = resistance;
    *total = resistance;
    *positive = this->m_unit->m_unitFieldResistancebuffmodspositive[school];
    *negative = this->m_unit->m_unitFieldResistancebuffmodsnegative[school];
    *baseValue = *total - *positive - *negative;

    if (*total < 0) {
        *effective = 0;
        *total = 0;
    }
}

// VERIFY: fca45f02 auto
// OFFSET: 0x004F5F20
const WOWGUID* CGUnit::GetCharmerOrCreatorGuid() const {
    CGUnitData* unit = this->m_unit;

    if (unit->m_unitFieldCharmedby) {
        return &unit->m_unitFieldCharmedby;
    }

    return &unit->m_unitFieldCreatedby;
}

// VERIFY: 5d850478 auto
// OFFSET: 0x004F5F40
float CGUnit::GetMeleeRange(const CGUnit* target) const {
    float range = target->m_unit->m_unitFieldCombatreach + this->m_unit->m_unitFieldCombatreach + 1.3333334f;

    if (range < 5.0f) {
        return 5.0f;
    }

    return range;
}

// VERIFY: 803010fd manual
// OFFSET: 0x005774B0
int32_t CGUnit::GetEffectiveStat(int32_t stat) const {
    return this->m_unit->m_unitFieldStat[stat] < 0 ? 0 : this->m_unit->m_unitFieldStat[stat];
}

// VERIFY: 4fc14c7a manual
// OFFSET: 0x00716160
int32_t CGUnit::HasAuraState(int32_t state) const {
    return (this->m_unit->m_unitFieldAurastate & (1 << (state - 1))) != 0;
}

// VERIFY: a8bc343d manual
// OFFSET: 0x004F5290
int32_t CGUnit::IsOnFlyingSpline() const {
    CMoveSpline* spline = this->m_movement->m_spline;

    if (spline && !(spline->m_flags & 0x400) && (spline->m_flags & 0x2000)) {
        return 1;
    }

    return 0;
}

// VERIFY: dd6976d0 auto
// OFFSET: 0x005F9810
int32_t CGUnit::IsOnSpline() const {
    CMoveSpline* spline = this->m_movement->m_spline;
    return spline && !(spline->m_flags & 0x400);
}

// VERIFY: ea2cbe01 auto
// OFFSET: 0x005FEE10
int32_t CGUnit::IsJumping() const {
    return (this->m_movement->m_flags & MOVEMENTFLAG_FALLING) && 0.0f != this->m_movement->m_fallVelocity;
}

// VERIFY: 4e64bda0 auto
// OFFSET: 0x005FA9E0
int32_t CGUnit::CanMove() const {
    return this->m_unit->m_unitFieldHealth > 0 && !(this->m_movement->m_flags & 0x100A00) && this->m_unit->m_unitFieldBytes1[0] != 7;
}

// VERIFY: 68641d42 auto
// OFFSET: 0x004D4240
uint32_t CGUnit::DescriptorToMirrorOffset(uint32_t offset, uint32_t unused_a2, uint32_t unused_a3, uint32_t base) {
    int32_t objectFields = CGObject::TotalFieldsSaved();
    return (offset & 3) + 4 * (objectFields + MirrorIndexFromFieldIndex((offset - base) >> 2));
}

// VERIFY: 93ca7f49 auto
// OFFSET: 0x004F52C0
int32_t CGUnit::TotalFields() {
    return CGObject::TotalFieldsSaved() + 123;
}

// VERIFY: 65e96d9b auto
// OFFSET: 0x005F97F0
float CGUnit::GetFacing(float facing) const {
    return this->m_movement->GetFacing(facing);
}

// VERIFY: 5712b6fb manual
// OFFSET: 0x004F5300
float __stdcall CGUnit::GetPowerRegenBase(int32_t powerType, uint8_t inCombat) {
    static float s_regen[7][2] = {
        { 0.0f, 0.0f },
        { -12.5f, 0.0f },
        { 5.0f, 5.0f },
        { 10.0f, 10.0f },
        { 0.0f, 0.0f },
        { 0.0f, 0.0f },
        { -12.5f, 0.0f }
    };

    return s_regen[powerType][inCombat];
}

// VERIFY: 5dbc6590 auto
// OFFSET: 0x004F5390
float CGUnit::GetPowerRegen(int32_t powerType, bool inCombat) const {
    if (inCombat) {
        return CGUnit::GetPowerRegenBase(powerType, 1) + this->m_unit->m_unitFieldPowerRegenInterruptedFlatModifier[powerType];
    }

    return CGUnit::GetPowerRegenBase(powerType, 0) + this->m_unit->m_unitFieldPowerRegenFlatModifier[powerType];
}

// VERIFY: 90149eb1 manual
// OFFSET: 0x004F7250
bool CGUnit::IsPossessedBy(const WOWGUID& guid) const {
    return (this->m_unit->m_unitFieldFlags & 0x1000000) && *this->GetCharmerOrCreatorGuid() == guid;
}

// VERIFY: 3c755101 auto
// OFFSET: 0x00716140
bool CGUnit::HasCreator() const {
    return this->m_unit->m_unitFieldCreatedby.m_guid != 0;
}

// VERIFY: 94ea7916 auto
// OFFSET: 0x0053B8C0
bool CGUnit::HasSummoner() const {
    return this->m_unit->m_unitFieldSummonedby.m_guid != 0;
}

// VERIFY: 00a60867 manual
// OFFSET: 0x004F5F80
bool CGUnit::IsGuildMaster() const {
    uint32_t flags = this->m_unit->m_unitNpcFlags;
    return (flags & 0x40000) && (flags & 0x80000);
}

// VERIFY: 124e37fd auto
// OFFSET: 0x004F5F00
bool CGUnit::IsCharmed() const {
    return this->m_unit->m_unitFieldCharmedby != WOWGUID();
}

// VERIFY: 2ed8af1f manual
// OFFSET: 0x005EE050
int32_t CGUnit::GetSpellCostModifier(int32_t schoolMask) const {
    int32_t result = 0;
    bool found = false;

    for (uint32_t i = 0; i < sizeof(this->m_unit->m_unitFieldPowerCostModifier) / sizeof(this->m_unit->m_unitFieldPowerCostModifier[0]); i++) {
        if (((1 << i) & schoolMask) && (!found || this->m_unit->m_unitFieldPowerCostModifier[i] < result)) {
            result = this->m_unit->m_unitFieldPowerCostModifier[i];
            found = true;
        }
    }

    return result;
}

// VERIFY: 335247cb manual
// OFFSET: 0x005EE0B0
float CGUnit::GetSpellCostMultiplier(int32_t schoolMask) const {
    float result = 0.0f;
    bool found = false;

    for (uint32_t i = 0; i < sizeof(this->m_unit->m_unitFieldPowerCostMultiplier) / sizeof(this->m_unit->m_unitFieldPowerCostMultiplier[0]); i++) {
        if (((1 << i) & schoolMask) && (!found || result > this->m_unit->m_unitFieldPowerCostMultiplier[i])) {
            result = this->m_unit->m_unitFieldPowerCostMultiplier[i];
            found = true;
        }
    }

    return result + 1.0f;
}

// VERIFY: 43c5b8ef auto
// OFFSET: 0x0058AAB0
int32_t CGUnit::GetQuestGreenRange() const {
    static const int32_t s_questGreenRange[20] = { 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8 };
    uint32_t band = static_cast<int32_t>(this->m_unit->m_unitFieldLevel) / 5;
    return band < 20 ? s_questGreenRange[band] : 8;
}

// VERIFY: 467ea880 manual
// OFFSET: 0x00716190
bool CGUnit::IsSitting() const {
    uint8_t standState = this->m_unit->m_unitFieldBytes1[0];
    return standState == 1 || (standState >= 4 && standState <= 6);
}

// VERIFY: 01cf0dc8 auto
// OFFSET: 0x006FED70
C3Vector CGUnit::GetVelocity() const {
    float speed = this->m_movement->m_currentSpeed;
    return C3Vector(this->m_movement->m_moveDir.x * speed, this->m_movement->m_moveDir.y * speed, speed * this->m_movement->m_moveDir.z);
}
