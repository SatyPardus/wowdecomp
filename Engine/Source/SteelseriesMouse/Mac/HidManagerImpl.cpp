#if defined(__APPLE__)

#include <new>
#include "Engine/Source/SteelseriesMouse/HidManager.h"
#include "Engine/Source/Storm/H/storm.h"

class HidManagerImpl final : public HidManager {
    public:
    bool EnumerateDevices() override { return false; }
    void SetWindow(void* window) override { static_cast<void>(window); }
    uint32_t GetDeviceCount() override { return 0; }
    bool WriteWait(const void* data, uint32_t size) override {
        static_cast<void>(data);
        static_cast<void>(size);
        return false;
    }
    bool Write(const void* data, uint32_t size) override {
        static_cast<void>(data);
        static_cast<void>(size);
        return false;
    }
    bool SetFeature(void* data, uint32_t size) override {
        static_cast<void>(data);
        static_cast<void>(size);
        return false;
    }
};

HidManager::~HidManager() {
}

HidManager* HidManager::Create() {
    return new (SMemAlloc(sizeof(HidManagerImpl), __FILE__, __LINE__, 0)) HidManagerImpl();
}

#endif
