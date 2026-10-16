#if defined(WOWDECOMP_ARCH_32)

#include <cstring>
#include <mbstring.h>
#include <new>
#include <utility>
#include "Engine/Source/SteelseriesMouse/Win32/HidManagerImpl.h"
#include "Engine/Source/Storm/H/storm.h"

// INLINED
HidManager::~HidManager() {
}

// VERIFY: 23eea9ed manual
// OFFSET: 0x008C5050
HidManager* HidManager::Create() {
    return new (SMemAlloc(sizeof(HidManagerImpl), __FILE__, __LINE__, 0)) HidManagerImpl();
}

// VERIFY: 3753ec22 manual
// OFFSET: 0x008C3A80
HidDevice::HidDevice() : m_thread(nullptr), m_threadId(0), m_threadProc(nullptr), m_file(nullptr), m_preparsedData(nullptr), m_readBufferSize(0), m_readEvent(nullptr), m_writeEvent(nullptr), m_readPending(0), m_manager(nullptr), m_readSignal(nullptr), m_stopEvent(nullptr), m_stale(0), m_devInst(0) {
    this->m_productString[0] = '\0';
    this->m_serialNumber[0] = '\0';
}

// VERIFY: a8d84285 manual
// OFFSET: 0x008C3A10
HidDevice::HidDevice(DWORD devInst) : m_thread(nullptr), m_threadId(0), m_threadProc(nullptr), m_file(nullptr), m_preparsedData(nullptr), m_readSignal(nullptr), m_stopEvent(nullptr), m_devInst(devInst) {
}

// VERIFY: ad56d01b manual
// OFFSET: 0x008C3E40
HidDevice::HidDevice(const HidDevice& device) : m_thread(nullptr), m_threadId(0), m_threadProc(nullptr), m_interfaceDetail(std::move(const_cast<HidDevice&>(device).m_interfaceDetail)), m_file(device.m_file), m_openRead(device.m_openRead), m_openWrite(device.m_openWrite), m_openOverlapped(device.m_openOverlapped), m_openExclusive(device.m_openExclusive), m_preparsedData(device.m_preparsedData), m_attributes(device.m_attributes), m_caps(device.m_caps), m_writeOverlapped(device.m_writeOverlapped), m_readOverlapped(device.m_readOverlapped), m_readBuffer(std::move(const_cast<HidDevice&>(device).m_readBuffer)), m_readBufferSize(device.m_readBufferSize), m_readEvent(device.m_readEvent), m_writeEvent(device.m_writeEvent), m_readPending(device.m_readPending), m_hardwareId(device.m_hardwareId), m_manager(device.m_manager), m_readSignal(device.m_readSignal), m_stopEvent(device.m_stopEvent), m_stale(device.m_stale), m_devInst(device.m_devInst) {
    memcpy(this->unused_0040, device.unused_0040, sizeof(this->unused_0040));
    const_cast<HidDevice&>(device).m_file = nullptr;
    const_cast<HidDevice&>(device).m_preparsedData = nullptr;
    strcpy_s(this->m_productString, sizeof(this->m_productString), device.m_productString);
    strcpy_s(this->m_serialNumber, sizeof(this->m_serialNumber), device.m_serialNumber);
}

// VERIFY: 1f4838f4 manual
// OFFSET: 0x008C3B10
HidDevice::~HidDevice() {
    if (this->m_stopEvent) {
        SetEvent(this->m_stopEvent);
    }

    if (this->m_thread) {
        if (WaitForSingleObject(this->m_thread, 5000) == WAIT_TIMEOUT) {
            TerminateThread(this->m_thread, 0);
        }

        CloseHandle(this->m_thread);
        this->m_thread = nullptr;
    }

    this->ReleaseHandles();

    if (this->m_readSignal) {
        CloseHandle(this->m_readSignal);
    }

    if (this->m_stopEvent) {
        CloseHandle(this->m_stopEvent);
    }

    if (this->m_readBuffer.get()) {
        delete[] this->m_readBuffer.release();
    }
}

