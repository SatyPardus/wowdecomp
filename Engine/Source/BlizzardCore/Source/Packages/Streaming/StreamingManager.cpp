#include "StreamingManager.h"
#include <cstdint>
#include <cstring>
#include <string>
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/MPQGlobals.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFile.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Mopaq/SFileArchives.h"
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"

// STATIC: 0x00AB7AD0
static std::string s_emptyFile;

// STATIC: 0x00AB7AEC
static const char* s_localeArchives[8] = { "terrain.MPQ", "wmo.MPQ", "model.MPQ", "texture.MPQ", "misc.MPQ", "fonts.MPQ", "interface.MPQ", "dbc.MPQ" };

// STATIC: 0x00AB7B0C
static const char* s_regionArchives[5] = { "Installer Tome.MPQ", "Movies.MPQ", "expansion.MPQ", "expansionspeech.MPQ", "expansionloc.MPQ" };

// STATIC: 0x00AB7B20
static const char* s_startClientList = "TrialLists/StartClient.lst";

// STATIC: 0x00AB7B24
static const char* s_startCharacterList = "TrialLists/StartCharacter.lst";

// STATIC: 0x00AB7B28
static const char* s_startRaceCommonList = "TrialLists/StartRaceCommon.lst";

// STATIC: 0x00AB7B2C
static const char* s_startRaceList = "TrialLists/Start%s.lst";

// STATIC: 0x00AB7B30
static const char* s_startBackgroundZoneList = "TrialLists/StartBackgroundZone%d.lst";

// STATIC: 0x00AB7B34
static const char* s_startBackgroundList = "TrialLists/StartBackground%s.lst";

// STATIC: 0x00AB7B74
static const char* s_lineSeparator = "\r\n";

// STATIC: 0x009E4DB8
static const int32_t s_manifestZones[7] = { 10, 44, 267, 331, 400, 406, 11 };

// STATIC: 0x00B31EEC
static int32_t s_savedBgPreloadSleep;

// VERIFY: 543c851a auto
// OFFSET: 0x0041E220
bool OpenDataArchive(const char* name, System_SFile::ARCHIVEREC** archive) {
    char path[260];
    Blizzard::String::JoinPath(path, sizeof(path), "Data\\", name);

    *archive = System_MPQGlobals::FindArchiveByName(path);

    if (*archive) {
        return true;
    }

    return Blizzard::Mopaq::SFileOpenArchive(path, 0, 0, archive);
}

// VERIFY: 64139cb0 manual
// OFFSET: 0x0041E280
int32_t StreamingManager::GetNextState() {
    switch (this->m_gameState) {
        case 1:
            return 1;

        case 2:
            return 3;

        case 3:
            return 4;

        case 4:
            return 12;

        default:
            return 0;
    }
}

// VERIFY: 467668b8 manual
// OFFSET: 0x0041E2C0
void StreamingManager::AdvanceState(uint32_t& index) {
    switch (this->m_state) {
        case 0:
            this->m_state = 1;
            break;

        case 1:
        case 3:
        case 5:
            this->m_state = 2;
            break;

        case 2:
            this->m_state = 6;
            break;

        case 4:
            this->m_state = 5;
            break;

        case 6:
            if (index == 8) {
                this->m_state = 7;
                index = 0;
            }

            break;

        case 7:
            if (index == 7) {
                this->m_state = this->m_enabled ? 9 : 11;
                index = 0;
            }

            break;

        case 8:
            if (index == 2) {
                this->m_state = 9;
                index = 0;
            }

            break;

        case 9:
            if (index == 8) {
                this->m_state = 11;
                index = 0;
            }

            break;

        case 10:
            if (index == 5) {
                this->m_state = 11;
                index = 0;
            }

            break;

        default:
            this->m_state = 12;
            break;
    }
}

// VERIFY: 47a1ea71 manual
// OFFSET: 0x0041E3B0
void StreamingManager::Shutdown() {
    if (this->m_state != 12) {
        this->m_state = 12;
        this->m_gameState = 4;
        this->m_dirty = true;
        Blizzard::Lock::EventSet(this->m_event);
    }

    if (this->m_thread) {
        Blizzard::Thread::WaitForever(this->m_thread);
        Blizzard::Thread::Close(this->m_thread);
    }

    this->m_thread = nullptr;
}

// VERIFY: 2dda1ba2 manual
// OFFSET: 0x0041E410
int32_t GetManifestZone(uint32_t index) {
    if (index > 6) {
        return 0;
    }

    return s_manifestZones[index];
}

