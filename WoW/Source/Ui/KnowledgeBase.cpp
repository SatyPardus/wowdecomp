#include "KnowledgeBase.h"
#include <cstdlib>
#include "Engine/Source/Base/DynamicString.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Os/TimeManager.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Ui/CKBArticle.h"
#include "WoW/Source/Ui/CKBQuery.h"
#include "WoW/Source/Client.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/Types.h"
#include "Engine/Source/FrameScript/FrameScript.h"

// STATIC: 0x00ACD538
CKBSetup g_KnowledgeBaseSetup;

// STATIC: 0x00BEA7F8
CKBPage* CKBPage::s_pLoadingQueue_Head;

// VERIFY: 17635375 auto
// OFFSET: 0x00550AE0
CKBPage::CKBPage() {
    this->m_state = 0;
    this->m_data = nullptr;
    this->m_tree = nullptr;
    this->m_loadingNext = nullptr;
    this->m_inQueue = 0;
}

// VERIFY: 8d159cad manual
// OFFSET: 0x00550B80
bool CKBPage::LoadingCallback(void* param, const char* data, uint32_t size, bool done, HTTPError::ErrorCode error) {
    CKBPage* page = static_cast<CKBPage*>(param);

    if (!page) {
        return false;
    }

    page->m_lock.Enter();
    bool loading = page->m_state == 1;

    if (!done) {
        if (!data) {
            loading = false;
        }

        if (!size) {
            page->m_lock.Leave();
            return true;
        }
    }

    if (error || !loading) {
        if (error == 2) {
            page->m_state = 5;
        } else if (error == 3) {
            page->m_state = 6;
        } else {
            page->m_state = 4;
        }

        page->m_lock.Leave();
        return false;
    }

    if (!page->m_data) {
        page->m_data = new (SMemAlloc(sizeof(CDynamicString), __FILE__, __LINE__, 0)) CDynamicString();
        page->m_data->SetCapacity(0x8000);
    }

    if (!page->m_data) {
        page->m_state = 7;
        page->m_lock.Leave();
        return false;
    }

    page->m_data->Append(data, size);

    if (!done) {
        page->m_lock.Leave();
        return true;
    }

    if (page->m_tree) {
        XMLTree_Free(page->m_tree);
    }

    page->m_tree = XMLTree_Load(page->m_data->m_buffer, page->m_data->m_length);
    CDynamicString* buffer = page->m_data;

    if (buffer) {
        buffer->~CDynamicString();
        SMemFree(buffer, "delete", SLOG_FUNCTION, 0);
    }

    page->m_data = nullptr;

    if (!page->m_tree) {
        page->m_state = 8;
        page->m_lock.Leave();
        return false;
    }

    page->_ComputeCachedData();
    page->m_state = 2;
    page->m_lock.Leave();
    return true;
}

// VERIFY: ec9f19a2 auto
// OFFSET: 0x00550E10
int32_t CKBSetup::BeginLoading(int32_t numArticles, int32_t pageNumber) {
    if (this->m_state == 1 || this->m_inQueue) {
        return 0;
    }

    CDynamicString url;
    url.SetCapacity(1024);
    url += KnowledgeBaseGetURLForLocale();
    url += "getKBSetup.xml";
    url += "?languageId=";
    url += s_locale;
    url += "&numArticles=";
    url += numArticles;
    url += "&pageNumber=";
    url += pageNumber;
    url += "&locale=";
    url += s_localeCode;

    if (!OsURLDownload(url.m_buffer, &CKBPage::LoadingCallback, this)) {
        this->m_state = 3;
        this->_SendFailureEvent();
        return 0;
    }

    this->m_currentCategory = -1;
    this->m_languages.m_node = nullptr;
    this->m_categories.m_node = nullptr;
    this->m_subCategories.m_node = nullptr;
    this->m_articleHeaders.m_node = nullptr;
    this->m_state = 1;
    this->m_loadingNext = CKBPage::s_pLoadingQueue_Head;
    CKBPage::s_pLoadingQueue_Head = this;
    this->m_inQueue = 1;
    return 1;
}

// VERIFY: 772164f4 auto
// OFFSET: [0x00550B10, 0x005526D0]
CKBPage::~CKBPage() {
    if (this->m_tree) {
        XMLTree_Free(this->m_tree);
        XMLTree_Cleanup();
    }

    if (this->m_data) {
        CDynamicString* data = this->m_data;
        data->~CDynamicString();
        SMemFree(data, "delete", SLOG_FUNCTION, 0);
    }
}

