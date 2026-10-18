#pragma once
class CGxFormat;

namespace GxMetal {
// The native host owns its window/scene. Defaults describe its real drawable,
// independently of the legacy DBC's Windows framebuffer choice.
bool GetHostFormat(CGxFormat& format);
// Coalesce display restarts requested during input/update/paint. The complete
// restart runs after submission, before the next drawable is acquired.
bool DeferDisplayRestart(void (*restart)());
}
