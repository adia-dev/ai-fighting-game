#pragma once
#include "Animation.hpp"
#include <string>

namespace PiksyAnimationLoader {
// Parses a Piksy JSON animation file and returns an Animation.
Animation loadAnimation(const std::string &jsonFilePath);
} // namespace PiksyAnimationLoader
