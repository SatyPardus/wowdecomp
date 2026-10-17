#ifndef TOOLS_APPLEBOOTSTRAP_PROBE_H
#define TOOLS_APPLEBOOTSTRAP_PROBE_H
#include <string>

namespace AppleBootstrap {
// Diagnostics only: does not mount an archive stack or select patch/locale order.
bool CheckCore(const std::string& scratchDirectory, std::string& report);
bool InspectData(const std::string& dataRoot, std::string& report);
}
#endif
