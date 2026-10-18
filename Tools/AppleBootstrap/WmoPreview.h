#pragma once
#include <cstdint>
#include <memory>
#include <string>

// One real WMO group, submitted by CMapObj::UnifiedRender. The preview supplies
// an explicit camera/daylight and selects no projected shadows; it does not run
// World portal traversal or doodads. Owns an exclusive preview
// world context, like ModelPreview, and must die before Services/Gx teardown.
class WmoPreview {
public:
    explicit WmoPreview(const std::string& selection); // archive root path[:group]
    ~WmoPreview();
    WmoPreview(const WmoPreview&) = delete;
    WmoPreview& operator=(const WmoPreview&) = delete;
    bool Ready(bool wait = false);
    bool Draw(float elapsedSeconds, uint32_t width, uint32_t height, bool fullViewport = false, bool liquidOnly = false);
    uint32_t BatchCount() const;
private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
