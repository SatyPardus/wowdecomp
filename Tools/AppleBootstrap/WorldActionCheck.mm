#include "WorldActionCheck.h"
#include "NativeView.h"
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "WoW/Source/DB/DBClient/DBClient.h"
#include "WoW/Source/DB/Rec/SpellRec.h"
#include "WoW/Source/Object/ObjectClient/Player_C.h"
#include "WoW/Source/Object/Unit.h"
#include "WoW/Source/ObjectMgrClient/ObjectMgrClient.h"
#include "WoW/Source/Ui/ActionBarFrame.h"
#include "WoW/Source/Ui/GameUI.h"
#include "WoW/Source/Ui/UIBindings.h"
#import <Carbon/Carbon.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace {
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
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
