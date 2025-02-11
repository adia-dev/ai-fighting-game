#pragma once
#include "Game/Character.hpp"
#include "Rendering/Text.hpp"
#include <SDL.h>
#include <sstream>

class DebugOverlay {
public:
  static void renderCharacterInfo(SDL_Renderer *renderer,
                                  const Character &character,
                                  const Camera &camera, const Config &config) {
    Vector2f screenPos =
        worldToScreen(character.mover.position, camera, config);

    std::stringstream ss;
    ss << "Pos: (" << (int)character.mover.position.x << ", "
       << (int)character.mover.position.y << ")\n"
       << "Health: " << character.health << "/" << character.maxHealth << "\n"
       << "Stamina: " << (int)character.stamina << "\n"
       << "Animation: " << character.animator->getCurrentAnimationKey() << "\n"
       << "Combo: " << character.comboCount;

    SDL_Color textColor = {255, 255, 255, 255};
    drawText(renderer, ss.str(), (int)screenPos.x + 50, (int)screenPos.y - 80,
             textColor);
  }

  static void renderGameZones(SDL_Renderer *renderer, const Camera &camera,
                              const Config &config) {
    Vector2f leftDanger =
        worldToScreen(Vector2f(config.ai.deadzoneBoundary, 0), camera, config);
    Vector2f rightDanger = worldToScreen(
        Vector2f(config.windowWidth - config.ai.deadzoneBoundary, 0), camera,
        config);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 64);
    SDL_Rect leftRect = {0, 0, (int)leftDanger.x, config.windowHeight};
    SDL_RenderFillRect(renderer, &leftRect);

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 64);
    SDL_Rect rightRect = {(int)rightDanger.x, 0,
                          config.windowWidth - (int)rightDanger.x,
                          config.windowHeight};
    SDL_RenderFillRect(renderer, &rightRect);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 64);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
  }

private:
  static Vector2f worldToScreen(const Vector2f &worldPos, const Camera &camera,
                                const Config &config) {
    float offsetX =
        config.windowWidth * 0.5f - camera.position.x * camera.scale;
    float offsetY =
        config.windowHeight * 0.5f - camera.position.y * camera.scale;

    return Vector2f(offsetX + worldPos.x * camera.scale,
                    offsetY + worldPos.y * camera.scale);
  }

  static void drawOptimalRangeCircle(SDL_Renderer *renderer,
                                     const Camera &camera,
                                     const Config &config) {
    const int SEGMENTS = 32;
    const float OPTIMAL_RANGE = 200.0f;

    for (int i = 0; i < SEGMENTS; i++) {
      float angle1 = (float)i / SEGMENTS * 2 * M_PI;
      float angle2 = (float)(i + 1) / SEGMENTS * 2 * M_PI;

      Vector2f pos1(OPTIMAL_RANGE * cos(angle1), OPTIMAL_RANGE * sin(angle1));
      Vector2f pos2(OPTIMAL_RANGE * cos(angle2), OPTIMAL_RANGE * sin(angle2));

      Vector2f screenPos1 = worldToScreen(pos1, camera, config);
      Vector2f screenPos2 = worldToScreen(pos2, camera, config);

      SDL_RenderDrawLine(renderer, (int)screenPos1.x, (int)screenPos1.y,
                         (int)screenPos2.x, (int)screenPos2.y);
    }
  }
};
