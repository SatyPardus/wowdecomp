#include "WorldActionCheck.h"
#include "NativeView.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Frame/CSimpleButton.h"
#include "Engine/Source/FrameScript/FrameScript.h"
#include "Engine/Source/ConsoleBase/ConsoleVar.h"
#include "WoW/Source/ConsoleDevice/ConsoleDevice.h"
#include "Engine/Source/Gx/Metal/CGxDeviceMetal.h"
#include "Engine/Source/ShaderEffect/ShadowCache.h"
#include "Engine/Source/Services/Texture.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/DB/Rec/SpellRangeRec.h"
#include "WoW/Source/DB/Rec/AreaTableRec.h"
#include "WoW/Source/Glue/CGlueMgr.h"
#include "WoW/Source/Glue/CharacterSelection.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/ObjectClient/Item_C.h"
#include "WoW/Source/Object/ObjectClient/GameObject_C.h"
#include "WoW/Source/Magic/MagicClient/Spell_C.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/FrameTime.h"
#include "WoW/Source/Object/ObjectClient/UnitCombatLog_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/Object/Object.h"
#include "WoW/Source/Object/ItemStats.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/GossipInfo.h"
#include "WoW/Source/Ui/MerchantFrame.h"
#include "WoW/Source/Ui/LootFrame.h"
#include "WoW/Source/Ui/QuestFrame.h"
#include "WoW/Source/Ui/QuestLog.h"
#include "WoW/Source/Ui/SpellBookFrame.h"
#include "WoW/Source/Ui/WorldFrame.h"
#include "WoW/Source/Ui/UIBindings.h"
#import <Carbon/Carbon.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <algorithm>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw NativeWorldCheckFailure(message); }
}
bool CheckNativeWorldWalk(WowNativeView* view, bool recoverCorpse) {
    static std::vector<C2Vector> points;
    static unsigned readyFrames, waypoint, settledFrames;
    static bool initialized, done, forward, left, right;
    static uint32_t map;
    static float bestDistance;
    static std::chrono::steady_clock::time_point progressed, legStarted;
    if (done) return true;
    auto key = [&](unsigned short code, NSString* text, bool value, bool& held) {
        if (held == value) return;
        NSEvent* event = [NSEvent keyEventWithType:value ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        if (value) [view keyDown:event]; else [view keyUp:event];
        held = value;
    };
    auto stop = [&] {
        key(kVK_ANSI_W, @"w", false, forward);
        key(kVK_LeftArrow, @"\uF702", false, left);
        key(kVK_RightArrow, @"\uF703", false, right);
    };
    try {
        auto* player = ClntObjMgrGetActivePlayerObj();
        if (!CGGameUI::s_inWorld || !player) {
            Require(!initialized, "World left during native walking check"); return false;
        }
        if (!initialized && ++readyFrames < 120) return false;
        if (recoverCorpse) {
            Require(player->m_player->m_playerFlags & 0x10, "Corpse return requires the original ghost state");
            Require(CGGameUI::m_corpseMapID == int32_t(ClntObjMgrGetMapID()) &&
                CGGameUI::m_corpseActualMapID == CGGameUI::m_corpseMapID, "Corpse return requires a corpse on this map");
            if (CGGameUI::m_corpseInRange) {
                stop(); done = true;
                std::fprintf(stderr, "Native corpse return: original corpse range reached, movement stopped\n");
                return true;
            }
        }
        const auto position = player->GetPosition();
        const auto now = std::chrono::steady_clock::now();
        if (!initialized) {
            Require(CGUIBindings::s_bindings, "Native walking requires existing character bindings");
            for (const auto& pair : {std::pair{"W", "MOVEFORWARD"}, {"LEFT", "TURNLEFT"}, {"RIGHT", "TURNRIGHT"}}) {
                const char* action = CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE, pair.first, 1);
                Require(action && !std::strcmp(action, pair.second), "Native walking does not match the existing movement bindings");
            }
            const char* cursor = std::getenv("WOW_WORLD_WALK");
            Require(cursor && *cursor, "Native walking requires explicit WOW_WORLD_WALK x:y waypoints");
            C2Vector previous(position.x, position.y);
            float total = 0;
            while (*cursor) {
                char* end = nullptr;
                const float x = std::strtof(cursor, &end);
                Require(end != cursor && *end == ':' && std::isfinite(x) && std::abs(x) < 17067, "Invalid native walking X coordinate");
                cursor = end + 1;
                const float y = std::strtof(cursor, &end);
                Require(end != cursor && (*end == ';' || !*end) && std::isfinite(y) && std::abs(y) < 17067, "Invalid native walking Y coordinate");
                const float length = std::hypot(x - previous.x, y - previous.y);
                total += length;
                Require(points.size() < 128 && length <= 200 && total <= 2000, "Native walking route exceeds its bounded local range");
                points.emplace_back(x, y); previous = points.back();
                cursor = *end ? end + 1 : end;
                Require(!*end || *cursor, "Native walking route has an empty final waypoint");
            }
            map = ClntObjMgrGetMapID(); initialized = true;
            bestDistance = 1e10f; progressed = legStarted = now;
            std::fprintf(stderr, "Native world walk: map %u, origin %.2f %.2f %.2f, outdoors %d, %zu waypoints, %.2f units\n", map, position.x, position.y, position.z,
                player->m_worldObject ? World::QueryOutdoors(player->m_worldObject) : -1, points.size(), total);
        }
        Require(ClntObjMgrGetMapID() == map && (recoverCorpse || !player->IsDeadOrGhost()), "Native walking map or living-player precondition changed");
        if (waypoint == points.size()) {
            stop();
            if (++settledFrames < 120) return false;
            std::fprintf(stderr, "Native world walk: completed through original movement/collision, final %.2f %.2f %.2f, outdoors %d, 120 settled frames\n", position.x, position.y, position.z,
                player->m_worldObject ? World::QueryOutdoors(player->m_worldObject) : -1);
            done = true; return true;
        }
        const auto& destination = points[waypoint];
        const float distance = std::hypot(destination.x - position.x, destination.y - position.y);
        if (distance < .45f) {
            stop();
            std::fprintf(stderr, "Native world walk: waypoint %u at %.2f %.2f %.2f, outdoors %d\n", waypoint + 1, position.x, position.y, position.z,
                player->m_worldObject ? World::QueryOutdoors(player->m_worldObject) : -1);
            ++waypoint; bestDistance = 1e10f; progressed = legStarted = now;
            return false;
        }
        if (distance < bestDistance - .05f) { bestDistance = distance; progressed = now; }
        if (std::chrono::duration<double>(now - progressed).count() > 4 || std::chrono::duration<double>(now - legStarted).count() > 60) {
            std::fprintf(stderr, "Native world walk: blocked at %.2f %.2f %.2f, waypoint %.2f %.2f, distance %.2f\n", position.x, position.y, position.z, destination.x, destination.y, distance);
            throw NativeWorldCheckFailure("Original walking made no progress; route stopped without a position override");
        }
        const float turn = std::remainder(std::atan2(destination.y - position.y, destination.x - position.x) - player->GetFacing(), 6.283185307f);
        key(kVK_LeftArrow, @"\uF702", turn > .04f, left);
        key(kVK_RightArrow, @"\uF703", turn < -.04f, right);
        key(kVK_ANSI_W, @"w", std::abs(turn) < .15f, forward);
        return false;
    } catch (...) { stop(); throw; }
}
bool CheckNativeWorldCorpse(WowNativeView* view) {
    static unsigned phase;
    static NSPoint point;
    static std::chrono::steady_clock::time_point started;
    if (phase == 4) return true;
    if (!phase) {
        if (!CheckNativeWorldWalk(view, true)) return false;
        phase = 1; started = std::chrono::steady_clock::now();
    }
    auto* player = ClntObjMgrGetActivePlayerObj();
    Require(CGGameUI::s_inWorld && player, "World left during corpse recovery");
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    if (phase == 1) {
        Require(CGGameUI::m_corpseInRange, "Original corpse recovery range was lost");
        CSimpleButton* button = nullptr;
        auto* lua = FrameScript_GetContext();
        for (unsigned i=1; i<=4; ++i) {
            char name[48]; std::snprintf(name, sizeof(name), "StaticPopup%u", i);
            auto* popup = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name, CSimpleFrame::GetObjectType()));
            if (!popup || !popup->m_isVisible) continue;
            lua_rawgeti(lua, LUA_REGISTRYINDEX, popup->GetLuaObjectRef()); lua_getfield(lua, -1, "which");
            const char* which = lua_tostring(lua, -1);
            const bool recover = which && !std::strcmp(which, "RECOVER_CORPSE"); lua_pop(lua, 2);
            if (!recover) continue;
            std::snprintf(name, sizeof(name), "StaticPopup%uButton1", i);
            button = static_cast<CSimpleButton*>(CScriptObject::GetScriptObjectByName(name, CSimpleButton::GetObjectType()));
            break;
        }
        if (!button || !button->m_isVisible || button->m_state == BUTTONSTATE_DISABLED) {
            Require(elapsed < 130, "Original corpse recovery popup did not become available"); return false;
        }
        CRect rect; Require(button->GetRect(&rect), "Corpse recovery button has no rectangle");
        float x,y; DDCToNDC((rect.minX+rect.maxX)*.5f, (rect.minY+rect.maxY)*.5f, &x, &y);
        Require(x>0 && x<1 && y>0 && y<1, "Corpse recovery button is outside the view");
        point = [view convertPoint:NSMakePoint(x*view.bounds.size.width, (view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        for (auto type : {NSEventTypeMouseMoved, NSEventTypeLeftMouseDown}) {
            NSEvent* event = [NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
            if (type == NSEventTypeMouseMoved) [view mouseMoved:event]; else [view mouseDown:event];
        }
        phase = 2; started = now;
    } else if (phase == 2 && elapsed >= .05) {
        [view mouseUp:[NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:point modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0]];
        phase = 3; started = now;
    } else if (phase == 3) {
        if (!player->IsDeadOrGhost() && player->m_unit->m_unitFieldHealth > 0) {
            const auto p = player->GetPosition();
            std::fprintf(stderr, "Native corpse recovery: original Accept button restored living server state, health %d/%d, position %.2f %.2f %.2f\n",
                player->m_unit->m_unitFieldHealth, player->m_unit->m_unitFieldMaxhealth, p.x,p.y,p.z);
            phase = 4; return true;
        }
        Require(elapsed < 10, "Original corpse recovery was not confirmed by the server");
    }
    return false;
}
bool CheckNativeWorldPanels(WowNativeView* view) {
    struct Panel { unsigned short code; const char* key; const char* action; const char* frame; };
    static constexpr Panel panels[] = {
        {kVK_ANSI_C, "C", "TOGGLECHARACTER0", "CharacterFrame"},
        {kVK_ANSI_P, "P", "TOGGLESPELLBOOK", "SpellBookFrame"},
        {kVK_ANSI_L, "L", "TOGGLEQUESTLOG", "QuestLogFrame"},
        {kVK_ANSI_N, "N", "TOGGLETALENTS", "PlayerTalentFrame"},
        {kVK_ANSI_M, "M", "TOGGLEWORLDMAP", "WorldMapFrame"}
    };
    static unsigned panel, phase, readyFrames;
    static std::chrono::steady_clock::time_point started;
    if (panel == sizeof(panels) / sizeof(panels[0])) return true;
    if (!CGGameUI::s_inWorld || !ClntObjMgrGetActivePlayerObj()) {
        Require(!panel && !phase, "World left during native UI panel check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    const auto& current = panels[panel];
    auto* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(current.frame, CSimpleFrame::GetObjectType()));
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - started).count();
    auto key = [&](bool down) {
        NSString* text = [[NSString stringWithUTF8String:current.key] lowercaseString];
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:current.code];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto next = [&] { ++phase; started = now; };
    if (!phase) {
        Require(CGUIBindings::s_bindings, "Native UI panel check requires character bindings");
        const char* action = CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE, current.key, 1);
        Require(action && !std::strcmp(action, current.action), "Native UI panel key differs from the existing binding");
        Require(!frame || !frame->m_isVisible, "Native UI panel check requires initially closed panels");
        key(true); next();
    } else if (phase == 1 && elapsed >= .05) { key(false); next(); }
    else if (phase == 2) {
        if (frame && frame->m_isVisible) {
            std::fprintf(stderr, "Native world panels: %s opened through %s\n", current.frame, current.key);
            next();
        } else Require(elapsed < 5, "Native binding did not open its original UI panel");
    } else if (phase == 3 && elapsed >= 2) {
        Require(frame && frame->m_isVisible, "Original UI panel disappeared before live drawing completed");
        key(true); next();
    } else if (phase == 4 && elapsed >= .05) { key(false); next(); }
    else if (phase == 5) {
        if (frame && !frame->m_isVisible) {
            std::fprintf(stderr, "Native world panels: %s rendered and closed through its original binding\n", current.frame);
            ++panel; phase = readyFrames = 0;
            if (panel == sizeof(panels) / sizeof(panels[0])) {
                std::fputs("Native world panels: character, spellbook, quest log, talents and world map passed\n", stderr);
                return true;
            }
        } else Require(elapsed < 5, "Native binding did not close its original UI panel");
    }
    return false;
}

bool CheckNativeWorldQuest(WowNativeView* view) {
    static unsigned phase, readyFrames;
    static WOWGUID target;
    static uint32_t coins, quests[25];
    static std::chrono::steady_clock::time_point started;
    if (phase == 5) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during native quest check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    auto* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("QuestFrame", CSimpleFrame::GetObjectType()));
    Require(frame, "Original quest frame is missing");
    auto project = [](CGGameObject_C* object, float& x, float& y) {
        const auto feet = object->GetPosition(), head = object->GetNamePosition(); C3Vector screen;
        if (!CGWorldFrame::s_currentWorldFrame->GetScreenCoordinates(C3Vector(feet.x, feet.y, feet.z + (head.z-feet.z)*.4f), &screen, nullptr)) return false;
        DDCToNDC(screen.x, screen.y, &x, &y); return x > .15f && x < .85f && y > .2f && y < .85f;
    };
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    auto next = [&] { ++phase; started = now; };
    if (!phase) {
        Require(!frame->m_isShown, "Quest check requires a closed quest dialog");
        struct Search { decltype(project)* project; CGGameObject_C* object=nullptr; } search{&project};
        ClntObjMgrEnumVisibleObjects([](WOWGUID guid, void* context) -> int32_t {
            auto& search = *static_cast<Search*>(context);
            auto* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(guid, TYPEMASK_GAMEOBJECT, __FILE__, __LINE__));
            if (!object || !object->IsQuestGiver() || !object->CanUseNow()) return 1;
            float x, y;
            if ((*search.project)(object, x, y)) { search.object = object; return 0; }
            return 1;
        }, &search);
        Require(search.object, "Quest check needs an on-screen usable quest-giver object in range");
        target = search.object->m_obj->m_guid; coins = player->GetCoinage();
        for (unsigned i=0; i<25; ++i) quests[i] = player->m_player->m_playerQuestLog[i].m_questId;
        float x, y; Require(project(search.object, x, y), "Quest-giver projection changed");
        const NSPoint point = [view convertPoint:NSMakePoint(x*view.bounds.size.width, (view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        [view mouseMoved:[NSEvent mouseEventWithType:NSEventTypeMouseMoved location:point modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:0 pressure:0]];
        std::fprintf(stderr, "Native quest: picking original %s\n", search.object->GetObjectName());
        next();
    } else if ((phase == 1 && elapsed >= .1) || (phase == 2 && elapsed >= .05)) {
        auto* object = static_cast<CGGameObject_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_GAMEOBJECT, __FILE__, __LINE__));
        float x, y; Require(object && project(object, x, y), "Quest-giver moved before native click");
        if (phase == 1) {
            auto* hit = ClntObjMgrObjectPtr(CGWorldFrame::s_currentWorldFrame->m_hitGuid, TYPEMASK_OBJECT, __FILE__, __LINE__);
            std::fprintf(stderr, "Native quest pick: %.4f %.4f, hit matches %d, hit type %u\n", x, y,
                CGWorldFrame::s_currentWorldFrame->m_hitGuid == target, hit ? hit->m_obj->m_type : 0);
            Require(CGWorldFrame::s_currentWorldFrame->m_hitGuid == target, "Native mouse did not hit the quest-giver object");
        }
        const NSPoint point = [view convertPoint:NSMakePoint(x*view.bounds.size.width, (view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        const bool down = phase == 1;
        CGEventRef event = CGEventCreateMouseEvent(nullptr, down ? kCGEventRightMouseDown:kCGEventRightMouseUp,
            CGPointMake(point.x, NSMaxY(NSScreen.screens.firstObject.frame)-point.y), kCGMouseButtonRight);
        Require(event, "Cannot create native quest right click"); NSEvent* native = [NSEvent eventWithCGEvent:event];
        if (down) [view rightMouseDown:native]; else [view rightMouseUp:native];
        CFRelease(event); next();
    } else if (phase == 3) {
        const char* panelName = CGQuestInfo::m_state == 1 ? "QuestFrameDetailPanel" :
            CGQuestInfo::m_state == 2 ? "QuestFrameProgressPanel" : "QuestFrameRewardPanel";
        const char* text = CGQuestInfo::m_state == 1 ? CGQuestInfo::m_questText :
            CGQuestInfo::m_state == 2 ? CGQuestInfo::m_progressText : CGQuestInfo::m_rewardText;
        auto* panel = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(panelName, CSimpleFrame::GetObjectType()));
        if (CGQuestInfo::m_npc == target && CGQuestInfo::m_currentQuest && CGQuestInfo::m_questTitle[0] &&
            CGQuestInfo::m_state >= 1 && CGQuestInfo::m_state <= 3 && text[0] && frame->m_isVisible && panel && panel->m_isVisible) {
            std::fprintf(stderr, "Native quest: server quest %d, %s; original %s visible\n",
                CGQuestInfo::m_currentQuest, CGQuestInfo::m_questTitle, panelName);
            next();
        } else if (elapsed >= 10) {
            std::fprintf(stderr, "Native quest result: source matches %d, interact matches %d, state %d, quest %d, available %d, in progress %d, frame visible %d\n",
                CGQuestInfo::m_npc == target, CGGameUI::m_interactTarget == target, CGQuestInfo::m_state, CGQuestInfo::m_currentQuest,
                CGQuestInfo::m_numQuests, CGQuestInfo::m_numInProgress, frame->m_isVisible);
            throw NativeWorldCheckFailure("Native quest-giver click did not display a server quest offer");
        }
    } else if (phase == 4 && elapsed >= 2) {
        Require(frame->m_isVisible && player->GetCoinage() == coins, "Quest offer disappeared or changed money");
        for (unsigned i=0; i<25; ++i)
            Require(quests[i] == player->m_player->m_playerQuestLog[i].m_questId, "Reading the quest offer changed the quest log");
        std::fputs("Native quest: server offer rendered; quest log and money unchanged; no acceptance or reward selected\n", stderr);
        next(); return true;
    }
    return false;
}

bool CheckNativeWorldReward(WowNativeView* view) {
    static unsigned phase, releasePhase, settledFrames;
    static uint32_t questID, coins, xp, level, rewardMoney, rewardXP;
    static uint32_t requestedItem, choiceIndex, rewardCount, previousCount;
    static NSPoint point;
    static bool mouseHeld;
    static bool choicePending, choiceSelected;
    static std::chrono::steady_clock::time_point started;
    if (phase == 7) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during quest reward check"); return false;
    }
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    auto advance = [&](unsigned next) { phase = next; started = now; };
    auto frame = [](const char* name) {
        return static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name, CSimpleFrame::GetObjectType()));
    };
    auto mouse = [&](NSEventType type) {
        NSEvent* event = [NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
        if (type == NSEventTypeMouseMoved) [view mouseMoved:event];
        else if (type == NSEventTypeLeftMouseDown) [view mouseDown:event];
        else [view mouseUp:event];
    };
    auto click = [&](CSimpleFrame* button, unsigned next) {
        Require(button && button->m_isVisible, "Original quest button is not visible");
        CRect rect; Require(button->GetRect(&rect), "Original quest button has no rectangle");
        float x,y; DDCToNDC((rect.minX+rect.maxX)*.5f, (rect.minY+rect.maxY)*.5f, &x,&y);
        Require(x>0 && x<1 && y>0 && y<1, "Original quest button is outside the view");
        point = [view convertPoint:NSMakePoint(x*view.bounds.size.width, (view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        mouse(NSEventTypeMouseMoved); mouse(NSEventTypeLeftMouseDown);
        mouseHeld = true; releasePhase = next; started = now;
    };
    if (mouseHeld) {
        if (elapsed >= .05) { mouse(NSEventTypeLeftMouseUp); mouseHeld = false; advance(releasePhase); }
        return false;
    }
    if (!phase) {
        if (!questID) {
            const char* value = std::getenv("WOW_WORLD_QUEST"); char* end = nullptr;
            Require(value && *value, "Quest reward check requires an explicit WOW_WORLD_QUEST ID");
            const unsigned long parsed = std::strtoul(value, &end, 10);
            Require(end != value && !*end && parsed > 0 && parsed <= INT32_MAX, "Invalid quest reward ID");
            questID = uint32_t(parsed);
            if (const char* item = std::getenv("WOW_WORLD_REWARD_ITEM")) {
                const unsigned long parsedItem = std::strtoul(item, &end, 10);
                Require(end != item && !*end && parsedItem > 0 && parsedItem <= INT32_MAX,"Invalid explicit quest reward item ID");
                requestedItem = uint32_t(parsedItem);
            }
        }
        if (!CheckNativeWorldNPC(view, false, true)) return false;
        const int32_t index = CGQuestLog::GetQuestIndex(questID);
        Require(index > 0 && CGQuestLog::IsQuestCompleteEx(index-1, 1), "Requested quest is not complete in the original log");
        coins = player->GetCoinage(); xp = player->m_player->m_playerXp; level = player->m_unit->m_unitFieldLevel;
        Require(!CGQuestLog::GetQuestIndex(INT32_MAX) && CGQuestLog::ComputeBlobQuestObjectives(INT32_MAX) == 0,
            "Absent quest retained objectives during a pending map update");
        std::fprintf(stderr,"Native quest reward: before submission, money %u, XP %u, level %u; absent quest POI rejected\n",coins,xp,level);
        advance(1);
    } else if (phase == 1 && elapsed >= 1) {
        if (CGQuestInfo::m_currentQuest == int32_t(questID) && CGQuestInfo::m_state >= 2) { advance(3); return false; }
        CSimpleFrame* button = nullptr;
        auto* gossip = frame("GossipFrame");
        if (gossip && gossip->m_isVisible) {
            int32_t index = 0;
            for (int32_t i=0; i<CGGossipInfo::GetNumActiveQuests(); ++i) {
                const auto* quest = CGGossipInfo::GetActiveQuest(i);
                if (quest && quest->m_questID == questID) { index = i+1; break; }
            }
            Require(index, "Requested completed quest is not offered by this NPC");
            auto* lua = FrameScript_GetContext();
            for (unsigned i=1; i<=32; ++i) {
                char name[40]; std::snprintf(name,sizeof(name),"GossipTitleButton%u",i);
                auto* candidate = frame(name);
                if (!candidate || !candidate->m_isVisible || candidate->m_id != uint32_t(index)) continue;
                lua_rawgeti(lua,LUA_REGISTRYINDEX,candidate->GetLuaObjectRef()); lua_getfield(lua,-1,"type");
                const char* type = lua_tostring(lua,-1); const bool active = type && !std::strcmp(type,"Active"); lua_pop(lua,2);
                if (active) { button = candidate; break; }
            }
        } else {
            for (int32_t i=0; i<CGQuestInfo::m_numInProgress; ++i) {
                if (CGQuestInfo::m_inProgressQuests[i].m_questID != int32_t(questID)) continue;
                char name[40]; std::snprintf(name,sizeof(name),"QuestTitleButton%d",i+1); button = frame(name); break;
            }
        }
        click(button,2);
        std::fprintf(stderr,"Native quest reward: selected completed quest %u through its original title button\n",questID);
    } else if (phase == 2) {
        if (CGQuestInfo::m_currentQuest == int32_t(questID) && (CGQuestInfo::m_state == 2 || CGQuestInfo::m_state == 3)) advance(3);
        else Require(elapsed < 10,"Server did not open the requested completed quest");
    } else if (phase == 3 && elapsed >= 2) {
        Require(CGQuestInfo::m_currentQuest == int32_t(questID),"Quest changed before completion");
        if (CGQuestInfo::m_state == 3) { advance(4); return false; }
        auto* button = static_cast<CSimpleButton*>(CScriptObject::GetScriptObjectByName("QuestFrameCompleteButton",CSimpleButton::GetObjectType()));
        Require(CGQuestInfo::m_state == 2 && CGQuestInfo::IsCompletable() && CGQuestInfo::m_requiredMoney <= 0 &&
            button && button->m_state != BUTTONSTATE_DISABLED,"Original quest progress button is not available without payment");
        click(button,4);
    } else if (phase == 4) {
        if (CGQuestInfo::m_currentQuest == int32_t(questID) && CGQuestInfo::m_state == 3) {
            Require(CGQuestInfo::m_rewardMoney >= 0,"Quest reward check does not pay money");
            const int32_t choices = CGQuestInfo::GetNumQuestChoices();
            if (choices) {
                Require(requestedItem && choices <= 6,"A choice reward requires an explicit WOW_WORLD_REWARD_ITEM ID");
                for (int32_t i=0; i<choices; ++i) {
                    const auto& item = CGQuestInfo::m_questItems[i].m_choice;
                    if (item.m_itemID == int32_t(requestedItem)) { choiceIndex = i+1; rewardCount = item.m_count; break; }
                }
                Require(choiceIndex && rewardCount,"Requested item is not a server quest choice");
                previousCount = player->m_bag.GetItemTypeCount(requestedItem,0x6);
            } else Require(!requestedItem,"Requested item is absent from this quest's rewards");
            rewardMoney = CGQuestInfo::m_rewardMoney; rewardXP = CGQuestInfo::m_rewardXP;
            auto* panel = frame("QuestFrameRewardPanel"); Require(panel && panel->m_isVisible,"Original reward panel is missing");
            std::fprintf(stderr,"Native quest reward: server offers %s, %u copper, %u XP, choice item %u x%u\n",CGQuestInfo::m_questTitle,rewardMoney,rewardXP,requestedItem,rewardCount);
            advance(5);
        } else Require(elapsed < 10,"Server did not open the original reward panel");
    } else if (phase == 5 && elapsed >= 2) {
        Require(CGQuestInfo::m_currentQuest == int32_t(questID) && CGQuestInfo::m_state == 3 &&
            player->GetCoinage() == coins && player->m_player->m_playerXp == xp,"Quest reward state changed before submission");
        if (choiceIndex && !choiceSelected) {
            auto* lua = FrameScript_GetContext();
            if (choicePending) {
                auto* info = frame("QuestInfoFrame"); Require(info,"Original quest info frame is missing");
                lua_rawgeti(lua,LUA_REGISTRYINDEX,info->GetLuaObjectRef()); lua_getfield(lua,-1,"itemChoice");
                const bool selected = lua_tointeger(lua,-1) == choiceIndex; lua_pop(lua,2);
                Require(selected,"Native click did not select the requested original reward item");
                choiceSelected = true;
            } else {
                CSimpleFrame* choice = nullptr;
                for (unsigned i=1; i<=10; ++i) {
                    char name[40]; std::snprintf(name,sizeof(name),"QuestInfoItem%u",i); auto* candidate = frame(name);
                    if (!candidate || !candidate->m_isVisible || candidate->m_id != choiceIndex) continue;
                    lua_rawgeti(lua,LUA_REGISTRYINDEX,candidate->GetLuaObjectRef()); lua_getfield(lua,-1,"type");
                    const char* type = lua_tostring(lua,-1); const bool isChoice = type && !std::strcmp(type,"choice"); lua_pop(lua,2);
                    if (isChoice) { choice = candidate; break; }
                }
                click(choice,5); choicePending = true; return false;
            }
        }
        auto* button = static_cast<CSimpleButton*>(CScriptObject::GetScriptObjectByName("QuestFrameCompleteQuestButton",CSimpleButton::GetObjectType()));
        Require(button && button->m_state != BUTTONSTATE_DISABLED,"Original quest reward button is disabled");
        click(button,6);
    } else if (phase == 6) {
        bool present = false;
        for (const auto& quest : player->m_player->m_playerQuestLog) present |= quest.m_questId == questID;
        const bool experience = player->m_unit->m_unitFieldLevel > level || uint64_t(player->m_player->m_playerXp) >= uint64_t(xp)+rewardXP;
        const bool itemReceived = !requestedItem || player->m_bag.GetItemTypeCount(requestedItem,0x6) == previousCount+rewardCount;
        if (!present && !CGQuestLog::GetQuestIndex(questID) && uint64_t(player->GetCoinage()) == uint64_t(coins)+rewardMoney && experience && itemReceived) {
            if (++settledFrames < 120) return false;
            std::fprintf(stderr,"Native quest reward: server removed quest %u, money %u -> %u, XP %u -> %u, level %u -> %u, item %u count %u -> %u; original buttons and 120 settled frames passed\n",
                questID,coins,player->GetCoinage(),xp,player->m_player->m_playerXp,level,player->m_unit->m_unitFieldLevel,requestedItem,previousCount,
                requestedItem ? player->m_bag.GetItemTypeCount(requestedItem,0x6):0);
            advance(7); return true;
        }
        Require(elapsed < 15,"Quest reward was not confirmed in the server quest log, money and XP");
    }
    return false;
}

bool CheckNativeWorldInventory(WowNativeView* view) {
    static unsigned phase, readyFrames, source, destination;
    static WOWGUID slots[16], item;
    static uint32_t coins, counts[16];
    static NSPoint point;
    static std::chrono::steady_clock::time_point started;
    if (phase == 17) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during native inventory check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    auto frame = [](const char* name) { return static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name, CSimpleFrame::GetObjectType())); };
    auto* bag = frame("ContainerFrame1"); Require(bag, "Original backpack frame is missing");
    auto slotItem = [&](unsigned slot) { return player->GetBag()->GetItemPointer(slot + 22); };
    auto guid = [&](unsigned slot) { auto* object = slotItem(slot); return object ? object->m_obj->m_guid : WOWGUID(); };
    auto key = [&](bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
            characters:@"b" charactersIgnoringModifiers:@"b" isARepeat:NO keyCode:kVK_ANSI_B];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto mouse = [&](NSEventType type) {
        NSEvent* event = [NSEvent mouseEventWithType:type location:point modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
        if (type == NSEventTypeMouseMoved) [view mouseMoved:event];
        else if (type == NSEventTypeLeftMouseDown) [view mouseDown:event]; else [view mouseUp:event];
    };
    auto selectSlot = [&](unsigned slot) {
        Require(bag->m_isVisible && bag->m_id == 0, "Original backpack is not visible");
        CSimpleFrame* button = nullptr;
        for (unsigned i=1; i<=16; ++i) {
            char name[48]; std::snprintf(name, sizeof(name), "ContainerFrame1Item%u", i);
            auto* candidate = frame(name);
            if (candidate && candidate->m_isVisible && candidate->m_id == int32_t(slot)) { button = candidate; break; }
        }
        CRect rect; Require(button && button->GetRect(&rect), "Inventory slot has no visible original button");
        float x, y; DDCToNDC((rect.minX+rect.maxX)*.5f, (rect.minY+rect.maxY)*.5f, &x, &y);
        Require(x>0 && x<1 && y>0 && y<1, "Inventory slot is outside the game view");
        point = [view convertPoint:NSMakePoint(x*view.bounds.size.width, (view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        mouse(NSEventTypeMouseMoved);
    };
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    auto next = [&] { ++phase; started = now; };
    if (!phase) {
        Require(CGUIBindings::s_bindings && !CGGameUI::m_cursorItemType && !bag->m_isShown, "Inventory check requires a closed backpack and empty cursor");
        const char* action = CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE, "B", 1);
        Require(action && !std::strcmp(action, "TOGGLEBACKPACK"), "Inventory check differs from the current B binding");
        unsigned alternate = 0;
        for (unsigned slot=1; slot<=16; ++slot) {
            slots[slot-1] = guid(slot); auto* object = slotItem(slot);
            counts[slot-1] = object ? object->m_item->m_itemFieldStackCount : 0;
            if (!object) destination = slot;
            else if (object->m_obj->m_entryID == 6948 && !(object->m_itemFlags & 1)) { item = object->m_obj->m_guid; source = slot; }
            else if (object->m_obj->m_entryID != 6948 && !(object->m_itemFlags & 1)) alternate = slot;
        }
        if (!destination) destination = alternate;
        Require(source && destination, "Inventory check requires an unlocked Hearthstone and another available backpack slot");
        std::fprintf(stderr, "Native inventory: existing Hearthstone slot %u; destination %u is %s\n", source, destination,
            slots[destination-1] ? "occupied (swap and restore)":"empty");
        coins = player->GetCoinage(); key(true); next();
    } else if (phase == 1 && elapsed >= .05) { key(false); next(); }
    else if (phase == 2 && elapsed >= .3) { selectSlot(source); next(); }
    else if ((phase == 3 || phase == 6 || phase == 9 || phase == 12) && elapsed >= .2) { mouse(NSEventTypeLeftMouseDown); next(); }
    else if ((phase == 4 || phase == 7 || phase == 10 || phase == 13) && elapsed >= .05) { mouse(NSEventTypeLeftMouseUp); next(); }
    else if (phase == 5 || phase == 11) {
        if (CGGameUI::GetCursorItem() == item) { selectSlot(phase == 5 ? destination:source); next(); }
        else Require(elapsed < 3, "Original bag click did not pick up the existing item cursor");
    } else if (phase == 8) {
        auto* moved = slotItem(destination);
        if (guid(source) == slots[destination-1] && guid(destination) == item && moved && !(moved->m_itemFlags & 1) && !CGGameUI::m_cursorItemType) {
            std::fprintf(stderr, "Native inventory: server moved existing Hearthstone from backpack %u to %u\n", source, destination);
            selectSlot(destination); next();
        } else Require(elapsed < 8, "Server did not acknowledge the native backpack move");
    } else if (phase == 14) {
        bool restored = !CGGameUI::m_cursorItemType;
        for (unsigned slot=1; slot<=16; ++slot) {
            const auto* object = slotItem(slot);
            restored &= guid(slot) == slots[slot-1] &&
                (object ? object->m_item->m_itemFieldStackCount : 0) == counts[slot-1];
        }
        if (restored && slotItem(source) && !(slotItem(source)->m_itemFlags & 1)) {
            Require(player->GetCoinage() == coins, "Inventory move changed player money");
            std::fputs("Native inventory: server restored all 16 backpack slots and counts; original cursor cleared and money unchanged\n", stderr);
            key(true); next();
        } else Require(elapsed < 8, "Server did not restore the original backpack layout");
    } else if (phase == 15 && elapsed >= .05) { key(false); next(); }
    else if (phase == 16) {
        if (!bag->m_isVisible) { next(); return true; }
        Require(elapsed < 3, "Original backpack did not close after inventory restoration");
    }
    return false;
}

bool CheckNativeWorldActions(WowNativeView* view) {
    static unsigned readyFrames, phase, button;
    static int32_t spell;
    static int32_t initialAuraEnd;
    static uint32_t mana;
    static NSPoint buffPoint;
    static std::chrono::steady_clock::time_point started;
    if (phase == 12) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during native action check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    auto key = [&](unsigned short code, NSString* text, bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto binding = [&](const char* name, const char* expected) {
        Require(CGUIBindings::s_bindings != nullptr, "Native action check requires loaded character bindings");
        const char* action = CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE, name, 1);
        Require(action && !std::strcmp(action, expected), "Native action check does not match the character's current bindings");
    };
    auto rightButton = [&](bool down) {
        // NSEvent's mouseEventWithType constructor leaves buttonNumber at zero
        // even for RightMouseDown/Up. CGEvent carries the actual button identity.
        CGEventRef event = CGEventCreateMouseEvent(nullptr, down ? kCGEventRightMouseDown : kCGEventRightMouseUp,
            CGPointMake(buffPoint.x, NSMaxY(NSScreen.screens.firstObject.frame) - buffPoint.y), kCGMouseButtonRight);
        Require(event != nullptr, "Cannot create native buff right-click event");
        NSEvent* native = [NSEvent eventWithCGEvent:event];
        Require(native.buttonNumber == 1 && NSPointInRect([view convertPoint:native.locationInWindow fromView:nil],view.bounds),
            "Synthetic right click does not match the native buff button");
        if (down) [view rightMouseDown:native]; else [view rightMouseUp:native];
        CFRelease(event);
    };
    auto bag = [&]() {
        auto* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("ContainerFrame1", CSimpleFrame::GetObjectType()));
        Require(frame != nullptr, "Original backpack frame is missing"); return frame;
    };
    const unsigned short codes[] = {kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6,
        kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9, kVK_ANSI_0, kVK_ANSI_Minus, kVK_ANSI_Equal};
    NSString* const texts[] = {@"1", @"2", @"3", @"4", @"5", @"6", @"7", @"8", @"9", @"0", @"-", @"="};
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - started).count();
    auto next = [&] { ++phase; started = now; };
    if (!phase) {
        binding("F1", "TARGETSELF"); binding("B", "TOGGLEBACKPACK");
        Require(!CGActionBar::m_bonusBarOffset && !CGActionBar::m_tempPageActiveFlags && CGActionBar::m_currentPage < 6,
            "Native action check requires a regular action bar");
        SpellRec base;
        Require(g_spellDB.GetRecord(1126, &base), "Original Mark of the Wild spell record is missing");
        const unsigned first = CGActionBar::m_currentPage * 12;
        for (unsigned i = 0; i < 12; ++i) {
            if (!CGActionBar::IsSpell(first + i)) continue;
            int32_t pet = 0; const int32_t id = CGActionBar::GetSpell(first + i, pet); SpellRec candidate;
            std::fprintf(stderr, "Native world actions: button %u has spell %d\n", i + 1, id);
            if (!pet && g_spellDB.GetRecord(id, &candidate) && !std::strcmp(candidate.m_name, base.m_name)) {
                spell = id; button = i;
            }
        }
        Require(spell != 0, "Native action check requires Mark of the Wild on the current action bar");
        char command[32]; std::snprintf(command, sizeof(command), "ACTIONBUTTON%u", button + 1);
        binding(texts[button].UTF8String, command);
        const auto* aura = player->GetAura(player->GetAuraIndex(spell));
        initialAuraEnd = aura ? aura->m_endTime : 0;
        Require(!bag()->m_isShown, "Close the backpack before checking native actions");
        key(kVK_F1, @"\uF704", true); next();
    } else if (phase == 1 && elapsed >= 0.05) { key(kVK_F1, @"\uF704", false); next(); }
    else if (phase == 2 && elapsed >= 0.3) {
        Require(CGGameUI::m_lockedTarget == ClntObjMgrGetActivePlayer(), "Native F1 did not select the player");
        mana = player->m_unit->m_unitFieldPower[0];
        key(codes[button], texts[button], true); next();
    } else if (phase == 3 && elapsed >= 0.05) { key(codes[button], texts[button], false); next(); }
    else if (phase == 4) {
        const int32_t index = player->GetAuraIndex(spell);
        if (index >= 0 && player->HasAuraActive(spell) && player->GetAura(index)->m_endTime != initialAuraEnd &&
            player->m_unit->m_unitFieldPower[0] < mana) {
            Require(player->GetAuraCaster(index) == ClntObjMgrGetActivePlayer().m_guid, "Buff aura has a different caster");
            const auto* aura = player->GetAura(index);
            Require(aura && aura->m_duration > 0, "Server buff aura has no duration");
            std::fprintf(stderr, "Native world actions: self-target spell %d, server aura %d ms, mana %u -> %u\n",
                spell, aura->m_duration, mana, player->m_unit->m_unitFieldPower[0]);
            key(kVK_ANSI_B, @"b", true); next();
        } else Require(elapsed < 5, "Native action button did not produce the server buff aura and mana change");
    } else if (phase == 5 && elapsed >= 0.05) { key(kVK_ANSI_B, @"b", false); next(); }
    else if (phase == 6 && elapsed >= 1) {
        Require(bag()->m_isShown, "Native B did not open the original backpack");
        key(kVK_ANSI_B, @"b", true); next();
    } else if (phase == 7 && elapsed >= 0.05) { key(kVK_ANSI_B, @"b", false); next(); }
    else if (phase == 8 && elapsed >= 0.3) {
        Require(!bag()->m_isShown, "Native B did not close the original backpack"); next();
    } else if (phase == 9) {
        CSimpleFrame* frame = nullptr;
        for (unsigned i = 0; i < 32; ++i) {
            const uint32_t slot = player->GetSortedAuraSlot(i, nullptr, nullptr, 1);
            if (slot == UINT32_MAX || player->GetAuraSpellId(slot) != spell) continue;
            char name[32]; std::snprintf(name, sizeof(name), "BuffButton%u", i + 1);
            frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name, CSimpleFrame::GetObjectType()));
            break;
        }
        Require(frame && frame->m_isShown, "Server buff has no original UI icon");
        const bool consolidated = !frame->m_isVisible;
        if (consolidated) {
            frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("ConsolidatedBuffs", CSimpleFrame::GetObjectType()));
            Require(frame && frame->m_isVisible && elapsed < 3, "Original consolidated buff icon did not become visible");
        }
        CRect rect; Require(frame->GetRect(&rect), "Original buff icon has no rectangle");
        float x, y; DDCToNDC((rect.minX + rect.maxX) * .5f, (rect.minY + rect.maxY) * .5f, &x, &y);
        Require(x > 0 && x < 1 && y > 0 && y < 1, "Original buff icon is outside the game view");
        buffPoint = [view convertPoint:NSMakePoint(x * view.bounds.size.width, (view.isFlipped ? 1-y : y) * view.bounds.size.height) toView:nil];
        NSEvent* move = [NSEvent mouseEventWithType:NSEventTypeMouseMoved location:buffPoint modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:0 pressure:0];
        [view mouseMoved:move];
        if (!consolidated) {
            rightButton(true);
            next();
        }
    } else if (phase == 10 && elapsed >= 0.05) {
        rightButton(false);
        next();
    } else if (phase == 11) {
        if (!player->HasAuraBySpellId(spell)) {
            std::fputs("Native world actions: self target, original action-bar cast, server mana/aura/cancellation and backpack open/close passed\n", stderr);
            next(); return true;
        }
        Require(elapsed < 5, "Original buff icon did not cancel the server aura");
    }
    return false;
}

