#include "Character.hpp"
#include "Core/Input.hpp"
#include "Core/Logger.hpp"
#include "Data/Animation.hpp"
#include <SDL.h>
#include <algorithm>

Character::Character(Animator *anim)
    : animator(anim), health(100), maxHealth(100), onGround(false),
      isMoving(false), groundFrames(0), inputDirection(0) {}

SDL_Rect Character::getHitboxRect(HitboxType type) const {
  const auto &hitboxes = animator->getCurrentHitboxes();
  SDL_Rect collisionRect;
  bool foundCollisionBox = false;
  for (const auto &hb : hitboxes) {
    if (!hb.enabled || hb.type != type)
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
    collisionRect = {0, 0, 0, 0};
    Logger::debug("No collision hitbox found; using full empty rect.");
  }
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
  if (Input::isKeyDown(SDL_SCANCODE_SPACE) && onGround &&
      groundFrames >= STABLE_GROUND_FRAMES) {
    jump();
  }
}

void Character::attack() {
  FramePhase phase = animator->getCurrentFramePhase();
  if (phase == FramePhase::Recovery) {
    auto currentAnimationKey = animator->getCurrentAnimationKey();
    if (currentAnimationKey == "Attack") {
      animator->play("Attack 2");
      Logger::debug("Combo x2, Launching second attack !!");
    } else if (currentAnimationKey == "Attack 2") {
      Logger::debug("Combo x2, Launching second attack !!!");
      animator->play("Attack 3");
    }
  } else {
    animator->play("Attack");
  }
  Logger::debug("Attack initiated.");
}

void Character::jump() {
  groundFrames = 0;
  mover.velocity.y = -500.0f;
  onGround = false;
  Logger::debug("Jump initiated.");
}

void Character::applyDamage(int damage, bool survive) {
  health -= damage;
  if (health < 0)
    health = survive ? 1 : 0;
  Logger::debug("Damage applied: " + std::to_string(damage) +
                ". Health now: " + std::to_string(health));
}

void Character::update(float deltaTime) {
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

  if (animator->getCurrentFramePhase() == FramePhase::Active ||
      animator->getCurrentFramePhase() == FramePhase::Startup) {
    // Block input during attack phases.
  } else {
    if (!isMoving) {
      if (animator->isAnimationFinished()) {
        animator->play("Idle");
      }
    } else {
      animator->play("Walk");
    }
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
