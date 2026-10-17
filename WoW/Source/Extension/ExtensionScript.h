#ifndef WOW_SOURCE_EXTENSION_EXTENSIONSCRIPT_H
#define WOW_SOURCE_EXTENSION_EXTENSIONSCRIPT_H

#include <cstdint>

class ExtensionScript {
    public:
    virtual ~ExtensionScript() = default;

    const char* GetName() const { return m_name; }

    protected:
    explicit ExtensionScript(const char* name) : m_name(name) {}

    private:
    const char* m_name;
};

class ClientScript : public ExtensionScript {
    protected:
    explicit ClientScript(const char* name) : ExtensionScript(name) {}

    public:
    virtual void OnStartup() {}

    virtual void OnShutdown() {}

    virtual void OnEnterWorld(uint32_t mapId) {}


    virtual void OnLeaveWorld() {}
};

#endif