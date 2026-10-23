#include "ViolenceLevels.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "Engine/Source/ConsoleBase/Types.h"
#include "Engine/Source/Services/Texture.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Client.h"

// STATIC: 0x00AF4E14
static VIOLENCELEVELS s_maxViolenceLevel[9] = { VIOLENCELEVEL_HIGH, VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_HIGH, VIOLENCELEVEL_HIGH, VIOLENCELEVEL_MEDIUM, VIOLENCELEVEL_HIGH, VIOLENCELEVEL_HIGH, VIOLENCELEVEL_HIGH, VIOLENCELEVEL_HIGH };

// STATIC: 0x00AF4E38
static const char* s_bloodSpurtTextures[2] = { "BloodSpurtSmall01_Low.blp", "BloodSpurtSmall01_Medium.blp" };

// STATIC: 0x00AF4E40
static const char* s_bloodSpurtTexture = "BloodSpurtSmall01.blp";

// STATIC: 0x00D3920C
VIOLENCELEVELS s_violenceLevel;

// VERIFY: 0375dcf4 manual
// OFFSET: 0x007F39B0
bool ViolenceLevelChanged(CVar* var, const char* oldValue, const char* value, void* arg) {
    int32_t level = SStrToInt(value);
    s_violenceLevel = static_cast<VIOLENCELEVELS>(level);

    if (level < VIOLENCELEVEL_NONE || level > s_maxViolenceLevel[s_locale]) {
        level = s_maxViolenceLevel[s_locale];
        s_violenceLevel = static_cast<VIOLENCELEVELS>(level);
    }

    if (level == VIOLENCELEVEL_HIGH) {
        TextureSetTextureSubstitutions(nullptr, nullptr);
    } else {
        TextureSetTextureSubstitutions(s_bloodSpurtTexture, s_bloodSpurtTextures[level]);
    }

    return true;
}

// VERIFY: 07f0f122 auto
// OFFSET: 0x007F3A10
void ViolenceLevelsRegisterCVars() {
    char value[12];
    SStrPrintf(value, sizeof(value), "%d", s_maxViolenceLevel[s_locale]);
    CVar::Register("violenceLevel", "Sets the violence level of the game", 0x10, value, ViolenceLevelChanged, CATEGORY_GAME, false, nullptr, false);
}
