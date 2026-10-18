#if defined(__APPLE__)
#include "Engine/Source/BlizzardCore/SFile2-Core.h"
#include "Engine/Source/Os/OsFile.h"
#include "WoW/Source/WowServices/ArchiveLayout.h"
#include "WoW/Source/WowServices/ClientData.h"
#include "WoW/Source/Client.h"
#include <algorithm>
#include <stdexcept>
#include <string>

bool CheckDataFilesExist() {
    // ClientData selects these paths from the configured resource root. The
    // native app's working directory is not the installation directory.
    std::string path = SFile::m_basePath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += System_SFile2::s_dataPath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') path += '/';
    path += ClientArchiveLayout[17].m_name;
    std::replace(path.begin(), path.end(), '\\', '/');
    return OsFileExists(path.c_str()) != 0;
}

uint8_t GetClientExpansionLevel() {
    const auto* client = ClientData::Selected();
    return client ? client->ExpansionLevel() : 0;
}

void ListInterfaceArchive(const char* prefix, int32_t (*callback)(const char*, void*), void* parameter) {
    const auto* client = ClientData::Selected();
    if (!client) throw std::runtime_error("Interface enumeration requires a mounted client installation");
    client->ListInterface(prefix, callback, parameter);
}
#endif
