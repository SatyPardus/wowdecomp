#if defined(__APPLE__)

#include "WoW/Source/Ui/UILCD/LCD.h"

TSList<POIIcon, TSGetLink<POIIcon> > CGLCD::m_POIIcons;
TSList<MAPDATA, TSGetLink<MAPDATA> > CGLCD::m_mapData;
CEzLcd* CGLCD::m_lcd;
int32_t CGLCD::m_chatAlertLines;
uint8_t CGLCD::initComplete;
TSHashTable<LCDHANDLE, HASHKEY_CONSTSTRI> CGLCD::m_handles;
CGPlayer_C* CGLCD::m_player;
uint8_t CGLCD::m_inWorld;

void CGLCD::ClearWaitQueueScreen() {}
void CGLCD::IdleDisconnectWarning() {}
void CGLCD::Initialize() { CGLCD::Shutdown(); }
void CGLCD::SetScreen(int32_t screen) { static_cast<void>(screen); }
void CGLCD::ShowBattlefieldScreen() {}
void CGLCD::HideBattlefieldAlert() {}
void CGLCD::OnBattlefieldReady(const BattlefieldStatus* status) { static_cast<void>(status); }
void CGLCD::OnBattlefieldActive() {}
void CGLCD::UpdateBattlefieldStats() {}

void CGLCD::Shutdown() {
    CGLCD::m_handles.Clear();
    CGLCD::m_lcd = nullptr;
    CGLCD::m_mapData.Clear();
    CGLCD::m_POIIcons.Clear();
    CGLCD::initComplete = 0;
    CGLCD::m_chatAlertLines = 0;
}

int32_t CGLCD::Update(const void* data, void* param) {
    static_cast<void>(data);
    static_cast<void>(param);
    return 0;
}

void CGLCD::OnWhisper(const char* name) { static_cast<void>(name); }
void CGLCD::AuctionOutbid(const char* name) { static_cast<void>(name); }
void CGLCD::AuctionExpired(const char* name) { static_cast<void>(name); }
void CGLCD::AuctionSold(const char* name) { static_cast<void>(name); }
void CGLCD::AuctionWon(const char* name) { static_cast<void>(name); }
void CGLCD::PushAlert(const char* text, int32_t a2) { static_cast<void>(text); static_cast<void>(a2); }
void CGLCD::Login() {}

void* CGLCD::GetHandle(const char* name) {
    static_cast<void>(name);
    return nullptr;
}

int32_t CGLCD::GetScreenFromName(const char* name) {
    static_cast<void>(name);
    return -1;
}

void CGLCD::CheckInput() {}
void CGLCD::UpdateMapPositions() {}
void CGLCD::SetStat(const char* name, int32_t value) { static_cast<void>(name); static_cast<void>(value); }
void CGLCD::SetStat(const char* name, float value, bool percent) { static_cast<void>(name); static_cast<void>(value); static_cast<void>(percent); }
void CGLCD::SetStat(const char* name, int32_t minValue, int32_t maxValue) { static_cast<void>(name); static_cast<void>(minValue); static_cast<void>(maxValue); }
void CGLCD::SetStat(const char* name, const char* value) { static_cast<void>(name); static_cast<void>(value); }
void CGLCD::SetHandleText(const char* name, const char* text) { static_cast<void>(name); static_cast<void>(text); }
void CGLCD::DisplayMeleeStats() {}
void CGLCD::DisplayRangedStats() {}
void CGLCD::DisplayDefenseStats() {}
void CGLCD::DisplaySpellStats() {}
void CGLCD::SetAlertText() {}
void CGLCD::PostInit() {}

bool CGLCD::LoadLCDMap(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    static_cast<void>(screen);
    static_cast<void>(node);
    static_cast<void>(status);
    static_cast<void>(subscreen);
    return false;
}

bool CGLCD::LoadLCDText(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    static_cast<void>(screen);
    static_cast<void>(node);
    static_cast<void>(status);
    static_cast<void>(subscreen);
    return false;
}

bool CGLCD::LoadLCDBitmap(int32_t screen, XMLNode* node, CStatus* status) {
    static_cast<void>(screen);
    static_cast<void>(node);
    static_cast<void>(status);
    return false;
}

bool CGLCD::LoadLCDElement(int32_t screen, XMLNode* node, CStatus* status, int32_t subscreen) {
    static_cast<void>(screen);
    static_cast<void>(node);
    static_cast<void>(status);
    static_cast<void>(subscreen);
    return false;
}

bool CGLCD::LoadGlobals(XMLNode* node, CStatus* status) {
    static_cast<void>(node);
    static_cast<void>(status);
    return false;
}

bool CGLCD::GeneratePOIs(XMLNode* node) {
    static_cast<void>(node);
    return false;
}

bool CGLCD::LoadScreen(XMLNode* node, CStatus* status) {
    static_cast<void>(node);
    static_cast<void>(status);
    return false;
}

bool CGLCD::LoadLayout() {
    return false;
}

void CGLCD::EnterWorld() { CGLCD::m_inWorld = 1; }
void CGLCD::LeaveWorld() { CGLCD::m_inWorld = 0; }
void CGLCD::Logout() { CGLCD::m_player = nullptr; CGLCD::m_inWorld = 0; }

void LCDHANDLE::SetText(const char* text) {
    static_cast<void>(text);
}

#endif
