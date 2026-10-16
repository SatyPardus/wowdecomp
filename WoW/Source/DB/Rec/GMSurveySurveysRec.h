#ifndef WOW_SOURCE_DB_REC_GMSURVEYSURVEYSREC_H
#define WOW_SOURCE_DB_REC_GMSURVEYSURVEYSREC_H

#include <cstddef>
#include <cstdint>

class SFileImpl;

class GMSurveySurveysRec {
    public:
    static const int32_t s_numColumns = 11;
    static const int32_t s_rowSize = 44;
    static const bool s_generatedID = false;

    /* 0000 */ int32_t m_id;
    /* 0004 */ int32_t m_questionIds [10];

    GMSurveySurveysRec();
    ~GMSurveySurveysRec();
    static const char* GetFilename();
    bool Read(SFileImpl* file, const char* stringBuffer);
};

#endif
