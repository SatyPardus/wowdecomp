#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CLIENTDBSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_CLIENTDBSCRIPT_H

#include "WoW/Source/Extension/ScriptObject.h"

class WowClientDB_Base;

// The client databases (WoW/Source/DB): every DBFilesClient\*.dbc store is a WowClientDB<T> or
// WowClientDB_RLE<T> global whose Load (WowClientDB_Common<T>::Load, WoW/Source/DB/WowClientDB.h)
// opens T::GetFilename(), checks the WDBC header against T::s_numColumns / T::s_rowSize, reads
// the records through T::Read and the string block. ClientDBInitialize (DBClient.cpp) loads the
// stock stores in one batch (StaticDBLoadAll) and builds the derived tables; a few stores load
// elsewhere through the same Load (Startup_Strings before the extensions exist, VideoHardware,
// the Light* stores in InitLights).
//
// An extension adds a store of its own by defining a record type with the same contract as
// WoW/Source/DB/Rec/*Rec.h (GetFilename, s_numColumns, s_rowSize, s_generatedID, m_id, Read) and
// a WowClientDB<MyRec> global, then calling Load on it from OnClientDBInitialize. Load is fatal
// when the file is missing or its header disagrees, so check SFile::FileExists first when the
// file is optional.
class ClientDBScript : public ScriptObject {
    protected:
    explicit ClientDBScript(const char* name);

    public:
    // End of WowClientDB_Common<T>::Load on success: the records and strings are in memory,
    // db.m_numRecords / m_minID / m_maxID are set, db.m_loaded is 1. fileName is T::GetFilename()
    // (the archive path of the .dbc). Not fired for a store whose file has zero records (Load
    // returns early and leaves it unloaded) and, for the stores loaded before the extensions are
    // initialised, nobody is registered yet to receive it.
    virtual void OnClientDBLoaded(WowClientDB_Base& db, const char* fileName) {}

    // End of ClientDBInitialize: every stock store is loaded and the derived tables (game tables,
    // resistances, terrain type flags, weapon subclasses, item subclass table, lights) are built.
    // Load extension stores here.
    virtual void OnClientDBInitialize() {}

    // Start of ClientDBShutdown (from WowClientDestroy): the derived tables are about to be
    // cleared. Release what OnClientDBInitialize set up; the stores themselves free their
    // records in their destructors.
    virtual void OnClientDBShutdown() {}
};

#endif