bool CheckNativeWorldShadows() {
    static unsigned phase, frames;
    static int original;
    static constexpr int modes[] = {0, 1, 2, 3, 4, 5, 0, 3};
    if (phase == 10) return true;
    if (!CGGameUI::s_inWorld || !ClntObjMgrGetActivePlayerObj()) {
        Require(!phase, "World left during live shadow-quality check"); return false;
    }
    auto* setting = CVar::LookupRegistered("extShadowQuality");
    Require(setting != nullptr, "Original shadow-quality setting is missing");
    if (++frames < 120) return false;
    if (!phase) original = setting->m_intValue;
    else {
        const int mode = phase == 9 ? original : modes[phase - 1];
        Require(setting->m_intValue == mode && CShadowCache::GetShadowMode() == mode && !CShadowCache::s_recreateTextures,
            "Original shadow setting did not recreate its active cache");
        const unsigned size = mode == 2 || mode >= 4 ? 2048 : 1024;
        auto texture = [&](CTexture* value, bool present) {
            Require(bool(value) == present, "Shadow-quality change retained the wrong texture set");
            if (value) Require(value->m_gxWidth == size && value->m_gxHeight == size && value->m_gxTexFormat == GxTex_R32F,
                "Shadow-quality change used the wrong texture size/format");
        };
        texture(CShadowCache::s_extDynShadow.m_textures[0], mode != 0);
        texture(CShadowCache::s_intDynShadow, mode != 0);
        for (const auto& band : CShadowCache::s_extShadowBands) {
            texture(band.m_textures[0], mode > 2);
            texture(band.m_textures[1], mode > 2 && mode < 5);
        }
        std::fprintf(stderr, "Native world shadows: quality %d, %u-pixel cache, 120 rendered frames passed\n", mode, size);
        if (phase == 9) {
            ++phase;
            std::fputs("Native world shadows: all six original qualities, disable/recreate and original setting restoration passed\n", stderr);
            return true;
        }
    }
    const int next = phase == 8 ? original : modes[phase];
    Require(setting->Set(next, true, false, false, false), "Original shadow-quality callback rejected the requested mode");
    ++phase; frames = 0;
    return false;
}

