#include "WoW/Source/Ui/UILCD/Windows/EZ_LCD.h"
#include "Engine/Source/Storm/H/storm.h"
#include <new>
#include "WoW/Source/Ui/UILCD/Windows/EZ_LCD_Page.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDBitmap.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDOutput.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDText.h"

// VERIFY: 12b1cb5e auto
// OFFSET: 0x009591B0
DWORD WINAPI CEzLcd::OnSoftButtonCB(int device, DWORD dwButtons, const PVOID pContext) {
    static_cast<CEzLcd*>(pContext)->OnSoftButtons(dwButtons);
    return 0;
}

// VERIFY: 5306e66e manual
// OFFSET: 0x00959940
CEzLcd::CEzLcd() {
    this->unused_01D4 = -1;
    this->unused_01D8 = -1;
    this->m_output = nullptr;
    this->m_currentColorPage = nullptr;
    this->m_currentMonoPage = nullptr;
    this->m_colorGlobalPage = nullptr;
    this->m_monoGlobalPage = nullptr;
    this->unused_01AC = 0;
    this->unk_01A8 = 0;
    this->m_currentColorPageIndex = 0;
    this->m_currentMonoPageIndex = 0;
    this->m_lcdType = 3;
    this->m_buttonsPending = 0;
    this->m_buttonsCurrent = 0;
    InitializeCriticalSection(&this->m_buttonLock);

    this->m_monoPages.SetCount(7);
    this->m_colorPages.SetCount(13);

    for (uint32_t i = 0; i < this->m_monoPages.Count(); i++) {
        this->m_monoPages[i] = nullptr;
    }

    for (uint32_t i = 0; i < this->m_colorPages.Count(); i++) {
        this->m_colorPages[i] = nullptr;
    }
}

// VERIFY: e63fd5eb manual
// OFFSET: 0x00959AE0
CEzLcd::~CEzLcd() {
    for (uint32_t i = 0; i < this->m_monoPages.Count(); i++) {
        if (this->m_monoPages[i]) {
            delete this->m_monoPages[i];
        }

        this->m_monoPages[i] = nullptr;
    }

    this->m_monoPages.Clear();

    for (uint32_t i = 0; i < this->m_colorPages.Count(); i++) {
        if (this->m_colorPages[i]) {
            delete this->m_colorPages[i];
        }

        this->m_colorPages[i] = nullptr;
    }

    this->m_monoPages.Clear();
    this->m_colorPages.Clear();

    delete this->m_colorGlobalPage;
    this->m_colorGlobalPage = nullptr;

    delete this->m_monoGlobalPage;
    this->m_monoGlobalPage = nullptr;

    this->m_connection.Shutdown();

    DeleteCriticalSection(&this->m_buttonLock);
}

// VERIFY: 6c0ec21a auto
// OFFSET: 0x009591D0
void CEzLcd::OnSoftButtons(uint32_t buttons) {
    EnterCriticalSection(&this->m_buttonLock);
    this->m_buttonsPending |= buttons;
    LeaveCriticalSection(&this->m_buttonLock);
}

