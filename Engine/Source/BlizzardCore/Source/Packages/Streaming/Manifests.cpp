#include "Manifests.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"
#include "Engine/Source/BlizzardCore/Source/System/Memory/MemoryStorm.h"
#include "Engine/Source/BlizzardCore/Source/System/String/Path.h"
#include "Engine/Source/BlizzardCore/Source/System/String/String.h"
#include "Engine/Source/BlizzardCore/Source/System/Time/Time.h"

// STATIC: 0x00AB90A4
const char* Blizzard::Streaming::s_httpPrefix = "http://";

// STATIC: 0x00AB90A8
const char* Blizzard::Streaming::s_filePrefix = "file://";

// STATIC: 0x00B38170
Blizzard::Streaming::ManifestInfo* Blizzard::Streaming::ManifestInfo::s_instance;

// STATIC: 0x00B38660
static Blizzard::Streaming::Manifests::ServerItem s_serverItem;

// VERIFY: 87b3943b auto
// OFFSET: 0x004268C0
bool Blizzard::Streaming::Manifests::ManifestItem::HandleTag(const char* tag, const char* value) {
    return false;
}

// VERIFY: 1e529c00 manual
// OFFSET: 0x00427D60
bool Blizzard::Streaming::SplitLine(char* line, char** value, char** tag, int32_t* length) {
    if (!*line) {
        return false;
    }

    *tag = nullptr;
    *value = line;

    char* cursor = line;

    while (*cursor) {
        char c = *cursor;

        if (c == '=') {
            *cursor = '\0';
            *tag = line;
            *value = cursor + 1;
        } else if (c == '\r' || c == '\n') {
            while (*cursor == '\r' || *cursor == '\n') {
                *cursor++ = '\0';
            }

            *length = cursor - line;
            return true;
        }

        cursor++;
    }

    *length = cursor - line;
    return true;
}

// VERIFY: 81de1fcb manual
// OFFSET: 0x00427EC0
bool STORMAPI Blizzard::Streaming::ManifestInfo::GetItemTag(char* line, char** value, char** tag, int32_t* length) {
    if (!*line) {
        return false;
    }

    *tag = nullptr;
    *value = line;

    char* cursor = line;

    while (*cursor) {
        char c = *cursor;

        if (c == '=') {
            *cursor = '\0';
            *tag = line;
            *value = cursor + 1;
        } else if (c == '\r' || c == '\n') {
            while (*cursor == '\r' || *cursor == '\n') {
                *cursor++ = '\0';
            }

            *length = cursor - line;
            return true;
        }

        cursor++;
    }

    *length = cursor - line;
    return false;
}

// VERIFY: 94f35d35 manual
// OFFSET: 0x0042D6B0
bool Blizzard::Streaming::Manifests::TransportItem::HandleTag(const char* tag, const char* value) {
    if (!tag) {
        return false;
    }

    if (!_stricmp(tag, "md5size")) {
        this->m_md5Size = atol(value);
        return true;
    }

    if (_stricmp(tag, "splitsize")) {
        return false;
    }

    this->m_splitSize = atol(value);
    return true;
}

// VERIFY: abc1338a auto
// OFFSET: 0x0042D730
Blizzard::Time::Timestamp Blizzard::Streaming::Manifests::TimeslotItem::ParseTimestamp(const char* time) {
    Blizzard::Time::TimeRec rec;
    memset(&rec.second, 0, 16);
    rec.day = 1;
    rec.month = 1;
    rec.year = 2000;

    int32_t hour;
    int32_t minute;
    sscanf(time, "%d:%d", &hour, &minute);

    rec.hour = hour;
    rec.minute = minute;

    return Blizzard::Time::MakeTime(rec);
}

// VERIFY: f94ee91f auto
// OFFSET: 0x0042D7A0
char* STORMAPI Blizzard::Streaming::TrimLine(char* line) {
    char* start = line;

    if (!*start) {
        return start;
    }

    uint32_t length = strlen(line);

    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        start++;
        length--;

        if (!*start) {
            break;
        }
    }

    for (char* end = &start[length]; *end; *end-- = '\0') {
        char c = *end;

        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
    }

    return start;
}

// VERIFY: 3ee26353 manual
// OFFSET: 0x00432720
bool Blizzard::Streaming::Manifests::TimeslotItem::HandleTag(const char* tag, const char* value) {
    if (!tag) {
        return false;
    }

    if (!_stricmp(tag, "starttime")) {
        this->m_startTime = ParseTimestamp(value);
        return true;
    }

    if (!_stricmp(tag, "endtime")) {
        this->m_endTime = ParseTimestamp(value);
        return true;
    }

    if (_stricmp(tag, "usage")) {
        return false;
    }

    this->m_usage = atol(value);
    return true;
}

// VERIFY: 573566f6 manual
// OFFSET: 0x00437A80
Blizzard::Streaming::Manifests::ManifestItem::~ManifestItem() {
}

// VERIFY: af315f8d manual
// OFFSET: [0x00437D20, 0x00437DA0]
Blizzard::Streaming::Manifests::ParameterItem::~ParameterItem() {
}

// VERIFY: 2f850873 manual
// OFFSET: 0x00437D70
Blizzard::Streaming::Manifests::ParameterItem::ParameterItem() {
}

// VERIFY: 42880ac2 manual
// OFFSET: 0x00437DD0
int32_t Blizzard::Streaming::Manifests::ServerItem::GetUsage(Blizzard::Time::Timestamp time) {
    int32_t usage = 0;

    for (std::vector<UsageSlot>::iterator it = this->m_usageSlots.begin(); it != this->m_usageSlots.end(); ++it) {
        if (it->m_time > time || !this->m_active) {
            break;
        }

        usage = it->m_usage;
    }

    return usage;
}

