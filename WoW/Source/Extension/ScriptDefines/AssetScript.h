#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_ASSETSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_ASSETSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

class SFileImpl;

// Data file access. Every file the client reads by name from the archives or the data directory
// (.dbc, .m2, .skin, .anim, .blp, .wmo, .adt, FrameXML, addons, sounds) goes through
// SFile::OpenEx(nullptr, name, ...) (Engine/Source/BlizzardCore/SFile2-Core.cpp), either directly
// or through SFile::ReadFile / SFile::LoadFile. The hook runs in that function before the name is
// looked up (System_SFile2::FindFile), so a script can substitute another name and the lookup,
// the open and the read all happen on the substitute as if it had been asked for.
//
// SFile::OpenEx is called from whichever thread opens a file: the main thread for almost
// everything, the streaming and data-preload threads in trial/streaming mode. A hook must not
// touch engine state and must be reentrant; reading its own immutable tables is fine.
class AssetScript : public ScriptObject {
    protected:
    explicit AssetScript(const char* name);

    public:
    // name is the path being opened, as the caller spelled it (archive paths use backslashes,
    // case varies). To redirect, copy the substitute path into redirect (redirectSize bytes,
    // 260) and return true; the first script that returns true wins and later scripts are not
    // asked. Return false to leave the request alone. The substitute must itself exist in the
    // archives or under the data path: this hook changes which file is opened, it does not
    // supply bytes. Not consulted for opens that name an archive explicitly (SFile::OpenEx with
    // an SArchive). SFile::FileExists and FileExistsEx ask the same question, so an existence
    // check follows the redirect too.
    virtual bool OnFileOpen(const char* name, char* redirect, uint32_t redirectSize) { return false; }

    // SFile::OpenEx on success, before the handle is returned to the caller: name is the path
    // that was opened (after any redirect), file the open handle (file.m_type tells the source:
    // 0 a file on disk, 3 an archive entry, 5 fully read into file.m_data for a 0x20000 open;
    // file.m_name the resolved name). Observe only; the caller owns and closes the handle.
    virtual void OnFileOpened(const char* name, SFileImpl& file) {}
};

#endif