bool CheckNativeWorldMultisample() {
    static unsigned phase, frames;
    static uint32_t original;
    static constexpr uint32_t modes[] = {2,4,1,4};
    if (phase == 6) return true;
    if (!CGGameUI::s_inWorld || !ClntObjMgrGetActivePlayerObj()) {
        Require(!phase,"World left during MSAA display check"); return false;
    }
    auto* setting=CVar::LookupRegistered("gxMultisample");
    Require(setting,"Original multisample setting is missing");
    if (++frames<120) return false;
    if (!phase) original=setting->m_intValue;
    else {
        const uint32_t expected=phase==5 ? original:modes[phase-1];
        Require(setting->m_intValue==expected && g_theGxDevicePtr->m_format.m_multisampleCount==expected &&
            s_lastGoodFormat.m_multisampleCount==expected,"Original MSAA restart did not apply the selected sample count");
        std::fprintf(stderr,"Native world MSAA: %ux, 120 rendered frames passed\n",expected);
        if (phase==5) {
            ++phase; std::fputs("Native world MSAA: 2x/4x/off/recreate and original setting restoration passed\n",stderr); return true;
        }
    }
    const uint32_t next=phase==4 ? original:modes[phase];
    Require(setting->Set(next,true,false,false,false),"Original MSAA setting rejected requested count");
    Require(CCGxRestart(nullptr,nullptr),"Original multisample display restart failed");
    ++phase; frames=0; return false;
}

