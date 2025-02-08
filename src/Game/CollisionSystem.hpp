#pragma once
#include "Data/Vector2f.hpp"
#include "Game/Character.hpp"
#include <SDL.h>

class CollisionSystem {
public:
  // Returns true if the two rectangles intersect.
  static bool checkCollision(const SDL_Rect &a, const SDL_Rect &b);

  // Adjusts the positions of two characters if their collision rectangles
  // overlap.
  static void resolveCollision(Character &a, Character &b);

  // Applies an impulse force to separate two colliding characters.
  static void applyCollisionImpulse(Character &a, Character &b,
                                    float impulseStrength);
};
