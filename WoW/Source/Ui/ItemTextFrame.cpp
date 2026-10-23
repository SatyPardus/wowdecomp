#include "ItemTextFrame.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/DB/DBClient/DBCacheInstances.h"
#include "WoW/Source/Object/Item.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/Object/ItemTextCache.h"
#include "WoW/Source/Object/PageTextCache.h"
#include "WoW/Source/Object/ObjectClient/Bag_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/ScriptEvents.h"

// STATIC: 0x00BFC948
WOWGUID CGItemText::m_itemGUID;

// STATIC: 0x00BFC940
uint32_t CGItemText::m_currentPage;

// STATIC: 0x00BFA9F8
uint8_t CGItemText::m_hasItemText;

// STATIC: 0x00BFAA00
char CGItemText::m_text[8000];

// STATIC: 0x00BFC958
TSGrowableArray<int32_t> CGItemText::m_pages;

// VERIFY: 6900c62a manual
// OFFSET: 0x00589DC0
void CGItemText::InitializeGame() {
    CGItemText::m_itemGUID.m_guid = 0;
    CGItemText::m_hasItemText = 0;
    CGItemText::m_currentPage = 0;
    CGItemText::m_pages.SetCount(2);
    CGItemText::m_pages[0] = 0;
    CGItemText::m_pages[1] = 0;
    CGItemText::m_text[0] = '\0';
}

// VERIFY: 954655b8 auto
// OFFSET: 0x00589E50
void CGItemText::ShutdownGame() {
    CGItemText::m_pages.Clear();
}

// VERIFY: c412497c manual
// OFFSET: 0x00589E90
void CGItemText::DisplayText(const WOWGUID& guid, int32_t translate) {
    if (CGItemText::m_itemGUID != guid) {
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        return;
    }

    if (CGItemText::m_hasItemText) {
        WOWGUID objectGuid = object->m_obj->m_guid;
        ItemTextCache_C* text = g_itemTextDBCache.GetRecord(CGItemText::m_itemGUID.m_guid, objectGuid, CGItemText::ItemTextCallback, nullptr, false);

        if (text) {
            WOWGUID player = ClntObjMgrGetActivePlayer();
            QuestParserParseText(text->m_text, CGItemText::m_text, sizeof(CGItemText::m_text), player, nullptr, 0);
            ProfanityFilter(CGItemText::m_text, false, false);
            FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_READY, nullptr);
        }

        return;
    }

    WOWGUID objectGuid = object->m_obj->m_guid;
    PageTextCache_C* page = g_pageTextCache.GetRecord(CGItemText::m_pages.m_data[CGItemText::m_currentPage], objectGuid, CGItemText::ItemTextCallback, nullptr, false);

    if (!page) {
        return;
    }

    if (CGItemText::m_pages.m_count == CGItemText::m_currentPage + 2) {
        CGItemText::m_pages.SetCount(2 * CGItemText::m_pages.m_count);
    }

    CGItemText::m_pages.m_data[CGItemText::m_currentPage + 1] = page->m_nextPage;
    char buffer[4000];
    WOWGUID player = ClntObjMgrGetActivePlayer();
    QuestParserParseText(page->m_text, buffer, sizeof(buffer), player, nullptr, 0);

    if (object->m_obj->m_type & TYPE_ITEM) {
        if (static_cast<CGItem_C*>(object)->m_item->m_itemFieldFlags & ITEM_DFLAG_TRANSLATED) {
            SStrCopy(CGItemText::m_text, buffer, sizeof(CGItemText::m_text));
        } else {
            WOWGUID none = WOWGUID();
            ItemStats_C* stats = g_itemDBCache.GetRecord(object->m_obj->m_entryID, none, nullptr, nullptr, false);
            uint32_t skill = 0;

            if (translate) {
                CGPlayer_C* activePlayer = ClntObjMgrGetActivePlayerObj();

                if (activePlayer) {
                    activePlayer->GetLanguageSkill(stats->m_languageId, skill);
                }
            }

            CGChat::TranslateMessage(stats->m_languageId, skill, buffer, CGItemText::m_text, sizeof(CGItemText::m_text), 1, 1);
        }
    } else if (object->m_obj->m_type & TYPE_GAMEOBJECT) {
        int32_t language = static_cast<CGGameObject_C*>(object)->GetPageTextLanguage();
        uint32_t skill = 0;

        if (translate) {
            CGPlayer_C* activePlayer = ClntObjMgrGetActivePlayerObj();

            if (activePlayer) {
                activePlayer->GetLanguageSkill(language, skill);
            }
        }

        CGChat::TranslateMessage(language, skill, buffer, CGItemText::m_text, sizeof(CGItemText::m_text), 1, 1);
    }

    FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_READY, nullptr);
}

