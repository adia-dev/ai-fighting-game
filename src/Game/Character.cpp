#include "Character.hpp"
#include "Core/DebugEvents.hpp"
#include "Core/DebugGlobals.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include "Data/Vector2f.hpp"
#include <SDL.h>
#include <algorithm>

Character::Character(Animator *anim)
    : animator(anim), health(1000), maxHealth(1000), onGround(false),
      isMoving(false), groundFrames(0), inputDirection(0) {}

SDL_Rect Character::getHitboxRect(HitboxType type) const {
  const auto &hitboxes = animator->getCurrentHitboxes();
  SDL_Rect collisionRect;
  bool foundCollisionBox = false;

  // Get the current frame rect to use its width as reference
  SDL_Rect frameRect = animator->getCurrentFrameRect();

  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != type)
      continue;

    SDL_Rect hbRect;
    if (animator->getFlip()) {
      // When flipped, mirror the hitbox's x coordinate relative to the frame
      // width
      hbRect.x = frameRect.w - (hb.x + hb.w);
    } else {
      hbRect.x = hb.x;
    }
    hbRect.y = hb.y;
    hbRect.w = hb.w;
    hbRect.h = hb.h;

    if (!foundCollisionBox) {
      collisionRect = hbRect;
      foundCollisionBox = true;
    } else {
      // Combine multiple hitboxes into one bounding box
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
    // If no hitbox found, use a default small box in the center
    collisionRect = {frameRect.w / 4, frameRect.h / 4, frameRect.w / 2,
                     frameRect.h / 2};
  }

  // Position the hitbox relative to the world
  collisionRect.x += static_cast<int>(mover.position.x);
  collisionRect.y += static_cast<int>(mover.position.y);
  return collisionRect;
}

void Character::handleInput() {
  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Startup || phase == FramePhase::Active) {
    return;
  }

  const float moveForce = 500.0f;
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
  if (Input::isKeyDown(SDL_SCANCODE_A)) {
    attack();
  }
  if (Input::isKeyDown(SDL_SCANCODE_B)) {
    block();
  }
  if (Input::isKeyDown(SDL_SCANCODE_SPACE) && onGround &&
      groundFrames >= STABLE_GROUND_FRAMES) {
    jump();
  }
}

void Character::attack() {
  FramePhase phase = animator->getCurrentFramePhase();
  auto currentAnimationKey = animator->getCurrentAnimationKey();

  // Combo system
  if (phase == FramePhase::Recovery) {
    if (currentAnimationKey == "Attack") {
      animator->play("Attack 2");
      Logger::debug("Combo x2, Launching second attack!");
      return;
    } else if (currentAnimationKey == "Attack 2") {
      animator->play("Attack 3");
      Logger::debug("Combo x3, Launching third attack!");
      return;
    }
  }

  // Normal attack if not in a combo
  if (phase != FramePhase::Active && phase != FramePhase::Startup) {
    animator->play("Attack");
    Logger::debug("Attack initiated.");
  }
}

void Character::block() {
  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Active)
    return;
  animator->play("Block");
}

void Character::jump() {
  groundFrames = 0;
  mover.velocity.y = -500.0f;
  onGround = false;
  Logger::debug("Jump initiated.");
}

void Character::move(const Vector2f &force) { mover.applyForce(force); }

void Character::applyDamage(int damage, bool survive) {
  bool isBlocking = animator->getCurrentAnimationKey() == "Block";
  health -= damage * (isBlocking ? 0.1f : 1.f);
  if (health < 0)
    health = survive ? 1 : 0;
  Logger::debug("Damage applied: " + std::to_string(damage) +
                ". Health now: " + std::to_string(health));
  // If floating damage is enabled, add a damage event.
  if (g_showFloatingDamage) {
    addDamageEvent(mover.position, damage);
  }
}

void Character::update(float deltaTime) {
  // First compute collision rectangle and apply physics
  SDL_Rect collRect = getHitboxRect();
  if (collRect.h == 0) {
    collRect = animator->getCurrentFrameRect();
  }

  // Apply gravity if not on ground
  if (!onGround) {
    mover.applyForce(Vector2f(0, GRAVITY));
  }

  // Update physics and animation
  mover.update(deltaTime);
  animator->update(deltaTime);

  // Ground detection and correction
  collRect = getHitboxRect();
  if (collRect.h == 0) {
    collRect = animator->getCurrentFrameRect();
  }
  int charBottom = static_cast<int>(mover.position.y) + collRect.h;

  // Ground collision check and correction
  if (charBottom >= GROUND_LEVEL) {
    // Snap to ground
    mover.position.y = GROUND_LEVEL - collRect.h;
    mover.velocity.y = 0;
    onGround = true;
    groundFrames++;
  } else if (charBottom < GROUND_LEVEL - GROUND_THRESHOLD) {
    onGround = false;
    groundFrames = 0;
  }

  FramePhase currentPhase = animator->getCurrentFramePhase();

  // Animation state update
  if (currentPhase == FramePhase::Active ||
      currentPhase == FramePhase::Startup) {
    // Don't interrupt attack animations
    return;
  }

  // Don't transition to idle/walk during recovery unless overridden by a new
  // attack
  if (currentPhase == FramePhase::Recovery &&
      !animator->isAnimationFinished()) {
    return;
  }

  bool shouldBeIdle = !isMoving || (std::abs(mover.velocity.x) < 2.0f &&
                                    std::abs(mover.velocity.y) < 2.0f);

  if (shouldBeIdle) {
    if (animator->getCurrentAnimationKey() != "Idle") {
      animator->play("Idle");
    }
  } else if (isMoving && animator->getCurrentAnimationKey() != "Walk") {
    animator->play("Walk");
  }
}

void Character::render(SDL_Renderer *renderer, float cameraScale) {
  animator->render(renderer, static_cast<int>(mover.position.x),
                   static_cast<int>(mover.position.y), cameraScale);
  SDL_Rect collRect = getHitboxRect();
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
  SDL_Rect myRect = getHitboxRect();
  SDL_Rect targetRect = target.getHitboxRect();
  int myCenterX = myRect.x + myRect.w / 2;
  int targetCenterX = targetRect.x + targetRect.w / 2;
  animator->setFlip(targetCenterX < myCenterX);
  int forwardDirection = (targetCenterX > myCenterX) ? 1 : -1;
  if (isMoving) {
    if (inputDirection == forwardDirection) {
      animator->setReverse(false);
      Logger::debug("Playing walk animation normally (forward).");
    } else {
      animator->setReverse(true);
      Logger::debug("Playing walk animation in reverse (backward).");
    }
  } else {
    animator->setReverse(false);
  }
}