// VERIFY: 07f11350 manual
// OFFSET: 0x0043ACA0
bool Blizzard::Streaming::FileManifest::SplitItem(char* line, MANIFEST_ITEM* item) {
    char* field = line;

    if (!*line) {
        return false;
    }

    uint32_t index = 0;
    char* cursor = field;

    do {
        if (*cursor == ';') {
            do {
                *cursor++ = '\0';
            } while (*cursor == ';');

            switch (index) {
                case 0: {
                    char absolute[260];
                    Blizzard::File::MakeAbsolutePath(field, absolute, 260, false);

                    char normalized[260];
                    char* dst = normalized;
                    const char* src = absolute;
                    uint32_t left = 260;

                    while (true) {
                        char c = *src++;
                        left--;

                        if (!c) {
                            *dst = '\0';
                            break;
                        }

                        *dst++ = c == '/' ? '\\' : c;

                        if (!left) {
                            dst[-1] = '\0';
                            break;
                        }
                    }

                    item->m_absolutePath.assign(normalized, strlen(normalized));
                    item->m_path.assign(field, strlen(field));
                    break;
                }

                case 1:
                    item->m_size = std::strtoll(field, nullptr, 10);
                    break;

                case 2:
                    item->m_fileVersion.assign(field, strlen(field));
                    break;

                case 3:
                    item->m_flags = atol(field);
                    break;
            }

            index++;
            field = cursor;
        }

        cursor++;
    } while (*cursor);

    if (index == 3) {
        item->m_flags = atol(field);
    }

    return true;
}

// VERIFY: afa848ba manual
// OFFSET: 0x0043AE10
bool Blizzard::Streaming::SourceManifest::SplitItem(char* line, MANIFEST_ITEM* item) {
    char* field = line;

    if (!*line) {
        return false;
    }

    uint32_t index = 0;
    char* cursor = field;

    do {
        if (*cursor == ';') {
            do {
                *cursor++ = '\0';
            } while (*cursor == ';');

            if (index == 0) {
                item->m_name.assign(field, strlen(field));
            } else if (index == 1) {
                item->m_path.assign(field, strlen(field));
            }

            index++;
            field = cursor;
        }

        cursor++;
    } while (*cursor);

    if (index == 1) {
        item->m_path.assign(field, strlen(field));
    }

    return true;
}

// VERIFY: d2867546 manual
// OFFSET: 0x0043DBF0
bool Blizzard::Streaming::Manifests::ServerPathItem::HandleTag(const char* tag, const char* value) {
    if (!tag || _stricmp(tag, "path")) {
        return false;
    }

    this->m_path = std::string(value);
    return true;
}

// VERIFY: d5f9f625 manual
// OFFSET: 0x0043DCC0
bool Blizzard::Streaming::Manifests::ParameterItem::HandleTag(const char* tag, const char* value) {
    if (!tag || _stricmp(tag, "value")) {
        return false;
    }

    this->m_value = std::string(value);
    return true;
}

// VERIFY: 0df0b1e2 auto
// OFFSET: 0x0043DD90
bool Blizzard::Streaming::FileManifest::SplitServerItem(char* line, std::string& base, std::string& path) {
    if (!*line) {
        return false;
    }

    uint32_t prefixLength;

    if (s_httpPrefix && !_strnicmp(s_httpPrefix, line, strlen(s_httpPrefix))) {
        prefixLength = s_httpPrefix ? strlen(s_httpPrefix) : 0;
    } else if (s_filePrefix && !_strnicmp(s_filePrefix, line, strlen(s_filePrefix))) {
        prefixLength = s_filePrefix ? strlen(s_filePrefix) : 0;
    } else {
        return false;
    }

    char* cursor = &line[prefixLength];

    if (!*cursor) {
        return false;
    }

    while (*cursor != '/') {
        if (!*++cursor) {
            return false;
        }
    }

    for (; *cursor == '/'; cursor++) {
        *cursor = '\0';
    }

    base.assign(line, strlen(line));
    base.append("/", 1);
    path.assign(cursor, strlen(cursor));

    return true;
}

// VERIFY: 737c5b41 manual
// OFFSET: 0x0043D6E0
std::string Blizzard::Streaming::Manifests::FileItem::GetRemotePath() {
    return this->m_path + this->m_fileName;
}

// VERIFY: b28e420e manual
// OFFSET: [0x00444000, 0x00447580]
Blizzard::Streaming::Manifests::ServerItem::~ServerItem() {
}

// VERIFY: 07f9cb91 manual
// OFFSET: 0x004440A0
Blizzard::Streaming::Manifests::FileItem::FileItem() {
}

// VERIFY: 6b2adeb3 manual
// OFFSET: [0x00444130, 0x00444250]
Blizzard::Streaming::Manifests::FileItem::~FileItem() {
}

// VERIFY: 50d349cb manual
// OFFSET: 0x004474D0
Blizzard::Streaming::Manifests::ServerItem::ServerItem() {
    UsageSlot slot;
    slot.m_time = TimeslotItem::ParseTimestamp("0:00");
    slot.m_usage = 100;
    this->m_usageSlots.push_back(slot);

    this->m_uses = 0;
    this->m_canDeactivate = 0;
    this->m_maxRetry = 5;
    this->m_active = true;
}

