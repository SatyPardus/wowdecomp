#include "MapBaseObj.h"

// VERIFY: a9c26690 manual
// OFFSET: [0x007D71D0, 0x007D72A0]
CMapBaseObj::~CMapBaseObj() {
}

// VERIFY: ec60c430 manual
// OFFSET: 0x007D7260
CMapBaseObj::CMapBaseObj() {
    this->m_type = CMapBaseObj::Type_BaseObj;
    this->m_refCount = 0;
    this->m_flags = static_cast<MapObjFlag>(0);
}

// VERIFY: a8266a2d auto
// OFFSET: 0x00632050
void CMapBaseObj::SelectLights(CM2Lighting* lighting) {
}

// VERIFY: e2a6c90a auto
// OFFSET: 0x00632050
void CMapBaseObj::SelectUnderwater(CM2Lighting* lighting) {
}
