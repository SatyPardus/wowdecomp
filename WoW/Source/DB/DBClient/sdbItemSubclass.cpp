#include <cstdint>
#include <cstring>
#include "Engine/Source/Storm/H/stpl.h"
#include "WoW/Source/DB/DBClient/DBClient.h"

// STATIC: 0x00C5DEA8
static TSFixedArray<const ItemSubClassRec*> s_itemSubclasses[17];

// VERIFY: e9c59b21 manual
// OFFSET: 0x00681090
const ItemSubClassRec* SDBItemSubclassGetSubClassRec(uint32_t classID, uint32_t subClassID) {
    if (classID >= sizeof(s_itemSubclasses) / sizeof(s_itemSubclasses[0]) || subClassID >= s_itemSubclasses[classID].Count()) {
        return nullptr;
    }

    return s_itemSubclasses[classID][subClassID];
}

// VERIFY: 600df192 manual
// OFFSET: 0x006810C0
void SDBItemSubclassInitialize() {
    if (g_itemSubClassDB.m_numRecords <= 0) {
        return;
    }

    int32_t counts[sizeof(s_itemSubclasses) / sizeof(s_itemSubclasses[0])];
    memset(counts, 0, sizeof(counts));

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        const ItemSubClassRec* rec = &g_itemSubClassDB.m_records[i];

        if (rec && rec->m_classID >= 0 && counts[rec->m_classID] <= rec->m_subClassID) {
            counts[rec->m_classID] = rec->m_subClassID + 1;
        }
    }

    for (uint32_t i = 0; i < sizeof(s_itemSubclasses) / sizeof(s_itemSubclasses[0]); i++) {
        s_itemSubclasses[i].SetCount(counts[i]);
    }

    for (int32_t i = 0; i < g_itemSubClassDB.m_numRecords; i++) {
        const ItemSubClassRec* rec = &g_itemSubClassDB.m_records[i];

        if (rec && rec->m_classID >= 0) {
            s_itemSubclasses[rec->m_classID][rec->m_subClassID] = rec;
        }
    }
}
