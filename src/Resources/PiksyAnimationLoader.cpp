#include <Resources/PiksyAnimationLoader.hpp>

#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include "utilities/json.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
using json = nlohmann::json;

namespace PiksyAnimationLoader {

std::map<std::string, Animation>
loadAnimation(const std::string &jsonFilePath) {
  std::ifstream ifs(jsonFilePath);
  if (!ifs.is_open())
    throw std::runtime_error("Could not open file: " + jsonFilePath);

  json j;
  ifs >> j;

  std::map<std::string, Animation> animations;

  if (!j.contains("animations") || !j["animations"].is_array() ||
      j["animations"].empty())
    throw std::runtime_error("Invalid JSON: no animations found");

  for (auto animJson : j["animations"]) {
    Animation animation;
    std::string name = animJson.value("name", "Unnamed Animation");
    animation.name = name;
    animation.loop = animJson.value("loop", true);

    if (!animJson.contains("frames") || !animJson["frames"].is_array())
      throw std::runtime_error("Invalid JSON: no frames array");

    for (auto &frameJson : animJson["frames"]) {
      if (!frameJson.value("enabled", false))
        continue;

      Frame frame;

      frame.frameRect.x = frameJson["x"].get<int>();
      frame.frameRect.y = frameJson["y"].get<int>();
      frame.frameRect.w = frameJson["w"].get<int>();
      frame.frameRect.h = frameJson["h"].get<int>();
      frame.flipped = frameJson.value("flipped", false);
      frame.phase = frameJson.value("phase", FramePhase::None);

      if (frameJson.contains("frame_data") &&
          frameJson["frame_data"].contains("metadata") &&
          frameJson["frame_data"]["metadata"].contains("duration_ms")) {
        frame.duration_ms =
            frameJson["frame_data"]["metadata"]["duration_ms"].get<float>();
      } else {
        frame.duration_ms = 100.0f;
      }

      if (frameJson.contains("frame_data") &&
          frameJson["frame_data"].contains("hitboxes") &&
          frameJson["frame_data"]["hitboxes"].is_array()) {
        for (auto &hbJson : frameJson["frame_data"]["hitboxes"]) {
          if (!hbJson.value("enabled", false))
            continue;
          Hitbox hitbox;
          hitbox.id = hbJson.value("id", "unknown");
          hitbox.x = hbJson["x"].get<int>();
          hitbox.y = hbJson["y"].get<int>();
          hitbox.w = hbJson["w"].get<int>();
          hitbox.h = hbJson["h"].get<int>();
          hitbox.enabled = true;

          std::string hibtox_data_type_id = hitbox.id + "_data_type";
          if (frameJson["frame_data"].contains("custom_data") &&
              frameJson["frame_data"]["custom_data"].contains(
                  hibtox_data_type_id)) {
            hitbox.type =
                frameJson["frame_data"]["custom_data"][hibtox_data_type_id]
                    .get<HitboxType>();
          } else {
            hitbox.type = HitboxType::Collision;
          }
          frame.hitboxes.push_back(hitbox);
        }
      }
      animation.frames.push_back(frame);
    }

    animations.emplace(name, animation);
  }

  return animations;
}

} // namespace PiksyAnimationLoader
