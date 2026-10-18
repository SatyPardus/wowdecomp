#pragma once
#include <memory>
namespace Liquid {
class IEnvironment;
class CInstance;
} // namespace Liquid

// Exclusive original LiquidSystem banks/procedural textures for a preview.
// Atmosphere is supplied by the owner; these water-color bands are explicit.
class LiquidPreviewContext {
  public:
    LiquidPreviewContext();
    ~LiquidPreviewContext();
    static Liquid::IEnvironment *CreateEnvironment(bool interior);
    static bool Ready(Liquid::CInstance &instance, bool wait);

  private:
    struct Impl;
    std::unique_ptr<Impl> m;
};
