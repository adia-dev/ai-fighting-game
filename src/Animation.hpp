#pragma once
#include <SDL.h>
#include <string>
#include <vector>

// A simple hitbox record.
struct Hitbox {
  std::string id; // e.g. "hitbox_0"
  bool enabled;
  int x, y, w, h;
  int dataType; // e.g. if 1 then use as collision box
};

// A single frame of an animation.
struct Frame {
  SDL_Rect frameRect;           // The rectangle from the spritesheet.
  float duration_ms;            // How long this frame lasts.
  bool flipped;                 // Whether to flip the frame horizontally.
  std::vector<Hitbox> hitboxes; // One or more hitboxes for this frame.
};

// An animation record: a named sequence of frames.
struct Animation {
  std::string name;
  std::vector<Frame> frames;
  bool loop; // Whether the animation loops.
};
