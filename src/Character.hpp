#pragma once

#include "Animator.hpp"
#include "Mover.hpp"
#include <SDL.h>

static const float GRAVITY = 980.0f;
static const int GROUND_LEVEL = 500;

class Character {
public:
  Mover mover;
  Animator *animator;

  int health;
  int maxHealth;
  bool onGround;

  Character(Animator *anim);

  SDL_Rect getCollisionRect() const;

  void update(float deltaTime);

  void render(SDL_Renderer *renderer);

  void handleInput(const Uint8 *keystate);

  void jump();

  void applyDamage(int damage);
};
