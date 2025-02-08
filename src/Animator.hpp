#pragma once
#include "Animation.hpp"
#include <SDL.h>
#include <vector>

class Animator {
public:
  // Construct an Animator using a spritesheet texture.
  Animator(SDL_Texture *texture);

  // Set the current animation.
  void setAnimation(const Animation &anim);

  // Update the animation timer (deltaTime in milliseconds).
  void update(float deltaTime);

  // Render the current frame at the given screen position.
  void render(SDL_Renderer *renderer, int x, int y);

  // Retrieve current hitboxes (if needed for collision detection).
  const std::vector<Hitbox> &getCurrentHitboxes() const;

  SDL_Rect getCurrentFrameRect() const;

  // Set whether to flip the sprite horizontally.
  void setFlip(bool flip) { m_flip = flip; }
  bool getFlip() const { return m_flip; }

private:
  SDL_Texture *m_texture;
  Animation m_animation;
  int m_currentFrameIndex;
  float m_timer;
  bool m_flip;
};
