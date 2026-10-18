#include "AccountData.h"
#include "ClientServices.h"
#include "WoW/Source/Glue/CGlueMgr.h"

namespace {
class ClientAccountDataHost final : public AccountDataHost {
    NetClient& Connection() const override { return *ClientServices::Connection(); }
    const char* AccountName() const override { return ClientServices::GetAccountName(); }
    const char* RealmName() const override { return ClientServices::GetSelectedRealmName(); }
    const char* CharacterName() const override { return ClientServices::GetCharacterName(); }
    WOWGUID ActiveCharacter() const override { return CGlueMgr::GetActive(); }
};
}

void AccountDataInitializeBasicSystem() {
    static ClientAccountDataHost host;
    AccountDataInitializeBasicSystem(host);
}
