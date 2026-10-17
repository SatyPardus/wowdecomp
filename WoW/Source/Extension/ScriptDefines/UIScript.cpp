#include "WoW/Source/Extension/ScriptDefines/UIScript.h"
#include <vector>
#include "Engine/Source/FrameXML/FrameXML.h"
#include "Engine/Source/Storm/H/storm.h"
#include "WoW/Source/Extension/ExtensionMgr.h"
#include "WoW/Source/Extension/ExtensionMgrMacros.h"

// Element types registered by extensions. They enter FrameXML's factory table right after the
// engine's own (OnFrameXmlInitialized, from InitializeClient); one registered later, while the
// table exists, enters it at once. The table is cleared only when the client is destroyed.
struct EXTENSIONXMLELEMENT {
    const char* m_name;
    ExtensionFrameFactory m_factory;
    bool m_unique;
};

static std::vector<EXTENSIONXMLELEMENT> s_xmlElements;
static bool s_frameXmlInitialized;

static void RegisterElement(const EXTENSIONXMLELEMENT& element) {
    // FrameXML_RegisterFactory refuses a name that is already in the table; the engine's names
    // are in before any extension's, so a collision always means the stock type stays.
    if (FrameXML_RegisterFactory(element.m_name, element.m_factory, element.m_unique)) {
        sExtensionMgr->Log("Extensions: XML element <%s> registered", element.m_name);
    } else {
        sExtensionMgr->Log("Extensions: XML element <%s> not registered, the name is taken", element.m_name);
    }
}

void ExtensionMgr::RegisterXmlElement(const char* name, ExtensionFrameFactory factory, bool unique) {
    if (!name || !factory) {
        return;
    }

    for (const EXTENSIONXMLELEMENT& element : s_xmlElements) {
        if (!SStrCmpI(element.m_name, name, STORM_MAX_STR)) {
            Log("Extensions: XML element <%s> already registered by an extension, ignored", name);
            return;
        }
    }

    s_xmlElements.push_back({ name, factory, unique });

    if (s_frameXmlInitialized) {
        RegisterElement(s_xmlElements.back());
    }
}

void ExtensionMgr::OnFrameXmlInitialized() {
    s_frameXmlInitialized = true;

    for (const EXTENSIONXMLELEMENT& element : s_xmlElements) {
        RegisterElement(element);
    }
}

void ExtensionMgr::ClearXmlElements() {
    s_xmlElements.clear();
    s_frameXmlInitialized = false;
}

void ExtensionMgr::OnFrameCreated(CSimpleFrame* frame, XMLNode* node) {
    CALL_HOOKS(UIScript, script->OnFrameCreated(*frame, *node));
}

void ExtensionMgr::OnUnknownFrameType(const char* name, XMLNode* node) {
    CALL_HOOKS(UIScript, script->OnUnknownFrameType(name, *node));
}

void ExtensionMgr::OnFrameDestroyed(CSimpleFrame* frame) {
    CALL_HOOKS(UIScript, script->OnFrameDestroyed(*frame));
}

UIScript::UIScript(const char* name) : ScriptObject(name) {
    ScriptRegistry<UIScript>::AddScript(this);
    sExtensionMgr->Log("Extensions: registered UIScript %s", name);
}
