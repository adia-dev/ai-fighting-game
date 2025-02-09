#pragma once

#include <string>

namespace R {
inline static std::string texture(const std::string &resource_name) {
  std::string texture_path(RESOURCE_DIR);
  texture_path += "/textures/" + resource_name;
  return texture_path;
}

inline static std::string animation(const std::string &resource_name) {
  std::string animation_path(RESOURCE_DIR);
  animation_path += "/animations/" + resource_name;
  return animation_path;
}

inline static std::string font(const std::string &resource_name) {
  std::string font_path(RESOURCE_DIR);
  font_path += "/fonts/" + resource_name;
  return font_path;
}

} // namespace R
