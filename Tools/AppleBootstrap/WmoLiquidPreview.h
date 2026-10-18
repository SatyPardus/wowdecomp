#pragma once
#include <memory>
class CMapObj;
class CMapObjGroup;
class C44Matrix;
class C3Vector;

// Original LiquidSystem material/settings/geometry in the WMO preview's
// exclusive world context. Lighting and water-color bands are explicit inputs.
class WmoLiquidPreview {
public:
    WmoLiquidPreview(CMapObj& object, CMapObjGroup& group);
    ~WmoLiquidPreview();
    bool Ready(bool wait);
    void Draw(const C3Vector& camera);
private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