// VERIFY: 3a32c162 auto
// OFFSET: 0x009592F0
int32_t CEzLcd::Initialize(const char* friendlyName, int32_t lcdType, int32_t isAutoStartable, int32_t isPersistent, lgLcdConfigureContext* configContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext) {
    if (this->unk_01A8 || this->unused_01AC) {
        return E_FAIL;
    }

    SStrCopy(this->m_friendlyName, friendlyName, STORM_MAX_STR);

    this->unused_01CC = 0x80000000;
    this->unused_01D0 = 0x80000000;
    this->unused_01B8 = 1;
    this->m_initialized = 0;
    this->m_configContext = configContext;
    this->m_isPersistent = isPersistent;
    this->m_isAutoStartable = isAutoStartable;
    this->m_currentColorPage = nullptr;
    this->m_currentMonoPage = nullptr;
    this->m_currentColorPageIndex = 0;
    this->m_currentMonoPageIndex = 0;

    lgLcdConnectContextEx connectContext;
    connectContext.appFriendlyName = this->m_friendlyName;
    connectContext.isPersistent = isPersistent;
    connectContext.isAutostartable = isAutoStartable;

    if (configContext) {
        connectContext.onConfigure = *configContext;
    } else {
        connectContext.onConfigure.configCallback = nullptr;
        connectContext.onConfigure.configContext = nullptr;
    }

    switch (lcdType) {
        case 0: {
            connectContext.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_BW;
            break;
        }

        case 1: {
            connectContext.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_QVGA;
            break;
        }

        case 2: {
            connectContext.dwAppletCapabilitiesSupported = LGLCD_APPLET_CAP_BW | LGLCD_APPLET_CAP_QVGA;
            break;
        }
    }

    this->m_lcdType = lcdType;
    connectContext.dwReserved1 = 0;
    connectContext.onNotify.notificationCallback = nullptr;
    connectContext.onNotify.notifyContext = nullptr;

    if (!softbuttonsChangedContext) {
        this->m_softbuttonsChangedContext.softbuttonsChangedCallback = CEzLcd::OnSoftButtonCB;
        this->m_softbuttonsChangedContext.softbuttonsChangedContext = this;
    } else {
        this->m_softbuttonsChangedContext = *softbuttonsChangedContext;
    }

    if (!this->m_connection.InitializeConnection(connectContext, &this->m_softbuttonsChangedContext)) {
        this->m_initialized = 0;
        return E_FAIL;
    }

    this->m_initialized = 1;

    if (this->m_lcdType == 1 || this->m_lcdType == 2) {
        this->m_output = this->m_connection.ColorOutput();
    }

    if (this->m_lcdType == 0 || this->m_lcdType == 2) {
        this->m_output = this->m_connection.MonoOutput();
    }

    this->unused_01B8 = 0;

    return S_OK;
}

// VERIFY: e1f25790 auto
// OFFSET: 0x00959110
void CEzLcd::Update() {
    if (this->m_initialized) {
        this->m_connection.Update();
        this->m_buttonsPrevious = this->m_buttonsCurrent;

        EnterCriticalSection(&this->m_buttonLock);
        this->m_buttonsCurrent = this->m_buttonsPending;
        this->m_buttonsPending = 0;
        LeaveCriticalSection(&this->m_buttonLock);
    }
}

// VERIFY: 23e6b3c3 manual
// OFFSET: 0x00959240
int32_t CEzLcd::IsConnected() {
    return this->m_connection.IsConnected();
}

// VERIFY: f18f64d0 manual
// OFFSET: 0x00958E30
int32_t CEzLcd::IsDeviceAvailable(int32_t type) {
    CLCDOutput* output;

    if (type == 0) {
        output = this->m_connection.MonoOutput();
    } else if (type == 1) {
        output = this->m_connection.ColorOutput();
    } else {
        return 0;
    }

    if (!output) {
        return 0;
    }

    return output->IsOpened();
}

// VERIFY: 1830df89 auto
// OFFSET: 0x00958E70
void CEzLcd::ModifyDisplay(int32_t display) {
    if (display == 0) {
        this->m_output = this->m_connection.MonoOutput();
    } else if (display == 1) {
        this->m_output = this->m_connection.ColorOutput();
    }
}

// VERIFY: ccb95f2e auto
// OFFSET: 0x00958EB0
HRESULT CEzLcd::SetText(void* handle, const WCHAR* text) {
    CLCDBase* object = static_cast<CLCDBase*>(handle);

    if (!object) {
        return E_FAIL;
    }

    if (object->GetObjectType() == 1 || object->GetObjectType() == 0) {
        if (object->GetObjectType() == 1) {
            static_cast<CLCDText*>(object)->SetText(text);
            return S_OK;
        }
    }

    return E_FAIL;
}

