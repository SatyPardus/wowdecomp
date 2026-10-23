#ifndef WOW_SOURCE_GAME_VIOLENCELEVELS_H
#define WOW_SOURCE_GAME_VIOLENCELEVELS_H

#include <cstdint>

enum VIOLENCELEVELS : int32_t {
    VIOLENCELEVEL_NONE = 0,
    VIOLENCELEVEL_MEDIUM = 1,
    VIOLENCELEVEL_HIGH = 2,
    // VIOLENCELEVEL_NUMVIOLENCELEVELS = 3,
};

struct CVar;

extern VIOLENCELEVELS s_violenceLevel;

bool ViolenceLevelChanged(CVar* var, const char* oldValue, const char* value, void* arg);
void ViolenceLevelsRegisterCVars();
VIOLENCELEVELS ViolenceGetLevel();

#endif
