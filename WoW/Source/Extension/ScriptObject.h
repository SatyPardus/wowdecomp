#ifndef WOW_SOURCE_EXTENSION_SCRIPTOBJECT_H
#define WOW_SOURCE_EXTENSION_SCRIPTOBJECT_H

#include <cstdint>

// Base of every script an extension can register. A script is a plain object with one virtual per
// hook: an extension derives from one of the script types under ScriptDefines/ (ClientScript,
// WorldScript, RenderScript, NetScript, LuaScript), overrides the hooks it needs and instantiates it
// with new from its Add<Name>Extension() entry point. The script type's constructor adds the
// instance to ScriptRegistry<T>, which owns it from then on and deletes it in ExtensionMgr::Unload().
//
// Hooks are called on the main thread, in registration order (the order the generated loader called
// the extensions in, which is the sorted directory order, then the order of the new expressions
// inside an entry point).
//
// Only hooks that the client actually fires are declared. A planned hook is listed in
// docs/extensions.md until its call site exists, so an override can never compile and then
// silently do nothing.
class ScriptObject {
    friend class ExtensionMgr;

    public:
    const char* GetName() const { return m_name; }

    protected:
    explicit ScriptObject(const char* name) : m_name(name) {}
    virtual ~ScriptObject() = default;

    private:
    const char* m_name;
};

#endif
