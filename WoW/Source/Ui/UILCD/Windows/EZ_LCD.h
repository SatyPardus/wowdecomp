#ifndef WOW_SOURCE_UI_UILCD_WINDOWS_EZ_LCD_H
#define WOW_SOURCE_UI_UILCD_WINDOWS_EZ_LCD_H

#include <cstdint>
#include <windows.h>
#include "Engine/Source/Storm/H/stpl.h"
#include "ThirdParty/lgLcd/lglcd.h"
#include "WoW/Source/Ui/UILCD/Windows/LCDConnection.h"

class CEzLcdPage;
class CLCDBitmap;
class CLCDOutput;
class CLCDText;

class CEzLcd {
    public:
    /* 0004 */ char m_friendlyName[260];
    /* 0108 */ CLCDConnection m_connection;
    /* 0174 */ int32_t m_lcdType;
    /* 0178 */ TSGrowableArray<CEzLcdPage*> m_monoPages;
    /* 0188 */ TSGrowableArray<CEzLcdPage*> m_colorPages;
    /* 0198 */ CEzLcdPage* m_currentMonoPage;
    /* 019C */ CEzLcdPage* m_currentColorPage;
    /* 01A0 */ CEzLcdPage* m_monoGlobalPage;
    /* 01A4 */ CEzLcdPage* m_colorGlobalPage;
    /* 01A8 */ uint32_t unk_01A8;
    /* 01AC */ uint32_t unused_01AC;
    /* 01B0 */ int32_t m_currentMonoPageIndex;
    /* 01B4 */ int32_t m_currentColorPageIndex;
    /* 01B8 */ int32_t unused_01B8;
    /* 01BC */ int32_t m_initialized;
    /* 01C0 */ lgLcdConfigureContext* m_configContext;
    /* 01C4 */ int32_t m_isPersistent;
    /* 01C8 */ int32_t m_isAutoStartable;
    /* 01CC */ uint32_t unused_01CC;
    /* 01D0 */ uint32_t unused_01D0;
    /* 01D4 */ int32_t unused_01D4;
    /* 01D8 */ int32_t unused_01D8;
    /* 01DC */ CRITICAL_SECTION m_buttonLock;
    /* 01F4 */ uint32_t m_buttonsPending;
    /* 01F8 */ uint32_t m_buttonsPrevious;
    /* 01FC */ uint32_t m_buttonsCurrent;
    /* 0200 */ lgLcdSoftbuttonsChangedContext m_softbuttonsChangedContext;
    /* 0208 */ CLCDOutput* m_output;

    static DWORD WINAPI OnSoftButtonCB(int device, DWORD dwButtons, const PVOID pContext);

    CEzLcd();
    ~CEzLcd();
    virtual void OnSoftButtons(uint32_t buttons);
    int32_t Initialize(const char* friendlyName, int32_t lcdType, int32_t isAutoStartable, int32_t isPersistent, lgLcdConfigureContext* configContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext);
    int32_t IsConnected();
    void ShowSubscreen(int32_t subscreen);
    HRESULT SetVisible(void* handle, int32_t visible);
    int32_t IsDeviceAvailable(int32_t type);
    void Update();
    int32_t ShowPage(int32_t page);
    void HideGlobals();
    static HRESULT SetText(void* handle, const char* text);
    void ModifyDisplay(int32_t display);
    HRESULT SetText(void* handle, const WCHAR* text);
    HRESULT SetTextWordWrap(void* handle, bool enable);
    HRESULT SetSize(void* handle, int32_t width, int32_t height);
    HRESULT SetBitmap(void* handle, HBITMAP bitmap);
    HRESULT SetOrigin(void* handle, int32_t x, int32_t y);
    HRESULT SetAlpha(void* handle, int32_t alpha);
    int32_t ButtonReleased(int32_t button);
    void SetCurrentPage(CEzLcdPage* page);
    int32_t AddNewPage(int32_t page);
    void SetBackground(COLORREF color);
    CLCDText* AddText(int32_t type, int32_t size, int32_t alignment, int32_t maxLengthPixels, int32_t userData, int32_t numberOfLines, int32_t fontWeight);
    CLCDBitmap* AddBitmap(int32_t width, int32_t height, int32_t bitmapWidth, int32_t bitmapHeight);
    CEzLcdPage* GetGlobalPage();
    HRESULT ModifyControlsOnPage(int32_t page);
};

#endif
