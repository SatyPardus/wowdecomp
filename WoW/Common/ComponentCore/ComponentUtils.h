#ifndef WOW_COMMON_COMPONENTCORE_COMPONENTUTILS_H
#define WOW_COMMON_COMPONENTCORE_COMPONENTUTILS_H

#include <cstddef>
#include <cstdint>
#include "WoW/Common/ComponentCore/Types.h"

struct st_race {
    /* 0000 */ CharVariationList m_variation [CHARTEXTUREVARIATIONS_NUM];
};

int32_t BuildComponentArray(uint32_t count, st_race** lookup);
bool ComponentFlagsMatch(uint32_t flags, COMPONENT_SELECTION selection);
int32_t ComponentGetNumColors(st_race* lookup, int32_t raceID, int32_t sexID, CHARTEXTUREVARIATIONS variation, int32_t variationIndex);
int32_t ComponentGetNumVariations(st_race* lookup, int32_t raceID, int32_t sexID, CHARTEXTUREVARIATIONS variation);
CharSectionsRec* ComponentGetSectionsRecord(st_race* lookup, int32_t raceID, int32_t sexID, CHARTEXTUREVARIATIONS variation, int32_t variationIndex, int32_t colorIndex, bool* found);
bool ComponentValidateBase(st_race* lookup, int32_t raceID, int32_t sexID, CHARTEXTUREVARIATIONS variation, int32_t variationIndex, int32_t colorIndex);
bool ComponentValidateFace(st_race* lookup, int32_t raceID, int32_t sexID, int32_t classID, int32_t colorIndex, int32_t variationIndex, COMPONENT_CONTEXT context);
bool ComponentValidateFacialFeature(st_race* lookup, uint32_t* facialFeatures, int32_t raceID, int32_t sexID, int32_t classID, int32_t colorIndex, int32_t variationIndex, COMPONENT_CONTEXT context);
bool ComponentValidateHair(st_race* lookup, int32_t raceID, int32_t sexID, int32_t classID, int32_t colorIndex, int32_t variationIndex, COMPONENT_CONTEXT context);
bool ComponentValidateSkin(st_race* lookup, int32_t raceID, int32_t sexID, int32_t classID, int32_t colorIndex, COMPONENT_CONTEXT context);
int32_t CountFacialFeatures(uint32_t count, uint32_t** facialFeatures);
COMPONENT_CONTEXT GetContextFromSelection(COMPONENT_SELECTION selection);
COMPONENT_SELECTION GetSelectionFromContext(COMPONENT_CONTEXT context, int32_t classID);

#endif
