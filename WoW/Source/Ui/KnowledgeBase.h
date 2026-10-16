#ifndef WOW_SOURCE_UI_KNOWLEDGEBASE_H
#define WOW_SOURCE_UI_KNOWLEDGEBASE_H

#include <cstddef>
#include <cstdint>
#include "Engine/Source/Os/OsURLDownload.h"
#include "Engine/Source/Storm/Source/SLock.h"
#include "WoW/Source/Ui/CKBIterator.h"

class CDynamicString;
class XMLNode;
struct XMLTree;
struct lua_State;

class CGKnowledgeBase {
    public:
    /* 0000 */ uint8_t unused_0000[4];
    /* 0004 */ uint32_t unused_0004;
    /* 0008 */ uint8_t unused_0008[44];
    /* 0034 */ uint32_t unused_0034;
    /* 0038 */ uint8_t unused_0038[8];
    /* 0040 */ uint32_t unused_0040;
    /* 0044 */ uint8_t unused_0044[4];
    /* 0048 */ uint32_t unused_0048;
};

class CKBPage {
    public:
    static CKBPage* s_pLoadingQueue_Head;

    static void Initialize();
    static void Shutdown();
    static void UpdateLoadingQueue();

    static bool LoadingCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error);

    /* 0004 */ uint32_t m_state;
    /* 0008 */ SCritSect m_lock;
    /* 0030 */ CDynamicString* m_data;
    /* 0034 */ XMLTree* m_tree;
    /* 0038 */ CKBPage* m_loadingNext;
    /* 003C */ uint32_t m_inQueue;

    CKBPage();
    virtual ~CKBPage();
    virtual void _ComputeCachedData();
    virtual void _SendSuccessEvent();
    virtual void _SendFailureEvent();
    int32_t _BeginLoading();
    XMLNode* _GetRoot(CKBIterator& iterator);
    int32_t _GetInt(XMLNode* node, const char* attribute) const;
    int32_t _GetInt(const char* element, const char* attribute) const;
    bool _GetBool(XMLNode* node, const char* attribute) const;
    const char* _GetBodyString(XMLNode* node) const;
    const char* _GetBodyString(const char* element, const char* child) const;
    bool _GetBodyBool(const char* element, const char* child) const;
    int32_t _GetBodyInt(const char* element, const char* child) const;
};

class CKBSetup : public CKBPage {
    public:
    /* 0040 */ int32_t m_numLanguages;
    /* 0044 */ int32_t m_numCategories;
    /* 0048 */ int32_t m_numSubCategories;
    /* 004C */ int32_t m_numArticleHeaders;
    /* 0050 */ int32_t m_currentCategory;
    /* 0054 */ CKBIterator m_languages;
    /* 005C */ CKBIterator m_categories;
    /* 0064 */ CKBIterator m_subCategories;
    /* 006C */ CKBIterator m_articleHeaders;

    CKBSetup();
    int32_t BeginLoading(int32_t numArticles, int32_t pageNumber);
    XMLNode* GetLanguage(int32_t index);
    XMLNode* GetCategoryNode(int32_t index);
    XMLNode* GetArticleHeader(int32_t index);
    void GetCategory(int32_t category);
    XMLNode* GetSubCategory(int32_t category, int32_t subCategory);
    virtual void _ComputeCachedData();
    virtual void _SendSuccessEvent();
    virtual void _SendFailureEvent();
};

extern CKBSetup g_KnowledgeBaseSetup;

const char* KnowledgeBaseGetURLForLocale();
void KnowledgeBase_RegisterScriptFunctions();
void KnowledgeBase_UnregisterScriptFunctions();

#endif
