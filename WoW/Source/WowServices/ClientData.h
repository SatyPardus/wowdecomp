#ifndef WOW_SOURCE_WOWSERVICES_CLIENTDATA_H
#define WOW_SOURCE_WOWSERVICES_CLIENTDATA_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace System_SFile { class ARCHIVEREC; }

// Owns a full local installation's SFile mounts. Remote streaming and the
// client's signature policy remain separate initialization steps.
class ClientData {
public:
    struct CloseArchive { void operator()(System_SFile::ARCHIVEREC* archive) const; };
    struct Mount {
        std::string path;
        std::string prefix;
        int32_t priority;
        std::unique_ptr<System_SFile::ARCHIVEREC, CloseArchive> archive;
    };
    struct Asset {
        std::vector<uint8_t> bytes;
        std::string archive;
        bool digestVerified = false;
    };

    ClientData(const std::string& dataRoot, const std::string& requestedLocale);
    ~ClientData();
    ClientData(const ClientData&) = delete;
    ClientData& operator=(const ClientData&) = delete;
    Asset Read(const std::string& name, uint32_t maximumBytes, bool verifyDigest = false) const;
    const std::string& Locale() const { return m_locale; }
    const std::vector<Mount>& Mounts() const { return m_mounts; }

private:
    std::string m_root;
    std::string m_locale;
    std::vector<Mount> m_mounts;
    std::string m_previousBasePath;
    std::string m_previousDataPath;
    std::string m_previousAlternatePath;
    bool m_filePathsSelected = false;
    std::string Resolve(const std::string& relative) const;
    bool Open(const std::string& relative, int32_t priority, bool required);
};

#endif
