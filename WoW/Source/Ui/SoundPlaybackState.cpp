#include "WoW/Source/Ui/ChatFrame.h"
#include "WoW/Source/Ui/GameUI.h"

// Shared UI state queried by world audio before the complete UI is loaded.
// STATIC: 0x00B4AE40
int32_t CGChat::m_paused;

// STATIC: 0x00BD0800
int32_t CGGameUI::m_inMovie;

// VERIFY: 2c5750ac auto
// VERIFY: 13c41e04 auto
// OFFSET: 0x004CFB80
void CGChat::Pause() {
    CGChat::m_paused++;
}

// VERIFY: 62ef2d2f auto
// OFFSET: 0x004CFB90
void CGChat::Resume() {
    CGChat::m_paused--;
}

// VERIFY: 7732767c auto
// OFFSET: 0x004CFBA0
int32_t CGChat::IsPaused() {
    return CGChat::m_paused;
}

