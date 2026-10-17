#include "RealmList.h"
#include <cmath>
#include <cstdlib>
#include <new>
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Glue/Types.h"
#include "WoW/Source/WowServices/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00AC4188
int32_t CRealmList::m_selectedCategory = -1;

// STATIC: 0x00AC418C
int32_t CRealmList::m_preferredCategory = -1;

// STATIC: 0x00B6B0BC
float CRealmList::m_avgLoad;

// STATIC: 0x00B6B0C0
float CRealmList::m_stdDevLoad;

// STATIC: 0x00B6B09C
RealmSortCriterion CRealmList::m_sortCriteria[4];

// STATIC: 0x00B6B0C4
TSFixedArray<RealmCategory*> CRealmList::m_categories;

// VERIFY: 430ef903 auto
// OFFSET: 0x004DE5B0
float GetRealmPopulation(REALM_INFO* realm) {
    if (static_cast<int8_t>(realm->m_flags) < 0) {
        return 2.0f;
    }

    if (realm->m_flags & 0x20) {
        return -3.0f;
    }

    if (realm->m_flags & 0x40) {
        return -2.0f;
    }

    if (CRealmList::m_avgLoad - CRealmList::m_stdDevLoad > realm->m_population) {
        return -1.0f;
    }

    if (CRealmList::m_avgLoad + CRealmList::m_stdDevLoad < realm->m_population) {
        return 1.0f;
    }

    return 0.0f;
}

// VERIFY: 505d0861 manual
// OFFSET: 0x004DE630
int32_t QSortCategory(const void* a, const void* b) {
    REALM_INFO* realm1 = ClientServices::GetRealmInfoByIndex(*static_cast<const uint32_t*>(a));
    REALM_INFO* realm2 = ClientServices::GetRealmInfoByIndex(*static_cast<const uint32_t*>(b));

    if (!realm1 || !realm2) {
        return 0;
    }

    int32_t result = 0;

    for (uint32_t i = 0; i < 4; i++) {
        switch (CRealmList::m_sortCriteria[i].m_criterion) {
            case 0: {
                if (!realm1->m_numCharacters && !realm2->m_numCharacters) {
                    continue;
                }

                if (realm1->m_numCharacters == realm2->m_numCharacters) {
                    SStrSortUTF8I(realm1->m_realmName, realm2->m_realmName, STORM_MAX_STR);
                }

                result = realm2->m_numCharacters < realm1->m_numCharacters ? -1 : 1;
                break;
            }

            case 1: {
                float population1 = GetRealmPopulation(realm1);
                float population2 = GetRealmPopulation(realm2);

                if (population1 > population2) {
                    result = 1;
                }

                if (population1 < population2) {
                    result = -1;
                }

                break;
            }

            case 2: {
                result = SStrSortUTF8I(realm1->m_realmName, realm2->m_realmName, STORM_MAX_STR);
                break;
            }

            case 3: {
                if (!g_cfg_ConfigsDB.m_numRecords) {
                    continue;
                }

                int32_t pvp1 = 0;
                int32_t roleplaying1 = 0;
                int32_t pvp2 = 0;
                int32_t roleplaying2 = 0;

                for (int32_t j = 0; j < g_cfg_ConfigsDB.m_numRecords; j++) {
                    const Cfg_ConfigsRec* config = &g_cfg_ConfigsDB.m_records[j];

                    if (config && config->m_realmType == realm1->m_type) {
                        pvp1 = config->m_playerKillingAllowed != 0;
                        roleplaying1 = config->m_roleplaying != 0;
                    }

                    if (config && config->m_realmType == realm2->m_type) {
                        pvp2 = config->m_playerKillingAllowed != 0;
                        roleplaying2 = config->m_roleplaying != 0;
                    }
                }

                if (pvp1 == pvp2) {
                    if (roleplaying1 == roleplaying2) {
                        continue;
                    }

                    result = 2 * (roleplaying1 != 0) - 1;
                } else if (roleplaying1 == roleplaying2) {
                    result = 2 * (pvp1 != 0) - 1;
                } else {
                    result = 2 * (roleplaying1 != 0) - 1;
                }

                break;
            }

            default: {
                continue;
            }
        }

        if (!result) {
            continue;
        }

        return CRealmList::m_sortCriteria[i].m_reverse ? -result : result;
    }

    return 0;
}

