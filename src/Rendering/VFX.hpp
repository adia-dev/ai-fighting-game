#include "Data/Vector2f.hpp"

struct ScreenShake {
  float duration;
  float intensity;
  float currentTime;
  Vector2f offset;

  void update(float deltaTime) {
    if (currentTime >= duration)
      return;

    currentTime += deltaTime;
    float progress = currentTime / duration;
    float remaining = 1.0f - progress;

    // Random shake offset
    offset.x = (rand() % 100 - 50) * 0.1f * intensity * remaining;
    offset.y = (rand() % 100 - 50) * 0.1f * intensity * remaining;
  }
};

struct SlowMotion {
  float duration;
  float timeScale;
  float currentTime;
  bool active;
};
