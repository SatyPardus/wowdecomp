#pragma once
class CGxFormat;

namespace GxMetal {
// The native host owns its window/scene. Defaults describe its real drawable,
// independently of the legacy DBC's Windows framebuffer choice.
bool GetHostFormat(CGxFormat& format);
// Call before acquiring a host drawable. macOS windows render at their native
// backing size; UIKit retains its explicitly selected rendering resolution.
// False means a stale activation/resize drawable was discarded; retry next frame.
bool PrepareHostDrawable(void* nativeView);
// Coalesce display restarts requested during input/update/paint. The complete
// restart runs after submission, before the next drawable is acquired.
bool DeferDisplayRestart(void (*restart)());
}
