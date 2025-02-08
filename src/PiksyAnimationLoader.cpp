#include "PiksyAnimationLoader.hpp"
#include "utilities/json.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using json = nlohmann::json;

namespace PiksyAnimationLoader {

Animation loadAnimation(const std::string &jsonFilePath) {
  std::ifstream ifs(jsonFilePath);
  if (!ifs.is_open())
    throw std::runtime_error("Could not open file: " + jsonFilePath);

  json j;
  ifs >> j;

  Animation animation;
  // We assume the JSON contains an "animations" array; take the first
  // animation.
  if (!j.contains("animations") || !j["animations"].is_array() ||
      j["animations"].empty())
    throw std::runtime_error("Invalid JSON: no animations found");

  auto animJson = j["animations"][0];
  animation.name = animJson.value("name", "Unnamed Animation");
  animation.loop = true; // For our MVP we simply loop.

  // Get the frames array.
  if (!animJson.contains("frames") || !animJson["frames"].is_array())
    throw std::runtime_error("Invalid JSON: no frames array");

  for (auto &frameJson : animJson["frames"]) {
    if (!frameJson.value("enabled", false))
      continue; // Skip disabled frames.

    Frame frame;
    // Read the top-level rectangle data.
    frame.frameRect.x = frameJson["x"].get<int>();
    frame.frameRect.y = frameJson["y"].get<int>();
    frame.frameRect.w = frameJson["w"].get<int>();
    frame.frameRect.h = frameJson["h"].get<int>();
    frame.flipped = frameJson.value("flipped", false);

    // Duration (in ms) from frame_data->metadata->duration_ms.
    if (frameJson.contains("frame_data") &&
        frameJson["frame_data"].contains("metadata") &&
        frameJson["frame_data"]["metadata"].contains("duration_ms")) {
      frame.duration_ms =
          frameJson["frame_data"]["metadata"]["duration_ms"].get<float>();
    } else {
      frame.duration_ms = 100.0f; // default value
    }

    // Process hitboxes from frame_data->hitboxes array.
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
        // Get the hitbox data type from custom_data (if available).
        if (frameJson["frame_data"].contains("custom_data") &&
            frameJson["frame_data"]["custom_data"].contains(
                "hitbox_0_data_type")) {
          hitbox.dataType =
              frameJson["frame_data"]["custom_data"]["hitbox_0_data_type"]
                  .get<int>();
        } else {
          hitbox.dataType = 0;
        }
        frame.hitboxes.push_back(hitbox);
      }
    }
    animation.frames.push_back(frame);
  }

  return animation;
}

} // namespace PiksyAnimationLoader
