#pragma once
#include "Data/Vector2f.hpp"
#include <SDL.h>
#include <string>

class RLVisualization {
public:
  static void renderRadar(SDL_Renderer *renderer,
                          const Vector2f &relativePosition,
                          float distanceToOpponent, const SDL_Rect &bounds) {

    SDL_SetRenderDrawColor(renderer, 0, 20, 0, 200);
    SDL_RenderFillRect(renderer, &bounds);

    SDL_SetRenderDrawColor(renderer, 0, 100, 0, 100);
    const int GRID_CELLS = 8;
    for (int i = 1; i < GRID_CELLS; i++) {
      float x = bounds.x + (bounds.w * i) / GRID_CELLS;
      float y = bounds.y + (bounds.h * i) / GRID_CELLS;

      SDL_RenderDrawLine(renderer, x, bounds.y, x, bounds.y + bounds.h);
      SDL_RenderDrawLine(renderer, bounds.x, y, bounds.x + bounds.w, y);
    }

    const int RANGE_CIRCLES = 3;
    SDL_SetRenderDrawColor(renderer, 0, 150, 0, 100);
    for (int i = 1; i <= RANGE_CIRCLES; i++) {
      float radius = (bounds.w / 2.0f) * (i / float(RANGE_CIRCLES));
      renderCircle(renderer, bounds.x + bounds.w / 2, bounds.y + bounds.h / 2,
                   static_cast<int>(radius));
    }

    int centerX = bounds.x + bounds.w / 2;
    int centerY = bounds.y + bounds.h / 2;
    float scale = bounds.w / 800.0f;

    int blipX = centerX + static_cast<int>(relativePosition.x * scale);
    int blipY = centerY + static_cast<int>(relativePosition.y * scale);

    float distanceRatio = std::min(distanceToOpponent / 400.0f, 1.0f);
    SDL_SetRenderDrawColor(renderer, static_cast<Uint8>(255 * distanceRatio),
                           static_cast<Uint8>(255 * (1.0f - distanceRatio)), 0,
                           255);

    const int BLIP_SIZE = 3;
    SDL_Rect blipRect = {blipX - BLIP_SIZE, blipY - BLIP_SIZE, BLIP_SIZE * 2,
                         BLIP_SIZE * 2};
    SDL_RenderFillRect(renderer, &blipRect);

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &bounds);
  }

  static void renderStatePanel(SDL_Renderer *renderer, const SDL_Rect &bounds,
                               float health, float opponentHealth,
                               const std::string &currentAction, float reward,
                               float confidence) {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
    SDL_RenderFillRect(renderer, &bounds);

    int y = bounds.y + 5;
    const int BAR_HEIGHT = 15;
    const int BAR_SPACING = 25;

    renderGauge(renderer, bounds.x + 5, y, bounds.w - 10, BAR_HEIGHT, health,
                {0, 255, 0, 255}, "Health");
    y += BAR_SPACING;

    renderGauge(renderer, bounds.x + 5, y, bounds.w - 10, BAR_HEIGHT,
                opponentHealth, {255, 0, 0, 255}, "Opp Health");
    y += BAR_SPACING;

    renderGauge(renderer, bounds.x + 5, y, bounds.w - 10, BAR_HEIGHT,
                confidence, {0, 255, 255, 255}, "Confidence");
    y += BAR_SPACING;

    float normalizedReward = (reward + 100.0f) / 200.0f;
    renderGauge(renderer, bounds.x + 5, y, bounds.w - 10, BAR_HEIGHT,
                normalizedReward, {255, 255, 0, 255}, "Reward");
    y += BAR_SPACING;

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderDrawRect(renderer, &bounds);
  }

private:
  static void renderGauge(SDL_Renderer *renderer, int x, int y, int width,
                          int height, float value, SDL_Color color,
                          const std::string &label) {

    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_Rect bgRect = {x, y, width, height};
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_Rect valueRect = {x, y, static_cast<int>(width * value), height};
    SDL_RenderFillRect(renderer, &valueRect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bgRect);
  }

  static void renderCircle(SDL_Renderer *renderer, int x0, int y0, int radius) {
    int x = radius - 1;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int err = dx - (radius << 1);

    while (x >= y) {
      SDL_RenderDrawPoint(renderer, x0 + x, y0 + y);
      SDL_RenderDrawPoint(renderer, x0 + y, y0 + x);
      SDL_RenderDrawPoint(renderer, x0 - y, y0 + x);
      SDL_RenderDrawPoint(renderer, x0 - x, y0 + y);
      SDL_RenderDrawPoint(renderer, x0 - x, y0 - y);
      SDL_RenderDrawPoint(renderer, x0 - y, y0 - x);
      SDL_RenderDrawPoint(renderer, x0 + y, y0 - x);
      SDL_RenderDrawPoint(renderer, x0 + x, y0 - y);

      if (err <= 0) {
        y++;
        err += dy;
        dy += 2;
      }
      if (err > 0) {
        x--;
        dx += 2;
        err += dx - (radius << 1);
      }
    }
  }
};