// VERIFY: 14b81430 auto
// OFFSET: 0x005EEB70
void CKBPage::_ComputeCachedData() {
}

// VERIFY: 8bb4a93d auto
// OFFSET: 0x00550B60
int32_t CKBPage::_BeginLoading() {
    this->m_state = 1;
    this->m_loadingNext = CKBPage::s_pLoadingQueue_Head;
    CKBPage::s_pLoadingQueue_Head = this;
    this->m_inQueue = 1;
    return 1;
}

// VERIFY: de149e18 manual
// OFFSET: 0x00550D10
XMLNode* CKBPage::_GetRoot(CKBIterator& iterator) {
    XMLNode* root = XMLTree_GetRoot(this->m_tree);

    if (root) {
        iterator.m_node = root;
        iterator.m_index = 0;
    }

    return root;
}

// VERIFY: bb9406ad auto
// OFFSET: [0x00550D40, 0x00551010]
int32_t CKBPage::_GetInt(XMLNode* node, const char* attribute) const {
    if (this->m_state != 2 || !this->m_tree || !node) {
        return 0;
    }

    const char* value = node->GetAttributeByName(attribute);
    return value ? atol(value) : 0;
}

// VERIFY: 24e29a0d auto
// OFFSET: 0x00550D80
const char* CKBPage::_GetBodyString(XMLNode* node) const {
    if (this->m_state != 2 || !this->m_tree) {
        return nullptr;
    }

    return node->m_body;
}

// VERIFY: c12a4731 auto
// OFFSET: 0x00550DA0
bool CKBPage::_GetBodyBool(const char* element, const char* child) const {
    const char* body = this->_GetBodyString(element, child);
    return body && !SStrCmpI(body, "true", STORM_MAX_STR);
}

// VERIFY: 9b5ccfb5 manual
// OFFSET: [0x005510B0, 0x005512B0]
bool CKBPage::_GetBool(XMLNode* node, const char* attribute) const {
    if (this->m_state != 2 || !this->m_tree || !node) {
        return false;
    }

    const char* value = node->GetAttributeByName(attribute);
    return value && !SStrCmpI(value, "true", STORM_MAX_STR);
}

// VERIFY: d61059d6 manual
// OFFSET: 0x00551300
int32_t CKBPage::_GetInt(const char* element, const char* attribute) const {
    if (this->m_state != 2 || !this->m_tree) {
        return 0;
    }

    XMLNode* root = XMLTree_GetRoot(this->m_tree);

    if (!root) {
        return 0;
    }

    XMLNode* node = root->GetChildByName(element);

    if (!node) {
        return 0;
    }

    const char* value = node->GetAttributeByName(attribute);
    return value ? atol(value) : 0;
}

// INLINED
const char* CKBPage::_GetBodyString(const char* element, const char* child) const {
    if (this->m_state != 2 || !this->m_tree) {
        return nullptr;
    }

    XMLNode* root = XMLTree_GetRoot(this->m_tree);

    if (!root) {
        return nullptr;
    }

    XMLNode* node = root->GetChildByName(element);

    if (!node) {
        return nullptr;
    }

    node = node->GetChildByName(child);
    return node ? node->m_body : nullptr;
}

// VERIFY: 58d9c09c manual
// OFFSET: [0x00551550, 0x005515A0]
int32_t CKBPage::_GetBodyInt(const char* element, const char* child) const {
    const char* body = this->_GetBodyString(element, child);
    return body ? atol(body) : 0;
}

// VERIFY: 8c81adf5 manual
// OFFSET: 0x00550F50
XMLNode* CKBSetup::GetLanguage(int32_t index) {
    if (this->m_state != 2 || !this->m_tree) {
        return nullptr;
    }

    if (!this->m_languages.m_node) {
        XMLNode* root = XMLTree_GetRoot(this->m_tree);

        if (root) {
            XMLNode* languages = root->GetChildByName("Languages");

            if (languages) {
                this->m_languages.m_node = languages->m_child;
                this->m_languages.m_index = 0;
            }
        }
    }

    this->m_languages.GoTo(index);
    return this->m_languages.m_node;
}

// VERIFY: 6baf22c5 manual
// OFFSET: 0x00550FB0
XMLNode* CKBSetup::GetCategoryNode(int32_t index) {
    if (this->m_state != 2 || !this->m_tree) {
        return nullptr;
    }

    if (!this->m_categories.m_node) {
        XMLNode* root = XMLTree_GetRoot(this->m_tree);

        if (root) {
            XMLNode* categories = root->GetChildByName("Categories");

            if (categories) {
                this->m_categories.m_node = categories->m_child;
                this->m_categories.m_index = 0;
            }
        }
    }

    this->m_categories.GoTo(index);
    return this->m_categories.m_node;
}

