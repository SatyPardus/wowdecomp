#pragma once
#include <string>
#include "Engine/Source/BLPFile/BlpImage.h"
#include "Engine/Source/Gx/Metal/UIShader.h"
namespace AppleBootstrap {
bool CheckMetal(const BLP::Image& image, const GxMetal::UIShader& ui, std::string& report);
}
