
#pragma once
#include "Animator.hpp"
#include "Mover.hpp"
#include "Vector2f.hpp"
#include <SDL.h>
#include <string>

// Constants:
static const float GRAVITY = 980.0f; // pixels per second squared
static const int GROUND_LEVEL = 500; // y

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
