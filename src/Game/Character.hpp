#pragma once
#include "CharacterState.hpp"
#include "Core/Config.hpp"
#include "Data/Animation.hpp"
#include "Game/Mover.hpp"
#include "Rendering/Animator.hpp"
#include "Rendering/Camera.hpp"
#include <SDL.h>

class Character {
public:
  Mover mover;
  Animator *animator;
  int health;
  int maxHealth;
  bool onGround;
  bool isMoving;
  int groundFrames;
  bool lastAttackLanded;
  bool lastBlockEffective;

  // TODO: Change it to an enum
  // Last horizontal input: -1 for left, +1 for right, 0 for none.
  int inputDirection;
  int comboCount = 0;
  float stamina;
  float maxStamina;

  // State
  CharacterState state;

  Character(Animator *anim, Config &config);

  SDL_Rect getHitboxRect(HitboxType type = HitboxType::Collision) const;

  void handleInput();

  void update(float deltaTime);

  void render(SDL_Renderer *renderer, float cameraScale = 1.0f);
  void renderWithCamera(SDL_Renderer *renderer, const Camera &camera,
                        const Config &config);

  void jump();

  void move(const Vector2f &force);

  void applyDamage(int damage, bool survive = false);

  void updateFacing(const Character &target);

  void attack();

  void dash();

  void block();

  void updateJumpAnimation();

private:
  Config &m_config;
};
