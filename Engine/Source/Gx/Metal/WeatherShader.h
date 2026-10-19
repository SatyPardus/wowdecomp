#pragma once
#include "PostProcessShader.h"

namespace GxMetal {
enum class WeatherVertex : uint32_t { None, Snow, Sand, Rain, Splash };
inline WeatherVertex WeatherEffect(FragmentEffect effect) {
    if (effect == FragmentEffect::WeatherSnow) return WeatherVertex::Snow;
    if (effect == FragmentEffect::WeatherSand) return WeatherVertex::Sand;
    if (effect == FragmentEffect::WeatherRain) return WeatherVertex::Rain;
    if (effect == FragmentEffect::WeatherSplash) return WeatherVertex::Splash;
    return WeatherVertex::None;
}
}
