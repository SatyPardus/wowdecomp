#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "Engine/Source/Storm/H/storm.h"

// VERIFY: 486d0703 auto
// OFFSET: 0x006D5C30
uint32_t GetDisplayIdFromRaceAndGender(uint32_t race, uint32_t gender) {
    if (gender >= 3) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    ChrRacesRec* raceRec = g_chrRacesDB.GetRecord(race);

    if (!raceRec) {
        return 0;
    }

    switch (gender) {
        case 0: {
            return raceRec->m_maleDisplayID;
        }

        case 1: {
            return raceRec->m_femaleDisplayID;
        }

        default: {
            return 0;
        }
    }
}

// VERIFY: 24f1b2a8 manual
// OFFSET: 0x006DC810
CreatureModelDataRec* GetModelDataRec(uint32_t race, uint32_t gender) {
    if (gender >= 3) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    uint32_t displayID = GetDisplayIdFromRaceAndGender(race, gender);

    if (!displayID) {
        return nullptr;
    }

    CreatureDisplayInfoRec* displayRec = g_creatureDisplayInfoDB.GetRecord(displayID);

    if (!displayRec) {
        DebugLog("Error, unknown displayInfo %d specified for player race %d sex %d!", displayID, race, gender);
        return nullptr;
    }

    CreatureModelDataRec* modelRec = g_creatureModelDataDB.GetRecord(displayRec->m_modelID);

    if (!modelRec) {
        DebugLog("Error, unknown model record %d specified for player race %d sex %d!", displayRec->m_modelID, race, gender);
    }

    return modelRec;
}

// VERIFY: fa026898 auto
// OFFSET: 0x00715970
const char* CGUnit_C::GetDisplayRaceNameFromRecord(const ChrRacesRec* rec, int32_t sex, int32_t* displaySex) {
    if (displaySex) {
        *displaySex = sex;
    }

    if (!rec) {
        return nullptr;
    }

    if (sex == 0) {
        if (*rec->m_nameMale) {
            return rec->m_nameMale;
        }

        if (*rec->m_nameFemale) {
            if (displaySex) {
                *displaySex = 1;
            }

            return rec->m_nameFemale;
        }
    } else if (sex == 1) {
        if (*rec->m_nameFemale) {
            return rec->m_nameFemale;
        }

        if (*rec->m_nameMale) {
            if (displaySex) {
                *displaySex = 0;
            }

            return rec->m_nameMale;
        }
    }

    return rec->m_name;
}

// VERIFY: a01b044c auto
// OFFSET: 0x007159E0
const char* CGUnit_C::GetDisplayClassNameFromRecord(const ChrClassesRec* rec, int32_t sex, int32_t* displaySex) {
    if (displaySex) {
        *displaySex = sex;
    }

    if (!rec) {
        return nullptr;
    }

    if (sex == 0) {
        if (*rec->m_nameMale) {
            return rec->m_nameMale;
        }

        if (*rec->m_nameFemale) {
            if (displaySex) {
                *displaySex = 1;
            }

            return rec->m_nameFemale;
        }
    } else if (sex == 1) {
        if (*rec->m_nameFemale) {
            return rec->m_nameFemale;
        }

        if (*rec->m_nameMale) {
            if (displaySex) {
                *displaySex = 0;
            }

            return rec->m_nameMale;
        }
    }

    return rec->m_name;
}