// INLINED
bool HidDevice::operator<(const HidDevice& device) const {
    return this->m_devInst < device.m_devInst;
}

// VERIFY: 2fb3f3d8 manual
// OFFSET: 0x008C3000
void HidDevice::ReleaseHandles() {
    if (this->m_preparsedData) {
        HidD_FreePreparsedData(this->m_preparsedData);
        this->m_preparsedData = nullptr;
    }

    if (this->m_file) {
        this->m_readEvent = nullptr;
        this->m_writeEvent = nullptr;
        this->m_readPending = 0;
        CancelIo(this->m_file);
        CloseHandle(this->m_file);
        this->m_file = nullptr;
    }
}

// VERIFY: f378d0a6 manual
// OFFSET: 0x008C34B0
bool HidDevice::Open(int32_t read, int32_t write, int32_t overlapped, int32_t exclusive, int32_t query) {
    DWORD share = 0;
    DWORD access = 0;

    if (this->m_file) {
        if (this->m_openRead == read && this->m_openWrite == write && this->m_openOverlapped == overlapped && this->m_openExclusive == exclusive) {
            return true;
        }

        this->ReleaseHandles();
    }

    if (read) {
        access = GENERIC_READ;
    }

    if (write) {
        access |= GENERIC_WRITE;
    }

    if (!exclusive) {
        share = FILE_SHARE_READ | FILE_SHARE_WRITE;
    }

    this->m_file = CreateFileA(this->m_interfaceDetail->DevicePath, access, share, nullptr, OPEN_EXISTING, overlapped ? FILE_FLAG_OVERLAPPED : 0, nullptr);

    if (this->m_file == INVALID_HANDLE_VALUE) {
        GetLastError();
        return false;
    }

    this->m_openRead = read;
    this->m_openWrite = write;
    this->m_openOverlapped = overlapped;
    this->m_openExclusive = exclusive;

    if (query) {
        try {
            if (!HidD_GetPreparsedData(this->m_file, &this->m_preparsedData)) {
                throw 1;
            }

            this->m_attributes.Size = sizeof(this->m_attributes);

            if (!HidD_GetAttributes(this->m_file, &this->m_attributes)) {
                throw 1;
            }

            if (!HidP_GetCaps(this->m_preparsedData, &this->m_caps)) {
                throw 1;
            }

            if (!HidD_GetProductString(this->m_file, this->m_productString, sizeof(this->m_productString))) {
                throw 1;
            }

            HidD_GetSerialNumberString(this->m_file, this->m_serialNumber, sizeof(this->m_serialNumber));
        } catch (...) {
            this->ReleaseHandles();
            return false;
        }
    }

    return true;
}

// VERIFY: 5f84aaec manual
// OFFSET: 0x008C3680
bool HidDevice::Write(const void* data, uint32_t size) {
    if (!this->Open(1, 1, 0, 0, 0) || !this->m_file) {
        return false;
    }

    DWORD written;
    return WriteFile(this->m_file, data, size, &written, nullptr) != 0;
}

// VERIFY: bd90f3e6 manual
// OFFSET: 0x008C3270
VOID CALLBACK HidDevice::ReadCompletion(DWORD error, DWORD bytes, LPOVERLAPPED overlapped) {
    HidDevice* device = static_cast<HidDevice*>(overlapped->hEvent);
    device->m_readPending = 0;
    GetOverlappedResult(device->m_file, &device->m_readOverlapped, &device->m_bytesRead, FALSE);

    if (device->m_readEvent) {
        SetEvent(device->m_readEvent);
    }

    device->m_readEvent = nullptr;
}

// VERIFY: 150eaef1 manual
// OFFSET: 0x008C32C0
VOID CALLBACK HidDevice::WriteCompletion(DWORD error, DWORD bytes, LPOVERLAPPED overlapped) {
    HidDevice* device = static_cast<HidDevice*>(overlapped->hEvent);

    if (device->m_writeEvent) {
        SetEvent(device->m_writeEvent);
    }

    device->m_writeEvent = nullptr;
}

