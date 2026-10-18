#if defined(WOW_LOCAL_FILES_ONLY)
#include <atomic>
#include <cerrno>
#include <stdexcept>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"

// Disabled-streaming query semantics from the original backend. This build
// installs no storage manifest and never redirects a local path to a part file.
// Download requests fail explicitly; full local MPQs still use normal SFile IO.
namespace Blizzard::Streaming {
bool s_streamingReady = false;
bool s_isStreamingTrial = false;
bool IsStreamedFile(const char*) { return false; }
bool IsStreamingMode() { return false; }
void MakePartFileName(char*, uint32_t) {}
int32_t CheckFileRange(const char*, int64_t, int32_t, const char*, int32_t) { return -1; }
int32_t GetIsLocalAmount(const char*, int64_t, int32_t size, const char*) { return size; }
static bool UnsupportedDownload() {
    errno = ENOTSUP;
    File::SetLastError(9);
    File::AddToLastErrorStack(ENOTSUP, "Remote streaming is unavailable in the local-files build", 1);
    return false;
}
bool MakeLocalAsync(File::StreamRecord*, int64_t, int32_t, int32_t, const char*) { return UnsupportedDownload(); }
bool MakeLocalBlocking(File::StreamRecord*, int64_t, int32_t, int32_t, const char*) { return UnsupportedDownload(); }
bool InitializeStreaming(const char*, const char*, const char*, const char*, bool) { return UnsupportedDownload(); }
}
bool CheckIfStreamingAndTrial() { return false; }
// These queries gate Glue character selection and the loading screen. The
// original StreamingManager returns ready / 1.0 and ignores state, enable and
// preload requests when CheckIfStreamingAndTrial() is false. This describes
// local installation mode, not successful completion of a remote download.
namespace { std::atomic<int32_t> s_localRace{-1}; }
void StreamingSetRace(int32_t raceID) { s_localRace.store(raceID); }
int32_t StreamingGetRace() { return s_localRace.load(); }
bool StreamingIsRaceDataReady(int32_t) { return true; }
float StreamingGetLoadFraction(int32_t) { return 1.0f; }
void StreamingSetState(int32_t) {}
void StreamingSetEnabled(bool) {}
void StartDataPreloadThread() {}
void StreamingSetMapId(int32_t) {
    throw std::runtime_error("Remote map streaming is unavailable in the local-files build");
}
void StreamingSetAreaOfInterest(int32_t, int32_t) {
    throw std::runtime_error("Remote area streaming is unavailable in the local-files build");
}
#endif
