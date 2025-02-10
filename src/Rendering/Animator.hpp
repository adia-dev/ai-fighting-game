#pragma once

#include <Data/Animation.hpp>
#include <SDL.h>
#include <map>
#include <string>
#include <vector>

class Animator {
public:
  // Construct an Animator using a spritesheet texture.
  Animator(SDL_Texture *texture);

  // Construct an Animator using a spritesheet texture
  // and forwards the animations
  Animator(SDL_Texture *texture,
           const std::map<std::string, Animation> &animations);

  // Add an animation with a key.
  void addAnimation(const std::string &key, const Animation &anim);

  // Play (switch to) the animation identified by key.
  void play(const std::string &key);

  // Update the animation timer (deltaTime in seconds).
  void update(float deltaTime);

  // Render the current frame at the given screen position, scaled by 'scale'.
  void render(SDL_Renderer *renderer, int x, int y, float scale);

  // Retrieve current hitboxes.
  const std::vector<Hitbox> &getCurrentHitboxes() const;

  // Get the current frame’s rectangle.
  SDL_Rect getCurrentFrameRect() const;

  FramePhase getCurrentFramePhase() const;

  Animation &getAnimation(const std::string &name);

  bool hasAnimation(const std::string &name);

  // Set whether to flip the sprite horizontally.
  void setFlip(bool flip) { m_flip = flip; }
  bool getFlip() const { return m_flip; }

  // Set whether the animation should play in reverse.
  void setReverse(bool reverse) { m_reverse = reverse; }
  bool getReverse() const { return m_reverse; }

  bool isAnimationFinished() const;

  std::string getCurrentAnimationKey() const;

  void setFrameIndex(int index) {
    m_currentFrameIndex = index;
    m_timer = 0.0f;
  }

private:
  SDL_Texture *m_texture;
  std::map<std::string, Animation> m_animations;
  Animation m_currentAnimation;
  std::string m_currentKey;
  int m_currentFrameIndex;
  float m_timer;
  bool m_flip;
  bool m_reverse;
  bool m_completedOnce = false;
};