bool CheckNativeWorldNPC(WowNativeView* view, bool browseMerchant, bool questGiverOnly) {
    static unsigned phase,readyFrames;
    static WOWGUID target;
    static uint32_t coins;
    static NSPoint optionPoint;
    static std::chrono::steady_clock::time_point started;
    if(phase==7 || (phase==4 && !browseMerchant)) return true;
    auto* player=ClntObjMgrGetActivePlayerObj();
    if(!CGGameUI::s_inWorld || !player) { Require(!phase,"World left during native NPC check"); return false; }
    if(!phase && ++readyFrames<120) return false;
    auto* frame=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("GossipFrame",CSimpleFrame::GetObjectType()));
    Require(frame,"Original NPC gossip frame is missing");
    auto project=[](CGUnit_C* unit,float& x,float& y) {
        const auto feet=unit->GetPosition(),head=unit->GetNamePosition(); C3Vector screen;
        if(!CGWorldFrame::s_currentWorldFrame->GetScreenCoordinates(C3Vector(feet.x,feet.y,feet.z+(head.z-feet.z)*.4f),&screen,nullptr)) return false;
        DDCToNDC(screen.x,screen.y,&x,&y); return x>.15f && x<.85f && y>.2f && y<.85f;
    };
    const auto now=std::chrono::steady_clock::now();
    const double elapsed=std::chrono::duration<double>(now-started).count();
    auto next=[&] { ++phase; started=now; };
    if(!phase) {
        Require(!frame->m_isShown,"NPC check requires a closed gossip dialog");
        struct Search { CGPlayer_C* player; decltype(project)* project; uint32_t flag; CGUnit_C* unit=nullptr; float distanceSquared=25; } search{player,&project,questGiverOnly ? 2u:1u};
        ClntObjMgrEnumVisibleObjects([](WOWGUID guid,void* context)->int32_t {
            auto& search=*static_cast<Search*>(context);
            auto* unit=static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid,TYPEMASK_UNIT,__FILE__,__LINE__));
            if(!unit || unit==search.player || (unit->m_obj->m_type&TYPEMASK_PLAYER) || !(unit->m_unit->m_unitNpcFlags&search.flag) ||
               unit->IsDeadOrGhost() || !search.player->CanInteract(unit)) return 1;
            const auto d=unit->GetPosition()-search.player->GetPosition(); const float distance=d.x*d.x+d.y*d.y+d.z*d.z; float x,y;
            if(distance<search.distanceSquared && (*search.project)(unit,x,y)) { search.unit=unit; search.distanceSquared=distance; }
            return 1;
        },&search);
        Require(search.unit,"Native NPC check requires an on-screen gossip NPC within five units");
        target=search.unit->m_obj->m_guid;
        std::fprintf(stderr,"Native NPC: picking %s, flags %u, squared distance %.3f\n",search.unit->GetObjectName(),search.unit->m_unit->m_unitNpcFlags,search.distanceSquared);
        float x,y; Require(project(search.unit,x,y),"NPC left the world view");
        const NSPoint p=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        [view mouseMoved:[NSEvent mouseEventWithType:NSEventTypeMouseMoved location:p modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:0 pressure:0]];
        next();
    } else if((phase==1 && elapsed>=.1) || (phase==2 && elapsed>=.05)) {
        auto* unit=static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target,TYPEMASK_UNIT,__FILE__,__LINE__)); float x,y;
        Require(unit && project(unit,x,y),"NPC moved before native right click");
        const NSPoint p=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        const bool down=phase==1;
        CGEventRef event=CGEventCreateMouseEvent(nullptr,down ? kCGEventRightMouseDown:kCGEventRightMouseUp,
            CGPointMake(p.x,NSMaxY(NSScreen.screens.firstObject.frame)-p.y),kCGMouseButtonRight);
        Require(event,"Cannot create native NPC right click"); NSEvent* native=[NSEvent eventWithCGEvent:event];
        if(down) [view rightMouseDown:native]; else [view rightMouseUp:native]; CFRelease(event); next();
    } else if(phase==3) {
        auto* questFrame=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("QuestFrame",CSimpleFrame::GetObjectType()));
        const bool gossip=CGGossipInfo::m_gossip==target && CGGossipInfo::m_gossipText[0] && frame->m_isVisible &&
            (CGGossipInfo::m_gossipOptionsCount || CGGossipInfo::GetNumActiveQuests() || CGGossipInfo::GetNumAvailableQuests());
        const bool quest=questGiverOnly && CGQuestInfo::m_npc==target && questFrame && questFrame->m_isVisible;
        if(gossip || quest) {
            Require(CGGameUI::m_lockedTarget==target,"Native NPC click did not select the original target");
            std::fprintf(stderr,"Native NPC: original target and server %s displayed; %u gossip options, %d active quests; no option selected\n",
                quest ? "quest dialog":"gossip",CGGossipInfo::m_gossipOptionsCount,CGGossipInfo::GetNumActiveQuests());
            next(); return !browseMerchant;
        }
        Require(elapsed<8,"Native NPC right click did not display the original server gossip");
    } else if(phase==4 && elapsed>=.3) {
        unsigned option=0;
        for(unsigned i=0;i<32;++i) {
            const auto& candidate=CGGossipInfo::m_options[i];
            if(candidate.m_index>=0 && candidate.m_type==1 && !candidate.m_coded && !candidate.m_boxMoney && !candidate.m_boxText) { option=i+1; break; }
        }
        Require(option,"NPC check requires a free, uncoded vendor gossip option");
        CSimpleFrame* button=nullptr;
        auto* lua=FrameScript_GetContext();
        for(unsigned i=1;i<=32;++i) {
            char name[40]; std::snprintf(name,sizeof(name),"GossipTitleButton%u",i);
            auto* candidate=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name,CSimpleFrame::GetObjectType()));
            if(!candidate || !candidate->m_isVisible || candidate->m_id!=option) continue;
            lua_rawgeti(lua,LUA_REGISTRYINDEX,candidate->GetLuaObjectRef()); lua_getfield(lua,-1,"type");
            const char* type=lua_tostring(lua,-1); const bool gossip=type && !std::strcmp(type,"Gossip"); lua_pop(lua,2);
            if(gossip) { button=candidate; break; }
        }
        Require(button,"Original vendor gossip button is not visible");
        CRect rect; Require(button->GetRect(&rect),"Vendor gossip button has no rectangle");
        float x,y; DDCToNDC((rect.minX+rect.maxX)*.5f,(rect.minY+rect.maxY)*.5f,&x,&y);
        Require(x>0 && x<1 && y>0 && y<1,"Vendor gossip button is outside the view");
        optionPoint=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        coins=player->GetCoinage();
        for(auto type:{NSEventTypeMouseMoved,NSEventTypeLeftMouseDown}) {
            NSEvent* event=[NSEvent mouseEventWithType:type location:optionPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
                windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
            if(type==NSEventTypeMouseMoved) [view mouseMoved:event]; else [view mouseDown:event];
        }
        next();
    } else if(phase==5 && elapsed>=.05) {
        [view mouseUp:[NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:optionPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0]];
        next();
    } else if(phase==6) {
        auto* merchant=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("MerchantFrame",CSimpleFrame::GetObjectType()));
        if(CGMerchantInfo::m_merchant==target && CGMerchantInfo::m_numMerchantItems>0 && !CGMerchantInfo::m_pendingItemStats && merchant && merchant->m_isVisible) {
            Require(player->GetCoinage()==coins,"Browsing the merchant changed the player's money");
            std::fprintf(stderr,"Native NPC: vendor gossip selected through original UI; %d server items loaded, merchant visible, money unchanged\n",CGMerchantInfo::m_numMerchantItems);
            next(); return true;
        }
        Require(elapsed<12,"Original merchant list did not finish loading");
    }
    return false;
}

