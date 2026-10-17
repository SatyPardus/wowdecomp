// HelloWorld: the smallest possible extension. It registers one script of every type and logs the
// hooks it receives, so the discovery -> registration -> dispatch chain of each context can be
// checked end to end in Logs\Extensions.log, and it uses each registration API once (a Lua
// function and event, a message handler and a send, an XML element type). It is the template for
// a real extension; it does nothing else on purpose. Per-frame and per-object hooks log their
// first calls and counts afterwards, never every call.

void AddHelloWorldAssetScripts();
void AddHelloWorldClientDBScripts();
void AddHelloWorldClientScripts();
void AddHelloWorldConsoleScripts();
void AddHelloWorldLuaScripts();
void AddHelloWorldMapObjectScripts();
void AddHelloWorldModelScripts();
void AddHelloWorldNetScripts();
void AddHelloWorldObjectScripts();
void AddHelloWorldRenderScripts();
void AddHelloWorldTerrainScripts();
void AddHelloWorldTextureScripts();
void AddHelloWorldUIScripts();
void AddHelloWorldUnitScripts();
void AddHelloWorldWorldScripts();

// The entry point the generated loader calls: Add<DirectoryName>Extension(). Instantiate every
// script of the extension from here; a script registers itself in its constructor and the registry
// owns it from then on. Registrations (functions, events, handlers, element types) go here too.
void AddHelloWorldExtension() {
    AddHelloWorldAssetScripts();
    AddHelloWorldClientDBScripts();
    AddHelloWorldClientScripts();
    AddHelloWorldConsoleScripts();
    AddHelloWorldLuaScripts();
    AddHelloWorldMapObjectScripts();
    AddHelloWorldModelScripts();
    AddHelloWorldNetScripts();
    AddHelloWorldObjectScripts();
    AddHelloWorldRenderScripts();
    AddHelloWorldTerrainScripts();
    AddHelloWorldTextureScripts();
    AddHelloWorldUIScripts();
    AddHelloWorldUnitScripts();
    AddHelloWorldWorldScripts();
}
