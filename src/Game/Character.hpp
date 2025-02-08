#pragma once
#include "CharacterState.hpp"
#include "Data/Animation.hpp"
#include "Game/Mover.hpp"
#include "Rendering/Animator.hpp"
#include <SDL.h>

// TODO: Use constants from Config
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

  // Last horizontal input: -1 for left, +1 for right, 0 for none.
  int inputDirection;

  // State
  CharacterState state;

  Character(Animator *anim);

  SDL_Rect getHitboxRect(HitboxType type = HitboxType::Collision) const;

  void handleInput();

  void update(float deltaTime);

  void render(SDL_Renderer *renderer, float cameraScale = 1.0f);

  void jump();

  void applyDamage(int damage, bool survive = false);

  void updateFacing(const Character &target);

  void attack();
};