// VERIFY: da5b20b3 manual
// OFFSET: 0x004DE860
void CRealmList::SortRealms(int32_t criterion) {
    uint32_t i;

    for (i = 0; i < 4; i++) {
        if (CRealmList::m_sortCriteria[i].m_criterion == criterion) {
            break;
        }
    }

    if (i < 4) {
        int32_t reverse = CRealmList::m_sortCriteria[i].m_reverse;

        if (!i) {
            reverse = reverse == 0;
        }

        for (uint32_t j = i; j; j--) {
            CRealmList::m_sortCriteria[j] = CRealmList::m_sortCriteria[j - 1];
        }

        CRealmList::m_sortCriteria[0].m_criterion = criterion;
        CRealmList::m_sortCriteria[0].m_reverse = reverse;
    }

    for (uint32_t j = 0; j < CRealmList::m_categories.Count(); j++) {
        qsort(CRealmList::m_categories[j]->m_realms.m_data, CRealmList::m_categories[j]->m_numRealms, sizeof(uint32_t), QSortCategory);
    }

    FrameScript_SignalEvent(GLUE_EVENT_OPEN_REALM_LIST, nullptr);
}

// VERIFY: 173835e2 manual
// OFFSET: 0x004DE910
uint32_t GetCategoryIndex(int32_t visibleIndex) {
    int32_t visible = -1;

    for (int32_t i = 0; i < static_cast<int32_t>(CRealmList::m_categories.Count()); i++) {
        RealmCategory* category = CRealmList::m_categories[i];

        if (category && category->m_numRealms && ++visible == visibleIndex) {
            return i;
        }
    }

    return 0;
}

// VERIFY: 110c6d85 manual
// OFFSET: 0x004DE960
void CRealmList::SetPreferredInfo(int32_t index, int32_t pvp, int32_t roleplaying) {
    uint32_t count = CRealmList::m_categories.Count();

    if (!count) {
        return;
    }

    uint32_t categoryIndex = GetCategoryIndex(index);

    if (categoryIndex > count) {
        categoryIndex = 0;
    }

    CRealmList::m_selectedCategory = categoryIndex;
    CRealmList::m_preferredCategory = CRealmList::m_categories[categoryIndex]->m_category->m_id;

    int32_t fallback = -1;

    for (uint32_t i = 0; i < CRealmList::m_categories[categoryIndex]->m_numRealms; i++) {
        REALM_INFO* realm = ClientServices::GetRealmInfoByIndex(CRealmList::m_categories[categoryIndex]->m_realms[i]);

        if (!realm) {
            continue;
        }

        uint8_t flags = realm->m_flags;

        if (flags & 0x3) {
            continue;
        }

        if (flags & 0x20) {
            FrameScript_SignalEvent(GLUE_EVENT_SUGGEST_REALM, "%d%d", index, i + 1);
            return;
        }

        int32_t realmRoleplaying = 0;
        int32_t realmPvp = 0;

        for (int32_t j = 0; j < g_cfg_ConfigsDB.m_numRecords; j++) {
            const Cfg_ConfigsRec* config = &g_cfg_ConfigsDB.m_records[j];

            if (config && config->m_realmType == realm->m_type) {
                realmPvp = config->m_playerKillingAllowed != 0;
                realmRoleplaying = config->m_roleplaying != 0;
                break;
            }
        }

        if (realmPvp == pvp && roleplaying == realmRoleplaying) {
            if (static_cast<int8_t>(flags) >= 0) {
                FrameScript_SignalEvent(GLUE_EVENT_SUGGEST_REALM, "%d%d", index, i + 1);
                return;
            }

            fallback = i;
        }
    }

    if (fallback == -1) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_REALM_LIST, nullptr);
    } else {
        FrameScript_SignalEvent(GLUE_EVENT_SUGGEST_REALM, "%d%d", index, fallback + 1);
    }
}

