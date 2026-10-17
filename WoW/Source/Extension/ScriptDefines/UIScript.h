#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_UISCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_UISCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

class CSimpleFrame;
class XMLNode;

// A frame factory an extension registers with ExtensionMgr::RegisterXmlElement: the shape of the
// engine's own Create_SimpleButton and of CGCooldown::Create. It allocates and constructs a
// CSimpleFrame subclass under parent (which may be null) and returns it; FrameXML then runs
// PreLoadXML / LoadXML / PostLoadXML on it with the element's XMLNode.
typedef CSimpleFrame* (*ExtensionFrameFactory)(CSimpleFrame* parent);

// FrameXML, the UI element layer (Engine/Source/FrameXML/FrameXML.cpp). An XML element name
// (<Frame>, <Button>, <Cooldown>, ...) maps to a frame factory in one name-keyed table:
// FrameXML_RegisterDefault fills it with the engine widgets at InitializeClient, the glue and the
// game UI add theirs (ModelFFX, WorldFrame, GameTooltip, Cooldown, Minimap, ...), and the table
// lives until WowClientDestroy. FrameXML_CreateFrame resolves an element through it, both for XML
// files and for Lua's CreateFrame("Type", ...), which builds a one-element XMLNode and takes the
// same path. ExtensionMgr::RegisterXmlElement adds an extension's element to that table.
//
// A new element type is a CSimpleFrame subclass: its own static object type
// (++FrameScript_Object::s_objectTypes), GetObjectTypeName / IsA overrides so Lua's
// GetObjectType() and IsObjectType() know it, a LoadXML override for its attributes and child
// elements, an allocator with operator delete (frames are freed through the virtual destructor and
// CSimpleFrame::operator delete returns memory to CSimpleFrame's own allocator, so a subclass
// allocated elsewhere must define its own), and optionally a script metatable
// (FrameScript_Object::CreateScriptMetaTable with a RegisterScriptMethods, created at each game
// state like CGCooldown::CreateScriptMetaTable in LoadScriptFunctions) when it adds Lua methods;
// without one it inherits the Frame methods.
class UIScript : public ScriptObject {
    protected:
    explicit UIScript(const char* name);

    public:
    // FrameXML_CreateFrame on success, after PreLoadXML / LoadXML / PostLoadXML: frame is the new
    // frame, node the element that described it (node.m_name is the element type, "name" /
    // "parent" / "inherits" its attributes). Every frame from an XML file and every Lua
    // CreateFrame comes through here, including the engine's and other extensions' types.
    virtual void OnFrameCreated(CSimpleFrame& frame, XMLNode& node) {}

    // FrameXML_CreateFrame when no factory is registered for the element: name is the element
    // type, node the element. The engine logs "Unknown frame type" and creates nothing; a script
    // cannot supply the frame here (register the type with RegisterXmlElement instead), it can
    // record or report the miss.
    virtual void OnUnknownFrameType(const char* name, XMLNode& node) {}

    // Start of CSimpleFrame::~CSimpleFrame: the frame is leaving (its lifecycle state is being
    // set to destroyed), still complete, about to unregister from its CSimpleTop and free its
    // regions, batches and children links. Every frame type derives from CSimpleFrame, so this
    // pairs with OnFrameCreated for any per-frame state an extension keeps.
    virtual void OnFrameDestroyed(CSimpleFrame& frame) {}
};

#endif