// VERIFY: f5afc03e manual
// OFFSET: 0x008C36D0
bool HidDevice::BeginRead(uint32_t size, HANDLE event) {
    if (this->m_readPending) {
        return false;
    }

    if (!this->Open(1, 1, 1, 0, 0) || !this->m_file) {
        return false;
    }

    this->m_readOverlapped.Offset = 0;
    this->m_readOverlapped.OffsetHigh = 0;
    this->m_readOverlapped.hEvent = this;

    if (this->m_readBufferSize && size > this->m_readBufferSize) {
        delete[] this->m_readBuffer.release();
        this->m_readBufferSize = 0;
    }

    if (!this->m_readBufferSize) {
        this->m_readBuffer = std::unique_ptr<uint8_t>(static_cast<uint8_t*>(SMemAlloc(size, __FILE__, __LINE__, 0)));
        this->m_readBufferSize = size;
    }

    this->m_readEvent = event;
    this->m_readPending = size;
    ReadFileEx(this->m_file, this->m_readBuffer.get(), this->m_readBufferSize, &this->m_readOverlapped, ReadCompletion);
    return true;
}

// VERIFY: 29d43e3b manual
// OFFSET: 0x008C37C0
bool HidDevice::WriteEx(const void* data, uint32_t size, HANDLE event) {
    if (!this->Open(1, 1, 1, 0, 0) || !this->m_file) {
        return false;
    }

    this->m_writeEvent = event;
    this->m_writeOverlapped.Offset = 0;
    this->m_writeOverlapped.OffsetHigh = 0;
    this->m_writeOverlapped.hEvent = this;
    return WriteFileEx(this->m_file, data, size, &this->m_readOverlapped, WriteCompletion) != 0;
}

// VERIFY: d440c369 manual
// OFFSET: 0x008C3830
void HidDevice::ReadLoop() {
    this->BeginRead(3, this->m_readSignal);
    HANDLE handles[2] = { this->m_readSignal, this->m_stopEvent };

    while (true) {
        DWORD result = WaitForMultipleObjectsEx(2, handles, FALSE, INFINITE, TRUE);

        if (result == WAIT_OBJECT_0) {
            DWORD bytes = this->m_bytesRead;
            const uint8_t* buffer = this->m_readBuffer.get();
            this->m_bytesRead = 0;

            if (bytes) {
                if (this->m_manager && this->m_manager->m_window) {
                    uint32_t report = 0;
                    reinterpret_cast<uint16_t*>(&report)[0] = *reinterpret_cast<const uint16_t*>(buffer);
                    reinterpret_cast<uint8_t*>(&report)[2] = buffer[2];
                    PostMessageA(this->m_manager->m_window, WM_USER + 3, report >> 3, reinterpret_cast<LPARAM>(this));
                }

                this->BeginRead(3, this->m_readSignal);
            }
        } else if (result == WAIT_OBJECT_0 + 1) {
            return;
        }
    }
}

// VERIFY: 3ff915a9 manual
// OFFSET: 0x008C3930
DWORD WINAPI HidDevice::ThreadProc(void* param) {
    HidDevice* device = static_cast<HidDevice*>(param);

    if (device->m_threadProc) {
        (device->*device->m_threadProc)();
    } else {
        device->ReadLoop();
    }

    device->m_threadId = 0;
    return 0;
}

// VERIFY: faedbae4 auto
// OFFSET: 0x008C3960
bool HidDevice::WriteWait(const void* data, uint32_t size) {
    HANDLE event = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    bool written = this->WriteEx(data, size, event);

    if (written) {
        while (WaitForSingleObjectEx(event, INFINITE, TRUE)) {
        }
    }

    CloseHandle(event);
    return written;
}