// VERIFY: 83b4356e auto
// OFFSET: 0x00958F10
HRESULT CEzLcd::SetTextWordWrap(void* handle, bool enable) {
    CLCDBase* object = static_cast<CLCDBase*>(handle);

    if (!object || object->GetObjectType() != 1) {
        return E_FAIL;
    }

    static_cast<CLCDText*>(object)->SetWordWrap(enable);
    return S_OK;
}

// VERIFY: f0137223 auto
// OFFSET: 0x00958F50
HRESULT CEzLcd::SetSize(void* handle, int32_t width, int32_t height) {
    CLCDBase* object = static_cast<CLCDBase*>(handle);

    if (!object) {
        return E_FAIL;
    }

    object->SetSizeWH(width, height);
    return S_OK;
}

// VERIFY: 4b51ae1a auto
// OFFSET: 0x00958F80
HRESULT CEzLcd::SetBitmap(void* handle, HBITMAP bitmap) {
    if (!bitmap || !handle) {
        return E_FAIL;
    }

    static_cast<CLCDBitmap*>(handle)->SetBitmap(bitmap);
    return S_OK;
}

// VERIFY: ed80489e manual
// OFFSET: 0x00958FB0
HRESULT CEzLcd::SetOrigin(void* handle, int32_t x, int32_t y) {
    CLCDBase* object = static_cast<CLCDBase*>(handle);

    if (!object) {
        return E_FAIL;
    }

    POINT origin = { x, y };
    object->SetOrigin(origin);
    return S_OK;
}

// VERIFY: 7bb16566 manual
// OFFSET: 0x00958FE0
HRESULT CEzLcd::SetVisible(void* handle, int32_t visible) {
    CLCDBase* object = static_cast<CLCDBase*>(handle);

    if (!object) {
        return E_FAIL;
    }

    object->Show(visible);
    return S_OK;
}

// VERIFY: 2c827ba3 manual
// OFFSET: 0x00959010
HRESULT CEzLcd::SetAlpha(void* handle, int32_t alpha) {
    if (!handle) {
        return E_FAIL;
    }

    static_cast<CLCDBitmap*>(handle)->SetAlpha(alpha);
    return S_OK;
}

// VERIFY: a067b8af manual
// OFFSET: 0x00959040
int32_t CEzLcd::ButtonReleased(int32_t button) {
    switch (button) {
        case 0:
            return (this->m_buttonsPrevious & LGLCDBUTTON_BUTTON0) && !(this->m_buttonsCurrent & LGLCDBUTTON_BUTTON0);
        case 1:
            return (this->m_buttonsPrevious & LGLCDBUTTON_BUTTON1) && !(this->m_buttonsCurrent & LGLCDBUTTON_BUTTON1);
        case 2:
            return (this->m_buttonsPrevious & LGLCDBUTTON_BUTTON2) && !(this->m_buttonsCurrent & LGLCDBUTTON_BUTTON2);
        case 3:
            return (this->m_buttonsPrevious & LGLCDBUTTON_BUTTON3) && !(this->m_buttonsCurrent & LGLCDBUTTON_BUTTON3);
        case 4:
            return (this->m_buttonsPrevious & LGLCDBUTTON_LEFT) && !(this->m_buttonsCurrent & LGLCDBUTTON_LEFT);
        case 5:
            return (this->m_buttonsPrevious & LGLCDBUTTON_RIGHT) && !(this->m_buttonsCurrent & LGLCDBUTTON_RIGHT);
        case 6:
            return (this->m_buttonsPrevious & LGLCDBUTTON_UP) && !(this->m_buttonsCurrent & LGLCDBUTTON_UP);
        case 7:
            return (this->m_buttonsPrevious & LGLCDBUTTON_DOWN) && !(this->m_buttonsCurrent & LGLCDBUTTON_DOWN);
        case 8:
            return (this->m_buttonsPrevious & LGLCDBUTTON_OK) && !(this->m_buttonsCurrent & LGLCDBUTTON_OK);
        case 9:
            return (this->m_buttonsPrevious & LGLCDBUTTON_CANCEL) && !(this->m_buttonsCurrent & LGLCDBUTTON_CANCEL);
        case 10:
            return (this->m_buttonsPrevious & LGLCDBUTTON_MENU) && !(this->m_buttonsCurrent & LGLCDBUTTON_MENU);
        default:
            return 0;
    }
}

