#include "Rendering/Animator.hpp"
#include "Core/DebugGlobals.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include <utility>

Animator::Animator(SDL_Texture *texture)
    : m_texture(texture), m_currentFrameIndex(0), m_timer(0.0f), m_flip(false),
      m_reverse(false) {}

Animator::Animator(SDL_Texture *texture,
                   const std::map<std::string, Animation> &animations)
    : m_texture(texture), m_animations(animations), m_currentFrameIndex(0),
      m_timer(0.0f), m_flip(false), m_reverse(false) {}

void Animator::addAnimation(const std::string &key, const Animation &anim) {
  m_animations.emplace(key, anim);
}

void Animator::play(const std::string &key) {
  if (m_currentKey == key && !m_completedOnce) {
    return;
  }

  auto it = m_animations.find(key);
  if (it != m_animations.end()) {
    m_currentKey = key;
    m_currentAnimation = it->second;
    m_currentFrameIndex =
        m_reverse ? (m_currentAnimation.frames.size() - 1) : 0;
    m_timer = 0.0f;
    m_completedOnce = false;
    Logger::debug("Playing animation: " + key +
                  (m_reverse ? " (reverse)" : ""));
  }
}

void Animator::update(float deltaTime) {
  if (m_currentAnimation.frames.empty()) {
    return;
  }

  // Accumulate time more precisely
  m_timer += deltaTime * 1000.0f;

  // Track how many frames we need to advance
  while (m_timer >=
         m_currentAnimation.frames[m_currentFrameIndex].duration_ms) {
    m_timer -= m_currentAnimation.frames[m_currentFrameIndex].duration_ms;

    if (!m_reverse) {
      m_currentFrameIndex++;
      if (m_currentFrameIndex >=
          static_cast<int>(m_currentAnimation.frames.size())) {
        if (m_currentAnimation.loop) {
          m_currentFrameIndex = 0;
          // Signal animation completion for non-looping animations
          m_completedOnce = true;
        } else {
          m_currentFrameIndex = m_currentAnimation.frames.size() - 1;
          m_completedOnce = true;
        }
      }
    } else {
      m_currentFrameIndex--;
      if (m_currentFrameIndex < 0) {
        if (m_currentAnimation.loop) {
          m_currentFrameIndex = m_currentAnimation.frames.size() - 1;
          m_completedOnce = true;
        } else {
          m_currentFrameIndex = 0;
          m_completedOnce = true;
        }
      }
    }
  }

  Logger::debug("Animation State:");
  Logger::debug("  Current Key: " + m_currentKey);
  Logger::debug("  Frame Index: " + std::to_string(m_currentFrameIndex));
  Logger::debug("  Timer: " + std::to_string(m_timer));
  Logger::debug("  Phase: " +
                std::string(frame_phase_to_string(getCurrentFramePhase())));
  Logger::debug("  Completed Once: " +
                std::string(m_completedOnce ? "true" : "false"));
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

  // Debug: Draw hitboxes
  for (const auto &hitbox : frame.hitboxes) {
    if (g_showDebugOverlay && hitbox.enabled) {
      SDL_Rect hitRect;
      if (m_flip) {
        // Mirror the hitbox relative to the frame width when flipped
        hitRect.x =
            x + static_cast<int>((frame.frameRect.w - (hitbox.x + hitbox.w)) *
                                 scale);
      } else {
        hitRect.x = x + static_cast<int>(hitbox.x * scale);
      }
      hitRect.y = y + static_cast<int>(hitbox.y * scale);
      hitRect.w = static_cast<int>(hitbox.w * scale);
      hitRect.h = static_cast<int>(hitbox.h * scale);

      // Set color based on hitbox type
      switch (hitbox.type) {
      case HitboxType::Hit:
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        break;
      case HitboxType::Collision:
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        break;
      case HitboxType::Block:
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        break;
      case HitboxType::Grab:
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        break;
      }
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

FramePhase Animator::getCurrentFramePhase() const {
  if (m_currentAnimation.frames.empty())
    return FramePhase::None;
  return m_currentAnimation.frames[m_currentFrameIndex].phase;
}

bool Animator::isAnimationFinished() const {
  Logger::debug("ANIMATION FINISHED: " + m_currentAnimation.name);
  if (!m_currentAnimation.loop && !m_currentAnimation.frames.empty() &&
      m_currentFrameIndex ==
          static_cast<int>(m_currentAnimation.frames.size()) - 1) {
    return true;
  }
  return false;
}

std::string Animator::getCurrentAnimationKey() const { return m_currentKey; }
Animation &Animator::getAnimation(const std::string &name) {
  return m_animations[name];
}

bool Animator::hasAnimation(const std::string &name) {
  return m_animations.count(name);
}
