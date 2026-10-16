#include "WoW/Source/Ui/UILCD/Windows/LCDConnection.h"
#include <cstring>
#include "WoW/Source/Ui/UILCD/Windows/LCDGfx.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDOutput.h"

// STATIC: 0x00DCE1A4
LONG CLCDConnection::g_lInitCount;

// VERIFY: 45f8899c auto
// OFFSET: 0x0095A6E0
CLCDConnection::CLCDConnection() {
    this->m_hConnection = LGLCD_INVALID_CONNECTION;
    this->m_pColorOutput = nullptr;
    this->m_pGfxColor = nullptr;
    this->m_pMonoOutput = nullptr;
    this->m_pGfxMono = nullptr;
    this->unused_0040 = 0;
    this->m_pSoftbuttonsChangedCtx = nullptr;
    InitializeCriticalSection(&this->m_csCallback);
}

// VERIFY: 57a4e5e4 manual
// OFFSET: [0x0095A730, 0x0095A800]
CLCDConnection::~CLCDConnection() {
    this->Disconnect();

    if (this->m_pMonoOutput) {
        delete this->m_pMonoOutput;
        this->m_pMonoOutput = nullptr;
    }

    if (this->m_pColorOutput) {
        delete this->m_pColorOutput;
        this->m_pColorOutput = nullptr;
    }

    if (this->m_pGfxMono) {
        delete this->m_pGfxMono;
        this->m_pGfxMono = nullptr;
    }

    if (this->m_pGfxColor) {
        delete this->m_pGfxColor;
        this->m_pGfxColor = nullptr;
    }
}

// VERIFY: 139c498f manual
// OFFSET: 0x0095AAB0
int32_t CLCDConnection::InitializeConnection(lgLcdConnectContextEx& connectContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext) {
    if (!connectContext.dwAppletCapabilitiesSupported || (connectContext.dwAppletCapabilitiesSupported & LGLCD_APPLET_CAP_BW)) {
        this->m_pMonoOutput = this->AllocMonoOutput();
        this->m_pGfxMono = new CLCDGfxMono();
        this->m_pGfxMono->Initialize();
        this->m_pMonoOutput->SetGfx(this->m_pGfxMono);
    }

    if (connectContext.dwAppletCapabilitiesSupported & LGLCD_APPLET_CAP_QVGA) {
        this->m_pColorOutput = this->AllocColorOutput();
        this->m_pGfxColor = new CLCDGfxColor();
        this->m_pGfxColor->Initialize();
        this->m_pColorOutput->SetGfx(this->m_pGfxColor);
    }

    if (InterlockedIncrement(&CLCDConnection::g_lInitCount) == 1 && lgLcdInit() != ERROR_SUCCESS) {
        InterlockedDecrement(&CLCDConnection::g_lInitCount);
        return 0;
    }

    memset(&this->m_lcdConnectCtxEx, 0, sizeof(this->m_lcdConnectCtxEx));
    this->m_lcdConnectCtxEx.onNotify.notificationCallback = nullptr;
    this->m_lcdConnectCtxEx.appFriendlyName = nullptr;
    this->m_lcdConnectCtxEx.isPersistent = FALSE;
    this->m_lcdConnectCtxEx.isAutostartable = FALSE;
    this->m_lcdConnectCtxEx.dwAppletCapabilitiesSupported = 0;
    this->m_lcdConnectCtxEx.dwReserved1 = 0;
    this->m_lcdConnectCtxEx.onNotify.notifyContext = nullptr;
    this->m_lcdConnectCtxEx.connection = LGLCD_INVALID_CONNECTION;
    memcpy(&this->m_lcdConnectCtxEx, &connectContext, sizeof(this->m_lcdConnectCtxEx));
    this->m_pSoftbuttonsChangedCtx = softbuttonsChangedContext;

    if (!this->m_lcdConnectCtxEx.onNotify.notificationCallback) {
        this->m_lcdConnectCtxEx.onNotify.notificationCallback = CLCDConnection::_OnNotificationCallback;
        this->m_lcdConnectCtxEx.onNotify.notifyContext = this;
    }

    this->ConnectEx();

    if (this->m_hConnection != LGLCD_INVALID_CONNECTION) {
        Sleep(50);
        this->Update();
    }

    return 1;
}

// VERIFY: 632aab96 manual
// OFFSET: 0x0095A3D0
void CLCDConnection::Shutdown() {
    this->Disconnect();

    if (!InterlockedDecrement(&CLCDConnection::g_lInitCount)) {
        lgLcdDeInit();
    }

    this->m_pSoftbuttonsChangedCtx = nullptr;
}

// VERIFY: 2ca96c41 auto
// OFFSET: 0x0095A480
int32_t CLCDConnection::IsConnected() {
    return this->m_hConnection != LGLCD_INVALID_CONNECTION;
}

