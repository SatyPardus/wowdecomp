#ifndef ENGINE_SOURCE_STEELSERIESMOUSE_WIN32_HIDMANAGERIMPL_H
#define ENGINE_SOURCE_STEELSERIESMOUSE_WIN32_HIDMANAGERIMPL_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include "Engine/Source/SteelseriesMouse/HidManager.h"

class HidManagerImpl;

class HidDevice {
    public:
    static DWORD WINAPI ThreadProc(void* param);
    static VOID CALLBACK ReadCompletion(DWORD error, DWORD bytes, LPOVERLAPPED overlapped);
    static VOID CALLBACK WriteCompletion(DWORD error, DWORD bytes, LPOVERLAPPED overlapped);

    /* 0000 */ HANDLE m_thread;
    /* 0004 */ DWORD m_threadId;
    /* 0008 */ void (HidDevice::*m_threadProc)();
    /* 0018 */ std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA_A> m_interfaceDetail;
    /* 001C */ HANDLE m_file;
    /* 0020 */ int32_t m_openRead;
    /* 0024 */ int32_t m_openWrite;
    /* 0028 */ int32_t m_openOverlapped;
    /* 002C */ int32_t m_openExclusive;
    /* 0030 */ PHIDP_PREPARSED_DATA m_preparsedData;
    /* 0034 */ HIDD_ATTRIBUTES m_attributes;
    /* 0040 */ uint8_t unused_0040[16];
    /* 0050 */ HIDP_CAPS m_caps;
    /* 0090 */ OVERLAPPED m_writeOverlapped;
    /* 00A4 */ OVERLAPPED m_readOverlapped;
    /* 00B8 */ std::unique_ptr<uint8_t> m_readBuffer;
    /* 00BC */ uint32_t m_readBufferSize;
    /* 00C0 */ DWORD m_bytesRead;
    /* 00C4 */ HANDLE m_readEvent;
    /* 00C8 */ HANDLE m_writeEvent;
    /* 00CC */ uint32_t m_readPending;
    /* 00D0 */ std::string m_hardwareId;
    /* 00EC */ HidManagerImpl* m_manager;
    /* 00F0 */ HANDLE m_readSignal;
    /* 00F4 */ HANDLE m_stopEvent;
    /* 00F8 */ uint8_t unused_00F8[3];
    /* 00FB */ uint8_t m_stale;
    /* 00FC */ DWORD m_devInst;
    /* 0100 */ char m_productString[256];
    /* 0200 */ char m_serialNumber[256];

    HidDevice();
    explicit HidDevice(DWORD devInst);
    HidDevice(const HidDevice& device);
    ~HidDevice();
    bool operator<(const HidDevice& device) const;
    void ReleaseHandles();
    bool Open(int32_t read, int32_t write, int32_t overlapped, int32_t exclusive, int32_t query);
    bool Write(const void* data, uint32_t size);
    bool BeginRead(uint32_t size, HANDLE event);
    bool WriteEx(const void* data, uint32_t size, HANDLE event);
    void ReadLoop();
    bool WriteWait(const void* data, uint32_t size);
    bool StartReadThread(HidManagerImpl* manager);
    bool GetDeviceProperty(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property, std::string& value);
    bool EnumerateInterface(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD index);
};

class HidManagerImpl : public HidManager {
    public:
    /* 0004 */ std::set<HidDevice> m_devices;
    /* 0010 */ uint32_t unused_0010;
    /* 0014 */ uint32_t unused_0014;
    /* 0018 */ HidDevice m_nullDevice;
    /* 0318 */ HWND m_window;
    /* 031C */ DWORD m_inputDevInst;
    /* 0320 */ DWORD m_outputDevInst;

    HidManagerImpl();
    ~HidManagerImpl() override;
    bool EnumerateDevices() override;
    void SetWindow(void* window) override;
    uint32_t GetDeviceCount() override;
    bool WriteWait(const void* data, uint32_t size) override;
    bool Write(const void* data, uint32_t size) override;
    bool SetFeature(void* data, uint32_t size) override;
    HidDevice& GetDevice(DWORD devInst);
};

#endif
