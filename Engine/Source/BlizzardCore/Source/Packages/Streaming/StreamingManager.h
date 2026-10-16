#ifndef ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_STREAMINGMANAGER_H
#define ENGINE_SOURCE_BLIZZARDCORE_SOURCE_PACKAGES_STREAMING_STREAMINGMANAGER_H

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/System/Lock/Lock.h"
#include "Engine/Source/BlizzardCore/Source/System/Thread/Thread.h"

namespace System_SFile {
class ARCHIVEREC;
}

class StreamingFileEntry {
    public:
    /* 0000 */ std::string m_name;
    /* 001C */ std::vector<std::string> m_files;
    /* 002C */ uint32_t m_cursor;
    /* 0030 */ uint32_t m_next;
    /* 0034 */ uint32_t unused_0034;
    /* 0038 */ int64_t m_localAmount;
    /* 0040 */ bool m_complete;
    /* 0044 */ int32_t m_priority;
    /* 0048 */ System_SFile::ARCHIVEREC* m_archive;

    StreamingFileEntry();
    ~StreamingFileEntry();
    void AdvanceCursor();
    float GetLocalFraction();
    bool LoadListfile(const char* archiveName, const char* name, const char* extension);
    const std::string& NextFile();
    void ParseListfile(const char* text, const char* extension);
    void ProcessQueue();
};

class StreamingManager {
    public:
    /* 0000 */ std::map<std::string, StreamingFileEntry*> m_entries;
    /* 000C */ int32_t m_state;
    /* 0010 */ int32_t m_gameState;
    /* 0014 */ bool m_enabled;
    /* 0018 */ int32_t m_zone;
    /* 001C */ int32_t m_subzone;
    /* 0020 */ int32_t m_mapId;
    /* 0024 */ int32_t m_race;
    /* 0028 */ int32_t m_area;
    /* 002C */ int32_t m_areaChunk;
    /* 0030 */ bool m_dirty;
    /* 0031 */ bool unused_0031;
    /* 0034 */ uint8_t unused_0034[260];
    /* 0138 */ StreamingFileEntry* m_current;
    /* 013C */ Blizzard::Lock::Event m_event;
    /* 0140 */ Blizzard::Thread::ThreadRecord* m_thread;

    StreamingManager();
    ~StreamingManager();
    void AdvanceState(uint32_t& index);
    StreamingFileEntry* CreateEntry(int32_t type, const std::string& key);
    float GetBaseFraction();
    float GetLoadFraction(int32_t race);
    int32_t GetNextState();
    bool IsBaseDataReady();
    bool IsRaceDataReady(int32_t race);
    uint32_t Run();
    void SelectEntry(StreamingFileEntry* previous);
    void Shutdown();
};

std::string BuildManifestKey(int32_t type, int32_t race, int32_t zone, uint32_t index);
const char* GetLocaleArchiveName(uint32_t index);
int32_t GetManifestZone(uint32_t index);
const char* GetRegionArchiveName(uint32_t index);
int32_t GetManifestRace(int32_t index);
bool OpenDataArchive(const char* name, System_SFile::ARCHIVEREC** archive);
std::string RaceIdToName(int32_t race);
uint32_t StreamingThreadProc(void* param);

#endif