// VERIFY: 335b116f manual
// OFFSET: 0x008C3C40
bool HidDevice::StartReadThread(HidManagerImpl* manager) {
    if (!this->Open(1, 1, 1, 0, 0)) {
        return false;
    }

    this->m_manager = manager;

    if (!this->m_readSignal) {
        this->m_readSignal = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    }

    if (!this->m_stopEvent) {
        this->m_stopEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    }

    ResetEvent(this->m_readSignal);
    ResetEvent(this->m_stopEvent);

    if (!this->m_thread) {
        this->m_threadProc = nullptr;
        this->m_thread = CreateThread(nullptr, 0, ThreadProc, this, 0, &this->m_threadId);
    }

    return true;
}

// VERIFY: 3a2bc364 manual
// OFFSET: 0x008C4040
bool HidDevice::GetDeviceProperty(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD property, std::string& value) {
    DWORD required = 0;
    DWORD type;
    SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, deviceInfoData, property, &type, nullptr, 0, &required);
    DWORD error = GetLastError();

    if (!required || (error != ERROR_INSUFFICIENT_BUFFER && error != ERROR_SUCCESS)) {
        return false;
    }

    do {
        value.resize(required);
        SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, deviceInfoData, property, &type, reinterpret_cast<PBYTE>(const_cast<char*>(value.c_str())), static_cast<DWORD>(value.size()), &required);
        error = GetLastError();
    } while (error == ERROR_INSUFFICIENT_BUFFER);

    if (error != ERROR_SUCCESS) {
        return false;
    }

    _mbsupr_s(reinterpret_cast<unsigned char*>(const_cast<char*>(value.c_str())), value.size());
    size_t length = value.find(' ') - 1;

    if (length >= strlen(value.c_str())) {
        length = strlen(value.c_str());
    }

    value.resize(length);
    return true;
}

// VERIFY: 1b93022f manual
// OFFSET: 0x008C4250
bool HidDevice::EnumerateInterface(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, DWORD index) {
    DWORD required = 0;
    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    SP_DEVICE_INTERFACE_DATA interfaceData;
    interfaceData.cbSize = sizeof(interfaceData);

    if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, deviceInfoData, &hidGuid, index, &interfaceData)) {
        return false;
    }

    SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &interfaceData, nullptr, 0, &required, nullptr);

    if (!required) {
        return false;
    }

    this->m_interfaceDetail = std::unique_ptr<SP_DEVICE_INTERFACE_DETAIL_DATA_A>(static_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_A*>(SMemAlloc(required, __FILE__, __LINE__, 0)));
    this->m_interfaceDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

    if (!SetupDiGetDeviceInterfaceDetailA(deviceInfoSet, &interfaceData, this->m_interfaceDetail.get(), required, &required, nullptr)) {
        return false;
    }

    if (!this->GetDeviceProperty(deviceInfoSet, deviceInfoData, SPDRP_HARDWAREID, this->m_hardwareId)) {
        return false;
    }

    this->m_devInst = deviceInfoData->DevInst;

    if (!this->Open(0, 0, 0, 0, 1)) {
        return false;
    }

    this->ReleaseHandles();
    return true;
}

// VERIFY: 9699ba4f manual
// OFFSET: 0x008C4F90
HidManagerImpl::HidManagerImpl() : unused_0010(0), unused_0014(0), m_window(nullptr), m_inputDevInst(0), m_outputDevInst(0) {
}

// VERIFY: 90088bd5 manual
// OFFSET: [0x008C4EE0, 0x008C5020]
HidManagerImpl::~HidManagerImpl() {
    this->m_devices.clear();
}