bool CheckNativeWorldTravel() {
    static unsigned phase,readyFrames,destinationFrames;
    static uint32_t oldMap;
    static WOWGUID character;
    static C3Vector origin;
    static std::chrono::steady_clock::time_point started;
    if(phase==4) return true;
    const auto now=std::chrono::steady_clock::now();
    const double elapsed=std::chrono::duration<double>(now-started).count();
    auto* player=ClntObjMgrGetActivePlayerObj();
    if(!CGGameUI::s_inWorld || !player) {
        Require(!phase || (phase>=2 && elapsed<90),"World left before the original teleport cast completed"); return false;
    }
    if(!phase && ++readyFrames<120) return false;
    auto next=[&] { ++phase; started=now; };
    if(!phase) {
        const int32_t slot=GetSpellSlot(18960,0);
        Require(slot>=0 && GetSpellIdBySlot(slot,0)==18960,"Travel check requires an already learned Teleport: Moonglade");
        Require(ClntObjMgrGetMapID()==0,"Travel check starts on the Eastern Kingdoms");
        Require(CGGameUI::GetCursorSpell()<=0,"Travel check requires an empty spell cursor");
        oldMap=ClntObjMgrGetMapID(); character=ClntObjMgrGetActivePlayer(); origin=player->GetPosition();
        std::fprintf(stderr,"Native world travel: learned spell 18960, original book slot %d, map %u, origin %.2f %.2f %.2f\n",slot,oldMap,origin.x,origin.y,origin.z);
        CGSpellBook::CastSpell(slot,0,character); next();
    } else if(phase==1) {
        auto* bar=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("CastingBarFrame",CSimpleFrame::GetObjectType()));
        if(player->m_castingSpellId==18960 && bar && bar->m_isVisible) { std::fputs("Native world travel: original teleport cast bar visible\n",stderr); next(); }
        else Require(elapsed<5,"Known teleport spell did not start its original cast");
    } else if(phase==2) {
        if(ClntObjMgrGetMapID()!=oldMap) {
            Require(ClntObjMgrGetMapID()==1 && ClntObjMgrGetActivePlayer()==character,"Server teleport reached an unexpected map or character"); next();
        } else Require(elapsed<90,"Original teleport did not complete the server map transfer");
    } else if(phase==3 && ++destinationFrames>=180) {
        const auto position=player->GetPosition();
        std::fprintf(stderr,"Native world travel: map %u -> %u, destination %.2f %.2f %.2f, same character and 180 destination frames\n",oldMap,ClntObjMgrGetMapID(),position.x,position.y,position.z);
        next(); return true;
    }
    return false;
}