// VERIFY: 85b8db82 auto
// OFFSET: 0x0095A440
void CLCDConnection::Disconnect() {
    if (this->m_pColorOutput) {
        this->m_pColorOutput->Close();
    }

    if (this->m_pMonoOutput) {
        this->m_pMonoOutput->Close();
    }

    if (this->m_hConnection != LGLCD_INVALID_CONNECTION) {
        lgLcdDisconnect(this->m_hConnection);
        this->m_hConnection = LGLCD_INVALID_CONNECTION;
    }
}

// VERIFY: 5261550b manual
// OFFSET: 0x0095A6C0
CLCDOutput* CLCDConnection::AllocMonoOutput() {
    return new CLCDOutput();
}

// VERIFY: 39e8db1c manual
// OFFSET: 0x0095A6C0
CLCDOutput* CLCDConnection::AllocColorOutput() {
    return new CLCDOutput();
}

// VERIFY: 8cc75894 auto
// OFFSET: 0x0095A4A0
CLCDOutput* CLCDConnection::MonoOutput() {
    return this->m_pMonoOutput;
}

// VERIFY: b54799f0 auto
// OFFSET: 0x00499FB0
CLCDOutput* CLCDConnection::ColorOutput() {
    return this->m_pColorOutput;
}

// VERIFY: 23f2e34b manual
// OFFSET: 0x0095A400
void CLCDConnection::ConnectEx() {
    this->Disconnect();

    if (this->m_hConnection != LGLCD_INVALID_CONNECTION) {
        return;
    }

    if (lgLcdConnectEx(&this->m_lcdConnectCtxEx) == ERROR_SUCCESS) {
        this->m_hConnection = this->m_lcdConnectCtxEx.connection;
    } else {
        this->m_hConnection = LGLCD_INVALID_CONNECTION;
    }
}

// VERIFY: 58a497a3 auto
// OFFSET: 0x0095A490
int32_t CLCDConnection::GetConnectionId() {
    return this->m_hConnection;
}

// VERIFY: 31273646 manual
// OFFSET: 0x0095A4B0
void CLCDConnection::OnDeviceRemoval(uint32_t displayType) {
    CLCDOutput** output;

    if (displayType == LGLCD_DEVICE_BW) {
        output = &this->m_pMonoOutput;
    } else if (displayType == LGLCD_DEVICE_QVGA) {
        output = &this->m_pColorOutput;
    } else {
        return;
    }

    if (*output) {
        (*output)->ResetDeviceType();
        (*output)->Close();
    }
}

// VERIFY: 187915b3 auto
// OFFSET: 0x0095A4E0
void CLCDConnection::OnNotification(uint32_t notification, uint32_t param1) {
    switch (notification) {
        case LGLCD_NOTIFICATION_DEVICE_ARRIVAL:
            this->OnDeviceArrival(param1);
            break;

        case LGLCD_NOTIFICATION_DEVICE_REMOVAL:
            this->OnDeviceRemoval(param1);
            break;

        case LGLCD_NOTIFICATION_CLOSE_CONNECTION:
            this->Disconnect();
            break;

        case LGLCD_NOTIFICATION_APPLET_DISABLED:
            this->OnAppletDisabled();
            break;

        case LGLCD_NOTIFICATION_APPLET_ENABLED:
            this->OnAppletEnabled();
            break;
    }
}

// VERIFY: 9afda492 auto
// OFFSET: 0x0095A550
void CLCDConnection::OnSoftButtonEvent(int32_t device, uint32_t buttons) {
    if (this->m_pMonoOutput && device == this->m_pMonoOutput->GetDeviceId()) {
        this->m_pMonoOutput->OnSoftButtonEvent(buttons);
    } else if (this->m_pColorOutput && device == this->m_pColorOutput->GetDeviceId()) {
        this->m_pColorOutput->OnSoftButtonEvent(buttons);
    }
}

// VERIFY: 98903f7e auto
// OFFSET: 0x0095A5B0
void CLCDConnection::FreeMonoOutput() {
    if (this->m_pMonoOutput) {
        delete this->m_pMonoOutput;
        this->m_pMonoOutput = nullptr;
    }
}

// VERIFY: 0cd9cf1f auto
// OFFSET: 0x0095A5D0
void CLCDConnection::FreeColorOutput() {
    if (this->m_pColorOutput) {
        delete this->m_pColorOutput;
        this->m_pColorOutput = nullptr;
    }
}

// VERIFY: 79c52cb2 manual
// OFFSET: 0x0095A5F0
int32_t CLCDConnection::Initialize(lgLcdConnectContext& connectContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext) {
    this->m_pMonoOutput = new CLCDOutput();
    this->m_pGfxMono = new CLCDGfxMono();
    this->m_pGfxMono->Initialize();
    this->m_pMonoOutput->SetGfx(this->m_pGfxMono);

    lgLcdConnectContextEx connectContextEx;
    memset(&connectContextEx, 0, sizeof(connectContextEx));
    connectContextEx.appFriendlyName = connectContext.appFriendlyName;
    connectContextEx.isPersistent = connectContext.isPersistent;
    connectContextEx.isAutostartable = connectContext.isAutostartable;
    connectContextEx.onConfigure = connectContext.onConfigure;
    connectContextEx.connection = LGLCD_INVALID_CONNECTION;
    connectContextEx.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_BW;
    return this->InitializeConnection(connectContextEx, softbuttonsChangedContext);
}

