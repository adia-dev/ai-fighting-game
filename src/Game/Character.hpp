#pragma once
#include "Data/Animation.hpp"
#include "Game/Mover.hpp"
#include "Rendering/Animator.hpp"

#include <SDL.h>

static const float GRAVITY = 980.0f;
static const int GROUND_LEVEL = 500;
static const int GROUND_THRESHOLD = 5;
static const int STABLE_GROUND_FRAMES = 3;

class Character {
public:
  Mover mover;
  Animator *animator;
  int health;
  int maxHealth;
  bool onGround;
  bool isMoving;
  int groundFrames;

  // Preloaded animations for quick switching.
  Animation walkAnimation;
  Animation idleAnimation;

  // Last horizontal input: -1 for left, +1 for right, 0 for none.
  int inputDirection;

  Character(Animator *anim);

  SDL_Rect getCollisionRect() const;

  // Instead of passing keystate, the Character now reads input via our Input
  // helper.
  void handleInput();

  void update(float deltaTime);

  void render(SDL_Renderer *renderer, float cameraScale = 1.0f);

  void jump();

  void applyDamage(int damage);

  // Update facing and (if needed) set reverse playback if moving backward.
  void updateFacing(const Character &target);
};
