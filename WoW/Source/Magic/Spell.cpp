#include "Spell.h"
#include <cmath>
#include "Engine/Source/Tempest/C3Vector.h"
#include "WoW/Source/Magic/MagicClient/SpellEnums.h"

// VERIFY: cbd89f17 auto
// OFFSET: 0x0076BD50
bool Spell_IsAuraEffect(int32_t effect) {
    switch (effect) {
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_PERSISTENT_AREA_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            return true;

        default:
            return false;
    }
}

// VERIFY: 64431913 auto
// OFFSET: 0x0076BE10
bool Spell_IsUnitAuraEffect(int32_t effect) {
    switch (effect) {
        case SPELL_EFFECT_APPLY_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            return true;

        default:
            return false;
    }
}

// VERIFY: 0c750047 auto
// OFFSET: 0x0076BED0
bool Spell_IsAreaAuraEffect(int32_t effect) {
    switch (effect) {
        case SPELL_EFFECT_PERSISTENT_AREA_AURA:
        case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
        case SPELL_EFFECT_APPLY_AREA_AURA_RAID:
        case SPELL_EFFECT_APPLY_AREA_AURA_PET:
        case SPELL_EFFECT_APPLY_AREA_AURA_FRIEND:
        case SPELL_EFFECT_APPLY_AREA_AURA_ENEMY:
        case SPELL_EFFECT_APPLY_AREA_AURA_OWNER:
            return true;

        default:
            return false;
    }
}

// VERIFY: 7a1110db auto
// OFFSET: 0x0076BF80
float SpellUtils_ComputeMissileTrajectoryTravelTime(float pitch, float speed, const C3Vector* delta, float gravity) {
    bool hasGravity = fabs(gravity) > 0.0001f;
    float horizontal = cos(pitch) * speed;

    if (horizontal > 0.0001f) {
        return sqrt(delta->y * delta->y + delta->x * delta->x) / horizontal;
    }

    if (speed <= 0.0001f) {
        if (!hasGravity) {
            return -1.0f;
        }
    } else if (!hasGravity) {
        return fabs(delta->z) / speed;
    }

    float vertical = speed * sin(pitch);
    float discriminant = vertical * vertical - (delta->z * gravity + delta->z * gravity);

    if (discriminant < 0.0f) {
        return -1.0f;
    }

    float root = sqrt(discriminant);
    float scale = -1.0f / gravity;
    float first = (root - vertical) * scale;
    float second = scale * (-vertical - root);

    if (first >= 0.0f && (second < 0.0f || first < second)) {
        return first;
    }

    return second;
}

// VERIFY: 7fc1a99f auto
// OFFSET: 0x0076C070
int32_t SpellUtils_ComputeMissileTrajectory(float pitch, const C3Vector* delta, float gravity, float* speed, float* time, C3Vector* velocity) {
    float c = cos(pitch);
    float s = sin(pitch);
    float horizontal = sqrt(delta->y * delta->y + delta->x * delta->x);

    if (fabs(c) < 0.0001f || fabs(horizontal) < 0.0001f || fabs(gravity) < 0.0001f) {
        return 1;
    }

    float half = (1.0f / c * horizontal * s - delta->z) / gravity;
    float squared = half + half;

    if (squared <= 0.01f) {
        return 0;
    }

    float t = sqrt(squared);
    *time = t;
    float v = 1.0f / c * (horizontal / t);
    *speed = v;

    if (velocity) {
        float inverse = 1.0f / horizontal;
        velocity->x = v * delta->x * inverse * c;
        velocity->y = c * (inverse * *speed) * delta->y;
        velocity->z = s * *speed;
    }

    return 2;
}