// VERIFY: 880f51d6 manual
// OFFSET: 0x008C4A90
bool HidManagerImpl::EnumerateDevices() {
    bool found = false;

    for (std::set<HidDevice>::iterator it = this->m_devices.begin(); it != this->m_devices.end(); ++it) {
        const_cast<HidDevice&>(*it).m_stale = 1;
    }

    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);
    HDEVINFO deviceInfoSet = SetupDiGetClassDevsA(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(deviceInfoData);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        HidDevice& device = this->GetDevice(deviceInfoData.DevInst);

        if (HidDevice(0) < device) {
            device.m_stale = 0;
        } else {
            for (DWORD index = 0;; index++) {
                HidDevice candidate;

                if (!candidate.EnumerateInterface(deviceInfoSet, &deviceInfoData, index)) {
                    break;
                }

                char hardwareId[100];
                strcpy_s(hardwareId, sizeof(hardwareId), candidate.m_hardwareId.c_str());
                memset(&hardwareId[26], '0', 4);

                if (!_mbsnicmp(reinterpret_cast<const unsigned char*>(hardwareId), reinterpret_cast<const unsigned char*>("HID\\VID_1038&PID_0777&REV_0000&MI_01"), 36)) {
                    this->m_inputDevInst = candidate.m_devInst;
                    std::pair<std::set<HidDevice>::iterator, bool> result = this->m_devices.insert(candidate);
                    const_cast<HidDevice&>(*result.first).StartReadThread(this);
                    found = true;
                } else {
                    strcpy_s(hardwareId, sizeof(hardwareId), candidate.m_hardwareId.c_str());
                    memset(&hardwareId[26], '0', 4);

                    if (!_mbsnicmp(reinterpret_cast<const unsigned char*>(hardwareId), reinterpret_cast<const unsigned char*>("HID\\VID_1038&PID_0777&REV_0000&MI_00"), 36)) {
                        this->m_outputDevInst = candidate.m_devInst;
                        this->m_devices.insert(candidate);
                    }
                }
            }
        }

        deviceInfoData.cbSize = sizeof(deviceInfoData);
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    for (std::set<HidDevice>::iterator it = this->m_devices.begin(); it != this->m_devices.end();) {
        std::set<HidDevice>::iterator next = it;
        ++next;

        if (it->m_stale) {
            if (it->m_devInst == this->m_outputDevInst) {
                this->m_outputDevInst = 0;
            }

            if (it->m_devInst == this->m_inputDevInst) {
                this->m_inputDevInst = 0;
            }

            this->m_devices.erase(it);
        }

        it = next;
    }

    return found;
}

// VERIFY: 85f16669 manual
// OFFSET: 0x008C3050
void HidManagerImpl::SetWindow(void* window) {
    this->m_window = static_cast<HWND>(window);
}

// VERIFY: 90d469ad manual
// OFFSET: 0x008C32F0
uint32_t HidManagerImpl::GetDeviceCount() {
    return static_cast<uint32_t>(this->m_devices.size());
}

// VERIFY: ceed9e3c manual
// OFFSET: 0x008C3D80
bool HidManagerImpl::WriteWait(const void* data, uint32_t size) {
    if (!this->m_outputDevInst) {
        return false;
    }

    return this->GetDevice(this->m_outputDevInst).WriteWait(data, size);
}

// VERIFY: 1dcaf049 manual
// OFFSET: 0x008C3DB0
bool HidManagerImpl::Write(const void* data, uint32_t size) {
    if (!this->m_outputDevInst) {
        return false;
    }

    return this->GetDevice(this->m_outputDevInst).Write(data, size);
}

// VERIFY: 362cf46c manual
// OFFSET: 0x008C3DE0
bool HidManagerImpl::SetFeature(void* data, uint32_t size) {
    if (!this->m_outputDevInst) {
        return false;
    }

    HidDevice& device = this->GetDevice(this->m_outputDevInst);

    if (!device.Open(0, 1, 0, 0, 0) || !device.m_file) {
        return false;
    }

    return HidD_SetFeature(device.m_file, data, size) != 0;
}

// VERIFY: 34318f6f manual
// OFFSET: 0x008C3CF0
HidDevice& HidManagerImpl::GetDevice(DWORD devInst) {
    std::set<HidDevice>::iterator it = this->m_devices.find(HidDevice(devInst));

    if (it == this->m_devices.end()) {
        return this->m_nullDevice;
    }

    return const_cast<HidDevice&>(*it);
}

#endif