// VERIFY: 7857dcee auto
// OFFSET: 0x005EEB70
void CLCDConnection::OnAppletEnabled() {
}

// VERIFY: 7480ab95 auto
// OFFSET: 0x005EEB70
void CLCDConnection::OnAppletDisabled() {
}

// VERIFY: 840872af auto
// OFFSET: 0x005EEB70
void CLCDConnection::OnConfigure() {
}

// VERIFY: 2b1b6a6a auto
// OFFSET: 0x005EEB70
void CLCDConnection::OnCallbackEvent() {
}

// VERIFY: 78ac59fd manual
// OFFSET: 0x0095A830
DWORD WINAPI CLCDConnection::_OnNotificationCallback(int connection, const PVOID pContext, DWORD notificationCode, DWORD notifyParm1, DWORD notifyParm2, DWORD notifyParm3, DWORD notifyParm4) {
    CLCDConnection* self = static_cast<CLCDConnection*>(pContext);

    EnterCriticalSection(&self->m_csCallback);
    CB_EVENT_NODE* node = self->m_callbackEvents.NewNode(2, 0, 0);
    node->m_event.m_code = notificationCode;
    node->m_event.m_type = 2;
    node->m_event.m_param1 = notifyParm1;
    node->m_event.m_param2 = notifyParm2;
    LeaveCriticalSection(&self->m_csCallback);

    self->OnCallbackEvent();
    return 0;
}

// VERIFY: 04f20c54 manual
// OFFSET: 0x0095A8F0
DWORD WINAPI CLCDConnection::_OnSoftButtonsCallback(int device, DWORD buttons, const PVOID context) {
    CLCDConnection* self = static_cast<CLCDConnection*>(context);

    EnterCriticalSection(&self->m_csCallback);
    CB_EVENT_NODE* node = self->m_callbackEvents.NewNode(2, 0, 0);
    node->m_event.m_code = device;
    node->m_event.m_type = 0;
    node->m_event.m_param1 = buttons;
    LeaveCriticalSection(&self->m_csCallback);

    self->OnCallbackEvent();
    return 0;
}

// VERIFY: 8719e482 manual
// OFFSET: 0x0095A9B0
void CLCDConnection::OnDeviceArrival(uint32_t displayType) {
    CLCDOutput** output;

    if (displayType == LGLCD_DEVICE_BW) {
        output = &this->m_pMonoOutput;
    } else if (displayType == LGLCD_DEVICE_QVGA) {
        output = &this->m_pColorOutput;
    } else {
        return;
    }

    if (!*output) {
        return;
    }

    lgLcdOpenByTypeContext context;
    memset(&context, 0, sizeof(context));
    context.connection = this->m_hConnection;
    context.deviceType = displayType;
    context.onSoftbuttonsChanged.softbuttonsChangedCallback = CLCDConnection::_OnSoftButtonsCallback;
    context.onSoftbuttonsChanged.softbuttonsChangedContext = this;
    context.device = LGLCD_INVALID_DEVICE;

    if (this->m_pSoftbuttonsChangedCtx) {
        context.onSoftbuttonsChanged = *this->m_pSoftbuttonsChangedCtx;
    }

    (*output)->OpenByType(context);
}

// VERIFY: fa6544f6 manual
// OFFSET: 0x0095AC10
int32_t CLCDConnection::PopEvent(CB_EVENT& event) {
    memset(&event, 0, sizeof(event));
    EnterCriticalSection(&this->m_csCallback);
    CB_EVENT_NODE* node = this->m_callbackEvents.Head();

    if (!node) {
        LeaveCriticalSection(&this->m_csCallback);
        return 0;
    }

    event = node->m_event;
    this->m_callbackEvents.DeleteNode(node);
    LeaveCriticalSection(&this->m_csCallback);
    return 1;
}

// VERIFY: d063d5ba manual
// OFFSET: 0x0095ACA0
void CLCDConnection::Update() {
    if (this->m_hConnection == LGLCD_INVALID_CONNECTION) {
        this->ConnectEx();
    }

    CB_EVENT event;

    while (this->PopEvent(event)) {
        if (event.m_type == 0) {
            this->OnSoftButtonEvent(event.m_code, event.m_param1);
        } else if (event.m_type == 1) {
            this->OnConfigure();
        } else if (event.m_type == 2) {
            this->OnNotification(event.m_code, event.m_param1);
        }
    }

    for (int32_t i = 0; i < 2; i++) {
        CLCDOutput** output = &this->m_pMonoOutput;

        if (i) {
            output = &this->m_pColorOutput;
        }

        if (!*output) {
            continue;
        }

        if ((*output)->IsOpened()) {
            (*output)->OnUpdate(GetTickCount());
            (*output)->Draw();
        }

        if (!(*output)->IsOpened() && (*output)->HasDeviceType()) {
            (*output)->ReOpenDeviceType();
        }
    }
}