// VERIFY: ae616726 manual
// OFFSET: 0x004475B0
void Blizzard::Streaming::Manifests::ServerItem::InsertUsageSlot(UsageSlot& slot, Blizzard::Time::Timestamp end) {
    UsageSlot* endSlot = nullptr;
    std::vector<UsageSlot>::iterator it = this->m_usageSlots.begin();

    while (it != this->m_usageSlots.end()) {
        if (endSlot) {
            if (it->m_time > endSlot->m_time) {
                break;
            }

            if (&*it == endSlot) {
                ++it;
            } else {
                endSlot->m_usage = it->m_usage;
                it = this->m_usageSlots.erase(it);
            }
        } else {
            if (slot.m_time > it->m_time) {
                ++it;
                continue;
            }

            it = this->m_usageSlots.insert(it, slot);
            ++it;
            endSlot = &*it;
            it->m_time = end;
        }
    }

    if (!endSlot) {
        this->m_usageSlots.push_back(slot);

        if (end) {
            UsageSlot endUsage;
            endUsage.m_time = end;
            endUsage.m_usage = 100;
            this->m_usageSlots.push_back(endUsage);
        }
    }
}

// VERIFY: 2407f93f manual
// OFFSET: 0x00447750
void Blizzard::Streaming::Manifests::ServerItem::InsertTimeslot(TimeslotItem* timeslot) {
    UsageSlot slot;

    if (timeslot->m_startTime == timeslot->m_endTime) {
        this->m_usageSlots.clear();
        slot.m_time = TimeslotItem::ParseTimestamp("0:00");
        slot.m_usage = timeslot->m_usage;
        this->InsertUsageSlot(slot, 0);
    } else if (timeslot->m_startTime > timeslot->m_endTime) {
        slot.m_time = TimeslotItem::ParseTimestamp("0:00");
        slot.m_usage = timeslot->m_usage;
        this->InsertUsageSlot(slot, timeslot->m_endTime);
        slot.m_time = timeslot->m_startTime;
        this->InsertUsageSlot(slot, 0);
    } else {
        slot.m_time = timeslot->m_startTime;
        slot.m_usage = timeslot->m_usage;
        this->InsertUsageSlot(slot, timeslot->m_endTime);
    }
}

// VERIFY: 836a47f6 manual
// OFFSET: 0x00447810
bool Blizzard::Streaming::ManifestInfo::FinalizeManifestItems() {
    bool result = true;

    for (std::map<std::string, Manifests::FileItem>::iterator it = this->m_files.begin(); it != this->m_files.end(); ++it) {
        Manifests::FileItem& file = it->second;

        if (!file.m_size) {
            Blizzard::File::CreateDirectory(it->first.c_str(), false);
            continue;
        }

        if (file.m_path.size()) {
            std::map<std::string, Manifests::ServerPathItem>::iterator path = this->m_serverPaths.find(file.m_path);

            if (path == this->m_serverPaths.end()) {
                result = false;
            } else {
                file.m_path = path->second.m_path;
            }
        }

        if (!file.m_transportItem.size()) {
            file.m_transport = nullptr;
            continue;
        }

        std::map<std::string, Manifests::TransportItem>::iterator transport = this->m_transports.find(file.m_transportItem);

        if (transport == this->m_transports.end()) {
            if (this->m_version <= 1) {
                file.m_transport = nullptr;
            } else {
                result = false;
            }
        } else {
            file.m_transport = &transport->second;
        }
    }

    for (std::map<std::string, Manifests::ServerItem>::iterator it = this->m_servers.begin(); it != this->m_servers.end(); ++it) {
        for (std::vector<std::string>::iterator id = it->second.m_timeslotIds.begin(); id != it->second.m_timeslotIds.end(); ++id) {
            std::map<std::string, Manifests::TimeslotItem>::iterator timeslot = this->m_timeslots.find(*id);

            if (timeslot == this->m_timeslots.end()) {
                result = false;
            } else {
                it->second.InsertTimeslot(&timeslot->second);
            }
        }
    }

    return result;
}

// VERIFY: c742dab3 manual
// OFFSET: 0x0044C630
bool Blizzard::Streaming::Transport::SplitItem(char* line, TRANSPORT_ITEM* item) {
    char* field = line;

    if (!*line) {
        return false;
    }

    uint32_t index = 0;
    item->m_md5Size = 0;
    item->m_splitSize = 0;

    char* cursor = field;

    if (!*cursor) {
        return true;
    }

    do {
        if (*cursor == ';') {
            do {
                *cursor++ = '\0';
            } while (*cursor == ';');

            switch (index) {
                case 0:
                    item->m_name.assign(field, strlen(field));
                    index = 1;
                    break;

                case 1:
                    item->m_md5Size = atol(field);
                    index = 2;
                    break;

                case 2:
                    item->m_splitSize = atol(field);
                    index = 3;
                    break;

                case 3:
                    index = 4;
                    break;

                case 4:
                    item->m_servers.push_back(std::string(field));
                    break;
            }

            field = cursor;
        }

        cursor++;
    } while (*cursor);

    if (index == 4) {
        item->m_servers.push_back(std::string(field));
    }

    return true;
}

// VERIFY: d05cb762 manual
// OFFSET: 0x0044C790
bool Blizzard::Streaming::Manifests::FileItem::HandleTag(const char* tag, const char* value) {
    if (tag) {
        if (!_stricmp(tag, "name")) {
            this->m_fileName = std::string(value);
            return true;
        }

        if (!_stricmp(tag, "size")) {
            this->m_size = std::strtoll(value, nullptr, 10);
            return true;
        }

        if (!_stricmp(tag, "fileversion")) {
            this->m_fileVersion = std::string(value);
            return true;
        }

        if (!_stricmp(tag, "flags")) {
            this->m_flags = atol(value);
            return true;
        }
    }

    if (Blizzard::String::EqualI(tag, "path")) {
        this->m_path = std::string(value);
        return true;
    }

    if (Blizzard::String::EqualI(tag, "transportitem")) {
        this->m_transportItem = std::string(value);
        return true;
    }

    if (Blizzard::String::EqualI(tag, "validserver")) {
        this->m_validServers.push_back(std::string(value));
        return true;
    }

    return false;
}

