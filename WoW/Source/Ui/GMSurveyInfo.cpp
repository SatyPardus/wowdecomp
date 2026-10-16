#include "GMSurveyInfo.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Base/CDataStore.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Net/Types.h"
#include "WoW/Source/WowSvcs/WowSvcsClient/ClientServices.h"

// STATIC: 0x00C1E8C4
uint8_t CGGMSurveyInfo::m_surveyAnswers[10];

// STATIC: 0x00C1E8D8
char* CGGMSurveyInfo::m_surveyAnswerComments[10];

// STATIC: 0x00C1E900
char* CGGMSurveyInfo::m_surveyComment;

// VERIFY: 520e5a5d manual
// OFFSET: 0x005AC530
const char* CGGMSurveyInfo::GetQuestion(uint32_t index) {
    if (index < 1 || index > 10) {
        return nullptr;
    }

    GMSurveyCurrentSurveyRec* current = g_gmSurveyCurrentSurveyDB.GetRecord(s_locale);
    GMSurveySurveysRec* survey = g_gmSurveySurveysDB.GetRecord(current ? current->m_gmsurveyID : 0);

    if (!survey) {
        return nullptr;
    }

    int32_t questionId = survey->m_questionIds[index - 1];

    if (questionId <= 0) {
        return nullptr;
    }

    GMSurveyQuestionsRec* question = g_gmSurveyQuestionsDB.GetRecord(questionId);
    return question ? question->m_question : nullptr;
}

// VERIFY: 5721e142 manual
// OFFSET: 0x005AC5B0
const char* CGGMSurveyInfo::GetAnswer(uint32_t index, int32_t answer) {
    if (index < 1 || index > 10) {
        return nullptr;
    }

    GMSurveyCurrentSurveyRec* current = g_gmSurveyCurrentSurveyDB.GetRecord(s_locale);
    GMSurveySurveysRec* survey = g_gmSurveySurveysDB.GetRecord(current ? current->m_gmsurveyID : 0);

    if (!survey) {
        return nullptr;
    }

    int32_t questionId = survey->m_questionIds[index - 1];

    if (questionId <= 0 || !g_gmSurveyQuestionsDB.GetRecord(questionId)) {
        return nullptr;
    }

    for (int32_t i = 0; i < g_gmSurveyAnswersDB.m_numRecords; i++) {
        GMSurveyAnswersRec* rec = g_gmSurveyAnswersDB.GetRecordByIndex(i);

        if (rec && rec->m_gmsurveyQuestionID == questionId && rec->m_sortIndex == answer - 1) {
            return rec->m_answer;
        }
    }

    return nullptr;
}

// VERIFY: a6ffe8f6 manual
// OFFSET: 0x005AC680
int32_t CGGMSurveyInfo::GetNumAnswers(uint32_t index) {
    if (index < 1 || index > 10) {
        return 0;
    }

    GMSurveyCurrentSurveyRec* current = g_gmSurveyCurrentSurveyDB.GetRecord(s_locale);

    if (!g_gmSurveySurveysDB.GetRecord(current ? current->m_gmsurveyID : 0)) {
        return 0;
    }

    if (static_cast<int32_t>(index - 1) <= 0 || !g_gmSurveyQuestionsDB.GetRecord(index - 1)) {
        return 0;
    }

    int32_t count = 0;

    for (int32_t i = 0; i < g_gmSurveyAnswersDB.m_numRecords; i++) {
        GMSurveyAnswersRec* rec = g_gmSurveyAnswersDB.GetRecord(i);

        if (rec && rec->m_gmsurveyQuestionID == static_cast<int32_t>(index - 1)) {
            count++;
        }
    }

    return count;
}

// VERIFY: 1157a9a0 manual
// OFFSET: 0x005ACD50
void CGGMSurveyInfo::Submit() {
    CDataStore msg;
    msg.Put(static_cast<uint32_t>(CMSG_GM_SURVEY_SUBMIT));
    GMSurveyCurrentSurveyRec* current = g_gmSurveyCurrentSurveyDB.GetRecord(s_locale);
    msg.Put(static_cast<uint32_t>(current ? current->m_gmsurveyID : 0));

    for (uint32_t i = 0; i < 10; i++) {
        GMSurveyCurrentSurveyRec* currentSurvey = g_gmSurveyCurrentSurveyDB.GetRecord(s_locale);
        GMSurveySurveysRec* survey = g_gmSurveySurveysDB.GetRecord(currentSurvey ? currentSurvey->m_gmsurveyID : 0);
        int32_t questionId = survey ? survey->m_questionIds[i] : 0;
        msg.Put(static_cast<uint32_t>(questionId));

        if (!questionId) {
            break;
        }

        msg.Put(CGGMSurveyInfo::m_surveyAnswers[i]);
        msg.PutString(CGGMSurveyInfo::m_surveyAnswerComments[i] ? CGGMSurveyInfo::m_surveyAnswerComments[i] : "");
    }

    msg.PutString(CGGMSurveyInfo::m_surveyComment ? CGGMSurveyInfo::m_surveyComment : "");
    msg.Finalize();
    ClientServices::Send2(&msg);
    CGGMSurveyInfo::FreeAnswers();
}

// VERIFY: cd4c7b4d auto
// OFFSET: 0x005EEB70
void CGGMSurveyInfo::InitializeGame() {
}

// VERIFY: 849050ba manual
// OFFSET: 0x005AC1F0
void CGGMSurveyInfo::FreeAnswers() {
    for (uint32_t i = 0; i < 10; i++) {
        CGGMSurveyInfo::m_surveyAnswers[i] = 0;

        if (CGGMSurveyInfo::m_surveyAnswerComments[i]) {
            SMemFree(CGGMSurveyInfo::m_surveyAnswerComments[i], __FILE__, __LINE__, 0);
        }

        CGGMSurveyInfo::m_surveyAnswerComments[i] = nullptr;
    }

    if (CGGMSurveyInfo::m_surveyComment) {
        SMemFree(CGGMSurveyInfo::m_surveyComment, __FILE__, __LINE__, 0);
    }

    CGGMSurveyInfo::m_surveyComment = nullptr;
}

// VERIFY: 281b645f manual
// OFFSET: 0x005AC260
void CGGMSurveyInfo::SetAnswer(uint32_t index, uint8_t answer, const char* comment) {
    if (index > 9) {
        return;
    }

    CGGMSurveyInfo::m_surveyAnswers[index] = answer;

    if (CGGMSurveyInfo::m_surveyAnswerComments[index]) {
        SMemFree(CGGMSurveyInfo::m_surveyAnswerComments[index], __FILE__, __LINE__, 0);
    }

    CGGMSurveyInfo::m_surveyAnswerComments[index] = nullptr;

    if (comment && *comment) {
        CGGMSurveyInfo::m_surveyAnswerComments[index] = SStrDupA(comment, __FILE__, __LINE__);
    }
}

// VERIFY: 8239d7e7 manual
// OFFSET: 0x005AC2D0
void CGGMSurveyInfo::SetComment(const char* comment) {
    if (CGGMSurveyInfo::m_surveyComment) {
        SMemFree(CGGMSurveyInfo::m_surveyComment, __FILE__, __LINE__, 0);
    }

    CGGMSurveyInfo::m_surveyComment = nullptr;

    if (comment && *comment) {
        CGGMSurveyInfo::m_surveyComment = SStrDupA(comment, __FILE__, __LINE__);
    }
}

// VERIFY: df97a125 auto
// OFFSET: 0x005AC520
void CGGMSurveyInfo::ShutdownGame() {
    CGGMSurveyInfo::FreeAnswers();
}