bool CheckNativeWorldHearthstone(WowNativeView* view) {
    static unsigned phase, readyFrames, settledFrames, slot;
    static uint32_t oldMap, bindArea;
    static C3Vector origin;
    static WOWGUID hearthstone;
    static NSPoint itemPoint;
    static std::chrono::steady_clock::time_point started;
    if (phase == 8) return true;
    const auto now=std::chrono::steady_clock::now();
    const double elapsed=std::chrono::duration<double>(now-started).count();
    auto* player=ClntObjMgrGetActivePlayerObj();
    if(!CGGameUI::s_inWorld || !player) {
        Require(!phase || (phase>=5 && elapsed<60),"World left before Hearthstone travel"); return false;
    }
    if(!phase && ++readyFrames<120) return false;
    auto frame=[](const char* name) { return static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name,CSimpleFrame::GetObjectType())); };
    auto key=[&](bool down) {
        NSEvent* event=[NSEvent keyEventWithType:down ? NSEventTypeKeyDown:NSEventTypeKeyUp location:NSZeroPoint modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
            characters:@"b" charactersIgnoringModifiers:@"b" isARepeat:NO keyCode:kVK_ANSI_B];
        if(down) [view keyDown:event]; else [view keyUp:event];
    };
    auto click=[&](bool down) {
        CGEventRef event=CGEventCreateMouseEvent(nullptr,down ? kCGEventRightMouseDown:kCGEventRightMouseUp,
            CGPointMake(itemPoint.x,NSMaxY(NSScreen.screens.firstObject.frame)-itemPoint.y),kCGMouseButtonRight);
        Require(event!=nullptr,"Cannot create native item click"); NSEvent* native=[NSEvent eventWithCGEvent:event];
        if(down) [view rightMouseDown:native]; else [view rightMouseUp:native]; CFRelease(event);
    };
    auto next=[&] { ++phase; started=now; };
    if(!phase) {
        Require(CGUIBindings::s_bindings,"Travel check requires character bindings");
        const char* action=CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE,"B",1);
        Require(action && !std::strcmp(action,"TOGGLEBACKPACK"),"Travel check does not match backpack binding");
        auto* bag=frame("ContainerFrame1"); Require(bag && !bag->m_isShown,"Travel check requires a closed backpack");
        CGItem_C* item=nullptr;
        for(unsigned i=23;i<=38;++i) {
            auto* candidate=player->GetBag()->GetItemPointer(i);
            if(candidate && candidate->m_obj->m_entryID==6948) { item=candidate; slot=i-22; break; }
        }
        Require(item!=nullptr,"Travel check requires the existing Hearthstone in the backpack");
        uint32_t duration=0,start=0,enable=0; Spell_C_GetItemCooldown(item,&duration,&start,&enable);
        Require(!duration || !start,"Existing Hearthstone is on cooldown");
        hearthstone=item->m_obj->m_guid; oldMap=ClntObjMgrGetMapID(); origin=player->GetPosition(); bindArea=CGPlayer_C::GetBindAreaID();
        const auto* area=g_areaTableDB.GetRecord(bindArea); Require(area,"Server bind area is missing");
        std::fprintf(stderr,"Native world travel: existing Hearthstone slot %u, server bind %u (%s), map %u, origin %.2f %.2f %.2f\n",
            slot,bindArea,area->m_areaName,oldMap,origin.x,origin.y,origin.z);
        key(true); next();
    } else if(phase==1 && elapsed>=.05) { key(false); next(); }
    else if(phase==2 && elapsed>=.3) {
        auto* bag=frame("ContainerFrame1"); Require(bag && bag->m_isVisible && bag->m_id==0,"Original backpack did not open");
        CSimpleFrame* button=nullptr;
        for(unsigned i=1;i<=16;++i) {
            char name[48]; std::snprintf(name,sizeof(name),"ContainerFrame1Item%u",i); auto* candidate=frame(name);
            if(candidate && candidate->m_isVisible && candidate->m_id==int32_t(slot)) { button=candidate; break; }
        }
        CRect rect; Require(button && button->GetRect(&rect),"Existing Hearthstone has no visible bag button");
        float x,y; DDCToNDC((rect.minX+rect.maxX)*.5f,(rect.minY+rect.maxY)*.5f,&x,&y);
        Require(x>0 && x<1 && y>0 && y<1,"Hearthstone button is outside the game view");
        itemPoint=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped ? 1-y:y)*view.bounds.size.height) toView:nil];
        [view mouseMoved:[NSEvent mouseEventWithType:NSEventTypeMouseMoved location:itemPoint modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:0 pressure:0]];
        next();
    } else if(phase==3 && elapsed>=.1) { click(true); next(); }
    else if(phase==4 && elapsed>=.05) { click(false); next(); }
    else if(phase==5) {
        auto* bar=frame("CastingBarFrame");
        if(bar && bar->m_isVisible) { std::fputs("Native world travel: original item-use cast bar visible\n",stderr); next(); }
        else Require(elapsed<5,"Native item click did not start the original Hearthstone cast");
    } else if(phase==6) {
        const auto position=player->GetPosition();
        const auto delta=position-origin;
        if(ClntObjMgrGetMapID()!=oldMap || delta.x*delta.x+delta.y*delta.y+delta.z*delta.z>625) {
            Require(CGPlayer_C::GetBindAreaID()==bindArea,"Server bind changed during travel"); next();
        } else Require(elapsed<45,"Hearthstone did not produce server-directed travel");
    } else if(phase==7 && ++settledFrames>=120) {
        auto* item=static_cast<CGItem_C*>(ClntObjMgrObjectPtr(hearthstone,TYPEMASK_ITEM,__FILE__,__LINE__));
        Require(item && item->m_obj->m_entryID==6948,"Hearthstone did not survive original world travel");
        uint32_t duration=0,start=0,enable=0; Spell_C_GetItemCooldown(item,&duration,&start,&enable);
        Require(duration && start,"Server did not apply the original Hearthstone cooldown");
        const auto p=player->GetPosition(); int32_t area=0; World::QueryAreaIdAtPosition(&p,&area);
        std::fprintf(stderr,"Native world travel: map %u -> %u, destination %.2f %.2f %.2f, area %d, %u ms item cooldown, 120 destination frames\n",
            oldMap,ClntObjMgrGetMapID(),p.x,p.y,p.z,area,duration);
        next(); return true;
    }
    return false;
}


