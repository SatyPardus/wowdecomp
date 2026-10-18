#if defined(__APPLE__)
#include "WoW/Source/Ui/UILCD/LCD.h"
// Apple does not load the optional Windows Logitech LCD driver. These client
// notifications follow the original no-connected-LCD path; they do not create
// a peripheral, install an idle callback, or change the game's normal UI.
void CGLCD::Initialize() {}
void CGLCD::Shutdown() {}
void CGLCD::Login() {}
void CGLCD::Logout() {}
void CGLCD::EnterWorld() {}
void CGLCD::LeaveWorld() {}
void CGLCD::ClearWaitQueueScreen() {}
void CGLCD::IdleDisconnectWarning() {}
void CGLCD::ShowBattlefieldScreen() {}
void CGLCD::HideBattlefieldAlert() {}
void CGLCD::OnBattlefieldReady(const BattlefieldStatus*) {}
void CGLCD::OnBattlefieldActive() {}
void CGLCD::OnWhisper(const char*) {}
void CGLCD::AuctionOutbid(const char*) {}
void CGLCD::AuctionExpired(const char*) {}
void CGLCD::AuctionSold(const char*) {}
void CGLCD::AuctionWon(const char*) {}
#endif
