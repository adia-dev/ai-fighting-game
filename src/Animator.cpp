// Animator.cpp

#include "Animator.hpp"

Animator::Animator(SDL_Texture *texture)
    : m_texture(texture), m_currentFrameIndex(0), m_timer(0.0f) {}

void Animator::setAnimation(const Animation &anim) {
  m_animation = anim;
  m_currentFrameIndex = 0;
  m_timer = 0.0f;
}

void Animator::update(float deltaTime) {
  if (m_animation.frames.empty())
    return;

  // Convert deltaTime (in seconds) to milliseconds:
  m_timer += deltaTime * 1000.0f;
  float currentDuration = m_animation.frames[m_currentFrameIndex].duration_ms;
  if (m_timer >= currentDuration) {
    m_timer -= currentDuration;
    m_currentFrameIndex++;
    if (m_currentFrameIndex >= (int)m_animation.frames.size()) {
      if (m_animation.loop)
        m_currentFrameIndex = 0;
      else
        m_currentFrameIndex = m_animation.frames.size() - 1;
    }
  }
}

void Animator::render(SDL_Renderer *renderer, int x, int y) {
  if (m_animation.frames.empty())
    return;

  const Frame &frame = m_animation.frames[m_currentFrameIndex];
  SDL_Rect dest = {x, y, frame.frameRect.w, frame.frameRect.h};
  SDL_RendererFlip flip = frame.flipped ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  SDL_RenderCopyEx(renderer, m_texture, &frame.frameRect, &dest, 0.0, nullptr,
                   flip);

  // Draw hitbox outlines (only those with dataType == 1) for debugging.
  for (const auto &hitbox : frame.hitboxes) {
    if (hitbox.enabled && hitbox.dataType == 1) {
      SDL_Rect hitRect = {x + hitbox.x, y + hitbox.y, hitbox.w, hitbox.h};
      SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
      SDL_RenderDrawRect(renderer, &hitRect);
    }
  }
}

const std::vector<Hitbox> &Animator::getCurrentHitboxes() const {
  if (m_animation.frames.empty()) {
    static std::vector<Hitbox> empty;
    return empty;
  }
  return m_animation.frames[m_currentFrameIndex].hitboxes;
}

SDL_Rect Animator::getCurrentFrameRect() const {
  if (m_animation.frames.empty())
    return SDL_Rect{0, 0, 0, 0};
  return m_animation.frames[m_currentFrameIndex].frameRect;
}
