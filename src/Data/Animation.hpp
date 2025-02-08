#pragma once
#include "FightEnums.hpp"
#include <SDL.h>
#include <string>
#include <vector>

struct Hitbox {
  std::string id;
  bool enabled;
  int x, y, w, h;
  HitboxType type;
};

struct Frame {
  SDL_Rect frameRect;
  float duration_ms;
  bool flipped;
  std::vector<Hitbox> hitboxes;
  FramePhase phase;
};

struct Animation {
  std::string name;
  std::vector<Frame> frames;
  bool loop;
};
