#pragma once
#include "Animation.hpp"
#include <SDL.h>
#include <map>
#include <string>
#include <vector>

class Animator {
public:
  // Construct an Animator using a spritesheet texture.
  Animator(SDL_Texture *texture);

  // Add an animation with the given key.
  void addAnimation(const std::string &key, const Animation &anim);

  // Play (switch to) the animation identified by the key.
  void play(const std::string &key);

  // Update the current animation (deltaTime in seconds).
  void update(float deltaTime);

  // Render the current frame at the given screen position.
  void render(SDL_Renderer *renderer, int x, int y);

  // Retrieve current hitboxes.
  const std::vector<Hitbox> &getCurrentHitboxes() const;

  // Get the current frame’s rectangle.
  SDL_Rect getCurrentFrameRect() const;

  // Set/get the horizontal flip.
  void setFlip(bool flip) { m_flip = flip; }
  bool getFlip() const { return m_flip; }

private:
  SDL_Texture *m_texture; // The spritesheet.
  std::map<std::string, Animation> m_animations;
  Animation m_currentAnimation;
  std::string m_currentKey;
  int m_currentFrameIndex;
  float m_timer;
  bool m_flip;
};