// VERIFY: 67ff9938 manual
// OFFSET: 0x004DF580
void CRealmList::Initialize() {
    int32_t count = g_cfg_CategoriesDB.m_numRecords;
    CRealmList::m_categories.SetCount(static_cast<uint32_t>(count));

    for (int32_t i = 0; i < count; i++) {
        CRealmList::m_categories[i] = new (SMemAlloc(sizeof(RealmCategory), __FILE__, __LINE__, 0)) RealmCategory;
        CRealmList::m_categories[i]->m_category = &g_cfg_CategoriesDB.m_records[i];
        CRealmList::m_categories[i]->m_numRealms = 0;
    }

    for (uint32_t i = 0; i < 4; i++) {
        CRealmList::m_sortCriteria[i].m_criterion = i;
        CRealmList::m_sortCriteria[i].m_reverse = 0;
    }
}

// VERIFY: 3b30e23a manual
// OFFSET: 0x004DF670
void CRealmList::Shutdown() {
    for (uint32_t i = 0; i < CRealmList::m_categories.Count(); i++) {
        CRealmList::m_categories[i]->m_realms.Clear();

        RealmCategory* category = CRealmList::m_categories[i];

        if (category) {
            category->~RealmCategory();
            SMemFree(category, "delete", SLOG_FUNCTION, 0);
        }
    }

    CRealmList::m_categories.Clear();
}

// VERIFY: 853f8f8d manual
// OFFSET: 0x004DF710
void CRealmList::UpdateList() {
    for (uint32_t i = 0; i < CRealmList::m_categories.Count(); i++) {
        CRealmList::m_categories[i]->m_numRealms = 0;
    }

    uint32_t realmCount = ClientServices::CreateLoginResponse()->m_realmList.Count();
    int32_t selectedCategory = -1;
    float stdDevLoad = 0.0f;

    CRealmList::m_avgLoad = 0.0f;

    for (uint32_t i = 0; i < realmCount; i++) {
        REALM_INFO* realm = ClientServices::GetRealmInfoByIndex(i);

        if (!realm) {
            continue;
        }

        if (!SStrCmpUTF8I(ClientServices::GetSelectedRealmName(), realm->m_realmName, STORM_MAX_STR)) {
            if (CRealmList::m_preferredCategory < 0) {
                CRealmList::m_preferredCategory = realm->m_category;
            }

            selectedCategory = realm->m_category;
        }

        CRealmList::m_avgLoad += realm->m_population;

        for (uint32_t j = 0; j < CRealmList::m_categories.Count(); j++) {
            RealmCategory* category = CRealmList::m_categories[j];

            if (realm->m_category == category->m_category->m_id) {
                if (category->m_realms.Count() == category->m_numRealms) {
                    category->m_realms.SetCount(category->m_numRealms + 1);
                }

                category->m_realms[category->m_numRealms] = i;
                category->m_numRealms++;
            }
        }
    }

    if (realmCount <= 1) {
        CRealmList::m_avgLoad = 1.0f;
    } else {
        CRealmList::m_avgLoad /= realmCount;

        float variance = 0.0f;

        for (uint32_t i = 0; i < realmCount; i++) {
            REALM_INFO* realm = ClientServices::GetRealmInfoByIndex(i);

            if (realm) {
                float delta = realm->m_population - CRealmList::m_avgLoad;
                variance += delta * delta;
            }
        }

        stdDevLoad = sqrt(variance / (realmCount - 1)) * 0.6f;
    }

    CRealmList::m_stdDevLoad = stdDevLoad;

    for (uint32_t i = 0; i < CRealmList::m_categories.Count(); i++) {
        qsort(CRealmList::m_categories[i]->m_realms.m_data, CRealmList::m_categories[i]->m_numRealms, sizeof(uint32_t), QSortCategory);
    }

    for (uint32_t i = 0; i < CRealmList::m_categories.Count(); i++) {
        if (CRealmList::m_categories[i]->m_category->m_id == selectedCategory) {
            CRealmList::m_selectedCategory = i;
            break;
        }
    }

    if (ClientServices::Connection()->m_characterList.Count() || CRealmList::m_preferredCategory != -1) {
        FrameScript_SignalEvent(GLUE_EVENT_OPEN_REALM_LIST, nullptr);
    } else {
        int32_t count = 0;

        for (uint32_t i = 0; i < CRealmList::m_categories.Count(); i++) {
            RealmCategory* category = CRealmList::m_categories[i];

            if (category && category->m_numRealms) {
                count++;
            }
        }

        FrameScript_SignalEvent(GLUE_EVENT_GET_PREFERRED_REALM_INFO, "%d", count);
    }
}