// VERIFY: e76cbe2c manual
// OFFSET: 0x0044C9F0
bool Blizzard::Streaming::Manifests::ServerItem::HandleTag(const char* tag, const char* value) {
    if (tag) {
        if (!_stricmp(tag, "location")) {
            this->m_location = std::string(value);
            return true;
        }

        if (!_stricmp(tag, "timeslotid")) {
            this->m_timeslotIds.push_back(std::string(value));
            return true;
        }

        if (!_stricmp(tag, "maxretry")) {
            this->m_maxRetry = atol(value);
            return true;
        }

        if (!_stricmp(tag, "candeactivate")) {
            this->m_canDeactivate = atol(value);
            return true;
        }
    }

    return false;
}

// VERIFY: 0d42f349 manual
// OFFSET: 0x004500B0
Blizzard::Streaming::Manifests::ManifestItem* Blizzard::Streaming::ManifestInfo::HandleBaseTag(const char* tag, const char* value) {
    if (tag) {
        if (!_stricmp(tag, "file")) {
            Manifests::FileItem item;
            std::string name(value);
            std::string key = ResolveFilePath(name);

            this->m_files[key] = item;
            this->m_files[key].m_name = name;
            this->m_files[key].m_localPath = std::string(key);

            return &this->m_files[key];
        }

        if (!_stricmp(tag, "server")) {
            Manifests::ServerItem item;
            std::string key(value);

            this->m_servers[key] = item;
            this->m_servers[key].m_name = key;

            return &this->m_servers[key];
        }

        if (!_stricmp(tag, "serverpath")) {
            Manifests::ServerPathItem item;
            std::string key(value);

            this->m_serverPaths[key] = item;

            return &this->m_serverPaths[key];
        }

        if (!_stricmp(tag, "timeslot")) {
            Manifests::TimeslotItem item;
            std::string key(value);

            this->m_timeslots[key] = item;

            return &this->m_timeslots[key];
        }
    }

    if (Blizzard::String::EqualI(tag, "transport")) {
        Manifests::TransportItem item;
        std::string key(value);

        this->m_transports[key] = item;

        return &this->m_transports[key];
    }

    if (Blizzard::String::EqualI(tag, "parameter")) {
        Manifests::ParameterItem item;
        std::string key(value);

        this->m_parameters[key] = item;

        return &this->m_parameters[key];
    }

    return nullptr;
}

// VERIFY: bab63d86 manual
// OFFSET: 0x00450580
std::string Blizzard::Streaming::ManifestInfo::GetRandomServerLocation(const std::vector<std::string>& validServers) {
    std::string names[5];
    int32_t weights[5];
    int32_t count = 0;
    int32_t total = 0;

    Blizzard::Time::TimeRec rec;
    Blizzard::Time::BreakTime(Blizzard::Time::GetTimestamp(), rec);
    rec.year = 2000;
    rec.month = 1;
    rec.day = 1;
    rec.nanosecond = 0;
    rec.second = 0;
    Blizzard::Time::Timestamp now = Blizzard::Time::MakeTime(rec);

    if (validServers.size()) {
        for (std::vector<std::string>::const_iterator it = validServers.begin(); it != validServers.end(); ++it) {
            std::map<std::string, Manifests::ServerItem>::iterator server = this->m_servers.find(*it);

            if (server == this->m_servers.end()) {
                continue;
            }

            int32_t usage = server->second.GetUsage(now);

            if (usage > 0) {
                total += usage;
                names[count] = server->first;
                weights[count++] = total;
            }
        }
    } else {
        for (std::map<std::string, Manifests::ServerItem>::iterator server = this->m_servers.begin(); server != this->m_servers.end(); ++server) {
            int32_t usage = server->second.GetUsage(now);

            if (usage > 0) {
                total += usage;
                names[count] = server->first;
                weights[count++] = total;
            }
        }
    }

    if (total) {
        int32_t roll = rand() % total;

        for (int32_t i = 0; i < count; i++) {
            if (weights[i] > roll) {
                Manifests::ServerItem& server = this->m_servers[names[i]];
                server.m_uses++;
                return server.m_location;
            }
        }
    }

    return std::string("", 0);
}

// VERIFY: cf7fb8cc manual
// OFFSET: 0x00451EE0
Blizzard::Streaming::ManifestInfo::ManifestInfo() {
    this->m_version = 0;
    srand(static_cast<uint32_t>(time(nullptr)));
}

// VERIFY: fb6c09d1 manual
// OFFSET: 0x00452040
Blizzard::Streaming::ManifestInfo::~ManifestInfo() {
}

// VERIFY: 3a1f4459 manual
// OFFSET: 0x004525A0
Blizzard::Streaming::ManifestInfo* Blizzard::Streaming::ManifestInfo::Get() {
    if (!s_instance) {
        s_instance = new ManifestInfo();
    }

    return s_instance;
}