// VERIFY: dffb6335 auto
// OFFSET: 0x0041E430
const char* GetLocaleArchiveName(uint32_t index) {
    if (index > 7) {
        return "";
    }

    return s_localeArchives[index];
}

// VERIFY: 195557b2 auto
// OFFSET: 0x0041E450
const char* GetRegionArchiveName(uint32_t index) {
    if (index > 4) {
        return "";
    }

    return s_regionArchives[index];
}

// VERIFY: 886669bf auto
// OFFSET: 0x0041E470
int32_t GetManifestRace(int32_t index) {
    switch (index) {
        case 0:
            return 1;

        case 1:
            return 4;

        case 2:
            return 5;

        case 3:
            return 6;

        case 4:
            return 2;

        case 5:
            return 7;

        case 6:
            return 3;

        case 7:
            return 8;

        default:
            return -1;
    }
}

// VERIFY: 1455f2fd auto
// OFFSET: 0x0041E4F0
void SetActiveDataSource(bool restore) {
    if (restore) {
        Blizzard::Streaming::FileManifest::SetDefaultBgPreloadSleep(s_savedBgPreloadSleep);
        return;
    }

    if (Blizzard::Streaming::FileManifest::GetBGPreloadSleep()) {
        s_savedBgPreloadSleep = Blizzard::Streaming::FileManifest::GetBGPreloadSleep();
    }

    Blizzard::Streaming::FileManifest::SetDefaultBgPreloadSleep(0);
}

// VERIFY: c456bdee manual
// OFFSET: 0x0041E820
float StreamingFileEntry::GetLocalFraction() {
    if (this->m_complete) {
        return 1.0f;
    }

    int64_t local = this->m_localAmount;
    int64_t total = this->m_localAmount;

    for (uint32_t i = this->m_cursor + 1; i < this->m_files.size(); i++) {
        Blizzard::Mopaq::SFileGetIsLocalAmount(this->m_files[i].c_str(), local, total);
    }

    return static_cast<float>(static_cast<double>(local) / static_cast<double>(total));
}

// VERIFY: f2a68773 manual
// OFFSET: 0x0041EBB0
std::string RaceIdToName(int32_t race) {
    switch (race) {
        case 1:
            return std::string("Human");

        case 2:
            return std::string("Orc");

        case 3:
            return std::string("Dwarf");

        case 4:
            return std::string("NightElf");

        case 5:
            return std::string("Undead");

        case 6:
            return std::string("Tauren");

        case 7:
            return std::string("Gnome");

        case 8:
            return std::string("Troll");

        default:
            return std::string();
    }
}

// VERIFY: 2e925757 manual
// OFFSET: 0x0041ECC0
std::string BuildManifestKey(int32_t type, int32_t race, int32_t zone, uint32_t index) {
    std::string key;
    char name[260];

    switch (type) {
        case 0:
            key = s_startClientList;
            break;

        case 1:
            key = s_startCharacterList;
            break;

        case 2:
            key = s_startRaceCommonList;
            break;

        case 3: {
            std::string raceName = RaceIdToName(race);

            if (raceName.size()) {
                Blizzard::String::Format(name, sizeof(name), s_startRaceList, raceName.c_str());
                key = name;
            } else {
                key = s_startRaceCommonList;
            }

            break;
        }

        case 4:
        case 7: {
            int32_t number = type == 4 ? zone : GetManifestZone(index);

            if (number <= 0) {
                break;
            }

            Blizzard::String::Format(name, sizeof(name), s_startBackgroundZoneList, number);
            key = std::string(name);
            break;
        }

        case 5:
        case 6: {
            std::string raceName = RaceIdToName(type == 5 ? race : GetManifestRace(index));

            if (raceName.size()) {
                Blizzard::String::Format(name, sizeof(name), s_startBackgroundList, raceName.c_str());
                key = name;
            }

            break;
        }

        case 9:
            key = GetLocaleArchiveName(index);
            break;

        case 10:
            key = GetRegionArchiveName(index);
            break;
    }

    return key;
}

// VERIFY: 3cb231be manual
// OFFSET: 0x0041F460
float StreamingManager::GetBaseFraction() {
    std::string key = BuildManifestKey(2, 0, 0, 0);
    std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.find(key);

    if (it == this->m_entries.end()) {
        return 0.0f;
    }

    return it->second->GetLocalFraction();
}

// VERIFY: 1da05e68 manual
// OFFSET: 0x0041F530
float StreamingManager::GetLoadFraction(int32_t race) {
    float base = this->GetBaseFraction() * 0.5f;

    std::string key = BuildManifestKey(3, race, 0, 0);
    std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.find(key);

    if (it == this->m_entries.end()) {
        return base;
    }

    return it->second->GetLocalFraction() * 0.5f + base;
}