bool CheckNativeWorldCombat(WowNativeView* view, bool melee, bool encounter, bool lootOnly) {
    static unsigned phase, readyFrames, button;
    static int32_t spell, health;
    static uint32_t mana;
    static uint32_t meleeDamage, attackStarted;
    static uint32_t outgoingSwings, incomingSwings, outgoingDamage, incomingDamage, playerHealth;
    static uint32_t lootCoins, oldCoins;
    static bool lootShift;
    static unsigned corpsePickAttempt;
    static int32_t lootItem, oldItemCount, lootCount;
    static NSPoint lootPoint;
    static WOWGUID target;
    static std::chrono::steady_clock::time_point started;
    static std::chrono::steady_clock::time_point progressed;
    static float approachRange, lastDistance;
    static bool approachForward, approachLeft, approachRight;
    if ((phase == 6 && !encounter) || phase == 15) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase, "World left during native combat check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    auto* unit = static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(target, TYPEMASK_UNIT, __FILE__, __LINE__));
    if (melee && phase >= 4 && phase < 6) {
        // Confirm damage from retained server SWING_DAMAGE events rather than
        // transient animation state, without changing combat-log filters/UI.
        for(auto* entry=s_entries.Head();entry;entry=s_entries.Next(entry))
            if(entry->m_event==1 && entry->m_sourceGuid==player->m_obj->m_guid && entry->m_destGuid==target &&
                int32_t(entry->m_timestamp-attackStarted)>=0 && entry->m_info.m_values[1]>0)
                meleeDamage=entry->m_info.m_values[1];
        if (encounter) {
            uint32_t outgoing=0, incoming=0, dealt=0, taken=0;
            for (auto* entry=s_entries.Head(); entry; entry=s_entries.Next(entry)) {
                if (int32_t(entry->m_timestamp-attackStarted)<0 || (entry->m_event!=1 && entry->m_event!=2)) continue;
                if (entry->m_sourceGuid==player->m_obj->m_guid && entry->m_destGuid==target) {
                    ++outgoing; if (entry->m_event==1) dealt += std::max(entry->m_info.m_values[1],0);
                } else if (entry->m_sourceGuid==target && entry->m_destGuid==player->m_obj->m_guid) {
                    ++incoming; if (entry->m_event==1) taken += std::max(entry->m_info.m_values[1],0);
                }
            }
            outgoingSwings=std::max(outgoingSwings,outgoing); incomingSwings=std::max(incomingSwings,incoming);
            outgoingDamage=std::max(outgoingDamage,dealt); incomingDamage=std::max(incomingDamage,taken);
        }
    }
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - started).count();
    auto next = [&] { ++phase; started = now; };
    const unsigned short codes[] = {kVK_ANSI_1,kVK_ANSI_2,kVK_ANSI_3,kVK_ANSI_4,kVK_ANSI_5,kVK_ANSI_6,
        kVK_ANSI_7,kVK_ANSI_8,kVK_ANSI_9,kVK_ANSI_0,kVK_ANSI_Minus,kVK_ANSI_Equal};
    NSString* const texts[] = {@"1",@"2",@"3",@"4",@"5",@"6",@"7",@"8",@"9",@"0",@"-",@"="};
    auto nativeKey = [&](unsigned short code, NSString* text, bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto key = [&](bool down) { nativeKey(melee ? kVK_ANSI_T:codes[button],melee ? @"t":texts[button],down); };
    auto shift = [&](bool down) {
        [view flagsChanged:[NSEvent keyEventWithType:NSEventTypeFlagsChanged location:NSZeroPoint
            modifierFlags:down ? NSEventModifierFlagShift | NX_DEVICELSHIFTKEYMASK : 0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
            characters:@"" charactersIgnoringModifiers:@"" isARepeat:NO keyCode:kVK_Shift]];
    };
    auto point = [&](CGUnit_C* candidate, float& x, float& y) {
        const auto feet = candidate->GetPosition(); const auto head = candidate->GetNamePosition();
        C3Vector body(feet.x, feet.y, feet.z + (head.z - feet.z) * (candidate->IsDeadOrGhost() ? .1f:.4f)), projected;
        if (!CGWorldFrame::s_currentWorldFrame->GetScreenCoordinates(body, &projected, nullptr)) return false;
        DDCToNDC(projected.x, projected.y, &x, &y);
        if (candidate->IsDeadOrGhost()) {
            static constexpr float offsets[] = {0, .008f, -.008f, .016f, -.016f};
            x += offsets[corpsePickAttempt % 5]; y += offsets[(corpsePickAttempt / 5) % 5];
        }
        return x > .2f && x < .9f && y > .2f && y < .85f;
    };
    auto mouse = [&](NSEventType type) {
        Require(unit != nullptr, "Native combat target disappeared before picking");
        float x, y; Require(point(unit, x, y), "Native combat target moved outside the world view");
        const NSPoint location = [view convertPoint:NSMakePoint(x * view.bounds.size.width, (view.isFlipped ? 1-y : y) * view.bounds.size.height) toView:nil];
        if (type == NSEventTypeRightMouseDown || type == NSEventTypeRightMouseUp) {
            // AppKit's synthetic mouseEvent initializer reports button 0 even
            // for right-click types. Preserve the actual native button identity.
            const bool down = type == NSEventTypeRightMouseDown;
            CGEventRef cg = CGEventCreateMouseEvent(nullptr, down ? kCGEventRightMouseDown:kCGEventRightMouseUp,
                CGPointMake(location.x, NSMaxY(NSScreen.screens.firstObject.frame)-location.y), kCGMouseButtonRight);
            Require(cg, "Cannot create native corpse right click");
            NSEvent* event = [NSEvent eventWithCGEvent:cg];
            if (down) [view rightMouseDown:event]; else [view rightMouseUp:event];
            CFRelease(cg); return;
        }
        NSEvent* event = [NSEvent mouseEventWithType:type location:location modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
        if (type == NSEventTypeMouseMoved) [view mouseMoved:event];
        else if (type == NSEventTypeLeftMouseDown) [view mouseDown:event];
        else if (type == NSEventTypeLeftMouseUp) [view mouseUp:event];
    };
    auto lootMouse = [&](NSEventType type) {
        NSEvent* event=[NSEvent mouseEventWithType:type location:lootPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil eventNumber:0 clickCount:1 pressure:0];
        if (type==NSEventTypeMouseMoved) [view mouseMoved:event];
        else if (type==NSEventTypeLeftMouseDown) [view mouseDown:event]; else [view mouseUp:event];
    };
    if (phase >= 20) {
        auto set = [&](unsigned short code, NSString* text, bool value, bool& held) {
            if (value != held) { nativeKey(code,text,value); held=value; }
        };
        auto stop = [&] {
            set(kVK_ANSI_W,@"w",false,approachForward);
            set(kVK_LeftArrow,@"\uF702",false,approachLeft);
            set(kVK_RightArrow,@"\uF703",false,approachRight);
        };
        try {
            Require(unit && !player->IsDeadOrGhost(), "Approached target disappeared or player died");
            const auto a=player->GetPosition(), b=unit->GetPosition();
            const float distance=std::hypot(b.x-a.x,b.y-a.y);
            if (phase == 22) {
                if (elapsed > .4) phase=0;
                return false;
            }
            if (distance < approachRange) {
                stop(); phase=22; started=now;
                std::fprintf(stderr,"Native world combat: approached through original movement to %.2f units at %.2f %.2f %.2f\n",distance,a.x,a.y,a.z);
                return false;
            }
            if (distance < lastDistance-.1f) { lastDistance=distance; progressed=now; }
            if (std::chrono::duration<double>(now-progressed).count()>4 || elapsed>30) {
                std::fprintf(stderr,"Native world combat: approach stopped at %.2f %.2f %.2f; target %.2f %.2f %.2f\n",a.x,a.y,a.z,b.x,b.y,b.z);
                throw NativeWorldCheckFailure("Native approach made no progress; no position override applied");
            }
            const float turn=std::remainder(std::atan2(b.y-a.y,b.x-a.x)-player->GetFacing(),6.283185307f);
            set(kVK_LeftArrow,@"\uF702",turn>.04f,approachLeft);
            set(kVK_RightArrow,@"\uF703",turn<-.04f,approachRight);
            set(kVK_ANSI_W,@"w",std::abs(turn)<.15f,approachForward);
        } catch (...) { stop(); throw; }
        return false;
    }
    if (!phase && lootOnly) {
        struct Search { CGPlayer_C* player; CGUnit_C* unit=nullptr; float distance=6; } search{player};
        ClntObjMgrEnumVisibleObjects([](WOWGUID guid,void* context)->int32_t {
            auto& search=*static_cast<Search*>(context);
            auto* candidate=static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid,TYPEMASK_UNIT,__FILE__,__LINE__));
            if (!candidate || (candidate->m_obj->m_type&TYPEMASK_PLAYER) || !candidate->IsDeadOrGhost() ||
                !candidate->CanBeLooted(FrameTime::s_curTimeMs) || !search.player->CanLoot(candidate)) return 1;
            const auto a=search.player->GetPosition(),b=candidate->GetPosition();
            const float distance=std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
            if(distance<search.distance) { search.distance=distance; search.unit=candidate; }
            return 1;
        },&search);
        Require(search.unit,"Loot check needs an existing lootable non-player corpse within six units");
        target=search.unit->m_obj->m_guid; phase=6; started=now;
        std::fprintf(stderr,"Native loot: existing %s corpse, distance %.2f\n",search.unit->GetObjectName(),search.distance);
        return false;
    }
    if (!phase) {
        if (encounter) {
            Require(melee && g_autoLootDefaultCVar, "Encounter check requires the original loot preference and attack binding");
            Require(player->m_unit->m_unitFieldLevel>=10 && int64_t(player->m_unit->m_unitFieldHealth)*10>=int64_t(player->m_unit->m_unitFieldMaxhealth)*9,
                "Encounter check requires a healthy level-10-or-higher existing character");
            for (unsigned slot=23; slot<39; ++slot) if (auto* item=player->GetBag()->GetItemPointer(slot))
                std::fprintf(stderr,"Native encounter inventory: backpack %u, item %u, count %u\n",slot-22,item->m_obj->m_entryID,item->m_item->m_itemFieldStackCount);
        }
        float selectionRange = 4.1f;
        if (melee) {
            Require(CGUIBindings::s_bindings != nullptr,"Native melee check requires existing bindings");
            const char* binding=CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE,"T",1);
            Require(binding && !std::strcmp(binding,"ATTACKTARGET"),"Native melee check requires the existing T attack binding");
            approachRange=4;
        } else {
            Require(!CGActionBar::m_bonusBarOffset && !CGActionBar::m_tempPageActiveFlags && CGActionBar::m_currentPage < 6,
                "Native combat check requires a regular action bar");
            SpellRec base; Require(g_spellDB.GetRecord(8921, &base), "Original Moonfire record is missing");
            const unsigned first = CGActionBar::m_currentPage * 12;
            for (unsigned i = 0; i < 12; ++i) {
                if (!CGActionBar::IsSpell(first+i)) continue;
                int32_t pet=0; const int32_t id=CGActionBar::GetSpell(first+i,pet); SpellRec candidate;
                if (!pet && g_spellDB.GetRecord(id,&candidate) && !std::strcmp(candidate.m_name,base.m_name)) { spell=id; button=i; }
            }
            Require(spell && CGUIBindings::s_bindings, "Native combat check requires Moonfire on the existing action bar");
            char command[32]; std::snprintf(command,sizeof(command),"ACTIONBUTTON%u",button+1);
            const char* binding=CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE,texts[button].UTF8String,1);
            Require(binding && !std::strcmp(binding,command), "Native combat check does not match current bindings");
            SpellRec selected; Require(g_spellDB.GetRecord(spell,&selected), "Combat spell is missing");
            const auto* range=g_spellRangeDB.GetRecord(selected.m_rangeIndex);
            Require(range != nullptr, "Combat spell range is missing");
            approachRange=range->m_rangeMax[0]-5;
            selectionRange=range->m_rangeMax[0]-1;
        }
        struct Search { CGPlayer_C* player; float distance; decltype(point)* project; bool encounter; CGUnit_C* selected=nullptr; CGUnit_C* nearby=nullptr; float nearbyDistance=125; } search{player,selectionRange,&point,encounter};
        ClntObjMgrEnumVisibleObjects([](WOWGUID guid,void* context) -> int32_t {
            auto& search=*static_cast<Search*>(context);
            auto* candidate=static_cast<CGUnit_C*>(ClntObjMgrObjectPtr(guid,TYPEMASK_UNIT,__FILE__,__LINE__));
            if (!candidate || candidate == search.player || (candidate->m_obj->m_type & TYPEMASK_PLAYER) ||
                candidate->IsDeadOrGhost() || !search.player->CanAttack(candidate)) return 1;
            if (search.encounter) {
                if (candidate->GetCreatureType()==8 || candidate->m_unit->m_unitFieldLevel<3 || candidate->m_unit->m_unitFieldLevel>8 || candidate->m_unit->m_unitFieldHealth<80) return 1;
            } else if (candidate->GetCreatureType()!=8) return 1;
            const auto a=search.player->GetPosition(),b=candidate->GetPosition();
            const float distance=std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));
            float x,y; const bool visible=(*search.project)(candidate,x,y);
            std::fprintf(stderr,"Native world combat candidate: %s, distance %.2f, visible %d, at %.2f %.2f %.2f\n",candidate->GetObjectName(),distance,visible,b.x,b.y,b.z);
            if (distance<search.nearbyDistance) { search.nearbyDistance=distance; search.nearby=candidate; }
            if (distance<search.distance && visible) { search.distance=distance; search.selected=candidate; }
            return 1;
        }, &search);
        if (!search.selected && search.nearby) {
            for (const auto& pair : {std::pair{"W","MOVEFORWARD"}, {"LEFT","TURNLEFT"}, {"RIGHT","TURNRIGHT"}}) {
                const char* action=CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE,pair.first,1);
                Require(action && !std::strcmp(action,pair.second),"Native combat approach does not match current movement bindings");
            }
            target=search.nearby->m_obj->m_guid;
            const auto a=player->GetPosition(),b=search.nearby->GetPosition();
            const float turn=std::remainder(std::atan2(b.y-a.y,b.x-a.x)-player->GetFacing(),6.283185307f);
            std::fprintf(stderr,"Native world combat: approach from %.2f %.2f %.2f, turn %.3f radians\n",a.x,a.y,a.z,turn);
            phase=20; started=progressed=now; lastDistance=search.nearbyDistance; return false;
        }
        Require(search.selected != nullptr, encounter ? "Native encounter needs a live level 3-8 non-player target with at least 80 health" : "Native combat check needs a live on-screen attackable critter within action range");
        unit=search.selected; target=unit->m_obj->m_guid; health=unit->m_unit->m_unitFieldHealth;
        Require(health>0, "Native combat target has no health");
        std::fprintf(stderr,"Native world combat: picking %s, health %d, spell %d, distance %.2f\n",unit->GetObjectName(),health,spell,search.distance);
        mouse(NSEventTypeMouseMoved); next();
    } else if (phase == 1 && elapsed >= .1) { mouse(NSEventTypeLeftMouseDown); next(); }
    else if (phase == 2 && elapsed >= .05) { mouse(NSEventTypeLeftMouseUp); next(); }
    else if (phase == 3 && elapsed >= .5) {
        auto* frame=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("TargetFrame",CSimpleFrame::GetObjectType()));
        Require(CGGameUI::m_lockedTarget == target && frame && frame->m_isVisible, "Native world click did not select the combat target and display its original target frame");
        mana=player->m_unit->m_unitFieldPower[0]; playerHealth=player->m_unit->m_unitFieldHealth;
        attackStarted=FrameTime::s_curTimeMs; key(true); next();
    } else if (phase == 4 && elapsed >= .05) { key(false); next(); }
    else if (phase == 5) {
        Require(unit != nullptr, "Server combat target disappeared before its health/death update");
        if (unit->m_unit->m_unitFieldHealth < health && (melee ? meleeDamage>0 : player->m_unit->m_unitFieldPower[0] < mana) && (!encounter || unit->IsDeadOrGhost())) {
            std::fprintf(stderr,"Native world combat: original click/target frame/%s, server target health %d -> %d, mana %u -> %u, melee damage %u, dead %d\n",
                melee ? "T melee":"Moonfire",health,unit->m_unit->m_unitFieldHealth,mana,player->m_unit->m_unitFieldPower[0],meleeDamage,unit->IsDeadOrGhost());
            if (encounter) {
                std::fprintf(stderr,"Native encounter: %u outgoing / %u incoming swings, %u dealt / %u taken damage, player health %u -> %u\n",
                    outgoingSwings,incomingSwings,outgoingDamage,incomingDamage,playerHealth,player->m_unit->m_unitFieldHealth);
                Require(outgoingSwings>=2 && incomingSwings && !player->IsDeadOrGhost(),"Encounter did not retain repeated two-sided server combat");
            }
            next(); return !encounter;
        }
        if(elapsed>=(encounter ? 60:melee ? 15:8)) {
            std::fprintf(stderr,"Native combat result: target health %d -> %d, dead %d, retained melee damage %u\n",
                health,unit->m_unit->m_unitFieldHealth,unit->IsDeadOrGhost(),meleeDamage);
            throw NativeWorldCheckFailure("Original attack did not produce the expected server damage update");
        }
    } else if (phase == 6) {
        Require(unit && unit->IsDeadOrGhost(), "Encounter corpse disappeared before loot");
        if (unit->CanBeLooted(FrameTime::s_curTimeMs) && player->CanLoot(unit)) {
            mouse(NSEventTypeMouseMoved);
            lootShift=g_autoLootDefaultCVar->m_intValue!=0;
            if (lootShift) shift(true);
            next();
        }
        else Require(elapsed<8,"Server corpse did not become lootable");
    } else if (phase == 7 && elapsed>=.2) {
        if (CGGameUI::IsAutoLooting()) {
            if (lootShift) shift(false);
            throw NativeWorldCheckFailure("Existing Shift binding did not select manual loot; preference left unchanged");
        }
        if (CGWorldFrame::s_currentWorldFrame->m_hitGuid != target) {
            Require(++corpsePickAttempt < 25, "Native cursor did not pick the original corpse mesh");
            mouse(NSEventTypeMouseMoved); started=now; return false;
        }
        mouse(NSEventTypeRightMouseDown); next();
    } else if (phase == 8 && elapsed>=.05) {
        mouse(NSEventTypeRightMouseUp); if (lootShift) shift(false); next();
    }
    else if (phase == 9) {
        auto* frame=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("LootFrame",CSimpleFrame::GetObjectType()));
        if (CGLootInfo::m_lootObj==target && !CGLootInfo::m_itemsPending && frame && frame->m_isVisible) {
            oldCoins=player->GetCoinage(); lootCoins=CGLootInfo::m_coins;
            int selected=lootCoins ? 0:-1;
            if (!lootCoins) for (int i=0;i<CGLootWindow::GetNumLootItems();++i) {
                const int id=CGLootInfo::GetLootItem(i), count=CGLootInfo::GetLootItemCount(i);
                const int owned=player->GetBag()->GetItemTypeCount(id,0x6);
                std::fprintf(stderr,"Native encounter loot: slot %d, item %d, count %d, owned %d, free %d\n",i+1,id,count,owned,player->GetBag()->GetNumFreeInvSlots());
                struct StackSpace { int id,count; } requested{id,count};
                const bool stackFits=player->GetBag()->FindItem([](const CGItem_C* item,void* context)->int32_t {
                    auto& request=*static_cast<StackSpace*>(context);
                    if (item->m_obj->m_entryID!=uint32_t(request.id)) return 0;
                    const auto* stats=item->GetStats();
                    return stats && int64_t(stats->m_stackable)-item->m_item->m_itemFieldStackCount>=request.count;
                },&requested,0x6)!=nullptr;
                if (id>0 && count>0 && (stackFits || player->GetBag()->GetNumFreeInvSlots()>0) && selected<0) {
                    selected=i; lootItem=id; oldItemCount=owned; lootCount=count;
                }
            }
            Require(selected>=0,"Loot is visible but no currency or item can fit the existing inventory");
            char name[32];std::snprintf(name,sizeof(name),"LootButton%d",selected+1);
            auto* button=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name,CSimpleFrame::GetObjectType()));
            CRect rect; Require(button && button->m_isVisible && button->GetRect(&rect),"Original loot button is not visible");
            float x,y; DDCToNDC((rect.minX+rect.maxX)*.5f,(rect.minY+rect.maxY)*.5f,&x,&y);
            lootPoint=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped?1-y:y)*view.bounds.size.height) toView:nil];
            std::fprintf(stderr,"Native encounter: original loot frame, %u coins, selecting %s\n",lootCoins,name);
            lootMouse(NSEventTypeMouseMoved); next();
        } else Require(elapsed<8,"Native corpse interaction did not open the original loot frame");
    } else if (phase == 10 && elapsed>=.2) { lootMouse(NSEventTypeLeftMouseDown); next(); }
    else if (phase == 11 && elapsed>=.05) { lootMouse(NSEventTypeLeftMouseUp); next(); }
    else if (phase == 12) {
        const bool acquired=lootCoins ? player->GetCoinage()==oldCoins+lootCoins : player->GetBag()->GetItemTypeCount(lootItem,0x6)==oldItemCount+lootCount;
        if (acquired) {
            std::fprintf(stderr,"Native encounter: server delivered %u coins / item %d count %d through original loot UI\n",lootCoins,lootItem,lootCount);
            auto* close=static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("LootCloseButton",CSimpleFrame::GetObjectType()));
            if (close && close->m_isVisible) {
                CRect rect; Require(close->GetRect(&rect),"Original loot close button has no bounds");
                float x,y;DDCToNDC((rect.minX+rect.maxX)*.5f,(rect.minY+rect.maxY)*.5f,&x,&y);
                lootPoint=[view convertPoint:NSMakePoint(x*view.bounds.size.width,(view.isFlipped?1-y:y)*view.bounds.size.height) toView:nil];
                lootMouse(NSEventTypeMouseMoved); lootMouse(NSEventTypeLeftMouseDown); next();
            } else { phase=14; started=now; }
        } else Require(elapsed<8,"Server did not deliver the selected loot into the existing inventory");
    } else if (phase == 13 && elapsed>=.05) { lootMouse(NSEventTypeLeftMouseUp); next(); }
    else if (phase == 14) {
        if (!CGLootInfo::m_lootObj && elapsed>=1) {
            std::fputs(lootOnly ? "Native loot: original corpse/UI, server acquisition and release passed\n" : "Native encounter: sustained combat, death, original loot UI, server acquisition and release passed\n",stderr);
            next(); return true;
        }
        Require(elapsed<8,"Original loot UI did not release the server corpse");
    }
    return false;
}


