#pragma once
#include "Data/Vector2f.hpp"
#include "Game/Character.hpp"
#include <SDL.h>

class CollisionSystem {
public:
  static bool checkCollision(const SDL_Rect &a, const SDL_Rect &b);

  static void resolveCollision(Character &a, Character &b);

  static void applyCollisionImpulse(Character &a, Character &b,
                                    float impulseStrength);
};