// VERIFY: a7aefdb8 manual
// OFFSET: 0x00452610
std::string Blizzard::Streaming::ManifestInfo::GetParameter(std::string name) {
    std::map<std::string, Manifests::ParameterItem>::iterator it = Get()->m_parameters.find(name);

    if (it == Get()->m_parameters.end()) {
        return std::string();
    }

    return std::string(it->second.m_value);
}

// VERIFY: 6be6b41c manual
// OFFSET: 0x004526F0
void Blizzard::Streaming::ManifestInfo::SetParameter(std::string name, std::string value) {
    std::string parameterName = name;
    std::string parameterValue = value;

    Manifests::ParameterItem& parameter = Get()->m_parameters[name];
    parameter.m_name = parameterName;
    parameter.m_value = parameterValue;
}

// VERIFY: 2d8ac238 manual
// OFFSET: 0x00452810
void Blizzard::Streaming::ManifestInfo::SetParameter(std::string name, int32_t value) {
    char text[16];
    Blizzard::String::Format(text, sizeof(text), "%d", value);

    std::string parameterName = name;
    std::string parameterValue = std::string(text);

    Manifests::ParameterItem& parameter = Get()->m_parameters[name];
    parameter.m_name = parameterName;
    parameter.m_value = parameterValue;
}

// VERIFY: ba9688a6 manual
// OFFSET: 0x00452970
void Blizzard::Streaming::ManifestInfo::SetServerPathItem(std::string name, std::string path) {
    std::string serverPathName = name;
    std::string serverPathPath = path;

    Manifests::ServerPathItem& serverPath = Get()->m_serverPaths[name];
    serverPath.m_name = serverPathName;
    serverPath.m_path = serverPathPath;
}

// VERIFY: 93bd2e83 manual
// OFFSET: 0x00452A90
void Blizzard::Streaming::ManifestInfo::SetTransportItem(std::string name, const Manifests::TransportItem& item) {
    Get()->m_transports[name] = item;
}

// VERIFY: d6041b06 manual
// OFFSET: 0x00452B10
void Blizzard::Streaming::ManifestInfo::SetFileItem(std::string name, const Manifests::FileItem& item) {
    Get()->m_files[name] = item;

    if (Get()->m_transports.find(item.m_name) != Get()->m_transports.end()) {
        Get()->m_files[name].m_transportItem = item.m_name;
    }

    if (Get()->m_serverPaths.find(item.m_name) != Get()->m_serverPaths.end()) {
        Get()->m_files[name].m_path = item.m_name;
    }
}

// VERIFY: dc677d5a manual
// OFFSET: 0x00452C30
void Blizzard::Streaming::ManifestInfo::SetServerItem(std::string name, const Manifests::ServerItem& item) {
    Get()->m_servers[name] = item;
}

// VERIFY: 6485535f manual
// OFFSET: 0x00452C90
Blizzard::Streaming::Manifests::FileItem* Blizzard::Streaming::ManifestInfo::GetFileItem(const std::string& name) {
    std::map<std::string, Manifests::FileItem>::iterator it = Get()->m_files.find(name);

    if (it == Get()->m_files.end()) {
        return nullptr;
    }

    return &it->second;
}

// VERIFY: dee498ff manual
// OFFSET: 0x00452D00
Blizzard::Streaming::Manifests::TransportItem* Blizzard::Streaming::ManifestInfo::GetTransportItem(const std::string& name) {
    std::map<std::string, Manifests::FileItem>::iterator it = Get()->m_files.find(name);

    if (it == Get()->m_files.end()) {
        return nullptr;
    }

    return it->second.m_transport;
}

// VERIFY: c8b57a76 manual
// OFFSET: 0x00452D70
std::string Blizzard::Streaming::ManifestInfo::GetRemotePath(const std::string& name) {
    Manifests::FileItem* file = GetFileItem(name);

    if (!file) {
        return std::string();
    }

    std::string location = Get()->GetRandomServerLocation(file->m_validServers);
    return location + file->GetRemotePath();
}

// VERIFY: 1ffd4246 manual
// OFFSET: 0x00452E50
std::vector<std::string> Blizzard::Streaming::ManifestInfo::GetFileList(const char* path) {
    std::vector<std::string> files;

    char directory[260];
    Blizzard::File::MakeAbsolutePath(path, directory, sizeof(directory), false);

    char separator = Blizzard::String::FirstPathSeparator(directory);
    int32_t length = strlen(directory);
    char last = length > 0 ? directory[length - 1] : '\0';

    if (!length || last != separator) {
        if (last == '/' || last == '\\') {
            length--;
        }

        if (length > 258) {
            length = 258;
        }

        directory[length] = separator;
        directory[length + 1] = '\0';
    }

    char normalized[260];
    char* dst = normalized;
    const char* src = directory;
    uint32_t left = 260;

    while (true) {
        char c = *src++;
        left--;

        if (!c) {
            break;
        }

        *dst++ = c == '/' ? '\\' : c;

        if (!left) {
            dst--;
            break;
        }
    }

    *dst = '\0';

    for (std::map<std::string, Manifests::FileItem>::iterator it = Get()->m_files.begin(); it != Get()->m_files.end(); ++it) {
        if (it->second.m_size <= 0) {
            continue;
        }

        const char* localPath = it->second.m_localPath.c_str();
        char localDirectory[260];
        char fileName[260];

        if (localPath) {
            const char* name = localPath;

            for (const char* cursor = localPath; *cursor; cursor++) {
                if (*cursor == '/' || *cursor == '\\') {
                    name = cursor + 1;
                }
            }

            int32_t directoryLength = name - localPath + 1;
            Blizzard::String::Copy(localDirectory, localPath, directoryLength <= 260 ? directoryLength : 260);
            Blizzard::String::Copy(fileName, name, 260);
        } else {
            localDirectory[0] = '\0';
            fileName[0] = '\0';
        }

        if (!_stricmp(normalized, localDirectory)) {
            files.push_back(std::string(fileName));
        }
    }

    return files;
}

