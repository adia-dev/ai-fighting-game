#include "Animator.hpp"
#include <iostream>

Animator::Animator(SDL_Texture *texture)
    : m_texture(texture), m_currentFrameIndex(0), m_timer(0.0f), m_flip(false),
      m_reverse(false) {}

void Animator::addAnimation(const std::string &key, const Animation &anim) {
  m_animations.emplace(key, anim);
}

void Animator::play(const std::string &key) {

  if (m_currentKey == key)
    return;
  auto it = m_animations.find(key);
  if (it != m_animations.end()) {
    m_currentKey = key;
    m_currentAnimation = it->second;
    m_currentFrameIndex =
        m_reverse ? (m_currentAnimation.frames.size() - 1) : 0;
    m_timer = 0.0f;
    std::cout << "[DEBUG] Playing animation: " << key
              << (m_reverse ? " (reverse)" : "") << "\n";
  } else {
    std::cerr << "[ERROR] Animation key '" << key << "' not found.\n";
  }
}

void Animator::update(float deltaTime) {
  if (m_currentAnimation.frames.empty())
    return;

  m_timer += deltaTime * 1000.0f;
  float currentDuration =
      m_currentAnimation.frames[m_currentFrameIndex].duration_ms;
  if (m_timer >= currentDuration) {
    m_timer -= currentDuration;
    if (!m_reverse) {
      m_currentFrameIndex++;
      if (m_currentFrameIndex >=
          static_cast<int>(m_currentAnimation.frames.size())) {
        if (m_currentAnimation.loop)
          m_currentFrameIndex = 0;
        else
          m_currentFrameIndex = m_currentAnimation.frames.size() - 1;
      }
    } else {
      m_currentFrameIndex--;
      if (m_currentFrameIndex < 0) {
        if (m_currentAnimation.loop)
          m_currentFrameIndex = m_currentAnimation.frames.size() - 1;
        else
          m_currentFrameIndex = 0;
      }
    }
  }
}

void Animator::render(SDL_Renderer *renderer, int x, int y) {
  if (m_currentAnimation.frames.empty())
    return;
  const Frame &frame = m_currentAnimation.frames[m_currentFrameIndex];
  SDL_Rect dest = {x, y, frame.frameRect.w, frame.frameRect.h};
  SDL_RendererFlip flip = m_flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  SDL_RenderCopyEx(renderer, m_texture, &frame.frameRect, &dest, 0.0, nullptr,
                   flip);

  for (const auto &hitbox : frame.hitboxes) {
    if (hitbox.enabled && hitbox.dataType == 1) {
      SDL_Rect hitRect = {x + hitbox.x, y + hitbox.y, hitbox.w, hitbox.h};
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      SDL_RenderDrawRect(renderer, &hitRect);
    }
  }
}

const std::vector<Hitbox> &Animator::getCurrentHitboxes() const {
  if (m_currentAnimation.frames.empty()) {
    static std::vector<Hitbox> empty;
    return empty;
  }
  return m_currentAnimation.frames[m_currentFrameIndex].hitboxes;
}

SDL_Rect Animator::getCurrentFrameRect() const {
  if (m_currentAnimation.frames.empty())
    return SDL_Rect{0, 0, 0, 0};
  return m_currentAnimation.frames[m_currentFrameIndex].frameRect;
}
