#pragma once

#include <string>

enum class ControlMode { Human, AI, Disabled };

struct CharacterControl {
  ControlMode mode = ControlMode::AI;
  bool enabled = true;
  float epsilon = 0.3f;
  float learningRate = 0.001f;
  float discountFactor = 0.95f;
  std::string name;

  CharacterControl(const std::string &characterName) : name(characterName) {}
};