// VERIFY: 27921cff manual
// OFFSET: 0x0041FE70
StreamingFileEntry::StreamingFileEntry() {
    this->m_cursor = 0;
    this->m_next = 0;
    this->m_localAmount = 0;
    this->m_complete = false;
    this->m_archive = nullptr;
    this->m_priority = 6;
}

// VERIFY: 6ea96e53 manual
// OFFSET: 0x0041FED0
StreamingFileEntry::~StreamingFileEntry() {
    this->m_files.erase(this->m_files.begin(), this->m_files.end());

    if (this->m_archive) {
        Blizzard::Mopaq::SFileCloseArchive(this->m_archive);
    }
}

// INLINED
const std::string& StreamingFileEntry::NextFile() {
    if (this->m_next < this->m_files.size()) {
        return this->m_files[this->m_next++];
    }

    return s_emptyFile;
}

// VERIFY: 37a9dd94 manual
// OFFSET: 0x0041FFA0
void StreamingFileEntry::AdvanceCursor() {
    uint32_t cursor = this->m_cursor;
    this->m_cursor = this->m_next;

    if (this->m_next == this->m_files.size()) {
        this->m_files.erase(this->m_files.begin(), this->m_files.end());
        this->m_complete = true;
        return;
    }

    int64_t total = 0;

    for (uint32_t i = cursor + 1; i < this->m_next; i++) {
        Blizzard::Mopaq::SFileGetIsLocalAmount(this->m_files[i].c_str(), this->m_localAmount, total);
    }
}

// VERIFY: 595705af manual
// OFFSET: 0x004200A0
void StreamingFileEntry::ProcessQueue() {
    std::string file = this->NextFile();

    while (file.size()) {
        if (Blizzard::Mopaq::SFileFileIsLocal(file.c_str(), this->m_priority) != 1) {
            this->m_next = this->m_cursor;
            return;
        }

        this->AdvanceCursor();
        file = this->NextFile();
    }
}

// VERIFY: d758e44b manual
// OFFSET: 0x00420570
StreamingManager::~StreamingManager() {
    if (this->m_thread && Blizzard::Thread::IsRunning(this->m_thread)) {
        this->Shutdown();
    }

    Blizzard::Lock::EventDestroy(this->m_event);

    for (std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.begin(); it != this->m_entries.end(); ++it) {
        if (it->second) {
            delete it->second;
        }

        it->second = nullptr;
    }

    this->m_entries.clear();
}

// VERIFY: a962109b manual
// OFFSET: 0x00420770
StreamingManager::StreamingManager() {
    this->m_current = nullptr;
    this->m_dirty = true;
    this->m_gameState = 0;
    this->m_enabled = false;
    this->m_zone = -1;
    this->m_mapId = -1;
    this->m_race = -1;
    this->m_subzone = -1;
    this->m_area = -1;
    this->m_areaChunk = -1;
    this->m_state = 0;
    Blizzard::Lock::EventCreate(this->m_event);
    this->m_thread = nullptr;

    this->m_entries.insert(std::make_pair(std::string(), static_cast<StreamingFileEntry*>(nullptr)));

    this->unused_0031 = false;
}

// VERIFY: fefe834e manual
// OFFSET: 0x004208D0
StreamingManager* GetStreamingManagerSingleton() {
    // STATIC: 0x00B31EF0
    static StreamingManager s_manager;

    return &s_manager;
}

// VERIFY: a3524c67 manual
// OFFSET: 0x00420950
void StreamingSetState(int32_t state) {
    if (!CheckIfStreamingAndTrial()) {
        return;
    }

    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_gameState != state) {
        manager->m_gameState = state;
        manager->m_dirty = true;
        Blizzard::Lock::EventSet(manager->m_event);
    }
}

// VERIFY: d36f57c2 manual
// OFFSET: 0x00420980
void StreamingSetRace(int32_t raceID) {
    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_race != raceID) {
        manager->m_race = raceID;
        manager->m_dirty = true;
        Blizzard::Lock::EventSet(manager->m_event);
    }
}

// VERIFY: 7cf14a57 manual
// OFFSET: 0x004209B0
void StreamingSetEnabled(bool enabled) {
    if (!CheckIfStreamingAndTrial()) {
        return;
    }

    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_enabled != enabled) {
        manager->m_enabled = enabled;
        Blizzard::Lock::EventSet(manager->m_event);
    }
}

// VERIFY: 2688b484 auto
// OFFSET: 0x004209E0
float StreamingGetLoadFraction(int32_t raceID) {
    if (!CheckIfStreamingAndTrial()) {
        return 1.0f;
    }

    return GetStreamingManagerSingleton()->GetLoadFraction(raceID);
}

