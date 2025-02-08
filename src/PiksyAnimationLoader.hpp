#pragma once
#include "Animation.hpp"
#include <map>
#include <string>

namespace PiksyAnimationLoader {
// Parses a Piksy JSON animation file and returns an Animation.
std::map<std::string, Animation> loadAnimation(const std::string &jsonFilePath);
} // namespace PiksyAnimationLoader