// VERIFY: 0c387579 manual
// OFFSET: 0x00453150
void Blizzard::Streaming::ManifestInfo::DeactivateServer(std::string url, const std::string& name) {
    if (!GetFileItem(name)) {
        return;
    }

    for (std::map<std::string, Manifests::ServerItem>::iterator it = Get()->m_servers.begin(); it != Get()->m_servers.end(); ++it) {
        uint32_t length = it->second.m_location.size();
        const char* location = it->second.m_location.c_str();

        if (location && !_strnicmp(url.c_str(), location, length) && it->second.m_canDeactivate >= 1) {
            it->second.m_active = false;
        }
    }
}

// VERIFY: ca248d72 manual
// OFFSET: 0x00453280
int32_t Blizzard::Streaming::ManifestInfo::GetServerItemMaxRetry(std::string url, const std::string& name) {
    if (!GetFileItem(name)) {
        return 5;
    }

    for (std::map<std::string, Manifests::ServerItem>::iterator it = Get()->m_servers.begin(); it != Get()->m_servers.end(); ++it) {
        uint32_t length = it->second.m_location.size();
        const char* location = it->second.m_location.c_str();

        if (location && !_strnicmp(url.c_str(), location, length)) {
            return it->second.m_maxRetry;
        }
    }

    return 5;
}

// VERIFY: 362b874e manual
// OFFSET: 0x004533E0
int32_t Blizzard::Streaming::FileManifest::GetBGPreloadSleep() {
    std::string value = ManifestInfo::GetParameter(std::string("bgpreloadsleep"));

    if (!value.size()) {
        return 100;
    }

    int32_t sleep = atol(value.c_str());

    if (sleep < 0) {
        sleep = 0;
    }

    return sleep;
}

// VERIFY: d8f4bc70 manual
// OFFSET: 0x00453480
bool Blizzard::Streaming::FileManifest::GetIsStreamingTrial() {
    std::string value = ManifestInfo::GetParameter(std::string("istrial"));

    if (!value.size()) {
        return false;
    }

    return atol(value.c_str()) != 0;
}

// VERIFY: c87346ca manual
// OFFSET: 0x00453510
bool Blizzard::Streaming::FileManifest::SetSpeedTest(int32_t speedTest) {
    if (!speedTest) {
        return true;
    }

    int32_t minimum = 10;
    int32_t maximum = 1000;

    ManifestInfo::SetParameter(std::string("bgpreloadsleep"), 25000000 / speedTest);

    int32_t sleep = atol(ManifestInfo::GetParameter(std::string("bgpreloadsleep")).c_str());
    ManifestInfo::SetParameter(std::string("bgpreloadsleep"), sleep >= minimum ? sleep : minimum);

    sleep = atol(ManifestInfo::GetParameter(std::string("bgpreloadsleep")).c_str());
    ManifestInfo::SetParameter(std::string("bgpreloadsleep"), sleep <= maximum ? sleep : maximum);

    return true;
}

// VERIFY: e96edca9 auto
// OFFSET: 0x004536B0
std::vector<std::string> Blizzard::Streaming::FileManifest::GetFileList(const char* path) {
    return ManifestInfo::GetFileList(path);
}

// VERIFY: 26fb72b5 manual
// OFFSET: 0x004536E0
void Blizzard::Streaming::FileManifest::SetDefaultBgPreloadSleep(int32_t sleep) {
    ManifestInfo::SetParameter(std::string("bgpreloadsleep"), sleep);
}

// VERIFY: 18f21e75 manual
// OFFSET: 0x00455D40
bool Blizzard::Streaming::Transport::ReadDirectFileFromBuffer(char* buffer, int32_t size) {
    char* line = buffer;

    if (!*line) {
        return true;
    }

    char* end = buffer + size;

    while (line < end) {
        char* value;
        char* tag;
        int32_t length;

        if (!SplitLine(line, &value, &tag, &length)) {
            Blizzard::File::AddToLastErrorStack(0, "Transport::ReadDirectFile - GetItemTag failed", 20);
            return false;
        }

        line += length;

        if (tag && (tag == "version" || !_strnicmp(tag, "version", 7))) {
            if (atol(value) != 1) {
                return false;
            }
        } else {
            TRANSPORT_ITEM item;
            SplitItem(value, &item);

            Manifests::TransportItem transport;
            transport.m_name = item.m_name;
            transport.m_md5Size = item.m_md5Size;
            transport.m_splitSize = item.m_splitSize;

            ManifestInfo::SetTransportItem(std::string(item.m_name), transport);
        }

        if (!*line) {
            return true;
        }
    }

    return true;
}

// VERIFY: 8cb841ed manual
// OFFSET: 0x0045AD60
bool Blizzard::Streaming::SourceManifest::Initialize(const char* path) {
    if (!path || !*path) {
        return true;
    }

    if (s_httpPrefix && !_strnicmp(path, s_httpPrefix, strlen(s_httpPrefix))) {
        return ReadIndirectFile(path);
    }

    return ReadDirectFile(path);
}