// VERIFY: d971e27d manual
// OFFSET: 0x00420A10
void Blizzard::Streaming::SetZone(int32_t areaId, int32_t subzoneId) {
    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_zone != areaId) {
        manager->m_zone = areaId;
        manager->m_dirty = true;
        Blizzard::Lock::EventSet(manager->m_event);
    }

    manager->m_subzone = subzoneId;
}

// VERIFY: b4d2aafc manual
// OFFSET: 0x00420A50
void StreamingSetAreaOfInterest(int32_t chunkY, int32_t chunkX) {
    StreamingManager* manager = GetStreamingManagerSingleton();

    int32_t areaChunk = ((chunkY + 15) >> 4) + (((chunkX + 15) >> 4) << 6);

    if (manager->m_areaChunk != areaChunk) {
        manager->m_areaChunk = areaChunk;
        manager->m_dirty = true;
        Blizzard::Lock::EventSet(manager->m_event);
    }

    manager->m_area = chunkY + (chunkX << 10);
}

// VERIFY: a90bb0db manual
// OFFSET: 0x00420AA0
void StreamingSetMapId(int32_t mapId) {
    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_mapId != mapId) {
        manager->m_mapId = mapId;
        manager->m_dirty = true;
        Blizzard::Lock::EventSet(manager->m_event);
    }
}

// VERIFY: db85f2d0 manual
// OFFSET: 0x00420B80
void StreamingFileEntry::ParseListfile(const char* text, const char* extension) {
    while (text && *text) {
        const char* end = Blizzard::String::Find(text, s_lineSeparator, 0x7FFFFFFF);

        if (!end) {
            break;
        }

        int32_t length = end - text;
        char line[260];
        Blizzard::String::Copy(line, text, length + 1);
        line[length] = '\0';
        text = end + Blizzard::String::Length(s_lineSeparator);

        if (extension && !Blizzard::String::EqualI(extension, Blizzard::String::FindExtension(line))) {
            continue;
        }

        this->m_files.push_back(std::string(line));
    }
}

// VERIFY: b170d288 manual
// OFFSET: 0x00420CB0
bool StreamingFileEntry::LoadListfile(const char* archiveName, const char* name, const char* extension) {
    this->m_name = name;

    System_SFile::ARCHIVEREC* archive = nullptr;

    if (archiveName) {
        name = "(listfile)";

        if (OpenDataArchive(archiveName, &archive)) {
            this->m_archive = archive;
        }

        if (!archive) {
            return false;
        }
    }

    void* data;
    uint32_t size;

    if (!Blizzard::Mopaq::SFileLoadFile(archive, name, &data, &size, 1, 1)) {
        return false;
    }

    this->ParseListfile(static_cast<const char*>(data), extension);
    Blizzard::Mopaq::SFileUnloadFile(data);

    return true;
}

// VERIFY: d3ba46c9 manual
// OFFSET: 0x00420D50
StreamingFileEntry* StreamingManager::CreateEntry(int32_t type, const std::string& key) {
    StreamingFileEntry* entry = nullptr;

    switch (type) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            entry = new StreamingFileEntry();
            entry->LoadListfile(nullptr, key.c_str(), nullptr);
            break;

        case 9:
        case 10:
            entry = new StreamingFileEntry();
            entry->LoadListfile(key.c_str(), key.c_str(), nullptr);
            break;

        default:
            return nullptr;
    }

    if (entry) {
        this->m_entries.insert(std::make_pair(key, entry));
    }

    return entry;
}

// VERIFY: cb64b588 manual
// OFFSET: 0x00420F00
void StreamingManager::SelectEntry(StreamingFileEntry* previous) {
    int32_t race = this->m_race;
    uint32_t index = 0;
    StreamingFileEntry* entry = previous;

    for (;;) {
        if (entry) {
            this->AdvanceState(index);
        }

        do {
            std::string key = BuildManifestKey(this->m_state, race, this->m_zone, index);
            std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.find(key);

            if (it == this->m_entries.end()) {
                entry = this->CreateEntry(this->m_state, key);
            } else {
                entry = it->second;

                if (!entry) {
                    goto select;
                }
            }

            if (this->m_state >= 6) {
                index++;
            }
        } while (!entry);

        if (!entry->m_complete) {
            break;
        }
    }

select:
    if (entry == this->m_current) {
        return;
    }

    if (entry) {
        int32_t priority;

        if (this->m_state < 0) {
            priority = 6;
        } else if (this->m_state <= 3) {
            priority = 3;
        } else if (this->m_state == 4) {
            priority = 4;
        } else {
            priority = 6;
        }

        entry->m_priority = priority;
    }

    if (this->m_current) {
        this->m_current->m_next = this->m_current->m_cursor;
        Blizzard::Streaming::CancelQueue(this->m_current->m_priority);
    }

    this->m_current = entry;
}