// VERIFY: dfcba8fe auto
// OFFSET: 0x00959170
void CEzLcd::SetCurrentPage(CEzLcdPage* page) {
    if (this->m_connection.MonoOutput() == this->m_output) {
        this->m_currentMonoPage = page;
    } else {
        this->m_currentColorPage = page;
    }
}

// VERIFY: ea1afd0a manual
// OFFSET: 0x00959200
void CEzLcd::ShowSubscreen(int32_t subscreen) {
    CEzLcdPage* page;

    if (this->m_output == this->m_connection.MonoOutput()) {
        page = this->m_currentMonoPage;
    } else {
        page = this->m_currentColorPage;
    }
    page->ShowSubscreen(subscreen);
}

// VERIFY: 6e0acd1e manual
// OFFSET: 0x009594A0
int32_t CEzLcd::AddNewPage(int32_t page) {
    CEzLcdPage* newPage = new CEzLcdPage(this);
    newPage->Initialize();
    newPage->SetExpiration(INFINITE);

    if (this->m_output == this->m_connection.ColorOutput()) {
        newPage->SetSizeWH(LGLCD_QVGA_BMP_WIDTH, LGLCD_QVGA_BMP_HEIGHT);
    }

    TSGrowableArray<CEzLcdPage*>* pages = &this->m_monoPages;

    if (this->m_output != this->m_connection.MonoOutput()) {
        pages = &this->m_colorPages;
    }
    (*pages)[page] = newPage;
    return pages->Count();
}

// VERIFY: b2ddf2f6 manual
// OFFSET: 0x00959540
int32_t CEzLcd::ShowPage(int32_t page) {
    TSGrowableArray<CEzLcdPage*>* pages = &this->m_monoPages;

    if (this->m_output != this->m_connection.MonoOutput()) {
        pages = &this->m_colorPages;
    }

    if (page > static_cast<int32_t>(pages->Count()) || page < -1) {
        return E_FAIL;
    }

    CEzLcdPage* newPage = (*pages)[page];

    if (this->m_connection.MonoOutput() == this->m_output) {
        this->m_currentMonoPage = newPage;
    } else {
        this->m_currentColorPage = newPage;
    }

    CLCDOutput* output = this->m_output;
    CEzLcdPage* currentPage;

    if (output == this->m_connection.MonoOutput()) {
        currentPage = this->m_currentMonoPage;
    } else {
        currentPage = this->m_currentColorPage;
    }

    output->ShowPage(currentPage, 1);

    if (this->m_connection.MonoOutput() == this->m_output) {
        this->m_currentMonoPageIndex = page;
    } else {
        this->m_currentColorPageIndex = page;
    }

    return S_OK;
}

// VERIFY: 9bae2d97 manual
// OFFSET: 0x00959610
void CEzLcd::SetBackground(COLORREF color) {
    CEzLcdPage* page;

    if (this->m_output == this->m_connection.MonoOutput()) {
        page = this->m_currentMonoPage;
    } else {
        page = this->m_currentColorPage;
    }

    if (!page || this->m_output != this->m_connection.ColorOutput()) {
        return;
    }

    if (this->m_output == this->m_connection.MonoOutput()) {
        this->m_currentMonoPage->SetPageBackground(color);
    } else {
        this->m_currentColorPage->SetPageBackground(color);
    }
}