// VERIFY: e18b8494 auto
// OFFSET: 0x00456720
bool Blizzard::Streaming::ManifestInfo::ReadDirectFile(const char* path) {
    void* data;
    uint32_t size;

    if (!System_File::LoadFile(path, &data, &size, 4)) {
        Blizzard::File::AddToLastErrorStack(0, "Manifests::ReadDirectFile - LoadFile failed", 20);
        return false;
    }

    bool result = this->ReadDirectFileFromBuffer(static_cast<char*>(data), size);
    Blizzard::Memory::Free(data);

    if (!result) {
        Blizzard::File::AddToLastErrorStack(0, "Manifests::ReadDirectFile - ReadDirectFileFromBuffer failed", 20);
    }

    return result;
}

// VERIFY: 78da1596 auto
// OFFSET: 0x00456980
bool Blizzard::Streaming::FileManifest::ReadDirectFile(const char* path) {
    void* data;
    uint32_t size;

    if (!System_File::LoadFile(path, &data, &size, 4)) {
        Blizzard::File::AddToLastErrorStack(0, "FileManifest::ReadDirectFile - LoadFile failed", 20);
        return false;
    }

    bool result = ReadDirectFileFromBuffer(static_cast<char*>(data), size);
    Blizzard::Memory::Free(data);

    if (!result) {
        Blizzard::File::AddToLastErrorStack(0, "FileManifest::ReadDirectFile - ReadDirectFileFromBuffer failed", 20);
    }

    return result;
}

// VERIFY: 4ef7dc5c manual
// OFFSET: 0x00456A00
bool Blizzard::Streaming::SourceManifest::ReadDirectFileFromBuffer(char* buffer, int32_t size) {
    std::string base;
    std::string path;

    char* line = buffer;

    if (!*line) {
        return true;
    }

    char* end = buffer + size;

    while (line < end) {
        char* value;
        char* tag;
        int32_t length;

        if (!SplitLine(line, &value, &tag, &length)) {
            Blizzard::Memory::Free(buffer);
            Blizzard::File::AddToLastErrorStack(0, "SourceManifest::ReadDirectfile - GetItemTag failed", 20);
            return false;
        }

        char* next = &line[length];

        if (tag && (tag == "version" || !_strnicmp(tag, "version", 7))) {
            if (atol(value) != 1) {
                return false;
            }
        } else if (tag && (tag == "source" || !_strnicmp(tag, "source", 6))) {
            if (FileManifest::SplitServerItem(value, base, path)) {
                Manifests::ServerItem server;
                server.m_name = base;
                server.m_location = base;
                ManifestInfo::SetServerItem(std::string(base), server);
                ManifestInfo::SetServerPathItem(std::string("old-version1-base"), std::string(path));
            }
        } else {
            MANIFEST_ITEM item;
            SplitItem(value, &item);

            char sourcePath[260];

            if (item.m_path.size() != static_cast<size_t>(-1)) {
                const char* src = item.m_path.c_str();

                if (src) {
                    char* dst = sourcePath;
                    char* limit = &sourcePath[item.m_path.size()];

                    while (dst < limit) {
                        char c = *src++;

                        if (!c) {
                            break;
                        }

                        *dst++ = c;
                    }

                    *dst = '\0';
                } else {
                    sourcePath[0] = '\0';
                }
            }

            if (FileManifest::SplitServerItem(sourcePath, base, path)) {
                ManifestInfo::SetServerPathItem(std::string(item.m_name), std::string(path));
            }
        }

        if (!*next) {
            return true;
        }

        line = next;
    }

    return true;
}

// VERIFY: 4525f827 auto
// OFFSET: 0x00459240
bool Blizzard::Streaming::Transport::ReadDirectFile(const char* path) {
    void* data;
    uint32_t size;

    if (!System_File::LoadFile(path, &data, &size, 4)) {
        Blizzard::File::AddToLastErrorStack(0, "Transport::ReadDirectFile - failed to load file", 20);
        return false;
    }

    bool result = ReadDirectFileFromBuffer(static_cast<char*>(data), size);
    Blizzard::Memory::Free(data);

    return result;
}

// VERIFY: c1052c97 auto
// OFFSET: 0x004595F0
bool Blizzard::Streaming::SourceManifest::ReadDirectFile(const char* path) {
    void* data;
    uint32_t size;

    if (!System_File::LoadFile(path, &data, &size, 4)) {
        Blizzard::File::AddToLastErrorStack(0, "SourceManifest::ReadDirectfile - failed to load file", 20);
        return false;
    }

    bool result = ReadDirectFileFromBuffer(static_cast<char*>(data), size);
    Blizzard::Memory::Free(data);

    return result;
}