// VERIFY: 71021321 manual
// OFFSET: 0x0058A1A0
void CGItemText::SetItem(const WOWGUID& guid, int32_t a2) {
    if (guid != CGItemText::m_itemGUID) {
        CGGameUI::ClearInteractTarget(CGItemText::m_itemGUID, 0);
        CGItemText::m_itemGUID = guid;
        CGItemText::m_hasItemText = 0;
        CGItemText::m_currentPage = 0;
        CGItemText::m_pages.m_data[0] = 0;
        CGItemText::m_pages.m_data[1] = 0;
        CGItemText::m_text[0] = '\0';

        if (!CGItemText::m_itemGUID) {
            FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_CLOSED, nullptr);
            return;
        }
    } else if (!a2 && guid) {
        CGGameUI::ClearInteractTarget(CGItemText::m_itemGUID, 0);
        FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_CLOSED, nullptr);
        return;
    }

    CGObject_C* object = ClntObjMgrObjectPtr(guid, TYPE_OBJECT, __FILE__, __LINE__);

    if (!object) {
        CGItemText::m_itemGUID = WOWGUID();
        return;
    }

    int32_t page = object->GetPageTextID(CGItemText::ItemTextCallback);
    bool hasText = (object->m_obj->m_type & TYPE_ITEM) && (static_cast<CGItem_C*>(object)->m_item->m_itemFieldFlags & 0x200);
    CGItemText::m_hasItemText = hasText;

    if (!page && !hasText) {
        return;
    }

    CGItemText::m_pages.m_data[0] = page;
    WOWGUID objectGuid = object->m_obj->m_guid;
    void* record;

    if (page) {
        record = g_pageTextCache.GetRecord(CGItemText::m_pages.m_data[CGItemText::m_currentPage], objectGuid, CGItemText::ItemTextCallback, nullptr, false);
    } else {
        record = g_itemTextDBCache.GetRecord(CGItemText::m_itemGUID.m_guid, objectGuid, CGItemText::ItemTextCallback, nullptr, false);
    }

    if (!record) {
        return;
    }

    CGGameUI::SetInteractTarget(guid);
    FrameScript_SignalEvent(SCRIPT_EVENT_ITEM_TEXT_BEGIN, nullptr);

    if (!(object->m_obj->m_type & TYPE_ITEM) || CGItemText::m_hasItemText || (static_cast<CGItem_C*>(object)->m_item->m_itemFieldFlags & ITEM_DFLAG_TRANSLATED)) {
        CGItemText::DisplayText(guid, 1);
        return;
    }

    CGPlayer_C* player = ClntObjMgrGetActivePlayerObj();

    if (!player) {
        return;
    }

    CGItem_C* container = static_cast<CGItem_C*>(ClntObjMgrObjectPtr(static_cast<CGItem_C*>(object)->m_item->m_itemFieldContained, TYPE_OBJECT, __FILE__, __LINE__));

    if (!container) {
        return;
    }

    int32_t slot = container->GetBag()->FindItemSlot(object->m_obj->m_guid);

    if (slot >= 0) {
        player->ReadItem(static_cast<CGItem_C*>(object)->m_item->m_itemFieldContained, slot);
    }
}

// VERIFY: 300c9002 auto
// OFFSET: 0x0058A450
void CGItemText::ItemTextCallback(int32_t id, const WOWGUID& guid, void* param, bool success) {
    if (success && CGItemText::m_itemGUID == guid) {
        CGItemText::SetItem(guid, 1);
    }
}

// VERIFY: d0e6f233 auto
// OFFSET: 0x0058A430
void CGItemText::LeaveWorld() {
    CGItemText::SetItem(WOWGUID{ 0 }, 0);
}