bool CheckNativeWorldCasting(WowNativeView* view) {
    static unsigned phase, readyFrames, button;
    static int32_t spell;
    static uint32_t mana, castDuration;
    static std::chrono::steady_clock::time_point started;
    if (phase == 12) return true;
    auto* player = ClntObjMgrGetActivePlayerObj();
    if (!CGGameUI::s_inWorld || !player) {
        Require(!phase,"World left during native casting check"); return false;
    }
    if (!phase && ++readyFrames < 120) return false;
    const unsigned short codes[] = {kVK_ANSI_1,kVK_ANSI_2,kVK_ANSI_3,kVK_ANSI_4,kVK_ANSI_5,kVK_ANSI_6,
        kVK_ANSI_7,kVK_ANSI_8,kVK_ANSI_9,kVK_ANSI_0,kVK_ANSI_Minus,kVK_ANSI_Equal};
    NSString* const texts[] = {@"1",@"2",@"3",@"4",@"5",@"6",@"7",@"8",@"9",@"0",@"-",@"="};
    auto key = [&](unsigned short code,NSString* text,bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:text charactersIgnoringModifiers:text isARepeat:NO keyCode:code];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto binding = [&](const char* key,const char* expected) {
        Require(CGUIBindings::s_bindings,"Native casting check requires character bindings");
        const char* action = CGUIBindings::s_bindings->GetCommandAction(BINDING_MODE_ACTIVE,key,1);
        Require(action && !std::strcmp(action,expected),"Native casting check does not match current character bindings");
    };
    auto castBar = [&] {
        auto* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName("CastingBarFrame",CSimpleFrame::GetObjectType()));
        return frame && frame->m_isVisible;
    };
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now-started).count();
    auto next = [&] { ++phase; started=now; };
    if (!phase) {
        binding("F1","TARGETSELF"); binding("ESCAPE","TOGGLEGAMEMENU");
        Require(!CGActionBar::m_bonusBarOffset && !CGActionBar::m_tempPageActiveFlags && CGActionBar::m_currentPage < 6,
            "Native casting check requires a regular action bar");
        SpellRec base; Require(g_spellDB.GetRecord(5185,&base),"Original Healing Touch record is missing");
        const unsigned first=CGActionBar::m_currentPage*12;
        for (unsigned i=0;i<12;++i) {
            if (!CGActionBar::IsSpell(first+i)) continue;
            int32_t pet=0; const int32_t id=CGActionBar::GetSpell(first+i,pet); SpellRec candidate;
            if (!pet && g_spellDB.GetRecord(id,&candidate) && !std::strcmp(candidate.m_name,base.m_name)) { spell=id; button=i; }
        }
        Require(spell,"Native casting check requires Healing Touch on the existing action bar");
        char command[32]; std::snprintf(command,sizeof(command),"ACTIONBUTTON%u",button+1); binding(texts[button].UTF8String,command);
        key(kVK_F1,@"\uF704",true); next();
    } else if (phase == 1 && elapsed >= .05) { key(kVK_F1,@"\uF704",false); next(); }
    else if (phase == 2 && elapsed >= .3) {
        Require(CGGameUI::m_lockedTarget == ClntObjMgrGetActivePlayer(),"Native F1 did not select the healing target");
        mana=player->m_unit->m_unitFieldPower[0]; key(codes[button],texts[button],true); next();
    } else if (phase == 3 && elapsed >= .05) { key(codes[button],texts[button],false); next(); }
    else if (phase == 4 || phase == 10) {
        if (player->m_castingSpellId == uint32_t(spell) && castBar()) {
            castDuration=player->m_castEndTime-player->m_castStartTime;
            Require(castDuration >= 1000,"Healing Touch check requires a noninstant cast");
            std::fprintf(stderr,"Native world casting: spell %d, %u ms original cast bar, mana %u\n",spell,castDuration,mana);
            next();
        } else Require(elapsed < 3,"Native action button did not start the original cast bar");
    } else if (phase == 5 && elapsed >= .3) { key(kVK_Escape,@"\x1b",true); next(); }
    else if (phase == 6 && elapsed >= .05) { key(kVK_Escape,@"\x1b",false); next(); }
    else if (phase == 7) {
        if (!player->m_castingSpellId && !player->m_castSpellId) next();
        else Require(elapsed < 2,"Native Escape did not cancel the original cast");
    } else if (phase == 8 && elapsed >= castDuration/1000.0 + .5) {
        Require(!player->m_castingSpellId && player->m_unit->m_unitFieldPower[0] >= mana,
            "Cancelled healing cast consumed server mana or resumed");
        std::fputs("Native world casting: Escape cancellation retained server mana\n",stderr);
        mana=player->m_unit->m_unitFieldPower[0]; key(codes[button],texts[button],true); next();
    } else if (phase == 9 && elapsed >= .05) { key(codes[button],texts[button],false); next(); }
    else if (phase == 11) {
        if (!player->m_castingSpellId && !player->m_castSpellId && player->m_unit->m_unitFieldPower[0] < mana) {
            Require(elapsed >= castDuration/1000.0-.5,"Healing cast completed before its original duration");
            std::fprintf(stderr,"Native world casting: completed spell %d after %.3f s, server mana %u -> %u; cancellation/completion passed\n",
                spell,elapsed,mana,player->m_unit->m_unitFieldPower[0]);
            next(); return true;
        }
        Require(elapsed < castDuration/1000.0+5,"Healing cast did not finish with the server mana update");
    }
    return false;
}

bool CheckNativeWorldReentry(WowNativeView* view) {
    static unsigned phase, stableFrames;
    static WOWGUID character;
    static NSPoint point;
    static std::chrono::steady_clock::time_point started;
    if (phase == 7) return true;
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - started).count();
    auto next = [&] { ++phase; stableFrames = 0; started = now; };
    auto escape = [&](bool down) {
        NSEvent* event = [NSEvent keyEventWithType:down ? NSEventTypeKeyDown : NSEventTypeKeyUp
            location:NSZeroPoint modifierFlags:0 timestamp:NSProcessInfo.processInfo.systemUptime
            windowNumber:view.window.windowNumber context:nil characters:@"\x1b" charactersIgnoringModifiers:@"\x1b" isARepeat:NO keyCode:kVK_Escape];
        if (down) [view keyDown:event]; else [view keyUp:event];
    };
    auto mouse = [&](NSEventType type) {
        NSEvent* event = [NSEvent mouseEventWithType:type location:point modifierFlags:0
            timestamp:NSProcessInfo.processInfo.systemUptime windowNumber:view.window.windowNumber context:nil
            eventNumber:0 clickCount:1 pressure:0];
        if (type == NSEventTypeMouseMoved) [view mouseMoved:event];
        else if (type == NSEventTypeLeftMouseDown) [view mouseDown:event];
        else [view mouseUp:event];
    };
    auto click = [&](const char* name) {
        auto* frame = static_cast<CSimpleFrame*>(CScriptObject::GetScriptObjectByName(name, CSimpleFrame::GetObjectType()));
        Require(frame && frame->m_isVisible, "Original logout/reentry button is not visible");
        CRect rect; Require(frame->GetRect(&rect), "Original logout/reentry button has no rectangle");
        float x, y; DDCToNDC((rect.minX + rect.maxX)*.5f, (rect.minY + rect.maxY)*.5f, &x, &y);
        Require(x > 0 && x < 1 && y > 0 && y < 1, "Original logout/reentry button is outside the view");
        point = [view convertPoint:NSMakePoint(x * view.bounds.size.width, (view.isFlipped ? 1-y : y) * view.bounds.size.height) toView:nil];
        mouse(NSEventTypeMouseMoved); mouse(NSEventTypeLeftMouseDown);
    };
    if (!phase) {
        if (!CGGameUI::s_inWorld || ++stableFrames < 120) return false;
        character = ClntObjMgrGetActivePlayer();
        Require(character.m_guid != 0, "Reentry check requires an active player");
        escape(true); next();
    } else if (phase == 1 && elapsed >= .05) { escape(false); next(); }
    else if (phase == 2 && elapsed >= .3) { click("GameMenuButtonLogout"); next(); }
    else if (phase == 3 && elapsed >= .05) {
        mouse(NSEventTypeLeftMouseUp); next();
        std::fputs("Native world reentry: requested logout through the original game menu\n", stderr);
    } else if (phase == 4) {
        Require(elapsed < 45, "Original logout did not return to character selection");
        if (CGGameUI::s_inWorld || std::strcmp(CGlueMgr::m_currentScreen, "charselect") || CGlueMgr::m_idleState != IDLE_NONE) return false;
        const auto index = CCharacterSelection::s_selectionIndex;
        Require(index >= 0 && uint32_t(index) < CCharacterSelection::s_characterList.Count() &&
            CCharacterSelection::s_characterList[index].m_characterInfo.m_guid == character,
            "Original character selection did not retain the same character");
        if (++stableFrames < 120) return false;
        std::fputs("Native world reentry: character selection rendered after server logout\n", stderr);
        click("CharSelectEnterWorldButton"); next();
    } else if (phase == 5 && elapsed >= .05) { mouse(NSEventTypeLeftMouseUp); next(); }
    else if (phase == 6) {
        Require(elapsed < 60, "Original character-selection button did not reenter the world");
        if (!CGGameUI::s_inWorld) return false;
        Require(ClntObjMgrGetActivePlayer() == character, "Original world reentry selected a different character");
        if (++stableFrames < 120) return false;
        std::fputs("Native world reentry: same player returned and rendered 120 frames through the original UI/server lifecycle\n", stderr);
        next(); return true;
    }
    return false;
}
