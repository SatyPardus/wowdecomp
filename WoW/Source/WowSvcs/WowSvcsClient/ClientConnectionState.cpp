#include "ClientServices.h"
#include "Engine/Source/Storm/H/storm.h"

// STATIC: 0x00C79CF4
ClientConnection* ClientServices::s_currentConnection;

// VERIFY: ddc9b4a6 manual
// OFFSET: 0x006B0970
ClientConnection* ClientServices::Connection() {
    if (!ClientServices::s_currentConnection) {
        SErrDisplayAppFatalMessage("", "", 0);
    }

    return ClientServices::s_currentConnection;
}

