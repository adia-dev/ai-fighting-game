#include "Character.hpp"
#include "Core/Input.hpp"
#include <SDL.h>
#include <algorithm>
#include <iostream>

Character::Character(Animator *anim)
    : animator(anim), health(100), maxHealth(100), onGround(false),
      isMoving(false), groundFrames(0), inputDirection(0) {}

SDL_Rect Character::getCollisionRect() const {
  const auto &hitboxes = animator->getCurrentHitboxes();
  SDL_Rect collisionRect;
  bool foundCollisionBox = false;
  for (const auto &hb : hitboxes) {
    // Only use hitboxes of type Collision
    if (!hb.enabled || hb.type != HitboxType::Collision)
      continue;
    SDL_Rect hbRect = {hb.x, hb.y, hb.w, hb.h};
    if (!foundCollisionBox) {
      collisionRect = hbRect;
      foundCollisionBox = true;
    } else {
      int x1 = std::min(collisionRect.x, hbRect.x);
      int y1 = std::min(collisionRect.y, hbRect.y);
      int x2 = std::max(collisionRect.x + collisionRect.w, hbRect.x + hbRect.w);
      int y2 = std::max(collisionRect.y + collisionRect.h, hbRect.y + hbRect.h);
      collisionRect.x = x1;
      collisionRect.y = y1;
      collisionRect.w = x2 - x1;
      collisionRect.h = y2 - y1;
    }
  }
  if (!foundCollisionBox) {
    collisionRect = animator->getCurrentFrameRect();
    std::cout << "[DEBUG] No collision hitbox found; using full frame rect.\n";
  }
  collisionRect.x += static_cast<int>(mover.position.x);
  collisionRect.y += static_cast<int>(mover.position.y);
  return collisionRect;
}

void Character::handleInput() {
  // If the current frame is in Startup or Active phase (e.g. during an attack)
  // then block movement input.
  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Startup || phase == FramePhase::Active) {
    // Optionally, you can allow certain keys (or allow directional input in
    // recovery)
    return;
  }

  // Otherwise, allow movement.
  const float moveForce = 500.0f; // Or use m_config.defaultMoveForce
  isMoving = false;
  inputDirection = 0;

  if (Input::isKeyDown(SDL_SCANCODE_LEFT)) {
    mover.applyForce(Vector2f(-moveForce, 0));
    isMoving = true;
    inputDirection = -1;
  }
  if (Input::isKeyDown(SDL_SCANCODE_RIGHT)) {
    mover.applyForce(Vector2f(moveForce, 0));
    isMoving = true;
    inputDirection = 1;
  }
  // Attack key: use A key.
  if (Input::isKeyDown(SDL_SCANCODE_A)) {
    // Only launch an attack if not already in an attack animation
    // (Since we block input above during startup/active, this means the
    // previous attack finished its blocking phase)
    attack();
  }
  if (Input::isKeyDown(SDL_SCANCODE_SPACE) && onGround &&
      groundFrames >= STABLE_GROUND_FRAMES) {
    jump();
  }
}

void Character::attack() {
  // Start the attack animation.
  // The attack animation is assumed to be non-looping and its frames are tagged
  // with the appropriate FramePhase values (Startup, Active, Recovery).
  animator->play("Attack");
  std::cout << "[DEBUG] Attack initiated.\n";
}

void Character::jump() {
  groundFrames = 0;
  mover.velocity.y = -500.0f;
  onGround = false;
  std::cout << "[DEBUG] Jump initiated.\n";
}

void Character::applyDamage(int damage) {
  health -= damage;
  if (health < 0)
    health = 0;
  std::cout << "[DEBUG] Damage applied: " << damage
            << ". Health now: " << health << "\n";
}

void Character::update(float deltaTime) {
  // Apply gravity if not on ground.
  if (mover.position.y < GROUND_LEVEL - GROUND_THRESHOLD)
    mover.applyForce(Vector2f(0, GRAVITY));

  mover.update(deltaTime);
  animator->update(deltaTime);

  if (mover.position.y >= (GROUND_LEVEL - GROUND_THRESHOLD)) {
    mover.position.y = GROUND_LEVEL;
    mover.velocity.y = 0;
    onGround = true;
    groundFrames++;
  } else {
    onGround = false;
    groundFrames = 0;
  }

  // If we are in the Attack animation, use the current frame’s phase
  // to block or allow movement.
  // For example, if the phase is Startup or Active, do nothing.
  // If the phase is Recovery, you may optionally allow some movement (or wait
  // until the animation fully finishes).
  if (animator->getCurrentFramePhase() == FramePhase::Active ||
      animator->getCurrentFramePhase() == FramePhase::Startup) {
    // Block new movement input here; you might also want to zero out horizontal
    // velocity if you want to “lock” the character.
    // (Optionally, you could call handleInput() only when in Recovery.)
  } else {
    // Not in attack blocking phase. If no input is given, default to idle.
    if (!isMoving) {
      animator->play("Idle");
    } else {
      animator->play("Walk");
    }
  }
}

void Character::render(SDL_Renderer *renderer, float cameraScale) {
  animator->render(renderer, static_cast<int>(mover.position.x),
                   static_cast<int>(mover.position.y), cameraScale);
  // Render health bar (unchanged)
  SDL_Rect collRect = getCollisionRect();
  SDL_Rect healthBar = {collRect.x, collRect.y - 10, collRect.w, 5};
  float ratio = static_cast<float>(health) / maxHealth;
  SDL_Rect healthFill = {healthBar.x, healthBar.y,
                         static_cast<int>(healthBar.w * ratio), healthBar.h};
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  SDL_RenderFillRect(renderer, &healthBar);
  SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
  SDL_RenderFillRect(renderer, &healthFill);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderDrawRect(renderer, &healthBar);
}

void Character::updateFacing(const Character &target) {
  SDL_Rect myRect = getCollisionRect();
  SDL_Rect targetRect = target.getCollisionRect();
  int myCenterX = myRect.x + myRect.w / 2;
  int targetCenterX = targetRect.x + targetRect.w / 2;
  // Always face the enemy.
  animator->setFlip(targetCenterX < myCenterX);
  // Determine if input is “forward” relative to the enemy.
  int forwardDirection = (targetCenterX > myCenterX) ? 1 : -1;
  if (isMoving) {
    if (inputDirection == forwardDirection) {
      animator->setReverse(false);
      std::cout << "[DEBUG] Playing walk animation normally (forward).\n";
    } else {
      animator->setReverse(true);
      std::cout << "[DEBUG] Playing walk animation in reverse (backward).\n";
    }
  } else {
    animator->setReverse(false);
  }
  // std::cout << "[DEBUG] updateFacing: My center = " << myCenterX
  //           << ", Target center = " << targetCenterX
  //           << " => flip = " << (targetCenterX < myCenterX ? "true" :
  //           "false")
  //           << ", inputDirection = " << inputDirection << "\n";
}
