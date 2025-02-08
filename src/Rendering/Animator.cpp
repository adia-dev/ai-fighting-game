#include <Rendering/Animator.hpp>
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
  m_timer += deltaTime * 1000.0f; // deltaTime is in seconds, convert to ms.
  float currentDuration =
      m_currentAnimation.frames[m_currentFrameIndex].duration_ms;
  if (m_timer >= currentDuration) {
    m_timer -= currentDuration;
    if (!m_reverse) {
      m_currentFrameIndex++;
      if (m_currentFrameIndex >=
          static_cast<int>(m_currentAnimation.frames.size())) {
        m_currentFrameIndex =
            m_currentAnimation.loop ? 0 : m_currentAnimation.frames.size() - 1;
      }
    } else {
      m_currentFrameIndex--;
      if (m_currentFrameIndex < 0) {
        m_currentFrameIndex =
            m_currentAnimation.loop ? m_currentAnimation.frames.size() - 1 : 0;
      }
    }
  }
}

void Animator::render(SDL_Renderer *renderer, int x, int y, float scale) {
  if (m_currentAnimation.frames.empty())
    return;
  const Frame &frame = m_currentAnimation.frames[m_currentFrameIndex];
  SDL_Rect dest;
  dest.x = x;
  dest.y = y;
  dest.w = static_cast<int>(frame.frameRect.w * scale);
  dest.h = static_cast<int>(frame.frameRect.h * scale);
  SDL_RendererFlip flip = m_flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
  SDL_RenderCopyEx(renderer, m_texture, &frame.frameRect, &dest, 0.0, nullptr,
                   flip);

  // Draw collision hitboxes (scaled) for debugging.
  for (const auto &hitbox : frame.hitboxes) {
    if (hitbox.enabled && hitbox.dataType == 1) {
      SDL_Rect hitRect;
      hitRect.x = x + static_cast<int>(hitbox.x * scale);
      hitRect.y = y + static_cast<int>(hitbox.y * scale);
      hitRect.w = static_cast<int>(hitbox.w * scale);
      hitRect.h = static_cast<int>(hitbox.h * scale);
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
