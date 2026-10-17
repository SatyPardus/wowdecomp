#if defined(WOW_LOCAL_FILES_ONLY)
#include <cerrno>
#include "Engine/Source/BlizzardCore/Source/Packages/Streaming/Streaming.h"
#include "Engine/Source/BlizzardCore/Source/System/File/File.h"

// Disabled-streaming query semantics from the original backend. This build
// installs no storage manifest and never redirects a local path to a part file.
// Download requests fail explicitly; full local MPQs still use normal SFile IO.
namespace Blizzard::Streaming {
bool s_streamingReady = false;
bool s_isStreamingTrial = false;
bool IsStreamedFile(const char*) { return false; }
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
}
#endif
