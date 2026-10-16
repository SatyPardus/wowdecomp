#ifndef WOW_SOURCE_UI_UILCD_WINDOWS_LCDCONNECTION_H
#define WOW_SOURCE_UI_UILCD_WINDOWS_LCDCONNECTION_H

#include <cstdint>
#include <windows.h>
#include "Engine/Source/Storm/H/stpl.h"
#include "ThirdParty/lgLcd/lglcd.h"

class CLCDGfxBase;
class CLCDOutput;

class CLCDConnection {
    public:
    struct CB_EVENT {
        /* 0000 */ int32_t m_type;
        /* 0004 */ DWORD m_code;
        /* 0008 */ DWORD m_param1;
        /* 000C */ DWORD m_param2;
        /* 0010 */ DWORD m_param3;
        /* 0014 */ DWORD m_param4;
    };

    struct CB_EVENT_NODE : public TSLinkedNode<CB_EVENT_NODE> {
        /* 0008 */ CB_EVENT m_event;
    };

    static LONG g_lInitCount;

    /* 0004 */ lgLcdConnectContextEx m_lcdConnectCtxEx;
    /* 002C */ int32_t m_hConnection;
    /* 0030 */ CLCDOutput* m_pColorOutput;
    /* 0034 */ CLCDGfxBase* m_pGfxColor;
    /* 0038 */ CLCDOutput* m_pMonoOutput;
    /* 003C */ CLCDGfxBase* m_pGfxMono;
    /* 0040 */ uint32_t unused_0040;
    /* 0044 */ lgLcdSoftbuttonsChangedContext* m_pSoftbuttonsChangedCtx;
    /* 0048 */ CRITICAL_SECTION m_csCallback;
    /* 0060 */ TSList<CB_EVENT_NODE, TSGetLink<CB_EVENT_NODE> > m_callbackEvents;

    static DWORD WINAPI _OnNotificationCallback(int connection, const PVOID pContext, DWORD notificationCode, DWORD notifyParm1, DWORD notifyParm2, DWORD notifyParm3, DWORD notifyParm4);

    CLCDConnection();
    virtual ~CLCDConnection();
    virtual int32_t InitializeConnection(lgLcdConnectContextEx& connectContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext);
    virtual int32_t Initialize(lgLcdConnectContext& connectContext, lgLcdSoftbuttonsChangedContext* softbuttonsChangedContext);
    virtual void Shutdown();
    virtual int32_t IsConnected();
    virtual int32_t GetConnectionId();
    virtual void Update();
    virtual void OnDeviceArrival(uint32_t displayType);
    virtual void OnDeviceRemoval(uint32_t displayType);
    virtual void OnAppletEnabled();
    virtual void OnAppletDisabled();
    virtual void OnNotification(uint32_t notification, uint32_t param1);
    virtual void OnConfigure();
    virtual void OnSoftButtonEvent(int32_t device, uint32_t buttons);
    virtual void OnCallbackEvent();
    virtual void ConnectEx();
    virtual void Disconnect();
    virtual CLCDOutput* AllocMonoOutput();
    virtual CLCDOutput* AllocColorOutput();
    virtual void FreeMonoOutput();
    virtual void FreeColorOutput();
    CLCDOutput* ColorOutput();
    CLCDOutput* MonoOutput();
    int32_t PopEvent(CB_EVENT& event);
    static DWORD WINAPI _OnSoftButtonsCallback(int device, DWORD buttons, const PVOID context);
};

#endif