// VERIFY: a956c6ba manual
// OFFSET: 0x0045D1E0
bool Blizzard::Streaming::FileManifest::ReadDirectFileFromBuffer(char* buffer, int32_t size) {
    char* line = buffer;

    if (!*line) {
        return true;
    }

    char* end = buffer + size;

    while (line < end) {
        char* value;
        char* tag;
        int32_t length;

        if (!SplitLine(line, &value, &tag, &length)) {
            Blizzard::File::AddToLastErrorStack(0, "FileManifest::ReadDirectFileFromBuffer - GetItemTag failed", 20);
            return false;
        }

        char* next = &line[length];
        bool isItem = !tag;

        if (isItem) {
        } else if (tag == "version" || !_strnicmp(tag, "version", 7)) {
            if (atol(value) != 1) {
                return false;
            }
        } else if (tag == "isTrial" || !_strnicmp(tag, "isTrial", 7)) {
            ManifestInfo::SetParameter(std::string("istrial"), std::string(value));
        } else if (tag == "manifest" || !_strnicmp(tag, "manifest", 8)) {
            uint32_t prefixLength = s_httpPrefix ? strlen(s_httpPrefix) : 0;

            if (value && s_httpPrefix && !_strnicmp(value, s_httpPrefix, prefixLength)) {
                return ReadIndirectFile(value);
            }

            return ReadDirectFile(value);
        } else if (tag == "sourcemanifest" || !_strnicmp(tag, "sourcemanifest", 14)) {
            if (!SourceManifest::Initialize(value)) {
                Blizzard::File::AddToLastErrorStack(0, "FileManifest::ReadDirectFileFromBuffer - SourceManifest::Initialize failed", 20);
                return false;
            }
        } else if (tag == "transportmanifest" || !_strnicmp(tag, "transportmanifest", 17)) {
            if (value && *value) {
                bool result = _strnicmp(value, "http://", 7) ? Transport::ReadDirectFile(value) : Transport::ReadIndirectFile(value);

                if (!result) {
                    Blizzard::File::AddToLastErrorStack(0, "FileManifest::ReadDirectFileFromBuffer - Transport::Initialize failed", 20);
                    return false;
                }
            }
        } else if (tag == "source" || !_strnicmp(tag, "source", 6)) {
            std::string base;
            std::string path;

            if (SplitServerItem(value, base, path)) {
                Manifests::ServerItem server;
                server.m_name = base;
                server.m_location = base;
                ManifestInfo::SetServerItem(std::string(base), server);
                ManifestInfo::SetServerPathItem(std::string("old-version1-base"), std::string(path));
            }
        } else if (tag == "bgpreloadsleep" || !_strnicmp(tag, "bgpreloadsleep", 14)) {
            ManifestInfo::SetParameter(std::string("bgpreloadsleep"), std::string(value));
        } else {
            isItem = true;
        }

        if (isItem) {
            MANIFEST_ITEM item;
            SplitItem(value, &item);

            if (!item.m_size) {
                if (Blizzard::File::Exists(item.m_absolutePath.c_str()) != 2) {
                    Blizzard::File::CreateDirectory(item.m_absolutePath.c_str(), true);
                }
            }

            std::string key = ResolveFilePath(item.m_path);

            Manifests::FileItem file;
            file.m_name = item.m_path;
            file.m_fileName = item.m_path;
            file.m_localPath = key;
            file.m_size = item.m_size;
            file.m_path.assign("old-version1-base", 17);
            file.m_fileVersion = item.m_fileVersion;
            file.m_transportItem = item.m_path;

            ManifestInfo::SetFileItem(std::string(key), file);
        }

        line = next;

        if (!*line) {
            return true;
        }
    }

    return true;
}

// VERIFY: 1d033fdf manual
// OFFSET: 0x00461290
bool Blizzard::Streaming::ManifestInfo::ReadDirectFileFromBuffer(char* buffer, int32_t size) {
    Manifests::ManifestItem* item = nullptr;
    char* line = buffer;

    if (!*line) {
        return true;
    }

    char* end = &buffer[size];

    while (line < end) {
        char* value;
        char* tag;
        int32_t length;

        if (!GetItemTag(line, &value, &tag, &length)) {
            Blizzard::File::AddToLastErrorStack(0, "ManifestInfo::ReadDirectFileFromBuffer - GetItemTag failed", 20);
            return false;
        }

        char* next = &line[length];
        value = TrimLine(value);
        tag = TrimLine(tag);

        if (tag && !_stricmp(tag, "version")) {
            this->m_version = atol(value);

            if (this->m_version == 1) {
                return FileManifest::ReadDirectFileFromBuffer(next, size + next - buffer);
            }
        } else if (tag && !_stricmp(tag, "manifest")) {
            if (value && s_httpPrefix && !_strnicmp(value, s_httpPrefix, Blizzard::String::Length(s_httpPrefix))) {
                this->ReadIndirectFile(value);
            } else {
                this->ReadDirectFile(value);
            }
        } else if (!item || !item->HandleTag(tag, value)) {
            item = this->HandleBaseTag(tag, value);

            if (!item) {
                Blizzard::File::AddToLastErrorStack(0, "ManifestInfo::ReadDirectFileFromBuffer - Tag Parsing failed", 20);
                return false;
            }
        }

        if (!*next) {
            return true;
        }

        line = next;
    }

    return true;
}

// VERIFY: 85a3a88a manual
// OFFSET: 0x00461620
bool Blizzard::Streaming::ManifestInfo::Initialize(const char* path) {
    ManifestInfo* info = Get();

    if (!path) {
        return true;
    }

    bool result;

    if (s_httpPrefix && !_strnicmp(path, s_httpPrefix, strlen(s_httpPrefix))) {
        result = info->ReadIndirectFile(path);
    } else {
        result = info->ReadDirectFile(path);
    }

    if (!result) {
        return result;
    }

    return info->FinalizeManifestItems() & result;
}

// VERIFY: b443e1e7 manual
// OFFSET: 0x004383C0
int32_t System_Streaming::HttpCallback::GetTimeout() {
    return this->m_timeout;
}

// VERIFY: 71266dc6 manual
// OFFSET: 0x004383D0
void System_Streaming::HttpCallback::SetTimeOutValue(uint32_t timeout) {
    this->m_timeout = timeout;
}

// VERIFY: dba30a86 manual
// OFFSET: 0x0043B060
bool System_Streaming::HttpCallback::ReceivedData(const void* data, int32_t size) {
    if (size > 0) {
        this->m_data.append(static_cast<const char*>(data), size);
        this->m_received += size;
    }

    return true;
}

// VERIFY: 3cf1ff27 auto
// OFFSET: 0x008A1420
bool System_Streaming::HttpCallback::ShouldBlock() {
    return true;
}
