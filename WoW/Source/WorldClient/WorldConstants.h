#ifndef WOW_SOURCE_WORLDCLIENT_WORLDCONSTANTS_H
#define WOW_SOURCE_WORLDCLIENT_WORLDCONSTANTS_H

// Keep the stored float values: calculating reciprocals can round differently.
static const float SMOLTILE_SIZE = 4.1666665f;
static const float OO_COORD_TO_CHUNK = 0.029999999f;
static const float OO_COORD_TO_SUBCHUNK = 0.23999999f;

// Distance-to-bucket scale for world scene sorting.
static const float CBDIVISOR = 0.029999999f;

static const float MD_AREA_POLY_SIZE = 533.33331f;
static const float TERRAIN_SPEC_EXP = 20.0f;
static const float HALF_WORLD_SIZE = 17066.666f;

static const float OO_COORD_TO_SHADOW = 1.92f;
static const float HALF_SMOLTILE_SIZE = 2.0833333f;

#endif