// VERIFY: 80098012 manual
// OFFSET: 0x00551050
XMLNode* CKBSetup::GetArticleHeader(int32_t index) {
    if (this->m_state != 2 || !this->m_tree) {
        return nullptr;
    }

    if (!this->m_articleHeaders.m_node) {
        XMLNode* root = XMLTree_GetRoot(this->m_tree);

        if (root) {
            XMLNode* headers = root->GetChildByName("ArticleHeaders");

            if (headers) {
                this->m_articleHeaders.m_node = headers->m_child;
                this->m_articleHeaders.m_index = 0;
            }
        }
    }

    this->m_articleHeaders.GoTo(index);
    return this->m_articleHeaders.m_node;
}

// VERIFY: 34a727b3 manual
// OFFSET: 0x00551100
void CKBSetup::_ComputeCachedData() {
    XMLNode* node = nullptr;
    XMLNode* root = XMLTree_GetRoot(this->m_tree);

    if (root) {
        XMLNode* languages = root->GetChildByName("Languages");

        if (languages) {
            node = languages->m_child;
            this->m_articleHeaders.m_node = node;
            this->m_articleHeaders.m_index = 0;
        }
    }

    for (this->m_numLanguages = 0; node; node = node->m_next) {
        this->m_numLanguages++;
    }

    root = XMLTree_GetRoot(this->m_tree);

    if (root) {
        XMLNode* categories = root->GetChildByName("Categories");

        if (categories) {
            node = categories->m_child;
            this->m_articleHeaders.m_node = node;
            this->m_articleHeaders.m_index = 0;
        }
    }

    for (this->m_numCategories = 0; node; node = node->m_next) {
        this->m_numCategories++;
    }

    this->m_numSubCategories = 0;
    root = XMLTree_GetRoot(this->m_tree);

    if (root) {
        XMLNode* headers = root->GetChildByName("ArticleHeaders");

        if (headers) {
            node = headers->m_child;
            this->m_articleHeaders.m_node = node;
            this->m_articleHeaders.m_index = 0;
        }
    }

    for (this->m_numArticleHeaders = 0; node; node = node->m_next) {
        this->m_numArticleHeaders++;
    }
}

// VERIFY: 63e48dff auto
// OFFSET: 0x005511E0
void CKBSetup::GetCategory(int32_t category) {
    this->m_subCategories.m_node = nullptr;
    this->m_subCategories.m_index = 0;
    this->m_numSubCategories = 0;
    this->m_currentCategory = -1;

    XMLNode* node = this->GetCategoryNode(category);

    if (!node) {
        return;
    }

    XMLNode* subCategories = node->GetChildByName("SubCategories");

    if (!subCategories) {
        return;
    }

    XMLNode* child = subCategories->m_child;

    if (!child) {
        return;
    }

    this->m_currentCategory = category;
    this->m_subCategories.m_node = child;

    do {
        this->m_numSubCategories++;
        child = child->m_next;
    } while (child);
}

// VERIFY: a20cef53 manual
// OFFSET: 0x005520B0
XMLNode* CKBSetup::GetSubCategory(int32_t category, int32_t subCategory) {
    if (this->m_state != 2 || !this->m_tree || category < 0) {
        return nullptr;
    }

    if (this->m_currentCategory != category || !this->m_subCategories.m_node) {
        this->GetCategory(category);
    }

    this->m_subCategories.GoTo(subCategory);
    return this->m_subCategories.m_node;
}

// VERIFY: f258e870 auto
// OFFSET: 0x00552100
void CKBSetup::_SendSuccessEvent() {
    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_KNOWLEDGE_BASE_SETUP_LOAD_SUCCESS, nullptr);
    }
}

// VERIFY: f1e62326 auto
// OFFSET: 0x005EEB70
void CKBPage::_SendSuccessEvent() {
}

// VERIFY: 5d3a5e30 auto
// OFFSET: 0x005EEB70
void CKBPage::_SendFailureEvent() {
}

// INLINED
CKBSetup::CKBSetup() {
    this->m_numLanguages = 0;
    this->m_numCategories = 0;
    this->m_numSubCategories = 0;
    this->m_numArticleHeaders = 0;
    this->m_currentCategory = -1;
    this->m_languages.m_node = nullptr;
    this->m_languages.m_index = 0;
    this->m_categories.m_node = nullptr;
    this->m_categories.m_index = 0;
    this->m_subCategories.m_node = nullptr;
    this->m_subCategories.m_index = 0;
    this->m_articleHeaders.m_node = nullptr;
    this->m_articleHeaders.m_index = 0;
}

