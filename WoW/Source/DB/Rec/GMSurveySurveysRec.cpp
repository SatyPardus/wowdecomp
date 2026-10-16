#include "WoW/Source/DB/Rec/GMSurveySurveysRec.h"
#include "Engine/Source/BlizzardCore/SFile2-Core.h"

GMSurveySurveysRec::GMSurveySurveysRec() {
}

GMSurveySurveysRec::~GMSurveySurveysRec() {
}

// VERIFY: 9720fafd auto
// OFFSET: 0x008AFE40
const char* GMSurveySurveysRec::GetFilename() {
    return "DBFilesClient\\GMSurveySurveys.dbc";
}

// VERIFY: 0111dfdf auto
// OFFSET: 0x008AFE50
bool GMSurveySurveysRec::Read(SFileImpl* file, const char* stringBuffer) {
    int32_t idRead = SFileReadFile(file, &this->m_id, sizeof(this->m_id), nullptr, nullptr, 0);
    int32_t questionIdsRead = SFileReadFile(file, this->m_questionIds, sizeof(this->m_questionIds), nullptr, nullptr, 0);

    if (!idRead
        | !questionIdsRead) {
        return false;
    }

    return true;
}