// VERIFY: 43e92da7 manual
// OFFSET: 0x00421100
uint32_t StreamingManager::Run() {
    Blizzard::Streaming::Completion completion;
    completion.m_event = &this->m_event;

    bool cancelled = false;

    do {
        if (this->m_dirty) {
            Blizzard::Lock::EventWait(0, this->m_event);
            this->m_dirty = false;
            this->m_state = this->GetNextState();

            if (this->m_current && (cancelled || this->m_state == 12)) {
                Blizzard::Streaming::RemoveCompletion(&completion, this->m_current->m_priority);
                Blizzard::Lock::EventWait(0, this->m_event);
            }

            if (this->m_state == 12) {
                break;
            }

            this->SelectEntry(nullptr);
        }

        if (!this->m_current) {
            Blizzard::Lock::EventWaitForever(this->m_event);
            continue;
        }

        completion.m_result = -1;
        completion.m_done = false;
        int32_t queued = 0;

        while (!this->m_dirty) {
            std::string file = this->m_current->NextFile();

            if (!file.size()) {
                break;
            }

            if (!Blizzard::Mopaq::SFileFileIsLocal(file.c_str(), this->m_current->m_priority)) {
                Blizzard::Mopaq::SFileMakeFileLocal(file.c_str(), this->m_current->m_priority, false);
                queued++;
            }

            if (queued >= 20) {
                break;
            }
        }

        if (queued > 0) {
            if (this->m_dirty) {
                continue;
            }

            if (Blizzard::Streaming::QueueCompletion(&completion, this->m_current->m_priority)) {
                Blizzard::Lock::EventWaitForever(this->m_event);

                if (this->m_dirty) {
                    cancelled = true;
                    continue;
                }
            }
        }

        if (!this->m_dirty) {
            if ((completion.m_result >= 0 && !completion.m_done) || !queued) {
                this->m_current->AdvanceCursor();

                if (this->m_current->m_complete) {
                    this->SelectEntry(this->m_current);
                }
            }
        }
    } while (this->m_state != 12);

    this->m_current = nullptr;
    return 0;
}

// VERIFY: 3e2a32db manual
// OFFSET: 0x004213A0
bool StreamingManager::IsBaseDataReady() {
    std::string key = BuildManifestKey(2, 0, 0, 0);
    std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.find(key);

    if (it == this->m_entries.end()) {
        this->CreateEntry(2, key);
        it = this->m_entries.find(key);
        it->second->ProcessQueue();
    }

    return it->second->m_complete;
}

// VERIFY: 18927a4c manual
// OFFSET: 0x00421490
bool StreamingManager::IsRaceDataReady(int32_t race) {
    if (!this->IsBaseDataReady()) {
        return false;
    }

    std::string key = BuildManifestKey(3, race, 0, 0);
    std::map<std::string, StreamingFileEntry*>::iterator it = this->m_entries.find(key);

    if (it == this->m_entries.end()) {
        this->CreateEntry(3, key);
        it = this->m_entries.find(key);
        it->second->ProcessQueue();
    }

    return it->second->m_complete;
}

// VERIFY: e335d9d8 auto
// OFFSET: 0x00421590
uint32_t StreamingThreadProc(void* param) {
    return static_cast<StreamingManager*>(param)->Run();
}

// VERIFY: fc1cd2c9 auto
// OFFSET: 0x004215A0
bool StreamingIsRaceDataReady(int32_t raceID) {
    if (!CheckIfStreamingAndTrial()) {
        return true;
    }

    return GetStreamingManagerSingleton()->IsRaceDataReady(raceID);
}

// VERIFY: a9af6e8e manual
// OFFSET: 0x004215D0
int32_t StreamingGetRace() {
    return GetStreamingManagerSingleton()->m_race;
}

// VERIFY: e6668dc4 manual
// OFFSET: 0x00421630
void StartDataPreloadThread() {
    if (!CheckIfStreamingAndTrial()) {
        return;
    }

    StreamingManager* manager = GetStreamingManagerSingleton();

    if (manager->m_thread && Blizzard::Thread::IsRunning(manager->m_thread)) {
        return;
    }

    Blizzard::Thread::Create(StreamingThreadProc, manager, &manager->m_thread, "DataPreload", 0, 0);
}
