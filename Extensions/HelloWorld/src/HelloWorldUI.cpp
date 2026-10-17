#include <new>
#include <typeinfo>
#include "Engine/Source/Base/CDataAllocator.h"
#include "Engine/Source/Base/Status.h"
#include "Engine/Source/Frame/CScriptObject.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/FrameScript/FrameScript_Object.h"
#include "Engine/Source/FrameXML/XMLTree.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/lua-5.1/src/lauxlib.h"
#include "Engine/Source/lua-5.1/src/lua.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// A new UI element type, <HelloWorldFrame>, usable from XML and from CreateFrame("HelloWorldFrame").
// It is a Frame with one extra attribute, greeting="...", two Lua methods of its own
// (SetGreeting / GetGreeting, through a script metatable the core creates in every state) and its
// own object type so Lua's GetObjectType() / IsObjectType("HelloWorldFrame") recognise it. Frames
// are destroyed through the virtual destructor and the static operator delete of their class, so
// the type owns its allocator like CGCooldown does.
class CHelloWorldFrame : public CSimpleFrame {
    public:
    static CDataAllocator s_allocator;
    static int32_t s_metatable;
    static int32_t s_objectType;
    static const char* s_objectTypeName;

    static void operator delete(void* ptr) {
        if (ptr) {
            CHelloWorldFrame::s_allocator.PutData(ptr, nullptr, 0);
        }
    }

    static CSimpleFrame* Create(CSimpleFrame* parent) {
        void* data = CHelloWorldFrame::s_allocator.GetData(0, typeid(CHelloWorldFrame).raw_name(), SLOG_OBJECT);
        return data ? new (data) CHelloWorldFrame(parent) : nullptr;
    }

    static int32_t GetObjectType() {
        if (!CHelloWorldFrame::s_objectType) {
            CHelloWorldFrame::s_objectType = ++FrameScript_Object::s_objectTypes;
        }

        return CHelloWorldFrame::s_objectType;
    }

    explicit CHelloWorldFrame(CSimpleFrame* parent) : CSimpleFrame(parent) {
        this->m_greeting[0] = '\0';
    }

    bool IsA(int32_t type) override {
        return type == CHelloWorldFrame::GetObjectType() || CSimpleFrame::IsA(type);
    }

    bool IsA(const char* typeName) override {
        return !SStrCmpI(typeName, CHelloWorldFrame::s_objectTypeName, STORM_MAX_STR) || CSimpleFrame::IsA(typeName);
    }

    const char* GetObjectTypeName() override {
        return CHelloWorldFrame::s_objectTypeName;
    }

    // The type's own method table when the core has built it for this state, the plain Frame
    // table otherwise.
    int32_t GetScriptMetaTable() override {
        return CHelloWorldFrame::s_metatable != LUA_NOREF ? CHelloWorldFrame::s_metatable : CSimpleFrame::GetScriptMetaTable();
    }

    // The Frame attributes and children first, then the type's own attribute.
    void LoadXML(XMLNode* node, CStatus* status) override {
        CSimpleFrame::LoadXML(node, status);

        const char* greeting = node->GetAttributeByName("greeting");
        const char* name = node->GetAttributeByName("name");
        SStrCopy(this->m_greeting, greeting ? greeting : "", sizeof(this->m_greeting));
        sExtensionMgr->Log("HelloWorld: <HelloWorldFrame> %s created, greeting=\"%s\"", name && *name ? name : "(unnamed)", this->m_greeting);
    }

    char m_greeting[64];
};

CDataAllocator CHelloWorldFrame::s_allocator(sizeof(CHelloWorldFrame), 16);
int32_t CHelloWorldFrame::s_metatable = LUA_NOREF;
int32_t CHelloWorldFrame::s_objectType;
const char* CHelloWorldFrame::s_objectTypeName = "HelloWorldFrame";

// frame:SetGreeting("text") / frame:GetGreeting(): the type's Lua methods, in the shape of every
// engine frame method (FrameScript_GetObjectThis checks the object type).
int32_t Script_SetGreeting(lua_State* L) {
    CHelloWorldFrame* frame = static_cast<CHelloWorldFrame*>(FrameScript_GetObjectThis(L, CHelloWorldFrame::GetObjectType()));
    const char* greeting = lua_tolstring(L, 2, nullptr);
    SStrCopy(frame->m_greeting, greeting ? greeting : "", sizeof(frame->m_greeting));

    return 0;
}

int32_t Script_GetGreeting(lua_State* L) {
    CHelloWorldFrame* frame = static_cast<CHelloWorldFrame*>(FrameScript_GetObjectThis(L, CHelloWorldFrame::GetObjectType()));
    lua_pushstring(L, frame->m_greeting);

    return 1;
}

FrameScript_Method s_helloWorldFrameMethods[] = {
    { "SetGreeting", &Script_SetGreeting },
    { "GetGreeting", &Script_GetGreeting },
};

// Fills the type's method table: the Frame methods, then its own.
void RegisterHelloWorldFrameMethods(lua_State* L) {
    CSimpleFrame::RegisterScriptMethods(L);
    FrameScript_Object::FillScriptMethodTable(L, s_helloWorldFrameMethods, sizeof(s_helloWorldFrameMethods) / sizeof(s_helloWorldFrameMethods[0]));
}

// Counts every frame the UI builds and destroys and reports element types nothing knows.
class HelloWorldUIScript : public UIScript {
    public:
    HelloWorldUIScript() : UIScript("HelloWorldUIScript") {}

    void OnFrameCreated(CSimpleFrame& frame, XMLNode& node) override {
        m_created++;

        if (m_created <= 3 || m_created % 1000 == 0) {
            sExtensionMgr->Log("HelloWorld: OnFrameCreated <%s> (%u frame(s) created, %u destroyed)", node.m_name.m_str, m_created, m_destroyed);
        }
    }

    void OnFrameDestroyed(CSimpleFrame& frame) override {
        m_destroyed++;

        if (m_destroyed <= 3) {
            sExtensionMgr->Log("HelloWorld: OnFrameDestroyed %s", frame.GetObjectTypeName());
        }
    }

    void OnUnknownFrameType(const char* name, XMLNode& node) override {
        sExtensionMgr->Log("HelloWorld: OnUnknownFrameType <%s>", name);
    }

    private:
    uint32_t m_created = 0;
    uint32_t m_destroyed = 0;
};

}

void AddHelloWorldUIScripts() {
    sExtensionMgr->RegisterXmlElement("HelloWorldFrame", CHelloWorldFrame::Create);
    sExtensionMgr->RegisterScriptMetaTable(&CHelloWorldFrame::s_metatable, RegisterHelloWorldFrameMethods);
    new HelloWorldUIScript();
}