// VERIFY: c0fc03e2 manual
// OFFSET: 0x00959680
CLCDText* CEzLcd::AddText(int32_t type, int32_t size, int32_t alignment, int32_t maxLengthPixels, int32_t userData, int32_t numberOfLines, int32_t fontWeight) {
    CEzLcdPage* page;

    if (this->m_output == this->m_connection.MonoOutput()) {
        page = this->m_currentMonoPage;
    } else {
        page = this->m_currentColorPage;
    }

    if (!page) {
        return nullptr;
    }

    if (this->m_output == this->m_connection.MonoOutput()) {
        if (this->m_output == this->m_connection.MonoOutput()) {
            page = this->m_currentMonoPage;
        } else {
            page = this->m_currentColorPage;
        }

        return page->AddText(type, size, alignment, maxLengthPixels, userData, numberOfLines);
    }

    if (this->m_output != this->m_connection.ColorOutput()) {
        return nullptr;
    }

    if (this->m_output == this->m_connection.MonoOutput()) {
        return this->m_currentMonoPage->AddColorText(type, size, alignment, maxLengthPixels, userData, numberOfLines, fontWeight);
    }

    return this->m_currentColorPage->AddColorText(type, size, alignment, maxLengthPixels, userData, numberOfLines, fontWeight);
}

// VERIFY: 4fbdc963 manual
// OFFSET: 0x00959750
CLCDBitmap* CEzLcd::AddBitmap(int32_t width, int32_t height, int32_t bitmapWidth, int32_t bitmapHeight) {
    CEzLcdPage* page;

    if (this->m_output == this->m_connection.MonoOutput()) {
        page = this->m_currentMonoPage;
    } else {
        page = this->m_currentColorPage;
    }

    if (!page) {
        return nullptr;
    }

    if (this->m_output == this->m_connection.MonoOutput()) {
        return this->m_currentMonoPage->AddBitmap(width, height, bitmapWidth, bitmapHeight);
    }

    return this->m_currentColorPage->AddBitmap(width, height, bitmapWidth, bitmapHeight);
}

// VERIFY: 2e266ee6 auto
// OFFSET: 0x009597B0
CEzLcdPage* CEzLcd::GetGlobalPage() {
    if (this->m_output == this->m_connection.MonoOutput()) {
        if (!this->m_monoGlobalPage) {
            this->m_monoGlobalPage = new (SMemAlloc(sizeof(CEzLcdPage), __FILE__, __LINE__, 0)) CEzLcdPage();

            if (this->m_connection.MonoOutput()) {
                this->m_connection.MonoOutput()->SetGlobalPage(this->m_monoGlobalPage);
            }
        }

        return this->m_monoGlobalPage;
    }

    if (!this->m_colorGlobalPage) {
        this->m_colorGlobalPage = new (SMemAlloc(sizeof(CEzLcdPage), __FILE__, __LINE__, 0)) CEzLcdPage();

        if (this->m_connection.ColorOutput()) {
            this->m_connection.ColorOutput()->SetGlobalPage(this->m_colorGlobalPage);
        }
    }

    return this->m_colorGlobalPage;
}

// VERIFY: 66a98c46 manual
// OFFSET: 0x00959890
void CEzLcd::HideGlobals() {
    CEzLcdPage* page = this->GetGlobalPage();

    if (!page) {
        return;
    }

    for (CLCDBASE_NODE* node = page->m_Objects.Head(); node; node = page->m_Objects.Next(node)) {
        node->m_pObject->Show(0);
    }
}

// VERIFY: 02bce841 auto
// OFFSET: 0x009598B0
HRESULT CEzLcd::ModifyControlsOnPage(int32_t page) {
    if (page == -1) {
        CEzLcdPage* globalPage = this->GetGlobalPage();

        if (this->m_connection.MonoOutput() == this->m_output) {
            this->m_currentMonoPage = globalPage;
        } else {
            this->m_currentColorPage = globalPage;
        }

        return S_OK;
    }

    TSGrowableArray<CEzLcdPage*>* pages = &this->m_monoPages;

    if (this->m_output != this->m_connection.MonoOutput()) {
        pages = &this->m_colorPages;
    }

    if (page > static_cast<int32_t>(pages->Count()) || page < -1) {
        return E_FAIL;
    }

    this->SetCurrentPage((*pages)[page]);
    return S_OK;
}