// VERIFY: 1b2d7a48 manual
// OFFSET: 0x005EEB70
void CKBPage::Initialize() {
}

// VERIFY: c721f005 auto
// OFFSET: 0x00550940
const char* KnowledgeBaseGetURLForLocale() {
    switch (LocaleNameToIndex(s_localeCode)) {
        case DB_LOCALE_deDE:
        case DB_LOCALE_enGB:
        case DB_LOCALE_esES:
        case DB_LOCALE_frFR:
        case DB_LOCALE_ruRU:
            return "http://support.wow-europe.com/kb/";

        case DB_LOCALE_koKR:
            return "http://support.worldofwarcraft.co.kr/kb/";

        case DB_LOCALE_zhCN:
        case DB_LOCALE_enCN:
            return "http://cn.kbase.blizzard.com/kb/wow/";

        case DB_LOCALE_zhTW:
        case DB_LOCALE_enTW:
            return "http://support.wowtaiwan.com.tw/kb/";

        default:
            return "http://support.worldofwarcraft.com/kb/";
    }
}

// VERIFY: 8a5437bf manual
// OFFSET: 0x00550A80
void CKBPage::UpdateLoadingQueue() {
    CKBPage* prev = nullptr;
    CKBPage* page = CKBPage::s_pLoadingQueue_Head;

    while (page) {
        if (page->m_state == 1) {
            prev = page;
            page = page->m_loadingNext;
            continue;
        }

        if (prev) {
            prev->m_loadingNext = page->m_loadingNext;
        } else {
            CKBPage::s_pLoadingQueue_Head = page->m_loadingNext;
        }

        if (page->m_state == 2) {
            page->_SendSuccessEvent();
        } else {
            page->_SendFailureEvent();
        }

        CKBPage* next = page->m_loadingNext;
        page->m_inQueue = 0;
        page->m_loadingNext = nullptr;
        page = next;
    }
}

// VERIFY: c9bdf799 manual
// OFFSET: 0x00552740
void CKBPage::Shutdown() {
    while (CKBPage::s_pLoadingQueue_Head) {
        OsSleep(1);
        CKBPage::UpdateLoadingQueue();
    }

    if (g_KnowledgeBaseSetup.m_tree) {
        XMLTree_Free(g_KnowledgeBaseSetup.m_tree);
        XMLTree_Cleanup();
        g_KnowledgeBaseSetup.m_tree = nullptr;
    }

    if (g_KnowledgeBaseSetup.m_data) {
        CDynamicString* data = g_KnowledgeBaseSetup.m_data;
        data->~CDynamicString();
        SMemFree(data, "delete", SLOG_FUNCTION, 0);
        g_KnowledgeBaseSetup.m_data = nullptr;
    }

    g_KnowledgeBaseSetup.m_state = 0;

    if (g_KnowledgeBaseQuery.m_tree) {
        XMLTree_Free(g_KnowledgeBaseQuery.m_tree);
        XMLTree_Cleanup();
        g_KnowledgeBaseQuery.m_tree = nullptr;
    }

    if (g_KnowledgeBaseQuery.m_data) {
        CDynamicString* data = g_KnowledgeBaseQuery.m_data;
        data->~CDynamicString();
        SMemFree(data, "delete", SLOG_FUNCTION, 0);
        g_KnowledgeBaseQuery.m_data = nullptr;
    }

    g_KnowledgeBaseQuery.m_state = 0;

    if (g_KnowledgeBaseArticle.m_tree) {
        XMLTree_Free(g_KnowledgeBaseArticle.m_tree);
        XMLTree_Cleanup();
        g_KnowledgeBaseArticle.m_tree = nullptr;
    }

    if (g_KnowledgeBaseArticle.m_data) {
        CDynamicString* data = g_KnowledgeBaseArticle.m_data;
        data->~CDynamicString();
        SMemFree(data, "delete", SLOG_FUNCTION, 0);
        g_KnowledgeBaseArticle.m_data = nullptr;
    }

    g_KnowledgeBaseArticle.m_state = 0;
}

// VERIFY: 22ec21ae auto
// OFFSET: 0x00552120
void CKBSetup::_SendFailureEvent() {
    if (CGGameUI::m_initialized) {
        FrameScript_SignalEvent(SCRIPT_EVENT_KNOWLEDGE_BASE_SETUP_LOAD_FAILURE, nullptr);
    }
}
