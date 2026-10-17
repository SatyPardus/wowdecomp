#pragma once
#include <string>
#include "Engine/Source/BLPFile/BlpImage.h"
namespace AppleBootstrap {
bool CheckMetal(const BLP::Image& image, std::string& report);
}
